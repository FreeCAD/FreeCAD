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

#include <typeinfo>

#include "Exception.h"
#include "Console.h"
#include <CXX/Objects.hxx>

using namespace Base;


TYPESYSTEM_SOURCE(Base::Exception,Base::BaseClass);


Exception::Exception(void)
  : _line(0)
  , _isTranslatable(false)
  , _isReported(false)
{
  _sErrMsg = "FreeCAD Exception";
}

Exception::Exception(const Exception &inst)
  : _sErrMsg(inst._sErrMsg)
  , _file(inst._file)
  , _line(inst._line)
  , _function(inst._function)
  , _isTranslatable(inst._isTranslatable)
  , _isReported(inst._isReported)
{
}

Exception::Exception(const char * sMessage)
  : _sErrMsg(sMessage)
  , _line(0)
  , _isTranslatable(false)
  , _isReported(false)
{
}

Exception::Exception(const std::string& sMessage)
  : _sErrMsg(sMessage)
  , _line(0)
  , _isTranslatable(false)
  , _isReported(false)
{
}

Exception &Exception::operator=(const Exception &inst)
{
    _sErrMsg = inst._sErrMsg;
    _file = inst._file;
    _line = inst._line;
    _function = inst._function;
    return *this;
}

const char* Exception::what(void) const throw()
{
    return _sErrMsg.c_str();
}

void Exception::ReportException (void) const
{
    if (!_isReported) {
        std::string str = "";

        if (!_sErrMsg.empty())
            str+= (_sErrMsg + " ");

        if (!_function.empty()) {
            str+="In ";
            str+=_function;
            str+= " ";
        }

        std::string _linestr = std::to_string(_line);

        if (!_file.empty() && !_linestr.empty()) {
            // strip absolute path
            std::size_t pos = _file.find("src");

            if (pos!=std::string::npos) {
                str+="in ";
                str+= _file.substr(pos);
                str+= ":";
                str+=_linestr;
            }
        }

        Console().Error("Exception (%s): %s \n",Console().Time(),str.c_str());
        _isReported = true;
    }
}

PyObject * Exception::getPyObject(void)
{
    Py::Dict edict;
    edict.setItem("sclassname", Py::String(typeid(*this).name()));
    edict.setItem("sErrMsg", Py::String(this->getMessage()));
    edict.setItem("sfile", Py::String(this->getFile()));
#if PY_MAJOR_VERSION >= 3
    edict.setItem("iline", Py::Long(this->getLine()));
#else
    edict.setItem("iline", Py::Int(this->getLine()));
#endif
    edict.setItem("sfunction", Py::String(this->getFunction()));
    edict.setItem("swhat", Py::String(this->what()));
    edict.setItem("btranslatable", Py::Boolean(this->getTranslatable()));
    edict.setItem("breported", Py::Boolean(this->_isReported));
    return Py::new_reference_to(edict);
}

