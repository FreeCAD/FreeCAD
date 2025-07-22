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
#ifndef _PreComp_
#include <utility>
#endif

#include "Console.h"
#include "PyObjectBase.h"

#include "Exception.h"

FC_LOG_LEVEL_INIT("Exception", true, true)

using namespace Base;


TYPESYSTEM_SOURCE(Base::Exception, Base::BaseClass)
Exception::Exception(std::string message)
    : errorMessage {std::move(message)}
{}

Exception::Exception(const Exception& inst) = default;

Exception::Exception(Exception&& inst) noexcept = default;

Exception& Exception::operator=(const Exception& inst)
{
    errorMessage = inst.errorMessage;
    fileName = inst.fileName;
    lineNum = inst.lineNum;
    functionName = inst.functionName;
    isTranslatable = inst.isTranslatable;
    return *this;
}

Exception& Exception::operator=(Exception&& inst) noexcept
{
    errorMessage = std::move(inst.errorMessage);
    fileName = std::move(inst.fileName);
    lineNum = inst.lineNum;
    functionName = std::move(inst.functionName);
    isTranslatable = inst.isTranslatable;
    return *this;
}

const char* Exception::what() const noexcept
{
    return errorMessage.c_str();
}

void Exception::reportException() const
{
    if (hasBeenReported) {
        return;
    }

    std::string msg = errorMessage.empty() ? typeid(*this).name() : errorMessage;

#ifdef FC_DEBUG
    if (!functionName.empty()) {
        msg = functionName + " -- " + msg;
    }
#endif

    _FC_ERR(fileName.c_str(), lineNum, msg);
    hasBeenReported = true;
}

PyObject* Exception::getPyObject()
{
    Py::Dict edict;
    edict.setItem("sclassname", Py::String(typeid(*this).name()));
    edict.setItem("sErrMsg", Py::String(getMessage()));
    edict.setItem("sfile", Py::String(getFile()));
    edict.setItem("iline", Py::Long(getLine()));
    edict.setItem("sfunction", Py::String(getFunction()));
    edict.setItem("swhat", Py::String(what()));
    edict.setItem("btranslatable", Py::Boolean(getTranslatable()));
    edict.setItem("breported", Py::Boolean(hasBeenReported));
    return new_reference_to(edict);
}

void Exception::setPyObject(PyObject* pydict)
{
    try {
        if (pydict && Py::_Dict_Check(pydict)) {
            const Py::Dict edict(pydict);
            if (edict.hasKey("sfile")) {
                fileName = Py::String(edict.getItem("sfile"));
            }

            if (edict.hasKey("sfunction")) {
                functionName = Py::String(edict.getItem("sfunction"));
            }

            if (edict.hasKey("sErrMsg")) {
                errorMessage = Py::String(edict.getItem("sErrMsg"));
            }

            if (edict.hasKey("iline")) {
                lineNum = static_cast<int>(Py::Long(edict.getItem("iline")));
            }
            if (edict.hasKey("btranslatable")) {
                isTranslatable = static_cast<bool>(Py::Boolean(edict.getItem("btranslatable")));
            }
            if (edict.hasKey("breported")) {
                hasBeenReported = static_cast<bool>(Py::Boolean(edict.getItem("breported")));
            }
        }
    }
    catch (Py::Exception& e) {
        e.clear();  // should never happen
    }
}

PyObject* Exception::getPyExceptionType() const
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

TYPESYSTEM_SOURCE(Base::AbortException, Base::Exception)

AbortException::AbortException(const std::string& message)
    : Exception(message)
{}

const char* AbortException::what() const noexcept
{
    return Exception::what();
}

PyObject* AbortException::getPyExceptionType() const
{
    return PyExc_FC_FreeCADAbort;
}

// ---------------------------------------------------------

XMLBaseException::XMLBaseException(const std::string& message)
    : Exception(message)
{}

PyObject* XMLBaseException::getPyExceptionType() const
{
    return PyExc_FC_XMLBaseException;
}

// ---------------------------------------------------------

XMLParseException::XMLParseException(const std::string& message)
    : XMLBaseException(message)
{}

const char* XMLParseException::what() const noexcept
{
    return XMLBaseException::what();
}

PyObject* XMLParseException::getPyExceptionType() const
{
    return PyExc_FC_XMLParseException;
}

// ---------------------------------------------------------

XMLAttributeError::XMLAttributeError(const std::string& message)
    : XMLBaseException(message)
{}

const char* XMLAttributeError::what() const noexcept
{
    return XMLBaseException::what();
}

