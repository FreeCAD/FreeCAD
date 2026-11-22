// SPDX-License-Identifier: LGPL-2.1-or-later
// SPDX-FileCopyrightText: 2015 Stefan Tröger
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


#include <App/Document.h>
#include <Base/Placement.h>

#include "Origin.h"

using namespace App;

PROPERTY_SOURCE(App::Origin, App::LocalCoordinateSystem)

Origin::Origin()
{
    // App::Origin is a LCS for which placement is fixed to identity.
    Placement.setStatus(Property::Hidden, true);
    Placement.setStatus(Property::ReadOnly, true);
}

Origin::~Origin() = default;

void Origin::onDocumentRestored()
{
    LocalCoordinateSystem::onDocumentRestored();

    // Make existing origin placements read only.
    Placement.setStatus(Property::ReadOnly, true);
}
