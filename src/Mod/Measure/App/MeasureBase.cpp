/***************************************************************************
 *   Copyright (c) 2023 David Friedli <david[at]friedli-be.ch>             *
 *                                                                         *
 *   This file is part of FreeCAD.                                         *
 *                                                                         *
 *   FreeCAD is free software: you can redistribute it and/or modify it    *
 *   under the terms of the GNU Lesser General Public License as           *
 *   published by the Free Software Foundation, either version 2.1 of the  *
 *   License, or (at your option) any later version.                       *
 *                                                                         *
 *   FreeCAD is distributed in the hope that it will be useful, but        *
 *   WITHOUT ANY WARRANTY; without even the implied warranty of            *
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU      *
 *   Lesser General Public License for more details.                       *
 *                                                                         *
 *   You should have received a copy of the GNU Lesser General Public      *
 *   License along with FreeCAD. If not, see                               *
 *   <https://www.gnu.org/licenses/>.                                      *
 *                                                                         *
 **************************************************************************/

#include "PreCompiled.h"

#include <App/PropertyGeo.h>
#include <Base/PlacementPy.h>
#include <App/FeaturePythonPyImp.h>
#include <App/DocumentObjectPy.h>

#include "MeasureBase.h"
// Generated from MeasureBasePy.xml
#include "MeasureBasePy.h"

using namespace Measure;


PROPERTY_SOURCE(Measure::MeasureBase, App::DocumentObject)

MeasureBase::MeasureBase()
{
    ADD_PROPERTY_TYPE(
        Placement,
        (Base::Placement()),
        nullptr,
        App::PropertyType(App::Prop_ReadOnly | App::Prop_Output | App::Prop_NoRecompute),
        "Visual placement of the measurement");
}


PyObject* MeasureBase::getPyObject(void)
{
    if (PythonObject.is(Py::_None())) {
        // ref counter is set to 1
        PythonObject = Py::Object(new MeasureBasePy(this), true);
    }
    return Py::new_reference_to(PythonObject);
}

Py::Object MeasureBase::getProxyObject() const
{
    Base::PyGILStateLocker lock;
    App::Property* prop = this->getPropertyByName("Proxy");
    if (!prop) {
        return Py::None();
    }
    return dynamic_cast<App::PropertyPythonObject*>(prop)->getValue();
};

std::vector<App::DocumentObject*> MeasureBase::getSubject() const
{
    Base::PyGILStateLocker lock;

    Py::Object proxy = getProxyObject();

    // Pass the feature object to the proxy
    Py::Tuple args(1);
    args.setItem(0, Py::Object(const_cast<MeasureBase*>(this)->getPyObject()));

    Py::Object ret;
    try {
        ret = proxy.callMemberFunction("getSubject", args);
    }
    catch (Py::Exception&) {
        Base::PyException e;
        e.reportException();
        return {};
    }

    Py::Sequence retTuple(ret);
    std::vector<App::DocumentObject*> retVec;
    for (Py::Object o : retTuple) {
        retVec.push_back(static_cast<App::DocumentObjectPy*>(o.ptr())->getDocumentObjectPtr());
    }

    return retVec;
};


void MeasureBase::parseSelection(const App::MeasureSelection& selection)
{
    Base::PyGILStateLocker lock;

    Py::Object proxy = getProxyObject();

    // Convert selection to python list
    Py::Tuple selectionPy = App::MeasureManager::getSelectionPy(selection);

    Py::Tuple args(2);

    // Pass the feature object to the proxy
    args.setItem(0, Py::Object(const_cast<MeasureBase*>(this)->getPyObject()));
    args.setItem(1, selectionPy);

    // Call the parseSelection method of the proxy object
    try {
        proxy.callMemberFunction("parseSelection", args);
    }
    catch (Py::Exception&) {
        Base::PyException e;
        e.reportException();
    }
}


std::vector<std::string> MeasureBase::getInputProps()
{
    Base::PyGILStateLocker lock;
    Py::Object proxy = getProxyObject();

    if (proxy.isNone()) {
        return {};
    }

    Py::Object ret;
    try {
        ret = proxy.callMemberFunction("getInputProps");
    }
    catch (Py::Exception&) {
        Base::PyException e;
        e.reportException();
        return {};
    }
    Py::Sequence propsPy(ret);

    // Get cpp vector from propsPy
    std::vector<std::string> props;
    for (Py::Object o : propsPy) {
        props.push_back(o.as_string());
    }

    return props;
}


QString MeasureBase::getResultString()
{
    Py::Object proxy = getProxyObject();
    Base::PyGILStateLocker lock;

    if (!proxy.isNone()) {

        // Pass the feature object to the proxy
        Py::Tuple args(1);
        args.setItem(0, Py::Object(const_cast<MeasureBase*>(this)->getPyObject()));

        Py::Object ret;
        try {
            ret = proxy.callMemberFunction("getResultString", args);
        }
        catch (Py::Exception&) {
            Base::PyException e;
            e.reportException();
            return QString();
        }
        return QString::fromStdString(ret.as_string());
    }

    App::Property* prop = getResultProp();
    if (prop == nullptr) {
        return QString();
    }

    if (prop->isDerivedFrom<App::PropertyQuantity>()) {
        return QString::fromStdString(
            static_cast<App::PropertyQuantity*>(prop)->getQuantityValue().getUserString());
    }


    return QString();
}

void MeasureBase::onDocumentRestored()
{
    // Force recompute the measurement
    recompute();
}

Base::Placement MeasureBase::getPlacement()
{
    return this->Placement.getValue();
}


// Python Drawing feature ---------------------------------------------------------

namespace App
{
/// @cond DOXERR
PROPERTY_SOURCE_TEMPLATE(Measure::MeasurePython, Measure::MeasureBase)
template<>
const char* Measure::MeasurePython::getViewProviderName(void) const
{
    return "MeasureGui::ViewProviderMeasure";
}
template<>
PyObject* Measure::MeasurePython::getPyObject()
{
    if (PythonObject.is(Py::_None())) {
        // ref counter is set to 1
        PythonObject = Py::Object(new FeaturePythonPyT<Measure::MeasureBasePy>(this), true);
    }
    return Py::new_reference_to(PythonObject);
}
/// @endcond

// explicit template instantiation
template class MeasureExport FeaturePythonT<Measure::MeasureBase>;
}  // namespace App
