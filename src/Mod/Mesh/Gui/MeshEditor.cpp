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
# include <algorithm>
# include <QMenu>
# include <QTimer>
# include <Inventor/SoPickedPoint.h>
# include <Inventor/details/SoFaceDetail.h>
# include <Inventor/details/SoPointDetail.h>
# include <Inventor/events/SoLocation2Event.h>
# include <Inventor/events/SoMouseButtonEvent.h>
# include <Inventor/nodes/SoBaseColor.h>
# include <Inventor/nodes/SoCamera.h>
# include <Inventor/nodes/SoCoordinate3.h>
# include <Inventor/nodes/SoDirectionalLight.h>
# include <Inventor/nodes/SoDrawStyle.h>
# include <Inventor/nodes/SoFaceSet.h>
# include <Inventor/nodes/SoLineSet.h>
# include <Inventor/nodes/SoMarkerSet.h>
# include <Inventor/nodes/SoSeparator.h>
# include <Inventor/nodes/SoShapeHints.h>
#endif

#include "MeshEditor.h"
#include "SoFCMeshObject.h"
#include <App/Document.h>
#include <Mod/Mesh/App/MeshFeature.h>
#include <Gui/Application.h>
#include <Gui/View3DInventor.h>
#include <Gui/View3DInventorViewer.h>

using namespace MeshGui;

PROPERTY_SOURCE(MeshGui::ViewProviderFace, Gui::ViewProviderDocumentObject)

ViewProviderFace::ViewProviderFace() : mesh(0), current_index(-1)
{
    pcCoords = new SoCoordinate3();
    pcCoords->ref();
    pcCoords->point.setNum(0);
    pcFaces = new SoFaceSet;
    pcFaces->ref();
    pcMeshPick = new SoFCMeshPickNode();
    pcMeshPick->ref();
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
    pointStyle->pointSize = 8.0f;
    markers->addChild(pointStyle);

    SoBaseColor * markcol = new SoBaseColor;
    markcol->rgb.setValue(1.0f, 1.0f, 0.0f);
    SoPointSet* marker = new SoPointSet();
    markers->addChild(markcol);
    markers->addChild(pcCoords);
    markers->addChild(marker);

    // Draw face
    SoGroup* faces = new SoGroup();
    SoDrawStyle* faceStyle = new SoDrawStyle();
    faceStyle->style = SoDrawStyle::FILLED;
    faces->addChild(faceStyle);

    SoShapeHints * flathints = new SoShapeHints;
    //flathints->vertexOrdering = SoShapeHints::COUNTERCLOCKWISE ;
    //flathints->shapeType = SoShapeHints::UNKNOWN_SHAPE_TYPE;
    faces->addChild(flathints);

    SoBaseColor* basecol = new SoBaseColor;
    if (mesh) {
        App::Color col = mesh->ShapeColor.getValue();
        basecol->rgb.setValue(col.r, col.g, col.b);
    }
    else {
        basecol->rgb.setValue(1.0f, 0.0f, 0.0f);
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
    if (strcmp(ModeName, "Face") == 0)
        setDisplayMaskMode("Face");
    else if (strcmp(ModeName, "Marker") == 0)
        setDisplayMaskMode("Marker");
    ViewProviderDocumentObject::setDisplayMode(ModeName);
}

const char* ViewProviderFace::getDefaultDisplayMode() const
{
    return "Marker";
}

std::vector<std::string> ViewProviderFace::getDisplayModes(void) const
{
    std::vector<std::string> modes;
    modes.push_back("Marker");
    modes.push_back("Face");
    return modes;
}

SoPickedPoint* ViewProviderFace::getPickedPoint(const SbVec2s& pos, const SoQtViewer* viewer) const
{
    SoSeparator* root = new SoSeparator;
    root->ref();
    root->addChild(viewer->getHeadlight());
    root->addChild(viewer->getCamera());
    root->addChild(this->pcMeshPick);

    SoRayPickAction rp(viewer->getViewportRegion());
    rp.setPoint(pos);
    rp.apply(root);
    root->unref();

    // returns a copy of the point
    SoPickedPoint* pick = rp.getPickedPoint();
    //return (pick ? pick->copy() : 0); // needs the same instance of CRT under MS Windows
    return (pick ? new SoPickedPoint(*pick) : 0);
}

// ----------------------------------------------------------------------

/* TRANSLATOR MeshGui::MeshFaceAddition */

MeshFaceAddition::MeshFaceAddition(Gui::View3DInventor* parent)
  : QObject(parent), faceView(new MeshGui::ViewProviderFace())
{
}

MeshFaceAddition::~MeshFaceAddition()
{
    delete faceView;
}

void MeshFaceAddition::startEditing(MeshGui::ViewProviderMesh* vp)
{
    Gui::View3DInventor* view = static_cast<Gui::View3DInventor*>(parent());
    Gui::View3DInventorViewer* viewer = view->getViewer();
    viewer->setEditing(true);
    viewer->setRedirectToSceneGraph(true);

    faceView->mesh = vp;
    faceView->attach(vp->getObject());
    viewer->addViewProvider(faceView);
    //faceView->mesh->startEditing();
    viewer->addEventCallback(SoEvent::getClassTypeId(),
        MeshFaceAddition::addFacetCallback, this);
}

void MeshFaceAddition::finishEditing()
{
    Gui::View3DInventor* view = static_cast<Gui::View3DInventor*>(parent());
    Gui::View3DInventorViewer* viewer = view->getViewer();
    viewer->setEditing(false);
    viewer->setRedirectToSceneGraph(false);

    viewer->removeViewProvider(faceView);
    //faceView->mesh->finishEditing();
    viewer->removeEventCallback(SoEvent::getClassTypeId(),
        MeshFaceAddition::addFacetCallback, this);
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
    mesh->addFacets(faces);
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
    if (faceView->index.size() < 3)
        return;
    std::swap(faceView->index[0], faceView->index[1]);
    SbVec3f v1 = faceView->pcCoords->point[0];
    SbVec3f v2 = faceView->pcCoords->point[1];
    faceView->pcCoords->point.set1Value(0, v2);
    faceView->pcCoords->point.set1Value(1, v1);
}

bool MeshFaceAddition::addMarkerPoint()
{
    if (faceView->current_index < 0)
        return false;
    if (faceView->index.size() >= 3)
        return false;
    faceView->index.push_back(faceView->current_index);
    faceView->current_index = -1;
    if (faceView->index.size() == 3)
        faceView->setDisplayMode("Face");
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
            const MeshCore::MeshFacetArray& facets = mf->Mesh.getValuePtr()->getKernel().GetFacets();
            const MeshCore::MeshPointArray& points = mf->Mesh.getValuePtr()->getKernel().GetPoints();
            // is the face index valid?
            int face_index = fd->getFaceIndex();
            if (face_index >= (int)facets.size())
                return;
            // is a border facet picked? 
            MeshCore::MeshFacet f = facets[face_index];
            if (!f.HasOpenEdge())
                return;

            int point_index = -1;
            float distance = FLT_MAX;
            Base::Vector3f pnt;
            SbVec3f face_pnt;

            for (int i=0; i<3; i++) {
                int index = (int)f._aulPoints[i];
                if (std::find(faceView->index.begin(), faceView->index.end(), index) != faceView->index.end())
                    continue; // already inside
                if (f._aulNeighbours[i] == ULONG_MAX ||
                    f._aulNeighbours[(i+2)%3] == ULONG_MAX) {
                    pnt = points[index];
                    float len = Base::DistanceP2(pnt, Base::Vector3f(vec[0],vec[1],vec[2]));
                    if (len < distance) {
                        distance = len;
                        point_index = index;
                        face_pnt.setValue(pnt.x,pnt.y,pnt.z);
                    }
                }
            }

            if (point_index < 0)
                return; // picked point is rejected

            int num = faceView->pcCoords->point.getNum();
            if (faceView->current_index >= 0) {
                num = std::max<int>(num-1, 0);
            }
            faceView->current_index = point_index;
            faceView->pcCoords->point.set1Value(num, face_pnt);
            return;
        }
    }
}

