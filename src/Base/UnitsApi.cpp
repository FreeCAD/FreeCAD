// SPDX-License-Identifier: LGPL-2.1-or-later

/***************************************************************************
 *   Copyright (c) 2009 Jürgen Riegel <FreeCAD@juergen-riegel.net>         *
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

#include <iomanip>

#include <CXX/WrapPython.h>

#include "Exception.h"
#include "UnitsApi.h"
#include "UnitsSchema.h"
#include "UnitsSchemas.h"
#include "UnitsSchemasData.h"

using Base::UnitsApi;
using Base::UnitsSchema;
using Base::UnitsSchemas;

std::vector<std::string> UnitsApi::getDescriptions()
{
    return schemas->descriptions();
}

std::vector<std::string> UnitsApi::getNames()
{
    return schemas->names();
}

std::size_t UnitsApi::count()
{
    return static_cast<int>(schemas->count());
}

bool UnitsApi::isMultiUnitAngle()
{
    return schemas->currentSchema()->isMultiUnitAngle();
}

bool UnitsApi::isMultiUnitLength()
{
    return schemas->currentSchema()->isMultiUnitLength();
}

std::string UnitsApi::getBasicLengthUnit()
{
    return schemas->currentSchema()->getBasicLengthUnit();
}

void UnitsApi::setDecimals(const int prec)
{
    decimals = prec;
}

int UnitsApi::getDecimals()
{
    return decimals < 0 ? schemas->getDecimals() : decimals;
}

void UnitsApi::setDenominator(int frac)
{
    denominator = frac;
}

int UnitsApi::getDenominator()
{
    return denominator < 0 ? schemas->defFractDenominator() : denominator;
}

std::unique_ptr<UnitsSchema> UnitsApi::createSchema(const std::size_t num)
{
    return std::make_unique<UnitsSchema>(schemas->spec(num));
}

void UnitsApi::setSchema(const std::string& name)
{
    schemas->select(name);
}

void UnitsApi::setSchema(const size_t num)
{
    schemas->select(num);
}

double UnitsApi::toDouble(PyObject* args, const Base::Unit& u)
{
    if (PyUnicode_Check(args)) {
        std::string str(PyUnicode_AsUTF8(args));
        // Parse the string
        Quantity q = Quantity::parse(str);
        if (q.getUnit() == u) {
            return q.getValue();
        }
        throw Base::UnitsMismatchError("Wrong unit type!");
    }

    if (PyFloat_Check(args)) {
        return PyFloat_AsDouble(args);
    }
    if (PyLong_Check(args)) {
        return static_cast<double>(PyLong_AsLong(args));
    }

    throw Base::UnitsMismatchError("Wrong parameter type!");
}

std::string UnitsApi::schemaTranslate(const Quantity& quant, double& factor, std::string& unitString)
{
    return schemas->currentSchema()->translate(quant, factor, unitString);
}

std::string UnitsApi::schemaTranslate(const Quantity& quant)
{
    double dummy1 {};  // to satisfy GCC
    std::string dummy2;
    return schemas->currentSchema()->translate(quant, dummy1, dummy2);
}
