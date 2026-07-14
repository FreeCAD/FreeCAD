// SPDX-License-Identifier: LGPL-2.1-or-later

#include "PreCompiled.h"

#ifndef _PreComp_
#include <QByteArray>
#include <QCoreApplication>
#include <QGuiApplication>
#endif

#include "GuiApplicationBootstrap.h"

#include <Base/Console.h>
#include <Base/Interpreter.h>

#include <cerrno>
#include <cstdlib>
#include <cstring>

namespace App::QtApp
{
namespace
{
struct BootstrapState
{
    std::unique_ptr<QGuiApplication> application;
    PyObject* pythonApplication {nullptr};  // Held for process lifetime when created via PySide
    bool forcedPlatform {false};
};

BootstrapState& state()
{
    static BootstrapState bootstrapState;
    return bootstrapState;
}

void setProcessEnvVar(const char* key, const char* value)
{
#ifdef FC_OS_WIN32
    if (_putenv_s(key, value) != 0) {
        Base::Console().warning(
            "App::QtApp::ensureGuiApplication: failed to set %s via _putenv_s\n", key);
    }
#else
    if (::setenv(key, value, 1) != 0) {
        Base::Console().warning(
            "App::QtApp::ensureGuiApplication: failed to set %s via setenv (%s)\n",
            key,
            std::strerror(errno));
    }
#endif
}

void syncPythonEnvironment(const char* key, const char* value)
{
    if (!Py_IsInitialized()) {
        return;
    }

    Base::PyGILStateLocker locker;

    PyObject* osModule = PyImport_ImportModule("os");
    if (!osModule) {
        PyErr_Clear();
        return;
    }

    PyObject* environ = PyObject_GetAttrString(osModule, "environ");
    Py_DECREF(osModule);
    if (!environ) {
        PyErr_Clear();
        return;
    }

    PyObject* pyValue = PyUnicode_FromString(value);
    if (!pyValue) {
        PyErr_Clear();
        Py_DECREF(environ);
        return;
    }

    if (PyMapping_SetItemString(environ, key, pyValue) != 0) {
        PyErr_Clear();
    }

    Py_DECREF(pyValue);
    Py_DECREF(environ);
}

bool validateExistingInstance()
{
    auto* guiApp = qobject_cast<QGuiApplication*>(QCoreApplication::instance());
    if (guiApp) {
        return true;
    }

    if (QCoreApplication::instance()) {
        Base::Console().warning(
            "App::QtApp::ensureGuiApplication: existing QCoreApplication is not a QGuiApplication; "
            "cannot initialise GUI stack. Host embedder must create QGuiApplication (or allow "
            "FreeCAD to) before calling headless render paths.\n");
    }

    return false;
}

void maybeForceEnvironment(BootstrapState& bootstrapState)
{
    bootstrapState.forcedPlatform = false;
    if (qEnvironmentVariableIsEmpty("QT_QPA_PLATFORM")) {
        const QByteArray platformValue = QByteArrayLiteral("offscreen");
        qputenv("QT_QPA_PLATFORM", platformValue);
        setProcessEnvVar("QT_QPA_PLATFORM", platformValue.constData());
        syncPythonEnvironment("QT_QPA_PLATFORM", platformValue.constData());
        bootstrapState.forcedPlatform = true;
    }
    if (qEnvironmentVariableIsEmpty("QT_OPENGL")) {
        const QByteArray openglValue = QByteArrayLiteral("software");
        qputenv("QT_OPENGL", openglValue);
        setProcessEnvVar("QT_OPENGL", openglValue.constData());
        syncPythonEnvironment("QT_OPENGL", openglValue.constData());
    }
}

bool bootstrapWithPySide(BootstrapState& bootstrapState, const char* moduleName)
{
    if (!Py_IsInitialized()) {
        return false;
    }

    Base::PyGILStateLocker locker;

    PyObject* module = PyImport_ImportModule(moduleName);
    if (!module) {
        PyErr_Clear();
        return false;
    }

    PyObject* qguiApplicationClass = PyObject_GetAttrString(module, "QGuiApplication");
    Py_DECREF(module);
    if (!qguiApplicationClass) {
        PyErr_Clear();
        return false;
    }

    PyObject* instanceFunc = PyObject_GetAttrString(qguiApplicationClass, "instance");
    if (!instanceFunc) {
        PyErr_Clear();
        Py_DECREF(qguiApplicationClass);
        return false;
    }

    PyObject* existing = PyObject_CallObject(instanceFunc, nullptr);
    Py_DECREF(instanceFunc);
    if (!existing) {
        PyErr_Clear();
    }
    else {
        const bool alreadyRunning = existing != Py_None;
        Py_DECREF(existing);
        if (alreadyRunning) {
            Py_DECREF(qguiApplicationClass);
            return true;
        }
    }

    PyObject* argvList = PyList_New(0);
    if (!argvList) {
        PyErr_Clear();
        Py_DECREF(qguiApplicationClass);
        return false;
    }

    auto appendArg = [argvList](const char* value) -> bool {
        PyObject* str = PyUnicode_FromString(value);
        if (!str) {
            PyErr_Clear();
            return false;
        }
        const int result = PyList_Append(argvList, str);
        Py_DECREF(str);
        if (result != 0) {
            PyErr_Clear();
            return false;
        }
        return true;
    };

    if (!appendArg("FreeCADHeadless")) {
        Py_DECREF(argvList);
        Py_DECREF(qguiApplicationClass);
        return false;
    }

    if (bootstrapState.forcedPlatform) {
        if (!appendArg("-platform") || !appendArg("offscreen")) {
            Py_DECREF(argvList);
            Py_DECREF(qguiApplicationClass);
            return false;
        }
    }

    PyObject* args = PyTuple_New(1);
    if (!args) {
        Py_DECREF(argvList);
        Py_DECREF(qguiApplicationClass);
        PyErr_Clear();
        return false;
    }

    PyTuple_SET_ITEM(args, 0, argvList);  // steals reference

    PyObject* newApp = PyObject_CallObject(qguiApplicationClass, args);
    Py_DECREF(args);
    Py_DECREF(qguiApplicationClass);

    if (!newApp) {
        Base::Console().warning(
            "App::QtApp::ensureGuiApplication: %s failed to create QGuiApplication\n",
            moduleName);
        PyErr_Clear();
        return false;
    }

    bootstrapState.pythonApplication = newApp;  // keep reference alive for process lifetime
    if (!QGuiApplication::instance()) {
        Base::Console().warning(
            "App::QtApp::ensureGuiApplication: %s QGuiApplication failed to initialise\n",
            moduleName);
        Py_DECREF(newApp);
        bootstrapState.pythonApplication = nullptr;
        return false;
    }

    return true;
}

}  // namespace

bool ensureGuiApplication()
{
    if (hasGuiApplication()) {
        return true;
    }

    if (validateExistingInstance()) {
        return true;
    }

    // Do not try to create a QGuiApplication when a non-GUI QCoreApplication
    // is already active; Qt only permits a single application instance and
    // embedders must provide QGuiApplication themselves in that case.
    if (QCoreApplication::instance()) {
        return false;
    }

    auto& bootstrapState = state();

    maybeForceEnvironment(bootstrapState);

    // Try Python-side bootstrap first to reuse any existing interpreter state
#if QT_VERSION_MAJOR >= 6
    if (bootstrapWithPySide(bootstrapState, "PySide6.QtGui")) {
        return true;
    }
#else
    if (bootstrapWithPySide(bootstrapState, "PySide2.QtGui")) {
        return true;
    }
#endif

    static char appName[] = "FreeCADHeadless";
    static char platformOpt[] = "-platform";
    static char platformVal[] = "offscreen";
    static char* argvWithPlatform[] = {appName, platformOpt, platformVal, nullptr};
    static int argcWithPlatform = 3;
    static char* argvNoPlatform[] = {appName, nullptr};
    static int argcNoPlatform = 1;

    int argc = bootstrapState.forcedPlatform ? argcWithPlatform : argcNoPlatform;
    char** argv = bootstrapState.forcedPlatform ? argvWithPlatform : argvNoPlatform;

    try {
        bootstrapState.application = std::make_unique<QGuiApplication>(argc, argv);
    }
    catch (const std::exception& e) {
        Base::Console().error("App::QtApp::ensureGuiApplication: %s\n", e.what());
        bootstrapState.application.reset();
        return false;
    }
    catch (...) {
        Base::Console().error(
            "App::QtApp::ensureGuiApplication: unknown error while instantiating QGuiApplication\n");
        bootstrapState.application.reset();
        return false;
    }

    if (!hasGuiApplication()) {
        Base::Console().error(
            "App::QtApp::ensureGuiApplication: QGuiApplication failed to initialise\n");
        bootstrapState.application.reset();
        return false;
    }

    // Intentionally leak the QGuiApplication; Qt teardown ordering can crash when we delete it
    // after Python shutdown. Qt keeps the global instance for the process lifetime.
    bootstrapState.application.release();
    return true;
}

bool hasGuiApplication()
{
    return qobject_cast<QGuiApplication*>(QCoreApplication::instance()) != nullptr;
}

}  // namespace App::QtApp
