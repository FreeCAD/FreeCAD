#include "PreCompiled.h"

#include "ConstraintPy.h"
#include "ConstraintPy.cpp"

#include "ValueSetPy.h"
#include "PyUtils.h"

using namespace FCS;
using DualNumber = Base::DualNumber;


int ConstraintPy::initialization()
{
    getConstraintPtr()->_twin = this;
    return 0;
}
int ConstraintPy::finalization()
{
    return 0;
}

// returns a string which represents the object e.g. when printed in python
std::string ConstraintPy::representation(void) const
{
    std::stringstream ss;
    ss << "<" << getConstraintPtr()->getTypeId().getName() << " object>";
    return ss.str();
}

PyObject* ConstraintPy::netError(PyObject* args)
{
    PyObject* vals = Py_None;
    if (!PyArg_ParseTuple(args, "O!", &(ValueSetPy::Type), &vals))
        return nullptr;

    HValueSet hvals (vals, false);
    DualNumber ret = getConstraintPtr()->netError(*hvals);
    return Py::new_reference_to(pyDualNumber(ret));

}

PyObject* ConstraintPy::errorVec(PyObject* args)
{
    PyObject* pyvals = Py_None;
    if (!PyArg_ParseTuple(args, "O!", &(ValueSetPy::Type), &pyvals))
        return nullptr;
    HValueSet vals(pyvals, false);
    std::vector<DualNumber> errvec(getConstraintPtr()->rank());
    getConstraintPtr()->error(*vals, errvec.data());

    Py::List ret(errvec.size());
    for(int i = 0; i < errvec.size(); ++i){
        ret[i] = pyDualNumber(errvec[i]);
    }
    return Py::new_reference_to(ret);
}

PyObject* ConstraintPy::copy(PyObject *args)
{
    if (!PyArg_ParseTuple(args, ""))
        return nullptr;
    return Py::new_reference_to(getConstraintPtr()->copy());
}

PyObject* ConstraintPy::update(PyObject* args)
{
    if (!PyArg_ParseTuple(args, ""))
        return nullptr;
    getConstraintPtr()->update();
    return Py::new_reference_to(Py::None());
}



Py::List ConstraintPy::getParameters(void) const
{
    return asPyList(
        getConstraintPtr()->parameters()
    );
}

Py::Float ConstraintPy::getWeight(void) const
{
    return Py::Float(getConstraintPtr()->weight());
}

void  ConstraintPy::setWeight(Py::Float arg)
{
    getConstraintPtr()->setWeight(arg.as_double());
}

Py::Float ConstraintPy::getNetError(void) const
{
    return Py::Float(getConstraintPtr()->netError());
}

Py::Long ConstraintPy::getRank(void) const
{
    return Py::Long(getConstraintPtr()->rank());
}

Py::Object ConstraintPy::getUserData(void) const
{
    return getConstraintPtr()->userData;
    throw Py::AttributeError("Not yet implemented");
}

void  ConstraintPy::setUserData(Py::Object arg)
{
    getConstraintPtr()->userData = arg;
}

Py::String ConstraintPy::getLabel(void) const
{
    return Py::String(getConstraintPtr()->label,"utf-8");
}

void  ConstraintPy::setLabel(Py::String arg)
{
    getConstraintPtr()->label = arg.as_std_string("utf-8");
}

Py::Long ConstraintPy::getTag(void) const
{
    return Py::Long(getConstraintPtr()->tag);
}

void  ConstraintPy::setTag(Py::Long arg)
{
    getConstraintPtr()->tag = arg.as_long();
}

PyObject* ConstraintPy::getCustomAttributes(const char* /*attr*/) const
{
    return 0;
}

int ConstraintPy::setCustomAttributes(const char* /*attr*/, PyObject* /*obj*/)
{
    return 0;
}
