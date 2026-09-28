// SPDX-License-Identifier: LGPL-2.1-or-later

#include <gtest/gtest.h>
#include <gmock/gmock.h>

#include <Python.h>

#include <array>
#include <cstdlib>
#include <filesystem>
#include <format>
#include <fstream>
#include <string>

#include <CXX/Objects.hxx>

#include "Base/Exception.h"
#include "Base/FileInfo.h"
#include "Base/Interpreter.h"
#include <src/TempDirectory.h>

namespace
{

constexpr int interpreterArgc = 1;
std::array<char*, interpreterArgc> interpreterArgv {const_cast<char*>("FreeCADInterpreterTest")};

/// Name of the Python target class used by the method dispatch tests.
constexpr const char* methodTargetScript = "class _InterpreterTestTarget:\n"
                                           "    def noop(self):\n"
                                           "        pass\n"
                                           "    def add(self, a, b):\n"
                                           "        return a + b\n"
                                           "    def greet(self):\n"
                                           "        return 'hello'\n"
                                           "    def items(self):\n"
                                           "        return [1, 2, 3]\n"
                                           "_interpreter_test_target = _InterpreterTestTarget()\n";

PyObject* getMethodTarget()
{
    return Base::Interpreter().getValue("", "_interpreter_test_target");
}

}  // namespace

/**
 * @brief Regression tests for Base::InterpreterSingleton after its PyCXX conversion.
 *
 * The interpreter is brought up through InterpreterSingleton::init(), never through a raw
 * Py_Initialize(), so the tests exercise the same initialization path as the application.
 */
class InterpreterTest: public ::testing::Test
{
protected:
    static void SetUpTestSuite()
    {
        Base::Interpreter().init(interpreterArgc, interpreterArgv.data());
    }

    void SetUp() override
    {
        gilState = PyGILState_Ensure();
    }

    void TearDown() override
    {
        if (PyErr_Occurred() != nullptr) {
            PyErr_Clear();
        }
        PyGILState_Release(gilState);
    }

private:
    PyGILState_STATE gilState {};
};

TEST_F(InterpreterTest, runStringReturnsReprOfResult)  // NOLINT
{
    // Py_file_input executes statements, so the script result is always None
    EXPECT_EQ(Base::Interpreter().runString("1 + 1"), "None");
    EXPECT_EQ(Base::Interpreter().runString("_interpreter_test_var = 5"), "None");
    EXPECT_EQ(PyLong_AsLong(Base::Interpreter().runStringObject("_interpreter_test_var").ptr()), 5);
}

TEST_F(InterpreterTest, runStringObjectReturnsOwnedReference)  // NOLINT
{
    Py::Object result = Base::Interpreter().runStringObject("[1, 2, 3]");
    ASSERT_FALSE(result.isNull());
    EXPECT_TRUE(result.isList());
    EXPECT_EQ(PyList_Size(result.ptr()), 3);
    EXPECT_EQ(result.reference_count(), 1);

    {
        Py::Object copy = result;
        EXPECT_EQ(result.reference_count(), 2);
    }
    EXPECT_EQ(result.reference_count(), 1);
}

TEST_F(InterpreterTest, runStringWithKeyExchangesStrings)  // NOLINT
{
    const std::string result
        = Base::Interpreter().runStringWithKey("key = key + ' world'", "key", "hello");
    EXPECT_EQ(result, "hello world");

    // Once the application has registered its exception producers, PyException::throwException()
    // may reconstruct the original exception (e.g. Base::RuntimeError), so accept the base class.
    EXPECT_THROW(
        Base::Interpreter().runStringWithKey("raise RuntimeError('failure')", "key"),
        Base::Exception
    );
}

TEST_F(InterpreterTest, runStringWithKeyRemovedKeyRaises)  // NOLINT
{
    // A script deleting the key used to crash on a null object; it must raise instead and
    // leave no Python error behind.
    EXPECT_THROW(Base::Interpreter().runStringWithKey("del key", "key", "value"), Base::RuntimeError);
    EXPECT_EQ(PyErr_Occurred(), nullptr);
}

TEST_F(InterpreterTest, runStringArgDoesNotTruncateCommands)  // NOLINT
{
    // The previous fixed 1024-byte buffer silently executed the truncated command.
    const std::string padding(1100, 'x');
    Base::Interpreter().runStringArg("_interpreter_test_long_arg = '%s'", padding.c_str());
    EXPECT_TRUE(
        Base::Interpreter()
            .runStringObject(std::format("_interpreter_test_long_arg == '{}'", padding).c_str())
            .isTrue()
    );
    Base::Interpreter().runString("del _interpreter_test_long_arg");
}

