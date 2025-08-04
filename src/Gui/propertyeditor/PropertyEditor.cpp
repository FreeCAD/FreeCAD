/***************************************************************************
 *   Copyright (c) 2004 Werner Mayer <wmayer[at]users.sourceforge.net>     *
 *                                                                         *
 *   This file is part of the FreeCAD CAx development system.              *
 *                                                                         *
 *   This library is free software; you can redistribute it and/or         *
 *   modify it under the terms of the GNU Library General Public           *
 *   License as published by the Free Software Foundation; either          *
 *   version 2 of the License, or (at your option) any later version.      *
 *                                                                         *
 *   This library  is distributed in the hope that it will be useful,      *
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of        *
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the         *
 *   GNU Library General Public License for more details.                  *
 *                                                                         *
 *   You should have received a copy of the GNU Library General Public     *
 *   License along with this library; see the file COPYING.LIB. If not,    *
 *   write to the Free Software Foundation, Inc., 59 Temple Place,         *
 *   Suite 330, Boston, MA  02111-1307, USA                                *
 *                                                                         *
 ***************************************************************************/


#include "PreCompiled.h"

#ifndef _PreComp_
#include <boost/algorithm/string/predicate.hpp>
#include <QApplication>
#include <QClipboard>
#include <QInputDialog>
#include <QHeaderView>
#include <QMenu>
#include <QPainter>
#endif

#include <App/Application.h>
#include <App/AutoTransaction.h>
#include <App/Document.h>
#include <Base/Console.h>
#include <Base/Tools.h>

#include "PropertyEditor.h"
#include "Dialogs/DlgAddProperty.h"
#include "MainWindow.h"
#include "PropertyItemDelegate.h"
#include "PropertyModel.h"
#include "PropertyView.h"
#include "ViewProviderDocumentObject.h"


FC_LOG_LEVEL_INIT("PropertyView", true, true)

using namespace Gui::PropertyEditor;

PropertyEditor::PropertyEditor(QWidget* parent)
    : QTreeView(parent)
    , autoexpand(false)
    , autoupdate(false)
    , committing(false)
    , delaybuild(false)
    , binding(false)
    , checkDocument(false)
    , closingEditor(false)
    , dragInProgress(false)
{
    propertyModel = new PropertyModel(this);
    setModel(propertyModel);

    setVerticalScrollMode(QAbstractItemView::ScrollPerPixel);
    setHorizontalScrollMode(QAbstractItemView::ScrollPerPixel);

    delegate = new PropertyItemDelegate(this);
    delegate->setItemEditorFactory(new PropertyItemEditorFactory);
    setItemDelegate(delegate);

    setAlternatingRowColors(true);
    setRootIsDecorated(false);
    setExpandsOnDoubleClick(true);

#if QT_VERSION < QT_VERSION_CHECK(6, 0, 0)
    QStyleOptionViewItem opt = PropertyEditor::viewOptions();
#else
    QStyleOptionViewItem opt;
    initViewItemOption(&opt);
#endif
    this->background = opt.palette.dark();
    this->groupColor = opt.palette.color(QPalette::BrightText);

    this->_itemBackground.setColor(QColor(0, 0, 0, 0));

    this->setSelectionMode(QAbstractItemView::ExtendedSelection);

    // clang-format off
    connect(this, &QTreeView::activated, this, &PropertyEditor::onItemActivated);
    connect(this, &QTreeView::clicked, this, &PropertyEditor::onItemActivated);
    connect(this, &QTreeView::expanded, this, &PropertyEditor::onItemExpanded);
    connect(this, &QTreeView::collapsed, this, &PropertyEditor::onItemCollapsed);
    connect(propertyModel, &QAbstractItemModel::rowsMoved, this, &PropertyEditor::onRowsMoved);
    connect(propertyModel, &QAbstractItemModel::rowsRemoved, this, &PropertyEditor::onRowsRemoved);
    // clang-format on

    setHeaderHidden(true);
    viewport()->installEventFilter(this);
    viewport()->setMouseTracking(true);

    auto hGrp = App::GetApplication().GetParameterGroupByPath(
        "User parameter:BaseApp/Preferences/DockWindows/PropertyView");
    int firstColumnSize = hGrp->GetInt("FirstColumnSize", 0);
    if (firstColumnSize != 0) {
        header()->resizeSection(0, firstColumnSize);
    }
}

