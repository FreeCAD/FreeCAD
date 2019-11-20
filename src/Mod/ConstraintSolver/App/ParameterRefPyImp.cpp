#include "PreCompiled.h"

#include "ParameterRef.h"

#include "ParameterRefPy.h"
#include "ParameterRefPy.cpp"

std::string ParameterRefPy::representation(void) const
{
    std::stringstream ss;
    if (getParameterRefPtr()->isNull())
        return "<ParameterRef (Null!)>";
    ss << "<";
    if (getParameterRefPtr()->param().label.size() == 0)
        ss << "unnamed ";
    ss << "ParameterRef [";
    if (getParameterRefPtr()->isMaster())
        ss << getParameterRefPtr()->masterIndex();
    else
        ss << getParameterRefPtr()->ownIndex() << "->" << getParameterRefPtr()->masterIndex();
    ss << "]";
    if (getParameterRefPtr()->param().label.size() > 0)
        ss << " '" << getParameterRefPtr()->param().label << "'";
    ss << ">";
    return ss.str();
}

PyObject* ParameterRefPy::isSameRef(PyObject* args)
{
    PyObject* pyother;
    if (!PyArg_ParseTuple(args, "O!", &(ParameterRefPy::Type), &pyother))
        return nullptr;
    getParameterRefPtr()->throwNull();
    bool ret = this->getParameterRefPtr()->isSameRef(*(UnsafePyHandle<ParameterRef>(pyother, false)));
    return Py::new_reference_to(Py::Boolean(ret));
}

PyObject* ParameterRefPy::isSameValue(PyObject* args)
{
    PyObject* pyother;
    if (!PyArg_ParseTuple(args, "O!", &(ParameterRefPy::Type), &pyother))
        return nullptr;
    getParameterRefPtr()->throwNull();
    bool ret = this->getParameterRefPtr()->isSameValue(*(UnsafePyHandle<ParameterRef>(pyother, false)));
    return Py::new_reference_to(Py::Boolean(ret));
}

PyObject* ParameterRefPy::isFixed(PyObject* args)
{
    if (! PyArg_ParseTuple(args, ""))
        return nullptr;
    getParameterRefPtr()->throwNull();
    return new_reference_to(Py::Boolean(getParameterRefPtr()->isFixed()));
}

PyObject* ParameterRefPy::fix(PyObject* args)
{
    if (! PyArg_ParseTuple(args, ""))
        return nullptr;
    getParameterRefPtr()->throwNull();
    getParameterRefPtr()->fix();
    return new_reference_to(Py::None());
}



Py::Object ParameterRefPy::getHost(void) const
{
    return this->getParameterRefPtr()->host();
}

Py::Long ParameterRefPy::getOwnIndex(void) const
{
    getParameterRefPtr()->throwNull();
    return Py::Long(this->getParameterRefPtr()->ownIndex());
}

Py::Long ParameterRefPy::getMasterIndex(void) const
{
    getParameterRefPtr()->throwNull();
    return Py::Long(this->getParameterRefPtr()->masterIndex());
}

Py::Object ParameterRefPy::getMaster(void) const
{
    getParameterRefPtr()->throwNull();
    return this->getParameterRefPtr()->getPyHandle();
}

Py::Float ParameterRefPy::getValue(void) const
{
    getParameterRefPtr()->throwNull();
    return Py::Float(this->getParameterRefPtr()->savedValue());
}

void  ParameterRefPy::setValue(Py::Float arg)
{
    getParameterRefPtr()->throwNull();
    this->getParameterRefPtr()->savedValue() = arg.as_double();
}

Py::Float ParameterRefPy::getOwnValue(void) const
{
    getParameterRefPtr()->throwNull();
    return Py::Float(this->getParameterRefPtr()->ownSavedValue());
}

void  ParameterRefPy::setOwnValue(Py::Float arg)
{
    getParameterRefPtr()->throwNull();
    this->getParameterRefPtr()->ownSavedValue() = arg.as_double();
}

Py::Float ParameterRefPy::getMasterScale(void) const
{
    getParameterRefPtr()->throwNull();
    return Py::Float(this->getParameterRefPtr()->masterScale());
}

void  ParameterRefPy::setMasterScale(Py::Float arg)
{
    getParameterRefPtr()->throwNull();
    this->getParameterRefPtr()->masterScale() = arg.as_double();
}

Py::Float ParameterRefPy::getOwnScale(void) const
{
    getParameterRefPtr()->throwNull();
    return Py::Float(this->getParameterRefPtr()->ownScale());
}

void  ParameterRefPy::setOwnScale(Py::Float arg)
{
    getParameterRefPtr()->throwNull();
    this->getParameterRefPtr()->ownScale() = arg.as_double();
}

Py::Boolean ParameterRefPy::getOwnFixed(void) const
{
    getParameterRefPtr()->throwNull();
    return Py::Boolean(this->getParameterRefPtr()->param().fixed);
}

void  ParameterRefPy::setOwnFixed(Py::Boolean arg)
{
    getParameterRefPtr()->throwNull();
    this->getParameterRefPtr()->param().fixed = arg.as_bool();
}

Py::Long ParameterRefPy::getTag(void) const
{
    getParameterRefPtr()->throwNull();
    return Py::Long(this->getParameterRefPtr()->param().tag);
}

void  ParameterRefPy::setTag(Py::Long arg)
{
    getParameterRefPtr()->throwNull();
    this->getParameterRefPtr()->param().tag = arg.as_long();
}

Py::String ParameterRefPy::getLabel(void) const
{
    getParameterRefPtr()->throwNull();
    return Py::String(this->getParameterRefPtr()->param().label, "utf-8");
}

void  ParameterRefPy::setLabel(Py::String arg)
{
    getParameterRefPtr()->throwNull();
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
