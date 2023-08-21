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


#ifndef BASE_EXCEPTION_H
#define BASE_EXCEPTION_H

#include <csignal>
#include <string>
#include "BaseClass.h"
#include "FileInfo.h"


using PyObject = struct _object;

/* MACROS FOR THROWING EXCEPTIONS */

/// the macros do NOT mark any message for translation
/// If you want to mark text for translation, use the QT_TRANSLATE_NOOP macro
/// with the context "Exceptions" and the right throwing macro from below (the one ending in T)
/// example:
/// THROWMT(Base::ValueError,QT_TRANSLATE_NOOP("Exceptions","The multiplicity cannot be increased beyond the degree of the B-Spline."));
///
/// N.B.: The QT_TRANSLATE_NOOP macro won't translate your string. It will just allow lupdate to identify that string for translation so that
/// if you ask for a translation (and the translator have provided one) at that time it gets translated (e.g. in the UI before showing the message
/// of the exception).

#ifdef _MSC_VER

# define THROW(exception) {exception myexcp; myexcp.setDebugInformation(__FILE__,__LINE__,__FUNCSIG__); throw myexcp;}
# define THROWM(exception, message) {exception myexcp(message); myexcp.setDebugInformation(__FILE__,__LINE__,__FUNCSIG__); throw myexcp;}
# define THROWMF_FILEEXCEPTION(message,filenameorfileinfo) {FileException myexcp(message, filenameorfileinfo); myexcp.setDebugInformation(__FILE__,__LINE__,__FUNCSIG__); throw myexcp;}

# define THROWT(exception) {exception myexcp; myexcp.setDebugInformation(__FILE__,__LINE__,__FUNCSIG__); myexcp.setTranslatable(true); throw myexcp;}
# define THROWMT(exception, message) {exception myexcp(message); myexcp.setDebugInformation(__FILE__,__LINE__,__FUNCSIG__); myexcp.setTranslatable(true); throw myexcp;}
# define THROWMFT_FILEEXCEPTION(message,filenameorfileinfo) {FileException myexcp(message, filenameorfileinfo); myexcp.setDebugInformation(__FILE__,__LINE__,__FUNCSIG__); myexcp.setTranslatable(true); throw myexcp;}

#elif defined(__GNUC__)

# define THROW(exception) {exception myexcp; myexcp.setDebugInformation(__FILE__,__LINE__,__PRETTY_FUNCTION__); throw myexcp;}
# define THROWM(exception, message) {exception myexcp(message); myexcp.setDebugInformation(__FILE__,__LINE__,__PRETTY_FUNCTION__); throw myexcp;}
# define THROWMF_FILEEXCEPTION(message,filenameorfileinfo) {FileException myexcp(message, filenameorfileinfo); myexcp.setDebugInformation(__FILE__,__LINE__,__PRETTY_FUNCTION__); throw myexcp;}

# define THROWT(exception) {exception myexcp; myexcp.setDebugInformation(__FILE__,__LINE__,__PRETTY_FUNCTION__); myexcp.setTranslatable(true); throw myexcp;}
# define THROWMT(exception, message) {exception myexcp(message); myexcp.setDebugInformation(__FILE__,__LINE__,__PRETTY_FUNCTION__); myexcp.setTranslatable(true); throw myexcp;}
# define THROWMFT_FILEEXCEPTION(message,filenameorfileinfo) {FileException myexcp(message, filenameorfileinfo); myexcp.setDebugInformation(__FILE__,__LINE__,__PRETTY_FUNCTION__); myexcp.setTranslatable(true); throw myexcp;}

#else

# define THROW(exception) {exception myexcp; myexcp.setDebugInformation(__FILE__,__LINE__,__func__); throw myexcp;}
# define THROWM(exception, message) {exception myexcp(message); myexcp.setDebugInformation(__FILE__,__LINE__,__func__); throw myexcp;}
# define THROWMF_FILEEXCEPTION(message,filenameorfileinfo) {FileException myexcp(message, filenameorfileinfo); myexcp.setDebugInformation(__FILE__,__LINE__,__func__);  throw myexcp;}

# define THROWT(exception) {exception myexcp; myexcp.setDebugInformation(__FILE__,__LINE__,__func__); myexcp.setTranslatable(true); throw myexcp;}
# define THROWMT(exception, message) {exception myexcp(message); myexcp.setDebugInformation(__FILE__,__LINE__,__func__); myexcp.setTranslatable(true); throw myexcp;}
# define THROWMFT_FILEEXCEPTION(message,filenameorfileinfo) {FileException myexcp(message, filenameorfileinfo); myexcp.setDebugInformation(__FILE__,__LINE__,__func__); myexcp.setTranslatable(true); throw myexcp;}