PropertyEditor::~PropertyEditor()
{
    QItemEditorFactory* f = delegate->itemEditorFactory();
    delegate->setItemEditorFactory(nullptr);
    delete f;
}

void PropertyEditor::setAutomaticExpand(bool v)
{
    autoexpand = v;
}

bool PropertyEditor::isAutomaticExpand(bool) const
{
    return autoexpand;
}

void PropertyEditor::onItemExpanded(const QModelIndex& index)
{
    auto item = static_cast<PropertyItem*>(index.internalPointer());
    item->setExpanded(true);
    for (int i = 0, c = item->childCount(); i < c; ++i) {
        setExpanded(propertyModel->index(i, 0, index), item->child(i)->isExpanded());
    }
}

void PropertyEditor::onItemCollapsed(const QModelIndex& index)
{
    auto item = static_cast<PropertyItem*>(index.internalPointer());
    item->setExpanded(false);
}

void PropertyEditor::setAutomaticDocumentUpdate(bool v)
{
    autoupdate = v;
}

bool PropertyEditor::isAutomaticDocumentUpdate(bool) const
{
    return autoupdate;
}

QBrush PropertyEditor::groupBackground() const
{
    return this->background;
}

void PropertyEditor::setGroupBackground(const QBrush& c)
{
    this->background = c;
}

QColor PropertyEditor::groupTextColor() const
{
    return this->groupColor;
}

void PropertyEditor::setGroupTextColor(const QColor& c)
{
    this->groupColor = c;
}

QBrush PropertyEditor::itemBackground() const
{
    return this->_itemBackground;
}

void PropertyEditor::setItemBackground(const QBrush& c)
{
    this->_itemBackground = c;
}

#if QT_VERSION < QT_VERSION_CHECK(6, 0, 0)
QStyleOptionViewItem PropertyEditor::viewOptions() const
{
    QStyleOptionViewItem option = QTreeView::viewOptions();
    option.showDecorationSelected = true;
    return option;
}
#else
void PropertyEditor::initViewItemOption(QStyleOptionViewItem* option) const
{
    QTreeView::initViewItemOption(option);
    option->showDecorationSelected = true;
}
#endif

bool PropertyEditor::event(QEvent* event)
{
    if (event->type() == QEvent::ShortcutOverride) {
        auto kevent = static_cast<QKeyEvent*>(event);
        Qt::KeyboardModifiers ShiftKeypadModifier = Qt::ShiftModifier | Qt::KeypadModifier;
        if (kevent->modifiers() == Qt::NoModifier || kevent->modifiers() == Qt::ShiftModifier
            || kevent->modifiers() == Qt::KeypadModifier
            || kevent->modifiers() == ShiftKeypadModifier) {
            switch (kevent->key()) {
                case Qt::Key_Delete:
                case Qt::Key_Home:
                case Qt::Key_End:
                case Qt::Key_Backspace:
                case Qt::Key_Left:
                case Qt::Key_Right:
                    kevent->accept();
                default:
                    break;
            }
        }
    }
    return QTreeView::event(event);
}

void PropertyEditor::commitData(QWidget* editor)
{
    committing = true;
    QTreeView::commitData(editor);
    committing = false;
    if (delaybuild) {
        delaybuild = false;
        propertyModel->buildUp(PropertyModel::PropertyList());
    }
}

void PropertyEditor::editorDestroyed(QObject* editor)
{
    QTreeView::editorDestroyed(editor);

    // When editing expression through context menu, the editor (ExpLineEditor)
    // deletes itself when finished, so it won't trigger closeEditor signal. We
    // must handle it here to perform auto update.
    closeTransaction();
}

