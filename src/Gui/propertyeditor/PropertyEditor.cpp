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
# include <QApplication>
# include <QPainter>
# include <QMenu>
# include <QDebug>
# include <QDialog>
# include <QMessageBox>
# include <QCheckBox>
# include <QInputDialog>
# include <QDesktopWidget>
#endif

#include <boost/algorithm/string/predicate.hpp>

#include <Base/Console.h>
#include <Base/Tools.h>
#include <App/Application.h>
#include <App/Document.h>
#include <App/AutoTransaction.h>
#include "ViewProviderDocumentObject.h"
#include "MainWindow.h"
#include "DlgAddProperty.h"
#include "PropertyEditor.h"
#include "PropertyItemDelegate.h"
#include "PropertyModel.h"
#include "PropertyView.h"
#include "Command.h"
#include "Action.h"

FC_LOG_LEVEL_INIT("PropertyView",true,true)

using namespace Gui::PropertyEditor;

PropertyEditor::PropertyEditor(QWidget *parent)
    : QTreeView(parent)
    , autoexpand(false)
    , autoupdate(false)
    , committing(false)
    , delaybuild(false)
    , binding(false)
    , checkDocument(false)
    , closingEditor(false)
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

    QStyleOptionViewItem opt = viewOptions();
    this->background = opt.palette.dark();
    this->groupColor = opt.palette.color(QPalette::BrightText);

    this->_itemBackground.setColor(QColor(0,0,0,0));

    this->setSelectionMode(QAbstractItemView::ExtendedSelection);

    connect(this, SIGNAL(activated(const QModelIndex &)), this, SLOT(onItemActivated(const QModelIndex &)));
    connect(this, SIGNAL(clicked(const QModelIndex &)), this, SLOT(onItemActivated(const QModelIndex &)));
    connect(this, SIGNAL(expanded(const QModelIndex &)), this, SLOT(onItemExpanded(const QModelIndex &)));
    connect(this, SIGNAL(collapsed(const QModelIndex &)), this, SLOT(onItemCollapsed(const QModelIndex &)));
    connect(propertyModel, SIGNAL(rowsMoved(const QModelIndex &, int, int, const QModelIndex &, int)),
            this, SLOT(onRowsMoved(const QModelIndex &, int, int, const QModelIndex &, int)));
    connect(propertyModel, SIGNAL(rowsRemoved(const QModelIndex &, int, int)),
            this, SLOT(onRowsRemoved(const QModelIndex &, int, int)));
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

void PropertyEditor::onItemExpanded(const QModelIndex &index)
{
    PropertyItem* item = static_cast<PropertyItem*>(index.internalPointer());
    item->setExpanded(true);
    for(int i=0, c=item->childCount(); i<c; ++i)
        setExpanded(propertyModel->index(i, 0, index), item->child(i)->isExpanded());
}

void PropertyEditor::collapseItem(const QModelIndex &index)
{
    PropertyItem* item = static_cast<PropertyItem*>(index.internalPointer());
    if (item->isSeparator())
        return;
    item->setExpanded(false);
    for(int i=0, c=item->childCount(); i<c; ++i)
        setExpanded(propertyModel->index(i, 0, index), false);
}

