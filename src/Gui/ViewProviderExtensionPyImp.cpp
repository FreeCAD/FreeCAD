/***************************************************************************
 *   Copyright (c) 2020 Werner Mayer <wmayer[at]users.sourceforge.net>     *
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
# include <sstream>
#endif

// inclusion of the generated files (generated out of PropertyContainerPy.xml)
#include "ViewProviderExtensionPy.h"
#include "ViewProviderExtensionPy.cpp"
#include "ViewProviderDocumentObject.h"

using namespace Gui;

// returns a string which represent the object e.g. when printed in python
std::string ViewProviderExtensionPy::representation() const
{
    return {"<view provider extension>"};
}

PyObject* ViewProviderExtensionPy::setIgnoreOverlayIcon(PyObject *args)
{
    PyObject* ignore;
    const char* name = nullptr;
    if (!PyArg_ParseTuple(args, "O!s", &PyBool_Type, &ignore, &name))
        return nullptr;

    ViewProviderExtension* ext = getViewProviderExtensionPtr();
    if (name) {
        Base::Type type = Base::Type::fromName(name);
        ext = dynamic_cast<ViewProviderExtension*>(ext->getExtendedContainer()->getExtension(type, true, true));
        if (!ext) {
            PyErr_SetString(PyExc_NameError, "no such extension");
            return nullptr;
        }
    }

    ext->setIgnoreOverlayIcon(Base::asBoolean(ignore));
    Py_Return;
}

PyObject* ViewProviderExtensionPy::ignoreOverlayIcon(PyObject *args)
{
    const char* name = nullptr;
    if (!PyArg_ParseTuple(args, "s", &name))
        return nullptr;

    ViewProviderExtension* ext = getViewProviderExtensionPtr();
    if (name) {
        Base::Type type = Base::Type::fromName(name);
        ext = dynamic_cast<ViewProviderExtension*>(ext->getExtendedContainer()->getExtension(type, true, true));
        if (!ext) {
            PyErr_SetString(PyExc_NameError, "no such extension");
            return nullptr;
        }
    }

    bool ignore = ext->ignoreOverlayIcon();
    return Py_BuildValue("O", (ignore ? Py_True : Py_False));
}

PyObject *ViewProviderExtensionPy::getCustomAttributes(const char* /*attr*/) const
{
    return nullptr;
}

int ViewProviderExtensionPy::setCustomAttributes(const char* /*attr*/, PyObject * /*obj*/)
{
    return 0;
}
