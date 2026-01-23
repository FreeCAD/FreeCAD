// SPDX-License-Identifier: LGPL-2.1-or-later

/****************************************************************************
 *   Copyright (c) 2002 Jürgen Riegel <juergen.riegel@web.de>               *
 *   Copyright (c) 2023 FreeCAD Project Association                         *
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

#include <algorithm>
#include <cstring>
#include <QAbstractButton>
#include <QApplication>
#include <QCheckBox>
#include <QDoubleSpinBox>
#include <QSpinBox>
#include <QComboBox>
#include <QCursor>
#include <QDebug>
#include <QFrame>
#include <QGroupBox>
#include <QLabel>
#include <QLineEdit>
#include <QListWidget>
#include <QMap>
#include <QMenu>
#include <QMessageBox>
#include <QRadioButton>
#include <QScreen>
#include <QScrollArea>
#include <QScrollBar>
#include <QTabWidget>
#include <QTimer>
#include <QToolButton>
#include <QToolTip>
#include <QProcess>
#include <QPushButton>
#include <QWindow>
#include <QKeyEvent>
#include <QFocusEvent>
#include <QMouseEvent>
#include <QPointer>
#include <QSet>
#include <QStyledItemDelegate>
#include <QPainter>

#include <App/Application.h>
#include <Base/Console.h>
#include <Base/Exception.h>
#include <Base/Tools.h>

#include "Dialogs/DlgPreferencesImp.h"
#include "ui_DlgPreferences.h"
#include "BitmapFactory.h"
#include "MainWindow.h"
#include "Tools.h"
#include "WidgetFactory.h"

using namespace Gui::Dialog;

// Simple delegate to render first line bold, second line normal
// used by search box
class MixedFontDelegate: public QStyledItemDelegate
{
    static constexpr int horizontalPadding = 12;
    static constexpr int verticalPadding = 4;

public:
    explicit MixedFontDelegate(QObject* parent = nullptr)
        : QStyledItemDelegate(parent)
    {}

    void paint(QPainter* painter, const QStyleOptionViewItem& option, const QModelIndex& index) const override
    {
        if (!index.isValid()) {
            QStyledItemDelegate::paint(painter, option, index);
            return;
        }

        QString pathText, widgetText;
        extractTextData(index, pathText, widgetText);

        if (pathText.isEmpty()) {
            QStyledItemDelegate::paint(painter, option, index);
            return;
        }

        QFont boldFont, normalFont;
        createFonts(option.font, boldFont, normalFont);

        LayoutInfo layout
            = calculateLayout(pathText, widgetText, boldFont, normalFont, option.rect.width());

        painter->save();

        // draw selection background if selected
        if (option.state & QStyle::State_Selected) {
            painter->fillRect(option.rect, option.palette.highlight());
        }

        // Set text color based on selection
        QColor textColor = (option.state & QStyle::State_Selected)
            ? option.palette.highlightedText().color()
            : option.palette.text().color();
        painter->setPen(textColor);

        // draw path in bold (Tab/Page) with wrapping
        painter->setFont(boldFont);
        QRect boldRect(
            option.rect.left() + horizontalPadding,
            option.rect.top() + verticalPadding,
            layout.availableWidth,
            layout.pathHeight
        );
        painter->drawText(boldRect, Qt::TextWordWrap | Qt::AlignTop, pathText);

        // draw widget text in normal font (if present)
        if (!widgetText.isEmpty()) {
            painter->setFont(normalFont);
            QRect normalRect(
                option.rect.left() + horizontalPadding,
                option.rect.top() + verticalPadding + layout.pathHeight,
                layout.availableWidth,
                layout.widgetHeight
            );
            painter->drawText(normalRect, Qt::TextWordWrap | Qt::AlignTop, widgetText);
        }

        painter->restore();
    }

    QSize sizeHint(const QStyleOptionViewItem& option, const QModelIndex& index) const override
    {
        if (!index.isValid()) {
            return QStyledItemDelegate::sizeHint(option, index);
        }

        QString pathText, widgetText;
        extractTextData(index, pathText, widgetText);

        if (pathText.isEmpty()) {
            return QStyledItemDelegate::sizeHint(option, index);
        }

        QFont boldFont, normalFont;
        createFonts(option.font, boldFont, normalFont);

        LayoutInfo layout
            = calculateLayout(pathText, widgetText, boldFont, normalFont, option.rect.width());

        return {layout.totalWidth, layout.totalHeight};
    }

private:
    struct LayoutInfo
    {
        int availableWidth;
        int pathHeight;
        int widgetHeight;
        int totalWidth;
        int totalHeight;
    };

    void extractTextData(const QModelIndex& index, QString& pathText, QString& widgetText) const
    {
        // Use separate roles - all items should have proper role data
        pathText = index.data(PreferencesSearchController::PathRole).toString();
        widgetText = index.data(PreferencesSearchController::WidgetTextRole).toString();
    }

    void createFonts(const QFont& baseFont, QFont& boldFont, QFont& normalFont) const
    {
        boldFont = baseFont;
        boldFont.setBold(true);
        boldFont.setPointSize(boldFont.pointSize() - 1);  // make header smaller like a subtitle

        normalFont = baseFont;  // keep widget text at normal size
    }

    LayoutInfo calculateLayout(
        const QString& pathText,
        const QString& widgetText,
        const QFont& boldFont,
        const QFont& normalFont,
        int containerWidth
    ) const
    {

        QFontMetrics boldFm(boldFont);
        QFontMetrics normalFm(normalFont);

        int availableWidth = containerWidth
            - horizontalPadding * 2;  // account for left and right padding
        if (availableWidth <= 0) {
            constexpr int defaultPopupWidth = 300;
            availableWidth = defaultPopupWidth
                - horizontalPadding * 2;  // Fallback to popup width minus padding
        }

        // Calculate dimensions for path text (bold)
        QRect pathBoundingRect
            = boldFm.boundingRect(QRect(0, 0, availableWidth, 0), Qt::TextWordWrap, pathText);
        int pathHeight = pathBoundingRect.height();
        int pathWidth = pathBoundingRect.width();

        // Calculate dimensions for widget text (normal font, if present)
        int widgetHeight = 0;
        int widgetWidth = 0;
        if (!widgetText.isEmpty()) {
            QRect widgetBoundingRect
                = normalFm.boundingRect(QRect(0, 0, availableWidth, 0), Qt::TextWordWrap, widgetText);
            widgetHeight = widgetBoundingRect.height();
            widgetWidth = widgetBoundingRect.width();
        }

        int totalWidth = qMax(pathWidth, widgetWidth)
            + horizontalPadding * 2;  // +24 horizontal padding
        int totalHeight = verticalPadding * 2 + pathHeight
            + widgetHeight;  // 8 vertical padding + content heights

        LayoutInfo layout;
        layout.availableWidth = availableWidth;
        layout.pathHeight = pathHeight;
        layout.widgetHeight = widgetHeight;
        layout.totalWidth = totalWidth;
        layout.totalHeight = totalHeight;
        return layout;
    }
};

bool isParentOf(const QModelIndex& parent, const QModelIndex& child)
{
    for (auto it = child; it.isValid(); it = it.parent()) {
        if (it == parent) {
            return true;
        }
    }

    return false;
}

QModelIndex findRootIndex(const QModelIndex& index)
{
    auto root = index;

    while (root.parent().isValid()) {
        root = root.parent();
    }

    return root;
}

QWidget* PreferencesPageItem::getWidget() const
{
    return _widget;
}

void PreferencesPageItem::setWidget(QWidget* widget)
{
    if (_widget) {
        _widget->setProperty(PropertyName, QVariant::fromValue<PreferencesPageItem*>(nullptr));
    }

    _widget = widget;
    _widget->setProperty(PropertyName, QVariant::fromValue(this));
}

bool PreferencesPageItem::isExpanded() const
{
    return _expanded;
}

void PreferencesPageItem::setExpanded(bool expanded)
{
    _expanded = expanded;
}

// NOLINTBEGIN
Q_DECLARE_METATYPE(PreferencesPageItem*);

const int DlgPreferencesImp::GroupNameRole = Qt::UserRole + 1;
const int DlgPreferencesImp::PageNameRole = Qt::UserRole + 2;

/* TRANSLATOR Gui::Dialog::DlgPreferencesImp */

