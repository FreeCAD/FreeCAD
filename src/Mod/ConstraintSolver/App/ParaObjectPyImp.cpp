#include "PreCompiled.h"

#include "PyUtils.h"
#include "ParameterStore.h"
#include "ParameterStorePy.h"

#include "ParaObjectPy.h"
#include "ParaObjectPy.cpp"


int ParaObjectPy::initialization()
{
    getParaObjectPtr()->_twin = this;
    return 0;
}
int ParaObjectPy::finalization()
{
    return 0;
}

// returns a string which represents the object e.g. when printed in python
std::string ParaObjectPy::representation(void) const
{
    return getParaObjectPtr()->repr();
}

PyObject* ParaObjectPy::update(PyObject* args)
{
    if (!PyArg_ParseTuple(args, ""))
        return nullptr;
    getParaObjectPtr()->update();
    return Py::new_reference_to(Py::None());
}

PyObject* ParaObjectPy::touch(PyObject* args)
{
    if (!PyArg_ParseTuple(args, ""))
        return nullptr;
    getParaObjectPtr()->touch();
    return Py::new_reference_to(Py::None());
}

PyObject* ParaObjectPy::copy(PyObject* args)
{
    if (!PyArg_ParseTuple(args, ""))
        return nullptr;
    return Py::new_reference_to(getParaObjectPtr()->copy().getHandledObject());
}

PyObject* ParaObjectPy::makeParameters(PyObject* args)
{
    PyObject* pystore = Py_None;
    if (!PyArg_ParseTuple(args, "O!", &(ParameterStorePy::Type), &pystore))
        return nullptr;
    std::vector<ParameterRef> refs =
        getParaObjectPtr()->makeParameters(HParameterStore(pystore, false));
    return Py::new_reference_to(asPyList(refs));
}

PyObject* ParaObjectPy::makeRuleConstraints(PyObject *args)
{
    if (!PyArg_ParseTuple(args, ""))
        return nullptr;
    auto ret = getParaObjectPtr()->makeRuleConstraints();
    return Py::new_reference_to(asPyObjectList(ret));
}

PyObject* ParaObjectPy::isComplete(PyObject* args)
{
    if (!PyArg_ParseTuple(args, ""))
        return nullptr;
    return Py::new_reference_to(Py::Boolean(
        getParaObjectPtr()->isComplete()
    ));
}

PyObject* ParaObjectPy::throwIfIncomplete(PyObject* args)
{
    if (!PyArg_ParseTuple(args, ""))
        return nullptr;
    getParaObjectPtr()->throwIfIncomplete();
    return Py::new_reference_to(Py::None());
}



Py::List ParaObjectPy::getParameters(void) const
{
    return asPyList(
        getParaObjectPtr()->parameters()
    );
}

Py::List ParaObjectPy::getNamedParameters(void) const
{
    auto params = getParaObjectPtr()->_attrs;
    Py::List ret(params.size());
    for(size_t i = 0; i < params.size(); ++i){
        Py::Tuple tup(2);
        tup[0] = Py::String(params[i].name);
        tup[1] = params[i].value->getPyHandle().getHandledObject();
        ret[i] = tup;
    }
    return ret;
}

Py::List ParaObjectPy::getNamedChildren(void) const
{
    auto params = getParaObjectPtr()->_children;
    Py::List ret(params.size());
    for(size_t i = 0; i < params.size(); ++i){
        Py::Tuple tup(2);
        tup[0] = Py::String(params[i].name);
        tup[1] = (*(params[i].value)).getHandledObject();
        ret[i] = tup;
    }
    return ret;
}

Py::Boolean ParaObjectPy::getTouched(void) const
{
    return Py::Boolean(getParaObjectPtr()->isTouched());
}

Py::Long ParaObjectPy::getTag(void) const
{
    return Py::Long(getParaObjectPtr()->tag);
}

void  ParaObjectPy::setTag(Py::Long arg)
{
    getParaObjectPtr()->tag = arg.as_long();
}

Py::Object ParaObjectPy::getUserData(void) const
{
    return getParaObjectPtr()->userData;
}

void  ParaObjectPy::setUserData(Py::Object arg)
{
    getParaObjectPtr()->userData = arg;
}

Py::String ParaObjectPy::getLabel(void) const
{
    return Py::String(getParaObjectPtr()->label,"utf-8");
}

void  ParaObjectPy::setLabel(Py::String arg)
{
    getParaObjectPtr()->label = arg.as_std_string("utf-8");
}

PyObject* ParaObjectPy::getCustomAttributes(const char* attr) const
{
    try {
        return Py::new_reference_to(getParaObjectPtr()->getAttr(attr));
    } catch (Py::AttributeError&) {
        PyErr_Clear();
        return nullptr;
    }
}

int ParaObjectPy::setCustomAttributes(const char* attr, PyObject* obj)
{
    try {
        getParaObjectPtr()->setAttr(attr, Py::Object(obj, false));
        return 1;
    } catch (Py::AttributeError&) {
        PyErr_Clear();
        return 0;
    }
}
