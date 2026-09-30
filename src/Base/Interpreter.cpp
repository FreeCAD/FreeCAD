// SPDX-License-Identifier: LGPL-2.1-or-later

/***************************************************************************
 *   Copyright (c) 2002 Jürgen Riegel <juergen.riegel@web.de>              *
 *   Copyright (c) 2026 Frank Martínez <mnesarco>                          *
 *                                                                         *
 *   This file is part of the FreeCAD CAx development system.              *
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU Library General Public License (LGPL)   *
 *   as published by the Free Software Foundation; either version 2 of     *
 *   the License, or (at your option) any later version.                   *
 *   for detail see the LICENCE text file.                                 *
 *                                                                         *
 *   FreeCAD is distributed in the hope that it will be useful,            *
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of        *
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the         *
 *   GNU Library General Public License for more details.                  *
 *                                                                         *
 *   You should have received a copy of the GNU Library General Public     *
 *   License along with FreeCAD; if not, write to the Free Software        *
 *   Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  *
 *   USA                                                                   *
 *                                                                         *
 ***************************************************************************/

#include <algorithm>
#include <array>
#include <cassert>
#include <cstdarg>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <filesystem>
#include <format>
#include <memory>
#include <mutex>
#include <optional>
#include <span>
#include <string>
#include <string_view>
#include <system_error>
#include <vector>

#include <FCConfig.h>

#include "Interpreter.h"
#include "Console.h"
#include "ExceptionFactory.h"
#include "FCGlobal.h"
#include "PyObjectBase.h"
#include "Stream.h"

#ifdef FC_OS_WIN32
# include <windows.h>
#else
# include <dlfcn.h>
#endif

#include <frameobject.h>

using namespace Base;
namespace fs = std::filesystem;
using namespace std::string_literals;

namespace
{

constexpr std::size_t pythonErrorTextSize = 2024;
using PythonErrorText = std::array<char, pythonErrorTextSize>;

/// Locate libpython.so/dll/dylib dynamic library path on disk based on dynamic linking.
fs::path getLibPythonDir()
{
#ifdef FC_OS_WIN32
    HMODULE hModule = NULL;
    GetModuleHandleExW(
        GET_MODULE_HANDLE_EX_FLAG_FROM_ADDRESS | GET_MODULE_HANDLE_EX_FLAG_UNCHANGED_REFCOUNT,
        reinterpret_cast<LPCWSTR>(&Py_Initialize),  // NOLINT
        &hModule
    );
    wchar_t path[MAX_PATH];
    GetModuleFileNameW(hModule, path, MAX_PATH);
    return fs::path(path).parent_path();
#else
    // NOLINTNEXTLINE
    auto initAddr = reinterpret_cast<void*>(&Py_Initialize);
    Dl_info info;
    if (dladdr(initAddr, &info)) {
        return fs::path(info.dli_fname).parent_path().lexically_normal();
    }
    throw Base::RuntimeError("Failed to configure python environment");
#endif
}

/// An interpreter candidate is a regular, executable file (symlinks are followed).
bool isExecutableFile(const fs::path& path)
{
    std::error_code ec;
    const fs::file_status status = fs::status(path, ec);
    if (ec || !fs::is_regular_file(status)) {
        return false;
    }
#ifdef FC_OS_WIN32
    return true;
#else
    constexpr auto exec_bits = fs::perms::owner_exec | fs::perms::group_exec | fs::perms::others_exec;
    return (status.permissions() & exec_bits) != fs::perms::none;
#endif
}

/// Locate the python interpreter belonging to the loaded libpython. All candidates are
/// derived from the libpython location and, on Unix, carry the exact version in their
/// name (pythonX.Y), so the result can never be an unrelated Python found on PATH.
/// Returns nullopt when no such interpreter exists so the caller can keep Python's
/// default instead of configuring a foreign or nonexistent executable.
std::optional<fs::path> getPythonExecutablePath()
{
    const fs::path base_path = getLibPythonDir();

#ifdef FC_OS_WIN32
    // The interpreter lives in the installation directory of the loaded pythonXY.dll
    // whose name already pins the version.
    const auto candidates = std::to_array({
        base_path / "python.exe",
        base_path.parent_path() / "python.exe",
        base_path / "DLLs" / "python.exe",
    });
#else
    const std::string versioned = std::format("python{}.{}", PY_MAJOR_VERSION, PY_MINOR_VERSION);
    std::vector<fs::path> candidates {
        base_path.parent_path() / "bin" / versioned,  // conda/venv, homebrew framework
        base_path / "bin" / versioned,                // python.org macOS framework
    };
    // Linux multiarch / lib64: /usr/lib[/<triplet>]/libpythonX.Y.so -> /usr/bin/pythonX.Y.
    const fs::path lib_dir = base_path.parent_path();
    if (lib_dir.filename() == "lib" || lib_dir.filename() == "lib64") {
        candidates.emplace_back(lib_dir.parent_path() / "bin" / versioned);
    }
#endif

    for (const auto& candidate : candidates) {
        if (isExecutableFile(candidate)) {
            return candidate;
        }
    }

    return std::nullopt;
}

void copyErrorText(std::span<char> dest, std::string_view text)
{
    const size_t count = std::min(text.size(), dest.size() - 1);
    // NOLINTNEXTLINE
    auto last = std::copy_n(text.data(), count, dest.data());
    *last = '\0';
}

void copyErrorText(std::span<char> dest, const char* text)
{
    if (text == nullptr) {
        dest[0] = '\0';
        return;
    }
    copyErrorText(dest, std::string_view(text));
}

/// RAII capture/restore of the Python error indicator.
class PyErrorGuard
{
public:
    PyErrorGuard() noexcept
    {
        PyErr_Fetch(&type, &value, &traceback);
    }

