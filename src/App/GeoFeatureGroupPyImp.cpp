
#include "PreCompiled.h"

#include "App/GeoFeatureGroup.h"

// inclusion of the generated files (generated out of GeoFeatureGroupPy.xml)
#include "GeoFeatureGroupPy.h"
#include "GeoFeatureGroupPy.cpp"

using namespace App;

// returns a string which represents the object e.g. when printed in python
std::string GeoFeatureGroupPy::representation(void) const
{
    return std::string("<GeoFeatureGroup object>");
}


PyObject *GeoFeatureGroupPy::getCustomAttributes(const char* /*attr*/) const
{
    return 0;
}

int GeoFeatureGroupPy::setCustomAttributes(const char* /*attr*/, PyObject* /*obj*/)
{
    return 0;
}


