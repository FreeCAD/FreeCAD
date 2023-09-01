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
# include <Inventor/SbRotation.h>
# include <Inventor/SoFullPath.h>
# include <Inventor/details/SoDetail.h>
# include <Inventor/nodes/SoSeparator.h>
# include <Inventor/nodes/SoSwitch.h>
# include <QByteArray>
# include <QDataStream>
#endif

#include <Base/BoundBoxPy.h>
#include <Base/PyWrapParseTupleAndKeywords.h>

#include "PythonWrapper.h"
#include "SoFCDB.h"

// inclusion of the generated files (generated out of ViewProviderPy.xml)
#include <Gui/ViewProviderPy.h>
#include <Gui/ViewProviderPy.cpp>
#include <Gui/View3DPy.h>
#include <Gui/View3DInventor.h>
#include <Base/Interpreter.h>
#include <Base/Matrix.h>
#include <Base/MatrixPy.h>
#include <Base/Placement.h>
#include <Base/PlacementPy.h>
#include <App/Document.h>
#include <App/DocumentObject.h>
#include <App/DocumentObjectPy.h>


using namespace Gui;

// returns a string which represent the object e.g. when printed in python
std::string ViewProviderPy::representation() const
{
    return "<View provider object>";
}

PyObject*  ViewProviderPy::addProperty(PyObject *args)
{
    char *sType,*sName=nullptr,*sGroup=nullptr,*sDoc=nullptr;
    short attr=0;
    std::string sDocStr;
    PyObject *ro = Py_False, *hd = Py_False;
    if (!PyArg_ParseTuple(args, "s|ssethO!O!", &sType,&sName,&sGroup,"utf-8",&sDoc,&attr,
        &PyBool_Type, &ro, &PyBool_Type, &hd))
        return nullptr;

    if (sDoc) {
        sDocStr = sDoc;
        PyMem_Free(sDoc);
    }

    App::Property* prop=nullptr;
    try {
        prop = getViewProviderPtr()->addDynamicProperty(sType,sName,sGroup,sDocStr.c_str(),attr,
            Base::asBoolean(ro), Base::asBoolean(hd));
    }
    catch (const Base::Exception& e) {
        throw Py::RuntimeError(e.what());
    }
    if (!prop) {
        std::stringstream str;
        str << "No property found of type '" << sType << "'" << std::ends;
        throw Py::TypeError(str.str());
    }

    return Py::new_reference_to(this);
}

PyObject*  ViewProviderPy::removeProperty(PyObject *args)
{
    char *sName;
    if (!PyArg_ParseTuple(args, "s", &sName))
        return nullptr;

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
    if (!PyArg_ParseTuple(args, ""))
        return nullptr;

    std::vector<Base::Type> ary;
    Base::Type::getAllDerivedFrom(App::Property::getClassTypeId(), ary);
    Py::List res;
    for (auto & it : ary) {
        auto data = static_cast<Base::BaseClass*>(it.createInstance());
        if (data) {
            delete data;
            res.append(Py::String(it.getName()));
        }
    }
    return Py::new_reference_to(res);
}

PyObject*  ViewProviderPy::show(PyObject *args)
{
    if (!PyArg_ParseTuple(args, ""))
        return nullptr;

    PY_TRY {
        getViewProviderPtr()->show();
        Py_Return;
    }
    PY_CATCH;
}

PyObject*  ViewProviderPy::hide(PyObject *args)
{
    if (!PyArg_ParseTuple(args, ""))
        return nullptr;

    PY_TRY {
        getViewProviderPtr()->hide();
        Py_Return;
    }
    PY_CATCH;
}

PyObject*  ViewProviderPy::isVisible(PyObject *args)
{
    if (!PyArg_ParseTuple(args, ""))
        return nullptr;

    PY_TRY {
        return Py::new_reference_to(Py::Boolean(getViewProviderPtr()->isShow()));
    }
    PY_CATCH;
}

