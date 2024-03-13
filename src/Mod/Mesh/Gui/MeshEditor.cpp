/***************************************************************************
 *   Copyright (c) 2010 Werner Mayer <wmayer[at]users.sourceforge.net>     *
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
#include <algorithm>
#include <functional>
#include <QMenu>
#include <QTimer>

#include <Inventor/SbLine.h>
#include <Inventor/SoPickedPoint.h>
#include <Inventor/details/SoFaceDetail.h>
#include <Inventor/events/SoKeyboardEvent.h>
#include <Inventor/events/SoLocation2Event.h>
#include <Inventor/events/SoMouseButtonEvent.h>
#include <Inventor/nodes/SoBaseColor.h>
#include <Inventor/nodes/SoCamera.h>
#include <Inventor/nodes/SoCoordinate3.h>
#include <Inventor/nodes/SoDirectionalLight.h>
#include <Inventor/nodes/SoDrawStyle.h>
#include <Inventor/nodes/SoFaceSet.h>
#include <Inventor/nodes/SoPickStyle.h>
#include <Inventor/nodes/SoPointSet.h>
#include <Inventor/nodes/SoSeparator.h>
#include <Inventor/nodes/SoShapeHints.h>
#endif

#include <App/Application.h>
#include <App/Document.h>
#include <Gui/View3DInventor.h>
#include <Gui/View3DInventorViewer.h>
#include <Gui/WaitCursor.h>
#include <Mod/Mesh/App/MeshFeature.h>
#include <Mod/Mesh/App/Core/Algorithm.h>

#include "MeshEditor.h"
#include "SoFCMeshObject.h"
#include "SoPolygon.h"


using namespace MeshGui;
namespace sp = std::placeholders;

PROPERTY_SOURCE(MeshGui::ViewProviderFace, Gui::ViewProviderDocumentObject)

ViewProviderFace::ViewProviderFace()
{
    // NOLINTBEGIN
    pcCoords = new SoCoordinate3();
    pcCoords->ref();
    pcCoords->point.setNum(0);
    pcFaces = new SoFaceSet;
    pcFaces->ref();
    pcMeshPick = new SoFCMeshPickNode();
    pcMeshPick->ref();
    // NOLINTEND
}

ViewProviderFace::~ViewProviderFace()
{
    pcCoords->unref();
    pcFaces->unref();
    pcMeshPick->unref();
}

void ViewProviderFace::attach(App::DocumentObject* obj)
{
    ViewProviderDocumentObject::attach(obj);

    pcMeshPick->mesh.setValue(static_cast<Mesh::Feature*>(obj)->Mesh.getValuePtr());

    // Draw markers
    SoGroup* markers = new SoGroup();
    SoDrawStyle* pointStyle = new SoDrawStyle();
    pointStyle->style = SoDrawStyle::POINTS;
    pointStyle->pointSize = 8.0F;
    markers->addChild(pointStyle);

    SoBaseColor* markcol = new SoBaseColor;
    markcol->rgb.setValue(1.0F, 1.0F, 0.0F);
    SoPointSet* marker = new SoPointSet();
    markers->addChild(markcol);
    markers->addChild(pcCoords);
    markers->addChild(marker);

    // Draw face
    SoGroup* faces = new SoGroup();
    SoDrawStyle* faceStyle = new SoDrawStyle();
    faceStyle->style = SoDrawStyle::FILLED;
    faces->addChild(faceStyle);

    SoShapeHints* flathints = new SoShapeHints;
    // flathints->vertexOrdering = SoShapeHints::COUNTERCLOCKWISE ;
    // flathints->shapeType = SoShapeHints::UNKNOWN_SHAPE_TYPE;
    faces->addChild(flathints);

    SoBaseColor* basecol = new SoBaseColor;
    if (mesh) {
        App::Color col = mesh->ShapeColor.getValue();
        basecol->rgb.setValue(col.r, col.g, col.b);
    }
    else {
        basecol->rgb.setValue(1.0F, 0.0F, 0.0F);
    }

    faces->addChild(basecol);
    faces->addChild(pcCoords);
    faces->addChild(pcFaces);

    SoGroup* face_marker = new SoGroup();
    face_marker->addChild(faces);
    face_marker->addChild(markers);

    addDisplayMaskMode(markers, "Marker");
    addDisplayMaskMode(face_marker, "Face");
    setDisplayMode("Marker");
}

void ViewProviderFace::setDisplayMode(const char* ModeName)
{
    if (strcmp(ModeName, "Face") == 0) {
        setDisplayMaskMode("Face");
    }
    else if (strcmp(ModeName, "Marker") == 0) {
        setDisplayMaskMode("Marker");
    }
    ViewProviderDocumentObject::setDisplayMode(ModeName);
}

const char* ViewProviderFace::getDefaultDisplayMode() const
{
    return "Marker";
}

std::vector<std::string> ViewProviderFace::getDisplayModes() const
{
    std::vector<std::string> modes;
    modes.emplace_back("Marker");
    modes.emplace_back("Face");
    return modes;
}

SoPickedPoint* ViewProviderFace::getPickedPoint(const SbVec2s& pos,
                                                const Gui::View3DInventorViewer* viewer) const
{
    SoSeparator* root = new SoSeparator;
    root->ref();
    root->addChild(viewer->getHeadlight());
    root->addChild(viewer->getSoRenderManager()->getCamera());
    root->addChild(this->pcMeshPick);

    SoRayPickAction rp(viewer->getSoRenderManager()->getViewportRegion());
    rp.setPoint(pos);
    rp.apply(root);
    root->unref();

    // returns a copy of the point
    SoPickedPoint* pick = rp.getPickedPoint();
    // return (pick ? pick->copy() : 0); // needs the same instance of CRT under MS Windows
    return (pick ? new SoPickedPoint(*pick) : nullptr);
}

// ----------------------------------------------------------------------

/* TRANSLATOR MeshGui::MeshFaceAddition */

