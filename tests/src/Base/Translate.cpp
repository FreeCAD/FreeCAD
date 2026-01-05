// SPDX-License-Identifier: LGPL-2.1-or-later

#include <gtest/gtest.h>

#include <functional>

#include <Python.h>

#include "Base/Translation.h"
#include "Base/Translate.h"

namespace
{

class ScopedTranslator
{
public:
    explicit ScopedTranslator(const Base::Translation::Translator* translator)
        : previous {Base::Translation::getTranslator()}
    {
        Base::Translation::setTranslator(translator);
    }

    ~ScopedTranslator()
    {
        Base::Translation::setTranslator(previous);
    }

    ScopedTranslator(const ScopedTranslator&) = delete;
    ScopedTranslator(ScopedTranslator&&) = delete;
    ScopedTranslator& operator=(const ScopedTranslator&) = delete;
    ScopedTranslator& operator=(ScopedTranslator&&) = delete;

private:
    const Base::Translation::Translator* previous;
};

class TestTranslator final : public Base::Translation::Translator
{
public:
    std::function<std::string(std::string_view, std::string_view, std::string_view, int)> onTranslate;
    std::function<bool(std::string_view)> onInstallTranslator;
    std::function<bool(const std::vector<std::string>&)> onRemoveTranslators;

    std::string translate(
        std::string_view context,
        std::string_view sourceText,
        std::string_view disambiguation,
        int n
    ) const override
    {
        if (onTranslate) {
            return onTranslate(context, sourceText, disambiguation, n);
        }
        return std::string(sourceText);
    }

    bool installTranslator(std::string_view filename) const override
    {
        if (onInstallTranslator) {
            return onInstallTranslator(filename);
        }
        return false;
    }

    bool removeTranslators(const std::vector<std::string>& filenames) const override
    {
        if (onRemoveTranslators) {
            return onRemoveTranslators(filenames);
        }
        return false;
    }
};
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
    Base::Translation::setTranslator(nullptr);
    Base::Translate mod;

    PyObject* module = PyImport_ImportModule("__Translate__");
    ASSERT_NE(nullptr, module);

    PyObject* retA = callTranslate(module, "Ctx", "Hello");
    ASSERT_NE(nullptr, retA);
    ASSERT_TRUE(PyUnicode_Check(retA));
    EXPECT_STREQ("Hello", PyUnicode_AsUTF8(retA));
    Py_DECREF(retA);

    TestTranslator translator;
    translator.onTranslate = [](std::string_view, std::string_view, std::string_view, int) {
        return std::string("Bonjour");
    };
    ScopedTranslator scoped(&translator);

    PyObject* retB = callTranslate(module, "Ctx", "Hello");
    ASSERT_NE(nullptr, retB);
    ASSERT_TRUE(PyUnicode_Check(retB));
    EXPECT_STREQ("Bonjour", PyUnicode_AsUTF8(retB));
    Py_DECREF(retB);

    Py_DECREF(module);
}

TEST(TranslateModule, InstallAndRemoveUseHandlers)
{
    Py_Initialize();

    std::vector<std::string> installed;
    std::vector<std::string> removed;

    TestTranslator translator;
    translator.onInstallTranslator = [&installed](std::string_view filename) {
        installed.push_back(std::string(filename));
        return true;
    };
    translator.onRemoveTranslators = [&removed](const std::vector<std::string>& filenames) {
        removed = filenames;
        return true;
    };
    ScopedTranslator scoped(&translator);

    Base::Translate mod;
    PyObject* module = PyImport_ImportModule("__Translate__");
    ASSERT_NE(nullptr, module);

    PyObject* funcInstall = PyObject_GetAttrString(module, "installTranslator");
    ASSERT_NE(nullptr, funcInstall);
    PyObject* argsInstall = Py_BuildValue("(s)", "a.qm");
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

    EXPECT_EQ(std::vector<std::string>({"a.qm"}), installed);
    EXPECT_EQ(std::vector<std::string>({"a.qm"}), removed);
}
