#include "PreCompiled.h"
#include "SubShapeBinder.h"

// The following template specialization won't compile in SubShapeBinder.cpp
// because the inclusion of <App/Part.h>, which defines class App::Part that
// confuses gcc and keep complaining about on App::Part::SubShapeBinderPython
// not declared.
namespace App {
PROPERTY_SOURCE_TEMPLATE(Part::SubShapeBinderPython, Part::SubShapeBinder)
template<> const char* Part::SubShapeBinderPython::getViewProviderName(void) const {
    return "PartGui::ViewProviderSubShapeBinderPython";
}
template class PartExport FeaturePythonT<Part::SubShapeBinder>;
}
