/***************************************************************************
 *   Copyright (c) 2009 Werner Mayer <wmayer[at]users.sourceforge.net>     *
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


int createSWIGPointerObj_T(const char* TypeName, void* obj, PyObject** ptr, int own)
{
    swig_module_info *module = SWIG_GetModule(NULL);
    if (!module)
        return 1;

    swig_type_info * swig_type = 0;
    swig_type = SWIG_TypeQuery(TypeName);
    if (!swig_type)
        throw Base::Exception("Cannot find type information for requested type");
    
    *ptr = SWIG_NewPointerObj(obj,swig_type,own);
    if (*ptr == 0)
        throw Base::Exception("Cannot convert into requested type");

    // success
    return 0;
}

int convertSWIGPointerObj_T(const char* TypeName, PyObject* obj, void** ptr, int flags)
{
    swig_module_info *module = SWIG_GetModule(NULL);
    if (!module)
        return 1;

    swig_type_info * swig_type = 0;
    swig_type = SWIG_TypeQuery(TypeName);
    if (!swig_type)
        throw Base::Exception("Cannot find type information for requested type");

    // return value of 0 is on success
    if (SWIG_ConvertPtr(obj, ptr, swig_type, flags))
        throw Base::Exception("Cannot convert into requested type");

    // success
    return 0;
}

void cleanupSWIG_T(const char* TypeName)
{
    swig_module_info *swig_module = SWIG_GetModule(NULL);
    if (!swig_module)
        return;

    swig_type_info * swig_type = 0;
    swig_type = SWIG_TypeQuery(TypeName);
    if (!swig_type)
        return;

    PyObject *module, *dict;
    PyInterpreterState *interp = PyThreadState_GET()->interp;
    PyObject *modules = interp->modules;
    module = PyDict_GetItemString(modules, "__builtin__");
    if (module != NULL && PyModule_Check(module)) {
        dict = PyModule_GetDict(module);
        PyDict_SetItemString(dict, "_", Py_None);
    }

    module = PyDict_GetItemString(modules, "__main__");
    if (module != NULL && PyModule_Check(module)) {
        PyObject* dict = PyModule_GetDict(module);
        if (!dict) return;

        Py_ssize_t pos;
        PyObject *key, *value;
        pos = 0;
        while (PyDict_Next(dict, &pos, &key, &value)) {
            if (value != Py_None && PyString_Check(key)) {
                void* ptr = 0;
                if (SWIG_ConvertPtr(value, &ptr, 0, 0) == 0)
                    PyDict_SetItem(dict, key, Py_None);
            }
        }
    }

    // Run garbage collector
    PyGC_Collect();
}
