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
# include <memory>
#endif

#include <CXX/Objects.hxx>
#include "Exception.h"
/// Here the FreeCAD includes sorted by Base,App,Gui......
#include "UnitsApi.h"
#include "Quantity.h"
#include "QuantityPy.h" 



using namespace Base;

//**************************************************************************
// Python stuff of UnitsApi

// UnitsApi Methods						// Methods structure
PyMethodDef UnitsApi::Methods[] = {
    //{"translateUnit",  (PyCFunction) UnitsApi::sTranslateUnit, METH_VARARGS,
    // "translateUnit(string) -> double\n\n"
    // "calculate a mathematical expression with units to a number. \n"
    // "can be used for simple unit translation like: \n"
    // " translateUnit('10m')\n"
    // " or for more complex espressions:\n"
    // " translateUnit('sin(pi)/50.0 m/s^2')\n"
    //},
    //{"getWithPrefs",  (PyCFunction) UnitsApi::sGetWithPrefs, METH_VARARGS,
    // "getWithPrefs(type,[string|float|int]) -> double\n\n"
    // "Translation to internal regarding user prefs \n"
    // " That means if no unit is issued the user prefs are in \n"
    // " charge. If one unit is used the user prefs get ignored\n"
    // " type can be: \n"
    // " Length  \n"  
    // " Area  \n"  
    // " Volume  \n"  
    // " Angle  \n" 
    // " TimeSpan  \n"
    // " Velocity  \n"
    // " Acceleration \n" 
    // " Mass   \n"
    // " Temperature \n"

    //},
    {"parseQuantity",  (PyCFunction) UnitsApi::sParseQuantity, METH_VARARGS,
     "parseQuantity(string) -> Base.Quantity()\n\n"
     "calculate a mathematical expression with units to a quantity object. \n"
     "can be used for simple unit translation like: \n"
     "parseQuantity('10m')\n"
     "or for more complex espressions:\n"
     "parseQuantity('sin(pi)/50.0 m/s^2')\n"
    },
    {"listSchemas",  (PyCFunction) UnitsApi::sListSchemas, METH_VARARGS,
     "listSchemas() -> a tuple of schemas\n\n"
     "listSchemas(int) -> description of the given schema\n\n"
    },
    {"getSchema",  (PyCFunction) UnitsApi::sGetSchema, METH_VARARGS,
     "getSchema() -> int\n\n"
     "The int is the position of the tuple returned by listSchemas"
    },
    {"setSchema",  (PyCFunction) UnitsApi::sSetSchema, METH_VARARGS,
     "setSchema(int) -> None\n\n"
     "Sets the current schema to the given number, if possible"
    },
    {"schemaTranslate",  (PyCFunction) UnitsApi::sSchemaTranslate, METH_VARARGS,
     "schemaTranslate(Quantity, int) -> tuple\n\n"
     "Translate a quantity to a given schema"
    },

    {NULL, NULL, 0, NULL}		/* Sentinel */
};

//PyObject* UnitsApi::sTranslateUnit(PyObject * /*self*/, PyObject *args)
//{
//    char *pstr;
//    if (!PyArg_ParseTuple(args, "s", &pstr))     // convert args: Python->C
//        return NULL;                             // NULL triggers exception
//    try {
//        return Py::new_reference_to(Py::Object(Py::Float(UnitsApi::translateUnit(pstr))));
//    }
//    catch (const Base::Exception& e) {
//        PyErr_Format(PyExc_IOError, "invalid unit expression %s: %s\n", pstr, e.what());
//        return 0L;
//    }
//    catch (const std::exception& e) {
//        PyErr_Format(PyExc_IOError, "invalid unit expression %s: %s\n", pstr, e.what());
//        return 0L;
//    }
//}
//
//PyObject* UnitsApi::sGetWithPrefs(PyObject * /*self*/, PyObject *args)
//{
//    char     *type;
//    PyObject *obj;
//    if (!PyArg_ParseTuple(args, "sO", &type,&obj))     // convert args: Python->C
//        return NULL;                                   // NULL triggers exception
//    try {
//        QuantityType t;
//        if(strcmp("Length",type)==0)
//            t = Length;
//        else{
//            PyErr_Format(PyExc_IOError, "invalid quantity type: %s!", type);
//            return 0L;
//        }
//
//        double result = toDblWithUserPrefs(t,obj);
//        return Py::new_reference_to(Py::Object(Py::Float(result)));
//    }
//    catch (const Base::Exception&) {
//        PyErr_Format(PyExc_IOError, "invalid unit expression \n");
//        return 0L;
//    }
//    catch (const std::exception&) {
//        PyErr_Format(PyExc_IOError, "invalid unit expression \n");
//        return 0L;
//    }
//}

