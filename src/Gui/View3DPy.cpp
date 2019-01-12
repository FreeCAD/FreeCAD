/***************************************************************************
 *   Copyright (c) Jürgen Riegel          (juergen.riegel@web.de) 2002     *
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

#ifndef __InventorAll__
# include "InventorAll.h"
# include <sstream>
# include <QColor>
# include <QDir>
# include <QFileInfo>
# include <QImage>
# include <Inventor/SbViewVolume.h>
# include <Inventor/nodes/SoCamera.h>
#endif

#include <QtOpenGL.h>
#include "View3DPy.h"
#include "ViewProviderDocumentObject.h"
#include "ViewProviderExtern.h"
#include "Application.h"
#include "Document.h"
#include "NavigationStyle.h"
#include "SoFCSelectionAction.h"
#include "SoFCOffscreenRenderer.h"
#include "SoFCVectorizeSVGAction.h"
#include "SoFCVectorizeU3DAction.h"
#include "SoFCDB.h"
#include "View3DInventor.h"
#include "View3DInventorViewer.h"
#include "View3DViewerPy.h"
#include "ActiveObjectList.h"


#include <Base/Console.h>
#include <Base/Exception.h>
#include <Base/Interpreter.h>
#include <Base/PlacementPy.h>
#include <Base/Rotation.h>
#include <Base/RotationPy.h>
#include <Base/VectorPy.h>
#include <Base/GeometryPyCXX.h>

#include <App/Document.h>
#include <App/DocumentObject.h>
#include <App/DocumentObjectPy.h>
#include <CXX/Objects.hxx>

using namespace Gui;


void View3DInventorPy::init_type()
{
    behaviors().name("View3DInventorPy");
    behaviors().doc("Python binding class for the Inventor viewer class");
    // you must have overwritten the virtual functions
    behaviors().supportRepr();
    behaviors().supportGetattr();
    behaviors().supportSetattr();

    add_varargs_method("message",&View3DInventorPy::message,"message()");
    add_varargs_method("fitAll",&View3DInventorPy::fitAll,"fitAll()");
    add_keyword_method("boxZoom",&View3DInventorPy::boxZoom,"boxZoom()");

    add_varargs_method("viewBottom",&View3DInventorPy::viewBottom,"viewBottom()");
    add_varargs_method("viewFront",&View3DInventorPy::viewFront,"viewFront()");
    add_varargs_method("viewLeft",&View3DInventorPy::viewLeft,"viewLeft()");
    add_varargs_method("viewRear",&View3DInventorPy::viewRear,"viewRear()");
    add_varargs_method("viewRight",&View3DInventorPy::viewRight,"viewRight()");
    add_varargs_method("viewTop",&View3DInventorPy::viewTop,"viewTop()");
    add_varargs_method("viewAxometric",&View3DInventorPy::viewIsometric,"viewAxonometric()"); // for backward compatibility
    add_varargs_method("viewAxonometric",&View3DInventorPy::viewIsometric,"viewAxonometric()");
    add_varargs_method("viewIsometric",&View3DInventorPy::viewIsometric,"viewIsometric()");
    add_varargs_method("viewDimetric",&View3DInventorPy::viewDimetric,"viewDimetric()");
    add_varargs_method("viewTrimetric",&View3DInventorPy::viewTrimetric,"viewTrimetric()");
    add_varargs_method("viewDefaultOrientation",&View3DInventorPy::viewDefaultOrientation,"viewDefaultOrientation()");
    add_varargs_method("viewRotateLeft",&View3DInventorPy::viewRotateLeft,"viewRotateLeft()");
    add_varargs_method("viewRotateRight",&View3DInventorPy::viewRotateRight,"viewRotateRight()");
    add_varargs_method("zoomIn",&View3DInventorPy::zoomIn,"zoomIn()");
    add_varargs_method("zoomOut",&View3DInventorPy::zoomOut,"zoomOut()");
    add_varargs_method("viewPosition",&View3DInventorPy::viewPosition,"viewPosition()");
    add_varargs_method("startAnimating",&View3DInventorPy::startAnimating,"startAnimating()");
    add_varargs_method("stopAnimating",&View3DInventorPy::stopAnimating,"stopAnimating()");
    add_varargs_method("setAnimationEnabled",&View3DInventorPy::setAnimationEnabled,"setAnimationEnabled()");
    add_varargs_method("isAnimationEnabled",&View3DInventorPy::isAnimationEnabled,"isAnimationEnabled()");
    add_varargs_method("dump",&View3DInventorPy::dump,"dump()");
    add_varargs_method("dumpNode",&View3DInventorPy::dumpNode,"dumpNode(node)");
    add_varargs_method("setStereoType",&View3DInventorPy::setStereoType,"setStereoType()");
    add_varargs_method("getStereoType",&View3DInventorPy::getStereoType,"getStereoType()");
    add_varargs_method("listStereoTypes",&View3DInventorPy::listStereoTypes,"listStereoTypes()");
    add_varargs_method("saveImage",&View3DInventorPy::saveImage,"saveImage()");
    add_varargs_method("saveVectorGraphic",&View3DInventorPy::saveVectorGraphic,"saveVectorGraphic()");
    add_varargs_method("getCamera",&View3DInventorPy::getCamera,"getCamera()");
    add_varargs_method("getCameraNode",&View3DInventorPy::getCameraNode,"getCameraNode()");
    add_varargs_method("getViewDirection",&View3DInventorPy::getViewDirection,"getViewDirection() --> tuple of integers\n"
        "returns the direction vector the view is currently pointing at as tuple with xyz values\n"
    );
    add_varargs_method("setViewDirection",&View3DInventorPy::setViewDirection,"setViewDirection(tuple) --> None\n"
        "Sets the direction the view is pointing at. The direction must be given as tuple with\n"
        "three coordinates xyz"
    );
    add_varargs_method("setCamera",&View3DInventorPy::setCamera,"setCamera()");
    add_varargs_method("setCameraOrientation",&View3DInventorPy::setCameraOrientation,"setCameraOrientation()");
    add_varargs_method("getCameraOrientation",&View3DInventorPy::getCameraOrientation,"getCameraOrientation()");
    add_varargs_method("getCameraType",&View3DInventorPy::getCameraType,"getCameraType()");
    add_varargs_method("setCameraType",&View3DInventorPy::setCameraType,"setCameraType()");
    add_varargs_method("listCameraTypes",&View3DInventorPy::listCameraTypes,"listCameraTypes()");
    add_varargs_method("getCursorPos",&View3DInventorPy::getCursorPos,
        "getCursorPos() -> tuple of integers\n"
        "\n"
        "Return the current cursor position relative to the coordinate system of the\n"
        "viewport region.\n");
    add_varargs_method("getObjectInfo",&View3DInventorPy::getObjectInfo,
        "getObjectInfo(tuple(int,int), [pick_radius]) -> dictionary or None\n"
        "\n"
        "Return a dictionary with the name of document, object and component. The\n"
        "dictionary also contains the coordinates of the appropriate 3d point of\n"
        "the underlying geometry in the scenegraph.\n"
        "If no geometry was found 'None' is returned, instead.\n");
    add_varargs_method("getObjectsInfo",&View3DInventorPy::getObjectsInfo,
        "getObjectsInfo(tuple(int,int), [pick_radius]) -> dictionary or None\n"
        "\n"
        "Does the same as getObjectInfo() but returns a list of dictionaries or None.\n");
    add_varargs_method("getSize",&View3DInventorPy::getSize,"getSize()");
    add_varargs_method("getPoint",&View3DInventorPy::getPoint,
        "getPoint(pixel coords (as integer)) -> 3D vector\n"
        "\n"
        "Return the according 3D point on the focal plane to the given 2D point (in\n"
        "pixel coordinates).\n");
    add_varargs_method("getPointOnScreen",&View3DInventorPy::getPointOnScreen,
        "getPointOnScreen(3D vector) -> pixel coords (as integer)\n"
        "\n"
        "Return the projected 3D point (in pixel coordinates).\n");
    add_varargs_method("addEventCallback",&View3DInventorPy::addEventCallback,"addEventCallback()");
    add_varargs_method("removeEventCallback",&View3DInventorPy::removeEventCallback,"removeEventCallback()");
    add_varargs_method("setAnnotation",&View3DInventorPy::setAnnotation,"setAnnotation()");
    add_varargs_method("removeAnnotation",&View3DInventorPy::removeAnnotation,"removeAnnotation()");
    add_varargs_method("getSceneGraph",&View3DInventorPy::getSceneGraph,"getSceneGraph()");
    add_varargs_method("getViewer",&View3DInventorPy::getViewer,"getViewer()");
    add_varargs_method("addEventCallbackPivy",&View3DInventorPy::addEventCallbackPivy,"addEventCallbackPivy()");
    add_varargs_method("removeEventCallbackPivy",&View3DInventorPy::removeEventCallbackPivy,"removeEventCallbackPivy()");
    add_varargs_method("addEventCallbackSWIG",&View3DInventorPy::addEventCallbackPivy,
        "Deprecated -- use addEventCallbackPivy()");
    add_varargs_method("removeEventCallbackSWIG",&View3DInventorPy::removeEventCallbackPivy,
        "Deprecated -- use removeEventCallbackPivy()");
    add_varargs_method("listNavigationTypes",&View3DInventorPy::listNavigationTypes,"listNavigationTypes()");
    add_varargs_method("getNavigationType",&View3DInventorPy::getNavigationType,"getNavigationType()");
    add_varargs_method("setNavigationType",&View3DInventorPy::setNavigationType,"setNavigationType()");
    add_varargs_method("setAxisCross",&View3DInventorPy::setAxisCross,"switch the big axis-cross on and off");
    add_varargs_method("hasAxisCross",&View3DInventorPy::hasAxisCross,"check if the big axis-cross is on or off()");
    add_varargs_method("addDraggerCallback",&View3DInventorPy::addDraggerCallback,
        "addDraggerCallback(SoDragger, String CallbackType, function)\n"
        "Add a DraggerCalback function to the coin node\n"
        "Possibles types :\n"
        "'addFinishCallback','addStartCallback','addMotionCallback','addValueChangedCallback'\n");
    add_varargs_method("removeDraggerCallback",&View3DInventorPy::removeDraggerCallback,
        "removeDraggerCallback(SoDragger, String CallbackType, function)\n"
        "Remove the DraggerCalback function from the coin node\n"
        "Possibles types :\n"
        "'addFinishCallback','addStartCallback','addMotionCallback','addValueChangedCallback'\n");
    add_varargs_method("setActiveObject", &View3DInventorPy::setActiveObject, "setActiveObject(name,object)\nadd or set a new active object");
    add_varargs_method("getActiveObject", &View3DInventorPy::getActiveObject, "getActiveObject(name)\nreturns the active object for the given type");
    add_varargs_method("getViewProvidersOfType", &View3DInventorPy::getViewProvidersOfType, "getViewProvidersOfType(name)\nreturns a list of view providers for the given type");
    add_varargs_method("redraw", &View3DInventorPy::redraw, "redraw(): renders the scene on screen (useful for animations)");

}

View3DInventorPy::View3DInventorPy(View3DInventor *vi)
  : _view(vi)
{
}

View3DInventorPy::~View3DInventorPy()
{
    Base::PyGILStateLocker lock;
    for (std::list<PyObject*>::iterator it = callbacks.begin(); it != callbacks.end(); ++it)
        Py_DECREF(*it);
}

Py::Object View3DInventorPy::repr()
{
    std::string s;
    std::ostringstream s_out;
    if (!_view)
        throw Py::RuntimeError("Cannot print representation of deleted object");
    s_out << "View3DInventor";
    return Py::String(s_out.str());
}

View3DInventorPy::method_varargs_handler View3DInventorPy::pycxx_handler = 0;

PyObject *View3DInventorPy::method_varargs_ext_handler(PyObject *_self_and_name_tuple, PyObject *_args)
{
    try {
        return pycxx_handler(_self_and_name_tuple, _args);
    }
    catch (const Base::Exception& e) {
        throw Py::RuntimeError(e.what());
    }
    catch (const std::exception& e) {
        throw Py::RuntimeError(e.what());
    }
    catch(...) {
        throw Py::RuntimeError("Unknown C++ exception");
    }
}

Py::Object View3DInventorPy::getattr(const char * attr)
{
    if (!_view) {
        std::string s;
        std::ostringstream s_out;
        s_out << "Cannot access attribute '" << attr << "' of deleted object";
        throw Py::RuntimeError(s_out.str());
    }
    else {
        // see if an active object has the same name
        App::DocumentObject *docObj = _view->getActiveObject<App::DocumentObject*>(attr);
        if (docObj) {
            return Py::Object(docObj->getPyObject(),true);
        }
        else {
            // else looking for a method with the name and call it
            Py::Object obj = Py::PythonExtension<View3DInventorPy>::getattr(attr);
            if (PyCFunction_Check(obj.ptr())) {
                PyCFunctionObject* op = reinterpret_cast<PyCFunctionObject*>(obj.ptr());
                if (op->m_ml->ml_flags == METH_VARARGS) {
                    if (!pycxx_handler)
                        pycxx_handler = op->m_ml->ml_meth;
                    op->m_ml->ml_meth = method_varargs_ext_handler;
                }
            }
            return obj;
        }
    }
}

int View3DInventorPy::setattr(const char * attr, const Py::Object & value)
{
    if (!_view) {
        std::string s;
        std::ostringstream s_out;
        s_out << "Cannot access attribute '" << attr << "' of deleted object";
        throw Py::RuntimeError(s_out.str());
    }
    else {
        return Py::PythonExtension<View3DInventorPy>::setattr(attr, value);
    }
}

Py::Object View3DInventorPy::message(const Py::Tuple& args)
{
    const char **ppReturn = 0;
    char *psMsgStr;
    if (!PyArg_ParseTuple(args.ptr(), "s;Message string needed (string)",&psMsgStr))     // convert args: Python->C 
        throw Py::Exception();

    try {
        _view->onMsg(psMsgStr,ppReturn);
    }
    catch (const Base::Exception& e) {
        throw Py::RuntimeError(e.what());
    }
    catch (const std::exception& e) {
        throw Py::RuntimeError(e.what());
    }
    catch(...) {
        throw Py::RuntimeError("Unknown C++ exception");
    }
    return Py::None();
}

Py::Object View3DInventorPy::fitAll(const Py::Tuple& args)
{
    double factor = 1.0;
    if (!PyArg_ParseTuple(args.ptr(), "|d", &factor))
        throw Py::Exception();

    try {
        _view->getViewer()->viewAll((float)factor);
    }
    catch (const Base::Exception& e) {
        throw Py::RuntimeError(e.what());
    }
    catch (const std::exception& e) {
        throw Py::RuntimeError(e.what());
    }
    catch(...) {
        throw Py::RuntimeError("Unknown C++ exception");
    }
    return Py::None();
}

Py::Object View3DInventorPy::boxZoom(const Py::Tuple& args, const Py::Dict& kwds)
{
    static char* kwds_box[] = {"XMin", "YMin", "XMax", "YMax", NULL};
    short xmin, ymin, xmax, ymax;
    if (!PyArg_ParseTupleAndKeywords(args.ptr(), kwds.ptr(), "hhhh", kwds_box,
                                     &xmin, &ymin, &xmax, &ymax))
        throw Py::Exception();

    SbBox2s box(xmin, ymin, xmax, ymax);
    _view->getViewer()->boxZoom(box);
    return Py::None();
}

/**
 Formulas to get quaternion for axonometric views:

 \code
from math import sqrt, degrees, asin, atan
p1=App.Rotation(App.Vector(1,0,0),90)
p2=App.Rotation(App.Vector(0,0,1),alpha)
p3=App.Rotation(p2.multVec(App.Vector(1,0,0)),beta)
p4=p3.multiply(p2).multiply(p1)

from pivy import coin
c=Gui.ActiveDocument.ActiveView.getCameraNode()
c.orientation.setValue(*p4.Q)
 \endcode

 The angles alpha and beta depend on the type of axonometry
 Isometric:
 \code
alpha=45
beta=degrees(asin(-sqrt(1.0/3.0)))
 \endcode

 Dimetric:
 \code
alpha=degrees(asin(sqrt(1.0/8.0)))
beta=degrees(-asin(1.0/3.0))
 \endcode

 Trimetric:
 \code
alpha=30.0
beta=-35.0
 \endcode

 Verification code that the axonomtries are correct:

 \code
from pivy import coin
c=Gui.ActiveDocument.ActiveView.getCameraNode()
vo=App.Vector(c.getViewVolume().getMatrix().multVecMatrix(coin.SbVec3f(0,0,0)).getValue())
vx=App.Vector(c.getViewVolume().getMatrix().multVecMatrix(coin.SbVec3f(10,0,0)).getValue())
vy=App.Vector(c.getViewVolume().getMatrix().multVecMatrix(coin.SbVec3f(0,10,0)).getValue())
vz=App.Vector(c.getViewVolume().getMatrix().multVecMatrix(coin.SbVec3f(0,0,10)).getValue())
(vx-vo).Length
(vy-vo).Length
(vz-vo).Length

# Projection
vo.z=0
vx.z=0
vy.z=0
vz.z=0

(vx-vo).Length
(vy-vo).Length
(vz-vo).Length
 \endcode

 See also:
 http://www.mathematik.uni-marburg.de/~thormae/lectures/graphics1/graphics_6_2_ger_web.html#1
 http://www.mathematik.uni-marburg.de/~thormae/lectures/graphics1/code_v2/Axonometric/qt/Axonometric.cpp
 https://de.wikipedia.org/wiki/Arkussinus_und_Arkuskosinus
*/

