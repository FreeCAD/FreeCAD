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

#include <Base/GeometryPyCXX.h>
#include <Base/MatrixPy.h>
#include "DocumentObject.h"
#include "Document.h"
#include "Expression.h"
#include "GeoFeature.h"
#include "GroupExtension.h"
#include "GeoFeatureGroupExtension.h"

// inclusion of the generated files (generated out of DocumentObjectPy.xml)
#include <App/DocumentObjectPy.h>
#include <App/DocumentObjectPy.cpp>

using namespace App;

// returns a string which represent the object e.g. when printed in python
std::string DocumentObjectPy::representation(void) const
{
    DocumentObject* object = this->getDocumentObjectPtr();
    std::stringstream str;
    str << "<" << object->getTypeId().getName() << " object>";
    return str.str();
}

Py::String DocumentObjectPy::getName(void) const
{
    DocumentObject* object = this->getDocumentObjectPtr();
    const char* internal = object->getNameInDocument();
    if (!internal) {
        throw Py::RuntimeError(std::string("This object is currently not part of a document"));
    }
    return Py::String(std::string(internal));
}

Py::String DocumentObjectPy::getFullName(void) const
{
    return Py::String(getDocumentObjectPtr()->getFullName());
}

Py::Object DocumentObjectPy::getDocument(void) const
{
    DocumentObject* object = this->getDocumentObjectPtr();
    Document* doc = object->getDocument();
    if (!doc) {
        return Py::None();
    }
    else {
        return Py::Object(doc->getPyObject(), true);
    }
}

