/***************************************************************************
 *   Copyright (c) 2009 Juergen Riegel <juergen.riegel@web.de>             *
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
# include <QRegExp>
# include <QString>
#endif


#include "TaskAssemblyConstraints.h"

#include <Constraint.h>

#include <Base/Tools.h>
#include <Base/Console.h>
#include <Gui/Application.h>
#include <Gui/Document.h>
#include <Gui/BitmapFactory.h>
#include <Gui/ViewProvider.h>
#include <Gui/MainWindow.h>
#include <Gui/BitmapFactory.h>
#include <boost/bind.hpp>

using namespace AssemblyGui;
using namespace Gui::TaskView;

extern Assembly::Item* ActiveAsmObject;

TaskAssemblyConstraints::TaskAssemblyConstraints(ViewProviderConstraint* vp)
    : TaskBox(Gui::BitmapFactory().pixmap("document-new"),tr("Constraints"),true, 0), view(vp)
{
    // we need a separate container widget to add all controls to
    proxy = new QWidget(this);
    ui = new Ui::TaskAssemblyConstraints();
    ui->setupUi(proxy);
    this->groupLayout()->addWidget(proxy);

    //initially hide the value and orientation field
    ui->value_widget->hide();
    ui->orientation_widget->hide();

    //set all basic values
    Assembly::Product* ass = NULL;
    Assembly::Constraint* obj =  dynamic_cast<Assembly::Constraint*>(vp->getObject());

    if(obj->First.getValue()) {
        QString str;
        str = QString::fromLatin1(obj->First.getValue()->getNameInDocument()) + QString::fromLatin1(".") + QString::fromStdString(obj->First.getSubValues().front());
        ui->first_geom->setText(str);
        ass = dynamic_cast<Assembly::PartRef*>(obj->First.getValue())->getParentAssembly();
    };

    if(obj->Second.getValue()) {
        QString str;
        str = QString::fromLatin1(obj->Second.getValue()->getNameInDocument()) + QString::fromLatin1(".") + QString::fromStdString(obj->Second.getSubValues().front());
        ui->second_geom->setText(str);

        if(!ass)
            ass = dynamic_cast<Assembly::PartRef*>(obj->Second.getValue())->getParentAssembly();
    };

    //if(ass)
    //    ass->getToplevelAssembly()->execute();
    //else
    //  return;

    //get the individual constraint settings
    ui->value->setValue(obj->Value.getValue());
    setOrientation(dcm::Direction(obj->Orientation.getValue()));
    setSolutionSpace(dcm::SolutionSpace(obj->SolutionSpace.getValue()));

    int v = obj->Type.getValue();

    if(v==0)
        ui->fix->setChecked(true);

    if(v==1)
        ui->distance->setChecked(true);

    if(v==2)
        ui->orientation->setChecked(true);

    if(v==3)
        ui->angle->setChecked(true);

    if(v==4)
        ui->align->setChecked(true);

    if(v==5)
        ui->coincident->setChecked(true);


    setPossibleConstraints();
    setPossibleOptions();

    //setup all signals for event processing
    QObject::connect(
        ui->fix, SIGNAL(toggled(bool)),
        this, SLOT(on_constraint_selection(bool)));
    QObject::connect(
        ui->distance, SIGNAL(toggled(bool)),
        this, SLOT(on_constraint_selection(bool)));
    QObject::connect(
        ui->orientation, SIGNAL(toggled(bool)),
        this, SLOT(on_constraint_selection(bool)));
    QObject::connect(
        ui->angle, SIGNAL(toggled(bool)),
        this, SLOT(on_constraint_selection(bool)));
    QObject::connect(
        ui->align, SIGNAL(toggled(bool)),
        this, SLOT(on_constraint_selection(bool)));
    QObject::connect(
        ui->coincident, SIGNAL(toggled(bool)),
        this, SLOT(on_constraint_selection(bool)));
    QObject::connect(
        ui->parallel, SIGNAL(toggled(bool)),
        this, SLOT(on_orientation_selection(bool)));
    QObject::connect(
        ui->equal, SIGNAL(toggled(bool)),
        this, SLOT(on_orientation_selection(bool)));
    QObject::connect(
        ui->opposite, SIGNAL(toggled(bool)),
        this, SLOT(on_orientation_selection(bool)));
    QObject::connect(
        ui->perpendicular, SIGNAL(toggled(bool)),
        this, SLOT(on_orientation_selection(bool)));
    QObject::connect(
        ui->bidirectional, SIGNAL(toggled(bool)),
        this, SLOT(on_solutionspace_selection(bool)));
    QObject::connect(
        ui->pos_direction, SIGNAL(toggled(bool)),
        this, SLOT(on_solutionspace_selection(bool)));
    QObject::connect(
        ui->neg_direction, SIGNAL(toggled(bool)),
        this, SLOT(on_solutionspace_selection(bool)));
    QObject::connect(
        ui->perpendicular, SIGNAL(toggled(bool)),
        this, SLOT(on_orientation_selection(bool)));
    QObject::connect(
        ui->value, SIGNAL(valueChanged(double)),
        this, SLOT(on_value_change(double)));
    QObject::connect(
        ui->clear_first, SIGNAL(pressed()),
        this, SLOT(on_clear_first()));
    QObject::connect(
        ui->clear_second, SIGNAL(pressed()),
        this, SLOT(on_clear_second()));

}

dcm::Direction TaskAssemblyConstraints::getOrientation()
{
    if(ui->parallel->isChecked())
        return dcm::parallel;

    if(ui->equal->isChecked())
        return dcm::equal;

    if(ui->opposite->isChecked())
        return dcm::opposite;

    return dcm::perpendicular;
}

void TaskAssemblyConstraints::setOrientation(dcm::Direction d)
{
    switch(d) {
    case dcm::perpendicular:
        ui->perpendicular->setChecked(true);
        break;

    case dcm::equal:
        ui->equal->setChecked(true);
        break;

    case dcm::opposite:
        ui->opposite->setChecked(true);
        break;

    default
            :
        ui->parallel->setChecked(true);
    }
}

dcm::SolutionSpace TaskAssemblyConstraints::getSolutionSpace()
{
    if(ui->bidirectional->isChecked())
        return dcm::bidirectional;

    if(ui->pos_direction->isChecked())
        return dcm::positiv_directional;

    return dcm::negative_directional;
}

void TaskAssemblyConstraints::setSolutionSpace(dcm::SolutionSpace d)
{
    switch(d) {
    case dcm::bidirectional:
        ui->bidirectional->setChecked(true);
        break;

    case dcm::positiv_directional:
        ui->pos_direction->setChecked(true);
        break;

    default
            :
        ui->neg_direction->setChecked(true);
    }
}


TaskAssemblyConstraints::~TaskAssemblyConstraints()
{
    delete ui;
}

void TaskAssemblyConstraints::onSelectionChanged(const Gui::SelectionChanges& msg)
{
    //if(msg.Type == Gui::SelectionChanges::AddSelection) {

    //    //add it as the first geometry?
    //    if(ui->first_geom->text().isEmpty()) {
    //        std::vector<Gui::SelectionObject> objs = Gui::Selection().getSelectionEx();
    //        Assembly::Constraint* con =  dynamic_cast<Assembly::Constraint*>(view->getObject());

    //        if(!ActiveAsmObject || !ActiveAsmObject->getTypeId().isDerivedFrom(Assembly::Product::getClassTypeId())) {
    //            QMessageBox::warning(Gui::getMainWindow(), QObject::tr("No active Assembly"),
    //                                 QObject::tr("You need a active (blue) Assembly to insert a Constraint. Please create a new one or make one active (double click)."));
    //            return;
    //        }

    //        std::pair<Assembly::PartRef*, Assembly::Product*> part1 = static_cast<Assembly::Product*>(ActiveAsmObject)->getContainingPart(objs.back().getObject());
    //        con->First.setValue(part1.first, objs.back().getSubNames());
    //        QString str;
    //        str = QString::fromLatin1(part1.first->getNameInDocument()) + QString::fromLatin1(".") + QString::fromStdString(con->First.getSubValues().front());
    //        ui->first_geom->setText(str);

    //        App::GetApplication().getActiveDocument()->recompute();
    //        setPossibleConstraints();
    //        setPossibleOptions();
    //        view->draw();
    //        return;
    //    }

    //    if(ui->second_geom->text().isEmpty()) {
    //        std::vector<Gui::SelectionObject> objs = Gui::Selection().getSelectionEx();
    //        Assembly::Constraint* con =  dynamic_cast<Assembly::Constraint*>(view->getObject());

    //        if(!ActiveAsmObject || !ActiveAsmObject->getTypeId().isDerivedFrom(Assembly::Product::getClassTypeId())) {
    //            QMessageBox::warning(Gui::getMainWindow(), QObject::tr("No active Assembly"),
    //                                 QObject::tr("You need a active (blue) Assembly to insert a Constraint. Please create a new one or make one active (double click)."));
    //            return;
    //        }

    //        std::pair<Assembly::PartRef*, Assembly::Product*> part2 = static_cast<Assembly::Product*>(ActiveAsmObject)->getContainingPart(objs.back().getObject());
    //        con->Second.setValue(part2.first, objs.back().getSubNames());
    //        QString str;
    //        str = QString::fromLatin1(part2.first->getNameInDocument()) + QString::fromLatin1(".") + QString::fromStdString(con->Second.getSubValues().front());
    //        ui->second_geom->setText(str);

    //        App::GetApplication().getActiveDocument()->recompute();
    //        setPossibleConstraints();
    //        setPossibleOptions();
    //        view->draw();
    //        return;
    //    }
    //}
}

void TaskAssemblyConstraints::on_constraint_selection(bool clicked)
{
    if(clicked) {
        Assembly::Constraint* obj =  dynamic_cast<Assembly::Constraint*>(view->getObject());

        if(ui->fix->isChecked())
            obj->Type.setValue("Fix");

        if(ui->distance->isChecked())
            obj->Type.setValue("Distance");

        if(ui->orientation->isChecked())
            obj->Type.setValue("Orientation");

        if(ui->angle->isChecked())
            obj->Type.setValue("Angle");

        if(ui->align->isChecked())
            obj->Type.setValue("Align");

        if(ui->coincident->isChecked())
            obj->Type.setValue("Coincident");

        App::GetApplication().getActiveDocument()->recompute();
        view->draw();
    }

    setPossibleOptions();

}

void TaskAssemblyConstraints::on_orientation_selection(bool clicked)
{
    if(clicked) {
        Assembly::Constraint* obj =  dynamic_cast<Assembly::Constraint*>(view->getObject());
        obj->Orientation.setValue(getOrientation());

        App::GetApplication().getActiveDocument()->recompute();
        view->draw();
    }
}

void TaskAssemblyConstraints::on_solutionspace_selection(bool clicked)
{
    if(clicked) {
        Assembly::Constraint* obj =  dynamic_cast<Assembly::Constraint*>(view->getObject());
        obj->SolutionSpace.setValue(getSolutionSpace());

        App::GetApplication().getActiveDocument()->recompute();
        view->draw();
    }
}

void TaskAssemblyConstraints::on_value_change(double val)
{

    Assembly::Constraint* obj =  dynamic_cast<Assembly::Constraint*>(view->getObject());
    obj->Value.setValue(ui->value->value());

    App::GetApplication().getActiveDocument()->recompute();
    view->draw();
}

void TaskAssemblyConstraints::on_clear_first()
{
    Assembly::Constraint* obj =  dynamic_cast<Assembly::Constraint*>(view->getObject());
    obj->First.setValue(NULL);
    ui->first_geom->clear();
    setPossibleConstraints();
    setPossibleOptions();
    view->draw();
}

void TaskAssemblyConstraints::on_clear_second()
{

    Assembly::Constraint* obj =  dynamic_cast<Assembly::Constraint*>(view->getObject());
    obj->Second.setValue(NULL);
    ui->second_geom->clear();
    setPossibleConstraints();
    setPossibleOptions();
    view->draw();
}

void TaskAssemblyConstraints::setPossibleOptions() {

    ////disable all orientations for later easy disabling
    //ui->parallel->setEnabled(false);
    //ui->equal->setEnabled(false);
    //ui->opposite->setEnabled(false);
    //ui->perpendicular->setEnabled(false);

    ////disable solution spaces for later easy enabling
    //ui->bidirectional->setEnabled(false);
    //ui->pos_direction->setEnabled(false);
    //ui->neg_direction->setEnabled(false);

    ////this only works if both objects are set
    //Assembly::Constraint* obj =  dynamic_cast<Assembly::Constraint*>(view->getObject());

    //if(obj->First.getValue()) {

    //    Assembly::PartRef* p1 = dynamic_cast<Assembly::PartRef*>(obj->First.getValue());

    //    if(!p1)
    //        return;

    //    Assembly::Product* ass = p1->getParentAssembly();

    //    //extract the geometries to use for comparison
    //    boost::shared_ptr<Geometry3D> g1 = ass->m_solver->getGeometry3D(obj->First.getSubValues()[0].c_str());

    //    if(!g1)
    //        return;

    //    if(obj->Second.getValue()) {

    //        Assembly::PartRef* p2 = dynamic_cast<Assembly::PartRef*>(obj->Second.getValue());

    //        if(!p2)
    //            return;

    //        boost::shared_ptr<Geometry3D> g2 = ass->m_solver->getGeometry3D(obj->Second.getSubValues()[0].c_str());

    //        if(!g2)
    //            return;

    //        //distance
    //        if(obj->Type.getValue() == 1) {

    //            if(isCombination(g1,g2, dcm::geometry::point, dcm::geometry::plane) ||
    //                    isCombination(g1,g2, dcm::geometry::point, dcm::geometry::cylinder)) {
    //                ui->bidirectional->setEnabled(true);
    //                ui->pos_direction->setEnabled(true);
    //                ui->neg_direction->setEnabled(true);
    //            };
    //        };

    //        //align & coincident
    //        if(obj->Type.getValue() == 4 || obj->Type.getValue() == 5) {

    //            if(isCombination(g1,g2, dcm::geometry::point, dcm::geometry::plane) ||
    //                    isCombination(g1,g2, dcm::geometry::point, dcm::geometry::cylinder) ||
    //                    isCombination(g1,g2, dcm::geometry::line, dcm::geometry::plane) ||
    //                    isCombination(g1,g2, dcm::geometry::line, dcm::geometry::cylinder)  ||
    //                    isCombination(g1,g2, dcm::geometry::plane, dcm::geometry::plane) ||
    //                    isCombination(g1,g2, dcm::geometry::plane, dcm::geometry::cylinder)) {
    //                ui->bidirectional->setEnabled(true);
    //                ui->pos_direction->setEnabled(true);
    //                ui->neg_direction->setEnabled(true);
    //            };

    //            if(isCombination(g1,g2, dcm::geometry::line, dcm::geometry::cylinder)  ||
    //                    isCombination(g1,g2, dcm::geometry::plane, dcm::geometry::plane) ||
    //                    isCombination(g1,g2, dcm::geometry::line, dcm::geometry::cylinder) ||
    //                    isCombination(g1,g2, dcm::geometry::cylinder, dcm::geometry::cylinder)) {
    //                ui->parallel->setEnabled(true);
    //                ui->equal->setEnabled(true);
    //                ui->opposite->setEnabled(true);

    //                //ensure that perpendicular is not checked
    //                if(ui->perpendicular->isChecked()) {
    //                    ui->parallel->setChecked(true);
    //                    obj->Orientation.setValue((long)0);
    //                }
    //            };

    //            if(isCombination(g1,g2, dcm::geometry::line, dcm::geometry::plane)  ||
    //                    isCombination(g1,g2, dcm::geometry::plane, dcm::geometry::cylinder)) {
    //                ui->perpendicular->setEnabled(true);

    //                //ensure that perpendicular is checked
    //                if(!ui->perpendicular->isChecked()) {
    //                    ui->perpendicular->setChecked(true);
    //                    obj->Orientation.setValue((long)3);
    //                }
    //            };
    //        };

    //        //orientation
    //        if(obj->Type.getValue() == 2) {
    //            ui->parallel->setEnabled(true);
    //            ui->equal->setEnabled(true);
    //            ui->opposite->setEnabled(true);
    //            ui->perpendicular->setEnabled(true);
    //        }

    //    }
    //}
};

void TaskAssemblyConstraints::setPossibleConstraints()
{
    ////disable all constraints for easier enabling
    //ui->fix->setEnabled(false);
    //ui->distance->setEnabled(false);
    //ui->orientation->setEnabled(false);
    //ui->angle->setEnabled(false);
    //ui->align->setEnabled(false);
    //ui->coincident->setEnabled(false);

    //Assembly::Constraint* obj =  dynamic_cast<Assembly::Constraint*>(view->getObject());

    //if(obj->First.getValue()) {

    //    Assembly::PartRef* p1 = dynamic_cast<Assembly::PartRef*>(obj->First.getValue());

    //    if(!p1)
    //        return;

    //    Assembly::Product* ass = p1->getParentAssembly();

    //    //extract the geometries to use for comparison
    //    boost::shared_ptr<Geometry3D> g1 = ass->m_solver->getGeometry3D(obj->First.getSubValues()[0].c_str());

    //    //let's see if we have a part, if not give feedback to the user by color
    //    if(!g1) {
    //        QPalette palette = ui->widget->palette();
    //        palette.setColor(ui->first_geom->backgroundRole(), QColor(255, 0, 0, 127));
    //        ui->first_geom->setPalette(palette);
    //    }
    //    else {
    //        //set normal color as we ma need to revert the red background
    //        ui->first_geom->setPalette(ui->widget->palette());
    //    }

    //    if(obj->Second.getValue()) {

    //        Assembly::PartRef* p2 = dynamic_cast<Assembly::PartRef*>(obj->Second.getValue());

    //        if(!p2)
    //            return;

    //        boost::shared_ptr<Geometry3D> g2 = ass->m_solver->getGeometry3D(obj->Second.getSubValues()[0].c_str());

    //        //let's see if we have a part, if not give feedback to the user by color
    //        if(!g2) {
    //            QPalette palette = ui->widget->palette();
    //            palette.setColor(ui->second_geom->backgroundRole(), QColor(255, 0, 0, 127));
    //            ui->second_geom->setPalette(palette);
    //        }
    //        else {
    //            //set normal color as we ma need to revert the red background
    //            ui->second_geom->setPalette(ui->widget->palette());
    //        }

    //        //return only here to allow coloring both line edits red if needed
    //        if(!g1 || !g2)
    //            return;

    //        //check all valid combinaions
    //        if(isCombination(g1,g2, dcm::geometry::point, dcm::geometry::point)) {
    //            ui->distance->setEnabled(true);
    //            ui->coincident->setEnabled(true);
    //        };

    //        if(isCombination(g1,g2, dcm::geometry::point, dcm::geometry::line)) {
    //            ui->distance->setEnabled(true);
    //            ui->coincident->setEnabled(true);
    //        };

    //        if(isCombination(g1,g2, dcm::geometry::point, dcm::geometry::plane)) {
    //            ui->distance->setEnabled(true);
    //            ui->coincident->setEnabled(true);
    //        };

    //        if(isCombination(g1,g2, dcm::geometry::point, dcm::geometry::cylinder)) {
    //            ui->distance->setEnabled(true);
    //            ui->coincident->setEnabled(true);
    //        };

    //        if(isCombination(g1,g2, dcm::geometry::line, dcm::geometry::line)) {
    //            ui->distance->setEnabled(true);
    //            ui->orientation->setEnabled(true);
    //            ui->angle->setEnabled(true);
    //            ui->coincident->setEnabled(true);
    //            ui->align->setEnabled(true);
    //        };

    //        if(isCombination(g1,g2, dcm::geometry::line, dcm::geometry::plane)) {
    //            ui->orientation->setEnabled(true);
    //            ui->angle->setEnabled(true);
    //            ui->coincident->setEnabled(true);
    //            ui->align->setEnabled(true);
    //        };

    //        if(isCombination(g1,g2, dcm::geometry::line, dcm::geometry::cylinder)) {
    //            ui->distance->setEnabled(true);
    //            ui->orientation->setEnabled(true);
    //            ui->angle->setEnabled(true);
    //            ui->coincident->setEnabled(true);
    //            ui->align->setEnabled(true);
    //        };

    //        if(isCombination(g1,g2, dcm::geometry::plane, dcm::geometry::plane)) {
    //            ui->orientation->setEnabled(true);
    //            ui->angle->setEnabled(true);
    //            ui->coincident->setEnabled(true);
    //            ui->align->setEnabled(true);
    //        };

    //        if(isCombination(g1,g2, dcm::geometry::plane, dcm::geometry::cylinder)) {
    //            ui->orientation->setEnabled(true);
    //            ui->angle->setEnabled(true);
    //            ui->align->setEnabled(true);
    //        };

    //        if(isCombination(g1,g2, dcm::geometry::cylinder, dcm::geometry::cylinder)) {
    //            ui->coincident->setEnabled(true);
    //            ui->orientation->setEnabled(true);
    //            ui->angle->setEnabled(true);
    //        };
    //    }
    //    else {
    //        //return here to allow check for second geometry and color both red if needed
    //        if(!g1)
    //            return;

    //        //only fix works
    //        ui->fix->setEnabled(true);
    //    };
    //}
}

bool TaskAssemblyConstraints::isCombination(boost::shared_ptr<Geometry3D> g1, boost::shared_ptr<Geometry3D> g2, dcm::geometry::types t1, dcm::geometry::types t2)
{
    if(g1->getGeometryType() == t1 && g2->getGeometryType() == t2)
        return true;

    if(g1->getGeometryType() == t2 && g2->getGeometryType() == t1)
        return true;

    return false;
}




#include "moc_TaskAssemblyConstraints.cpp"