MeshFaceAddition::MeshFaceAddition(Gui::View3DInventor* parent)
    : QObject(parent)
    , faceView(new MeshGui::ViewProviderFace())
{}

MeshFaceAddition::~MeshFaceAddition()
{
    delete faceView;
}

void MeshFaceAddition::startEditing(MeshGui::ViewProviderMesh* vp)
{
    Gui::View3DInventor* view = static_cast<Gui::View3DInventor*>(parent());
    Gui::View3DInventorViewer* viewer = view->getViewer();
    viewer->setEditing(true);
    viewer->setSelectionEnabled(false);
    viewer->setRedirectToSceneGraph(true);
    viewer->setRedirectToSceneGraphEnabled(true);

    faceView->mesh = vp;
    faceView->attach(vp->getObject());
    viewer->addViewProvider(faceView);
    // faceView->mesh->startEditing();
    viewer->addEventCallback(SoEvent::getClassTypeId(), MeshFaceAddition::addFacetCallback, this);
}

void MeshFaceAddition::finishEditing()
{
    Gui::View3DInventor* view = static_cast<Gui::View3DInventor*>(parent());
    Gui::View3DInventorViewer* viewer = view->getViewer();
    viewer->setEditing(false);
    viewer->setSelectionEnabled(true);
    viewer->setRedirectToSceneGraph(false);
    viewer->setRedirectToSceneGraphEnabled(false);

    viewer->removeViewProvider(faceView);
    // faceView->mesh->finishEditing();
    viewer->removeEventCallback(SoEvent::getClassTypeId(),
                                MeshFaceAddition::addFacetCallback,
                                this);
    this->deleteLater();
}

void MeshFaceAddition::addFace()
{
    Mesh::Feature* mf = static_cast<Mesh::Feature*>(faceView->mesh->getObject());
    App::Document* doc = mf->getDocument();
    doc->openTransaction("Add triangle");
    Mesh::MeshObject* mesh = mf->Mesh.startEditing();
    MeshCore::MeshFacet f;
    f._aulPoints[0] = faceView->index[0];
    f._aulPoints[1] = faceView->index[1];
    f._aulPoints[2] = faceView->index[2];
    std::vector<MeshCore::MeshFacet> faces;
    faces.push_back(f);
    mesh->addFacets(faces, true);
    mf->Mesh.finishEditing();
    doc->commitTransaction();

    clearPoints();
}

