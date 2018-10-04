/***************************************************************************
 *   Copyright (c) 2007 Werner Mayer <wmayer[at]users.sourceforge.net>     *
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
# include <Inventor/nodes/SoSeparator.h>
# include <QByteArray>
# include <QDataStream>
#endif

#include <Inventor/SoDB.h>
#include <Inventor/actions/SoWriteAction.h>
#include <Inventor/nodes/SoCone.h>
#include <Inventor/nodes/SoSeparator.h>

#include "ViewProvider.h"
#include "WidgetFactory.h"

// inclusion of the generated files (generated out of ViewProviderPy2.xml)
#include <Gui/ViewProviderPy.h>
#include <Gui/ViewProviderPy.cpp>
#include <Base/Interpreter.h>
#include <Base/Matrix.h>
#include <Base/MatrixPy.h>
#include <Base/Placement.h>
#include <Base/PlacementPy.h>
#include <App/DocumentObject.h>

using namespace Gui;

// returns a string which represent the object e.g. when printed in python
std::string ViewProviderPy::representation(void) const
{
    return "<View provider object>";
}

PyObject*  ViewProviderPy::addProperty(PyObject *args)
{
    char *sType,*sName=0,*sGroup=0,*sDoc=0;
    short attr=0;
    std::string sDocStr;
    PyObject *ro = Py_False, *hd = Py_False;
    if (!PyArg_ParseTuple(args, "s|ssethO!O!", &sType,&sName,&sGroup,"utf-8",&sDoc,&attr,
        &PyBool_Type, &ro, &PyBool_Type, &hd))     // convert args: Python->C
        return NULL;                             // NULL triggers exception

    if (sDoc) {
        sDocStr = sDoc;
        PyMem_Free(sDoc);
    }

    App::Property* prop=0;
    try {
        prop = getViewProviderPtr()->addDynamicProperty(sType,sName,sGroup,sDocStr.c_str(),attr,
            PyObject_IsTrue(ro) ? true : false, PyObject_IsTrue(hd) ? true : false);
    }
    catch (const Base::Exception& e) {
        throw Py::RuntimeError(e.what());
    }
    if (!prop) {
        std::stringstream str;
        str << "No property found of type '" << sType << "'" << std::ends;
        throw Py::Exception(Base::BaseExceptionFreeCADError,str.str());
    }

    return Py::new_reference_to(this);
}

PyObject*  ViewProviderPy::removeProperty(PyObject *args)
{
    char *sName;
    if (!PyArg_ParseTuple(args, "s", &sName))
        return NULL;

    try {
        bool ok = getViewProviderPtr()->removeDynamicProperty(sName);
        return Py_BuildValue("O", (ok ? Py_True : Py_False));
    }
    catch (const Base::Exception& e) {
        throw Py::RuntimeError(e.what());
    }
}

PyObject*  ViewProviderPy::supportedProperties(PyObject *args)
{
    if (!PyArg_ParseTuple(args, ""))     // convert args: Python->C
        return NULL;                    // NULL triggers exception

    std::vector<Base::Type> ary;
    Base::Type::getAllDerivedFrom(App::Property::getClassTypeId(), ary);
    Py::List res;
    for (std::vector<Base::Type>::iterator it = ary.begin(); it != ary.end(); ++it) {
        Base::BaseClass *data = static_cast<Base::BaseClass*>(it->createInstance());
        if (data) {
            delete data;
            res.append(Py::String(it->getName()));
        }
    }
    return Py::new_reference_to(res);
}

PyObject*  ViewProviderPy::show(PyObject *args)
{
    if (!PyArg_ParseTuple(args, ""))     // convert args: Python->C 
        return NULL;                       // NULL triggers exception 
    PY_TRY {
        getViewProviderPtr()->show();  
        Py_Return;
    } PY_CATCH;
}

PyObject*  ViewProviderPy::hide(PyObject *args)
{
    if (!PyArg_ParseTuple(args, ""))     // convert args: Python->C 
        return NULL;                       // NULL triggers exception 
    PY_TRY {
        getViewProviderPtr()->hide();  
        Py_Return;
    } PY_CATCH;
}

PyObject*  ViewProviderPy::isVisible(PyObject *args)
{
    if (!PyArg_ParseTuple(args, ""))     // convert args: Python->C 
        return NULL;                       // NULL triggers exception 
    PY_TRY {
        return Py_BuildValue("O", (getViewProviderPtr()->isShow() ? Py_True : Py_False));
    } PY_CATCH;
}

PyObject* ViewProviderPy::addDisplayMode(PyObject * args)
{
    char* mode;
    PyObject* obj;
    if (!PyArg_ParseTuple(args, "Os", &obj, &mode))
        return NULL;

    void* ptr = 0;
    try {
        Base::Interpreter().convertSWIGPointerObj("pivy.coin","SoNode *", obj, &ptr, 0);
    }
    catch (const Base::Exception& e) {
        PyErr_SetString(PyExc_RuntimeError, e.what());
        return 0;
    }

    PY_TRY {
        SoNode* node = reinterpret_cast<SoNode*>(ptr);
        getViewProviderPtr()->addDisplayMaskMode(node,mode);
        Py_Return;
    } PY_CATCH;
}

PyObject*  ViewProviderPy::listDisplayModes(PyObject *args)
{
    if (!PyArg_ParseTuple(args, ""))     // convert args: Python->C 
        return NULL;                       // NULL triggers exception 
    PY_TRY {
        std::vector<std::string> modes = getViewProviderPtr()->getDisplayModes();  
        PyObject* pyList = PyList_New(modes.size()); 
        int i=0;

        for ( std::vector<std::string>::iterator it = modes.begin(); it != modes.end(); ++it ) {
#if PY_MAJOR_VERSION >= 3
            PyObject* str = PyUnicode_FromString(it->c_str());
#else
            PyObject* str = PyString_FromString(it->c_str());
#endif
            PyList_SetItem(pyList, i++, str);
        }

        return pyList;
    } PY_CATCH;
}

PyObject*  ViewProviderPy::toString(PyObject *args)
{
    if (!PyArg_ParseTuple(args, ""))     // convert args: Python->C 
        return NULL;                     // NULL triggers exception 
    PY_TRY {
        std::string buffer = getViewProviderPtr()->toString();
        return Py::new_reference_to(Py::String(buffer));
    } PY_CATCH;
}

PyObject*  ViewProviderPy::setTransformation(PyObject *args)
{
    PyObject* p;
    Base::Matrix4D mat;
    if (PyArg_ParseTuple(args, "O!",&(Base::MatrixPy::Type),&p)) {
        mat = *static_cast<Base::MatrixPy*>(p)->getMatrixPtr();
        getViewProviderPtr()->setTransformation(mat);
        Py_Return;
    }
    PyErr_Clear();
    if (PyArg_ParseTuple(args, "O!",&(Base::PlacementPy::Type),&p)) {
        Base::PlacementPy* plc = static_cast<Base::PlacementPy*>(p);
        getViewProviderPtr()->setTransformation(plc->getPlacementPtr()->toMatrix());
        Py_Return;
    }

    PyErr_SetString(Base::BaseExceptionFreeCADError, "Either set matrix or placement to set transformation");
    return 0;
}

PyObject* ViewProviderPy::claimChildren(PyObject* args)
{
    if (!PyArg_ParseTuple(args, ""))
        return NULL;                     // NULL triggers exception

    std::vector<App::DocumentObject*> children = this->getViewProviderPtr()->claimChildren();
    Py::List ret;
    for(App::DocumentObject* child: children){
        if (child)
            ret.append(Py::asObject(child->getPyObject()));
        else
            ret.append(Py::None());
    }
    return Py::new_reference_to(ret);
}

PyObject *ViewProviderPy::getCustomAttributes(const char* attr) const
{
    // search for dynamic property
    App::Property* prop = getViewProviderPtr()->getDynamicPropertyByName(attr);
    if (prop)
        return prop->getPyObject();
    else
        return 0;
}

int ViewProviderPy::setCustomAttributes(const char* attr, PyObject* value)
{
    // search for dynamic property
    try {
        App::Property* prop = getViewProviderPtr()->getDynamicPropertyByName(attr);
        if (prop) {
            prop->setPyObject(value);
            return 1;
        }
        return 0;
    }
    catch (Base::Exception &exc) {
        PyErr_Format(PyExc_AttributeError, "Attribute (Name: %s) error: '%s' ", attr, exc.what());
        return -1;
    }
    catch (...) {
        PyErr_Format(PyExc_AttributeError, "Unknown error in attribute %s", attr);
        return -1;
    }
}

Py::Object ViewProviderPy::getAnnotation(void) const
{
    try {
        SoNode* node = getViewProviderPtr()->getAnnotation();
        PyObject* Ptr = Base::Interpreter().createSWIGPointerObj("pivy.coin", "SoSeparator *", node, 1);
        node->ref();
        return Py::Object(Ptr, true);
    }
    catch (const Base::Exception& e) {
        throw Py::RuntimeError(e.what());
    }
}

void  ViewProviderPy::setAnnotation(Py::Object)
{

}

Py::Object ViewProviderPy::getRootNode(void) const
{
    try {
        SoNode* node = getViewProviderPtr()->getRoot();
        PyObject* Ptr = Base::Interpreter().createSWIGPointerObj("pivy.coin","SoSeparator *", node, 1);
        node->ref();
        return Py::Object(Ptr, true);
    }
    catch (const Base::Exception& e) {
        throw Py::RuntimeError(e.what());
    }
}

void  ViewProviderPy::setRootNode(Py::Object)
{

}

static char * buffer;
static size_t buffer_size = 0;

static void *
buffer_realloc(void * bufptr, size_t size)
{
    buffer = (char *)realloc(bufptr, size);
    buffer_size = size;
    return buffer;
}

static SbString
buffer_writeaction(SoNode * root)
{
    SoOutput out;
    buffer = (char *)malloc(1024);
    buffer_size = 1024;
    out.setBuffer(buffer, buffer_size, buffer_realloc);

    SoWriteAction wa(&out);
    wa.apply(root);

    SbString s(buffer);
    free(buffer);
    return s;
}

Py::String ViewProviderPy::getIV(void) const
{
    SbString buf = buffer_writeaction(getViewProviderPtr()->getRoot());
    return Py::String(buf.getString());
}

Py::Object ViewProviderPy::getIcon(void) const
{
#if 0
    QByteArray ba;
    QDataStream str(&ba, QIODevice::WriteOnly);
    QIcon icon = getViewProviderPtr()->getIcon();
    str << icon;
    return Py::String(ba.constData(), ba.size());
#else
    PythonWrapper wrap;
    wrap.loadGuiModule();
    wrap.loadWidgetsModule();
    QIcon icon = getViewProviderPtr()->getIcon();
    return wrap.fromQIcon(new QIcon(icon));
#endif
}
