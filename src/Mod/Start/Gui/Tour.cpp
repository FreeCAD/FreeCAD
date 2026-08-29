// SPDX-License-Identifier: LGPL-2.1-or-later
/****************************************************************************
 *                                                                          *
 *   Copyright (c) 2024 The FreeCAD Project Association AISBL               *
 *                                                                          *
 *   This file is part of FreeCAD.                                          *
 *                                                                          *
 *   FreeCAD is free software: you can redistribute it and/or modify it     *
 *   under the terms of the GNU Lesser General Public License as            *
 *   published by the Free Software Foundation, either version 2.1 of the   *
 *   License, or (at your option) any later version.                        *
 *                                                                          *
 *   FreeCAD is distributed in the hope that it will be useful, but         *
 *   WITHOUT ANY WARRANTY; without even the implied warranty of             *
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU       *
 *   Lesser General Public License for more details.                        *
 *                                                                          *
 *   You should have received a copy of the GNU Lesser General Public       *
 *   License along with FreeCAD. If not, see                                *
 *   <https://www.gnu.org/licenses/>.                                       *
 *                                                                          *
 ***************************************************************************/

#include "Tour.h"

#include "TourStops.h"

#include <algorithm>
#include <array>
#include <cmath>

#include <QDockWidget>
#include <QDesktopServices>
#include <QEvent>
#include <QFrame>
#include <QHBoxLayout>
#include <QIcon>
#include <QLabel>
#include <QListWidget>
#include <QMainWindow>
#include <QPainter>
#include <QPainterPath>
#include <QPointer>
#include <QPushButton>
#include <QResizeEvent>
#include <QStyledItemDelegate>
#include <QTextLayout>
#include <QTimer>
#include <QToolBar>
#include <QVBoxLayout>
#include <QWidget>
#include <QUrl>

#include <Gui/Action.h>
#include <Gui/BitmapFactory.h>
#include <Gui/Application.h>
#include <Gui/Command.h>
#include <Gui/Document.h>
#include <Gui/WorkbenchSelector.h>

#include <Base/Interpreter.h>

namespace StartGui
{
namespace
{
constexpr double kLayoutMarginEm = 1.5;       // margin around the overlay and chapter list
constexpr double kLayoutLeftGapEm = 1.5;      // gap between the chapter list and the tooltip bubble
constexpr double kBubbleGapEm = 1.0;          // gap between a target widget and the tooltip bubble
constexpr double kChapterMinWidthEm = 10.5;   // keep the chapter list usable on narrower windows
constexpr double kChapterMaxWidthEm = 12.5;   // chapter list width cap
constexpr double kChapterMinHeightEm = 3.75;  // minimum height for the chapter list
constexpr double kChapterFloorHeightEm = 11.25;  // absolute floor for max chapter list height
constexpr double kChapterMinTopEm = 5.5;         // minimum top offset for the chapter list
constexpr double kMinBubbleWidthEm = 16.0;       // minimum bubble width before it starts shrinking
constexpr double kMaxBubbleWidthEm = 26.0;       // maximum bubble width

// Some themes leave stale values for foreground roles, leaving unreadable text contrast
// Using known background to select contrasting text color instead of relying on theme.
// If there is a better way to do this, please implement
QColor textColorForBackground(const QColor& background)
{
    // Rec. 601 perceptual luminance weighting, sRGB channels ~linear enough for this threshold.
    const qreal luminance
        = (0.299 * background.redF() + 0.587 * background.greenF() + 0.114 * background.blueF());
    return luminance > 0.5 ? QColor(Qt::black) : QColor(Qt::white);
}

class ChapterItemDelegate: public QStyledItemDelegate
{
public:
    explicit ChapterItemDelegate(QObject* parent = nullptr)
        : QStyledItemDelegate(parent)
    {}