PyObject*  ViewProviderPy::canDragObject(PyObject *args)
{
    PyObject *obj = Py_None;
    if (!PyArg_ParseTuple(args, "|O", &obj))
        return nullptr;

    PY_TRY {
        Base::PyTypeCheck(&obj, &App::DocumentObjectPy::Type);
        bool ret;
        if (!obj)
            ret = getViewProviderPtr()->canDragObjects();
        else
            ret = getViewProviderPtr()->canDragObject(
                    static_cast<App::DocumentObjectPy*>(obj)->getDocumentObjectPtr());

        return Py::new_reference_to(Py::Boolean(ret));
    }
    PY_CATCH;
}

PyObject*  ViewProviderPy::canDropObject(PyObject *args, PyObject *kw)
{
    PyObject *obj = Py_None;
    PyObject *owner = Py_None;
    PyObject *pyElements = Py_None;
    const char *subname = nullptr;
    static const std::array<const char *, 5> kwlist{"obj", "owner", "subname", "elem", nullptr};
    if (!Base::Wrapped_ParseTupleAndKeywords(args, kw, "|OOsO", kwlist,
                                            &obj, &owner, &subname, &pyElements)) {
        return nullptr;
    }

    PY_TRY {
        Base::PyTypeCheck(&obj, &App::DocumentObjectPy::Type, "expecting 'obj' to be of type App.DocumentObject or None");
        Base::PyTypeCheck(&owner, &App::DocumentObjectPy::Type, "expecting 'owner' to be of type App.DocumentObject or None");
        Base::PyTypeCheck(&pyElements, PySequence_Check, "expecting 'elem' to be sequence or None");

        bool ret;
        App::DocumentObject* pcObject;
        App::DocumentObject* pcOwner = nullptr;
        App::PropertyStringList elements;
        if (!obj && (owner || pyElements || subname)) {
            PyErr_SetString(PyExc_ValueError, "'obj' must be specified if 'owner', 'subname' or 'elem' is given");
            return nullptr;
        }
        if(!obj) {
            ret = getViewProviderPtr()->canDropObjects();
            return Py::new_reference_to(Py::Boolean(ret));
        }
        pcObject = static_cast<App::DocumentObjectPy*>(obj)->getDocumentObjectPtr();
        if (owner)
            pcOwner = static_cast<App::DocumentObjectPy*>(owner)->getDocumentObjectPtr();
        if (pyElements) {
            try {
                elements.setPyObject(pyElements);
            }
            catch(...) {
                PyErr_SetString(PyExc_TypeError, "'elem' must be a sequence of strings");
                return nullptr;
            }
        }
        ret = getViewProviderPtr()->canDropObjectEx(pcObject,pcOwner,subname,elements.getValues());
        return Py::new_reference_to(Py::Boolean(ret));
    }
    PY_CATCH;
}

PyObject*  ViewProviderPy::canDragAndDropObject(PyObject *args)
{
    PyObject *obj;
    if (!PyArg_ParseTuple(args, "O!", &App::DocumentObjectPy::Type,&obj))
        return nullptr;

    PY_TRY {
        bool ret = getViewProviderPtr()->canDragAndDropObject(
                    static_cast<App::DocumentObjectPy*>(obj)->getDocumentObjectPtr());
        return Py::new_reference_to(Py::Boolean(ret));
    }
    PY_CATCH;
}

PyObject*  ViewProviderPy::dropObject(PyObject *args, PyObject *kw)
{
    PyObject *obj;
    PyObject *owner = Py_None;
    PyObject *pyElements = Py_None;
    const char *subname = nullptr;
    static const std::array<const char *, 5> kwlist{"obj", "owner", "subname", "elem", nullptr};
    if (!Base::Wrapped_ParseTupleAndKeywords(args, kw, "O!|OsO", kwlist,
                                             &App::DocumentObjectPy::Type, &obj, &owner, &subname, &pyElements)) {
        return nullptr;
    }

    PY_TRY {
        Base::PyTypeCheck(&owner, &App::DocumentObjectPy::Type, "expecting 'owner' to be of type App.DocumentObject or None");
        Base::PyTypeCheck(&pyElements, PySequence_Check, "expecting 'elem' to be sequence or None");

        auto pcObject = static_cast<App::DocumentObjectPy*>(obj)->getDocumentObjectPtr();
        App::DocumentObject *pcOwner = nullptr;
        App::PropertyStringList elements;
        if (owner)
            pcOwner = static_cast<App::DocumentObjectPy*>(owner)->getDocumentObjectPtr();
        if (pyElements) {
            try {
                elements.setPyObject(pyElements);
            }
            catch(...) {
                PyErr_SetString(PyExc_TypeError, "'elem' must be a sequence of strings");
                return nullptr;
            }
        }
        auto ret = getViewProviderPtr()->dropObjectEx(pcObject,pcOwner, subname,elements.getValues());
        return Py::new_reference_to(Py::String(ret));
    }
    PY_CATCH;
}