    ~PyErrorGuard()
    {
        PyErr_Restore(type, value, traceback);
    }

    FC_DISABLE_COPY_MOVE(PyErrorGuard)

    PyObject* exceptionValue() const noexcept
    {
        return value;
    }

private:
    PyObject* type {};
    PyObject* value {};
    PyObject* traceback {};
};

/// Fixed-capacity capture of the last Python error. The text buffers are fixed size and
/// allocation-free; the objects are owned PyCXX references.
///
/// The state is intentionally leaked at process exit (like the previous raw globals were) so
/// that no Python reference is released after the interpreter has been finalized.
struct PythonErrorState
{
    PythonErrorText errorType {};
    PythonErrorText errorInfo {};
    PythonErrorText stackTrace {};
    Py::Object exceptionType;
    Py::Object traceback;
    Py::Object errorDict;
};

PythonErrorState& pythonErrorState()
{
    static auto* state = new PythonErrorState();  // NOLINT(cppcoreguidelines-owning-memory)
    return *state;
}

/// Initialize the interpreter on first use through InterpreterSingleton::init().
/// Must be called before acquiring the GIL: PyGILState_Ensure() cannot bootstrap an
/// uninitialized runtime, and init() releases the GIL once Python is initialized.
void ensureInterpreterInitialized()
{
    if (!Py_IsInitialized()) {
        static std::string app_name = "FreeCAD";
        static std::array<char*, 1> argv {app_name.data()};
        InterpreterSingleton::Instance().init(static_cast<int>(argv.size()), argv.data());
    }
}

Py::Module getMainModule()
{
    // The interpreter must have been initialized through InterpreterSingleton::init()
    return Py::Module("__main__");  // not incref'd by the caller, owned by PyCXX
}

// Convert traceback to string and store
void readTracebackData(const Py::Object& errorTraceback, PythonErrorState& state)
{
    Py::Object traceStream(nullptr, true);
    if (!errorTraceback.isNull()) {
        Py::Object ioModule = Py::asObject(PyImport_ImportModule("io"));
        if (!ioModule.isNull()) {
            Py::Object stringIO = ioModule.getAttr("StringIO");
            if (!stringIO.isNull() && stringIO.isCallable()) {
                traceStream = Py::Callable(stringIO).apply();
            }
        }
    }

    bool traceOk = false;
    if (!traceStream.isNull() && PyTraceBack_Print(errorTraceback.ptr(), traceStream.ptr()) == 0) {
        Py::Object traceValue = traceStream.callMemberFunction("getvalue");
        if (traceValue.isString()) {
            copyErrorText(state.stackTrace, static_cast<std::string>(Py::String(traceValue)));
            traceOk = true;
        }
    }

    if (!traceOk) {
        if (PyFrameObject* frame = PyEval_GetFrame(); frame != nullptr) {
            const int line = PyFrame_GetLineNumber(frame);
            const Py::Object code = Py::asObject(
                reinterpret_cast<PyObject*>(PyFrame_GetCode(frame))
            );  // NOLINT
            const char* file = PyUnicode_AsUTF8(
                reinterpret_cast<PyCodeObject*>(code.ptr())->co_filename  // NOLINT
            );
            if (file != nullptr) {
                const auto pref = fs::path::preferred_separator + "src"s
                    + fs::path::preferred_separator;
                const char* src = strstr(file, pref.c_str());
                const auto result = std::format_to_n(
                    state.stackTrace.data(),
                    static_cast<long>(state.stackTrace.size()) - 1,
                    "{}({})",
                    src ? std::next(src, 5) : file,
                    line
                );
                *result.out = '\0';
            }
        }
    }
}

bool readErrorData(const Py::Object& errorData, PythonErrorState& state)
{
    bool hasErrorDict = false;
    if (errorData.isDict()) {
        // Prefer the 'swhat' entry of a FreeCAD exception dictionary
        const Py::Dict dict(errorData);
        bool hasWhat = false;
        if (dict.hasKey("swhat")) {
            if (const Py::Object value = dict.getItem("swhat"); value.isString()) {
                copyErrorText(state.errorInfo, static_cast<std::string>(Py::String(value)));
                hasWhat = true;
            }
        }
        if (!hasWhat) {
            copyErrorText(state.errorInfo, "<unknown exception data>");
        }
        hasErrorDict = true;
    }
    else {
        copyErrorText(state.errorInfo, "<unknown exception data>");
        if (!errorData.isNull()) {
            try {
                if (Py::String text = errorData.str(); text.isString()) {
                    copyErrorText(state.errorInfo, static_cast<std::string>(text));
                }
            }
            catch (const Py::BaseException&) {
                // Keep the fallback text and the error indicator set
            }
        }
    }
    return hasErrorDict;
}

void readErrorTypeData(const Py::Object& errorType, PythonErrorState& state)
{
    if (!errorType.isNull()) {
        try {
            if (const Py::String text = errorType.str(); text.isString()) {
                copyErrorText(state.errorType, static_cast<std::string>(text));
            }
        }
        catch (const Py::BaseException&) {
            // Keep empty value
        }
    }
}

/// Grab CPython error state.
/// The caller must hold the GIL.
void fetchPythonErrorState()
{
    PyObject* errobj = nullptr;
    PyObject* errdata = nullptr;
    PyObject* errtraceback = nullptr;
    PyErr_Fetch(&errobj, &errdata, &errtraceback);  // all 3 incref'd

    // PyCXX now owns the fetched references
    const Py::Object errorType = Py::asObject(errobj);
    const Py::Object errorData = Py::asObject(errdata);
    const Py::Object errorTraceback = Py::asObject(errtraceback);

    auto& state = pythonErrorState();
    state.errorType.fill('\0');
    state.errorInfo.fill('\0');
    state.stackTrace.fill('\0');

    readErrorTypeData(errorType, state);
    const bool hasErrorDict = readErrorData(errorData, state);
    readTracebackData(errorTraceback, state);

    // PyException keeps borrowing the exception type object to avoid
    // reference book-keeping in its copy constructor.
    state.exceptionType = errorType;
    state.traceback = errorTraceback;
    if (hasErrorDict) {
        state.errorDict = errorData;
    }
    else {
        state.errorDict = nullptr;
    }
}

}  // namespace

