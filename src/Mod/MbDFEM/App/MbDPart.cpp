// SPDX-License-Identifier: LGPL-2.1-or-later

#include "MbDPart.h"

#include <algorithm>

#include <App/Document.h>

#include "MbDGroup.h"
#include "MbDMarker.h"
#include "MbDPartPy.h"

PROPERTY_SOURCE(MbDFEM::MbDPart, App::DocumentObjectGroup)

MbDFEM::MbDPart::MbDPart()
{
    ADD_PROPERTY_TYPE(Placement,
                      (Base::Placement()),
                      "MbDFEM",
                      App::Prop_None,
                      "Placement of this part");
    ADD_PROPERTY_TYPE(markers,
                      (nullptr),
                      "MbDFEM",
                      App::Prop_None,
                      "Markers belonging to this part");
    ADD_PROPERTY_TYPE(_markersGroup,
                      (nullptr),
                      "MbDFEM",
                      App::Prop_Hidden,
                      "Tree group containing this part's markers");
}

void MbDFEM::MbDPart::addMarker(MbDMarker* marker)
{
    auto values = markers.getValues();
    if (marker && std::find(values.begin(), values.end(), marker) == values.end()) {
        values.push_back(marker);
        markers.setValues(values);
    }
    if (marker) {
        ensureMarkersGroup()->addObject(marker);
    }
}

App::DocumentObjectGroup* MbDFEM::MbDPart::ensureMarkersGroup()
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

PyObject* MbDFEM::MbDPart::getPyObject()
{
    if (PythonObject.is(Py::_None())) {
        PythonObject = Py::Object(new MbDPartPy(this), true);
    }
    return Py::new_reference_to(PythonObject);
}
