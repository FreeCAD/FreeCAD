/***************************************************************************
 *   Copyright (c) 2008 Werner Mayer <wmayer[at]users.sourceforge.net>     *
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
# include <BRepAdaptor_Surface.hxx>
# include <BRepLProp_SLProps.hxx>
# include <BRepGProp_Face.hxx>
# include <Precision.hxx>
# include <TopoDS.hxx>
# include <TopoDS_Face.hxx>
# include <TopExp_Explorer.hxx>
# include <QMessageBox>
#endif

#include "ui_DlgExtrusion.h"
#include "DlgExtrusion.h"
#include "../App/PartFeature.h"
#include <Base/Console.h>
#include <App/Application.h>
#include <App/Document.h>
#include <App/DocumentObject.h>
#include <Gui/Application.h>
#include <Gui/BitmapFactory.h>
#include <Gui/Command.h>
#include <Gui/Document.h>
#include <Gui/ViewProvider.h>
#include <Gui/WaitCursor.h>
#include <Gui/Utilities.h>

using namespace PartGui;

DlgExtrusion::DlgExtrusion(QWidget* parent, Qt::WFlags fl)
  : QDialog(parent, fl), ui(new Ui_DlgExtrusion)
{
    ui->setupUi(this);
    ui->labelNormal->hide();
    ui->viewButton->hide();
    ui->dirLen->setMinimumWidth(55); // needed to show all digits
    findShapes();

    Gui::ItemViewSelection sel(ui->treeWidget);
    sel.applyFrom(Gui::Selection().getObjectsOfType(Part::Feature::getClassTypeId()));
}

/*  
 *  Destroys the object and frees any allocated resources
 */
DlgExtrusion::~DlgExtrusion()
{
    // no need to delete child widgets, Qt does it all for us
    delete ui;
}

void DlgExtrusion::changeEvent(QEvent *e)
{
    if (e->type() == QEvent::LanguageChange) {
        ui->retranslateUi(this);
    }
    QDialog::changeEvent(e);
}

void DlgExtrusion::findShapes()
{
    App::Document* activeDoc = App::GetApplication().getActiveDocument();
    if (!activeDoc) return;
    Gui::Document* activeGui = Gui::Application::Instance->getDocument(activeDoc);
    this->document = activeDoc->getName();
    this->label = activeDoc->Label.getValue();

    std::vector<App::DocumentObject*> objs = activeDoc->getObjectsOfType
        (Part::Feature::getClassTypeId());
    for (std::vector<App::DocumentObject*>::iterator it = objs.begin(); it!=objs.end(); ++it) {
        const TopoDS_Shape& shape = static_cast<Part::Feature*>(*it)->Shape.getValue();
        if (canExtrude(shape)) {
            QTreeWidgetItem* item = new QTreeWidgetItem(ui->treeWidget);
            item->setText(0, QString::fromUtf8((*it)->Label.getValue()));
            item->setData(0, Qt::UserRole, QString::fromAscii((*it)->getNameInDocument()));
            Gui::ViewProvider* vp = activeGui->getViewProvider(*it);
            if (vp)
                item->setIcon(0, vp->getIcon());
        }
    }
}

bool DlgExtrusion::canExtrude(const TopoDS_Shape& shape) const
{
    if (shape.IsNull())
        return false;
    TopAbs_ShapeEnum type = shape.ShapeType();
    if (type == TopAbs_VERTEX || type == TopAbs_EDGE ||
        type == TopAbs_WIRE || type == TopAbs_FACE ||
        type == TopAbs_SHELL)
        return true;
    if (type == TopAbs_COMPOUND) {
        TopExp_Explorer xp;
        xp.Init(shape,TopAbs_SOLID);
        while (xp.More()) {
            return false;
        }
        xp.Init(shape,TopAbs_COMPSOLID);
        while (xp.More()) {
            return false;
        }

        return true;
    }

    return false;
}

void DlgExtrusion::accept()
{
    apply();
    QDialog::accept();
}

