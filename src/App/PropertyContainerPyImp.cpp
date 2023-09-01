/***************************************************************************
 *   Copyright (c) 2007 Jürgen Riegel <juergen.riegel@web.de>              *
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
#endif

#include "PropertyContainer.h"
#include "Property.h"
#include "DocumentObject.h"
#include <Base/PyWrapParseTupleAndKeywords.h>

#include <boost/iostreams/device/array.hpp>
#include <boost/iostreams/stream.hpp>

// inclusion of the generated files (generated out of PropertyContainerPy.xml)
#include "PropertyContainerPy.h"
#include "PropertyContainerPy.cpp"

FC_LOG_LEVEL_INIT("Property", true, 2)

using namespace App;

// returns a string which represent the object e.g. when printed in python
std::string PropertyContainerPy::representation() const
{
    return {"<property container>"};
}

PyObject*  PropertyContainerPy::getPropertyByName(PyObject *args)
{
    char *pstr;
    int checkOwner=0;
    if (!PyArg_ParseTuple(args, "s|i", &pstr, &checkOwner))
        return nullptr;

    if (checkOwner < 0 || checkOwner > 2) {
        PyErr_SetString(PyExc_ValueError, "'checkOwner' expected in the range [0, 2]");
        return nullptr;
    }

    App::Property* prop = getPropertyContainerPtr()->getPropertyByName(pstr);
    if (!prop) {
        PyErr_Format(PyExc_AttributeError, "Property container has no property '%s'", pstr);
        return nullptr;
    }

    if (!checkOwner || (checkOwner==1 && prop->getContainer()==getPropertyContainerPtr()))
        return prop->getPyObject();

    Py::TupleN res(Py::asObject(prop->getContainer()->getPyObject()), Py::asObject(prop->getPyObject()));

    return Py::new_reference_to(res);
}

PyObject*  PropertyContainerPy::getPropertyTouchList(PyObject *args)
{
    char *pstr;
    if (!PyArg_ParseTuple(args, "s", &pstr))
        return nullptr;

    App::Property* prop = getPropertyContainerPtr()->getPropertyByName(pstr);
    if (prop && prop->isDerivedFrom(PropertyLists::getClassTypeId())) {
        const auto &touched = static_cast<PropertyLists*>(prop)->getTouchList();
        Py::Tuple ret(touched.size());
        int i=0;
        for(int idx : touched)
            ret.setItem(i++,Py::Long(idx));
        return Py::new_reference_to(ret);
    }
    else if (!prop) {
        PyErr_Format(PyExc_AttributeError, "Property container has no property '%s'", pstr);
        return nullptr;
    }
    else {
        PyErr_Format(PyExc_AttributeError, "Property '%s' is not of list type", pstr);
        return nullptr;
    }
}

PyObject*  PropertyContainerPy::getTypeOfProperty(PyObject *args)
{
    Py::List ret;
    char *pstr;
    if (!PyArg_ParseTuple(args, "s", &pstr))
        return nullptr;

    Property* prop =  getPropertyContainerPtr()->getPropertyByName(pstr);
    if (!prop) {
        PyErr_Format(PyExc_AttributeError, "Property container has no property '%s'", pstr);
        return nullptr;
    }

    short Type =  prop->getType();
    if (Type & Prop_ReadOnly)
        ret.append(Py::String("ReadOnly"));
    if (Type & Prop_Transient)
        ret.append(Py::String("Transient"));
    if (Type & Prop_Hidden)
        ret.append(Py::String("Hidden"));
    if (Type & Prop_Output)
        ret.append(Py::String("Output"));
    if (Type & Prop_NoRecompute)
        ret.append(Py::String("NoRecompute"));
    if (Type & Prop_NoPersist)
        ret.append(Py::String("NoPersist"));

    return Py::new_reference_to(ret);
}

PyObject*  PropertyContainerPy::getTypeIdOfProperty(PyObject *args)
{
    char *pstr;
    if (!PyArg_ParseTuple(args, "s", &pstr))
        return nullptr;

    Property* prop =  getPropertyContainerPtr()->getPropertyByName(pstr);
    if (!prop) {
        PyErr_Format(PyExc_AttributeError, "Property container has no property '%s'", pstr);
        return nullptr;
    }

    Py::String str(prop->getTypeId().getName());
    return Py::new_reference_to(str);
}

PyObject*  PropertyContainerPy::setEditorMode(PyObject *args)
{
    char* name;
    short type;
    if (PyArg_ParseTuple(args, "sh", &name, &type)) {
        App::Property* prop = getPropertyContainerPtr()->getPropertyByName(name);
        if (!prop) {
            PyErr_Format(PyExc_AttributeError, "Property container has no property '%s'", name);
            return nullptr;
        }

        std::bitset<32> status(prop->getStatus());
        status.set(Property::ReadOnly, (type & 1) > 0);
        status.set(Property::Hidden, (type & 2) > 0);
        prop->setStatusValue(status.to_ulong());

        Py_Return;
    }

    PyErr_Clear();
    PyObject *iter;
    if (PyArg_ParseTuple(args, "sO", &name, &iter)) {
        if (PyTuple_Check(iter) || PyList_Check(iter)) {
            Py::Sequence seq(iter);
            App::Property* prop = getPropertyContainerPtr()->getPropertyByName(name);
            if (!prop) {
                PyErr_Format(PyExc_AttributeError, "Property container has no property '%s'", name);
                return nullptr;
            }

            // reset all bits first
            std::bitset<32> status(prop->getStatus());
            status.reset(Property::ReadOnly);
            status.reset(Property::Hidden);
            for (Py::Sequence::iterator it = seq.begin();it!=seq.end();++it) {
                std::string str = static_cast<std::string>(Py::String(*it));
                if (str == "ReadOnly")
                    status.set(Property::ReadOnly);
                else if (str == "Hidden")
                    status.set(Property::Hidden);
            }
            prop->setStatusValue(status.to_ulong());

            Py_Return;
        }
    }

    PyErr_SetString(PyExc_TypeError, "First argument must be str, second can be int, list or tuple");
    return nullptr;
}

static const std::map<std::string, int> &getStatusMap() {
    static std::map<std::string,int> statusMap;
    if(statusMap.empty()) {
        statusMap["Immutable"] = Property::Immutable;
        statusMap["ReadOnly"] = Property::ReadOnly;
        statusMap["Hidden"] = Property::Hidden;
        statusMap["Transient"] = Property::Transient;
        statusMap["MaterialEdit"] = Property::MaterialEdit;
        statusMap["NoMaterialListEdit"] = Property::NoMaterialListEdit;
        statusMap["Output"] = Property::Output;
        statusMap["LockDynamic"] = Property::LockDynamic;
        statusMap["NoModify"] = Property::NoModify;
        statusMap["PartialTrigger"] = Property::PartialTrigger;
        statusMap["NoRecompute"] = Property::NoRecompute;
        statusMap["CopyOnChange"] = Property::CopyOnChange;
        statusMap["UserEdit"] = Property::UserEdit;
    }
    return statusMap;
}

PyObject*  PropertyContainerPy::setPropertyStatus(PyObject *args)
{
    char* name;
    PyObject *pyValue;
    if (!PyArg_ParseTuple(args, "sO", &name, &pyValue))
        return nullptr;

    App::Property* prop = getPropertyContainerPtr()->getPropertyByName(name);
    if (!prop) {
        PyErr_Format(PyExc_AttributeError, "Property container has no property '%s'", name);
        return nullptr;
    }

    auto linkProp = Base::freecad_dynamic_cast<App::PropertyLinkBase>(prop);
    std::bitset<32> status(prop->getStatus());
    size_t count = 1;
    bool isSeq = false;
    if (PyList_Check(pyValue) || PyTuple_Check(pyValue)) {
        isSeq = true;
        count = PySequence_Size(pyValue);
    }

    for(size_t i=0; i<count; ++i) {
        Py::Object item;
        if (isSeq)
            item = Py::Object(PySequence_GetItem(pyValue,i));
        else
            item = Py::Object(pyValue);
        bool value = true;
        if (item.isString()) {
            const auto &statusMap = getStatusMap();
            auto v = static_cast<std::string>(Py::String(item));
            if(v.size()>1 && v[0] == '-') {
                value = false;
                v = v.substr(1);
            }
            auto it = statusMap.find(v);
            if(it == statusMap.end()) {
                if(linkProp && v == "AllowPartial") {
                    linkProp->setAllowPartial(value);
                    continue;
                }
                PyErr_Format(PyExc_ValueError, "Unknown property status '%s'", v.c_str());
                return nullptr;
            }
            status.set(it->second,value);
        }
        else if (item.isNumeric()) {
            int v = Py::Int(item);
            if(v<0) {
                value = false;
                v = -v;
            }
            if(v==0 || v>31)
                PyErr_Format(PyExc_ValueError, "Status value out of range '%d'", v);
            status.set(v,value);
        }
        else {
            PyErr_SetString(PyExc_TypeError, "Expects status type to be Int or String");
            return nullptr;
        }
    }

    prop->setStatusValue(status.to_ulong());
    Py_Return;
}

PyObject*  PropertyContainerPy::getPropertyStatus(PyObject *args)
{
    char* name = "";
    if (!PyArg_ParseTuple(args, "|s", &name))
        return nullptr;

    Py::List ret;
    const auto &statusMap = getStatusMap();
    if (!name[0]) {
        for(auto &v : statusMap)
            ret.append(Py::String(v.first.c_str()));
    }
    else {
        App::Property* prop = getPropertyContainerPtr()->getPropertyByName(name);
        if (!prop) {
            PyErr_Format(PyExc_AttributeError, "Property container has no property '%s'", name);
            return nullptr;
        }

        auto linkProp = Base::freecad_dynamic_cast<App::PropertyLinkBase>(prop);
        if (linkProp && linkProp->testFlag(App::PropertyLinkBase::LinkAllowPartial))
            ret.append(Py::String("AllowPartial"));

        std::bitset<32> bits(prop->getStatus());
        for(size_t i=1; i<bits.size(); ++i) {
            if(!bits[i]) continue;
            bool found = false;
            for(auto &v : statusMap) {
                if(v.second == static_cast<int>(i)) {
                    ret.append(Py::String(v.first.c_str()));
                    found = true;
                    break;
                }
            }
            if (!found)
                ret.append(Py::Int(static_cast<long>(i)));
        }
    }
    return Py::new_reference_to(ret);
}

PyObject*  PropertyContainerPy::getEditorMode(PyObject *args)
{
    char* name;
    if (!PyArg_ParseTuple(args, "s", &name))
        return nullptr;

    App::Property* prop = getPropertyContainerPtr()->getPropertyByName(name);
    if (!prop) {
        PyErr_Format(PyExc_AttributeError, "Property container has no property '%s'", name);
        return nullptr;
    }

    Py::List ret;
    if (prop) {
        short Type =  prop->getType();
        if ((prop->testStatus(Property::ReadOnly)) || (Type & Prop_ReadOnly))
            ret.append(Py::String("ReadOnly"));
        if ((prop->testStatus(Property::Hidden)) || (Type & Prop_Hidden))
            ret.append(Py::String("Hidden"));
    }
    return Py::new_reference_to(ret);
}

PyObject*  PropertyContainerPy::getGroupOfProperty(PyObject *args)
{
    char *pstr;
    if (!PyArg_ParseTuple(args, "s", &pstr))
        return nullptr;

    Property* prop = getPropertyContainerPtr()->getPropertyByName(pstr);
    if (!prop) {
        PyErr_Format(PyExc_AttributeError, "Property container has no property '%s'", pstr);
        return nullptr;
    }

    const char* Group = getPropertyContainerPtr()->getPropertyGroup(prop);
    if (Group)
        return Py::new_reference_to(Py::String(Group));
    else
        return Py::new_reference_to(Py::String(""));
}

PyObject*  PropertyContainerPy::setGroupOfProperty(PyObject *args)
{
    char *pstr;
    char *group;
    if (!PyArg_ParseTuple(args, "ss", &pstr, &group))
        return nullptr;

    PY_TRY {
        Property* prop = getPropertyContainerPtr()->getDynamicPropertyByName(pstr);
        if (!prop) {
            PyErr_Format(PyExc_AttributeError, "Property container has no dynamic property '%s'", pstr);
            return nullptr;
        }
        prop->getContainer()->changeDynamicProperty(prop,group,nullptr);
        Py_Return;
    }
    PY_CATCH
}


PyObject*  PropertyContainerPy::getDocumentationOfProperty(PyObject *args)
{
    char *pstr;
    if (!PyArg_ParseTuple(args, "s", &pstr))
        return nullptr;

    Property* prop = getPropertyContainerPtr()->getPropertyByName(pstr);
    if (!prop) {
        PyErr_Format(PyExc_AttributeError, "Property container has no property '%s'", pstr);
        return nullptr;
    }

    const char* docstr = getPropertyContainerPtr()->getPropertyDocumentation(prop);
    if (docstr)
        return Py::new_reference_to(Py::String(docstr));
    else
        return Py::new_reference_to(Py::String(""));
}

PyObject*  PropertyContainerPy::setDocumentationOfProperty(PyObject *args)
{
    char *pstr;
    char *doc;
    if (!PyArg_ParseTuple(args, "ss", &pstr, &doc))
        return nullptr;

    PY_TRY {
        Property* prop = getPropertyContainerPtr()->getDynamicPropertyByName(pstr);
        if (!prop) {
            PyErr_Format(PyExc_AttributeError, "Property container has no dynamic property '%s'", pstr);
            return nullptr;
        }
        prop->getContainer()->changeDynamicProperty(prop,nullptr,doc);
        Py_Return;
    }
    PY_CATCH
}

PyObject*  PropertyContainerPy::getEnumerationsOfProperty(PyObject *args)
{
    char *pstr;
    if (!PyArg_ParseTuple(args, "s", &pstr))
        return nullptr;

    Property* prop = getPropertyContainerPtr()->getPropertyByName(pstr);
    if (!prop) {
        PyErr_Format(PyExc_AttributeError, "Property container has no property '%s'", pstr);
        return nullptr;
    }

    PropertyEnumeration *enumProp = dynamic_cast<PropertyEnumeration*>(prop);
    if (!enumProp)
        Py_Return;

    std::vector<std::string> enumerations = enumProp->getEnumVector();
    Py::List ret;
    for (const auto & it : enumerations) {
        ret.append(Py::String(it));
    }
    return Py::new_reference_to(ret);
}

Py::List PropertyContainerPy::getPropertiesList() const
{
    Py::List ret;
    std::map<std::string,Property*> Map;

    getPropertyContainerPtr()->getPropertyMap(Map);

    for (std::map<std::string,Property*>::const_iterator It=Map.begin(); It!=Map.end(); ++It)
        ret.append(Py::String(It->first));

    return ret;
}


PyObject* PropertyContainerPy::dumpPropertyContent(PyObject *args, PyObject *kwds)
{
    int compression = 3;
    const char* property;
    static const std::array<const char *, 3> kwds_def {"Property", "Compression", nullptr};
    PyErr_Clear();
    if (!Base::Wrapped_ParseTupleAndKeywords(args, kwds, "s|i", kwds_def, &property, &compression)) {
        return nullptr;
    }

    Property* prop = getPropertyContainerPtr()->getPropertyByName(property);
    if (!prop) {
        PyErr_Format(PyExc_AttributeError, "Property container has no property '%s'", property);
        return nullptr;
    }

    //setup the stream. the in flag is needed to make "read" work
    std::stringstream stream(std::stringstream::out | std::stringstream::in | std::stringstream::binary);
    try {
        prop->dumpToStream(stream, compression);
    }
    catch (...) {
       PyErr_SetString(PyExc_IOError, "Unable to parse content into binary representation");
       return nullptr;
    }

    //build the byte array with correct size
    if (!stream.seekp(0, stream.end)) {
        PyErr_SetString(PyExc_IOError, "Unable to find end of stream");
        return nullptr;
    }

    std::stringstream::pos_type offset = stream.tellp();
    if (!stream.seekg(0, stream.beg)) {
        PyErr_SetString(PyExc_IOError, "Unable to find begin of stream");
        return nullptr;
    }

    PyObject* ba = PyByteArray_FromStringAndSize(nullptr, offset);

    //use the buffer protocol to access the underlying array and write into it
    Py_buffer buf = Py_buffer();
    PyObject_GetBuffer(ba, &buf, PyBUF_WRITABLE);
    try {
        if(!stream.read((char*)buf.buf, offset)) {
            PyErr_SetString(PyExc_IOError, "Error copying data into byte array");
            return nullptr;
        }
        PyBuffer_Release(&buf);
    }
    catch (...) {
        PyBuffer_Release(&buf);
        PyErr_SetString(PyExc_IOError, "Error copying data into byte array");
        return nullptr;
    }

    return ba;
}

PyObject* PropertyContainerPy::restorePropertyContent(PyObject *args)
{
    PyObject* buffer;
    char* property;
    if( !PyArg_ParseTuple(args, "sO", &property, &buffer) )
        return nullptr;

    Property* prop = getPropertyContainerPtr()->getPropertyByName(property);
    if (!prop) {
        PyErr_Format(PyExc_AttributeError, "Property container has no property '%s'", property);
        return nullptr;
    }

    //check if it really is a buffer
    if( !PyObject_CheckBuffer(buffer) ) {
        PyErr_SetString(PyExc_TypeError, "Must be a buffer object");
        return nullptr;
    }

    Py_buffer buf;
    if(PyObject_GetBuffer(buffer, &buf, PyBUF_SIMPLE) < 0)
        return nullptr;

    if(!PyBuffer_IsContiguous(&buf, 'C')) {
        PyErr_SetString(PyExc_TypeError, "Buffer must be contiguous");
        return nullptr;
    }

    //check if it really is a buffer
    try {
        using Device = boost::iostreams::basic_array_source<char>;
        boost::iostreams::stream<Device> stream((char*)buf.buf, buf.len);
        prop->restoreFromStream(stream);
    }
    catch(...) {
        PyErr_SetString(PyExc_IOError, "Unable to restore content");
        return nullptr;
    }

    Py_Return;
}

PyObject *PropertyContainerPy::getCustomAttributes(const char* attr) const
{
    // search in PropertyList
    if(FC_LOG_INSTANCE.level()>FC_LOGLEVEL_TRACE) {
        FC_TRACE("Get property " << attr);
    }
    Property *prop = getPropertyContainerPtr()->getPropertyByName(attr);
    if (prop) {
        PyObject* pyobj = prop->getPyObject();
        if (!pyobj && PyErr_Occurred()) {
            // the Python exception is already set
            throw Py::Exception();
        }
        return pyobj;
    }
    else if (Base::streq(attr, "__dict__")) {
        // get the properties to the C++ PropertyContainer class
        std::map<std::string,App::Property*> Map;
        getPropertyContainerPtr()->getPropertyMap(Map);

        Py::Dict dict;
        for (const auto & it : Map) {
            dict.setItem(it.first, Py::String(""));
        }
        return Py::new_reference_to(dict);
    }
    ///FIXME: For v0.20: Do not use stuff from Part module here!
    else if(Base::streq(attr,"Shape") && getPropertyContainerPtr()->isDerivedFrom(App::DocumentObject::getClassTypeId())) {
        // Special treatment of Shape property
        static PyObject *_getShape = nullptr;
        if(!_getShape) {
            _getShape = Py_None;
            PyObject *mod = PyImport_ImportModule("Part");
            if(!mod) {
                PyErr_Clear();
            } else {
                Py::Object pyMod = Py::asObject(mod);
                if(pyMod.hasAttr("getShape"))
                    _getShape = Py::new_reference_to(pyMod.getAttr("getShape"));
            }
        }
        if(_getShape != Py_None) {
            Py::Tuple args(1);
            args.setItem(0,Py::Object(const_cast<PropertyContainerPy*>(this)));
            auto res = PyObject_CallObject(_getShape, args.ptr());
            if(!res)
                PyErr_Clear();
            else {
                Py::Object pyres(res,true);
                if(pyres.hasAttr("isNull")) {
                    Py::Callable func(pyres.getAttr("isNull"));
                    if(!func.apply().isTrue())
                        return Py::new_reference_to(res);
                }
            }
        }
    }

    return nullptr;
}

int PropertyContainerPy::setCustomAttributes(const char* attr, PyObject *obj)
{
    // search in PropertyList
    Property *prop = getPropertyContainerPtr()->getPropertyByName(attr);
    if (prop) {
        // Read-only attributes must not be set over its Python interface
        if(prop->testStatus(Property::Immutable)) {
            std::stringstream s;
            s << "Object attribute '" << attr << "' is read-only";
            throw Py::AttributeError(s.str());
        }

        FC_TRACE("Set property " << prop->getFullName());
        prop->setPyObject(obj);
        return 1;
    }

    return 0;
}
