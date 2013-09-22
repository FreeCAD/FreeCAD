/***************************************************************************
 *   Copyright (c) 2009 Werner Mayer <wmayer[at]users.sourceforge.net>     *
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
# include <gp_Dir.hxx>
# include <gp_Lin.hxx>
# include <gp_Pnt.hxx>
# include <BRepAdaptor_Curve.hxx>
# include <TopExp_Explorer.hxx>
# include <TopoDS.hxx>
# include <TopoDS_Edge.hxx>
#endif

#include "ui_DlgRevolution.h"
#include "DlgRevolution.h"
#include "../App/PartFeature.h"
#include <App/Application.h>
#include <App/Document.h>
#include <App/DocumentObject.h>
#include <Gui/Application.h>
#include <Gui/BitmapFactory.h>
#include <Gui/Command.h>
#include <Gui/Document.h>
#include <Gui/Utilities.h>
#include <Gui/ViewProvider.h>
#include <Gui/WaitCursor.h>
#include <Mod/Part/App/Tools.h>

using namespace PartGui;

class DlgRevolution::EdgeSelection : public Gui::SelectionFilterGate
{
public:
    gp_Pnt loc;
    gp_Dir dir;
    bool canSelect;

    EdgeSelection()
        : Gui::SelectionFilterGate((Gui::SelectionFilter*)0)
    {
    }
    bool allow(App::Document*pDoc, App::DocumentObject*pObj, const char*sSubName)
    {
        this->canSelect = false;
        if (!pObj->isDerivedFrom(Part::Feature::getClassTypeId()))
            return false;
        if (!sSubName || sSubName[0] == '\0')
            return false;
        std::string element(sSubName);
        if (element.substr(0,4) != "Edge")
            return false;
        Part::Feature* fea = static_cast<Part::Feature*>(pObj);
        try {
            TopoDS_Shape sub = fea->Shape.getShape().getSubShape(sSubName);
            if (!sub.IsNull() && sub.ShapeType() == TopAbs_EDGE) {
                const TopoDS_Edge& edge = TopoDS::Edge(sub);
                BRepAdaptor_Curve adapt(edge);
                if (adapt.GetType() == GeomAbs_Line) {
                    gp_Lin line = adapt.Line();
                    this->loc = line.Location();
                    this->dir = line.Direction();
                    this->canSelect = true;
                    return true;
                }
            }
        }
        catch (...) {
        }

        return false;
    }
};

DlgRevolution::DlgRevolution(QWidget* parent, Qt::WFlags fl)
  : Gui::LocationDialog(parent, fl), filter(0)
{
    ui = new Ui_RevolutionComp(this);
    ui->xPos->setRange(-DBL_MAX,DBL_MAX);
    ui->yPos->setRange(-DBL_MAX,DBL_MAX);
    ui->zPos->setRange(-DBL_MAX,DBL_MAX);
    findShapes();

    Gui::ItemViewSelection sel(ui->treeWidget);
    sel.applyFrom(Gui::Selection().getObjectsOfType(Part::Feature::getClassTypeId()));
}

/*  
 *  Destroys the object and frees any allocated resources
 */
DlgRevolution::~DlgRevolution()
{
    // no need to delete child widgets, Qt does it all for us
    Gui::Selection().rmvSelectionGate();
    delete ui;
}

void DlgRevolution::directionActivated(int index)
{
    ui->directionActivated(this, index);
}

Base::Vector3d DlgRevolution::getDirection() const
{
    return ui->getDirection();
}

void DlgRevolution::changeEvent(QEvent *e)
{
    if (e->type() == QEvent::LanguageChange) {
        ui->retranslate(this);
    }
    else {
        QDialog::changeEvent(e);
    }
}

