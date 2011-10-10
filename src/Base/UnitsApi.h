/***************************************************************************
 *   Copyright (c) 2009 Juergen Riegel  (FreeCAD@juergen-riegel.net>              *
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

// (re-)defined in pyconfig.h
#if defined (_POSIX_C_SOURCE)
#   undef    _POSIX_C_SOURCE
#endif
#if defined (_XOPEN_SOURCE)
#   undef    _XOPEN_SOURCE
#endif

#include <string>
#include <Python.h>
#include <QString>
#include "UnitsSchema.h"

namespace Base {
    

/**
 * The UnitsApi
 */
class BaseExport UnitsApi 
{

public:
    /** Constructs a UnitsApi object. */
    UnitsApi(const char* filter);
    UnitsApi(const std::string& filter);
    virtual ~UnitsApi();

    /** set Schema
     * set the UnitsSchema of the Application
     * this a represented by a class of type UnitSchema which
     * defines a set of standard units for that schema and rules 
     * for representative strings.
     */
    static void setSchema(UnitSystem s);


    /// raw parser interface to calculat units (only from and to internal)
    static double translateUnit(const char*);
    static double translateUnit(const QString &);


    /** @name Translation from internal to user prefs */
    //@{
    /// generate a string (UTF-8) for a quantity in user prefered system
    static QString toStrWithUserPrefs(QuantityType t,double Value);
    /// generate a string for the value and the unit seperately for a quantity in user prefered system
    static void toStrWithUserPrefs(QuantityType t,double Value,QString &outValue,QString &outUnit);
    /// generate a python for a quantity in user prefered system
    static PyObject *toPyWithUserPrefs(QuantityType t,double Value);
    //@}

    /** @name Translation to internal regarding user prefs 
     * That means if no unit is issued the user prefs are in 
     * charge. If one unit is used the user prefs get ignored
     */
    //@{
    /// generate a value for a quantity with default user prefered system
    static double toDblWithUserPrefs(QuantityType t,const QString & Str);
    /// generate a value for a quantity with default user prefered system
    static double toDblWithUserPrefs(QuantityType t,const char* Str);
    /// generate a value for a quantity with default user prefered system
    static double toDblWithUserPrefs(QuantityType t,double UserVal);
    /// generate a value for a quantity with default user prefered system
    static double toDblWithUserPrefs(QuantityType t,PyObject *ArgObj);
    //@}

    /** @name query and set the user preferences */
    //@{
    /// set the default unit of a quantity type (e.g. m/s)
    static void setPrefOf(QuantityType t,const char* Str);
    /// get the default unit of a quantity (e.g. m/s)
    static const QString & getPrefUnitOf(QuantityType t);
    /// get the name of a quantity (e.g. lenth)
    static const QString getQuantityName(QuantityType t);
    /// get the translation factor for the default unit of a quantity
    static const double getPrefFactorOf(QuantityType t);
    /// set the application defaults
    static void setDefaults(void);
    //@}

    double Result;

    // Python interface
    static PyMethodDef    Methods[];


protected:
    // not used at the moment
    static UnitsSchema *  UserPrefSystem;

    /// cached factor to translate
    static double   UserPrefFactor [50] ;
    /// name of the unit the user wants to use as quantities
    static QString  UserPrefUnit   [50] ;

    // do the real work
    static double parse(const char*,bool &UsedUnit);

protected: // the python API wrapper methodes
    static PyObject *sTranslateUnit   (PyObject *self,PyObject *args,PyObject *kwd);
    static PyObject *sGetWithPrefs    (PyObject *self,PyObject *args,PyObject *kwd);
};

} // namespace Base


#endif // BASE_UNITSAPI_H
