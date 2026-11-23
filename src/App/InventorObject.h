// SPDX-License-Identifier: LGPL-2.1-or-later
// SPDX-FileCopyrightText: 2007 Werner Mayer
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

#include "GeoFeature.h"
#include "PropertyStandard.h"


namespace App
{

class AppExport InventorObject: public GeoFeature
{
    PROPERTY_HEADER_WITH_OVERRIDE(App::InventorObject);

public:
    /// Constructor
    InventorObject();
    ~InventorObject() override;

    /// returns the type name of the ViewProvider
    const char* getViewProviderName() const override
    {
        return "Gui::ViewProviderInventorObject";
    }
    DocumentObjectExecReturn* execute() override
    {
        return DocumentObject::StdReturn;
    }
    short mustExecute() const override;
    PyObject* getPyObject() override;

    PropertyString Buffer;
    PropertyString FileName;
};

}  // namespace App