void DlgRevolution::findShapes()
{
    App::Document* activeDoc = App::GetApplication().getActiveDocument();
    if (!activeDoc) return;
    Gui::Document* activeGui = Gui::Application::Instance->getDocument(activeDoc);

    std::vector<App::DocumentObject*> objs = activeDoc->getObjectsOfType
        (Part::Feature::getClassTypeId());
    for (std::vector<App::DocumentObject*>::iterator it = objs.begin(); it!=objs.end(); ++it) {
        const TopoDS_Shape& shape = static_cast<Part::Feature*>(*it)->Shape.getValue();
        if (shape.IsNull()) continue;

        TopExp_Explorer xp;
        xp.Init(shape,TopAbs_SOLID);
        if (xp.More()) continue; // solids not allowed
        xp.Init(shape,TopAbs_COMPSOLID);
        if (xp.More()) continue; // compound solids not allowed
        // So allowed are: vertex, edge, wire, face, shell and compound
        QTreeWidgetItem* item = new QTreeWidgetItem(ui->treeWidget);
        item->setText(0, QString::fromUtf8((*it)->Label.getValue()));
        item->setData(0, Qt::UserRole, QString::fromAscii((*it)->getNameInDocument()));
        Gui::ViewProvider* vp = activeGui->getViewProvider(*it);
        if (vp) item->setIcon(0, vp->getIcon());
    }
}

void DlgRevolution::accept()
{
    if (ui->treeWidget->selectedItems().isEmpty()) {
        QMessageBox::critical(this, windowTitle(), 
            tr("Select a shape for revolution, first."));
        return;
    }

    Gui::WaitCursor wc;
    App::Document* activeDoc = App::GetApplication().getActiveDocument();
    activeDoc->openTransaction("Revolve");

    QString shape, type, name;
    QList<QTreeWidgetItem *> items = ui->treeWidget->selectedItems();
    for (QList<QTreeWidgetItem *>::iterator it = items.begin(); it != items.end(); ++it) {
        shape = (*it)->data(0, Qt::UserRole).toString();
        type = QString::fromAscii("Part::Revolution");
        name = QString::fromAscii(activeDoc->getUniqueObjectName("Revolve").c_str());
        Base::Vector3d axis = this->getDirection();

        QString code = QString::fromAscii(
            "FreeCAD.ActiveDocument.addObject(\"%1\",\"%2\")\n"
            "FreeCAD.ActiveDocument.%2.Source = FreeCAD.ActiveDocument.%3\n"
            "FreeCAD.ActiveDocument.%2.Axis = (%4,%5,%6)\n"
            "FreeCAD.ActiveDocument.%2.Base = (%7,%8,%9)\n"
            "FreeCAD.ActiveDocument.%2.Angle = %10\n"
            "FreeCADGui.ActiveDocument.%3.Visibility = False\n")
            .arg(type).arg(name).arg(shape)
            .arg(axis.x,0,'f',2)
            .arg(axis.y,0,'f',2)
            .arg(axis.z,0,'f',2)
            .arg(ui->xPos->value(),0,'f',2)
            .arg(ui->yPos->value(),0,'f',2)
            .arg(ui->zPos->value(),0,'f',2)
            .arg(ui->angle->value(),0,'f',2);
        Gui::Application::Instance->runPythonCode((const char*)code.toAscii());
        QByteArray to = name.toAscii();
        QByteArray from = shape.toAscii();
        Gui::Command::copyVisual(to, "ShapeColor", from);
        Gui::Command::copyVisual(to, "LineColor", from);
        Gui::Command::copyVisual(to, "PointColor", from);
    }

    activeDoc->commitTransaction();
    activeDoc->recompute();
    QDialog::accept();
}

void DlgRevolution::on_selectLine_clicked()
{
    if (!filter) {
        filter = new EdgeSelection();
        Gui::Selection().addSelectionGate(filter);
    }
}

void DlgRevolution::onSelectionChanged(const Gui::SelectionChanges& msg)
{
    if (msg.Type == Gui::SelectionChanges::AddSelection) {
        if (filter && filter->canSelect) {
            ui->setPosition (Base::convertTo<Base::Vector3d>(filter->loc));
            ui->setDirection(Base::convertTo<Base::Vector3d>(filter->dir));
        }
    }
}

// ---------------------------------------

TaskRevolution::TaskRevolution()
{
    widget = new DlgRevolution();
    taskbox = new Gui::TaskView::TaskBox(
        Gui::BitmapFactory().pixmap("Part_Revolve"),
        widget->windowTitle(), true, 0);
    taskbox->groupLayout()->addWidget(widget);
    Content.push_back(taskbox);
}

TaskRevolution::~TaskRevolution()
{
    // automatically deleted in the sub-class
}

bool TaskRevolution::accept()
{
    widget->accept();
    return (widget->result() == QDialog::Accepted);
}

#include "moc_DlgRevolution.cpp"
