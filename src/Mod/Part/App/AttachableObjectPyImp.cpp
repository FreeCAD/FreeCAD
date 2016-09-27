
#include "PreCompiled.h"

#include "Mod/Part/App/AttachableObject.h"
#include "OCCError.h"

#include "AttachEnginePy.h"

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

PyObject* AttachableObjectPy::changeAttacherType(PyObject *args)
{
    const char* typeName;
    if (!PyArg_ParseTuple(args, "s", &typeName))
        return 0;
    bool ret;
    try{
        ret = this->getAttachableObjectPtr()->changeAttacherType(typeName);
    } catch (Standard_Failure) {
        Handle_Standard_Failure e = Standard_Failure::Caught();
        PyErr_SetString(PartExceptionOCCError, e->GetMessageString());
        return NULL;
    } catch (Base::Exception &e) {
        PyErr_SetString(Base::BaseExceptionFreeCADError, e.what());
        return NULL;
    }
    return Py::new_reference_to(Py::Boolean(ret));
}

Py::Object AttachableObjectPy::getAttacher(void) const
{
    try {
        this->getAttachableObjectPtr()->attacher(); //throws if attacher is not set
    } catch (Base::Exception) {
        return Py::None();
    }

    try {
        return Py::Object( new Attacher::AttachEnginePy(this->getAttachableObjectPtr()->attacher().copy()), true);
    } catch (Standard_Failure) {
        Handle_Standard_Failure e = Standard_Failure::Caught();
        throw Py::Exception(Part::PartExceptionOCCError, e->GetMessageString());
    } catch (Base::Exception &e) {
        throw Py::Exception(Base::BaseExceptionFreeCADError, e.what());
    }

}

PyObject *AttachableObjectPy::getCustomAttributes(const char* /*attr*/) const
{
    return 0;
}

int AttachableObjectPy::setCustomAttributes(const char* /*attr*/, PyObject* /*obj*/)
{
    return 0;
}


