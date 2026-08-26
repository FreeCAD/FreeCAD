// SPDX-License-Identifier: LGPL-2.1-or-later

#include "ActiveAssemblyResolver.h"
#include "AssemblyViewCapture.h"
#include "MainThreadGateway.h"
#include "../App/CadXService.h"

#include <QJsonDocument>
#include <QJsonObject>

#include <Base/Console.h>
#include <Base/Interpreter.h>
#include <Base/PyObjectBase.h>

#include <Python.h>

namespace
{
// PyCXX gives this extension the GIL on entry.  BlockingQueuedConnection can
// wait for the GUI thread, so keep the Python interpreter available to that
// thread and restore it before constructing/returning PyCXX objects.
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

namespace CadXGui
{
class Module: public Py::ExtensionModule<Module>
{
public:
    Module()
        : Py::ExtensionModule<Module>("CadXGuiApp")
    {
        add_varargs_method("execute_tool", &Module::executeTool, "execute_tool(name, arguments_json)");
        add_varargs_method("graph_evidence", &Module::graphEvidence,
                           "graph_evidence(graph_id, graph_revision)");
        add_noargs_method("tool_names", &Module::toolNames, "Return native provider-neutral tool names.");
        initialize("CadX active-view capture gateway.");
        std::string diagnostic;
        if (!CadX::service().registerGuiSnapshotProvider(
                [](const std::string& arguments) { return executeGuiSnapshot(arguments); },
                diagnostic)) {
            Base::Console().error(("CadX snapshot registration failed: " + diagnostic + "\n").c_str());
        }
    }

private:
    Py::Object executeTool(const Py::Tuple& args)
    {
        if (args.size() != 2) {
            throw Py::TypeError("execute_tool expects a tool name and JSON arguments");
        }
        const auto name = static_cast<std::string>(Py::String(args[0]));
        const auto arguments = static_cast<std::string>(Py::String(args[1]));
        CadX::ToolResult result;
        {
            ScopedGilRelease release;
            result = CadX::service().executeTool(name, arguments);
        }
        return Py::String(result.toJson());
    }

    Py::Object toolNames()
    {
        Py::List names;
        for (const auto& definition : CadX::service().toolRegistry().definitions()) {
            names.append(Py::String(definition.name));
        }
        return names;
    }

    static CadX::ToolResult executeGuiSnapshot(const std::string& arguments)
    {
        const auto parsed = QJsonDocument::fromJson(QByteArray::fromStdString(arguments));
        const auto object = parsed.object();
        CadX::AssemblyViewCaptureOptions options;
        if (object.contains("geometry_detail")) {
            options.geometryDetail = object.value("geometry_detail").toString().toStdString();
        }
        if (object.contains("include_view_state")) {
            options.includeViewState = object.value("include_view_state").toBool();
        }
        const auto refresh = object.value("refresh").toString("always");
        if (refresh == "if_stale") {
            const auto resolved = CadX::ActiveAssemblyResolver().resolve();
            if (resolved.ok) {
                const CadX::GraphScope scope {
                    resolved.context.documentUid,
                    resolved.context.assemblyObjectName};
                const auto current = CadX::service().graphStore().current(scope, false);
                if (current) {
                    return CadX::service().summarizeSnapshot(*current.snapshot);
                }
            }
        }
        const auto captured = CadX::AssemblyViewCapture().capture(options);
        if (!captured.ok) {
            return CadX::ToolResult::failure(captured.errorCode, captured.diagnostic, true);
        }
        return CadX::service().publishCapture(captured.capture);
    }

    CadX::MainThreadGateway _gateway;

    Py::Object graphEvidence(const Py::Tuple& args)
    {
        if (args.size() != 2) {
            throw Py::TypeError("graph_evidence expects a graph id and revision");
        }
        const auto graphId = static_cast<std::string>(Py::String(args[0]));
        const auto revision = static_cast<std::string>(Py::String(args[1]));
        return Py::String(CadX::service().exportGraphEvidence(graphId, revision).toJson());
    }
};

PyObject* initModule()
{
    return Base::Interpreter().addModule(new Module);
}
}  // namespace CadXGui

PyMOD_INIT_FUNC(CadXGuiApp)
{
    PyObject* mod = CadXGui::initModule();
    Base::Console().log("Loading CadXGuiApp module... done\n");
    PyMOD_Return(mod);
}
