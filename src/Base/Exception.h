// SPDX-License-Identifier: LGPL-2.1-or-later

/***************************************************************************
 *   Copyright (c) 2002 Jürgen Riegel <juergen.riegel@web.de>              *
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

#pragma once

#include <FCConfig.h>

#include <csignal>
#include <source_location>
#include <string>

#include "BaseClass.h"
#include "FileInfo.h"

using PyObject = struct _object;  // NOLINT

// Remove once all used compilers support this
#if defined(__cpp_lib_source_location)
# define HAVE_STD_SOURCE_LOCATION 1
#else
# undef HAVE_STD_SOURCE_LOCATION
#endif
// std::source_location is implemented, but buggy in Clang 15
#if defined(__clang__) && __clang_major__ <= 15
# undef HAVE_STD_SOURCE_LOCATION
#endif

/// The macros do NOT mark any message for translation
/// If you want to mark text for translation, use the QT_TRANSLATE_NOOP macro
/// with the context "Exceptions" and the right throwing macro from below (the one ending with T)
/// example:
/// THROWMT(Base::ValueError,QT_TRANSLATE_NOOP("Exceptions","The multiplicity cannot be increased
/// beyond the degree of the B-Spline."));
///
/// N.B.: The QT_TRANSLATE_NOOP macro won't translate your string. It will just allow lupdate to
/// identify that string for translation so that if you ask for a translation (and the translator
/// have provided one) at that time it gets translated (e.g. in the UI before showing the message
/// of the exception).

#if defined(HAVE_STD_SOURCE_LOCATION)
// NOLINTBEGIN(*-macro-usage)
# define THROWM(exc, msg) Base::setupAndThrowException<exc>((msg), std::source_location::current());
# define THROWMT(exc, msg) \
     Base::setupAndThrowException<exc>((msg), std::source_location::current(), true);
# define FC_THROWM(exception, msg) \
     do { \
         std::stringstream ss; \
         ss << msg; \
         THROWM(exception, ss.str()); \
     } while (0)
// NOLINTEND(*-macro-usage)

namespace Base
{
template<typename ExceptionType>
[[noreturn]] void setupAndThrowException(
    const std::string message,
    const std::source_location location,
    const bool translatable = false
)
{
    ExceptionType exception {message};
    exception.setTranslatable(translatable);
    exception.setDebugInformation(location);
    throw exception;
}  // NOLINT // unreachable
}  // namespace Base

#else  // HAVE_STD_SOURCE_LOCATION

# ifdef _MSC_VER
#  define FC_THROW_INFO __FILE__, __LINE__, __FUNCSIG__
# elif __GNUC__
#  define FC_THROW_INFO __FILE__, __LINE__, __PRETTY_FUNCTION__
# else
#  define FC_THROW_INFO __FILE__, __LINE__, __func__
# endif

# define THROWM(exc, msg) Base::setupAndThrowException<exc>(msg, FC_THROW_INFO);
# define THROWMT(exc, msg) Base::setupAndThrowException<exc>(msg, FC_THROW_INFO, true);
# define FC_THROWM(exception, msg) \
     do { \
         std::stringstream ss; \
         ss << msg; \
         THROWM(exception, ss.str()); \
     } while (0)
namespace Base
{
template<typename ExceptionType>
[[noreturn]] void setupAndThrowException(
    const std::string message,
    const char* file,
    const int line,
    const char* func,
    const bool translatable = false
)
{
    ExceptionType exception {message};
    exception.setTranslatable(translatable);
    exception.setDebugInformation(file, line, func);
    throw exception;
}  // NOLINT // unreachable
}  // namespace Base

#endif  // HAVE_STD_SOURCE_LOCATION

//--------------------------------------------------------------------------------------------------

template<typename Exception>
constexpr void THROWM_(
    const std::string& msg,
    const std::source_location location = std::source_location::current()
)
{
    Base::setupAndThrowException<Exception>(msg, location);
}

template<typename Exception>
constexpr void THROWMT_(
    const std::string& msg,
    const std::source_location location = std::source_location::current()
)
{
    Base::setupAndThrowException<Exception>(msg, location, true);
}

template<typename Exception>
constexpr void FC_THROWM_(
    const std::string& raw_msg,
    const std::source_location location = std::source_location::current()
)
{
    THROWM_<Exception>(raw_msg, location);
}

namespace Base
{

class BaseExport Exception: public BaseClass
{
    TYPESYSTEM_HEADER_WITH_OVERRIDE();

public:
    explicit Exception(std::string message = "FreeCAD Exception");
    ~Exception() noexcept override = default;

    Exception& operator=(const Exception& inst);
    Exception& operator=(Exception&& inst) noexcept;

    virtual const char* what() const noexcept;
    virtual void reportException() const;  // once only

    inline void setMessage(const std::string& message);
    // what may differ from the message given by the user in
    // derived classes
    inline std::string getMessage() const;
    inline std::string getFile() const;
    inline int getLine() const;
    inline std::string getFunction() const;
    inline bool getTranslatable() const;
    inline bool getReported() const;
    inline void setReported(bool reported) const;

#if defined(HAVE_STD_SOURCE_LOCATION)
    inline void setDebugInformation(const std::source_location& location);
#else
    inline void setDebugInformation(const char* file, int line, const char* func);
#endif

    inline void setTranslatable(bool translatable);

    PyObject* getPyObject() override;             // exception data
    void setPyObject(PyObject* pydict) override;  // set the exception data

    virtual PyObject* getPyExceptionType() const;
    virtual void setPyException() const;

protected:
    Exception(const Exception& inst);
    Exception(Exception&& inst) noexcept;

private:
    std::string errorMessage;
    std::string fileName;
    int lineNum {0};
    std::string functionName;
    bool isTranslatable {false};
    mutable bool hasBeenReported {false};
};

class BaseExport AbortException: public Exception
{
    TYPESYSTEM_HEADER_WITH_OVERRIDE();

public:
    explicit AbortException(const std::string& message = "Aborted operation");

    const char* what() const noexcept override;
    PyObject* getPyExceptionType() const override;
};

class BaseExport XMLBaseException: public Exception
{
public:
    explicit XMLBaseException(const std::string& message = "XML base exception");

    PyObject* getPyExceptionType() const override;
};

class BaseExport XMLParseException: public XMLBaseException
{
public:
    explicit XMLParseException(const std::string& message = "XML parse exception");

    const char* what() const noexcept override;
    PyObject* getPyExceptionType() const override;
};

class BaseExport XMLAttributeError: public XMLBaseException
{
public:
    explicit XMLAttributeError(const std::string& message = "XML attribute error");

    const char* what() const noexcept override;
    PyObject* getPyExceptionType() const override;
};

class BaseExport FileException: public Exception
{
public:
    explicit FileException(
        const std::string& message = "Unknown file exception happened",
        const std::string& fileName = ""
    );
    FileException(const std::string& message, const FileInfo& File);

    const char* what() const noexcept override;
    void reportException() const override;
    std::string getFileName() const;
    PyObject* getPyObject() override;

    void setPyObject(PyObject* pydict) override;

    PyObject* getPyExceptionType() const override;

private:
    FileInfo file;
    // necessary   for what() legacy behaviour as it returns a buffer that
    // can not be of a temporary object to be destroyed at end of what()
    std::string _sErrMsgAndFileName;
    void setFileName(const std::string& fileName);
};

class BaseExport FileSystemError: public Exception
{
public:
    explicit FileSystemError(const std::string& message = "File system error");
    FileSystemError(const FileSystemError&) = default;
    FileSystemError(FileSystemError&&) = default;

    ~FileSystemError() noexcept override = default;
    FileSystemError& operator=(const FileSystemError&) = default;
    FileSystemError& operator=(FileSystemError&&) = default;

    PyObject* getPyExceptionType() const override;
};

/** errors in a data structure */
class BaseExport BadFormatError: public Exception
{
public:
    explicit BadFormatError(const std::string& message = "Bad format error");
    BadFormatError(const BadFormatError&) = default;
    BadFormatError(BadFormatError&&) = default;

