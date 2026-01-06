/****************************************************************************
 *   Copyright (c) 2020 Zheng Lei (realthunder) <realthunder.dev@gmail.com> *
 *                                                                          *
 *   This file is part of the FreeCAD CAx development system.               *
 *                                                                          *
 *   This library is free software; you can redistribute it and/or          *
 *   modify it under the terms of the GNU Library General Public            *
 *   License as published by the Free Software Foundation; either           *
 *   version 2 of the License, or (at your option) any later version.       *
 *                                                                          *
 *   This library  is distributed in the hope that it will be useful,       *
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of         *
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the          *
 *   GNU Library General Public License for more details.                   *
 *                                                                          *
 *   You should have received a copy of the GNU Library General Public      *
 *   License along with this library; see the file COPYING.LIB. If not,     *
 *   write to the Free Software Foundation, Inc., 59 Temple Place,          *
 *   Suite 330, Boston, MA  02111-1307, USA                                 *
 *                                                                          *
 ****************************************************************************/


#include <QAction>
#include <QApplication>
#include <QBoxLayout>
#include <QComboBox>
#include <QDockWidget>
#include <QHeaderView>
#include <QKeyEvent>
#include <QMdiArea>
#include <QMenu>
#include <QPainter>
#include <QPointer>
#include <QSpacerItem>
#include <QSplitter>
#include <QTabBar>
#include <QTextStream>
#include <QTimerEvent>
#include <QToolTip>
#include <QTreeView>
#include <QScrollBar>


#include <QPainterPath>
#include <QPropertyAnimation>

#include <array>
#include <unordered_map>

#include "OverlayWidgets.h"

#include <Base/Tools.h>
#include <Base/Console.h>
#include <App/Application.h>
#include "Application.h"
#include "BitmapFactory.h"
#include "Clipping.h"
#include "ComboView.h"
#include "Command.h"
#include "Control.h"
#include "MainWindow.h"
#include "MDIView.h"
#include "NaviCube.h"
#include "OverlayManager.h"
#include "OverlayParams.h"
#include "TaskView/TaskView.h"
#include "Tree.h"
#include "TreeParams.h"
#include "propertyeditor/PropertyEditor.h"

FC_LOG_LEVEL_INIT("Dock", true, true);

using namespace Gui;

OverlayDragFrame* OverlayTabWidget::_DragFrame;
QDockWidget* OverlayTabWidget::_DragFloating;
QWidget* OverlayTabWidget::_Dragging;
OverlayTabWidget* OverlayTabWidget::_LeftOverlay;
OverlayTabWidget* OverlayTabWidget::_RightOverlay;
OverlayTabWidget* OverlayTabWidget::_TopOverlay;
OverlayTabWidget* OverlayTabWidget::_BottomOverlay;

static inline int widgetMinSize(const QWidget* widget, bool margin = false)
{
    return widget->fontMetrics().ascent() + widget->fontMetrics().descent() + (margin ? 4 : 0);
}

// -----------------------------------------------------------

OverlayProxyWidget::OverlayProxyWidget(OverlayTabWidget* tabOverlay)
    : QWidget(tabOverlay->parentWidget())
    , owner(tabOverlay)
    , _hintColor(QColor(50, 50, 50, 150))
{
    dockArea = owner->getDockArea();
    timer.setSingleShot(true);
    QObject::connect(&timer, &QTimer::timeout, this, &OverlayProxyWidget::onTimer);
    setAttribute(Qt::WA_TransparentForMouseEvents, true);
}

bool OverlayProxyWidget::isActivated() const
{
    return drawLine && isVisible();
}

OverlayProxyWidget::HitTest OverlayProxyWidget::hitTest(const QPoint& globalPt, bool delay)
{
    if (!isVisible() || !owner->count()) {
        return HitTest::HitNone;
    }

    auto pt = mapFromGlobal(globalPt);

    QTabBar* tabbar = owner->tabBar();
    if (tabbar->isVisible() && tabbar->tabAt(pt) >= 0) {
        ToolTip::showText(globalPt, QObject::tr("Press Esc to hide hint"), this);
        return HitTest::HitOuter;
    }

    HitTest hit = HitTest::HitNone;
    QRect rect = this->getRect();
    QSize s = this->size();
    int hintSize = OverlayParams::getDockOverlayHintTriggerSize();
    // if (owner->getState() == OverlayTabWidget::State::HintHidden)
    //     hintSize *= 2;
    switch (dockArea) {
        case Qt::LeftDockWidgetArea:
            if (pt.y() >= 0 && pt.y() <= s.height() && pt.x() > 0 && pt.x() < hintSize) {
                hit = HitTest::HitOuter;
            }
            break;
        case Qt::RightDockWidgetArea:
            if (pt.y() >= 0 && pt.y() <= s.height() && pt.x() < s.width() && pt.x() > -hintSize) {
                hit = HitTest::HitOuter;
            }
            break;
        case Qt::TopDockWidgetArea:
            if (pt.x() >= 0 && pt.x() <= s.width() && pt.y() > 0 && pt.y() < hintSize) {
                hit = HitTest::HitOuter;
            }
            break;
        case Qt::BottomDockWidgetArea:
            if (pt.x() >= 0 && pt.x() <= s.width() && pt.y() < s.height() && pt.y() > -hintSize) {
                hit = HitTest::HitOuter;
            }
            break;
    }
    if (rect.contains(pt)) {
        hit = HitTest::HitInner;
        ToolTip::showText(globalPt, QObject::tr("Press Esc to hide hint"), this);
    }
    else if (drawLine) {
        ToolTip::hideText();
    }

    if (owner->getState() == OverlayTabWidget::State::HintHidden) {
        if (hit == HitTest::HitNone) {
            owner->setState(OverlayTabWidget::State::Normal);
        }
        else {
            hit = HitTest::HitNone;
            ToolTip::hideText();
        }
    }
    if (hit != HitTest::HitNone) {
        if (drawLine) {
            timer.stop();
        }
        else if (delay) {
            if (!timer.isActive()) {
                timer.start(OverlayParams::getDockOverlayHintDelay());
            }
            return hit;
        }
        else {
            timer.stop();
            owner->setState(OverlayTabWidget::State::Hint);
            drawLine = true;
            update();
        }

        auto view = getMainWindow()->activeWindow();
        auto overlayOnHoverAllowed = view && view->onHasMsg("AllowsOverlayOnHover");

        if (owner->getState() != OverlayTabWidget::State::Hidden && hit == HitTest::HitOuter
            && overlayOnHoverAllowed && OverlayParams::getDockOverlayActivateOnHover()) {
            if (owner->isVisible() && owner->tabBar()->isVisible()) {
                QSize size = owner->tabBar()->size();
                QPoint pt = owner->tabBar()->mapToGlobal(QPoint(size.width(), size.height()));
                QPoint pos = QCursor::pos();
                switch (this->dockArea) {
                    case Qt::LeftDockWidgetArea:
                    case Qt::RightDockWidgetArea:
                        if (pos.y() < pt.y()) {
                            return HitTest::HitNone;
                        }
                        break;
                    case Qt::TopDockWidgetArea:
                    case Qt::BottomDockWidgetArea:
                        if (pos.x() < pt.x()) {
                            return HitTest::HitNone;
                        }
                        break;
                    default:
                        break;
                }
            }
            owner->setState(OverlayTabWidget::State::Showing);
        }
    }
    else if (!drawLine) {
        timer.stop();
    }
    else if (delay) {
        if (!timer.isActive()) {
            timer.start(OverlayParams::getDockOverlayHintDelay());
        }
    }
    else {
        timer.stop();
        owner->setState(OverlayTabWidget::State::Normal);
        drawLine = false;
        ToolTip::hideText();
        update();
    }
    return hit;
}

void OverlayProxyWidget::onTimer()
{
    hitTest(QCursor::pos(), false);
}

#if QT_VERSION < QT_VERSION_CHECK(6, 0, 0)
void OverlayProxyWidget::enterEvent(QEvent*)
#else
void OverlayProxyWidget::enterEvent(QEnterEvent*)
#endif
{
    if (!owner->count()) {
        return;
    }

    if (!drawLine) {
        if (!timer.isActive()) {
            timer.start(OverlayParams::getDockOverlayHintDelay());
        }
    }
}

void OverlayProxyWidget::hideEvent(QHideEvent*)
{
    drawLine = false;
}

void OverlayProxyWidget::onMousePress()
{
    if (!owner->count()) {
        return;
    }

    if (owner->getState() == OverlayTabWidget::State::HintHidden) {
        return;
    }

    owner->setState(OverlayTabWidget::State::Showing);
}

QBrush OverlayProxyWidget::hintColor() const
{
    return _hintColor;
}

void OverlayProxyWidget::setHintColor(const QBrush& brush)
{
    _hintColor = brush;
}

QRect OverlayProxyWidget::getRect() const
{
    QRect rect = this->rect();
    if (owner->isVisible() && owner->tabBar()->isVisible()) {
        QSize size = owner->tabBar()->size();
        QPoint pt = owner->tabBar()->mapToGlobal(QPoint(size.width(), size.height()));
        pt = this->mapFromGlobal(pt);
        switch (this->dockArea) {
            case Qt::LeftDockWidgetArea:
            case Qt::RightDockWidgetArea:
                rect.setTop(pt.y());
                break;
            case Qt::TopDockWidgetArea:
            case Qt::BottomDockWidgetArea:
                rect.setLeft(pt.x());
                break;
            default:
                break;
        }
    }
    switch (this->dockArea) {
        case Qt::LeftDockWidgetArea:
            if (int offset = OverlayParams::getDockOverlayHintLeftOffset()) {
                rect.moveTop(std::max(rect.top() + offset, rect.bottom() - 10));
            }
            if (int length = OverlayParams::getDockOverlayHintLeftLength()) {
                rect.setHeight(std::min(length, rect.height()));
            }
            break;
        case Qt::RightDockWidgetArea:
            if (int offset = OverlayParams::getDockOverlayHintRightOffset()) {
                rect.moveTop(std::max(rect.top() + offset, rect.bottom() - 10));
            }
            if (int length = OverlayParams::getDockOverlayHintRightLength()) {
                rect.setHeight(std::min(length, rect.height()));
            }
            break;
        case Qt::TopDockWidgetArea:
            if (int offset = OverlayParams::getDockOverlayHintTopOffset()) {
                rect.moveLeft(std::max(rect.left() + offset, rect.right() - 10));
            }
            if (int length = OverlayParams::getDockOverlayHintTopLength()) {
                rect.setWidth(std::min(length, rect.width()));
            }
            break;
        case Qt::BottomDockWidgetArea:
            if (int offset = OverlayParams::getDockOverlayHintBottomOffset()) {
                rect.moveLeft(std::max(rect.left() + offset, rect.right() - 10));
            }
            if (int length = OverlayParams::getDockOverlayHintBottomLength()) {
                rect.setWidth(std::min(length, rect.width()));
            }
            break;
        default:
            break;
    }
    return rect;
}

void OverlayProxyWidget::paintEvent(QPaintEvent*)
{
    if (!drawLine) {
        return;
    }
    QPainter painter(this);
    painter.setOpacity(_hintColor.color().alphaF());
    painter.setPen(Qt::transparent);
    painter.setBrush(_hintColor);

    QRect rect = this->getRect();
    painter.drawRect(rect);
}

OverlayToolButton::OverlayToolButton(QWidget* parent)
    : QToolButton(parent)
{
    setCursor(Qt::ArrowCursor);
}

// --------------------------------------------------------------------