std::list<DlgPreferencesImp::TGroupPages> DlgPreferencesImp::_pages;
std::map<std::string, DlgPreferencesImp::Group> DlgPreferencesImp::_groupMap;

DlgPreferencesImp* DlgPreferencesImp::_activeDialog = nullptr;
// NOLINTEND

/**
 *  Constructs a DlgPreferencesImp which is a child of 'parent', with
 *  widget flags set to 'fl'
 *
 *  The dialog will by default be modeless, unless you set 'modal' to
 *  true to construct a modal dialog.
 */
DlgPreferencesImp::DlgPreferencesImp(QWidget* parent, Qt::WindowFlags fl)
    : QDialog(parent, fl)
    , ui(new Ui_DlgPreferences)
    , invalidParameter(false)
    , canEmbedScrollArea(true)
    , restartRequired(false)
{
    ui->setupUi(this);

    // remove unused help button
    setWindowFlags(windowFlags() & ~Qt::WindowContextHelpButtonHint);

    // Initialize search controller
    m_searchController = std::make_unique<PreferencesSearchController>(this, this);

    setupConnections();

    ui->groupsTreeView->setModel(&_model);

    // Configure search controller after UI setup
    m_searchController->setPreferencesModel(&_model);
    m_searchController->setGroupNameRole(GroupNameRole);
    m_searchController->setPageNameRole(PageNameRole);

    setupPages();

    // Maintain a static pointer to the current active dialog (if there is one) so that
    // if the static page manipulation functions are called while the dialog is showing
    // it can update its content.
    DlgPreferencesImp::_activeDialog = this;
}

/**
 *  Destroys the object and frees any allocated resources.
 */
DlgPreferencesImp::~DlgPreferencesImp()
{
    // Remove global event filter
    qApp->removeEventFilter(this);

    if (DlgPreferencesImp::_activeDialog == this) {
        DlgPreferencesImp::_activeDialog = nullptr;
    }
}

void DlgPreferencesImp::setupConnections()
{
    connect(ui->buttonBox, &QDialogButtonBox::clicked, this, &DlgPreferencesImp::onButtonBoxClicked);
    connect(ui->buttonBox, &QDialogButtonBox::helpRequested, getMainWindow(), &MainWindow::whatsThis);
    connect(ui->groupsTreeView, &QTreeView::clicked, this, &DlgPreferencesImp::onPageSelected);
    connect(ui->groupsTreeView, &QTreeView::expanded, this, &DlgPreferencesImp::onGroupExpanded);
    connect(ui->groupsTreeView, &QTreeView::collapsed, this, &DlgPreferencesImp::onGroupCollapsed);
    connect(ui->buttonReset, &QPushButton::clicked, this, &DlgPreferencesImp::showResetOptions);
    connect(
        ui->groupWidgetStack,
        &QStackedWidget::currentChanged,
        this,
        &DlgPreferencesImp::onStackWidgetChange
    );
    // Connect search functionality to controller
    connect(
        ui->searchBox,
        &QLineEdit::textChanged,
        m_searchController.get(),
        &PreferencesSearchController::onSearchTextChanged
    );

    // Connect navigation signal from controller to dialog
    connect(
        m_searchController.get(),
        &PreferencesSearchController::navigationRequested,
        this,
        &DlgPreferencesImp::onNavigationRequested
    );

    // Install event filter on search box for arrow key navigation
    ui->searchBox->installEventFilter(this);

    // Install global event filter to handle clicks outside popup
    qApp->installEventFilter(this);
}

void DlgPreferencesImp::setupPages()
{
    // make sure that pages are ready to create
    GetWidgetFactorySupplier();

    for (const auto& [name, pages] : _pages) {
        auto* group = createGroup(name);

        for (const auto& page : pages) {
            createPageInGroup(group, page);
        }
    }

    updatePageDependentWidgets();
}

QPixmap DlgPreferencesImp::loadIconForGroup(const std::string& name) const
{
    // normalize file name
    auto normalizeName = [](std::string str) {
        std::transform(str.begin(), str.end(), str.begin(), [](unsigned char ch) {
            return ch == ' ' ? '_' : std::tolower(ch);
        });
        return str;
    };

    std::string fileName = normalizeName(name);
    fileName = std::string("preferences-") + fileName;

    const int px = 24;
    QSize iconSize(px, px);
    QPixmap icon = Gui::BitmapFactory().pixmapFromSvg(fileName.c_str(), iconSize);

    if (icon.isNull()) {
        icon = Gui::BitmapFactory().pixmap(fileName.c_str());
        if (icon.isNull()) {
            qWarning() << "No group icon found for " << fileName.c_str();
        }
        else if (icon.size() != iconSize) {
            icon = icon.scaled(iconSize, Qt::KeepAspectRatio, Qt::SmoothTransformation);
        }
    }

    return icon;
}

/**
 * Create the necessary widgets for a new group named \a groupName. Returns a
 * pointer to the group's SettingsPageItem: that widget's lifetime is managed by the
 * QStandardItemModel, do not manually deallocate.
 */
PreferencesPageItem* DlgPreferencesImp::createGroup(const std::string& groupName)
{
    QString groupNameQString = QString::fromStdString(groupName);

    std::string iconName;

    QString tooltip;
    getGroupData(groupName, iconName, tooltip);

    auto groupPages = new QStackedWidget;
    groupPages->setProperty(GroupNameProperty, QVariant(groupNameQString));

    connect(groupPages, &QStackedWidget::currentChanged, this, &DlgPreferencesImp::onStackWidgetChange);


    if (ui->groupWidgetStack->count() > 0) {
        groupPages->setSizePolicy(QSizePolicy::Ignored, QSizePolicy::Ignored);
    }
    ui->groupWidgetStack->addWidget(groupPages);

    auto item = new PreferencesPageItem;

    item->setData(QVariant(groupNameQString), GroupNameRole);
    item->setText(QObject::tr(groupNameQString.toLatin1()));
    item->setToolTip(tooltip);
    item->setIcon(loadIconForGroup(iconName));
    item->setTextAlignment(Qt::AlignLeft | Qt::AlignVCenter);
    item->setFlags(Qt::ItemIsSelectable | Qt::ItemIsEnabled);
    item->setWidget(groupPages);
    item->setSelectable(false);

    _model.invisibleRootItem()->appendRow(item);

    return item;
}


PreferencePage* DlgPreferencesImp::createPreferencePage(
    const std::string& pageName,
    const std::string& groupName
)
{
    PreferencePage* page = WidgetFactory().createPreferencePage(pageName.c_str());

    if (!page) {
        return nullptr;
    }

    auto resetMargins = [](QWidget* widget) {
        widget->setContentsMargins(0, 0, 0, 0);
        if (auto layout = widget->layout()) {
            layout->setContentsMargins(0, 0, 0, 0);
        }
    };

    // settings layout already takes care for margins, we need to reset everything to 0
    resetMargins(page);

    // special handling for PreferenceUiForm to reset margins for forms too
    if (auto uiFormPage = qobject_cast<PreferenceUiForm*>(page)) {
        resetMargins(uiFormPage->form());
    }

    page->setProperty(GroupNameProperty, QString::fromStdString(groupName));
    page->setProperty(PageNameProperty, QString::fromStdString(pageName));

    return page;
}

/**
 * Create a new preference page called \a pageName in the group \a groupItem.
 */
void DlgPreferencesImp::createPageInGroup(PreferencesPageItem* groupItem, const std::string& pageName)
{
    try {
        PreferencePage* page
            = createPreferencePage(pageName, groupItem->data(GroupNameRole).toString().toStdString());

        if (!page) {
            Base::Console().warning("%s is not a preference page\n", pageName.c_str());
            return;
        }

        auto pageItem = new PreferencesPageItem;

        pageItem->setText(page->windowTitle());
        pageItem->setEditable(false);
        pageItem->setData(groupItem->data(GroupNameRole), GroupNameRole);
        pageItem->setData(QString::fromStdString(pageName), PageNameRole);
        pageItem->setWidget(page);

        groupItem->appendRow(pageItem);

        page->loadSettings();

        auto pages = qobject_cast<QStackedWidget*>(groupItem->getWidget());

        if (pages->count() > 0) {
            page->setSizePolicy(QSizePolicy::Ignored, QSizePolicy::Ignored);
        }

        pages->addWidget(page);
        addSizeHint(page);
    }
    catch (const Base::Exception& e) {
        Base::Console().error("Base exception thrown for '%s'\n", pageName.c_str());
        e.reportException();
    }
    catch (const std::exception& e) {
        Base::Console().error("C++ exception thrown for '%s' (%s)\n", pageName.c_str(), e.what());
    }
}

