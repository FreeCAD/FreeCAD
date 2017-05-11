/***************************************************************************
 *   (c) Jürgen Riegel (juergen.riegel@web.de) 2002                        *
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
 *   Juergen Riegel 2002                                                   *
 ***************************************************************************/


#ifndef BASE_EXCEPTION_H
#define BASE_EXCEPTION_H

#include <exception>
#include <stdexcept>
#include <string>
#include <signal.h>
#include "FileInfo.h"
#include "BaseClass.h"

/* MACROS FOR THROWING EXCEPTIONS */

#ifdef _MSC_VER

# define THROW(exception) {exception myexcp; myexcp.setDebugInformation(__FILE__,__LINE__,__FUNCSIG__); throw myexcp;}
# define THROWM(exception, message) {exception myexcp(message); myexcp.setDebugInformation(__FILE__,__LINE__,__FUNCSIG__); throw myexcp;}
# define THROWMF_FILEEXCEPTION(message,filenameorfileinfo) {FileException myexcp(message, filenameorfileinfo); myexcp.setDebugInformation(__FILE__,__LINE__,__FUNCSIG__); throw myexcp;}

#elif defined(__GNUC__)

# define THROW(exception) {exception myexcp; myexcp.setDebugInformation(__FILE__,__LINE__,__PRETTY_FUNCTION__); throw myexcp;}
# define THROWM(exception, message) {exception myexcp(message); myexcp.setDebugInformation(__FILE__,__LINE__,__PRETTY_FUNCTION__); throw myexcp;}
# define THROWMF_FILEEXCEPTION(message,filenameorfileinfo) {FileException myexcp(message, filenameorfileinfo); myexcp.setDebugInformation(__FILE__,__LINE__,__PRETTY_FUNCTION__); throw myexcp;}

#else

# define THROW(exception) {exception myexcp; myexcp.setDebugInformation(__FILE__,__LINE__,__func__); throw myexcp;}
# define THROWM(exception, message) {exception myexcp(message); myexcp.setDebugInformation(__FILE__,__LINE__,__func__); throw myexcp;}
# define THROWMF_FILEEXCEPTION(message,filenameorfileinfo) {FileException myexcp(message, filenameorfileinfo); myexcp.setDebugInformation(__FILE__,__LINE__,__func__); throw myexcp;}

#endif

namespace Base
{

class BaseExport Exception : public BaseClass
{
  TYPESYSTEM_HEADER();

public:

  virtual ~Exception() throw() {}

  Exception &operator=(const Exception &inst);

  virtual const char* what(void) const throw();

  virtual void ReportException (void) const;

  inline void setMessage(const char * sMessage);
  inline void setMessage(const std::string& sMessage);
  // what may differ from the message given by the user in
  // derived classes
  inline std::string getMessage() const;
  
  /// setter methods for including debug information
  /// intended to use via macro for autofilling of debugging information
  inline void setDebugInformation(const std::string & file, const int line, const std::string & function);

protected:
public: // FIXME: Remove the public keyword
 /* sMessage may be:
  * - an UI compliant string subsceptible of being translated and shown to the user in the UI
  * - a very technical message not intended to be traslated or shown to the user in the UI
  * The preferred way of throwing an exception is using the macros above. This way, the file, 
  * line and function are automatically inserted. */
  Exception(const char * sMessage);
  Exception(const std::string& sMessage);
  Exception(void);
  Exception(const Exception &inst);

protected:
  std::string _sErrMsg;
  std::string _file;
  std::string _line;
  std::string _function;
};


/**
 * The AbortException is thrown if a pending operation was aborted.
 * @author Werner Mayer
 */
class BaseExport AbortException : public Exception
{
public:
  /// Construction
  AbortException(const char * sMessage);
  /// Construction
  AbortException();
  /// Construction
  AbortException(const AbortException &inst);

  /// Destruction
  virtual ~AbortException() throw() {}
  /// Description of the exception
  virtual const char* what() const throw();
};

/**
 * The XMLBaseException can be used to indicate any kind of XML related errors.
 * @author Werner Mayer
 */
class BaseExport XMLBaseException : public Exception
{
public:
  /// Construction
  XMLBaseException(const char * sMessage);
  XMLBaseException(const std::string& sMessage);
  /// Construction
  XMLBaseException(const XMLBaseException &inst);

  /// Destruction
  virtual ~XMLBaseException() throw() {}
};

/**
 * The XMLParseException is thrown if parsing an XML failed.
 * @author Werner Mayer
 */
class BaseExport XMLParseException : public Exception
{
public:
  /// Construction
  XMLParseException(const char * sMessage);
  /// Construction
  XMLParseException(const std::string& sMessage);
  /// Construction
  XMLParseException();
  /// Construction
  XMLParseException(const XMLParseException &inst);