void MeshFaceAddition::clearPoints()
{
    faceView->index.clear();
    faceView->current_index = -1;
    faceView->pcCoords->point.setNum(0);
    faceView->setDisplayMode("Marker");
}

void MeshFaceAddition::flipNormal()
{
    if (faceView->index.size() < 3) {
        return;
    }
    std::swap(faceView->index[0], faceView->index[1]);
    SbVec3f v1 = faceView->pcCoords->point[0];
    SbVec3f v2 = faceView->pcCoords->point[1];
    faceView->pcCoords->point.set1Value(0, v2);
    faceView->pcCoords->point.set1Value(1, v1);
}

bool MeshFaceAddition::addMarkerPoint()
{
    if (faceView->current_index < 0) {
        return false;
    }
    if (faceView->index.size() >= 3) {
        return false;
    }
    faceView->index.push_back(faceView->current_index);
    faceView->current_index = -1;
    if (faceView->index.size() == 3) {
        faceView->setDisplayMode("Face");
    }
    return true;
}

void MeshFaceAddition::showMarker(SoPickedPoint* pp)
{
    const SbVec3f& vec = pp->getPoint();
    const SoDetail* detail = pp->getDetail();
    if (detail) {
        if (detail->isOfType(SoFaceDetail::getClassTypeId())) {
            const SoFaceDetail* fd = static_cast<const SoFaceDetail*>(detail);
            Mesh::Feature* mf = static_cast<Mesh::Feature*>(faceView->mesh->getObject());
            const MeshCore::MeshFacetArray& facets =
                mf->Mesh.getValuePtr()->getKernel().GetFacets();
            const MeshCore::MeshPointArray& points =
                mf->Mesh.getValuePtr()->getKernel().GetPoints();
            // is the face index valid?
            int face_index = fd->getFaceIndex();
            if (face_index >= (int)facets.size()) {
                return;
            }
            // is a border facet picked?
            MeshCore::MeshFacet f = facets[face_index];
            if (!f.HasOpenEdge()) {
                // check if a neighbour facet is at the border
                bool ok = false;
                for (Mesh::FacetIndex nbIndex : f._aulNeighbours) {
                    if (facets[nbIndex].HasOpenEdge()) {
                        f = facets[nbIndex];
                        ok = true;
                        break;
                    }
                }
                if (!ok) {
                    return;
                }
            }

            int point_index = -1;
            float distance = FLT_MAX;
            Base::Vector3f pnt;
            SbVec3f face_pnt;

            for (int i = 0; i < 3; i++) {
                int index = (int)f._aulPoints[i];
                if (std::find(faceView->index.begin(), faceView->index.end(), index)
                    != faceView->index.end()) {
                    continue;  // already inside
                }
                if (f._aulNeighbours[i] == MeshCore::FACET_INDEX_MAX
                    || f._aulNeighbours[(i + 2) % 3] == MeshCore::FACET_INDEX_MAX) {
                    pnt = points[index];
                    float len = Base::DistanceP2(pnt, Base::Vector3f(vec[0], vec[1], vec[2]));
                    if (len < distance) {
                        distance = len;
                        point_index = index;
                        face_pnt.setValue(pnt.x, pnt.y, pnt.z);
                    }
                }
            }

            if (point_index < 0) {
                return;  // picked point is rejected
            }

            int num = faceView->pcCoords->point.getNum();
            if (faceView->current_index >= 0) {
                num = std::max<int>(num - 1, 0);
            }
            faceView->current_index = point_index;
            faceView->pcCoords->point.set1Value(num, face_pnt);
            return;
        }
    }
}

