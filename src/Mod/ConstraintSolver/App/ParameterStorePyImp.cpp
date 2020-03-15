#include "PreCompiled.h"

#include "ParameterStore.h"
#include "ParameterRefPy.h"

#include "ParameterStorePy.h"
#include "ParameterStorePy.cpp"

#include "ValueSet.h"

#include "PyUtils.h"

PyObject* ParameterStorePy::PyMake(struct _typeobject *, PyObject* , PyObject* )  // Python wrapper
{
    // create a new instance of ParameterStorePy and the Twin object
    return Py::new_reference_to(ParameterStore::make().getHandledObject());
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
        &label, &value, &scale, &PyBool_Type, &fixed, &tag
    );
    if (!parsed)
        return nullptr;
    ParameterRef ref = getParameterStorePtr()->add(Parameter(label, value, scale, fixed == Py_True, tag));
    return Py::new_reference_to(ref.getPyHandle().getHandledObject());
}

PyObject* ParameterStorePy::addN(PyObject* args)
{
    int n;
    if(PyArg_ParseTuple(args, "i", &n)){
        std::vector<ParameterRef> refs = this->getParameterStorePtr()->add(n);
        return Py::new_reference_to(asPyList(refs));
    }
    PyErr_Clear();

    PyObject* arg = nullptr;
    if(PyArg_ParseTuple(args, "O", &arg)){
        Py::List ret;
        try{
            Py::Sequence seq(arg);
            for(Py::Object it : seq){
                if(it.isTuple()){//typecheck, throws if not tuple
                    Py::Dict d;
                    ret.append(Py::Object(this->addOne(it.ptr(), d.ptr()), true));
                } else if (it.isDict()) {
                    Py::Tuple t;
                    ret.append(Py::Object(this->addOne(t.ptr(), it.ptr()), true));
                } else {
                    std::stringstream ss;
                    ss << "addN: elements of list can be tuples or dicts, got "
                       << it.type().repr().as_string();
                    throw Py::Exception(PyExc_TypeError, ss.str());
                }
            }
            return Py::new_reference_to(ret);
        } catch (Py::Exception&) {
            return nullptr; //error message whould already be set by PyCXX
        }
    };
    PyErr_SetString(PyExc_TypeError, "Wrong argument type. Support: int, or list of tuples, or list of dicts");
    return nullptr;
}

PyObject* ParameterStorePy::copy(PyObject* args)
{
    if (! PyArg_ParseTuple(args, ""))
        return nullptr;
    return Py::new_reference_to(getParameterStorePtr()->copy().getHandledObject());
}

PyObject* ParameterStorePy::asValueSet(PyObject* args)
{
    if (! PyArg_ParseTuple(args, ""))
        return nullptr;
    return Py::new_reference_to(getParameterStorePtr()->asValueSet().self().getHandledObject());
}

PyObject* ParameterStorePy::constrainEqual(PyObject* args)
{
    PyObject* param1 = nullptr;
    PyObject* param2 = nullptr;
    PyObject* mean_out = Py_True;
    if (! PyArg_ParseTuple(args, "O!O!|O!", &ParameterRefPy::Type, &param1, &ParameterRefPy::Type, &param2, &PyBool_Type, &mean_out))
        return nullptr;
    auto emsg = getParameterStorePtr()->constrainEqual(
        *(static_cast<ParameterRefPy*>(param1)->getParameterRefPtr()),
        *(static_cast<ParameterRefPy*>(param2)->getParameterRefPtr()),
        Py::Boolean(mean_out).isTrue()
    );
    std::string ret;
    switch(emsg){
        case ParameterStore::eConstrainEqual_Result::Redundant:
            ret = "Redundant";
        break;
        case ParameterStore::eConstrainEqual_Result::Constrained:
            ret = "Constrained";
        break;
    }
    return Py::new_reference_to(Py::String(ret));
}

PyObject* ParameterStorePy::deconstrain(PyObject* args)
{
    PyObject* param = Py_None;
    if (! PyArg_ParseTuple(args, "|O!", &ParameterRefPy::Type, &param))
        return nullptr;
    if (param == Py_None)
        getParameterStorePtr()->deconstrain();
    else
        getParameterStorePtr()->deconstrain(*UnsafePyHandle<ParameterRef>(param, /*owned=*/false));
    return Py::new_reference_to(Py::None());
}

PyObject* ParameterStorePy::sync(PyObject* args)
{
    PyObject* param = Py_None;
    if (! PyArg_ParseTuple(args, "|O!", &ParameterRefPy::Type, &param))
        return nullptr;
    if (param == Py_None)
        getParameterStorePtr()->sync();
    else
        getParameterStorePtr()->sync(*UnsafePyHandle<ParameterRef>(param, /*owned=*/false));
    return Py::new_reference_to(Py::None());
}

PyObject* ParameterStorePy::allFree(PyObject* args)
{
    if (! PyArg_ParseTuple(args, ""))
        return nullptr;
    auto vec = getParameterStorePtr()->allFree();
    return Py::new_reference_to(asPyList(vec));
}

PyObject* ParameterStorePy::allFixed(PyObject* args)
{
    if (! PyArg_ParseTuple(args, ""))
        return nullptr;
    auto vec = getParameterStorePtr()->allFixed();
    return Py::new_reference_to(asPyList(vec));
}

PyObject* ParameterStorePy::allDriven(PyObject* args)
{
    if (! PyArg_ParseTuple(args, ""))
        return nullptr;
    auto vec = getParameterStorePtr()->allDriven();
    return Py::new_reference_to(asPyList(vec));
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
        (*getParameterStorePtr())[i].ownSavedValue() = Py::Float(arg[i]).as_double();
    }
}

Py::List ParameterStorePy::getScales(void) const
{
    //return Py::List();
    throw Py::AttributeError("Not yet implemented");
}

void  ParameterStorePy::setScales(Py::List arg)
{
    (void)arg;
    throw Py::AttributeError("Not yet implemented");
}

Py::Long ParameterStorePy::getDofCount(void) const
{
    return Py::Long(getParameterStorePtr()->dofCount());
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
    return Py::new_reference_to((*myself)[int(index)].getPyHandle().getHandledObject());
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
                ret.setItem(i, (*myself)[cur].getPyHandle().getHandledObject());
            }

            return Py::new_reference_to(ret);
        }
    }

    PyErr_Format(PyExc_TypeError,
                 "Indices must be integers or slices, not %.200s",
                 Py_TYPE(item)->tp_name);
    return nullptr;
}

int ParameterStorePy::sequence_ass_item(PyObject* , Py_ssize_t, PyObject* )
{
    PyErr_SetString(PyExc_NotImplementedError, "Not yet implemented");
    return -1;
}

int ParameterStorePy::sequence_contains(PyObject* self, PyObject* pcItem)
{
    HParameterStore myself(self, false);
    Py::Object it(pcItem);
    if(!(PyObject_TypeCheck(pcItem, &ParameterRefPy::Type)))
        return false;
    HParameterRef param(pcItem, false);
    return myself->has(*param);
}


PyObject* ParameterStorePy::getCustomAttributes(const char* /*attr*/) const
{
    return 0;
}

int ParameterStorePy::setCustomAttributes(const char* /*attr*/, PyObject* /*obj*/)
{
    return 0;
}
