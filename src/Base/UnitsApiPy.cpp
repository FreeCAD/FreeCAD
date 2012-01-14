/***************************************************************************
 *   Copyright (c) Juergen Riegel         <juergen.riegel@web.de>          *
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


#include "PreCompiled.h"

#ifndef _PreComp_
# include <Python.h>
#endif

#include <CXX/Objects.hxx>
#include "Exception.h"
/// Here the FreeCAD includes sorted by Base,App,Gui......
#include "UnitsApi.h"



using namespace Base;

//**************************************************************************
// Python stuff of UnitsApi

// UnitsApi Methods						// Methods structure
PyMethodDef UnitsApi::Methods[] = {
    {"translateUnit",  (PyCFunction) UnitsApi::sTranslateUnit  ,1,
     "translateUnit(string) -> double\n\n"
     "calculate a mathematical expression with units to a number. \n"
     "can be used for simple unit translation like: \n"
     " translateUnit('10m')\n"
     " or for more complex espressions:\n"
     " translateUnit('sin(pi)/50.0 m/s^2')\n"
    },
    {"getWithPrefs",  (PyCFunction) UnitsApi::sGetWithPrefs  ,1,
     "getWithPrefs(type,[string|float|int]) -> double\n\n"
     "Translation to internal regarding user prefs \n"
     " That means if no unit is issued the user prefs are in \n"
     " charge. If one unit is used the user prefs get ignored\n"
     " type can be: \n"
     " Length  \n"  
     " Area  \n"  
     " Volume  \n"  
     " Angle  \n" 
     " TimeSpan  \n"
     " Velocity  \n"
     " Acceleration \n" 
     " Mass   \n"
     " Temperature \n"

    },

    {NULL, NULL, 0, NULL}		/* Sentinel */
};

PyObject* UnitsApi::sTranslateUnit(PyObject * /*self*/, PyObject *args,PyObject * /*kwd*/)
{
    char *pstr;
    if (!PyArg_ParseTuple(args, "s", &pstr))     // convert args: Python->C
        return NULL;                             // NULL triggers exception
    try {
        Py::Tuple res(10);
        UnitsSignature sig = UnitsApi::translateUnit(pstr);
        res.setItem(0,Py::Float(sig.Value));
        res.setItem(1,Py::Float(sig.Spread));
        res.setItem(2,Py::Int(sig.Length));
        res.setItem(3,Py::Int(sig.Mass));
        res.setItem(4,Py::Int(sig.Time));
        res.setItem(5,Py::Int(sig.ElectricCurrent));
        res.setItem(6,Py::Int(sig.ThermodynamicTemperature));
        res.setItem(7,Py::Int(sig.AmountOfSubstance));
        res.setItem(8,Py::Int(sig.LuminoseIntensity));
        res.setItem(9,Py::Int(sig.Angle));
        return Py::new_reference_to(res);
    }
    catch (const Base::Exception& e) {
        PyErr_Format(PyExc_IOError, "invalid unit expression %s: %s\n", pstr, e.what());
        return 0L;
    }
    catch (const std::exception& e) {
        PyErr_Format(PyExc_IOError, "invalid unit expression %s: %s\n", pstr, e.what());
        return 0L;
    }
}

PyObject* UnitsApi::sGetWithPrefs(PyObject * /*self*/, PyObject *args,PyObject * /*kwd*/)
{
    char     *type;
    PyObject *obj;
    if (!PyArg_ParseTuple(args, "sO", &type,&obj))     // convert args: Python->C
        return NULL;                                   // NULL triggers exception
    try {
        QuantityType t;
        if(strcmp("Length",type)==0)
            t = Length;
        else{
            PyErr_Format(PyExc_IOError, "invalid quantity type: %s!", type);
            return 0L;
        }

        double result = toDblWithUserPrefs(t,obj);
        return Py::new_reference_to(Py::Object(Py::Float(result)));
    }
    catch (const Base::Exception&) {
        PyErr_Format(PyExc_IOError, "invalid unit expression \n");
        return 0L;
    }
    catch (const std::exception&) {
        PyErr_Format(PyExc_IOError, "invalid unit expression \n");
        return 0L;
    }
}

