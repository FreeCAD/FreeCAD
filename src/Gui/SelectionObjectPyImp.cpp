/***************************************************************************
 *   Copyright (c) 2009 Juergen Riegel (FreeCAD@juergen-riegel.net)        *
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

#include "Gui/SelectionObject.h"
#include "App/Document.h"
#include "App/DocumentObject.h"
#include "App/Application.h"

// inclusion of the generated files (generated out of SelectionObjectPy.xml)
#include "SelectionObjectPy.h"
#include "SelectionObjectPy.cpp"

using namespace Gui;

// returns a string which represents the object e.g. when printed in python
std::string SelectionObjectPy::representation(void) const
{
    return "<SelectionObject>";
}



PyObject* SelectionObjectPy::remove(PyObject * /*args*/)
{
    PyErr_SetString(PyExc_NotImplementedError, "Not yet implemented");
    return 0;
}

PyObject* SelectionObjectPy::isA(PyObject * /*args*/)
{
    PyErr_SetString(PyExc_NotImplementedError, "Not yet implemented");
    return 0;
}



Py::String SelectionObjectPy::getObjectName(void) const
{
    return Py::String(getSelectionObjectPtr()->getFeatName());
}

Py::List SelectionObjectPy::getSubElementNames(void) const
{
    Py::List temp;
    std::vector<std::string> objs = getSelectionObjectPtr()->getSubNames();

    for(std::vector<std::string>::const_iterator it= objs.begin();it!=objs.end();++it)
        temp.append(Py::String(*it));

    return temp;
}

Py::String SelectionObjectPy::getFullName(void) const
{
	std::string buf;
	//buf = getSelectionObjectPtr()->getDocName();
	//buf += ".";
	//buf += getSelectionObjectPtr()->getFeatName();
	//if(getSelectionObjectPtr()->getSubName()){
	//	buf += ".";
	//	buf += getSelectionObjectPtr()->getSubName();
	//}
    return Py::String(buf.c_str());
}

Py::String SelectionObjectPy::getDocumentName(void) const
{
    return Py::String(getSelectionObjectPtr()->getDocName());
}

Py::Object SelectionObjectPy::getDocument(void) const
{
    return Py::Object(getSelectionObjectPtr()->getObject()->getDocument()->getPyObject(), true);
}

Py::Object SelectionObjectPy::getObject(void) const
{
    return Py::Object(getSelectionObjectPtr()->getObject()->getPyObject(), true);
}

Py::List SelectionObjectPy::getSubObjects(void) const
{
    Py::List temp;
    std::vector<PyObject *> objs = getSelectionObjectPtr()->getObject()->getPySubObjects(getSelectionObjectPtr()->getSubNames());
    for(std::vector<PyObject *>::const_iterator it= objs.begin();it!=objs.end();++it)
        temp.append(Py::Object(*it,true));
    return temp;
}

Py::Boolean SelectionObjectPy::getHasSubObjects(void) const
{
    return Py::Boolean(getSelectionObjectPtr()->hasSubNames());
}

PyObject *SelectionObjectPy::getCustomAttributes(const char* /*attr*/) const
{
    return 0;
}

int SelectionObjectPy::setCustomAttributes(const char* /*attr*/, PyObject* /*obj*/)
{
    return 0; 
}


