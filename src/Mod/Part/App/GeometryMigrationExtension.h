/***************************************************************************
 *   Copyright (c) 2020 Abdullah Tahiri <abdullah.tahiri.yo@gmail.com>     *
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

#ifndef PART_GEOMETRYMIGRATIONEXTENSION_H
#define PART_GEOMETRYMIGRATIONEXTENSION_H

#include <bitset>
#include "Geometry.h"


namespace Part
{

// This is a light-weight c++ only geometry extension to enable migration of information that was stored within
// the Part WB and should be migrated to another WB
//
// It is designed so that a single extension can migrate different types of data.
//
// To migrate data:
// 1. Add an enum bit to indicate the type of migration type
// 2. Add the data members to store the information and accessors
class PartExport GeometryMigrationExtension : public Part::GeometryExtension
{
    TYPESYSTEM_HEADER_WITH_OVERRIDE();
public:

    // Indicates the type of migration to be performed, it is stored as a bitset, so several
    // migrations may take place in a single extension.
    // It is intended to support also LinkStage3 migration with a single framework (Id, Ref, ...)
    enum MigrationType {
            None                    = 0,
            Construction            = 1,
            NumMigrationType        // Must be the last
    };

    GeometryMigrationExtension() = default;
    ~GeometryMigrationExtension() override = default;

    std::unique_ptr<Part::GeometryExtension> copy() const override;

    PyObject *getPyObject() override;


    virtual bool getConstruction() const {return ConstructionState;}
    virtual void setConstruction(bool construction) {ConstructionState = construction;}

    virtual bool testMigrationType(int flag) const { return GeometryMigrationFlags.test((size_t)(flag)); };
    virtual void setMigrationType(int flag, bool v=true) { GeometryMigrationFlags.set((size_t)(flag), v); };

protected:
    void copyAttributes(Part::GeometryExtension * cpy) const override;

private:
    GeometryMigrationExtension(const GeometryMigrationExtension&) = default;

private:
    using MigrationTypeFlagType = std::bitset<32>;
    MigrationTypeFlagType           GeometryMigrationFlags;
    bool                            ConstructionState{false};

};

} //namespace Part


#endif // PART_GEOMETRYMIGRATIONEXTENSION_H
