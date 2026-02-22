// SPDX-License-Identifier: LGPL-2.1-or-later

/***************************************************************************
 *   Copyright (c) 2009 Jürgen Riegel <FreeCAD@juergen-riegel.net>         *
 *                                                                         *
 *   This file is part of the FreeCAD CAx development system.              *
 *                                                                         *
 *   This library is free software; you can redistribute it and/or         *
 *   modify it under the terms of the GNU Library General Public           *
 *   License as published by the Free Software Foundation; either          *
 *   version 2 of the License, or (at your option) any later version.      *
 *                                                                         *
 *   This library  is distributed in the hope that it will be useful,      *
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of        *
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the         *
 *   GNU Library General Public License for more details.                  *
 *                                                                         *
 *   You should have received a copy of the GNU Library General Public     *
 *   License along with this library; see the file COPYING.LIB. If not,    *
 *   write to the Free Software Foundation, Inc., 59 Temple Place,         *
 *   Suite 330, Boston, MA  02111-1307, USA                                *
 *                                                                         *
 ***************************************************************************/

#pragma once

#include <string>
#include <memory>

#include "UnitsSchemasSpecs.h"
#include "Base/Quantity.h"

namespace Base
{
class Quantity;

/**
 * An individual schema object
 */
class BaseExport UnitsSchema
{
public:
    explicit UnitsSchema(UnitsSchemaSpec spec);
    UnitsSchema() = delete;

    [[nodiscard]] bool isMultiUnitLength() const;
    [[nodiscard]] bool isMultiUnitAngle() const;
    [[nodiscard]] std::string getBasicLengthUnit() const;
    [[nodiscard]] std::string getName() const;
    [[nodiscard]] std::string getDescription() const;
    [[nodiscard]] int getNum() const;

    std::string translate(const Quantity& quant) const;
    std::string translate(const Quantity& quant, double& factor, std::string& unitString) const;

private:
    [[nodiscard]] static std::string toLocale(
        const Quantity& quant,
        double factor,
        const std::string& unitString
    );

    UnitsSchemaSpec spec;
};


}  // namespace Base
