
/***************************************************************************
 *   Copyright (c) Jürgen Riegel          (juergen.riegel@web.de) 2007     *
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
#include "PropertyLinks.h"
#include "Application.h"

#include <boost/iostreams/device/array.hpp>
#include <boost/iostreams/stream.hpp>

// inclution of the generated files (generated out of PropertyContainerPy.xml)
#include "PropertyContainerPy.h"
#include "PropertyContainerPy.cpp"

using namespace App;

// returns a string which represent the object e.g. when printed in python
std::string PropertyContainerPy::representation(void) const
{
    return std::string("<property container>");
}

PyObject*  PropertyContainerPy::getPropertyByName(PyObject *args)
{
    char *pstr;
    if (!PyArg_ParseTuple(args, "s", &pstr))     // convert args: Python->C
        return NULL;                             // NULL triggers exception
    App::Property* prop = getPropertyContainerPtr()->getPropertyByName(pstr);
    if (prop) {
        return prop->getPyObject();
    }
    else {
        PyErr_Format(PyExc_AttributeError, "Property container has no property '%s'", pstr);
        return NULL;
    }
}

PyObject*  PropertyContainerPy::getPropertyTouchList(PyObject *args)
{
    char *pstr;
    if (!PyArg_ParseTuple(args, "s", &pstr))     // convert args: Python->C
        return NULL;                             // NULL triggers exception
    App::Property* prop = getPropertyContainerPtr()->getPropertyByName(pstr);
    if (prop && prop->isDerivedFrom(PropertyLists::getClassTypeId())) {
        const auto &touched = static_cast<PropertyLists*>(prop)->getTouchList();
        Py::Tuple ret(touched.size());
        int i=0;
        for(int idx : touched)
            ret.setItem(i++,Py::Long(idx));
        return Py::new_reference_to(ret);
    }
    else if(!prop)
        PyErr_Format(PyExc_AttributeError, "Property container has no property '%s'", pstr);
    else
        PyErr_Format(PyExc_AttributeError, "Property '%s' is not of list type", pstr);
    return NULL;
}

PyObject*  PropertyContainerPy::getTypeOfProperty(PyObject *args)
{
    Py::List ret;
    char *pstr;
    if (!PyArg_ParseTuple(args, "s", &pstr))     // convert args: Python->C
        return NULL;                             // NULL triggers exception

    Property* prop =  getPropertyContainerPtr()->getPropertyByName(pstr);
    if (!prop) {
        PyErr_Format(PyExc_AttributeError, "Property container has no property '%s'", pstr);
        return 0;
    }

    short Type =  prop->getType();
    if (Type & Prop_Hidden)
        ret.append(Py::String("Hidden"));
    if (Type & Prop_ReadOnly)
        ret.append(Py::String("ReadOnly"));
    if (Type & Prop_Output)
        ret.append(Py::String("Output"));
    if (Type & Prop_NoRecompute)
        ret.append(Py::String("NoRecompute"));
    if (Type & Prop_Transient)
        ret.append(Py::String("Transient"));

    return Py::new_reference_to(ret);
}

PyObject*  PropertyContainerPy::getTypeIdOfProperty(PyObject *args)
{
    char *pstr;
    if (!PyArg_ParseTuple(args, "s", &pstr))     // convert args: Python->C
        return NULL;                             // NULL triggers exception

    Property* prop =  getPropertyContainerPtr()->getPropertyByName(pstr);
    if (!prop) {
        PyErr_Format(PyExc_AttributeError, "Property container has no property '%s'", pstr);
        return 0;
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
            return 0;
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
                return 0;
            }

            // reset all bits first
            std::bitset<32> status(prop->getStatus());
            status.reset(Property::ReadOnly);
            status.reset(Property::Hidden);
            for (Py::Sequence::iterator it = seq.begin();it!=seq.end();++it) {
                std::string str = (std::string)Py::String(*it);
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
    return 0;
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
    }
    return statusMap;
}

PyObject*  PropertyContainerPy::setPropertyStatus(PyObject *args)
{
    char* name;
    PyObject *pyValue;
    if (!PyArg_ParseTuple(args, "sO", &name, &pyValue))
        return 0;
    App::Property* prop = getPropertyContainerPtr()->getPropertyByName(name);
    if (!prop) {
        PyErr_Format(PyExc_AttributeError, "Property container has no property '%s'", name);
        return 0;
    }
    auto linkProp = Base::freecad_dynamic_cast<App::PropertyLinkBase>(prop);

    std::bitset<32> status(prop->getStatus());
    size_t count = 1;
    bool isSeq = false;
    if(PyList_Check(pyValue) || PyTuple_Check(pyValue)) {
        isSeq = true;
        count = PySequence_Size(pyValue);
    }
    for(size_t i=0;i<count;++i) {
        Py::Object item;
        if(isSeq)
            item = Py::Object(PySequence_GetItem(pyValue,i));
        else
            item = Py::Object(pyValue);
        bool value = true;
        if(item.isString()) {
            const auto &statusMap = getStatusMap();
            auto v = (std::string)Py::String(item);
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
                return 0;
            }
            status.set(it->second,value);
        }else if(item.isNumeric()) {
            int v = Py::Int(item);
            if(v<0) {
                value = false;
                v = -v;
            }
            if(v==0 || v>31)
                PyErr_Format(PyExc_ValueError, "Status value out of range '%d'", v);
            status.set(v,value);
        } else {
            PyErr_SetString(PyExc_TypeError, "Expects status type to be Int or String");
            return 0;
        }
    }

    prop->setStatusValue(status.to_ulong());
    Py_Return;
}

PyObject*  PropertyContainerPy::getPropertyStatus(PyObject *args)
{
    char* name = "";
    if (!PyArg_ParseTuple(args, "|s", &name))     // convert args: Python->C
        return NULL;                             // NULL triggers exception

    Py::List ret;
    const auto &statusMap = getStatusMap();
    if(!name[0]) {
        for(auto &v : statusMap)
            ret.append(Py::String(v.first.c_str()));
    }else{
        App::Property* prop = getPropertyContainerPtr()->getPropertyByName(name);
        if (prop) {
            std::bitset<32> bits(prop->getStatus());
            for(size_t i=1;i<bits.size();++i) {
                if(!bits[i]) continue;
                bool found = false;
                for(auto &v : statusMap) {
                    if(v.second == (int)i) {
                        ret.append(Py::String(v.first.c_str()));
                        found = true;
                        break;
                    }
                }
                if(!found) 
                    ret.append(Py::Int((long)i));
            }
        }
    }
    return Py::new_reference_to(ret);
}

PyObject*  PropertyContainerPy::getEditorMode(PyObject *args)
{
    char* name;
    if (!PyArg_ParseTuple(args, "s", &name))     // convert args: Python->C
        return NULL;                             // NULL triggers exception

    App::Property* prop = getPropertyContainerPtr()->getPropertyByName(name);
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
    if (!PyArg_ParseTuple(args, "s", &pstr))     // convert args: Python->C
        return NULL;                             // NULL triggers exception

    Property* prop = getPropertyContainerPtr()->getPropertyByName(pstr);
    if (!prop) {
        PyErr_Format(PyExc_AttributeError, "Property container has no property '%s'", pstr);
        return 0;
    }

    const char* Group = getPropertyContainerPtr()->getPropertyGroup(prop);
    if (Group)
        return Py::new_reference_to(Py::String(Group));
    else
        return Py::new_reference_to(Py::String(""));
}

PyObject*  PropertyContainerPy::getDocumentationOfProperty(PyObject *args)
{
    char *pstr;
    if (!PyArg_ParseTuple(args, "s", &pstr))     // convert args: Python->C
        return NULL;                             // NULL triggers exception

    Property* prop = getPropertyContainerPtr()->getPropertyByName(pstr);
    if (!prop) {
        PyErr_Format(PyExc_AttributeError, "Property container has no property '%s'", pstr);
        return 0;
    }

    const char* Group = getPropertyContainerPtr()->getPropertyDocumentation(prop);
    if (Group)
        return Py::new_reference_to(Py::String(Group));
    else
        return Py::new_reference_to(Py::String(""));
}

Py::List PropertyContainerPy::getPropertiesList(void) const
{
    Py::List ret;
    std::map<std::string,Property*> Map;

    getPropertyContainerPtr()->getPropertyMap(Map);

    for (std::map<std::string,Property*>::const_iterator It=Map.begin();It!=Map.end();++It)
        ret.append(Py::String(It->first));

    return ret;
}


PyObject* PropertyContainerPy::dumpPropertyContent(PyObject *args, PyObject *kwds)
{
    int compression = 3;
    char* property;
    static char* kwds_def[] = {"Property", "Compression",NULL};
    PyErr_Clear();
    if (!PyArg_ParseTupleAndKeywords(args, kwds, "s|i", kwds_def, &property, &compression)) {
        return NULL;
    }

    Property* prop = getPropertyContainerPtr()->getPropertyByName(property);
    if (!prop) {
        PyErr_Format(PyExc_AttributeError, "Property container has no property '%s'", property);
        return 0;
    }

    //setup the stream. the in flag is needed to make "read" work
    std::stringstream stream(std::stringstream::out | std::stringstream::in | std::stringstream::binary);
    try {
        prop->dumpToStream(stream, compression);
    }
    catch (...) {
       PyErr_SetString(PyExc_IOError, "Unable parse content into binary representation");
       return NULL; 
    }

    //build the byte array with correct size
    if (!stream.seekp(0, stream.end)) {
        PyErr_SetString(PyExc_IOError, "Unable to find end of stream");
        return NULL;
    }

    std::stringstream::pos_type offset = stream.tellp();
    if (!stream.seekg(0, stream.beg)) {
        PyErr_SetString(PyExc_IOError, "Unable to find begin of stream");
        return NULL;
    }

    PyObject* ba = PyByteArray_FromStringAndSize(NULL, offset);

    //use the buffer protocol to access the underlying array and write into it
    Py_buffer buf = Py_buffer();
    PyObject_GetBuffer(ba, &buf, PyBUF_WRITABLE);
    try {
        if(!stream.read((char*)buf.buf, offset)) {
            PyErr_SetString(PyExc_IOError, "Error copying data into byte array");
            return NULL;
        }
        PyBuffer_Release(&buf);
    }
    catch (...) {
        PyBuffer_Release(&buf);
        PyErr_SetString(PyExc_IOError, "Error copying data into byte array");
        return NULL;
    }

    return ba;
}

PyObject* PropertyContainerPy::restorePropertyContent(PyObject *args)
{
    PyObject* buffer;
    char* property;
    if( !PyArg_ParseTuple(args, "sO", &property, &buffer) )
        return NULL;

    Property* prop = getPropertyContainerPtr()->getPropertyByName(property);
    if (!prop) {
        PyErr_Format(PyExc_AttributeError, "Property container has no property '%s'", property);
        return 0;
    }

    //check if it really is a buffer
    if( !PyObject_CheckBuffer(buffer) ) {
        PyErr_SetString(PyExc_TypeError, "Must be a buffer object");
        return NULL;
    }

    Py_buffer buf;
    if(PyObject_GetBuffer(buffer, &buf, PyBUF_SIMPLE) < 0)
        return NULL;

    if(!PyBuffer_IsContiguous(&buf, 'C')) {
        PyErr_SetString(PyExc_TypeError, "Buffer must be contiguous");
        return NULL;
    }

    //check if it really is a buffer
    try {
        typedef boost::iostreams::basic_array_source<char> Device;
        boost::iostreams::stream<Device> stream((char*)buf.buf, buf.len);
        prop->restoreFromStream(stream);
    }
    catch(...) {
        PyErr_SetString(PyExc_IOError, "Unable to restore content");
        return NULL;
    }

    Py_Return;
}

PyObject *PropertyContainerPy::getCustomAttributes(const char* attr) const
{
    // search in PropertyList
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
        PyObject *dict = PyDict_New();
        if (dict) {
            for ( std::map<std::string,App::Property*>::iterator it = Map.begin(); it != Map.end(); ++it )
#if PY_MAJOR_VERSION >= 3
                PyDict_SetItem(dict, PyUnicode_FromString(it->first.c_str()), PyUnicode_FromString(""));
#else
                PyDict_SetItem(dict, PyString_FromString(it->first.c_str()), PyString_FromString(""));
#endif
            if (PyErr_Occurred()) {
                Py_DECREF(dict);
                dict = NULL;
            }
        }
        return dict;
    }

    return 0;
}

int PropertyContainerPy::setCustomAttributes(const char* attr, PyObject *obj)
{
    // search in PropertyList
    Property *prop = getPropertyContainerPtr()->getPropertyByName(attr);
    if (prop) {
        // Read-only attributes must not be set over its Python interface
        if(prop->testStatus(Property::Immutable) ||
           (getPropertyContainerPtr()->getPropertyType(prop) & Prop_ReadOnly))
        {
            std::stringstream s;
            s << "Object attribute '" << attr << "' is read-only";
            throw Py::AttributeError(s.str());
        }

        prop->setPyObject(obj);
        return 1;
    }

    return 0;
}