OverlayTabWidget::OverlayTabWidget(QWidget* parent, Qt::DockWidgetArea pos)
    : QTabWidget(parent)
    , dockArea(pos)
{
    // This is necessary to capture any focus lost from switching the tab,
    // otherwise the lost focus will leak to the parent, i.e. MdiArea, which may
    // cause unexpected Mdi sub window switching.
    // setFocusPolicy(Qt::StrongFocus);

    _imageScale = 0.0;

    splitter = new OverlaySplitter(this);

    _graphicsEffect = new OverlayGraphicsEffect(splitter);
    splitter->setGraphicsEffect(_graphicsEffect);

    _graphicsEffectTab = new OverlayGraphicsEffect(this);
    _graphicsEffectTab->setEnabled(false);
    tabBar()->setGraphicsEffect(_graphicsEffectTab);

    Command* cmdHide = nullptr;
    switch (pos) {
        case Qt::LeftDockWidgetArea:
            _LeftOverlay = this;
            setTabPosition(QTabWidget::West);
            splitter->setOrientation(Qt::Vertical);
            cmdHide = Application::Instance->commandManager().getCommandByName(
                "Std_DockOverlayToggleLeft"
            );
            break;
        case Qt::RightDockWidgetArea:
            _RightOverlay = this;
            setTabPosition(QTabWidget::East);
            splitter->setOrientation(Qt::Vertical);
            cmdHide = Application::Instance->commandManager().getCommandByName(
                "Std_DockOverlayToggleRight"
            );
            break;
        case Qt::TopDockWidgetArea:
            _TopOverlay = this;
            setTabPosition(QTabWidget::North);
            splitter->setOrientation(Qt::Horizontal);
            cmdHide = Application::Instance->commandManager().getCommandByName(
                "Std_DockOverlayToggleTop"
            );
            break;
        case Qt::BottomDockWidgetArea:
            _BottomOverlay = this;
            setTabPosition(QTabWidget::South);
            splitter->setOrientation(Qt::Horizontal);
            cmdHide = Application::Instance->commandManager().getCommandByName(
                "Std_DockOverlayToggleBottom"
            );
            break;
        default:
            break;
    }

    proxyWidget = new OverlayProxyWidget(this);
    proxyWidget->hide();
    _setOverlayMode(proxyWidget, OverlayOption::Enable);

    setOverlayMode(true);
    hide();

    actTransparent.setCheckable(true);
    actTransparent.setData(QStringLiteral("OBTN Transparent"));
    actTransparent.setParent(this);
    addAction(&actTransparent);

    actAutoHide.setData(QStringLiteral("OBTN AutoHide"));

    actEditHide.setData(QStringLiteral("OBTN EditHide"));

    actEditShow.setData(QStringLiteral("OBTN EditShow"));

    actTaskShow.setData(QStringLiteral("OBTN TaskShow"));

    actNoAutoMode.setData(QStringLiteral("OBTN NoAutoMode"));

    actAutoMode.setData(QStringLiteral("OBTN AutoMode"));
    actAutoMode.setParent(this);
    autoModeMenu.hide();
    autoModeMenu.setToolTipsVisible(true);
    autoModeMenu.addAction(&actNoAutoMode);
    autoModeMenu.addAction(&actAutoHide);
    autoModeMenu.addAction(&actEditShow);
    autoModeMenu.addAction(&actEditHide);
    autoModeMenu.addAction(&actTaskShow);
    addAction(&actAutoMode);

    actOverlay.setData(QStringLiteral("OBTN Overlay"));
    actOverlay.setParent(this);
    addAction(&actOverlay);

    if (cmdHide) {
        cmdHide->addTo(this);
    }

    retranslate();
    refreshIcons();

    connect(tabBar(), &QTabBar::tabBarClicked, this, &OverlayTabWidget::onCurrentChanged);
    connect(tabBar(), &QTabBar::tabMoved, this, &OverlayTabWidget::onTabMoved);
    tabBar()->installEventFilter(this);

    timer.setSingleShot(true);
    connect(&timer, &QTimer::timeout, this, &OverlayTabWidget::setupLayout);

    repaintTimer.setSingleShot(true);
    connect(&repaintTimer, &QTimer::timeout, this, &OverlayTabWidget::onRepaint);

    _animator = new QPropertyAnimation(this, "animation", this);
    _animator->setStartValue(0.0);
    _animator->setEndValue(1.0);
    connect(_animator, &QAbstractAnimation::stateChanged, this, &OverlayTabWidget::onAnimationStateChanged);
}

void OverlayTabWidget::refreshIcons()
{
    auto curStyleSheet = App::GetApplication()
                             .GetParameterGroupByPath("User parameter:BaseApp/Preferences/MainWindow")
                             ->GetASCII("StyleSheet", "None");

    QPixmap pxAutoHide;

    if (isStyleSheetDark(curStyleSheet)) {
        actOverlay.setIcon(BitmapFactory().pixmap("qss:overlay/icons/overlay_light.svg"));
        actNoAutoMode.setIcon(BitmapFactory().pixmap("qss:overlay/icons/mode_light.svg"));
        actTaskShow.setIcon(BitmapFactory().pixmap("qss:overlay/icons/taskshow_light.svg"));
        actEditShow.setIcon(BitmapFactory().pixmap("qss:overlay/icons/editshow_light.svg"));
        actEditHide.setIcon(BitmapFactory().pixmap("qss:overlay/icons/edithide_light.svg"));
        actTransparent.setIcon(BitmapFactory().pixmap("qss:overlay/icons/transparent_light.svg"));
        pxAutoHide = BitmapFactory().pixmap("qss:overlay/icons/autohide_light.svg");
    }
    else {
        actOverlay.setIcon(BitmapFactory().pixmap("qss:overlay/icons/overlay.svg"));
        actNoAutoMode.setIcon(BitmapFactory().pixmap("qss:overlay/icons/mode.svg"));
        actTaskShow.setIcon(BitmapFactory().pixmap("qss:overlay/icons/taskshow.svg"));
        actEditShow.setIcon(BitmapFactory().pixmap("qss:overlay/icons/editshow.svg"));
        actEditHide.setIcon(BitmapFactory().pixmap("qss:overlay/icons/edithide.svg"));
        actTransparent.setIcon(BitmapFactory().pixmap("qss:overlay/icons/transparent.svg"));
        pxAutoHide = BitmapFactory().pixmap("qss:overlay/icons/autohide.svg");
    }

    actAutoHide.setIcon(rotateAutoHideIcon(pxAutoHide, dockArea));

    syncAutoMode();
}

void OverlayTabWidget::onAnimationStateChanged()
{
    if (_animator->state() != QAbstractAnimation::Running) {
        setAnimation(0);
        if (_animator->startValue().toReal() == 0.0) {
            hide();
            OverlayManager::instance()->refresh();
        }
        if (_state == State::Showing) {
            setState(State::Normal);
        }
    }
}

void OverlayTabWidget::setAnimation(qreal t)
{
    if (t != _animation) {
        _animation = t;
        setupLayout();
    }
}

void OverlayTabWidget::startShow()
{
    if (isVisible() || _state > State::Normal) {
        return;
    }

    int duration = OverlayParams::getDockOverlayAnimationDuration();
    bool setmode = _state != State::Showing;
    if (duration) {
        _animator->setStartValue(1.0);
        _animator->setEndValue(0.0);
        _animator->setDuration(duration);
        _animator->setEasingCurve((QEasingCurve::Type)OverlayParams::getDockOverlayAnimationCurve());
        _animator->start();
    }
    else if (_state == State::Showing) {
        setState(State::Normal);
    }
    proxyWidget->hide();
    show();
    raise();
    if (setmode) {
        setOverlayMode(overlaid);
    }
}

QWidget* OverlayTabWidget::createTitleButton(QAction* action, int size)
{
    auto button = new OverlayToolButton(nullptr);
    button->setObjectName(action->data().toString());
    button->setDefaultAction(action);
    button->setAutoRaise(true);
    button->setContentsMargins(0, 0, 0, 0);
    button->setFixedSize(size, size);
    return button;
}

void OverlayTabWidget::startHide()
{
    if (!isVisible() || _state > State::Normal
        || (_animator->state() == QAbstractAnimation::Running
            && _animator->startValue().toReal() == 0.0)) {
        return;
    }
    int duration = OverlayParams::getDockOverlayAnimationDuration();
    if (!duration) {
        hide();
    }
    else {
        _animator->setStartValue(0.0);
        _animator->setEndValue(1.0);
        _animator->setDuration(duration);
        _animator->setEasingCurve((QEasingCurve::Type)OverlayParams::getDockOverlayAnimationCurve());
        _animator->start();
    }
}

bool OverlayTabWidget::event(QEvent* ev)
{
    switch (ev->type()) {
        case QEvent::MouseButtonRelease:
            if (mouseGrabber() == this) {
                releaseMouse();
                ev->accept();
                return true;
            }
            break;
        case QEvent::MouseMove:
        case QEvent::ContextMenu:
            if (QApplication::mouseButtons() == Qt::NoButton && mouseGrabber() == this) {
                releaseMouse();
                ev->accept();
                return true;
            }
            break;
        case QEvent::MouseButtonPress:
            ev->accept();
            return true;
        default:
            break;
    }
    return QTabWidget::event(ev);
}

int OverlayTabWidget::testAlpha(const QPoint& _pos, int radiusScale)
{
    if (!count() || (!isOverlaid() && !isTransparent()) || !isVisible()) {
        return -1;
    }

    if (tabBar()->isVisible() && tabBar()->tabAt(tabBar()->mapFromGlobal(_pos)) >= 0) {
        return -1;
    }

    if (titleBar->isVisible() && titleBar->rect().contains(titleBar->mapFromGlobal(_pos))) {
        return -1;
    }

    if (!splitter->isVisible()) {
        return 0;
    }

    auto pos = splitter->mapFromGlobal(_pos);
    QSize size = splitter->size();
    if (pos.x() < 0 || pos.y() < 0 || pos.x() >= size.width() || pos.y() >= size.height()) {
        if (this->rect().contains(this->mapFromGlobal(_pos))) {
            return 0;
        }
        return -1;
    }

    if (_image.isNull()) {
        auto pixmap = splitter->grab();
        _imageScale = pixmap.devicePixelRatio();
        _image = pixmap.toImage();
    }

    int res = qAlpha(_image.pixel(pos * _imageScale));
    int radius = OverlayParams::getDockOverlayAlphaRadius() * radiusScale;
    if (res || radius <= 0) {
        return res;
    }

    radius *= _imageScale;
    for (int i = -radius; i < radius; ++i) {
        for (int j = -radius; j < radius; ++j) {
            if (pos.x() + i < 0 || pos.y() + j < 0 || pos.x() + i >= size.width()
                || pos.y() + j >= size.height()) {
                continue;
            }
            res = qAlpha(_image.pixel(pos * _imageScale + QPoint(i, j)));
            if (res) {
                return res;
            }
        }
    }
    return 0;
}

void OverlayTabWidget::paintEvent(QPaintEvent* ev)
{
    Base::StateLocker guard(repainting);
    repaintTimer.stop();
    if (!_image.isNull()) {
        _image = QImage();
    }
    QTabWidget::paintEvent(ev);
}

void OverlayTabWidget::onRepaint()
{
    Base::StateLocker guard(repainting);
    repaintTimer.stop();
    if (!_image.isNull()) {
        _image = QImage();
    }
    splitter->repaint();
}

void OverlayTabWidget::scheduleRepaint()
{
    if (!repainting && isVisible() && _graphicsEffect) {
        repaintTimer.start(100);
    }
}

QColor OverlayTabWidget::effectColor() const
{
    return _graphicsEffect->color();
}

void OverlayTabWidget::setEffectColor(const QColor& color)
{
    _graphicsEffect->setColor(color);
    _graphicsEffectTab->setColor(color);
}

int OverlayTabWidget::effectWidth() const
{
    return _graphicsEffect->size().width();
}

void OverlayTabWidget::setEffectWidth(int s)
{
    auto size = _graphicsEffect->size();
    size.setWidth(s);
    _graphicsEffect->setSize(size);
    _graphicsEffectTab->setSize(size);
}

