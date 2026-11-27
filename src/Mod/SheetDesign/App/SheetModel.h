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

#include <string>
#include <utility>
#include <vector>

#include <TopoDS_Edge.hxx>
#include <TopoDS_Face.hxx>
#include <TopoDS_Wire.hxx>
#include <TopoDS_Shape.hxx>
#include <gp_Dir2d.hxx>
#include <gp_Pnt.hxx>
#include <gp_Pnt2d.hxx>
#include <gp_Trsf.hxx>
#include <gp_Vec2d.hxx>

#include <Mod/SheetDesign/SheetDesignGlobal.h>


namespace SheetDesign
{

/// A relief: a cut into the parent panel at an end of a bend, where the flange has a gap.
struct Relief
{
    enum Type
    {
        None = 0,
        Rectangle = 1,
        Round = 2
    };
    Type type {None};
    double width {0.0};  ///< Along the bend line, into the gap.
    double depth {0.0};  ///< From the bend line into the parent panel.
};

/// The values that apply to the full sheet.
struct SheetParams
{
    double thickness {1.0};
    double radius {1.0};   ///< Inner bend radius.
    double kFactor {0.5};  ///< ANSI K-factor: neutral axis position from the inner surface.
    Relief relief;         ///< The relief for a flange that does not give its own.
    /// Optional table of K-factors: pairs of (bend radius / thickness, ANSI K-factor), in the
    /// sequence of the ratio. A bend uses the first row with a ratio that is not smaller than
    /// its own ratio. With no table, all bends use kFactor.
    std::vector<std::pair<double, double>> kTable;
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
    std::string tag;              ///< Stable name of the panel, from the feature that made it.
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
    TopoDS_Shape strip;      ///< The flat region of the bend: one face or more.
    bool plain {true};       ///< The strip is the full rectangle from the bend line.
    /// The bend closes a loop of panels. Its two panels have their position from other bends,
    /// and the bend has no solid.
    bool closing {false};
    gp_Pnt2d farStart;  ///< The far edge of a rectangular flange, for a further bend.
    gp_Pnt2d farEnd;
};

/// A formed region of a panel: a shape that a tool presses into the sheet.
struct FormedRegion
{
    int panel {-1};
    TopoDS_Face outline;  ///< The region in flat space. The flat pattern keeps its material.
    TopoDS_Shape solid;   ///< The formed sheet, in flat space coordinates.
    double volume {0.0};
    std::string tag;
};

/**
 * The origin of a straight boundary segment: the feature and the role that made it.
 *
 * A panel profile changes when a later feature cuts it or adds to it, and the sequence of its
 * edges then changes. The origin does not change, thus it gives a stable name for the edges
 * of the folded solid and of the flat pattern.
 */
struct Origin
{
    std::string tag;
    int panel {-1};  ///< -1: the origin applies to all panels.
    gp_Pnt2d a;      ///< In flat space.
    gp_Pnt2d b;
};

/// Stable names for elements of a shape: the index of the element (from 1) and its name.
struct ElementNames
{
    std::vector<std::pair<int, std::string>> edges;
    std::vector<std::pair<int, std::string>> faces;
};

/// The values of a flange that are not its length, its angle, or its direction.
struct FlangeOptions
{
    double gap1 {0.0};  ///< From the start of the edge to the start of the flange.
    double gap2 {0.0};
    /// Moves the bend line out of the parent panel before the bend. The parent panel becomes
    /// longer along the bend line. A negative value makes it shorter. Use one thickness for a
    /// flap that folds across a second flap.
    double offset {0.0};
    Relief relief;  ///< A cut at each end of the bend that has a gap.
    /// The angle of each side of the flange, in radians. A positive value makes the flange
    /// narrower to its far end. The side starts at the bend line.
    double miter1 {0.0};
    double miter2 {0.0};
    /// Cut this flange and a flange on an adjacent edge on the line between them, if the two
    /// overlap in the flat pattern.
    bool autoMiter {true};
    double miterGap {0.0};  ///< The distance between two flanges after an automatic miter.
    /// The outline of the flange, as a face in flat space that touches the edge. The part of
    /// the outline on the edge gives the bend line. Null: the flange is a rectangle.
    TopoDS_Face profile;
    double radius {-1.0};   ///< The inner radius of the bend. Negative: the radius of the rule.
    bool anyAngle {false};  ///< Permit a bend of more than 180 degrees, for a hem.
    /// Makes the panel of the flange wider than its bend, at the start and at the end of the
    /// edge. A closed corner uses this.
    double extend1 {0.0};
    double extend2 {0.0};
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
    std::vector<Origin> origins;
    std::vector<FormedRegion> forms;