PyObject* XMLAttributeError::getPyExceptionType() const
{
    return PyExc_FC_XMLAttributeError;
}

// ---------------------------------------------------------

FileException::FileException(const std::string& message, const std::string& fileName)
    : Exception(message)
    , file(fileName)
{
    setFileName(fileName);
}

FileException::FileException(const std::string& message, const FileInfo& File)
    : Exception(message)
    , file(File)
{
    setFileName(File.filePath());
}

void FileException::setFileName(const std::string& fileName)
{
    file.setFile(fileName);
    _sErrMsgAndFileName = getMessage();
    if (!getFile().empty()) {
        _sErrMsgAndFileName += ": ";
        _sErrMsgAndFileName += fileName;
    }
}

std::string FileException::getFileName() const
{
    return file.fileName();
}

const char* FileException::what() const noexcept
{
    return _sErrMsgAndFileName.c_str();
}

void FileException::reportException() const
{
    if (getReported()) {
        return;
    }
    std::string msg = _sErrMsgAndFileName.empty() ? typeid(*this).name() : _sErrMsgAndFileName;

#ifdef FC_DEBUG
    if (!getFunction().empty()) {
        msg = getFunction() + " -- " + msg;
    }
#endif

    _FC_ERR(getFile().c_str(), getLine(), msg);
    setReported(true);
}

PyObject* FileException::getPyObject()
{
    Py::Dict edict(Exception::getPyObject(), true);
    edict.setItem("filename", Py::String(this->file.fileName()));
    return new_reference_to(edict);
}

void FileException::setPyObject(PyObject* pydict)
{
    if (pydict) {
        Exception::setPyObject(pydict);

        if (const Py::Dict edict(pydict); edict.hasKey("filename")) {
            setFileName(Py::String(edict.getItem("filename")).as_std_string("utf-8"));
        }
    }
}

PyObject* FileException::getPyExceptionType() const
{
    return PyExc_IOError;
}

// ---------------------------------------------------------

FileSystemError::FileSystemError(const std::string& message)
    : Exception(message)
{}

PyObject* FileSystemError::getPyExceptionType() const
{
    return PyExc_IOError;
}

// ---------------------------------------------------------

BadFormatError::BadFormatError(const std::string& message)
    : Exception(message)
{}

PyObject* BadFormatError::getPyExceptionType() const
{
    return PyExc_FC_BadFormatError;
}

// ---------------------------------------------------------

MemoryException::MemoryException(const std::string& message)
    : Exception(message)  // NOLINT(*-throw-keyword-missing)
{}

#if defined(__GNUC__)
const char* MemoryException::what() const noexcept
{
    return Exception::what();  // from Exception, not std::bad_alloc
}
#endif

PyObject* MemoryException::getPyExceptionType() const
{
    return PyExc_MemoryError;
}

// ---------------------------------------------------------

AccessViolation::AccessViolation(const std::string& message)
    : Exception(message)
{}

PyObject* AccessViolation::getPyExceptionType() const
{
    return PyExc_OSError;
}

// ---------------------------------------------------------

AbnormalProgramTermination::AbnormalProgramTermination(const std::string& message)
    : Exception(message)
{}

PyObject* AbnormalProgramTermination::getPyExceptionType() const
{
    return PyExc_InterruptedError;
}

// ---------------------------------------------------------

UnknownProgramOption::UnknownProgramOption(const std::string& message)
    : Exception(message)
{}

PyObject* UnknownProgramOption::getPyExceptionType() const
{
    return PyExc_FC_UnknownProgramOption;
}

// ---------------------------------------------------------

ProgramInformation::ProgramInformation(const std::string& message)
    : Exception(message)
{}

// ---------------------------------------------------------

TypeError::TypeError(const std::string& message)
    : Exception(message)
{}

PyObject* TypeError::getPyExceptionType() const
{
    return PyExc_TypeError;
}

// ---------------------------------------------------------

ValueError::ValueError(const std::string& message)
    : Exception(message)
{}

PyObject* ValueError::getPyExceptionType() const
{
    return PyExc_ValueError;
}

// ---------------------------------------------------------

IndexError::IndexError(const std::string& message)
    : Exception(message)
{}

PyObject* IndexError::getPyExceptionType() const
{
    return PyExc_IndexError;
}

// ---------------------------------------------------------

NameError::NameError(const std::string& message)
    : Exception(message)
{}

PyObject* NameError::getPyExceptionType() const
{
    return PyExc_NameError;
}

// ---------------------------------------------------------

