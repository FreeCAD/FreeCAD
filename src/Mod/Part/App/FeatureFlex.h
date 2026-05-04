// SPDX-License-Identifier: LGPL-2.1-or-later

/***************************************************************************
 *   Copyright (c) 2026 F. Foinant-Willig <flachyjoe@gmail.com>            *
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

#include <App/PropertyStandard.h>
#include <App/PropertyUnits.h>

#include <Mod/Part/PartGlobal.h>

#include "PartFeature.h"


namespace Part
{

class PartExport Flex: public Part::Feature
{
    PROPERTY_HEADER_WITH_OVERRIDE(Part::Flex);

public:
    Flex();

    App::PropertyLink Base;
    App::PropertyFloat Pitch;
    App::PropertyIntegerConstraint Samples;

    enum class FlexMode
    {
        Bend,
        Twist,
        Identity,
    };

    /**
     * @brief The FlexParameters struct is supposed to be filled with final
     * Flex parameters and be passed to FlexShape.
     */
    struct FlexParameters
    {
        double pitch {10.0};
        int samples {10};
        FlexMode mode {FlexMode::Twist};
    };

    /** @name methods override feature */
    //@{
    /// recalculate the feature
    App::DocumentObjectExecReturn* execute() override;
    short mustExecute() const override;
    /// returns the type name of the view provider
    const char* getViewProviderName() const override
    {
        return "PartGui::ViewProviderFlex";
    }
    //@}

    Flex::FlexParameters computeFinalParameters() const;

    /**
     * @brief FlexShape powers the deformation feature.
     * @param source: the shape to be deformed
     * @param params: deformation parameters
     * @return result of deformation
     */
    static TopoShape FlexShape(const TopoShape& source, const FlexParameters& params);

    static TopoShape curve(const TopoShape& source, const Flex::FlexParameters& params);
    static TopoShape twist(const TopoShape& source, const Flex::FlexParameters& params);
    static TopoShape identity(const TopoShape& source, const Flex::FlexParameters& params);

private:
    static App::PropertyIntegerConstraint::Constraints sampleRange;
};

}  // namespace Part
