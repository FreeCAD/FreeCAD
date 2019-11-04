#include "PreCompiled.h"

#include "ParameterStore.h"

#include "ParameterStorePy.h"
#include "ParameterStorePy.cpp"

PyObject *ParameterStorePy::PyMake(struct _typeobject *, PyObject *, PyObject *)  // Python wrapper
{
    // create a new instance of ParameterStorePy and the Twin object
    return Py::new_reference_to(ParameterStore::make());
}

// constructor method
int ParameterStorePy::PyInit(PyObject* /*args*/, PyObject* /*kwd*/)
{
    return 0;
}


// returns a string which represents the object e.g. when printed in python
std::string ParameterStorePy::representation(void) const
{
    return std::string("<ParameterStore object>");
}

PyObject* ParameterStorePy::addOne(PyObject* args, PyObject* kwd)
{
    //label = '', value = 0.0, scale = 1.0, fixed = False, tag = 0
    const char* kwlist[] = {"label", "value", "scale", "fixed", "tag", nullptr};
    const char* label = "";
    double value = 0.0, scale = 1.0;
    PyObject* fixed = Py_False;
    int tag = 0;
    bool parsed =
    PyArg_ParseTupleAndKeywords(args, kwd, "|sddO!i", const_cast<char**>(kwlist),
        &label, &value, &scale, &fixed, &PyBool_Type, &tag
    );
    if (!parsed)
        return nullptr;
    ParameterRef ref = getParameterStorePtr()->add(Parameter(label, value, scale, fixed == Py_True, tag));
    Py::Object r = ref.getPyObject();
    Base::Console().Warning("ref cnt: %i", int( r.reference_count()));
    return Py::new_reference_to(ref.getPyObject());
}

PyObject* ParameterStorePy::addN(PyObject *args)
{
    int n;
    if(PyArg_ParseTuple(args, "i", &n)){
        std::vector<ParameterRef> refs = this->getParameterStorePtr()->add(n);
        Py::List ret(refs.size());
        for(int i = 0; i < refs.size(); ++i){
            ret[i] = refs[i].getPyObject();
        }
        return Py::new_reference_to(ret);
    }

    PyObject* arg = nullptr;
    if(PyArg_ParseTuple(args, "O", arg)){
        Py::List ret;
        try{
            Py::Sequence seq(arg);
            for(Py::Object it : seq){
                if(it.isTuple()){//typecheck, throws if not tuple
                    Py::Dict d;
                    ret.append(Py::Object(this->addOne(it.ptr(), d.ptr())));
                } else if (it.isDict()) {
                    Py::Tuple t;
                    ret.append(Py::Object(this->addOne(t.ptr(), it.ptr())));
                } else {
                    std::stringstream ss;
                    ss << "addN: elements of list can be tuples or dicts, got "
                       << it.type().repr().as_string();
                    throw Py::Exception(PyExc_TypeError, ss.str());
                }
            }
        } catch (Py::Exception) {
            return nullptr; //error message whould already be set by PyCXX
        }
    };
    PyErr_SetString(PyExc_TypeError, "Wrong argument type. Support: int, or list of tuples, or list of dicts");
    return nullptr;
}

PyObject* ParameterStorePy::copy(PyObject* args)
{
    PyErr_SetString(PyExc_NotImplementedError, "Not yet implemented");
    return 0;
}



Py::List ParameterStorePy::getValues(void) const
{
    int sz = getParameterStorePtr()->size();
    Py::List ret(sz);
    for(int i = 0; i < sz; ++i){
        ret[i] = Py::Float(getParameterStorePtr()->value(i));
    }
    return ret;
}

void  ParameterStorePy::setValues(Py::List arg)
{
    int sz = arg.size();
    if(sz != getParameterStorePtr()->size()){
        std::stringstream ss;
        ss << "Length of list (" << sz
           << ") doesn't match the number of parameters ("
           << getParameterStorePtr()->size() << ").";
        throw Py::ValueError(ss.str());
    }
    for(int i = 0; i < sz; ++i){
        getParameterStorePtr()->value(i) = Py::Float(arg[i]).as_double();
    }
}

Py::List ParameterStorePy::getScales(void) const
{
    //return Py::List();
    throw Py::AttributeError("Not yet implemented");
}

void  ParameterStorePy::setScales(Py::List arg)
{
    throw Py::AttributeError("Not yet implemented");
}


Py_ssize_t ParameterStorePy::sequence_length(PyObject* self)
{
    HParameterStore myself(self, /*new_reference = */false);
    return myself->size();
}

PyObject* ParameterStorePy::sequence_item(PyObject* self, Py_ssize_t index)
{
    HParameterStore myself(self, /*new_reference = */false);
    if(index < 0 || index >= myself->size()){
        PyErr_SetString(PyExc_IndexError, "Index out of range");
        return nullptr;
    }
    return Py::new_reference_to((*myself)[int(index)].getPyObject());
}

PyObject* ParameterStorePy::mapping_subscript(PyObject* self, PyObject* item)
{
    HParameterStore myself(self, /*new_reference = */false);
    if (PyIndex_Check(item)) {
        Py_ssize_t i = PyNumber_AsSsize_t(item, PyExc_IndexError);
        if (i == -1 && PyErr_Occurred())
            return NULL;
        if (i < 0)
            i += sequence_length(self);
        return sequence_item(self, i);
    }
    else if (PySlice_Check(item)) {
        Py_ssize_t start, stop, step, slicelength, cur, i;
#if PY_MAJOR_VERSION < 3
        PySliceObject* slice = reinterpret_cast<PySliceObject*>(item);
#else
        PyObject* slice = item;
#endif

        if (PySlice_GetIndicesEx(slice,
                         sequence_length(self),
                         &start, &stop, &step, &slicelength) < 0) {
            return nullptr;
        }

        if (slicelength <= 0) {
            return PyList_New(0);
        } else {
            Py::List ret(slicelength);

            for (cur = start, i = 0; i < slicelength;
                 cur += step, i++) {
                ret.setItem(i, (*myself)[cur].getPyObject());
            }

            return Py::new_reference_to(ret);
        }
    }

    PyErr_Format(PyExc_TypeError,
                 "Indices must be integers or slices, not %.200s",
                 Py_TYPE(item)->tp_name);
    return nullptr;
}

int ParameterStorePy::sequence_ass_item(PyObject *, Py_ssize_t, PyObject *)
{
    PyErr_SetString(PyExc_NotImplementedError, "Not yet implemented");
    return -1;
}

int ParameterStorePy::sequence_contains(PyObject *, PyObject *)
{
    PyErr_SetString(PyExc_NotImplementedError, "Not yet implemented");
    return -1;
}


PyObject *ParameterStorePy::getCustomAttributes(const char* /*attr*/) const
{
    return 0;
}

int ParameterStorePy::setCustomAttributes(const char* /*attr*/, PyObject* /*obj*/)
{
    return 0;
}
