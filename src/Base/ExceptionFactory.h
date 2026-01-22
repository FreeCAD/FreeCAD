// SPDX-License-Identifier: LGPL-2.1-or-later

/***************************************************************************
 *   Copyright (c) 2017 Abdullah Tahiri <abdullah.tahiri.yo@gmail.com>     *
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


#pragma once


#include "Factory.h"
#include <FCGlobal.h>
#include <typeinfo>

// Python stuff
using PyObject = struct _object;

namespace Base
{

struct PyExceptionData
{
    PyObject* pyexc {};
    std::string message;
    bool reported {};
};

/// Abstract base class of all exception producers
class BaseExport AbstractExceptionProducer: public AbstractProducer
{
public:
    AbstractExceptionProducer() = default;
    // just implement it
    void* Produce() const override
    {
        return nullptr;
    }
    virtual void raiseException(PyObject* pydict) const = 0;
    virtual void raiseExceptionByType(const PyExceptionData&) const = 0;
};

// --------------------------------------------------------------------

/** The ExceptionFactory */
class BaseExport ExceptionFactory: public Factory
{
public:
    static ExceptionFactory& Instance();
    static void Destruct();

    void raiseException(PyObject* pydict) const;
    void raiseExceptionByType(const PyExceptionData& data) const;

private:
    static ExceptionFactory* _pcSingleton;  // NOLINT

    ExceptionFactory() = default;
};

/* Producers */

template<class CLASS>
class ExceptionProducer: public AbstractExceptionProducer
{
public:
    ExceptionProducer()
    {
        CLASS cls;
        pyExcType = cls.getPyExceptionType();
        ExceptionFactory::Instance().AddProducer(typeid(CLASS).name(), this);
    }

    void raiseException(PyObject* pydict) const override
    {
        CLASS cls;
        cls.setPyObject(pydict);

        throw cls;
    }

    void raiseExceptionByType(const PyExceptionData& data) const override
    {
        if (pyExcType == data.pyexc) {
            CLASS cls;
            cls.setMessage(data.message);
            cls.setReported(data.reported);

            throw cls;
        }
    }

private:
    PyObject* pyExcType {};
};

}  // namespace Base
