/***************************************************************************
 *   Copyright (c) 2016 Stefan Tröger <stefantroeger@gmx.net>              *
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


#ifndef APP_EXTENSIONPYTHON_H
#define APP_EXTENSIONPYTHON_H

#include <Base/Interpreter.h>
#include "Extension.h"
#include "PropertyPythonObject.h"

namespace App {

/**
 * Generic Python extension class which allows every extension derived
 * class to behave as a Python extension -- simply by subclassing.
 */
template <class ExtensionT>
class ExtensionPythonT : public ExtensionT //NOLINT
{
    EXTENSION_PROPERTY_HEADER_WITH_OVERRIDE(App::ExtensionPythonT<ExtensionT>);

public:
    using Inherited = ExtensionT;

    ExtensionPythonT() {
        ExtensionT::m_isPythonExtension = true;
        ExtensionT::initExtensionType(ExtensionPythonT::getExtensionClassTypeId());
    }
    ~ExtensionPythonT() override = default;

    ExtensionPythonT(const ExtensionPythonT&) = delete;
    ExtensionPythonT(ExtensionPythonT&&) = delete;
    ExtensionPythonT& operator= (const ExtensionPythonT&) = delete;
    ExtensionPythonT& operator= (ExtensionPythonT&&) = delete;
};

using ExtensionPython = ExtensionPythonT<App::Extension>;

// Helper macros to define python extensions
#define EXTENSION_PROXY_FIRST(function) \
    Base::PyGILStateLocker lock;\
    Py::Object result;\
    try {\
        Property* proxy = this->getExtendedContainer()->getPropertyByName("Proxy");\
        if (proxy && proxy->getTypeId() == PropertyPythonObject::getClassTypeId()) {\
            Py::Object feature = static_cast<PropertyPythonObject*>(proxy)->getValue();\
            if (feature.hasAttr(std::string("function"))) {\
                if (feature.hasAttr("__object__")) {\
                    Py::Callable method(feature.getAttr(std::string("function")));




#define EXTENSION_PROXY_SECOND(function)\
                    result = method.apply(args);\
                }\
                else {\
                    Py::Callable method(feature.getAttr(std::string("function")));

#define EXTENSION_PROXY_THIRD()\
                    result = method.apply(args);\
                }\
            }\
        }\
    }\
    catch (Py::Exception&) {\
        Base::PyException e;\
        e.ReportException();\
    }

#define EXTENSION_PROXY_NOARG(function)\
    EXTENSION_PROXY_FIRST(function) \
    Py::Tuple args;\
    EXTENSION_PROXY_SECOND(function) \
    Py::Tuple args(1);\
    args.setItem(0, Py::Object(this->getExtensionPyObject(), true));\
    EXTENSION_PROXY_THIRD()

#define EXTENSION_PROXY_ONEARG(function, arg)\
    EXTENSION_PROXY_FIRST(function) \
    Py::Tuple args;\
    args.setItem(0, arg); \
    EXTENSION_PROXY_SECOND(function) \
    Py::Tuple args(2);\
    args.setItem(0, Py::Object(this->getExtensionPyObject(), true));\
    args.setItem(1, arg); \
    EXTENSION_PROXY_THIRD()

#define EXTENSION_PYTHON_OVERRIDE_VOID_NOARGS(function)\
    virtual void function() override {\
        EXTENSION_PROXY_NOARGS(function)\
    };

#define EXTENSION_PYTHON_OVERRIDE_OBJECT_NOARGS(function)\
    virtual PyObject* function() override {\
        EXTENSION_PROXY_NOARGS(function)\
        return res.ptr();\
    };

} //App

#endif // APP_EXTENSIONPYTHON_H