SbRotation Camera::rotation(Camera::Orientation view)
{
    switch (view) {
    case Top:
        return SbRotation(0, 0, 0, 1);
    case Bottom:
        return SbRotation(0, 1, 0, 0);
    case Front: {
        float root = (float)(sqrt(2.0)/2.0);
        return SbRotation(root, 0, 0, root);
    }
    case Rear: {
        float root = (float)(sqrt(2.0)/2.0);
        return SbRotation(0, root, root, 0);
    }
    case Left:
        return SbRotation(-0.5, 0.5, 0.5, -0.5);
    case Right:
        return SbRotation(0.5, 0.5, 0.5, 0.5);
    case Isometric:
        //from math import sqrt, degrees, asin
        //p1=App.Rotation(App.Vector(1,0,0),45)
        //p2=App.Rotation(App.Vector(0,0,1),-45)
        //p3=p2.multiply(p1)
        //return SbRotation(0.353553f, -0.146447f, -0.353553f, 0.853553f);

        //from math import sqrt, degrees, asin
        //p1=App.Rotation(App.Vector(1,0,0),90)
        //p2=App.Rotation(App.Vector(0,0,1),135)
        //p3=App.Rotation(App.Vector(-1,1,0),degrees(asin(-sqrt(1.0/3.0))))
        //p4=p3.multiply(p2).multiply(p1)
        //return SbRotation(0.17592, 0.424708, 0.820473, 0.339851);

        //from math import sqrt, degrees, asin
        //p1=App.Rotation(App.Vector(1,0,0),90)
        //p2=App.Rotation(App.Vector(0,0,1),45)
        //#p3=App.Rotation(App.Vector(1,1,0),45)
        //p3=App.Rotation(App.Vector(1,1,0),degrees(asin(-sqrt(1.0/3.0))))
        //p4=p3.multiply(p2).multiply(p1)
        return SbRotation(0.424708f, 0.17592f, 0.339851f, 0.820473f);
    case Dimetric:
        return SbRotation(0.567952f, 0.103751f, 0.146726f, 0.803205f);
    case Trimetric:
        return SbRotation(0.446015f, 0.119509f, 0.229575f, 0.856787f);
    default:
        return SbRotation(0, 0, 0, 1);
    }
}

Py::Object View3DInventorPy::viewBottom(const Py::Tuple& args)
{
    if (!PyArg_ParseTuple(args.ptr(), ""))
        throw Py::Exception();

    try {
        _view->getViewer()->setCameraOrientation(Camera::rotation(Camera::Bottom));
    }
    catch (const Base::Exception& e) {
        throw Py::RuntimeError(e.what());
    }
    catch (const std::exception& e) {
        throw Py::RuntimeError(e.what());
    }
    catch(...) {
        throw Py::RuntimeError("Unknown C++ exception");
    }

    return Py::None();
}

Py::Object View3DInventorPy::viewFront(const Py::Tuple& args)
{
    if (!PyArg_ParseTuple(args.ptr(), ""))
        throw Py::Exception();

    try {
        _view->getViewer()->setCameraOrientation(Camera::rotation(Camera::Front));
    }
    catch (const Base::Exception& e) {
        throw Py::RuntimeError(e.what());
    }
    catch (const std::exception& e) {
        throw Py::RuntimeError(e.what());
    }
    catch(...) {
        throw Py::RuntimeError("Unknown C++ exception");
    }

    return Py::None();
}

Py::Object View3DInventorPy::viewLeft(const Py::Tuple& args)
{
    if (!PyArg_ParseTuple(args.ptr(), ""))
        throw Py::Exception();

    try {
        _view->getViewer()->setCameraOrientation(Camera::rotation(Camera::Left));
    }
    catch (const Base::Exception& e) {
        throw Py::RuntimeError(e.what());
    }
    catch (const std::exception& e) {
        throw Py::RuntimeError(e.what());
    }
    catch(...) {
        throw Py::RuntimeError("Unknown C++ exception");
    }

    return Py::None();
}

Py::Object View3DInventorPy::viewRear(const Py::Tuple& args)
{
    if (!PyArg_ParseTuple(args.ptr(), ""))
        throw Py::Exception();

    try {
        _view->getViewer()->setCameraOrientation(Camera::rotation(Camera::Rear));
    }
    catch (const Base::Exception& e) {
        throw Py::RuntimeError(e.what());
    }
    catch (const std::exception& e) {
        throw Py::RuntimeError(e.what());
    }
    catch(...) {
        throw Py::RuntimeError("Unknown C++ exception");
    }

    return Py::None();
}

