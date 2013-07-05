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
#include <string>
#include <signal.h>
#include "FileInfo.h"
#include "BaseClass.h"

namespace Base
{

class BaseExport Exception : public BaseClass
{
  TYPESYSTEM_HEADER();

public:
  Exception(const char * sMessage);
  Exception(const std::string& sMessage);
  Exception(void);
  Exception(const Exception &inst);
  virtual ~Exception() throw() {}

  Exception &operator=(const Exception &inst);
  virtual const char* what(void) const throw();
  virtual void ReportException (void) const;
  inline void setMessage(const char * sMessage);
  inline void setMessage(const std::string& sMessage);
  
protected:
  std::string _sErrMsg;
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
protected:
  FileInfo file;
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


inline void Exception::setMessage(const char * sMessage)
{
  _sErrMsg = sMessage;
}

inline void Exception::setMessage(const std::string& sMessage)
{
  _sErrMsg = sMessage;
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

