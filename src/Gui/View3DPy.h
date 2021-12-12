/***************************************************************************
 *   Copyright (c) 2005 Jürgen Riegel <juergen.riegel@web.de>              *
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


#ifndef GUI_VIEW3DPY_H
#define GUI_VIEW3DPY_H

#include <Base/PyObjectBase.h>
#include <CXX/Extensions.hxx>
#include <Gui/MDIView.h>
#include <Gui/MDIViewPy.h>

class SoEventCallback;
class SoDragger;
class QImage;

namespace Gui {

class View3DInventor;

class Camera
{
public:
    enum Orientation {
        Top,
        Bottom,
        Front,
        Rear,
        Left,
        Right,
        Isometric,
        Dimetric,
        Trimetric,
    };

    static SbRotation rotation(Orientation view);
};

class View3DInventorPy : public Py::PythonExtension<View3DInventorPy>
{
public:
    using BaseType = Py::PythonExtension<View3DInventorPy>;
    static void init_type();    // announce properties and methods

    View3DInventorPy(View3DInventor *vi);
    ~View3DInventorPy();

    View3DInventor* getView3DIventorPtr();
    Py::Object repr();
    Py::Object getattr(const char *);
    int setattr(const char *, const Py::Object &);
    Py::Object cast_to_base(const Py::Tuple&);

    Py::Object fitAll(const Py::Tuple&);
    Py::Object boxZoom(const Py::Tuple&, const Py::Dict&);
    Py::Object viewBottom(const Py::Tuple&);
    Py::Object viewFront(const Py::Tuple&);
    Py::Object viewLeft(const Py::Tuple&);
    Py::Object viewRear(const Py::Tuple&);
    Py::Object viewRight(const Py::Tuple&);
    Py::Object viewTop(const Py::Tuple&);
    Py::Object viewIsometric(const Py::Tuple&);
    Py::Object viewDimetric(const Py::Tuple&);
    Py::Object viewTrimetric(const Py::Tuple&);
    Py::Object viewDefaultOrientation(const Py::Tuple&);
    Py::Object viewPosition(const Py::Tuple&);
    Py::Object viewRotateLeft(const Py::Tuple&);
    Py::Object viewRotateRight(const Py::Tuple&);
    Py::Object zoomIn(const Py::Tuple&);
    Py::Object zoomOut(const Py::Tuple&);
    Py::Object startAnimating(const Py::Tuple&);
    Py::Object stopAnimating(const Py::Tuple&);
    Py::Object setAnimationEnabled(const Py::Tuple&);
    Py::Object isAnimationEnabled(const Py::Tuple&);
    Py::Object setPopupMenuEnabled(const Py::Tuple&);
    Py::Object isPopupMenuEnabled(const Py::Tuple&);
    Py::Object dump(const Py::Tuple&);
    Py::Object dumpNode(const Py::Tuple&);
    Py::Object setStereoType(const Py::Tuple&);
    Py::Object getStereoType(const Py::Tuple&);
    Py::Object listStereoTypes(const Py::Tuple&);
    Py::Object saveImage(const Py::Tuple&);
    Py::Object saveVectorGraphic(const Py::Tuple&);
    Py::Object getCamera(const Py::Tuple&);
    Py::Object getViewDirection(const Py::Tuple&);
    Py::Object setViewDirection(const Py::Tuple&);
    Py::Object setCamera(const Py::Tuple&);
    Py::Object setCameraOrientation(const Py::Tuple&);
    Py::Object getCameraOrientation(const Py::Tuple&);
    Py::Object getCameraType(const Py::Tuple&);
    Py::Object setCameraType(const Py::Tuple&);
    Py::Object getCameraNode(const Py::Tuple&);
    Py::Object listCameraTypes(const Py::Tuple&);
    Py::Object getCursorPos(const Py::Tuple&);
    Py::Object getObjectInfo(const Py::Tuple&);
    Py::Object getObjectsInfo(const Py::Tuple&);
    Py::Object getSize(const Py::Tuple&);
    Py::Object getPointOnFocalPlane(const Py::Tuple&);
    Py::Object projectPointToLine(const Py::Tuple&);
    Py::Object getPointOnScreen(const Py::Tuple&);
    Py::Object addEventCallback(const Py::Tuple&);
    Py::Object removeEventCallback(const Py::Tuple&);
    Py::Object setAnnotation(const Py::Tuple&);
    Py::Object removeAnnotation(const Py::Tuple&);
    Py::Object getSceneGraph(const Py::Tuple&);
    Py::Object getViewer(const Py::Tuple&);
    Py::Object addEventCallbackPivy(const Py::Tuple&);
    Py::Object removeEventCallbackPivy(const Py::Tuple&);
    Py::Object listNavigationTypes(const Py::Tuple&);
    Py::Object getNavigationType(const Py::Tuple&);
    Py::Object setNavigationType(const Py::Tuple&);
    Py::Object setAxisCross(const Py::Tuple&);
    Py::Object hasAxisCross(const Py::Tuple&);
    Py::Object addDraggerCallback(const Py::Tuple&);
    Py::Object removeDraggerCallback(const Py::Tuple&);
    Py::Object setActiveObject(const Py::Tuple&);
    Py::Object getActiveObject(const Py::Tuple&);
    Py::Object getViewProvidersOfType(const Py::Tuple&);
    Py::Object redraw(const Py::Tuple&);
    Py::Object setName(const Py::Tuple&);
    Py::Object toggleClippingPlane(const Py::Tuple& args, const Py::Dict &);
    Py::Object hasClippingPlane(const Py::Tuple& args);
    Py::Object graphicsView(const Py::Tuple& args);
    Py::Object setCornerCrossVisible(const Py::Tuple& args);
    Py::Object isCornerCrossVisible(const Py::Tuple& args);
    Py::Object setCornerCrossSize(const Py::Tuple& args);
    Py::Object getCornerCrossSize(const Py::Tuple& args);

private:
    static void eventCallback(void * ud, SoEventCallback * n);
    static void eventCallbackPivy(void * ud, SoEventCallback * n);
    static void eventCallbackPivyEx(void * ud, SoEventCallback * n);
    static void draggerCallback(void * ud, SoDragger* dragger);

private:
    typedef PyObject* (*method_varargs_handler)(PyObject *_self, PyObject *_args);
    static method_varargs_handler pycxx_handler;
    static PyObject *method_varargs_ext_handler(PyObject *_self, PyObject *_args);
    Py::Object getattribute(const char *);

private:
    Gui::MDIViewPy base;
    std::list<PyObject*> callbacks;
};

} // namespace Gui

#endif //GUI_VIEW3DPY_H
