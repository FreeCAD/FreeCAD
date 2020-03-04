#include "PreCompiled.h"

#include <Base/PyObjectBase.h>
#include "PyUtils.h"

#include <Base/DualNumberPy.h>

using namespace FCS;

PyObject* FCS::pyTryCatch(std::function<Py::Object()> body, PyObject* errorreturn)
{
    try {
        return Py::new_reference_to(body());
    }
    catch(Base::AbortException &e)
    {
        e.ReportException();
        PyErr_SetObject(Base::BaseExceptionFreeCADAbort,e.getPyObject());
        return errorreturn;
    }
    catch(Base::Exception &e)
    {
        auto pye = e.getPyExceptionType();
        if(!pye)
            pye = Base::BaseExceptionFreeCADError;
        PyErr_SetObject(pye,e.getPyObject());
        return errorreturn;
    }
    catch(std::exception &e)
    {
        PyErr_SetString(Base::BaseExceptionFreeCADError,e.what());
        return errorreturn;
    }
    catch(const Py::Exception&)
    {
        // The exception text is already set
        return errorreturn;
    }
    catch(const char *e)
    {
        PyErr_SetString(Base::BaseExceptionFreeCADError,e);
        return errorreturn;
    }
#ifndef DONT_CATCH_CXX_EXCEPTIONS
    catch(...)
    {
        PyErr_SetString(Base::BaseExceptionFreeCADError,"Unknown C++ exception");
        return NULL;
    }
#endif
}



Py::Module FCS::import(std::string modname)
{
    PyObject* pcmod = PyImport_ImportModule(modname.c_str());
    if (pcmod == nullptr)
        throw Py::Exception();
    return Py::Module(pcmod);
}

Base::DualNumber FCS::asDualNumber(PyObject* ob)
{
    if (PyObject_TypeCheck(ob, &Base::DualNumberPy::Type)) {
        return static_cast<Base::DualNumberPy*>(ob)->value;
    }
    else if (PyLong_Check(ob)) {
        return PyLong_AsDouble(ob);
    }
    else if (PyFloat_Check(ob)) {
        return PyFloat_AsDouble(ob);
    }
    else {
        throw Py::TypeError(Py::Object(ob).type().as_string() + " object can't be converted to a dual number");
    }
}