void PropertyEditor::currentChanged(const QModelIndex& current, const QModelIndex& previous)
{
    FC_LOG("current changed " << current.row() << "," << current.column() << "  " << previous.row()
                              << "," << previous.column());

    QTreeView::currentChanged(current, previous);

    // if (previous.isValid())
    //     closePersistentEditor(model()->buddy(previous));

    // DO NOT activate editor here, use onItemActivate() which response to
    // signals of activated and clicked.
    //
    // if (current.isValid())
    //     openPersistentEditor(model()->buddy(current));
}

void PropertyEditor::closeEditor()
{
    if (editingIndex.isValid()) {
        Base::StateLocker guard(closingEditor);
        bool hasFocus = activeEditor && activeEditor->hasFocus();
#ifdef Q_OS_MACOS
        // Brute-force workaround for https://github.com/FreeCAD/FreeCAD/issues/14350
        int currentIndex = 0;
        QTabBar* tabBar = nullptr;
        if (auto mdiArea = Gui::MainWindow::getInstance()->findChild<QMdiArea*>()) {
            tabBar = mdiArea->findChild<QTabBar*>();
            if (tabBar) {
                currentIndex = tabBar->currentIndex();
            }
        }
#endif
        closePersistentEditor(editingIndex);
#ifdef Q_OS_MACOS
        if (tabBar) {
            tabBar->setCurrentIndex(currentIndex);
        }
#endif
        editingIndex = QPersistentModelIndex();
        activeEditor = nullptr;
        if (hasFocus) {
            setFocus();
        }
    }
}

void PropertyEditor::openEditor(const QModelIndex& index)
{
    if (editingIndex == index && activeEditor) {
        return;
    }

    closeEditor();

    openPersistentEditor(model()->buddy(index));

    if (!editingIndex.isValid() || !autoupdate) {
        return;
    }

    auto& app = App::GetApplication();
    if (app.getActiveTransaction()) {
        FC_LOG("editor already transacting " << app.getActiveTransaction());
        return;
    }
    auto item = static_cast<PropertyItem*>(editingIndex.internalPointer());
    auto items = item->getPropertyData();
    for (auto propItem = item->parent(); items.empty() && propItem; propItem = propItem->parent()) {
        items = propItem->getPropertyData();
    }
    if (items.empty()) {
        FC_LOG("editor no item");
        return;
    }
    auto prop = items[0];
    auto parent = prop->getContainer();
    auto obj = freecad_cast<App::DocumentObject*>(parent);
    if (!obj) {
        auto view = freecad_cast<ViewProviderDocumentObject*>(parent);
        if (view) {
            obj = view->getObject();
        }
    }
    if (!obj || !obj->getDocument()) {
        FC_LOG("invalid object");
        return;
    }
    if (obj->getDocument()->hasPendingTransaction()) {
        FC_LOG("pending transaction");
        return;
    }
    std::ostringstream str;
    str << tr("Edit").toUtf8().constData() << ' ';
    for (auto prop : items) {
        if (prop->getContainer() != obj) {
            obj = nullptr;
            break;
        }
    }
    if (obj && obj->isAttachedToDocument()) {
        str << obj->getNameInDocument() << '.';
    }
    else {
        str << tr("property").toUtf8().constData() << ' ';
    }
    str << prop->getName();
    if (items.size() > 1) {
        str << "...";
    }
    transactionID = app.setActiveTransaction(str.str().c_str());
    FC_LOG("editor transaction " << app.getActiveTransaction());
}

void PropertyEditor::onItemActivated(const QModelIndex& index)
{
    if (index.column() != 1) {
        return;
    }
    openEditor(index);
}

void PropertyEditor::recomputeDocument(App::Document* doc)
{
    try {
        if (doc && !doc->isTransactionEmpty()) {
            // Between opening and committing a transaction a recompute
            // could already have been done
            if (doc->isTouched()) {
                doc->recompute();
            }
        }
    }
    // do not re-throw
    catch (const Base::Exception& e) {
        e.reportException();
    }
    catch (const std::exception& e) {
        Base::Console().error(
            "Unhandled std::exception caught in PropertyEditor::recomputeDocument.\n"
            "The error message is: %s\n",
            e.what());
    }
    catch (...) {
        Base::Console().error(
            "Unhandled unknown exception caught in PropertyEditor::recomputeDocument.\n");
    }
}

