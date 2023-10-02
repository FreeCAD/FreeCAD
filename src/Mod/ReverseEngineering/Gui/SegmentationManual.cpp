/***************************************************************************
 *   Copyright (c) 2020 Werner Mayer <wmayer[at]users.sourceforge.net>     *
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
#include <QPushButton>
#endif

#include <App/Document.h>
#include <Gui/Application.h>
#include <Gui/Document.h>
#include <Gui/Selection.h>
#include <Mod/Mesh/App/Core/Algorithm.h>
#include <Mod/Mesh/App/Core/Approximation.h>
#include <Mod/Mesh/App/Core/Segmentation.h>
#include <Mod/Mesh/App/MeshFeature.h>
#include <Mod/Mesh/Gui/ViewProvider.h>

#include "SegmentationManual.h"
#include "ui_SegmentationManual.h"


using namespace ReverseEngineeringGui;

SegmentationManual::SegmentationManual(QWidget* parent, Qt::WindowFlags fl)
    : QWidget(parent, fl)
    , ui(new Ui_SegmentationManual)
{
    ui->setupUi(this);
    setupConnections();
    ui->spSelectComp->setRange(1, INT_MAX);
    ui->spSelectComp->setValue(10);

    Gui::Selection().clearSelection();
    meshSel.setCheckOnlyVisibleTriangles(ui->visibleTriangles->isChecked());
    meshSel.setCheckOnlyPointToUserTriangles(ui->screenTriangles->isChecked());
    meshSel.setEnabledViewerSelection(false);
}

SegmentationManual::~SegmentationManual() = default;

void SegmentationManual::setupConnections()
{
    connect(ui->selectRegion,
            &QPushButton::clicked,
            this,
            &SegmentationManual::onSelectRegionClicked);
    connect(ui->selectAll, &QPushButton::clicked, this, &SegmentationManual::onSelectAllClicked);
    connect(ui->selectComponents,
            &QPushButton::clicked,
            this,
            &SegmentationManual::onSelectComponentsClicked);
    connect(ui->selectTriangle,
            &QPushButton::clicked,
            this,
            &SegmentationManual::onSelectTriangleClicked);
    connect(ui->deselectAll,
            &QPushButton::clicked,
            this,
            &SegmentationManual::onDeselectAllClicked);
    connect(ui->visibleTriangles,
            &QCheckBox::toggled,
            this,
            &SegmentationManual::onVisibleTrianglesToggled);
    connect(ui->screenTriangles,
            &QCheckBox::toggled,
            this,
            &SegmentationManual::onScreenTrianglesToggled);
    connect(ui->cbSelectComp, &QCheckBox::toggled, this, &SegmentationManual::onSelectCompToggled);
    connect(ui->planeDetect,
            &QPushButton::clicked,
            this,
            &SegmentationManual::onPlaneDetectClicked);
    connect(ui->cylinderDetect,
            &QPushButton::clicked,
            this,
            &SegmentationManual::onCylinderDetectClicked);
    connect(ui->sphereDetect,
            &QPushButton::clicked,
            this,
            &SegmentationManual::onSphereDetectClicked);
}

void SegmentationManual::changeEvent(QEvent* e)
{
    if (e->type() == QEvent::LanguageChange) {
        ui->retranslateUi(this);
    }
    QWidget::changeEvent(e);
}

void SegmentationManual::onSelectRegionClicked()
{
    meshSel.startSelection();
}

void SegmentationManual::onSelectAllClicked()
{
    // select the complete meshes
    meshSel.fullSelection();
}

void SegmentationManual::onDeselectAllClicked()
{
    // deselect all meshes
    meshSel.clearSelection();
}

void SegmentationManual::onSelectComponentsClicked()
{
    // select components up to a certain size
    int size = ui->spSelectComp->value();
    meshSel.selectComponent(size);
}

void SegmentationManual::onVisibleTrianglesToggled(bool on)
{
    meshSel.setCheckOnlyVisibleTriangles(on);
}

void SegmentationManual::onScreenTrianglesToggled(bool on)
{
    meshSel.setCheckOnlyPointToUserTriangles(on);
}

void SegmentationManual::onSelectCompToggled(bool on)
{
    meshSel.setAddComponentOnClick(on);
}

class SegmentationManual::Private
{
public:
    static void findGeometry(
        int minFaces,
        double tolerance,
        std::function<MeshCore::AbstractSurfaceFit*(const std::vector<Base::Vector3f>&,
                                                    const std::vector<Base::Vector3f>&)> fitFunc)
    {
        Gui::Document* gdoc = Gui::Application::Instance->activeDocument();
        if (!gdoc) {
            return;
        }

        App::Document* adoc = gdoc->getDocument();
        std::vector<Mesh::Feature*> meshes = adoc->getObjectsOfType<Mesh::Feature>();
        for (auto it : meshes) {
            MeshGui::ViewProviderMesh* vpm =
                static_cast<MeshGui::ViewProviderMesh*>(gdoc->getViewProvider(it));
            const Mesh::MeshObject& mesh = it->Mesh.getValue();

            if (mesh.hasSelectedFacets()) {
                const MeshCore::MeshKernel& kernel = mesh.getKernel();

                std::vector<MeshCore::FacetIndex> facets;
                std::vector<MeshCore::PointIndex> vertexes;
                mesh.getFacetsFromSelection(facets);
                vertexes = mesh.getPointsFromFacets(facets);
                MeshCore::MeshPointArray coords = kernel.GetPoints(vertexes);

                std::vector<Base::Vector3f> points, normals;
                normals = kernel.GetFacetNormals(facets);
                points.insert(points.end(), coords.begin(), coords.end());
                coords.clear();

                MeshCore::AbstractSurfaceFit* surfFit = fitFunc(points, normals);
                if (surfFit) {
                    MeshCore::MeshSegmentAlgorithm finder(kernel);

                    std::vector<MeshCore::MeshSurfaceSegmentPtr> segm;
                    segm.emplace_back(
                        std::make_shared<MeshCore::MeshDistanceGenericSurfaceFitSegment>(
                            surfFit,
                            kernel,
                            minFaces,
                            tolerance));
                    finder.FindSegments(segm);

                    for (const auto& segmIt : segm) {
                        const std::vector<MeshCore::MeshSegment>& data = segmIt->GetSegments();
                        for (const auto& dataIt : data) {
                            vpm->addSelection(dataIt);
                        }
                    }
                }
            }
        }
    }
};

void SegmentationManual::onPlaneDetectClicked()
{
    auto func = [=](const std::vector<Base::Vector3f>& points,
                    const std::vector<Base::Vector3f>& normal) -> MeshCore::AbstractSurfaceFit* {
        Q_UNUSED(normal)

        MeshCore::PlaneFit fit;
        fit.AddPoints(points);
        if (fit.Fit() < FLOAT_MAX) {
            Base::Vector3f base = fit.GetBase();
            Base::Vector3f axis = fit.GetNormal();
            return new MeshCore::PlaneSurfaceFit(base, axis);
        }

        return nullptr;
    };
    Private::findGeometry(ui->numPln->value(), ui->tolPln->value(), func);
}

void SegmentationManual::onCylinderDetectClicked()
{
    auto func = [=](const std::vector<Base::Vector3f>& points,
                    const std::vector<Base::Vector3f>& normal) -> MeshCore::AbstractSurfaceFit* {
        Q_UNUSED(normal)

        MeshCore::CylinderFit fit;
        fit.AddPoints(points);
        if (!normal.empty()) {
            Base::Vector3f base = fit.GetGravity();
            Base::Vector3f axis = fit.GetInitialAxisFromNormals(normal);
            fit.SetInitialValues(base, axis);
        }
        if (fit.Fit() < FLOAT_MAX) {
            Base::Vector3f base = fit.GetBase();
            Base::Vector3f axis = fit.GetAxis();
            float radius = fit.GetRadius();
            return new MeshCore::CylinderSurfaceFit(base, axis, radius);
        }

        return nullptr;
    };
    Private::findGeometry(ui->numCyl->value(), ui->tolCyl->value(), func);
}

void SegmentationManual::onSphereDetectClicked()
{
    auto func = [=](const std::vector<Base::Vector3f>& points,
                    const std::vector<Base::Vector3f>& normal) -> MeshCore::AbstractSurfaceFit* {
        Q_UNUSED(normal)

        MeshCore::SphereFit fit;
        fit.AddPoints(points);
        if (fit.Fit() < FLOAT_MAX) {
            Base::Vector3f base = fit.GetCenter();
            float radius = fit.GetRadius();
            return new MeshCore::SphereSurfaceFit(base, radius);
        }

        return nullptr;
    };
    Private::findGeometry(ui->numSph->value(), ui->tolSph->value(), func);
}

void SegmentationManual::createSegment()
{
    Gui::Document* gdoc = Gui::Application::Instance->activeDocument();
    if (!gdoc) {
        return;
    }
    // delete all selected faces
    App::Document* adoc = gdoc->getDocument();
    gdoc->openCommand(QT_TRANSLATE_NOOP("Command", "Segmentation"));

    std::vector<Mesh::Feature*> meshes = adoc->getObjectsOfType<Mesh::Feature>();
    bool selected = false;
    for (auto it : meshes) {
        const Mesh::MeshObject& mesh = it->Mesh.getValue();
        const MeshCore::MeshKernel& kernel = mesh.getKernel();

        MeshCore::MeshAlgorithm algo(kernel);
        unsigned long ct = algo.CountFacetFlag(MeshCore::MeshFacet::SELECTED);
        if (ct > 0) {
            selected = true;

            std::vector<MeshCore::FacetIndex> facets;
            algo.GetFacetsFlag(facets, MeshCore::MeshFacet::SELECTED);

            std::unique_ptr<Mesh::MeshObject> segment(mesh.meshFromSegment(facets));
            Mesh::Feature* feaSegm =
                static_cast<Mesh::Feature*>(adoc->addObject("Mesh::Feature", "Segment"));
            Mesh::MeshObject* feaMesh = feaSegm->Mesh.startEditing();
            feaMesh->swap(*segment);
            feaMesh->clearFacetSelection();
            feaSegm->Mesh.finishEditing();

            if (ui->checkBoxHideSegm->isChecked()) {
                feaSegm->Visibility.setValue(false);
            }

            if (ui->checkBoxCutSegm->isChecked()) {
                Mesh::MeshObject* editmesh = it->Mesh.startEditing();
                editmesh->deleteFacets(facets);
                it->Mesh.finishEditing();
            }
        }
    }

    if (!selected) {
        gdoc->abortCommand();
    }
    else {
        gdoc->commitCommand();
    }
    meshSel.clearSelection();
}

void SegmentationManual::onSelectTriangleClicked()
{
    meshSel.selectTriangle();
    meshSel.setAddComponentOnClick(ui->cbSelectComp->isChecked());
}

void SegmentationManual::reject()
{
    // deselect all meshes
    meshSel.clearSelection();
    meshSel.setEnabledViewerSelection(true);
}

// -------------------------------------------------

/* TRANSLATOR ReverseEngineeringGui::TaskSegmentationManual */

TaskSegmentationManual::TaskSegmentationManual()
{
    widget = new SegmentationManual();
    taskbox = new Gui::TaskView::TaskBox(QPixmap(), widget->windowTitle(), false, nullptr);
    taskbox->groupLayout()->addWidget(widget);
    Content.push_back(taskbox);
}

void TaskSegmentationManual::modifyStandardButtons(QDialogButtonBox* box)
{
    QPushButton* btn = box->button(QDialogButtonBox::Ok);
    btn->setText(tr("Create"));
}

bool TaskSegmentationManual::accept()
{
    return false;
}

void TaskSegmentationManual::clicked(int id)
{
    if (id == QDialogButtonBox::Ok) {
        widget->createSegment();
    }
    else if (id == QDialogButtonBox::Close) {
        widget->reject();
    }
}

#include "moc_SegmentationManual.cpp"