// ---------------------------------------------------------

bool Base::warnDeprecatedPythonApi(
    const char* apiKind,
    const char* qualifiedName,
    const PythonApiDeprecation& deprecation
)
{
    ensureInterpreterInitialized();
    PyGILStateLocker locker;
    assert(apiKind && *apiKind);
    assert(qualifiedName && *qualifiedName);
    assert(deprecation.deprecatedIn && *deprecation.deprecatedIn);
    assert(deprecation.removedIn && *deprecation.removedIn);

    std::string message = std::format(
        "{} '{}' is deprecated since FreeCAD {} and will be removed in FreeCAD {}",
        apiKind,
        qualifiedName,
        deprecation.deprecatedIn,
        deprecation.removedIn
    );
    const std::string_view replacement = deprecation.replacement ? deprecation.replacement : "";
    if (!replacement.empty()) {
        message += std::format("; use {} instead", replacement);
    }
    const std::string_view details = deprecation.details ? deprecation.details : "";
    if (!details.empty()) {
        message += std::format("; {}", details);
    }
    if (!std::string_view(".!?").contains(message.back())) {
        message += '.';
    }

    int warningResult = PyErr_WarnEx(PyExc_DeprecationWarning, message.c_str(), 1);
    return warningResult >= 0;
}

PyException::PyException(const Py::Object& obj)
{
    setMessage(obj.as_string());
    // _exceptionType stays a borrowed pointer because PyException has defaulted copy/move
    // operations; the (intentionally leaked) error state owns the type object instead.
    // NOLINTBEGIN
    const Py::Type type = obj.type();
    _exceptionType = type.ptr();
    _errorType = reinterpret_cast<PyTypeObject*>(type.ptr())->tp_name;
    pythonErrorState().exceptionType = static_cast<const Py::Object&>(type);
    // NOLINTEND
}

PyException::PyException()
{
    fetchPythonErrorState(); /* fetch (and clear) exception */

    auto& state = pythonErrorState();

    setPyObject(state.errorDict.ptr());

    std::string prefix = state.errorType.data(); /* exception name text */
    std::string error = state.errorInfo.data();  /* exception data text */

    setMessage(error);
    _errorType = prefix;

    // Keep the type object alive in the error state (intentionally leaked on exit):
    // _exceptionType is a borrowed pointer because PyException has defaulted copy/move
    // operations and Python references must not be released during unwinding.
    // NOLINTNEXTLINE
    _exceptionType = state.exceptionType.ptr();

    _stackTrace = state.stackTrace.data(); /* exception traceback text */

    // This should be done in the constructor because when doing
    // in the destructor it's not always clear when it is called
    // and thus may clear a Python exception when it should not.
    PyGILStateLocker locker;
    PyErr_Clear();  // must be called to keep Python interpreter in a valid state (Werner)
}

PyException::~PyException() noexcept = default;

void PyException::throwException()
{
    PyException myexcp;
    myexcp.reportException();
    myexcp.raiseException();
}

void PyException::raiseException()
{
    PyGILStateLocker locker;
    auto& state = pythonErrorState();
    if (state.errorDict.ptr() != nullptr) {
        // delete the Python dict upon destruction of edict
        Py::Dict edict(state.errorDict);
        state.errorDict = nullptr;

        if (_exceptionType == Base::PyExc_FC_FreeCADAbort) {
            edict.setItem("sclassname", Py::String(typeid(AbortException).name()));
        }
        if (getReported()) {
            edict.setItem("breported", Py::True());
        }
        Base::ExceptionFactory::Instance().raiseException(edict.ptr());
    }

    PyExceptionData data {
        .pyexc = _exceptionType,
        .message = getMessage(),
        .reported = getReported(),
    };
    Base::ExceptionFactory::Instance().raiseExceptionByType(data);

    // Fallback
    throw *this;
}