int OverlayTabWidget::effectHeight() const
{
    return _graphicsEffect->size().height();
}

void OverlayTabWidget::setEffectHeight(int s)
{
    auto size = _graphicsEffect->size();
    size.setHeight(s);
    _graphicsEffect->setSize(size);
    _graphicsEffectTab->setSize(size);
}

qreal OverlayTabWidget::effectOffsetX() const
{
    return _graphicsEffect->offset().x();
}

void OverlayTabWidget::setEffectOffsetX(qreal d)
{
    auto offset = _graphicsEffect->offset();
    offset.setX(d);
    _graphicsEffect->setOffset(offset);
    _graphicsEffectTab->setOffset(offset);
}

qreal OverlayTabWidget::effectOffsetY() const
{
    return _graphicsEffect->offset().y();
}

void OverlayTabWidget::setEffectOffsetY(qreal d)
{
    auto offset = _graphicsEffect->offset();
    offset.setY(d);
    _graphicsEffect->setOffset(offset);
    _graphicsEffectTab->setOffset(offset);
}

qreal OverlayTabWidget::effectBlurRadius() const
{
    return _graphicsEffect->blurRadius();
}

void OverlayTabWidget::setEffectBlurRadius(qreal r)
{
    _graphicsEffect->setBlurRadius(r);
    _graphicsEffectTab->setBlurRadius(r);
}

bool OverlayTabWidget::effectEnabled() const
{
    return _effectEnabled;
}

void OverlayTabWidget::setEffectEnabled(bool enable)
{
    _effectEnabled = enable;
}

bool OverlayTabWidget::eventFilter(QObject* o, QEvent* ev)
{
    if (ev->type() == QEvent::Resize && o == tabBar()) {
        if (_state <= State::Normal) {
            timer.start(10);
        }
    }
    return QTabWidget::eventFilter(o, ev);
}

void OverlayTabWidget::restore(ParameterGrp::handle handle)
{
    if (!handle) {
        hGrp = handle;
        return;
    }

    if (!parentWidget()) {
        return;
    }

    // If overlay was ever used and disabled by the user it should respect that choice
    if (handle->GetInt("Width", 0) != 0 || handle->GetInt("Height", 0) != 0) {
        // save current value with old default to prevent layout change
        handle->SetASCII("Widgets", handle->GetASCII("Widgets", ""));
    }

    std::string widgets
        = handle->GetASCII("Widgets", getDockArea() == Qt::RightDockWidgetArea ? "Tasks," : "");

    for (auto& name : QString::fromUtf8(widgets.c_str()).split(QLatin1Char(','))) {
        if (name.isEmpty()) {
            continue;
        }
        OverlayManager::instance()->registerDockWidget(name, this);
        auto dock = getMainWindow()->findChild<QDockWidget*>(name);
        if (dock) {
            addWidget(dock, dock->windowTitle());
        }
    }

    QSize minimumSizeHint = parentWidget()->minimumSizeHint();

    int width = handle->GetInt("Width", minimumSizeHint.width());
    int height = handle->GetInt("Height", minimumSizeHint.height());
    int offset1 = handle->GetInt("Offset1", 0);
    int offset2 = handle->GetInt("Offset3", 0);
    setOffset(QSize(offset1, offset2));
    setSizeDelta(handle->GetInt("Offset2", 0));

    // Special handling for broken state in #24963.
    //
    // This basically is due to how OverlayTabWidget::setRect is implemented. If it faces width (or
    // height) of 0 it forces the width to be minimumOverlayWidth * 3, which in the default config
    // appears to be 90. 90 is obviously way too small of a value to display any widget in the side
    // panel, so we need to basically need to treat anything smaller or equal to that as an
    // incorrect value for width. We use here 100 just to be safe.
    //
    // For the height value of 100 may be reasonable, so we leave it as is.
    if (width <= 100) {  // NOLINT(*-avoid-magic-numbers)
        width = minimumSizeHint.width();
    }

    if (width || height) {
        QRect rect(0, 0, width > 0 ? width : this->width(), height > 0 ? height : this->height());
        switch (dockArea) {
            case Qt::RightDockWidgetArea:
                rect.moveRight(parentWidget()->size().width());
                break;
            case Qt::BottomDockWidgetArea:
                rect.moveBottom(parentWidget()->size().height());
                break;
            default:
                break;
        }

        setRect(rect);
    }
    if (handle->GetBool("AutoHide", false)) {
        setAutoMode(AutoMode::AutoHide);
    }
    else if (handle->GetBool("EditHide", false)) {
        setAutoMode(AutoMode::EditHide);
    }
    else if (handle->GetBool("EditShow", false)) {
        setAutoMode(AutoMode::EditShow);
    }
    else if (handle->GetBool("TaskShow", getDockArea() == Qt::RightDockWidgetArea)) {
        setAutoMode(AutoMode::TaskShow);
    }
    else {
        setAutoMode(AutoMode::NoAutoMode);
    }

    setTransparent(handle->GetBool("Transparent", getDockArea() == Qt::RightDockWidgetArea));

    _sizemap.clear();
    std::string savedSizes = handle->GetASCII("Sizes", "");
    QList<int> sizes;
    int idx = 0;
    for (auto& size : QString::fromUtf8(savedSizes.c_str()).split(QLatin1Char(','))) {
        sizes.append(size.toInt());
        _sizemap[dockWidget(idx++)] = sizes.back();
    }

    FC_LOG("restore " << objectName().toUtf8().constData() << " " << savedSizes);

    getSplitter()->setSizes(sizes);
    hGrp = handle;

    // if updated save the state into settings to preserve layout in case of defaults change
    saveTabs();
}

void OverlayTabWidget::saveTabs()
{
    if (!hGrp) {
        return;
    }

    std::ostringstream os, os2;
    _sizemap.clear();
    auto sizes = splitter->sizes();
    bool first = true;
    for (int i = 0, c = splitter->count(); i < c; ++i) {
        auto dock = dockWidget(i);
        if (!dock) {
            continue;
        }
        if (dock->objectName().size()) {
            os << dock->objectName().toUtf8().constData() << ",";
            if (first) {
                first = false;
            }
            else {
                os2 << ",";
            }
            os2 << sizes[i];
        }
        _sizemap[dock] = sizes[i];
    }
    Base::StateLocker lock(_saving);
    hGrp->SetASCII("Widgets", os.str().c_str());
    hGrp->SetASCII("Sizes", os2.str().c_str());
    FC_LOG("save " << objectName().toUtf8().constData() << " " << os2.str());
}

void OverlayTabWidget::onTabMoved(int from, int to)
{
    QWidget* w = splitter->widget(from);
    splitter->insertWidget(to, w);
    saveTabs();
}

void OverlayTabWidget::setTitleBar(QWidget* w)
{
    titleBar = w;
}

void OverlayTabWidget::changeEvent(QEvent* e)
{
    QTabWidget::changeEvent(e);
    if (e->type() == QEvent::LanguageChange) {
        retranslate();
    }
}

void OverlayTabWidget::retranslate()
{
    actTransparent.setToolTip(tr("Toggle transparent mode"));
    actNoAutoMode.setText(tr("None"));
    actNoAutoMode.setToolTip(tr("Turn off auto hide/show"));
    actAutoHide.setText(tr("Auto hide"));
    actAutoHide.setToolTip(tr("Auto hide docked widgets on leave"));
    actEditHide.setText(tr("Hide on edit"));
    actEditHide.setToolTip(tr("Auto hide docked widgets on editing"));
    actEditShow.setText(tr("Show on edit"));
    actEditShow.setToolTip(tr("Auto show docked widgets on editing"));
    actTaskShow.setText(tr("Auto task"));
    actTaskShow.setToolTip(
        tr("Auto show task view for any current task, and hide the view when there is no task.")
    );
    actOverlay.setToolTip(tr("Toggle overlay"));
    syncAutoMode();
}