void Exception::setPyObject( PyObject * pydict)
{
    if (pydict!=NULL) {
        Py::Dict edict(pydict);
        if (edict.hasKey("sfile"))
            _file = static_cast<std::string>(Py::String(edict.getItem("sfile")));

        if (edict.hasKey("sfunction"))
            _function = static_cast<std::string>(Py::String(edict.getItem("sfunction")));

        if (edict.hasKey("sErrMsg"))
            _sErrMsg = static_cast<std::string>(Py::String(edict.getItem("sErrMsg")));

        if (edict.hasKey("iline"))
#if PY_MAJOR_VERSION >= 3
            _line = static_cast<long>(Py::Long(edict.getItem("iline")));
#else
            _line = static_cast<int>(Py::Int(edict.getItem("iline")));
#endif
        if (edict.hasKey("btranslatable"))
            _isTranslatable = static_cast<bool>(Py::Boolean(edict.getItem("btranslatable")));
        if (edict.hasKey("breported"))
            _isReported = static_cast<bool>(Py::Boolean(edict.getItem("breported")));
    }
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


XMLBaseException::XMLBaseException()
  : Exception()
{
}

XMLBaseException::XMLBaseException(const char * sMessage)
  : Exception(sMessage)
{
}

XMLBaseException::XMLBaseException(const std::string& sMessage)
  : Exception(sMessage)
{
}

XMLBaseException::XMLBaseException(const XMLBaseException &inst)
  : Exception(inst)
{
}

// ---------------------------------------------------------

XMLParseException::XMLParseException(const char * sMessage)
  : XMLBaseException(sMessage)
{
}

XMLParseException::XMLParseException(const std::string& sMessage)
  : XMLBaseException(sMessage)
{
}

XMLParseException::XMLParseException()
{
    _sErrMsg = "XML parse exception";
}

XMLParseException::XMLParseException(const XMLParseException &inst)
  : XMLBaseException(inst)
{
}

const char* XMLParseException::what() const throw()
{
    return XMLBaseException::what();
}

// ---------------------------------------------------------

XMLAttributeError::XMLAttributeError(const char * sMessage)
  : XMLBaseException(sMessage)
{
}

XMLAttributeError::XMLAttributeError(const std::string& sMessage)
  : XMLBaseException(sMessage)
{
}

XMLAttributeError::XMLAttributeError()
{
    _sErrMsg = "XML attribute error";
}

XMLAttributeError::XMLAttributeError(const XMLAttributeError &inst)
  : XMLBaseException(inst)
{
}

const char* XMLAttributeError::what() const throw()
{
    return XMLBaseException::what();
}

// ---------------------------------------------------------


FileException::FileException(const char * sMessage, const char * sFileName)
  : Exception( sMessage ),file(sFileName)
{
    if (sFileName) {
        _sErrMsgAndFileName = _sErrMsg + ": ";
        _sErrMsgAndFileName += sFileName;
    }
}

FileException::FileException(const char * sMessage, const FileInfo& File)
  : Exception( sMessage ),file(File)
{
    _sErrMsgAndFileName = _sErrMsg + ": ";
    _sErrMsgAndFileName += File.fileName();
}

FileException::FileException()
  : Exception( "Unknown file exception happened" )
{
    _sErrMsgAndFileName = _sErrMsg;
}

FileException::FileException(const FileException &inst)
  : Exception(inst._sErrMsg.c_str())
  , file(inst.file)
  , _sErrMsgAndFileName(inst._sErrMsgAndFileName.c_str())
{
}

std::string FileException::getFileName() const
{
    return file.fileName();
}

FileException & FileException::operator=(const FileException &inst)
{
    Exception::operator = (inst);
    file = inst.file;
    _sErrMsgAndFileName = inst._sErrMsgAndFileName;
    return *this;
}

const char* FileException::what() const throw()
{
    return _sErrMsgAndFileName.c_str();
}

void FileException::ReportException (void) const
{
    if (!_isReported) {
        std::string str = "";
        
        if (!_sErrMsgAndFileName.empty())
            str+= (_sErrMsgAndFileName + " ");
        
        if (!_function.empty()) {
            str+="In ";
            str+=_function;
            str+= " ";
        }
        
        std::string _linestr = std::to_string(_line);
        
        if (!_file.empty() && !_linestr.empty()) {
            // strip absolute path
            std::size_t pos = _file.find("src");
            
            if (pos!=std::string::npos) {
                str+="in ";
                str+= _file.substr(pos);
                str+= ":";
                str+=_linestr;
            }
        }
        
        Console().Error("Exception (%s): %s \n",Console().Time(),str.c_str());
        _isReported = true;
    }
}

PyObject * FileException::getPyObject(void)
{
    Py::Dict edict(Exception::getPyObject(), true);
    edict.setItem("filename", Py::String(this->file.fileName()));
    return Py::new_reference_to(edict);
}

void FileException::setPyObject( PyObject * pydict)
{
    if (pydict!=NULL) {
        Exception::setPyObject(pydict);

        Py::Dict edict(pydict);
        if (edict.hasKey("filename"))
            file.setFile(static_cast<std::string>(Py::String(edict.getItem("filename"))));
    }
}

// ---------------------------------------------------------


FileSystemError::FileSystemError()
  : Exception()
{
}

FileSystemError::FileSystemError(const char * sMessage)
  : Exception(sMessage)
{
}

FileSystemError::FileSystemError(const std::string& sMessage)
  : Exception(sMessage)
{
}

FileSystemError::FileSystemError(const FileSystemError &inst)
  : Exception(inst)
{
}

// ---------------------------------------------------------


BadFormatError::BadFormatError()
  : Exception()
{
}

BadFormatError::BadFormatError(const char * sMessage)
  : Exception(sMessage)
{
}

BadFormatError::BadFormatError(const std::string& sMessage)
  : Exception(sMessage)
{
}

BadFormatError::BadFormatError(const BadFormatError &inst)
  : Exception(inst)
{
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

AccessViolation::AccessViolation(const char * sMessage)
  : Exception(sMessage)
{
}

AccessViolation::AccessViolation(const std::string& sMessage)
  : Exception(sMessage)
{
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

AbnormalProgramTermination::AbnormalProgramTermination(const char * sMessage)
  : Exception(sMessage)
{
}

AbnormalProgramTermination::AbnormalProgramTermination(const std::string& sMessage)
  : Exception(sMessage)
{
}

AbnormalProgramTermination::AbnormalProgramTermination(const AbnormalProgramTermination &inst)
 : Exception(inst)
{
}

// ---------------------------------------------------------

UnknownProgramOption::UnknownProgramOption()
  : Exception()
{
}

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

ProgramInformation::ProgramInformation()
  : Exception()
{
}

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

TypeError::TypeError()
  : Exception()
{
}

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

ValueError::ValueError()
  : Exception()
{
}

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

IndexError::IndexError()
  : Exception()
{
}

IndexError::IndexError(const char * sMessage)
  : Exception(sMessage)
{
}

IndexError::IndexError(const std::string& sMessage)
  : Exception(sMessage)
{
}

IndexError::IndexError(const IndexError &inst)
 : Exception(inst)
{
}

// ---------------------------------------------------------

AttributeError::AttributeError()
  : Exception()
{
}

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

RuntimeError::RuntimeError()
  : Exception()
{
}

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

BadGraphError::BadGraphError()
  : RuntimeError("The graph must be a DAG.")
{
}

BadGraphError::BadGraphError(const char * sMessage)
  : RuntimeError(sMessage)
{
}

BadGraphError::BadGraphError(const std::string& sMessage)
  : RuntimeError(sMessage)
{
}

BadGraphError::BadGraphError(const BadGraphError &inst)
  : RuntimeError(inst)
{
}

// ---------------------------------------------------------

NotImplementedError::NotImplementedError()
  : Exception()
{
}

NotImplementedError::NotImplementedError(const char * sMessage)
  : Exception(sMessage)
{
}

NotImplementedError::NotImplementedError(const std::string& sMessage)
  : Exception(sMessage)
{
}

NotImplementedError::NotImplementedError(const NotImplementedError &inst)
  : Exception(inst)
{
}

// ---------------------------------------------------------

DivisionByZeroError::DivisionByZeroError()
  : Exception()
{
}

DivisionByZeroError::DivisionByZeroError(const char * sMessage)
  : Exception(sMessage)
{
}

DivisionByZeroError::DivisionByZeroError(const std::string& sMessage)
  : Exception(sMessage)
{
}

DivisionByZeroError::DivisionByZeroError(const DivisionByZeroError &inst)
  : Exception(inst)
{
}

// ---------------------------------------------------------

ReferencesError::ReferencesError()
: Exception()
{
}

ReferencesError::ReferencesError(const char * sMessage)
  : Exception(sMessage)
{
}

ReferencesError::ReferencesError(const std::string& sMessage)
  : Exception(sMessage)
{
}

ReferencesError::ReferencesError(const ReferencesError &inst)
  : Exception(inst)
{
}

// ---------------------------------------------------------

ExpressionError::ExpressionError()
  : Exception()
{
}

ExpressionError::ExpressionError(const char * sMessage)
  : Exception(sMessage)
{
}

ExpressionError::ExpressionError(const std::string& sMessage)
  : Exception(sMessage)
{
}

ExpressionError::ExpressionError(const ExpressionError &inst)
  : Exception(inst)
{
}

// ---------------------------------------------------------

ParserError::ParserError()
  : Exception()
{
}

ParserError::ParserError(const char * sMessage)
  : Exception(sMessage)
{
}

ParserError::ParserError(const std::string& sMessage)
  : Exception(sMessage)
{
}

ParserError::ParserError(const ParserError &inst)
  : Exception(inst)
{
}

// ---------------------------------------------------------

UnicodeError::UnicodeError()
  : Exception()
{
}

UnicodeError::UnicodeError(const char * sMessage)
  : Exception(sMessage)
{
}

UnicodeError::UnicodeError(const std::string& sMessage)
  : Exception(sMessage)
{
}

UnicodeError::UnicodeError(const UnicodeError &inst)
  : Exception(inst)
{
}

// ---------------------------------------------------------

OverflowError::OverflowError()
  : Exception()
{
}

OverflowError::OverflowError(const char * sMessage)
  : Exception(sMessage)
{
}

OverflowError::OverflowError(const std::string& sMessage)
  : Exception(sMessage)
{
}

OverflowError::OverflowError(const OverflowError &inst)
 : Exception(inst)
{
}

// ---------------------------------------------------------

UnderflowError::UnderflowError()
  : Exception()
{
}

UnderflowError::UnderflowError(const char * sMessage)
  : Exception(sMessage)
{
}

UnderflowError::UnderflowError(const std::string& sMessage)
  : Exception(sMessage)
{
}

UnderflowError::UnderflowError(const UnderflowError &inst)
  : Exception(inst)
{
}

// ---------------------------------------------------------

UnitsMismatchError::UnitsMismatchError()
  : Exception()
{
}

UnitsMismatchError::UnitsMismatchError(const char * sMessage)
  : Exception(sMessage)
{
}

UnitsMismatchError::UnitsMismatchError(const std::string& sMessage)
  : Exception(sMessage)
{
}

UnitsMismatchError::UnitsMismatchError(const UnitsMismatchError &inst)
  : Exception(inst)
{
}

// ---------------------------------------------------------

CADKernelError::CADKernelError()
  : Exception()
{
}

CADKernelError::CADKernelError(const char * sMessage)
  : Exception(sMessage)
{
}

CADKernelError::CADKernelError(const std::string& sMessage)
  : Exception(sMessage)
{
}

CADKernelError::CADKernelError(const CADKernelError &inst)
  : Exception(inst)
{
}


// ---------------------------------------------------------

RestoreError::RestoreError()
  : Exception()
{
}

RestoreError::RestoreError(const char * sMessage)
  : Exception(sMessage)
{
}

RestoreError::RestoreError(const std::string& sMessage)
  : Exception(sMessage)
{
}

RestoreError::RestoreError(const RestoreError &inst)
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
    std::cerr << "SIGSEGV signal raised: " << signum << std::endl;
    throw std::runtime_error ("throw_signal");
}
#endif