void DlgExtrusion::apply()
{
    if (ui->treeWidget->selectedItems().isEmpty()) {
        QMessageBox::critical(this, windowTitle(), 
            tr("Select a shape for extrusion, first."));
        return;
    }

    Gui::WaitCursor wc;
    App::Document* activeDoc = App::GetApplication().getDocument(this->document.c_str());
    if (!activeDoc) {
        QMessageBox::critical(this, windowTitle(), 
            tr("The document '%1' doesn't exist.").arg(QString::fromUtf8(this->label.c_str())));
        return;
    }
    activeDoc->openTransaction("Extrude");

    QString shape, type, name;
    QList<QTreeWidgetItem *> items = ui->treeWidget->selectedItems();
    for (QList<QTreeWidgetItem *>::iterator it = items.begin(); it != items.end(); ++it) {
        shape = (*it)->data(0, Qt::UserRole).toString();
        type = QString::fromAscii("Part::Extrusion");
        name = QString::fromAscii(activeDoc->getUniqueObjectName("Extrude").c_str());
        double len = ui->dirLen->value();
        double dirX = ui->dirX->value();
        double dirY = ui->dirY->value();
        double dirZ = ui->dirZ->value();
        double angle = ui->taperAngle->value();
        bool makeSolid = ui->makeSolid->isChecked();

        // inspect geometry
        App::DocumentObject* obj = activeDoc->getObject((const char*)shape.toAscii());
        if (!obj || !obj->isDerivedFrom(Part::Feature::getClassTypeId())) continue;
        Part::Feature* fea = static_cast<Part::Feature*>(obj);
        const TopoDS_Shape& data = fea->Shape.getValue();
        if (data.IsNull()) continue;

        // check for planes
        if (ui->checkNormal->isChecked() && data.ShapeType() == TopAbs_FACE) {
            BRepAdaptor_Surface adapt(TopoDS::Face(data));
            if (adapt.GetType() == GeomAbs_Plane) {
                double u = 0.5*(adapt.FirstUParameter() + adapt.LastUParameter());
                double v = 0.5*(adapt.FirstVParameter() + adapt.LastVParameter());
                BRepLProp_SLProps prop(adapt,u,v,1,Precision::Confusion());
                if (prop.IsNormalDefined()) {
                    gp_Pnt pnt; gp_Vec vec;
                    // handles the orientation state of the shape
                    BRepGProp_Face(TopoDS::Face(data)).Normal(u,v,pnt,vec);
                    dirX = vec.X();
                    dirY = vec.Y();
                    dirZ = vec.Z();
                }
            }
        }

        QString code = QString::fromAscii(
            "FreeCAD.getDocument(\"%1\").addObject(\"%2\",\"%3\")\n"
            "FreeCAD.getDocument(\"%1\").%3.Base = FreeCAD.getDocument(\"%1\").%4\n"
            "FreeCAD.getDocument(\"%1\").%3.Dir = (%5,%6,%7)\n"
            "FreeCAD.getDocument(\"%1\").%3.Solid = (%8)\n"
            "FreeCAD.getDocument(\"%1\").%3.TaperAngle = (%9)\n"
            "FreeCADGui.getDocument(\"%1\").%4.Visibility = False\n")
            .arg(QString::fromAscii(this->document.c_str()))
            .arg(type).arg(name).arg(shape)
            .arg(dirX*len)
            .arg(dirY*len)
            .arg(dirZ*len)
            .arg(makeSolid ? QLatin1String("True") : QLatin1String("False"))
            .arg(angle);
        Gui::Application::Instance->runPythonCode((const char*)code.toAscii());
        QByteArray to = name.toAscii();
        QByteArray from = shape.toAscii();
        Gui::Command::copyVisual(to, "ShapeColor", from);
        Gui::Command::copyVisual(to, "LineColor", from);
        Gui::Command::copyVisual(to, "PointColor", from);
    }

    activeDoc->commitTransaction();
    try {
        activeDoc->recompute();
    }
    catch (const std::exception& e) {
        Base::Console().Error("%s\n", e.what());
    }
    catch (const Base::Exception& e) {
        Base::Console().Error("%s\n", e.what());
    }
    catch (...) {
        Base::Console().Error("General error while extruding\n");
    }
}

void DlgExtrusion::on_checkNormal_toggled(bool b)
{
    //ui->dirX->setDisabled(b);
    //ui->dirY->setDisabled(b);
    //ui->dirZ->setDisabled(b);
    ui->labelNormal->setVisible(b);
}

// ---------------------------------------

TaskExtrusion::TaskExtrusion()
{
    widget = new DlgExtrusion();
    taskbox = new Gui::TaskView::TaskBox(
        Gui::BitmapFactory().pixmap("Part_Extrude"),
        widget->windowTitle(), true, 0);
    taskbox->groupLayout()->addWidget(widget);
    Content.push_back(taskbox);
}

TaskExtrusion::~TaskExtrusion()
{
    // automatically deleted in the sub-class
}

bool TaskExtrusion::accept()
{
    widget->accept();
    return (widget->result() == QDialog::Accepted);
}

void TaskExtrusion::clicked(int id)
{
    if (id == QDialogButtonBox::Apply) {
        widget->apply();
    }
}

#include "moc_DlgExtrusion.cpp"