#endif

#define FC_THROWM(_exception,_msg) do {\
    std::stringstream ss;\
    ss << _msg;\
    THROWM(_exception,ss.str().c_str());\
}while(0)

namespace Base
{

class BaseExport Exception : public BaseClass
{
  TYPESYSTEM_HEADER_WITH_OVERRIDE();
public:

  ~Exception() noexcept override = default;

  Exception &operator=(const Exception &inst);

  virtual const char* what() const noexcept;

  /// Reports exception. It includes a mechanism to only report an exception once.
  virtual void ReportException () const;

  inline void setMessage(const char * sMessage);
  inline void setMessage(const std::string& sMessage);
  // what may differ from the message given by the user in
  // derived classes
  inline std::string getMessage() const;
  inline std::string getFile() const;
  inline int getLine() const;
  inline std::string getFunction() const;
  inline bool getTranslatable() const;
  inline bool getReported() const { return _isReported; }

  /// setter methods for including debug information
  /// intended to use via macro for autofilling of debugging information
  inline void setDebugInformation(const std::string & file, const int line, const std::string & function);

  inline void setTranslatable(bool translatable);

  inline void setReported(bool reported) { _isReported = reported; }

  /// returns a Python dictionary containing the exception data
  PyObject * getPyObject() override;
  /// returns sets the exception data from a Python dictionary
  void setPyObject( PyObject * pydict) override;

  /// returns the corresponding python exception type
  virtual PyObject * getPyExceptionType() const;
  /// Sets the Python error indicator and an error message
  virtual void setPyException() const;

protected:
 /* sMessage may be:
  * - a UI compliant string susceptible to being translated and shown to the user in the UI
  * - a very technical message not intended to be translated or shown to the user in the UI
  * The preferred way of throwing an exception is using the macros above.
  * This way, the file, line, and function are automatically inserted. */
  Exception(const char * sMessage);
  Exception(const std::string& sMessage);
  Exception();
  Exception(const Exception &inst);

protected:
  std::string _sErrMsg;
  std::string _file;
  int _line;
  std::string _function;
  bool _isTranslatable;
  mutable bool _isReported;
};


/**
 * The AbortException is thrown if a pending operation was aborted.
 * @author Werner Mayer
 */
class BaseExport AbortException : public Exception
{
  TYPESYSTEM_HEADER_WITH_OVERRIDE();
public:
  /// Construction
  AbortException(const char * sMessage);
  /// Construction
  AbortException();

  /// Destruction
  ~AbortException() noexcept override = default;
  /// Description of the exception
  const char* what() const noexcept override;
  /// returns the corresponding python exception type
  PyObject * getPyExceptionType() const override;
};

/**
 * The XMLBaseException can be used to indicate any kind of XML related errors.
 * @author Werner Mayer
 */
class BaseExport XMLBaseException : public Exception
{
public:
  /// Construction
  XMLBaseException();
  XMLBaseException(const char * sMessage);
  XMLBaseException(const std::string& sMessage);

  /// Destruction
  ~XMLBaseException() noexcept override = default;
  PyObject * getPyExceptionType() const override;
};

/**
 * The XMLParseException is thrown if parsing an XML failed.
 * @author Werner Mayer
 */
class BaseExport XMLParseException : public XMLBaseException
{
public:
  /// Construction
  XMLParseException(const char * sMessage);
  /// Construction
  XMLParseException(const std::string& sMessage);
  /// Construction
  XMLParseException();

  /// Destruction
  ~XMLParseException() noexcept override = default;
  /// Description of the exception
  const char* what() const noexcept override;
  PyObject * getPyExceptionType() const override;
};

/**
 * The XMLAttributeError is thrown if a requested attribute doesn't exist.
 * @author Werner Mayer
 */
class BaseExport XMLAttributeError : public XMLBaseException
{
public:
  /// Construction
  XMLAttributeError(const char * sMessage);
  /// Construction
  XMLAttributeError(const std::string& sMessage);
  /// Construction
  XMLAttributeError();

