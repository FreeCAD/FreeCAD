/***************************************************************************
 *   Copyright (c) 2018 Werner Mayer <wmayer[at]users.sourceforge.net>     *
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
# include <sstream>
# include <BRep_Tool.hxx>
# include <gp_Pnt.hxx>
# include <TopExp_Explorer.hxx>
# include <TopoDS.hxx>
# include <TopTools_IndexedMapOfShape.hxx>
# include <Inventor/SoPickedPoint.h>
# include <Inventor/events/SoMouseButtonEvent.h>
# include <Inventor/nodes/SoCamera.h>
#endif
#include <Gui/View3DInventor.h>
#include <Gui/View3DInventorViewer.h>
#include <App/Document.h>
#include <App/DocumentObject.h>
#include <Gui/Application.h>
#include <Gui/MainWindow.h>
#include <Gui/Selection.h>
#include <Gui/SelectionFilter.h>
#include <Gui/Utilities.h>

#include "BoxSelection.h"
#include "ViewProviderExt.h"


using namespace PartGui;

class BoxSelection::FaceSelectionGate : public Gui::SelectionFilterGate
{
public:
    FaceSelectionGate()
        : Gui::SelectionFilterGate()
    {
    }
    ~FaceSelectionGate() override = default;
    bool allow(App::Document*, App::DocumentObject*, const char*sSubName) override
    {
        if (!sSubName || sSubName[0] == '\0')
            return false;
        std::string element(sSubName);
        return element.substr(0,4) == "Face";
    }
};

BoxSelection::BoxSelection() = default;

BoxSelection::~BoxSelection() = default;

void BoxSelection::setAutoDelete(bool on)
{
    autodelete = on;
}

bool BoxSelection::isAutoDelete() const
{
    return autodelete;
}

void BoxSelection::selectionCallback(void * ud, SoEventCallback * cb)
{
    Gui::View3DInventorViewer* view  = static_cast<Gui::View3DInventorViewer*>(cb->getUserData());
    view->removeEventCallback(SoMouseButtonEvent::getClassTypeId(), selectionCallback, ud);
    view->setSelectionEnabled(true);

    std::vector<SbVec2f> picked = view->getGLPolygon();
    SoCamera* cam = view->getSoRenderManager()->getCamera();
    SbViewVolume vv = cam->getViewVolume();
    Gui::ViewVolumeProjection proj(vv);
    Base::Polygon2d polygon;
    if (picked.size() == 2) {
        SbVec2f pt1 = picked[0];
        SbVec2f pt2 = picked[1];
        polygon.Add(Base::Vector2d(pt1[0], pt1[1]));
        polygon.Add(Base::Vector2d(pt1[0], pt2[1]));
        polygon.Add(Base::Vector2d(pt2[0], pt2[1]));
        polygon.Add(Base::Vector2d(pt2[0], pt1[1]));
    }
    else {
        for (const auto& it : picked)
            polygon.Add(Base::Vector2d(it[0],it[1]));
    }

    BoxSelection* self = static_cast<BoxSelection*>(ud);
    App::Document* doc = App::GetApplication().getActiveDocument();
    if (doc) {
        cb->setHandled();

        std::vector<Part::Feature*> geom = doc->getObjectsOfType<Part::Feature>();
        for (auto it : geom) {
            Gui::ViewProvider* vp = Gui::Application::Instance->getViewProvider(it);
            if (!vp->isVisible())
                continue;
            const TopoDS_Shape& shape = it->Shape.getValue();
            self->addShapeToSelection(doc->getName(), it->getNameInDocument(), proj, polygon, shape, self->shapeEnum);
        }
        view->redraw();
    }

    Gui::Selection().rmvSelectionGate();

    if (self->isAutoDelete()) {
        delete self;
    }
}

const char* BoxSelection::nameFromShapeType(TopAbs_ShapeEnum type) const
{
    switch (type) {
    case TopAbs_FACE:
        return "Face";
    case TopAbs_EDGE:
        return "Edge";
    case TopAbs_VERTEX:
        return "Vertex";
    default:
        return nullptr;
    }
}

void BoxSelection::addShapeToSelection(const char* doc, const char* obj,
                                       const Gui::ViewVolumeProjection& proj,
                                       const Base::Polygon2d& polygon,
                                       const TopoDS_Shape& shape,
                                       TopAbs_ShapeEnum subtype)
{
    try {
        const char* subname = nameFromShapeType(subtype);
        if (!subname)
            return;

        TopTools_IndexedMapOfShape M;
        TopExp_Explorer xp(shape, subtype);
        while (xp.More()) {
            M.Add(xp.Current());
            xp.Next();
        }

        for (Standard_Integer k = 1; k <= M.Extent(); k++) {
            const TopoDS_Shape& subshape = M(k);

            TopExp_Explorer xp_vertex(subshape, TopAbs_VERTEX);
            while (xp_vertex.More()) {
                gp_Pnt p = BRep_Tool::Pnt(TopoDS::Vertex(xp_vertex.Current()));
                Base::Vector3d pt2d;
                pt2d = proj(Base::Vector3d(p.X(), p.Y(), p.Z()));
                if (polygon.Contains(Base::Vector2d(pt2d.x, pt2d.y))) {
                    std::stringstream str;
                    str << subname << k;
                    Gui::Selection().addSelection(doc, obj, str.str().c_str());
                    break;
                }
                xp_vertex.Next();
            }
        }
    }
    catch (...) {
    }
}

void BoxSelection::start(TopAbs_ShapeEnum shape)
{
    Gui::View3DInventor* view = qobject_cast<Gui::View3DInventor*>(Gui::getMainWindow()->activeWindow());
    if (view) {
        Gui::View3DInventorViewer* viewer = view->getViewer();
        if (!viewer->isSelecting()) {
            viewer->startSelection(Gui::View3DInventorViewer::Rubberband);
            viewer->addEventCallback(SoMouseButtonEvent::getClassTypeId(), selectionCallback, this);
            // avoid that the selection node handles the event otherwise the callback function won't be
            // called immediately
            viewer->setSelectionEnabled(false);
            shapeEnum = shape;
        }
    }
}
