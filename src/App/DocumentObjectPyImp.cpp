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

#include "DocumentObject.h"
#include "Document.h"

// inclusion of the generated files (generated out of DocumentObjectPy.xml)
#include "DocumentObjectPy.h"
#include "DocumentObjectPy.cpp"

using namespace App;

// returns a string which represent the object e.g. when printed in python
std::string DocumentObjectPy::representation(void) const
{
    return std::string("<Document object>");
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

PyObject*  DocumentObjectPy::touch(PyObject * args)
{
    if (!PyArg_ParseTuple(args, ""))     // convert args: Python->C 
        return NULL;                    // NULL triggers exception 
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
    if (uptodate) {
        list.append(Py::String("Up-to-date"));
    }
    return list;
}

Py::Object DocumentObjectPy::getViewObject(void) const
{
    try {
        Py::Module module(PyImport_ImportModule("FreeCADGui"),true);
        Py::Callable method(module.getAttr("getDocument"));
        Py::Tuple arg(1);
        arg.setItem(0, Py::String(getDocumentObjectPtr()->getDocument()->getName()));
        Py::Object doc = method.apply(arg);
        method = doc.getAttr("getObject");
        arg.setItem(0, Py::String(getDocumentObjectPtr()->getNameInDocument()));
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

Py::List DocumentObjectPy::getOutList(void) const
{
    Py::List ret;
    std::vector<DocumentObject*> list = getDocumentObjectPtr()->getOutList();

    for (std::vector<DocumentObject*>::iterator It=list.begin();It!=list.end();++It)
        ret.append(Py::Object((*It)->getPyObject(), true));

    return ret;
}

PyObject *DocumentObjectPy::getCustomAttributes(const char* /*attr*/) const
{
    return 0;
}

int DocumentObjectPy::setCustomAttributes(const char* attr, PyObject *obj)
{
    // search in PropertyList
    Property *prop = getDocumentObjectPtr()->getPropertyByName(attr);
    if (prop) {
        // Read-only attributes must not be set over its Python interface
        short Type =  getDocumentObjectPtr()->getPropertyType(prop);
        if (Type & Prop_ReadOnly) {
            std::stringstream s;
            s << "'DocumentObject' attribute '" << attr << "' is read-only"; 
            throw Py::AttributeError(s.str());
        }

        prop->setPyObject(obj);
        return 1;
    } 

    return 0;
}
