// SPDX-License-Identifier: LGPL-2.1-or-later

/***************************************************************************
 *   Copyright (c) 2026 Werner Mayer <wmayer[at]users.sourceforge.net>     *
 *                                                                         *
 *   This file is part of FreeCAD.                                         *
 *                                                                         *
 *   FreeCAD is free software: you can redistribute it and/or modify it    *
 *   under the terms of the GNU Lesser General Public License as           *
 *   published by the Free Software Foundation, either version 2.1 of the  *
 *   License, or (at your option) any later version.                       *
 *                                                                         *
 *   FreeCAD is distributed in the hope that it will be useful, but        *
 *   WITHOUT ANY WARRANTY; without even the implied warranty of            *
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU      *
 *   Lesser General Public License for more details.                       *
 *                                                                         *
 *   You should have received a copy of the GNU Lesser General Public      *
 *   License along with FreeCAD. If not, see                               *
 *   <https://www.gnu.org/licenses/>.                                      *
 *                                                                         *
 **************************************************************************/

#include "PreCompiled.h"
#ifndef _PreComp_
# include <Inventor/SoPickedPoint.h>
# include <Inventor/details/SoFaceDetail.h>
# include <Inventor/events/SoMouseButtonEvent.h>
# include <QAction>
# include <QMenu>
# include <QMessageBox>
# include <QPointer>
# include <QStatusBar>
# include <QTimer>
#endif

#include "PatchOnMesh.h"
#include "CurveOnMesh.h"
#include <Base/Converter.h>
#include <App/Document.h>
#include <Gui/Application.h>
#include <Gui/CommandT.h>
#include <Gui/Document.h>
#include <Gui/MainWindow.h>
#include <Gui/Utilities.h>
#include <Gui/View3DInventor.h>
#include <Gui/View3DInventorViewer.h>
#include <Mod/Mesh/App/Core/Algorithm.h>
#include <Mod/Mesh/App/Core/Grid.h>
#include <Mod/Mesh/App/Core/MeshKernel.h>
#include <Mod/Mesh/App/Core/Projection.h>
#include <Mod/Mesh/App/MeshFeature.h>
#include <Mod/Mesh/Gui/ViewProvider.h>

using namespace MeshPartGui;

class PatchOnMeshHandler::Private
{
public:
    FC_DISABLE_COPY_MOVE(Private)
    struct PickedPoint
    {
        MeshCore::FacetIndex facet {};
        SbVec3f point;
        SbVec3f normal;
    };

    struct ApproxPar
    {
        double tol3d = 1.0e-2;
        int maxDegree = 5;
        GeomAbs_Shape cont = GeomAbs_C2;
    };

    QPointer<Gui::View3DInventor> viewer;
    std::unique_ptr<ViewProviderCurveOnMesh> curve;
    Gui::ViewProviderDocumentObject* mesh {nullptr};
    std::vector<PickedPoint> pickedPoints;
    std::list<std::vector<Base::Vector3f>> cutLines;
    std::unique_ptr<MeshCore::MeshFacetGrid> grid;
    MeshCore::MeshKernel kernel;
    ApproxPar par;

    static void vertexCallback(void* ud, SoEventCallback* cb);

    Private()
        : curve(new ViewProviderCurveOnMesh)
    {}
    ~Private() = default;
    static std::vector<SbVec3f> convert(const std::vector<Base::Vector3f>& points)
    {
        std::vector<SbVec3f> pts;
        pts.reserve(points.size());
        for (const auto& it : points) {
            pts.push_back(Base::convertTo<SbVec3f>(it));
        }
        return pts;
    }
    void createGrid()
    {
        auto mf = mesh->getObject<Mesh::Feature>();
        const Mesh::MeshObject& meshObject = mf->Mesh.getValue();
        kernel = meshObject.getKernel();
        kernel.Transform(meshObject.getTransform());

        MeshCore::MeshAlgorithm alg(kernel);
        float fAvgLen = alg.GetAverageEdgeLength();
        const float factor = 5.0F;
        grid = std::make_unique<MeshCore::MeshFacetGrid>(kernel, factor * fAvgLen);
    }
    bool projectLineOnMesh(const PickedPoint& pick)
    {
        PickedPoint last = pickedPoints.back();
        std::vector<Base::Vector3f> polyline;

        MeshCore::MeshProjection meshProjection(kernel);
        auto v1 = Base::convertTo<Base::Vector3f>(last.point);
        auto v2 = Base::convertTo<Base::Vector3f>(pick.point);
        auto vd = Base::convertTo<Base::Vector3f>(viewer->getViewer()->getViewDirection());
        if (meshProjection.projectLineOnMesh(*grid, v1, last.facet, v2, pick.facet, vd, polyline)) {
            if (polyline.size() > 1) {
                cutLines.push_back(polyline);
                return true;
            }
        }

        return false;
    }
    App::DocumentObject* getSource() const
    {
        return mesh->getObject();
    }
    App::Document* getDocument() const
    {
        if (!mesh) {
            return nullptr;
        }

        if (auto doc = mesh->getDocument()) {
            return doc->getDocument();
        }

        return nullptr;
    }
};

