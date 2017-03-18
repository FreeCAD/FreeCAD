
#include "PreCompiled.h"

#include "Mod/Spreadsheet/App/PropertySheet.h"

// inclusion of the generated files (generated out of PropertySheetPy.xml)
#include "PropertySheetPy.h"
#include "PropertySheetPy.cpp"

using namespace Spreadsheet;

// returns a string which represents the object e.g. when printed in python
std::string PropertySheetPy::representation(void) const
{
    return std::string("<PropertySheet object>");
}

PyObject *PropertySheetPy::PyMake(struct _typeobject *, PyObject *, PyObject *)  // Python wrapper
{
    // create a new instance of PropertySheetPy and the Twin object 
    return new PropertySheetPy(new PropertySheet);
}

// constructor method
int PropertySheetPy::PyInit(PyObject* /*args*/, PyObject* /*kwd*/)
{
    return 0;
}






PyObject *PropertySheetPy::getCustomAttributes(const char* /*attr*/) const
{
    return 0;
}

int PropertySheetPy::setCustomAttributes(const char* /*attr*/, PyObject* /*obj*/)
{
    return 0; 
}