TEST_F(InterpreterTest, runMethodHonoursResultFormats)  // NOLINT
{
    Base::Interpreter().runString(methodTargetScript);
    PyObject* target = getMethodTarget();
    ASSERT_NE(target, nullptr);

    EXPECT_NO_THROW(Base::Interpreter().runMethodVoid(target, "noop"));

    int sum = 0;
    Base::Interpreter().runMethod(target, "add", "i", &sum, "ii", 40, 2);
    EXPECT_EQ(sum, 42);

    // "s" returns a copy the caller must free
    char* greeting = nullptr;
    Base::Interpreter().runMethod(target, "greet", "s", &greeting);
    ASSERT_NE(greeting, nullptr);
    EXPECT_STREQ(greeting, "hello");
    std::free(greeting);  // NOLINT

    // "O" hands the reference over to the caller
    PyObject* items = nullptr;
    Base::Interpreter().runMethod(target, "items", "O", &items);
    ASSERT_NE(items, nullptr);
    EXPECT_TRUE(PyList_Check(items));
    EXPECT_EQ(PyList_Size(items), 3);
    EXPECT_EQ(Py_REFCNT(items), 1);
    Py_DECREF(items);

    PyObject* objectResult = Base::Interpreter().runMethodObject(target, "items");
    ASSERT_NE(objectResult, nullptr);
    EXPECT_TRUE(PyList_Check(objectResult));
    EXPECT_EQ(Py_REFCNT(objectResult), 1);
    Py_DECREF(objectResult);

    Py_DECREF(target);
}

TEST_F(InterpreterTest, runMethodObjectDoesNotLeakReferences)  // NOLINT
{
    Base::Interpreter().runString(methodTargetScript);
    PyObject* target = getMethodTarget();
    ASSERT_NE(target, nullptr);

    const Py_ssize_t targetRefcount = Py_REFCNT(target);
    for (int i = 0; i < 1000; ++i) {
        PyObject* result = Base::Interpreter().runMethodObject(target, "items");
        ASSERT_NE(result, nullptr);
        ASSERT_EQ(Py_REFCNT(result), 1);
        Py_DECREF(result);
    }
    EXPECT_EQ(Py_REFCNT(target), targetRefcount);

    Py_DECREF(target);
}

TEST_F(InterpreterTest, runMethodErrorsMapToFreeCadExceptions)  // NOLINT
{
    Base::Interpreter().runString(methodTargetScript);
    PyObject* target = getMethodTarget();
    ASSERT_NE(target, nullptr);

    EXPECT_THROW(Base::Interpreter().runMethodVoid(target, "missing_method"), Base::PyException);

    int result = 0;
    EXPECT_THROW(
        Base::Interpreter().runMethod(target, "add", "i", &result, "(i)", 1),
        Base::RuntimeError
    );

    Py_DECREF(target);
}

TEST_F(InterpreterTest, pyExceptionCapturesPythonErrorDetails)  // NOLINT
{
    PyErr_SetString(PyExc_ValueError, "boom");

    const Base::PyException e;
    using ::testing::HasSubstr;
    EXPECT_THAT(e.getErrorType(), HasSubstr("ValueError"));
    EXPECT_THAT(e.what(), HasSubstr("boom"));
    EXPECT_FALSE(e.getReported());
    // constructing the exception consumes and clears the Python error indicator
    EXPECT_EQ(PyErr_Occurred(), nullptr);
}

TEST_F(InterpreterTest, pyExceptionDoesNotReusePreviousStackTrace)  // NOLINT
{
    EXPECT_THROW(Base::Interpreter().runString("raise ValueError('x')"), Base::Exception);

    // A traceback-less error must not report the previous error's traceback
    PyErr_SetString(PyExc_ValueError, "no-traceback");
    const Base::PyException e;
    EXPECT_TRUE(e.getStackTrace().empty());
}

TEST_F(InterpreterTest, pyExceptionKeepsDynamicExceptionTypeAlive)  // NOLINT
{
    Py::Object exceptionType = Base::Interpreter().runStringObject(
        "type('_InterpreterTestDynamicError', (Exception,), {})"
    );
    ASSERT_FALSE(exceptionType.isNull());

    Py::Object instance = Py::Callable(exceptionType).apply(Py::TupleN(Py::String("dynamic")));
    ASSERT_FALSE(instance.isNull());

    // Hand the only remaining references over to the Python error indicator and drop ours
    PyErr_SetObject(exceptionType.ptr(), instance.ptr());
    instance = Py::Object();
    exceptionType = Py::Object();

    // The dynamically created type must survive the capture even though the exception
    // instance that referenced it is destroyed while fetching the error.
    const Base::PyException e;
    PyObject* captured = e.getPyExceptionType();
    ASSERT_NE(captured, nullptr);
    ASSERT_TRUE(PyType_Check(captured));
    EXPECT_STREQ(reinterpret_cast<PyTypeObject*>(captured)->tp_name, "_InterpreterTestDynamicError");
}

TEST_F(InterpreterTest, runStringFailureRaisesBaseException)  // NOLINT
{
    EXPECT_THROW(Base::Interpreter().runString("raise ValueError('boom')"), Base::Exception);
}