  /// Destruction
  ~XMLAttributeError() noexcept override = default;
  /// Description of the exception
  const char* what() const noexcept override;
  PyObject * getPyExceptionType() const override;
};

/** File exception handling class
 * This class is specialized to go with exception thrown in case of File IO Problems.
 * @author Juergen Riegel
 */
class BaseExport FileException : public Exception
{
public:
  /// With massage and file name
  FileException(const char * sMessage, const char * sFileName=nullptr);
  /// With massage and file name
  FileException(const char * sMessage, const FileInfo& File);
  /// standard construction
  FileException();
  /// Construction
  FileException(const FileException&) = default;
  /// Destruction
  ~FileException() noexcept override = default;
  /// Assignment operator
  FileException &operator=(const FileException &inst);
  /// Description of the exception
  const char* what() const noexcept override;
  /// Report generation
  void ReportException () const override;
  /// Get file name for use with translatable message
  std::string getFileName() const;
  /// returns a Python dictionary containing the exception data
  PyObject * getPyObject() override;
  /// returns sets the exception data from a Python dictionary
  void setPyObject( PyObject * pydict) override;

  PyObject * getPyExceptionType() const override;
protected:
  FileInfo file;
  // necessary   for what() legacy behaviour as it returns a buffer that
  // can not be of a temporary object to be destroyed at end of what()
  std::string _sErrMsgAndFileName;
  void setFileName(const char * sFileName=nullptr);
};

/**
 * The FileSystemError can be used to indicate errors on file system
 * e.g. if renaming of a file failed.
 * @author Werner Mayer
 */
class BaseExport FileSystemError : public Exception
{
public:
  /// Construction
  FileSystemError();
  FileSystemError(const char * sMessage);
  FileSystemError(const std::string& sMessage);
  /// Destruction
  ~FileSystemError() noexcept override = default;
  PyObject * getPyExceptionType() const override;
};

/**
 * The BadFormatError can be used to indicate errors in a data structure.
 * @author Werner Mayer
 */
class BaseExport BadFormatError : public Exception
{
public:
  /// Construction
  BadFormatError();
  BadFormatError(const char * sMessage);
  BadFormatError(const std::string& sMessage);
  /// Destruction
  ~BadFormatError() noexcept override = default;
  PyObject * getPyExceptionType() const override;
};

/**
 * The MemoryException is thrown if not enough memory can be allocated.
 * @author Werner Mayer
 */
#if defined (__GNUC__)
// It seems that the calling instance of our new handler expects a bad_alloc exception
class BaseExport MemoryException : public Exception, virtual public std::bad_alloc
#else
class BaseExport MemoryException : public Exception
#endif
{
public:
  /// Construction
  MemoryException();
  /// Construction
  MemoryException(const MemoryException &inst);
  /// Destruction
  ~MemoryException() noexcept override = default;
  /// Assignment operator
  MemoryException &operator=(const MemoryException &inst);
#if defined (__GNUC__)
  /// Description of the exception
  const char* what() const noexcept override;
#endif
  PyObject * getPyExceptionType() const override;
};

/**
 * The AccessViolation can be used in an own signal handler.
 * @author Werner Mayer
 */
class BaseExport AccessViolation : public Exception
{
public:
  /// Construction
  AccessViolation();
  AccessViolation(const char * sMessage);
  AccessViolation(const std::string& sMessage);
  /// Destruction
  ~AccessViolation() noexcept override = default;
  PyObject * getPyExceptionType() const override;
};

/**
 * The AbnormalProgramTermination can be used in an own signal handler.
 * @author Werner Mayer
 */
class BaseExport AbnormalProgramTermination : public Exception
{
public:
  /// Construction
  AbnormalProgramTermination();
  /// Construction
  AbnormalProgramTermination(const char * sMessage);
  AbnormalProgramTermination(const std::string& sMessage);
  /// Destruction
  ~AbnormalProgramTermination() noexcept override = default;
  PyObject * getPyExceptionType() const override;
};

/**
 * The UnknownProgramOption can be used to indicate an unknown program option.
 * @author Werner Mayer
 */
class BaseExport UnknownProgramOption : public Exception
{
public:
  /// Construction
  UnknownProgramOption();
  UnknownProgramOption(const char * sMessage);
  UnknownProgramOption(const std::string& sMessage);
  /// Destruction
  ~UnknownProgramOption() noexcept override = default;
  PyObject * getPyExceptionType() const override;
};

/**
 * The ProgramInformation can be used to show information about the program.
 * @author Werner Mayer
 */
class BaseExport ProgramInformation : public Exception
{
public:
  /// Construction
  ProgramInformation();
  ProgramInformation(const char * sMessage);
  ProgramInformation(const std::string& sMessage);

