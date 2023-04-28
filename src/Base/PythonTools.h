// SPDX-License-Identifier: LGPL-2.1-or-later

/***************************************************************************
 *   Copyright (c) 2023 Werner Mayer <wmayer[at]users.sourceforge.net>     *
 *                                                                         *
 *   This file is part of FreeCAD.                                         *
 *                                                                         *
 *   FreeCAD is free software: you can redistribute it and/or modify it    *
 *   under the terms of the GNU Lesser General Public License as           *
 *   published by the Free Software Foundation, either version 2.1 of the  *
 *   License, or (at your option) any later version.                       *
 *                                                                         *
 *   FreeCAD is distributed in the hope that it will be useful, but        *
 *   WITHOUT ANY WARRANTY; without even the implied warranty of            *
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU      *
 *   Lesser General Public License for more details.                       *
 *                                                                         *
 *   You should have received a copy of the GNU Lesser General Public      *
 *   License along with FreeCAD. If not, see                               *
 *   <https://www.gnu.org/licenses/>.                                      *
 *                                                                         *
 **************************************************************************/

#ifndef BASE_PYTHONTOOLS_H
#define BASE_PYTHONTOOLS_H

#include <string>
#include <FCGlobal.h>

using PyObject = struct _object;

namespace Base
{

class BaseExport PyTools
{
public:
    /*! Python helper function
     * This function encapsulatea the Decoding of UTF8 to a Python object
     * including exception handling.
     */
    static PyObject* asUnicodeObject(const char* str);
    static PyObject* asUnicodeObject(const std::string& str);

    static const char* asUTF8FromUnicode(PyObject* obj);

    static PyObject* runString(const char* str);
    static PyObject* runString(const char* str, PyObject* dict);
};

} // namespace Base

#endif // BASE_PYTHONTOOLS_H
