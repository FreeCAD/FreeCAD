/***************************************************************************
 *   Copyright (c) 2011 Werner Mayer <wmayer[at]users.sourceforge.net>     *
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
# include <QTextStream>
#endif

#include "ui_TaskLoft.h"
#include "TaskLoft.h"

#include <Gui/Application.h>
#include <Gui/Document.h>
#include <Gui/Selection.h>
#include <Gui/ViewProvider.h>

#include <Base/Console.h>
#include <Base/Interpreter.h>
#include <App/Application.h>
#include <App/Document.h>
#include <App/DocumentObject.h>
#include <Mod/Part/App/PartFeature.h>


using namespace PartGui;

class LoftWidget::Private
{
public:
    Ui_TaskLoft ui;
    std::string document;
    Private()
    {
    }
    ~Private()
    {
    }
};

/* TRANSLATOR PartGui::LoftWidget */

LoftWidget::LoftWidget(QWidget* parent)
  : d(new Private())
{
    Gui::Application::Instance->runPythonCode("from FreeCAD import Base");
    Gui::Application::Instance->runPythonCode("import Part");

    d->ui.setupUi(this);
    connect(d->ui.treeWidgetWire, SIGNAL(currentItemChanged(QTreeWidgetItem*, QTreeWidgetItem*)),
            this, SLOT(onCurrentItemChanged(QTreeWidgetItem*, QTreeWidgetItem*)));
    connect(d->ui.treeWidgetLoft, SIGNAL(currentItemChanged(QTreeWidgetItem*, QTreeWidgetItem*)),
            this, SLOT(onCurrentItemChanged(QTreeWidgetItem*, QTreeWidgetItem*)));
    findShapes();
}

LoftWidget::~LoftWidget()
{
    delete d;
}

void LoftWidget::findShapes()
{
    App::Document* activeDoc = App::GetApplication().getActiveDocument();
    Gui::Document* activeGui = Gui::Application::Instance->getDocument(activeDoc);
    if (!activeGui) return;
    d->document = activeDoc->getName();

    std::vector<Part::Feature*> objs = activeDoc->getObjectsOfType<Part::Feature>();

    for (std::vector<Part::Feature*>::iterator it = objs.begin(); it!=objs.end(); ++it) {
        const TopoDS_Shape& shape = (*it)->Shape.getValue();
        if (shape.IsNull()) continue;

        if (shape.ShapeType() == TopAbs_WIRE ||
            shape.ShapeType() == TopAbs_EDGE ||
            shape.ShapeType() == TopAbs_VERTEX) {
            QString label = QString::fromUtf8((*it)->Label.getValue());
            QString name = QString::fromAscii((*it)->getNameInDocument());
            
            QTreeWidgetItem* child = new QTreeWidgetItem();
            child->setText(0, label);
            child->setToolTip(0, label);
            child->setData(0, Qt::UserRole, name);
            Gui::ViewProvider* vp = activeGui->getViewProvider(*it);
            if (vp) child->setIcon(0, vp->getIcon());
            d->ui.treeWidgetWire->addTopLevelItem(child);
        }
    }
}

bool LoftWidget::accept()
{
    QString list, solid, ruled;
    if (d->ui.checkSolid->isChecked())
        solid = QString::fromAscii("True");
    else
        solid = QString::fromAscii("False");

    if (d->ui.checkRuledSurface->isChecked())
        ruled = QString::fromAscii("True");
    else
        ruled = QString::fromAscii("False");

    QTextStream str(&list);

    int count = d->ui.treeWidgetLoft->topLevelItemCount();
    if (count < 2) {
        QMessageBox::critical(this, tr("Too few elements"), tr("At least two vertices, edges or wires are required."));
        return false;
    }
    for (int i=0; i<count; i++) {
        QTreeWidgetItem* child = d->ui.treeWidgetLoft->topLevelItem(i);
        QString name = child->data(0, Qt::UserRole).toString();
        str << "App.getDocument('" << d->document.c_str() << "')." << name << ", ";
    }

    try {
        QString cmd;
        cmd = QString::fromAscii(
            "App.getDocument('%4').addObject('Part::Loft','Loft')\n"
            "App.getDocument('%4').ActiveObject.Sections=[%1]\n"
            "App.getDocument('%4').ActiveObject.Solid=%2\n"
            "App.getDocument('%4').ActiveObject.Ruled=%3\n"
            ).arg(list).arg(solid).arg(ruled).arg(QString::fromAscii(d->document.c_str()));

        Gui::Document* doc = Gui::Application::Instance->getDocument(d->document.c_str());
        if (!doc) throw Base::Exception("Document doesn't exist anymore");
        doc->openCommand("Loft");
        Gui::Application::Instance->runPythonCode((const char*)cmd.toAscii(), false, false);
        doc->commitCommand();
        doc->getDocument()->recompute();
    }
    catch (const Base::Exception& e) {
        Base::Console().Error("%s\n", e.what());
        return false;
    }

    return true;
}

