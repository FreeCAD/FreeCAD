// SPDX-License-Identifier: LGPL-2.1-or-later

#include "MbDItemIJ.h"

#include "MbDItemIJPy.h"
#include "MbDMarker.h"

PROPERTY_SOURCE(MbDFEM::MbDItemIJ, App::DocumentObject)

MbDFEM::MbDItemIJ::MbDItemIJ()
{
    ADD_PROPERTY_TYPE(markerI,
                      (nullptr),
                      "MbDFEM",
                      App::Prop_None,
                      "First marker referenced by this item");
    markerI.setScope(App::LinkScope::Global);
    ADD_PROPERTY_TYPE(markerJ,
                      (nullptr),
                      "MbDFEM",
                      App::Prop_None,
                      "Second marker referenced by this item");
    markerJ.setScope(App::LinkScope::Global);
}

void MbDFEM::MbDItemIJ::setMarkerI(MbDMarker* marker)
{
    markerI.setValue(marker);
}

void MbDFEM::MbDItemIJ::setMarkerJ(MbDMarker* marker)
{
    markerJ.setValue(marker);
}

void MbDFEM::MbDItemIJ::setMarkers(MbDMarker* firstMarker, MbDMarker* secondMarker)
{
    setMarkerI(firstMarker);
    setMarkerJ(secondMarker);
}

PyObject* MbDFEM::MbDItemIJ::getPyObject()
{
    if (PythonObject.is(Py::_None())) {
        PythonObject = Py::Object(new MbDItemIJPy(this), true);
    }
    return Py::new_reference_to(PythonObject);
}
