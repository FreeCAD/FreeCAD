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


#include "PreCompiled.h"

#include "Exception.h"
#include "Console.h"
#include "PyObjectBase.h"


FC_LOG_LEVEL_INIT("Exception", true, true)

using namespace Base;


TYPESYSTEM_SOURCE(Base::Exception,Base::BaseClass)


Exception::Exception()
  : _line(0)
  , _isTranslatable(false)
  , _isReported(false)
{
  _sErrMsg = "FreeCAD Exception";
}

Exception::Exception(const Exception &inst) = default;

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

const char* Exception::what() const noexcept
{
    return _sErrMsg.c_str();
}

void Exception::ReportException () const
{
    if (!_isReported) {
        const char *msg{};
        if (_sErrMsg.empty())
            msg = typeid(*this).name();
        else
            msg = _sErrMsg.c_str();
#ifdef FC_DEBUG
        if (_function.size()) {
            _FC_ERR(_file.c_str(),_line, _function << " -- " << msg);
        } else
#endif
            _FC_ERR(_file.c_str(),_line,msg);
        _isReported = true;
    }
}

PyObject * Exception::getPyObject()
{
    Py::Dict edict;
    edict.setItem("sclassname", Py::String(typeid(*this).name()));
    edict.setItem("sErrMsg", Py::String(this->getMessage()));
    edict.setItem("sfile", Py::String(this->getFile()));
    edict.setItem("iline", Py::Long(this->getLine()));
    edict.setItem("sfunction", Py::String(this->getFunction()));
    edict.setItem("swhat", Py::String(this->what()));
    edict.setItem("btranslatable", Py::Boolean(this->getTranslatable()));
    edict.setItem("breported", Py::Boolean(this->_isReported));
    return Py::new_reference_to(edict);
}

void Exception::setPyObject( PyObject * pydict)
{
    try {
        if (pydict && Py::_Dict_Check(pydict)) {
            Py::Dict edict(pydict);
            if (edict.hasKey("sfile"))
                _file = static_cast<std::string>(Py::String(edict.getItem("sfile")));

            if (edict.hasKey("sfunction"))
                _function = static_cast<std::string>(Py::String(edict.getItem("sfunction")));

            if (edict.hasKey("sErrMsg"))
                _sErrMsg = static_cast<std::string>(Py::String(edict.getItem("sErrMsg")));

            if (edict.hasKey("iline"))
                _line = static_cast<long>(Py::Long(edict.getItem("iline")));
            if (edict.hasKey("btranslatable"))
                _isTranslatable = static_cast<bool>(Py::Boolean(edict.getItem("btranslatable")));
            if (edict.hasKey("breported"))
                _isReported = static_cast<bool>(Py::Boolean(edict.getItem("breported")));
        }
    }
    catch (Py::Exception& e) {
        e.clear(); // should never happen
    }
}

PyObject * Exception::getPyExceptionType() const
{
    return PyExc_FC_GeneralError;
}

void Exception::setPyException() const
{
    PyObject* exc = getPyExceptionType();
    if (!exc) {
        exc = PyExc_FC_GeneralError;
    }

    PyErr_SetString(exc, what());
}

// ---------------------------------------------------------

TYPESYSTEM_SOURCE(Base::AbortException,Base::Exception)

AbortException::AbortException(const char * sMessage)
  : Exception( sMessage )
{
}

AbortException::AbortException()
{
    _sErrMsg = "Aborted operation";
}

const char* AbortException::what() const noexcept
{
    return Exception::what();
}

