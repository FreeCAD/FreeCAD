/****************************************************************************
 *   Copyright (c) 2017 Zheng, Lei (realthunder) <realthunder.dev@gmail.com>*
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
# include <Inventor/nodes/SoSeparator.h>
#endif

#include <Base/MatrixPy.h>
#include <Base/VectorPy.h>
#include <Base/BoundBoxPy.h>
#include <App/MaterialPy.h>
#include <App/DocumentObjectPy.h>

#include "ViewProviderDocumentObjectPy.h"
#include "ViewProviderLink.h"
#include "WidgetFactory.h"

#include "LinkViewPy.h"
#include "LinkViewPy.cpp"

using namespace Gui;

PyObject *LinkViewPy::PyMake(struct _typeobject *, PyObject *, PyObject *)  // Python wrapper
{
    return new LinkViewPy(new LinkView);
}

int LinkViewPy::PyInit(PyObject* /*args*/, PyObject* /*kwd*/)
{
    return 0;
}


// returns a string which represent the object e.g. when printed in python
std::string LinkViewPy::representation(void) const
{
    return "<Link view>";
}

PyObject* LinkViewPy::reset(PyObject *args) {
    if (!PyArg_ParseTuple(args, ""))
        return 0;

    PY_TRY {
        auto lv = getLinkViewPtr();
        lv->setSize(0);
        lv->setLink(0);
        Py_Return;
    } PY_CATCH;
}

PyObject* LinkViewPy::setMaterial(PyObject *args) {
    PyObject *pyObj;
    if (!PyArg_ParseTuple(args, "O", &pyObj))
        return 0;

    PY_TRY {
        auto lv = getLinkViewPtr();
        if(pyObj == Py_None) {
            lv->setMaterial(-1,0);
            Py_Return;
        }
        if(PyObject_TypeCheck(&pyObj,&App::MaterialPy::Type)) {
            lv->setMaterial(-1,static_cast<App::MaterialPy*>(pyObj)->getMaterialPtr());
            Py_Return;
        }
        if(PyDict_Check(pyObj)) {
            PyObject *key, *value;
            Py_ssize_t pos = 0;
            std::map<int,App::Material*> materials;
            while(PyDict_Next(pyObj, &pos, &key, &value)) {
                Py::Int idx(key);
                if(value == Py_None)
                    materials[(int)idx] = 0;
                else if(!PyObject_TypeCheck(&value,&App::MaterialPy::Type)) {
                    PyErr_SetString(PyExc_TypeError, "exepcting a type of material");
                    return 0;
                }else
                    materials[(int)idx] = static_cast<App::MaterialPy*>(value)->getMaterialPtr();
            }
            for(auto &v : materials)
                lv->setMaterial(v.first,v.second);
            Py_Return;
        }
        if(PySequence_Check(pyObj)) {
            Py::Sequence seq(pyObj);
            std::vector<App::Material*> materials;
            materials.resize(seq.size(),0);
            for(size_t i=0;i<seq.size();++i) {
                PyObject* item = seq[i].ptr();
                if(item == Py_None) continue;
                if(!PyObject_TypeCheck(&item,&App::MaterialPy::Type)) {
                    PyErr_SetString(PyExc_TypeError, "exepcting a type of material");
                    return 0;
                }
                materials[i] = static_cast<App::MaterialPy*>(item)->getMaterialPtr();
            }
            for(size_t i=0;i<materials.size();++i)
                lv->setMaterial(i,materials[i]);
            Py_Return;
        }
                
        PyErr_SetString(PyExc_TypeError, "exepcting a type of Material, [Material,...] or {Int:Material,}");
        return 0;
    } PY_CATCH;
}

PyObject* LinkViewPy::setTransform(PyObject *args) {
    PyObject *pyObj;
    if (!PyArg_ParseTuple(args, "O", &pyObj))
        return 0;

    PY_TRY {
        auto lv = getLinkViewPtr();
        if(PyObject_TypeCheck(pyObj,&Base::MatrixPy::Type)) {
            lv->setTransform(-1,*static_cast<Base::MatrixPy*>(pyObj)->getMatrixPtr());
            Py_Return;
        }
        if(PyDict_Check(pyObj)) {
            PyObject *key, *value;
            Py_ssize_t pos = 0;
            std::map<int,Base::Matrix4D*> mat;
            while(PyDict_Next(pyObj, &pos, &key, &value)) {
                Py::Int idx(key);
                if(!PyObject_TypeCheck(&value,&Base::MatrixPy::Type)) {
                    PyErr_SetString(PyExc_TypeError, "exepcting a type of Matrix");
                    return 0;
                }else
                    mat[(int)idx] = static_cast<Base::MatrixPy*>(value)->getMatrixPtr();
            }
            for(auto &v : mat)
                lv->setTransform(v.first,*v.second);
            Py_Return;
        }
        if(PySequence_Check(pyObj)) {
            Py::Sequence seq(pyObj);
            std::vector<Base::Matrix4D*> mat;
            mat.resize(seq.size(),0);
            for(size_t i=0;i<seq.size();++i) {
                PyObject* item = seq[i].ptr();
                if(!PyObject_TypeCheck(&item,&Base::MatrixPy::Type)) {
                    PyErr_SetString(PyExc_TypeError, "exepcting a type of Matrix");
                    return 0;
                }
                mat[i] = static_cast<Base::MatrixPy*>(item)->getMatrixPtr();
            }
            for(size_t i=0;i<mat.size();++i)
                lv->setTransform(i,*mat[i]);
            Py_Return;
        }
                
        PyErr_SetString(PyExc_TypeError, "exepcting a type of Matrix, [Matrix,...] or {Int:Matrix,...}");
        return 0;
    } PY_CATCH;
}

