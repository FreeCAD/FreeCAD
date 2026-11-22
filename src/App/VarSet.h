// SPDX-License-Identifier: LGPL-2.1-or-later
// SPDX-FileCopyrightText: 2024 Ondsel
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

#pragma once

#include "DocumentObject.h"

namespace App
{

/** A DocumentObject class with the purpose to store variables
 */
class AppExport VarSet: public App::DocumentObject
{
    PROPERTY_HEADER_WITH_OVERRIDE(App::VarSet);

public:
    VarSet() = default;
    ~VarSet() override = default;

    const char* getViewProviderName() const override;
};

}  // namespace App