    void paint(QPainter* painter, const QStyleOptionViewItem& option, const QModelIndex& index) const override
    {
        QStyleOptionViewItem opt(option);
        initStyleOption(&opt, index);
        painter->save();

        opt.text.clear();
        if (opt.widget != nullptr) {
            opt.widget->style()->drawControl(QStyle::CE_ItemViewItem, &opt, painter, opt.widget);
        }

        // Derive text color to contrast current background
        const QColor color = opt.state.testFlag(QStyle::State_Selected)
            ? textColorForBackground(opt.palette.color(QPalette::Highlight))
            : textColorForBackground(opt.palette.color(QPalette::Base));
        painter->setPen(color);

        const int indent = index.data(Qt::UserRole).toBool() ? 14 : 0;
        const QRect textRect = opt.rect.adjusted(10 + indent, 6, -10, -6);
        painter->setClipRect(opt.rect);
        layoutText(
            index.data(Qt::DisplayRole).toString(),
            opt.font,
            textRect.width(),
            [&](QTextLayout& layout) {
                layout.draw(painter, QPointF(textRect.left(), textRect.top()));
            }
        );

        painter->restore();
    }

    QSize sizeHint(const QStyleOptionViewItem& option, const QModelIndex& index) const override
    {
        QStyleOptionViewItem opt(option);
        initStyleOption(&opt, index);
        return QSize(
            opt.rect.width(),
            rowHeight(
                index.data(Qt::DisplayRole).toString(),
                opt.font,
                index.data(Qt::UserRole).toBool(),
                opt.rect.width()
            )
        );
    }

    static int rowHeight(const QString& text, const QFont& font, bool indented, int rowWidth)
    {
        const int availableWidth = std::max(1, rowWidth - 2 * 10 - (indented ? 14 : 0));
        qreal textHeight = 0;
        layoutText(text, font, availableWidth, [&](QTextLayout& layout) {
            textHeight = layout.boundingRect().height();
        });
        return static_cast<int>(std::ceil(textHeight)) + 2 * 6;
    }

private:
    template<typename Callback>
    static void layoutText(const QString& text, const QFont& font, int width, Callback&& callback)
    {
        QTextLayout layout(text, font);
        QTextOption textOption(Qt::AlignLeft | Qt::AlignVCenter);
        textOption.setWrapMode(QTextOption::WordWrap);
        layout.setTextOption(textOption);
        layout.beginLayout();
        qreal y = 0;
        while (true) {
            QTextLine line = layout.createLine();
            if (!line.isValid()) {
                break;
            }
            line.setLineWidth(width);
            line.setPosition(QPointF(0, y));
            y += line.height();
        }
        layout.endLayout();
        callback(layout);
    }
};

}  // namespace

class TourOverlay: public QWidget
{
    Q_OBJECT

public:
    explicit TourOverlay(QMainWindow* mainWindow);

protected:
    bool eventFilter(QObject* watched, QEvent* event) override;
    void resizeEvent(QResizeEvent* event) override;
    void showEvent(QShowEvent* event) override;
    void paintEvent(QPaintEvent*) override;
    void changeEvent(QEvent* event) override;

private:
    struct DockFloatState
    {
        QPointer<QDockWidget> dock;
        bool wasFloating = false;
        QDockWidget::DockWidgetFeatures features;
    };

    // UI construction and event handling.
    void buildUi();
    void applyThemeColors();

    // Layout, target lookup and geometry helpers.
    void applyLayout();
    int emUnit() const;
    QRect resolveTargetRect(const TourStop& stop) const;
    QPoint bubblePosition(
        const QRect& targetRect,
        const QSize& bubbleSize,
        int leftLimit,
        int rightLimit,
        int topLimit,
        int bottomLimit,
        int noTargetTop,
        bool rtl
    ) const;
    QWidget* commandWidget(const QString& commandName) const;
    QRect commandStripRect(const QStringList& commandNames) const;
    QToolBar* firstToolBar() const;
    QToolBar* partDesignToolBar() const;
    QWidget* workbenchSelector() const;
    QDockWidget* findDock(const QStringList& objectNames) const;

    // Tour flow and teardown.
    void showStop(int index);
    void advance();
    void closeTour();
    void createSketchOnXYPlane() const;
    bool isEditingSketch() const;