void DlgPreferencesImp::addSizeHint(QWidget* page)
{
    _sizeHintOfPages = _sizeHintOfPages.expandedTo(page->minimumSizeHint());
}

int DlgPreferencesImp::minimumPageWidth() const
{
    return _sizeHintOfPages.width();
}

int DlgPreferencesImp::minimumDialogWidth(int pageWidth) const
{
    // this is additional safety spacing to ensure that everything fits with scrollbar etc.
    const auto additionalMargin = style()->pixelMetric(QStyle::PM_ScrollBarExtent) + 8;

    QSize tree = ui->groupsTreeView->sizeHint();
    return pageWidth + tree.width() + additionalMargin;
}

void DlgPreferencesImp::updatePageDependentWidgets()
{
    auto currentPageItem = getCurrentPage();

    // update header of the page
    ui->headerLabel->setText(currentPageItem->text());

    // reset scroll area to start position
    ui->scrollArea->horizontalScrollBar()->setValue(0);
    ui->scrollArea->verticalScrollBar()->setValue(0);
}

/**
 * Adds a preference page with its class name \a className and
 * the group \a group it belongs to. To create this page it must
 * be registered in the WidgetFactory.
 * @see WidgetFactory
 * @see PrefPageProducer
 */
void DlgPreferencesImp::addPage(const std::string& className, const std::string& group)
{
    auto groupToAddTo = _pages.end();
    for (auto it = _pages.begin(); it != _pages.end(); ++it) {
        if (it->first == group) {
            groupToAddTo = it;
            break;
        }
    }

    if (groupToAddTo != _pages.end()) {
        // The group exists: add this page to the end of the list
        groupToAddTo->second.push_back(className);
    }
    else {
        // This is a new group: create it, with its one page
        std::list<std::string> pages;
        pages.push_back(className);
        _pages.emplace_back(group, pages);
    }

    if (DlgPreferencesImp::_activeDialog) {
        // If the dialog is currently showing, tell it to insert the new page
        _activeDialog->reloadPages();
    }
}

void DlgPreferencesImp::removePage(const std::string& className, const std::string& group)
{
    for (auto it = _pages.begin(); it != _pages.end(); ++it) {
        if (it->first == group) {
            if (className.empty()) {
                _pages.erase(it);
                return;
            }

            std::list<std::string>& p = it->second;
            for (auto jt = p.begin(); jt != p.end(); ++jt) {
                if (*jt == className) {
                    p.erase(jt);
                    if (p.empty()) {
                        _pages.erase(it);
                    }
                    return;
                }
            }
        }
    }
}

/**
 * Sets a custom icon name or tool tip for a given group.
 */
void DlgPreferencesImp::setGroupData(const std::string& name, const std::string& icon, const QString& tip)
{
    Group group;
    group.iconName = icon;
    group.tooltip = tip;
    _groupMap[name] = group;
}

/**
 * Gets the icon name or tool tip for a given group. If no custom name or tool tip is given
 * they are determined from the group name.
 */
void DlgPreferencesImp::getGroupData(const std::string& group, std::string& icon, QString& tip)
{
    auto it = _groupMap.find(group);

    if (it != _groupMap.end()) {
        icon = it->second.iconName;
        tip = it->second.tooltip;
    }

    if (icon.empty()) {
        icon = group;
    }

    if (tip.isEmpty()) {
        tip = QObject::tr(group.c_str());
    }
}

/**
 * Activates the page at position \a index of the group with name \a group.
 */
void DlgPreferencesImp::activateGroupPage(const QString& group, int index)
{
    for (int i = 0; i < ui->groupWidgetStack->count(); i++) {
        auto* pageStackWidget = qobject_cast<QStackedWidget*>(ui->groupWidgetStack->widget(i));

        if (!pageStackWidget) {
            continue;
        }

        if (pageStackWidget->property(GroupNameProperty).toString() == group) {
            ui->groupWidgetStack->setCurrentWidget(pageStackWidget);
            pageStackWidget->setCurrentIndex(index);

            updatePageDependentWidgets();

            return;
        }
    }
}

/**
 * Activates the page with name \a pageName of the group with name \a group.
 */
void DlgPreferencesImp::activateGroupPageByPageName(const QString& group, const QString& pageName)
{

    for (int i = 0; i < ui->groupWidgetStack->count(); i++) {
        auto* pageStackWidget = qobject_cast<QStackedWidget*>(ui->groupWidgetStack->widget(i));

        if (!pageStackWidget) {
            continue;
        }

        if (pageStackWidget->property(GroupNameProperty).toString() == group) {
            ui->groupWidgetStack->setCurrentWidget(pageStackWidget);
            for (int pageIdx = 0; pageIdx < pageStackWidget->count(); pageIdx++) {
                auto page = qobject_cast<PreferencePage*>(pageStackWidget->widget(pageIdx));
                if (page) {
                    if (page->property(PageNameProperty).toString() == pageName) {
                        pageStackWidget->setCurrentIndex(pageIdx);
                        break;
                    }
                }
            }

            updatePageDependentWidgets();

            return;
        }
    }
}

/**
 * Returns the group name \a group and position \a index of the active page.
 */
void DlgPreferencesImp::activeGroupPage(QString& group, int& index) const
{
    auto groupWidget = qobject_cast<QStackedWidget*>(ui->groupWidgetStack->currentWidget());

    if (groupWidget) {
        group = groupWidget->property(GroupNameProperty).toString();
        index = groupWidget->currentIndex();
    }
}

void DlgPreferencesImp::accept()
{
    this->invalidParameter = false;

    applyChanges();

    if (!this->invalidParameter) {
        QDialog::accept();
        restartIfRequired();
    }
}

void DlgPreferencesImp::reject()
{
    QDialog::reject();
    restartIfRequired();
}

void DlgPreferencesImp::onButtonBoxClicked(QAbstractButton* btn)
{
    if (ui->buttonBox->standardButton(btn) == QDialogButtonBox::Apply) {
        applyChanges();
    }
}

void DlgPreferencesImp::showResetOptions()
{
    QMenu menu(this);

    auto currentPageItem = getCurrentPage();
    auto currentGroupItem = static_cast<PreferencesPageItem*>(currentPageItem->parent());

    auto pageText = currentPageItem->text();
    auto groupText = currentGroupItem->text();

    // Reset per page
    QAction* pageAction = menu.addAction(tr("Reset Page '%1'").arg(pageText), this, [&] {
        restorePageDefaults(currentPageItem);
    });
    pageAction->setToolTip(tr("Resets the user settings for the page '%1'").arg(pageText));

    // Reset per group
    QAction* groupAction = menu.addAction(
        tr("Reset Group '%1'").arg(groupText),

        this,
        [&] { restorePageDefaults(static_cast<PreferencesPageItem*>(currentPageItem->parent())); }
    );
    groupAction->setToolTip(tr("Resets the user settings for the group '%1'").arg(groupText));

    // Reset all
    QAction* allAction = menu.addAction(tr("Reset All"), this, &DlgPreferencesImp::restoreDefaults);

    allAction->setToolTip(tr("Resets the user settings entirely"));

    connect(&menu, &QMenu::hovered, [&menu](QAction* hover) {
        QPoint pos = menu.pos();
        const int pad = 10;
        pos.rx() += menu.width() + pad;
        QToolTip::showText(pos, hover->toolTip());
    });

    menu.exec(QCursor::pos());
}

