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
#include <Mod/SheetMetalCore/SheetMetalCoreGlobal.h>

#include "SheetModel.h"


namespace SheetMetalCore
{

/**
 * Base class of the features that change the sheet model.
 *
 * Each feature holds the model that results from its operation. The shape of the feature is the
 * folded solid of that model.
 */
class SheetMetalCoreExport Feature: public Part::Feature
{
    PROPERTY_HEADER_WITH_OVERRIDE(SheetMetalCore::Feature);

public:
    Feature();

    App::PropertyBool Refine;

    App::DocumentObjectExecReturn* execute() override;

    /// The model after this feature. The function makes the model again if it is not available.
    std::shared_ptr<const SheetModel> getModel();

    const char* getViewProviderName() const override
    {
        return "PartGui::ViewProviderPart";
    }

protected:
    virtual std::shared_ptr<SheetModel> buildModel();
    static std::shared_ptr<const SheetModel> sourceModel(App::DocumentObject* object);

private:
    std::shared_ptr<const SheetModel> model;
};

/// The first panel of a sheet metal part, made from a closed planar profile.
class SheetMetalCoreExport BasePanel: public Feature
{
    PROPERTY_HEADER_WITH_OVERRIDE(SheetMetalCore::BasePanel);

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
 * A sheet metal part from its finished flat pattern.
 *
 * The bend lines are the center lines of the bend zones. They divide the profile into panels.
 */
class SheetMetalCoreExport FromFlat: public Feature
{
    PROPERTY_HEADER_WITH_OVERRIDE(SheetMetalCore::FromFlat);

public:
    FromFlat();

    App::PropertyLink Profile;
    App::PropertyLink BendLines;
    App::PropertyFloatList Angles;
    App::PropertyLink Rule;

protected:
    std::shared_ptr<SheetModel> buildModel() override;
};

/**
 * A sheet metal part from a folded solid that a different tool made.
 *
 * The feature reads the panels and the bends from the faces of the solid.
 */
class SheetMetalCoreExport FromSolid: public Feature
{
    PROPERTY_HEADER_WITH_OVERRIDE(SheetMetalCore::FromSolid);

public:
    FromSolid();

    App::PropertyLinkSub Face;
    App::PropertyLink Rule;

protected:
    std::shared_ptr<SheetModel> buildModel() override;
};

/// A bend and a rectangular panel on a straight edge.
class SheetMetalCoreExport Flange: public Feature
{
    PROPERTY_HEADER_WITH_OVERRIDE(SheetMetalCore::Flange);

public:
    Flange();

    App::PropertyLinkSub Edge;
    App::PropertyLength Length;
    App::PropertyAngle Angle;
    App::PropertyBool Reversed;
    App::PropertyLength Gap1;
    App::PropertyLength Gap2;

protected:
    std::shared_ptr<SheetModel> buildModel() override;
};

/// A cut through the sheet. The profile is in the plane of a panel or of the flat pattern.
class SheetMetalCoreExport Cut: public Feature
{
    PROPERTY_HEADER_WITH_OVERRIDE(SheetMetalCore::Cut);

public:
    Cut();

    App::PropertyLink Source;
    App::PropertyLink Profile;
    App::PropertyEnumeration Space;

protected:
    std::shared_ptr<SheetModel> buildModel() override;
};

/// The flat pattern of a sheet metal feature: one face and the bend center lines.
class SheetMetalCoreExport FlatPattern: public Part::Feature
{
    PROPERTY_HEADER_WITH_OVERRIDE(SheetMetalCore::FlatPattern);

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

}  // namespace SheetMetalCore