    QMainWindow* _mainWindow;
    QFrame* _bubble = nullptr;
    QLabel* _headline = nullptr;
    QLabel* _body = nullptr;
    QPushButton* _nextButton = nullptr;
    QPushButton* _readMoreButton = nullptr;
    QListWidget* _chapters = nullptr;
    QList<TourStop> _stops;
    QRect _targetRect;
    int _index = 0;
    QList<DockFloatState> _dockFloatStates;
    bool _wasOverlayTransparent = false;
};

TourOverlay::TourOverlay(QMainWindow* mainWindow)
    : QWidget(mainWindow)
    , _mainWindow(mainWindow)
{
    buildUi();
    setGeometry(_mainWindow->rect());
    _mainWindow->installEventFilter(this);

    for (auto dock : _mainWindow->findChildren<QDockWidget*>()) {
        _dockFloatStates.append({dock, dock->isFloating(), dock->features()});
        dock->setFloating(false);
        dock->setFeatures(dock->features() & ~QDockWidget::DockWidgetFloatable);
    }
    if (auto* overlayCommand = Gui::Application::Instance->commandManager().getCommandByName(
            "Std_DockOverlayToggleTransparent"
        )) {
        if (auto* overlayAction = overlayCommand->getAction()) {
            _wasOverlayTransparent = overlayAction->isChecked();
            if (_wasOverlayTransparent) {
                overlayCommand->invoke(0);
            }
        }
    }

    showStop(0);

    // Ensure the overlay is drawn above all other widgets
    QTimer::singleShot(0, this, [this]() {
        raise();
        update();
        QTimer::singleShot(0, this, [this]() {
            raise();
            update();
        });
    });
}

// UI construction and event handling.
void TourOverlay::buildUi()
{
    setAttribute(Qt::WA_NoSystemBackground);
    setAttribute(Qt::WA_TranslucentBackground);

    _bubble = new QFrame(this);
    _bubble->setObjectName(QStringLiteral("TourBubbleFrame"));

    auto bubbleLayout = new QVBoxLayout(_bubble);
    bubbleLayout->setContentsMargins(16, 14, 16, 14);
    bubbleLayout->setSpacing(10);

    auto topRow = new QHBoxLayout();
    topRow->setSpacing(8);
    topRow->setContentsMargins(0, 0, 0, 0);

    auto mascot = new QLabel();
    mascot->setFixedSize(26, 26);
    mascot->setAlignment(Qt::AlignCenter);
    const auto cloneIcon = QIcon(Gui::BitmapFactory().pixmap("PartDesign_Clone"));
    mascot->setPixmap(cloneIcon.pixmap(QSize(26, 26)));

    _headline = new QLabel();
    _headline->setStyleSheet(QStringLiteral("font-weight: bold; font-size: 13px;"));
    _headline->setContentsMargins(0, 0, 0, 0);
    _headline->setSizePolicy(QSizePolicy::Preferred, QSizePolicy::Fixed);

    topRow->addWidget(mascot);
    topRow->addWidget(_headline);
    topRow->addStretch();
    bubbleLayout->addLayout(topRow);

    _body = new QLabel();
    _body->setWordWrap(true);
    _body->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Preferred);
    _body->setTextFormat(Qt::RichText);
    _body->setContentsMargins(0, 0, 0, 0);
    bubbleLayout->addWidget(_body);

    auto buttonRow = new QHBoxLayout();
    auto skipButton = new QPushButton(tr("Skip tour"));
    connect(skipButton, &QPushButton::clicked, this, &TourOverlay::closeTour);

    auto readMoreButton = new QPushButton(tr("Read more"));
    _readMoreButton = readMoreButton;
    _readMoreButton->hide();
    connect(readMoreButton, &QPushButton::clicked, this, []() {
        QDesktopServices::openUrl(QUrl(QStringLiteral("https://wiki.freecad.org/Getting_started")));
    });

    _nextButton = new QPushButton(tr("Next"));
    connect(_nextButton, &QPushButton::clicked, this, &TourOverlay::advance);

    buttonRow->addWidget(skipButton);
    buttonRow->addWidget(readMoreButton);
    buttonRow->addStretch();
    buttonRow->addWidget(_nextButton);
    bubbleLayout->addLayout(buttonRow);

    _chapters = new QListWidget(this);
    _chapters->setCursor(Qt::PointingHandCursor);
    _chapters->setSpacing(0);
    _chapters->setVerticalScrollBarPolicy(Qt::ScrollBarAsNeeded);
    _chapters->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    _chapters->setTextElideMode(Qt::ElideNone);
    _chapters->setWordWrap(true);
    _chapters->setResizeMode(QListView::Adjust);
    _chapters->setUniformItemSizes(false);
    _chapters->setItemDelegate(new ChapterItemDelegate(_chapters));
    connect(_chapters, &QListWidget::currentRowChanged, this, &TourOverlay::showStop);

