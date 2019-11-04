#include "PreCompiled.h"

#include "ParameterRef.h"

#include "ParameterRefPy.h"
#include "ParameterRefPy.cpp"

std::string ParameterRefPy::representation(void) const
{
    return std::string("<ParameterRef object>");
}

PyObject* ParameterRefPy::isSameRef(PyObject *args)
{
    ParameterRefPy* pyother;
    if (!PyArg_ParseTuple(args, "O!", &(ParameterRefPy::Type), &pyother))
        return nullptr;
    bool ret = this->getParameterRefPtr()->isSameRef(*(pyother->getParameterRefPtr()));
    return Py::new_reference_to(Py::Boolean(ret));
}

PyObject* ParameterRefPy::isSameValue(PyObject *args)
{
    ParameterRefPy* pyother;
    if (!PyArg_ParseTuple(args, "O!", &(ParameterRefPy::Type), &pyother))
        return nullptr;
    bool ret = this->getParameterRefPtr()->isSameValue(*(pyother->getParameterRefPtr()));
    return Py::new_reference_to(Py::Boolean(ret));
}



Py::Object ParameterRefPy::getHost(void) const
{
    return this->getParameterRefPtr()->host();
}

Py::Long ParameterRefPy::getOwnIndex(void) const
{
    return Py::Long(this->getParameterRefPtr()->ownIndex());
}

Py::Long ParameterRefPy::getMasterIndex(void) const
{
    return Py::Long(this->getParameterRefPtr()->masterIndex());
}

Py::Object ParameterRefPy::getMaster(void) const
{
    return this->getParameterRefPtr()->getPyObject();
}

Py::Float ParameterRefPy::getValue(void) const
{
    return Py::Float(this->getParameterRefPtr()->value());
}

void  ParameterRefPy::setValue(Py::Float arg)
{
    this->getParameterRefPtr()->value() = arg.as_double();
}

Py::Float ParameterRefPy::getOwnValue(void) const
{
    return Py::Float(this->getParameterRefPtr()->ownValue());
}

void  ParameterRefPy::setOwnValue(Py::Float arg)
{
    this->getParameterRefPtr()->ownValue() = arg.as_double();
}

Py::Float ParameterRefPy::getMasterScale(void) const
{
    return Py::Float(this->getParameterRefPtr()->masterScale());
}

void  ParameterRefPy::setMasterScale(Py::Float arg)
{
    this->getParameterRefPtr()->masterScale() = arg.as_double();
}

Py::Float ParameterRefPy::getOwnScale(void) const
{
    return Py::Float(this->getParameterRefPtr()->ownScale());
}

void  ParameterRefPy::setOwnScale(Py::Float arg)
{
    this->getParameterRefPtr()->ownScale() = arg.as_double();
}

Py::Boolean ParameterRefPy::getFixed(void) const
{
    return Py::Boolean(this->getParameterRefPtr()->param().fixed);
}

void  ParameterRefPy::setFixed(Py::Boolean arg)
{
    this->getParameterRefPtr()->param().fixed = arg.as_bool();
}

Py::Long ParameterRefPy::getTag(void) const
{
    return Py::Long(this->getParameterRefPtr()->param().tag);
}

void  ParameterRefPy::setTag(Py::Long arg)
{
    this->getParameterRefPtr()->param().tag = arg.as_long();
}

Py::String ParameterRefPy::getLabel(void) const
{
    return Py::String(this->getParameterRefPtr()->param().label, "utf-8");
}

void  ParameterRefPy::setLabel(Py::String arg)
{
    this->getParameterRefPtr()->param().label = arg.as_std_string("utf-8");
}

PyObject *ParameterRefPy::getCustomAttributes(const char* /*attr*/) const
{
    return nullptr;
}

int ParameterRefPy::setCustomAttributes(const char* /*attr*/, PyObject* /*obj*/)
{
    return 0;
}
