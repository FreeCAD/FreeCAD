// SPDX-License-Identifier: LGPL-2.1-or-later
// SPDX-FileCopyrightText: 2002 Jürgen Riegel
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



#include <Base/Placement.h>

#include "Placement.h"

using namespace App;


PROPERTY_SOURCE(App::Placement, App::GeoFeature)


//===========================================================================
// Feature
//===========================================================================

Placement::Placement() = default;

Placement::~Placement() = default;


// Python feature ---------------------------------------------------------
namespace App
{
PROPERTY_SOURCE_TEMPLATE(App::PlacementPython, App::Placement)
template<>
const char* App::PlacementPython::getViewProviderName() const
{
    return "Gui::ViewProviderPlacementPython";
}
template class AppExport FeaturePythonT<App::Placement>;
}  // namespace App