Py::Object View3DInventorPy::viewRight(const Py::Tuple& args)
{
    if (!PyArg_ParseTuple(args.ptr(), ""))
        throw Py::Exception();

    try {
        _view->getViewer()->setCameraOrientation(Camera::rotation(Camera::Right));
    }
    catch (const Base::Exception& e) {
        throw Py::RuntimeError(e.what());
    }
    catch (const std::exception& e) {
        throw Py::RuntimeError(e.what());
    }
    catch(...) {
        throw Py::RuntimeError("Unknown C++ exception");
    }

    return Py::None();
}

Py::Object View3DInventorPy::viewTop(const Py::Tuple& args)
{
    if (!PyArg_ParseTuple(args.ptr(), ""))
        throw Py::Exception();

    try {
        _view->getViewer()->setCameraOrientation(Camera::rotation(Camera::Top));
    }
    catch (const Base::Exception& e) {
        throw Py::RuntimeError(e.what());
    }
    catch (const std::exception& e) {
        throw Py::RuntimeError(e.what());
    }
    catch(...) {
        throw Py::RuntimeError("Unknown C++ exception");
    }

    return Py::None();
}

Py::Object View3DInventorPy::viewIsometric(const Py::Tuple& args)
{
    if (!PyArg_ParseTuple(args.ptr(), ""))
        throw Py::Exception();

    try {
        _view->getViewer()->setCameraOrientation(Camera::rotation(Camera::Isometric));
    }
    catch (const Base::Exception& e) {
        throw Py::RuntimeError(e.what());
    }
    catch (const std::exception& e) {
        throw Py::RuntimeError(e.what());
    }
    catch(...) {
        throw Py::RuntimeError("Unknown C++ exception");
    }

    return Py::None();
}

Py::Object View3DInventorPy::viewDimetric(const Py::Tuple& args)
{
    if (!PyArg_ParseTuple(args.ptr(), ""))
        throw Py::Exception();

    try {
        _view->getViewer()->setCameraOrientation(Camera::rotation(Camera::Dimetric));
    }
    catch (const Base::Exception& e) {
        throw Py::RuntimeError(e.what());
    }
    catch (const std::exception& e) {
        throw Py::RuntimeError(e.what());
    }
    catch(...) {
        throw Py::RuntimeError("Unknown C++ exception");
    }

    return Py::None();
}

Py::Object View3DInventorPy::viewTrimetric(const Py::Tuple& args)
{
    if (!PyArg_ParseTuple(args.ptr(), ""))
        throw Py::Exception();

    try {
        _view->getViewer()->setCameraOrientation(Camera::rotation(Camera::Trimetric));
    }
    catch (const Base::Exception& e) {
        throw Py::RuntimeError(e.what());
    }
    catch (const std::exception& e) {
        throw Py::RuntimeError(e.what());
    }
    catch(...) {
        throw Py::RuntimeError("Unknown C++ exception");
    }

    return Py::None();
}

Py::Object View3DInventorPy::viewDefaultOrientation(const Py::Tuple& args)
{
    char* view = nullptr;
    if (!PyArg_ParseTuple(args.ptr(), "|s", &view))
        throw Py::Exception();

    try {
        std::string newDocView;
        SbRotation rot(0,0,0,1);
        if (view) {
            newDocView = view;
        }
        else {
            ParameterGrp::handle hGrp = App::GetApplication().GetParameterGroupByPath("User parameter:BaseApp/Preferences/View");
            newDocView = hGrp->GetASCII("NewDocumentCameraOrientation", "Top");
        }

        if (newDocView == "Top") {
            rot = Camera::rotation(Camera::Top);
        }
        else if (newDocView == "Bottom") {
            rot = Camera::rotation(Camera::Bottom);
        }
        else if (newDocView == "Front") {
            rot = Camera::rotation(Camera::Front);
        }
        else if (newDocView == "Rear") {
            rot = Camera::rotation(Camera::Rear);
        }
        else if (newDocView == "Left") {
            rot = Camera::rotation(Camera::Left);
        }
        else if (newDocView == "Right") {
            rot = Camera::rotation(Camera::Right);
        }
        else if (newDocView == "Isometric") {
            rot = Camera::rotation(Camera::Isometric);
        }
        else if (newDocView == "Dimetric") {
            rot = Camera::rotation(Camera::Dimetric);
        }
        else if (newDocView == "Trimetric") {
            rot = Camera::rotation(Camera::Trimetric);
        }

        _view->getViewer()->setCameraOrientation(rot);
    }
    catch (const Base::Exception& e) {
        throw Py::RuntimeError(e.what());
    }
    catch (const std::exception& e) {
        throw Py::RuntimeError(e.what());
    }
    catch(...) {
        throw Py::RuntimeError("Unknown C++ exception");
    }

    return Py::None();
}

Py::Object View3DInventorPy::viewRotateLeft(const Py::Tuple& args)
{
    if (!PyArg_ParseTuple(args.ptr(), ""))
        throw Py::Exception();

    try {
      SoCamera* cam = _view->getViewer()->getSoRenderManager()->getCamera();
      SbRotation rot = cam->orientation.getValue();
      SbVec3f vdir(0, 0, -1);
      rot.multVec(vdir, vdir);
      SbRotation nrot(vdir, (float)M_PI/2);
      cam->orientation.setValue(rot*nrot);
    }
    catch (const Base::Exception& e) {
        throw Py::RuntimeError(e.what());
    }
    catch (const std::exception& e) {
        throw Py::RuntimeError(e.what());
    }
    catch(...) {
        throw Py::RuntimeError("Unknown C++ exception");
    }

    return Py::None();
}

Py::Object View3DInventorPy::viewRotateRight(const Py::Tuple& args)
{
    if (!PyArg_ParseTuple(args.ptr(), ""))
        throw Py::Exception();

    try {
      SoCamera* cam = _view->getViewer()->getSoRenderManager()->getCamera();
      SbRotation rot = cam->orientation.getValue();
      SbVec3f vdir(0, 0, -1);
      rot.multVec(vdir, vdir);
      SbRotation nrot(vdir, (float)-M_PI/2);
      cam->orientation.setValue(rot*nrot);
    }
    catch (const Base::Exception& e) {
        throw Py::RuntimeError(e.what());
    }
    catch (const std::exception& e) {
        throw Py::RuntimeError(e.what());
    }
    catch(...) {
        throw Py::RuntimeError("Unknown C++ exception");
    }

    return Py::None();
}

Py::Object View3DInventorPy::zoomIn(const Py::Tuple& args)
{
    if (!PyArg_ParseTuple(args.ptr(), ""))
        throw Py::Exception();

    try {
        _view->getViewer()->navigationStyle()->zoomIn();
    }
    catch (const Base::Exception& e) {
        throw Py::RuntimeError(e.what());
    }
    catch (const std::exception& e) {
        throw Py::RuntimeError(e.what());
    }
    catch(...) {
        throw Py::RuntimeError("Unknown C++ exception");
    }

    return Py::None();
}

Py::Object View3DInventorPy::zoomOut(const Py::Tuple& args)
{
    if (!PyArg_ParseTuple(args.ptr(), ""))
        throw Py::Exception();

    try {
        _view->getViewer()->navigationStyle()->zoomOut();
    }
    catch (const Base::Exception& e) {
        throw Py::RuntimeError(e.what());
    }
    catch (const std::exception& e) {
        throw Py::RuntimeError(e.what());
    }
    catch(...) {
        throw Py::RuntimeError("Unknown C++ exception");
    }

    return Py::None();
}

Py::Object View3DInventorPy::setCameraOrientation(const Py::Tuple& args)
{
    PyObject* o;
    PyObject* m=Py_False;
    if (!PyArg_ParseTuple(args.ptr(), "O|O!", &o, &PyBool_Type, &m))
        throw Py::Exception();

    try {
        if (PyTuple_Check(o)) {
            Py::Tuple tuple(o);
            float q0 = (float)Py::Float(tuple[0]);
            float q1 = (float)Py::Float(tuple[1]);
            float q2 = (float)Py::Float(tuple[2]);
            float q3 = (float)Py::Float(tuple[3]);
            _view->getViewer()->setCameraOrientation(SbRotation(q0, q1, q2, q3), PyObject_IsTrue(m));
        }
        else if (PyObject_TypeCheck(o, &Base::RotationPy::Type)) {
            Base::Rotation r = (Base::Rotation)Py::Rotation(o,false);
            double q0, q1, q2, q3;
            r.getValue(q0, q1, q2, q3);
            _view->getViewer()->setCameraOrientation(SbRotation((float)q0, (float)q1, (float)q2, (float)q3), PyObject_IsTrue(m));
        }
        else {
            throw Py::ValueError("Neither tuple nor rotation object");
        }
    }
    catch (const Py::Exception&) {
        throw; // re-throw
    }
    catch (const Base::Exception& e) {
        throw Py::RuntimeError(e.what());
    }
    catch (const std::exception& e) {
        throw Py::RuntimeError(e.what());
    }
    catch(...) {
        throw Py::RuntimeError("Unknown C++ exception");
    }

    return Py::None();
}

Py::Object View3DInventorPy::getCameraOrientation(const Py::Tuple& args)
{
    if (!PyArg_ParseTuple(args.ptr(), ""))
        throw Py::Exception();
    SbRotation rot = _view->getViewer()->getCameraOrientation();
    float q0,q1,q2,q3;
    rot.getValue(q0,q1,q2,q3);
    return Py::Rotation(Base::Rotation(q0,q1,q2,q3));
}

Py::Object View3DInventorPy::viewPosition(const Py::Tuple& args)
{
    PyObject* p=0;
    int steps = 20;
    int ms = 30;
    if (!PyArg_ParseTuple(args.ptr(), "|O!ii",&Base::PlacementPy::Type,&p,&steps,&ms))
        throw Py::Exception();

    if (p) {
        Base::Placement* plm = static_cast<Base::PlacementPy*>(p)->getPlacementPtr();
        Base::Rotation rot = plm->getRotation();
        Base::Vector3d pos = plm->getPosition();
        double q0,q1,q2,q3;
        rot.getValue(q0,q1,q2,q3);
        _view->getViewer()->moveCameraTo(
            SbRotation((float)q0, (float)q1, (float)q2, (float)q3),
            SbVec3f((float)pos.x, (float)pos.y, (float)pos.z), steps, ms);
    }

    SoCamera* cam = _view->getViewer()->getSoRenderManager()->getCamera();
    if (!cam) return Py::None();

    SbRotation rot = cam->orientation.getValue();
    SbVec3f pos = cam->position.getValue();
    float q0,q1,q2,q3;
    rot.getValue(q0,q1,q2,q3);
    Base::Placement plm(
        Base::Vector3d(pos[0], pos[1], pos[2]),
        Base::Rotation(q0, q1, q2, q3));
    return Py::Placement(plm);
}

Py::Object View3DInventorPy::startAnimating(const Py::Tuple& args)
{
    float x,y,z;
    float velocity;
    if (!PyArg_ParseTuple(args.ptr(), "ffff", &x,&y,&z,&velocity))
        throw Py::Exception();
    _view->getViewer()->startAnimating(SbVec3f(x,y,z),velocity);
    return Py::None();
}