PyObject* UnitsApi::sParseQuantity(PyObject * /*self*/, PyObject *args)
{
    char *pstr;
    if (!PyArg_ParseTuple(args, "et", "utf-8", &pstr))     // convert args: Python->C
        return NULL;                             // NULL triggers exception

    Quantity rtn;
    QString qstr = QString::fromUtf8(pstr);
    PyMem_Free(pstr);
    try {
        rtn = Quantity::parse(qstr);
    }
    catch (const Base::Exception&) {
        PyErr_Format(PyExc_IOError, "invalid unit expression \n");
        return 0L;
    }
    catch (const std::exception&) {
        PyErr_Format(PyExc_IOError, "invalid unit expression \n");
        return 0L;
    }

    return new QuantityPy(new Quantity(rtn));
}

PyObject* UnitsApi::sListSchemas(PyObject * /*self*/, PyObject *args)
{
    if (PyArg_ParseTuple(args, "")) {
        int num = NumUnitSystemTypes;
        Py::Tuple tuple(num);
        for (int i=0; i<num; i++) {
            tuple.setItem(i, Py::String(UnitsApi::getDescription(static_cast<UnitSystem>(i))));
        }

        return Py::new_reference_to(tuple);
    }

    PyErr_Clear();
    int index;
    if (PyArg_ParseTuple(args, "i", &index)) {
        int num = NumUnitSystemTypes;
        if (index < 0 || index >= num) {
            PyErr_SetString(PyExc_ValueError, "invalid schema value");
            return 0;
        }

        return Py_BuildValue("s", UnitsApi::getDescription(static_cast<UnitSystem>(index)));
    }

    PyErr_SetString(PyExc_TypeError, "int or empty argument list expected");
    return 0;
}

PyObject* UnitsApi::sGetSchema(PyObject * /*self*/, PyObject *args)
{
    if (!PyArg_ParseTuple(args, ""))
        return NULL;

    return Py_BuildValue("i", static_cast<int>(actSystem));
}

PyObject* UnitsApi::sSetSchema(PyObject * /*self*/, PyObject *args)
{
    PyErr_Clear();
    int index;
    if (PyArg_ParseTuple(args, "i", &index)) {
        int num = NumUnitSystemTypes;
        if (index < 0 || index >= num) {
            PyErr_SetString(PyExc_ValueError, "invalid schema value");
            return 0;
        }
        setSchema((UnitSystem)index);
    }
    Py_Return;
}

PyObject* UnitsApi::sSchemaTranslate(PyObject * /*self*/, PyObject *args)
{
    PyObject* q;
    int index;
    if (!PyArg_ParseTuple(args, "O!i", &(QuantityPy::Type), &q, &index))
        return 0;

    Quantity quant;
    quant = *static_cast<Base::QuantityPy*>(q)->getQuantityPtr();

    std::unique_ptr<UnitsSchema> schema(createSchema(static_cast<UnitSystem>(index)));
    if (!schema.get()) {
        PyErr_SetString(PyExc_ValueError, "invalid schema value");
        return 0;
    }

    double factor;
    QString uus;
    QString uss = schema->schemaTranslate(quant, factor, uus);

    Py::Tuple res(3);
    res[0] = Py::String(uss.toUtf8(),"utf-8");
    res[1] = Py::Float(factor);
    res[2] = Py::String(uus.toUtf8(),"utf-8");

    return Py::new_reference_to(res);
}
