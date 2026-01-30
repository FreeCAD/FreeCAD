// SPDX-License-Identifier: LGPL-2.1-or-later

#include <gtest/gtest.h>

#include <Python.h>

#include "Base/Translation.h"
#include "Base/Translate.h"
#include "TranslationTestHelpers.h"

namespace
{

PyObject* getTranslateModule()
{
    PyObject* modules = PyImport_GetModuleDict();
    PyObject* existing = PyDict_GetItemString(modules, "__Translate__");
    if (existing) {
        Py_INCREF(existing);
        return existing;
    }

    static Base::Translate* translateModule = new Base::Translate();  // NOLINT
    const Py::Object moduleObject = translateModule->moduleObject();
    PyObject* module = moduleObject.ptr();
    if (!module) {
        return nullptr;
    }

    if (PyDict_SetItemString(modules, "__Translate__", module) != 0) {
        return nullptr;
    }
    Py_INCREF(module);
    return module;
}
PyObject* callTranslate(PyObject* module, const char* context, const char* source)
{
    PyObject* func = PyObject_GetAttrString(module, "translate");
    if (!func) {
        return nullptr;
    }
    PyObject* args = Py_BuildValue("(ss)", context, source);
    PyObject* ret = PyObject_CallObject(func, args);
    Py_DECREF(args);
    Py_DECREF(func);
    return ret;
}

}  // namespace

TEST(TranslateModule, TranslateUsesHandlerOrFallback)
{
    Py_Initialize();
    PyGILState_STATE gil = PyGILState_Ensure();

    Base::Translation::setTranslator(nullptr);

    PyObject* module = getTranslateModule();
    ASSERT_NE(nullptr, module);

    PyObject* retA = callTranslate(module, "Ctx", "Hello");
    ASSERT_NE(nullptr, retA);
    ASSERT_TRUE(PyUnicode_Check(retA));
    EXPECT_STREQ("Hello", PyUnicode_AsUTF8(retA));
    Py_DECREF(retA);

    Base::Translation::Test::RecordingTranslator translator;
    translator.translateMode = Base::Translation::Test::RecordingTranslator::TranslateMode::Constant;
    translator.constantTranslation = "Bonjour";
    Base::Translation::Test::ScopedTranslator scoped(&translator);

    PyObject* retB = callTranslate(module, "Ctx", "Hello");
    ASSERT_NE(nullptr, retB);
    ASSERT_TRUE(PyUnicode_Check(retB));
    EXPECT_STREQ("Bonjour", PyUnicode_AsUTF8(retB));
    Py_DECREF(retB);

    Py_DECREF(module);
    PyGILState_Release(gil);
}

TEST(TranslateModule, InstallAndRemoveUseHandlers)
{
    Py_Initialize();
    PyGILState_STATE gil = PyGILState_Ensure();

    Base::Translation::Test::RecordingTranslator translator;
    translator.installResult = true;
    translator.removeResult = true;
    Base::Translation::Test::ScopedTranslator scoped(&translator);

    PyObject* module = getTranslateModule();
    ASSERT_NE(nullptr, module);

    PyObject* funcInstall = PyObject_GetAttrString(module, "installTranslator");
    ASSERT_NE(nullptr, funcInstall);
    PyObject* argInstall = PyUnicode_FromString("a.qm");
    ASSERT_NE(nullptr, argInstall);
    PyObject* argsInstall = PyTuple_Pack(1, argInstall);
    Py_DECREF(argInstall);
    ASSERT_NE(nullptr, argsInstall);
    PyObject* retInstall = PyObject_CallObject(funcInstall, argsInstall);
    Py_DECREF(argsInstall);
    Py_DECREF(funcInstall);
    ASSERT_NE(nullptr, retInstall);
    EXPECT_TRUE(PyObject_IsTrue(retInstall));
    Py_DECREF(retInstall);

    PyObject* funcRemove = PyObject_GetAttrString(module, "removeTranslators");
    ASSERT_NE(nullptr, funcRemove);
    PyObject* argsRemove = PyTuple_New(0);
    PyObject* retRemove = PyObject_CallObject(funcRemove, argsRemove);
    Py_DECREF(argsRemove);
    Py_DECREF(funcRemove);
    ASSERT_NE(nullptr, retRemove);
    EXPECT_TRUE(PyObject_IsTrue(retRemove));
    Py_DECREF(retRemove);

    Py_DECREF(module);

    EXPECT_EQ(translator.installCalls, 1);
    EXPECT_EQ(translator.lastInstalledFilename, "a.qm");
    EXPECT_EQ(translator.removeCalls, 1);
    EXPECT_EQ(translator.lastRemovedFilenames, std::vector<std::string>({"a.qm"}));

    PyGILState_Release(gil);
}
