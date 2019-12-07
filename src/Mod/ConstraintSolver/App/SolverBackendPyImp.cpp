#include "PreCompiled.h"

#include "SolverBackendPy.h"
#include "SolverBackendPy.cpp"

#include "ValueSetPy.h"
#include "SubSystemPy.h"

PyObject *SolverBackendPy::PyMake(struct _typeobject *, PyObject* args, PyObject *)  // Python wrapper
{
    const char* tn;
    if (! PyArg_ParseTuple(args, "s", &tn))
        return nullptr;
    Base::Type t = Base::Type::fromName(tn);
    if (t.isBad()){
        PyErr_SetString(PyExc_ValueError, "Type name not found");
        return nullptr;
    }

    if (! t.isDerivedFrom(SolverBackend::getClassTypeId())){
        std::stringstream ss;
        ss << "Type " << tn << " is not derived from FCS::SolverBackend";
        PyErr_SetString(PyExc_TypeError, ss.str().c_str());
        return nullptr;
    }
    SolverBackend* pcSB = static_cast<SolverBackend*>(t.createInstance());
    if (!pcSB){
        PyErr_SetString(PyExc_TypeError,"Couldn't create instance");
        return nullptr;
    }

    return pcSB->getPyObject();
}

// constructor method
int SolverBackendPy::PyInit(PyObject* /*args*/, PyObject* /*kwd*/)
{
    return 0;
}

int SolverBackendPy::initialization()
{
    getSolverBackendPtr()->_twin = this;
    return 0;
}
int SolverBackendPy::finalization()
{
    return 0;
}

// returns a string which represents the object e.g. when printed in python
std::string SolverBackendPy::representation(void) const
{
    std::stringstream ss;
    ss << "<" << getSolverBackendPtr()->getTypeId().getName() << " object>";
    return ss.str();
}

PyObject* SolverBackendPy::solve(PyObject *args)
{
    PyObject* pysys;
    PyObject* pyvals;
    if (!PyArg_ParseTuple(args, "O!O!", &SubSystemPy::Type, &pysys,
                          &ValueSetPy::Type, &pyvals))
        return nullptr;

    auto ret = getSolverBackendPtr()->solve(
        HSubSystem(pysys, false),
        HValueSet(pyvals, false)
    );
    const char* msg[] = {"Success", "Minimized", "Fail"};
    return Py::new_reference_to(Py::String(msg[int(ret)]));
}

PyObject* SolverBackendPy::solvePair(PyObject *args)
{
    PyObject* pymainsys;
    PyObject* pyauxsys;
    PyObject* pyvals;
    if (!PyArg_ParseTuple(args, "O!O!O!", &SubSystemPy::Type, &pymainsys,
                        &SubSystemPy::Type, &pyauxsys,
                        &ValueSetPy::Type, &pyvals))
        return nullptr;

    auto ret = getSolverBackendPtr()->solvePair(
        HSubSystem(pymainsys, false),
        HSubSystem(pyauxsys, false),
        HValueSet(pyvals, false)
    );
    const char* msg[] = {"Success", "Minimized", "Fail"};
    return Py::new_reference_to(Py::String(msg[int(ret)]));
}



Py::Dict SolverBackendPy::getPrefs(void) const
{
    return getSolverBackendPtr()->prefs().getPyValue();
}

void  SolverBackendPy::setPrefs(Py::Dict arg)
{
    getSolverBackendPtr()->prefs().setPyValue(arg);
}

PyObject *SolverBackendPy::getCustomAttributes(const char* /*attr*/) const
{
    return 0;
}

int SolverBackendPy::setCustomAttributes(const char* /*attr*/, PyObject* /*obj*/)
{
    return 0;
}