    _stops = buildStops(_mainWindow);
    _stops.front().description = tr(
        "Welcome to your project. This short tour will take just a few minutes to familiarize "
        "you with FreeCAD's document tree, workbenches, and the sketch and modeling commands "
        "you'll use most."
    );
    for (const auto& stop : _stops) {
        auto chapterItem = new QListWidgetItem(
            stop.isSubchapter ? QStringLiteral("\u2014 %1").arg(stop.chapterLabel) : stop.chapterLabel
        );
        chapterItem->setData(Qt::UserRole, stop.isSubchapter);
        _chapters->addItem(chapterItem);
    }

    applyThemeColors();
}

// Builds the bubble/chapter-list stylesheets from _mainWindow's palette (this widget is
// translucent, so its own palette isn't reliable). Re-invoked on palette/style changes.
void TourOverlay::applyThemeColors()
{
    const QPalette pal = _mainWindow->palette();

    const QColor bubbleBg = pal.color(QPalette::Window);
    const QColor bubbleText = textColorForBackground(bubbleBg);
    const QColor bubbleBorder = pal.color(QPalette::Mid);
    const QColor accent = pal.color(QPalette::Highlight);
    const QColor accentText = textColorForBackground(accent);
    const QColor accentHover = accent.lighter(115);
    const QColor accentBorder = accent.darker(130);

    _bubble->setStyleSheet(
        QStringLiteral(
            "QFrame#TourBubbleFrame { background: %1; border: 1px solid %2; border-radius: 8px; }"
        )
            .arg(bubbleBg.name(), bubbleBorder.name())
        + QStringLiteral("QLabel { color: %1; }").arg(bubbleText.name())
        + QStringLiteral(
              "QPushButton { background: %1; color: %2; border: 1px solid %3; border-radius: 4px; "
              "padding: 6px 12px; }"
        )
              .arg(accent.name(), accentText.name(), accentBorder.name())
        + QStringLiteral("QPushButton:hover { background: %1; }").arg(accentHover.name())
    );

    const QColor chapterBg = pal.color(QPalette::Base);
    const QColor chapterText = textColorForBackground(chapterBg);
    const QColor chapterBorder = pal.color(QPalette::Mid);
    const QColor chapterHoverBg = pal.color(QPalette::AlternateBase);

    _chapters->setStyleSheet(
        QStringLiteral(
            "QListWidget { background: %1; color: %2; border: 1px solid %3; border-radius: 10px; "
            "padding: 4px; }"
        )
            .arg(chapterBg.name(), chapterText.name(), chapterBorder.name())
        + QStringLiteral(
              "QListWidget::item { background: %1; border: none; border-radius: 0; margin: 0; "
              "border-bottom: 1px solid %2; }"
        )
              .arg(chapterBg.name(), chapterBorder.name())
        + QStringLiteral("QListWidget::item:hover { background: %1; }").arg(chapterHoverBg.name())
        + QStringLiteral(
              "QListWidget::item:selected { background: %1; color: %2; border-bottom-color: %1; }"
        )
              .arg(accent.name(), accentText.name())
    );

    update();
}

void TourOverlay::changeEvent(QEvent* event)
{
    QWidget::changeEvent(event);
    if (_bubble != nullptr && _chapters != nullptr
        && (event->type() == QEvent::PaletteChange || event->type() == QEvent::StyleChange)) {
        applyThemeColors();
    }
}

bool TourOverlay::eventFilter(QObject* watched, QEvent* event)
{
    if (watched == _mainWindow && event->type() == QEvent::Resize) {
        setGeometry(_mainWindow->rect());
        showStop(_index);
    }
    return QWidget::eventFilter(watched, event);
}

void TourOverlay::resizeEvent(QResizeEvent* event)
{
    QWidget::resizeEvent(event);
    setGeometry(_mainWindow->rect());
    showStop(_index);
}

void TourOverlay::showEvent(QShowEvent* event)
{
    QWidget::showEvent(event);
    setGeometry(_mainWindow->rect());
    raise();
    update();
}

