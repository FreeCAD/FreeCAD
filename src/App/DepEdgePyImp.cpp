// SPDX-License-Identifier: LGPL-2.1-or-later
/****************************************************************************
 *                                                                          *
 *   Copyright (c) 2025 Pieter Hijma <info@pieterhijma.net>                 *
 *                                                                          *
 *   This file is part of FreeCAD.                                          *
 *                                                                          *
 *   FreeCAD is free software: you can redistribute it and/or modify it     *
 *   under the terms of the GNU Lesser General Public License as            *
 *   published by the Free Software Foundation, either version 2.1 of the   *
 *   License, or (at your option) any later version.                        *
 *                                                                          *
 *   FreeCAD is distributed in the hope that it will be useful, but         *
 *   WITHOUT ANY WARRANTY; without even the implied warranty of             *
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU       *
 *   Lesser General Public License for more details.                        *
 *                                                                          *
 *   You should have received a copy of the GNU Lesser General Public       *
 *   License along with FreeCAD. If not, see                                *
 *   <https://www.gnu.org/licenses/>.                                       *
 *                                                                          *
 ***************************************************************************/

#include <App/DepEdge.h>
#include <App/DocumentObject.h>

#include <App/DepEdgePy.h>
#include <App/DepEdgePy.cpp>

using namespace App;

std::string DepEdgePy::representation() const
{
    std::stringstream str;
    auto& [fromObj, fromProp, toObj, toProp] = *getDepEdgePtr();

    str << fromObj->getFullName() << "." << (fromProp.empty() ? "HEAD" : fromProp)
        << " --> "
        << toObj->getFullName() << "." << (toProp.empty() ? "HEAD" : toProp);
    return {str.str()};
}

Py::Object DepEdgePy::getFromObj() const
{
    return Py::Object(getDepEdgePtr()->fromObj->getPyObject(), true);
}

Py::String DepEdgePy::getFromProp() const
{
    return Py::String(getDepEdgePtr()->fromProp);
}

Py::Object DepEdgePy::getToObj() const
{
    return Py::Object(getDepEdgePtr()->toObj->getPyObject(), true);
}

Py::String DepEdgePy::getToProp() const
{
    return Py::String(getDepEdgePtr()->toProp);
}

PyObject* DepEdgePy::getCustomAttributes(const char* /*attr*/) const
{
    return nullptr;
}

int DepEdgePy::setCustomAttributes(const char* /*attr*/, PyObject* /*obj*/)
{
    return 0;
}
