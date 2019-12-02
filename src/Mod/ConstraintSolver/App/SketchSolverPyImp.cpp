#include "PreCompiled.h"

#include "SketchSolverPy.h"
#include "SketchSolverPy.cpp"

#include "SubSystemPy.h"
#include "ValueSetPy.h"

#include "PyUtils.h"


PyObject *SketchSolverPy::PyMake(struct _typeobject *, PyObject *, PyObject *)  // Python wrapper
{
    // create a new instance of SketchSolverPy and the Twin object
    return Py::new_reference_to((new SketchSolver)->self());
}

// constructor method
int SketchSolverPy::PyInit(PyObject* /*args*/, PyObject* /*kwd*/)
{
    return 0;
}

int SketchSolverPy::initialization()
{
    getSketchSolverPtr()->_twin = this;
    return 0;
}
int SketchSolverPy::finalization()
{
    return 0;
}

// returns a string which represents the object e.g. when printed in python
std::string SketchSolverPy::representation(void) const
{
    return std::string("<SketchSolver object>");
}

PyObject* SketchSolverPy::solveDogLeg(PyObject *args)
{
    PyObject* pysys;
    PyObject* pyvals;
    PyObject* pyprefs = nullptr;
    if (!PyArg_ParseTuple(args, "O!O!|O!", &SubSystemPy::Type, &pysys,
                          &ValueSetPy::Type, &pyvals, &PyDict_Type, &pyprefs))
        return nullptr;

    SketchSolver::DogLegPrefs prefs;
    if (pyprefs)
        throw Py::NotImplementedError("preferences support not implemented"); //#fixme: implement
    auto ret = getSketchSolverPtr()->solveDogLeg(
        HSubSystem(pysys, false),
        HValueSet(pyvals, false),
        prefs
    );
    const char* msg[] = {"Success", "Minimized", "Fail"};
    return Py::new_reference_to(Py::String(msg[int(ret)]));
}


PyObject* SketchSolverPy::solveLM(PyObject *args)
{
    PyObject* pysys;
    PyObject* pyvals;
    PyObject* pyprefs = nullptr;
    if (!PyArg_ParseTuple(args, "O!O!|O!", &SubSystemPy::Type, &pysys,
                          &ValueSetPy::Type, &pyvals, &PyDict_Type, &pyprefs))
        return nullptr;

    SketchSolver::LMPrefs prefs;
    if (pyprefs)
        throw Py::NotImplementedError("preferences support not implemented"); //#fixme: implement
    auto ret = getSketchSolverPtr()->solveLM(
        HSubSystem(pysys, false),
        HValueSet(pyvals, false),
        prefs
    );
    const char* msg[] = {"Success", "Minimized", "Fail"};
    return Py::new_reference_to(Py::String(msg[int(ret)]));
}

PyObject* SketchSolverPy::solveBFGS(PyObject *args)
{
    PyObject* pysys;
    PyObject* pyvals;
    PyObject* pyprefs = nullptr;
    if (!PyArg_ParseTuple(args, "O!O!|O!", &SubSystemPy::Type, &pysys,
                          &ValueSetPy::Type, &pyvals, &PyDict_Type, &pyprefs))
        return nullptr;

    SketchSolver::BFGSPrefs prefs;
    if (pyprefs)
        throw Py::NotImplementedError("preferences support not implemented"); //#fixme: implement
    auto ret = getSketchSolverPtr()->solveBFGS(
        HSubSystem(pysys, false),
        HValueSet(pyvals, false),
        prefs
    );
    const char* msg[] = {"Success", "Minimized", "Fail"};
    return Py::new_reference_to(Py::String(msg[int(ret)]));
}

PyObject* SketchSolverPy::solveSQP(PyObject *args)
{
    PyObject* pymainsys;
    PyObject* pyauxsys;
    PyObject* pyvals;
    PyObject* pyprefs = nullptr;
    if (!PyArg_ParseTuple(args, "O!O!O!|O!",
                          &SubSystemPy::Type, &pymainsys,
                          &SubSystemPy::Type, &pyauxsys,
                          &ValueSetPy::Type, &pyvals, &PyDict_Type, &pyprefs))
        return nullptr;

    SketchSolver::SQPPrefs prefs;
    if (pyprefs)
        throw Py::NotImplementedError("preferences support not implemented"); //#fixme: implement
    auto ret = getSketchSolverPtr()->solveSQP(
        HSubSystem(pymainsys, false),
        HSubSystem(pyauxsys, false),
        HValueSet(pyvals, false),
        prefs
    );
    const char* msg[] = {"Success", "Minimized", "Fail"};
    return Py::new_reference_to(Py::String(msg[int(ret)]));
}

PyObject *SketchSolverPy::getCustomAttributes(const char* /*attr*/) const
{
    return 0;
}

int SketchSolverPy::setCustomAttributes(const char* /*attr*/, PyObject* /*obj*/)
{
    return 0;
}
