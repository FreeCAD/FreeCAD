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
#include "Quantity.h"


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
    /// return the active schema
    static UnitSystem getSchema(void){return actSystem;}

	static QString schemaTranslate(Base::Quantity quant,double &factor,QString &unitString);
    static QString schemaTranslate(Base::Quantity quant){ // to satisfy GCC
        double  dummy1;
        QString dummy2;
        return UnitsApi::schemaTranslate(quant,dummy1,dummy2);
    }
    /// generate a value for a quantity with default user prefered system
    static double toDbl(PyObject *ArgObj,const Base::Unit &u=Base::Unit());
    /// generate a value for a quantity with default user prefered system
    static Quantity toQuantity(PyObject *ArgObj,const Base::Unit &u=Base::Unit());

    // set the number of decimals
    static void setDecimals(int);
    // fet the number of decimals
    static int getDecimals();
    /// set the application defaults
    //static void setDefaults(void);
    //@}

    //double Result;

    // Python interface
    static PyMethodDef    Methods[];

    static double defaultFactor;


protected:
    // not used at the moment
    static UnitsSchema *  UserPrefSystem;
    static UnitSystem actSystem;
    /// number of decimals for floats
    static int      UserPrefDecimals;

    // do the real work
    //static double parse(const char*,bool &UsedUnit);

protected: // the python API wrapper methodes
    //static PyObject *sTranslateUnit   (PyObject *self,PyObject *args,PyObject *kwd);
    //static PyObject *sGetWithPrefs    (PyObject *self,PyObject *args,PyObject *kwd);
    static PyObject *sParseQuantity   (PyObject *self,PyObject *args,PyObject *kwd);
};

} // namespace Base


#endif // BASE_UNITSAPI_H
