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

#ifndef MASSPROPERTIES_UNITSYSTEM_H
#define MASSPROPERTIES_UNITSYSTEM_H

#include <string>

namespace MassPropertiesGui {

struct UnitConversions {
    double lengthFactor;
    double volumeFactor;
    double massFactor;
    double densityFactor;
    double areaFactor;
    double inertiaFactor;
    
    std::string lengthUnit;
    std::string volumeUnit;
    std::string massUnit;
    std::string densityUnit;
    std::string areaUnit;
    std::string inertiaUnit;
};

class UnitSystem {
public:
    static UnitConversions getConversions(const std::string& systemName);
    static std::string getPreferredSystemName();
    static int getSystemIndex(const std::string& systemName);
    
    static const char* metric_mm_kg;
    static const char* metric_m_kg;
    static const char* imperial_inch_lb;
    static const char* imperial_ft_lb;
};

} // namespace MassPropertiesGui

#endif // MASSPROPERTIES_UNITSYSTEM_H