void PropertyEditor::closeTransaction()
{
    int tid = 0;
    if (App::GetApplication().getActiveTransaction(&tid) && tid == transactionID) {
        if (autoupdate) {
            App::Document* doc = App::GetApplication().getActiveDocument();
            recomputeDocument(doc);
        }
        App::GetApplication().closeActiveTransaction();
    }
}

void PropertyEditor::closeEditor(QWidget* editor, QAbstractItemDelegate::EndEditHint hint)
{
    if (closingEditor) {
        return;
    }

    if (removingRows) {
        // When removing rows, QTreeView will temporary hide the editor which
        // will trigger Event::FocusOut and subsequently trigger call of
        // closeEditor() here. Since we are using persistent editor, QTreeView
        // will not destroy the editor. But we still needs to call
        // QTreeView::closeEditor() here, in case the editor belongs to the
        // removed rows.
        QTreeView::closeEditor(editor, hint);
        return;
    }

    closeTransaction();

    // If we are not removing rows, then QTreeView::closeEditor() does nothing
    // because we are using persistent editor, so we have to call our own
    // version of closeEditor()
    this->closeEditor();

    QModelIndex indexSaved = currentIndex();

    if (indexSaved.column() == 0) {
        // Calling setCurrentIndex() to make sure we focus on column 1 instead of 0.
        setCurrentIndex(propertyModel->buddy(indexSaved));
    }

    QModelIndex lastIndex = indexSaved;
    bool wrapped = false;
    do {
        QModelIndex index;
        if (hint == QAbstractItemDelegate::EditNextItem) {
            index = moveCursor(MoveDown, Qt::NoModifier);
        }
        else if (hint == QAbstractItemDelegate::EditPreviousItem) {
            index = moveCursor(MoveUp, Qt::NoModifier);
        }
        else {
            break;
        }
        if (!index.isValid() || index == lastIndex) {
            if (wrapped) {
                setCurrentIndex(propertyModel->buddy(indexSaved));
                break;
            }
            wrapped = true;
            if (hint == QAbstractItemDelegate::EditNextItem) {
                index = moveCursor(MoveHome, Qt::NoModifier);
            }
            else {
                index = moveCursor(MoveEnd, Qt::NoModifier);
            }
            if (!index.isValid() || index == indexSaved) {
                break;
            }
        }
        lastIndex = index;
        setCurrentIndex(propertyModel->buddy(index));

        auto item = static_cast<PropertyItem*>(index.internalPointer());
        // Skip readonly item, because the editor will be disabled and hence
        // does not accept focus, and in turn break Tab/Backtab navigation.
        if (item && item->isReadOnly()) {
            continue;
        }

        openEditor(index);

    } while (!editingIndex.isValid());
}

void PropertyEditor::reset()
{
    QTreeView::reset();

    closeTransaction();

    QModelIndex parent;
    int numRows = propertyModel->rowCount(parent);
    for (int i = 0; i < numRows; ++i) {
        QModelIndex index = propertyModel->index(i, 0, parent);
        auto item = static_cast<PropertyItem*>(index.internalPointer());
        if (item->childCount() == 0) {
            if (item->isSeparator()) {
                setRowHidden(i, parent, true);
            }
        }
        else {
            setEditorMode(index, 0, item->childCount() - 1);
        }
        if (item->isExpanded()) {
            setExpanded(index, true);
        }
    }
}

void PropertyEditor::onRowsMoved(const QModelIndex& parent,
                                 int start,
                                 int end,
                                 const QModelIndex& dst,
                                 int)
{
    if (parent != dst) {
        auto item = static_cast<PropertyItem*>(parent.internalPointer());
        if (item && item->isSeparator() && item->childCount() == 0) {
            setRowHidden(parent.row(), propertyModel->parent(parent), true);
        }
        item = static_cast<PropertyItem*>(dst.internalPointer());
        if (item && item->isSeparator() && item->childCount() == end - start + 1) {
            setRowHidden(dst.row(), propertyModel->parent(dst), false);
            setExpanded(dst, true);
        }
    }
}