void TourOverlay::paintEvent(QPaintEvent*)
{
    QPainter painter(this);
    painter.setRenderHint(QPainter::Antialiasing);

    QPainterPath path;
    path.addRect(QRectF(rect()));
    if (!_targetRect.isNull() && _mainWindow->rect().intersects(_targetRect)) {
        QPainterPath hole;
        hole.addRoundedRect(QRectF(_targetRect).adjusted(-6, -6, 6, 6), 8, 8);
        path = path.subtracted(hole);
    }
    painter.fillPath(path, QColor(0, 0, 0, 150));
    if (!_targetRect.isNull() && _mainWindow->rect().intersects(_targetRect)) {
        painter.setPen(QPen(palette().color(QPalette::Highlight), 2));
        painter.drawRoundedRect(QRectF(_targetRect).adjusted(-6, -6, 6, 6), 8, 8);
    }
}

// Layout, target lookup and geometry helpers.
QWidget* TourOverlay::commandWidget(const QString& commandName) const
{
    QWidget* fallback = nullptr;
    for (auto toolbar : _mainWindow->findChildren<QToolBar*>()) {
        for (auto action : toolbar->actions()) {
            if (action->objectName() != commandName) {
                continue;
            }
            auto widget = toolbar->widgetForAction(action);
            if (widget != nullptr && widget->isVisible()) {
                return widget;
            }
            if (fallback == nullptr) {
                fallback = widget;
            }
        }
    }
    return fallback;
}

QRect TourOverlay::commandStripRect(const QStringList& commandNames) const
{
    QRect strip;
    for (const auto& commandName : commandNames) {
        auto widget = commandWidget(commandName);
        if (widget == nullptr) {
            continue;
        }
        const auto topLeft = widget->mapTo(_mainWindow, QPoint(0, 0));
        const QRect widgetRect(topLeft, widget->size());
        strip = strip.isNull() ? widgetRect : strip.united(widgetRect);
    }
    return strip;
}

QDockWidget* TourOverlay::findDock(const QStringList& objectNames) const
{
    for (auto dock : _mainWindow->findChildren<QDockWidget*>()) {
        if (objectNames.contains(dock->objectName())) {
            return dock;
        }
    }
    return nullptr;
}

QToolBar* TourOverlay::firstToolBar() const
{
    const auto toolBars = _mainWindow->findChildren<QToolBar*>();
    return toolBars.empty() ? nullptr : toolBars.front();
}

QToolBar* TourOverlay::partDesignToolBar() const
{
    for (auto toolbar : _mainWindow->findChildren<QToolBar*>()) {
        for (auto action : toolbar->actions()) {
            if (action->objectName() == QStringLiteral("PartDesign_Pad")
                || action->objectName() == QStringLiteral("PartDesign_Pocket")) {
                return toolbar;
            }
        }
    }
    return firstToolBar();
}

QWidget* TourOverlay::workbenchSelector() const
{
    const auto selectors = _mainWindow->findChildren<Gui::WorkbenchComboBox*>();
    if (!selectors.empty()) {
        return selectors.front();
    }
    return _mainWindow->findChild<QWidget*>(QStringLiteral("WbTabBar"));
}

void TourOverlay::createSketchOnXYPlane() const
{
    try {
        Base::Interpreter().runStringObject(
            "exec(\"import FreeCAD as App, FreeCADGui as Gui\\n"
            "_body = next((obj for obj in App.ActiveDocument.Objects if obj.TypeId == "
            "'PartDesign::Body'), None) if App.ActiveDocument else None\\n"
            "if _body is not None and _body.Origin is not None:\\n"
            "    Gui.Selection.clearSelection()\\n"
            "    Gui.Selection.addSelection(_body.Origin.OriginFeatures[3])\\n"
            "    Gui.runCommand('PartDesign_NewSketch')\")"
        );
    }
    catch (Base::PyException& error) {
        error.reportException();
    }
}

bool TourOverlay::isEditingSketch() const
{
    auto doc = Gui::Application::Instance->activeDocument();
    return doc != nullptr && doc->getInEdit() != nullptr;
}

QRect TourOverlay::resolveTargetRect(const TourStop& stop) const
{
    if (!stop.highlight) {
        return QRect();
    }
    QRect targetRect = stop.commandNames.isEmpty() ? QRect() : commandStripRect(stop.commandNames);
    if (targetRect.isNull() && stop.widget != nullptr && stop.widget->isVisible()) {
        const auto topLeft = stop.widget->mapTo(_mainWindow, QPoint(0, 0));
        targetRect = QRect(topLeft, stop.widget->size());
    }
    if (stop.id == kNewSketchId) {
        auto* newSketchWidget = commandWidget(QStringLiteral("Sketcher_NewSketch"));
        const bool newSketchVisible = newSketchWidget != nullptr && newSketchWidget->isVisible();
        if (isEditingSketch() || !newSketchVisible) {
            targetRect = QRect();
        }
    }
    if (!targetRect.isNull() && !_mainWindow->rect().intersects(targetRect)) {
        targetRect = QRect();
    }
    return targetRect;
}