    /// Make a model with one panel. The profile must be a planar face in the plane z = 0.
    static SheetModel makeBase(
        const TopoDS_Face& flatProfile,
        const gp_Trsf& placement,
        const SheetParams& params,
        const std::string& tag = "Base"
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
        const SheetParams& params,
        const std::string& tag = "Refold"
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
        const SheetParams& params,
        const std::string& tag = "Unfold"
    );

    /// Find the panel boundary segment that agrees with a straight edge of the folded solid.
    bool findEdge(const TopoDS_Edge& foldedEdge, EdgeRef& ref) const;

    /// Add a bend and a child panel on a straight edge. Returns the index of the new bend.
    int addFlange(
        const EdgeRef& ref,
        double length,
        double angle,
        bool up,
        const FlangeOptions& options = FlangeOptions(),
        const std::string& tag = "Flange"
    );

    /**
     * Remove planar faces from the panels.
     *
     * @param faces  Shape with planar faces in folded space.
     * @param flat   True: the faces are in the plane of the flat pattern and cut all panels.
     *               False: each face cuts the panels that are in the same plane.
     * @return the number of panels that changed.
     */
    int cut(const TopoDS_Shape& faces, bool flat, const std::string& tag = "Cut", bool wrapped = false);

    /// Move a straight edge of a panel out by a length. A negative length moves it in.
    void extend(const EdgeRef& ref, double length, double gap1, double gap2, const std::string& tag);

    /// Add faces to the panels that they touch. Each face is in the plane of a folded panel.
    int extendBy(const TopoDS_Shape& faces, const std::string& tag);

    /// Move a line on a folded panel into flat space. Returns false if no panel holds it.
    bool toFlat(const gp_Pnt& p, const gp_Pnt& q, gp_Pnt2d& a, gp_Pnt2d& b) const;

    /// Divide a panel on a line in flat space and put a bend there. The line is the center line
    /// of the bend zone, thus the flat pattern does not change. Returns the index of the bend.
    int fold(const gp_Pnt2d& a, const gp_Pnt2d& b, double angle, bool up, const std::string& tag);

    /// Cut a circle or a square at each corner where two bends of one panel meet. The size is
    /// the diameter or the side. Returns the number of corners.
    int cornerRelief(bool square, double size, const std::string& tag);

    /**
     * Make a model from flat faces of a solid that is not a sheet. The first face is the root.
     * Faces with a straight edge in common get a bend on that edge, as far as a tree from the
     * root needs them. The other common edges are rips.
     *
     * @param inside  True: the sheet is in the solid, and the faces are its outer side.
     * @param ripGap  The distance between two panels at a rip.
     */
    static SheetModel fromFaces(
        const TopoDS_Shape& solid,
        const std::vector<TopoDS_Face>& faces,
        const SheetParams& params,
        bool inside,
        double ripGap,
        const std::string& tag
    );

    /**
     * Press a tool into a panel. The base face of the tool is on the panel. The formed sheet is
     * a shell on the other faces of the tool; an open face gives an opening in that shell.
     */
    int addForm(
        const TopoDS_Shape& tool,
        const TopoDS_Face& baseFace,
        const std::vector<TopoDS_Face>& openFaces,
        const std::string& tag
    );

    /// The outlines of the formed regions in flat space.
    std::vector<TopoDS_Wire> formOutlines() const;

    /// The unit normal of a boundary segment that points out of its panel.
    gp_Vec2d edgeNormal(const EdgeRef& ref) const;

