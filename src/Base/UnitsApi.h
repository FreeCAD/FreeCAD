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

#ifndef BASE_UNITSAPI_H
#define BASE_UNITSAPI_H

#include "UnitsSchema.h"
#include "UnitsSchemas.h"
#include "UnitsSchemasData.h"
#include "Quantity.h"


class QString;

using PyObject = struct _object;
using PyMethodDef = struct PyMethodDef;

namespace Base
{


class BaseExport UnitsApi
{
public:
    static void init();
    static std::unique_ptr<UnitsSchema> createSchema(std::size_t num);
    static void setSchema(const std::string& name);
    static void setSchema(std::size_t num);

    static std::string
    schemaTranslate(const Quantity& quant, double& factor, std::string& unitString);

    static std::string schemaTranslate(const Quantity& quant);

    /**
     * toString & toNumber:
     * Quantity to string. Optionally apply format
     * The string is a number in C locale (i.e. the decimal separator is always a dot)
     * Scientific notation (if needed).
     */

    /** INCLUDES unit */
    static std::string
    toString(const Quantity& quantity,
             const QuantityFormat& format = QuantityFormat(QuantityFormat::Default));

    /** Does NOT include unit */
    static std::string
    toNumber(const Quantity& quantity,
             const QuantityFormat& format = QuantityFormat(QuantityFormat::Default));

    /** Does NOT include unit */
    static std::string
    toNumber(double value, const QuantityFormat& format = QuantityFormat(QuantityFormat::Default));

    static double toDouble(PyObject* args, const Base::Unit& u = Base::Unit());

    static void setDecimals(std::size_t);
    static std::size_t getDecimals();
    static std::size_t getDefDecimals();

    static std::vector<std::string> getDescriptions();
    static std::vector<std::string> getNames();

    static std::size_t count();

    static bool isMultiUnitAngle();
    static bool isMultiUnitLength();
    static std::string getBasicLengthUnit();
    static std::size_t getFractDenominator();

    static std::size_t getDefSchemaNum()
    {
        return schemas->spec().num;
    }
    // Python interface
    static PyMethodDef Methods[];

protected:
    static inline auto schemas =
        std::make_unique<UnitsSchemas>(UnitsSchemasData::unitSchemasDataPack);
    static inline std::size_t decimals {2};
    static inline std::size_t denominator {2};

    // the python API wrapper methods
    static PyObject* sParseQuantity(PyObject* self, PyObject* args);
    static PyObject* sListSchemas(PyObject* self, PyObject* args);
    static PyObject* sGetSchema(PyObject* self, PyObject* args);
    static PyObject* sSetSchema(PyObject* self, PyObject* args);
    static PyObject* sSchemaTranslate(PyObject* self, PyObject* args);
    static PyObject* sToNumber(PyObject* self, PyObject* args);
};

}  // namespace Base

#endif  // BASE_UNITSAPI_H