Py::Object View3DInventorPy::stopAnimating(const Py::Tuple& args)
{
    if (!PyArg_ParseTuple(args.ptr(), ""))
        throw Py::Exception();
    _view->getViewer()->stopAnimating();
    return Py::None();
}

Py::Object View3DInventorPy::setAnimationEnabled(const Py::Tuple& args)
{
    int ok;
    if (!PyArg_ParseTuple(args.ptr(), "i", &ok))
        throw Py::Exception();
    _view->getViewer()->setAnimationEnabled(ok!=0);
    return Py::None();
}

Py::Object View3DInventorPy::isAnimationEnabled(const Py::Tuple& args)
{
    if (!PyArg_ParseTuple(args.ptr(), ""))
        throw Py::Exception();
    SbBool ok = _view->getViewer()->isAnimationEnabled();
    return Py::Boolean(ok ? true : false);
}

Py::Object View3DInventorPy::saveImage(const Py::Tuple& args)
{
    char *cFileName,*cColor="Current",*cComment="$MIBA";
    int w=-1,h=-1;

    if (!PyArg_ParseTuple(args.ptr(), "et|iiss","utf-8",&cFileName,&w,&h,&cColor,&cComment))
        throw Py::Exception();

    std::string encodedName = std::string(cFileName);
    PyMem_Free(cFileName);
    QFileInfo fi(QString::fromUtf8(encodedName.c_str()));

    if (!fi.absoluteDir().exists())
        throw Py::RuntimeError("Directory where to save image doesn't exist");

    QColor bg;
    QString colname = QString::fromLatin1(cColor);
    if (colname.compare(QLatin1String("Current"), Qt::CaseInsensitive) == 0)
        bg = QColor(); // assign an invalid color here
    else
        bg.setNamedColor(colname);

    QImage img;
    _view->getViewer()->savePicture(w, h, 8, bg, img);

    SoFCOffscreenRenderer& renderer = SoFCOffscreenRenderer::instance();
    SoCamera* cam = _view->getViewer()->getSoRenderManager()->getCamera();
    renderer.writeToImageFile(encodedName.c_str(), cComment, cam->getViewVolume().getMatrix(), img);

    return Py::None();
}

Py::Object View3DInventorPy::saveVectorGraphic(const Py::Tuple& args)
{
    char* filename;
    int ps=4;
    char* name="white";

    if (!PyArg_ParseTuple(args.ptr(), "s|is",&filename,&ps,&name))
        throw Py::Exception();

    std::unique_ptr<SoVectorizeAction> vo;
    Base::FileInfo fi(filename);
    if (fi.hasExtension("ps") || fi.hasExtension("eps")) {
        vo = std::unique_ptr<SoVectorizeAction>(new SoVectorizePSAction());
        //vo->setGouraudThreshold(0.0f);
    }
    else if (fi.hasExtension("svg")) {
        vo = std::unique_ptr<SoVectorizeAction>(new SoFCVectorizeSVGAction());
    }
    else if (fi.hasExtension("idtf")) {
        vo = std::unique_ptr<SoVectorizeAction>(new SoFCVectorizeU3DAction());
    }
    else {
        throw Py::RuntimeError("Not supported vector graphic");
    }

    SoVectorOutput * out = vo->getOutput();
    if (!out || !out->openFile(filename)) {
        std::ostringstream a_out;
        a_out << "Cannot open file '" << filename << "'";
        throw Py::RuntimeError(a_out.str());
    }

    QColor bg;
    QString colname = QString::fromLatin1(name);
    if (colname.compare(QLatin1String("Current"), Qt::CaseInsensitive) == 0)
        bg = _view->getViewer()->backgroundColor();
    else
        bg.setNamedColor(colname);

    _view->getViewer()->saveGraphic(ps,bg,vo.get());
    out->closeFile();
    return Py::None();
}

Py::Object View3DInventorPy::getCameraNode(const Py::Tuple& args)
{
    if (!PyArg_ParseTuple(args.ptr(), ""))
        throw Py::Exception();

    try {
        SoNode* camera = _view->getViewer()->getSoRenderManager()->getCamera();
        PyObject* proxy = 0;
        std::string type;
        type = "So"; // seems that So prefix is missing in camera node
        type += camera->getTypeId().getName().getString();
        type += " *";
        proxy = Base::Interpreter().createSWIGPointerObj("pivy.coin", type.c_str(), (void*)camera, 1);
        camera->ref();
        return Py::Object(proxy, true);
    }
    catch (const Base::Exception& e) {
        throw Py::RuntimeError(e.what());
    }
}

Py::Object View3DInventorPy::getCamera(const Py::Tuple& args)
{
    if (!PyArg_ParseTuple(args.ptr(), ""))
        throw Py::Exception();

    SoOutput out;
    char buffer[512];
    out.setBuffer(buffer, 512, 0);

    try {
        SoWriteAction wa(&out);
        SoCamera * cam = _view->getViewer()->getSoRenderManager()->getCamera();
        if (cam) wa.apply(cam);
        else buffer[0] = '\0';
        return Py::String(buffer);
    }
    catch (const Base::Exception& e) {
        throw Py::RuntimeError(e.what());
    }
    catch (const std::exception& e) {
        throw Py::RuntimeError(e.what());
    }
    catch(...) {
        throw Py::RuntimeError("Unknown C++ exception");
    }
}

Py::Object View3DInventorPy::getViewDirection(const Py::Tuple& args)
{
    if (!PyArg_ParseTuple(args.ptr(), ""))
        throw Py::Exception();
    try {
        SbVec3f dvec = _view->getViewer()->getViewDirection();
        return Py::Vector(Base::Vector3f(dvec[0], dvec[1], dvec[2]));
    }
    catch (const Base::Exception& e) {
        throw Py::RuntimeError(e.what());
    }
    catch (const std::exception& e) {
        throw Py::RuntimeError(e.what());
    }
    catch(...) {
        throw Py::RuntimeError("Unknown C++ exception");
    }
}

Py::Object View3DInventorPy::setViewDirection(const Py::Tuple& args)
{
    PyObject* object;
    if (!PyArg_ParseTuple(args.ptr(), "O", &object)) 
        throw Py::Exception(); 

    try {
        if (PyTuple_Check(object)) {
            Py::Tuple tuple(object);
            Py::Float x(tuple.getItem(0));
            Py::Float y(tuple.getItem(1));
            Py::Float z(tuple.getItem(2));
            SbVec3f dir;
            dir.setValue((float)x, (float)y, (float)z);
            if (dir.length() < 0.001f)
                throw Py::ValueError("Null vector cannot be used to set direction");
            _view->getViewer()->setViewDirection(dir);
            return Py::None();
        }
    }
    catch (const Py::Exception&) {
        throw; // re-throw
    }
    catch (const Base::Exception& e) {
        throw Py::RuntimeError(e.what());
    }
    catch (const std::exception& e) {
        throw Py::RuntimeError(e.what());
    }
    catch(...) {
        throw Py::RuntimeError("Unknown C++ exception");
    }

    return Py::None();

}


Py::Object View3DInventorPy::setCamera(const Py::Tuple& args)
{
    char* buffer;
    if (!PyArg_ParseTuple(args.ptr(), "s", &buffer))
        throw Py::Exception();

    try {
        _view->setCamera(buffer);
        return Py::None();
    }
    catch (const Base::Exception& e) {
        throw Py::RuntimeError(e.what());
    }
    catch (const std::exception& e) {
        throw Py::RuntimeError(e.what());
    }
    catch(...) {
        throw Py::RuntimeError("Unknown C++ exception");
    }
}

//FIXME: Once View3DInventor inherits from PropertyContainer we can use PropertyEnumeration.
const char* CameraTypeEnums[]= {"Orthographic","Perspective",NULL};

Py::Object View3DInventorPy::getCameraType(const Py::Tuple& args)
{
    if (!PyArg_ParseTuple(args.ptr(), ""))
        throw Py::Exception();

    SoCamera* cam = _view->getViewer()->getSoRenderManager()->getCamera();
    if (!cam) {
        throw Py::RuntimeError("No camera set!");
    }
    else if (cam->getTypeId() == SoOrthographicCamera::getClassTypeId()) {
        return Py::String(CameraTypeEnums[0]);
    }
    else if (cam->getTypeId() == SoPerspectiveCamera::getClassTypeId()) {
        return Py::String(CameraTypeEnums[1]);
    }
    else {
        throw Py::TypeError("Unknown camera type");
    }
}

Py::Object View3DInventorPy::setCameraType(const Py::Tuple& args)
{
    int cameratype=-1;
    if (!PyArg_ParseTuple(args.ptr(), "i", &cameratype)) {    // convert args: Python->C 
        char* modename;
        PyErr_Clear();
        if (!PyArg_ParseTuple(args.ptr(), "s", &modename))
            throw Py::Exception();
        for (int i=0; i<2; i++ ) {
            if (strncmp(CameraTypeEnums[i],modename,20) == 0 ) {
                cameratype = i;
                break;
            }
        }

        if (cameratype < 0) {
            std::string s;
            std::ostringstream s_out;
            s_out << "Unknown camera type '" << modename << "'";
            throw Py::NameError(s_out.str());
        }
    }

    if (cameratype < 0 || cameratype > 1)
        throw Py::IndexError("Out of range");
    if (cameratype==0)
        _view->getViewer()->setCameraType(SoOrthographicCamera::getClassTypeId());
    else
        _view->getViewer()->setCameraType(SoPerspectiveCamera::getClassTypeId());
    return Py::None();
}

Py::Object View3DInventorPy::listCameraTypes(const Py::Tuple& args)
{
    if (!PyArg_ParseTuple(args.ptr(), ""))
        throw Py::Exception();

    try {
        Py::List list(2);
        for (int i=0; i<2; i++) {
            list[i] = Py::String(CameraTypeEnums[i]);
        }
        return list;
    }
    catch (const Base::Exception& e) {
        throw Py::RuntimeError(e.what());
    }
    catch (const std::exception& e) {
        throw Py::RuntimeError(e.what());
    }
    catch(...) {
        throw Py::RuntimeError("Unknown C++ exception");
    }
}

Py::Object View3DInventorPy::dump(const Py::Tuple& args)
{
    char* filename;
    if (!PyArg_ParseTuple(args.ptr(), "s", &filename))
        throw Py::Exception();

    try {
        _view->dump(filename);
        return Py::None();
    }
    catch (const Base::Exception& e) {
        throw Py::RuntimeError(e.what());
    }
    catch (const std::exception& e) {
        throw Py::RuntimeError(e.what());
    }
    catch(...) {
        throw Py::RuntimeError("Unknown C++ exception");
    }
}

Py::Object View3DInventorPy::dumpNode(const Py::Tuple& args)
{
    PyObject* object;
    if (!PyArg_ParseTuple(args.ptr(), "O", &object))     // convert args: Python->C 
        throw Py::Exception();

    void* ptr = 0;
    try {
        Base::Interpreter().convertSWIGPointerObj("pivy.coin", "SoNode *", object, &ptr, 0);
    }
    catch (const Base::Exception& e) {
        throw Py::RuntimeError(e.what());
    }
    SoNode* node = reinterpret_cast<SoNode*>(ptr);
    return Py::String(SoFCDB::writeNodesToString(node));
}

