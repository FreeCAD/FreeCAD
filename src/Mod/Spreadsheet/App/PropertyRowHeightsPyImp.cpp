
#include "PreCompiled.h"

#include "Mod/Spreadsheet/App/PropertyRowHeights.h"

// inclusion of the generated files (generated out of PropertyRowHeightsPy.xml)
#include "PropertyRowHeightsPy.h"
#include "PropertyRowHeightsPy.cpp"

using namespace Spreadsheet;

// returns a string which represents the object e.g. when printed in python
std::string PropertyRowHeightsPy::representation(void) const
{
    return std::string("<PropertyRowHeights object>");
}

PyObject *PropertyRowHeightsPy::PyMake(struct _typeobject *, PyObject *, PyObject *)  // Python wrapper
{
    // create a new instance of PropertyRowHeightsPy and the Twin object 
    return new PropertyRowHeightsPy(new PropertyRowHeights);
}

// constructor method
int PropertyRowHeightsPy::PyInit(PyObject* /*args*/, PyObject* /*kwd*/)
{
    return 0;
}






PyObject *PropertyRowHeightsPy::getCustomAttributes(const char* /*attr*/) const
{
    return 0;
}

int PropertyRowHeightsPy::setCustomAttributes(const char* /*attr*/, PyObject* /*obj*/)
{
    return 0; 
}


