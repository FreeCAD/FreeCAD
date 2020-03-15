#include "PreCompiled.h"

#include "ParameterStore.h"
#include "ParameterStorePy.h"
#include "ParameterRefPy.h"

#include "ParameterSubsetPy.h"
#include "ParameterSubsetPy.cpp"

#include "PyUtils.h"

PyObject* ParameterSubsetPy::PyMake(struct _typeobject*, PyObject* args, PyObject* /*??*/)  // Python wrapper
{
    try{
    {//(parameter_store)
        PyObject* store;
        if (PyArg_ParseTuple(args, "O!",&(ParameterStorePy::Type), &store)){
            HParameterStore hstore (store, false);
            return Py::new_reference_to(ParameterSubset::make(hstore).getHandledObject());
        }
        PyErr_Clear();
    }
    {//(list_of_params)
        PyObject* pylist;
        if (PyArg_ParseTuple(args, "O!",&(PyList_Type), &pylist)){
            std::vector<ParameterRef> params;
            for(Py::Object it : Py::List(pylist)){
                ParameterRef par = * pyTypeCheck<ParameterRefPy>(it.ptr())->getParameterRefPtr();
                params.push_back(par);
            };
            return Py::new_reference_to(ParameterSubset::make(params).getHandledObject());
        }
        PyErr_Clear();
    }
    } catch (Py::Exception&) {
        return nullptr;
    } catch (Base::Exception& err) {
        return raiseBaseException(err);
    }
    PyErr_SetString(PyExc_TypeError, "Unsupported signature. Expected: one argument of type list(ParameterRef), or ParameterStore.");
    return nullptr;
}

// constructor method
int ParameterSubsetPy::PyInit(PyObject* /*args*/, PyObject* /*kwd*/)
{
    return 0;
}


// returns a string which represents the object e.g. when printed in python
std::string ParameterSubsetPy::representation(void) const
{
    return std::string("<ParameterSubset object>");
}


PyObject* ParameterSubsetPy::copy(PyObject* args)
{
    if (!PyArg_ParseTuple(args, ""))
        return nullptr;
    return Py::new_reference_to(getParameterSubsetPtr()->copy().getHandledObject());
}

PyObject* ParameterSubsetPy::add(PyObject* args)
{
    PyObject* param;
    PyObject* list;
    if (PyArg_ParseTuple(args, "O!",&(ParameterRefPy::Type), &param)){
        int ret = getParameterSubsetPtr()->add(*HParameterRef(param, false));
        return Py::new_reference_to(Py::Long(ret));
    }
    PyErr_Clear();
    if (PyArg_ParseTuple(args, "O", &list)){
        try {
            Py::Sequence seq(list);
            std::vector<ParameterRef> toAdd;
            for (Py::Object o : seq){
                if (!PyObject_TypeCheck(o.ptr(), &(ParameterRefPy::Type))){
                    std::stringstream ss;
                    ss << "Expected ParameterRef, got " << o.type().as_string();
                    throw Py::TypeError(ss.str());
                }
                toAdd.push_back(*HParameterRef(o));
            }
            return Py::new_reference_to(Py::Long(getParameterSubsetPtr()->add(toAdd)));
        } catch (Py::Exception&) {
            //exception string is already set
            return nullptr;
        }
    }

    PyErr_SetString(PyExc_TypeError, "Wrong number of arguments. Method add expects 1 argument, a ParameterRef or a list of ParameterRef's.");
    return nullptr;
}

PyObject* ParameterSubsetPy::remove(PyObject* args)
{
    PyObject* param;
    if (!PyArg_ParseTuple(args, "O!",&(ParameterRefPy::Type), &param))
        return nullptr;
    int ret = getParameterSubsetPtr()->remove(*HParameterRef(param, false));
    return Py::new_reference_to(Py::Long(ret));
}

PyObject* ParameterSubsetPy::clear(PyObject* args)
{
    if (!PyArg_ParseTuple(args, ""))
        return nullptr;
    getParameterSubsetPtr()->clear();
    return Py::new_reference_to(Py::None());
}

PyObject* ParameterSubsetPy::has(PyObject* args)
{
    PyObject* param;
    if (!PyArg_ParseTuple(args, "O!",&(ParameterRefPy::Type), &param))
        return nullptr;
    bool ret = getParameterSubsetPtr()->has(*HParameterRef(param, false));
    return Py::new_reference_to(Py::Boolean(ret));
}

PyObject* ParameterSubsetPy::indexOf(PyObject* args)
{
    PyObject* param;
    if (!PyArg_ParseTuple(args, "O!",&(ParameterRefPy::Type), &param))
        return nullptr;
    int ret = -1;
    ParameterRef p = *HParameterRef(param, false);
    if (getParameterSubsetPtr()->has(p))
        ret = getParameterSubsetPtr()->indexOf(p);
    return Py::new_reference_to(Py::Long(ret));
}


Py_ssize_t ParameterSubsetPy::sequence_length(PyObject* self)
{
    HParameterSubset myself(self, /*new_reference = */false);
    return myself->size();
}

PyObject*  ParameterSubsetPy::sequence_item(PyObject* self, Py_ssize_t index)
{
    HParameterSubset myself(self, /*new_reference = */false);
    if(index < 0 || index >= myself->size()){
        PyErr_SetString(PyExc_IndexError, "Index out of range");
        return nullptr;
    }
    return Py::new_reference_to((*myself)[int(index)].getPyHandle().getHandledObject());
}

PyObject*  ParameterSubsetPy::mapping_subscript(PyObject* , PyObject* )
{
    PyErr_SetString(PyExc_NotImplementedError, "Not yet implemented, use Parameters attribute");
    return 0;
}

int ParameterSubsetPy::sequence_ass_item(PyObject* , Py_ssize_t, PyObject* )
{
    PyErr_SetString(PyExc_NotImplementedError, "Not yet implemented, use add/remove methods");
    return -1;
}

int ParameterSubsetPy::sequence_contains(PyObject* self, PyObject* pcItem)
{
    HParameterSubset myself(self, false);
    Py::Object it(pcItem);
    if(!(PyObject_TypeCheck(pcItem, &ParameterRefPy::Type)))
        return false;
    HParameterRef param(pcItem, false);
    return myself->has(*param);
}


Py::Object ParameterSubsetPy::getHost(void) const
{
    return getParameterSubsetPtr()->host().getHandledObject();
}

Py::List ParameterSubsetPy::getParameters(void) const
{
    int sz = getParameterSubsetPtr()->size();
    Py::List ret(sz);
    for (int i = 0; i < sz; ++i){
        ret[i] = (*getParameterSubsetPtr())[i].getPyHandle().getHandledObject();
    }
    return ret;
}



PyObject* ParameterSubsetPy::getCustomAttributes(const char* /*attr*/) const
{
    return 0;
}

int ParameterSubsetPy::setCustomAttributes(const char* /*attr*/, PyObject* /*obj*/)
{
    return 0;
}