void OverlayTabWidget::syncAutoMode()
{
    auto curStyleSheet = App::GetApplication()
                             .GetParameterGroupByPath("User parameter:BaseApp/Preferences/MainWindow")
                             ->GetASCII("StyleSheet", "None");

    QAction* action = nullptr;
    switch (autoMode) {
        case AutoMode::AutoHide:
            action = &actAutoHide;
            if (isStyleSheetDark(curStyleSheet)) {
                QPixmap pxAutoHideMode = BitmapFactory().pixmap(
                    "qss:overlay/icons/autohide_lighter.svg"
                );
                pxAutoHideMode = rotateAutoHideIcon(pxAutoHideMode, dockArea);
                action->setIcon(pxAutoHideMode);
                actNoAutoMode.setIcon(BitmapFactory().pixmap("qss:overlay/icons/mode_light.svg"));
                actTaskShow.setIcon(BitmapFactory().pixmap("qss:overlay/icons/taskshow_light.svg"));
                actEditShow.setIcon(BitmapFactory().pixmap("qss:overlay/icons/editshow_light.svg"));
                actEditHide.setIcon(BitmapFactory().pixmap("qss:overlay/icons/edithide_light.svg"));
            }
            else {
                QPixmap pxAutoHideMode = BitmapFactory().pixmap("qss:overlay/icons/autohide.svg");
                pxAutoHideMode = rotateAutoHideIcon(pxAutoHideMode, dockArea);
                action->setIcon(pxAutoHideMode);
                actNoAutoMode.setIcon(BitmapFactory().pixmap("qss:overlay/icons/mode_lightgray.svg"));
                actTaskShow.setIcon(BitmapFactory().pixmap("qss:overlay/icons/taskshow_lightgray.svg"));
                actEditShow.setIcon(BitmapFactory().pixmap("qss:overlay/icons/editshow_lightgray.svg"));
                actEditHide.setIcon(BitmapFactory().pixmap("qss:overlay/icons/edithide_lightgray.svg"));
            }
            break;
        case AutoMode::EditShow:
            action = &actEditShow;
            if (isStyleSheetDark(curStyleSheet)) {
                QPixmap pxEditShowMode = BitmapFactory().pixmap(
                    "qss:overlay/icons/editshow_lighter.svg"
                );
                action->setIcon(pxEditShowMode);
                QPixmap pxAutoHideMode = BitmapFactory().pixmap("qss:overlay/icons/autohide_light.svg");
                pxAutoHideMode = rotateAutoHideIcon(pxAutoHideMode, dockArea);
                actAutoHide.setIcon(pxAutoHideMode);
                actNoAutoMode.setIcon(BitmapFactory().pixmap("qss:overlay/icons/mode_light.svg"));
                actTaskShow.setIcon(BitmapFactory().pixmap("qss:overlay/icons/taskshow_light.svg"));
                actEditHide.setIcon(BitmapFactory().pixmap("qss:overlay/icons/edithide_light.svg"));
            }
            else {
                QPixmap pxEditShowMode = BitmapFactory().pixmap("qss:overlay/icons/editshow.svg");
                action->setIcon(pxEditShowMode);
                QPixmap pxAutoHideMode = BitmapFactory().pixmap(
                    "qss:overlay/icons/autohide_lightgray.svg"
                );
                pxAutoHideMode = rotateAutoHideIcon(pxAutoHideMode, dockArea);
                actAutoHide.setIcon(pxAutoHideMode);
                actNoAutoMode.setIcon(BitmapFactory().pixmap("qss:overlay/icons/mode_lightgray.svg"));
                actTaskShow.setIcon(BitmapFactory().pixmap("qss:overlay/icons/taskshow_lightgray.svg"));
                actEditHide.setIcon(BitmapFactory().pixmap("qss:overlay/icons/edithide_lightgray.svg"));
            }
            break;
        case AutoMode::TaskShow:
            action = &actTaskShow;
            if (isStyleSheetDark(curStyleSheet)) {
                QPixmap pxTaskShowMode = BitmapFactory().pixmap(
                    "qss:overlay/icons/taskshow_lighter.svg"
                );
                action->setIcon(pxTaskShowMode);
                QPixmap pxAutoHideMode = BitmapFactory().pixmap("qss:overlay/icons/autohide_light.svg");
                pxAutoHideMode = rotateAutoHideIcon(pxAutoHideMode, dockArea);
                actNoAutoMode.setIcon(BitmapFactory().pixmap("qss:overlay/icons/mode_light.svg"));
                actEditShow.setIcon(BitmapFactory().pixmap("qss:overlay/icons/editshow_light.svg"));
                actAutoHide.setIcon(pxAutoHideMode);
                actEditHide.setIcon(BitmapFactory().pixmap("qss:overlay/icons/edithide_light.svg"));
            }
            else {
                QPixmap pxTaskShowMode = BitmapFactory().pixmap("qss:overlay/icons/taskshow.svg");
                action->setIcon(pxTaskShowMode);
                QPixmap pxAutoHideMode = BitmapFactory().pixmap(
                    "qss:overlay/icons/autohide_lightgray.svg"
                );
                pxAutoHideMode = rotateAutoHideIcon(pxAutoHideMode, dockArea);
                actNoAutoMode.setIcon(BitmapFactory().pixmap("qss:overlay/icons/mode_lightgray.svg"));
                actEditShow.setIcon(BitmapFactory().pixmap("qss:overlay/icons/editshow_lightgray.svg"));
                actAutoHide.setIcon(pxAutoHideMode);
                actEditHide.setIcon(BitmapFactory().pixmap("qss:overlay/icons/edithide_lightgray.svg"));
            }
            break;
        case AutoMode::EditHide:
            action = &actEditHide;
            if (isStyleSheetDark(curStyleSheet)) {
                QPixmap pxEditHideMode = BitmapFactory().pixmap(
                    "qss:overlay/icons/edithide_lighter.svg"
                );
                action->setIcon(pxEditHideMode);
                QPixmap pxAutoHideMode = BitmapFactory().pixmap("qss:overlay/icons/autohide_light.svg");
                pxAutoHideMode = rotateAutoHideIcon(pxAutoHideMode, dockArea);
                actNoAutoMode.setIcon(BitmapFactory().pixmap("qss:overlay/icons/mode_light.svg"));
                actEditShow.setIcon(BitmapFactory().pixmap("qss:overlay/icons/editshow_light.svg"));
                actAutoHide.setIcon(pxAutoHideMode);
                actTaskShow.setIcon(BitmapFactory().pixmap("qss:overlay/icons/taskshow_light.svg"));
            }
            else {
                QPixmap pxEditHideMode = BitmapFactory().pixmap("qss:overlay/icons/edithide.svg");
                action->setIcon(pxEditHideMode);
                QPixmap pxAutoHideMode = BitmapFactory().pixmap(
                    "qss:overlay/icons/autohide_lightgray.svg"
                );
                pxAutoHideMode = rotateAutoHideIcon(pxAutoHideMode, dockArea);
                actNoAutoMode.setIcon(BitmapFactory().pixmap("qss:overlay/icons/mode_lightgray.svg"));
                actEditShow.setIcon(BitmapFactory().pixmap("qss:overlay/icons/editshow_lightgray.svg"));
                actAutoHide.setIcon(pxAutoHideMode);
                actTaskShow.setIcon(BitmapFactory().pixmap("qss:overlay/icons/taskshow_lightgray.svg"));
            }
            break;
        default:
            action = &actNoAutoMode;
            if (isStyleSheetDark(curStyleSheet)) {
                QPixmap pxNoAutoMode = BitmapFactory().pixmap("qss:overlay/icons/mode_lighter.svg");
                action->setIcon(pxNoAutoMode);
                QPixmap pxAutoHideMode = BitmapFactory().pixmap("qss:overlay/icons/autohide_light.svg");
                pxAutoHideMode = rotateAutoHideIcon(pxAutoHideMode, dockArea);
                actTaskShow.setIcon(BitmapFactory().pixmap("qss:overlay/icons/taskshow_light.svg"));
                actEditShow.setIcon(BitmapFactory().pixmap("qss:overlay/icons/editshow_light.svg"));
                actAutoHide.setIcon(pxAutoHideMode);
                actEditHide.setIcon(BitmapFactory().pixmap("qss:overlay/icons/edithide_light.svg"));
            }
            else {
                QPixmap pxNoAutoMode = BitmapFactory().pixmap("qss:overlay/icons/mode.svg");
                action->setIcon(pxNoAutoMode);
                QPixmap pxAutoHideMode = BitmapFactory().pixmap(
                    "qss:overlay/icons/autohide_lightgray.svg"
                );
                pxAutoHideMode = rotateAutoHideIcon(pxAutoHideMode, dockArea);
                actTaskShow.setIcon(BitmapFactory().pixmap("qss:overlay/icons/taskshow_lightgray.svg"));
                actEditShow.setIcon(BitmapFactory().pixmap("qss:overlay/icons/editshow_lightgray.svg"));
                actAutoHide.setIcon(pxAutoHideMode);
                actEditHide.setIcon(BitmapFactory().pixmap("qss:overlay/icons/edithide_lightgray.svg"));
            }
            break;
    }
    actAutoMode.setIcon(action->icon());
    if (action == &actNoAutoMode) {
        actAutoMode.setToolTip(tr("Select auto show/hide mode"));
    }
    else {
        actAutoMode.setToolTip(action->toolTip());
    }
}

void OverlayTabWidget::onAction(QAction* action)
{
    if (action == &actAutoMode) {
        action = autoModeMenu.exec(QCursor::pos());
        if (action == &actNoAutoMode) {
            setAutoMode(AutoMode::NoAutoMode);
        }
        else if (action == &actAutoHide) {
            setAutoMode(AutoMode::AutoHide);
        }
        else if (action == &actEditShow) {
            setAutoMode(AutoMode::EditShow);
        }
        else if (action == &actTaskShow) {
            setAutoMode(AutoMode::TaskShow);
        }
        else if (action == &actEditHide) {
            setAutoMode(AutoMode::EditHide);
        }
        return;
    }
    else if (action == &actOverlay) {
        OverlayManager::instance()->setOverlayMode(OverlayManager::OverlayMode::ToggleActive);
        return;
    }
    else if (action == &actTransparent) {
        if (hGrp) {
            Base::StateLocker lock(_saving);
            hGrp->SetBool("Transparent", actTransparent.isChecked());
        }
    }
    OverlayManager::instance()->refresh(this);
}

void OverlayTabWidget::setState(State state)
{
    if (_state == state) {
        return;
    }
    switch (state) {
        case State::Normal:
            if (_state == State::Hidden) {
                // Only unhide through State::Showing, not State::Normal
                return;
            }
            else if (_state == State::Showing) {
                _state = state;
                return;
            }
            // fall through
        case State::Showing:
            _state = state;
            hide();
            if (dockArea == Qt::RightDockWidgetArea) {
                setTabPosition(East);
            }
            else if (dockArea == Qt::BottomDockWidgetArea) {
                setTabPosition(South);
            }
            if (this->count() == 1) {
                tabBar()->hide();
            }
            _graphicsEffectTab->setEnabled(false);
            titleBar->show();
            splitter->show();
            if (state == State::Showing) {
                OverlayManager::instance()->refresh(this);
            }
            break;
        case State::Hint:
            if (_state == State::HintHidden || _state == State::Hidden) {
                break;
            }
            _state = state;
            if (this->count() && OverlayParams::getDockOverlayHintTabBar()) {
                tabBar()->setToolTip(proxyWidget->toolTip());
                tabBar()->show();
                titleBar->hide();
                splitter->hide();
                _graphicsEffectTab->setEnabled(true);
                show();
                raise();
                proxyWidget->raise();
                if (dockArea == Qt::RightDockWidgetArea) {
                    setTabPosition(West);
                }
                else if (dockArea == Qt::BottomDockWidgetArea) {
                    setTabPosition(North);
                }
                OverlayManager::instance()->refresh(this);
            }
            break;
        case State::HintHidden:
            if (_state != State::Hidden) {
                _state = state;
            }
            proxyWidget->hide();
            hide();
            _graphicsEffectTab->setEnabled(true);
            break;
        case State::Hidden:
            startHide();
            _state = state;
            break;
        default:
            break;
    }
}

bool OverlayTabWidget::checkAutoHide() const
{
    if (autoMode == AutoMode::AutoHide) {
        return true;
    }

    if (OverlayParams::getDockOverlayAutoView()) {
        auto view = getMainWindow()->activeWindow();

        if (!view) {
            return true;
        }

        if (!view->onHasMsg("AllowsOverlayOnHover")) {
            return true;
        }

        if (!view->onHasMsg("CanPan") && view->parentWidget() && view->parentWidget()->isMaximized()) {
            return true;
        }
    }

    if (autoMode == AutoMode::EditShow) {
        return !Application::Instance->editDocument()
            && (!Control().taskPanel() || Control().taskPanel()->isEmpty(false));
    }

    if (autoMode == AutoMode::EditHide && Application::Instance->editDocument()) {
        return true;
    }

    return false;
}

void OverlayTabWidget::leaveEvent(QEvent*)
{
    if (titleBar && QWidget::mouseGrabber() == titleBar) {
        return;
    }
    OverlayManager::instance()->refresh();
}

#if QT_VERSION < QT_VERSION_CHECK(6, 0, 0)
void OverlayTabWidget::enterEvent(QEvent*)
#else
void OverlayTabWidget::enterEvent(QEnterEvent*)
#endif
{
    revealTime = QTime();
    OverlayManager::instance()->refresh();
}

void OverlayTabWidget::setRevealTime(const QTime& time)
{
    revealTime = time;
}

void OverlayTabWidget::_setOverlayMode(QWidget* widget, OverlayOption option)
{
    if (!widget) {
        return;
    }

    if (qobject_cast<QScrollBar*>(widget)) {
        auto parent = widget->parentWidget();
        if (parent) {
            parent = parent->parentWidget();
            if (qobject_cast<PropertyEditor::PropertyEditor*>(parent)) {
                auto scrollArea = static_cast<QAbstractScrollArea*>(parent);
                if (scrollArea->verticalScrollBar() == widget) {
                    if (!OverlayParams::getDockOverlayHidePropertyViewScrollBar()
                        || option == OverlayOption::Disable) {
                        widget->setStyleSheet(QString());
                    }
                    else {
                        static QString _style = QStringLiteral("*{width:0}");
                        widget->setStyleSheet(_style);
                    }
                }
            }
            auto treeView = qobject_cast<TreeWidget*>(parent);
            if (treeView) {
                auto scrollArea = static_cast<QAbstractScrollArea*>(parent);
                if (scrollArea->verticalScrollBar() == widget) {
                    if (!TreeParams::getHideScrollBar() || option == OverlayOption::Disable) {
                        widget->setStyleSheet(QString());
                    }
                    else {
                        static QString _style = QStringLiteral("*{width:0}");
                        widget->setStyleSheet(_style);
                    }
                }
            }

            if (treeView) {
                auto header = treeView->header();
                if (!TreeParams::getHideHeaderView() || option == OverlayOption::Disable) {
                    header->setStyleSheet(QString());
                }
                else {
                    static QString _style = QStringLiteral(
                        "QHeaderView:section {"
                        "height: 0px;"
                        "background-color: transparent;"
                        "padding: 0px;"
                        "border: transparent;}"
                    );
                    header->setStyleSheet(_style);
                }
            }
        }
    }

    auto tabbar = qobject_cast<QTabBar*>(widget);
    if (tabbar) {
        if (!tabbar->autoHide() || tabbar->count() > 1) {
            if (!OverlayManager::instance()->getHideTab()) {
                tabbar->setVisible(true);
            }
            else {
                tabbar->setVisible(
                    option == OverlayOption::Disable
                    || (option == OverlayOption::ShowTab && tabbar->count() > 1)
                );
            }
            return;
        }
    }

    if (!qobject_cast<QScrollArea*>(widget)
        || !qobject_cast<Dialog::Clipping*>(widget->parentWidget())) {
        if (option != OverlayOption::Disable) {
            widget->setWindowFlags(widget->windowFlags() | Qt::FramelessWindowHint);
        }
        else {
            widget->setWindowFlags(widget->windowFlags() & ~Qt::FramelessWindowHint);
        }
        widget->setAttribute(Qt::WA_NoSystemBackground, option != OverlayOption::Disable);
        widget->setAttribute(Qt::WA_TranslucentBackground, option != OverlayOption::Disable);
    }
}