    ~BadFormatError() noexcept override = default;
    BadFormatError& operator=(const BadFormatError&) = default;
    BadFormatError& operator=(BadFormatError&&) = default;
    PyObject* getPyExceptionType() const override;
};

#if defined(__GNUC__)
// calling instance of our new handler expects a bad_alloc exception
class BaseExport MemoryException: public Exception, virtual public std::bad_alloc
#else
class BaseExport MemoryException: public Exception
#endif
{
public:
    explicit MemoryException(const std::string& = "Not enough memory available");

#if defined(__GNUC__)
    const char* what() const noexcept override;
#endif

    PyObject* getPyExceptionType() const override;
};

/** can be used in an own signal handler */
class BaseExport AccessViolation: public Exception
{
public:
    explicit AccessViolation(const std::string& message = "Access violation");
    PyObject* getPyExceptionType() const override;
};

/** can be used in an own signal handler */
class BaseExport AbnormalProgramTermination: public Exception
{
public:
    explicit AbnormalProgramTermination(const std::string& message = "Abnormal program termination");
    PyObject* getPyExceptionType() const override;
};

class BaseExport UnknownProgramOption: public Exception
{
public:
    explicit UnknownProgramOption(const std::string& message = "Unknown program option");
    PyObject* getPyExceptionType() const override;
};

class BaseExport ProgramInformation: public Exception
{
public:
    explicit ProgramInformation(const std::string& message = "Program information");
};

class BaseExport TypeError: public Exception
{
public:
    explicit TypeError(const std::string& message = "Type error");
    PyObject* getPyExceptionType() const override;
};

