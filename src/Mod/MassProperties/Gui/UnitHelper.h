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

#ifndef MASSPROPERTIES_UNITHELPER_H
#define MASSPROPERTIES_UNITHELPER_H

#include <string>
#include <vector>

namespace Base
{
class Quantity;
}

namespace MassPropertiesGui {

class UnitHelper {
public:
    static int getPreferred();
    static int getComboIndex(int schemaIndex);
    static int getSchemaIndex(int comboIndex, int preferredSchemaIndex);
    static std::string translate(const Base::Quantity& quantity, int schemaIndex);
};

} // namespace MassPropertiesGui

#endif // MASSPROPERTIES_UNITHELPER_H