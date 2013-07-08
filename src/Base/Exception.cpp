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


#include "PreCompiled.h"


#include "Exception.h"
#include "Console.h"

using namespace Base;


TYPESYSTEM_SOURCE(Base::Exception,Base::BaseClass);


Exception::Exception(void)
{
  _sErrMsg = "FreeCAD Exception";
}

Exception::Exception(const Exception &inst)
 : BaseClass(),_sErrMsg(inst._sErrMsg)
{
}


Exception::Exception(const char * sMessage)
 : _sErrMsg(sMessage)
{
}

Exception::Exception(const std::string& sMessage)
 : _sErrMsg(sMessage)
{
}

Exception &Exception::operator=(const Exception &inst)
{
  _sErrMsg = inst._sErrMsg;
  return *this;
}

const char* Exception::what(void) const throw()
{
    return _sErrMsg.c_str();
}

void Exception::ReportException (void) const
{
    Console().Error("Exception (%s): %s \n",Console().Time(),what());
}

// ---------------------------------------------------------

AbortException::AbortException(const char * sMessage)
  : Exception( sMessage )
{
}

AbortException::AbortException()
{
    _sErrMsg = "Aborted operation";
}

AbortException::AbortException(const AbortException &inst)
 : Exception(inst)
{
}

const char* AbortException::what() const throw()
{
    return Exception::what();
}

// ---------------------------------------------------------

XMLParseException::XMLParseException(const char * sMessage)
  : Exception(sMessage)
{
}

XMLParseException::XMLParseException(const std::string& sMessage)
 : Exception(sMessage)
{
}

XMLParseException::XMLParseException()
{
    _sErrMsg = "XML parse exception";
}

XMLParseException::XMLParseException(const XMLParseException &inst)
 : Exception(inst)
{
}

const char* XMLParseException::what() const throw()
{
    return Exception::what();
}

// ---------------------------------------------------------

FileException::FileException(const char * sMessage, const char * sFileName)
  : Exception( sMessage ),file(sFileName)
{
    _sErrMsg += ": ";
    _sErrMsg += sFileName;
}

FileException::FileException(const char * sMessage, const FileInfo& File)
  : Exception( sMessage ),file(File)
{
    _sErrMsg += ": ";
    _sErrMsg += File.fileName();
}

FileException::FileException()
  : Exception( "Unknown file exeption happened" )
{
}

FileException::FileException(const FileException &inst)
  : Exception( inst._sErrMsg.c_str() ),file(inst.file)
{
}

const char* FileException::what() const throw()
{
    return Exception::what();
}

// ---------------------------------------------------------

MemoryException::MemoryException()
{
    _sErrMsg = "Not enough memory available";
}

MemoryException::MemoryException(const MemoryException &inst)
#if defined (__GNUC__)
 : std::bad_alloc(), Exception(inst) 
#else
 : Exception(inst)
#endif
{
}

#if defined (__GNUC__)
const char* MemoryException::what() const throw()
{
    // call what() of Exception, not of std::bad_alloc
    return Exception::what();
}
#endif

// ---------------------------------------------------------

AccessViolation::AccessViolation()
{
    _sErrMsg = "Access violation";
}

AccessViolation::AccessViolation(const AccessViolation &inst)
 : Exception(inst)
{
}

// ---------------------------------------------------------

AbnormalProgramTermination::AbnormalProgramTermination()
{
    _sErrMsg = "Abnormal program termination";
}

AbnormalProgramTermination::AbnormalProgramTermination(const AbnormalProgramTermination &inst)
 : Exception(inst)
{
}

// ---------------------------------------------------------

UnknownProgramOption::UnknownProgramOption(const char * sMessage)
  : Exception(sMessage)
{
}

UnknownProgramOption::UnknownProgramOption(const std::string& sMessage)
  : Exception(sMessage)
{
}

UnknownProgramOption::UnknownProgramOption(const UnknownProgramOption &inst)
 : Exception(inst)
{
}

// ---------------------------------------------------------

ProgramInformation::ProgramInformation(const char * sMessage)
  : Exception(sMessage)
{
}

ProgramInformation::ProgramInformation(const std::string& sMessage)
  : Exception(sMessage)
{
}

ProgramInformation::ProgramInformation(const ProgramInformation &inst)
 : Exception(inst)
{
}

// ---------------------------------------------------------

TypeError::TypeError(const char * sMessage)
  : Exception(sMessage)
{
}

TypeError::TypeError(const std::string& sMessage)
  : Exception(sMessage)
{
}

TypeError::TypeError(const TypeError &inst)
 : Exception(inst)
{
}

// ---------------------------------------------------------

ValueError::ValueError(const char * sMessage)
  : Exception(sMessage)
{
}

ValueError::ValueError(const std::string& sMessage)
  : Exception(sMessage)
{
}

ValueError::ValueError(const ValueError &inst)
 : Exception(inst)
{
}

// ---------------------------------------------------------

AttributeError::AttributeError(const char * sMessage)
  : Exception(sMessage)
{
}

AttributeError::AttributeError(const std::string& sMessage)
  : Exception(sMessage)
{
}

AttributeError::AttributeError(const AttributeError &inst)
 : Exception(inst)
{
}

// ---------------------------------------------------------

RuntimeError::RuntimeError(const char * sMessage)
  : Exception(sMessage)
{
}

RuntimeError::RuntimeError(const std::string& sMessage)
  : Exception(sMessage)
{
}

RuntimeError::RuntimeError(const RuntimeError &inst)
 : Exception(inst)
{
}

// ---------------------------------------------------------

#if defined(__GNUC__) && defined (FC_OS_LINUX)
#include <stdexcept>
#include <iostream>

SignalException::SignalException()
{
    memset (&new_action, 0, sizeof (new_action));
    new_action.sa_handler = throw_signal;
    sigemptyset (&new_action.sa_mask);
    new_action.sa_flags = 0;
    ok = (sigaction (SIGSEGV, &new_action, &old_action) < 0);
#ifdef _DEBUG
    std::cout << "Set new signal handler" << std::endl;
#endif
}

SignalException::~SignalException()
{
    sigaction (SIGSEGV, &old_action, NULL);
#ifdef _DEBUG
    std::cout << "Restore old signal handler" << std::endl;
#endif
}

void SignalException::throw_signal(int signum)
{
    std::cerr << "SIGSEGV signal raised" << std::endl;
    throw std::runtime_error ("throw_signal");
}
#endif