void PyException::reportException() const
{
    if (!getReported()) {
        setReported(true);
        // set sys.last_vars to make post-mortem debugging work
        PyGILStateLocker locker;
        auto& state = pythonErrorState();
        PySys_SetObject("last_traceback", state.traceback.ptr());
        Console().developerError("pyException", "{}{}: {}\n", _stackTrace, _errorType, what());
    }
}

void PyException::setPyException() const
{
    const std::string text = std::format("{}{}: {}", getStackTrace(), getErrorType(), what());
    PyErr_SetString(getPyExceptionType(), text.c_str());
}

// ---------------------------------------------------------

SystemExitException::SystemExitException()
{
    // Set exception message and code based upon the python sys.exit() code and/or message
    // based upon the following sys.exit() call semantics.
    //
    // Invocation       |  _exitCode  |  _sErrMsg
    // ---------------- +  ---------  +  --------
    // sys.exit(int#)   |   int#      |   "System Exit"
    // sys.exit(string) |   1         |   string
    // sys.exit()       |   1         |   "System Exit"

    long int errCode = 1;
    std::string errMsg = "System exit";

    PyGILStateLocker locker;
    PyObject* type = nullptr;
    PyObject* value = nullptr;
    PyObject* traceback = nullptr;
    PyErr_Fetch(&type, &value, &traceback);
    PyErr_NormalizeException(&type, &value, &traceback);

    Py::Object exceptionType = Py::asObject(type);
    Py::Object exceptionValue = Py::asObject(value);
    Py::Object exceptionTraceback = Py::asObject(traceback);

    if (!exceptionValue.isNull()) {
        if (Py::Object code = exceptionValue.getAttr("code"); !code.isNull()) {
            exceptionValue = code;
        }

        if (PyLong_Check(exceptionValue.ptr())) {
            errCode = Py::Long(exceptionValue).as_long();
        }
        else if (exceptionValue.isString()) {
            errMsg += std::format(": {}", exceptionValue.as_string());
        }
    }

    setMessage(errMsg);
    _exitCode = errCode;
}

// ---------------------------------------------------------

// Fixes #0000831: python print causes File descriptor error on windows
// NOLINTNEXTLINE
class PythonStdOutput: public Py::PythonClass<PythonStdOutput>
{
public:
    static void init_type();

    PythonStdOutput(Py::PythonClassInstance* self, Py::Tuple& args, Py::Dict& kwds)
        : Py::PythonClass<PythonStdOutput>(self, args, kwds)
    {}

    ~PythonStdOutput() override = default;

    Py::Object write(const Py::Tuple&)
    {
        return Py::None();
    }
    Py::Object flush(const Py::Tuple&)
    {
        return Py::None();
    }
};

PYCXX_VARARGS_METHOD_DECL(PythonStdOutput, write)
PYCXX_VARARGS_METHOD_DECL(PythonStdOutput, flush)

void PythonStdOutput::init_type()
{
    static std::once_flag once;
    std::call_once(once, [] {
        behaviors().name("PythonStdOutput");
        behaviors().doc("Python standard output");
        PYCXX_ADD_VARARGS_METHOD(write, write, "write()");
        PYCXX_ADD_VARARGS_METHOD(flush, flush, "flush()");
        behaviors().readyType();
    });
}

// ---------------------------------------------------------

InterpreterSingleton::InterpreterSingleton()
{
    this->_global = nullptr;
}

InterpreterSingleton::~InterpreterSingleton() = default;


std::string InterpreterSingleton::runString(const char* sCmd)
{
    ensureInterpreterInitialized();
    PyGILStateLocker locker;
    Py::Module module = getMainModule();  // get module
    if (module.isNull()) {
        throw PyException(); /* not incref'd */
    }
    Py::Dict dict = module.getDict(); /* get dict namespace */
    if (dict.isNull()) {
        throw PyException(); /* not incref'd */
    }

    Py::Object presult = Py::asObject(
        PyRun_String(sCmd, Py_file_input, dict.ptr(), dict.ptr())
    ); /* eval direct */
    if (presult.isNull()) {
        if (PyErr_ExceptionMatches(PyExc_SystemExit)) {
            throw SystemExitException();
        }

        PyException::throwException();
        return {};  // just to quieten code analyzers
    }

    try {
        return static_cast<std::string>(presult.repr());
    }
    catch (const Py::BaseException&) {
        PyErr_Clear();
        return {};
    }
}

/** runStringWithKey(psCmd, key, key_initial_value)
 * psCmd is python script to run
 * key is the name of a python string variable the script will have read/write
 * access to during script execution.  It will be our return value.
 * key_initial_value is the initial value c++ will set before calling the script
 * If the script runs successfully it will be able to change the value of key as
 * the return value, but if there is a runtime error key will not be changed even
 * if the error occurs after changing it inside the script.
 */

