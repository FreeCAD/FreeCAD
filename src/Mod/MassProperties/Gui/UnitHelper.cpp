// SPDX-License-Identifier: LGPL-2.1-or-later
// SPDX-FileNotice: Part of the FreeCAD project.

/******************************************************************************
 *                                                                            *
 *   © 2026 Morten Vajhøj                                                     *
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

#include "UnitHelper.h"

#include <algorithm>

#include <App/Application.h>
#include <App/Document.h>

#include <Base/Parameter.h>
#include <Base/Quantity.h>
#include <Base/UnitsApi.h>
#include <Base/UnitsSchema.h>

using namespace MassPropertiesGui;


int UnitHelper::getPreferred()
{
    auto params = App::GetApplication().GetParameterGroupByPath(
        "User parameter:BaseApp/Preferences/Units"
    );
    const bool ignoreProjectSchema = params->GetBool("IgnoreProjectSchema", false);
    int schemaIndex = params->GetInt("UserSchema", 0);

    if (!ignoreProjectSchema) {
        if (App::Document* doc = App::GetApplication().getActiveDocument()) {
            schemaIndex = doc->UnitSystem.getValue();
        }
    }

    if (schemaIndex < 0 || static_cast<std::size_t>(schemaIndex) >= Base::UnitsApi::count()) {
        schemaIndex = 0;
    }

    return schemaIndex;
}

int UnitHelper::getComboIndex(int schemaIndex)
{
    if (schemaIndex < 0) {
        return 0;
    }

    auto schema = Base::UnitsApi::createSchema(static_cast<std::size_t>(schemaIndex));
    const std::string lengthUnit = schema ? schema->getBasicLengthUnit() : "mm";

    if (lengthUnit == "mm") {
        return 0;
    }
    if (lengthUnit == "m") {
        return 1;
    }
    if (lengthUnit == "in") {
        return 2;
    }
    if (lengthUnit == "ft") {
        return 3;
    }
    return 0;
}

int UnitHelper::getSchemaIndex(int comboIndex, int preferredSchemaIndex)
{
    const auto names = Base::UnitsApi::getNames();

    switch (comboIndex) {
        case 1: {
            auto it = std::find(names.begin(), names.end(), "MKS");
            return it != names.end() ? static_cast<int>(std::distance(names.begin(), it)) : preferredSchemaIndex;
        }
        case 2: {
            auto it = std::find(names.begin(), names.end(), "Imperial");
            return it != names.end() ? static_cast<int>(std::distance(names.begin(), it)) : preferredSchemaIndex;
        }
        case 3: {
            auto it = std::find(names.begin(), names.end(), "ImperialCivil");
            return it != names.end() ? static_cast<int>(std::distance(names.begin(), it)) : preferredSchemaIndex;
        }
        default: {
            auto it = std::find(names.begin(), names.end(), "Internal");
            return it != names.end() ? static_cast<int>(std::distance(names.begin(), it)) : preferredSchemaIndex;
        }
    }
}

std::string UnitHelper::translate(const Base::Quantity& quantity, int schemaIndex)
{
    if (schemaIndex < 0) {
        return Base::UnitsApi::schemaTranslate(quantity);
    }

    auto schema = Base::UnitsApi::createSchema(static_cast<std::size_t>(schemaIndex));
    return schema ? schema->translate(quantity) : Base::UnitsApi::schemaTranslate(quantity);
}