//FIXME: Once View3DInventor inherits from PropertyContainer we can use PropertyEnumeration.
const char* StereoTypeEnums[]= {"None","Anaglyph","QuadBuffer","InterleavedRows","InterleavedColumns",NULL};

Py::Object View3DInventorPy::setStereoType(const Py::Tuple& args)
{
    int stereomode=-1;
    if (!PyArg_ParseTuple(args.ptr(), "i", &stereomode)) {
        char* modename;
        PyErr_Clear();
        if (!PyArg_ParseTuple(args.ptr(), "s", &modename))
            throw Py::Exception();
        for (int i=0; i<5; i++) {
            if (strncmp(StereoTypeEnums[i],modename,20) == 0) {
                stereomode = i;
                break;
            }
        }

        if (stereomode < 0) {
            std::string s;
            std::ostringstream s_out;
            s_out << "Unknown stereo type '" << modename << "'";
            throw Py::NameError(s_out.str());
        }
    }

    try {
        if (stereomode < 0 || stereomode > 4)
            throw Py::IndexError("Out of range");
        Quarter::SoQTQuarterAdaptor::StereoMode mode = Quarter::SoQTQuarterAdaptor::StereoMode(stereomode);
        _view->getViewer()->setStereoMode(mode);
        return Py::None();
    }
    catch (const Base::Exception& e) {
        throw Py::RuntimeError(e.what());
    }
    catch (const std::exception& e) {
        throw Py::RuntimeError(e.what());
    }
    catch(...) {
        throw Py::RuntimeError("Unknown C++ exception");
    }
}

Py::Object View3DInventorPy::getStereoType(const Py::Tuple& args)
{
    if (!PyArg_ParseTuple(args.ptr(), ""))
        throw Py::Exception();

    try {
        int mode = (int)(_view->getViewer()->stereoMode());
        return Py::String(StereoTypeEnums[mode]);
    }
    catch (const Base::Exception& e) {
        throw Py::RuntimeError(e.what());
    }
    catch (const std::exception& e) {
        throw Py::RuntimeError(e.what());
    }
    catch(...) {
        throw Py::RuntimeError("Unknown C++ exception");
    }
}

Py::Object View3DInventorPy::listStereoTypes(const Py::Tuple& args)
{
    if (!PyArg_ParseTuple(args.ptr(), ""))
        throw Py::Exception();

    try {
        Py::List list(5);
        for (int i=0; i<5; i++) {
            list[i] = Py::String(StereoTypeEnums[i]);
        }

        return list;
    }
    catch (const Base::Exception& e) {
        throw Py::RuntimeError(e.what());
    }
    catch (const std::exception& e) {
        throw Py::RuntimeError(e.what());
    }
    catch(...) {
        throw Py::RuntimeError("Unknown C++ exception");
    }
}

Py::Object View3DInventorPy::getCursorPos(const Py::Tuple& args)
{
    if (!PyArg_ParseTuple(args.ptr(), ""))
        throw Py::Exception();
    try {
        QPoint pos = _view->mapFromGlobal(QCursor::pos());
        Py::Tuple tuple(2);
        tuple.setItem(0, Py::Int(pos.x()));
        tuple.setItem(1, Py::Int(_view->height()-pos.y()-1));
        return tuple;
    }
    catch (const Py::Exception&) {
        throw;
    }
}

Py::Object View3DInventorPy::getObjectInfo(const Py::Tuple& args)
{
    PyObject* object;
    float r = _view->getViewer()->getPickRadius();
    if (!PyArg_ParseTuple(args.ptr(), "O|f", &object, &r))
        throw Py::Exception();

    try {
        //Note: For gcc (4.2) we need the 'const' keyword to avoid the compiler error:
        //conversion from 'Py::seqref<Py::Object>' to non-scalar type 'Py::Int' requested
        //We should report this problem to the PyCXX project as in the documentation an 
        //example without the 'const' keyword is used.
        //Or we can also write Py::Int x(tuple[0]);
        const Py::Tuple tuple(object);
        Py::Int x(tuple[0]);
        Py::Int y(tuple[1]);

        // As this method could be called during a SoHandleEventAction scene
        // graph traversal we must not use a second SoHandleEventAction as
        // we will get Coin warnings because of multiple scene graph traversals
        // which is regarded as error-prone.
        SoRayPickAction action(_view->getViewer()->getSoRenderManager()->getViewportRegion());
        action.setPoint(SbVec2s((long)x,(long)y));
        action.setRadius(r);
        action.apply(_view->getViewer()->getSoRenderManager()->getSceneGraph());
        SoPickedPoint *Point = action.getPickedPoint();

        Py::Object ret = Py::None();
        if (Point) {
            Py::Dict dict;
            SbVec3f pt = Point->getPoint();
            dict.setItem("x", Py::Float(pt[0]));
            dict.setItem("y", Py::Float(pt[1]));
            dict.setItem("z", Py::Float(pt[2]));

            Gui::Document* doc = _view->getViewer()->getDocument();
            ViewProvider *vp = doc ? doc->getViewProviderByPathFromTail(Point->getPath())
                    : _view->getViewer()->getViewProviderByPathFromTail(Point->getPath());
            if (vp && vp->isDerivedFrom(ViewProviderDocumentObject::getClassTypeId())) {
                ViewProviderDocumentObject* vpd = static_cast<ViewProviderDocumentObject*>(vp);
                dict.setItem("Document",
                    Py::String(vpd->getObject()->getDocument()->getName()));
                dict.setItem("Object",
                    Py::String(vpd->getObject()->getNameInDocument()));
                if (vp->useNewSelectionModel()) {
                    dict.setItem("Component",
                        Py::String(vpd->getElement(Point->getDetail())));
                }
                else {
                    // search for a SoFCSelection node
                    SoFCDocumentObjectAction objaction;
                    objaction.apply(Point->getPath());
                    if (objaction.isHandled()) {
                        dict.setItem("Component",
                            Py::String(objaction.componentName.getString()));
                    }
                }

                // ok, found the node of interest
                ret = dict;
            }
            else {
                // custom nodes not in a VP: search for a SoFCSelection node
                SoFCDocumentObjectAction objaction;
                objaction.apply(Point->getPath());
                if (objaction.isHandled()) {
                    dict.setItem("Document",
                        Py::String(objaction.documentName.getString()));
                    dict.setItem("Object",
                        Py::String(objaction.objectName.getString()));
                    dict.setItem("Component",
                        Py::String(objaction.componentName.getString()));
                    // ok, found the node of interest
                    ret = dict;
                }
            }
        }

        return ret;
    }
    catch (const Py::Exception&) {
        throw;
    }
}

Py::Object View3DInventorPy::getObjectsInfo(const Py::Tuple& args)
{
    PyObject* object;
    float r = _view->getViewer()->getPickRadius();
    if (!PyArg_ParseTuple(args.ptr(), "O|f", &object, &r))
        throw Py::Exception();

    try {
        //Note: For gcc (4.2) we need the 'const' keyword to avoid the compiler error:
        //conversion from 'Py::seqref<Py::Object>' to non-scalar type 'Py::Int' requested
        //We should report this problem to the PyCXX project as in the documentation an 
        //example without the 'const' keyword is used.
        //Or we can also write Py::Int x(tuple[0]);
        const Py::Tuple tuple(object);
        Py::Int x(tuple[0]);
        Py::Int y(tuple[1]);

        // As this method could be called during a SoHandleEventAction scene
        // graph traversal we must not use a second SoHandleEventAction as
        // we will get Coin warnings because of multiple scene graph traversals
        // which is regarded as error-prone.
        SoRayPickAction action(_view->getViewer()->getSoRenderManager()->getViewportRegion());
        action.setPickAll(true);
        action.setRadius(r);
        action.setPoint(SbVec2s((long)x,(long)y));
        action.apply(_view->getViewer()->getSoRenderManager()->getSceneGraph());
        const SoPickedPointList& pp = action.getPickedPointList();

        Gui::Document* doc = _view->getViewer()->getDocument();
        Py::Object ret = Py::None();
        if (pp.getLength() > 0) {
            Py::List list;
            for (int i=0; i<pp.getLength(); i++) {
                Py::Dict dict;
                SoPickedPoint* point = static_cast<SoPickedPoint*>(pp.get(i));
                SbVec3f pt = point->getPoint();
                dict.setItem("x", Py::Float(pt[0]));
                dict.setItem("y", Py::Float(pt[1]));
                dict.setItem("z", Py::Float(pt[2]));

                ViewProvider *vp = doc ? doc->getViewProviderByPathFromTail(point->getPath())
                        : _view->getViewer()->getViewProviderByPathFromTail(point->getPath());
                if (vp && vp->isDerivedFrom(ViewProviderDocumentObject::getClassTypeId())) {
                    ViewProviderDocumentObject* vpd = static_cast<ViewProviderDocumentObject*>(vp);
                    dict.setItem("Document",
                        Py::String(vpd->getObject()->getDocument()->getName()));
                    dict.setItem("Object",
                        Py::String(vpd->getObject()->getNameInDocument()));
                    if (vp->useNewSelectionModel()) {
                        dict.setItem("Component",
                            Py::String(vpd->getElement(point->getDetail())));
                    }
                    else {
                        // search for a SoFCSelection node
                        SoFCDocumentObjectAction objaction;
                        objaction.apply(point->getPath());
                        if (objaction.isHandled()) {
                            dict.setItem("Component",
                                Py::String(objaction.componentName.getString()));
                        }
                    }
                    // ok, found the node of interest
                    list.append(dict);
                }
                else {
                    // custom nodes not in a VP: search for a SoFCSelection node
                    SoFCDocumentObjectAction objaction;
                    objaction.apply(point->getPath());
                    if (objaction.isHandled()) {
                        dict.setItem("Document",
                            Py::String(objaction.documentName.getString()));
                        dict.setItem("Object",
                            Py::String(objaction.objectName.getString()));
                        dict.setItem("Component",
                            Py::String(objaction.componentName.getString()));
                        // ok, found the node of interest
                        ret = dict;
                    }
                }
            }

            ret = list;
        }

        return ret;
    }
    catch (const Py::Exception&) {
        throw;
    }
}

Py::Object View3DInventorPy::getSize(const Py::Tuple& args)
{
    if (!PyArg_ParseTuple(args.ptr(), ""))
        throw Py::Exception();
    try {
        SbVec2s size = _view->getViewer()->getSoRenderManager()->getSize();
        Py::Tuple tuple(2);
        tuple.setItem(0, Py::Int(size[0]));
        tuple.setItem(1, Py::Int(size[1]));
        return tuple;
    }
    catch (const Py::Exception&) {
        throw;
    }
}