PatchOnMeshHandler::PatchOnMeshHandler(QObject* parent)
    : QObject {parent}
    , d_ptr {new Private}
{}

PatchOnMeshHandler::~PatchOnMeshHandler()
{
    d_ptr.reset();
}

void PatchOnMeshHandler::setParameters(int maxDegree, GeomAbs_Shape cont, double tol3d)
{
    d_ptr->par.maxDegree = maxDegree;
    d_ptr->par.cont = cont;
    d_ptr->par.tol3d = tol3d;
}

void PatchOnMeshHandler::enableCallback(Gui::View3DInventor* v)
{
    if (v && !d_ptr->viewer) {
        d_ptr->viewer = v;
        Gui::View3DInventorViewer* view3d = d_ptr->viewer->getViewer();
        view3d->addEventCallback(SoEvent::getClassTypeId(), Private::vertexCallback, this);
        view3d->addViewProvider(d_ptr->curve.get());
        view3d->setEditing(true);

        d_ptr->curve->setDisplayMode("Point");
    }
}

void PatchOnMeshHandler::disableCallback()
{
    if (d_ptr->viewer) {
        Gui::View3DInventorViewer* view3d = d_ptr->viewer->getViewer();
        view3d->setEditing(false);
        view3d->removeViewProvider(d_ptr->curve.get());
        view3d->removeEventCallback(SoEvent::getClassTypeId(), Private::vertexCallback, this);
    }
    d_ptr->viewer = nullptr;
}

std::vector<SbVec3f> PatchOnMeshHandler::getPoints() const
{
    std::vector<SbVec3f> pts;
    for (const auto& it : d_ptr->cutLines) {
        std::vector<SbVec3f> segm = Private::convert(it);
        pts.insert(pts.end(), segm.begin(), segm.end());
    }
    return pts;
}

void PatchOnMeshHandler::onContextMenu()
{
    QMenu menu;
    menu.addAction(tr("Create"), this, &PatchOnMeshHandler::onCreate);
    menu.addAction(tr("Clear"), this, &PatchOnMeshHandler::onClear);
    menu.addAction(tr("Cancel"), this, &PatchOnMeshHandler::onCancel);
    menu.exec(QCursor::pos());
}

void PatchOnMeshHandler::onCreate()
{
    try {
        performAction();
    }
    catch (const Base::Exception& e) {
        QMessageBox::critical(
            Gui::getMainWindow(),
            tr("Patch creation failed"),
            QString::fromLatin1(e.what())
        );
    }
}

void PatchOnMeshHandler::performAction()
{
    if (d_ptr->pickedPoints.size() != 4) {
        QMessageBox::critical(
            Gui::getMainWindow(),
            tr("Patch creation failed"),
            tr("To create a patch exactly four points are needed")
        );
        return;
    }

    App::Document* doc = d_ptr->getDocument();
    if (!doc) {
        return;
    }

    auto vd = d_ptr->viewer->getViewer()->getViewDirection();

    Gui::Application::Instance->activeDocument()->openCommand(
        QT_TRANSLATE_NOOP("Command", "Add patch")
    );

    App::DocumentObject* source = d_ptr->getSource();
    auto pp1 = d_ptr->pickedPoints[0].point;
    auto pp2 = d_ptr->pickedPoints[1].point;
    auto pp3 = d_ptr->pickedPoints[2].point;
    auto pp4 = d_ptr->pickedPoints[3].point;

    std::string name = doc->getUniqueObjectName("Patch");
    Gui::cmdAppDocumentArgs(doc, "addObject(%s, %s)", std::quoted("MeshPart::Patch"), std::quoted(name));

    auto patch = doc->getObject(name.c_str());
    Gui::cmdAppObjectArgs(patch, "Mesh = %s", Gui::Command::getObjectCmd(source));
    Gui::cmdAppObjectArgs(patch, "P1 = (%f, %f, %f)", pp1[0], pp1[1], pp1[2]);
    Gui::cmdAppObjectArgs(patch, "P2 = (%f, %f, %f)", pp2[0], pp2[1], pp2[2]);
    Gui::cmdAppObjectArgs(patch, "P3 = (%f, %f, %f)", pp3[0], pp3[1], pp3[2]);
    Gui::cmdAppObjectArgs(patch, "P4 = (%f, %f, %f)", pp4[0], pp4[1], pp4[2]);
    Gui::cmdAppObjectArgs(patch, "ViewDirection = (%f, %f, %f)", vd[0], vd[1], vd[2]);
    Gui::cmdAppObjectArgs(patch, "Tolerance = %f", d_ptr->par.tol3d);
    Gui::cmdAppObjectArgs(patch, "MaxDegree = %d", d_ptr->par.maxDegree);
    doc->recompute();
    Gui::Application::Instance->activeDocument()->commitCommand();

    reset();
    disableCallback();
}

