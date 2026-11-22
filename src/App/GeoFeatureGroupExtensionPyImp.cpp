// SPDX-License-Identifier: LGPL-2.1-or-later
// SPDX-FileCopyrightText: 2014 Jürgen Riegel
// SPDX-FileCopyrightText: 2015 Alexander Golubev
// SPDX-FileNotice: Part of the FreeCAD project.

/******************************************************************************
 *                                                                            *
 *   FreeCAD is free software: you can redistribute it and/or modify          *
 *   it under the terms of the GNU Lesser General Public License as           *
 *   published by the Free Software Foundation, either version 2.1            *
 *   of the License, or (at your option) any later version.                   *
 *                                                                            *
 *   FreeCAD is distributed in the hope that it will be useful,               *
 *   but WITHOUT ANY WARRANTY; without even the implied warranty              *
 *   of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.                  *
 *   See the GNU Lesser General Public License for more details.              *
 *                                                                            *
 *   You should have received a copy of the GNU Lesser General Public         *
 *   License along with FreeCAD. If not, see https://www.gnu.org/licenses     *
 *                                                                            *
 ******************************************************************************/



// inclusion of the generated files (generated out of GeoFeatureGroupExtensionPy.xml)
#include "GeoFeatureGroupExtensionPy.h"
#include "GeoFeatureGroupExtensionPy.cpp"

using namespace App;

// returns a string which represents the object e.g. when printed in python
std::string GeoFeatureGroupExtensionPy::representation() const
{
    return {"<GeoFeatureGroup object>"};
}

PyObject* GeoFeatureGroupExtensionPy::getCustomAttributes(const char* /*attr*/) const
{
    return nullptr;
}

int GeoFeatureGroupExtensionPy::setCustomAttributes(const char* /*attr*/, PyObject* /*obj*/)
{
    return 0;
}