Py::Object View3DInventorPy::getPoint(const Py::Tuple& args)
{
    short x,y;
    if (!PyArg_ParseTuple(args.ptr(), "hh", &x, &y)) {
        PyErr_Clear();
        Py::Tuple t(args[0]);
        x = (int)Py::Int(t[0]);
        y = (int)Py::Int(t[1]);
    }
    try {
        SbVec3f pt = _view->getViewer()->getPointOnScreen(SbVec2s(x,y));
        return Py::Vector(Base::Vector3f(pt[0], pt[1], pt[2]));
    }
    catch (const Base::Exception& e) {
        throw Py::RuntimeError(e.what());
    }
    catch (const Py::Exception&) {
        throw;
    }
}

Py::Object View3DInventorPy::getPointOnScreen(const Py::Tuple& args)
{
    PyObject* v;
    double vx,vy,vz;
    if (PyArg_ParseTuple(args.ptr(), "O!", &Base::VectorPy::Type, &v)) {
        Base::Vector3d* vec = static_cast<Base::VectorPy*>(v)->getVectorPtr();
        vx = vec->x;
        vy = vec->y;
        vz = vec->z;
    }
    else {
        PyErr_Clear();
        if (!PyArg_ParseTuple(args.ptr(), "ddd", &vx,&vy,&vz)) {
            throw Py::TypeError("Wrong argument, Vector or three floats expected expected");
        }
    }

    try {
        const SbViewportRegion& vp = _view->getViewer()->getSoRenderManager()->getViewportRegion();
        float fRatio = vp.getViewportAspectRatio();
        const SbVec2s& sp = vp.getViewportSizePixels();
        //float dX, dY; vp.getViewportSize().getValue(dX, dY);
        SbViewVolume vv = _view->getViewer()->getSoRenderManager()->getCamera()->getViewVolume(fRatio);

        SbVec3f pt(vx,vy,vz);
        vv.projectToScreen(pt, pt);

        //if (fRatio > 1.0f) {
        //    pt[0] = (pt[0] - 0.5f*dX) / fRatio + 0.5f*dX;
        //}
        //else {
        //    pt[1] = (pt[1] - 0.5f*dY) * fRatio + 0.5f*dY;
        //}

        int x = pt[0] * sp[0];
        int y = pt[1] * sp[1];
        Py::Tuple tuple(2);
        tuple.setItem(0, Py::Int(x));
        tuple.setItem(1, Py::Int(y));

        return tuple;
    }
    catch (const Base::Exception& e) {
        throw Py::RuntimeError(e.what());
    }
    catch (const Py::Exception&) {
        throw;
    }
}

Py::Object View3DInventorPy::listNavigationTypes(const Py::Tuple&)
{
    std::vector<Base::Type> types;
    Py::List styles;
    Base::Type::getAllDerivedFrom(UserNavigationStyle::getClassTypeId(), types);
    for (std::vector<Base::Type>::iterator it = types.begin()+1; it != types.end(); ++it) {
        styles.append(Py::String(it->getName()));
    }
    return styles;
}

Py::Object View3DInventorPy::getNavigationType(const Py::Tuple&)
{
    std::string name = _view->getViewer()->navigationStyle()->getTypeId().getName();
    return Py::String(name);
}

Py::Object View3DInventorPy::setNavigationType(const Py::Tuple& args)
{
    char* style;
    if (!PyArg_ParseTuple(args.ptr(), "s", &style))
        throw Py::Exception();
    Base::Type type = Base::Type::fromName(style);
    _view->getViewer()->setNavigationType(type);
    return Py::None();
}

void View3DInventorPy::eventCallback(void * ud, SoEventCallback * n)
{
    Base::PyGILStateLocker lock;
    try {
        Py::Dict dict;
        const SoEvent* e = n->getEvent();
        if (!e) return; // invalid event
        // Type
        dict.setItem("Type", Py::String(std::string(e->getTypeId().getName().getString())));
        // Time
        dict.setItem("Time", Py::String(std::string(e->getTime().formatDate().getString())));
        SbVec2s p = n->getEvent()->getPosition();
        Py::Tuple pos(2);
        pos.setItem(0, Py::Int(p[0]));
        pos.setItem(1, Py::Int(p[1]));
        // Position
        dict.setItem("Position", pos);
        // Shift, Ctrl, Alt down
        dict.setItem("ShiftDown", Py::Object((e->wasShiftDown() ? Py_True : Py_False)));
        dict.setItem("CtrlDown",  Py::Object((e->wasCtrlDown()  ? Py_True : Py_False)));
        dict.setItem("AltDown",   Py::Object((e->wasAltDown()   ? Py_True : Py_False)));
        if (e->isOfType(SoButtonEvent::getClassTypeId())) {
            std::string state;
            const SoButtonEvent* be = static_cast<const SoButtonEvent*>(e);
            switch (be->getState()) {
                case SoButtonEvent::UP:
                    state = "UP";
                    break;
                case SoButtonEvent::DOWN:
                    state = "DOWN";
                    break;
                default:
                    state = "UNKNOWN";
                    break;
            }

            dict.setItem("State", Py::String(state));
        }
        if (e->isOfType(SoKeyboardEvent::getClassTypeId())) {
            const SoKeyboardEvent* ke = static_cast<const SoKeyboardEvent*>(e);
            switch (ke->getKey()) {
                case SoKeyboardEvent::ANY:
                    dict.setItem("Key", Py::String("ANY"));
                    break;
                case SoKeyboardEvent::UNDEFINED:
                    dict.setItem("Key", Py::String("UNDEFINED"));
                    break;
                case SoKeyboardEvent::LEFT_SHIFT:
                case SoKeyboardEvent::RIGHT_SHIFT:
                    dict.setItem("Key", Py::String("SHIFT"));
                    break;
                case SoKeyboardEvent::LEFT_CONTROL:
                case SoKeyboardEvent::RIGHT_CONTROL:
                    dict.setItem("Key", Py::String("CONTROL"));
                    break;
                case SoKeyboardEvent::LEFT_ALT:
                case SoKeyboardEvent::RIGHT_ALT:
                    dict.setItem("Key", Py::String("ALT"));
                    break;
                case SoKeyboardEvent::HOME:
                    dict.setItem("Key", Py::String("HOME"));
                    break;
                case SoKeyboardEvent::LEFT_ARROW:
                    dict.setItem("Key", Py::String("LEFT_ARROW"));
                    break;
                case SoKeyboardEvent::UP_ARROW:
                    dict.setItem("Key", Py::String("UP_ARROW"));
                    break;
                case SoKeyboardEvent::RIGHT_ARROW:
                    dict.setItem("Key", Py::String("RIGHT_ARROW"));
                    break;
                case SoKeyboardEvent::DOWN_ARROW:
                    dict.setItem("Key", Py::String("DOWN_ARROW"));
                    break;
                case SoKeyboardEvent::PAGE_UP:
                    dict.setItem("Key", Py::String("PAGE_UP"));
                    break;
                case SoKeyboardEvent::PAGE_DOWN:
                    dict.setItem("Key", Py::String("PAGE_DOWN"));
                    break;
                case SoKeyboardEvent::END:
                    dict.setItem("Key", Py::String("END"));
                    break;
                case SoKeyboardEvent::PAD_ENTER:
                    dict.setItem("Key", Py::String("PAD_ENTER"));
                    break;
                case SoKeyboardEvent::PAD_F1:
                    dict.setItem("Key", Py::String("PAD_F1"));
                    break;
                case SoKeyboardEvent::PAD_F2:
                    dict.setItem("Key", Py::String("PAD_F2"));
                    break;
                case SoKeyboardEvent::PAD_F3:
                    dict.setItem("Key", Py::String("PAD_F3"));
                    break;
                case SoKeyboardEvent::PAD_F4:
                    dict.setItem("Key", Py::String("PAD_F4"));
                    break;
                case SoKeyboardEvent::PAD_0:
                    dict.setItem("Key", Py::String("PAD_0"));
                    break;
                case SoKeyboardEvent::PAD_1:
                    dict.setItem("Key", Py::String("PAD_1"));
                    break;
                case SoKeyboardEvent::PAD_2:
                    dict.setItem("Key", Py::String("PAD_2"));
                    break;
                case SoKeyboardEvent::PAD_3:
                    dict.setItem("Key", Py::String("PAD_3"));
                    break;
                case SoKeyboardEvent::PAD_4:
                    dict.setItem("Key", Py::String("PAD_4"));
                    break;
                case SoKeyboardEvent::PAD_5:
                    dict.setItem("Key", Py::String("PAD_5"));
                    break;
                case SoKeyboardEvent::PAD_6:
                    dict.setItem("Key", Py::String("PAD_6"));
                    break;
                case SoKeyboardEvent::PAD_7:
                    dict.setItem("Key", Py::String("PAD_7"));
                    break;
                case SoKeyboardEvent::PAD_8:
                    dict.setItem("Key", Py::String("PAD_8"));
                    break;
                case SoKeyboardEvent::PAD_9:
                    dict.setItem("Key", Py::String("PAD_9"));
                    break;
                case SoKeyboardEvent::PAD_ADD:
                    dict.setItem("Key", Py::String("PAD_ADD"));
                    break;
                case SoKeyboardEvent::PAD_SUBTRACT:
                    dict.setItem("Key", Py::String("PAD_SUBTRACT"));
                    break;
                case SoKeyboardEvent::PAD_MULTIPLY:
                    dict.setItem("Key", Py::String("PAD_MULTIPLY"));
                    break;
                case SoKeyboardEvent::PAD_DIVIDE:
                    dict.setItem("Key", Py::String("PAD_DIVIDE"));
                    break;
                case SoKeyboardEvent::PAD_TAB:
                    dict.setItem("Key", Py::String("PAD_TAB"));
                    break;
                case SoKeyboardEvent::PAD_DELETE:
                    dict.setItem("Key", Py::String("PAD_DELETE"));
                    break;
                case SoKeyboardEvent::F1:
                    dict.setItem("Key", Py::String("F1"));
                    break;
                case SoKeyboardEvent::F2:
                    dict.setItem("Key", Py::String("F2"));
                    break;
                case SoKeyboardEvent::F3:
                    dict.setItem("Key", Py::String("F3"));
                    break;
                case SoKeyboardEvent::F4:
                    dict.setItem("Key", Py::String("F4"));
                    break;
                case SoKeyboardEvent::F5:
                    dict.setItem("Key", Py::String("F5"));
                    break;
                case SoKeyboardEvent::F6:
                    dict.setItem("Key", Py::String("F6"));
                    break;
                case SoKeyboardEvent::F7:
                    dict.setItem("Key", Py::String("F7"));
                    break;
                case SoKeyboardEvent::F8:
                    dict.setItem("Key", Py::String("F8"));
                    break;
                case SoKeyboardEvent::F9:
                    dict.setItem("Key", Py::String("F9"));
                    break;
                case SoKeyboardEvent::F10:
                    dict.setItem("Key", Py::String("F10"));
                    break;
                case SoKeyboardEvent::F11:
                    dict.setItem("Key", Py::String("F11"));
                    break;
                case SoKeyboardEvent::F12:
                    dict.setItem("Key", Py::String("F12"));
                    break;
                case SoKeyboardEvent::BACKSPACE:
                    dict.setItem("Key", Py::String("BACKSPACE"));
                    break;
                case SoKeyboardEvent::TAB:
                    dict.setItem("Key", Py::String("TAB"));
                    break;
                case SoKeyboardEvent::RETURN:
                    dict.setItem("Key", Py::String("RETURN"));
                    break;
                case SoKeyboardEvent::PAUSE:
                    dict.setItem("Key", Py::String("PAUSE"));
                    break;
                case SoKeyboardEvent::SCROLL_LOCK:
                    dict.setItem("Key", Py::String("SCROLL_LOCK"));
                    break;
                case SoKeyboardEvent::ESCAPE:
                    dict.setItem("Key", Py::String("ESCAPE"));
                    break;
                case SoKeyboardEvent::KEY_DELETE:
                    dict.setItem("Key", Py::String("DELETE"));
                    break;
                case SoKeyboardEvent::PRINT:
                    dict.setItem("Key", Py::String("PRINT"));
                    break;
                case SoKeyboardEvent::INSERT:
                    dict.setItem("Key", Py::String("INSERT"));
                    break;
                case SoKeyboardEvent::NUM_LOCK:
                    dict.setItem("Key", Py::String("NUM_LOCK"));
                    break;
                case SoKeyboardEvent::CAPS_LOCK:
                    dict.setItem("Key", Py::String("CAPS_LOCK"));
                    break;
                case SoKeyboardEvent::SHIFT_LOCK:
                    dict.setItem("Key", Py::String("SHIFT_LOCK"));
                    break;
                case SoKeyboardEvent::SPACE:
                    dict.setItem("Key", Py::String("SPACE"));
                    break;
                case SoKeyboardEvent::APOSTROPHE:
                    dict.setItem("Key", Py::String("APOSTROPHE"));
                    break;
                case SoKeyboardEvent::COMMA:
                    dict.setItem("Key", Py::String("COMMA"));
                    break;
                case SoKeyboardEvent::MINUS:
                    dict.setItem("Key", Py::String("MINUS"));
                    break;
                case SoKeyboardEvent::PERIOD:
                    dict.setItem("Key", Py::String("PERIOD"));
                    break;
                case SoKeyboardEvent::SLASH:
                    dict.setItem("Key", Py::String("SLASH"));
                    break;
                case SoKeyboardEvent::SEMICOLON:
                    dict.setItem("Key", Py::String("SEMICOLON"));
                    break;
                case SoKeyboardEvent::EQUAL:
                    dict.setItem("Key", Py::String("EQUAL"));
                    break;
                case SoKeyboardEvent::BRACKETLEFT:
                    dict.setItem("Key", Py::String("BRACKETLEFT"));
                    break;
                case SoKeyboardEvent::BACKSLASH:
                    dict.setItem("Key", Py::String("BACKSLASH"));
                    break;
                case SoKeyboardEvent::BRACKETRIGHT:
                    dict.setItem("Key", Py::String("BRACKETRIGHT"));
                    break;
                case SoKeyboardEvent::GRAVE:
                    dict.setItem("Key", Py::String("GRAVE"));
                    break;
                default:
                    dict.setItem("Key", Py::Char(ke->getPrintableCharacter()));
                    break;
            }
        }
        if (e->isOfType(SoMouseButtonEvent::getClassTypeId())) {
            const SoMouseButtonEvent* mbe = static_cast<const SoMouseButtonEvent*>(e);
            std::string button;
            switch (mbe->getButton()) {
                case SoMouseButtonEvent::BUTTON1:
                    button = "BUTTON1";
                    break;
                case SoMouseButtonEvent::BUTTON2:
                    button = "BUTTON2";
                    break;
                case SoMouseButtonEvent::BUTTON3:
                    button = "BUTTON3";
                    break;
                case SoMouseButtonEvent::BUTTON4:
                    button = "BUTTON4";
                    break;
                case SoMouseButtonEvent::BUTTON5:
                    button = "BUTTON5";
                    break;
                default:
                    button = "ANY";
                    break;
            }

            dict.setItem("Button", Py::String(button));
        }
        if (e->isOfType(SoSpaceballButtonEvent::getClassTypeId())) {
            const SoSpaceballButtonEvent* sbe = static_cast<const SoSpaceballButtonEvent*>(e);
            std::string button;
            switch (sbe->getButton()) {
                case SoSpaceballButtonEvent::BUTTON1:
                    button = "BUTTON1";
                    break;
                case SoSpaceballButtonEvent::BUTTON2:
                    button = "BUTTON2";
                    break;
                case SoSpaceballButtonEvent::BUTTON3:
                    button = "BUTTON3";
                    break;
                case SoSpaceballButtonEvent::BUTTON4:
                    button = "BUTTON4";
                    break;
                case SoSpaceballButtonEvent::BUTTON5:
                    button = "BUTTON5";
                    break;
                case SoSpaceballButtonEvent::BUTTON6:
                    button = "BUTTON6";
                    break;
                case SoSpaceballButtonEvent::BUTTON7:
                    button = "BUTTON7";
                    break;
                default:
                    button = "ANY";
                    break;
            }

            dict.setItem("Button", Py::String(button));
        }
        if (e->isOfType(SoMotion3Event::getClassTypeId())) {
            const SoMotion3Event* me = static_cast<const SoMotion3Event*>(e);
            const SbVec3f& m = me->getTranslation();
            const SbRotation& r = me->getRotation();
            Py::Tuple mov(3);
            mov.setItem(0, Py::Float(m[0]));
            mov.setItem(1, Py::Float(m[1]));
            mov.setItem(2, Py::Float(m[2]));
            dict.setItem("Translation", mov);
            Py::Tuple rot(4);
            rot.setItem(0, Py::Float(r.getValue()[0]));
            rot.setItem(1, Py::Float(r.getValue()[1]));
            rot.setItem(2, Py::Float(r.getValue()[2]));
            rot.setItem(3, Py::Float(r.getValue()[3]));
            dict.setItem("Rotation", rot);
        }

        // now run the method
        Py::Callable method(reinterpret_cast<PyObject*>(ud));
        Py::Tuple args(1);
        args.setItem(0, dict);
        method.apply(args);
    }
    catch (const Py::Exception& e) {
        Py::Object o = Py::type(e);
        if (o.isString()) {
            Py::String s(o);
            Base::Console().Warning("%s\n", s.as_std_string("utf-8").c_str());
        }
        else {
            Py::String s(o.repr());
            Base::Console().Warning("%s\n", s.as_std_string("utf-8").c_str());
        }
        // Prints message to console window if we are in interactive mode
        PyErr_Print();
    }
}