void DlgPreferencesImp::restoreDefaults()
{
    QMessageBox box(this);
    box.setIcon(QMessageBox::Question);
    box.setWindowTitle(tr("Clear User Settings"));
    box.setText(tr("Clear all your user settings?"));
    box.setInformativeText(tr("All settings will be cleared."));
    box.setStandardButtons(QMessageBox::Yes | QMessageBox::No);
    box.setDefaultButton(QMessageBox::No);

    if (box.exec() == QMessageBox::Yes) {
        // keep this parameter
        bool saveParameter = App::GetApplication()
                                 .GetParameterGroupByPath("User parameter:BaseApp/Preferences/General")
                                 ->GetBool("SaveUserParameter", true);

        ParameterManager* mgr = App::GetApplication().GetParameterSet("User parameter");
        mgr->Clear();

        App::GetApplication()
            .GetParameterGroupByPath("User parameter:BaseApp/Preferences/General")
            ->SetBool("SaveUserParameter", saveParameter);

        reject();
    }
}
/**
 * If the dialog is currently showing and the static variable _pages changed, this function
 * will rescan that list of pages and add any that are new to the current dialog. It will not
 * remove any pages that are no longer in the list, and will not change the user's current
 * active page.
 */
void DlgPreferencesImp::reloadPages()
{
    // Make sure that pages are ready to create
    GetWidgetFactorySupplier();

    for (const auto& [group, pages] : _pages) {
        QString groupName = QString::fromStdString(group);

        // First, does this group already exist?
        PreferencesPageItem* groupItem = nullptr;

        auto root = _model.invisibleRootItem();
        for (int i = 0; i < root->rowCount(); i++) {
            auto currentGroupItem = static_cast<PreferencesPageItem*>(root->child(i));
            auto currentGroupName = currentGroupItem->data(GroupNameRole).toString();

            if (currentGroupName == groupName) {
                groupItem = currentGroupItem;
                break;
            }
        }

        // This is a new group that wasn't there when we started this instance of the dialog:
        if (!groupItem) {
            groupItem = createGroup(group);
        }

        // Move on to the pages in the group to see if we need to add any
        for (const auto& page : pages) {
            // Does this page already exist?
            QString pageName = QString::fromStdString(page);

            bool pageExists = false;

            for (int i = 0; i < groupItem->rowCount(); i++) {
                auto currentPageItem = static_cast<PreferencesPageItem*>(groupItem->child(i));

                if (currentPageItem->data(PageNameRole).toString() == pageName) {
                    pageExists = true;
                    break;
                }
            }

            // This is a new page that wasn't there when we started this instance of the dialog:
            if (!pageExists) {
                createPageInGroup(groupItem, page);
            }
        }
    }
}

void DlgPreferencesImp::applyChanges()
{
    // Checks if any of the classes that represent several pages of settings
    // (DlgSettings*.*) implement checkSettings() method.  If any of them do,
    // call it to validate if user input is correct.  If something fails (i.e.,
    // not correct), shows a messageBox and set this->invalidParameter = true to
    // cancel further operation in other methods (like in accept()).

    for (int i = 0; i < ui->groupWidgetStack->count(); i++) {
        auto pagesStackWidget = qobject_cast<QStackedWidget*>(ui->groupWidgetStack->widget(i));

        for (int j = 0; j < pagesStackWidget->count(); j++) {
            QWidget* page = pagesStackWidget->widget(j);

            int index = page->metaObject()->indexOfMethod("checkSettings()");

            if (index >= 0) {
                try {
                    page->qt_metacall(QMetaObject::InvokeMetaMethod, index, nullptr);
                }
                catch (const Base::Exception& e) {
                    ui->groupWidgetStack->setCurrentIndex(i);
                    pagesStackWidget->setCurrentIndex(j);

                    QMessageBox::warning(this, tr("Wrong parameter"), QString::fromLatin1(e.what()));

                    this->invalidParameter = true;

                    // exit early due to found errors
                    return;
                }
            }
        }
    }

    // If everything is ok (i.e., no validation problem), call method
    // saveSettings() in every subpage (DlgSetting*) object.
    for (int i = 0; i < ui->groupWidgetStack->count(); i++) {
        auto pageStackWidget = qobject_cast<QStackedWidget*>(ui->groupWidgetStack->widget(i));

        for (int j = 0; j < pageStackWidget->count(); j++) {
            auto page = qobject_cast<PreferencePage*>(pageStackWidget->widget(j));

            if (page) {
                page->saveSettings();
                restartRequired = restartRequired || page->isRestartRequired();
            }
        }
    }

    bool saveParameter = App::GetApplication()
                             .GetParameterGroupByPath("User parameter:BaseApp/Preferences/General")
                             ->GetBool("SaveUserParameter", true);

    if (saveParameter) {
        ParameterManager* parmgr = App::GetApplication().GetParameterSet("User parameter");
        parmgr->SaveDocument(App::Application::Config()["UserParameter"].c_str());
    }
}

void DlgPreferencesImp::restartIfRequired()
{
    if (restartRequired) {
        QMessageBox restartBox(parentWidget());  // current window likely already closed, cant
                                                 // parent to it

        restartBox.setIcon(QMessageBox::Warning);
        restartBox.setWindowTitle(tr("Restart Required"));
        restartBox.setText(tr("Restart FreeCAD for changes to take effect."));
        restartBox.setStandardButtons(QMessageBox::Ok | QMessageBox::Cancel);
        restartBox.setDefaultButton(QMessageBox::Cancel);
        auto okBtn = restartBox.button(QMessageBox::Ok);
        auto cancelBtn = restartBox.button(QMessageBox::Cancel);
        okBtn->setText(tr("Restart Now"));
        cancelBtn->setText(tr("Restart Later"));

        int exec = restartBox.exec();

        if (exec == QMessageBox::Ok) {
            // restart FreeCAD after a delay to give time to this dialog to close
            const int ms = 1000;
            QTimer::singleShot(ms, []() {
                QStringList args = QApplication::arguments();
                args.pop_front();
                if (getMainWindow()->close()) {
                    QProcess::startDetached(QApplication::applicationFilePath(), args);
                }
            });
        }
    }
}

void DlgPreferencesImp::showEvent(QShowEvent* ev)
{
    QDialog::showEvent(ev);

    auto screen = windowHandle()->screen();
    auto availableSize = screen->availableSize();

    // leave some portion of height so preferences window does not take
    // entire screen height. User will still be able to resize the window,
    // but it should never start too tall.
    auto maxStartHeight = availableSize.height() - minVerticalEmptySpace;

    if (height() > maxStartHeight) {
        auto heightDifference = availableSize.height() - maxStartHeight;

        // resize and reposition window so it is fully visible
        resize(width(), maxStartHeight);
        move(x(), heightDifference / 2);
    }

    expandToMinimumDialogWidth();
}

void DlgPreferencesImp::expandToMinimumDialogWidth()
{
    auto screen = windowHandle()->screen();
    auto availableSize = screen->availableSize();

    int mw = minimumDialogWidth(minimumPageWidth());
    // expand dialog to minimum size required but do not use more than specified width portion
    resize(std::min(int(maxScreenWidthCoveragePercent * availableSize.width()), mw), height());
}

void DlgPreferencesImp::onPageSelected(const QModelIndex& index)
{
    auto* currentItem = static_cast<PreferencesPageItem*>(_model.itemFromIndex(index));

    if (currentItem->hasChildren()) {
        auto pageIndex = currentItem->child(0)->index();

        ui->groupsTreeView->selectionModel()->select(pageIndex, QItemSelectionModel::ClearAndSelect);

        onPageSelected(pageIndex);

        return;
    }

    auto groupIndex = findRootIndex(index);

    auto* groupItem = static_cast<PreferencesPageItem*>(_model.itemFromIndex(groupIndex));
    auto* pagesStackWidget = static_cast<QStackedWidget*>(groupItem->getWidget());

    ui->groupWidgetStack->setCurrentWidget(groupItem->getWidget());

    if (index != groupIndex) {
        pagesStackWidget->setCurrentIndex(index.row());
    }

    updatePageDependentWidgets();
}

void DlgPreferencesImp::onGroupExpanded(const QModelIndex& index)
{
    auto root = findRootIndex(index);

    auto* groupItem = static_cast<PreferencesPageItem*>(_model.itemFromIndex(root));

    groupItem->setExpanded(true);
}

void DlgPreferencesImp::onGroupCollapsed(const QModelIndex& index)
{
    auto root = findRootIndex(index);

    auto* groupItem = static_cast<PreferencesPageItem*>(_model.itemFromIndex(root));

    groupItem->setExpanded(false);
}