PyObject* LinkViewPy::setType(PyObject *args) {
    short type;
    PyObject *sublink = Py_True;
    if (!PyArg_ParseTuple(args, "h|O", &type,&sublink))
        return 0;

    PY_TRY{
        getLinkViewPtr()->setNodeType((LinkView::SnapshotType)type,PyObject_IsTrue(sublink));
        Py_Return;
    } PY_CATCH;
}

PyObject*  LinkViewPy::setChildren(PyObject *args) {
    PyObject *pyObj;
    PyObject *pyVis = Py_None;
    short type=0;
    if (!PyArg_ParseTuple(args, "O|Os",&pyObj,&pyVis,&type))
        return 0;

    PY_TRY{
        App::PropertyBoolList vis;
        App::PropertyLinkList links;
        if(pyObj!=Py_None)
            links.setPyObject(pyObj);
        if(pyVis!=Py_None)
            vis.setPyObject(pyVis);
        getLinkViewPtr()->setChildren(links.getValue(),vis.getValue(),(LinkView::SnapshotType)type);
        Py_Return;
    } PY_CATCH;
}

PyObject*  LinkViewPy::setLink(PyObject *args)
{
    PyObject *pyObj;
    PyObject *pySubName = Py_None;
    if (!PyArg_ParseTuple(args, "O|O",&pyObj,&pySubName))
        return 0;

    PY_TRY {
        ViewProviderDocumentObject *vpd = 0;
        App::DocumentObject *obj = 0;
        if(pyObj!=Py_None) {
            if(PyObject_TypeCheck(pyObj,&App::DocumentObjectPy::Type))
                obj = static_cast<App::DocumentObjectPy*>(pyObj)->getDocumentObjectPtr();
            else if(PyObject_TypeCheck(pyObj,&ViewProviderDocumentObjectPy::Type))
                vpd = static_cast<ViewProviderDocumentObjectPy*>(pyObj)->getViewProviderDocumentObjectPtr();
            else {
                PyErr_SetString(PyExc_TypeError, 
                        "exepcting a type of DocumentObject or ViewProviderDocumentObject");
                return 0;
            }
        }

        // Too lazy to parse the argument...
        App::PropertyStringList prop;
        if(pySubName!=Py_None)
            prop.setPyObject(pySubName);

        if(obj)
            getLinkViewPtr()->setLink(obj,prop.getValue());  
        else
            getLinkViewPtr()->setLinkViewObject(vpd,prop.getValue());  
        Py_Return;
    } PY_CATCH;
}

Py::Object LinkViewPy::getOwner() const {
    auto owner = getLinkViewPtr()->getOwner();
    if(!owner) return Py::Object();
    return Py::Object(owner->getPyObject(),true);
}

void LinkViewPy::setOwner(Py::Object owner) {
    ViewProviderDocumentObject *vp = 0;
    if(!owner.isNone()) {
        if(!PyObject_TypeCheck(owner.ptr(),&ViewProviderDocumentObjectPy::Type))
            throw Py::TypeError("exepcting the owner to be of ViewProviderDocumentObject");
        vp = static_cast<ViewProviderDocumentObjectPy*>(
                owner.ptr())->getViewProviderDocumentObjectPtr();
    }
    getLinkViewPtr()->setOwner(vp);
}

Py::Object LinkViewPy::getLinkedView() const {
    auto linked = getLinkViewPtr()->getLinkedView();
    if(!linked)
        return Py::Object();
    return Py::Object(linked->getPyObject(),true);
}

Py::Object LinkViewPy::getSubNames() const {
    const auto &subs = getLinkViewPtr()->getSubNames();
    if(subs.empty())
        return Py::Object();
    Py::Tuple ret(subs.size());
    int i=0;
    for(auto &s : subs)
        ret.setItem(i++,Py::String(s.c_str()));
    return ret;
}