void MeshFaceAddition::addFacetCallback(void* ud, SoEventCallback* n)
{
    MeshFaceAddition* that = static_cast<MeshFaceAddition*>(ud);
    ViewProviderFace* face = that->faceView;
    Gui::View3DInventorViewer* view = static_cast<Gui::View3DInventorViewer*>(n->getUserData());

    const SoEvent* ev = n->getEvent();
    // If we are in navigation mode then ignore all but key events
    if (!view->isRedirectedToSceneGraph()) {
        if (!ev->getTypeId().isDerivedFrom(SoKeyboardEvent::getClassTypeId())) {
            return;
        }
    }
    if (ev->getTypeId() == SoLocation2Event::getClassTypeId()) {
        n->setHandled();
        if (face->index.size() < 3) {
            SoPickedPoint* point = face->getPickedPoint(ev->getPosition(), view);
            if (point) {
                that->showMarker(point);
                delete point;
            }
        }
    }
    else if (ev->getTypeId() == SoMouseButtonEvent::getClassTypeId()) {
        const SoMouseButtonEvent* mbe = static_cast<const SoMouseButtonEvent*>(ev);
        if (mbe->getButton() == SoMouseButtonEvent::BUTTON1
            || mbe->getButton() == SoMouseButtonEvent::BUTTON2
            || mbe->getButton() == SoMouseButtonEvent::BUTTON3) {
            n->setHandled();
        }
        if (mbe->getButton() == SoMouseButtonEvent::BUTTON1
            && mbe->getState() == SoButtonEvent::DOWN) {
            that->addMarkerPoint();
        }
        else if (mbe->getButton() == SoMouseButtonEvent::BUTTON1
                 && mbe->getState() == SoButtonEvent::UP) {
            if (face->index.size() == 3) {
                QMenu menu;
                QAction* add = menu.addAction(MeshFaceAddition::tr("Add triangle"));
                QAction* swp = menu.addAction(MeshFaceAddition::tr("Flip normal"));
                QAction* clr = menu.addAction(MeshFaceAddition::tr("Clear"));
                QAction* act = menu.exec(QCursor::pos());
                if (act == add) {
                    QTimer::singleShot(300, that, &MeshFaceAddition::addFace);
                }
                else if (act == swp) {
                    QTimer::singleShot(300, that, &MeshFaceAddition::flipNormal);
                }
                else if (act == clr) {
                    QTimer::singleShot(300, that, &MeshFaceAddition::clearPoints);
                }
            }
        }
        else if (mbe->getButton() == SoMouseButtonEvent::BUTTON2
                 && mbe->getState() == SoButtonEvent::UP) {
            QMenu menu;
            QAction* fin = menu.addAction(MeshFaceAddition::tr("Finish"));
            QAction* act = menu.exec(QCursor::pos());
            if (act == fin) {
                QTimer::singleShot(300, that, &MeshFaceAddition::finishEditing);
            }
        }
    }
    // toggle between edit and navigation mode
    else if (ev->getTypeId().isDerivedFrom(SoKeyboardEvent::getClassTypeId())) {
        const SoKeyboardEvent* const ke = static_cast<const SoKeyboardEvent*>(ev);
        if (ke->getState() == SoButtonEvent::DOWN && ke->getKey() == SoKeyboardEvent::ESCAPE) {
            SbBool toggle = view->isRedirectedToSceneGraph();
            view->setRedirectToSceneGraph(!toggle);
            n->setHandled();
        }
    }
}

// ----------------------------------------------------------------------

namespace MeshGui
{
// for sorting of elements
struct NofFacetsCompare
{
    bool operator()(const std::vector<Mesh::PointIndex>& rclC1,
                    const std::vector<Mesh::PointIndex>& rclC2)
    {
        return rclC1.size() < rclC2.size();
    }
};
}  // namespace MeshGui

/* TRANSLATOR MeshGui::MeshFillHole */

