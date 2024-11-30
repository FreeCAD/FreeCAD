/***************************************************************************
 *   Copyright (c) 2010 Juergen Riegel <FreeCAD@juergen-riegel.net>        *
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

#include "Mod/PartDesign/App/Body.h"
#include "Mod/PartDesign/App/Feature.h"

// inclusion of the generated files (generated out of ItemPy.xml)
#include "BodyPy.h"
#include "BodyPy.cpp"

using namespace PartDesign;

// returns a string which represents the object e.g. when printed in python
std::string BodyPy::representation() const
{
    return {"<body object>"};
}



PyObject *BodyPy::getCustomAttributes(const char* /*attr*/) const
{
    return nullptr;
}

int BodyPy::setCustomAttributes(const char* /*attr*/, PyObject* /*obj*/)
{
    return 0;
}

PyObject* BodyPy::insertObject(PyObject *args)
{
    PyObject* featurePy;
    PyObject* targetPy;
    PyObject* afterPy = Py_False;
    if (!PyArg_ParseTuple(args, "O!O|O!", &(App::DocumentObjectPy::Type), &featurePy,
                                          &targetPy, &PyBool_Type, &afterPy)) {
        return nullptr;
    }

    App::DocumentObject* feature = static_cast<App::DocumentObjectPy*>(featurePy)->getDocumentObjectPtr();
    App::DocumentObject* target = nullptr;
    if (PyObject_TypeCheck(targetPy, &(App::DocumentObjectPy::Type))) {
        target = static_cast<App::DocumentObjectPy*>(targetPy)->getDocumentObjectPtr();
    }

    if (!Body::isAllowed(feature)) {
        PyErr_SetString(PyExc_SystemError, "Only PartDesign features, datum features and sketches can be inserted into a Body");
        return nullptr;
    }

    bool after = Base::asBoolean(afterPy);
    Body* body = this->getBodyPtr();

    try {
        body->insertObject(feature, target, after);
    }
    catch (Base::Exception& e) {
        PyErr_SetString(PyExc_SystemError, e.what());
        return nullptr;
    }

    Py_Return;
}

Py::Object BodyPy::getVisibleFeature() const {
    for(auto obj : getBodyPtr()->Group.getValues()) {
        if(obj->Visibility.getValue() && obj->isDerivedFrom(PartDesign::Feature::getClassTypeId()))
            return Py::Object(obj->getPyObject(),true);
    }
    return Py::Object();
}

