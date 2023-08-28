/***************************************************************************
 *   Copyright (c) 2007 Werner Mayer <wmayer[at]users.sourceforge.net>     *
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
# include <QMessageBox>
# include <QTreeWidget>
# include <TopExp_Explorer.hxx>
# include <TopoDS_Shape.hxx>
#endif

#include <Base/Exception.h>
#include <Base/Tools.h>
#include <App/Application.h>
#include <App/Document.h>
#include <App/DocumentObject.h>
#include <App/DocumentObjectGroup.h>
#include <Gui/Application.h>
#include <Gui/BitmapFactory.h>
#include <Gui/Command.h>
#include <Gui/Document.h>
#include <Gui/Selection.h>
#include <Gui/ViewProvider.h>
#include <Gui/WaitCursor.h>
#include <Mod/Part/App/PartFeature.h>

#include "DlgBooleanOperation.h"
#include "ui_DlgBooleanOperation.h"


using namespace PartGui;
namespace sp = std::placeholders;

namespace PartGui {
    class BooleanOperationItem : public QTreeWidgetItem
    {
    public:
        explicit BooleanOperationItem(int type = Type)
            : QTreeWidgetItem(type)
        {
        }
        void setData (int column, int role, const QVariant & value) override
        {
            QTreeWidgetItem::setData(column, role, value);
            if (role == Qt::CheckStateRole && value.toBool()) {
                QTreeWidget* tree = this->treeWidget();
                if (!tree)
                    return;
                int numChild = tree->topLevelItemCount();
                for (int i=0; i<numChild; i++) {
                    QTreeWidgetItem* item = tree->topLevelItem(i);
                    for (int j=0; j<item->childCount(); j++) {
                        QTreeWidgetItem* child = item->child(j);
                        if (child && child->checkState(column) & Qt::Checked) {
                            if (child != this)
                                child->setCheckState(column, Qt::Unchecked);
                        }
                    }
                }
            }
        }
    };
}

/* TRANSLATOR PartGui::DlgBooleanOperation */

DlgBooleanOperation::DlgBooleanOperation(QWidget* parent)
  : QWidget(parent), ui(new Ui_DlgBooleanOperation)
{
    ui->setupUi(this);
    connect(ui->swapButton, &QPushButton::clicked,
            this, &DlgBooleanOperation::onSwapButtonClicked);
    connect(ui->firstShape, &QTreeWidget::currentItemChanged,
            this, &DlgBooleanOperation::currentItemChanged);
    connect(ui->secondShape, &QTreeWidget::currentItemChanged,
            this, &DlgBooleanOperation::currentItemChanged);
    //NOLINTBEGIN
    this->connectNewObject = App::GetApplication().signalNewObject.connect(std::bind
        (&DlgBooleanOperation::slotCreatedObject, this, sp::_1));
    this->connectModObject = App::GetApplication().signalChangedObject.connect(std::bind
        (&DlgBooleanOperation::slotChangedObject, this, sp::_1, sp::_2));
    //NOLINTEND
    findShapes();
}

/*
 *  Destroys the object and frees any allocated resources
 */
DlgBooleanOperation::~DlgBooleanOperation()
{
    // no need to delete child widgets, Qt does it all for us
    this->connectNewObject.disconnect();
    this->connectModObject.disconnect();
}

void DlgBooleanOperation::changeEvent(QEvent *e)
{
    if (e->type() == QEvent::LanguageChange) {
        ui->retranslateUi(this);
    }
    QWidget::changeEvent(e);
}

void DlgBooleanOperation::slotCreatedObject(const App::DocumentObject& obj)
{
    App::Document* activeDoc = App::GetApplication().getActiveDocument();
    if (!activeDoc)
        return;
    App::Document* doc = obj.getDocument();
    if (activeDoc == doc && obj.getTypeId().isDerivedFrom(Part::Feature::getClassTypeId())) {
        observe.push_back(&obj);
    }
}

