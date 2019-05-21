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
#endif

#include <Base/Console.h>
#include <Base/Tools.h>
#include <App/Application.h>
#include <App/Document.h>
#include "MainWindow.h"
#include "DlgAddProperty.h"
#include "PropertyEditor.h"
#include "PropertyItemDelegate.h"
#include "PropertyModel.h"
#include "PropertyView.h"

FC_LOG_LEVEL_INIT("PropertyView",true,true);

using namespace Gui::PropertyEditor;

PropertyEditor::PropertyEditor(QWidget *parent)
    : QTreeView(parent), autoupdate(false), committing(false), delaybuild(false), binding(false)
{
    propertyModel = new PropertyModel(this);
    setModel(propertyModel);

    setVerticalScrollMode(QAbstractItemView::ScrollPerPixel);
    setHorizontalScrollMode(QAbstractItemView::ScrollPerPixel);

    delegate = new PropertyItemDelegate(this);
    delegate->setItemEditorFactory(new PropertyItemEditorFactory);
    setItemDelegate(delegate);

    setAlternatingRowColors(true);
    setRootIsDecorated(true);

    QStyleOptionViewItem opt = viewOptions();
    this->background = opt.palette.dark();
    this->groupColor = opt.palette.color(QPalette::BrightText);

    this->setSelectionMode(QAbstractItemView::ExtendedSelection);

    connect(this, SIGNAL(activated(const QModelIndex &)), this, SLOT(onItemActivated(const QModelIndex &)));
    connect(this, SIGNAL(clicked(const QModelIndex &)), this, SLOT(onItemActivated(const QModelIndex &)));
}

PropertyEditor::~PropertyEditor()
{
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
    delegate->editorClosed(0,QAbstractItemDelegate::NoHint);
    QTreeView::editorDestroyed(editor);
}

void PropertyEditor::currentChanged ( const QModelIndex & current, const QModelIndex & previous )
{
    FC_LOG("current changed " << current.row()<<","<<current.column()
            << "  " << previous.row()<<","<<previous.column());

    QTreeView::currentChanged(current, previous);

    // if (previous.isValid())
    //     closePersistentEditor(model()->buddy(previous));

    // DO NOT activate editor here, use onItemActivate() which response to
    // signals of activated and clicked.
    //
    // if (current.isValid())
    //     openPersistentEditor(model()->buddy(current));
}

void PropertyEditor::onItemActivated ( const QModelIndex & index )
{
    if(index.column() == 1)
        edit(model()->buddy(index),AllEditTriggers,0);
}

void PropertyEditor::closeEditor (QWidget * editor, QAbstractItemDelegate::EndEditHint hint)
{
    if (autoupdate) {
        App::Document* doc = App::GetApplication().getActiveDocument();
        if (doc) {
            if (!doc->isTransactionEmpty()) {
                doc->commitTransaction();
                // Between opening and committing a transaction a recompute
                // could already have been done
                if (doc->isTouched())
                    doc->recompute();
            }
            else {
                doc->abortTransaction();
            }
        }
    }

    QModelIndex indexSaved = currentIndex();
    FC_LOG("index saved " << indexSaved.row() << ", " << indexSaved.column());

    QTreeView::closeEditor(editor, hint);

    QModelIndex lastIndex;
    while(this->state()!=EditingState) {
        QModelIndex index;
        if (hint == QAbstractItemDelegate::EditNextItem) {
            index = moveCursor(MoveDown,Qt::NoModifier);
        } else if(hint == QAbstractItemDelegate::EditPreviousItem) {
            index = moveCursor(MoveUp,Qt::NoModifier);
        } else
            break;
        if(!index.isValid() || index==lastIndex) {
            setCurrentIndex(indexSaved);
            break;
        }
        lastIndex = index;
        setCurrentIndex(index);
        edit(index,AllEditTriggers,0);
    }
}

void PropertyEditor::reset()
{
    QTreeView::reset();

    QModelIndex index;
    int numRows = propertyModel->rowCount(index);
    if (numRows > 0)
        setEditorMode(index, 0, numRows-1);
}

void PropertyEditor::rowsInserted (const QModelIndex & parent, int start, int end)
{
    QTreeView::rowsInserted(parent, start, end);
    setEditorMode(parent, start, end);
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

void PropertyEditor::buildUp(PropertyModel::PropertyList &&props)
{
    if (committing) {
        Base::Console().Warning("While committing the data to the property the selection has changed.\n");
        delaybuild = true;
        return;
    }

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
        for(auto prop : v.second)
            propOwners.insert(prop->getContainer());
    }
}

void PropertyEditor::updateProperty(const App::Property& prop)
{
    // forward this to the model if the property is changed from outside
    if (!committing)
        propertyModel->updateProperty(prop);
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
        if (propItem && propItem->isSeparator()) {
            // Set group header rows to span all columns
            setFirstColumnSpanned(i, parent, true);
        }
    }
}

void PropertyEditor::updateEditorMode(const App::Property& prop)
{
    // check if the parent object is selected
    std::string editor = prop.getEditorName();
    if (!PropertyView::showAll() && editor.empty())
        return;

    bool hidden = PropertyView::isPropertyHidden(&prop);
    bool readOnly = prop.testStatus(App::Property::ReadOnly);

    int column = 1;
    int numRows = propertyModel->rowCount();
    for (int i=0; i<numRows; i++) {
        QModelIndex item = propertyModel->index(i, column);
        PropertyItem* propItem = static_cast<PropertyItem*>(item.internalPointer());
        if (propItem && propItem->hasProperty(&prop)) {
            setRowHidden (i, QModelIndex(), hidden);

            propItem->updateData();
            if (item.isValid()) {
                updateItemEditor(!readOnly, column, item);
                dataChanged(item, item);
            }
            break;
        }
    }
}

