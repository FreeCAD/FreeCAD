/****************************************************************************
 *   Copyright (c) 2017 Zheng Lei (realthunder) <realthunder.dev@gmail.com> *
 *                                                                          *
 *   This file is part of the FreeCAD CAx development system.               *
 *                                                                          *
 *   This library is free software; you can redistribute it and/or          *
 *   modify it under the terms of the GNU Library General Public            *
 *   License as published by the Free Software Foundation; either           *
 *   version 2 of the License, or (at your option) any later version.       *
 *                                                                          *
 *   This library  is distributed in the hope that it will be useful,       *
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of         *
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the          *
 *   GNU Library General Public License for more details.                   *
 *                                                                          *
 *   You should have received a copy of the GNU Library General Public      *
 *   License along with this library; see the file COPYING.LIB. If not,     *
 *   write to the Free Software Foundation, Inc., 59 Temple Place,          *
 *   Suite 330, Boston, MA  02111-1307, USA                                 *
 *                                                                          *
 ****************************************************************************/

#include "PreCompiled.h"

#ifndef _PreComp_
# include <sstream>
#endif

#include "DocumentObjectPy.h"
#include "LinkBaseExtensionPy.h"
#include "LinkBaseExtensionPy.cpp"

using namespace App;

// returns a string which represent the object e.g. when printed in python
std::string LinkBaseExtensionPy::representation() const
{
    std::ostringstream str;
    str << "<" << getLinkBaseExtensionPtr()->getExtensionClassTypeId().getName() << ">";
    return str.str();
}

using PropTmpMap = std::map<std::string, std::pair<int,Property*> >;
using PropMap = std::map<std::string, Property*>;

static bool getProperty(PropTmpMap &props, const LinkBaseExtension::PropInfoMap &infoMap,
        const PropMap &propMap, PyObject *key, PyObject *value)
{
    std::ostringstream str;

    if(!PyUnicode_Check(key)) {
        PyErr_SetString(PyExc_TypeError, "key must be a unicode string");
        return false;
    }
    const char *keyStr = PyUnicode_AsUTF8(key);
    auto it = infoMap.find(keyStr);
    if(it == infoMap.end()){
        str << "unknown key '" << keyStr << "'";
        PyErr_SetString(PyExc_KeyError, str.str().c_str());
        return false;
    }

    const char *valStr = nullptr;
    if(key == value)
        valStr = keyStr;
    else if (value!=Py_None) {
        if(!PyUnicode_Check(value)) {
            PyErr_SetString(PyExc_TypeError, "value must be unicode string");
            return false;
        }
        valStr = PyUnicode_AsUTF8(value);
    }

    App::Property *prop = nullptr;
    auto &info = it->second;
    if(valStr) {
        auto pIt = propMap.find(valStr);
        if(pIt == propMap.end()) {
            str << "cannot find property '" << valStr << "'";
            PyErr_SetString(PyExc_ValueError, str.str().c_str());
            return false;
        }
        prop = pIt->second;
        if(!prop->isDerivedFrom(info.type)) {
            str << "expect property '" << keyStr << "(" << valStr
                << ") to be derived from '" << info.type.getName()
                << "', instead of '" << prop->getTypeId().getName() << "'";
            PyErr_SetString(PyExc_TypeError, str.str().c_str());
        }
    }
    props[keyStr] = std::make_pair(info.index,prop);
    return true;
}

PyObject* LinkBaseExtensionPy::configLinkProperty(PyObject *args, PyObject *keywds) {
    auto ext = getLinkBaseExtensionPtr();
    const auto &info = ext->getPropertyInfoMap();

    PropMap propMap;
    ext->getExtendedContainer()->getPropertyMap(propMap);

    PropTmpMap props;

    if(args && PyTuple_Check(args)) {
        for(Py_ssize_t pos=0;pos<PyTuple_GET_SIZE(args);++pos) {
            auto key = PyTuple_GET_ITEM(args,pos);
            if(!getProperty(props,info,propMap,key,key))
                return nullptr;
        }
    }
    if(keywds && PyDict_Check(keywds)) {
        PyObject *key, *value;
        Py_ssize_t pos = 0;
        while (PyDict_Next(keywds, &pos, &key, &value)) {
            if(!getProperty(props,info,propMap,key,value))
                return nullptr;
        }
    }
    for(auto &v : props)
        ext->setProperty(v.second.first,v.second.second);
    Py_Return;
}

PyObject* LinkBaseExtensionPy::getLinkExtProperty(PyObject *args)
{
    const char *name;
    if(!PyArg_ParseTuple(args,"s",&name))
        return nullptr;
    auto prop = getLinkBaseExtensionPtr()->getProperty(name);
    if(!prop) {
        PyErr_SetString(PyExc_AttributeError, "unknown property name");
        return nullptr;
    }
    return prop->getPyObject();
}

PyObject* LinkBaseExtensionPy::getLinkExtPropertyName(PyObject *args) {
    const char *name;
    if(!PyArg_ParseTuple(args,"s",&name))
        return nullptr;
    auto prop = getLinkBaseExtensionPtr()->getProperty(name);
    if(!prop) {
        PyErr_SetString(PyExc_AttributeError, "unknown property name");
        return nullptr;
    }
    auto container = getLinkBaseExtensionPtr()->getExtendedContainer();
    if(!container) {
        PyErr_SetString(PyExc_RuntimeError, "no extended container");
        return nullptr;
    }
    name = container->getPropertyName(prop);
    if(!name) {
        PyErr_SetString(PyExc_RuntimeError, "cannot find property name");
        return nullptr;
    }
    return Py::new_reference_to(Py::String(name));
}