void DlgPreferencesImp::onStackWidgetChange(int index)
{
    auto stack = qobject_cast<QStackedWidget*>(sender());

    for (int i = 0; i < stack->count(); i++) {
        auto current = stack->widget(i);
        current->setSizePolicy(QSizePolicy::Ignored, QSizePolicy::Ignored);
    }

    if (auto selected = stack->widget(index)) {
        selected->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
    }

    auto currentItem = getCurrentPage();

    if (!currentItem) {
        return;
    }

    auto currentIndex = currentItem->index();

    auto root = _model.invisibleRootItem();
    for (int i = 0; i < root->rowCount(); i++) {
        auto currentGroup = static_cast<PreferencesPageItem*>(root->child(i));
        auto currentGroupIndex = currentGroup->index();

        // don't do anything to group of selected item
        if (isParentOf(currentGroupIndex, currentIndex)) {
            continue;
        }

        if (!currentGroup->isExpanded()) {
            ui->groupsTreeView->collapse(currentGroupIndex);
        }
    }

    auto parentItem = currentItem;
    while ((parentItem = static_cast<PreferencesPageItem*>(parentItem->parent()))) {
        bool wasExpanded = parentItem->isExpanded();
        ui->groupsTreeView->expand(parentItem->index());
        parentItem->setExpanded(wasExpanded);
    }

    ui->groupsTreeView->selectionModel()->select(currentIndex, QItemSelectionModel::ClearAndSelect);
}

void DlgPreferencesImp::onNavigationRequested(const QString& groupName, const QString& pageName)
{
    navigateToSearchResult(groupName, pageName);
}

void DlgPreferencesImp::navigateToSearchResult(const QString& groupName, const QString& pageName)
{
    // Find the group and page items
    auto root = _model.invisibleRootItem();
    for (int i = 0; i < root->rowCount(); i++) {
        auto groupItem = static_cast<PreferencesPageItem*>(root->child(i));
        if (groupItem->data(GroupNameRole).toString() == groupName) {

            // Find the specific page
            for (int j = 0; j < groupItem->rowCount(); j++) {
                auto pageItem = static_cast<PreferencesPageItem*>(groupItem->child(j));
                if (pageItem->data(PageNameRole).toString() == pageName) {

                    // Expand the group if needed
                    ui->groupsTreeView->expand(groupItem->index());

                    // Select the page
                    ui->groupsTreeView->selectionModel()->select(
                        pageItem->index(),
                        QItemSelectionModel::ClearAndSelect
                    );

                    // Navigate to the page
                    onPageSelected(pageItem->index());

                    return;
                }
            }

            // If no specific page found, just navigate to the group
            ui->groupsTreeView->selectionModel()->select(
                groupItem->index(),
                QItemSelectionModel::ClearAndSelect
            );
            onPageSelected(groupItem->index());
            return;
        }
    }
}

void DlgPreferencesImp::changeEvent(QEvent* e)
{
    if (e->type() == QEvent::LanguageChange) {
        ui->retranslateUi(this);

        auto root = _model.invisibleRootItem();
        for (int i = 0; i < root->rowCount(); i++) {
            auto groupItem = static_cast<PreferencesPageItem*>(root->child(i));
            auto groupName = groupItem->data(GroupNameRole).toString();

            groupItem->setText(QObject::tr(groupName.toLatin1()));

            for (int j = 0; j < groupItem->rowCount(); j++) {
                auto pageModelItem = static_cast<PreferencesPageItem*>(groupItem->child(j));
                auto pageModelWidget = static_cast<PreferencePage*>(pageModelItem->getWidget());

                pageModelItem->setText(pageModelWidget->windowTitle());
            }
        }

        expandToMinimumDialogWidth();
        updatePageDependentWidgets();
    }
    else {
        QWidget::changeEvent(e);
    }
}

void DlgPreferencesImp::reload()
{
    for (int i = 0; i < ui->groupWidgetStack->count(); i++) {
        auto pageStackWidget = static_cast<QStackedWidget*>(ui->groupWidgetStack->widget(i));

        for (int j = 0; j < pageStackWidget->count(); j++) {
            auto page = qobject_cast<PreferencePage*>(pageStackWidget->widget(j));

            if (page) {
                page->loadSettings();
            }
        }
    }

    applyChanges();
}

void DlgPreferencesImp::restorePageDefaults(PreferencesPageItem* item)
{
    if (item->hasChildren()) {
        // If page has children iterate over them and restore each
        for (int i = 0; i < item->rowCount(); i++) {
            auto child = static_cast<PreferencesPageItem*>(item->child(i));

            restorePageDefaults(child);
        }
    }
    else {
        auto* page = qobject_cast<PreferencePage*>(item->getWidget());

        page->resetSettingsToDefaults();
        /**
         * Let's save the restart request before the page object is deleted and replaced with
         * the newPage object (which has restartRequired initialized to false)
         */
        restartRequired = restartRequired || page->isRestartRequired();

        std::string pageName = page->property(PageNameProperty).toString().toStdString();
        std::string groupName = page->property(GroupNameProperty).toString().toStdString();

        auto newPage = createPreferencePage(pageName, groupName);

        newPage->loadSettings();

        auto groupPageStack = qobject_cast<QStackedWidget*>(page->parentWidget());
        auto replacedWidgetIndex = groupPageStack->indexOf(page);
        auto currentWidgetIndex = groupPageStack->currentIndex();

        groupPageStack->removeWidget(page);
        groupPageStack->insertWidget(replacedWidgetIndex, newPage);

        item->setWidget(newPage);

        if (replacedWidgetIndex == currentWidgetIndex) {
            groupPageStack->setCurrentIndex(currentWidgetIndex);
        }
    }
}

PreferencesPageItem* DlgPreferencesImp::getCurrentPage() const
{
    auto groupPagesStack = qobject_cast<QStackedWidget*>(ui->groupWidgetStack->currentWidget());

    if (!groupPagesStack) {
        return nullptr;
    }

    auto pageWidget = qobject_cast<PreferencePage*>(groupPagesStack->currentWidget());

    if (!pageWidget) {
        return nullptr;
    }

    return pageWidget->property(PreferencesPageItem::PropertyName).value<PreferencesPageItem*>();
}

// PreferencesSearchController implementation
PreferencesSearchController::PreferencesSearchController(DlgPreferencesImp* parentDialog, QObject* parent)
    : QObject(parent)
    , m_parentDialog(parentDialog)
    , m_preferencesModel(nullptr)
    , m_groupNameRole(0)
    , m_pageNameRole(0)
    , m_searchBox(nullptr)
    , m_searchResultsList(nullptr)
{
    // Get reference to search box from parent dialog's UI
    m_searchBox = m_parentDialog->ui->searchBox;

    // Create the search results popup list
    m_searchResultsList = new QListWidget(m_parentDialog);
    m_searchResultsList->setWindowFlags(
        Qt::ToolTip | Qt::FramelessWindowHint | Qt::WindowStaysOnTopHint | Qt::X11BypassWindowManagerHint
    );
    m_searchResultsList->setVisible(false);
    m_searchResultsList->setMinimumWidth(300);
    m_searchResultsList->setMaximumHeight(400);  // Increased max height
    m_searchResultsList->setFrameStyle(QFrame::Box | QFrame::Raised);
    m_searchResultsList->setLineWidth(1);
    m_searchResultsList->setFocusPolicy(Qt::NoFocus);  // Don't steal focus from search box
    m_searchResultsList->setAttribute(Qt::WA_ShowWithoutActivating);  // Show without
                                                                      // activating/stealing focus
    m_searchResultsList->setWordWrap(true);                           // Enable word wrapping
    m_searchResultsList->setTextElideMode(Qt::ElideNone);  // Don't elide text, let it wrap instead
    m_searchResultsList->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);  // Disable horizontal
                                                                                // scrollbar
    m_searchResultsList->setSpacing(0);                   // Remove spacing between items
    m_searchResultsList->setContentsMargins(0, 0, 0, 0);  // Remove margins

    // Set custom delegate for mixed font rendering (bold first line, normal second line)
    m_searchResultsList->setItemDelegate(new MixedFontDelegate(m_searchResultsList));

    // Connect search results list signals
    connect(
        m_searchResultsList,
        &QListWidget::itemSelectionChanged,
        this,
        &PreferencesSearchController::onSearchResultSelected
    );
    connect(
        m_searchResultsList,
        &QListWidget::itemDoubleClicked,
        this,
        &PreferencesSearchController::onSearchResultDoubleClicked
    );
    connect(
        m_searchResultsList,
        &QListWidget::itemClicked,
        this,
        &PreferencesSearchController::onSearchResultClicked
    );

    // Install event filter for keyboard navigation in search results
    m_searchResultsList->installEventFilter(m_parentDialog);
}