ImportError::ImportError(const std::string& message)
    : Exception(message)
{}

PyObject* ImportError::getPyExceptionType() const
{
    return PyExc_ImportError;
}

// ---------------------------------------------------------

AttributeError::AttributeError(const std::string& message)
    : Exception(message)
{}

PyObject* AttributeError::getPyExceptionType() const
{
    return PyExc_AttributeError;
}

// ---------------------------------------------------------

PropertyError::PropertyError(const std::string& message)
    : AttributeError(message)
{}

PyObject* PropertyError::getPyExceptionType() const
{
    return PyExc_FC_PropertyError;
}

// ---------------------------------------------------------

RuntimeError::RuntimeError(const std::string& message)
    : Exception(message)
{}

PyObject* RuntimeError::getPyExceptionType() const
{
    return PyExc_RuntimeError;
}

// ---------------------------------------------------------

BadGraphError::BadGraphError(const std::string& message)
    : RuntimeError(message)
{}

PyObject* BadGraphError::getPyExceptionType() const
{
    return PyExc_FC_BadGraphError;
}

// ---------------------------------------------------------

NotImplementedError::NotImplementedError(const std::string& message)
    : Exception(message)
{}

PyObject* NotImplementedError::getPyExceptionType() const
{
    return PyExc_NotImplementedError;
}

// ---------------------------------------------------------

ZeroDivisionError::ZeroDivisionError(const std::string& message)
    : Exception(message)
{}

PyObject* ZeroDivisionError::getPyExceptionType() const
{
    return PyExc_ZeroDivisionError;
}

// ---------------------------------------------------------

ReferenceError::ReferenceError(const std::string& message)
    : Exception(message)
{}

PyObject* ReferenceError::getPyExceptionType() const
{
    return PyExc_ReferenceError;
}

// ---------------------------------------------------------

ExpressionError::ExpressionError(const std::string& message)
    : Exception(message)
{}

PyObject* ExpressionError::getPyExceptionType() const
{
    return PyExc_FC_ExpressionError;
}

// ---------------------------------------------------------

ParserError::ParserError(const std::string& message)
    : Exception(message)
{}

PyObject* ParserError::getPyExceptionType() const
{
    return PyExc_FC_ParserError;
}

// ---------------------------------------------------------

UnicodeError::UnicodeError(const std::string& message)
    : Exception(message)
{}

PyObject* UnicodeError::getPyExceptionType() const
{
    return PyExc_UnicodeError;
}

// ---------------------------------------------------------

OverflowError::OverflowError(const std::string& message)
    : Exception(message)
{}

PyObject* OverflowError::getPyExceptionType() const
{
    return PyExc_OverflowError;
}

// ---------------------------------------------------------

UnderflowError::UnderflowError(const std::string& message)
    : Exception(message)
{}

PyObject* UnderflowError::getPyExceptionType() const
{
    return PyExc_ArithmeticError;
}

// ---------------------------------------------------------

UnitsMismatchError::UnitsMismatchError(const std::string& message)
    : Exception(message)
{}

PyObject* UnitsMismatchError::getPyExceptionType() const
{
    return PyExc_ArithmeticError;
}

// ---------------------------------------------------------

CADKernelError::CADKernelError(const std::string& message)
    : Exception(message)
{}

PyObject* CADKernelError::getPyExceptionType() const
{
    return PyExc_FC_CADKernelError;
}

// ---------------------------------------------------------

RestoreError::RestoreError(const std::string& message)
    : Exception(message)
{}

PyObject* RestoreError::getPyExceptionType() const
{
    return PyExc_IOError;
}

// ---------------------------------------------------------

#if defined(__GNUC__) && defined(FC_OS_LINUX)
#include <stdexcept>
#include <iostream>
#include <csignal>

SignalException::SignalException()
{
    memset(&new_action, 0, sizeof(new_action));
    new_action.sa_handler = throw_signal;
    sigemptyset(&new_action.sa_mask);
    new_action.sa_flags = 0;
    ok = (sigaction(SIGSEGV, &new_action, &old_action) < 0);
#ifdef _DEBUG
    std::cout << "Set new signal handler" << std::endl;
#endif
}

SignalException::~SignalException()
{
    sigaction(SIGSEGV, &old_action, nullptr);
#ifdef _DEBUG
    std::cout << "Restore old signal handler" << std::endl;
#endif
}

void SignalException::throw_signal(const int signum)
{
    std::cerr << "SIGSEGV signal raised: " << signum << std::endl;
    throw std::runtime_error("throw_signal");
}
#endif