void OverlayTabWidget::setOverlayMode(QWidget* widget, OverlayOption option)
{
    if (!widget || (qobject_cast<QDialog*>(widget) && !qobject_cast<Dialog::Clipping*>(widget))
        || qobject_cast<TaskView::TaskBox*>(widget)) {
        return;
    }
    if (widget != tabBar()) {
        if (OverlayParams::getDockOverlayAutoMouseThrough() && option == OverlayOption::ShowTab) {
            widget->setMouseTracking(true);
        }
    }

    _setOverlayMode(widget, option);

    if (qobject_cast<QComboBox*>(widget)) {
        // do not set child QAbstractItemView of QComboBox, otherwise the drop down box
        // won't be shown
        return;
    }
    for (auto child : widget->children()) {
        setOverlayMode(qobject_cast<QWidget*>(child), option);
    }
}

void OverlayTabWidget::setTransparent(bool enable)
{
    if (actTransparent.isChecked() == enable) {
        return;
    }
    if (hGrp) {
        Base::StateLocker lock(_saving);
        hGrp->SetBool("Transparent", enable);
    }
    actTransparent.setChecked(enable);
    OverlayManager::instance()->refresh(this);
}

bool OverlayTabWidget::isTransparent() const
{
    if (!actTransparent.isChecked()) {
        return false;
    }
    if (OverlayParams::getDockOverlayAutoView()) {
        auto view = getMainWindow()->activeWindow();
        if (!view) {
            return false;
        }
        if (!view->onHasMsg("CanPan") && view->parentWidget() && view->parentWidget()->isMaximized()) {
            return false;
        }
    }
    return true;
}

bool OverlayTabWidget::isOverlaid(QueryOption option) const
{
    if (option != QueryOption::QueryOverlay && currentTransparent != isTransparent()) {
        return option == QueryOption::TransparencyChanged;
    }
    return overlaid;
}

void OverlayTabWidget::setAutoMode(AutoMode mode)
{
    if (autoMode == mode) {
        return;
    }
    autoMode = mode;

    if (hGrp) {
        bool autohide = false, editshow = false, edithide = false, taskshow = false;
        switch (mode) {
            case AutoMode::AutoHide:
                autohide = true;
                break;
            case AutoMode::EditShow:
                editshow = true;
                break;
            case AutoMode::EditHide:
                edithide = true;
                break;
            case AutoMode::TaskShow:
                taskshow = true;
                break;
            default:
                break;
        }
        Base::StateLocker lock(_saving);
        hGrp->SetBool("AutoHide", autohide);
        hGrp->SetBool("EditShow", editshow);
        hGrp->SetBool("EditHide", edithide);
        hGrp->SetBool("TaskShow", taskshow);
    }
    syncAutoMode();
    OverlayManager::instance()->refresh(this);
}

QDockWidget* OverlayTabWidget::currentDockWidget() const
{
    int index = -1;
    for (int size : splitter->sizes()) {
        ++index;
        if (size > 0) {
            return dockWidget(index);
        }
    }
    return dockWidget(currentIndex());
}

QDockWidget* OverlayTabWidget::dockWidget(int index) const
{
    if (index < 0 || index >= splitter->count()) {
        return nullptr;
    }
    return qobject_cast<QDockWidget*>(splitter->widget(index));
}

void OverlayTabWidget::updateSplitterHandles()
{
    if (overlaid || _state > State::Normal) {
        return;
    }
    for (int i = 0, c = splitter->count(); i < c; ++i) {
        auto handle = qobject_cast<OverlaySplitterHandle*>(splitter->handle(i));
        if (handle) {
            handle->showTitle(true);
        }
    }
}

bool OverlayTabWidget::onEscape()
{
    if (getState() == OverlayTabWidget::State::Hint || getState() == OverlayTabWidget::State::Hidden) {
        setState(OverlayTabWidget::State::HintHidden);
        return true;
    }
    if (!isVisible()) {
        return false;
    }
    if (titleBar->isVisible() && titleBar->underMouse()) {
        titleBar->hide();
        return true;
    }
    for (int i = 0, c = splitter->count(); i < c; ++i) {
        auto handle = qobject_cast<OverlaySplitterHandle*>(splitter->handle(i));
        if (handle->isVisible() && handle->underMouse()) {
            handle->showTitle(false);
            return true;
        }
    }
    return false;
}

void OverlayTabWidget::setOverlayMode(bool enable)
{
    overlaid = enable;

    if (!isVisible() || !count()) {
        return;
    }

    touched = false;

    if (_state <= State::Normal) {
        titleBar->setVisible(!enable || OverlayManager::instance()->isMouseTransparent());
        for (int i = 0, c = splitter->count(); i < c; ++i) {
            auto handle = qobject_cast<OverlaySplitterHandle*>(splitter->handle(i));
            if (handle) {
                handle->showTitle(!enable);
            }
        }
    }

    QString stylesheet;
    stylesheet = OverlayManager::instance()->getStyleSheet();
    currentTransparent = isTransparent();

    OverlayOption option;
    if (!enable && isTransparent()) {
        option = OverlayOption::ShowTab;
    }
    else if (enable && !isTransparent()
             && (autoMode == AutoMode::EditShow || autoMode == AutoMode::AutoHide)) {
        option = OverlayOption::Disable;
    }
    else {
        option = enable ? OverlayOption::Enable : OverlayOption::Disable;
    }
    setProperty("transparent", option != OverlayOption::Disable);

    proxyWidget->setStyleSheet(stylesheet);
    this->setStyleSheet(stylesheet);
    setOverlayMode(this, option);

    _graphicsEffect->setEnabled(effectEnabled() && (enable || isTransparent()));

    if (_state == State::Hint && OverlayParams::getDockOverlayHintTabBar()) {
        tabBar()->setToolTip(proxyWidget->toolTip());
        tabBar()->show();
    }
    else if (OverlayParams::getDockOverlayHideTabBar() || count() == 1) {
        tabBar()->hide();
    }
    else {
        tabBar()->setToolTip(QString());
        tabBar()->setVisible(!enable || !OverlayManager::instance()->getHideTab());
    }

    setRect(rectOverlay);
}

const QRect& OverlayTabWidget::getRect()
{
    return rectOverlay;
}

bool OverlayTabWidget::getAutoHideRect(QRect& rect) const
{
    rect = rectOverlay;
    int hintWidth = OverlayParams::getDockOverlayHintSize();
    switch (dockArea) {
        case Qt::LeftDockWidgetArea:
        case Qt::RightDockWidgetArea:
            if (_TopOverlay->isVisible() && _TopOverlay->_state <= State::Normal) {
                rect.setTop(std::max(rect.top(), _TopOverlay->rectOverlay.bottom()));
            }
            if (dockArea == Qt::RightDockWidgetArea) {
                rect.setLeft(rect.left() + std::max(rect.width() - hintWidth, 0));
            }
            else {
                rect.setRight(rect.right() - std::max(rect.width() - hintWidth, 0));
            }
            break;
        case Qt::TopDockWidgetArea:
        case Qt::BottomDockWidgetArea:
            if (_LeftOverlay->isVisible() && _LeftOverlay->_state <= State::Normal) {
                rect.setLeft(std::max(rect.left(), _LeftOverlay->rectOverlay.right()));
            }
            if (dockArea == Qt::TopDockWidgetArea) {
                rect.setBottom(rect.bottom() - std::max(rect.height() - hintWidth, 0));
            }
            else {
                rect.setTop(rect.top() + std::max(rect.height() - hintWidth, 0));
                if (_RightOverlay->isVisible() && _RightOverlay->_state <= State::Normal) {
                    QPoint offset = getMainWindow()->getMdiArea()->pos();
                    rect.setRight(std::min(rect.right(), _RightOverlay->x() - offset.x()));
                }
            }
            break;
        default:
            break;
    }
    return _state != State::Showing && overlaid && checkAutoHide();
}

void OverlayTabWidget::setOffset(const QSize& ofs)
{
    if (offset != ofs) {
        offset = ofs;
        if (hGrp) {
            Base::StateLocker lock(_saving);
            hGrp->SetInt("Offset1", ofs.width());
            hGrp->SetInt("Offset3", ofs.height());
        }
    }
}

void OverlayTabWidget::setSizeDelta(int delta)
{
    if (sizeDelta != delta) {
        if (hGrp) {
            Base::StateLocker lock(_saving);
            hGrp->SetInt("Offset2", delta);
        }
        sizeDelta = delta;
    }
}

void OverlayTabWidget::setRect(QRect rect)
{
    if (busy || !parentWidget() || !getMainWindow() || !getMainWindow()->getMdiArea()) {
        return;
    }

    if (rect.width() == 0) {
        rect.setWidth(OverlayParams::getDockOverlayMinimumSize() * 3);
    }
    if (rect.height() == 0) {
        rect.setHeight(OverlayParams::getDockOverlayMinimumSize() * 3);
    }

    switch (dockArea) {
        case Qt::LeftDockWidgetArea:
            rect.moveLeft(0);
            if (rect.width() < OverlayParams::getDockOverlayMinimumSize()) {
                rect.setWidth(OverlayParams::getDockOverlayMinimumSize());
            }
            break;
        case Qt::RightDockWidgetArea:
            if (rect.width() < OverlayParams::getDockOverlayMinimumSize()) {
                rect.setLeft(rect.right() - OverlayParams::getDockOverlayMinimumSize());
            }
            break;
        case Qt::TopDockWidgetArea:
            rect.moveTop(0);
            if (rect.height() < OverlayParams::getDockOverlayMinimumSize()) {
                rect.setHeight(OverlayParams::getDockOverlayMinimumSize());
            }
            break;
        case Qt::BottomDockWidgetArea:
            if (rect.height() < OverlayParams::getDockOverlayMinimumSize()) {
                rect.setTop(rect.bottom() - OverlayParams::getDockOverlayMinimumSize());
            }
            break;
        default:
            break;
    }

    if (hGrp && rect.size() != rectOverlay.size()) {
        Base::StateLocker lock(_saving);
        hGrp->SetInt("Width", rect.width());
        hGrp->SetInt("Height", rect.height());
    }
    rectOverlay = rect;

    QPoint offset = getMainWindow()->getMdiArea()->pos();

    if (getAutoHideRect(rect) || _state == State::Hint || _state == State::Hidden) {
        QRect rectHint = rect;
        if (_state != State::Hint && _state != State::Hidden) {
            startHide();
        }
        else if (count() && OverlayParams::getDockOverlayHintTabBar()) {
            switch (dockArea) {
                case Qt::LeftDockWidgetArea:
                case Qt::RightDockWidgetArea:
                    if (dockArea == Qt::LeftDockWidgetArea) {
                        rect.setWidth(tabBar()->width());
                    }
                    else {
                        rect.setLeft(rect.left() + rect.width() - tabBar()->width());
                    }
                    rect.setHeight(
                        std::max(rect.height(), tabBar()->y() + tabBar()->sizeHint().height() + 5)
                    );
                    break;
                case Qt::BottomDockWidgetArea:
                case Qt::TopDockWidgetArea:
                    if (dockArea == Qt::TopDockWidgetArea) {
                        rect.setHeight(tabBar()->height());
                    }
                    else {
                        rect.setTop(rect.top() + rect.height() - tabBar()->height());
                    }
                    rect.setWidth(
                        std::max(rect.width(), tabBar()->x() + tabBar()->sizeHint().width() + 5)
                    );
                    break;
                default:
                    break;
            }

            setGeometry(rect.translated(offset));
        }
        proxyWidget->setGeometry(rectHint.translated(offset));
        if (count()) {
            proxyWidget->show();
            proxyWidget->raise();
        }
        else {
            proxyWidget->hide();
        }
    }
    else {
        setGeometry(rectOverlay.translated(offset));

        for (int i = 0, count = splitter->count(); i < count; ++i) {
            splitter->widget(i)->show();
        }

        if (!isVisible() && count()) {
            proxyWidget->hide();
            startShow();
        }
    }
}

