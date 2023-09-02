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

#include <BRepBuilderAPI_MakePolygon.hxx>
#include <BRep_Builder.hxx>
#include <GeomAPI_ProjectPointOnSurf.hxx>
#include <Geom_Plane.hxx>
#include <Standard_Failure.hxx>
#include <TopoDS_Compound.hxx>
#include <TopoDS_Wire.hxx>
#endif

#include <App/Application.h>
#include <App/Document.h>
#include <App/DocumentObjectGroup.h>
#include <Base/Console.h>
#include <Gui/WaitCursor.h>
#include <Mod/Mesh/App/Core/Algorithm.h>
#include <Mod/Mesh/App/Core/Approximation.h>
#include <Mod/Mesh/App/Core/Curvature.h>
#include <Mod/Mesh/App/Core/Segmentation.h>
#include <Mod/Mesh/App/Core/Smoothing.h>
#include <Mod/Mesh/App/MeshFeature.h>
#include <Mod/Part/App/FaceMakerCheese.h>
#include <Mod/Part/App/PartFeature.h>

#include "Segmentation.h"
#include "ui_Segmentation.h"


using namespace ReverseEngineeringGui;

Segmentation::Segmentation(Mesh::Feature* mesh, QWidget* parent, Qt::WindowFlags fl)
    : QWidget(parent, fl)
    , ui(new Ui_Segmentation)
    , myMesh(mesh)
{
    ui->setupUi(this);
    ui->numPln->setRange(1, INT_MAX);
    ui->numPln->setValue(100);

    ui->checkBoxSmooth->setChecked(false);
}

Segmentation::~Segmentation() = default;