MeshFillHole::MeshFillHole(MeshHoleFiller& hf, Gui::View3DInventor* parent)
    : QObject(parent)
    , myHoleFiller(hf)
{
    // NOLINTBEGIN
    myBoundariesRoot = new SoSeparator;
    myBoundariesRoot->ref();
    myBoundaryRoot = new SoSeparator;
    myBoundaryRoot->ref();
    myBoundariesGroup = new SoSeparator();
    myBoundariesGroup->ref();
    myBridgeRoot = new SoSeparator;
    myBridgeRoot->ref();

    SoDrawStyle* pointStyle = new SoDrawStyle();
    pointStyle->style = SoDrawStyle::POINTS;
    pointStyle->pointSize = 8.0f;
    myBridgeRoot->addChild(pointStyle);

    SoBaseColor* markcol = new SoBaseColor;
    markcol->rgb.setValue(1.0f, 1.0f, 0.0f);
    myBridgeRoot->addChild(markcol);

    myVertex = new SoCoordinate3();
    myBridgeRoot->addChild(myVertex);
    myBridgeRoot->addChild(new SoPointSet);
    // NOLINTEND
}

MeshFillHole::~MeshFillHole()
{
    myBoundariesRoot->unref();
    myBoundariesGroup->unref();
    myBoundaryRoot->unref();
    myBridgeRoot->unref();
}

void MeshFillHole::startEditing(MeshGui::ViewProviderMesh* vp)
{
    this->myMesh = static_cast<Mesh::Feature*>(vp->getObject());

    Gui::View3DInventor* view = static_cast<Gui::View3DInventor*>(parent());
    Gui::View3DInventorViewer* viewer = view->getViewer();
    viewer->setEditing(true);
    // viewer->setRedirectToSceneGraph(true);
    viewer->addEventCallback(SoEvent::getClassTypeId(), MeshFillHole::fileHoleCallback, this);
    // NOLINTBEGIN
    myConnection = App::GetApplication().signalChangedObject.connect(
        std::bind(&MeshFillHole::slotChangedObject, this, sp::_1, sp::_2));
    // NOLINTEND

    Gui::coinRemoveAllChildren(myBoundariesRoot);
    myBoundariesRoot->addChild(viewer->getHeadlight());
    myBoundariesRoot->addChild(viewer->getSoRenderManager()->getCamera());
    myBoundariesRoot->addChild(myBoundariesGroup);
    Gui::coinRemoveAllChildren(myBoundaryRoot);
    myBoundaryRoot->addChild(viewer->getHeadlight());
    myBoundaryRoot->addChild(viewer->getSoRenderManager()->getCamera());
    createPolygons();
    static_cast<SoGroup*>(viewer->getSceneGraph())->addChild(myBridgeRoot);
}

void MeshFillHole::finishEditing()
{
    Gui::View3DInventor* view = static_cast<Gui::View3DInventor*>(parent());
    Gui::View3DInventorViewer* viewer = view->getViewer();
    viewer->setEditing(false);
    // viewer->setRedirectToSceneGraph(false);
    viewer->removeEventCallback(SoEvent::getClassTypeId(), MeshFillHole::fileHoleCallback, this);
    myConnection.disconnect();
    this->deleteLater();
    static_cast<SoGroup*>(viewer->getSceneGraph())->removeChild(myBridgeRoot);
}

void MeshFillHole::closeBridge()
{
    // Do the hole-filling
    Gui::WaitCursor wc;
    auto it = std::find(myPolygon.begin(), myPolygon.end(), myVertex1);
    auto jt = std::find(myPolygon.begin(), myPolygon.end(), myVertex2);
    if (it != myPolygon.end() && jt != myPolygon.end()) {
        // which iterator comes first
        if (jt < it) {
            std::swap(it, jt);
        }
        // split the boundary into two loops and take the shorter one
        std::list<TBoundary> bounds;
        TBoundary loop1;
        TBoundary loop2;
        loop1.insert(loop1.end(), myPolygon.begin(), it);
        loop1.insert(loop1.end(), jt, myPolygon.end());
        loop2.insert(loop2.end(), it, jt);
        // this happens when myVertex1 == myVertex2
        if (loop2.empty()) {
            bounds.push_back(loop1);
        }
        else if (loop1.size() < loop2.size()) {
            bounds.push_back(loop1);
        }
        else {
            bounds.push_back(loop2);
        }

        App::Document* doc = myMesh->getDocument();
        doc->openTransaction("Bridge && Fill hole");
        Mesh::MeshObject* pMesh = myMesh->Mesh.startEditing();
        bool ok = myHoleFiller.fillHoles(*pMesh, bounds, myVertex1, myVertex2);
        myMesh->Mesh.finishEditing();
        if (ok) {
            doc->commitTransaction();
        }
        else {
            doc->abortTransaction();
        }
    }
}