PyObject*  ViewProviderPy::dragObject(PyObject *args)
{
    PyObject *obj;
    if (!PyArg_ParseTuple(args, "O!", &App::DocumentObjectPy::Type,&obj))
        return nullptr;

    PY_TRY {
        getViewProviderPtr()->dragObject(
                static_cast<App::DocumentObjectPy*>(obj)->getDocumentObjectPtr());
        Py_Return;
    }
    PY_CATCH;
}

PyObject* ViewProviderPy::replaceObject(PyObject *args)
{
    PyObject *oldObj;
    PyObject *newObj;
    if (!PyArg_ParseTuple(args, "O!O!",
                &App::DocumentObjectPy::Type,&oldObj,
                &App::DocumentObjectPy::Type,&newObj))
        return nullptr;

    PY_TRY {
        int ret = getViewProviderPtr()->replaceObject(
                static_cast<App::DocumentObjectPy*>(oldObj)->getDocumentObjectPtr(),
                static_cast<App::DocumentObjectPy*>(newObj)->getDocumentObjectPtr());
        return Py::new_reference_to(Py::Int(ret));
    }
    PY_CATCH;
}

PyObject* ViewProviderPy::addDisplayMode(PyObject * args)
{
    char* mode;
    PyObject* obj;
    if (!PyArg_ParseTuple(args, "Os", &obj, &mode))
        return nullptr;

    void* ptr = nullptr;
    try {
        Base::Interpreter().convertSWIGPointerObj("pivy.coin","_p_SoNode", obj, &ptr, 0);
    }
    catch (const Base::Exception& e) {
        PyErr_SetString(PyExc_RuntimeError, e.what());
        return nullptr;
    }

    PY_TRY {
        auto node = static_cast<SoNode*>(ptr);
        getViewProviderPtr()->addDisplayMaskMode(node,mode);
        Py_Return;
    }
    PY_CATCH;
}

PyObject*  ViewProviderPy::listDisplayModes(PyObject *args)
{
    if (!PyArg_ParseTuple(args, ""))
        return nullptr;

    PY_TRY {
        std::vector<std::string> modes = getViewProviderPtr()->getDisplayModes();
        PyObject* pyList = PyList_New(modes.size());
        int i=0;

        for (const auto & mode : modes) {
            PyObject* str = PyUnicode_FromString(mode.c_str());
            PyList_SetItem(pyList, i++, str);
        }

        return pyList;
    }
    PY_CATCH;
}

PyObject*  ViewProviderPy::toString(PyObject *args)
{
    if (!PyArg_ParseTuple(args, ""))
        return nullptr;

    PY_TRY {
        std::string buffer = getViewProviderPtr()->toString();
        return Py::new_reference_to(Py::String(buffer));
    }
    PY_CATCH;
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
        auto plc = static_cast<Base::PlacementPy*>(p);
        getViewProviderPtr()->setTransformation(plc->getPlacementPtr()->toMatrix());
        Py_Return;
    }

    PyErr_SetString(PyExc_TypeError, "The transformation must be a Base.Matrix or a Base.Placement");
    return nullptr;
}