QPoint TourOverlay::bubblePosition(
    const QRect& targetRect,
    const QSize& bubbleSize,
    int leftLimit,
    int rightLimit,
    int topLimit,
    int bottomLimit,
    int noTargetTop,
    bool rtl
) const
{
    if (targetRect.isNull()) {
        const auto y
            = std::clamp(noTargetTop, topLimit, std::max(topLimit, bottomLimit - bubbleSize.height()));
        return QPoint(leftLimit, y);
    }

    const int bubbleGap = std::lround(kBubbleGapEm * emUnit());
    const QPoint besideNear(
        rtl ? targetRect.left() - bubbleSize.width() - bubbleGap : targetRect.right() + bubbleGap,
        targetRect.top()
    );
    const QPoint besideFar(
        rtl ? targetRect.right() + bubbleGap : targetRect.left() - bubbleSize.width() - bubbleGap,
        targetRect.top()
    );
    const QPoint below(targetRect.left(), targetRect.bottom() + bubbleGap);
    const QPoint above(targetRect.left(), targetRect.top() - bubbleSize.height() - bubbleGap);

    const std::array<QPoint, 4> candidates {besideNear, besideFar, below, above};
    auto overflow = [&](const QPoint& candidate) {
        const QRect candidateRect(candidate, bubbleSize);
        return std::max(0, leftLimit - candidateRect.left())
            + std::max(0, candidateRect.right() - rightLimit)
            + std::max(0, topLimit - candidateRect.top())
            + std::max(0, candidateRect.bottom() - bottomLimit);
    };
    for (const auto& candidate : candidates) {
        if (overflow(candidate) == 0) {
            return candidate;
        }
    }

    const QPoint* closest = &candidates.front();
    for (const auto& candidate : candidates) {
        if (overflow(candidate) < overflow(*closest)) {
            closest = &candidate;
        }
    }
    QPoint fallback = *closest;
    fallback.setX(
        std::clamp(fallback.x(), leftLimit, std::max(leftLimit, rightLimit - bubbleSize.width()))
    );
    fallback.setY(
        std::clamp(fallback.y(), topLimit, std::max(topLimit, bottomLimit - bubbleSize.height()))
    );
    return fallback;
}

int TourOverlay::emUnit() const
{
    return std::max(1, fontMetrics().height());
}

void TourOverlay::applyLayout()
{
    const int em = emUnit();
    const int layoutMargin = std::lround(kLayoutMarginEm * em);
    const int layoutLeftGap = std::lround(kLayoutLeftGapEm * em);
    const int chapterMinWidth = std::lround(kChapterMinWidthEm * em);
    const int chapterMaxWidth = std::lround(kChapterMaxWidthEm * em);
    const int chapterMinHeight = std::lround(kChapterMinHeightEm * em);
    const int chapterFloorHeight = std::lround(kChapterFloorHeightEm * em);
    const int chapterMinTop = std::lround(kChapterMinTopEm * em);
    const int minBubbleWidth = std::lround(kMinBubbleWidthEm * em);
    const int maxBubbleWidth = std::lround(kMaxBubbleWidthEm * em);

    const int chapterWidth = std::min(chapterMaxWidth, std::max(chapterMinWidth, width() / 6));
    const int chapterTop = std::max<int>(height() * 0.2, chapterMinTop);
    const auto maxChapterHeight = std::max(chapterFloorHeight, height() - chapterTop - layoutMargin);
    const auto rowWidth = chapterWidth - 2 * _chapters->frameWidth();

    int chapterContentHeight = 2 * _chapters->frameWidth();
    for (const auto& stop : _stops) {
        const auto label = stop.isSubchapter ? QStringLiteral("\u2014 %1").arg(stop.chapterLabel)
                                             : stop.chapterLabel;
        chapterContentHeight
            += ChapterItemDelegate::rowHeight(label, _chapters->font(), stop.isSubchapter, rowWidth);
    }

    const int chapterHeight = std::clamp(chapterContentHeight, chapterMinHeight, maxChapterHeight);
    const bool chapterScrollBarVisible = chapterContentHeight > chapterHeight;

    _chapters->setGeometry(layoutMargin, chapterTop, chapterWidth, chapterHeight);
    _chapters->setVerticalScrollBarPolicy(
        chapterScrollBarVisible ? Qt::ScrollBarAsNeeded : Qt::ScrollBarAlwaysOff
    );

    const int leftLimit = layoutMargin + chapterWidth + layoutLeftGap;
    const int rightLimit = width() - layoutMargin;
    const int topLimit = layoutMargin;
    const int bottomLimit = height() - layoutMargin;
    const bool rtl = layoutDirection() == Qt::RightToLeft;

    const auto availableBubbleWidth = std::max(0, rightLimit - leftLimit);
    const int bubbleWidth = availableBubbleWidth >= minBubbleWidth
        ? std::min(availableBubbleWidth, maxBubbleWidth)
        : std::max(1, availableBubbleWidth);

    _bubble->setFixedWidth(bubbleWidth);
    _bubble->setMaximumHeight(std::max(1, bottomLimit - topLimit));
    _bubble->layout()->invalidate();
    _bubble->layout()->activate();
    _bubble->adjustSize();
    _bubble->move(
        bubblePosition(_targetRect, _bubble->size(), leftLimit, rightLimit, topLimit, bottomLimit, chapterTop, rtl)
    );
}

