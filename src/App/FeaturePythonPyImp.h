/***************************************************************************
 *   Copyright (c) 2013 Werner Mayer <wmayer[at]users.sourceforge.net>     *
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

#ifndef APP_FEATUREPYTHONPYIMP_H
#define APP_FEATUREPYTHONPYIMP_H

#include <map>
#include <string>
#include <sstream>
#include <Base/Console.h>
#include <App/DocumentObjectPy.h>
#include <App/DocumentObject.h>
#include <App/DocumentPy.h>

namespace App
{

struct FeaturePythonClassInstance
{
    PyObject_HEAD
    PyObject *py_object;
};

/**
 * @author Werner Mayer
 */
template <class FeaturePyT>
class FeaturePythonPyT : public FeaturePyT
{
public:
    static PyTypeObject   Type;
    static PyMethodDef    Methods[];

    static PyObject *object_make(PyTypeObject *, PyObject *, PyObject *);
    static int object_init(PyObject* self, PyObject* args, PyObject*k);
    static void object_deallocator(PyObject *_self);
    static PyObject *getattro_handler(PyObject * obj, PyObject *attr);
    static int setattro_handler(PyObject *self, PyObject *name, PyObject *value);

public:
    FeaturePythonPyT(DocumentObject *pcObject, PyTypeObject *T = &Type);
    virtual ~FeaturePythonPyT();

    /** @name callbacks and implementers for the python object methods */
    //@{
    static PyObject* __getattr(PyObject * obj, char *attr);
    static  int __setattr(PyObject *PyObj, char *attr, PyObject *value);
    /// callback for the addProperty() method
    static PyObject * staticCallback_addProperty (PyObject *self, PyObject *args);
    /// implementer for the addProperty() method
    PyObject*  addProperty(PyObject *args);
    /// callback for the removeProperty() method
    static PyObject * staticCallback_removeProperty (PyObject *self, PyObject *args);
    /// implementer for the removeProperty() method
    PyObject*  removeProperty(PyObject *args);
    /// callback for the supportedProperties() method
    static PyObject * staticCallback_supportedProperties (PyObject *self, PyObject *args);
    /// implementer for the supportedProperties() method
    PyObject*  supportedProperties(PyObject *args);
    //@}

    /// getter method for special attributes (e.g. dynamic ones)
    PyObject *getCustomAttributes(const char* attr) const;
    /// setter for special attributes (e.g. dynamic ones)
    int setCustomAttributes(const char* attr, PyObject *obj);
    PyObject *_getattr(char *attr);              // __getattr__ function
    int _setattr(char *attr, PyObject *value);        // __setattr__ function
};

} //namespace App

#include "FeaturePythonPyImp.inl"

#endif // APP_FEATUREPYTHONPYIMP_H