PyObject* ViewProviderPy::claimChildren(PyObject* args)
{
    if (!PyArg_ParseTuple(args, ""))
        return nullptr;

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

PyObject* ViewProviderPy::partialRender(PyObject* args)
{
    PyObject *value = Py_None;
    PyObject *clear = Py_False;
    if (!PyArg_ParseTuple(args, "|OO!",&value,&PyBool_Type,&clear))
        return nullptr;

    std::vector<std::string> values;
    if(value != Py_None) {
        PyObject *item = nullptr;
        Py_ssize_t nSize;
        if (PyList_Check(value) || PyTuple_Check(value))
            nSize = PySequence_Size(value);
        else {
            item = value;
            value = nullptr;
            nSize = 1;
        }
        values.resize(nSize);
        for (Py_ssize_t i = 0; i < nSize; ++i) {
            if(value)
                item = PySequence_GetItem(value, i);
            if (PyUnicode_Check(item)) {
                values[i] = PyUnicode_AsUTF8(item);
            }
            else {
                std::string error = std::string("type must be str");
                error += " not, ";
                error += item->ob_type->tp_name;
                throw Base::TypeError(error);
            }
        }
    }

    Py::Int ret(getViewProviderPtr()->partialRender(values, Base::asBoolean(clear)));
    return Py::new_reference_to(ret);
}

PyObject* ViewProviderPy::getElementColors(PyObject* args)
{
    const char *element = nullptr;
    if (!PyArg_ParseTuple(args, "|s", &element))
        return nullptr;

    Py::Dict dict;
    for(auto &v : getViewProviderPtr()->getElementColors(element)) {
        auto &c = v.second;
        dict.setItem(Py::String(v.first),
                Py::TupleN(Py::Float(c.r),Py::Float(c.g),Py::Float(c.b),Py::Float(c.a)));
    }
    return Py::new_reference_to(dict);
}

PyObject* ViewProviderPy::setElementColors(PyObject* args)
{
    PyObject *pyObj;
    if (!PyArg_ParseTuple(args, "O", &pyObj))
        return nullptr;

    if(!PyDict_Check(pyObj))
        throw Py::TypeError("Expect a dict");

    std::map<std::string,App::Color> colors;
    Py::Dict dict(pyObj);
    for(auto it=dict.begin();it!=dict.end();++it) {
        const auto &value = *it;
        if(!value.first.isString() || !value.second.isSequence())
            throw Py::TypeError("Expect the dictionary to contain items of type elementName:(r,g,b,a)");

        App::PropertyColor prop;
        prop.setPyObject(value.second.ptr());
        colors[value.first.as_string()] = prop.getValue();
    }
    getViewProviderPtr()->setElementColors(colors);
    Py_Return;
}

PyObject* ViewProviderPy::getElementPicked(PyObject* args)
{
    PyObject *obj;
    if (!PyArg_ParseTuple(args, "O",&obj))
        return nullptr;

    void *ptr = nullptr;
    Base::Interpreter().convertSWIGPointerObj("pivy.coin", "_p_SoPickedPoint", obj, &ptr, 0);
    auto pp = static_cast<SoPickedPoint*>(ptr);
    if(!pp)
        throw Base::TypeError("type must be coin.SoPickedPoint");

    std::string name;
    if(!getViewProviderPtr()->getElementPicked(pp,name))
        Py_Return;

    return Py::new_reference_to(Py::String(name));
}

PyObject* ViewProviderPy::getDetailPath(PyObject* args)
{
    const char *sub;
    PyObject *path;
    PyObject *append = Py_True;
    if (!PyArg_ParseTuple(args, "sO|O!",&sub,&path,&PyBool_Type,&append))
        return nullptr;

    void *ptr = nullptr;
    Base::Interpreter().convertSWIGPointerObj("pivy.coin", "_p_SoPath", path, &ptr, 0);
    auto pPath = static_cast<SoPath*>(ptr);
    if(!pPath)
        throw Base::TypeError("'path' must be a coin.SoPath");
    SoDetail *det = nullptr;
    if(!getViewProviderPtr()->getDetailPath(sub,static_cast<SoFullPath*>(pPath),append,det)) {
        delete det;
        Py_Return;
    }
    if(!det)
        Py_Return;
    return Base::Interpreter().createSWIGPointerObj("pivy.coin", "_p_SoDetail", static_cast<void*>(det), 0);
}

PyObject *ViewProviderPy::signalChangeIcon(PyObject *args)
{
    if (!PyArg_ParseTuple(args, ""))
        return nullptr;

    getViewProviderPtr()->signalChangeIcon();
    Py_Return;
}

PyObject *ViewProviderPy::getBoundingBox(PyObject *args) {
    PyObject *transform=Py_True;
    PyObject *pyView = nullptr;
    const char *subname = nullptr;
    if (!PyArg_ParseTuple(args, "|sO!O!", &subname,&PyBool_Type,&transform,View3DInventorPy::type_object(),&pyView))
        return nullptr;

    PY_TRY {
        View3DInventor *view = nullptr;
        if(pyView)
            view = static_cast<View3DInventorPy*>(pyView)->getView3DIventorPtr();
        auto bbox = getViewProviderPtr()->getBoundingBox(subname, Base::asBoolean(transform), view);
        return new Base::BoundBoxPy(new Base::BoundBox3d(bbox));
    }
    PY_CATCH;
}

PyObject *ViewProviderPy::doubleClicked(PyObject *args) {
    if(!PyArg_ParseTuple(args, ""))
        return nullptr;

    PY_TRY {
        return Py::new_reference_to(Py::Boolean(getViewProviderPtr()->doubleClicked()));
    }
    PY_CATCH;
}

PyObject *ViewProviderPy::getCustomAttributes(const char* attr) const
{
    // search for dynamic property
    App::Property* prop = getViewProviderPtr()->getDynamicPropertyByName(attr);
    if (prop)
        return prop->getPyObject();
    else
        return nullptr;
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

Py::Object ViewProviderPy::getAnnotation() const
{
    try {
        auto node = getViewProviderPtr()->getAnnotation();
        PyObject* Ptr = Base::Interpreter().createSWIGPointerObj("pivy.coin", "_p_SoSeparator", node, 1);
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

Py::Object ViewProviderPy::getRootNode() const
{
    try {
        SoSeparator* node = getViewProviderPtr()->getRoot();
        PyObject* Ptr = Base::Interpreter().createSWIGPointerObj("pivy.coin","_p_SoSeparator", node, 1);
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

Py::Object ViewProviderPy::getSwitchNode() const
{
    try {
        SoSwitch* node = getViewProviderPtr()->getModeSwitch();
        PyObject* Ptr = Base::Interpreter().createSWIGPointerObj("pivy.coin","_p_SoSwitch", node, 1);
        node->ref();
        return Py::Object(Ptr, true);
    }
    catch (const Base::Exception& e) {
        throw Py::RuntimeError(e.what());
    }
}

void  ViewProviderPy::setSwitchNode(Py::Object)
{

}

Py::String ViewProviderPy::getIV() const
{
    std::string buf = Gui::SoFCDB::writeNodesToString(getViewProviderPtr()->getRoot());
    return {buf};
}

Py::Object ViewProviderPy::getIcon() const
{
    PythonWrapper wrap;
    wrap.loadGuiModule();
    wrap.loadWidgetsModule();
    QIcon icon = getViewProviderPtr()->getIcon();
    return wrap.fromQIcon(new QIcon(icon));
}

Py::Int ViewProviderPy::getDefaultMode() const
{
    return Py::Int((long)getViewProviderPtr()->getDefaultMode());
}

void ViewProviderPy::setDefaultMode(Py::Int arg)
{
    return getViewProviderPtr()->setDefaultMode(arg);
}

Py::Boolean ViewProviderPy::getCanRemoveChildrenFromRoot() const
{
    return {getViewProviderPtr()->canRemoveChildrenFromRoot()};
}

Py::Boolean ViewProviderPy::getLinkVisibility() const
{
    return {getViewProviderPtr()->isLinkVisible()};
}

void ViewProviderPy::setLinkVisibility(Py::Boolean arg)
{
    getViewProviderPtr()->setLinkVisible(arg);
}

Py::String ViewProviderPy::getDropPrefix() const
{
    return {getViewProviderPtr()->getDropPrefix()};
}
