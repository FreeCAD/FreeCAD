// SPDX-License-Identifier: LGPL-2.1-or-later
// SPDX-FileCopyrightText: 2026 sliptonic <shopinthewoods@gmail.com>
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

#include <memory>

#include <App/DocumentObject.h>
#include <App/PropertyLinks.h>
#include <App/PropertyStandard.h>
#include <App/PropertyUnits.h>
#include <Mod/Part/App/PartFeature.h>
#include <Mod/PartDesign/App/Feature.h>
#include <Mod/SheetDesign/SheetDesignGlobal.h>

#include "SheetModel.h"


namespace SheetDesign
{

/**
 * Base class of the features that change the sheet model.
 *
 * Each feature holds the model that results from its operation. The shape of the feature is the
 * folded solid of that model.
 */
class SheetDesignExport Feature: public PartDesign::Feature
{
    PROPERTY_HEADER_WITH_OVERRIDE(SheetDesign::Feature);

public:
    Feature();

    App::PropertyBool Refine;
    App::PropertyVolume Overlap;

    App::DocumentObjectExecReturn* execute() override;

    /// The model after this feature. The function makes the model again if it is not available.
    std::shared_ptr<const SheetModel> getModel();

    const char* getViewProviderName() const override
    {
        return "PartDesignGui::ViewProvider";
    }

    /// The transformation from the coordinates of the container of an object into the
    /// coordinates of the container of this feature. A container is, for example, a Body.
    gp_Trsf toLocal(const App::DocumentObject* object) const;

protected:
    virtual std::shared_ptr<SheetModel> buildModel();

    /// The sheet design feature that this feature continues. Null for an Entry.
    virtual App::DocumentObject* modelSource() const
    {
        return nullptr;
    }
    static std::shared_ptr<const SheetModel> sourceModel(App::DocumentObject* object);

private:
    std::shared_ptr<const SheetModel> model;
};

/// The first panel of a sheet part, made from a closed planar profile.
class SheetDesignExport BasePanel: public Feature
{
    PROPERTY_HEADER_WITH_OVERRIDE(SheetDesign::BasePanel);

public:
    BasePanel();

    App::PropertyLink Profile;

    /**
     * The object with the values for the full part, usually an App::VarSet. It must have the
     * properties Thickness, BendRadius, and KFactor.
     */
    App::PropertyLink Rule;

protected:
    std::shared_ptr<SheetModel> buildModel() override;
};

/**
 * A sheet part from its finished flat pattern.
 *
 * The bend lines are the center lines of the bend zones. They divide the profile into panels.
 */
class SheetDesignExport Refold: public Feature
{
    PROPERTY_HEADER_WITH_OVERRIDE(SheetDesign::Refold);

public:
    Refold();

    App::PropertyLink Profile;
    App::PropertyLink BendLines;
    App::PropertyFloatList Angles;
    App::PropertyLink Rule;

protected:
    std::shared_ptr<SheetModel> buildModel() override;
};

/**
 * A sheet part from a folded solid that a different tool made.
 *
 * The feature reads the panels and the bends from the faces of the solid.
 */
class SheetDesignExport Unfold: public Feature
{
    PROPERTY_HEADER_WITH_OVERRIDE(SheetDesign::Unfold);

public:
    Unfold();

    App::PropertyLinkSub Face;
    App::PropertyLink Rule;

protected:
    std::shared_ptr<SheetModel> buildModel() override;
};

/// A bend and a rectangular panel on a straight edge.
class SheetDesignExport Flange: public Feature
{
    PROPERTY_HEADER_WITH_OVERRIDE(SheetDesign::Flange);

public:
    Flange();

    App::PropertyLinkSub Edge;
    App::PropertyLink Profile;
    App::PropertyLength Length;
    App::PropertyAngle Angle;
    App::PropertyBool Reversed;
    App::PropertyLength Gap1;
    App::PropertyLength Gap2;
    App::PropertyDistance Offset;
    App::PropertyEnumeration Position;
    App::PropertyEnumeration LengthType;
    App::PropertyEnumeration ReliefType;
    App::PropertyLength ReliefWidth;
    App::PropertyLength ReliefDepth;
    App::PropertyAngle Miter1;
    App::PropertyAngle Miter2;
    App::PropertyBool AutoMiter;
    App::PropertyLength MiterGap;

protected:
    std::shared_ptr<SheetModel> buildModel() override;
    App::DocumentObject* modelSource() const override
    {
        return Edge.getValue();
    }
};

/// A cut through the sheet. The profile is in the plane of a panel or of the flat pattern.
class SheetDesignExport Cut: public Feature
{
    PROPERTY_HEADER_WITH_OVERRIDE(SheetDesign::Cut);

public:
    Cut();

    App::PropertyLink Source;
    App::PropertyLink Profile;
    App::PropertyEnumeration Space;

protected:
    std::shared_ptr<SheetModel> buildModel() override;
    App::DocumentObject* modelSource() const override
    {
        return Source.getValue();
    }
};

/// The flat pattern of a sheet design feature: one face and the bend center lines.
class SheetDesignExport FlatPattern: public Part::Feature
{
    PROPERTY_HEADER_WITH_OVERRIDE(SheetDesign::FlatPattern);

public:
    FlatPattern();

    App::PropertyLink Source;
    App::PropertyBool BendLines;

    App::DocumentObjectExecReturn* execute() override;

    const char* getViewProviderName() const override
    {
        return "PartGui::ViewProviderPart";
    }
};

}  // namespace SheetDesign
