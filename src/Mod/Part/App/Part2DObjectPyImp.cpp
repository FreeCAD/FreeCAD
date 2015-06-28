
#include "PreCompiled.h"

#include "Mod/Part/App/Part2DObject.h"
#include "OCCError.h"

// inclusion of the generated files (generated out of Part2DObjectPy.xml)
#include "Part2DObjectPy.h"
#include "Part2DObjectPy.cpp"

using namespace Part;

// returns a string which represents the object e.g. when printed in python
std::string Part2DObjectPy::representation(void) const
{
    return std::string("<Part::Part2DObject>");
}

PyObject* Part2DObjectPy::positionBySupport(PyObject *args)
{
    if (!PyArg_ParseTuple(args, ""))
        return 0;
    try{
        this->getPart2DObjectPtr()->positionBySupport();
    } catch (Standard_Failure) {
        Handle_Standard_Failure e = Standard_Failure::Caught();
        PyErr_SetString(PartExceptionOCCError, e->GetMessageString());
        return NULL;
    } catch (Base::Exception &e) {
        PyErr_SetString(Base::BaseExceptionFreeCADError, e.what());
        return NULL;
    }
    Py_INCREF(Py_None);
    return Py_None;
}


PyObject *Part2DObjectPy::getCustomAttributes(const char* /*attr*/) const
{
    return 0;
}

int Part2DObjectPy::setCustomAttributes(const char* /*attr*/, PyObject* /*obj*/)
{
    return 0; 
}


