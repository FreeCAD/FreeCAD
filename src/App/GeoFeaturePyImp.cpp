
#include "PreCompiled.h"

#include "App/GeoFeature.h"

// inclusion of the generated files (generated out of GeoFeaturePy.xml)
#include "GeoFeaturePy.h"
#include "GeoFeaturePy.cpp"

using namespace App;

// returns a string which represents the object e.g. when printed in python
std::string GeoFeaturePy::representation(void) const
{
    return std::string("<GeoFeature object>");
}



PyObject* GeoFeaturePy::getPaths(PyObject * /*args*/)
{
    PyErr_SetString(PyExc_NotImplementedError, "Not yet implemented");
    return 0;
}





PyObject *GeoFeaturePy::getCustomAttributes(const char* /*attr*/) const
{
    return 0;
}

int GeoFeaturePy::setCustomAttributes(const char* /*attr*/, PyObject* /*obj*/)
{
    return 0; 
}


