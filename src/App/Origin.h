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


#pragma once

#include "Datums.h"

namespace App
{

/** Base class of all geometric document objects.
 */
class AppExport Origin: public App::LocalCoordinateSystem
{
    PROPERTY_HEADER_WITH_OVERRIDE(App::Origin);
    Q_DECLARE_TR_FUNCTIONS(App::Origin)

public:
    /// Constructor
    Origin();
    ~Origin() override;

    void onDocumentRestored() override;

    /// returns the type name of the ViewProvider
    const char* getViewProviderName() const override
    {
        return "Gui::ViewProviderCoordinateSystem";
    }

    bool isOrigin() const override
    {
        return true;
    }
};

}  // namespace App
