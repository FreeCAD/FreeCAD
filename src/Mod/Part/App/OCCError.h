/***************************************************************************
 *   Copyright (c) Sebastian Hoogen   (github@sebastianhoogen.de) 2014     *
 *                                                                         *
 *   This file is part of the FreeCAD CAx development system.              *
 *                                                                         *
 *   This library is free software; you can redistribute it and/or         *
 *   modify it under the terms of the GNU Library General Public           *
 *   License as published by the Free Software Foundation; either          *
 *   version 2 of the License, or (at your option) any later version.      *
 *                                                                         *
 *   This library  is distributed in the hope that it will be useful,      *
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of        *
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the         *
 *   GNU Library General Public License for more details.                  *
 *                                                                         *
 *   You should have received a copy of the GNU Library General Public     *
 *   License along with this library; see the file COPYING.LIB. If not,    *
 *   write to the Free Software Foundation, Inc., 59 Temple Place,         *
 *   Suite 330, Boston, MA  02111-1307, USA                                *
 *                                                                         *
 ***************************************************************************/

#ifndef _OCCError_h_
#define _OCCError_h_

# include <Standard_Version.hxx>
# include <Standard_Failure.hxx>
# include <Standard_AbortiveTransaction.hxx>
# include <Standard_ConstructionError.hxx>
# if OCC_VERSION_HEX >= 0x060500
# include <Standard_DefineException.hxx>
# endif
# include <Standard_DimensionError.hxx>
# include <Standard_DimensionMismatch.hxx>
# include <Standard_DivideByZero.hxx>
# include <Standard_DomainError.hxx>
# include <Standard_ImmutableObject.hxx>
# include <Standard_LicenseError.hxx>
# include <Standard_LicenseNotFound.hxx>
# include <Standard_MultiplyDefined.hxx>
# include <Standard_NegativeValue.hxx>
# include <Standard_NoMoreObject.hxx>
# include <Standard_NoSuchObject.hxx>
# include <Standard_NotImplemented.hxx>
# include <Standard_NullObject.hxx>
# include <Standard_NullValue.hxx>
# include <Standard_NumericError.hxx>
# include <Standard_OutOfMemory.hxx>
# include <Standard_OutOfRange.hxx>
# include <Standard_Overflow.hxx>
# include <Standard_ProgramError.hxx>
# include <Standard_RangeError.hxx>
# include <Standard_TooManyUsers.hxx>
# include <Standard_TypeMismatch.hxx>
# include <Standard_Underflow.hxx>

#include <Base/Console.h>
#include <Base/PyObjectBase.h>
#include <Base/Interpreter.h>
#include <Base/Exception.h>
#include <App/Application.h>

namespace Part {
PartExport extern PyObject* PartExceptionOCCError;
PartExport extern PyObject* PartExceptionOCCDomainError;
PartExport extern PyObject* PartExceptionOCCRangeError;
PartExport extern PyObject* PartExceptionOCCConstructionError;
PartExport extern PyObject* PartExceptionOCCDimensionError;


#define PY_TRY	try 

/// see docu of PY_TRY 
#  define PY_CATCH_OCC catch (Standard_Failure &e)                  \
    {                                                               \
        std::string str;                                            \
        Standard_CString msg = e.GetMessageString();                \
        str += typeid(e).name();                                    \
        str += " ";                                                 \
        if (msg) {str += msg;}                                      \
        else     {str += "No OCCT Exception Message";}              \
        Base::Console().Error(str.c_str());                         \
        Py_Error(Part::PartExceptionOCCError,str.c_str());          \
    }                                                               \
    PY_CATCH
} //namespace Part
#endif  // _OCCError_h_