void PropertyEditor::onItemCollapsed(const QModelIndex &index)
{
    PropertyItem* item = static_cast<PropertyItem*>(index.internalPointer());
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

QStyleOptionViewItem PropertyEditor::viewOptions() const
{
    QStyleOptionViewItem option = QTreeView::viewOptions();
    option.showDecorationSelected = true;
    return option;
}

bool PropertyEditor::event(QEvent* event)
{
    if (event->type() == QEvent::ShortcutOverride) {
        QKeyEvent * kevent = static_cast<QKeyEvent*>(event);
        Qt::KeyboardModifiers ShiftKeypadModifier = Qt::ShiftModifier | Qt::KeypadModifier;
        if (kevent->modifiers() == Qt::NoModifier ||
            kevent->modifiers() == Qt::ShiftModifier ||
            kevent->modifiers() == Qt::KeypadModifier ||
            kevent->modifiers() == ShiftKeypadModifier) {
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

void PropertyEditor::commitData (QWidget * editor)
{
    committing = true;
    QTreeView::commitData(editor);
    committing = false;
    if (delaybuild) {
        delaybuild = false;
        propertyModel->buildUp(PropertyModel::PropertyList());
    }
}

void PropertyEditor::editorDestroyed (QObject * editor)
{
    QTreeView::editorDestroyed(editor);

    // When editing expression through context menu, the editor (ExpLineEditor)
    // deletes itself when finished, so it won't trigger closeEditor signal. We
    // must handle it here to perform auto update.
    closeTransaction();
}

void PropertyEditor::currentChanged ( const QModelIndex & current, const QModelIndex & previous )
{
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
        PropertyItem* item = static_cast<PropertyItem*>(editingIndex.internalPointer());
        int decimals = item->decimals();
        closePersistentEditor(editingIndex);
        editingIndex = QPersistentModelIndex();
        activeEditor = nullptr;
        if(hasFocus)
            setFocus();

        if (editingDecimals != decimals)
            propertyModel->updateDecimals();
    }
}

void PropertyEditor::openEditor(const QModelIndex &index)
{
    if(editingIndex == index && activeEditor)
        return;

    closeEditor();

    openPersistentEditor(model()->buddy(index));

    if(!editingIndex.isValid() || !autoupdate)
        return;

    auto &app = App::GetApplication();
    if(app.getActiveTransaction()) {
        FC_LOG("editor already transacting " << app.getActiveTransaction());
        return;
    }
    PropertyItem* item = static_cast<PropertyItem*>(editingIndex.internalPointer());
    auto items = item->getPropertyData();
    if (items.empty()) {
        for(item = item->parent(); item; item = item->parent()) {
            items = item->getPropertyData();
            if(!items.empty())
                break;
        }
        if(items.empty()) {
            FC_LOG("editor no item");
            return;
        }
    }
    auto prop = items[0];
    auto parent = prop->getContainer();
    App::DocumentObject *obj = nullptr;
    App::Document *doc = Base::freecad_dynamic_cast<App::Document>(parent);
    if(!doc) {
        obj  = Base::freecad_dynamic_cast<App::DocumentObject>(parent);
        if (!obj) {
            auto view  = Base::freecad_dynamic_cast<ViewProviderDocumentObject>(parent);
            if (view)
                obj = view->getObject();
        }
        if(!obj || !obj->getDocument()) {
            FC_LOG("invalid object");
            return;
        }
        doc = obj->getDocument();
    }
    if(doc->hasPendingTransaction()) {
        FC_LOG("pending transaction");
        return;
    }
    std::ostringstream str;
    str << tr("Edit").toUtf8().constData() << ' ';
    for(auto prop : items) {
        if(prop->getContainer()!=parent) {
            parent = 0;
            break;
        }
    }
    if(parent == doc)
        str << tr("document").toUtf8().constData();
    else if(obj && obj->getNameInDocument())
        str << obj->Label.getValue();
    else
        str << tr("property").toUtf8().constData();
    str << " " << item->data(0, Qt::DisplayRole).toString().toUtf8().constData();
    if(items.size()>1)
        str << "...";
    transactionID = app.setActiveTransaction(str.str().c_str());
    FC_LOG("editor transaction " << app.getActiveTransaction());
}

void PropertyEditor::onItemActivated ( const QModelIndex & index )
{
    if(index.column() != 1)
        return;
    openEditor(index);
}

void PropertyEditor::recomputeDocument(App::Document* doc)
{
    try {
        if (doc) {
            if (!doc->isTransactionEmpty()
                    && !doc->testStatus(App::Document::Recomputing)
                    && doc->mustExecute()) {
                // Between opening and committing a transaction a recompute
                // could already have been done
                if (doc->mustExecute())
                    doc->recompute();
            }
        }
    }
    // do not re-throw
    catch (const Base::Exception& e) {
        e.ReportException();
    }
    catch (const std::exception& e) {
        Base::Console().Error("Unhandled std::exception caught in PropertyEditor::recomputeDocument.\n"
                              "The error message is: %s\n", e.what());
    }
    catch (...) {
        Base::Console().Error("Unhandled unknown exception caught in PropertyEditor::recomputeDocument.\n");
    }
}

void PropertyEditor::closeTransaction()
{
    int tid = 0;
    if (App::GetApplication().getActiveTransaction(&tid) && tid == transactionID) {
        transactionID = 0;
        if (autoupdate) {
            App::Document* doc = App::GetApplication().getActiveDocument();
            recomputeDocument(doc);
        }
        App::GetApplication().closeActiveTransaction();
    }
}

void PropertyEditor::closeEditor (QWidget * editor, QAbstractItemDelegate::EndEditHint hint)
{
    if (closingEditor)
        return;

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
            index = moveCursor(MoveDown,Qt::NoModifier);
        } else if(hint == QAbstractItemDelegate::EditPreviousItem) {
            index = moveCursor(MoveUp,Qt::NoModifier);
        } else
            break;
        if (!index.isValid() || index == lastIndex) {
            if (wrapped) {
                setCurrentIndex(propertyModel->buddy(indexSaved));
                break;
            }
            wrapped = true;
            if (hint == QAbstractItemDelegate::EditNextItem)
                index = moveCursor(MoveHome, Qt::NoModifier);
            else
                index = moveCursor(MoveEnd, Qt::NoModifier);
            if (!index.isValid() || index == indexSaved)
                break;
        }
        lastIndex = index;
        setCurrentIndex(propertyModel->buddy(index));

        PropertyItem *item = static_cast<PropertyItem*>(index.internalPointer());
        // Skip readonly item, because the editor will be disabled and hence
        // does not accept focus, and in turn break Tab/Backtab navigation.
        if (item && item->isReadOnly())
            continue;

        openEditor(index);

    } while (!editingIndex.isValid());
}

void PropertyEditor::reset()
{
    QTreeView::reset();

    closeTransaction();

    QModelIndex parent;
    int numRows = propertyModel->rowCount(parent);
    for (int i=0; i<numRows; ++i) {
        QModelIndex index = propertyModel->index(i, 0, parent);
        PropertyItem *item = static_cast<PropertyItem*>(index.internalPointer());
        if (item->childCount() == 0) {
            if(item->isSeparator())
                setRowHidden(i, parent, true);
        } else
            setEditorMode(index, 0, item->childCount()-1);
        if(item->isExpanded())
            setExpanded(index, true);
    }
}

void PropertyEditor::onRowsMoved(const QModelIndex &parent, int start, int end, const QModelIndex &dst, int)
{
    if(parent != dst) {
        PropertyItem *item = static_cast<PropertyItem*>(parent.internalPointer());
        if(item && item->isSeparator() && item->childCount()==0)
            setRowHidden(parent.row(), propertyModel->parent(parent), true);
        item = static_cast<PropertyItem*>(dst.internalPointer());
        if(item && item->isSeparator() && item->childCount()==end-start+1) {
            setRowHidden(dst.row(), propertyModel->parent(dst), false);
            setExpanded(dst, true);
        }
    }
}

void PropertyEditor::rowsInserted (const QModelIndex & parent, int start, int end)
{
    QTreeView::rowsInserted(parent, start, end);

    PropertyItem *item = static_cast<PropertyItem*>(parent.internalPointer());
    if (item && item->isSeparator() && item->childCount() == end-start+1) {
        setRowHidden(parent.row(), propertyModel->parent(parent), false);
        if(item->isExpanded())
            setExpanded(parent, true);
    }

    for (int i=start; i<end; ++i) {
        QModelIndex index = propertyModel->index(i, 0, parent);
        PropertyItem *child = static_cast<PropertyItem*>(index.internalPointer());
        if(child->isSeparator()) {
            // Set group header rows to span all columns
            setFirstColumnSpanned(i, parent, true);
        }
        if(child->isExpanded())
            setExpanded(index, true);
    }

    if(parent.isValid())
        setEditorMode(parent, start, end);
}

void PropertyEditor::rowsAboutToBeRemoved (const QModelIndex & parent, int start, int end)
{
    QTreeView::rowsAboutToBeRemoved(parent, start, end);

    PropertyItem *item = static_cast<PropertyItem*>(parent.internalPointer());
    if (item && item->isSeparator() && item->childCount() == end-start+1)
        setRowHidden(parent.row(), propertyModel->parent(parent), true);

    if (editingIndex.isValid()) {
        if (editingIndex.row() >= start && editingIndex.row() <= end)
            closeTransaction();
        else {
            removingRows = 1;
            for (QWidget *w = qApp->focusWidget(); w; w = w->parentWidget()) {
                if(w == activeEditor) {
                    removingRows = -1;
                    break;
                }
            }
        }
    }
}

void PropertyEditor::onRowsRemoved(const QModelIndex &, int, int)
{
    if (removingRows < 0 && activeEditor)
        activeEditor->setFocus();
    removingRows = 0;
}

void PropertyEditor::drawBranches(QPainter *painter, const QRect &rect, const QModelIndex &index) const
{
    QTreeView::drawBranches(painter, rect, index);

    QStyleOptionViewItem opt = viewOptions();
    PropertyItem *property = static_cast<PropertyItem*>(index.internalPointer());
    if (property && property->isSeparator()) {
        painter->fillRect(rect, this->background);
    //} else if (selectionModel()->isSelected(index)) {
    //    painter->fillRect(rect, opt.palette.brush(QPalette::Highlight));
    }

    //QPen savedPen = painter->pen();
    //QColor color = static_cast<QRgb>(QApplication::style()->styleHint(QStyle::SH_Table_GridLineColor, &opt));
    //painter->setPen(QPen(color));
    //painter->drawLine(rect.x(), rect.bottom(), rect.right(), rect.bottom());
    //painter->setPen(savedPen);
}

void PropertyEditor::buildUp(PropertyModel::PropertyList &&props, bool _checkDocument)
{
    checkDocument = _checkDocument;

    if (committing) {
        Base::Console().Warning("While committing the data to the property the selection has changed.\n");
        delaybuild = true;
        return;
    }

    // closeTransaction();

    QModelIndex index = this->currentIndex();
    QStringList propertyPath = propertyModel->propertyPathFromIndex(index);
    if (!propertyPath.isEmpty())
        this->selectedProperty = propertyPath;
    propertyModel->buildUp(props);
    if (!this->selectedProperty.isEmpty()) {
        QModelIndex index = propertyModel->propertyIndexFromPath(this->selectedProperty);
        this->setCurrentIndex(index);
    }

    propList = std::move(props);
    propOwners.clear();
    for(auto &v : propList) {
        for(auto prop : v.second) {
            auto container = prop->getContainer();
            if(!container)
                continue;
            // Include document to get proper handling in PropertyView::slotDeleteDocument()
            if(checkDocument && container->isDerivedFrom(App::DocumentObject::getClassTypeId()))
                propOwners.insert(static_cast<App::DocumentObject*>(container)->getDocument());
            propOwners.insert(container);
        }
    }

    if (autoexpand)
        expandAll();

    this->header()->setVisible(!_hideHeader && !propList.empty());
}

void PropertyEditor::hideHeader(bool hide)
{
    _hideHeader = hide;
    this->header()->setVisible(!_hideHeader && !propList.empty());
}

void PropertyEditor::updateProperty(const App::Property& prop)
{
    // forward this to the model if the property is changed from outside
    propertyModel->updateProperty(prop, committing);
}

void PropertyEditor::setEditorMode(const QModelIndex & parent, int start, int end)
{
    int column = 1;
    for (int i=start; i<=end; i++) {
        QModelIndex item = propertyModel->index(i, column, parent);
        PropertyItem* propItem = static_cast<PropertyItem*>(item.internalPointer());
        if (!PropertyView::showAll() && propItem && propItem->testStatus(App::Property::Hidden)) {
            setRowHidden (i, parent, true);
        }
    }
}

void PropertyEditor::removeProperty(const App::Property& prop)
{
    for (PropertyModel::PropertyList::iterator it = propList.begin(); it != propList.end(); ++it) {
        // find the given property in the list and remove it if it's there
        std::vector<App::Property*>::iterator pos = std::find(it->second.begin(), it->second.end(), &prop);
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

enum MenuAction {
    MA_Print,
    MA_AutoExpand,
    MA_ShowAll,
    MA_ToggleHeader,
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
    MA_MaterialEdit,
};

void PropertyEditor::contextMenuEvent(QContextMenuEvent *ev) {
    QMenu menu;
    QAction *autoExpand = Action::addCheckBox(&menu, tr("Auto expand"), autoexpand);
    connect(autoExpand, &QAction::toggled, [this](bool checked) {
        this->autoexpand = checked;
        if (checked)
            this->expandAll();
        else {
            int numRows = propertyModel->rowCount(QModelIndex());
            for (int i=0; i<numRows; ++i)
                collapseItem(propertyModel->index(i, 0, QModelIndex()));
        }
    });

    QPoint pos(QCursor::pos());
    std::vector<QAction*> hiddenActions;
    QAction *showAll = Action::addCheckBox(&menu, tr("Show all"), PropertyView::showAll());
    QRect rect = QApplication::desktop()->availableGeometry(this);
    connect(showAll, &QAction::toggled, [&](bool checked) {
        PropertyView::setShowAll(checked);
        for (auto action : hiddenActions)
            action->setVisible(checked);
        QSize size = menu.sizeHint();
        if (pos.y() + size.height() > rect.height()) {
            QPoint p(pos.x(), rect.top() + std::max(0, rect.height()-size.height()));
            menu.move(p);
        }
    });

    QAction *toggleHeader = Action::addCheckBox(&menu, tr("Hide header"), PropertyView::isHeaderHidden());
    connect(toggleHeader, &QAction::toggled, [](bool checked) {
        PropertyView::toggleHeader(checked);
    });

    menu.addAction(tr("Print to console"))->setData(QVariant(MA_Print));

    auto sels = selectedIndexes();
    auto contextIndex = indexAt(ev->pos());
    if (contextIndex.isValid() && !selectionModel()->isSelected(contextIndex)) {
        selectionModel()->clearSelection();
        sels.clear();
        sels.append(contextIndex);
        selectionModel()->setCurrentIndex(contextIndex, QItemSelectionModel::NoUpdate);
    }

    std::set<App::DocumentObjectT> props;
    for(auto index : sels) {
        auto item = static_cast<PropertyItem*>(index.internalPointer());
        if(item->isSeparator())
            continue;
        for(auto parent=item;parent;parent=parent->parent()) {
            for (auto prop : parent->getPropertyData())
                props.emplace(prop);
        }
    }

    if(props.size() == 1) {
        auto item = static_cast<PropertyItem*>(contextIndex.internalPointer());
        auto prop = props.begin()->getProperty();
        if(item->isBound() 
            && prop
            && !prop->isDerivedFrom(App::PropertyExpressionEngine::getClassTypeId())
            && !prop->isReadOnly() 
            && !prop->testStatus(App::Property::Immutable)
            && !(prop->getType() & App::Prop_ReadOnly))
        {
            contextIndex = propertyModel->buddy(contextIndex);
            setCurrentIndex(contextIndex);
            menu.addSeparator();
            menu.addAction(tr("Expression..."))->setData(QVariant(MA_Expression));
        }
    }

    auto action = menu.addAction(tr("Add property"));
    action->setData(QVariant(MA_AddProp));
    hiddenActions.push_back(action);

    std::map<int, bool> flags;
    if(props.size()) {
        unsigned count = 0;
        for(auto &propT : props) {
            auto prop = propT.getProperty();
            if(prop && prop->testStatus(App::Property::PropDynamic)
                    && !boost::starts_with(prop->getName(),prop->getGroup()))
            {
                ++count;
            } else
                break;
        }
        if(count == props.size()) {
            action = menu.addAction(tr("Rename property group"));
            action->setData(QVariant(MA_EditPropGroup));
            hiddenActions.push_back(action);
        }
    }

    bool canRemove = !props.empty();
    unsigned long propType = 0;
    unsigned long propStatus = 0xffffffff;
    for(auto &propT : props) {
        auto prop = propT.getProperty();
        if (!prop)
            continue;
        propType |= prop->getType();
        propStatus &= prop->getStatus();
        if(!prop->testStatus(App::Property::PropDynamic)
            || prop->testStatus(App::Property::LockDynamic))
        {
            canRemove = false;
        }
    }
    if(canRemove) {
        auto action = menu.addAction(tr("Remove property"));
        action->setData(QVariant(MA_RemoveProp));
        hiddenActions.push_back(action);
    }

    if(props.size()) {
        QAction *action;
        action = menu.addSeparator();
        hiddenActions.push_back(action);

        QString text;
        bool checked;
        QCheckBox *checkbox;
#define _ACTION_SETUP(_name) do {\
            text = tr(#_name);\
            checked = (propStatus & (1<<App::Property::_name)) ? true : false;\
            action = Action::addCheckBox(&menu, text, checked, &checkbox);\
            action->setData(QVariant(MA_##_name));\
            hiddenActions.push_back(action);\
            flags[MA_##_name] = checked;\
        }while(0)
#define ACTION_SETUP(_name) do {\
            _ACTION_SETUP(_name);\
            if(propType & App::Prop_##_name) {\
                action->setText(text + QString::fromLatin1(" *"));\
                checkbox->setChecked(true);\
                checkbox->setDisabled(true);\
                flags.erase(MA_##_name);\
            }\
        }while(0)

        ACTION_SETUP(Hidden);
        ACTION_SETUP(Output);
        ACTION_SETUP(NoRecompute);
        ACTION_SETUP(ReadOnly);
        ACTION_SETUP(Transient);
        _ACTION_SETUP(Touched);
        _ACTION_SETUP(EvalOnRestore);
        _ACTION_SETUP(CopyOnChange);
        if (auto prop = props.begin()->getProperty()) {
            if (prop->isDerivedFrom(App::PropertyMaterial::getClassTypeId()))
                _ACTION_SETUP(MaterialEdit);
        }
    }

    if (!PropertyView::showAll()) {
        for (auto action : hiddenActions)
            action->setVisible(false);
    }
    action = menu.exec(pos);
    if (action) {
        sels = selectedIndexes();
        switch(action->data().toInt()) {
        case MA_Print: {
            std::set<App::Property*> propSet;
            for(auto index : sels) {
                auto item = static_cast<PropertyItem*>(index.internalPointer());
                if(item->isSeparator())
                    continue;
                for(auto parent=item;parent;parent=parent->parent()) {
                    for(auto prop : parent->getPropertyData()) {
                        if(propSet.insert(prop).second)
                            Command::runCommand(Command::Doc,prop->getFullName(true).c_str());
                    }
                }
            }
            break;
        }
        case MA_Expression:
            if(contextIndex == currentIndex()) {
                Base::FlagToggler<> flag(binding);
                openEditor(contextIndex);
            }
            break;
        case MA_AddProp: {
            App::AutoTransaction committer("Add property");
            std::unordered_set<App::PropertyContainer*> containers;
            auto sels = Gui::Selection().getSelection("*");
            if(sels.size() == 1)
                containers.insert(sels[0].pObject);
            else {
                for(auto &propT : props) {
                    if (auto prop = propT.getProperty())
                        containers.insert(prop->getContainer());
                }
            }
            Gui::Dialog::DlgAddProperty dlg(
                    Gui::getMainWindow(),std::move(containers));
            dlg.exec();
            break;
        }
        case MA_EditPropGroup: {
            auto prop = props.begin()->getProperty();
            if (!prop)
                break;
            // This operation is not undoable yet.
            const char *groupName = prop->getGroup();
            if(!groupName)
                groupName = "Base";
            QString res = QInputDialog::getText(Gui::getMainWindow(),
                    tr("Rename property group"), tr("Group name:"),
                    QLineEdit::Normal, QString::fromUtf8(groupName));
            if(res.size()) {
                std::string group = res.toUtf8().constData();
                for(auto &propT : props) {
                    if (auto prop = propT.getProperty())
                        prop->getContainer()->changeDynamicProperty(prop,group.c_str(),0);
                }
                buildUp(PropertyModel::PropertyList(propList),checkDocument);
            }
            break;
        }
        case MA_RemoveProp: {
            App::AutoTransaction committer("Remove property");
            for(auto &propT : props) {
                try {
                    if (auto prop = propT.getProperty())
                        prop->getContainer()->removeDynamicProperty(prop->getName());
                }catch(Base::Exception &e) {
                    e.ReportException();
                }
            }
            break;
        }
        default:
            break;
        }
    }

    for (auto action : menu.actions()) {
        auto it = flags.find(action->data().toInt());
        if (it == flags.end() || it->second == action->isChecked())
            continue;
        switch(it->first) {
#define ACTION_CHECK(_name) \
        case MA_##_name:\
            for(auto &propT : props) {\
                if (auto prop = propT.getProperty()) \
                    prop->setStatus(App::Property::_name,action->isChecked());\
            }\
            break
        ACTION_CHECK(Transient);
        ACTION_CHECK(ReadOnly);
        ACTION_CHECK(Output);
        ACTION_CHECK(Hidden);
        ACTION_CHECK(EvalOnRestore);
        ACTION_CHECK(CopyOnChange);
        ACTION_CHECK(MaterialEdit);
        case MA_Touched:
            for(auto &propT : props) {
                if (auto prop = propT.getProperty()) {
                    if(action->isChecked())
                        prop->touch();
                    else
                        prop->purgeTouched();
                }
            }
            break;
        default:
            break;
        }
    }
}

#include "moc_PropertyEditor.cpp"