  /// Destruction
  virtual ~XMLParseException() throw() {}
  /// Description of the exception
  virtual const char* what() const throw();
};

/** File exception handling class
 * This class is specialized to go with exception thrown in case of File IO Problems.
 * @author Juergen Riegel
 */
class BaseExport FileException : public Exception
{
public:
  /// With massage and file name
  FileException(const char * sMessage, const char * sFileName=0);
  /// With massage and file name
  FileException(const char * sMessage, const FileInfo& File);
  /// standard construction
  FileException();
  /// Construction
  FileException(const FileException &inst);
  /// Destruction
  virtual ~FileException() throw() {}
  /// Description of the exception
  virtual const char* what() const throw();
  /// Report generation
  virtual void ReportException (void) const;
  /// Get file name for use with tranlatable message
  std::string getFileName() const;
protected:
  FileInfo file;
  // necesary for what() legacy behaviour as it returns a buffer that can not be of a temporary object to be destroyed at end of what()
  std::string _sErrMsgAndFileName; 
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
  FileSystemError(const char * sMessage);
  FileSystemError(const std::string& sMessage);
  /// Construction
  FileSystemError(const FileSystemError &inst);
  /// Destruction
  virtual ~FileSystemError() throw() {}
};

/**
 * The BadFormatError can be used to indicate errors in a data structure.
 * @author Werner Mayer
 */
class BaseExport BadFormatError : public Exception
{
public:
  /// Construction
  BadFormatError(const char * sMessage);
  BadFormatError(const std::string& sMessage);
  /// Construction
  BadFormatError(const BadFormatError &inst);
  /// Destruction
  virtual ~BadFormatError() throw() {}
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
  virtual ~MemoryException() throw() {}
#if defined (__GNUC__)
  /// Description of the exception
  virtual const char* what() const throw();
#endif
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
  /// Construction
  AccessViolation(const AccessViolation &inst);
  /// Destruction
  virtual ~AccessViolation() throw() {}
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
  AbnormalProgramTermination(const AbnormalProgramTermination &inst);
  /// Destruction
  virtual ~AbnormalProgramTermination() throw() {}
};

/**
 * The UnknownProgramOption can be used to indicate an unknown program option.
 * @author Werner Mayer
 */
class BaseExport UnknownProgramOption : public Exception
{
public:
  /// Construction
  UnknownProgramOption(const char * sMessage);
  UnknownProgramOption(const std::string& sMessage);
  /// Construction
  UnknownProgramOption(const UnknownProgramOption &inst);
  /// Destruction
  virtual ~UnknownProgramOption() throw() {}
};

/**
 * The ProgramInformation can be used to show information about the program.
 * @author Werner Mayer
 */
class BaseExport ProgramInformation : public Exception
{
public:
  /// Construction
  ProgramInformation(const char * sMessage);
  ProgramInformation(const std::string& sMessage);
  /// Construction
  ProgramInformation(const ProgramInformation &inst);

