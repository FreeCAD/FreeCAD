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

#include "ComplexGeoData.h"

// inclusion of the generated files (generated out of ComplexGeoDataPy.xml)
#include <App/ComplexGeoDataPy.h>
#include <App/ComplexGeoDataPy.cpp>
#include <Base/BoundBoxPy.h>
#include <Base/MatrixPy.h>
#include <Base/PlacementPy.h>
#include <Base/VectorPy.h>
#include <Base/GeometryPyCXX.h>
#include <App/StringHasherPy.h>
#include <App/StringIDPy.h>

using namespace Data;
using namespace Base;

// returns a string which represent the object e.g. when printed in python
std::string ComplexGeoDataPy::representation(void) const
{
    return std::string("<ComplexGeoData object>");
}

PyObject*  ComplexGeoDataPy::getFacesFromSubelement(PyObject *args)
{
    char *type;
    int index;
    if (!PyArg_ParseTuple(args, "si", &type, &index))
        return 0;

    std::vector<Base::Vector3d> points;
    std::vector<Base::Vector3d> normals;
    std::vector<Data::ComplexGeoData::Facet> facets;
    try {
        Data::Segment* segm = getComplexGeoDataPtr()->getSubElement(type, index);
        getComplexGeoDataPtr()->getFacesFromSubelement(segm, points, normals, facets);
        delete segm;
    }
    catch (...) {
        PyErr_SetString(PyExc_RuntimeError, "failed to get sub-element from object");
        return 0;
    }

    Py::Tuple tuple(2);
    Py::List vertex;
    for (std::vector<Base::Vector3d>::const_iterator it = points.begin();
        it != points.end(); ++it)
        vertex.append(Py::asObject(new Base::VectorPy(*it)));
    tuple.setItem(0, vertex);
    Py::List facet;
    for (std::vector<Data::ComplexGeoData::Facet>::const_iterator
        it = facets.begin(); it != facets.end(); ++it) {
        Py::Tuple f(3);
        f.setItem(0,Py::Int((int)it->I1));
        f.setItem(1,Py::Int((int)it->I2));
        f.setItem(2,Py::Int((int)it->I3));
        facet.append(f);
    }
    tuple.setItem(1, facet);
    return Py::new_reference_to(tuple);
}

PyObject* ComplexGeoDataPy::getElementName(PyObject *args)
{
    char* input;
    int direction = 0;
    if (!PyArg_ParseTuple(args, "s|i", &input,&direction))
        return NULL;
    const char *ret = getComplexGeoDataPtr()->getElementName(input,direction);
    return Py::new_reference_to(Py::String(ret));
}

PyObject *ComplexGeoDataPy::setElementName(PyObject *args, PyObject *kwds) {
    const char *element;
    const char *name = 0;
    const char *postfix = 0;
    PyObject *pySid = Py_None;
    PyObject *overwrite = Py_False;

    static char *kwlist[] = {"element", "name", "postfix", "overwrite", "sid", NULL};
    if (!PyArg_ParseTupleAndKeywords(args, kwds, "s|sssOO", kwlist, 
                &element,&name,&postfix,&overwrite,&pySid))
        return NULL;
    std::vector<App::StringIDRef> sids;
    if(pySid != Py_None) {
        if(PyObject_TypeCheck(pySid,&App::StringIDPy::Type))
            sids.push_back(static_cast<App::StringIDPy*>(pySid)->getStringIDPtr());
        else if(PySequence_Check(pySid)) {
            Py::Sequence seq(pySid);
            for(auto it=seq.begin();it!=seq.end();++it) {
                auto ptr = (*it).ptr();
                if(PyObject_TypeCheck(ptr,&App::StringIDPy::Type))
                    sids.push_back(static_cast<App::StringIDPy*>(ptr)->getStringIDPtr());
                else
                    throw Py::TypeError("expect StringID in sid sequence");
            }
        } else
            throw Py::TypeError("expect sid to contain either StringID or sequence of StringID");
    }
    PY_TRY {
        const char *ret = getComplexGeoDataPtr()->setElementName(element,name, 
                postfix,&sids,PyObject_IsTrue(overwrite));
        return Py::new_reference_to(Py::String(ret));
    }PY_CATCH
}

Py::Object ComplexGeoDataPy::getHasher() const {
    auto self = getComplexGeoDataPtr();
    if(!self->Hasher)
        return Py::None();
    return Py::Object(self->Hasher->getPyObject(),true);
}