    /// The bend on a boundary segment of a panel, or -1.
    int bendAt(const EdgeRef& ref) const;

    /**
     * Cut slots along bend lines, with tabs between them, for a bend made by hand.
     *
     * @param which  Indexes of the bends. Empty: all bends.
     * @param width  The size of a slot across the bend. Zero: the width of the bend zone.
     */
    int perforate(
        const std::vector<int>& which,
        double cutLength,
        double tabLength,
        double width,
        const std::string& tag
    );

    /// The number of sharp folds that fromSolid replaced with a bend.
    int sharpFolds {0};

    /// The folded solid. With fuse = false, the result is a compound of solids.
    /**
     * @param overlap  If given, receives the volume of material that two or more panels or
     *                 bends have in the same space. Zero for a part that can exist.
     */
    TopoDS_Shape makeFolded(bool fuse, bool refine, double* overlap = nullptr) const;

    /**
     * The solid in a state between flat and folded, for a view of the fold.
     *
     * @param fractions  For each bend, the part of its angle that is done: 0 = flat, 1 = folded.
     *                   A bend with no value is folded.
     */
    TopoDS_Shape makeFoldedAt(
        const std::vector<double>& fractions,
        bool fuse,
        double* overlap = nullptr
    ) const;

    /// The flat pattern as one face in flat space.
    TopoDS_Shape makeFlat() const;

    /// The center lines of the bends in flat space.
    std::vector<TopoDS_Edge> bendLines() const;

    /**
     * Add a chain of bends and panels from the section of a contour.
     *
     * @param section  The points of an open line of straight segments, in the flat space of
     *                 the panel of the edge. The line is in a plane at 90 degrees to the edge.
     *                 It starts on the edge, on the top or the bottom side of the panel, and it
     *                 is the outline of that side of the sheet with sharp corners.
     * @return the index of the first new bend.
     */
    int addContour(
        const EdgeRef& ref,
        const std::vector<gp_Pnt>& section,
        const FlangeOptions& options = FlangeOptions(),
        const std::string& tag = "Flange"
    );

    /// The tag of the origin of a boundary segment. Empty if the segment has no origin.
    std::string segmentTag(int panel, const gp_Pnt2d& a, const gp_Pnt2d& b) const;

    /**
     * Stable names for the straight edges and the flat faces of the folded solid.
     *
     * An edge on the top or bottom side of a panel gets the tag of its boundary segment with
     * "_T" or "_B". A top or bottom face gets the tag of its panel. A side face gets the tag of
     * its segment with "_S".
     */
    ElementNames nameFolded(const TopoDS_Shape& folded) const;

    /// Stable names for the straight edges of the flat pattern face: the segment tag with "_F".
    ElementNames nameFlat(const TopoDS_Shape& flat) const;

    /// The stable name of the center line of a bend in the flat pattern.
    std::string bendLineName(std::size_t bend) const;

    /// The placement of the flat pattern (the folded position of the root panel).
    gp_Trsf rootPlacement() const;

    double bendAllowance(double angle) const;
    double bendAllowance(double angle, double radius) const;

    /// The K-factor for a bend with this inner radius, from the table or the single value.
    double kFactorFor(double radius) const;

private:
    /// Record an origin for each straight edge of a shape. The transformation moves the shape
    /// into flat space. The tag of an edge is the prefix and the index of the edge.
    void addOrigins(const TopoDS_Shape& shape, const gp_Trsf& toFlat, const std::string& prefix, int panel);

    /// True if a face in flat space is in the space of a different panel or of a bend zone.
    bool overlapsOthers(int panel, const TopoDS_Shape& patch) const;

    /// Set the folded position of all panels after a panel again.
    void place(int panel);

    /// The folded position of a panel after a bend, from the position of the panel before it.
    gp_Trsf afterBend(
        const gp_Trsf& parent,
        const gp_Pnt2d& start,
        const gp_Vec2d& normal,
        double angle,
        bool up,
        double radius,
        double knownAllowance = -1.0
    ) const;

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
        const TopoDS_Shape& strip
    );
};

}  // namespace SheetDesign