  /// Destruction
  ~ProgramInformation() noexcept override = default;
};

/**
 * The TypeError can be used to indicate the usage of a wrong type.
 * @author Werner Mayer
 */
class BaseExport TypeError : public Exception
{
public:
  /// Construction
  TypeError();
  TypeError(const char * sMessage);
  TypeError(const std::string& sMessage);
  /// Destruction
  ~TypeError() noexcept override = default;
  PyObject * getPyExceptionType() const override;
};

/**
 * The ValueError can be used to indicate the usage of a wrong value.
 * @author Werner Mayer
 */
class BaseExport ValueError : public Exception
{
public:
  /// Construction
  ValueError();
  ValueError(const char * sMessage);
  ValueError(const std::string& sMessage);
  /// Destruction
  ~ValueError() noexcept override = default;
  PyObject * getPyExceptionType() const override;
};

/**
 * The IndexError can be used when a sequence subscript is out of range.
 * @author Werner Mayer
 */
class BaseExport IndexError : public Exception
{
public:
  /// Construction
  IndexError();
  IndexError(const char * sMessage);
  IndexError(const std::string& sMessage);
  /// Destruction
  ~IndexError() noexcept override = default;
  PyObject * getPyExceptionType() const override;
};

class BaseExport NameError : public Exception
{
public:
  /// Construction
  NameError();
  NameError(const char * sMessage);
  NameError(const std::string& sMessage);
  /// Destruction
  ~NameError() noexcept override = default;
  PyObject * getPyExceptionType() const override;
};

class BaseExport ImportError : public Exception
{
public:
  /// Construction
  ImportError();
  ImportError(const char * sMessage);
  ImportError(const std::string& sMessage);
  /// Destruction
  ~ImportError() noexcept override = default;
  PyObject * getPyExceptionType() const override;
};

/**
 * The AttributeError can be used to indicate the usage of a wrong value.
 * @author Werner Mayer
 */
class BaseExport AttributeError : public Exception
{
public:
  /// Construction
  AttributeError();
  AttributeError(const char * sMessage);
  AttributeError(const std::string& sMessage);
  /// Destruction
  ~AttributeError() noexcept override = default;
  PyObject * getPyExceptionType() const override;
};

/**
 * The RuntimeError can be used to indicate an unknown exception at runtime.
 * @author Werner Mayer
 */
class BaseExport RuntimeError : public Exception
{
public:
  /// Construction
  RuntimeError();
  RuntimeError(const char * sMessage);
  RuntimeError(const std::string& sMessage);
  /// Destruction
  ~RuntimeError() noexcept override = default;
  PyObject * getPyExceptionType() const override;
};

/**
 * The BadGraphError can be used to indicate that a graph is e.g. not a DAG.
 * @author Werner Mayer
 */
class BaseExport BadGraphError : public RuntimeError
{
public:
  /// Construction
  BadGraphError();
  BadGraphError(const char * sMessage);
  BadGraphError(const std::string& sMessage);
  /// Destruction
  ~BadGraphError() noexcept override = default;
  PyObject * getPyExceptionType() const override;
};

/**
 * The NotImplementedError can be used to indicate that an invoked function is not implemented.
 * @author Werner Mayer
 */
class BaseExport NotImplementedError : public Exception
{
public:
  /// Construction
  NotImplementedError();
  NotImplementedError(const char * sMessage);
  NotImplementedError(const std::string& sMessage);
  /// Destruction
  ~NotImplementedError() noexcept override = default;
  PyObject * getPyExceptionType() const override;
};

/**
 * The ZeroDivisionError can be used to indicate a division by zero.
 * @author Werner Mayer
 */
class BaseExport ZeroDivisionError : public Exception
{
public:
  /// Construction
  ZeroDivisionError();
  ZeroDivisionError(const char * sMessage);
  ZeroDivisionError(const std::string& sMessage);
  /// Destruction
  ~ZeroDivisionError() noexcept override = default;
  PyObject * getPyExceptionType() const override;
};

/**
 * The ReferenceError can be used to indicate a reference counter has the wrong value.
 * @author Werner Mayer
 */
class BaseExport ReferenceError : public Exception
{
public:
  /// Construction
  ReferenceError();
  ReferenceError(const char * sMessage);
  ReferenceError(const std::string& sMessage);
  /// Destruction
  ~ReferenceError() noexcept override = default;
  PyObject * getPyExceptionType() const override;
};

/**
 * The ExpressionError can be used to indicate erroneous.input
 * to the expression engine.
 * @author Werner Mayer
 */
class BaseExport ExpressionError : public Exception
{
public:
  /// Construction
  ExpressionError();
  ExpressionError(const char * sMessage);
  ExpressionError(const std::string& sMessage);
  /// Destruction
  ~ExpressionError() noexcept override = default;
  PyObject * getPyExceptionType() const override;
};

/**
 * The ParserError can be used to indicate the parsing error.
 * @author Werner Mayer
 */
class BaseExport ParserError : public Exception
{
public:
  /// Construction
  ParserError();
  ParserError(const char * sMessage);
  ParserError(const std::string& sMessage);
  /// Destruction
  ~ParserError() noexcept override = default;
  PyObject * getPyExceptionType() const override;
};

/**
 * The UnicodeError can be used to indicate unicode encoding/decoding error.
 * @author Werner Mayer
 */
class BaseExport UnicodeError : public Exception
{
public:
  /// Construction
  UnicodeError();
  UnicodeError(const char * sMessage);
  UnicodeError(const std::string& sMessage);
  /// Destruction
  ~UnicodeError() noexcept override = default;
  PyObject * getPyExceptionType() const override;
};

/**
 * The OverflowError can be used to indicate overflows of numbers.
 * @author Werner Mayer
 */
class BaseExport OverflowError : public Exception
{
public:
  /// Construction
  OverflowError();
  OverflowError(const char * sMessage);
  OverflowError(const std::string& sMessage);
  /// Destruction
  ~OverflowError() noexcept override = default;
  PyObject * getPyExceptionType() const override;
};

/**
 * The UnderflowError can be used to indicate underflows of numbers.
 * @author Werner Mayer
 */
class BaseExport UnderflowError : public Exception
{
public:
  /// Construction
  UnderflowError();
  UnderflowError(const char * sMessage);
  UnderflowError(const std::string& sMessage);
  /// Destruction
  ~UnderflowError() noexcept override = default;
  PyObject * getPyExceptionType() const override;
};

/**
 * The UnitsMismatchError can be used to indicate that quantities with different units are used.
 * @author Werner Mayer
 */
class BaseExport UnitsMismatchError : public Exception
{
public:
  /// Construction
  UnitsMismatchError();
  UnitsMismatchError(const char * sMessage);
  UnitsMismatchError(const std::string& sMessage);
  /// Destruction
  ~UnitsMismatchError() noexcept override = default;
  PyObject * getPyExceptionType() const override;
};

