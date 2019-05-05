/***************************************************************************
 *   Copyright (c) Stefan Tröger          (stefantroeger@gmx.net) 2014     *
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
#endif


#include "View3DViewerPy.h"
#include <CXX/Objects.hxx>
#include <Base/Interpreter.h>
#include <Base/GeometryPyCXX.h>
#include <Base/VectorPy.h>
#include <Base/MatrixPy.h>
#include <Gui/View3DInventorViewer.h>

using namespace Gui;


void View3DInventorViewerPy::init_type()
{
    behaviors().name("View3DInventorViewerPy");
    behaviors().doc("Python binding class for the 3D viewer class");
    // you must have overwritten the virtual functions
    behaviors().supportRepr();
    behaviors().supportGetattr();
    behaviors().supportSetattr();

    add_varargs_method("getSoRenderManager",&View3DInventorViewerPy::getSoRenderManager,"getSoRenderManager() -> SoRenderManager\n"
        "Returns the render manager which is used to handle everything related to\n"
        "rendering the scene graph. It can be used to get full control over the\n"
        "render process\n"
    );
    add_varargs_method("getSoEventManager",&View3DInventorViewerPy::getSoEventManager,"getSoEventManager() -> SoEventManager\n"
        "Returns the event manager which is used to handle everything event related in\n"
        "the viewer. It can be used to change the event processing. This must however be\n"
        "done very carefully to not change the user interaction in an unpredictable manner.\n"
    );
    add_varargs_method("getSceneGraph", &View3DInventorViewerPy::getSceneGraph, "getSceneGraph() -> SoNode");
    add_varargs_method("setSceneGraph", &View3DInventorViewerPy::setSceneGraph, "setSceneGraph(SoNode)");

    add_varargs_method("seekToPoint",&View3DInventorViewerPy::seekToPoint,"seekToPoint(tuple) -> None\n"
     "Initiate a seek action towards the 3D intersection of the scene and the\n"
     "ray from the screen coordinate's point and in the same direction as the\n"
     "camera is pointing. If the tuple has two entries it is interpreted as the\n"
     "screen coordinates xy and the intersection point with the scene is\n"
     "calculated. If three entries are given it is interpreted as the intersection\n"
     "point xyz and the seek is done towards this point"
    );
    add_varargs_method("setFocalDistance",&View3DInventorViewerPy::setFocalDistance,"setFocalDistance(float) -> None\n");
    add_varargs_method("getFocalDistance",&View3DInventorViewerPy::getFocalDistance,"getFocalDistance() -> float\n");
    add_varargs_method("getPoint", &View3DInventorViewerPy::getPoint, "getPoint(x, y) -> Base::Vector(x,y,z)");
    add_varargs_method("getPickRadius", &View3DInventorViewerPy::getPickRadius,
        "getPickRadius(): returns radius of confusion in pixels for picking objects on screen (selection).");
    add_varargs_method("setPickRadius", &View3DInventorViewerPy::setPickRadius,
        "setPickRadius(new_radius): sets radius of confusion in pixels for picking objects on screen (selection).");
    add_varargs_method("setupEditingRoot", &View3DInventorViewerPy::setupEditingRoot,
        "setupEditingRoot(matrix=None): setup the editing ViewProvider's root node.\n"
        "All child coin nodes of the current editing ViewProvider will be transferred to\n"
        "an internal editing node of this viewer, with a new transformation node specified\n"
        "by 'matrix'. All ViewProviderLink to the editing ViewProvider will be temperary\n"
        "hidden. Call resetEditingRoot() to restore everything back to normal");
    add_varargs_method("resetEditingRoot", &View3DInventorViewerPy::resetEditingRoot,
        "resetEditingRoot(updateLinks=True): restore the editing ViewProvider's root node");
    add_varargs_method("setBackgroundColor", &View3DInventorViewerPy::setBackgroundColor,
        "setBackgroundColor(r,g,b): sets the background color of the current viewer.");
    add_varargs_method("setRedirectToSceneGraph", &View3DInventorViewerPy::setRedirectToSceneGraph,
        "setRedirectToSceneGraph(bool): enables or disables to redirect events directly to the scene graph.");
    add_varargs_method("isRedirectedToSceneGraph", &View3DInventorViewerPy::isRedirectedToSceneGraph,
        "isRedirectedToSceneGraph() -> bool: check whether event redirection is enabled.");
    add_varargs_method("setEnabledNaviCube", &View3DInventorViewerPy::setEnabledNaviCube,
        "setEnabledNaviCube(bool): enables or disables the navi cube of the viewer.");
    add_varargs_method("isEnabledNaviCube", &View3DInventorViewerPy::isEnabledNaviCube,
        "isEnabledNaviCube() -> bool: check whether the navi cube is enabled.");
    add_varargs_method("setNaviCubeCorner", &View3DInventorViewerPy::setNaviCubeCorner,
        "setNaviCubeCorner(int): sets the corner where to show the navi cube:\n"
        "0=top left, 1=top right, 2=bottom left, 3=bottom right");
}

View3DInventorViewerPy::View3DInventorViewerPy(View3DInventorViewer *vi)
  : _viewer(vi)
{
}

View3DInventorViewerPy::~View3DInventorViewerPy()
{
    Base::PyGILStateLocker lock;
    for (std::list<PyObject*>::iterator it = callbacks.begin(); it != callbacks.end(); ++it)
        Py_DECREF(*it);
}


Py::Object View3DInventorViewerPy::repr()
{
    std::ostringstream s_out;
    if (!_viewer)
        throw Py::RuntimeError("Cannot print representation of deleted object");
    s_out << "View3DInventorViewer";
    return Py::String(s_out.str());
}

View3DInventorViewerPy::method_varargs_handler View3DInventorViewerPy::pycxx_handler = 0;

PyObject *View3DInventorViewerPy::method_varargs_ext_handler(PyObject *_self_and_name_tuple, PyObject *_args)
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

Py::Object View3DInventorViewerPy::getattr(const char * attr)
{
    if (!_viewer) {
        std::string s;
        std::ostringstream s_out;
        s_out << "Cannot access attribute '" << attr << "' of deleted object";
        throw Py::RuntimeError(s_out.str());
    }
    else {
        Py::Object obj = Py::PythonExtension<View3DInventorViewerPy>::getattr(attr);
        if (PyCFunction_Check(obj.ptr())) {
            PyCFunctionObject* op = reinterpret_cast<PyCFunctionObject*>(obj.ptr());
            if (!pycxx_handler)
                pycxx_handler = op->m_ml->ml_meth;
            op->m_ml->ml_meth = method_varargs_ext_handler;
        }
        return obj;
    }
}

int View3DInventorViewerPy::setattr(const char * attr, const Py::Object & value)
{
    if (!_viewer) {
        std::string s;
        std::ostringstream s_out;
        s_out << "Cannot access attribute '" << attr << "' of deleted object";
        throw Py::RuntimeError(s_out.str());
    }
    else {
        return Py::PythonExtension<View3DInventorViewerPy>::setattr(attr, value);
    }
}

Py::Object View3DInventorViewerPy::getSoRenderManager(const Py::Tuple& args)
{
    if (!PyArg_ParseTuple(args.ptr(), ""))
        throw Py::Exception();

    try {
        SoRenderManager* manager = _viewer->getSoRenderManager();
        PyObject* proxy = 0;
        proxy = Base::Interpreter().createSWIGPointerObj("pivy.coin", "SoRenderManager *", (void*)manager, 0);
        return Py::Object(proxy, true);
    }
    catch (const Base::Exception& e) {
        throw Py::RuntimeError(e.what());
    }
}

Py::Object View3DInventorViewerPy::getSceneGraph(const Py::Tuple& args)
{
    if (!PyArg_ParseTuple(args.ptr(), ""))
        throw Py::Exception();

    try {
        SoNode* scene = _viewer->getSceneGraph();
        PyObject* proxy = 0;
        proxy = Base::Interpreter().createSWIGPointerObj("pivy.coin", "SoSeparator *", (void*)scene, 1);
        scene->ref();
        return Py::Object(proxy, true);
    }
    catch (const Base::Exception& e) {
        throw Py::RuntimeError(e.what());
    }
}

Py::Object View3DInventorViewerPy::setSceneGraph(const Py::Tuple& args)
{
    PyObject* proxy;
    if (!PyArg_ParseTuple(args.ptr(), "O", &proxy))
        throw Py::Exception();

    void* ptr = 0;
    try {
        Base::Interpreter().convertSWIGPointerObj("pivy.coin", "SoNode *", proxy, &ptr, 0);
        SoNode* node = static_cast<SoNode*>(ptr);
        _viewer->setSceneGraph(node);
        return Py::None();
    }
    catch (const Base::Exception& e) {
        throw Py::RuntimeError(e.what());
    }
}

Py::Object View3DInventorViewerPy::getSoEventManager(const Py::Tuple& args)
{
    if (!PyArg_ParseTuple(args.ptr(), ""))
        throw Py::Exception();

    try {
        SoEventManager* manager = _viewer->getSoEventManager();
        PyObject* proxy = 0;
        proxy = Base::Interpreter().createSWIGPointerObj("pivy.coin", "SoEventManager *", (void*)manager, 0);
        return Py::Object(proxy, true);
    }
    catch (const Base::Exception& e) {
        throw Py::RuntimeError(e.what());
    }
}

Py::Object View3DInventorViewerPy::seekToPoint(const Py::Tuple& args)
{
    PyObject* object;
    if (!PyArg_ParseTuple(args.ptr(), "O", &object))
        throw Py::Exception();

   try {
        const Py::Tuple tuple(object);

        // If the 3d point is given
        if (tuple.size() == 3) {
            Py::Float x = tuple[0];
            Py::Float y = tuple[1];
            Py::Float z = tuple[2];

            SbVec3f hitpoint((float)x,(float)y,(float)z);
            _viewer->seekToPoint(hitpoint);
        }
        else {
            Py::Int x(tuple[0]);
            Py::Int y(tuple[1]);
            
            SbVec2s hitpoint ((long)x,(long)y);
            _viewer->seekToPoint(hitpoint);
        }

        return Py::None();
    }
    catch (const Py::Exception&) {
        throw;
    }
}

Py::Object View3DInventorViewerPy::setFocalDistance(const Py::Tuple& args)
{
    float distance;
    if (!PyArg_ParseTuple(args.ptr(), "f", &distance))
        throw Py::Exception(); 

    try {
        SoCamera* cam = _viewer->getSoRenderManager()->getCamera();
        if (cam)
            cam->focalDistance.setValue(distance);
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

Py::Object View3DInventorViewerPy::getFocalDistance(const Py::Tuple& args)
{
    if (!PyArg_ParseTuple(args.ptr(), ""))
        throw Py::Exception();
    
    try {
        double d = _viewer->getSoRenderManager()->getCamera()->focalDistance.getValue();
        return Py::Float(d);
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

Py::Object View3DInventorViewerPy::getPoint(const Py::Tuple& args)
{
    short x,y;
    if (!PyArg_ParseTuple(args.ptr(), "hh", &x, &y)) {
        PyErr_Clear();
        Py::Tuple t(args[0]);
        x = (int)Py::Int(t[0]);
        y = (int)Py::Int(t[1]);
    }
    try {
        SbVec3f pt = _viewer->getPointOnScreen(SbVec2s(x,y));
        return Py::Vector(Base::Vector3f(pt[0], pt[1], pt[2]));
    }
    catch (const Base::Exception& e) {
        throw Py::RuntimeError(e.what());
    }
    catch (const Py::Exception&) {
        throw;
    }
}

Py::Object View3DInventorViewerPy::getPickRadius(const Py::Tuple& args)
{
    if (!PyArg_ParseTuple(args.ptr(), ""))
        throw Py::Exception();

    double d = _viewer->getPickRadius();
    return Py::Float(d);
}

Py::Object View3DInventorViewerPy::setPickRadius(const Py::Tuple& args)
{
    float r = 0.0;
    if (!PyArg_ParseTuple(args.ptr(), "f", &r)) {
        throw Py::Exception();
    }

    if (r < 0.001){
        throw Py::ValueError(std::string("Pick radius is zero or negative; positive number is required."));
    }
    try {
        _viewer->setPickRadius(r);
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

Py::Object View3DInventorViewerPy::setupEditingRoot(const Py::Tuple& args)
{
    PyObject *pynode = Py_None;
    PyObject *pymat = Py_None;
    if (!PyArg_ParseTuple(args.ptr(), "|OO!", &pynode,&Base::MatrixPy::Type,&pymat)) {
        throw Py::Exception();
    }

    Base::Matrix4D *mat = 0;
    if(pymat != Py_None)
        mat = static_cast<Base::MatrixPy*>(pymat)->getMatrixPtr();

    try {
        SoNode *node = 0;
        if(pynode!=Py_None) {
            void* ptr = 0;
            Base::Interpreter().convertSWIGPointerObj("pivy.coin", "SoNode *", pynode, &ptr, 0);
            node = reinterpret_cast<SoNode*>(ptr);
        }
        _viewer->setupEditingRoot(node,mat);
        return Py::None();
    }
    catch (const Base::Exception& e) {
        throw Py::Exception(Base::BaseExceptionFreeCADError,e.what());
    }
    catch (const std::exception& e) {
        throw Py::RuntimeError(e.what());
    }
    catch(...) {
        throw Py::RuntimeError("Unknown C++ exception");
    }
}

Py::Object View3DInventorViewerPy::resetEditingRoot(const Py::Tuple& args)
{
    PyObject *updateLinks = Py_True;
    if (!PyArg_ParseTuple(args.ptr(), "|O", &updateLinks)) {
        throw Py::Exception();
    }
    try {
        _viewer->resetEditingRoot(PyObject_IsTrue(updateLinks));
        return Py::None();
    }
    catch (const Base::Exception& e) {
        throw Py::Exception(Base::BaseExceptionFreeCADError,e.what());
    }
    catch (const std::exception& e) {
        throw Py::RuntimeError(e.what());
    }
    catch(...) {
        throw Py::RuntimeError("Unknown C++ exception");
    }
}

Py::Object View3DInventorViewerPy::setBackgroundColor(const Py::Tuple& args)
{
    float r,g,b = 0.0;
    if (!PyArg_ParseTuple(args.ptr(), "fff", &r, &g, &b)) {
        throw Py::Exception();
    }
    try {
        SbColor col(r,g,b);
        _viewer->setGradientBackgroundColor(col,col);
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

Py::Object View3DInventorViewerPy::setRedirectToSceneGraph(const Py::Tuple& args)
{
    PyObject* m=Py_False;
    if (!PyArg_ParseTuple(args.ptr(), "O!", &PyBool_Type, &m))
        throw Py::Exception();
    _viewer->setRedirectToSceneGraph(PyObject_IsTrue(m) ? true : false);
    return Py::None();
}

Py::Object View3DInventorViewerPy::isRedirectedToSceneGraph(const Py::Tuple& args)
{
    if (!PyArg_ParseTuple(args.ptr(), ""))
        throw Py::Exception();
    bool ok = _viewer->isRedirectedToSceneGraph();
    return Py::Boolean(ok);
}

Py::Object View3DInventorViewerPy::setEnabledNaviCube(const Py::Tuple& args)
{
    PyObject* m=Py_False;
    if (!PyArg_ParseTuple(args.ptr(), "O!", &PyBool_Type, &m))
        throw Py::Exception();
    _viewer->setEnabledNaviCube(PyObject_IsTrue(m));
    return Py::None();
}

Py::Object View3DInventorViewerPy::isEnabledNaviCube(const Py::Tuple& args)
{
    if (!PyArg_ParseTuple(args.ptr(), ""))
        throw Py::Exception();
    bool ok = _viewer->isEnabledNaviCube();
    return Py::Boolean(ok);
}

Py::Object View3DInventorViewerPy::setNaviCubeCorner(const Py::Tuple& args)
{
    int pos;
    if (!PyArg_ParseTuple(args.ptr(), "i", &pos))
        throw Py::Exception();
    if (pos < 0 || pos > 3)
        throw Py::IndexError("Value out of range");
    _viewer->setNaviCubeCorner(pos);
    return Py::None();
}