void OverlayTabWidget::addWidget(QDockWidget* dock, const QString& title)
{
    if (!getMainWindow() || !getMainWindow()->getMdiArea()) {
        return;
    }

    OverlayManager::instance()->registerDockWidget(dock->objectName(), this);

    OverlayManager::setFocusView();
    getMainWindow()->removeDockWidget(dock);

    auto titleWidget = dock->titleBarWidget();
    if (titleWidget && titleWidget->objectName() == QStringLiteral("OverlayTitle")) {
        // replace the title bar with an invisible widget to hide it. The
        // OverlayTabWidget uses its own title bar for all docks.
        auto w = new QWidget();
        w->setObjectName(QStringLiteral("OverlayTitle"));
        dock->setTitleBarWidget(w);
        w->hide();
        titleWidget->deleteLater();
    }

    dock->show();
    splitter->addWidget(dock);
    auto dummyWidget = new QWidget(this);
    addTab(dummyWidget, title);
    connect(dock, &QObject::destroyed, dummyWidget, &QObject::deleteLater);

    dock->setFeatures(dock->features() & ~QDockWidget::DockWidgetFloatable);
    if (count() == 1) {
        QRect rect = dock->geometry();
        QSize sizeMain = getMainWindow()->getMdiArea()->size();
        switch (dockArea) {
            case Qt::LeftDockWidgetArea:
            case Qt::RightDockWidgetArea:
                if (rect.width() > sizeMain.width() / 3) {
                    rect.setWidth(sizeMain.width() / 3);
                }
                break;
            case Qt::TopDockWidgetArea:
            case Qt::BottomDockWidgetArea:
                if (rect.height() > sizeMain.height() / 3) {
                    rect.setHeight(sizeMain.height() / 3);
                }
                break;
            default:
                break;
        }
        setRect(rect);
    }

    saveTabs();
}

int OverlayTabWidget::dockWidgetIndex(QDockWidget* dock) const
{
    return splitter->indexOf(dock);
}

void OverlayTabWidget::removeWidget(QDockWidget* dock, QDockWidget* lastDock)
{
    int index = dockWidgetIndex(dock);
    if (index < 0) {
        return;
    }

    OverlayManager::instance()->unregisterDockWidget(dock->objectName(), this);

    OverlayManager::setFocusView();
    dock->show();
    if (lastDock) {
        getMainWindow()->tabifyDockWidget(lastDock, dock);
    }
    else {
        getMainWindow()->addDockWidget(dockArea, dock);
    }

    auto w = this->widget(index);
    removeTab(index);
    w->deleteLater();

    if (!count()) {
        hide();
    }

    w = dock->titleBarWidget();
    if (w && w->objectName() == QStringLiteral("OverlayTitle")) {
        dock->setTitleBarWidget(nullptr);
        w->deleteLater();
    }
    OverlayManager::instance()->setupTitleBar(dock);

    dock->setFeatures(dock->features() | QDockWidget::DockWidgetFloatable);

    setOverlayMode(dock, OverlayOption::Disable);

    saveTabs();
}

void OverlayTabWidget::resizeEvent(QResizeEvent* ev)
{
    QTabWidget::resizeEvent(ev);
    if (_state <= State::Normal) {
        timer.start(10);
    }
}

void OverlayTabWidget::setupLayout()
{
    if (_state > State::Normal) {
        return;
    }

    if (count() == 1) {
        tabSize = 0;
    }
    else {
        int tsize;
        if (dockArea == Qt::LeftDockWidgetArea || dockArea == Qt::RightDockWidgetArea) {
            tsize = tabBar()->width();
        }
        else {
            tsize = tabBar()->height();
        }
        tabSize = tsize;
    }
    int titleBarSize = widgetMinSize(this, true);
    QRect rect, rectTitle;
    switch (tabPosition()) {
        case West:
            rectTitle = QRect(tabSize, 0, this->width() - tabSize, titleBarSize);
            rect = QRect(
                rectTitle.left(),
                rectTitle.bottom(),
                rectTitle.width(),
                this->height() - rectTitle.height()
            );
            break;
        case East:
            rectTitle = QRect(0, 0, this->width() - tabSize, titleBarSize);
            rect = QRect(
                rectTitle.left(),
                rectTitle.bottom(),
                rectTitle.width(),
                this->height() - rectTitle.height()
            );
            break;
        case North:
            rectTitle = QRect(0, tabSize, titleBarSize, this->height() - tabSize);
            rect = QRect(
                rectTitle.right(),
                rectTitle.top(),
                this->width() - rectTitle.width(),
                rectTitle.height()
            );
            break;
        case South:
            rectTitle = QRect(0, 0, titleBarSize, this->height() - tabSize);
            rect = QRect(
                rectTitle.right(),
                rectTitle.top(),
                this->width() - rectTitle.width(),
                rectTitle.height()
            );
            break;
    }
    if (_animation != 0.0) {
        switch (dockArea) {
            case Qt::LeftDockWidgetArea:
                rect.moveLeft(rect.left() - _animation * rect.width());
                break;
            case Qt::RightDockWidgetArea:
                rect.moveLeft(rect.left() + _animation * rect.width());
                break;
            case Qt::TopDockWidgetArea:
                rect.moveTop(rect.top() - _animation * rect.height());
                break;
            case Qt::BottomDockWidgetArea:
                rect.moveTop(rect.top() + _animation * rect.height());
                break;
            default:
                break;
        }
    }
    splitter->setGeometry(rect);
    titleBar->setGeometry(rectTitle);
}

void OverlayTabWidget::setCurrent(QDockWidget* widget)
{
    int index = dockWidgetIndex(widget);
    if (index >= 0) {
        setCurrentIndex(index);
    }
}

void OverlayTabWidget::onSplitterResize(int index)
{
    const auto& sizes = splitter->sizes();
    if (index >= 0 && index < sizes.count()) {
        if (sizes[index] == 0) {
            if (currentIndex() == index) {
                bool done = false;
                for (int i = index + 1; i < sizes.count(); ++i) {
                    if (sizes[i] > 0) {
                        setCurrentIndex(i);
                        done = true;
                        break;
                    }
                }
                if (!done) {
                    for (int i = index - 1; i >= 0; --i) {
                        if (sizes[i] > 0) {
                            setCurrentIndex(i);
                            break;
                        }
                    }
                }
            }
        }
        else {
            setCurrentIndex(index);
        }
    }

    saveTabs();
}

void OverlayTabWidget::onCurrentChanged(int index)
{
    setState(State::Showing);

    auto sizes = splitter->sizes();
    int i = 0;
    int size = splitter->orientation() == Qt::Vertical ? height() - tabBar()->height()
                                                       : width() - tabBar()->width();
    for (auto& s : sizes) {
        if (i++ == index) {
            s = size;
        }
        else {
            s = 0;
        }
    }
    splitter->setSizes(sizes);
    onSplitterResize(index);
    saveTabs();
}

void OverlayTabWidget::onSizeGripMove(const QPoint& p)
{
    if (!getMainWindow() || !getMainWindow()->getMdiArea()) {
        return;
    }

    QPoint pos = mapFromGlobal(p) + this->pos();
    QPoint offset = getMainWindow()->getMdiArea()->pos();
    QRect rect = this->rectOverlay.translated(offset);

    switch (dockArea) {
        case Qt::LeftDockWidgetArea:
            if (pos.x() - rect.left() < OverlayParams::getDockOverlayMinimumSize()) {
                return;
            }
            rect.setRight(pos.x());
            break;
        case Qt::RightDockWidgetArea:
            if (rect.right() - pos.x() < OverlayParams::getDockOverlayMinimumSize()) {
                return;
            }
            rect.setLeft(pos.x());
            break;
        case Qt::TopDockWidgetArea:
            if (pos.y() - rect.top() < OverlayParams::getDockOverlayMinimumSize()) {
                return;
            }
            rect.setBottom(pos.y());
            break;
        default:
            if (rect.bottom() - pos.y() < OverlayParams::getDockOverlayMinimumSize()) {
                return;
            }
            rect.setTop(pos.y());
            break;
    }
    this->setRect(rect.translated(-offset));
    OverlayManager::instance()->refresh();
}

QLayoutItem* OverlayTabWidget::prepareTitleWidget(QWidget* widget, const QList<QAction*>& actions)
{
    bool vertical = false;
    QBoxLayout* layout = nullptr;
    auto tabWidget = qobject_cast<OverlayTabWidget*>(widget->parentWidget());
    if (!tabWidget) {
        layout = new QBoxLayout(QBoxLayout::LeftToRight, widget);
    }
    else {
        switch (tabWidget->getDockArea()) {
            case Qt::LeftDockWidgetArea:
                layout = new QBoxLayout(QBoxLayout::LeftToRight, widget);
                break;
            case Qt::RightDockWidgetArea:
                layout = new QBoxLayout(QBoxLayout::RightToLeft, widget);
                break;
            case Qt::TopDockWidgetArea:
                layout = new QBoxLayout(QBoxLayout::TopToBottom, widget);
                vertical = true;
                break;
            case Qt::BottomDockWidgetArea:
                layout = new QBoxLayout(QBoxLayout::BottomToTop, widget);
                vertical = true;
                break;
            default:
                break;
        }
    }

    layout->addSpacing(5);
    layout->setContentsMargins(1, 1, 1, 1);
    int buttonSize = widgetMinSize(widget);
    auto spacer = new QSpacerItem(
        buttonSize,
        buttonSize,
        vertical ? QSizePolicy::Minimum : QSizePolicy::Expanding,
        vertical ? QSizePolicy::Expanding : QSizePolicy::Minimum
    );
    layout->addSpacerItem(spacer);

    for (auto action : actions) {
        layout->addWidget(OverlayTabWidget::createTitleButton(action, buttonSize));
    }

    if (tabWidget) {
        auto grip = new OverlaySizeGrip(tabWidget, vertical);
        QObject::connect(grip, &OverlaySizeGrip::dragMove, tabWidget, &OverlayTabWidget::onSizeGripMove);
        layout->addWidget(grip);
        grip->raise();
    }

    return spacer;
}

bool OverlayTabWidget::isStyleSheetDark(std::string curStyleSheet)
{
    if (curStyleSheet.find("dark") != std::string::npos
        || curStyleSheet.find("Dark") != std::string::npos) {
        return true;
    }
    return false;
}

QPixmap OverlayTabWidget::rotateAutoHideIcon(QPixmap pxAutoHide, Qt::DockWidgetArea dockArea)
{
    switch (dockArea) {
        case Qt::LeftDockWidgetArea:
            return pxAutoHide;
            break;
        case Qt::RightDockWidgetArea:
            return pxAutoHide.transformed(QTransform().scale(-1, 1));
            break;
        case Qt::TopDockWidgetArea:
            return pxAutoHide.transformed(QTransform().rotate(90));
            break;
        case Qt::BottomDockWidgetArea:
            return pxAutoHide.transformed(QTransform().rotate(-90));
            break;
        default:
            return pxAutoHide;
            break;
    }
}

