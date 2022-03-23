
#include "PreCompiled.h"

#include "Mod/Part/App/AttachExtension.h"
#include "OCCError.h"

#include "AttachEnginePy.h"

// inclusion of the generated files (generated out of AttachExtensionPy.xml)
#include "AttachExtensionPy.h"
#include "AttachExtensionPy.cpp"

using namespace Part;

// returns a string which represents the object e.g. when printed in python
std::string AttachExtensionPy::representation(void) const
{
    return std::string("<Part::AttachableObject>");
}

PyObject* AttachExtensionPy::positionBySupport(PyObject *args)
{
    if (!PyArg_ParseTuple(args, ""))
        return nullptr;
    bool bAttached = false;
    try{
        bAttached = this->getAttachExtensionPtr()->positionBySupport();
    } catch (Standard_Failure& e) {
        PyErr_SetString(PartExceptionOCCError, e.GetMessageString());
        return nullptr;
    } catch (Base::Exception &e) {
        e.setPyException();
        return nullptr;
    }
    return Py::new_reference_to(Py::Boolean(bAttached));
}

PyObject* AttachExtensionPy::changeAttacherType(PyObject *args)
{
    const char* typeName;
    if (!PyArg_ParseTuple(args, "s", &typeName))
        return nullptr;
    bool ret;
    try{
        ret = this->getAttachExtensionPtr()->changeAttacherType(typeName);
    } catch (Standard_Failure& e) {
        PyErr_SetString(PartExceptionOCCError, e.GetMessageString());
        return nullptr;
    } catch (Base::Exception &e) {
        e.setPyException();
        return nullptr;
    }
    return Py::new_reference_to(Py::Boolean(ret));
}

Py::Object AttachExtensionPy::getAttacher(void) const
{
    try {
        this->getAttachExtensionPtr()->attacher(); //throws if attacher is not set
    } catch (Base::Exception&) {
        return Py::None();
    }

    try {
        return Py::Object( new Attacher::AttachEnginePy(this->getAttachExtensionPtr()->attacher().copy()), true);
    } catch (Standard_Failure& e) {
        throw Py::Exception(Part::PartExceptionOCCError, e.GetMessageString());
    } catch (Base::Exception &e) {
        e.setPyException();
        throw Py::Exception();
    }

}

PyObject *AttachExtensionPy::getCustomAttributes(const char* /*attr*/) const
{
    return nullptr;
}

int AttachExtensionPy::setCustomAttributes(const char* /*attr*/, PyObject* /*obj*/)
{
    return 0;
}


