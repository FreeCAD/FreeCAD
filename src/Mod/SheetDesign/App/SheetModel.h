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

#include <vector>

#include <TopoDS_Edge.hxx>
#include <TopoDS_Face.hxx>
#include <TopoDS_Shape.hxx>
#include <gp_Dir2d.hxx>
#include <gp_Pnt2d.hxx>
#include <gp_Trsf.hxx>
#include <gp_Vec2d.hxx>

#include <Mod/SheetDesign/SheetDesignGlobal.h>


namespace SheetDesign
{

/// The values that apply to the full sheet.
struct SheetParams
{
    double thickness {1.0};
    double radius {1.0};   ///< Inner bend radius.
    double kFactor {0.5};  ///< ANSI K-factor: neutral axis position from the inner surface.
};

/**
 * A flat region of the sheet.
 *
 * The profile is a planar face in flat space (the plane z = 0). All panels use the same flat
 * space, thus the profiles of all panels and the strips of all bends make the flat pattern
 * without a transformation. The sheet material is between z = 0 and z = thickness.
 */
struct Panel
{
    TopoDS_Face profile;
    gp_Trsf folded;  ///< Maps flat space to the folded position.
    int parentBend {-1};
    mutable TopoDS_Shape solid;   ///< Prism of the profile in flat space. Empty = not made.
    mutable double volume {0.0};  ///< Volume of the prism.
};

/// A cylindrical bend that joins the straight edge of a parent panel to a child panel.
struct Bend
{
    int parentPanel {-1};
    int childPanel {-1};
    gp_Pnt2d start;  ///< Bend line on the parent edge, in flat space.
    gp_Pnt2d end;
    gp_Dir2d normal;     ///< In the flat plane, points from the parent into the bend.
    double angle {0.0};  ///< Radians, larger than 0.
    bool up {true};      ///< True: the bend goes to the +z side of flat space.
    double radius {0.0};
    double allowance {0.0};  ///< Width of the strip in the flat pattern.
    TopoDS_Face strip;       ///< The flat region of the bend.
};

/// A reference to a straight segment on the boundary of a panel.
struct EdgeRef
{
    int panel {-1};
    gp_Pnt2d a;
    gp_Pnt2d b;
    bool top {false};  ///< The source edge was on the z = thickness side.
};

/// A bend line of a flat pattern: the center line of the bend zone, in flat space.
struct BendLine
{
    gp_Pnt2d a;
    gp_Pnt2d b;
    double angle {0.0};  ///< Radians, larger than 0.
    bool up {true};
};

/**
 * The explicit sheet model: a tree of panels that bends join.
 *
 * The folded solid and the flat pattern are the two outputs of the same model. No operation
 * recognizes bends from a solid.
 */
class SheetDesignExport SheetModel
{
public:
    SheetParams params;
    std::vector<Panel> panels;
    std::vector<Bend> bends;

    /// Make a model with one panel. The profile must be a planar face in the plane z = 0.
    static SheetModel makeBase(
        const TopoDS_Face& flatProfile,
        const gp_Trsf& placement,
        const SheetParams& params
    );

    /**
     * Make a model from a finished flat pattern and its bend lines.
     *
     * The bend lines divide the profile into panels. The largest panel is the root panel.
     */
    static SheetModel fromFlat(
        const TopoDS_Face& flatProfile,
        const std::vector<BendLine>& lines,
        const gp_Trsf& placement,
        const SheetParams& params
    );

    /**
     * Make a model from a folded solid of constant thickness.
     *
     * The function reads the faces of one side of the sheet, from the root face through the
     * cylindrical bends. The thickness and the bend radii come from the geometry. The K-factor
     * and the radius for later bends come from the parameters.
     */
    static SheetModel fromSolid(
        const TopoDS_Shape& solid,
        const TopoDS_Face& rootFace,
        const SheetParams& params
    );

    /// Find the panel boundary segment that agrees with a straight edge of the folded solid.
    bool findEdge(const TopoDS_Edge& foldedEdge, EdgeRef& ref) const;

    /// Add a bend and a rectangular child panel. Returns the index of the new bend.
    /**
     * @param offset  Moves the bend line out of the parent panel before the bend. The parent
     *                panel becomes longer along the bend line. A negative value makes it
     *                shorter. Use one thickness for a flap that folds across a second flap.
     */
    int addFlange(
        const EdgeRef& ref,
        double length,
        double angle,
        bool up,
        double gap1,
        double gap2,
        double offset = 0.0
    );

    /**
     * Remove planar faces from the panels.
     *
     * @param faces  Shape with planar faces in folded space.
     * @param flat   True: the faces are in the plane of the flat pattern and cut all panels.
     *               False: each face cuts the panels that are in the same plane.
     * @return the number of panels that changed.
     */
    int cut(const TopoDS_Shape& faces, bool flat);

    /// The folded solid. With fuse = false, the result is a compound of solids.
    /**
     * @param overlap  If given, receives the volume of material that two or more panels or
     *                 bends have in the same space. Zero for a part that can exist.
     */
    TopoDS_Shape makeFolded(bool fuse, bool refine, double* overlap = nullptr) const;

    /// The flat pattern as one face in flat space.
    TopoDS_Shape makeFlat() const;

    /// The center lines of the bends in flat space.
    std::vector<TopoDS_Edge> bendLines() const;

    /// The placement of the flat pattern (the folded position of the root panel).
    gp_Trsf rootPlacement() const;

    double bendAllowance(double angle) const;
    double bendAllowance(double angle, double radius) const;

private:
    /// Add a bend between two panels and set the folded position of the child panel.
    void connect(
        int parent,
        int child,
        const gp_Pnt2d& start,
        const gp_Pnt2d& end,
        const gp_Vec2d& normal,
        double angle,
        bool up,
        double radius,
        const TopoDS_Face& strip
    );
};

}  // namespace SheetDesign