void PreferencesSearchController::setPreferencesModel(QStandardItemModel* model)
{
    m_preferencesModel = model;
}

void PreferencesSearchController::setGroupNameRole(int role)
{
    m_groupNameRole = role;
}

void PreferencesSearchController::setPageNameRole(int role)
{
    m_pageNameRole = role;
}

QListWidget* PreferencesSearchController::getSearchResultsList() const
{
    return m_searchResultsList;
}

bool PreferencesSearchController::isPopupVisible() const
{
    return m_searchResultsList && m_searchResultsList->isVisible();
}

bool PreferencesSearchController::isPopupUnderMouse() const
{
    return m_searchResultsList && m_searchResultsList->underMouse();
}

bool PreferencesSearchController::isPopupAncestorOf(QWidget* widget) const
{
    return m_searchResultsList && m_searchResultsList->isAncestorOf(widget);
}

void PreferencesSearchController::onSearchTextChanged(const QString& text)
{
    if (text.isEmpty()) {
        clearHighlights();
        m_searchResults.clear();
        m_lastSearchText.clear();
        hideSearchResultsList();
        return;
    }

    // Only perform new search if text changed
    if (text != m_lastSearchText) {
        performSearch(text);
        m_lastSearchText = text;
    }
}

void PreferencesSearchController::performSearch(const QString& searchText)
{
    clearHighlights();
    m_searchResults.clear();

    if (searchText.length() < 2) {
        hideSearchResultsList();
        return;
    }

    // Search through all groups and pages to collect ALL results
    auto root = m_preferencesModel->invisibleRootItem();
    for (int i = 0; i < root->rowCount(); i++) {
        auto groupItem = static_cast<PreferencesPageItem*>(root->child(i));
        auto groupName = groupItem->data(m_groupNameRole).toString();
        auto groupStack = qobject_cast<QStackedWidget*>(groupItem->getWidget());

        if (!groupStack) {
            continue;
        }

        // Search in each page of the group
        for (int j = 0; j < groupItem->rowCount(); j++) {
            auto pageItem = static_cast<PreferencesPageItem*>(groupItem->child(j));
            auto pageName = pageItem->data(m_pageNameRole).toString();
            auto pageWidget = qobject_cast<PreferencePage*>(pageItem->getWidget());

            if (!pageWidget) {
                continue;
            }

            // Collect all matching widgets in this page
            collectSearchResults(
                pageWidget,
                searchText,
                groupName,
                pageName,
                pageItem->text(),
                groupItem->text()
            );
        }
    }

    // Sort results by score (highest first)
    std::sort(
        m_searchResults.begin(),
        m_searchResults.end(),
        [](const SearchResult& a, const SearchResult& b) { return a.score > b.score; }
    );

    // Update UI with search results
    if (!m_searchResults.isEmpty()) {
        populateSearchResultsList();
        showSearchResultsList();
    }
    else {
        hideSearchResultsList();
    }
}

void PreferencesSearchController::clearHighlights()
{
    // Restore original styles for all highlighted widgets
    for (auto widget : m_highlightedWidgets) {
        if (widget && m_originalStyles.contains(widget)) {
            widget->setStyleSheet(m_originalStyles[widget]);
        }
    }
    m_highlightedWidgets.clear();
    m_originalStyles.clear();
}

void PreferencesSearchController::collectSearchResults(
    QWidget* widget,
    const QString& searchText,
    const QString& groupName,
    const QString& pageName,
    const QString& pageDisplayName,
    const QString& tabName
)
{
    if (!widget) {
        return;
    }

    // First, check if the page display name itself matches (highest priority)
    int pageScore = 0;
    if (fuzzyMatch(searchText, pageDisplayName, pageScore)) {
        SearchResult result {
            .groupName = groupName,
            .pageName = pageName,
            .widget = widget,              // Use the page widget itself
            .matchText = pageDisplayName,  // Use display name, not internal name
            .groupBoxName = QString(),     // No groupbox for page-level match
            .tabName = tabName,
            .pageDisplayName = pageDisplayName,
            .isPageLevelMatch = true,  // Mark as page-level match
            .score = pageScore + 2000  // Boost page-level matches
        };
        m_searchResults.append(result);
        // Continue searching for individual items even if page matches
    }

    // Search different widget types using the template method
    searchWidgetType<QLabel>(widget, searchText, groupName, pageName, pageDisplayName, tabName);
    searchWidgetType<QCheckBox>(widget, searchText, groupName, pageName, pageDisplayName, tabName);
    searchWidgetType<QRadioButton>(widget, searchText, groupName, pageName, pageDisplayName, tabName);
    searchWidgetType<QPushButton>(widget, searchText, groupName, pageName, pageDisplayName, tabName);
    searchWidgetType<QGroupBox>(widget, searchText, groupName, pageName, pageDisplayName, tabName);
    searchWidgetType<QComboBox>(widget, searchText, groupName, pageName, pageDisplayName, tabName);
    searchWidgetType<QSpinBox>(widget, searchText, groupName, pageName, pageDisplayName, tabName);
    searchWidgetType<QDoubleSpinBox>(widget, searchText, groupName, pageName, pageDisplayName, tabName);
}

void PreferencesSearchController::onSearchResultSelected()
{
    // This method is called when a search result is selected (arrow keys or single click)
    // Navigate immediately but keep popup open
    if (m_searchResultsList && m_searchResultsList->currentItem()) {
        navigateToCurrentSearchResult(PopupAction::KeepOpen);
    }

    ensureSearchBoxFocus();
}

void PreferencesSearchController::onSearchResultClicked()
{
    // Handle single click - navigate immediately but keep popup open
    if (m_searchResultsList && m_searchResultsList->currentItem()) {
        navigateToCurrentSearchResult(PopupAction::KeepOpen);
    }

    ensureSearchBoxFocus();
}

void PreferencesSearchController::onSearchResultDoubleClicked()
{
    // Handle double click - navigate and close popup
    if (m_searchResultsList && m_searchResultsList->currentItem()) {
        navigateToCurrentSearchResult(PopupAction::CloseAfter);
    }
}

void PreferencesSearchController::navigateToCurrentSearchResult(PopupAction action)
{
    QListWidgetItem* currentItem = m_searchResultsList->currentItem();

    // Skip if it's a separator (non-selectable item) or no item selected
    if (!currentItem || !(currentItem->flags() & Qt::ItemIsSelectable)) {
        return;
    }

    // Get the result index directly from the item data
    bool ok = false;
    int resultIndex = currentItem->data(Qt::UserRole).toInt(&ok);

    if (ok && resultIndex >= 0 && resultIndex < m_searchResults.size()) {
        const SearchResult& result = m_searchResults.at(resultIndex);

        // Emit signal to request navigation
        Q_EMIT navigationRequested(result.groupName, result.pageName);

        // Clear any existing highlights
        clearHighlights();

        // Only highlight specific widgets for non-page-level matches
        if (!result.isPageLevelMatch && !result.widget.isNull()) {
            applyHighlightToWidget(result.widget);
        }
        // For page-level matches, we just navigate without highlighting anything

        // Close popup only if requested (double-click or Enter)
        if (action == PopupAction::CloseAfter) {
            hideSearchResultsList();
        }
    }
}

void PreferencesSearchController::populateSearchResultsList()
{
    m_searchResultsList->clear();

    for (int i = 0; i < m_searchResults.size(); ++i) {
        const SearchResult& result = m_searchResults.at(i);

        // Create item without setting DisplayRole
        auto* item = new QListWidgetItem();

        // Store path and widget text in separate roles
        if (result.isPageLevelMatch) {
            // For page matches: parent group as header, page name as content
            item->setData(PathRole, result.tabName);
            item->setData(WidgetTextRole, result.pageDisplayName);
        }
        else {
            // For widget matches: full path as header, widget text as content
            QString pathText = result.tabName + QStringLiteral("/") + result.pageDisplayName;
            item->setData(PathRole, pathText);
            item->setData(WidgetTextRole, result.matchText);
        }
        item->setData(Qt::UserRole, i);  // Keep existing index storage

        m_searchResultsList->addItem(item);
    }

    // Select first actual item (not separator)
    if (!m_searchResults.isEmpty()) {
        m_searchResultsList->setCurrentRow(0);
    }
}