void PropertyEditor::rowsInserted(const QModelIndex& parent, int start, int end)
{
    QTreeView::rowsInserted(parent, start, end);

    auto item = static_cast<PropertyItem*>(parent.internalPointer());
    if (item && item->isSeparator() && item->childCount() == end - start + 1) {
        setRowHidden(parent.row(), propertyModel->parent(parent), false);
        if (item->isExpanded()) {
            setExpanded(parent, true);
        }
    }

    for (int i = start; i < end; ++i) {
        QModelIndex index = propertyModel->index(i, 0, parent);
        auto child = static_cast<PropertyItem*>(index.internalPointer());
        if (child->isSeparator()) {
            // Set group header rows to span all columns
            setFirstColumnSpanned(i, parent, true);
        }
        if (child->isExpanded()) {
            setExpanded(index, true);
        }
    }

    if (parent.isValid()) {
        setEditorMode(parent, start, end);
    }
}

void PropertyEditor::rowsAboutToBeRemoved(const QModelIndex& parent, int start, int end)
{
    QTreeView::rowsAboutToBeRemoved(parent, start, end);

    auto item = static_cast<PropertyItem*>(parent.internalPointer());
    if (item && item->isSeparator() && item->childCount() == end - start + 1) {
        setRowHidden(parent.row(), propertyModel->parent(parent), true);
    }

    if (editingIndex.isValid()) {
        if (editingIndex.row() >= start && editingIndex.row() <= end) {
            closeTransaction();
        }
        else {
            removingRows = 1;
            for (QWidget* w = qApp->focusWidget(); w; w = w->parentWidget()) {
                if (w == activeEditor) {
                    removingRows = -1;
                    break;
                }
            }
        }
    }
}

void PropertyEditor::onRowsRemoved(const QModelIndex&, int, int)
{
    if (removingRows < 0 && activeEditor) {
        activeEditor->setFocus();
    }
    removingRows = 0;
}

void PropertyEditor::drawBranches(QPainter* painter,
                                  const QRect& rect,
                                  const QModelIndex& index) const
{
    QTreeView::drawBranches(painter, rect, index);

    auto property = static_cast<PropertyItem*>(index.internalPointer());

    if (property && property->isSeparator()) {
        painter->fillRect(rect, this->background);
    }
}

void Gui::PropertyEditor::PropertyEditor::drawRow(QPainter* painter,
                                                  const QStyleOptionViewItem& options,
                                                  const QModelIndex& index) const
{
    // render background also for non alternate rows based on the `itemBackground` property.
    painter->fillRect(options.rect, itemBackground());

    QTreeView::drawRow(painter, options, index);
}

void PropertyEditor::buildUp(PropertyModel::PropertyList&& props, bool _checkDocument)
{
    checkDocument = _checkDocument;

    if (committing) {
        Base::Console().warning(
            "While committing the data to the property the selection has changed.\n");
        delaybuild = true;
        return;
    }

    // Do not close transaction here, because we are now doing incremental
    // update in PropertyModel::buildUp()
    //
    // closeTransaction();

    QModelIndex index = this->currentIndex();
    QStringList propertyPath = propertyModel->propertyPathFromIndex(index);
    if (!propertyPath.isEmpty()) {
        this->selectedProperty = propertyPath;
    }
    propertyModel->buildUp(props);
    if (!this->selectedProperty.isEmpty()) {
        QModelIndex index = propertyModel->propertyIndexFromPath(this->selectedProperty);
        this->setCurrentIndex(index);
    }

    propList = std::move(props);
    propOwners.clear();
    for (auto& v : propList) {
        for (auto prop : v.second) {
            auto container = prop->getContainer();
            if (!container) {
                continue;
            }
            // Include document to get proper handling in PropertyView::slotDeleteDocument()
            if (checkDocument && container->isDerivedFrom<App::DocumentObject>()) {
                propOwners.insert(static_cast<App::DocumentObject*>(container)->getDocument());
            }
            propOwners.insert(container);
        }
    }

    if (autoexpand) {
        expandAll();
    }
}