std::string InterpreterSingleton::runStringWithKey(
    const char* psCmd,
    const char* key,
    const char* key_initial_value
)
{
    ensureInterpreterInitialized();
    PyGILStateLocker locker;
    Py::Module module("__main__");
    if (module.isNull()) {
        throw PyException();
    }
    Py::Dict globalDictionary = module.getDict();
    Py::Dict localDictionary;
    Py::String initial_value(key_initial_value);
    localDictionary.setItem(key, initial_value);

    Py::Object presult = Py::asObject(
        PyRun_String(psCmd, Py_file_input, globalDictionary.ptr(), localDictionary.ptr())
    );
    if (presult.isNull()) {
        if (PyErr_ExceptionMatches(PyExc_SystemExit)) {
            throw SystemExitException();
        }

        PyException::throwException();
        return {};  // just to quieten code analyzers
    }

    Py::Object key_return_value = localDictionary.getItem(key);
    if (key_return_value.isNull()) {
        // getItem() sets a KeyError when the script removed the key instead of
        // updating it; clear it before reporting the contract violation.
        PyErr_Clear();
        throw RuntimeError(std::format("Python script did not return the key '{}'", key));
    }
    if (!key_return_value.isString()) {
        key_return_value = key_return_value.str();  // NOLINT
    }

    return static_cast<std::string>(Py::String(key_return_value).encode("utf-8"));
}

Py::Object InterpreterSingleton::runStringObject(const char* sCmd)
{
    ensureInterpreterInitialized();
    PyGILStateLocker locker;
    Py::Module module = getMainModule();  // get module
    if (module.isNull()) {
        throw PyException(); /* not incref'd */
    }
    Py::Dict dict = module.getDict(); /* get dict namespace */
    if (dict.isNull()) {
        throw PyException(); /* not incref'd */
    }

    Py::Object presult = Py::asObject(
        PyRun_String(sCmd, Py_eval_input, dict.ptr(), dict.ptr())
    ); /* eval direct */
    if (presult.isNull()) {
        if (PyErr_ExceptionMatches(PyExc_SystemExit)) {
            throw SystemExitException();
        }

        throw PyException();
    }

    return presult;
}

void InterpreterSingleton::systemExit()
{
    /* This code is taken from the original Python code */
    PyObject* exception {};
    PyObject* value {};
    PyObject* tb {};
    int exitcode = 0;

    PyErr_Fetch(&exception, &value, &tb);
    fflush(stdout);

    if (value != nullptr && value != Py_None) {
        if (PyExceptionInstance_Check(value)) {
            /* The error code should be in the `code' attribute. */
            PyObject* code = PyObject_GetAttrString(value, "code");
            if (code) {
                Py_DECREF(value);
                value = code;
            }
            /* If we failed to dig out the 'code' attribute,
               just let the else clause below print the error. */
        }
        if (value != nullptr && value != Py_None) {
            if (PyLong_Check(value)) {
                exitcode = static_cast<int>(PyLong_AsLong(value));
            }
            else {
                PyObject_Print(value, stderr, Py_PRINT_RAW);
                PySys_WriteStderr("\n");
                exitcode = 1;
            }
        }
    }

    /* Restore and clear the exception info, in order to properly decref
     * the exception, value, and traceback.  If we just exit instead,
     * these leak, which confuses PYTHONDUMPREFS output, and may prevent
     * some finalizers from running.
     */
    PyErr_Restore(exception, value, tb);
    PyErr_Clear();
    Py_Exit(exitcode);
    /* NOTREACHED */
}

void InterpreterSingleton::runInteractiveString(const char* sCmd)
{
    ensureInterpreterInitialized();
    PyGILStateLocker locker;
    Py::Module module = getMainModule();  // get module
    if (module.isNull()) {
        throw PyException(); /* not incref'd */
    }
    Py::Dict dict = module.getDict(); /* get dict namespace */
    if (dict.isNull()) {
        throw PyException(); /* not incref'd */
    }

    Py::Object presult = Py::asObject(
        PyRun_String(sCmd, Py_single_input, dict.ptr(), dict.ptr())
    ); /* eval direct */
    if (presult.isNull()) {
        if (PyErr_ExceptionMatches(PyExc_SystemExit)) {
            throw SystemExitException();
        }
        /* get latest python exception information */
        /* and print the error to the error output */
        RuntimeError exc("");  // do not use PyException since this clears the error indicator
        {
            // Restores the error indicator on scope exit, including on exceptions.
            const PyErrorGuard guard;
            // Since Python 3.12 PyErr_Fetch() always returns a normalized exception instance,
            // so the value must be stringified instead of assuming a raw string argument.
            if (PyObject* errdata = guard.exceptionValue(); errdata != nullptr) {
                Py::Object text = Py::asObject(PyObject_Str(errdata));
                if (!text.isNull() && text.isString()) {
                    exc.setMessage(static_cast<std::string>(Py::String(text)));
                }
            }
        }
        if (PyErr_Occurred()) {
            PyErr_Print();
        }
        throw exc;
    }
}

void InterpreterSingleton::runFile(const char* pxFileName, bool local)
{
#ifdef FC_OS_WIN32
    FileInfo fi(pxFileName);
    FILE* fp = _wfopen(fi.toStdWString().c_str(), L"r");
#else
    FILE* fp = fopen(pxFileName, "r");
#endif
    if (!fp) {
        throw FileException("Unknown file", pxFileName);
    }
    const std::unique_ptr<FILE, decltype(&fclose)> file(fp, &fclose);

    ensureInterpreterInitialized();
    PyGILStateLocker locker;
    Py::Module module("__main__");
    if (module.isNull()) {
        throw PyException();
    }
    Py::Dict dict = module.getDict();
    if (local) {
        dict = Py::asObject(PyDict_Copy(dict.ptr()));
    }

    if (!dict.hasKey("__file__")) {
        try {
            dict.setItem("__file__", Py::String(pxFileName));
        }
        catch (const Py::BaseException&) {
            // Keep the error indicator set and give up, as the previous implementation did
            return;
        }
    }

    Py::Object result = Py::asObject(
        PyRun_File(file.get(), pxFileName, Py_file_input, dict.ptr(), dict.ptr())
    );

    if (result.isNull()) {
        if (PyErr_ExceptionMatches(PyExc_SystemExit)) {
            throw SystemExitException();
        }
        throw PyException();
    }
}