void MeshFillHole::slotChangedObject(const App::DocumentObject& Obj, const App::Property& Prop)
{
    if (&Obj == myMesh && strcmp(Prop.getName(), "Mesh") == 0) {
        Gui::coinRemoveAllChildren(myBoundariesGroup);
        myVertex->point.setNum(0);
        myNumPoints = 0;
        myPolygon.clear();

        createPolygons();
    }
}

void MeshFillHole::createPolygons()
{
    Gui::WaitCursor wc;
    myPolygons.clear();

    SoPickStyle* pickStyle = new SoPickStyle();
    pickStyle->style = SoPickStyle::BOUNDING_BOX;
    myBoundariesGroup->addChild(pickStyle);
    myBoundaryRoot->addChild(pickStyle);

    // get mesh kernel
    const MeshCore::MeshKernel& rMesh = this->myMesh->Mesh.getValue().getKernel();

    // get the mesh boundaries as an array of point indices
    std::list<std::vector<Mesh::PointIndex>> borders;
    MeshCore::MeshAlgorithm cAlgo(rMesh);
    MeshCore::MeshPointIterator p_iter(rMesh);
    cAlgo.GetMeshBorders(borders);
    cAlgo.SplitBoundaryLoops(borders);

    // sort the borders in ascending order of the number of edges
    borders.sort(NofFacetsCompare());

    int32_t count = 0;
    for (auto& border : borders) {
        if (border.front() == border.back()) {
            border.pop_back();
        }
        count += border.size();
    }

    SoCoordinate3* coords = new SoCoordinate3();
    myBoundariesGroup->addChild(coords);
    myBoundaryRoot->addChild(coords);

    coords->point.setNum(count);
    int32_t index = 0;
    for (const auto& border : borders) {
        SoPolygon* polygon = new SoPolygon();
        polygon->startIndex = index;
        polygon->numVertices = border.size();
        myBoundariesGroup->addChild(polygon);
        myPolygons[polygon] = border;
        for (Mesh::PointIndex jt : border) {
            p_iter.Set(jt);
            coords->point.set1Value(index++, p_iter->x, p_iter->y, p_iter->z);
        }
    }
}

SoNode* MeshFillHole::getPickedPolygon(
    const SoRayPickAction& action /*SoNode* root, const SbVec2s& pos*/) const
{
    SoPolygon* poly = nullptr;
    const SoPickedPointList& points = action.getPickedPointList();
    for (int i = 0; i < points.getLength(); i++) {
        const SoPickedPoint* point = points[i];
        if (point
            && point->getPath()->getTail()->getTypeId() == MeshGui::SoPolygon::getClassTypeId()) {
            // we have something picked, now check if it was an SoPolygon node
            SoPolygon* node = static_cast<SoPolygon*>(point->getPath()->getTail());
            if (!poly) {
                poly = node;
            }
            // check which polygon has less edges
            else if (node->numVertices.getValue() < poly->numVertices.getValue()) {
                poly = node;
            }
        }
    }

    return poly;
}

float MeshFillHole::findClosestPoint(const SbLine& ray,
                                     const TBoundary& polygon,
                                     Mesh::PointIndex& vertex_index,
                                     SbVec3f& closestPoint) const
{
    // now check which vertex of the polygon is closest to the ray
    float minDist = FLT_MAX;
    vertex_index = MeshCore::POINT_INDEX_MAX;

    const MeshCore::MeshKernel& rMesh = myMesh->Mesh.getValue().getKernel();
    const MeshCore::MeshPointArray& pts = rMesh.GetPoints();
    for (Mesh::PointIndex it : polygon) {
        SbVec3f vertex;
        const Base::Vector3f& v = pts[it];
        vertex.setValue(v.x, v.y, v.z);
        SbVec3f point = ray.getClosestPoint(vertex);
        float distance = (vertex - point).sqrLength();
        if (distance < minDist) {
            minDist = distance;
            vertex_index = it;
            closestPoint = vertex;
        }
    }

    return minDist;
}