// Tour flow and teardown.
void TourOverlay::showStop(int index)
{
    if (_stops.isEmpty() || index >= _stops.size()) {
        closeTour();
        return;
    }

    _index = index;
    const auto& stop = _stops.at(index);

    bool workbenchChanged = false;
    if (!stop.workbenchName.isEmpty()) {
        Gui::Application::Instance->activateWorkbench(stop.workbenchName.toUtf8().constData());
        workbenchChanged = true;
    }

    _targetRect = resolveTargetRect(stop);
    _headline->setText(stop.headline);
    _body->setText(stop.description);
    _readMoreButton->setVisible(stop.id == kReadMoreId);

    _chapters->blockSignals(true);
    _chapters->setCurrentRow(index);
    _chapters->blockSignals(false);
    _nextButton->setText(index == _stops.size() - 1 ? tr("Done") : tr("Next"));

    applyLayout();
    raise();
    update();

    if (workbenchChanged) {
        QTimer::singleShot(0, this, [this, index]() {
            if (_index != index) {
                return;
            }
            const auto& currentStop = _stops.at(index);
            _targetRect = resolveTargetRect(currentStop);
            applyLayout();
            raise();
            update();
        });
    }
}

void TourOverlay::advance()
{
    const auto& stop = _stops.at(_index);
    if (stop.onExit.testFlag(TourStopExitAction::CreateSketchOnXYPlane)) {
        createSketchOnXYPlane();
    }
    if (stop.onExit.testFlag(TourStopExitAction::LeaveSketchEditMode)) {
        if (auto doc = Gui::Application::Instance->activeDocument()) {
            doc->resetEdit();
        }
    }

    const auto nextIndex = _index + 1;
    if (nextIndex < _stops.size()) {
        const auto& nextStop = _stops.at(nextIndex);
        if (!nextStop.workbenchName.isEmpty()) {
            Gui::Application::Instance->activateWorkbench(nextStop.workbenchName.toUtf8().constData());
        }
    }

    showStop(nextIndex);
}

void TourOverlay::closeTour()
{
    for (const auto& state : _dockFloatStates) {
        if (state.dock) {
            state.dock->setFeatures(state.features);
            state.dock->setFloating(state.wasFloating);
        }
    }
    if (_wasOverlayTransparent) {
        if (auto* overlayCommand = Gui::Application::Instance->commandManager().getCommandByName(
                "Std_DockOverlayToggleTransparent"
            )) {
            overlayCommand->invoke(0);
        }
    }
    deleteLater();
}

void Tour::start(QMainWindow* mainWindow)
{
    if (mainWindow == nullptr) {
        return;
    }

    Gui::Application::Instance->commandManager().runCommandByName("Std_New");
    Gui::Application::Instance->activateWorkbench("PartDesignWorkbench");
    Gui::Application::Instance->commandManager().runCommandByName("PartDesign_Body");

    auto* tourOverlay = new TourOverlay(mainWindow);
    tourOverlay->show();
}

}  // namespace StartGui

#include "Tour.moc"