bool InterpreterSingleton::loadModule(const char* psModName)
{
    ensureInterpreterInitialized();
    PyGILStateLocker locker;
    // Keep the historical contract of the removed PP_Load_Module: a null module name
    // refers to the main module.
    Py::Object module = Py::asObject(PyImport_ImportModule(psModName ? psModName : "__main__"));

    if (module.isNull()) {
        if (PyErr_ExceptionMatches(PyExc_SystemExit)) {
            throw SystemExitException();
        }

        throw PyException();
    }

    // the module is kept alive by sys.modules
    return true;
}

PyObject* InterpreterSingleton::addModule(Py::ExtensionModuleBase* mod)
{
    _modules.push_back(mod);
    return mod->module().ptr();
}

void InterpreterSingleton::cleanupModules()
{
    // This is only needed to make the address sanitizer happy
#if defined(__has_feature)
# if __has_feature(address_sanitizer)
    for (auto it : _modules) {
        delete it;
    }
    _modules.clear();
# endif
#endif
}

void InterpreterSingleton::addType(PyTypeObject* Type, PyObject* Module, const char* Name)
{
    // NOTE: To finish the initialization of our own type objects we must
    // call PyType_Ready, otherwise we run into a segmentation fault, later on.
    // This function is responsible for adding inherited slots from a type's base class.
    if (PyType_Ready(Type) < 0) {
        return;
    }
    PyModule_AddObject(Module, Name, Base::getTypeAsObject(Type));
}

void InterpreterSingleton::addPythonPath(const char* Path)
{
    ensureInterpreterInitialized();
    PyGILStateLocker locker;
    Py::List list(PySys_GetObject("path"));
    list.append(Py::String(Path));
}

std::string InterpreterSingleton::getPythonPath()
{
    ensureInterpreterInitialized();
    // Construct something that looks like the output of the now-deprecated Py_GetPath
    PyGILStateLocker lock;
    PyObject* pathObject = PySys_GetObject("path");
    if (pathObject == nullptr) {
        throw Base::RuntimeError("Failed to retrieve sys.path");
    }
    Py::List path(pathObject);

#ifdef FC_OS_WIN32
    constexpr std::string_view separator = ";";
#else
    constexpr std::string_view separator = ":";
#endif

    std::string result;
    for (Py_ssize_t i = 0; i < path.size(); ++i) {
        Py::Object item = path[i];
        if (item.isNull()) {
            throw Base::RuntimeError("Failed to retrieve item from path");
        }
        if (!item.isString()) {
            throw Base::RuntimeError("Failed to convert path item to UTF-8 string");
        }
        if (!result.empty()) {
            result += separator;
        }
        result += item.as_string();
    }
    return result;
}

namespace
{
void initInterpreter(int argc, char* argv[])
{
    PyConfig config;
    PyConfig_InitIsolatedConfig(&config);
    config.isolated = 0;
    config.user_site_directory = 1;
    const std::unique_ptr<PyConfig, decltype(&PyConfig_Clear)> configGuard(&config, &PyConfig_Clear);

    PyStatus status = PyConfig_SetBytesArgv(&config, argc, argv);
    if (PyStatus_Exception(status)) {
        throw Base::RuntimeError("Failed to set config");
    }

    if (const auto python_exe = getPythonExecutablePath()) {
        // Only sys.executable is overridden, keep program_name.
        const std::wstring python_exe_wide = python_exe->wstring();
        status = PyConfig_SetString(&config, &config.executable, python_exe_wide.c_str());
        if (PyStatus_Exception(status)) {
            throw Base::RuntimeError("Failed to set config");
        }
    }

    status = Py_InitializeFromConfig(&config);
    if (PyStatus_Exception(status)) {
        throw Base::RuntimeError("Failed to init from config");
    }

    // If FreeCAD was run from within a Python virtual environment, ensure that the site-packages
    // directory from that environment is used.
    const char* virtualenv = std::getenv("VIRTUAL_ENV");
    if (virtualenv) {
        PyConfig_Read(&config);
        const fs::path sitePackages = fs::path(virtualenv) / "lib"
            / std::format("python{}.{}", PY_MAJOR_VERSION, PY_MINOR_VERSION) / "site-packages";
        const std::wstring location = sitePackages.wstring();
        Py::Object venvLocation = Py::asObject(
            PyUnicode_FromWideChar(location.c_str(), static_cast<Py_ssize_t>(location.size()))
        );
        PyObject* path = PySys_GetObject("path");
        if (venvLocation.isNull() || path == nullptr) {
            throw Base::RuntimeError("Failed to configure virtual environment");
        }
        Py::List(path).append(venvLocation);
    }
}
}  // namespace

std::string InterpreterSingleton::init(int argc, char* argv[])
{
    try {
        if (!Py_IsInitialized()) {
            initInterpreter(argc, argv);
            PythonStdOutput::init_type();
            this->_global = PyEval_SaveThread();
        }
        return getPythonPath();
    }
    catch (const Exception& e) {
        e.reportException();
        throw;
    }
}

