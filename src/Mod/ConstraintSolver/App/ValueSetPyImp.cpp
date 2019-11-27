#include "PreCompiled.h"

#include "ParameterStore.h"
#include "ParameterStorePy.h"
#include "ParameterSubset.h"
#include "ParameterSubsetPy.h"
#include "ParameterRef.h"
#include "ParameterRefPy.h"
#include "PyUtils.h"

#include "ValueSetPy.h"
#include "ValueSetPy.cpp"

using DualNumber = Base::DualNumber;

PyObject* ValueSetPy::PyMake(struct _typeobject*, PyObject* args, PyObject* /*??*/)  // Python wrapper
{
    PyObject* store;
    if (PyArg_ParseTuple(args, "O!",&(ParameterStorePy::Type), &store)){
        HParameterStore hstore (store, false);
        return Py::new_reference_to(ValueSet::makeTrivial(hstore));
    }
    PyErr_Clear();

    PyObject* subset;
    if (PyArg_ParseTuple(args, "O!",&(ParameterSubsetPy::Type), &subset)){
        HParameterSubset hsubset (subset, false);
        return Py::new_reference_to(ValueSet::make(hsubset));
    }

    PyErr_SetString(PyExc_TypeError,
        "Wrong arguments.\n\n"
        "ValueSet constructor requires one argument, either ParameterStore, or ParameterSubset."
    );
    return nullptr;
}

// constructor method
int ValueSetPy::PyInit(PyObject* /*args*/, PyObject* /*kwd*/)
{
    return 0;
}


// returns a string which represents the object e.g. when printed in python
std::string ValueSetPy::representation(void) const
{
    return std::string("<ValueSet object>");
}

PyObject* ValueSetPy::copy(PyObject* args)
{
    if (!PyArg_ParseTuple(args, ""))
        return nullptr;
    return Py::new_reference_to(getValueSetPtr()->copy());
}

PyObject* ValueSetPy::reset(PyObject* args)
{
    if (!PyArg_ParseTuple(args, ""))
        return nullptr;
    getValueSetPtr()->reset();
    return Py::new_reference_to(Py::None());
}

PyObject* ValueSetPy::apply(PyObject* args)
{
    if (!PyArg_ParseTuple(args, ""))
        return nullptr;
    getValueSetPtr()->apply();
    return Py::new_reference_to(Py::None());
}

inline Eigen::VectorXd eiVecFromList(PyObject* list){
    Py::Sequence seq(list);
    int n = seq.length();
    Eigen::VectorXd vals(n);
    int i = 0;
    for (Py::Object it : seq){
        vals[i] = Py::Float(it).as_double();
        ++i;
    }
    return vals;
}

PyObject* ValueSetPy::setUpForDerivative(PyObject* args)
{
    PyObject* param;
    PyObject* list;
    if (PyArg_ParseTuple(args, "O!",&(ParameterRefPy::Type), &param)){
        bool ret = getValueSetPtr()->setForDerivative(*HParameterRef(param, false));
        return Py::new_reference_to(Py::Boolean(ret));
    }
    if (PyArg_ParseTuple(args, "O", &list)){
        try {
            getValueSetPtr()->setForDerivative(eiVecFromList(list));
            return Py::new_reference_to(Py::True());
        } catch (Py::Exception&) {
            //exception string is already set
            return nullptr;
        }
    }

    PyErr_SetString(PyExc_TypeError, "Wrong number of arguments. Method add expects 1 argument, a ParameterRef or a list of doubles.");
    return nullptr;
}


Py_ssize_t ValueSetPy::sequence_length(PyObject* self)
{
    HValueSet myself(self, /*new_reference = */false);
    return myself->size();
}

PyObject*  ValueSetPy::sequence_item(PyObject* self, Py_ssize_t index)
{
    HValueSet myself(self, /*new_reference = */false);
    if(index < 0 || index >= myself->size()){
        PyErr_SetString(PyExc_IndexError, "Index out of range");
        return nullptr;
    }
    DualNumber v = (*myself)[int(index)];
    return Py::new_reference_to(pyDualNumber(v));
}

PyObject*  ValueSetPy::mapping_subscript(PyObject* self, PyObject* key)
{
    HValueSet myself(self, /*new_reference = */false);

    if (PyIndex_Check(key)) {
        Py_ssize_t i = PyNumber_AsSsize_t(key, PyExc_IndexError);
        if (i == -1 && PyErr_Occurred())
            return nullptr;
        if (i < 0)
            i += sequence_length(self);
        return sequence_item(self, i);
    } else if (PySlice_Check(key)) {
        PyErr_SetString(PyExc_NotImplementedError, "Slicing not supported");
        return nullptr;
    } else if (PyObject_TypeCheck(key, &ParameterRefPy::Type)) {
        HParameterRef hp(key, false);
        DualNumber v = myself->get(*hp);
        return Py::new_reference_to(pyDualNumber(v));
    }
    PyErr_Format(PyExc_NotImplementedError, "Indexing can be by numbers and by parameter objects. Got %s", Py_TYPE(key)->tp_name);
    return nullptr;
}

int ValueSetPy::sequence_ass_item(PyObject* , Py_ssize_t, PyObject* )
{
    PyErr_SetString(PyExc_NotImplementedError, "Not yet implemented");
    return -1;
}

int ValueSetPy::sequence_contains(PyObject* self, PyObject* pcItem)
{
    HValueSet myself(self, false);
    Py::Object it(pcItem);
    if(!(PyObject_TypeCheck(pcItem, &ParameterRefPy::Type)))
        return false;
    HParameterRef param(pcItem, false);
    return myself->subset().has(*param);
}


Py::Object ValueSetPy::getHost(void) const
{
    return getValueSetPtr()->subset().host();
}

Py::Object ValueSetPy::getSubset(void) const
{
    return getValueSetPtr()->subset().self();
}

Py::List ValueSetPy::getValues(void) const
{
    return asPyList<Eigen::VectorXd, Py::Float>(getValueSetPtr()->values());
}

inline void throwSizeMismatch(int s1, int s2) {
    if (s1 != s2){
        std::stringstream ss;
        ss << "Length of the list (" << s1 << ") "
           << "doesn't match the number of parameters in subset ("
           << s2 <<")";
        throw Py::ValueError(ss.str());
    }
}

void  ValueSetPy::setValues(Py::List arg)
{
    Eigen::VectorXd vec = eiVecFromList(arg.ptr());
    throwSizeMismatch(vec.size(), getValueSetPtr()->size());
    getValueSetPtr()->values() = vec;
}

Py::List ValueSetPy::getDuals(void) const
{
    return asPyList<std::vector<double>, Py::Float>(getValueSetPtr()->duals());
}

void  ValueSetPy::setDuals(Py::List arg)
{
    Eigen::VectorXd vec = eiVecFromList(arg.ptr());
    throwSizeMismatch(vec.size(), getValueSetPtr()->size());
    getValueSetPtr()->setForDerivative(vec);
}

Py::List ValueSetPy::getSavedValues(void) const
{
    auto vec = getValueSetPtr()->savedValues();
    return asPyList<Eigen::VectorXd, Py::Float>(vec);
}

PyObject* ValueSetPy::getCustomAttributes(const char* /*attr*/) const
{
    return nullptr;
}

int ValueSetPy::setCustomAttributes(const char* /*attr*/, PyObject* /*obj*/)
{
    return 0;
}