PyObject* LinkViewPy::getElementPicked(PyObject* args)
{
    PyObject *obj;
    if (!PyArg_ParseTuple(args, "O",&obj))
        return NULL;
    void *ptr = 0;
    Base::Interpreter().convertSWIGPointerObj("pivy.coin", "SoPickedPoint *", obj, &ptr, 0);
    SoPickedPoint *pp = reinterpret_cast<SoPickedPoint*>(ptr);
    if(!pp) 
        throw Py::TypeError("type must be of coin.SoPickedPoint");
    PY_TRY{
        std::string name;
        if(!getLinkViewPtr()->linkGetElementPicked(pp,name))
            Py_Return;
        return Py::new_reference_to(Py::String(name));
    }PY_CATCH
}

PyObject* LinkViewPy::getDetailPath(PyObject* args)
{
    const char *sub;
    PyObject *path;
    if (!PyArg_ParseTuple(args, "sO",&sub,&path))
        return NULL;
    void *ptr = 0;
    Base::Interpreter().convertSWIGPointerObj("pivy.coin", "SoPath *", path, &ptr, 0);
    SoPath *pPath = reinterpret_cast<SoPath*>(ptr);
    if(!pPath) 
        throw Py::TypeError("type must be of coin.SoPath");
    PY_TRY{
        SoDetail *det = 0;
        getLinkViewPtr()->linkGetDetailPath(sub,static_cast<SoFullPath*>(pPath),det);
        if(!det)
            Py_Return;
        return Base::Interpreter().createSWIGPointerObj("pivy.coin", "SoDetail *", (void*)det, 0);
    }PY_CATCH
}

PyObject* LinkViewPy::getBoundBox(PyObject* args) {
    PyObject *vobj = Py_None;
    if (!PyArg_ParseTuple(args, "O",&vobj))
        return 0;
    ViewProviderDocumentObject *vpd = 0;
    if(vobj!=Py_None) {
        if(!PyObject_TypeCheck(vobj,&ViewProviderDocumentObjectPy::Type)) {
            PyErr_SetString(PyExc_TypeError, "exepcting a type of ViewProviderDocumentObject");
            return 0;
        }
        vpd = static_cast<ViewProviderDocumentObjectPy*>(vobj)->getViewProviderDocumentObjectPtr();
    }
    PY_TRY {
        auto bbox = getLinkViewPtr()->getBoundBox(vpd);
        Py::Object ret(new Base::BoundBoxPy(new Base::BoundBox3d(bbox)));
        return Py::new_reference_to(ret);
    }PY_CATCH
}

PyObject *LinkViewPy::getCustomAttributes(const char*) const
{
    return 0;
}

int LinkViewPy::setCustomAttributes(const char*, PyObject*)
{
    return 0;
}

Py::Object LinkViewPy::getRootNode(void) const
{
    try {
        SoNode* node = getLinkViewPtr()->getLinkRoot();
        PyObject* Ptr = Base::Interpreter().createSWIGPointerObj("pivy.coin","SoSeparator *", node, 1);
        node->ref();
        return Py::Object(Ptr, true);
    }
    catch (const Base::Exception& e) {
        throw Py::RuntimeError(e.what());
    }
}

Py::Object LinkViewPy::getVisibilities() const {
    auto linked = getLinkViewPtr();
    if(!linked->getSize())
        return Py::Object();
    Py::Tuple ret(linked->getSize());
    for(int i=0;i<linked->getSize();++i)
        ret.setItem(i,Py::Boolean(linked->isElementVisible(i)));
    return ret;
}

void LinkViewPy::setVisibilities(Py::Object value) {
    App::PropertyBoolList v;
    if(!value.isNone())
        v.setPyObject(value.ptr());

    auto linked = getLinkViewPtr();
    const auto &vis = v.getValue();
    for(int i=0;i<linked->getSize();++i)
        linked->setElementVisible(i,i>=(int)vis.size()||vis[i]);
}

PyObject* LinkViewPy::getChildren(PyObject *args) {
    if (!PyArg_ParseTuple(args, ""))
        return 0;
    auto children = getLinkViewPtr()->getChildren();
    if(children.empty())
        Py_Return;
    Py::Tuple ret(children.size());
    int i=0;
    for(auto vp : children)
        ret.setItem(i++,Py::Object(vp->getPyObject(),true));
    return Py::new_reference_to(ret);
}

Py::Int LinkViewPy::getCount() const {
    return Py::Int(getLinkViewPtr()->getSize());
}

void LinkViewPy::setCount(Py::Int count) {
    try {
        getLinkViewPtr()->setSize((int)count);
    } catch (const Base::Exception& e) {
        throw Py::RuntimeError(e.what());
    }
}

