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
        // The Python-capable view provider: the edit logic of the features is in Python.
        return "PartDesignGui::ViewProviderPython";
    }

    /**
     * Find the boundary segment of a panel for an edge or a side face of the shape of this
     * feature. A task panel uses this to test a selection.
     *
     * @return false if the element is not a straight edge of a flat panel.
     */
    bool findEdge(const char* element, EdgeRef& ref);

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

/// The first panel of a sheet part, made from a closed planar profile. An open profile of
/// straight segments gives a part with one bend at each corner of the profile.
class SheetDesignExport BasePanel: public Feature
{
    PROPERTY_HEADER_WITH_OVERRIDE(SheetDesign::BasePanel);

public:
    BasePanel();

    App::PropertyLink Profile;
    /// For an open profile: the size of the part at 90 degrees to the plane of the profile.
    App::PropertyLength Width;
    App::PropertyBool Symmetric;

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

/// A bend and a panel on each selected straight edge. Flanges on adjacent edges can get a
/// closed corner.
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
    App::PropertyEnumeration CornerType;
    App::PropertyLength CornerGap;
    App::PropertyDistance Extend1;
    App::PropertyDistance Extend2;
    App::PropertyBool UseRuleRadius;
    App::PropertyLength Radius;
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

/// Moves a straight edge of a panel, or adds the outline of a sketch to a panel.
class SheetDesignExport Extend: public Feature
{
    PROPERTY_HEADER_WITH_OVERRIDE(SheetDesign::Extend);

public:
    Extend();

    /// The source feature and the edge to move. With a profile, the edge is not necessary.
    App::PropertyLinkSub Edge;
    App::PropertyLink Profile;
    App::PropertyDistance Length;
    App::PropertyLength Gap1;
    App::PropertyLength Gap2;

protected:
    std::shared_ptr<SheetModel> buildModel() override;
    App::DocumentObject* modelSource() const override
    {
        return Edge.getValue();
    }
};

/// A bend on a line across a panel. The flat pattern does not change.
class SheetDesignExport Fold: public Feature
{
    PROPERTY_HEADER_WITH_OVERRIDE(SheetDesign::Fold);

public:
    Fold();

    App::PropertyLink Source;
    App::PropertyLink BendLines;
    App::PropertyAngle Angle;
    App::PropertyBool Reversed;

protected:
    std::shared_ptr<SheetModel> buildModel() override;
    App::DocumentObject* modelSource() const override
    {
        return Source.getValue();
    }
};

/// A hem: the edge of the sheet folded back on the sheet.
class SheetDesignExport Hem: public Feature
{
    PROPERTY_HEADER_WITH_OVERRIDE(SheetDesign::Hem);

public:
    Hem();

    App::PropertyLinkSub Edge;
    App::PropertyEnumeration Type;
    App::PropertyLength Length;
    App::PropertyLength Gap;
    App::PropertyBool Reversed;

protected:
    std::shared_ptr<SheetModel> buildModel() override;
    App::DocumentObject* modelSource() const override
    {
        return Edge.getValue();
    }
};

/// A cut at each corner where two bends of one panel meet.
class SheetDesignExport CornerRelief: public Feature
{
    PROPERTY_HEADER_WITH_OVERRIDE(SheetDesign::CornerRelief);

public:
    CornerRelief();

    App::PropertyLink Source;
    App::PropertyEnumeration Type;
    App::PropertyLength Size;

protected:
    std::shared_ptr<SheetModel> buildModel() override;
    App::DocumentObject* modelSource() const override
    {
        return Source.getValue();
    }
};

/// A complete base part from sizes: a plate, an angle, a channel, a tub, a hat, or a box.
class SheetDesignExport BaseShape: public Feature
{
    PROPERTY_HEADER_WITH_OVERRIDE(SheetDesign::BaseShape);

public:
    BaseShape();

    App::PropertyEnumeration Type;
    App::PropertyLength Length;
    App::PropertyLength Width;
    App::PropertyLength Height;
    App::PropertyLength FlangeWidth;
    App::PropertyLink Rule;

protected:
    std::shared_ptr<SheetModel> buildModel() override;
};

/**
 * A sheet part from the faces of a solid that is not a sheet, for example a box.
 *
 * The selected faces become panels. The first face is the root.
 */
class SheetDesignExport Convert: public Feature
{
    PROPERTY_HEADER_WITH_OVERRIDE(SheetDesign::Convert);

public:
    Convert();

    App::PropertyLinkSub Faces;
    App::PropertyLink Rule;
    App::PropertyBool Inside;
    App::PropertyLength RipGap;

protected:
    std::shared_ptr<SheetModel> buildModel() override;
};

/// A region of a panel that a tool presses out of the plane of the sheet.
class SheetDesignExport Form: public Feature
{
    PROPERTY_HEADER_WITH_OVERRIDE(SheetDesign::Form);

public:
    Form();

    App::PropertyLink Source;
    /// The tool solid. The first face is its base face on the sheet. More faces are open.
    App::PropertyLinkSub Tool;

protected:
    std::shared_ptr<SheetModel> buildModel() override;
    App::DocumentObject* modelSource() const override
    {
        return Source.getValue();
    }
};

/// Slots along bend lines with tabs between them, for a bend made by hand.
class SheetDesignExport Perforation: public Feature
{
    PROPERTY_HEADER_WITH_OVERRIDE(SheetDesign::Perforation);

public:
    Perforation();

    /// The source feature and edges on the bends. With no edge: all bends.
    App::PropertyLinkSub Edges;
    App::PropertyLength CutLength;
    App::PropertyLength TabLength;
    App::PropertyLength Width;

protected:
    std::shared_ptr<SheetModel> buildModel() override;
    App::DocumentObject* modelSource() const override
    {
        return Edges.getValue();
    }
};

/// The flat pattern of a sheet design feature: one face and the bend center lines. A compound
/// with the outlines of the formed regions is the last part of the shape, if the part has one.
class SheetDesignExport FlatPattern: public Part::Feature
{
    PROPERTY_HEADER_WITH_OVERRIDE(SheetDesign::FlatPattern);

public:
    FlatPattern();

    App::PropertyLink Source;
    App::PropertyBool BendLines;
    /// The bends, in the sequence of the bend lines of the shape. A positive angle is a bend
    /// to the top side of the root panel (up), a negative angle is a bend down.
    App::PropertyFloatList BendAngles;
    App::PropertyFloatList BendRadii;
    /// The number of each bend in the sequence of the press brake. Empty: the model sequence.
    App::PropertyIntegerList BendOrder;

    App::DocumentObjectExecReturn* execute() override;

    const char* getViewProviderName() const override
    {
        return "PartGui::ViewProviderPython";
    }
};

}  // namespace SheetDesign