void DlgBooleanOperation::slotChangedObject(const App::DocumentObject& obj,
                                            const App::Property& prop)
{
    std::list<const App::DocumentObject*>::iterator it;
    it = std::find(observe.begin(), observe.end(), &obj);
    if (it != observe.end() && prop.getTypeId() == Part::PropertyPartShape::getClassTypeId()) {
        const TopoDS_Shape& shape = static_cast<const Part::PropertyPartShape&>(prop).getValue();
        if (!shape.IsNull()) {
            Gui::Document* activeGui = Gui::Application::Instance->getDocument(obj.getDocument());
            QString label = QString::fromUtf8(obj.Label.getValue());
            QString name = QString::fromLatin1(obj.getNameInDocument());

            QTreeWidgetItem* child = new BooleanOperationItem();
            child->setCheckState(0, Qt::Unchecked);
            child->setText(0, label);
            child->setToolTip(0, label);
            child->setData(0, Qt::UserRole, name);
            Gui::ViewProvider* vp = activeGui->getViewProvider(&obj);
            if (vp)
                child->setIcon(0, vp->getIcon());

            QTreeWidgetItem* copy = new BooleanOperationItem();
            copy->setCheckState(0, Qt::Unchecked);
            copy->setText(0, label);
            copy->setToolTip(0, label);
            copy->setData(0, Qt::UserRole, name);
            if (vp)
                copy->setIcon(0, vp->getIcon());

            TopAbs_ShapeEnum type = shape.ShapeType();
            if (type == TopAbs_SOLID) {
                ui->firstShape->topLevelItem(0)->addChild(child);
                ui->secondShape->topLevelItem(0)->addChild(copy);
                ui->firstShape->topLevelItem(0)->setExpanded(true);
                ui->secondShape->topLevelItem(0)->setExpanded(true);
            }
            else if (type == TopAbs_SHELL) {
                ui->firstShape->topLevelItem(1)->addChild(child);
                ui->secondShape->topLevelItem(1)->addChild(copy);
                ui->firstShape->topLevelItem(1)->setExpanded(true);
                ui->secondShape->topLevelItem(1)->setExpanded(true);
            }
            else if (type == TopAbs_COMPOUND || type == TopAbs_COMPSOLID) {
                ui->firstShape->topLevelItem(2)->addChild(child);
                ui->secondShape->topLevelItem(2)->addChild(copy);
                ui->firstShape->topLevelItem(2)->setExpanded(true);
                ui->secondShape->topLevelItem(2)->setExpanded(true);
            }
            else if (type == TopAbs_FACE) {
                ui->firstShape->topLevelItem(3)->addChild(child);
                ui->secondShape->topLevelItem(3)->addChild(copy);
                ui->firstShape->topLevelItem(3)->setExpanded(true);
                ui->secondShape->topLevelItem(3)->setExpanded(true);
            }
            else { // belongs to none of these groups
                delete child; child = nullptr;
                delete copy; copy = nullptr;
            }

            // remove the watched object because now it is added to the tree
            observe.erase(it);
        }
    }
}

bool DlgBooleanOperation::hasSolids(const App::DocumentObject* obj) const
{
    if (obj->getTypeId().isDerivedFrom(Part::Feature::getClassTypeId())) {
        const TopoDS_Shape& shape = static_cast<const Part::Feature*>(obj)->Shape.getValue();
        TopExp_Explorer anExp (shape, TopAbs_SOLID);
        if (anExp.More()) {
            return true;
        }
    }

    return false;
}