// -----------------------------------------------------------

OverlayTitleBar::OverlayTitleBar(QWidget* parent)
    : QWidget(parent)
{
    setFocusPolicy(Qt::ClickFocus);
    setMouseTracking(true);
    setCursor(Qt::OpenHandCursor);
}

void OverlayTitleBar::setTitleItem(QLayoutItem* item)
{
    titleItem = item;
}

void OverlayTitleBar::paintEvent(QPaintEvent*)
{
    if (!titleItem) {
        return;
    }

    QDockWidget* dock = qobject_cast<QDockWidget*>(parentWidget());
    int vertical = false;
    int flags = Qt::AlignCenter;
    if (!dock) {
        OverlayTabWidget* tabWidget = qobject_cast<OverlayTabWidget*>(parentWidget());
        if (tabWidget) {
            switch (tabWidget->getDockArea()) {
                case Qt::TopDockWidgetArea:
                    vertical = true;
                // fallthrough
                case Qt::RightDockWidgetArea:
                    flags = Qt::AlignRight;
                    break;
                case Qt::BottomDockWidgetArea:
                    vertical = true;
                // fallthrough
                case Qt::LeftDockWidgetArea:
                    flags = Qt::AlignLeft;
                    break;
                default:
                    break;
            }
            dock = tabWidget->dockWidget(0);
        }
    }
    if (!dock) {
        return;
    }

    QPainter painter(this);
    if (qobject_cast<OverlayTabWidget*>(parentWidget())) {
        painter.fillRect(this->rect(), painter.background());
    }

    QRect r = titleItem->geometry();
    if (vertical) {
        r = r.transposed();
        painter.translate(r.left(), r.top() + r.width());
        painter.rotate(-90);
        painter.translate(-r.left(), -r.top());
    }

    QString title;
    if (OverlayManager::instance()->isMouseTransparent()) {
        if (timerId == 0) {
            timerId = startTimer(500);
        }
        title = blink ? tr("Mouse pass through, Esc to stop") : dock->windowTitle();
    }
    else {
        if (timerId != 0) {
            killTimer(timerId);
            timerId = 0;
        }
        title = dock->windowTitle();
    }
    QString text = painter.fontMetrics().elidedText(title, Qt::ElideRight, r.width());
    painter.drawText(r, flags, text);
}

void OverlayTitleBar::timerEvent(QTimerEvent* ev)
{
    if (timerId == ev->timerId()) {
        update();
        blink = !blink;
    }
}

static inline bool isNear(const QPoint& a, const QPoint& b, int tol = 16)
{
    QPoint d = a - b;
    return d.x() * d.x() + d.y() * d.y() < tol;
}

void OverlayTitleBar::endDrag()
{
    if (OverlayTabWidget::_Dragging == this) {
        OverlayTabWidget::_Dragging = nullptr;
        setCursor(Qt::OpenHandCursor);
        if (OverlayTabWidget::_DragFrame) {
            OverlayTabWidget::_DragFrame->hide();
        }
        if (OverlayTabWidget::_DragFloating) {
            OverlayTabWidget::_DragFrame->hide();
        }
    }
}

void OverlayTitleBar::mouseMoveEvent(QMouseEvent* me)
{
    if (ignoreMouse) {
        if (!(me->buttons() & Qt::LeftButton)) {
            ignoreMouse = false;
        }
        else {
            me->ignore();
            return;
        }
    }

    if (OverlayTabWidget::_Dragging != this && mouseMovePending && (me->buttons() & Qt::LeftButton)) {
        if (isNear(dragOffset, me->pos())) {
            return;
        }
        mouseMovePending = false;
        OverlayTabWidget::_Dragging = this;
    }

    if (OverlayTabWidget::_Dragging != this) {
        return;
    }

    if (!(me->buttons() & Qt::LeftButton)) {
        endDrag();
        return;
    }

#if QT_VERSION < QT_VERSION_CHECK(6, 0, 0)
    QPoint point = me->globalPos();
#else
    QPoint point = me->globalPosition().toPoint();
#endif

    OverlayManager::instance()->dragDockWidget(point, parentWidget(), dragOffset, dragSize);
}

void OverlayTitleBar::mousePressEvent(QMouseEvent* me)
{
    mouseMovePending = false;
    QWidget* parent = parentWidget();
    if (OverlayTabWidget::_Dragging || !parent || !getMainWindow() || me->button() != Qt::LeftButton) {
        return;
    }

    dragSize = parent->size();
    OverlayTabWidget* tabWidget = qobject_cast<OverlayTabWidget*>(parent);
    if (!tabWidget) {
        if (QApplication::queryKeyboardModifiers() == Qt::ShiftModifier) {
            ignoreMouse = true;
            me->ignore();
            return;
        }
    }
    else {
        for (int s : tabWidget->getSplitter()->sizes()) {
            if (!s) {
                continue;
            }
            if (tabWidget == OverlayTabWidget::_TopOverlay
                || tabWidget == OverlayTabWidget::_BottomOverlay) {
                dragSize.setWidth(s + this->width());
                dragSize.setHeight(tabWidget->height());
            }
            else {
                dragSize.setHeight(s + this->height());
                dragSize.setWidth(tabWidget->width());
            }
        }
    }
    ignoreMouse = false;
    QSize mwSize = getMainWindow()->size();
    dragSize.setWidth(
        std::max(
            OverlayParams::getDockOverlayMinimumSize(),
            static_cast<long>(std::min(mwSize.width() / 2, dragSize.width()))
        )
    );
    dragSize.setHeight(
        std::max(
            OverlayParams::getDockOverlayMinimumSize(),
            static_cast<long>(std::min(mwSize.height() / 2, dragSize.height()))
        )
    );

    dragOffset = me->pos();
    setCursor(Qt::ClosedHandCursor);
    mouseMovePending = true;
}

void OverlayTitleBar::mouseReleaseEvent(QMouseEvent* me)
{
    if (ignoreMouse) {
        me->ignore();
        return;
    }

    setCursor(Qt::OpenHandCursor);
    mouseMovePending = false;
    if (OverlayTabWidget::_Dragging != this) {
        return;
    }

    if (me->button() != Qt::LeftButton) {
        return;
    }

#if QT_VERSION < QT_VERSION_CHECK(6, 0, 0)
    QPoint point = me->globalPos();
#else
    QPoint point = me->globalPosition().toPoint();
#endif

    OverlayTabWidget::_Dragging = nullptr;
    OverlayManager::instance()->dragDockWidget(point, parentWidget(), dragOffset, dragSize, true);
    if (OverlayTabWidget::_DragFrame) {
        OverlayTabWidget::_DragFrame->hide();
    }
    if (OverlayTabWidget::_DragFloating) {
        OverlayTabWidget::_DragFloating->hide();
    }
}

void OverlayTitleBar::keyPressEvent(QKeyEvent* ke)
{
    if (OverlayTabWidget::_Dragging == this && ke->key() == Qt::Key_Escape) {
        endDrag();
    }
}


// -----------------------------------------------------------

OverlayDragFrame::OverlayDragFrame(QWidget* parent)
    : QWidget(parent)
{}

void OverlayDragFrame::paintEvent(QPaintEvent*)
{
    QPainter painter(this);
    painter.drawRect(0, 0, this->width() - 1, this->height() - 1);
    painter.setOpacity(0.3);
    painter.setBrush(QBrush(Qt::blue));
    painter.drawRect(0, 0, this->width() - 1, this->height() - 1);
}

QSize OverlayDragFrame::sizeHint() const
{
    return size();
}

QSize OverlayDragFrame::minimumSizeHint() const
{
    return minimumSize();
}

// -----------------------------------------------------------

OverlaySizeGrip::OverlaySizeGrip(QWidget* parent, bool vertical)
    : QWidget(parent)
    , vertical(vertical)
{
    if (vertical) {
        this->setFixedHeight(6);
        this->setMinimumWidth(widgetMinSize(this, true));
        this->setCursor(Qt::SizeVerCursor);
    }
    else {
        this->setFixedWidth(6);
        this->setMinimumHeight(widgetMinSize(this, true));
        this->setCursor(Qt::SizeHorCursor);
    }
    setMouseTracking(true);
}

void OverlaySizeGrip::paintEvent(QPaintEvent*)
{
    QPainter painter(this);
    painter.setPen(Qt::transparent);
    painter.setOpacity(0.5);
    painter.setBrush(QBrush(palette().color(QPalette::Shadow), Qt::Dense6Pattern));
    QRect rect(this->rect());
    painter.drawRect(rect);
}

void OverlaySizeGrip::mouseMoveEvent(QMouseEvent* me)
{
#if QT_VERSION < QT_VERSION_CHECK(6, 0, 0)
    QPoint point = me->globalPos();
#else
    QPoint point = me->globalPosition().toPoint();
#endif

    if ((me->buttons() & Qt::LeftButton)) {
        Q_EMIT dragMove(point);
    }
}

void OverlaySizeGrip::mousePressEvent(QMouseEvent*)
{}

void OverlaySizeGrip::mouseReleaseEvent(QMouseEvent*)
{}

// -----------------------------------------------------------

OverlaySplitter::OverlaySplitter(QWidget* parent)
    : QSplitter(parent)
{}

QSplitterHandle* OverlaySplitter::createHandle()
{
    auto widget = new OverlaySplitterHandle(this->orientation(), this);
    widget->setObjectName(QStringLiteral("OverlaySplitHandle"));
    QList<QAction*> actions;
    actions.append(&widget->actFloat);
    widget->setTitleItem(OverlayTabWidget::prepareTitleWidget(widget, actions));
    return widget;
}

// -----------------------------------------------------------

OverlaySplitterHandle::OverlaySplitterHandle(Qt::Orientation orientation, QSplitter* parent)
    : QSplitterHandle(orientation, parent)
{
    setMouseTracking(true);
    setFocusPolicy(Qt::ClickFocus);
    retranslate();
    refreshIcons();
    QObject::connect(&actFloat, &QAction::triggered, this, &OverlaySplitterHandle::onAction);
    timer.setSingleShot(true);
    QObject::connect(&timer, &QTimer::timeout, this, &OverlaySplitterHandle::onTimer);
}

void OverlaySplitterHandle::refreshIcons()
{
    actFloat.setIcon(BitmapFactory().pixmap("qss:overlay/icons/float.svg"));
}

void OverlaySplitterHandle::onTimer()
{
    if (isVisible() && qApp->widgetAt(QCursor::pos()) != this) {
        showTitle(false);
    }
}

void OverlaySplitterHandle::showEvent(QShowEvent* ev)
{
    if (OverlayParams::getDockOverlaySplitterHandleTimeout() > 0
        && qApp->widgetAt(QCursor::pos()) != this) {
        timer.start(OverlayParams::getDockOverlaySplitterHandleTimeout());
    }
    QSplitterHandle::showEvent(ev);
}

#if QT_VERSION < QT_VERSION_CHECK(6, 0, 0)
void OverlaySplitterHandle::enterEvent(QEvent* ev)
#else
void OverlaySplitterHandle::enterEvent(QEnterEvent* ev)
#endif
{
    timer.stop();
    QSplitterHandle::enterEvent(ev);
}

void OverlaySplitterHandle::leaveEvent(QEvent* ev)
{
    if (OverlayParams::getDockOverlaySplitterHandleTimeout() > 0) {
        timer.start(OverlayParams::getDockOverlaySplitterHandleTimeout());
    }
    QSplitterHandle::leaveEvent(ev);
}

