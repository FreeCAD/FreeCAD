// SPDX-License-Identifier: LGPL-2.1-or-later
// SPDX-FileNotice: Part of the FreeCAD project.

/***************************************************************************
 *   Copyright (c) 2016 Stefan Tröger <stefantroeger@gmx.net>              *
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

#include "ExtensionContainer.h"

// inclusion of the generated files (generated out of ExtensionPy.pyi)
#include <App/ExtensionPy.h>
#include <App/ExtensionPy.cpp>

using namespace App;

// returns a string which represent the object e.g. when printed in python
std::string ExtensionPy::representation() const
{
    return {"<extension>"};
}

Py::Object ExtensionPy::getExtendedObject() const
{
    return Py::Object(getExtensionPtr()->getExtendedContainer()->getPyObject(), true);
}

PyObject* ExtensionPy::getCustomAttributes(const char* /*attr*/) const
{
    return nullptr;
}

int ExtensionPy::setCustomAttributes(const char* /*attr*/, PyObject* /*obj*/)
{
    return 0;
}
