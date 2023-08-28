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

#include "MDIView.h"
#include "MDIViewPy.h"


class SoEventCallback;
class SoDragger;
class QImage;

namespace Gui {

class View3DInventor;

class View3DInventorPy : public Py::PythonExtension<View3DInventorPy>
{
public:
    using BaseType = Py::PythonExtension<View3DInventorPy>;
    static void init_type();    // announce properties and methods

    explicit View3DInventorPy(View3DInventor *vi);
    ~View3DInventorPy() override;

    View3DInventor* getView3DIventorPtr();
    Py::Object repr() override;
    Py::Object getattr(const char *) override;
    int setattr(const char *, const Py::Object &) override;
    Py::Object cast_to_base();

    Py::Object fitAll(const Py::Tuple&);
    Py::Object boxZoom(const Py::Tuple&, const Py::Dict&);
    Py::Object viewBottom();
    Py::Object viewFront();
    Py::Object viewLeft();
    Py::Object viewRear();
    Py::Object viewRight();
    Py::Object viewTop();
    Py::Object viewIsometric();
    Py::Object viewDimetric();
    Py::Object viewTrimetric();
    Py::Object viewDefaultOrientation(const Py::Tuple&);
    Py::Object viewPosition(const Py::Tuple&);
    Py::Object viewRotateLeft();
    Py::Object viewRotateRight();
    Py::Object zoomIn();
    Py::Object zoomOut();
    Py::Object startAnimating(const Py::Tuple&);
    Py::Object stopAnimating();
    Py::Object setAnimationEnabled(const Py::Tuple&);
    Py::Object isAnimationEnabled();
    Py::Object setPopupMenuEnabled(const Py::Tuple&);
    Py::Object isPopupMenuEnabled();
    Py::Object dump(const Py::Tuple&);
    Py::Object dumpNode(const Py::Tuple&);
    Py::Object setStereoType(const Py::Tuple&);
    Py::Object getStereoType();
    Py::Object listStereoTypes();
    Py::Object saveImage(const Py::Tuple&);
    Py::Object saveVectorGraphic(const Py::Tuple&);
    Py::Object getCamera();
    Py::Object getViewDirection();
    Py::Object getUpDirection();
    Py::Object setViewDirection(const Py::Tuple&);
    Py::Object setCamera(const Py::Tuple&);
    Py::Object setCameraOrientation(const Py::Tuple&);
    Py::Object getCameraOrientation();
    Py::Object getCameraType();
    Py::Object setCameraType(const Py::Tuple&);
    Py::Object getCameraNode();
    Py::Object listCameraTypes();
    Py::Object getCursorPos();
    Py::Object getObjectInfo(const Py::Tuple&);
    Py::Object getObjectsInfo(const Py::Tuple&);
    Py::Object getSize();
    Py::Object getPointOnFocalPlane(const Py::Tuple&);
    Py::Object projectPointToLine(const Py::Tuple&);
    Py::Object getPointOnViewport(const Py::Tuple&);
    Py::Object addEventCallback(const Py::Tuple&);
    Py::Object removeEventCallback(const Py::Tuple&);
    Py::Object setAnnotation(const Py::Tuple&);
    Py::Object removeAnnotation(const Py::Tuple&);
    Py::Object getSceneGraph();
    Py::Object getViewer();
    Py::Object addEventCallbackPivy(const Py::Tuple&);
    Py::Object removeEventCallbackPivy(const Py::Tuple&);
    Py::Object listNavigationTypes();
    Py::Object getNavigationType();
    Py::Object setNavigationType(const Py::Tuple&);
    Py::Object setAxisCross(const Py::Tuple&);
    Py::Object hasAxisCross();
    Py::Object addDraggerCallback(const Py::Tuple&);
    Py::Object removeDraggerCallback(const Py::Tuple&);
    Py::Object getViewProvidersOfType(const Py::Tuple&);
    Py::Object redraw();
    Py::Object setName(const Py::Tuple&);
    Py::Object toggleClippingPlane(const Py::Tuple& args, const Py::Dict &);
    Py::Object hasClippingPlane();
    Py::Object graphicsView();
    Py::Object setCornerCrossVisible(const Py::Tuple& args);
    Py::Object isCornerCrossVisible();
    Py::Object setCornerCrossSize(const Py::Tuple& args);
    Py::Object getCornerCrossSize();

private:
    void setDefaultCameraHeight(float);
    static void eventCallback(void * ud, SoEventCallback * n);
    static void eventCallbackPivy(void * ud, SoEventCallback * n);
    static void eventCallbackPivyEx(void * ud, SoEventCallback * n);
    static void draggerCallback(void * ud, SoDragger* dragger);

private:
    using method_varargs_handler = PyObject* (*)(PyObject *_self, PyObject *_args);
    static method_varargs_handler pycxx_handler;
    static PyObject *method_varargs_ext_handler(PyObject *_self, PyObject *_args);
    Py::Object getattribute(const char *);

private:
    Gui::MDIViewPy base;
    std::list<PyObject*> callbacks;
};

} // namespace Gui

#endif //GUI_VIEW3DPY_H