TEST_F(InterpreterTest, runStringMapsSystemExit)  // NOLINT
{
    try {
        Base::Interpreter().runString("raise SystemExit(7)");
        FAIL() << "Expected a Base::SystemExitException";
    }
    catch (const Base::SystemExitException& e) {
        EXPECT_EQ(e.getExitCode(), 7L);
    }
    catch (...) {
        FAIL() << "Expected a Base::SystemExitException";
    }

    try {
        Base::Interpreter().runString(R"(raise SystemExit("bye"))");
        FAIL() << "Expected a Base::SystemExitException";
    }
    catch (const Base::SystemExitException& e) {
        EXPECT_EQ(e.getExitCode(), 1L);
        EXPECT_THAT(e.what(), ::testing::HasSubstr("bye"));
    }
    catch (...) {
        FAIL() << "Expected a Base::SystemExitException";
    }
}

TEST_F(InterpreterTest, runInteractiveStringExecutesAndReportsErrors)  // NOLINT
{
    EXPECT_NO_THROW(Base::Interpreter().runInteractiveString("pass"));
    try {
        Base::Interpreter().runInteractiveString("raise RuntimeError('interactive-failure')");
        FAIL() << "Expected a Base::RuntimeError";
    }
    catch (const Base::RuntimeError& e) {
        // The Python error message must be preserved (normalized exception instance
        // since Python 3.12, not a raw string argument)
        EXPECT_THAT(e.what(), ::testing::HasSubstr("interactive-failure"));
    }
    catch (...) {
        FAIL() << "Expected a Base::RuntimeError";
    }
}

TEST_F(InterpreterTest, runFileLocalAndGlobalNamespaces)  // NOLINT
{
    tests::TempDirectory tempDir;
    const auto scriptPath = tempDir.path() / "interpreter_test.py";
    {
        std::ofstream script(scriptPath);
        script << "file_var = 'from-file'\n";
    }
    const std::string script = Base::FileInfo::pathToString(scriptPath);

    // local=true runs in a copy of the __main__ namespace
    Base::Interpreter().runFile(script.c_str(), true);
    EXPECT_FALSE(Base::Interpreter().runStringObject("'file_var' in globals()").isTrue());

    // local=false runs in the __main__ namespace itself
    Base::Interpreter().runFile(script.c_str(), false);
    EXPECT_TRUE(Base::Interpreter().runStringObject("'file_var' in globals()").isTrue());
    EXPECT_EQ(
        static_cast<std::string>(Py::String(Base::Interpreter().runStringObject("file_var"))),
        "from-file"
    );

    // cleanup
    Base::Interpreter().runString("del file_var");
    Base::Interpreter().runString("globals().pop('__file__', None)");
}

TEST_F(InterpreterTest, getValueReturnsOwnedReferenceFromMainModule)  // NOLINT
{
    Base::Interpreter().runString("_interpreter_test_value = [21, 42]");
    PyObject* value = Base::Interpreter().getValue("", "_interpreter_test_value");
    ASSERT_NE(value, nullptr);
    EXPECT_TRUE(PyList_Check(value));
    EXPECT_EQ(PyList_Size(value), 2);
    // the module dictionary keeps its own reference on top of the one handed to the caller
    EXPECT_GT(Py_REFCNT(value), 1);
    Py_DECREF(value);

    // the object must still be reachable after the caller released its reference
    value = Base::Interpreter().getValue("", "_interpreter_test_value");
    ASSERT_NE(value, nullptr);
    Py_DECREF(value);
}

TEST_F(InterpreterTest, loadModule)  // NOLINT
{
    EXPECT_TRUE(Base::Interpreter().loadModule("math"));
    // A null name refers to the main module (historical PP_Load_Module contract)
    EXPECT_TRUE(Base::Interpreter().loadModule(nullptr));
    EXPECT_THROW(Base::Interpreter().loadModule("_freecad_missing_test_module_"), Base::PyException);
}

TEST_F(InterpreterTest, replaceStdOutputInstallsSilentStreams)  // NOLINT
{
    Base::Interpreter().runString(
        "import sys\n"
        "_original_stdout = sys.stdout\n"
        "_original_stderr = sys.stderr\n"
    );
    Base::Interpreter().replaceStdOutput();
    EXPECT_NO_THROW(Base::Interpreter().runString("print('discarded')"));
    Base::Interpreter().runString(
        "sys.stdout = _original_stdout\n"
        "sys.stderr = _original_stderr\n"
        "del _original_stdout, _original_stderr\n"
    );
}

TEST_F(InterpreterTest, getPythonPathReturnsSearchPath)  // NOLINT
{
    EXPECT_FALSE(Base::Interpreter().getPythonPath().empty());
}

TEST_F(InterpreterTest, configuredPythonExecutableExists)  // NOLINT
{
    // sys.executable must point to a real interpreter, never to the FreeCAD binary or a
    // nonexistent path derived from the libpython directory.
    const std::string executable = static_cast<std::string>(
        Py::String(Base::Interpreter().runStringObject("__import__('sys').executable"))
    );
    ASSERT_FALSE(executable.empty());
    std::error_code ec;
    EXPECT_TRUE(std::filesystem::exists(std::filesystem::path(executable), ec)) << executable;
}

TEST_F(InterpreterTest, strToPythonEscapesSpecialCharacters)  // NOLINT
{
    EXPECT_EQ(Base::InterpreterSingleton::strToPython(R"(a\b"c'd)"), R"(a\\b\"c\'d)");
}