void PropertyEditor::updateProperty(const App::Property& prop)
{
    // forward this to the model if the property is changed from outside
    if (!committing) {
        propertyModel->updateProperty(prop);
    }
}

void PropertyEditor::setEditorMode(const QModelIndex& parent, int start, int end)
{
    int column = 1;
    for (int i = start; i <= end; i++) {
        QModelIndex item = propertyModel->index(i, column, parent);
        auto propItem = static_cast<PropertyItem*>(item.internalPointer());
        if (!PropertyView::showAll() && propItem && propItem->testStatus(App::Property::Hidden)) {
            setRowHidden(i, parent, true);
        }
    }
}

void PropertyEditor::removeProperty(const App::Property& prop)
{
    for (PropertyModel::PropertyList::iterator it = propList.begin(); it != propList.end(); ++it) {
        // find the given property in the list and remove it if it's there
        std::vector<App::Property*>::iterator pos =
            std::ranges::find(it->second, &prop);
        if (pos != it->second.end()) {
            it->second.erase(pos);
            // if the last property of this name is removed then also remove the whole group
            if (it->second.empty()) {
                propList.erase(it);
            }
            propertyModel->removeProperty(prop);
            break;
        }
    }
}

enum MenuAction
{
    MA_AutoExpand,
    MA_ShowHidden,
    MA_Expression,
    MA_RemoveProp,
    MA_AddProp,
    MA_EditPropGroup,
    MA_Transient,
    MA_Output,
    MA_NoRecompute,
    MA_ReadOnly,
    MA_Hidden,
    MA_Touched,
    MA_EvalOnRestore,
    MA_CopyOnChange,
    MA_Copy,
};

