// SPDX-License-Identifier: LGPL-2.1-or-later

#include <Base/Console.h>
#include <Base/Exception.h>
#include <Base/Interpreter.h>
#include <Base/PyObjectBase.h>
#include <App/DocumentObjectPy.h>

#include "AsmtIO.h"
#include "MbDAction.h"
#include "MbDAssembly.h"
#include "MbDFolders.h"
#include "MbDItemIJ.h"
#include "MbDJoint.h"
#include "MbDMassMarker.h"
#include "MbDMarker.h"
#include "MbDMotion.h"
#include "MbDParameters.h"
#include "MbDPart.h"

namespace MbDFEM
{

class Module: public Py::ExtensionModule<Module>
{
public:
    Module()
        : Py::ExtensionModule<Module>("MbDFEM")
    {
        add_varargs_method("exportAssemblyAsmt",
                           &Module::exportAssemblyAsmt,
                           "exportAssemblyAsmt(assembly, filename) -- Export an MbDAssembly as ASMT.");
        add_varargs_method("importSolvedAsmt",
                           &Module::importSolvedAsmt,
                           "importSolvedAsmt(assembly, filename) -- Import solved ASMT result series.");
        initialize("The MbDFEM module.");
    }

private:
    Py::Object invoke_method_varargs(void* method_def, const Py::Tuple& args) override
    {
        try {
            return Py::ExtensionModule<Module>::invoke_method_varargs(method_def, args);
        }
        catch (const Base::Exception& e) {
            throw Py::Exception(e.getPyExceptionType(), e.what());
        }
        catch (const std::exception& e) {
            throw Py::RuntimeError(e.what());
        }
    }

    Py::Object exportAssemblyAsmt(const Py::Tuple& args)
    {
        PyObject* object {};
        char* filename {};
        if (!PyArg_ParseTuple(args.ptr(),
                              "O!et",
                              &App::DocumentObjectPy::Type,
                              &object,
                              "utf-8",
                              &filename)) {
            throw Py::Exception();
        }

        std::string encodedFilename(filename);
        PyMem_Free(filename);

        auto* documentObject = static_cast<App::DocumentObjectPy*>(object)->getDocumentObjectPtr();
        auto* assembly = freecad_cast<MbDFEM::MbDAssembly*>(documentObject);
        if (!assembly) {
            throw Py::TypeError("exportAssemblyAsmt expects an MbDFEM::MbDAssembly");
        }

        return Py::String(MbDFEM::exportAssemblyAsmt(assembly, encodedFilename));
    }

    Py::Object importSolvedAsmt(const Py::Tuple& args)
    {
        PyObject* object {};
        char* filename {};
        if (!PyArg_ParseTuple(args.ptr(),
                              "O!et",
                              &App::DocumentObjectPy::Type,
                              &object,
                              "utf-8",
                              &filename)) {
            throw Py::Exception();
        }

        std::string encodedFilename(filename);
        PyMem_Free(filename);

        auto* documentObject = static_cast<App::DocumentObjectPy*>(object)->getDocumentObjectPtr();
        auto* assembly = freecad_cast<MbDFEM::MbDAssembly*>(documentObject);
        if (!assembly) {
            throw Py::TypeError("importSolvedAsmt expects an MbDFEM::MbDAssembly");
        }

        Py::List result;
        for (auto* imported : MbDFEM::importSolvedAsmt(assembly, encodedFilename)) {
            result.append(Py::Object(imported->getPyObject(), true));
        }
        return result;
    }
};

PyObject* initModule()
{
    return Base::Interpreter().addModule(new Module);
}

}  // namespace MbDFEM

PyMOD_INIT_FUNC(MbDFEM)
{
    // load dependent module
    try {
        Base::Interpreter().runString("import Part");
    }
    catch (const Base::Exception& e) {
        PyErr_SetString(PyExc_ImportError, e.what());
        PyMOD_Return(nullptr);
    }

    PyObject* module = MbDFEM::initModule();
    MbDFEM::MbDAssembly::init();
    MbDFEM::MbDPart::init();
    MbDFEM::MbDMarker::init();
    MbDFEM::MbDMassMarker::init();
    MbDFEM::MbDItemIJ::init();
    MbDFEM::MbDJoint::init();
    MbDFEM::MbDMotion::init();
    MbDFEM::MbDAction::init();
    MbDFEM::MbDGravity::init();
    MbDFEM::MbDSimulationParameters::init();
    MbDFEM::MbDAnimationParameters::init();
    MbDFEM::MbDAssembliesFolder::init();
    MbDFEM::MbDPartsFolder::init();
    MbDFEM::MbDFixedPartsFolder::init();
    MbDFEM::MbDMarkersFolder::init();
    MbDFEM::MbDJointsFolder::init();
    MbDFEM::MbDMotionsFolder::init();
    MbDFEM::MbDActionsFolder::init();
    Base::Console().log("Loading MbDFEM module... done\n");
    PyMOD_Return(module);
}