  /// Destruction
  virtual ~ProgramInformation() throw() {}
};

/**
 * The TypeError can be used to indicate the usage of a wrong type.
 * @author Werner Mayer
 */
class BaseExport TypeError : public Exception
{
public:
  /// Construction
  TypeError(const char * sMessage);
  TypeError(const std::string& sMessage);
  /// Construction
  TypeError(const TypeError &inst);
  /// Destruction
  virtual ~TypeError() throw() {}
};

/**
 * The ValueError can be used to indicate the usage of a wrong value.
 * @author Werner Mayer
 */
class BaseExport ValueError : public Exception
{
public:
  /// Construction
  ValueError(const char * sMessage);
  ValueError(const std::string& sMessage);
  /// Construction
  ValueError(const ValueError &inst);
  /// Destruction
  virtual ~ValueError() throw() {}
};

/**
 * The IndexError can be used when a sequence subscript is out of range.
 * @author Werner Mayer
 */
class BaseExport IndexError : public Exception
{
public:
  /// Construction
  IndexError(const char * sMessage);
  IndexError(const std::string& sMessage);
  /// Construction
  IndexError(const IndexError &inst);
  /// Destruction
  virtual ~IndexError() throw() {}
};

/**
 * The AttributeError can be used to indicate the usage of a wrong value.
 * @author Werner Mayer
 */
class BaseExport AttributeError : public Exception
{
public:
  /// Construction
  AttributeError(const char * sMessage);
  AttributeError(const std::string& sMessage);
  /// Construction
  AttributeError(const AttributeError &inst);
  /// Destruction
  virtual ~AttributeError() throw() {}
};

/**
 * The RuntimeError can be used to indicate an unknown exception at runtime.
 * @author Werner Mayer
 */
class BaseExport RuntimeError : public Exception
{
public:
  /// Construction
  RuntimeError(const char * sMessage);
  RuntimeError(const std::string& sMessage);
  /// Construction
  RuntimeError(const RuntimeError &inst);
  /// Destruction
  virtual ~RuntimeError() throw() {}
};

/**
 * The NotImplementedError can be used to indicate that an invoked function is not implemented.
 * @author Werner Mayer
 */
class BaseExport NotImplementedError : public Exception
{
public:
  /// Construction
  NotImplementedError(const char * sMessage);
  NotImplementedError(const std::string& sMessage);
  /// Construction
  NotImplementedError(const NotImplementedError &inst);
  /// Destruction
  virtual ~NotImplementedError() throw() {}
};

/**
 * The DivisionByZeroError can be used to indicate a division by zero.
 * @author Werner Mayer
 */
class BaseExport DivisionByZeroError : public Exception
{
public:
  /// Construction
  DivisionByZeroError(const char * sMessage);
  DivisionByZeroError(const std::string& sMessage);
  /// Construction
  DivisionByZeroError(const DivisionByZeroError &inst);
  /// Destruction
  virtual ~DivisionByZeroError() throw() {}
};

/**
 * The ReferencesError can be used to indicate a reference counter has the wrong value.
 * @author Werner Mayer
 */
class BaseExport ReferencesError : public Exception
{
public:
  /// Construction
  ReferencesError(const char * sMessage);
  ReferencesError(const std::string& sMessage);
  /// Construction
  ReferencesError(const ReferencesError &inst);
  /// Destruction
  virtual ~ReferencesError() throw() {}
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
  ExpressionError(const char * sMessage);
  ExpressionError(const std::string& sMessage);
  /// Construction
  ExpressionError(const ExpressionError &inst);
  /// Destruction
  virtual ~ExpressionError() throw() {}
};

/**
 * The ParserError can be used to indicate the parsing error.
 * @author Werner Mayer
 */
class BaseExport ParserError : public Exception
{
public:
  /// Construction
  ParserError(const char * sMessage);
  ParserError(const std::string& sMessage);
  /// Construction
  ParserError(const ParserError &inst);
  /// Destruction
  virtual ~ParserError() throw() {}
};

/**
 * The UnicodeError can be used to indicate unicode encoding/decoding error.
 * @author Werner Mayer
 */
class BaseExport UnicodeError : public Exception
{
public:
  /// Construction
  UnicodeError(const char * sMessage);
  UnicodeError(const std::string& sMessage);
  /// Construction
  UnicodeError(const UnicodeError &inst);
  /// Destruction
  virtual ~UnicodeError() throw() {}
};

/**
 * The OverflowError can be used to indicate overflows of numbers.
 * @author Werner Mayer
 */
class BaseExport OverflowError : public Exception
{
public:
  /// Construction
  OverflowError(const char * sMessage);
  OverflowError(const std::string& sMessage);
  /// Construction
  OverflowError(const OverflowError &inst);
  /// Destruction
  virtual ~OverflowError() throw() {}
};

/**
 * The UnderflowError can be used to indicate underflows of numbers.
 * @author Werner Mayer
 */
class BaseExport UnderflowError : public Exception
{
public:
  /// Construction
  UnderflowError(const char * sMessage);
  UnderflowError(const std::string& sMessage);
  /// Construction
  UnderflowError(const UnderflowError &inst);
  /// Destruction
  virtual ~UnderflowError() throw() {}
};

/**
 * The UnitsMismatchError can be used to indicate that quantities with different units are used.
 * @author Werner Mayer
 */
class BaseExport UnitsMismatchError : public Exception
{
public:
  /// Construction
  UnitsMismatchError(const char * sMessage);
  UnitsMismatchError(const std::string& sMessage);
  /// Construction
  UnitsMismatchError(const UnitsMismatchError &inst);
  /// Destruction
  virtual ~UnitsMismatchError() throw() {}
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
    CADKernelError(const char * sMessage);
    CADKernelError(const std::string& sMessage);
    /// Construction
    CADKernelError(const CADKernelError &inst);
    /// Destruction
    virtual ~CADKernelError() throw() {}
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

inline void Exception::setDebugInformation(const std::string & file, const int line, const std::string & function)
{
    _file = file;
    _line = std::to_string(line);
    _function = function;
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

