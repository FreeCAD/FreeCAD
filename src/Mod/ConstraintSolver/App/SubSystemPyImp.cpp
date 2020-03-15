#include "PreCompiled.h"

#include "SubSystemPy.h"
#include "SubSystemPy.cpp"

#include "ParameterRefPy.h"
#include "ParameterSubsetPy.h"
#include "ConstraintPy.h"
#include "PyUtils.h"

PyObject *SubSystemPy::PyMake(struct _typeobject *, PyObject *, PyObject *)  // Python wrapper
{
    // create a new instance of SubSystemPy and the Twin object
    return Py::new_reference_to((new SubSystem)->self().getHandledObject());
}

// constructor method
int SubSystemPy::PyInit(PyObject* /*args*/, PyObject* /*kwd*/)
{
    return 0;
}

int SubSystemPy::initialization()
{
    getSubSystemPtr()->_twin = this;
    return 0;
}
int SubSystemPy::finalization()
{
    return 0;
}

// returns a string which represents the object e.g. when printed in python
std::string SubSystemPy::representation(void) const
{
    return std::string("<SubSystem object>");
}

PyObject* SubSystemPy::touch(PyObject *args)
{
    if (!PyArg_ParseTuple(args, ""))
        return nullptr;
    getSubSystemPtr()->touch();
    return Py::new_reference_to(Py::None());
}

PyObject* SubSystemPy::addUnknown(PyObject *args)
{
    {
        PyObject* arg;
        if (PyArg_ParseTuple(args, "O!", &ParameterRefPy::Type, &arg)){
            getSubSystemPtr()->addUnknown(*HParameterRef(arg, false));
            return Py::new_reference_to(Py::None());
        }
        PyErr_Clear();
    }
    {
        PyObject* arg;
        if (PyArg_ParseTuple(args, "O!", &PyList_Type, &arg)){
            Py::List list (arg);
            for (Py::Object it: list){
                getSubSystemPtr()->addUnknown(
                    *pyTypeCheck<ParameterRefPy>(it.ptr())->getParameterRefPtr()
                );
            }
            return Py::new_reference_to(Py::None());
        }
        PyErr_Clear();
    }
    {
        PyObject* arg;
        if (PyArg_ParseTuple(args, "O!", &ParameterSubsetPy::Type, &arg)){
            getSubSystemPtr()->addUnknown(HParameterSubset(arg, false));
            return Py::new_reference_to(Py::None());
        }
        PyErr_Clear();
    }
    throw Py::TypeError("Call signature not supported. Can take: list of ParameterRef, or ParameterRef, or ParameterSubset");
}

PyObject* SubSystemPy::addConstraint(PyObject *args)
{
    {
        PyObject* arg;
        if (PyArg_ParseTuple(args, "O!", &ConstraintPy::Type, &arg)){
            getSubSystemPtr()->addConstraint(HConstraint(arg, false));
            return Py::new_reference_to(Py::None());
        }
        PyErr_Clear();
    }
    {
        PyObject* arg;
        if (PyArg_ParseTuple(args, "O!", &PyList_Type, &arg)){
            Py::List list (arg);
            for (Py::Object it: list){
                getSubSystemPtr()->addConstraint(HConstraint(
                    pyTypeCheck<ConstraintPy>(it.ptr())
                , false));
            }
            return Py::new_reference_to(Py::None());
        }
        PyErr_Clear();
    }
    throw Py::TypeError("Call signature not supported. Can take: list of Constraint, or Constraint");
}



Py::Object SubSystemPy::getParameterSet(void) const
{
    return getSubSystemPtr()->params().getHandledObject();
}

PyObject *SubSystemPy::getCustomAttributes(const char* /*attr*/) const
{
    return 0;
}

int SubSystemPy::setCustomAttributes(const char* /*attr*/, PyObject* /*obj*/)
{
    return 0;
}
