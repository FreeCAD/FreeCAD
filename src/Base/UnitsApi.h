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

#include <memory>
#include <QString>
#include <QCoreApplication>
#include "UnitsSchema.h"
#include "Quantity.h"

using PyObject = struct _object;
using PyMethodDef = struct PyMethodDef;

namespace Base {
using UnitsSchemaPtr = std::unique_ptr<UnitsSchema>;

/**
 * The UnitsApi
 */
class BaseExport UnitsApi
{
    Q_DECLARE_TR_FUNCTIONS(UnitsApi)

public:
    /** set Schema
     * set the UnitsSchema of the Application
     * this a represented by a class of type UnitSchema which
     * defines a set of standard units for that schema and rules
     * for representative strings.
     */
    static void setSchema(UnitSystem s);
    /// return the active schema
    static UnitSystem getSchema() {
        return currentSystem;
    }
    /// Returns a brief description of a schema
    static QString getDescription(UnitSystem);

    static QString schemaTranslate(const Base::Quantity& quant, double &factor, QString &unitString);
    static QString schemaTranslate(const Base::Quantity& quant) { // to satisfy GCC
        double  dummy1{};
        QString dummy2;
        return UnitsApi::schemaTranslate(quant, dummy1, dummy2);
    }

    /** Get a number as string for a quantity of a given format.
     * The string is a number in C locale (i.e. the decimal separator is always a dot) and if
     * needed represented in scientific notation. The string also includes the unit of the quantity.
     */
    static QString toString(const Base::Quantity& q, const QuantityFormat& f = QuantityFormat(QuantityFormat::Default));
    /** Get a number as string for a quantity of a given format.
     * The string is a number in C locale (i.e. the decimal separator is always a dot) and if
     * needed represented in scientific notation. The string doesn't include the unit of the quantity.
     */
    static QString toNumber(const Base::Quantity& q, const QuantityFormat& f = QuantityFormat(QuantityFormat::Default));
    /** Get a number as string for a double of a given format.
     * The string is a number in C locale (i.e. the decimal separator is always a dot) and if
     * needed represented in scientific notation. The string doesn't include the unit of the quantity.
     */
    static QString toNumber(double d, const QuantityFormat& f = QuantityFormat(QuantityFormat::Default));

    /// generate a value for a quantity with default user preferred system
    static double toDouble(PyObject* args, const Base::Unit& u = Base::Unit());
    /// generate a value for a quantity with default user preferred system
    static Quantity toQuantity(PyObject* args, const Base::Unit& u = Base::Unit());

    // set the number of decimals
    static void setDecimals(int);
    // get the number of decimals
    static int getDecimals();
    //@}

    //double Result;

    //return true if the current user schema uses multiple units for length (ex. Ft/In)
    static bool isMultiUnitLength();

    //return true if the current user schema uses multiple units for angles (ex. DMS)
    static bool isMultiUnitAngle();

    //return the basic unit of measure for length in the current user schema.
    static std::string getBasicLengthUnit();

    // Python interface
    static PyMethodDef    Methods[];

    /// return an instance of the given enum value
    static UnitsSchemaPtr createSchema(UnitSystem s);

protected:
    static UnitsSchemaPtr UserPrefSystem;
    static UnitSystem currentSystem;
    /// number of decimals for floats
    static int      UserPrefDecimals;

protected:
    // the python API wrapper methods
    static PyObject *sParseQuantity   (PyObject *self,PyObject *args);
    static PyObject *sListSchemas     (PyObject *self,PyObject *args);
    static PyObject *sGetSchema       (PyObject *self,PyObject *args);
    static PyObject *sSetSchema       (PyObject *self,PyObject *args);
    static PyObject *sSchemaTranslate (PyObject *self,PyObject *args);
    static PyObject *sToNumber        (PyObject *self,PyObject *args);
};

} // namespace Base


#endif // BASE_UNITSAPI_H
