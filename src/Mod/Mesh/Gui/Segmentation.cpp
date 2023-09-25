/***************************************************************************
 *   Copyright (c) 2012 Werner Mayer <wmayer[at]users.sourceforge.net>     *
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
#include <sstream>
#endif

#include <App/Application.h>
#include <App/Document.h>
#include <App/DocumentObjectGroup.h>
#include <Mod/Mesh/App/Core/Curvature.h>
#include <Mod/Mesh/App/Core/Segmentation.h>
#include <Mod/Mesh/App/Core/Smoothing.h>
#include <Mod/Mesh/App/MeshFeature.h>

#include "Segmentation.h"
#include "ui_Segmentation.h"


using namespace MeshGui;

Segmentation::Segmentation(Mesh::Feature* mesh, QWidget* parent, Qt::WindowFlags fl)
    : QWidget(parent, fl)
    , ui(new Ui_Segmentation)
    , myMesh(mesh)
{
    ui->setupUi(this);
    ui->numPln->setRange(1, INT_MAX);
    ui->numPln->setValue(100);
    ui->crvCyl->setRange(0, INT_MAX);
    ui->numCyl->setRange(1, INT_MAX);
    ui->numCyl->setValue(100);
    ui->crvSph->setRange(0, INT_MAX);
    ui->numSph->setRange(1, INT_MAX);
    ui->numSph->setValue(100);
    ui->crv1Free->setRange(-INT_MAX, INT_MAX);
    ui->crv2Free->setRange(-INT_MAX, INT_MAX);
    ui->numFree->setRange(1, INT_MAX);
    ui->numFree->setValue(100);

    ui->checkBoxSmooth->setChecked(false);
}

Segmentation::~Segmentation()
{
    // no need to delete child widgets, Qt does it all for us
    delete ui;
}

void Segmentation::accept()
{
    const Mesh::MeshObject* mesh = myMesh->Mesh.getValuePtr();
    // make a copy because we might smooth the mesh before
    MeshCore::MeshKernel kernel = mesh->getKernel();

    if (ui->checkBoxSmooth->isChecked()) {
        MeshCore::LaplaceSmoothing smoother(kernel);
        smoother.Smooth(ui->smoothSteps->value());
    }

    MeshCore::MeshSegmentAlgorithm finder(kernel);
    MeshCore::MeshCurvature meshCurv(kernel);
    meshCurv.ComputePerVertex();

    std::vector<MeshCore::MeshSurfaceSegmentPtr> segm;
    if (ui->groupBoxFree->isChecked()) {
        segm.emplace_back(
            std::make_shared<MeshCore::MeshCurvatureFreeformSegment>(meshCurv.GetCurvature(),
                                                                     ui->numFree->value(),
                                                                     ui->tol1Free->value(),
                                                                     ui->tol2Free->value(),
                                                                     ui->crv1Free->value(),
                                                                     ui->crv2Free->value()));
    }
    if (ui->groupBoxCyl->isChecked()) {
        segm.emplace_back(
            std::make_shared<MeshCore::MeshCurvatureCylindricalSegment>(meshCurv.GetCurvature(),
                                                                        ui->numCyl->value(),
                                                                        ui->tol1Cyl->value(),
                                                                        ui->tol2Cyl->value(),
                                                                        ui->crvCyl->value()));
    }
    if (ui->groupBoxSph->isChecked()) {
        segm.emplace_back(
            std::make_shared<MeshCore::MeshCurvatureSphericalSegment>(meshCurv.GetCurvature(),
                                                                      ui->numSph->value(),
                                                                      ui->tolSph->value(),
                                                                      ui->crvSph->value()));
    }
    if (ui->groupBoxPln->isChecked()) {
        segm.emplace_back(
            std::make_shared<MeshCore::MeshCurvaturePlanarSegment>(meshCurv.GetCurvature(),
                                                                   ui->numPln->value(),
                                                                   ui->tolPln->value()));
    }
    finder.FindSegments(segm);

    App::Document* document = App::GetApplication().getActiveDocument();
    document->openTransaction("Segmentation");

    std::string internalname = "Segments_";
    internalname += myMesh->getNameInDocument();
    App::DocumentObjectGroup* group = static_cast<App::DocumentObjectGroup*>(
        document->addObject("App::DocumentObjectGroup", internalname.c_str()));
    std::string labelname = "Segments ";
    labelname += myMesh->Label.getValue();
    group->Label.setValue(labelname);
    for (const auto& it : segm) {
        const std::vector<MeshCore::MeshSegment>& data = it->GetSegments();
        for (const auto& jt : data) {
            Mesh::MeshObject* segment = mesh->meshFromSegment(jt);
            Mesh::Feature* feaSegm =
                static_cast<Mesh::Feature*>(group->addObject("Mesh::Feature", "Segment"));
            Mesh::MeshObject* feaMesh = feaSegm->Mesh.startEditing();
            feaMesh->swap(*segment);
            feaSegm->Mesh.finishEditing();
            delete segment;

            std::stringstream label;
            label << feaSegm->Label.getValue() << " (" << it->GetType() << ")";
            feaSegm->Label.setValue(label.str());
        }
    }
    document->commitTransaction();
}

void Segmentation::changeEvent(QEvent* e)
{
    if (e->type() == QEvent::LanguageChange) {
        ui->retranslateUi(this);
    }
    QWidget::changeEvent(e);
}

// ---------------------------------------

/* TRANSLATOR MeshGui::TaskRemoveComponents */

TaskSegmentation::TaskSegmentation(Mesh::Feature* mesh)
{
    widget = new Segmentation(mesh);  // NOLINT
    taskbox = new Gui::TaskView::TaskBox(QPixmap(), widget->windowTitle(), false, nullptr);
    taskbox->groupLayout()->addWidget(widget);
    Content.push_back(taskbox);
}

bool TaskSegmentation::accept()
{
    widget->accept();
    return true;
}