void Segmentation::accept()
{
    if (myMesh.expired()) {
        return;
    }

    Gui::WaitCursor wc;
    bool createUnused = ui->createUnused->isChecked();
    bool createCompound = ui->createCompound->isChecked();
    BRep_Builder builder;
    TopoDS_Compound compound;
    builder.MakeCompound(compound);

    const Mesh::MeshObject* mesh = myMesh.get<Mesh::Feature>()->Mesh.getValuePtr();
    // make a copy because we might smooth the mesh before
    MeshCore::MeshKernel kernel = mesh->getKernel();
    MeshCore::MeshAlgorithm algo(kernel);

    if (ui->checkBoxSmooth->isChecked()) {
        MeshCore::LaplaceSmoothing smoother(kernel);
        smoother.Smooth(ui->smoothSteps->value());
    }

    MeshCore::MeshSegmentAlgorithm finder(kernel);
    MeshCore::MeshCurvature meshCurv(kernel);
    meshCurv.ComputePerVertex();

    // First create segments by curavture to get the surface type
    std::vector<MeshCore::MeshSurfaceSegmentPtr> segm;
    if (ui->groupBoxPln->isChecked()) {
        segm.emplace_back(
            std::make_shared<MeshCore::MeshCurvaturePlanarSegment>(meshCurv.GetCurvature(),
                                                                   ui->numPln->value(),
                                                                   ui->curvTolPln->value()));
    }
    finder.FindSegments(segm);

    std::vector<MeshCore::MeshSurfaceSegmentPtr> segmSurf;
    for (const auto& it : segm) {
        const std::vector<MeshCore::MeshSegment>& data = it->GetSegments();

        // For each planar segment compute a plane and use this then for a more accurate 2nd
        // segmentation
        if (strcmp(it->GetType(), "Plane") == 0) {
            for (const auto& jt : data) {
                std::vector<MeshCore::PointIndex> indexes = kernel.GetFacetPoints(jt);
                MeshCore::PlaneFit fit;
                fit.AddPoints(kernel.GetPoints(indexes));
                if (fit.Fit() < FLOAT_MAX) {
                    Base::Vector3f base = fit.GetBase();
                    Base::Vector3f axis = fit.GetNormal();
                    MeshCore::AbstractSurfaceFit* fitter =
                        new MeshCore::PlaneSurfaceFit(base, axis);
                    segmSurf.emplace_back(
                        std::make_shared<MeshCore::MeshDistanceGenericSurfaceFitSegment>(
                            fitter,
                            kernel,
                            ui->numPln->value(),
                            ui->distToPln->value()));
                }
            }
        }
    }
    finder.FindSegments(segmSurf);

    App::Document* document = App::GetApplication().getActiveDocument();
    document->openTransaction("Segmentation");

    std::string internalname = "Segments_";
    internalname += myMesh->getNameInDocument();

    App::DocumentObjectGroup* group = static_cast<App::DocumentObjectGroup*>(
        document->addObject("App::DocumentObjectGroup", internalname.c_str()));
    std::string labelname = "Segments ";
    labelname += myMesh->Label.getValue();
    group->Label.setValue(labelname);

    std::vector<App::DocumentObject*> failures;
    algo.SetFacetFlag(MeshCore::MeshFacet::TMP0);

    for (const auto& it : segmSurf) {
        const std::vector<MeshCore::MeshSegment>& data = it->GetSegments();
        std::shared_ptr<MeshCore::MeshDistanceGenericSurfaceFitSegment> genSegm =
            std::dynamic_pointer_cast<MeshCore::MeshDistanceGenericSurfaceFitSegment>(it);

        bool isPlanar = (strcmp(genSegm->GetType(), "Plane") == 0);
        for (const auto& jt : data) {
            // reset flag for facets of segment
            algo.ResetFacetsFlag(jt, MeshCore::MeshFacet::TMP0);

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

            if (createCompound) {
                std::list<std::vector<Base::Vector3f>> bounds;
                algo.GetFacetBorders(jt, bounds);

                // Handle planar segments
                if (isPlanar) {
                    std::vector<float> par = genSegm->Parameters();
                    gp_Pnt loc(par.at(0), par.at(1), par.at(2));
                    gp_Dir dir(par.at(3), par.at(4), par.at(5));

                    Handle(Geom_Plane) hPlane(new Geom_Plane(loc, dir));

                    std::vector<TopoDS_Wire> wires;
                    for (const auto& bound : bounds) {
                        // project the points onto the surface
                        std::vector<gp_Pnt> polygon;
                        std::transform(
                            bound.begin(),
                            bound.end(),
                            std::back_inserter(polygon),
                            [&hPlane](const Base::Vector3f& v) {
                                gp_Pnt p(v.x, v.y, v.z);
                                return GeomAPI_ProjectPointOnSurf(p, hPlane).NearestPoint();
                            });

                        BRepBuilderAPI_MakePolygon mkPoly;
                        for (std::vector<gp_Pnt>::reverse_iterator it = polygon.rbegin();
                             it != polygon.rend();
                             ++it) {
                            mkPoly.Add(*it);
                        }
                        if (mkPoly.IsDone()) {
                            wires.push_back(mkPoly.Wire());
                        }
                    }

                    try {
                        TopoDS_Shape shape = Part::FaceMakerCheese::makeFace(wires);
                        if (!shape.IsNull()) {
                            builder.Add(compound, shape);
                        }
                        else {
                            failures.push_back(feaSegm);
                            Base::Console().Warning("Failed to create face from %s\n",
                                                    feaSegm->Label.getValue());
                        }
                    }
                    catch (Standard_Failure&) {
                        failures.push_back(feaSegm);
                        Base::Console().Error("Fatal failure to create face from %s\n",
                                              feaSegm->Label.getValue());
                    }
                }
            }
        }
    }

    if (createUnused) {
        // collect all facets that don't have set the flag TMP0
        std::vector<MeshCore::FacetIndex> unusedFacets;
        algo.GetFacetsFlag(unusedFacets, MeshCore::MeshFacet::TMP0);

        if (!unusedFacets.empty()) {
            std::unique_ptr<Mesh::MeshObject> segment(mesh->meshFromSegment(unusedFacets));
            Mesh::Feature* feaSegm =
                static_cast<Mesh::Feature*>(group->addObject("Mesh::Feature", "Unused"));
            Mesh::MeshObject* feaMesh = feaSegm->Mesh.startEditing();
            feaMesh->swap(*segment);
            feaSegm->Mesh.finishEditing();
        }
    }
    if (createCompound) {
        Part::Feature* shapeFea =
            static_cast<Part::Feature*>(group->addObject("Part::Feature", "Compound"));
        shapeFea->Shape.setValue(compound);

        // create a sub-group where to move the problematic segments
        if (!failures.empty()) {
            App::DocumentObjectGroup* subgroup = static_cast<App::DocumentObjectGroup*>(
                group->addObject("App::DocumentObjectGroup", "Failed"));
            failures = group->removeObjects(failures);
            subgroup->Group.setValues(failures);
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
    widget = new Segmentation(mesh);
    taskbox = new Gui::TaskView::TaskBox(QPixmap(), widget->windowTitle(), false, nullptr);
    taskbox->groupLayout()->addWidget(widget);
    Content.push_back(taskbox);
}

bool TaskSegmentation::accept()
{
    widget->accept();
    return true;
}

#include "moc_Segmentation.cpp"
