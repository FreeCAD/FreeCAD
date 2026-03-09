// SPDX-License-Identifier: LGPL-2.1-or-later

/***************************************************************************
 *   Copyright (c) 2010 Juergen Riegel <FreeCAD@juergen-riegel.net>        *
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

#include <App/PropertyUnits.h>
#include "FeatureSketchBased.h"

namespace PartDesign
{

enum FuseOrder : std::uint8_t
{
    BaseFirst,
    FeatureFirst,
};

class PartDesignExport Revolution: public ProfileBased
{
    PROPERTY_HEADER_WITH_OVERRIDE(PartDesign::Revolution);

public:
    Revolution();

    App::PropertyEnumeration Type;
    App::PropertyVector Base;
    App::PropertyVector Axis;
    App::PropertyAngle Angle;
    App::PropertyAngle Angle2;

    /** if this property is set to a valid link, both Axis and Base properties
     *  are calculated according to the linked line
     */
    App::PropertyLinkSub ReferenceAxis;

    /**
     * Compatibility property that is required to preserve behavior from 1.0, that while incorrect
     * may have an impact over user files.
     */
    App::PropertyEnumeration FuseOrder;

    /** @name methods override feature */
    //@{
    /** Recalculate the feature
     * Revolves the Sketch around the given Axis (with basepoint Base)
     * The angle of the revolution is given by Angle.
     * If Midplane is true, then the revolution will extend for half of Angle on both sides of the
     * sketch plane. If Reversed is true then the direction of revolution will be reversed. The
     * created material will be fused with the sketch support (if there is one)
     */
    App::DocumentObjectExecReturn* execute() override;
    short mustExecute() const override;
    /// returns the type name of the view provider
    const char* getViewProviderName() const override
    {
        return "PartDesignGui::ViewProviderRevolution";
    }
    //@}

    void Restore(Base::XMLReader& reader) override;

    /// suggests a value for Reversed flag so that material is always added to the support
    bool suggestReversed();

    enum class RevolMethod
    {
        Angle,
        ThroughAll,
        ToLast = ThroughAll,
        ToFirst,
        ToFace,
        TwoAngles
    };

protected:
    /// updates Axis from ReferenceAxis
    void updateAxis();

    static const App::PropertyAngle::Constraints floatAngle;

    // See BRepFeat_MakeRevol
    enum RevolMode
    {
        CutFromBase = 0,
        FuseWithBase = 1,
        None = 2
    };

    RevolMethod methodFromString(const std::string& methodStr);

    /**
     * Generates a revolution of the input sketchshape and stores it in the given \a revol.
     */
    void generateRevolution(
        TopoShape& revol,
        const TopoShape& sketchshape,
        const gp_Ax1& ax1,
        const double angle,
        const double angle2,
        const bool midplane,
        const bool reversed,
        RevolMethod method
    );

    /**
     * Generates a revolution of the input \a profileshape.
     * It will be a stand-alone solid created with BRepFeat_MakeRevol.
     */
    void generateRevolution(
        TopoShape& revol,
        const TopoShape& baseshape,
        const TopoDS_Shape& profileshape,
        const TopoDS_Face& supportface,
        const TopoDS_Face& uptoface,
        const gp_Ax1& ax1,
        RevolMethod method,
        RevolMode Mode,
        Standard_Boolean Modify
    );

    /**
     * Disables settings that are not valid for the current method
     */
    void updateProperties(RevolMethod method);

private:
    static const char* TypeEnums[];
    static const char* FuseOrderEnums[];
};

}  // namespace PartDesign
