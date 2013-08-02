/***************************************************************************
 *   Copyright (c) Jürgen Riegel          (juergen.riegel@web.de) 2002     *
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


#include <Base/Console.h>
#include <Base/Interpreter.h>
#include <Base/Reader.h>

#include "FeaturePython.h"
#include "FeaturePythonPyImp.h"

using namespace App;

FeaturePythonImp::FeaturePythonImp(App::DocumentObject* o) : object(o)
{
}

FeaturePythonImp::~FeaturePythonImp()
{
}

DocumentObjectExecReturn *FeaturePythonImp::execute()
{
    // Run the execute method of the proxy object.
    Base::PyGILStateLocker lock;
    try {
        Property* proxy = object->getPropertyByName("Proxy");
        if (proxy && proxy->getTypeId() == PropertyPythonObject::getClassTypeId()) {
            Py::Object feature = static_cast<PropertyPythonObject*>(proxy)->getValue();
            if (feature.hasAttr("__object__")) {
                Py::Callable method(feature.getAttr(std::string("execute")));
                Py::Tuple args(0);
                method.apply(args);
            }
            else {
                Py::Callable method(feature.getAttr(std::string("execute")));
                Py::Tuple args(1);
                args.setItem(0, Py::Object(object->getPyObject(), true));
                method.apply(args);
            }
        }
    }
    catch (Py::Exception&) {
        Base::PyException e; // extract the Python error text
        e.ReportException();
        std::stringstream str;
        str << object->Label.getValue() << ": " << e.what();
        return new App::DocumentObjectExecReturn(str.str());
    }

    return DocumentObject::StdReturn;
}

void FeaturePythonImp::onChanged(const Property* prop)
{
    // Run the execute method of the proxy object.
    Base::PyGILStateLocker lock;
    try {
        Property* proxy = object->getPropertyByName("Proxy");
        if (proxy && proxy->getTypeId() == PropertyPythonObject::getClassTypeId()) {
            Py::Object feature = static_cast<PropertyPythonObject*>(proxy)->getValue();
            if (feature.hasAttr(std::string("onChanged"))) {
                if (feature.hasAttr("__object__")) {
                    Py::Callable method(feature.getAttr(std::string("onChanged")));
                    Py::Tuple args(1);
                    std::string prop_name = object->getName(prop);
                    args.setItem(0, Py::String(prop_name));
                    method.apply(args);
                }
                else {
                    Py::Callable method(feature.getAttr(std::string("onChanged")));
                    Py::Tuple args(2);
                    args.setItem(0, Py::Object(object->getPyObject(), true));
                    std::string prop_name = object->getName(prop);
                    args.setItem(1, Py::String(prop_name));
                    method.apply(args);
                }
            }
        }
    }
    catch (Py::Exception&) {
        Base::PyException e; // extract the Python error text
        e.ReportException();
    }
}

PyObject *FeaturePythonImp::getPyObject(void)
{
    // ref counter is set to 1
    return new FeaturePythonPyT<DocumentObjectPy>(object);
}

// ---------------------------------------------------------

namespace App {
PROPERTY_SOURCE_TEMPLATE(App::FeaturePython, App::DocumentObject)
template<> const char* App::FeaturePython::getViewProviderName(void) const {
    return "Gui::ViewProviderPythonFeature";
}
template<> PyObject* App::FeaturePython::getPyObject(void) {
    if (PythonObject.is(Py::_None())) {
        // ref counter is set to 1
        PythonObject = Py::Object(new FeaturePythonPyT<DocumentObjectPy>(this),true);
    }
    return Py::new_reference_to(PythonObject);
}
// explicit template instantiation
template class AppExport FeaturePythonT<DocumentObject>;
}

// ---------------------------------------------------------

namespace App {
PROPERTY_SOURCE_TEMPLATE(App::GeometryPython, App::GeoFeature)
template<> const char* App::GeometryPython::getViewProviderName(void) const {
    return "Gui::ViewProviderPythonGeometry";
}
// explicit template instantiation
template class AppExport FeaturePythonT<GeoFeature>;}