 /* The CADKernelError can be used to indicate an exception originating in the CAD Kernel
 * allowing to propagate the error messages of, for example, OCC Standard_Failure exception to
 * the FreeCAD application without making the FreeCAD application depend on OCC.
 * @author Abdullah Tahiri
 */
class BaseExport CADKernelError : public Exception
{
public:
    /// Construction
    CADKernelError();
    CADKernelError(const char * sMessage);
    CADKernelError(const std::string& sMessage);
    /// Destruction
    ~CADKernelError() noexcept override = default;
    PyObject * getPyExceptionType() const override;
};

/* The RestoreError can be used to try to do a best recovery effort when an error during restoring
 * occurs. The best recovery effort may be to ignore the element altogether or to insert a placeholder
 * depending on where the actual element being restored is used.
 *
 * For example, if it is part of an array (e.g. PropertyList) and the order in the array is relevant, it
 * is better to have a placeholder than to fail to restore the whole array.
 */
class BaseExport RestoreError : public Exception
{
public:
    /// Construction
    RestoreError();
    RestoreError(const char * sMessage);
    RestoreError(const std::string& sMessage);
    /// Destruction
    ~RestoreError() noexcept override = default;
    PyObject * getPyExceptionType() const override;
};



inline void Exception::setMessage(const char * sMessage)
{
  _sErrMsg = sMessage;
}

inline void Exception::setMessage(const std::string& sMessage)
{
  _sErrMsg = sMessage;
}

inline std::string Exception::getMessage() const
{
    return _sErrMsg;
}

inline std::string Exception::getFile() const
{
    return _file;
}

inline int Exception::getLine() const
{
    return _line;
}

inline std::string Exception::getFunction() const
{
    return _function;
}

inline bool Exception::getTranslatable() const
{
    return _isTranslatable;
}

inline void Exception::setDebugInformation(const std::string & file, const int line, const std::string & function)
{
    _file = file;
    _line = line;
    _function = function;
}

inline void Exception::setTranslatable(bool translatable)
{
    _isTranslatable = translatable;
}

#if defined(__GNUC__) && defined (FC_OS_LINUX)
class SignalException
{
public:
    SignalException();
    ~SignalException();

private:
    static void throw_signal(int signum);

private:
    struct sigaction new_action, old_action;
    bool ok;
};
#endif

} //namespace Base

#endif // BASE_EXCEPTION_H