PyObject * AbortException::getPyExceptionType() const
{
    return PyExc_FC_FreeCADAbort;
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

PyObject * XMLBaseException::getPyExceptionType() const
{
    return PyExc_FC_XMLBaseException;
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

const char* XMLParseException::what() const noexcept
{
    return XMLBaseException::what();
}

PyObject * XMLParseException::getPyExceptionType() const
{
    return PyExc_FC_XMLParseException;
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

const char* XMLAttributeError::what() const noexcept
{
    return XMLBaseException::what();
}

PyObject * XMLAttributeError::getPyExceptionType() const
{
    return PyExc_FC_XMLAttributeError;
}

// ---------------------------------------------------------


FileException::FileException(const char * sMessage, const char * sFileName)
  : Exception( sMessage ),file(sFileName)
{
    setFileName(sFileName);
}

FileException::FileException(const char * sMessage, const FileInfo& File)
  : Exception( sMessage ),file(File)
{
    setFileName(File.filePath().c_str());
}

FileException::FileException()
  : Exception( "Unknown file exception happened" )
{
    _sErrMsgAndFileName = _sErrMsg;
}

void FileException::setFileName(const char * sFileName)
{
    file.setFile(sFileName);
    _sErrMsgAndFileName = _sErrMsg;
    if (sFileName) {
        _sErrMsgAndFileName += ": ";
        _sErrMsgAndFileName += sFileName;
    }
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

const char* FileException::what() const noexcept
{
    return _sErrMsgAndFileName.c_str();
}

void FileException::ReportException () const
{
    if (!_isReported) {
        const char *msg{};
        if (_sErrMsgAndFileName.empty())
            msg = typeid(*this).name();
        else
            msg = _sErrMsgAndFileName.c_str();
#ifdef FC_DEBUG
        if (_function.size()) {
            _FC_ERR(_file.c_str(),_line, _function << " -- " << msg);
        } else
#endif
            _FC_ERR(_file.c_str(),_line,msg);
        _isReported = true;
    }
}

PyObject * FileException::getPyObject()
{
    Py::Dict edict(Exception::getPyObject(), true);
    edict.setItem("filename", Py::String(this->file.fileName()));
    return Py::new_reference_to(edict);
}

void FileException::setPyObject( PyObject * pydict)
{
    if (pydict) {
        Exception::setPyObject(pydict);

        Py::Dict edict(pydict);
        if (edict.hasKey("filename"))
            setFileName(Py::String(edict.getItem("filename")).as_std_string("utf-8").c_str());
    }
}

PyObject * FileException::getPyExceptionType() const
{
    return PyExc_IOError;
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

PyObject * FileSystemError::getPyExceptionType() const
{
    return PyExc_IOError;
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

PyObject * BadFormatError::getPyExceptionType() const
{
    return PyExc_FC_BadFormatError;
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

MemoryException & MemoryException::operator=(const MemoryException &inst)
{
    Exception::operator = (inst);
    return *this;
}

#if defined (__GNUC__)
const char* MemoryException::what() const noexcept
{
    // call what() of Exception, not of std::bad_alloc
    return Exception::what();
}
#endif

PyObject * MemoryException::getPyExceptionType() const
{
    return PyExc_MemoryError;
}

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

PyObject *AccessViolation::getPyExceptionType() const
{
    return PyExc_OSError;
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

PyObject *AbnormalProgramTermination::getPyExceptionType() const
{
    return PyExc_InterruptedError;
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

PyObject *UnknownProgramOption::getPyExceptionType() const
{
    return PyExc_FC_UnknownProgramOption;
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

PyObject *TypeError::getPyExceptionType() const
{
    return PyExc_TypeError;
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

PyObject *ValueError::getPyExceptionType() const
{
    return PyExc_ValueError;
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

PyObject *IndexError::getPyExceptionType() const
{
    return PyExc_IndexError;
}

// ---------------------------------------------------------

NameError::NameError()
  : Exception()
{
}

NameError::NameError(const char * sMessage)
  : Exception(sMessage)
{
}

NameError::NameError(const std::string& sMessage)
  : Exception(sMessage)
{
}

PyObject *NameError::getPyExceptionType() const
{
    return PyExc_NameError;
}

// ---------------------------------------------------------

ImportError::ImportError()
  : Exception()
{
}

ImportError::ImportError(const char * sMessage)
  : Exception(sMessage)
{
}

ImportError::ImportError(const std::string& sMessage)
  : Exception(sMessage)
{
}

PyObject *ImportError::getPyExceptionType() const
{
    return PyExc_ImportError;
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

PyObject *AttributeError::getPyExceptionType() const
{
    return PyExc_AttributeError;
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

PyObject *RuntimeError::getPyExceptionType() const
{
    return PyExc_RuntimeError;
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

PyObject *BadGraphError::getPyExceptionType() const
{
    return PyExc_FC_BadGraphError;
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

PyObject *NotImplementedError::getPyExceptionType() const
{
    return PyExc_NotImplementedError;
}

// ---------------------------------------------------------

ZeroDivisionError::ZeroDivisionError()
  : Exception()
{
}

ZeroDivisionError::ZeroDivisionError(const char * sMessage)
  : Exception(sMessage)
{
}

ZeroDivisionError::ZeroDivisionError(const std::string& sMessage)
  : Exception(sMessage)
{
}

PyObject *ZeroDivisionError::getPyExceptionType() const
{
    return PyExc_ZeroDivisionError;
}

// ---------------------------------------------------------

ReferenceError::ReferenceError()
: Exception()
{
}

ReferenceError::ReferenceError(const char * sMessage)
  : Exception(sMessage)
{
}

ReferenceError::ReferenceError(const std::string& sMessage)
  : Exception(sMessage)
{
}

PyObject *ReferenceError::getPyExceptionType() const
{
    return PyExc_ReferenceError;
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

PyObject * ExpressionError::getPyExceptionType() const
{
    return PyExc_FC_ExpressionError;
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

PyObject * ParserError::getPyExceptionType() const
{
    return PyExc_FC_ParserError;
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

PyObject *UnicodeError::getPyExceptionType() const
{
    return PyExc_UnicodeError;
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

PyObject *OverflowError::getPyExceptionType() const
{
    return PyExc_OverflowError;
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

PyObject *UnderflowError::getPyExceptionType() const
{
    return PyExc_ArithmeticError;
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

PyObject *UnitsMismatchError::getPyExceptionType() const
{
    return PyExc_ArithmeticError;
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

PyObject * CADKernelError::getPyExceptionType() const
{
    return PyExc_FC_CADKernelError;
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

PyObject * RestoreError::getPyExceptionType() const
{
    return PyExc_IOError;
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
    sigaction (SIGSEGV, &old_action, nullptr);
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