void PropertyEditor::updateItemEditor(bool enable, int column, const QModelIndex& parent)
{
    QWidget* editor = indexWidget(parent);
    if (editor)
        editor->setEnabled(enable);

    int numRows = propertyModel->rowCount(parent);
    for (int i=0; i<numRows; i++) {
        QModelIndex item = propertyModel->index(i, column, parent);
        if (item.isValid()) {
            updateItemEditor(enable, column, item);
        }
    }
}

bool PropertyEditor::appendProperty(const App::Property& prop) {
    return !!propOwners.count(prop.getContainer());
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
    MA_ShowAll,
    MA_Expression,
    MA_RemoveProp,
    MA_AddProp,
    MA_Transient,
    MA_Output,
    MA_NoRecompute,
    MA_ReadOnly,
    MA_Hidden,
    MA_Touched,
    MA_EvalOnRestore,
};

void PropertyEditor::contextMenuEvent(QContextMenuEvent *) {
    QMenu menu;
    QAction *showAll = menu.addAction(tr("Show all"));
    showAll->setCheckable(true);
    showAll->setChecked(PropertyView::showAll());
    showAll->setData(QVariant(MA_ShowAll));

    auto contextIndex = currentIndex();

    std::unordered_set<App::Property*> props;

    if(PropertyView::showAll()) {
        for(auto index : selectedIndexes()) {
            auto item = static_cast<PropertyItem*>(index.internalPointer());
            if(item->isSeparator())
                continue;
            for(auto parent=item;parent;parent=item->parent()) {
                const auto &ps = parent->getPropertyData();
                if(ps.size()) {
                    props.insert(ps.begin(),ps.end());
                    break;
                }
            }
        }

        if(props.size())
            menu.addAction(tr("Add property"))->setData(QVariant(MA_AddProp));

        bool canRemove = !props.empty();
        unsigned long propType = 0;
        unsigned long propStatus = 0xffffffff;
        for(auto prop : props) {
            propType |= prop->getType();
            propStatus &= prop->getStatus();
            if(!prop->testStatus(App::Property::PropDynamic)
                || prop->testStatus(App::Property::LockDynamic))
            {
                canRemove = false;
            }
        }
        if(canRemove)
            menu.addAction(tr("Remove property"))->setData(QVariant(MA_RemoveProp));

        if(props.size() == 1) {
            auto item = static_cast<PropertyItem*>(contextIndex.internalPointer());
            auto prop = *props.begin();
            if(item->isBound() 
                && !prop->isDerivedFrom(App::PropertyExpressionEngine::getClassTypeId())
                && !prop->isReadOnly() 
                && !(prop->getType() & App::Prop_ReadOnly))
            {
                contextIndex = propertyModel->buddy(contextIndex);
                setCurrentIndex(contextIndex);
                menu.addSeparator();
                menu.addAction(tr("Expression..."))->setData(QVariant(MA_Expression));
            }
        }

        if(props.size()) {
            menu.addSeparator();

            QAction *action;
            QString text;
#define _ACTION_SETUP(_name) do {\
                text = tr(#_name);\
                action = menu.addAction(text);\
                action->setData(QVariant(MA_##_name));\
                action->setCheckable(true);\
                if(propStatus & (1<<App::Property::_name))\
                    action->setChecked(true);\
            }while(0)
#define ACTION_SETUP(_name) do {\
                _ACTION_SETUP(_name);\
                if(propType & App::Prop_##_name) {\
                    action->setText(text + QString::fromLatin1(" *"));\
                    action->setChecked(true);\
                }\
            }while(0)

            ACTION_SETUP(Hidden);
            ACTION_SETUP(Output);
            ACTION_SETUP(NoRecompute);
            ACTION_SETUP(ReadOnly);
            ACTION_SETUP(Transient);
            _ACTION_SETUP(Touched);
            _ACTION_SETUP(EvalOnRestore);
        }
    }

    auto action = menu.exec(QCursor::pos());
    if(!action)
        return;

    switch(action->data().toInt()) {
    case MA_ShowAll:
        PropertyView::setShowAll(action->isChecked());
        return;
#define ACTION_CHECK(_name) \
    case MA_##_name:\
        for(auto prop : props) \
            prop->setStatus(App::Property::_name,action->isChecked());\
        break
    ACTION_CHECK(Transient);
    ACTION_CHECK(ReadOnly);
    ACTION_CHECK(Output);
    ACTION_CHECK(Hidden);
    ACTION_CHECK(EvalOnRestore);
    case MA_Touched:
        for(auto prop : props) {
            if(action->isChecked())
                prop->touch();
            else
                prop->purgeTouched();
        }
        break;
    case MA_Expression:
        if(contextIndex == currentIndex()) {
            closePersistentEditor(contextIndex);
            Base::FlagToggler<> flag(binding);
            edit(contextIndex,AllEditTriggers,0);
        }
        break;
    case MA_AddProp: {
        App::AutoTransaction committer("Add property");
        std::unordered_set<App::PropertyContainer*> containers;
        for(auto prop : props)
            containers.insert(prop->getContainer());
        Gui::Dialog::DlgAddProperty dlg(
                Gui::getMainWindow(),std::move(containers));
        dlg.exec();
        return;
    }
    case MA_RemoveProp: {
        App::AutoTransaction committer("Remove property");
        for(auto prop : props) {
            try {
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

#include "moc_PropertyEditor.cpp"