void PatchOnMeshHandler::reset()
{
    d_ptr->curve->clearVertex();
    d_ptr->curve->clearPoints();

    d_ptr->pickedPoints.clear();
    d_ptr->cutLines.clear();
}

void PatchOnMeshHandler::onClear()
{
    reset();
}

void PatchOnMeshHandler::onCancel()
{
    reset();
    disableCallback();
}

void PatchOnMeshHandler::handlePickedPoint(Gui::View3DInventorViewer* view, const SoPickedPoint* pp)
{
    if (!pp) {
        Gui::getMainWindow()->statusBar()->showMessage(tr("No point was picked"));
        return;
    }

    Gui::ViewProvider* vp = view->getViewProviderByPathFromTail(pp->getPath());
    if (auto mesh = dynamic_cast<MeshGui::ViewProviderMesh*>(vp)) {
        const SoDetail* detail = pp->getDetail();
        if (detail && detail->getTypeId() == SoFaceDetail::getClassTypeId()) {
            // get the mesh and build a grid
            if (!d_ptr->mesh) {
                d_ptr->mesh = mesh;
                d_ptr->createGrid();
            }
            else if (d_ptr->mesh != mesh) {
                Gui::getMainWindow()->statusBar()->showMessage(tr("Wrong mesh picked"));
                return;
            }

            const SbVec3f& p = pp->getPoint();
            const SbVec3f& n = pp->getNormal();

            Private::PickedPoint pick;
            pick.facet = static_cast<const SoFaceDetail*>(detail)->getFaceIndex();  // NOLINT
            pick.point = p;
            pick.normal = n;

            if (d_ptr->pickedPoints.empty()) {
                d_ptr->pickedPoints.push_back(pick);
                d_ptr->curve->addVertex(p);
            }
            else if (d_ptr->projectLineOnMesh(pick)) {
                d_ptr->curve->setPoints(getPoints());
                d_ptr->pickedPoints.push_back(pick);
                d_ptr->curve->addVertex(p);
            }
        }
    }
}

void PatchOnMeshHandler::Private::vertexCallback(void* ud, SoEventCallback* cb)
{
    auto view = static_cast<Gui::View3DInventorViewer*>(cb->getUserData());
    const SoEvent* ev = cb->getEvent();
    if (ev->getTypeId() == SoMouseButtonEvent::getClassTypeId()) {
        // set as handled
        cb->setHandled();

        const auto mbe = static_cast<const SoMouseButtonEvent*>(ev);  // NOLINT
        if (mbe->getButton() == SoMouseButtonEvent::BUTTON1
            && mbe->getState() == SoButtonEvent::DOWN) {
            const SoPickedPoint* pp = cb->getPickedPoint();
            auto self = static_cast<PatchOnMeshHandler*>(ud);
            self->handlePickedPoint(view, pp);
        }
        else if (
            mbe->getButton() == SoMouseButtonEvent::BUTTON2 && mbe->getState() == SoButtonEvent::UP
        ) {
            auto self = static_cast<PatchOnMeshHandler*>(ud);
            const int timeout = 100;
            QTimer::singleShot(timeout, self, &PatchOnMeshHandler::onContextMenu);
        }
    }
}

void PatchOnMeshHandler::recomputeDocument()
{
    if (d_ptr->viewer) {
        Gui::View3DInventorViewer* view3d = d_ptr->viewer->getViewer();
        App::Document* doc = view3d->getDocument()->getDocument();
        doc->recompute();
    }
}

#include "moc_PatchOnMesh.cpp"