void PreferencesSearchController::hideSearchResultsList()
{
    m_searchResultsList->setVisible(false);
}

void PreferencesSearchController::showSearchResultsList()
{
    // Configure popup size and position
    configurePopupSize();

    // Show the popup
    m_searchResultsList->setVisible(true);
    m_searchResultsList->raise();

    // Use QTimer to ensure focus returns to search box after Qt finishes processing the popup show event
    QTimer::singleShot(0, this, [this]() {
        if (m_searchBox) {
            m_searchBox->setFocus();
            m_searchBox->activateWindow();
        }
    });
}

QString PreferencesSearchController::findGroupBoxForWidget(QWidget* widget)
{
    if (!widget) {
        return {};
    }

    // Walk up the parent hierarchy to find a QGroupBox
    QWidget* parent = widget->parentWidget();
    while (parent) {
        auto* groupBox = qobject_cast<QGroupBox*>(parent);
        if (groupBox) {
            return groupBox->title();
        }
        parent = parent->parentWidget();
    }

    return {};
}


template<typename WidgetType>
void PreferencesSearchController::searchWidgetType(
    QWidget* parentWidget,
    const QString& searchText,
    const QString& groupName,
    const QString& pageName,
    const QString& pageDisplayName,
    const QString& tabName
)
{
    const QList<WidgetType*> widgets = parentWidget->findChildren<WidgetType*>();

    for (WidgetType* widget : widgets) {
        QString widgetText;

        // Get text based on widget type
        if constexpr (std::is_same_v<WidgetType, QLabel>) {
            widgetText = widget->text();
        }
        else if constexpr (std::is_same_v<WidgetType, QCheckBox>) {
            widgetText = widget->text();
        }
        else if constexpr (std::is_same_v<WidgetType, QGroupBox>) {
            widgetText = widget->title();
        }
        else if constexpr (std::is_same_v<WidgetType, QRadioButton>) {
            widgetText = widget->text();
        }
        else if constexpr (std::is_same_v<WidgetType, QPushButton>) {
            widgetText = widget->text();
        }

        if (!widgetText.isEmpty()) {
            int score = 0;
            if (fuzzyMatch(searchText, widgetText, score)) {
                SearchResult result {
                    .groupName = groupName,
                    .pageName = pageName,
                    .widget = widget,
                    .matchText = widgetText,
                    .groupBoxName = findGroupBoxForWidget(widget),
                    .tabName = tabName,
                    .pageDisplayName = pageDisplayName,
                    .isPageLevelMatch = false,
                    .score = score
                };
                m_searchResults.append(result);
            }
        }

        // search tooltip text for all widget types
        QString tooltip = widget->toolTip();
        if (!tooltip.isEmpty()) {
            int tooltipScore = 0;
            if (fuzzyMatch(searchText, tooltip, tooltipScore)) {
                SearchResult result {
                    .groupName = groupName,
                    .pageName = pageName,
                    .widget = widget,
                    .matchText = QStringLiteral("Tooltip: ") + tooltip,
                    .groupBoxName = findGroupBoxForWidget(widget),
                    .tabName = tabName,
                    .pageDisplayName = pageDisplayName,
                    .isPageLevelMatch = false,
                    .score = tooltipScore - 100  // lower score for tooltip match
                };
                m_searchResults.append(result);
            }
        }

        // search throughout combobox items
        if constexpr (std::is_same_v<WidgetType, QComboBox>) {
            for (int i = 0; i < widget->count(); ++i) {
                QString itemText = widget->itemText(i);
                if (!itemText.isEmpty()) {
                    int itemScore = 0;
                    if (fuzzyMatch(searchText, itemText, itemScore)) {
                        SearchResult result {
                            .groupName = groupName,
                            .pageName = pageName,
                            .widget = widget,
                            .matchText = QStringLiteral("Option: ") + itemText,
                            .groupBoxName = findGroupBoxForWidget(widget),
                            .tabName = tabName,
                            .pageDisplayName = pageDisplayName,
                            .isPageLevelMatch = false,
                            .score = itemScore + 50  // boost score for combo item
                        };
                        m_searchResults.append(result);
                    }
                }
            }
        }
    }
}

int PreferencesSearchController::calculatePopupHeight(int popupWidth) const
{
    int totalHeight = 0;
    int itemCount = m_searchResultsList->count();
    int visibleItemCount = 0;
    const int maxVisibleItems = 4;

    for (int i = 0; i < itemCount && visibleItemCount < maxVisibleItems; ++i) {
        QListWidgetItem* item = m_searchResultsList->item(i);
        if (!item) {
            continue;
        }

        // For separator items, use their widget height
        if (m_searchResultsList->itemWidget(item)) {
            totalHeight += m_searchResultsList->itemWidget(item)->sizeHint().height();
        }
        else {
            // For text items, use the delegate's size hint instead of calculating manually
            QStyleOptionViewItem option;
            option.rect = QRect(0, 0, popupWidth, 100);  // Temporary rect for calculation
            option.font = m_searchResultsList->font();

            QSize delegateSize = m_searchResultsList->itemDelegate()->sizeHint(
                option,
                m_searchResultsList->model()->index(i, 0)
            );
            totalHeight += delegateSize.height();

            visibleItemCount++;  // Only count actual items, not separators
        }
    }

    return qMax(50, totalHeight);  // Minimum 50px height
}

void PreferencesSearchController::configurePopupSize()
{
    if (m_searchResults.isEmpty()) {
        hideSearchResultsList();
        return;
    }

    // Set a fixed width to prevent flashing when content changes
    int popupWidth = 300;  // Fixed width for consistent appearance
    m_searchResultsList->setFixedWidth(popupWidth);

    // Calculate and set the height
    int finalHeight = calculatePopupHeight(popupWidth);
    m_searchResultsList->setFixedHeight(finalHeight);

    // Position the popup's upper-left corner at the upper-right corner of the search box
    QPoint globalPos = m_searchBox->mapToGlobal(QPoint(m_searchBox->width(), 0));

    // Check if popup would go off-screen to the right
    QScreen* screen = QApplication::screenAt(globalPos);
    if (!screen) {
        screen = QApplication::primaryScreen();
    }
    QRect screenGeometry = screen->availableGeometry();

    // If popup would extend beyond right edge of screen, position it below the search box instead
    if (globalPos.x() + popupWidth > screenGeometry.right()) {
        globalPos = m_searchBox->mapToGlobal(QPoint(0, m_searchBox->height()));
    }

    m_searchResultsList->move(globalPos);
}

// Fuzzy search implementation

bool PreferencesSearchController::isExactMatch(const QString& searchText, const QString& targetText)
{
    return targetText.toLower().contains(searchText.toLower());
}

