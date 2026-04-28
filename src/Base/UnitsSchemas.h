// SPDX-License-Identifier: LGPL-2.1-or-later

/************************************************************************
 *                                                                      *
 *   This file is part of the FreeCAD CAx development system.           *
 *                                                                      *
 *   This library is free software; you can redistribute it and/or      *
 *   modify it under the terms of the GNU Library General Public        *
 *   License as published by the Free Software Foundation; either       *
 *   version 2 of the License, or (at your option) any later version.   *
 *                                                                      *
 *   This library  is distributed in the hope that it will be useful,   *
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of     *
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the      *
 *   GNU Library General Public License for more details.               *
 *                                                                      *
 *   You should have received a copy of the GNU Library General Public  *
 *   License along with this library; see the file COPYING.LIB. If not, *
 *   write to the Free Software Foundation, Inc., 59 Temple Place,      *
 *   Suite 330, Boston, MA  02111-1307, USA                             *
 *                                                                      *
 ************************************************************************/

#pragma once

#include <functional>
#include <string>
#include <vector>

#include "UnitsSchema.h"
#include "UnitsSchemasSpecs.h"

namespace Base
{

/**
 * The interface to schema specifications
 * Has pointer to current schema
 */
class UnitsSchemas
{
public:
    explicit UnitsSchemas(const UnitsSchemasDataPack& pack);

    /** Make a schema and set as current*/
    void select();  // default
    void select(const std::string_view& name);
    void select(std::size_t num);

    /** Get a schema specification*/
    UnitsSchemaSpec spec();  // default, or the first spec
    UnitsSchemaSpec spec(const std::string_view& name);
    UnitsSchemaSpec spec(std::size_t num);

    size_t count() const;
    std::vector<std::string> names();
    std::vector<std::string> descriptions();
    std::size_t getDecimals() const;
    std::size_t defFractDenominator() const;
    void setdefFractDenominator(std::size_t size);

    UnitsSchema* currentSchema() const;

private:
    /** DRY utils */
    std::vector<std::string> getVec(const std::function<std::string(UnitsSchemaSpec)>& fn);
    UnitsSchemaSpec findSpec(const std::function<bool(UnitsSchemaSpec)>& fn);
    void makeCurr(const UnitsSchemaSpec& spec);

    UnitsSchemasDataPack pack;
    std::unique_ptr<UnitsSchema> current {std::make_unique<UnitsSchema>(spec())};
    std::size_t denominator;
};


}  // namespace Base
