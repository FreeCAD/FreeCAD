// SPDX-License-Identifier: LGPL-2.1-or-later

/******************************************************************************
 *   Copyright (c) 2012 Jan Rheinländer <jrheinlaender@users.sourceforge.net> *
 *                                                                            *
 *   This file is part of the FreeCAD CAx development system.                 *
 *                                                                            *
 *   This library is free software; you can redistribute it and/or            *
 *   modify it under the terms of the GNU Library General Public              *
 *   License as published by the Free Software Foundation; either             *
 *   version 2 of the License, or (at your option) any later version.         *
 *                                                                            *
 *   This library  is distributed in the hope that it will be useful,         *
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of           *
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the            *
 *   GNU Library General Public License for more details.                     *
 *                                                                            *
 *   You should have received a copy of the GNU Library General Public        *
 *   License along with this library; see the file COPYING.LIB. If not,       *
 *   write to the Free Software Foundation, Inc., 59 Temple Place,            *
 *   Suite 330, Boston, MA  02111-1307, USA                                   *
 *                                                                            *
 ******************************************************************************/


#pragma once

#include <App/PropertyUnits.h>
#include <App/PropertyStandard.h>
#include <App/PropertyLinks.h>
#include <App/DocumentObjectExtension.h>
#include <gp_Vec.hxx>
#include <gp_Dir.hxx>

#include <Mod/Part/PartGlobal.h>

namespace Part
{

enum class LinearPatternMode
{
    Extent,
    Spacing
};

enum class LinearPatternDirection : std::uint8_t
{
    First,
    Second
};

class PartExport LinearPatternExtension: public App::DocumentObjectExtension
{
    EXTENSION_PROPERTY_HEADER_WITH_OVERRIDE(Part::LinearPatternExtension);

public:
    LinearPatternExtension();
    ~LinearPatternExtension() override = default;

    App::PropertyLinkSub Direction;
    App::PropertyBool Reversed;
    App::PropertyEnumeration Mode;
    App::PropertyLength Length;
    App::PropertyLength Offset;
    App::PropertyIntegerConstraint Occurrences;
    App::PropertyFloatList Spacings;
    App::PropertyFloatList SpacingPattern;

    App::PropertyLinkSub Direction2;
    App::PropertyBool Reversed2;
    App::PropertyEnumeration Mode2;
    App::PropertyLength Length2;
    App::PropertyLength Offset2;
    App::PropertyIntegerConstraint Occurrences2;
    App::PropertyFloatList Spacings2;
    App::PropertyFloatList SpacingPattern2;

    gp_Vec calculateOffsetVector(LinearPatternDirection dir) const;
    std::vector<gp_Vec> calculateSteps(LinearPatternDirection dir, const gp_Vec& offsetVector) const;

    // Virtual resolution of the direction vector
    virtual gp_Dir getDirectionFromProperty(const App::PropertyLinkSub& dirProp) const;

    const std::list<gp_Trsf> calculateTransformations();

    void setReadWriteStatusForMode(LinearPatternDirection dir);
    void syncLengthAndOffset(LinearPatternDirection dir);

    void updateSpacings();
    void updateSpacings(LinearPatternDirection dir);

    short extensionMustExecute() override;
    void extensionOnChanged(const App::Property* prop) override;

    static const App::PropertyIntegerConstraint::Constraints intOccurrences;
    static const char* ModeEnums[];
};

}  // namespace Part