QSize OverlaySplitterHandle::sizeHint() const
{
    QSize size = QSplitterHandle::sizeHint();
    int minSize = widgetMinSize(this, true);
    if (this->orientation() == Qt::Vertical) {
        size.setHeight(std::max(minSize, size.height()));
    }
    else {
        size.setWidth(std::max(minSize, size.width()));
    }
    return size;
}

void OverlaySplitterHandle::onAction()
{
    auto action = qobject_cast<QAction*>(sender());
    if (action == &actFloat) {
        QDockWidget* dock = dockWidget();
        if (dock) {
            OverlayManager::instance()->floatDockWidget(dock);
        }
    }
}

QDockWidget* OverlaySplitterHandle::dockWidget()
{
    QSplitter* parent = splitter();
    if (!parent) {
        return nullptr;
    }

    if (parent->handle(this->idx) != this) {
        this->idx = -1;
        for (int i = 0, c = parent->count(); i < c; ++i) {
            if (parent->handle(i) == this) {
                this->idx = i;
                break;
            }
        }
    }
    return qobject_cast<QDockWidget*>(parent->widget(this->idx));
}

void OverlaySplitterHandle::retranslate()
{
    actFloat.setToolTip(QObject::tr("Toggle floating window"));
}

void OverlaySplitterHandle::changeEvent(QEvent* e)
{
    QSplitterHandle::changeEvent(e);
    if (e->type() == QEvent::LanguageChange) {
        retranslate();
    }
}

void OverlaySplitterHandle::setTitleItem(QLayoutItem* item)
{
    titleItem = item;
}

void OverlaySplitterHandle::showTitle(bool enable)
{
    if (_showTitle == enable) {
        return;
    }
    if (!enable) {
        unsetCursor();
    }
    else {
        setCursor(this->orientation() == Qt::Horizontal ? Qt::SizeHorCursor : Qt::SizeVerCursor);
        if (OverlayParams::getDockOverlaySplitterHandleTimeout() > 0
            && qApp->widgetAt(QCursor::pos()) != this) {
            timer.start(OverlayParams::getDockOverlaySplitterHandleTimeout());
        }
    }
    _showTitle = enable;
    for (auto child : findChildren<QWidget*>(QString(), Qt::FindDirectChildrenOnly)) {
        child->setVisible(enable);
    }
    update();
}

void OverlaySplitterHandle::paintEvent(QPaintEvent* e)
{
    if (!_showTitle) {
        return;
    }

    if (!titleItem) {
        QSplitterHandle::paintEvent(e);
        return;
    }

    int flags = Qt::AlignCenter;
    auto tabWidget = qobject_cast<OverlayTabWidget*>(splitter() ? splitter()->parentWidget() : nullptr);

    if (tabWidget) {
        switch (tabWidget->getDockArea()) {
            case Qt::TopDockWidgetArea:
            case Qt::RightDockWidgetArea:
                flags = Qt::AlignRight;
                break;
            case Qt::BottomDockWidgetArea:
            case Qt::LeftDockWidgetArea:
                flags = Qt::AlignLeft;
                break;
            default:
                break;
        }
    }

    QDockWidget* dock = dockWidget();
    if (!dock) {
        QSplitterHandle::paintEvent(e);
        return;
    }

    QPainter painter(this);
    painter.fillRect(this->rect(), painter.background());

    QRect r = titleItem->geometry();
    if (this->orientation() != Qt::Vertical) {
        r = r.transposed();
        painter.translate(r.left(), r.top() + r.width());
        painter.rotate(-90);
        painter.translate(-r.left(), -r.top());
    }
    QString text = painter.fontMetrics().elidedText(dock->windowTitle(), Qt::ElideRight, r.width());

    painter.drawText(r, flags, text);
}

void OverlaySplitterHandle::endDrag()
{
    auto tabWidget = qobject_cast<OverlayTabWidget*>(splitter()->parentWidget());
    if (tabWidget) {
        dockWidget();
        tabWidget->onSplitterResize(this->idx);
    }
    OverlayTabWidget::_Dragging = nullptr;
    setCursor(this->orientation() == Qt::Horizontal ? Qt::SizeHorCursor : Qt::SizeVerCursor);
    if (OverlayTabWidget::_DragFrame) {
        OverlayTabWidget::_DragFrame->hide();
    }
    if (OverlayTabWidget::_DragFloating) {
        OverlayTabWidget::_DragFloating->hide();
    }
}

void OverlaySplitterHandle::keyPressEvent(QKeyEvent* ke)
{
    if (OverlayTabWidget::_Dragging == this && ke->key() == Qt::Key_Escape) {
        endDrag();
    }
}

void OverlaySplitterHandle::mouseMoveEvent(QMouseEvent* me)
{
    if (OverlayTabWidget::_Dragging != this) {
        return;
    }

    if (!(me->buttons() & Qt::LeftButton)) {
        endDrag();
        return;
    }

#if QT_VERSION < QT_VERSION_CHECK(6, 0, 0)
    QPoint point = me->globalPos();
#else
    QPoint point = me->globalPosition().toPoint();
#endif

    if (dragging == 1) {
        OverlayTabWidget* overlay = qobject_cast<OverlayTabWidget*>(splitter()->parentWidget());
        QPoint pos = me->pos();
        if (overlay) {
            switch (overlay->getDockArea()) {
                case Qt::LeftDockWidgetArea:
                case Qt::RightDockWidgetArea:
                    if (pos.x() < 0 || pos.x() > overlay->width()) {
                        dragging = 2;
                    }
                    break;
                case Qt::TopDockWidgetArea:
                case Qt::BottomDockWidgetArea:
                    if (pos.y() < 0 || pos.y() > overlay->height()) {
                        dragging = 2;
                    }
                    break;
                default:
                    break;
            }
        }
        if (dragging == 1) {
            QPoint offset = parentWidget()->mapFromGlobal(point) - dragOffset;
            moveSplitter(this->orientation() == Qt::Horizontal ? offset.x() : offset.y());
            return;
        }
        setCursor(Qt::ClosedHandCursor);
    }

    OverlayManager::instance()->dragDockWidget(point, dockWidget(), dragOffset, dragSize);
}

void OverlaySplitterHandle::mousePressEvent(QMouseEvent* me)
{
    if (OverlayTabWidget::_Dragging || !getMainWindow() || me->button() != Qt::LeftButton) {
        return;
    }

    OverlayTabWidget::_Dragging = this;
    dragging = 1;
    dragOffset = me->pos();
    auto dock = dockWidget();
    if (dock) {
        dragSize = dock->size();
        dock->show();
    }
    else {
        dragSize = QSize();
    }

    QSize mwSize = getMainWindow()->size();
    dragSize.setWidth(
        std::max(
            OverlayParams::getDockOverlayMinimumSize(),
            static_cast<long>(std::min(mwSize.width() / 2, dragSize.width()))
        )
    );
    dragSize.setHeight(
        std::max(
            OverlayParams::getDockOverlayMinimumSize(),
            static_cast<long>(std::min(mwSize.height() / 2, dragSize.height()))
        )
    );
}

void OverlaySplitterHandle::mouseReleaseEvent(QMouseEvent* me)
{
    if (OverlayTabWidget::_Dragging != this || me->button() != Qt::LeftButton) {
        return;
    }

    if (dragging == 1) {
        endDrag();
        return;
    }
    endDrag();

#if QT_VERSION < QT_VERSION_CHECK(6, 0, 0)
    QPoint point = me->globalPos();
#else
    QPoint point = me->globalPosition().toPoint();
#endif

    OverlayManager::instance()->dragDockWidget(point, dockWidget(), dragOffset, dragSize, true);
    // Warning! the handle itself maybe deleted after return from
    // dragDockWidget().
}

// -----------------------------------------------------------

OverlayGraphicsEffect::OverlayGraphicsEffect(QObject* parent)
    : QGraphicsEffect(parent)
    , _enabled(false)
    , _size(1, 1)
    , _blurRadius(2.0f)
    , _color(0, 0, 0, 80)
{}

QT_BEGIN_NAMESPACE
extern Q_WIDGETS_EXPORT void qt_blurImage(
    QPainter* p,
    QImage& blurImage,
    qreal radius,
    bool quality,
    bool alphaOnly,
    int transposed = 0
);
QT_END_NAMESPACE

void OverlayGraphicsEffect::draw(QPainter* painter)
{
    // if nothing to show outside the item, just draw source
    if (!_enabled || _blurRadius + _size.height() <= 0 || _blurRadius + _size.width() <= 0) {
        drawSource(painter);
        return;
    }

    PixmapPadMode mode = QGraphicsEffect::PadToEffectiveBoundingRect;
    QPoint offset;
    QPixmap px = sourcePixmap(Qt::DeviceCoordinates, &offset, mode);

    // return if no source
    if (px.isNull()) {
        return;
    }

    QTransform restoreTransform = painter->worldTransform();
    painter->setWorldTransform(QTransform());

    // Calculate size for the background image
    QImage tmp(px.size(), QImage::Format_ARGB32_Premultiplied);
    tmp.setDevicePixelRatio(px.devicePixelRatioF());
    tmp.fill(0);
    QPainter tmpPainter(&tmp);
    QPainterPath clip;
    tmpPainter.setCompositionMode(QPainter::CompositionMode_Source);
    if (_size.width() == 0 && _size.height() == 0) {
        tmpPainter.drawPixmap(QPoint(0, 0), px);
    }
    else {
        // exclude splitter handles
        auto splitter = qobject_cast<QSplitter*>(parent());
        if (splitter) {
            int i = -1;
            for (int size : splitter->sizes()) {
                ++i;
                if (!size) {
                    continue;
                }
                QWidget* w = splitter->widget(i);
                if (w->findChild<TaskView::TaskView*>()) {
                    continue;
                }
                QRect rect = w->geometry();
                if (splitter->orientation() == Qt::Vertical) {
                    clip.addRect(rect.x(), rect.y() + 4, rect.width(), rect.height() - 4);
                }
                else {
                    clip.addRect(rect.x() + 4, rect.y(), rect.width() - 4, rect.height());
                }
            }
            if (clip.isEmpty()) {
                drawSource(painter);
                return;
            }
            tmpPainter.setClipPath(clip);
        }

        for (int x = -_size.width(); x <= _size.width(); ++x) {
            for (int y = -_size.height(); y <= _size.height(); ++y) {
                if (x || y) {
                    tmpPainter.drawPixmap(QPoint(x, y), px);
                    tmpPainter.setCompositionMode(QPainter::CompositionMode_SourceOver);
                }
            }
        }
    }
    tmpPainter.end();

    // blur the alpha channel
    QImage blurred(tmp.size(), QImage::Format_ARGB32_Premultiplied);
    blurred.setDevicePixelRatio(px.devicePixelRatioF());
    blurred.fill(0);
    QPainter blurPainter(&blurred);
    qt_blurImage(&blurPainter, tmp, blurRadius(), false, true);
    blurPainter.end();

    tmp = blurred;

    // blacken the image...
    tmpPainter.begin(&tmp);
    tmpPainter.setCompositionMode(QPainter::CompositionMode_SourceIn);
    tmpPainter.fillRect(tmp.rect(), color());
    tmpPainter.end();

    // draw the blurred shadow...
    painter->drawImage(QPointF(offset.x() + _offset.x(), offset.y() + _offset.y()), tmp);

    // draw the actual pixmap...
    painter->drawPixmap(offset, px, QRectF());

    // restore world transform
    painter->setWorldTransform(restoreTransform);
}

QRectF OverlayGraphicsEffect::boundingRectFor(const QRectF& rect) const
{
    if (!_enabled) {
        return rect;
    }
    return rect.united(rect.adjusted(
        -_blurRadius - _size.width() + _offset.x(),
        -_blurRadius - _size.height() + _offset.y(),
        _blurRadius + _size.width() + _offset.x(),
        _blurRadius + _size.height() + _offset.y()
    ));
}

#include "moc_OverlayWidgets.cpp"