void PropertyEditor::contextMenuEvent(QContextMenuEvent*)
{
    QMenu menu;
    QAction* autoExpand = nullptr;

    auto contextIndex = currentIndex();

    // acquiring the selected properties
    std::unordered_set<App::Property*> props;
    const auto indexes = selectedIndexes();
    for (const auto& index : indexes) {
        auto item = static_cast<PropertyItem*>(index.internalPointer());
        if (item->isSeparator()) {
            continue;
        }
        for (auto parent = item; parent; parent = parent->parent()) {
            const auto& ps = parent->getPropertyData();
            if (!ps.empty()) {
                props.insert(ps.begin(), ps.end());
                break;
            }
        }
        if (index.column() > 0) {
            continue;
        }
        const QVariant valueToCopy = contextIndex.data(Qt::DisplayRole);
        if (valueToCopy.isValid()) {
            QAction* copyAction = menu.addAction(tr("Copy"));
            copyAction->setData(QVariant(MA_Copy));
            menu.addSeparator();
        }
    }

    // add property
    menu.addAction(tr("Add Property"))->setData(QVariant(MA_AddProp));
    if (!props.empty() && std::all_of(props.begin(), props.end(), [](auto prop) {
            return prop->testStatus(App::Property::PropDynamic)
                && !boost::starts_with(prop->getName(), prop->getGroup());
        })) {
        menu.addAction(tr("Rename Property Group"))->setData(QVariant(MA_EditPropGroup));
    }

    // remove property
    bool canRemove = !props.empty();
    unsigned long propType = 0;
    unsigned long propStatus = 0xffffffff;
    for (auto prop : props) {
        propType |= prop->getType();
        propStatus &= prop->getStatus();
        if (!prop->testStatus(App::Property::PropDynamic)
            || prop->testStatus(App::Property::LockDynamic)) {
            canRemove = false;
        }
    }
    if (canRemove) {
        menu.addAction(tr("Remove Property"))->setData(QVariant(MA_RemoveProp));
    }

    // add a separator between adding/removing properties and the rest
    menu.addSeparator();

    // show all
    QAction* showHidden = menu.addAction(tr("Show Hidden"));
    showHidden->setCheckable(true);
    showHidden->setChecked(PropertyView::showAll());
    showHidden->setData(QVariant(MA_ShowHidden));

    // auto expand
    autoExpand = menu.addAction(tr("Auto-Expand"));
    autoExpand->setCheckable(true);
    autoExpand->setChecked(autoexpand);
    autoExpand->setData(QVariant(MA_AutoExpand));

    // expression
    if (props.size() == 1) {
        auto item = static_cast<PropertyItem*>(contextIndex.internalPointer());
        auto prop = *props.begin();
        if (item->isBound() && !prop->isDerivedFrom<App::PropertyExpressionEngine>()
            && !prop->isReadOnly() && !prop->testStatus(App::Property::Immutable)
            && !(prop->getType() & App::Prop_ReadOnly)) {
            contextIndex = propertyModel->buddy(contextIndex);
            setCurrentIndex(contextIndex);
            // menu.addSeparator();
            menu.addAction(tr("Expression…"))->setData(QVariant(MA_Expression));
        }
    }

    // the various flags
    if (!props.empty()) {
        menu.addSeparator();

        // the subMenu is allocated on the heap but managed by menu.
        auto subMenu = new QMenu(QStringLiteral("Status"), &menu);

        QAction* action;
        QString text;
#define _ACTION_SETUP(_name)                                                                       \
    do {                                                                                           \
        text = tr(#_name);                                                                         \
        action = subMenu->addAction(text);                                                         \
        action->setData(QVariant(MA_##_name));                                                     \
        action->setCheckable(true);                                                                \
        if (propStatus & (1 << App::Property::_name))                                              \
            action->setChecked(true);                                                              \
    } while (0)
#define ACTION_SETUP(_name)                                                                        \
    do {                                                                                           \
        _ACTION_SETUP(_name);                                                                      \
        if (propType & App::Prop_##_name) {                                                        \
            action->setText(text + QStringLiteral(" *"));                                          \
            action->setChecked(true);                                                              \
        }                                                                                          \
    } while (0)

        ACTION_SETUP(Hidden);
        ACTION_SETUP(Output);
        ACTION_SETUP(NoRecompute);
        ACTION_SETUP(ReadOnly);
        ACTION_SETUP(Transient);
        _ACTION_SETUP(Touched);
        _ACTION_SETUP(EvalOnRestore);
        _ACTION_SETUP(CopyOnChange);

        menu.addMenu(subMenu);
    }

    auto action = menu.exec(QCursor::pos());
    if (!action) {
        return;
    }

    switch (action->data().toInt()) {
        case MA_AutoExpand:
            if (autoExpand) {
                // Variable autoExpand should not be null when we arrive here, but
                // since we explicitly initialize the variable to nullptr, a check
                // nonetheless.
                autoexpand = autoExpand->isChecked();
                if (autoexpand) {
                    expandAll();
                }
            }
            return;
        case MA_ShowHidden:
            PropertyView::setShowAll(action->isChecked());
            return;
        case MA_Copy: {
            const QVariant valueToCopy = contextIndex.data(Qt::DisplayRole);
            if (valueToCopy.isValid()) {
                auto *clipboard = QApplication::clipboard();
                clipboard->setText(valueToCopy.toString());
            }
            return;
        }
#define ACTION_CHECK(_name)                                                                        \
    case MA_##_name:                                                                               \
        for (auto prop : props)                                                                    \
            prop->setStatus(App::Property::_name, action->isChecked());                            \
        break
            ACTION_CHECK(Transient);
            ACTION_CHECK(ReadOnly);
            ACTION_CHECK(Output);
            ACTION_CHECK(Hidden);
            ACTION_CHECK(EvalOnRestore);
            ACTION_CHECK(CopyOnChange);
        case MA_Touched:
            for (auto prop : props) {
                if (action->isChecked()) {
                    prop->touch();
                }
                else {
                    prop->purgeTouched();
                }
            }
            break;
        case MA_Expression:
            if (contextIndex == currentIndex()) {
                Base::FlagToggler<> flag(binding);
                closeEditor();
                openEditor(contextIndex);
            }
            break;
        case MA_AddProp: {
            App::AutoTransaction committer("Add property");
            std::unordered_set<App::PropertyContainer*> containers;
            auto sels = Gui::Selection().getSelection("*");
            if (sels.size() == 1) {
                containers.insert(sels[0].pObject);
            }
            else {
                for (auto prop : props) {
                    containers.insert(prop->getContainer());
                }
            }
            Gui::Dialog::DlgAddProperty dlg(Gui::getMainWindow(), std::move(containers));
            dlg.exec();
            return;
        }
        case MA_EditPropGroup: {
            // This operation is not undoable yet.
            const char* groupName = (*props.begin())->getGroup();
            if (!groupName) {
                groupName = "Base";
            }
            QString res = QInputDialog::getText(Gui::getMainWindow(),
                                                tr("Rename property group"),
                                                tr("Group name:"),
                                                QLineEdit::Normal,
                                                QString::fromUtf8(groupName));
            if (res.size()) {
                std::string group = res.toUtf8().constData();
                for (auto prop : props) {
                    prop->getContainer()->changeDynamicProperty(prop, group.c_str(), nullptr);
                }
                buildUp(PropertyModel::PropertyList(propList), checkDocument);
            }
            return;
        }
        case MA_RemoveProp: {
            App::AutoTransaction committer("Remove property");
            for (auto prop : props) {
                try {
                    prop->getContainer()->removeDynamicProperty(prop->getName());
                }
                catch (Base::Exception& e) {
                    e.reportException();
                }
            }
            break;
        }
        default:
            break;
    }
}


bool PropertyEditor::eventFilter(QObject* object, QEvent* event)
{
    if (object == viewport()) {
        QMouseEvent* mouse_event = dynamic_cast<QMouseEvent*>(event);
        if (mouse_event) {
            if (mouse_event->type() == QEvent::MouseMove) {
                if (dragInProgress) {  // apply dragging
                    QHeaderView* header_view = header();
                    int delta = mouse_event->pos().x() - dragPreviousPos;
                    dragPreviousPos = mouse_event->pos().x();
                    // using minimal size = dragSensibility * 2 to prevent collapsing
                    header_view->resizeSection(
                        dragSection,
                        qMax(dragSensibility * 2, header_view->sectionSize(dragSection) + delta));
                    return true;
                }
                else {  // set mouse cursor shape
                    if (indexResizable(mouse_event->pos()).isValid()) {
                        viewport()->setCursor(Qt::SplitHCursor);
                    }
                    else {
                        viewport()->setCursor(QCursor());
                    }
                }
            }
            else if (mouse_event->type() == QEvent::MouseButtonPress
                     && mouse_event->button() == Qt::LeftButton && !dragInProgress) {
                if (indexResizable(mouse_event->pos()).isValid()) {
                    dragInProgress = true;
#if QT_VERSION < QT_VERSION_CHECK(6, 0, 0)
                    dragPreviousPos = mouse_event->x();
#else
                    dragPreviousPos = mouse_event->position().toPoint().x();
#endif
                    dragSection = indexResizable(mouse_event->pos()).column();
                    return true;
                }
            }
            else if (mouse_event->type() == QEvent::MouseButtonRelease
                     && mouse_event->button() == Qt::LeftButton && dragInProgress) {
                dragInProgress = false;

                auto hGrp = App::GetApplication().GetParameterGroupByPath(
                    "User parameter:BaseApp/Preferences/DockWindows/PropertyView");
                hGrp->SetInt("FirstColumnSize", header()->sectionSize(0));
                return true;
            }
        }
    }
    return false;
}

QModelIndex PropertyEditor::indexResizable(QPoint mouse_pos)
{
    QModelIndex index = indexAt(mouse_pos - QPoint(dragSensibility + 1, 0));
    if (index.isValid()) {
        if (qAbs(visualRect(index).right() - mouse_pos.x()) < dragSensibility
            && header()->sectionResizeMode(index.column()) == QHeaderView::Interactive) {
            return index;
        }
    }
    return QModelIndex();
}

#include "moc_PropertyEditor.cpp"