void InterpreterSingleton::replaceStdOutput()
{
    ensureInterpreterInitialized();
    PyGILStateLocker locker;
    PythonStdOutput::init_type();
    Py::Object out = Py::Callable(PythonStdOutput::type()).apply();
    if (out.isNull()) {
        return;
    }
    PySys_SetObject("stdout", out.ptr());
    PySys_SetObject("stderr", out.ptr());
}

int InterpreterSingleton::cleanup(void (*func)())
{
    return Py_AtExit(func);
}

void InterpreterSingleton::finalize()
{
    if (!Py_IsInitialized() || this->_global == nullptr) {
        // Never initialized through init() or already finalized
        return;
    }
    try {
        PyEval_RestoreThread(this->_global);
        this->_global = nullptr;
        cleanupModules();
        Py_Finalize();
    }
    catch (...) {
    }
}

void InterpreterSingleton::runStringArg(const char* psCom, ...)
{
    // va stuff
    va_list namelessVars;
    va_start(namelessVars, psCom);  // Get the "..." vars

    va_list argsCopy;
    va_copy(argsCopy, namelessVars);
    const int len = vsnprintf(nullptr, 0, psCom, namelessVars);
    va_end(namelessVars);

    if (len < 0) {
        va_end(argsCopy);
        throw Base::RuntimeError("Failed to format the Python command");
    }

    std::string command(static_cast<std::size_t>(len) + 1, '\0');
    vsnprintf(command.data(), command.size(), psCom, argsCopy);
    va_end(argsCopy);
    command.resize(static_cast<std::size_t>(len));

    runString(command.c_str());
}


// Singleton:

std::unique_ptr<InterpreterSingleton> InterpreterSingleton::_pcSingleton;

InterpreterSingleton& InterpreterSingleton::Instance()
{
    // not initialized!
    if (!_pcSingleton) {
        _pcSingleton = std::make_unique<InterpreterSingleton>();
    }
    return *_pcSingleton;
}

void InterpreterSingleton::Destruct()
{
    // not initialized or double destruct!
    assert(_pcSingleton);
    _pcSingleton.reset();
}

int InterpreterSingleton::runCommandLine(const char* prompt)
{
    ensureInterpreterInitialized();
    PyGILStateLocker locker;
    if (prompt != nullptr) {
        const char* hint = "Use Ctrl-D (i.e. EOF) to exit.";
#ifdef FC_OS_WIN32
        hint = "Use Ctrl-Z plus Return to exit.";
#endif
        std::fputs(std::format("[{} <{}>]\n", prompt, hint).c_str(), stdout);
    }
    return PyRun_InteractiveLoop(stdin, "<stdin>");
}

/**
 *  Runs a member method of an object with no parameter and no return value
 *  void (void). There are other methods to run with returns
 */
void InterpreterSingleton::runMethodVoid(PyObject* pobject, const char* method)
{
    PyGILStateLocker locker;
    Py::Object presult;
    try {
        presult = Py::Object(pobject).callMemberFunction(method);
    }
    catch (const Py::BaseException&) {
        throw PyException(/*"Error running InterpreterSingleton::RunMethodVoid()"*/);
    }
    if (presult.isNull()) {
        throw PyException(/*"Error running InterpreterSingleton::RunMethodVoid()"*/);
    }
}

PyObject* InterpreterSingleton::runMethodObject(PyObject* pobject, const char* method)
{
    PyGILStateLocker locker;
    Py::Object presult;
    try {
        presult = Py::Object(pobject).callMemberFunction(method);
    }
    catch (const Py::BaseException&) {
        throw PyException();
    }
    if (presult.isNull()) {
        throw PyException();
    }

    return new_reference_to(presult);
}

namespace
{
/// Convert a Python result into the requested C target, preserving the legacy ownership
/// contract: format "O" hands the reference over to the caller, format "s" hands over a
/// freshly allocated string the caller must free().
int convertPythonResult(Py::Object& presult, const char* resFormat, void* resTarget)
{
    if (presult.isNull()) {  // error when run: fail
        return -1;
    }
    if (resTarget == nullptr) {  // passed target=NULL: ignore result
        return 0;
    }
    if (!PyArg_Parse(presult.ptr(), resFormat, resTarget)) {  // convert Python->C
        return -1;                                            // error in convert
    }
    const std::string_view format = resFormat ? std::string_view(resFormat) : std::string_view {};
    if (format == "O") {  // transfer the reference to the caller
        presult.increment_reference_count();
    }
    else if (format == "s") {  // copy string: caller owns it
        char** target = static_cast<char**>(resTarget);
#ifdef _MSC_VER
        *target = _strdup(*target);
#else
        *target = strdup(*target);
#endif
    }
    return 0;  // returns 0=success, -1=failure
}  // caller must decref if fmt="O"
   // caller must free() if fmt="s"
}  // namespace