PyObject* LinkBaseExtensionPy::getLinkPropertyInfo(PyObject *args)
{
    auto ext = getLinkBaseExtensionPtr();

    const auto &infos = ext->getPropertyInfo();

    if(PyArg_ParseTuple(args,"")) {
        Py::Tuple ret(infos.size());
        int i=0;
        for(const auto &info : infos) {
            ret.setItem(i++,Py::TupleN(Py::String(info.name),
                    Py::String(info.type.getName()),Py::String(info.doc)));
        }
        return Py::new_reference_to(ret);
    }

    short index = 0;
    if(PyArg_ParseTuple(args,"h",&index)) {
        if(index<0 || index>=(int)infos.size()) {
            PyErr_SetString(PyExc_ValueError, "index out of range");
            return nullptr;
        }
        Py::TupleN ret(Py::String(infos[index].name),
                Py::String(infos[index].type.getName()),Py::String(infos[index].doc));
        return Py::new_reference_to(ret);
    }

    char *name;
    if(PyArg_ParseTuple(args,"s",&name)) {
        for(const auto & info : infos) {
            if(strcmp(info.name,name)==0) {
                Py::TupleN ret(Py::String(info.type.getName()),
                               Py::String(info.doc));
                return Py::new_reference_to(ret);
            }
        }
        PyErr_SetString(PyExc_ValueError, "unknown property name");
        return nullptr;
    }

    PyErr_SetString(PyExc_ValueError, "invalid arguments");
    return nullptr;
}

void parseLink(LinkBaseExtension *ext, int index, PyObject *value) {
    App::DocumentObject *obj = nullptr;
    PropertyStringList subs;
    PropertyString sub;
    if(value!=Py_None) {
        if(PyObject_TypeCheck(value,&DocumentObjectPy::Type)) {
            obj = static_cast<DocumentObjectPy*>(value)->getDocumentObjectPtr();
        }else if(!PySequence_Check(value))
            throw Base::TypeError("Expects type of DocumentObject or sequence");
        else{
            Py::Sequence seq(value);
            if(seq[0].ptr() != Py_None) {
                if(!PyObject_TypeCheck(seq[0].ptr(),&DocumentObjectPy::Type))
                    throw Base::TypeError("Expects the first argument to be DocumentObject in sequence");
                obj = static_cast<DocumentObjectPy*>(seq[0].ptr())->getDocumentObjectPtr();
                if(seq.size()>1) {
                    sub.setPyObject(seq[1].ptr());
                    if(seq.size()>2)
                        subs.setPyObject(seq[2].ptr());
                }
            }
        }
    }
    ext->setLink(index,obj,sub.getValue(),subs.getValue());
}

PyObject* LinkBaseExtensionPy::setLink(PyObject *_args)
{
    Py::Sequence args(_args);
    PY_TRY {
        auto ext = getLinkBaseExtensionPtr();
        PyObject *pcObj = args.size()?args[0].ptr():Py_None;
        if(pcObj == Py_None) {
            ext->setLink(-1,nullptr);
        }else if(PyDict_Check(pcObj)) {
            PyObject *key, *value;
            Py_ssize_t pos = 0;
            while(PyDict_Next(pcObj, &pos, &key, &value))
                parseLink(ext,Py::Int(key),value);
        }else if(PySequence_Check(pcObj)) {
            ext->setLink(-1,nullptr);
            Py::Sequence seq(pcObj);
            for(Py_ssize_t i=0;i<seq.size();++i)
                parseLink(ext,i,seq[i].ptr());
        }else
            parseLink(ext,-1,_args);

        Py_Return;
    }PY_CATCH
}

PyObject* LinkBaseExtensionPy::cacheChildLabel(PyObject *args) {
    PyObject *enable = Py_True;
    if(!PyArg_ParseTuple(args,"|O",&enable))
        return nullptr;
    PY_TRY {
        getLinkBaseExtensionPtr()->cacheChildLabel(Base::asBoolean(enable) ? -1 : 0);
        Py_Return;
    }PY_CATCH;
}

PyObject* LinkBaseExtensionPy::flattenSubname(PyObject *args) {
    const char *subname;
    if(!PyArg_ParseTuple(args,"s",&subname))
        return nullptr;
    PY_TRY {
        return Py::new_reference_to(Py::String(
                    getLinkBaseExtensionPtr()->flattenSubname(subname)));
    }PY_CATCH;
}

PyObject* LinkBaseExtensionPy::expandSubname(PyObject *args) {
    const char *subname;
    if(!PyArg_ParseTuple(args,"s",&subname))
        return nullptr;
    PY_TRY {
        std::string sub(subname);
        getLinkBaseExtensionPtr()->expandSubname(sub);
        return Py::new_reference_to(Py::String(sub));
    }PY_CATCH;
}

Py::List LinkBaseExtensionPy::getLinkedChildren() const {
    Py::List ret;
    for(auto o : getLinkBaseExtensionPtr()->getLinkedChildren(true))
        ret.append(Py::asObject(o->getPyObject()));
    return ret;
}

PyObject *LinkBaseExtensionPy::getCustomAttributes(const char* /*attr*/) const
{
    return nullptr;
}

int LinkBaseExtensionPy::setCustomAttributes(const char* /*attr*/, PyObject * /*obj*/)
{
    return 0;
}