void DlgBooleanOperation::findShapes()
{
    App::Document* activeDoc = App::GetApplication().getActiveDocument();
    if (!activeDoc)
        return;
    Gui::Document* activeGui = Gui::Application::Instance->getDocument(activeDoc);
    if (!activeGui)
        return;

    std::vector<App::DocumentObject*> objs = activeDoc->getObjectsOfType
        (Part::Feature::getClassTypeId());

    QTreeWidgetItem *item_left=nullptr, *item_right=nullptr;
    for (auto obj : objs) {
        const TopoDS_Shape& shape = static_cast<Part::Feature*>(obj)->Shape.getValue();
        if (!shape.IsNull()) {
            QString label = QString::fromUtf8(obj->Label.getValue());
            QString name = QString::fromLatin1(obj->getNameInDocument());

            QTreeWidgetItem* child = new BooleanOperationItem();
            child->setCheckState(0, Qt::Unchecked);
            child->setText(0, label);
            child->setToolTip(0, label);
            child->setData(0, Qt::UserRole, name);
            Gui::ViewProvider* vp = activeGui->getViewProvider(obj);
            if (vp)
                child->setIcon(0, vp->getIcon());

            QTreeWidgetItem* copy = new BooleanOperationItem();
            copy->setCheckState(0, Qt::Unchecked);
            copy->setText(0, label);
            copy->setToolTip(0, label);
            copy->setData(0, Qt::UserRole, name);
            if (vp)
                copy->setIcon(0, vp->getIcon());

            TopAbs_ShapeEnum type = shape.ShapeType();
            if (type == TopAbs_SOLID) {
                ui->firstShape->topLevelItem(0)->addChild(child);
                ui->secondShape->topLevelItem(0)->addChild(copy);
            }
            else if (type == TopAbs_SHELL) {
                ui->firstShape->topLevelItem(1)->addChild(child);
                ui->secondShape->topLevelItem(1)->addChild(copy);
            }
            else if (type == TopAbs_COMPOUND || type == TopAbs_COMPSOLID) {
                ui->firstShape->topLevelItem(2)->addChild(child);
                ui->secondShape->topLevelItem(2)->addChild(copy);
            }
            else if (type == TopAbs_FACE) {
                ui->firstShape->topLevelItem(3)->addChild(child);
                ui->secondShape->topLevelItem(3)->addChild(copy);
            }
            else { // belongs to none of these groups
                delete child; child = nullptr;
                delete copy; copy = nullptr;
            }

            if (!item_left || !item_right) {
                bool selected = Gui::Selection().isSelected(obj);
                if (!item_left && selected)
                    item_left = child;
                else if (!item_right && selected)
                    item_right = copy;
            }
        }
    }

    if (item_left) {
        item_left->setCheckState(0, Qt::Checked);
        ui->firstShape->setCurrentItem(item_left);
    }
    if (item_right) {
        item_right->setCheckState(0, Qt::Checked);
        ui->secondShape->setCurrentItem(item_right);
    }
    for (int i = 0; i < ui->firstShape->topLevelItemCount(); i++) {
        QTreeWidgetItem* group = ui->firstShape->topLevelItem(i);
        group->setFlags(Qt::ItemIsEnabled);
        if (group->childCount() > 0)
            group->setExpanded(true);
    }
    for (int i = 0; i < ui->secondShape->topLevelItemCount(); i++) {
        QTreeWidgetItem* group = ui->secondShape->topLevelItem(i);
        group->setFlags(Qt::ItemIsEnabled);
        if (group->childCount() > 0)
            group->setExpanded(true);
    }
}

bool DlgBooleanOperation::indexOfCurrentItem(QTreeWidgetItem* item, int& top_ind, int& child_ind) const
{
    QTreeWidgetItem* parent = item->parent();
    if (parent) {
        top_ind = parent->treeWidget()->indexOfTopLevelItem(parent);
        child_ind = parent->indexOfChild(item);
        return true;
    }

    return false;
}

void DlgBooleanOperation::currentItemChanged(QTreeWidgetItem* current, QTreeWidgetItem * previous)
{
    Q_UNUSED(current);
    Q_UNUSED(previous);
//    if (current && current->flags() & Qt::ItemIsUserCheckable)
//        current->setCheckState(0, Qt::Checked);
    //if (previous && previous->flags() & Qt::ItemIsUserCheckable)
    //    previous->setCheckState(0, Qt::Unchecked);
}

void DlgBooleanOperation::onSwapButtonClicked()
{
    QTreeWidgetItem* lChild = ui->firstShape->currentItem();
    bool lsel = (lChild && (lChild->checkState(0) & Qt::Checked));
    QTreeWidgetItem* rChild = ui->secondShape->currentItem();
    bool rsel = (rChild && (rChild->checkState(0) & Qt::Checked));

    if (rsel) {
        int top_index, child_ind;
        if (indexOfCurrentItem(rChild, top_index, child_ind)) {
            QTreeWidgetItem* child = ui->firstShape->topLevelItem(top_index)->child(child_ind);
            child->setCheckState(0, Qt::Checked);
            ui->firstShape->setCurrentItem(child);
        }
    }
    if (lsel) {
        int top_index, child_ind;
        if (indexOfCurrentItem(lChild, top_index, child_ind)) {
            QTreeWidgetItem* child = ui->secondShape->topLevelItem(top_index)->child(child_ind);
            child->setCheckState(0, Qt::Checked);
            ui->secondShape->setCurrentItem(child);
        }
    }
}