void InterpreterSingleton::runMethod(
    PyObject* pobject,
    const char* method,
    const char* resfmt,
    void* cresult, /* convert to c/c++ */
    const char* argfmt,
    ...
) /* convert to python */
{
    va_list argslist; /* "pobject.method(args)" */
    va_start(argslist, argfmt);

    PyGILStateLocker locker;
    Py::Object pmeth = Py::Object(pobject).getAttr(method);
    if (pmeth.isNull()) { /* get callable object */
        va_end(argslist);
        throw AttributeError(
            "Error running InterpreterSingleton::RunMethod() method not defined"
        ); /* bound method?
              has self */
    }

    Py::Object pargs = Py::asObject(Py_VaBuildValue(argfmt, argslist)); /* args: c->python */
    va_end(argslist);

    if (pargs.isNull()) {
        throw TypeError("InterpreterSingleton::RunMethod() wrong arguments");
    }

    Py::Object presult = Py::asObject(PyObject_CallObject(pmeth.ptr(), pargs.ptr())); /* run */
    if (convertPythonResult(presult, resfmt, cresult) != 0) {
        if (PyErr_Occurred()) {
            PyErr_Print();
        }
        throw RuntimeError(
            "Error running InterpreterSingleton::RunMethod() exception in called method"
        );
    }
}

PyObject* InterpreterSingleton::getValue(const char* key, const char* result_var)
{
    ensureInterpreterInitialized();
    PyGILStateLocker locker;
    Py::Module module = getMainModule();  // get module
    if (module.isNull()) {
        throw PyException(); /* not incref'd */
    }
    Py::Dict dict = module.getDict(); /* get dict namespace */
    if (dict.isNull()) {
        throw PyException(); /* not incref'd */
    }

    Py::Object presult = Py::asObject(
        PyRun_String(key, Py_file_input, dict.ptr(), dict.ptr())
    ); /* eval direct */
    if (presult.isNull()) {
        throw PyException();
    }

    return new_reference_to(module.getAttr(result_var));
}

void InterpreterSingleton::dbgObserveFile(const char* sFileName)
{
    if (sFileName) {
        _cDebugFileName = sFileName;
    }
    else {
        _cDebugFileName = "";
    }
}


std::string InterpreterSingleton::strToPython(const char* Str)
{
    const std::string_view input = Str ? Str : "";
    std::string result;
    result.reserve(input.size());

    for (const char c : input) {
        switch (c) {
            case '\\':
                result += "\\\\";
                break;
            case '\"':
                result += "\\\"";
                break;
            case '\'':
                result += "\\\'";
                break;
            default:
                result += c;
        }
    }

    return result;
}

#if (defined(HAVE_SWIG) && (HAVE_SWIG == 1))
namespace Swig_python
{
extern int createSWIGPointerObj_T(const char* TypeName, void* obj, PyObject** ptr, int own);
extern int convertSWIGPointerObj_T(const char* TypeName, PyObject* obj, void** ptr, int flags);
extern void cleanupSWIG_T(const char* TypeName);
extern int getSWIGPointerTypeObj_T(const char* TypeName, PyTypeObject** ptr);
}  // namespace Swig_python
#endif

PyObject* InterpreterSingleton::createSWIGPointerObj(
    const char* Module,
    const char* TypeName,
    void* Pointer,
    int own
)
{
    int result = 0;
    PyObject* proxy = nullptr;
    ensureInterpreterInitialized();
    PyGILStateLocker locker;
    (void)Module;
#if (defined(HAVE_SWIG) && (HAVE_SWIG == 1))
    result = Swig_python::createSWIGPointerObj_T(TypeName, Pointer, &proxy, own);
#else
    (void)TypeName;
    (void)Pointer;
    (void)own;
    result = -1;  // indicates error
#endif

    if (result == 0) {
        return proxy;
    }

    // none of the SWIG's succeeded
    throw Base::RuntimeError("No SWIG wrapped library loaded");
}

bool InterpreterSingleton::convertSWIGPointerObj(
    const char* Module,
    const char* TypeName,
    PyObject* obj,
    void** ptr,
    int flags
)
{
    int result = 0;
    ensureInterpreterInitialized();
    PyGILStateLocker locker;
    (void)Module;
#if (defined(HAVE_SWIG) && (HAVE_SWIG == 1))
    result = Swig_python::convertSWIGPointerObj_T(TypeName, obj, ptr, flags);
#else
    (void)TypeName;
    (void)obj;
    (void)ptr;
    (void)flags;
    result = -1;  // indicates error
#endif

    if (result == 0) {
        return true;
    }

    // none of the SWIG's succeeded
    throw Base::RuntimeError("No SWIG wrapped library loaded");
}

void InterpreterSingleton::cleanupSWIG(const char* TypeName)
{
    ensureInterpreterInitialized();
    PyGILStateLocker locker;
#if (defined(HAVE_SWIG) && (HAVE_SWIG == 1))
    Swig_python::cleanupSWIG_T(TypeName);
#else
    (void)TypeName;
#endif
}

PyTypeObject* InterpreterSingleton::getSWIGPointerTypeObj(const char* Module, const char* TypeName)
{
    int result = 0;
    PyTypeObject* proxy = nullptr;
    ensureInterpreterInitialized();
    PyGILStateLocker locker;
    (void)Module;
#if (defined(HAVE_SWIG) && (HAVE_SWIG == 1))
    result = Swig_python::getSWIGPointerTypeObj_T(TypeName, &proxy);
#else
    (void)TypeName;
    result = -1;  // indicates error
#endif

    if (result == 0) {
        return proxy;
    }

    // none of the SWIG's succeeded
    throw Base::RuntimeError("No SWIG wrapped library loaded");
}