class BaseExport ValueError: public Exception
{
public:
    explicit ValueError(const std::string& message = "Value error");
    PyObject* getPyExceptionType() const override;
};

/** sequence subscript is out of range */
class BaseExport IndexError: public Exception
{
public:
    explicit IndexError(const std::string& message = "Index error");
    PyObject* getPyExceptionType() const override;
};

class BaseExport NameError: public Exception
{
public:
    explicit NameError(const std::string& message = "Name error");
    PyObject* getPyExceptionType() const override;
};

class BaseExport ImportError: public Exception
{
public:
    explicit ImportError(const std::string& message = "Import error");
    PyObject* getPyExceptionType() const override;
};

class BaseExport AttributeError: public Exception
{
public:
    explicit AttributeError(const std::string& message = "Attribute error");
    PyObject* getPyExceptionType() const override;
};

class BaseExport PropertyError: public AttributeError
{
public:
    explicit PropertyError(const std::string& message = "Property error");
    PyObject* getPyExceptionType() const override;
};

class BaseExport RuntimeError: public Exception
{
public:
    explicit RuntimeError(const std::string& message = "Runtime error");
    PyObject* getPyExceptionType() const override;
};

class BaseExport BadGraphError: public RuntimeError
{
public:
    explicit BadGraphError(const std::string& message = "Bad graph error");
    PyObject* getPyExceptionType() const override;
};

class BaseExport NotImplementedError: public Exception
{
public:
    explicit NotImplementedError(const std::string& message = "Not implemented error");
    PyObject* getPyExceptionType() const override;
};

class BaseExport ZeroDivisionError: public Exception
{
public:
    explicit ZeroDivisionError(const std::string& message = "Zero division error");
    PyObject* getPyExceptionType() const override;
};

class BaseExport ReferenceError: public Exception
{
public:
    explicit ReferenceError(const std::string& message = "Reference error");
    PyObject* getPyExceptionType() const override;
};

class BaseExport ExpressionError: public Exception
{
public:
    explicit ExpressionError(const std::string& message = "Expression error");
    PyObject* getPyExceptionType() const override;
};

class BaseExport ParserError: public Exception
{
public:
    explicit ParserError(const std::string& message = "Parser error");
    PyObject* getPyExceptionType() const override;
};

class BaseExport UnicodeError: public Exception
{
public:
    explicit UnicodeError(const std::string& message = "Unicode error");
    PyObject* getPyExceptionType() const override;
};

class BaseExport OverflowError: public Exception
{
public:
    explicit OverflowError(const std::string& message = "Overflow error");
    PyObject* getPyExceptionType() const override;
};

class BaseExport UnderflowError: public Exception
{
public:
    explicit UnderflowError(const std::string& message = "Underflow error");
    PyObject* getPyExceptionType() const override;
};

class BaseExport UnitsMismatchError: public Exception
{
public:
    explicit UnitsMismatchError(const std::string& message = "Units mismatch error");
    PyObject* getPyExceptionType() const override;
};

class BaseExport CADKernelError: public Exception
{
public:
    explicit CADKernelError(const std::string& message = "CAD kernel error");
    PyObject* getPyExceptionType() const override;
};

class BaseExport RestoreError: public Exception
{
public:
    explicit RestoreError(const std::string& message = "Restore error");
    PyObject* getPyExceptionType() const override;
};

inline void Exception::setMessage(const std::string& message)
{
    errorMessage = message;
}

inline std::string Exception::getMessage() const
{
    return errorMessage;
}

inline std::string Exception::getFile() const
{
    return fileName;
}

inline int Exception::getLine() const
{
    return lineNum;
}

inline std::string Exception::getFunction() const
{
    return functionName;
}

inline bool Exception::getTranslatable() const
{
    return isTranslatable;
}

inline bool Exception::getReported() const
{
    return hasBeenReported;
}

inline void Exception::setReported(const bool reported) const
{
    hasBeenReported = reported;
}

#if defined(HAVE_STD_SOURCE_LOCATION)
inline void Exception::setDebugInformation(const std::source_location& location)
{
    fileName = location.file_name();
    lineNum = static_cast<int>(location.line());
    functionName = location.function_name();
}
#else
inline void Exception::setDebugInformation(const char* file, int line, const char* func)
{
    fileName = file;
    lineNum = line;
    functionName = func;
}
#endif

inline void Exception::setTranslatable(const bool translatable)
{
    isTranslatable = translatable;
}

#if defined(__GNUC__) && defined(FC_OS_LINUX)
class SignalException
{
public:
    SignalException();
    ~SignalException();

private:
    static void throw_signal(int signum);

    struct sigaction new_action {};  // NOLINT (keep struct)
    struct sigaction old_action {};  // NOLINT (keep struct)
    bool ok {false};
};
#endif

}  // namespace Base