bool LoftWidget::reject()
{
    return true;
}

void LoftWidget::onCurrentItemChanged(QTreeWidgetItem* current, QTreeWidgetItem* previous)
{
    if (previous) {
        Gui::Selection().rmvSelection(d->document.c_str(),
            (const char*)previous->data(0,Qt::UserRole).toByteArray());
    }
    if (current) {
        Gui::Selection().addSelection(d->document.c_str(),
            (const char*)current->data(0,Qt::UserRole).toByteArray());
    }
}

void LoftWidget::on_addButton_clicked()
{
    QTreeWidgetItem* item = d->ui.treeWidgetWire->currentItem();
    if (item) {
        int index = d->ui.treeWidgetWire->indexOfTopLevelItem(item);
        item = d->ui.treeWidgetWire->takeTopLevelItem(index);
        d->ui.treeWidgetWire->setCurrentItem(0);
        d->ui.treeWidgetLoft->addTopLevelItem(item);
        d->ui.treeWidgetLoft->setCurrentItem(item);
    }
}

void LoftWidget::on_removeButton_clicked()
{
    QTreeWidgetItem* item = d->ui.treeWidgetLoft->currentItem();
    if (item) {
        int index = d->ui.treeWidgetLoft->indexOfTopLevelItem(item);
        item = d->ui.treeWidgetLoft->takeTopLevelItem(index);
        d->ui.treeWidgetLoft->setCurrentItem(0);
        d->ui.treeWidgetWire->addTopLevelItem(item);
        d->ui.treeWidgetWire->setCurrentItem(item);
    }
}

void LoftWidget::on_upButton_clicked()
{
    QTreeWidgetItem* item = d->ui.treeWidgetLoft->currentItem();
    if (item && d->ui.treeWidgetLoft->isItemSelected(item)) {
        int index = d->ui.treeWidgetLoft->indexOfTopLevelItem(item);
        if (index > 0) {
            d->ui.treeWidgetLoft->takeTopLevelItem(index);
            d->ui.treeWidgetLoft->insertTopLevelItem(index-1, item);
            d->ui.treeWidgetLoft->setCurrentItem(item);
        }
    }
}

void LoftWidget::on_downButton_clicked()
{
    QTreeWidgetItem* item = d->ui.treeWidgetLoft->currentItem();
    if (item && d->ui.treeWidgetLoft->isItemSelected(item)) {
        int index = d->ui.treeWidgetLoft->indexOfTopLevelItem(item);
        if (index < d->ui.treeWidgetLoft->topLevelItemCount()-1) {
            d->ui.treeWidgetLoft->takeTopLevelItem(index);
            d->ui.treeWidgetLoft->insertTopLevelItem(index+1, item);
            d->ui.treeWidgetLoft->setCurrentItem(item);
        }
    }
}

void LoftWidget::changeEvent(QEvent *e)
{
    QWidget::changeEvent(e);
    if (e->type() == QEvent::LanguageChange) {
        d->ui.retranslateUi(this);
    }
}


/* TRANSLATOR PartGui::TaskLoft */

TaskLoft::TaskLoft()
{
    widget = new LoftWidget();
    taskbox = new Gui::TaskView::TaskBox(
        QPixmap(), widget->windowTitle(), true, 0);
    taskbox->groupLayout()->addWidget(widget);
    Content.push_back(taskbox);
}

TaskLoft::~TaskLoft()
{
}

void TaskLoft::open()
{
}

void TaskLoft::clicked(int)
{
}

bool TaskLoft::accept()
{
    return widget->accept();
}

bool TaskLoft::reject()
{
    return widget->reject();
}

#include "moc_TaskLoft.cpp"