void DlgBooleanOperation::accept()
{
    int ltop, lchild, rtop, rchild;

    QTreeWidgetItem* litem = nullptr;
    int numLChild = ui->firstShape->topLevelItemCount();
    for (int i=0; i<numLChild; i++) {
        QTreeWidgetItem* item = ui->firstShape->topLevelItem(i);
        for (int j=0; j<item->childCount(); j++) {
            QTreeWidgetItem* child = item->child(j);
            if (child && child->checkState(0) & Qt::Checked) {
                litem = child;
                break;
            }
        }

        if (litem)
            break;
    }

    QTreeWidgetItem* ritem = nullptr;
    int numRChild = ui->secondShape->topLevelItemCount();
    for (int i=0; i<numRChild; i++) {
        QTreeWidgetItem* item = ui->secondShape->topLevelItem(i);
        for (int j=0; j<item->childCount(); j++) {
            QTreeWidgetItem* child = item->child(j);
            if (child && child->checkState(0) & Qt::Checked) {
                ritem = child;
                break;
            }
        }

        if (ritem)
            break;
    }

    if (!litem || !indexOfCurrentItem(litem,ltop,lchild)) {
        QMessageBox::critical(this, windowTitle(),
            tr("Select a shape on the left side, first"));
        return;
    }
    if (!ritem || !indexOfCurrentItem(ritem,rtop,rchild)) {
        QMessageBox::critical(this, windowTitle(),
            tr("Select a shape on the right side, first"));
        return;
    }
    if (ltop == rtop && lchild == rchild) {
        QMessageBox::critical(this, windowTitle(),
            tr("Cannot perform a boolean operation with the same shape"));
        return;
    }

    std::string shapeOne, shapeTwo;
    shapeOne = (const char*)litem->data(0, Qt::UserRole).toByteArray();
    shapeTwo = (const char*)ritem->data(0, Qt::UserRole).toByteArray();
    App::Document* activeDoc = App::GetApplication().getActiveDocument();
    if (!activeDoc) {
        QMessageBox::critical(this, windowTitle(),
            tr("No active document available"));
        return;
    }

    std::string method;
    App::DocumentObject* obj1 = activeDoc->getObject(shapeOne.c_str());
    App::DocumentObject* obj2 = activeDoc->getObject(shapeTwo.c_str());
    if (!obj1 || !obj2) {
        // objects don't exists (anymore)
        QMessageBox::critical(this, windowTitle(),
            tr("One of the selected objects doesn't exist anymore"));
        return;
    }

    if (ui->unionButton->isChecked()) {
        if (!hasSolids(obj1) || !hasSolids(obj2)) {
            QMessageBox::critical(this, windowTitle(),
                tr("Performing union on non-solids is not possible"));
            return;
        }
        method = "make_fuse";
    }
    else if (ui->interButton->isChecked()) {
        if (!hasSolids(obj1) || !hasSolids(obj2)) {
            QMessageBox::critical(this, windowTitle(),
                tr("Performing intersection on non-solids is not possible"));
            return;
        }
        method = "make_common";
    }
    else if (ui->diffButton->isChecked()) {
        if (!hasSolids(obj1) || !hasSolids(obj2)) {
            QMessageBox::critical(this, windowTitle(),
                tr("Performing difference on non-solids is not possible"));
            return;
        }
        method = "make_cut";
    }
    else if (ui->sectionButton->isChecked()) {
        method = "make_section";
    }

    try {
        Gui::WaitCursor wc;
        activeDoc->openTransaction("Boolean operation");
        std::vector<std::string> names;
        names.push_back(Base::Tools::quoted(shapeOne.c_str()));
        names.push_back(Base::Tools::quoted(shapeTwo.c_str()));
        Gui::Command::doCommand(Gui::Command::Doc,
            "from BOPTools import BOPFeatures");
        Gui::Command::doCommand(Gui::Command::Doc,
            "bp = BOPFeatures.BOPFeatures(App.activeDocument())");
        Gui::Command::doCommand(Gui::Command::Doc,
            "bp.%s([%s])", method.c_str(), Base::Tools::joinList(names).c_str());
        activeDoc->commitTransaction();
        activeDoc->recompute();
    }
    catch (const Base::Exception& e) {
        e.ReportException();
    }
}

// ---------------------------------------

TaskBooleanOperation::TaskBooleanOperation()
{
    widget = new DlgBooleanOperation();
    taskbox = new Gui::TaskView::TaskBox(
        Gui::BitmapFactory().pixmap("Part_Booleans"),
        widget->windowTitle(), false, nullptr);
    taskbox->groupLayout()->addWidget(widget);
    Content.push_back(taskbox);
}

void TaskBooleanOperation::clicked(int id)
{
    if (id == QDialogButtonBox::Apply) {
        widget->accept();
    }
}

#include "moc_DlgBooleanOperation.cpp"