Py::Dict ComplexGeoDataPy::getElementMap() const {
    Py::Dict ret;
    for(auto &v : getComplexGeoDataPtr()->getElementMap())
        ret.setItem(v.first,Py::String(v.second));
    return ret;
}

void ComplexGeoDataPy::setElementMap(Py::Dict dict) {
    std::map<std::string, std::string> map;
    for(auto it=dict.begin();it!=dict.end();++it) {
        const auto &value = *it;
        if(!value.first.isString() || !value.second.isString())
            throw Py::TypeError("expect only strings in the dict");
        map.emplace_hint(map.cend(),value.first.as_string(),Py::Object(value.second).as_string());
    }
    getComplexGeoDataPtr()->setElementMap(map);
}

Py::Dict ComplexGeoDataPy::getElementReverseMap() const {
    Py::Dict ret;
    for(auto &v : getComplexGeoDataPtr()->getElementMap()) {
        auto value = ret[Py::String(v.second)];
        Py::Object item(value);
        if(item.isNone())
            value = Py::String(v.first);
        else if(item.isList()) {
            Py::List list(item);
            list.append(Py::String(v.first));
        } else {
            Py::List list;
            list.append(item);
            list.append(Py::String(v.first));
            value = list;
        }
    }
    return ret;
}

Py::Int ComplexGeoDataPy::getElementMapSize() const {
    return Py::Int((long)getComplexGeoDataPtr()->getElementMapSize());
}

void ComplexGeoDataPy::setHasher(Py::Object obj) {
    auto self = getComplexGeoDataPtr();
    if(obj.isNone()) {
        if(self->Hasher) {
            self->Hasher = App::StringHasherRef();
            self->resetElementMap();
        }
    }else if(PyObject_TypeCheck(obj.ptr(),&App::StringHasherPy::Type)) {
        App::StringHasherRef ref(static_cast<App::StringHasherPy*>(obj.ptr())->getStringHasherPtr());
        if(self->Hasher != ref) {
            self->Hasher = ref;
            self->resetElementMap();
        }
    }else
        throw Py::TypeError("invalid type");
}

Py::Object ComplexGeoDataPy::getBoundBox(void) const
{
    return Py::BoundingBox(getComplexGeoDataPtr()->getBoundBox());
}

Py::Object ComplexGeoDataPy::getPlacement(void) const
{
    return Py::Placement(getComplexGeoDataPtr()->getPlacement());
}

void  ComplexGeoDataPy::setPlacement(Py::Object arg)
{
    PyObject* p = arg.ptr();
    if (PyObject_TypeCheck(p, &(Base::PlacementPy::Type))) {
        Base::Placement* trf = static_cast<Base::PlacementPy*>(p)->getPlacementPtr();
        getComplexGeoDataPtr()->setPlacement(*trf);
    }
    else {
        std::string error = std::string("type must be 'Placement', not ");
        error += p->ob_type->tp_name;
        throw Py::TypeError(error);
    }
}

Py::Object ComplexGeoDataPy::getMatrix(void) const
{
    return Py::Matrix(getComplexGeoDataPtr()->getTransform());
}

// FIXME would be better to call it setTransform() as in all other interfaces...
void  ComplexGeoDataPy::setMatrix(Py::Object arg)
{
    PyObject* p = arg.ptr();
    if (PyObject_TypeCheck(p, &(Base::MatrixPy::Type))) {
        Base::Matrix4D mat = static_cast<Base::MatrixPy*>(p)->value();
        getComplexGeoDataPtr()->setTransform(mat);
    }
    else {
        std::string error = std::string("type must be 'Matrix', not ");
        error += p->ob_type->tp_name;
        throw Py::TypeError(error);
    }
}

Py::Tuple ComplexGeoDataPy::getElementTypes() const {
    const auto &types = getComplexGeoDataPtr()->getElementTypes();
    Py::Tuple ret(types.size());
    int i=0;
    for(auto const &type : types)
        ret.setItem(i++,Py::String(type));
    return ret;
} 

Py::String ComplexGeoDataPy::getElementMapVersion() const {
    return Py::String(getComplexGeoDataPtr()->getElementMapVersion());
}

Py::Int ComplexGeoDataPy::getTag() const {
    return Py::Int(getComplexGeoDataPtr()->Tag);
}

void ComplexGeoDataPy::setTag(Py::Int tag) {
    getComplexGeoDataPtr()->Tag = tag;
}

PyObject *ComplexGeoDataPy::getCustomAttributes(const char* /*attr*/) const
{
    return 0;
}

int ComplexGeoDataPy::setCustomAttributes(const char* /*attr*/, PyObject* /*obj*/)
{
    return 0;
}