Py::Object View3DInventorPy::addEventCallback(const Py::Tuple& args)
{
    char* eventtype;
    PyObject* method;
    if (!PyArg_ParseTuple(args.ptr(), "sO", &eventtype, &method))
        throw Py::Exception();
    try {
        if (PyCallable_Check(method) == 0) {
            throw Py::TypeError("object is not callable");
        }
        SoType eventId = SoType::fromName(eventtype);
        if (eventId.isBad() || !eventId.isDerivedFrom(SoEvent::getClassTypeId())) {
            std::string s;
            std::ostringstream s_out;
            s_out << eventtype << " is not a valid event type";
            throw Py::TypeError(s_out.str());
        }

        _view->getViewer()->addEventCallback(eventId, View3DInventorPy::eventCallback, method);
        callbacks.push_back(method);
        Py_INCREF(method);
        return Py::Callable(method, false);
    }
    catch (const Py::Exception&) {
        throw;
    }
}

Py::Object View3DInventorPy::removeEventCallback(const Py::Tuple& args)
{
    char* eventtype;
    PyObject* method;
    if (!PyArg_ParseTuple(args.ptr(), "sO", &eventtype, &method))
        throw Py::Exception();
    try {
        if (PyCallable_Check(method) == 0) {
            throw Py::RuntimeError("object is not callable");
        }
        SoType eventId = SoType::fromName(eventtype);
        if (eventId.isBad() || !eventId.isDerivedFrom(SoEvent::getClassTypeId())) {
            std::string s;
            std::ostringstream s_out;
            s_out << eventtype << " is not a valid event type";
            throw Py::TypeError(s_out.str());
        }

        _view->getViewer()->removeEventCallback(eventId, View3DInventorPy::eventCallback, method);
        callbacks.remove(method);
        Py_DECREF(method);
        return Py::None();
    }
    catch (const Py::Exception&) {
        throw;
    }
}

Py::Object View3DInventorPy::setAnnotation(const Py::Tuple& args)
{
    char *psAnnoName,*psBuffer;
    if (!PyArg_ParseTuple(args.ptr(), "ss", &psAnnoName, &psBuffer))
        throw Py::Exception();
    ViewProviderExtern* view = 0;
    try {
        view = new ViewProviderExtern();
        view->setModeByString(psAnnoName, psBuffer);
    }
    catch (const Base::Exception& e) {
        delete view;
        throw Py::RuntimeError(e.what());
    }

    _view->getGuiDocument()->setAnnotationViewProvider(psAnnoName, view);
    return Py::None();
}

Py::Object View3DInventorPy::removeAnnotation(const Py::Tuple& args)
{
    char *psAnnoName;
    if (!PyArg_ParseTuple(args.ptr(), "s", &psAnnoName))
        throw Py::Exception();
    ViewProvider* view = 0;
    view = _view->getGuiDocument()->getAnnotationViewProvider(psAnnoName);
    if (view) {
        _view->getGuiDocument()->removeAnnotationViewProvider(psAnnoName);
        return Py::None();
    }
    else {
        std::string s;
        std::ostringstream s_out;
        s_out << "No such annotation '" << psAnnoName << "'";
        throw Py::KeyError(s_out.str());
    }
}

Py::Object View3DInventorPy::getSceneGraph(const Py::Tuple& args)
{
    if (!PyArg_ParseTuple(args.ptr(), ""))
        throw Py::Exception();

    try {
        SoNode* scene = _view->getViewer()->getSceneGraph();
        PyObject* proxy = 0;
        proxy = Base::Interpreter().createSWIGPointerObj("pivy.coin", "SoSeparator *", (void*)scene, 1);
        scene->ref();
        return Py::Object(proxy, true);
    }
    catch (const Base::Exception& e) {
        throw Py::RuntimeError(e.what());
    }
}

Py::Object View3DInventorPy::getViewer(const Py::Tuple& args)
{
    if (!PyArg_ParseTuple(args.ptr(), ""))
        throw Py::Exception();

    View3DInventorViewer* viewer = _view->getViewer();
    return Py::Object(viewer->getPyObject(), true);
}

void View3DInventorPy::eventCallbackPivy(void * ud, SoEventCallback * n)
{
    Base::PyGILStateLocker lock;
    const SoEvent* e = n->getEvent();
    std::string type = e->getTypeId().getName().getString();
    type += " *";

    PyObject* proxy = 0;
    try {
        proxy = Base::Interpreter().createSWIGPointerObj("pivy.coin", type.c_str(), (void*)e, 0);
        // now run the method
        Py::Object event(proxy,true);
        Py::Callable method(reinterpret_cast<PyObject*>(ud));
        Py::Tuple args(1);
        args.setItem(0, event);
        method.apply(args);
    }
    catch (const Base::Exception&) {
        return;
    }
    catch (const Py::Exception& e) {
        Py::Object o = Py::type(e);
        if (o.isString()) {
            Py::String s(o);
            Base::Console().Warning("%s\n", s.as_std_string("utf-8").c_str());
        }
        else {
            Py::String s(o.repr());
            Base::Console().Warning("%s\n", s.as_std_string("utf-8").c_str());
        }
        // Prints message to console window if we are in interactive mode
        PyErr_Print();
    }
}

