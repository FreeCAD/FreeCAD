// SPDX-License-Identifier: LGPL-2.1-or-later

#include "CadXService.h"

#include <Base/Console.h>
#include <Base/Interpreter.h>
#include <Base/PyObjectBase.h>

#include <Python.h>

namespace CadX
{
namespace
{
class ScopedGilRelease
{
public:
    ScopedGilRelease()
        : _state(PyEval_SaveThread())
    {}

    ScopedGilRelease(const ScopedGilRelease&) = delete;
    ScopedGilRelease& operator=(const ScopedGilRelease&) = delete;

    ~ScopedGilRelease()
    {
        PyEval_RestoreThread(_state);
    }

private:
    PyThreadState* _state;
};
}  // namespace

class Module: public Py::ExtensionModule<Module>
{
public:
    Module()
        : Py::ExtensionModule<Module>("CadXApp")
    {
        add_varargs_method("execute_tool", &Module::executeTool, "execute_tool(name, arguments_json)");
        add_varargs_method("graph_evidence", &Module::graphEvidence,
                           "graph_evidence(graph_id, graph_revision)");
        add_noargs_method("tool_names", &Module::toolNames, "Return native provider-neutral tool names.");
        initialize("Provider-neutral CadX graph services.");
    }

private:
    Py::Object executeTool(const Py::Tuple& args)
    {
        if (args.size() != 2) {
            throw Py::TypeError("execute_tool expects a tool name and JSON arguments");
        }
        const auto name = static_cast<std::string>(Py::String(args[0]));
        const auto arguments = static_cast<std::string>(Py::String(args[1]));
        ToolResult result;
        {
            ScopedGilRelease release;
            result = service().executeTool(name, arguments);
        }
        return Py::String(result.toJson());
    }

    Py::Object toolNames()
    {
        Py::List names;
        for (const auto& definition : service().toolRegistry().definitions()) {
            names.append(Py::String(definition.name));
        }
        return names;
    }

    Py::Object graphEvidence(const Py::Tuple& args)
    {
        if (args.size() != 2) {
            throw Py::TypeError("graph_evidence expects a graph id and revision");
        }
        const auto graphId = static_cast<std::string>(Py::String(args[0]));
        const auto revision = static_cast<std::string>(Py::String(args[1]));
        return Py::String(service().exportGraphEvidence(graphId, revision).toJson());
    }
};

}  // namespace CadX

namespace CadX
{
PyObject* initModule()
{
    return Base::Interpreter().addModule(new Module);
}
}  // namespace CadX

PyMOD_INIT_FUNC(CadXApp)
{
    PyObject* mod = CadX::initModule();
    Base::Console().log("Loading CadXApp module... done\n");
    PyMOD_Return(mod);
}
