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
# include <BRepGProp.hxx>
# include <GProp_GProps.hxx>
# include <gp_Pnt.hxx>
# include <TopExp_Explorer.hxx>
# include <TopoDS.hxx>
# include <TopTools_IndexedMapOfShape.hxx>
# include <QFontMetrics>
# include <QMessageBox>
# include <QSet>
# include <Python.h>
# include <Inventor/SoPickedPoint.h>
# include <Inventor/actions/SoRayPickAction.h>
# include <Inventor/actions/SoSearchAction.h>
# include <Inventor/details/SoFaceDetail.h>
# include <Inventor/events/SoMouseButtonEvent.h>
# include <Inventor/nodes/SoCamera.h>
# include <Inventor/nodes/SoSeparator.h>
#endif

#include "BoxSelection.h"
#include "ViewProviderExt.h"

#include <Gui/Application.h>
#include <Gui/Document.h>
#include <Gui/MainWindow.h>
#include <Gui/Selection.h>
#include <Gui/SelectionFilter.h>
#include <Gui/SoFCUnifiedSelection.h>
#include <Gui/Utilities.h>
#include <Gui/View3DInventor.h>
#include <Gui/View3DInventorViewer.h>

#include <App/Document.h>
#include <App/DocumentObject.h>
#include <Mod/Part/App/PartFeature.h>


using namespace PartGui;

class BoxSelection::FaceSelectionGate : public Gui::SelectionFilterGate
{
public:
    FaceSelectionGate()
        : Gui::SelectionFilterGate()
    {
    }
    ~FaceSelectionGate()
    {
    }
    bool allow(App::Document*, App::DocumentObject*, const char*sSubName)
    {
        if (!sSubName || sSubName[0] == '\0')
            return false;
        std::string element(sSubName);
        return element.substr(0,4) == "Face";
    }
};

BoxSelection::BoxSelection()
{

}

BoxSelection::~BoxSelection()
{

}

void BoxSelection::selectionCallback(void * ud, SoEventCallback * cb)
{
    Gui::View3DInventorViewer* view  = reinterpret_cast<Gui::View3DInventorViewer*>(cb->getUserData());
    view->removeEventCallback(SoMouseButtonEvent::getClassTypeId(), selectionCallback, ud);
    SoNode* root = view->getSceneGraph();
    static_cast<Gui::SoFCUnifiedSelection*>(root)->selectionRole.setValue(true);

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
        for (std::vector<SbVec2f>::const_iterator it = picked.begin(); it != picked.end(); ++it)
            polygon.Add(Base::Vector2d((*it)[0],(*it)[1]));
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
            self->addFacesToSelection(doc->getName(), it->getNameInDocument(), proj, polygon, shape);
        }
        view->redraw();
    }

    Gui::Selection().rmvSelectionGate();
    delete self;
}

void BoxSelection::addFacesToSelection(const char* doc, const char* obj,
                                       const Gui::ViewVolumeProjection& proj,
                                       const Base::Polygon2d& polygon,
                                       const TopoDS_Shape& shape)
{
    try {
        TopTools_IndexedMapOfShape M;

        TopExp_Explorer xp_face(shape,TopAbs_FACE);
        while (xp_face.More()) {
            M.Add(xp_face.Current());
            xp_face.Next();
        }

        for (Standard_Integer k = 1; k <= M.Extent(); k++) {
            const TopoDS_Shape& face = M(k);

            TopExp_Explorer xp_vertex(face,TopAbs_VERTEX);
            while (xp_vertex.More()) {
                gp_Pnt p = BRep_Tool::Pnt(TopoDS::Vertex(xp_vertex.Current()));
                Base::Vector3d pt2d;
                pt2d = proj(Base::Vector3d(p.X(), p.Y(), p.Z()));
                if (polygon.Contains(Base::Vector2d(pt2d.x, pt2d.y))) {
                    std::stringstream str;
                    str << "Face" << k;
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

void BoxSelection::start()
{
    Gui::View3DInventor* view = qobject_cast<Gui::View3DInventor*>(Gui::getMainWindow()->activeWindow());
    if (view) {
        Gui::View3DInventorViewer* viewer = view->getViewer();
        if (!viewer->isSelecting()) {
            viewer->startSelection(Gui::View3DInventorViewer::Rubberband);
            viewer->addEventCallback(SoMouseButtonEvent::getClassTypeId(), selectionCallback, this);
            // avoid that the selection node handles the event otherwise the callback function won't be
            // called immediately
            SoNode* root = viewer->getSceneGraph();
            static_cast<Gui::SoFCUnifiedSelection*>(root)->selectionRole.setValue(false);

            Gui::Selection().addSelectionGate(new FaceSelectionGate);
        }
    }
}
