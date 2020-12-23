/****************************************************************************
 *   Copyright (c) 2019 Zheng, Lei (realthunder) <realthunder.dev@gmail.com>*
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
# include <Inventor/nodes/SoGroup.h>
# include <Inventor/details/SoDetail.h>
# include <Inventor/SoFullPath.h>
#endif

#include "AxisOriginPy.h"
#include "AxisOriginPy.cpp"

using namespace Gui;

PyObject *AxisOriginPy::PyMake(struct _typeobject *, PyObject *, PyObject *)  // Python wrapper
{
    return new AxisOriginPy(new AxisOrigin);
}

int AxisOriginPy::PyInit(PyObject* /*args*/, PyObject* /*kwd*/)
{
    return 0;
}


// returns a string which represent the object e.g. when printed in python
std::string AxisOriginPy::representation(void) const
{
    return "<AxisOrigin>";
}

PyObject* AxisOriginPy::getElementPicked(PyObject* args)
{
    PyObject *obj;
    if (!PyArg_ParseTuple(args, "O",&obj))
        return NULL;
    void *ptr = 0;
    Base::Interpreter().convertSWIGPointerObj("pivy.coin", "_p_SoPickedPoint", obj, &ptr, 0);
    SoPickedPoint *pp = reinterpret_cast<SoPickedPoint*>(ptr);
    if(!pp) 
        throw Base::TypeError("type must be of coin.SoPickedPoint");
    std::string name;
    if(!getAxisOriginPtr()->getElementPicked(pp,name))
        Py_Return;
    return Py::new_reference_to(Py::String(name));
}

PyObject* AxisOriginPy::getDetailPath(PyObject* args)
{
    const char *sub;
    PyObject *path;
    if (!PyArg_ParseTuple(args, "sO",&sub,&path))
        return NULL;
    void *ptr = 0;
    Base::Interpreter().convertSWIGPointerObj("pivy.coin", "_p_SoPath", path, &ptr, 0);
    SoPath *pPath = reinterpret_cast<SoPath*>(ptr);
    if(!pPath) 
        throw Base::TypeError("type must be of coin.SoPath");
    SoDetail *det = 0;
    if(!getAxisOriginPtr()->getDetailPath(
            sub,static_cast<SoFullPath*>(pPath),det))
    {
        if(det) delete det;
        Py_Return;
    }
    if(!det)
        return Py::new_reference_to(Py::True());
    return Base::Interpreter().createSWIGPointerObj("pivy.coin", "_p_SoDetail", (void*)det, 0);
}

Py::Float AxisOriginPy::getAxisLength() const {
    return Py::Float(getAxisOriginPtr()->getAxisLength());
}

void AxisOriginPy::setAxisLength(Py::Float size) {
    getAxisOriginPtr()->setAxisLength(size);
}

Py::Float AxisOriginPy::getLineWidth() const {
    return Py::Float(getAxisOriginPtr()->getLineWidth());
}

void AxisOriginPy::setLineWidth(Py::Float size) {
    getAxisOriginPtr()->setLineWidth(size);
}

Py::Float AxisOriginPy::getPointSize() const {
    return Py::Float(getAxisOriginPtr()->getPointSize());
}

void AxisOriginPy::setPointSize(Py::Float size) {
    getAxisOriginPtr()->setPointSize(size);
}

Py::Float AxisOriginPy::getScale() const {
    return Py::Float(getAxisOriginPtr()->getScale());
}

void AxisOriginPy::setScale(Py::Float size) {
    getAxisOriginPtr()->setScale(size);
}

Py::Tuple AxisOriginPy::getPlane() const {
    auto info = getAxisOriginPtr()->getPlane();
    Py::Tuple ret(2);
    ret.setItem(0,Py::Float(info.first));
    ret.setItem(1,Py::Float(info.second));
    return ret;
}

void AxisOriginPy::setPlane(Py::Tuple tuple) {
    float s,d;
    if (!PyArg_ParseTuple(*tuple, "dd",&s,&d))
        throw Py::Exception();
    getAxisOriginPtr()->setPlane(s,d);
}

Py::Dict AxisOriginPy::getLabels() const {
    Py::Dict dict;
    for(auto &v : getAxisOriginPtr()->getLabels())
        dict.setItem(Py::String(v.first),Py::String(v.second));
    return dict;
}

void AxisOriginPy::setLabels(Py::Dict dict) {
    std::map<std::string,std::string> labels;
    for(auto it=dict.begin();it!=dict.end();++it) {
        const auto &value = *it;
        labels[value.first.as_string()] = Py::Object(value.second).as_string();
    }
    getAxisOriginPtr()->setLabels(labels);
}

Py::Object AxisOriginPy::getNode(void) const
{
    SoGroup* node = getAxisOriginPtr()->getNode();
    PyObject* Ptr = Base::Interpreter().createSWIGPointerObj("pivy.coin","SoGroup *", node, 1);
    node->ref();
    return Py::Object(Ptr, true);
}

PyObject *AxisOriginPy::getCustomAttributes(const char* /*attr*/) const
{
    return 0;
}

int AxisOriginPy::setCustomAttributes(const char* /*attr*/, PyObject* /*obj*/)
{
    return 0; 
}