PyObject*  DocumentObjectPy::addProperty(PyObject *args)
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
        prop = getDocumentObjectPtr()->addDynamicProperty(sType,sName,sGroup,sDocStr.c_str(),attr,
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

PyObject*  DocumentObjectPy::removeProperty(PyObject *args)
{
    char *sName;
    if (!PyArg_ParseTuple(args, "s", &sName))
        return NULL;

    try {
        bool ok = getDocumentObjectPtr()->removeDynamicProperty(sName);
        return Py_BuildValue("O", (ok ? Py_True : Py_False));
    }
    catch (const Base::Exception& e) {
        throw Py::RuntimeError(e.what());
    }
}

PyObject*  DocumentObjectPy::supportedProperties(PyObject *args)
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

PyObject*  DocumentObjectPy::touch(PyObject * args)
{
    char *propName = 0;
    if (!PyArg_ParseTuple(args, "|s",&propName))     // convert args: Python->C 
        return NULL;                    // NULL triggers exception 
    if(propName) {
        if(!propName[0]) {
            getDocumentObjectPtr()->touch(true);
            Py_Return;
        }
        auto prop = getDocumentObjectPtr()->getPropertyByName(propName);
        if(!prop) 
            throw Py::RuntimeError("Property not found");
        prop->touch();
        Py_Return;
    }

    getDocumentObjectPtr()->touch();
    Py_Return;
}

PyObject*  DocumentObjectPy::purgeTouched(PyObject * args)
{
    if (!PyArg_ParseTuple(args, ""))     // convert args: Python->C 
        return NULL;                    // NULL triggers exception 
    getDocumentObjectPtr()->purgeTouched();
    Py_Return;
}

PyObject*  DocumentObjectPy::enforceRecompute(PyObject * args)
{
    if (!PyArg_ParseTuple(args, ""))     // convert args: Python->C
        return NULL;                    // NULL triggers exception
    getDocumentObjectPtr()->enforceRecompute();
    Py_Return;
}

Py::List DocumentObjectPy::getState(void) const
{
    DocumentObject* object = this->getDocumentObjectPtr();
    Py::List list;
    bool uptodate = true;
    if (object->isTouched()) {
        uptodate = false;
        list.append(Py::String("Touched"));
    }
    if (object->isError()) {
        uptodate = false;
        list.append(Py::String("Invalid"));
    }
    if (object->isRecomputing()) {
        uptodate = false;
        list.append(Py::String("Recompute"));
    }
    if (object->testStatus(App::Recompute2)) {
        list.append(Py::String("Recompute2"));
    }
    if (object->isRestoring()) {
        uptodate = false;
        list.append(Py::String("Restore"));
    }
    if (object->testStatus(App::Expand)){
        list.append(Py::String("Expanded"));
    }
    if (object->testStatus(App::PartialObject)){
        list.append(Py::String("Partial"));
    }
    if (object->testStatus(App::ObjImporting)){
        list.append(Py::String("Importing"));
    }
    if (uptodate) {
        list.append(Py::String("Up-to-date"));
    }
    return list;
}

Py::Object DocumentObjectPy::getViewObject(void) const
{
    try {
        Py::Module module(PyImport_ImportModule("FreeCADGui"),true);
        if (!module.hasAttr("getDocument")) {
            // in v0.14+, the GUI module can be loaded in console mode (but doesn't have all its document methods)
            return Py::None();
        }
        if(!getDocumentObjectPtr()->getDocument()) {
            throw Py::RuntimeError("Object has no document");
        }
        const char* internalName = getDocumentObjectPtr()->getNameInDocument();
        if (!internalName) {
            throw Py::RuntimeError("Object has been removed from document");
        }

        Py::Callable method(module.getAttr("getDocument"));
        Py::Tuple arg(1);
        arg.setItem(0, Py::String(getDocumentObjectPtr()->getDocument()->getName()));
        Py::Object doc = method.apply(arg);
        method = doc.getAttr("getObject");
        arg.setItem(0, Py::String(internalName));
        Py::Object obj = method.apply(arg);
        return obj;
    }
    catch (Py::Exception& e) {
        if (PyErr_ExceptionMatches(PyExc_ImportError)) {
            // the GUI is not up, hence None is returned
            e.clear();
            return Py::None();
        }
        // FreeCADGui is loaded, so there must be wrong something else
        throw; // re-throw
    }
}

Py::List DocumentObjectPy::getInList(void) const
{
    Py::List ret;
    std::vector<DocumentObject*> list = getDocumentObjectPtr()->getInList();

    for (std::vector<DocumentObject*>::iterator It=list.begin();It!=list.end();++It)
        ret.append(Py::Object((*It)->getPyObject(), true));

    return ret;
}

Py::List DocumentObjectPy::getInListRecursive(void) const
{
    Py::List ret;
    try {
        std::vector<DocumentObject*> list = getDocumentObjectPtr()->getInListRecursive();

        for (std::vector<DocumentObject*>::iterator It = list.begin(); It != list.end(); ++It)
            ret.append(Py::Object((*It)->getPyObject(), true));
 
    }
    catch (const Base::Exception& e) {
        throw Py::IndexError(e.what());
    }
    return ret;    
}

Py::List DocumentObjectPy::getInListEx(void) const
{
    Py::List ret;
    for(auto obj : getDocumentObjectPtr()->getInListEx(false))
        ret.append(Py::Object(obj->getPyObject(), true));
    return ret;    
}

Py::List DocumentObjectPy::getInListExRecursive(void) const
{
    Py::List ret;
    for(auto obj : getDocumentObjectPtr()->getInListEx(true))
        ret.append(Py::Object(obj->getPyObject(), true));
    return ret;    
}

Py::List DocumentObjectPy::getOutList(void) const
{
    Py::List ret;
    std::vector<DocumentObject*> list = getDocumentObjectPtr()->getOutList();

    for (std::vector<DocumentObject*>::iterator It=list.begin();It!=list.end();++It)
        ret.append(Py::Object((*It)->getPyObject(), true));

    return ret;
}

Py::List DocumentObjectPy::getOutListRecursive(void) const
{
    Py::List ret;
    try {
        std::vector<DocumentObject*> list = getDocumentObjectPtr()->getOutListRecursive();

        // create the python list for the output
        for (std::vector<DocumentObject*>::iterator It = list.begin(); It != list.end(); ++It)
            ret.append(Py::Object((*It)->getPyObject(), true));
    }
    catch (const Base::Exception& e) {
        throw Py::IndexError(e.what());
    }

    return ret;
}

PyObject*  DocumentObjectPy::setExpression(PyObject * args)
{
    char * path = NULL;
    PyObject * expr;
    char * comment = 0;

    if (!PyArg_ParseTuple(args, "sO|s", &path, &expr, &comment))     // convert args: Python->C
        return NULL;                    // NULL triggers exception

    App::ObjectIdentifier p(ObjectIdentifier::parse(getDocumentObjectPtr(), path));

    if (Py::Object(expr).isNone())
        getDocumentObjectPtr()->setExpression(p, boost::shared_ptr<Expression>());
#if PY_MAJOR_VERSION >= 3
    else if (PyUnicode_Check(expr)) {
        const char * exprStr = PyUnicode_AsUTF8(expr);
#else
    else if (PyString_Check(expr)) {
        const char * exprStr = PyString_AsString(expr);
#endif
        boost::shared_ptr<Expression> shared_expr(Expression::parse(getDocumentObjectPtr(), exprStr));
        if(shared_expr && comment)
            shared_expr->comment = comment;

        getDocumentObjectPtr()->setExpression(p, shared_expr);
    }
    else if (PyUnicode_Check(expr)) {
#if PY_MAJOR_VERSION >= 3
        std::string exprStr = PyUnicode_AsUTF8(expr);
#else
        PyObject* unicode = PyUnicode_AsEncodedString(expr, "utf-8", 0);
        if (unicode) {
            std::string exprStr = PyString_AsString(unicode);
            Py_DECREF(unicode);
            boost::shared_ptr<Expression> shared_expr(Expression::parse(getDocumentObjectPtr(), exprStr.c_str()));

            if(shared_expr && comment)
                shared_expr->comment = comment;
            getDocumentObjectPtr()->setExpression(p, shared_expr);
        }
        else {
            // utf-8 encoding failed
            return 0;
        }
#endif
    }
    else
        throw Py::TypeError("String or None expected.");
    Py_Return;
}

PyObject*  DocumentObjectPy::recompute(PyObject *args)
{
    PyObject *recursive=Py_False;
    if (!PyArg_ParseTuple(args, "|O",&recursive))
        return NULL;

    try {
        bool ok = getDocumentObjectPtr()->recomputeFeature(PyObject_IsTrue(recursive));
        return Py_BuildValue("O", (ok ? Py_True : Py_False));
    }
    catch (const Base::Exception& e) {
        throw Py::RuntimeError(e.what());
    }
}

PyObject*  DocumentObjectPy::getSubObject(PyObject *args, PyObject *keywds)
{
    PyObject *obj;
    short retType = 0;
    PyObject *pyMat = Py_None;
    PyObject *doTransform = Py_True;
    short depth = 0;
    static char *kwlist[] = {"subname","retType","matrix","transform","depth", NULL};
    if (!PyArg_ParseTupleAndKeywords(args, keywds, "O|hOOh", kwlist,
                &obj,&retType,&pyMat,&doTransform,&depth))
        return 0;

    if(retType<0 || retType>6) {
        PyErr_SetString(PyExc_TypeError, "invalid retType, can only be integer 0~6");
        return 0;
    }

    std::vector<std::string> subs;
    bool single=true;
    if (PyUnicode_Check(obj)) {
#if PY_MAJOR_VERSION >= 3
        subs.push_back(PyUnicode_AsUTF8(obj));
#else
        PyObject* unicode = PyUnicode_AsUTF8String(obj);
        subs.push_back(PyString_AsString(unicode));
        Py_DECREF(unicode);
    }
    else if (PyString_Check(obj)) {
        subs.push_back(PyString_AsString(obj));
#endif
    } else if (PySequence_Check(obj)) {
        single=false;
        Py::Sequence shapeSeq(obj);
        for (Py::Sequence::iterator it = shapeSeq.begin(); it != shapeSeq.end(); ++it) {
            PyObject* item = (*it).ptr();
            if (PyUnicode_Check(item)) {
#if PY_MAJOR_VERSION >= 3
               subs.push_back(PyUnicode_AsUTF8(item));
#else
                PyObject* unicode = PyUnicode_AsUTF8String(item);
                subs.push_back(PyString_AsString(unicode));
                Py_DECREF(unicode);
            }
            else if (PyString_Check(item)) {
                subs.push_back(PyString_AsString(item));
#endif
            }else{
                PyErr_SetString(PyExc_TypeError, "non-string object in sequence");
                return 0;
            }
        }
    }else{
        PyErr_SetString(PyExc_TypeError, "subname must be either a string or sequence of string");
        return 0;
    }

    bool transform = PyObject_IsTrue(doTransform);

    struct SubInfo {
        App::DocumentObject *sobj;
        Py::Object obj;
        Py::Object pyObj;
        Base::Matrix4D mat;
        SubInfo(const Base::Matrix4D &mat):mat(mat){}
    };

    Base::Matrix4D mat;
    if(pyMat!=Py_None) {
        if(!PyObject_TypeCheck(pyMat,&Base::MatrixPy::Type)) {
            PyErr_SetString(PyExc_TypeError, "expect argument 'matrix' to be of type Base.Matrix");
            return 0;
        }
        mat = *static_cast<Base::MatrixPy*>(pyMat)->getMatrixPtr();
    }

    PY_TRY {
        std::vector<SubInfo> ret;
        for(const auto &sub : subs) {
            ret.emplace_back(mat);
            auto &info = ret.back();
            PyObject *pyObj = 0;
            info.sobj = getDocumentObjectPtr()->getSubObject(
                    sub.c_str(),retType!=0&&retType!=2?0:&pyObj,&info.mat,transform,depth);
            if(pyObj)
                info.pyObj = Py::Object(pyObj,true);
            if(info.sobj) 
                info.obj = Py::Object(info.sobj->getPyObject(),true);
        }
        if(ret.empty())
            Py_Return;

        if(single) {
            if(retType==0)
                return Py::new_reference_to(ret[0].pyObj);
            else if(retType==1 && pyMat==Py_None)
                return Py::new_reference_to(ret[0].obj);
            else if(!ret[0].sobj)
                Py_Return;
            else if(retType==3)
                return Py::new_reference_to(Py::Placement(Base::Placement(ret[0].mat)));
            else if(retType==4)
                return Py::new_reference_to(Py::Matrix(ret[0].mat));
            else if(retType==5 || retType==6) {
                ret[0].sobj->getLinkedObject(true,&ret[0].mat,false);
                if(retType==5)
                    return Py::new_reference_to(Py::Placement(Base::Placement(ret[0].mat)));
                else
                    return Py::new_reference_to(Py::Matrix(ret[0].mat));
            }
            Py::Tuple rret(retType==1?2:3);
            rret.setItem(0,ret[0].obj);
            rret.setItem(1,Py::Object(new Base::MatrixPy(ret[0].mat)));
            if(retType!=1)
                rret.setItem(2,ret[0].pyObj);
            return Py::new_reference_to(rret);
        }
        Py::Tuple tuple(ret.size());
        for(size_t i=0;i<ret.size();++i) {
            if(retType==0)
                tuple.setItem(i,ret[i].pyObj);
            else if(retType==1 && pyMat==Py_None)
                tuple.setItem(i,ret[i].obj);
            else if(!ret[i].sobj)
                tuple.setItem(i, Py::Object());
            else if(retType==3)
                tuple.setItem(i,Py::Placement(Base::Placement(ret[0].mat)));
            else if(retType==4)
                tuple.setItem(i,Py::Matrix(ret[0].mat));
            else if(retType==5 || retType==6) {
                ret[i].sobj->getLinkedObject(true,&ret[i].mat,false);
                if(retType==5)
                    tuple.setItem(i,Py::Placement(Base::Placement(ret[i].mat)));
                else
                    tuple.setItem(i,Py::Matrix(ret[i].mat));
            } else {
                Py::Tuple rret(retType==1?2:3);
                rret.setItem(0,ret[i].obj);
                rret.setItem(1,Py::Object(new Base::MatrixPy(ret[i].mat)));
                if(retType!=1)
                    rret.setItem(2,ret[i].pyObj);
                tuple.setItem(i,rret);
            }
        }
        return Py::new_reference_to(tuple);
    }PY_CATCH
}

PyObject*  DocumentObjectPy::getSubObjects(PyObject *args) {
    int reason = 0;
    if (!PyArg_ParseTuple(args, "|i", &reason))
        return NULL;

    PY_TRY {
        auto names = getDocumentObjectPtr()->getSubObjects(reason);
        Py::Tuple pyObjs(names.size());
        for(size_t i=0;i<names.size();++i)
            pyObjs.setItem(i,Py::String(names[i]));
        return Py::new_reference_to(pyObjs);
    }PY_CATCH;
}

PyObject*  DocumentObjectPy::getLinkedObject(PyObject *args, PyObject *keywds)
{
    PyObject *recursive = Py_True;
    PyObject *pyMat = Py_None;
    PyObject *transform = Py_True;
    short depth = 0;
    static char *kwlist[] = {"recursive","matrix","transform","depth", NULL};
    if (!PyArg_ParseTupleAndKeywords(args, keywds, "|OOOh", kwlist,
                &recursive,&pyMat,&transform,&depth))
        return NULL;

    Base::Matrix4D _mat;
    Base::Matrix4D *mat = 0;
    if(pyMat!=Py_None) {
        if(!PyObject_TypeCheck(pyMat,&Base::MatrixPy::Type)) {
            PyErr_SetString(PyExc_TypeError, "expect argument 'matrix' to be of type Base.Matrix");
            return 0;
        }
        _mat = *static_cast<Base::MatrixPy*>(pyMat)->getMatrixPtr();
        mat = &_mat;
    }

    PY_TRY {
        auto linked = getDocumentObjectPtr()->getLinkedObject(
                PyObject_IsTrue(recursive), mat, PyObject_IsTrue(transform),depth);
        if(!linked)
            linked = getDocumentObjectPtr();
        auto pyObj = Py::Object(linked->getPyObject(),true);
        if(mat) {
            Py::Tuple ret(2);
            ret.setItem(0,pyObj);
            ret.setItem(1,Py::Object(new Base::MatrixPy(*mat)));
            return Py::new_reference_to(ret);
        }
        return Py::new_reference_to(pyObj);
    } PY_CATCH;
}

PyObject*  DocumentObjectPy::isElementVisible(PyObject *args)
{
    char *element = 0;
    if (!PyArg_ParseTuple(args, "s", &element))
        return NULL;
    PY_TRY {
        return Py_BuildValue("h", getDocumentObjectPtr()->isElementVisible(element));
    } PY_CATCH;
}

PyObject*  DocumentObjectPy::setElementVisible(PyObject *args)
{
    char *element = 0;
    PyObject *visible = Py_True;
    if (!PyArg_ParseTuple(args, "s|O", &element,&visible))
        return NULL;
    PY_TRY {
        return Py_BuildValue("h", getDocumentObjectPtr()->setElementVisible(element,PyObject_IsTrue(visible)));
    } PY_CATCH;
}

PyObject*  DocumentObjectPy::hasChildElement(PyObject *args)
{
    if (!PyArg_ParseTuple(args, ""))
        return NULL;
    PY_TRY {
        return Py_BuildValue("O", getDocumentObjectPtr()->hasChildElement()?Py_True:Py_False);
    } PY_CATCH;
}

PyObject*  DocumentObjectPy::getParentGroup(PyObject *args)
{
    if (!PyArg_ParseTuple(args, ""))
        return NULL;

    try {
        auto grp = GroupExtension::getGroupOfObject(getDocumentObjectPtr());
        if(!grp) {
            Py_INCREF(Py_None);
            return Py_None;
        }
        return grp->getPyObject();
    }
    catch (const Base::Exception& e) {
        throw Py::RuntimeError(e.what());
    }
}

PyObject*  DocumentObjectPy::getParentGeoFeatureGroup(PyObject *args)
{
    if (!PyArg_ParseTuple(args, ""))
        return NULL;

    try {
        auto grp = GeoFeatureGroupExtension::getGroupOfObject(getDocumentObjectPtr());
        if(!grp) {
            Py_INCREF(Py_None);
            return Py_None;
        }
        return grp->getPyObject();
    }
    catch (const Base::Exception& e) {
        throw Py::RuntimeError(e.what());
    }
}

Py::Boolean DocumentObjectPy::getMustExecute() const
{
    try {
        return Py::Boolean(getDocumentObjectPtr()->mustExecute()?true:false);
    }
    catch (const Base::Exception& e) {
        throw Py::RuntimeError(e.what());
    }
}

PyObject*  DocumentObjectPy::getPathsByOutList(PyObject *args)
{
    PyObject* o;
    if (!PyArg_ParseTuple(args, "O!", &DocumentObjectPy::Type, &o))
        return NULL;

    try {
        DocumentObject* target = static_cast<DocumentObjectPy*>
                (o)->getDocumentObjectPtr();
        auto array = getDocumentObjectPtr()->getPathsByOutList(target);
        Py::List list;
        for (auto it : array) {
            Py::List path;
            for (auto jt : it) {
                path.append(Py::asObject(jt->getPyObject()));
            }
            list.append(path);
        }
        return Py::new_reference_to(list);
    }
    catch (const Base::Exception& e) {
        throw Py::RuntimeError(e.what());
    }
}

PyObject *DocumentObjectPy::getElementMapVersion(PyObject *args) {
    const char *name;
    if (!PyArg_ParseTuple(args, "s", &name))
        return NULL;

    Property *prop = getDocumentObjectPtr()->getPropertyByName(name);
    if(!prop)
        throw Py::ValueError("property not found");
    return Py::new_reference_to(Py::String(getDocumentObjectPtr()->getElementMapVersion(prop)));
}

PyObject *DocumentObjectPy::getCustomAttributes(const char* attr) const
{
    // search for dynamic property
    Property* prop = getDocumentObjectPtr()->getDynamicPropertyByName(attr);
    if (prop)
        return prop->getPyObject();
    else
        return 0;
}

int DocumentObjectPy::setCustomAttributes(const char* attr, PyObject *obj)
{
    // explicitly search for dynamic property
    try {
        Property* prop = getDocumentObjectPtr()->getDynamicPropertyByName(attr);
        if (prop) {
            if(prop->testStatus(Property::Immutable)) {
                std::stringstream s;
                s << "'DocumentObject' attribute '" << attr << "' is read-only"; 
                throw Py::AttributeError(s.str());
            }
            prop->setPyObject(obj);
            return 1;
        }
    }
    catch (Base::ValueError &exc) {
        std::stringstream s;
        s << "Property '" << attr << "': " << exc.what();
        throw Py::ValueError(s.str());
    }
    catch (Base::Exception &exc) {
        std::stringstream s;
        s << "Attribute (Name: " << attr << ") error: '" << exc.what() << "' ";
        throw Py::AttributeError(s.str());
    }
    catch (Py::AttributeError &) {
        throw;
    }catch (...) {
        std::stringstream s;
        s << "Unknown error in attribute " << attr;
        throw Py::AttributeError(s.str());
    }

    // search in PropertyList
    Property *prop = getDocumentObjectPtr()->getPropertyByName(attr);
    if (prop) {
        // Read-only attributes must not be set over its Python interface
        if(prop->testStatus(Property::Immutable) ||
           (getDocumentObjectPtr()->getPropertyType(prop) & Prop_ReadOnly))
        {
            std::stringstream s;
            s << "'DocumentObject' attribute '" << attr << "' is read-only"; 
            throw Py::AttributeError(s.str());
        }

        try {
            prop->setPyObject(obj);
        }
        catch (const Base::TypeError& e) {
            std::stringstream s;
            s << "Property '" << prop->getName() << "': " << e.what();
            throw Py::TypeError(s.str());
        }
        return 1;
    } 

    return 0;
}

Py::Int DocumentObjectPy::getID() const {
    return Py::Int(getDocumentObjectPtr()->getID());
}

Py::Boolean DocumentObjectPy::getRemoving() const {
    return Py::Boolean(getDocumentObjectPtr()->testStatus(ObjectStatus::Remove));
}

PyObject *DocumentObjectPy::resolve(PyObject *args)
{
    const char *subname;
    if (!PyArg_ParseTuple(args, "s",&subname))
        return NULL;                             // NULL triggers exception 

    PY_TRY {
        std::string elementName;
        const char *subElement = 0;
        App::DocumentObject *parent = 0;
        auto obj = getDocumentObjectPtr()->resolve(subname,&parent,&elementName,&subElement);

        Py::Tuple ret(4);
        ret.setItem(0,obj?Py::Object(obj->getPyObject(),true):Py::None());
        ret.setItem(1,parent?Py::Object(parent->getPyObject(),true):Py::None());
        ret.setItem(2,Py::String(elementName.c_str()));
        ret.setItem(3,Py::String(subElement?subElement:""));
        return Py::new_reference_to(ret);
    } PY_CATCH;

    Py_Return;
}

PyObject *DocumentObjectPy::resolveSubElement(PyObject *args)
{
    const char *subname;
    PyObject *append = Py_False;
    int type = 0;
    if (!PyArg_ParseTuple(args, "s|Oi",&subname,&append,&type))
        return NULL;                             // NULL triggers exception 

    PY_TRY {
        std::pair<std::string,std::string> elementName;
        auto obj = GeoFeature::resolveElement(getDocumentObjectPtr(), subname,elementName,
                PyObject_IsTrue(append),(GeoFeature::ElementNameType)type);
        Py::Tuple ret(3);
        ret.setItem(0,obj?Py::Object(obj->getPyObject(),true):Py::None());
        ret.setItem(1,Py::String(elementName.first));
        ret.setItem(2,Py::String(elementName.second));
        return Py::new_reference_to(ret);
    } PY_CATCH;

    Py_Return;
}

Py::List DocumentObjectPy::getParents() const {
    Py::List ret;
    for(auto &v : getDocumentObjectPtr()->getParents())
        ret.append(Py::TupleN(Py::Object(v.first->getPyObject(),true),Py::String(v.second)));
    return ret;
}

PyObject *DocumentObjectPy::adjustRelativeLinks(PyObject *args) {
    PyObject *pyobj;
    PyObject *recursive = Py_True;
    if (!PyArg_ParseTuple(args, "O!|O",&DocumentObjectPy::Type,&pyobj,&recursive))
        return NULL;
    PY_TRY {
        auto obj = static_cast<DocumentObjectPy*>(pyobj)->getDocumentObjectPtr();
        auto inList = obj->getInListEx(true);
        inList.insert(obj);
        std::set<App::DocumentObject *> visited;
        return Py::new_reference_to(Py::Boolean(
                    getDocumentObjectPtr()->adjustRelativeLinks(inList,
                        PyObject_IsTrue(recursive)?&visited:nullptr)));
    }PY_CATCH
}

Py::String DocumentObjectPy::getOldLabel() const {
    return Py::String(getDocumentObjectPtr()->getOldLabel());
}

Py::Boolean DocumentObjectPy::getNoTouch() const {
    return Py::Boolean(getDocumentObjectPtr()->testStatus(ObjectStatus::NoTouch));
}

void DocumentObjectPy::setNoTouch(Py::Boolean value) {
    getDocumentObjectPtr()->setStatus(ObjectStatus::NoTouch,value.isTrue());
}
