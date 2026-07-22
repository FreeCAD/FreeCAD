// SPDX-License-Identifier: LGPL-2.1-or-later

#include "MbDAssembly.h"

#include <algorithm>

#include <App/Document.h>

#include "MbDAssemblyPy.h"
#include "MbDGroup.h"
#include "MbDMarker.h"
#include "MbDPart.h"

PROPERTY_SOURCE(MbDFEM::MbDAssembly, App::DocumentObjectGroup)

MbDFEM::MbDAssembly::MbDAssembly()
{
    ADD_PROPERTY_TYPE(Placement,
                      (Base::Placement()),
                      "MbDFEM",
                      App::Prop_None,
                      "Placement of this assembly");
    ADD_PROPERTY_TYPE(parts,
                      (nullptr),
                      "MbDFEM",
                      App::Prop_None,
                      "Parts belonging to this assembly");
    ADD_PROPERTY_TYPE(markers,
                      (nullptr),
                      "MbDFEM",
                      App::Prop_None,
                      "Markers belonging to this assembly");
    ADD_PROPERTY_TYPE(_partsGroup,
                      (nullptr),
                      "MbDFEM",
                      App::Prop_Hidden,
                      "Tree group containing this assembly's parts");
    ADD_PROPERTY_TYPE(_markersGroup,
                      (nullptr),
                      "MbDFEM",
                      App::Prop_Hidden,
                      "Tree group containing this assembly's markers");
}

void MbDFEM::MbDAssembly::addPart(MbDPart* part)
{
    auto values = parts.getValues();
    if (part && std::find(values.begin(), values.end(), part) == values.end()) {
        values.push_back(part);
        parts.setValues(values);
    }

    ensureMarkersGroup();
    if (part) {
        ensurePartsGroup()->addObject(part);
    }
}

void MbDFEM::MbDAssembly::addMarker(MbDMarker* marker)
{
    auto values = markers.getValues();
    if (marker && std::find(values.begin(), values.end(), marker) == values.end()) {
        values.push_back(marker);
        markers.setValues(values);
    }

    if (marker) {
        ensureMarkersGroup()->addObject(marker);
    }
    ensurePartsGroup();
}

App::DocumentObjectGroup* MbDFEM::MbDAssembly::ensurePartsGroup()
{
    if (auto* group = dynamic_cast<App::DocumentObjectGroup*>(_partsGroup.getValue())) {
        return group;
    }

    const std::string name = std::string(getNameInDocument()) + "_Parts";
    auto* group = static_cast<App::DocumentObjectGroup*>(
        getDocument()->addObject("MbDFEM::MbDGroup", name.c_str()));
    group->Label.setValue("Parts");
    _partsGroup.setValue(group);
    App::GroupExtension::addObject(group);
    return group;
}

App::DocumentObjectGroup* MbDFEM::MbDAssembly::ensureMarkersGroup()
{
    if (auto* group = dynamic_cast<App::DocumentObjectGroup*>(_markersGroup.getValue())) {
        return group;
    }

    const std::string name = std::string(getNameInDocument()) + "_Markers";
    auto* group = static_cast<App::DocumentObjectGroup*>(
        getDocument()->addObject("MbDFEM::MbDGroup", name.c_str()));
    group->Label.setValue("Markers");
    _markersGroup.setValue(group);
    App::GroupExtension::addObject(group);
    return group;
}

PyObject* MbDFEM::MbDAssembly::getPyObject()
{
    if (PythonObject.is(Py::_None())) {
        PythonObject = Py::Object(new MbDAssemblyPy(this), true);
    }
    return Py::new_reference_to(PythonObject);
}