bool PreferencesSearchController::fuzzyMatch(
    const QString& searchText,
    const QString& targetText,
    int& score
)
{
    if (searchText.isEmpty()) {
        score = 0;
        return true;
    }

    const QString lowerSearch = searchText.toLower();
    const QString lowerTarget = targetText.toLower();

    // First check for exact substring match (highest score)
    if (lowerTarget.contains(lowerSearch)) {
        // Score based on how early the match appears and how much of the string it covers
        int matchIndex = lowerTarget.indexOf(lowerSearch);
        int coverage = (lowerSearch.length() * 100) / lowerTarget.length();  // Percentage coverage
        score = 1000 - matchIndex + coverage;  // Higher score for earlier matches and better coverage
        return true;
    }

    // For fuzzy matching, require minimum search length to avoid too many false positives
    if (lowerSearch.length() < 3) {
        score = 0;
        return false;
    }

    // Fuzzy matching: check if all characters appear in order
    int searchIndex = 0;
    int targetIndex = 0;
    int consecutiveMatches = 0;
    int maxConsecutive = 0;
    int firstMatchIndex = -1;
    int lastMatchIndex = -1;

    while (searchIndex < lowerSearch.length() && targetIndex < lowerTarget.length()) {
        if (lowerSearch[searchIndex] == lowerTarget[targetIndex]) {
            if (firstMatchIndex == -1) {
                firstMatchIndex = targetIndex;
            }
            lastMatchIndex = targetIndex;
            searchIndex++;
            consecutiveMatches++;
            maxConsecutive = qMax(maxConsecutive, consecutiveMatches);
        }
        else {
            consecutiveMatches = 0;
        }
        targetIndex++;
    }

    // Check if all search characters were found
    if (searchIndex == lowerSearch.length()) {
        // Calculate match density - how spread out are the matches?
        int matchSpan = lastMatchIndex - firstMatchIndex + 1;
        int density = (lowerSearch.length() * 100) / matchSpan;  // Characters per span

        // Require minimum density - matches shouldn't be too spread out
        if (density < 20) {  // Less than 20% density is too sparse
            score = 0;
            return false;
        }

        // Require minimum coverage of search term
        int coverage = (lowerSearch.length() * 100) / lowerTarget.length();
        if (coverage < 15 && lowerTarget.length() > 20) {  // For long strings, require better coverage
            score = 0;
            return false;
        }

        // Calculate score based on:
        // - Match density (how compact the matches are)
        // - Consecutive matches bonus
        // - Coverage (how much of target string is the search term)
        // - Position bonus (earlier matches are better)
        int densityScore = qMin(density, 100);  // Cap at 100
        int consecutiveBonus = (maxConsecutive * 30) / lowerSearch.length();
        int coverageScore = qMin(coverage * 2, 100);        // Coverage is important
        int positionBonus = qMax(0, 50 - firstMatchIndex);  // Earlier is better

        score = densityScore + consecutiveBonus + coverageScore + positionBonus;

        // Minimum score threshold for fuzzy matches
        if (score < 80) {
            score = 0;
            return false;
        }

        return true;
    }

    score = 0;
    return false;
}

void PreferencesSearchController::ensureSearchBoxFocus()
{
    if (m_searchBox && !m_searchBox->hasFocus()) {
        m_searchBox->setFocus();
    }
}

QString PreferencesSearchController::getHighlightStyleForWidget(QWidget* widget)
{
    const auto baseStyle = QStringLiteral(
        "background-color: #E3F2FD; color: #1565C0; border: 2px solid #2196F3; border-radius: 3px;"
    );

    if (qobject_cast<QLabel*>(widget)) {
        return QStringLiteral("QLabel { ") + baseStyle + QStringLiteral(" padding: 2px; }");
    }

    if (qobject_cast<QCheckBox*>(widget)) {
        return QStringLiteral("QCheckBox { ") + baseStyle + QStringLiteral(" padding: 2px; }");
    }

    if (qobject_cast<QRadioButton*>(widget)) {
        return QStringLiteral("QRadioButton { ") + baseStyle + QStringLiteral(" padding: 2px; }");
    }

    if (qobject_cast<QGroupBox*>(widget)) {
        return QStringLiteral("QGroupBox::title { ") + baseStyle + QStringLiteral(" padding: 2px; }");
    }

    if (qobject_cast<QPushButton*>(widget)) {
        return QStringLiteral("QPushButton { ") + baseStyle + QStringLiteral(" }");
    }

    return QStringLiteral("QWidget { ") + baseStyle + QStringLiteral(" padding: 2px; }");
}

void PreferencesSearchController::applyHighlightToWidget(QWidget* widget)
{
    if (!widget) {
        return;
    }

    m_originalStyles[widget] = widget->styleSheet();
    widget->setStyleSheet(getHighlightStyleForWidget(widget));
    m_highlightedWidgets.append(widget);
}

bool PreferencesSearchController::handleSearchBoxKeyPress(QKeyEvent* keyEvent)
{
    if (!m_searchResultsList->isVisible() || m_searchResults.isEmpty()) {
        return false;
    }

    switch (keyEvent->key()) {
        case Qt::Key_Down: {
            // Move selection down in popup, skipping separators
            const int currentRow = m_searchResultsList->currentRow();
            const int totalItems = m_searchResultsList->count();
            for (int i = 1; i < totalItems; ++i) {
                const int nextRow = (currentRow + i) % totalItems;
                if (QListWidgetItem* item = m_searchResultsList->item(nextRow);
                    item && (item->flags() & Qt::ItemIsSelectable)) {
                    m_searchResultsList->setCurrentRow(nextRow);
                    break;
                }
            }
            return true;
        }
        case Qt::Key_Up: {
            // Move selection up in popup, skipping separators
            const int currentRow = m_searchResultsList->currentRow();
            const int totalItems = m_searchResultsList->count();
            for (int i = 1; i < totalItems; ++i) {
                int prevRow = (currentRow - i + totalItems) % totalItems;
                QListWidgetItem* item = m_searchResultsList->item(prevRow);
                if (item && (item->flags() & Qt::ItemIsSelectable)) {
                    m_searchResultsList->setCurrentRow(prevRow);
                    break;
                }
            }
            return true;
        }
        case Qt::Key_Return:
        case Qt::Key_Enter:
            navigateToCurrentSearchResult(PopupAction::CloseAfter);
            return true;
        case Qt::Key_Escape:
            hideSearchResultsList();
            return true;
        default:
            return false;
    }
}

bool PreferencesSearchController::handlePopupKeyPress(const QKeyEvent* keyEvent)
{
    switch (keyEvent->key()) {
        case Qt::Key_Return:
        case Qt::Key_Enter:
            navigateToCurrentSearchResult(PopupAction::CloseAfter);
            return true;
        case Qt::Key_Escape:
            hideSearchResultsList();
            ensureSearchBoxFocus();
            return true;
        default:
            return false;
    }
}

bool PreferencesSearchController::isClickOutsidePopup(const QMouseEvent* mouseEvent) const
{
#if QT_VERSION < QT_VERSION_CHECK(6, 0, 0)
    QPoint globalPos = mouseEvent->globalPos();
#else
    const QPoint globalPos = mouseEvent->globalPosition().toPoint();
#endif
    const auto searchBoxRect = QRect(m_searchBox->mapToGlobal(QPoint(0, 0)), m_searchBox->size());
    auto popupRect = QRect(m_searchResultsList->mapToGlobal(QPoint(0, 0)), m_searchResultsList->size());

    return !searchBoxRect.contains(globalPos) && !popupRect.contains(globalPos);
}

bool DlgPreferencesImp::eventFilter(QObject* obj, QEvent* event)
{
    // Handle search box key presses
    if (obj == ui->searchBox && event->type() == QEvent::KeyPress) {
        auto* keyEvent = static_cast<QKeyEvent*>(event);
        return m_searchController->handleSearchBoxKeyPress(keyEvent);
    }

    // Handle popup key presses
    if (obj == m_searchController->getSearchResultsList() && event->type() == QEvent::KeyPress) {
        auto* keyEvent = static_cast<QKeyEvent*>(event);
        return m_searchController->handlePopupKeyPress(keyEvent);
    }

    // Prevent popup from stealing focus
    if (obj == m_searchController->getSearchResultsList() && event->type() == QEvent::FocusIn) {
        m_searchController->ensureSearchBoxFocus();
        return true;
    }

    // Handle search box focus loss
    if (obj == ui->searchBox && event->type() == QEvent::FocusOut) {
        auto* focusEvent = static_cast<QFocusEvent*>(event);
        if (focusEvent->reason() != Qt::PopupFocusReason
            && focusEvent->reason() != Qt::MouseFocusReason) {
            // Only hide if focus is going somewhere else, not due to popup interaction
            QTimer::singleShot(100, this, [this]() {
                if (!ui->searchBox->hasFocus() && !m_searchController->isPopupUnderMouse()) {
                    m_searchController->hideSearchResultsList();
                }
            });
        }
    }

    // Handle clicks outside popup
    if (event->type() == QEvent::MouseButtonPress) {
        auto* mouseEvent = static_cast<QMouseEvent*>(event);
        auto widget = qobject_cast<QWidget*>(obj);

        // Check if click is outside search area
        if (m_searchController->isPopupVisible() && obj != m_searchController->getSearchResultsList()
            && obj != ui->searchBox && widget &&  // Only check if obj is actually a QWidget
            !m_searchController->isPopupAncestorOf(widget) && !ui->searchBox->isAncestorOf(widget)) {

            if (m_searchController->isClickOutsidePopup(mouseEvent)) {
                m_searchController->hideSearchResultsList();
                m_searchController->clearHighlights();
            }
        }
    }

    return QDialog::eventFilter(obj, event);
}

#include "moc_DlgPreferencesImp.cpp"
