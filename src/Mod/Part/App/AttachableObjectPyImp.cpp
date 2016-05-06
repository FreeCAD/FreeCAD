
#include "PreCompiled.h"

#include "Mod/Part/App/AttachableObject.h"
#include "OCCError.h"

// inclusion of the generated files (generated out of AttachableObjectPy.xml)
#include "AttachableObjectPy.h"
#include "AttachableObjectPy.cpp"

using namespace Part;

// returns a string which represents the object e.g. when printed in python
std::string AttachableObjectPy::representation(void) const
{
    return std::string("<Part::AttachableObject>");
}

PyObject* AttachableObjectPy::positionBySupport(PyObject *args)
{
    if (!PyArg_ParseTuple(args, ""))
        return 0;
    bool bAttached = false;
    try{
        bAttached = this->getAttachableObjectPtr()->positionBySupport();
    } catch (Standard_Failure) {
        Handle_Standard_Failure e = Standard_Failure::Caught();
        PyErr_SetString(PartExceptionOCCError, e->GetMessageString());
        return NULL;
    } catch (Base::Exception &e) {
        PyErr_SetString(Base::BaseExceptionFreeCADError, e.what());
        return NULL;
    }
    return Py::new_reference_to(Py::Boolean(bAttached));
}


PyObject *AttachableObjectPy::getCustomAttributes(const char* /*attr*/) const
{
    return 0;
}

int AttachableObjectPy::setCustomAttributes(const char* /*attr*/, PyObject* /*obj*/)
{
    return 0;
}


