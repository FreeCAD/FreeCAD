// SPDX-License-Identifier: LGPL-2.1-or-later

#include "MbDPart.h"

#include <algorithm>

#include <App/Document.h>

#include "MbDFolders.h"
#include "MbDMarker.h"
#include "MbDPartPy.h"

PROPERTY_SOURCE(MbDFEM::MbDPart, App::DocumentObject)

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
    ADD_PROPERTY_TYPE(_markersFolder,
                      (nullptr),
                      "MbDFEM",
                      App::Prop_Hidden,
                      "Tree folder containing this part's markers");
}

void MbDFEM::MbDPart::addMarker(MbDMarker* marker)
{
    auto values = markers.getValues();
    if (marker && std::find(values.begin(), values.end(), marker) == values.end()) {
        values.push_back(marker);
        markers.setValues(values);
    }

    if (marker) {
        auto* folder = ensureMarkersFolder();
        if (folder && !folder->hasObject(marker)) {
            folder->addObject(marker);
        }
    }
}

App::DocumentObjectGroup* MbDFEM::MbDPart::getMarkersFolder() const
{
    return dynamic_cast<App::DocumentObjectGroup*>(_markersFolder.getValue());
}

App::DocumentObjectGroup* MbDFEM::MbDPart::ensureMarkersFolder()
{
    if (auto* folder = getMarkersFolder()) {
        return folder;
    }
    if (!getDocument()) {
        return nullptr;
    }

    const std::string name = std::string(getNameInDocument()) + "_Markers";
    auto* folder = static_cast<App::DocumentObjectGroup*>(
        getDocument()->addObject("MbDFEM::MbDMarkersFolder", name.c_str()));
    folder->Label.setValue("Markers");
    _markersFolder.setValue(folder);
    return folder;
}

PyObject* MbDFEM::MbDPart::getPyObject()
{
    if (PythonObject.is(Py::_None())) {
        PythonObject = Py::Object(new MbDPartPy(this), true);
    }
    return Py::new_reference_to(PythonObject);
}
