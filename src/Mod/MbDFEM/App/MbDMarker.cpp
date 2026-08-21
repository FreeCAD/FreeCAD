// SPDX-License-Identifier: LGPL-2.1-or-later

#include "MbDMarker.h"

#include <cstring>

PROPERTY_SOURCE(MbDFEM::MbDMarker, Part::Feature)

MbDFEM::MbDMarker::MbDMarker()
{
    ADD_PROPERTY_TYPE(Geometry,
                      (nullptr),
                      "MbDFEM",
                      App::Prop_None,
                      "Circular edge or cylindrical face referenced by this marker");
}

App::DocumentObjectExecReturn* MbDFEM::MbDMarker::execute()
{
    return App::DocumentObject::execute();
}

void MbDFEM::MbDMarker::handleChangedPropertyName(Base::XMLReader& reader,
                                                  const char* typeName,
                                                  const char* propName)
{
    Base::Type type = Base::Type::fromName(typeName);
    if (Geometry.getTypeId() == type && std::strcmp(propName, "Edge") == 0) {
        Geometry.Restore(reader);
    }
    else {
        Part::Feature::handleChangedPropertyName(reader, typeName, propName);
    }
}
