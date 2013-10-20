/***************************************************************************
 *   Copyright (c) 2013 Werner Mayer <wmayer[at]users.sourceforge.net>     *
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
# include <climits>
# include <Inventor/SbBox2s.h>
# include <Inventor/SoPickedPoint.h>
# include <Inventor/details/SoFaceDetail.h>
# include <Inventor/events/SoLocation2Event.h>
# include <Inventor/events/SoMouseButtonEvent.h>
# include <Inventor/nodes/SoCamera.h>
#endif

#include "MeshSelection.h"
#include "ViewProvider.h"

#include <Base/Console.h>
#include <Base/Tools.h>
#include <App/Application.h>
#include <App/Document.h>
#include <Gui/Application.h>
#include <Gui/Document.h>
#include <Gui/MouseSelection.h>
#include <Gui/NavigationStyle.h>
#include <Gui/Utilities.h>
#include <Gui/View3DInventor.h>
#include <Gui/View3DInventorViewer.h>
#include <Mod/Mesh/App/MeshFeature.h>
#include <Mod/Mesh/App/Core/Algorithm.h>
#include <Mod/Mesh/App/Core/MeshKernel.h>
#include <Mod/Mesh/App/Core/Iterator.h>
#include <Mod/Mesh/App/Core/TopoAlgorithm.h>
#include <Mod/Mesh/App/Core/Tools.h>

using namespace MeshGui;

#define CROSS_WIDTH 16
#define CROSS_HEIGHT 16
#define CROSS_HOT_X 7
#define CROSS_HOT_Y 7

unsigned char MeshSelection::cross_bitmap[] = {
  0xc0, 0x03, 0x40, 0x02, 0x40, 0x02, 0x40, 0x02,
  0x40, 0x02, 0x40, 0x02, 0x7f, 0xfe, 0x01, 0x80,
  0x01, 0x80, 0x7f, 0xfe, 0x40, 0x02, 0x40, 0x02,
  0x40, 0x02, 0x40, 0x02, 0x40, 0x02, 0xc0, 0x03
};

unsigned char MeshSelection::cross_mask_bitmap[] = {
  0xc0, 0x03, 0xc0, 0x03, 0xc0, 0x03, 0xc0, 0x03,
  0xc0, 0x03, 0xc0, 0x03, 0xff, 0xff, 0xff, 0xff,
  0xff, 0xff, 0xff, 0xff, 0xc0, 0x03, 0xc0, 0x03,
  0xc0, 0x03, 0xc0, 0x03, 0xc0, 0x03, 0xc0, 0x03
};

MeshSelection::MeshSelection()
  : onlyPointToUserTriangles(false), onlyVisibleTriangles(false), _activeCB(0)
{
}

MeshSelection::~MeshSelection()
{
    if (_activeCB) {
        Gui::View3DInventorViewer* viewer = this->getViewer();
        if (viewer)
            stopInteractiveCallback(viewer);
    }
}

void MeshSelection::setObjects(const std::vector<Gui::SelectionObject>& obj)
{
    meshObjects = obj;
}

std::vector<App::DocumentObject*> MeshSelection::getObjects() const
{
    std::vector<App::DocumentObject*> objs;
    if (!meshObjects.empty()) {
        for (std::vector<Gui::SelectionObject>::iterator it = meshObjects.begin(); it != meshObjects.end(); ++it) {
            App::DocumentObject* obj = it->getObject();
            if (obj) {
                objs.push_back(obj);
            }
        }
    }
    // get all objects of the active document
    else {
        App::Document* doc = App::GetApplication().getActiveDocument();
        if (doc)
            objs = doc->getObjectsOfType(Mesh::Feature::getClassTypeId());
    }

    return objs;
}

std::list<ViewProviderMesh*> MeshSelection::getViewProviders() const
{
    std::vector<App::DocumentObject*> objs = getObjects();
    std::list<ViewProviderMesh*> vps;
    for (std::vector<App::DocumentObject*>::iterator it = objs.begin(); it != objs.end(); ++it) {
        if ((*it)->isDerivedFrom(Mesh::Feature::getClassTypeId())) {
            Gui::ViewProvider* vp = Gui::Application::Instance->getViewProvider(*it);
            if (vp->isVisible())
                vps.push_back(static_cast<ViewProviderMesh*>(vp));
        }
    }

    return vps;
}

Gui::View3DInventorViewer* MeshSelection::getViewer() const
{
    Gui::Document* doc = Gui::Application::Instance->activeDocument();
    if (!doc) return 0;
    Gui::MDIView* view = doc->getActiveView();
    if (view && view->getTypeId().isDerivedFrom(Gui::View3DInventor::getClassTypeId())) {
        Gui::View3DInventorViewer* viewer = static_cast<Gui::View3DInventor*>(view)->getViewer();
        return viewer;
    }

    return 0;
}

void MeshSelection::startInteractiveCallback(Gui::View3DInventorViewer* viewer,SoEventCallbackCB *cb)
{
    if (this->_activeCB)
        return;
    viewer->setEditing(true);
    viewer->addEventCallback(SoMouseButtonEvent::getClassTypeId(), cb, this);
    this->_activeCB = cb;
}

void MeshSelection::stopInteractiveCallback(Gui::View3DInventorViewer* viewer)
{
    if (!this->_activeCB)
        return;
    if (viewer->isEditing()) {
        viewer->setEditing(false);
        viewer->removeEventCallback(SoMouseButtonEvent::getClassTypeId(), this->_activeCB, this);
        this->_activeCB = 0;
    }
}

void MeshSelection::prepareBrushSelection(bool add,SoEventCallbackCB *cb)
{
    // a rubberband to select a rectangle area of the meshes
    Gui::View3DInventorViewer* viewer = this->getViewer();
    if (viewer) {
        stopInteractiveCallback(viewer);
        startInteractiveCallback(viewer, cb);
        // set cross cursor
        Gui::BrushSelection* brush = new Gui::BrushSelection();
        brush->setColor(1.0f,0.0f,0.0f);
        brush->setLineWidth(3.0f);
        viewer->navigationStyle()->startSelection(brush);
        SoQtCursor::CustomCursor custom;
        custom.dim.setValue(CROSS_WIDTH, CROSS_HEIGHT);
        custom.hotspot.setValue(CROSS_HOT_X, CROSS_HOT_Y);
        custom.bitmap = cross_bitmap;
        custom.mask = cross_mask_bitmap;
        viewer->setComponentCursor(SoQtCursor(&custom));
        this->addToSelection = add;
    }
}

void MeshSelection::startSelection()
{
    prepareBrushSelection(true, selectGLCallback);
}

void MeshSelection::startDeselection()
{
    prepareBrushSelection(false, selectGLCallback);
}

void MeshSelection::stopSelection()
{
    Gui::View3DInventorViewer* viewer = getViewer();
    if (viewer)
        stopInteractiveCallback(viewer);
}

void MeshSelection::fullSelection()
{
    // select the complete meshes
    std::list<ViewProviderMesh*> views = getViewProviders();
    for (std::list<ViewProviderMesh*>::iterator it = views.begin(); it != views.end(); ++it) {
        Mesh::Feature* mf = static_cast<Mesh::Feature*>((*it)->getObject());
        const Mesh::MeshObject* mo = mf->Mesh.getValuePtr();
        std::vector<unsigned long> faces(mo->countFacets());
        std::generate(faces.begin(), faces.end(), Base::iotaGen<unsigned long>(0));
        (*it)->addSelection(faces);
    }
}

void MeshSelection::clearSelection()
{
    std::list<ViewProviderMesh*> views = getViewProviders();
    for (std::list<ViewProviderMesh*>::iterator it = views.begin(); it != views.end(); ++it) {
        (*it)->clearSelection();
    }
}

bool MeshSelection::deleteSelection()
{
    // delete all selected faces
    bool selected = false;
    std::list<ViewProviderMesh*> views = getViewProviders();
    for (std::list<ViewProviderMesh*>::iterator it = views.begin(); it != views.end(); ++it) {
        Mesh::Feature* mf = static_cast<Mesh::Feature*>((*it)->getObject());
        unsigned long ct = MeshCore::MeshAlgorithm(mf->Mesh.getValue().getKernel()).
            CountFacetFlag(MeshCore::MeshFacet::SELECTED);
        if (ct > 0) {
            selected = true;
            break;
        }
    }
    if (!selected)
        return false; // nothing todo

    for (std::list<ViewProviderMesh*>::iterator it = views.begin(); it != views.end(); ++it) {
        (*it)->deleteSelection();
    }

    return true;
}

void MeshSelection::invertSelection()
{
    std::list<ViewProviderMesh*> views = getViewProviders();
    for (std::list<ViewProviderMesh*>::iterator it = views.begin(); it != views.end(); ++it) {
        Mesh::Feature* mf = static_cast<Mesh::Feature*>((*it)->getObject());
        const Mesh::MeshObject* mo = mf->Mesh.getValuePtr();
        const MeshCore::MeshFacetArray& faces = mo->getKernel().GetFacets();
        unsigned long num_notsel = std::count_if(faces.begin(), faces.end(),
            std::bind2nd(MeshCore::MeshIsNotFlag<MeshCore::MeshFacet>(),
            MeshCore::MeshFacet::SELECTED));
        std::vector<unsigned long> notselect;
        notselect.reserve(num_notsel);
        MeshCore::MeshFacetArray::_TConstIterator beg = faces.begin();
        MeshCore::MeshFacetArray::_TConstIterator end = faces.end();
        for (MeshCore::MeshFacetArray::_TConstIterator jt = beg; jt != end; ++jt) {
            if (!jt->IsFlag(MeshCore::MeshFacet::SELECTED))
                notselect.push_back(jt-beg);
        }
        (*it)->setSelection(notselect);
    }
}

void MeshSelection::selectComponent(int size)
{
    std::list<ViewProviderMesh*> views = getViewProviders();
    for (std::list<ViewProviderMesh*>::iterator it = views.begin(); it != views.end(); ++it) {
        Mesh::Feature* mf = static_cast<Mesh::Feature*>((*it)->getObject());
        const Mesh::MeshObject* mo = mf->Mesh.getValuePtr();

        std::vector<std::vector<unsigned long> > segm;
        MeshCore::MeshComponents comp(mo->getKernel());
        comp.SearchForComponents(MeshCore::MeshComponents::OverEdge,segm);

        std::vector<unsigned long> faces;
        for (std::vector<std::vector<unsigned long> >::iterator jt = segm.begin(); jt != segm.end(); ++jt) {
            if (jt->size() < (unsigned long)size)
                faces.insert(faces.end(), jt->begin(), jt->end());
        }

        (*it)->addSelection(faces);
    }
}

void MeshSelection::deselectComponent(int size)
{
    std::list<ViewProviderMesh*> views = getViewProviders();
    for (std::list<ViewProviderMesh*>::iterator it = views.begin(); it != views.end(); ++it) {
        Mesh::Feature* mf = static_cast<Mesh::Feature*>((*it)->getObject());
        const Mesh::MeshObject* mo = mf->Mesh.getValuePtr();

        std::vector<std::vector<unsigned long> > segm;
        MeshCore::MeshComponents comp(mo->getKernel());
        comp.SearchForComponents(MeshCore::MeshComponents::OverEdge,segm);

        std::vector<unsigned long> faces;
        for (std::vector<std::vector<unsigned long> >::iterator jt = segm.begin(); jt != segm.end(); ++jt) {
            if (jt->size() > (unsigned long)size)
                faces.insert(faces.end(), jt->begin(), jt->end());
        }

        (*it)->removeSelection(faces);
    }
}

void MeshSelection::selectTriangle()
{
    this->addToSelection = true;

    Gui::View3DInventorViewer* viewer = this->getViewer();
    if (viewer) {
        viewer->setEditingCursor(QCursor(Qt::OpenHandCursor));
        stopInteractiveCallback(viewer);
        startInteractiveCallback(viewer, pickFaceCallback);
    }
}

void MeshSelection::deselectTriangle()
{
    this->addToSelection = false;

    Gui::View3DInventorViewer* viewer = this->getViewer();
    if (viewer) {
        viewer->setEditingCursor(QCursor(Qt::OpenHandCursor));
        stopInteractiveCallback(viewer);
        startInteractiveCallback(viewer, pickFaceCallback);
    }
}

void MeshSelection::setCheckOnlyPointToUserTriangles(bool on)
{
    onlyPointToUserTriangles = on;
}

bool MeshSelection::isCheckedOnlyPointToUserTriangles() const
{
    return onlyPointToUserTriangles;
}

void MeshSelection::setCheckOnlyVisibleTriangles(bool on)
{
    onlyVisibleTriangles = on;
}

bool MeshSelection::isCheckedOnlyVisibleTriangles() const
{
    return onlyVisibleTriangles;
}

void MeshSelection::setAddComponentOnClick(bool on)
{
    addComponent = on;
}

void MeshSelection::setRemoveComponentOnClick(bool on)
{
    removeComponent = on;
}

void MeshSelection::selectGLCallback(void * ud, SoEventCallback * n)
{
    // When this callback function is invoked we must leave the edit mode
    Gui::View3DInventorViewer* view  = reinterpret_cast<Gui::View3DInventorViewer*>(n->getUserData());
    MeshSelection* self = reinterpret_cast<MeshSelection*>(ud);
    self->stopInteractiveCallback(view);
    n->setHandled();
    std::vector<SbVec2f> polygon = view->getGLPolygon();
    if (polygon.size() < 3)
        return;
    if (polygon.front() != polygon.back())
        polygon.push_back(polygon.front());

    SbVec3f pnt, dir;
    view->getNearPlane(pnt, dir);
    Base::Vector3f point (pnt[0],pnt[1],pnt[2]);
    Base::Vector3f normal(dir[0],dir[1],dir[2]);

    std::list<ViewProviderMesh*> views = self->getViewProviders();
    for (std::list<ViewProviderMesh*>::iterator it = views.begin(); it != views.end(); ++it) {
        ViewProviderMesh* vp = static_cast<ViewProviderMesh*>(*it);

        std::vector<unsigned long> faces;
        const Mesh::MeshObject& mesh = static_cast<Mesh::Feature*>((*it)->getObject())->Mesh.getValue();
        const MeshCore::MeshKernel& kernel = mesh.getKernel();

        // simply get all triangles under the polygon
        SoCamera* cam = view->getCamera();
        SbViewVolume vv = cam->getViewVolume();
        Gui::ViewVolumeProjection proj(vv);
        vp->getFacetsFromPolygon(polygon, proj, true, faces);
        if (self->onlyVisibleTriangles) {
            const SbVec2s& sz = view->getViewportRegion().getWindowSize();
            short width,height; sz.getValue(width,height);
            std::vector<SbVec2s> pixelPoly = view->getPolygon();
            SbBox2s rect;
            for (std::vector<SbVec2s>::iterator it = pixelPoly.begin(); it != pixelPoly.end(); ++it) {
                const SbVec2s& p = *it;
                rect.extendBy(SbVec2s(p[0],height-p[1]));
            }
            std::vector<unsigned long> rf; rf.swap(faces);
            std::vector<unsigned long> vf = vp->getVisibleFacetsAfterZoom
                (rect, view->getViewportRegion(), view->getCamera());

            // get common facets of the viewport and the visible one
            std::sort(vf.begin(), vf.end());
            std::sort(rf.begin(), rf.end());
            std::back_insert_iterator<std::vector<unsigned long> > biit(faces);
            std::set_intersection(vf.begin(), vf.end(), rf.begin(), rf.end(), biit);
        }

        // if set filter out all triangles which do not point into user direction
        if (self->onlyPointToUserTriangles) {
            std::vector<unsigned long> screen;
            screen.reserve(faces.size());
            MeshCore::MeshFacetIterator it_f(kernel);
            for (std::vector<unsigned long>::iterator it = faces.begin(); it != faces.end(); ++it) {
                it_f.Set(*it);
                if (it_f->GetNormal() * normal > 0.0f) {
                    screen.push_back(*it);
                }
            }

            faces.swap(screen);
        }

        if (self->addToSelection)
            vp->addSelection(faces);
        else
            vp->removeSelection(faces);
    }

    view->render();
}

void MeshSelection::pickFaceCallback(void * ud, SoEventCallback * n)
{
    // handle only mouse button events
    if (n->getEvent()->isOfType(SoMouseButtonEvent::getClassTypeId())) {
        const SoMouseButtonEvent * mbe = static_cast<const SoMouseButtonEvent*>(n->getEvent());
        Gui::View3DInventorViewer* view  = reinterpret_cast<Gui::View3DInventorViewer*>(n->getUserData());

        // Mark all incoming mouse button events as handled, especially, to deactivate the selection node
        n->getAction()->setHandled();
        if (mbe->getButton() == SoMouseButtonEvent::BUTTON1 && mbe->getState() == SoButtonEvent::DOWN) {
            const SoPickedPoint * point = n->getPickedPoint();
            if (point == NULL) {
                Base::Console().Message("No facet picked.\n");
                return;
            }

            n->setHandled();

            // By specifying the indexed mesh node 'pcFaceSet' we make sure that the picked point is
            // really from the mesh we render and not from any other geometry
            Gui::ViewProvider* vp = static_cast<Gui::ViewProvider*>(view->getViewProviderByPath(point->getPath()));
            if (!vp || !vp->getTypeId().isDerivedFrom(ViewProviderMesh::getClassTypeId()))
                return;
            ViewProviderMesh* mesh = static_cast<ViewProviderMesh*>(vp);
            MeshSelection* self = reinterpret_cast<MeshSelection*>(ud);
            std::list<ViewProviderMesh*> views = self->getViewProviders();
            if (std::find(views.begin(), views.end(), mesh) == views.end())
                return;
            const SoDetail* detail = point->getDetail(/*mesh->getShapeNode()*/);
            if (detail && detail->getTypeId() == SoFaceDetail::getClassTypeId()) {
                // get the boundary to the picked facet
                unsigned long uFacet = static_cast<const SoFaceDetail*>(detail)->getFaceIndex();
                if (self->addToSelection) {
                    if (self->addComponent)
                        mesh->selectComponent(uFacet);
                    else
                        mesh->selectFacet(uFacet);
                }
                else {
                    if (self->removeComponent)
                        mesh->deselectComponent(uFacet);
                    else
                        mesh->deselectFacet(uFacet);
                }
            }
        }
    }
}
