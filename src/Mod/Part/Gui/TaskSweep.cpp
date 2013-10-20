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

#include "ui_TaskSweep.h"
#include "TaskSweep.h"

#include <Gui/Application.h>
#include <Gui/BitmapFactory.h>
#include <Gui/Document.h>
#include <Gui/Selection.h>
#include <Gui/SelectionFilter.h>
#include <Gui/ViewProvider.h>

#include <Base/Console.h>
#include <Base/Interpreter.h>
#include <App/Application.h>
#include <App/Document.h>
#include <App/DocumentObject.h>
#include <Mod/Part/App/PartFeature.h>


using namespace PartGui;

class SweepWidget::Private
{
public:
    Ui_TaskSweep ui;
    std::string document;
    Private()
    {
    }
    ~Private()
    {
    }
};

/* TRANSLATOR PartGui::SweepWidget */

SweepWidget::SweepWidget(QWidget* parent)
  : d(new Private())
{
    Gui::Application::Instance->runPythonCode("from FreeCAD import Base");
    Gui::Application::Instance->runPythonCode("import Part");

    d->ui.setupUi(this);
    d->ui.selector->setAvailableLabel(tr("Vertex/Edge/Wire/Face"));
    d->ui.selector->setSelectedLabel(tr("Sweep"));

    connect(d->ui.selector->availableTreeWidget(), SIGNAL(currentItemChanged(QTreeWidgetItem*, QTreeWidgetItem*)),
            this, SLOT(onCurrentItemChanged(QTreeWidgetItem*, QTreeWidgetItem*)));
    connect(d->ui.selector->selectedTreeWidget(), SIGNAL(currentItemChanged(QTreeWidgetItem*, QTreeWidgetItem*)),
            this, SLOT(onCurrentItemChanged(QTreeWidgetItem*, QTreeWidgetItem*)));

    findShapes();
}

SweepWidget::~SweepWidget()
{
    delete d;
}

void SweepWidget::findShapes()
{
    App::Document* activeDoc = App::GetApplication().getActiveDocument();
    Gui::Document* activeGui = Gui::Application::Instance->getDocument(activeDoc);
    if (!activeGui) return;
    d->document = activeDoc->getName();

    std::vector<Part::Feature*> objs = activeDoc->getObjectsOfType<Part::Feature>();

    for (std::vector<Part::Feature*>::iterator it = objs.begin(); it!=objs.end(); ++it) {
        const TopoDS_Shape& shape = (*it)->Shape.getValue();
        if (shape.IsNull()) continue;

        if (shape.ShapeType() == TopAbs_FACE ||
            shape.ShapeType() == TopAbs_WIRE ||
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
            d->ui.selector->availableTreeWidget()->addTopLevelItem(child);
        }
    }
}

bool SweepWidget::accept()
{
    Gui::SelectionFilter edgeFilter  ("SELECT Part::Feature SUBELEMENT Edge COUNT 1..");
    Gui::SelectionFilter partFilter  ("SELECT Part::Feature COUNT 1");
    bool matchEdge = edgeFilter.match();
    bool matchPart = partFilter.match();
    if (!matchEdge && !matchPart) {
        QMessageBox::critical(this, tr("Sweep path"), tr("Select an edge or wire you want to sweep along."));
        return false;
    }

    // get the selected object
    std::string selection;
    if (matchEdge) {
        const std::vector<Gui::SelectionObject>& result = edgeFilter.Result[0];
        selection = result.front().getAsPropertyLinkSubString();
    }
    else {
        const std::vector<Gui::SelectionObject>& result = partFilter.Result[0];
        selection = result.front().getAsPropertyLinkSubString();
    }

    QString list, solid, frenet;
    if (d->ui.checkSolid->isChecked())
        solid = QString::fromAscii("True");
    else
        solid = QString::fromAscii("False");

    if (d->ui.checkFrenet->isChecked())
        frenet = QString::fromAscii("True");
    else
        frenet = QString::fromAscii("False");

    QTextStream str(&list);

    int count = d->ui.selector->selectedTreeWidget()->topLevelItemCount();
    if (count < 1) {
        QMessageBox::critical(this, tr("Too few elements"), tr("At least one edge or wire is required."));
        return false;
    }
    for (int i=0; i<count; i++) {
        QTreeWidgetItem* child = d->ui.selector->selectedTreeWidget()->topLevelItem(i);
        QString name = child->data(0, Qt::UserRole).toString();
        str << "App.getDocument('" << d->document.c_str() << "')." << name << ", ";
    }

    try {
        QString cmd;
        cmd = QString::fromAscii(
            "App.getDocument('%5').addObject('Part::Sweep','Sweep')\n"
            "App.getDocument('%5').ActiveObject.Sections=[%1]\n"
            "App.getDocument('%5').ActiveObject.Spine=%2\n"
            "App.getDocument('%5').ActiveObject.Solid=%3\n"
            "App.getDocument('%5').ActiveObject.Frenet=%4\n"
            )
            .arg(list)
            .arg(QLatin1String(selection.c_str()))
            .arg(solid)
            .arg(frenet)
            .arg(QString::fromAscii(d->document.c_str()));

        Gui::Document* doc = Gui::Application::Instance->getDocument(d->document.c_str());
        if (!doc) throw Base::Exception("Document doesn't exist anymore");
        doc->openCommand("Sweep");
        Gui::Application::Instance->runPythonCode((const char*)cmd.toAscii(), false, false);
        doc->getDocument()->recompute();
        App::DocumentObject* obj = doc->getDocument()->getActiveObject();
        if (obj && !obj->isValid()) {
            std::string msg = obj->getStatusString();
            doc->abortCommand();
            throw Base::Exception(msg);
        }
        doc->commitCommand();
    }
    catch (const Base::Exception& e) {
        QMessageBox::warning(this, tr("Input error"), QString::fromAscii(e.what()));
        return false;
    }

    return true;
}

bool SweepWidget::reject()
{
    return true;
}

void SweepWidget::onCurrentItemChanged(QTreeWidgetItem* current, QTreeWidgetItem* previous)
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

void SweepWidget::changeEvent(QEvent *e)
{
    QWidget::changeEvent(e);
    if (e->type() == QEvent::LanguageChange) {
        d->ui.retranslateUi(this);
        d->ui.selector->setAvailableLabel(tr("Vertex/Wire"));
        d->ui.selector->setSelectedLabel(tr("Sweep"));
    }
}


/* TRANSLATOR PartGui::TaskSweep */

TaskSweep::TaskSweep()
{
    widget = new SweepWidget();
    taskbox = new Gui::TaskView::TaskBox(
        Gui::BitmapFactory().pixmap("Part_Sweep"),
        widget->windowTitle(), true, 0);
    taskbox->groupLayout()->addWidget(widget);
    Content.push_back(taskbox);
}

TaskSweep::~TaskSweep()
{
}

void TaskSweep::open()
{
}

void TaskSweep::clicked(int)
{
}

bool TaskSweep::accept()
{
    return widget->accept();
}

bool TaskSweep::reject()
{
    return widget->reject();
}

#include "moc_TaskSweep.cpp"