void MeshFaceAddition::addFacetCallback(void * ud, SoEventCallback * n)
{
    MeshFaceAddition* that = reinterpret_cast<MeshFaceAddition*>(ud);
    ViewProviderFace* face =  that->faceView;
    Gui::View3DInventorViewer* view  = reinterpret_cast<Gui::View3DInventorViewer*>(n->getUserData());

    const SoEvent* ev = n->getEvent();
    if (ev->getTypeId() == SoLocation2Event::getClassTypeId()) {
        // set as handled
        n->getAction()->setHandled();
        n->setHandled();
        if (face->index.size() < 3) {
            SoPickedPoint * point = face->getPickedPoint(ev->getPosition(), view);
            if (point) {
                that->showMarker(point);
                delete point;
            }
        }
    }
    else if (ev->getTypeId() == SoMouseButtonEvent::getClassTypeId()) {
        // set as handled
        n->getAction()->setHandled();
        n->setHandled();
        const SoMouseButtonEvent * mbe = static_cast<const SoMouseButtonEvent *>(ev);
        if (mbe->getButton() == SoMouseButtonEvent::BUTTON1 && mbe->getState() == SoButtonEvent::DOWN) {
            that->addMarkerPoint();
        }
        else if (mbe->getButton() == SoMouseButtonEvent::BUTTON1 && mbe->getState() == SoButtonEvent::UP) {
            if (face->index.size() == 3) {
                QMenu menu;
                QAction* add = menu.addAction(MeshFaceAddition::tr("Add triangle"));
                QAction* swp = menu.addAction(MeshFaceAddition::tr("Flip normal"));
                QAction* clr = menu.addAction(MeshFaceAddition::tr("Clear"));
                QAction* act = menu.exec(QCursor::pos());
                if (act == add) {
                    QTimer::singleShot(300, that, SLOT(addFace()));
                }
                else if (act == swp) {
                    QTimer::singleShot(300, that, SLOT(flipNormal()));
                }
                else if (act == clr) {
                    QTimer::singleShot(300, that, SLOT(clearPoints()));
                }
            }
        }
        else if (mbe->getButton() == SoMouseButtonEvent::BUTTON2 && mbe->getState() == SoButtonEvent::UP) {
            QMenu menu;
            QAction* fin = menu.addAction(MeshFaceAddition::tr("Finish"));
            QAction* act = menu.exec(QCursor::pos());
            if (act == fin) {
                QTimer::singleShot(300, that, SLOT(finishEditing()));
            }
        }
    }
}

#include "moc_MeshEditor.cpp"