void MeshFillHole::fileHoleCallback(void* ud, SoEventCallback* n)
{
    MeshFillHole* self = static_cast<MeshFillHole*>(ud);
    Gui::View3DInventorViewer* view = static_cast<Gui::View3DInventorViewer*>(n->getUserData());

    const SoEvent* ev = n->getEvent();
    if (ev->getTypeId() == SoLocation2Event::getClassTypeId()) {
        n->setHandled();
        SoRayPickAction rp(view->getSoRenderManager()->getViewportRegion());
        rp.setPoint(ev->getPosition());
        rp.setPickAll(true);
        if (self->myNumPoints == 0) {
            rp.apply(self->myBoundariesRoot);
        }
        else {
            rp.apply(self->myBoundaryRoot);
        }
        SoNode* node = self->getPickedPolygon(rp);
        if (node) {
            auto it = self->myPolygons.find(node);
            if (it != self->myPolygons.end()) {
                // now check which vertex of the polygon is closest to the ray
                Mesh::PointIndex vertex_index {};
                SbVec3f closestPoint;
                float minDist =
                    self->findClosestPoint(rp.getLine(), it->second, vertex_index, closestPoint);
                if (minDist < 1.0F) {
                    if (self->myNumPoints == 0) {
                        self->myVertex->point.set1Value(0, closestPoint);
                    }
                    else {
                        self->myVertex->point.set1Value(1, closestPoint);
                    }
                }
            }
        }
    }
    else if (ev->getTypeId() == SoMouseButtonEvent::getClassTypeId()) {
        n->setHandled();
        const SoMouseButtonEvent* mbe = static_cast<const SoMouseButtonEvent*>(ev);
        if (mbe->getButton() == SoMouseButtonEvent::BUTTON1
            && mbe->getState() == SoButtonEvent::UP) {
            if (self->myNumPoints > 1) {
                return;
            }
            SoRayPickAction rp(view->getSoRenderManager()->getViewportRegion());
            rp.setPoint(ev->getPosition());
            rp.setPickAll(true);
            if (self->myNumPoints == 0) {
                rp.apply(self->myBoundariesRoot);
            }
            else {
                rp.apply(self->myBoundaryRoot);
            }
            SoNode* node = self->getPickedPolygon(rp);
            if (node) {
                auto it = self->myPolygons.find(node);
                if (it != self->myPolygons.end()) {
                    // now check which vertex of the polygon is closest to the ray
                    Mesh::PointIndex vertex_index {};
                    SbVec3f closestPoint;
                    float minDist = self->findClosestPoint(rp.getLine(),
                                                           it->second,
                                                           vertex_index,
                                                           closestPoint);
                    if (minDist < 1.0F) {
                        if (self->myNumPoints == 0) {
                            self->myBoundaryRoot->addChild(node);
                            self->myVertex->point.set1Value(0, closestPoint);
                            self->myNumPoints = 1;
                            self->myVertex1 = vertex_index;
                        }
                        else {
                            // myVertex2 can be equal to myVertex1 which does a full hole-filling
                            self->myBoundaryRoot->removeChild(node);
                            self->myVertex->point.set1Value(1, closestPoint);
                            self->myNumPoints = 2;
                            self->myVertex2 = vertex_index;
                            self->myPolygon = it->second;
                            QTimer::singleShot(300, self, &MeshFillHole::closeBridge);
                        }
                    }
                }
            }
        }
        else if (mbe->getButton() == SoMouseButtonEvent::BUTTON2
                 && mbe->getState() == SoButtonEvent::UP) {
            QMenu menu;
            QAction* fin = menu.addAction(MeshFillHole::tr("Finish"));
            QAction* act = menu.exec(QCursor::pos());
            if (act == fin) {
                QTimer::singleShot(300, self, &MeshFillHole::finishEditing);
            }
        }
    }
}

#include "moc_MeshEditor.cpp"
