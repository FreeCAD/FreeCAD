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

#define THROW(exception) throw Base::exception(__FILE__,__LINE__,__PRETTY_FUNCTION__);
#define THROWM(exception, message) throw Base::exception(message,__FILE__,__LINE__,__PRETTY_FUNCTION__);

#define THROWMF_FILEEXCEPTION(message,filenameorfileinfo) throw Base::FileException(message,filenameorfileinfo,__FILE__,__LINE__,__PRETTY_FUNCTION__);

namespace Base
{

class BaseExport Exception : public BaseClass
{
  TYPESYSTEM_HEADER();

public:

  virtual ~Exception() throw() {}

  Exception &operator=(const Exception &inst);

  virtual const char* what(void) const throw();

  void ReportException (void) const;
  /// returns the message string used in ReportException
  virtual std::string report() const;

  inline void setMessage(const char * sMessage);
  inline void setMessage(const std::string& sMessage);
  // what may differ from the message given by the user in
  // derived classes
  inline std::string getMessage();

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

  ///intended to use via macro for autofilling of debuging information
  Exception(const std::string& sMessage, const std::string & file, const int line, const std::string & function);
  /// intended to use via macro for autofilling of debuging information
  Exception(const std::string & file, const int line, const std::string & function);

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

  /// intended to use via macro for autofilling of debuging information
  AbortException(const std::string& sMessage, const std::string & file, const int line, const std::string & function);
  /// intended to use via macro for autofilling of debuging information
  AbortException(const std::string & file, const int line, const std::string & function);
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
  /// intended to use via macro for autofilling of debuging information
  XMLBaseException(const std::string& sMessage, const std::string & file, const int line, const std::string & function);

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
  /// intended to use via macro for autofilling of debuging information
  XMLParseException(const std::string& sMessage, const std::string & file, const int line, const std::string & function);
  /// intended to use via macro for autofilling of debuging information
  XMLParseException(const std::string & file, const int line, const std::string & function);
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
  
  /// intended to use via macro for autofilling of debuging information
  FileException(const std::string& sMessage, const char * sFileName, const std::string & file, const int line, const std::string & function);
  /// intended to use via macro for autofilling of debuging information
  FileException(const std::string& sMessage, const FileInfo& File, const std::string & file, const int line, const std::string & function);
  /// intended to use via macro for autofilling of debuging information
  FileException(const std::string & file, const int line, const std::string & function);
  /// Destruction
  virtual ~FileException() throw() {}
  /// Description of the exception
  virtual const char* what() const throw();
  /// Report string (for report view)
  virtual std::string report() const;
  /// Get file name for use with tranlatable message
  std::string getFileName() const;
protected:
  FileInfo file;
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
  /// intended to use via macro for autofilling of debuging information
  FileSystemError(const std::string& sMessage, const std::string & file, const int line, const std::string & function);
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
  /// intended to use via macro for autofilling of debuging information
  BadFormatError(const std::string& sMessage, const std::string & file, const int line, const std::string & function);
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
  /// intended to use via macro for autofilling of debuging information
  MemoryException(const std::string & file, const int line, const std::string & function);  
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
  /// intended to use via macro for autofilling of debuging information
  AccessViolation(const std::string& sMessage, const std::string & file, const int line, const std::string & function);
  /// intended to use via macro for autofilling of debuging information
  AccessViolation(const std::string & file, const int line, const std::string & function);
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
  /// intended to use via macro for autofilling of debuging information
  AbnormalProgramTermination(const std::string& sMessage, const std::string & file, const int line, const std::string & function);
  /// intended to use via macro for autofilling of debuging information
  AbnormalProgramTermination(const std::string & file, const int line, const std::string & function);
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
  /// intended to use via macro for autofilling of debuging information
  UnknownProgramOption(const std::string& sMessage, const std::string & file, const int line, const std::string & function);
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
  /// intended to use via macro for autofilling of debuging information
  ProgramInformation(const std::string& sMessage, const std::string & file, const int line, const std::string & function);

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
  /// intended to use via macro for autofilling of debuging information
  TypeError(const std::string& sMessage, const std::string & file, const int line, const std::string & function);
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
  /// intended to use via macro for autofilling of debuging information
  ValueError(const std::string& sMessage, const std::string & file, const int line, const std::string & function);
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
  /// intended to use via macro for autofilling of debuging information
  IndexError(const std::string& sMessage, const std::string & file, const int line, const std::string & function);
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
  /// intended to use via macro for autofilling of debuging information
  AttributeError(const std::string& sMessage, const std::string & file, const int line, const std::string & function);
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
  /// intended to use via macro for autofilling of debuging information
  RuntimeError(const std::string& sMessage, const std::string & file, const int line, const std::string & function);
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
  /// intended to use via macro for autofilling of debuging information
  NotImplementedError(const std::string& sMessage, const std::string & file, const int line, const std::string & function);
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
  /// intended to use via macro for autofilling of debuging information
  DivisionByZeroError(const std::string& sMessage, const std::string & file, const int line, const std::string & function);
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
  /// intended to use via macro for autofilling of debuging information
  ReferencesError(const std::string& sMessage, const std::string & file, const int line, const std::string & function);
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
  /// intended to use via macro for autofilling of debuging information
  ExpressionError(const std::string& sMessage, const std::string & file, const int line, const std::string & function);
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
  /// intended to use via macro for autofilling of debuging information
  ParserError(const std::string& sMessage, const std::string & file, const int line, const std::string & function);
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
  /// intended to use via macro for autofilling of debuging information
  UnicodeError(const std::string& sMessage, const std::string & file, const int line, const std::string & function);
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
  /// intended to use via macro for autofilling of debuging information
  OverflowError(const std::string& sMessage, const std::string & file, const int line, const std::string & function);
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
  /// intended to use via macro for autofilling of debuging information
  UnderflowError(const std::string& sMessage, const std::string & file, const int line, const std::string & function);
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
  /// intended to use via macro for autofilling of debuging information
  UnitsMismatchError(const std::string& sMessage, const std::string & file, const int line, const std::string & function);
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
    /// intended to use via macro for autofilling of debuging information
    CADKernelError(const std::string& sMessage, const std::string & file, const int line, const std::string & function);
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

inline std::string Exception::getMessage()
{
    return _sErrMsg;
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