void View3DInventorPy::eventCallbackPivyEx(void * ud, SoEventCallback * n)
{
    Base::PyGILStateLocker lock;
    std::string type = "SoEventCallback *";

    PyObject* proxy = 0;
    try {
        proxy = Base::Interpreter().createSWIGPointerObj("pivy.coin", type.c_str(), (void*)n, 0);
        // now run the method
        Py::Object event(proxy,true);
        Py::Callable method(reinterpret_cast<PyObject*>(ud));
        Py::Tuple args(1);
        args.setItem(0, event);
        method.apply(args);
    }
    catch (const Base::Exception&) {
        return;
    }
    catch (const Py::Exception& e) {
        Py::Object o = Py::type(e);
        if (o.isString()) {
            Py::String s(o);
            Base::Console().Warning("%s\n", s.as_std_string("utf-8").c_str());
        }
        else {
            Py::String s(o.repr());
            Base::Console().Warning("%s\n", s.as_std_string("utf-8").c_str());
        }
        // Prints message to console window if we are in interactive mode
        PyErr_Print();
    }
}

Py::Object View3DInventorPy::addEventCallbackPivy(const Py::Tuple& args)
{
    PyObject* proxy;
    PyObject* method;
    int ex=1; // if 1, use eventCallbackPivyEx
    if (!PyArg_ParseTuple(args.ptr(), "OO|i", &proxy, &method,&ex))
        throw Py::Exception();

    void* ptr = 0;
    try {
        Base::Interpreter().convertSWIGPointerObj("pivy.coin", "SoType *", proxy, &ptr, 0);
    }
    catch (const Base::Exception& e) {
        throw Py::RuntimeError(e.what());
    }

    SoType* eventId = reinterpret_cast<SoType*>(ptr);
    if (eventId->isBad() || !eventId->isDerivedFrom(SoEvent::getClassTypeId())) {
        std::string s;
        std::ostringstream s_out;
        s_out << eventId->getName().getString() << "is not a valid event type";
        throw Py::TypeError(s_out.str());
    }

    try {
        if (PyCallable_Check(method) == 0) {
            throw Py::TypeError("object is not callable");
        }

        SoEventCallbackCB* callback = (ex == 1 ? 
            View3DInventorPy::eventCallbackPivyEx :
            View3DInventorPy::eventCallbackPivy);
        _view->getViewer()->addEventCallback(*eventId, callback, method);
        callbacks.push_back(method);
        Py_INCREF(method);
        return Py::Callable(method, false);
    }
    catch (const Py::Exception&) {
        throw;
    }
}

Py::Object View3DInventorPy::removeEventCallbackPivy(const Py::Tuple& args)
{
    PyObject* proxy;
    PyObject* method;
    int ex=1; // if 1, use eventCallbackPivyEx
    if (!PyArg_ParseTuple(args.ptr(), "OO|i", &proxy, &method,&ex))
        throw Py::Exception();

    void* ptr = 0;
    try {
        Base::Interpreter().convertSWIGPointerObj("pivy.coin", "SoType *", proxy, &ptr, 0);
    }
    catch (const Base::Exception& e) {
        throw Py::RuntimeError(e.what());
    }

    SoType* eventId = reinterpret_cast<SoType*>(ptr);
    if (eventId->isBad() || !eventId->isDerivedFrom(SoEvent::getClassTypeId())) {
        std::string s;
        std::ostringstream s_out;
        s_out << eventId->getName().getString() << "is not a valid event type";
        throw Py::TypeError(s_out.str());
    }

    try {
        if (PyCallable_Check(method) == 0) {
            throw Py::TypeError("object is not callable");
        }

        SoEventCallbackCB* callback = (ex == 1 ? 
            View3DInventorPy::eventCallbackPivyEx :
            View3DInventorPy::eventCallbackPivy);
        _view->getViewer()->removeEventCallback(*eventId, callback, method);
        callbacks.remove(method);
        Py_DECREF(method);
        return Py::Callable(method, false);
    }
    catch (const Py::Exception&) {
        throw;
    }
}

Py::Object View3DInventorPy::setAxisCross(const Py::Tuple& args)
{
    int ok;
    if (!PyArg_ParseTuple(args.ptr(), "i", &ok))
        throw Py::Exception();
    _view->getViewer()->setAxisCross(ok!=0);
    return Py::None();
}

Py::Object View3DInventorPy::hasAxisCross(const Py::Tuple& args)
{
    if (!PyArg_ParseTuple(args.ptr(), ""))
        throw Py::Exception();
    SbBool ok = _view->getViewer()->hasAxisCross();
    return Py::Boolean(ok ? true : false);
}

void View3DInventorPy::draggerCallback(void * ud, SoDragger* n)
{
    Base::PyGILStateLocker lock;
    PyObject* proxy = 0;
    try {
        proxy = Base::Interpreter().createSWIGPointerObj("pivy.coin", "SoDragger *", (void*)n, 0);
        //call the method
        Py::Object dragger(proxy,true);
        Py::Callable method(reinterpret_cast<PyObject*>(ud));
        Py::Tuple args(1);
        args.setItem(0, dragger);
        method.apply(args);
    }
    catch (const Base::Exception& e) {
        throw Py::RuntimeError(e.what());
    }
    catch (const Py::Exception& e) {
        Py::Object o = Py::type(e);
        if (o.isString()) {
            Py::String s(o);
            Base::Console().Warning("%s\n", s.as_std_string("utf-8").c_str());
        }
        else {
            Py::String s(o.repr());
            Base::Console().Warning("%s\n", s.as_std_string("utf-8").c_str());
        }
        // Prints message to console window if we are in interactive mode
        PyErr_Print();
    }
}

Py::Object View3DInventorPy::addDraggerCallback(const Py::Tuple& args)
{
    PyObject* dragger;
    char* type;
    PyObject* method;
    if (!PyArg_ParseTuple(args.ptr(), "OsO", &dragger,&type, &method))
        throw Py::Exception();


    //Check if dragger is a SoDragger object and cast
    void* ptr = 0;
    try {
        Base::Interpreter().convertSWIGPointerObj("pivy.coin", "SoDragger *", dragger, &ptr, 0);
    }
    catch (const Base::Exception&) {
        throw Py::TypeError("The first argument must be of type SoDragger");
    }
    SoDragger* drag = reinterpret_cast<SoDragger*>(ptr);

    //Check if method is callable
    if (PyCallable_Check(method) == 0) {
        throw Py::TypeError("the method is not callable");
    }

    try {
        if (strcmp(type,"addFinishCallback")==0) {
            drag->addFinishCallback(draggerCallback,method);
        }
        else if (strcmp(type,"addStartCallback")==0) {
            drag->addStartCallback(draggerCallback,method);
        }
        else if (strcmp(type,"addMotionCallback")==0) {
            drag->addMotionCallback(draggerCallback,method);
        }
        else if (strcmp(type,"addValueChangedCallback")==0) {
            drag->addValueChangedCallback(draggerCallback,method);
        }
        else {
            std::string s;
            std::ostringstream s_out;
            s_out << type << " is not a valid dragger callback type";
            throw Py::TypeError(s_out.str());
        }

        callbacks.push_back(method);
        Py_INCREF(method);
        return Py::Callable(method, false);
    }
    catch (const Py::Exception&) {
        throw;
    }
}

Py::Object View3DInventorPy::removeDraggerCallback(const Py::Tuple& args)
{
    PyObject* dragger;
    char* type;
    PyObject* method;
    if (!PyArg_ParseTuple(args.ptr(), "OsO", &dragger, &type, &method))
        throw Py::Exception();

    //Check if dragger is a SoDragger object and cast
    void* ptr = 0;
    try {
        Base::Interpreter().convertSWIGPointerObj("pivy.coin", "SoDragger *", dragger, &ptr, 0);
    }
    catch (const Base::Exception&) {
        throw Py::TypeError("The first argument must be of type SoDragger");
    }

    SoDragger* drag = reinterpret_cast<SoDragger*>(ptr);
    try {
        if (strcmp(type, "addFinishCallback") == 0) {
            drag->removeFinishCallback(draggerCallback, method);
        }
        else if (strcmp(type, "addStartCallback") == 0) {
            drag->removeStartCallback(draggerCallback, method);
        }
        else if (strcmp(type, "addMotionCallback") == 0) {
            drag->removeMotionCallback(draggerCallback, method);
        }
        else if (strcmp(type, "addValueChangedCallback") == 0) {
            drag->removeValueChangedCallback(draggerCallback, method);
        }
        else {
            std::string s;
            std::ostringstream s_out;
            s_out << type << " is not a valid dragger callback type";
            throw Py::TypeError(s_out.str());
        }

        callbacks.remove(method);
        Py_DECREF(method);
        return Py::Callable(method, false);
    }
    catch (const Py::Exception&) {
        throw;
    }
}

Py::Object View3DInventorPy::setActiveObject(const Py::Tuple& args)
{
    PyObject* docObject = 0;
    char* name;

    //allow reset of active object by setting "None"
    if (args.length() == 2 && args.back() == Py::None()) {
        PyArg_Parse(args.front().ptr(), "s", &name);
        _view->setActiveObject(NULL, name);
        return Py::None();
    }

    if (!PyArg_ParseTuple(args.ptr(), "sO!", &name, &App::DocumentObjectPy::Type, &docObject))
        throw Py::Exception();

    if (docObject){
        App::DocumentObject* obj = static_cast<App::DocumentObjectPy*>(docObject)->getDocumentObjectPtr();
        _view->setActiveObject(obj, name);
    }
    return Py::None();
}

Py::Object View3DInventorPy::getActiveObject(const Py::Tuple& args)
{
    char* name;
    if (!PyArg_ParseTuple(args.ptr(), "s", &name))
        throw Py::Exception();

    App::DocumentObject* obj = _view->getActiveObject<App::DocumentObject*>(name);
    if(!obj)
        return Py::None();

    return Py::Object(obj->getPyObject());
}

Py::Object View3DInventorPy::getViewProvidersOfType(const Py::Tuple& args)
{
    char* name;
    if (!PyArg_ParseTuple(args.ptr(), "s", &name))
        throw Py::Exception();

    std::vector<ViewProvider*> vps = _view->getViewer()->getViewProvidersOfType(Base::Type::fromName(name));
    Py::List list;
    for (std::vector<ViewProvider*>::iterator it = vps.begin(); it != vps.end(); ++it) {
        list.append(Py::asObject((*it)->getPyObject()));
    }

    return list;
}

Py::Object View3DInventorPy::redraw(const Py::Tuple& args)
{
    if (!PyArg_ParseTuple(args.ptr(), ""))
        throw Py::Exception();
    _view->getViewer()->redraw();
    return Py::None();
}
