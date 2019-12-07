#include "PreCompiled.h"

#include "SolverBackend.h"
#include "SolverBackendPy.h"

#include "PyUtils.h"

TYPESYSTEM_SOURCE_ABSTRACT(FCS::SolverBackend, Base::BaseClass);

using namespace FCS;


eSolveResult SolverBackend::solve(HSubSystem sys, HValueSet vals)
{
    //default implementation redirects the call to solvePair
    HSubSystem sysaux = (new SubSystem(sys->params()))->self();
    sysaux->update();
    return solvePair(sys,sysaux,vals);
}

eSolveResult SolverBackend::solvePair(HSubSystem mainsys, HSubSystem auxsys, HValueSet vals)
{
    //default implementation hacks its way using single solve
    HSubSystem merged = (new SubSystem(mainsys->params()))->self();
    for(const HConstraint& c : mainsys->constraints()){
        merged->addConstraint(c);
    }
    for(const HConstraint& c : auxsys->constraints()){
        merged->addConstraint(c);
    }
    merged->update();

    //we try solving merged system. If it is minimized, we then solve mainsys
    //to reduce its error as much as possible, and return.
    auto mergedresult = solve(merged, vals);
    if (mergedresult == eSolveResult::Minimized && prefs().pairSolveMode == ePairSolveMode::Sequential)
        return solve(mainsys, vals);
    else
        return mergedresult; //success

}

PyObject* SolverBackend::getPyObject()
{
    if (_twin == nullptr){
        new SolverBackendPy(this);
        assert(_twin);
        return _twin;
    } else {
        return Py::new_reference_to(_twin);
    }
}

HSolverBackend SolverBackend::self()
{
    return HSolverBackend(getPyObject(), true);
}

SolverError::SolverError()
    :Exception()
{

}

SolverError::SolverError(const char* sMessage)
    : Exception(sMessage)
{

}

SolverError::SolverError(const std::string& sMessage)
    : Exception(sMessage)
{

}

SolverError::SolverError(const SolverError& inst)
    : Exception(inst)
{

}

PyObject* SolverError::getPyExceptionType() const
{
    return Base::BaseExceptionFreeCADError; //#FIXME: specialize
}

Py::Dict SolverBackend::Prefs::getPyValue() const
{
    Py::Dict ret;
    ret["maxIter"] = Py::Long(this->maxIter);
    ret["maxIterSizeMult"] = Py::Boolean(this->maxIterSizeMult);
    const char* debugModeStrings[] = {"NoDebug", "Minimal", "IterationLevel", nullptr};
    ret["debugMode"] = Py::String(debugModeStrings[int(this->debugMode)]);
    ret["errorForSolved"] = Py::Float(this->errorForSolved);
    const char* pairSolveModeStrings[] = {"Sequential", "Merged", nullptr};
    ret["pairSolveMode"] = Py::String(pairSolveModeStrings[int(this->pairSolveMode)]);
    return ret;
}

void SolverBackend::Prefs::setAttr(std::string attrname, Py::Object value)
{
    if (attrname == "maxIter"){
        this->maxIter = Py::Long(value);
    }
    else if (attrname == "maxIterSizeMult") {
        this->maxIterSizeMult = Py::Boolean(value);
    }
    else if (attrname == "debugMode") {
        const char* debugModeStrings[] = {"NoDebug", "Minimal", "IterationLevel", nullptr};
        this->debugMode = str2enum<eDebugMode>(value, debugModeStrings);
    }
    else if (attrname == "errorForSolved") {
        this->errorForSolved = Py::Float(value);
    }
    else if (attrname == "pairSolveMode") {
        const char* pairSolveModeStrings[] = {"Sequential", "Merged", nullptr};
        this->pairSolveMode = str2enum<ePairSolveMode>(value, pairSolveModeStrings);
    }
    else
        throw Base::AttributeError("No solver preference named "+attrname);

}

void SolverBackend::Prefs::setPyValue(Py::Dict d)
{
    for (auto pair : d) {
        try {
            this->setAttr(Py::String(pair.first), pair.second);
        } catch (Base::AttributeError& e){
            //do not throw attribute errors for improved compatibility with future code
            Base::Console().Warning(e.what());
            Base::Console().Warning("\n");
        }
    }
}
