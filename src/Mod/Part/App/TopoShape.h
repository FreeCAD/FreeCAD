// SPDX-License-Identifier: LGPL-2.1-or-later

/***************************************************************************
 *   Copyright (c) 2002 Jürgen Riegel <juergen.riegel@web.de>              *
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

#include <iosfwd>
#include <list>
#include <unordered_map>

#include <App/ComplexGeoData.h>
#include <Base/Exception.h>
#include <Mod/Part/PartGlobal.h>

#include <TopoDS_Compound.hxx>
#include <TopoDS_Wire.hxx>
#include <TopTools_ListOfShape.hxx>
#include <BRepBuilderAPI_MakeShape.hxx>
#include <BRepBuilderAPI_Sewing.hxx>
#include <BRepOffsetAPI_ThruSections.hxx>
#include <BRepOffsetAPI_MakePipeShell.hxx>
#include <BRepFeat_MakePrism.hxx>
#include <BRepPrimAPI_MakeHalfSpace.hxx>
#include <BRepTools_History.hxx>
#include <BRepTools_ReShape.hxx>
#include <ShapeFix_Root.hxx>

class gp_Ax1;
class gp_Ax2;
class gp_Pln;
class gp_Vec;

namespace Base
{
class Color;
}

namespace Part
{

struct ShapeHasher;
class TopoShape;
class TopoShapeCache;
using TopoShapeMap = std::unordered_map<TopoShape, TopoShape, ShapeHasher, ShapeHasher>;

/* A special sub-class to indicate null shapes
 */
// NOLINTNEXTLINE cppcoreguidelines-special-member-functions
class PartExport NullShapeException: public Base::ValueError
{
public:
    /// Construction
    NullShapeException();
    explicit NullShapeException(const char* sMessage);
    explicit NullShapeException(const std::string& sMessage);
    /// Destruction
    ~NullShapeException() noexcept override = default;
};

/* A special sub-class to indicate boolean failures
 */
// NOLINTNEXTLINE cppcoreguidelines-special-member-functions
class PartExport BooleanException: public Base::CADKernelError
{
public:
    /// Construction
    BooleanException();
    explicit BooleanException(const char* sMessage);
    explicit BooleanException(const std::string& sMessage);
    /// Destruction
    ~BooleanException() noexcept override = default;
};

class PartExport ShapeSegment: public Data::Segment
{
    // NOLINTNEXTLINE cppcoreguidelines-avoid-non-const-global-variables
    TYPESYSTEM_HEADER_WITH_OVERRIDE();

public:
    explicit ShapeSegment(TopoDS_Shape ShapeIn)
        : Shape(std::move(ShapeIn))
    {}
    ShapeSegment() = default;
    std::string getName() const override;

    TopoDS_Shape Shape;
};

/// When tracing an element's history, one can either stop the trace when the element's type
/// changes, or continue tracing the history through the change. This enumeration replaces a boolean
/// parameter in the original Toponaming branch by realthunder.
enum class HistoryTraceType
{
    stopOnTypeChange,
    followTypeChange
};

/// Behavior of refines when a problem arises; either leave the shape untouched or throw an
/// exception. This replaces a boolean parameter in the original Toponaming branch by realthunder..
enum class RefineFail
{
    shapeUntouched,
    throwException
};

/// Behavior of findSubShapesWithSharedVertex.
enum class CheckGeometry
{
    ignoreGeometry,
    checkGeometry
};

enum class LinearizeFace
{
    noFaces,
    linearizeFaces
};

enum class LinearizeEdge
{
    noEdges,
    linearizeEdges
};

enum class IsSolid
{
    notSolid,
    solid
};

enum class IsRuled
{
    notRuled,
    ruled
};

enum class IsClosed
{
    notClosed,
    closed
};

/// Option to manage discontinuity in pipe sweeping
enum class TransitionMode
{
    /** Discontinuities are treated by modification of the sweeping mode.
     * The pipe is "transformed" at the fractures of the spine. This mode
     * assumes building a self-intersected shell.
     */
    Transformed,

    /** Discontinuities are treated like right corner. Two pieces of the
     * pipe corresponding to two adjacent segments of the spine are
     * extended and intersected at a fracture of the spine.
     */
    RightCorner,

    /** Discontinuities are treated like round corner. The corner is
     * treated as rotation of the profile around an axis which passes
     * through the point of the spine's fracture. This axis is based on
     * cross product of directions tangent to the adjacent segments of the
     * spine at their common point.
     */
    RoundCorner
};

enum class MakeSolid
{
    noSolid,
    makeSolid
};

enum class MapElement
{
    noMap,
    map
};

/// Defines how to fill the holes that may appear after offset two adjacent faces
enum class JoinType
{
    arc,
    tangent,
    intersection,
};

enum class Flip
{
    none,
    flip
};

enum class ChamferType
{
    equalDistance,
    twoDistances,
    distanceAngle
};

enum class CheckScale
{
    noScaleCheck,
    checkScale
};

enum class CopyType
{
    noCopy,
    copy
};

/// Filling style when making a BSpline face
enum FillingStyle
{
    /// The style with the flattest patches
    stretch,
    /// A rounded style of patch with less depth than those of Curved
    coons,
    /// The style with the most rounded patches
    curved,
};

enum class CoordinateSystem
{
    relativeToSpine,
    global
};

enum class Spine
{
    notOn,
    on
};

enum class FillType
{
    noFill,
    fill
};

enum class OpenResult
{
    noOpenResult,
    allowOpenResult
};

// See BRepFeat_MakeRevol
enum class RevolMode
{
    CutFromBase = 0,
    FuseWithBase = 1,
    None = 2
};

/** The representation for a CAD Shape
 */
// NOLINTNEXTLINE cppcoreguidelines-special-member-functions
class PartExport TopoShape: public Data::ComplexGeoData
{
    // NOLINTNEXTLINE cppcoreguidelines-avoid-non-const-global-variables
    TYPESYSTEM_HEADER_WITH_OVERRIDE();

public:
    TopoShape(
        long Tag = 0,  // NOLINT google-explicit-constructor
        App::StringHasherRef hasher = App::StringHasherRef(),
        const TopoDS_Shape& shape = TopoDS_Shape()
    );  // Cannot be made explicit
    TopoShape(
        const TopoDS_Shape&,  // NOLINT google-explicit-constructor
        long Tag = 0,
        App::StringHasherRef hasher = App::StringHasherRef()
    );  // Cannot be made explicit
    TopoShape(const TopoShape&);
    ~TopoShape() override;

    void setShape(const TopoDS_Shape& shape, bool resetElementMap = true);

    inline void setShape(const TopoShape& shape)
    {
        *this = shape;
    }

    inline const TopoDS_Shape& getShape() const
    {
        return this->_Shape;
    }

    void operator=(const TopoShape&);

    bool operator==(const TopoShape& other) const
    {
        return _Shape.IsEqual(other._Shape);
    }

    virtual bool isSame(const Data::ComplexGeoData& other) const;

    /** @name Placement control */
    //@{
    /// set the transformation of the CasCade Shape
    void setTransform(const Base::Matrix4D& rclTrf) override;
    /// get the transformation of the CasCade Shape
    Base::Matrix4D getTransform() const override;
    /// Bound box from the CasCade shape
    Base::BoundBox3d getBoundBox() const override;
    /// More precise bound box from the CasCade shape
    Base::BoundBox3d getBoundBoxOptimal() const;
    bool getCenterOfGravity(Base::Vector3d& center) const override;
    static void convertTogpTrsf(const Base::Matrix4D& mtrx, gp_Trsf& trsf);
    static void convertToMatrix(const gp_Trsf& trsf, Base::Matrix4D& mtrx);
    static Base::Matrix4D convert(const gp_Trsf& trsf);
    static gp_Trsf convert(const Base::Matrix4D& mtrx);
    //@}

    /** @name Getting basic geometric entities */
    //@{
private:
    /** Get lines from sub-shape */
    void getLinesFromSubShape(
        const TopoDS_Shape& shape,
        std::vector<Base::Vector3d>& vertices,
        std::vector<Line>& lines
    ) const;
    void getFacesFromDomains(
        const std::vector<Domain>& domains,
        std::vector<Base::Vector3d>& vertices,
        std::vector<Facet>& faces
    ) const;

public:
    /// Get the standard accuracy to be used with getPoints, getLines or getFaces
    double getAccuracy() const override;
    /** Get points from object with given accuracy */
    void getPoints(
        std::vector<Base::Vector3d>& Points,
        std::vector<Base::Vector3d>& Normals,
        double Accuracy,
        uint16_t flags = 0
    ) const override;
    /** Get lines from object with given accuracy */
    void getLines(
        std::vector<Base::Vector3d>& Points,
        std::vector<Line>& lines,
        double Accuracy,
        uint16_t flags = 0
    ) const override;
    void getFaces(
        std::vector<Base::Vector3d>& Points,
        std::vector<Facet>& faces,
        double Accuracy,
        uint16_t flags = 0
    ) const override;
    void setFaces(
        const std::vector<Base::Vector3d>& Points,
        const std::vector<Facet>& faces,
        double tolerance = 1.0e-06
    );  // NOLINT
    void getDomains(std::vector<Domain>&) const;
    //@}

    /** @name Subelement management */
    //@{
    /// Search to see if a SubShape matches
    static Data::MappedElement chooseMatchingSubShapeByPlaneOrLine(
        const TopoShape& shapeToFind,
        const TopoShape& shapeToLookIn
    );
    /// Unlike \ref getTypeAndIndex() this function only handles the supported
    /// element types. It works only if Name is just an element name (with or without TNP hash).
    static std::pair<std::string, unsigned long> getElementTypeAndIndex(const char* Name);
    /** Sub type list
     *  List of different subelement types
     *  it is NOT a list of the subelements itself
     */
    std::vector<const char*> getElementTypes() const override;
    unsigned long countSubElements(const char* Type) const override;
    /// get the subelement by type and number
    Data::Segment* getSubElement(const char* Type, unsigned long index) const override;
    /** Get lines from segment */
    void getLinesFromSubElement(
        const Data::Segment* segment,
        std::vector<Base::Vector3d>& Points,
        std::vector<Line>& lines
    ) const override;
    /** Get faces from segment */
    void getFacesFromSubElement(
        const Data::Segment* segment,
        std::vector<Base::Vector3d>& Points,
        std::vector<Base::Vector3d>& PointNormals,
        std::vector<Facet>& faces
    ) const override;
    //@}
    /**
     * Locate the TopoDS_Shape associated with a Topo"sub"Shape of the given name
     * @param Type      The complete name of the subshape - for example "Face2"
     * @param silent    True to suppress the exception throw if the shape isn't found
     * @return          The shape or a null TopoDS_Shape
     */
    TopoDS_Shape getSubShape(const char* Type, bool silent = false) const;
    /**
     * Locate a subshape's TopoDS_Shape by type enum and index.  See doc above.
     * @param type      Shape type enum value
     * @param idx       Index number of the subshape within the shape
     * @param silent    True to suppress the exception throw
     * @return  The shape, or a null TopoShape.
     */
    TopoDS_Shape getSubShape(TopAbs_ShapeEnum type, int idx, bool silent = false) const;
    /**
     * Locate a subshape by name within this shape.  If null or empty Type specified, try my own
     * shapeType; if I'm not a COMPOUND OR COMPSOLID, return myself; otherwise, look to see if I
     * have any singular SOLID, SHELL, FACE, WIRE, EDGE or VERTEX and return that.
     * If a Type is specified, then treat it as the complete name of the subshape - for example
     * "Face3" and try to find and return that shape.
     * @param Type      The Shape name
     * @param silent    True to suppress the exception throw if the shape isn't found.
     * @return          The shape or a null TopoShape.
     */
    TopoShape getSubTopoShape(const char* Type, bool silent = false) const;
    /**
     * Locate a subshape by type enum and index.  See doc above.
     * @param type      Shape type enum value
     * @param idx       Index number of the subshape within the shape
     * @param silent    True to suppress the exception throw
     * @return  The shape, or a null TopoShape.
     */
    TopoShape getSubTopoShape(TopAbs_ShapeEnum type, int idx, bool silent = false) const;
    /**
     * Locate all of the sub TopoShapes of a given type, while avoiding a given type
     * @param type The type to find
     * @param avoid The type to avoid
     * @return The sub TopoShapes.
     */
    std::vector<TopoShape> getSubTopoShapes(
        TopAbs_ShapeEnum type = TopAbs_SHAPE,
        TopAbs_ShapeEnum avoid = TopAbs_SHAPE
    ) const;
    /**
     * Locate all of the sub TopoDS_Shapes of a given type, while avoiding a given type
     * @param type The type to find
     * @param avoid The type to avoid
     * @return The sub TopoDS_Shapes.
     */
    std::vector<TopoDS_Shape> getSubShapes(
        TopAbs_ShapeEnum type = TopAbs_SHAPE,
        TopAbs_ShapeEnum avoid = TopAbs_SHAPE
    ) const;
    /**
     * Locate all the Edges in the Wires of this shape
     * @param mapElement If True, map the subelements ( Edges ) found
     * @return Vector of the edges
     */
    std::vector<TopoShape> getOrderedEdges(MapElement mapElement = MapElement::map) const;
    /**
     * Locate all the Vertexes in the Wires of this shape
     * @param mapElement If True, map the subelements ( Vertexes )  found
     * @return Vector of the Vertexes
     */
    std::vector<TopoShape> getOrderedVertexes(MapElement mapElement = MapElement::map) const;
    unsigned long countSubShapes(const char* Type) const;
    unsigned long countSubShapes(TopAbs_ShapeEnum type) const;
    bool hasSubShape(const char* Type) const;
    bool hasSubShape(TopAbs_ShapeEnum type) const;
    /// get the Topo"sub"Shape with the given name
    PyObject* getPySubShape(const char* Type, bool silent = false) const;
    PyObject* getPyObject() override;
    void setPyObject(PyObject* obj) override;

    /** @name Save/restore */
    //@{
    void Save(Base::Writer& writer) const override;
    void Restore(Base::XMLReader& reader) override;

    void SaveDocFile(Base::Writer& writer) const override;
    void RestoreDocFile(Base::Reader& reader) override;
    unsigned int getMemSize() const override;
    //@}

    /** @name Input/Output */
    //@{
    void read(const char* FileName);
    void write(const char* FileName) const;
    void dump(std::ostream& out) const;
    void importIges(const char* FileName);
    void importStep(const char* FileName);
    void importBrep(const char* FileName);
    void importBrep(std::istream&, int indicator = 1);
    void importBinary(std::istream&);
    void exportIges(const char* FileName) const;
    void exportStep(const char* FileName) const;
    void exportBrep(const char* FileName) const;
    void exportBrep(std::ostream&) const;
    void exportBinary(std::ostream&) const;
    void exportStl(const char* FileName, double deflection) const;
    void exportFaceSet(double, double, const std::vector<Base::Color>&, std::ostream&) const;
    void exportLineSet(std::ostream&) const;
    //@}

    /** @name Query*/
    //@{
    bool isNull() const;
    bool isValid() const;
    bool isEmpty() const;
    bool analyze(bool runBopCheck, std::ostream&) const;
    bool isClosed() const;
    bool isCoplanar(const TopoShape& other, double tol = -1) const;
    bool findPlane(gp_Pln& plane, double tol = -1, double atol = -1) const;
    /// Returns true if the expansion of the shape is infinite, false otherwise
    bool isInfinite() const;
    /// Checks whether the shape is a planar face
    bool isPlanar(double tol = 1.0e-7) const;  // NOLINT
    /// Check if this shape is a single linear edge, works on BSplineCurve and BezierCurve
    bool isLinearEdge() const;
    /// Check if this shape is a single planar face, works on BSplineSurface and BezierSurface
    bool isPlanarFace(double tol = 1e-7) const;  // NOLINT
    //@}

    /** @name Boolean operation*/
    //@{
    TopoDS_Shape cut(TopoDS_Shape) const;
    TopoDS_Shape cut(const std::vector<TopoDS_Shape>&, Standard_Real tolerance = -1.0) const;
    TopoDS_Shape common(TopoDS_Shape) const;
    TopoDS_Shape common(const std::vector<TopoDS_Shape>&, Standard_Real tolerance = -1.0) const;
    TopoDS_Shape fuse(TopoDS_Shape) const;
    TopoDS_Shape fuse(const std::vector<TopoDS_Shape>&, Standard_Real tolerance = -1.0) const;
    TopoDS_Shape section(TopoDS_Shape, Standard_Boolean approximate = Standard_False) const;
    TopoDS_Shape section(
        const std::vector<TopoDS_Shape>&,
        Standard_Real tolerance = -1.0,
        Standard_Boolean approximate = Standard_False
    ) const;
    std::list<TopoDS_Wire> slice(const Base::Vector3d&, double) const;
    TopoDS_Compound slices(const Base::Vector3d&, const std::vector<double>&) const;
    /**
     * @brief generalFuse: run general fuse algorithm between this and shapes
     * supplied as sOthers
     *
     * @param sOthers (input): list of shapes to run the algorithm between
     * (this is automatically added to the list)
     *
     * @param tolerance (input): fuzzy value (pass zero to disable fuzzyness
     * and use shape tolerances only)
     *
     * @param mapInOut (output): pointer to list of lists, to write the info
     * which shapes in result came from which argument shape. The length of
     * list is equal to length of sOthers+1. First element is a list of shapes
     * that came from shape of this, and the rest are those that come from
     * shapes in sOthers. If the info is not needed, nullptr can be passed.
     *
     * @return compound of slices that can be combined to reproduce results of
     * cut, fuse, common. The shapes share edges and faces where they touch.
     * For example, if input shapes are two intersecting spheres, GFA returns
     * three solids: two cuts and common.
     */
    TopoDS_Shape generalFuse(
        const std::vector<TopoDS_Shape>& sOthers,
        Standard_Real tolerance,
        std::vector<TopTools_ListOfShape>* mapInOut = nullptr
    ) const;
    //@}

    /** Sweeping */
    //@{
    TopoDS_Shape makePipe(const TopoDS_Shape& profile) const;
    TopoDS_Shape makePipeShell(
        const TopTools_ListOfShape& profiles,
        const Standard_Boolean make_solid,
        const Standard_Boolean isFrenet = Standard_False,
        int transition = 0
    ) const;
    TopoDS_Shape makePrism(const gp_Vec&) const;
    /// revolve shape. Note: isSolid is deprecated (instead, use some Part::FaceMaker to make a
    /// face, first).
    TopoDS_Shape revolve(const gp_Ax1&, double d, Standard_Boolean isSolid = Standard_False) const;
    TopoDS_Shape makeSweep(const TopoDS_Shape& profile, double, int) const;
    TopoDS_Shape makeTube(double radius, double tol, int cont, int maxdeg, int maxsegm) const;
    TopoDS_Shape makeTorus(
        Standard_Real radius1,
        Standard_Real radius2,
        Standard_Real angle1,
        Standard_Real angle2,
        Standard_Real angle3,
        Standard_Boolean isSolid = Standard_True
    ) const;
    TopoDS_Shape makeHelix(
        Standard_Real pitch,
        Standard_Real height,
        Standard_Real radius,
        Standard_Real angle = 0,
        Standard_Boolean left = Standard_False,
        Standard_Boolean style = Standard_False
    ) const;
    TopoDS_Shape makeLongHelix(
        Standard_Real pitch,
        Standard_Real height,
        Standard_Real radius,
        Standard_Real angle = 0,
        Standard_Boolean left = Standard_False
    ) const;
    TopoDS_Shape makeSpiralHelix(
        Standard_Real radiusbottom,
        Standard_Real radiustop,
        Standard_Real height,
        Standard_Real nbturns = 1,
        Standard_Real breakperiod = 1,
        Standard_Boolean left = Standard_False
    ) const;
    TopoDS_Shape makeThread(
        Standard_Real pitch,
        Standard_Real depth,
        Standard_Real height,
        Standard_Real radius
    ) const;
    TopoDS_Shape makeLoft(
        const TopTools_ListOfShape& profiles,
        Standard_Boolean isSolid,
        Standard_Boolean isRuled,
        Standard_Boolean isClosed = Standard_False,
        Standard_Integer maxDegree = 5
    ) const;
    TopoDS_Shape makeOffsetShape(
        double offset,
        double tol,
        bool intersection = false,
        bool selfInter = false,
        short offsetMode = 0,
        short join = 0,
        bool fill = false
    ) const;
    TopoDS_Shape makeOffset2D(
        double offset,
        short joinType = 0,
        bool fill = false,
        bool allowOpenResult = false,
        bool intersection = false
    ) const;
    TopoDS_Shape makeThickSolid(
        const TopTools_ListOfShape& remFace,
        double offset,
        double tol,
        bool intersection = false,
        bool selfInter = false,
        short offsetMode = 0,
        short join = 0
    ) const;
    //@}

    /** @name Manipulation*/
    //@{
    void transformGeometry(const Base::Matrix4D& rclMat) override;
    void bakeInTransform();
    TopoDS_Shape transformGShape(const Base::Matrix4D&, bool copy = false) const;
    bool transformShape(const Base::Matrix4D&, bool copy, bool checkScale = false);
    TopoDS_Shape mirror(const gp_Ax2&) const;
    TopoDS_Shape toNurbs() const;
    TopoDS_Shape replaceShape(const std::vector<std::pair<TopoDS_Shape, TopoDS_Shape>>& s) const;
    TopoDS_Shape removeShape(const std::vector<TopoDS_Shape>& s) const;
    void sewShape(double tolerance = 1.0e-06);
    bool fix();
    bool fix(double, double, double);
    bool fixSolidOrientation();
    bool removeInternalWires(double);
    TopoDS_Shape removeSplitter() const;
    TopoDS_Shape defeaturing(const std::vector<TopoDS_Shape>& s) const;
    TopoDS_Shape makeShell(const TopoDS_Shape&) const;
    //@}

    /// Wire re-orientation when calling splitWires()
    enum SplitWireReorient
    {
        /// Keep original reorientation
        NoReorient,
        /// Make outer wire forward, and inner wires reversed
        Reorient,
        /// Make both outer and inner wires forward
        ReorientForward,
        /// Make both outer and inner wires reversed
        ReorientReversed,
    };

    /** Return the outer and inner wires of a face
     *
     * @param innerWiresOutput: optional output of inner wires
     * @param reorient: wire reorientation, see SplitWireReorient
     *
     * @return Return the outer wire
     */
    TopoShape splitWires(
        std::vector<TopoShape>* innerWiresOutput = nullptr,
        SplitWireReorient reorient = Reorient
    ) const;

    /** @name Element name mapping aware shape maker
     *
     * To be complete in next batch of patches
     */
    //@{
    TopoShape& makeCompound(
        const std::vector<TopoShape>& shapes,
        const char* op = nullptr,
        bool force = true
    );

    TopoShape& makeWires(
        const TopoShape& shape,
        const char* op = nullptr,
        bool fix = false,
        double tol = 0.0
    );
    TopoShape makeWires(const char* op = nullptr, bool fix = false, double tol = 0.0) const
    {
        return TopoShape().makeWires(*this, op, fix, tol);
    }
    TopoShape& makeFace(
        const std::vector<TopoShape>& shapes,
        const char* op = nullptr,
        const char* maker = nullptr
    );
    TopoShape& makeFace(const TopoShape& shape, const char* op = nullptr, const char* maker = nullptr);
    TopoShape makeFace(const char* op = nullptr, const char* maker = nullptr) const
    {
        return TopoShape().makeFace(*this, op, maker);
    }
    bool _makeTransform(
        const TopoShape& shape,
        const Base::Matrix4D& mat,
        const char* op = nullptr,
        bool checkScale = false,
        bool copy = false
    );

    TopoShape& makeTransform(
        const TopoShape& shape,
        const Base::Matrix4D& mat,
        const char* op = nullptr,
        bool checkScale = false,
        bool copy = false
    )
    {
        _makeTransform(shape, mat, op, checkScale, copy);
        return *this;
    }
    TopoShape makeTransform(
        const Base::Matrix4D& mat,
        const char* op = nullptr,
        bool checkScale = false,
        bool copy = false
    ) const
    {
        return TopoShape().makeTransform(*this, mat, op, checkScale, copy);
    }

    TopoShape& makeTransform(
        const TopoShape& shape,
        const gp_Trsf& trsf,
        const char* op = nullptr,
        bool copy = false
    );
    TopoShape makeTransform(const gp_Trsf& trsf, const char* op = nullptr, bool copy = false) const
    {
        return TopoShape().makeTransform(*this, trsf, op, copy);
    }

    /** Move the shape to a new location
     *
     * @param loc: location
     *
     * The location is applied in addition to any current transformation of the shape
     */
    void move(const TopLoc_Location& loc)
    {
        _Shape.Move(loc);
    }
    /** Return a new shape that is moved to a new location
     *
     * @param loc: location
     *
     * @return Return a shallow copy of the shape moved to the new location
     *         that is applied in addition to any current transformation of the
     *         shape
     */
    TopoShape moved(const TopLoc_Location& loc) const
    {
        TopoShape ret(*this);
        ret._Shape.Move(loc);
        return ret;
    }
    /** Move and/or rotate the shape
     *
     * @param trsf: OCCT transformation (must not have scale)
     *
     * The transformation is applied in addition to any current transformation
     * of the shape
     */
    void move(const gp_Trsf& trsf)
    {
        move(_Shape, trsf);
    }
    /** Return a new transformed shape
     *
     * @param trsf: OCCT transformation (must not have scale)
     *
     * @return Return a shallow copy of the shape transformed to the new
     *         location that is applied in addition to any current
     *         transformation of the shape
     */
    TopoShape moved(const gp_Trsf& trsf) const
    {
        return moved(_Shape, trsf);
    }
    /** Set a new location for the shape
     *
     * @param loc: shape location
     *
     * Any previous location of the shape is discarded before applying the
     * input location
     */
    void locate(const TopLoc_Location& loc)
    {
        _Shape.Location(loc);
    }
    /** ReturnSet a new location for the shape
     *
     * @param loc: shape location
     *
     * @return Return a shallow copy the shape in a new location. Any previous
     *         location of the shape is discarded before applying the input
     *         location
     */
    TopoShape located(const TopLoc_Location& loc) const
    {
        TopoShape ret(*this);
        ret._Shape.Location(loc);
        return ret;
    }
    /** Set a new transformation for the shape
     *
     * @param trsf: OCCT transformation (must not have scale)
     *
     * Any previous transformation of the shape is discarded before applying
     * the input transformation
     */
    void locate(const gp_Trsf& trsf)
    {
        located(_Shape, trsf);
    }
    /** Set a new transformation for the shape
     *
     * @param trsf: OCCT transformation (must not have scale)
     *
     * Any previous transformation of the shape is discarded before applying
     * the input transformation.
     */
    TopoShape located(const gp_Trsf& trsf) const
    {
        return located(_Shape, trsf);
    }

    static TopoDS_Shape& move(TopoDS_Shape& tds, const TopLoc_Location& loc);
    static TopoDS_Shape moved(const TopoDS_Shape& tds, const TopLoc_Location& loc);
    static TopoDS_Shape& move(TopoDS_Shape& tds, const gp_Trsf& transfer);
    static TopoDS_Shape moved(const TopoDS_Shape& tds, const gp_Trsf& transfer);
    static TopoDS_Shape& locate(TopoDS_Shape& tds, const TopLoc_Location& loc);
    static TopoDS_Shape located(const TopoDS_Shape& tds, const TopLoc_Location& loc);
    static TopoDS_Shape& locate(TopoDS_Shape& tds, const gp_Trsf& transfer);
    static TopoDS_Shape located(const TopoDS_Shape& tds, const gp_Trsf& transfer);

    TopoShape& makeGTransform(
        const TopoShape& shape,
        const Base::Matrix4D& mat,
        const char* op = nullptr,
        bool copy = false
    );
    TopoShape makeGTransform(const Base::Matrix4D& mat, const char* op = nullptr, bool copy = false) const
    {
        return TopoShape().makeGTransform(*this, mat, op, copy);
    }

    /** Refine the input shape by merging faces/edges that share the same geometry
     *
     * @param shape: input shape
     * @param op: optional string to be encoded into topo naming for indicating
     *            the operation
     * @param no_fail: if throwException, throw exception if failed to refine. Or else,
     *                 if shapeUntouched the shape remains untouched if failed.
     *
     * @return The original content of this TopoShape is discarded and replaced
     *         with the refined shape. The function returns the TopoShape
     *         itself as a self reference so that multiple operations can be
     *         carried out for the same shape in the same line of code.
     */
    TopoShape& makeElementRefine(
        const TopoShape& shape,
        const char* op = nullptr,
        RefineFail no_fail = RefineFail::throwException
    );

    /** Refine the input shape by merging faces/edges that share the same geometry
     *
     * @param source: input shape
     * @param op: optional string to be encoded into topo naming for indicating
     *            the operation
     * @param no_fail: if throwException, throw exception if failed to refine. Or else,
     *                 if shapeUntouched the shape remains untouched if failed.
     *
     * @return Return a refined shape. The shape itself is not modified
     */
    TopoShape makeElementRefine(
        const char* op = nullptr,
        RefineFail no_fail = RefineFail::throwException
    ) const
    {
        return TopoShape(Tag, Hasher).makeElementRefine(*this, op, no_fail);
    }


    TopoShape& makeRefine(
        const TopoShape& shape,
        const char* op = nullptr,
        RefineFail no_fail = RefineFail::throwException
    );

    TopoShape makeRefine(const char* op = nullptr, RefineFail no_fail = RefineFail::throwException) const
    {
        return TopoShape().makeRefine(*this, op, no_fail);
    }
    //@}

    /** Make a hollowed solid by removing some faces from a given solid
     *
     * @param shape: input shape
     * @param faces: list of faces to remove, must be sub shape of the input shape
     * @param offset: thickness of the walls
     * @param tol: tolerance criterion for coincidence in generated shapes
     * @param intersection: whether to check intersection in all generated parallel.
     * @param selfInter: whether to eliminate self intersection.
     * @param offsetMode: defines the construction type of parallels applied to free edges
     * @param join: join type. Only support JoinType::Arc and JoinType::Intersection.
     * @param op: optional string to be encoded into topo naming for indicating
     *            the operation
     *
     * @return The original content of this TopoShape is discarded and replaced
     *         with the new shape. The function returns the TopoShape itself as
     *         a self reference so that multiple operations can be carried out
     *         for the same shape in the same line of code.
     */
    TopoShape& makeElementThickSolid(
        const TopoShape& shape,
        const std::vector<TopoShape>& faces,
        double offset,
        double tol,
        bool intersection = false,
        bool selfInter = false,
        short offsetMode = 0,
        JoinType join = JoinType::arc,
        const char* op = nullptr
    );

    /** Make a hollowed solid by removing some faces from a given solid
     *
     * @param faces: list of faces to remove, must be sub shape of the input shape
     * @param offset: thickness of the walls
     * @param tol: tolerance criterion for coincidence in generated shapes
     * @param intersection: whether to check intersection in all generated parallel
     *                      (OCCT document states the option is not fully implemented)
     * @param selfInter: whether to eliminate self intersection
     *                   (OCCT document states the option is not implemented)
     * @param offsetMode: defines the construction type of parallels applied to free edges
     *                    (OCCT document states the option is not implemented)
     * @param join: join type. Only support JoinType::Arc and JoinType::Intersection.
     * @param op: optional string to be encoded into topo naming for indicating
     *            the operation
     *
     * @return Return the generated new shape. The TopoShape itself is not modified.
     */
    TopoShape makeElementThickSolid(
        const std::vector<TopoShape>& faces,
        double offset,
        double tol,
        bool intersection = false,
        bool selfInter = false,
        short offsetMode = 0,
        JoinType join = JoinType::arc,
        const char* op = nullptr
    ) const
    {
        return TopoShape(0, Hasher).makeElementThickSolid(
            *this,
            faces,
            offset,
            tol,
            intersection,
            selfInter,
            offsetMode,
            join,
            op
        );
    }
    /** Make a 3D offset of a given shape
     *
     * @param source: source shape
     * @param offset: distance to offset
     * @param tol: tolerance criterion for coincidence in generated shapes
     * @param intersection: whether to check intersection in all generated parallel
     *                      (OCCT document states the option is not fully implemented)
     * @param selfInter: whether to eliminate self intersection
     *                   (OCCT document states the option is not implemented)
     * @param offsetMode: defines the construction type of parallels applied to free edges
     *                    (OCCT document states the option is not implemented)
     * @param join: join type. Only support JoinType::Arc and JoinType::Intersection.
     * @param fill: whether to build a solid by fill the offset
     * @param op: optional string to be encoded into topo naming for indicating
     *            the operation
     *
     * @return The original content of this TopoShape is discarded and replaced
     *         with the new shape. The function returns the TopoShape itself as
     *         a self reference so that multiple operations can be carried out
     *         for the same shape in the same line of code.
     */
    TopoShape& makeElementOffset(
        const TopoShape& source,
        double offset,
        double tol,
        bool intersection = false,
        bool selfInter = false,
        short offsetMode = 0,
        JoinType join = JoinType::arc,
        FillType fill = FillType::noFill,
        const char* op = nullptr
    );

    /** Make a 3D offset of this shape
     *
     * @param offset: distance to offset
     * @param tol: tolerance criterion for coincidence in generated shapes
     * @param intersection: whether to check intersection in all generated parallel
     *                      (OCCT document states the option is not fully implemented)
     * @param selfInter: whether to eliminate self intersection
     *                   (OCCT document states the option is not implemented)
     * @param offsetMode: defines the construction type of parallels applied to free edges
     *                    (OCCT document states the option is not implemented)
     * @param fill: whether to build a solid by fill the offset
     * @param join: join type. Only support JoinType::Arc and JoinType::Intersection.
     * @param op: optional string to be encoded into topo naming for indicating
     *            the operation
     *
     * @return Return the new shape. The TopoShape itself is not modified.
     */
    TopoShape makeElementOffset(
        double offset,
        double tol,
        bool intersection = false,
        bool selfInter = false,
        short offsetMode = 0,
        JoinType join = JoinType::arc,
        FillType fill = FillType::noFill,
        const char* op = nullptr
    ) const
    {
        return TopoShape(0, Hasher)
            .makeElementOffset(*this, offset, tol, intersection, selfInter, offsetMode, join, fill, op);
    }

    /** Make a 2D offset of a given shape
     *
     * @param source: source shape of edge, wire, face, or compound
     * @param offset: distance to offset
     * @param allowOpenResult: whether to allow open edge/wire
     * @param join: join type. Only support JoinType::Arc and JoinType::Intersection.
     * @param intersection: if true, then offset all non-compound shape
     *                      together to deal with possible intersection after
     *                      expanding the shape.  If false, then offset each
     *                      shape separately.
     * @param op: optional string to be encoded into topo naming for indicating
     *            the operation
     *
     * @return The original content of this TopoShape is discarded and replaced
     *         with the new shape. The function returns the TopoShape itself as
     *         a self reference so that multiple operations can be carried out
     *         for the same shape in the same line of code.
     */
    TopoShape& makeElementOffset2D(
        const TopoShape& source,
        double offset,
        JoinType join = JoinType::arc,
        FillType fill = FillType::noFill,
        OpenResult allowOpenResult = OpenResult::allowOpenResult,
        bool intersection = false,
        const char* op = nullptr
    );
    /** Make a 2D offset of a given shape
     *
     * @param source: source shape of edge, wire, face, or compound
     * @param offset: distance to offset
     * @param allowOpenResult: whether to allow open edge/wire
     * @param join: join type. Only support JoinType::Arc and JoinType::Intersection.
     * @param intersection: if true, then offset all non-compound shape
     *                      together to deal with possible intersection after
     *                      expanding the shape.  If false, then offset each
     *                      shape separately.
     * @param op: optional string to be encoded into topo naming for indicating
     *            the operation
     *
     * @return Return the new shape. The TopoShape itself is not modified.
     */
    TopoShape makeElementOffset2D(
        double offset,
        JoinType join = JoinType::arc,
        FillType fill = FillType::noFill,
        OpenResult allowOpenResult = OpenResult::allowOpenResult,
        bool intersection = false,
        const char* op = nullptr
    ) const
    {
        return TopoShape(0, Hasher)
            .makeElementOffset2D(*this, offset, join, fill, allowOpenResult, intersection, op);
    }

    /** Make a 2D offset of face with separate control for outer and inner (hole) wires
     *
     * @param source: source shape of any type, but only faces inside will be used
     * @param offset: distance to offset for outer wires of the faces
     * @param innerOffset: distance to offset for inner wires of the faces
     * @param join: join type of outer wire. Only support JoinType::Arc and JoinType::Intersection.
     * @param innerJoin: join type of inner wire. Only support JoinType::Arc and
     * JoinType::Intersection.
     * @param op: optional string to be encoded into topo naming for indicating
     *            the operation
     *
     * @return The original content of this TopoShape is discarded and replaced
     *         with the new shape. The function returns the TopoShape itself as
     *         a self reference so that multiple operations can be carried out
     *         for the same shape in the same line of code.
     */
    TopoShape& makeElementOffsetFace(
        const TopoShape& source,
        double offset,
        double innerOffset,
        JoinType join = JoinType::arc,
        JoinType innerJoin = JoinType::arc,
        const char* op = nullptr
    );

    /** Make a 2D offset of face with separate control for outer and inner (hole) wires
     *
     * @param source: source shape of any type, but only faces inside will be used
     * @param offset: distance to offset for outer wires of the faces
     * @param innerOffset: distance to offset for inner wires of the faces
     * @param join: join type of outer wire. Only support JoinType::Arc and JoinType::Intersection.
     * @param innerJoin: join type of inner wire. Only support JoinType::Arc and
     * JoinType::Intersection.
     * @param op: optional string to be encoded into topo naming for indicating
     *            the operation
     *
     * @return Return the new shape. The TopoShape itself is not modified.
     */
    TopoShape makeElementOffsetFace(
        double offset,
        double innerOffset,
        JoinType join = JoinType::arc,
        JoinType innerJoin = JoinType::arc,
        const char* op = nullptr
    ) const
    {
        return TopoShape(0, Hasher)
            .makeElementOffsetFace(*this, offset, innerOffset, join, innerJoin, op);
    }


    /** Make revolved shell around a basis shape
     *
     * @param base: the base shape
     * @param axis: the revolving axis
     * @param d: rotation angle in degree
     * @param face_maker: optional type name of the maker used to make a
     *                    face from basis shape
     * @param op: optional string to be encoded into topo naming for indicating
     *            the operation
     *
     * @return The original content of this TopoShape is discarded and replaced
     *         with the new shape. The function returns the TopoShape itself as
     *         a self reference so that multiple operations can be carried out
     *         for the same shape in the same line of code.
     */
    TopoShape& makeElementRevolve(
        const TopoShape& base,
        const gp_Ax1& axis,
        double d,
        const char* face_maker = 0,
        const char* op = nullptr
    );

    /** Make revolved shell around a basis shape
     *
     * @param axis: the revolving axis
     * @param d: rotation angle in degree
     * @param face_maker: optional type name of the maker used to make a
     *                    face from basis shape
     * @param op: optional string to be encoded into topo naming for indicating
     *            the operation
     *
     * @return Return the generated new shape. The TopoShape itself is not modified.
     */
    TopoShape makeElementRevolve(
        const gp_Ax1& axis,
        double d,
        const char* face_maker = nullptr,
        const char* op = nullptr
    ) const
    {
        return TopoShape(0, Hasher).makeElementRevolve(*this, axis, d, face_maker, op);
    }


    /** Make revolved shell around a basis shape
     *
     * @param base: the basis shape (solid)
     * @param profile: the shape to be revolved
     * @param axis: the revolving axis
     * @param face_maker: optional type name of the maker used to make a
     *                    face from basis shape
     * @param supportface:  the bottom face for the revolution, or null
     * @param uptoface:  the upper limit face for the revolution, or null
     * @param Mode: the opencascade defined modes
     * @param Modify: if opencascade should modify existing shapes
     * @param op: optional string to be encoded into topo naming for indicating
     *            the operation
     *
     * @return Return the generated new shape. The TopoShape itself is not modified.
     */
    TopoShape& makeElementRevolution(
        const TopoShape& _base,
        const TopoDS_Shape& profile,
        const gp_Ax1& axis,
        const TopoDS_Face& supportface,
        const TopoDS_Face& uptoface,
        const char* face_maker = nullptr,
        RevolMode Mode = RevolMode::None,
        Standard_Boolean Modify = Standard_True,
        const char* op = nullptr
    );

    /** Make revolved shell around a basis shape
     *
     * @param axis: the revolving axis
     * @param face_maker: optional type name of the maker used to make a
     *                    face from basis shape
     * @param supportface:  the bottom face for the revolution, or null
     * @param uptoface:  the upper limit face for the revolution, or null
     * @param Mode: the opencascade defined modes
     * @param Modify: if opencascade should modify existing shapes
     * @param op: optional string to be encoded into topo naming for indicating
     *            the operation
     *
     * @return Return the generated new shape. The TopoShape itself is not modified.
     */
    TopoShape& makeElementRevolution(
        const gp_Ax1& axis,
        const TopoDS_Shape& profile,
        const TopoDS_Face& supportface,
        const TopoDS_Face& uptoface,
        const char* face_maker = nullptr,
        RevolMode Mode = RevolMode::None,
        Standard_Boolean Modify = Standard_True,
        const char* op = nullptr
    ) const
    {
        return TopoShape(0, Hasher).makeElementRevolution(
            *this,
            profile,
            axis,
            supportface,
            uptoface,
            face_maker,
            Mode,
            Modify,
            op
        );
    }

    /** Make a prism that is a linear sweep of a basis shape
     *
     * @param base: the basis shape
     * @param vec: vector defines the sweep direction
     * @param op: optional string to be encoded into topo naming for indicating
     *            the operation
     *
     * @return The original content of this TopoShape is discarded and replaced
     *         with the new shape. The function returns the TopoShape itself as
     *         a self reference so that multiple operations can be carried out
     *         for the same shape in the same line of code.
     */
    TopoShape& makeElementPrism(const TopoShape& base, const gp_Vec& vec, const char* op = nullptr);

    /** Make a prism that is a linear sweep of this shape
     *
     * @param vec: vector defines the sweep direction
     * @param op: optional string to be encoded into topo naming for indicating
     *            the operation
     *
     * @return Return the generated new shape. The TopoShape itself is not modified.
     */
    TopoShape makeElementPrism(const gp_Vec& vec, const char* op = nullptr) const
    {
        return TopoShape(0, Hasher).makeElementPrism(*this, vec, op);
    }

    /// Operation mode for makeElementPrismUntil()
    enum PrismMode
    {
        /// Remove the generated prism shape from the base shape with boolean cut
        CutFromBase = 0,
        /// Add generated prism shape to the base shape with fusion
        FuseWithBase = 1,
        /// Return the generated prism shape without base shape
        None = 2
    };
    /** Make a prism that is either depression or protrusion of a profile shape up to a given face
     *
     * @param base: the base shape
     * @param profile: profile shape used for sweeping to make the prism
     * @param supportFace: optional face serves to determining the type of
     *                     operation. If it is inside the basis shape, a local
     *                     operation such as glueing can be performed.
     * @param upToFace: sweep the profile up until this give face.
     * @param direction: the direction to sweep the profile
     * @param mode: defines what shape to return. @sa PrismMode
     * @param checkLimits: If true, then remove limit (boundary) of up to face.
     *                     If false, then the generate prism may go beyond the
     *                     boundary of the up to face.
     * @param op: optional string to be encoded into topo naming for indicating
     *            the operation
     *
     * @return The original content of this TopoShape is discarded and replaced
     *         with the new shape. The function returns the TopoShape itself as
     *         a self reference so that multiple operations can be carried out
     *         for the same shape in the same line of code.
     */
    TopoShape& makeElementPrismUntil(
        const TopoShape& base,
        const TopoShape& profile,
        const TopoShape& supportFace,
        const TopoShape& upToFace,
        const gp_Dir& direction,
        PrismMode mode,
        Standard_Boolean checkLimits = Standard_True,
        const char* op = nullptr
    );

    /** Make a prism based on this shape that is either depression or protrusion of a profile shape
     * up to a given face
     *
     * @param profile: profile shape used for sweeping to make the prism
     * @param supportFace: optional face serves to determining the type of
     *                     operation. If it is inside the basis shape, a local
     *                     operation such as glueing can be performed.
     * @param upToFace: sweep the profile up until this give face.
     * @param direction: the direction to sweep the profile
     * @param mode: defines what shape to return. @sa PrismMode
     * @param checkLimits: If true, then remove limit (boundary) of up to face.
     *                     If false, then the generate prism may go beyond the
     *                     boundary of the up to face.
     * @param op: optional string to be encoded into topo naming for indicating
     *            the operation
     *
     * @return Return the generated new shape. The TopoShape itself is not modified.
     */
    TopoShape makeElementPrismUntil(
        const TopoShape& profile,
        const TopoShape& supportFace,
        const TopoShape& upToFace,
        const gp_Dir& direction,
        PrismMode mode,
        Standard_Boolean checkLimits = Standard_True,
        const char* op = nullptr
    ) const
    {
        return TopoShape(0, Hasher).makeElementPrismUntil(
            *this,
            profile,
            supportFace,
            upToFace,
            direction,
            mode,
            checkLimits,
            op
        );
    }


    /* Make a shell or solid by sweeping profile wire along a spine
     *
     * @params sources: source shapes. The first shape is used as spine. The
     *                  rest are used as section profiles. Can be of type edge,
     *                  wire, face, or compound of those. For face, only outer
     *                  wire is used.
     * @params makeSolid: whether to create solid
     * @param isFrenet: if true, then assume the profiles transition is Frenet,
     *                  or else a corrected Frenet trihedron is used.
     * @param transition: @sa TransitionMode
     * @param op: optional string to be encoded into topo naming for indicating
     *            the operation
     * @param tol3d: 3D tolerance
     * @param tolBound: boundary tolerance
     * @param tolAngular: angular tolerance
     *
     * @return The original content of this TopoShape is discarded and replaced
     *         with the new shape. The function returns the TopoShape itself as
     *         a self reference so that multiple operations can be carried out
     *         for the same shape in the same line of code.
     */
    TopoShape& makeElementPipeShell(
        const std::vector<TopoShape>& sources,
        const MakeSolid makeSolid,
        const Standard_Boolean isFrenet,
        TransitionMode transition = TransitionMode::Transformed,
        const char* op = nullptr,
        double tol3d = 0.0,
        double tolBound = 0.0,
        double tolAngular = 0.0
    );

    /* Make a shape with some subshapes replaced.
     *
     * @param source: the source shape
     * @param s: replacement mapping the existing sub shape of source to new shapes
     *
     * @return The original content of this TopoShape is discarded and replaced
     *         with the new shape. The function returns the TopoShape itself as
     *         a self reference so that multiple operations can be carried out
     *         for the same shape in the same line of code.
     */
    TopoShape& replaceElementShape(
        const TopoShape& source,
        const std::vector<std::pair<TopoShape, TopoShape>>& s
    );
    /* Make a new shape using this shape with some subshapes replaced by others
     *
     * @param s: replacement mapping the existing sub shape of source to new shapes
     *
     * @return Return the new shape. The TopoShape itself is not modified.
     */
    TopoShape replaceElementShape(const std::vector<std::pair<TopoShape, TopoShape>>& s) const
    {
        return TopoShape(0, Hasher).replaceElementShape(*this, s);
    }

    /* Make a shape with some subshapes removed
     *
     * @param source: the source shape
     * @param s: the subshapes to be removed
     *
     * @return The original content of this TopoShape is discarded and replaced
     *         with the new shape. The function returns the TopoShape itself as
     *         a self reference so that multiple operations can be carried out
     *         for the same shape in the same line of code.
     */
    TopoShape& removeElementShape(const TopoShape& source, const std::vector<TopoShape>& s);
    /* Make a new shape using this shape with some subshapes removed
     *
     * @param s: the subshapes to be removed
     *
     * @return Return the new shape. The TopoShape itself is not modified.
     */
    TopoShape removeElementShape(const std::vector<TopoShape>& s) const
    {
        return TopoShape(0, Hasher).removeElementShape(*this, s);
    }

    /** Make shape using generalized fusion and return the modified sub shapes
     *
     * @param sources: the source shapes
     * @param modified: return the modified sub shapes
     * @param tol: tolerance
     * @param op: optional string to be encoded into topo naming for indicating
     *            the operation
     *
     * @return The original content of this TopoShape is discarded and replaced
     *         with the new shape. The function returns the TopoShape itself as
     *         a self reference so that multiple operations can be carried out
     *         for the same shape in the same line of code.
     */
    TopoShape& makeElementGeneralFuse(
        const std::vector<TopoShape>& sources,
        std::vector<std::vector<TopoShape>>& modified,
        double tol = -1.0,
        const char* op = nullptr
    );

    /** Make a fusion of input shapes
     *
     * @param sources: the source shapes
     * @param op: optional string to be encoded into topo naming for indicating
     *            the operation
     * @param tol: tolerance for the fusion
     *
     * @return The original content of this TopoShape is discarded and replaced
     *         with the new shape. The function returns the TopoShape itself as
     *         a self reference so that multiple operations can be carried out
     *         for the same shape in the same line of code.
     */
    TopoShape& makeElementFuse(
        const std::vector<TopoShape>& sources,
        const char* op = nullptr,
        double tol = -1.0
    );
    /** Make a fusion of this shape and an input shape
     *
     * @param source: the source shape
     * @param op: optional string to be encoded into topo naming for indicating
     *            the operation
     * @param tol: tolerance for the fusion
     *
     * @return Return the new shape. The TopoShape itself is not modified.
     */
    TopoShape makeElementFuse(const TopoShape& source, const char* op = nullptr, double tol = -1.0) const
    {
        return TopoShape(0, Hasher).makeElementFuse({*this, source}, op, tol);
    }

    /** Make a boolean cut of this shape with an input shape
     *
     * @param source: the source shape
     * @param op: optional string to be encoded into topo naming for indicating
     *            the operation
     * @param tol: tolerance for the fusion
     *
     * @return The original content of this TopoShape is discarded and replaced
     *         with the new shape. The function returns the TopoShape itself as
     *         a self reference so that multiple operations can be carried out
     *         for the same shape in the same line of code.
     */
    TopoShape& makeElementCut(
        const std::vector<TopoShape>& sources,
        const char* op = nullptr,
        double tol = -1.0
    );
    /** Make a boolean cut of this shape with an input shape
     *
     * @param source: the source shape
     * @param op: optional string to be encoded into topo naming for indicating
     *            the operation
     * @param tol: tolerance for the fusion
     *
     * @return Return the new shape. The TopoShape itself is not modified.
     */
    TopoShape makeElementCut(const TopoShape& source, const char* op = nullptr, double tol = -1.0) const
    {
        return TopoShape(0, Hasher).makeElementCut({*this, source}, op, tol);
    }

    /** Make a boolean xor of this shape with an input shape
     *
     * @param source: the source shape
     * @param op: optional string to be encoded into topo naming for indicating
     *            the operation
     * @param tol: tolerance for the fusion
     *
     * @return The original content of this TopoShape is discarded and replaced
     *         with the new shape. The function returns the TopoShape itself as
     *         a self reference so that multiple operations can be carried out
     *         for the same shape in the same line of code.
     */
    TopoShape& makeElementXor(
        const std::vector<TopoShape>& sources,
        const char* op = nullptr,
        double tol = -1.0
    );
    /** Make a boolean xor of this shape with an input shape
     *
     * @param source: the source shape
     * @param op: optional string to be encoded into topo naming for indicating
     *            the operation
     * @param tol: tolerance for the fusion
     *
     * @return Return the new shape. The TopoShape itself is not modified.
     */
    TopoShape makeElementXor(const TopoShape& source, const char* op = nullptr, double tol = -1.0) const
    {
        return TopoShape(0, Hasher).makeElementXor({*this, source}, op, tol);
    }

    /** Try to simplify geometry of any linear/planar subshape to line/plane
     *
     * @return Return true if the shape is modified
     */
    bool linearize(LinearizeFace face, LinearizeEdge edge);

    static TopAbs_ShapeEnum shapeType(const char* type, bool silent = false);
    static TopAbs_ShapeEnum shapeType(char type, bool silent = false);
    TopAbs_ShapeEnum shapeType(bool silent = false) const;
    static const std::string& shapeName(TopAbs_ShapeEnum type, bool silent = false);
    const std::string& shapeName(bool silent = false) const;
    static std::pair<TopAbs_ShapeEnum, int> shapeTypeAndIndex(const char* name);
    static std::pair<TopAbs_ShapeEnum, int> shapeTypeAndIndex(const Data::IndexedName& name);

    Data::MappedName setElementComboName(
        const Data::IndexedName& element,
        const std::vector<Data::MappedName>& names,
        const char* marker = nullptr,
        const char* op = nullptr,
        const Data::ElementIDRefs* sids = nullptr
    );

    std::vector<Data::MappedName> decodeElementComboName(
        const Data::IndexedName& element,
        const Data::MappedName& name,
        const char* marker = nullptr,
        std::string* postfix = nullptr
    ) const;

    void reTagElementMap(
        long tag,  // NOLINT google-default-arguments
        App::StringHasherRef hasher,
        const char* postfix = nullptr
    ) override;

    long isElementGenerated(const Data::MappedName& name, int depth = 1) const;

    /** @name sub shape cached functions
     *
     * Mapped element names introduces some overhead when getting sub shapes
     * from a shape. These functions use internal caches for sub-shape maps to
     * improve performance.
     */
    //@{
    void initCache(int reset = 0) const;
    int findShape(const TopoDS_Shape& subshape) const;
    TopoDS_Shape findShape(const char* name) const;
    TopoDS_Shape findShape(TopAbs_ShapeEnum type, int idx) const;
    int findAncestor(const TopoDS_Shape& subshape, TopAbs_ShapeEnum type) const;
    TopoDS_Shape findAncestorShape(const TopoDS_Shape& subshape, TopAbs_ShapeEnum type) const;
    std::vector<int> findAncestors(const TopoDS_Shape& subshape, TopAbs_ShapeEnum type) const;
    std::vector<TopoDS_Shape> findAncestorsShapes(
        const TopoDS_Shape& subshape,
        TopAbs_ShapeEnum type
    ) const;
    /** Find sub shapes with shared Vertexes.
     *
     * Renamed: searchSubShape -> findSubShapesWithSharedVertex
     *
     * unlike findShape(), the input shape does not have to be an actual
     * sub-shape of this shape. The sub-shape is searched by shape geometry
     * Note that subshape must be a Vertex, Edge, or Face.
     *
     * @param subshape: a sub shape to search
     * @param names: optional output of found sub shape indexed based name
     * @param checkGeometry: whether to compare shape geometry
     * @param tol: tolerance to check coincident vertices
     * @param atol: tolerance to check for same angles
     */
    std::vector<TopoShape> findSubShapesWithSharedVertex(
        const TopoShape& subshape,
        std::vector<std::string>* names = nullptr,
        Data::SearchOptions = Data::SearchOption::CheckGeometry,
        double tol = 1e-7,
        double atol = 1e-12
    ) const;
    //@}

    void copyElementMap(const TopoShape& topoShape, const char* op = nullptr);
    bool canMapElement(const TopoShape& other) const;
    void cacheRelatedElements(
        const Data::MappedName& name,
        HistoryTraceType sameType,
        const QVector<Data::MappedElement>& names
    ) const;

    bool getRelatedElementsCached(
        const Data::MappedName& name,
        HistoryTraceType sameType,
        QVector<Data::MappedElement>& names
    ) const;

    void mapSubElement(const TopoShape& other, const char* op = nullptr, bool forceHasher = false);
    void mapSubElement(const std::vector<TopoShape>& shapes, const char* op = nullptr);
    void mapSubElementsTo(std::vector<TopoShape>& shapes, const char* op = nullptr) const;
    bool hasPendingElementMap() const;

    std::string getElementMapVersion() const override;

    void flushElementMap() const override;

    Data::ElementMapPtr resetElementMap(Data::ElementMapPtr elementMap = Data::ElementMapPtr()) override;

    std::vector<Data::IndexedName> getHigherElements(
        const char* element,
        bool silent = false
    ) const override;

    /** Helper class to return the generated and modified shape given an input shape
     *
     * Shape history information is extracted using OCCT APIs
     * BRepBuilderAPI_MakeShape::Generated/Modified(). However, there is often
     * some glitches in various derived class. So we use this class as an
     * abstraction, and create various derived classes to deal with the glitches.
     */
    struct PartExport Mapper
    {
        /// Helper vector for temporary storage of both generated and modified shapes
        mutable std::vector<TopoDS_Shape> _res;
        virtual ~Mapper()
        {}
        /// Return a list of shape generated from the given input shape
        virtual const std::vector<TopoDS_Shape>& generated(const TopoDS_Shape&) const
        {
            return _res;
        }
        /// Return a list of shape modified from the given input shape
        virtual const std::vector<TopoDS_Shape>& modified(const TopoDS_Shape&) const
        {
            return _res;
        }
    };
    /** Make an evolved shape
     *
     * An evolved shape is built from a planar spine (face or wire) and a
     * profile (wire). The evolved shape is the unlooped sweep (pipe) of the
     * profile along the spine. Self-intersections are removed.
     * Note that the underlying OCCT method is very finicky about parameters and
     * make throw "Unimplemented" exceptions for various types.
     *
     * @param spine: the spine shape, must be planar face or wire
     * @param profile: the profile wire, must be planar, or a line segment
     * @param join: the join type (only support Arc at the moment)
     * @param axeProf: determine the coordinate system for the profile
     * @param solid: whether to make a solid
     * @param profOnSpine: whether the profile is connect with the spine
     * @param op: optional string to be encoded into topo naming for indicating
     *            the operation
     *
     * @return The original content of this TopoShape is discarded and replaced
     *         with the new shape. The function returns the TopoShape itself as
     *         a self reference so that multiple operations can be carried out
     *         for the same shape in the same line of code.
     */
    TopoShape& makeElementEvolve(
        const TopoShape& spine,
        const TopoShape& profile,
        JoinType join = JoinType::arc,
        CoordinateSystem = CoordinateSystem::global,
        MakeSolid solid = MakeSolid::noSolid,
        Spine profOnSpine = Spine::notOn,
        double tol = 0.0,
        const char* op = nullptr
    );

    /** Make an evolved shape using this shape as spine
     *
     * An evolved shape is built from a planar spine (face or wire) and a
     * profile (wire). The evolved shape is the unlooped sweep (pipe) of the
     * profile along the spine. Self-intersections are removed.
     *
     * @param profile: the profile wire, must be planar, or a line segment
     * @param join: the join type (only support Arc at the moment)
     * @param axeProf: determine the coordinate system for the profile
     * @param solid: whether to make a solid
     * @param profOnSpine: whether the profile is connect with the spine
     * @param op: optional string to be encoded into topo naming for indicating
     *            the operation
     *
     * @return Return the new shape. The TopoShape itself is not modified.
     */
    TopoShape makeElementEvolve(
        const TopoShape& profile,
        JoinType join = JoinType::arc,
        CoordinateSystem axeProf = CoordinateSystem::global,
        MakeSolid solid = MakeSolid::noSolid,
        Spine profOnSpine = Spine::notOn,
        double tol = 0.0,
        const char* op = nullptr
    )
    {
        return TopoShape(0, Hasher)
            .makeElementEvolve(*this, profile, join, axeProf, solid, profOnSpine, tol, op);
    }

    /** Make an loft that is a shell or solid passing through a set of sections in a given sequence
     *
     * @param sources: the source shapes. The first shape is used as the spine,
     *                 and the rest as sections. The sections can be of any
     *                 type, but only wires are used. The first and last
     *                 section may be vertex.
     * @param isSolid: whether to make a solid
     * @param isRuled: if isRuled then the faces generated between the edges of
     *                 two consecutive section wires are ruled surfaces. If
     *                 notRuled, then they are smoothed out by approximation
     * @param isClosed: If isClosed, then the first section is duplicated to close
     *                  the loft as the last section
     * @param maxDegree: define the maximal U degree of the result surface
     * @param op: optional string to be encoded into topo naming for indicating
     *            the operation
     *
     * @return The original content of this TopoShape is discarded and replaced
     *         with the new shape. The function returns the TopoShape itself as
     *         a self reference so that multiple operations can be carried out
     *         for the same shape in the same line of code.
     */
    TopoShape& makeElementLoft(
        const std::vector<TopoShape>& sources,
        IsSolid isSolid,
        IsRuled isRuled,
        IsClosed isClosed = IsClosed::notClosed,
        Standard_Integer maxDegree = 5,
        const char* op = nullptr
    );

    /** Make a ruled surface
     *
     * @param sources: the source shapes, each of which must contain either a
     *                 single edge or a single wire.
     * @param orientation: A Qt::Orientation, where Qt::Horizontal is 1 and Qt::Vertical is 2.
     * @param op: optional string to be encoded into topo naming for indicating
     *            the operation
     *
     * @return The original content of this TopoShape is discarded and replaced
     *         with the new shape. The function returns the TopoShape itself as
     *         a self reference so that multiple operations can be carried out
     *         for the same shape in the same line of code.
     */
    TopoShape& makeElementRuledSurface(
        const std::vector<TopoShape>& source,
        int orientation = 0,
        const char* op = nullptr
    );

    /** Core function to generate mapped element names from shape history
     *
     * @param shape: the new shape
     * @param mapper: for mapping input shapes to generated/modified shapes
     * @param sources: list of source shapes.
     * @param op: optional string to be encoded into topo naming for indicating
     *            the operation
     *
     * @return The original content of this TopoShape is discarded and replaced
     *         with the given new shape. The function returns the TopoShape
     *         itself as a self reference so that multiple operations can be
     *         carried out for the same shape in the same line of code.
     */
    TopoShape& makeShapeWithElementMap(
        const TopoDS_Shape& shape,
        const Mapper& mapper,
        const std::vector<TopoShape>& sources,
        const char* op = nullptr
    );
    /**
     * When given a single shape to create a compound, two results are possible: either to simply
     * return the shape as given, or to force it to be placed in a Compound.
     */
    enum class SingleShapeCompoundCreationPolicy
    {
        returnShape,
        forceCompound
    };

    /** Make a compound shape
     *
     * @param shapes: input shapes
     * @param op: optional string to be encoded into topo naming for indicating
     *            the operation
     * @param policy: set behavior when only a single shape is given
     *
     * @return The original content of this TopoShape is discarded and replaced
     *         with the new shape. The function returns the TopoShape itself as
     *         a reference so that multiple operations can be carried out for
     *         the same shape in the same line of code.
     */
    TopoShape& makeElementCompound(
        const std::vector<TopoShape>& shapes,
        const char* op = nullptr,
        SingleShapeCompoundCreationPolicy policy = SingleShapeCompoundCreationPolicy::forceCompound
    );


    enum class ConnectionPolicy
    {
        requireSharedVertex,
        mergeWithTolerance
    };

    /** Make a compound of wires by connecting input edges
     *
     * @param shapes: input shapes. Can be any type of shape. Edges will be
     *                extracted for building wires.
     * @param op: optional string to be encoded into topo naming for indicating
     *            the operation
     * @param keepOrder: whether to respect the order of the input edges
     * @param tol: tolerance for checking the distance of two vertex to decide
     *             if two edges are connected
     * @param shared: if true, then only connect edges if they shared the same
     *                vertex, or else use \c tol to check for connection.
     * @param output: optional output mapping from wire edges to input edge.
     *                Note that edges may be modified after adding to the wire,
     *                so the output edges may not be the same as the input
     *                ones.
     *
     * @return The function produces either a wire or a compound of wires. The
     *         original content of this TopoShape is discarded and replaced
     *         with the new shape. The function returns the TopoShape itself as
     *         a reference so that multiple operations can be carried out for
     *         the same shape in the same line of code.
     */
    TopoShape& makeElementWires(
        const std::vector<TopoShape>& shapes,
        const char* op = nullptr,
        double tol = 0.0,
        ConnectionPolicy policy = ConnectionPolicy::mergeWithTolerance,
        TopoShapeMap* output = nullptr
    );


    /** Make a compound of wires by connecting input edges
     *
     * @param shape: input shape. Can be any type of shape. Edges will be
     *               extracted for building wires.
     * @param op: optional string to be encoded into topo naming for indicating
     *            the operation
     * @param keepOrder: whether to respect the order of the input edges
     * @param tol: tolerance for checking the distance of two vertex to decide
     *             if two edges are connected
     * @param policy: if requireSharedVertex, then only connect edges if they shared the same
     *                vertex. If mergeWithTolerance use \c tol to check for connection.
     * @param output: optional output mapping from wire edges to input edge.
     *                Note that edges may be modified after adding to the wire,
     *                so the output edges may not be the same as the input
     *                ones.
     *
     * @return The function produces either a wire or a compound of wires. The
     *         original content of this TopoShape is discarded and replaced
     *         with the new shape. The function returns the TopoShape itself as
     *         a reference so that multiple operations can be carried out for
     *         the same shape in the same line of code.
     */
    TopoShape& makeElementWires(
        const TopoShape& shape,
        const char* op = nullptr,
        double tol = 0.0,
        ConnectionPolicy policy = ConnectionPolicy::mergeWithTolerance,
        TopoShapeMap* output = nullptr
    );

    /** Make a compound of wires by connecting input edges in the given order
     *
     * @param shapes: input shapes. Can be any type of shape. Edges will be
     *                extracted for building wires.
     * @param op: optional string to be encoded into topo naming for indicating
     *            the operation
     * @param tol: tolerance for checking the distance of two vertex to decide
     *             if two edges are connected
     * @param output: optional output mapping from wire edges to input edge.
     *                Note that edges may be modified after adding to the wire,
     *                so the output edges may not be the same as the input
     *                ones.
     *
     * @return Same as makeElementWires() but respects the order of the input edges.
     *         The function produces either a wire or a compound of wires. The
     *         original content of this TopoShape is discarded and replaced
     *         with the new shape. The function returns the TopoShape itself as
     *         a reference so that multiple operations can be carried out for
     *         the same shape in the same line of code.
     */
    TopoShape& makeElementOrderedWires(
        const std::vector<TopoShape>& shapes,
        const char* op = nullptr,
        double tol = 0.0,
        TopoShapeMap* output = nullptr
    );

    /** Make a wire or compound of wires with the edges inside the this shape
     *
     * @param op: optional string to be encoded into topo naming for indicating
     *            the operation
     * @param keepOrder: whether to respect the order of the input edges
     * @param tol: tolerance for checking the distance of two vertex to decide
     *             if two edges are connected
     * @param shared: if true, then only connect edges if they shared the same
     *                vertex, or else use \c tol to check for connection.
     * @param output: optional output mapping from wire edges to input edge.
     *                Note that edges may be modified after adding to the wire,
     *                so the output edges may not be the same as the input
     *                ones.
     *
     *
     * @return The function returns a new shape of either a single wire or a
     *         compound of wires. The shape itself is not modified.
     */
    TopoShape makeElementWires(
        const char* op = nullptr,
        double tol = 0.0,
        ConnectionPolicy policy = ConnectionPolicy::mergeWithTolerance,
        TopoShapeMap* output = nullptr
    ) const
    {
        return TopoShape(0, Hasher).makeElementWires(*this, op, tol, policy, output);
    }

    /** Make a new shape with transformation that may contain non-uniform scaling
     *
     * @param source: input shape
     * @param mat: transformation matrix
     * @param op: optional string to be encoded into topo naming for indicating
     *            the operation
     * @param copy: whether to perform deep copy of the shape. If false, the
     *              shape will still be copied if there is scaling.
     *
     * @return The original content of this TopoShape is discarded and replaced
     *         with the new transformed shape. The function returns the
     *         TopoShape itself as a self reference so that multiple operations
     *         can be carried out for the same shape in the same line of code.
     */
    TopoShape& makeElementGTransform(
        const TopoShape& source,
        const Base::Matrix4D& mat,
        const char* op = nullptr,
        CopyType copy = CopyType::noCopy
    );

    /** Make a new shape with transformation that may contain non-uniform scaling
     *
     * @param source: input shape
     * @param mat: transformation matrix
     * @param op: optional string to be encoded into topo naming for indicating
     *            the operation
     * @param copy: whether to perform deep copy of the shape. If false, the
     *              shape will still be copied if there is scaling.
     *
     * @return Return a new shape with transformation. The shape itself is not
     *         modified
     */
    TopoShape makeElementGTransform(
        const Base::Matrix4D& mat,
        const char* op = nullptr,
        CopyType copy = CopyType::noCopy
    ) const
    {
        return TopoShape(Tag, Hasher).makeElementGTransform(*this, mat, op, copy);
    }

    /** Make a deep copy of the shape
     *
     * @param source: input shape
     * @param op: optional string to be encoded into topo naming for indicating
     *            the operation
     * @param copyGeom: whether to copy internal geometry of the shape
     * @param copyMesh: whether to copy internal meshes of the shape
     *
     * @return The original content of this TopoShape is discarded and replaced
     *         with a deep copy of the input shape. The function returns the
     *         TopoShape itself as a self reference so that multiple operations
     *         can be carried out for the same shape in the same line of code.
     */
    TopoShape& makeElementCopy(
        const TopoShape& source,
        const char* op = nullptr,
        bool copyGeom = true,
        bool copyMesh = false
    );

    /** Make a deep copy of the shape
     *
     * @param op: optional string to be encoded into topo naming for indicating
     *            the operation
     * @param copyGeom: whether to copy internal geometry of the shape
     * @param copyMesh: whether to copy internal meshes of the shape
     *
     * @return Return a deep copy of the shape. The shape itself is not
     *         modified
     */
    TopoShape makeElementCopy(const char* op = nullptr, bool copyGeom = true, bool copyMesh = false) const
    {
        return TopoShape(Tag, Hasher).makeElementCopy(*this, op, copyGeom, copyMesh);
    }


    /** Generalized shape making with mapped element name from shape history
     *
     * @param maker: op code from OpCodes
     * @param sources: list of source shapes.
     * @param op: optional string to be encoded into topo naming for indicating
     *            the operation
     * @param tol: tolerance option available to some shape making algorithm
     *
     * @return The original content of this TopoShape is discarded and replaced
     *         with the new shape built by the shape maker. The function
     *         returns the TopoShape itself as a self reference so that
     *         multiple operations can be carried out for the same shape in the
     *         same line of code.
     */
    TopoShape& makeElementBoolean(
        const char* maker,
        const std::vector<TopoShape>& sources,
        const char* op = nullptr,
        double tol = -1.0
    );
    /** Generalized shape making with mapped element name from shape history
     *
     * @param maker: op code from TopoShapeOpCodes
     * @param source: source shape.
     * @param op: optional string to be encoded into topo naming for indicating
     *            the operation
     * @param tol: tolerance option available to some shape making algorithm
     *
     * @return The original content of this TopoShape is discarded and replaced
     *         with the new shape built by the shape maker. The function
     *         returns the TopoShape itself as a self reference so that
     *         multiple operations can be carried out for the same shape in the
     *         same line of code.
     */
    TopoShape& makeElementBoolean(
        const char* maker,
        const TopoShape& source,
        const char* op = nullptr,
        double tol = -1.0
    );

    /** Generalized shape making with mapped element name from shape history
     *
     * @param maker: op code from TopoShapeOpCodes
     * @param op: optional string to be encoded into topo naming for indicating
     *            the operation
     * @param tol: tolerance option available to some shape making algorithm
     *
     * @return Returns the new shape with mappend element name generated from
     *         shape history using this shape as the source. The shape itself
     *         is not modified.
     */
    TopoShape makeElementBoolean(const char* maker, const char* op = nullptr, double tol = -1.0) const
    {
        return TopoShape(0, Hasher).makeElementBoolean(maker, *this, op, tol);
    }

    /** Make a mirrored shape
     *
     * @param source: the source shape
     * @param axis: the axis for mirroring
     * @param op: optional string to be encoded into topo naming for indicating
     *            the operation
     *
     * @return The original content of this TopoShape is discarded and replaced
     *         with the new shape. The function returns the TopoShape itself as
     *         a self reference so that multiple operations can be carried out
     *         for the same shape in the same line of code.
     */
    TopoShape& makeElementMirror(const TopoShape& source, const gp_Ax2& axis, const char* op = nullptr);
    /** Make a mirrored shape
     *
     * @param source: the source shape
     * @param axis: the axis for mirroring
     * @param op: optional string to be encoded into topo naming for indicating
     *            the operation
     *
     * @return Return the new shape. The TopoShape itself is not modified.
     */
    TopoShape makeElementMirror(const gp_Ax2& ax, const char* op = nullptr) const
    {
        return TopoShape(0, Hasher).makeElementMirror(*this, ax, op);
    }

    /** Make a cross section slice
     *
     * @param source: the source shape
     * @param dir: direction of the normal of the section plane
     * @param distance: distance to move the section plane
     * @param op: optional string to be encoded into topo naming for indicating
     *            the operation
     *
     * @return The original content of this TopoShape is discarded and replaced
     *         with the new shape. The function returns the TopoShape itself as
     *         a self reference so that multiple operations can be carried out
     *         for the same shape in the same line of code.
     */
    TopoShape& makeElementSlice(
        const TopoShape& source,
        const Base::Vector3d& dir,
        double distance,
        const char* op = nullptr
    );
    /** Make a cross section slice
     *
     * @param source: the source shape
     * @param dir: direction of the normal of the section plane
     * @param distance: distance to move the section plane
     * @param op: optional string to be encoded into topo naming for indicating
     *            the operation
     *
     * @return Return the new shape. The TopoShape itself is not modified.
     */
    TopoShape makeElementSlice(const Base::Vector3d& dir, double distance, const char* op = nullptr) const
    {
        return TopoShape(0, Hasher).makeElementSlice(*this, dir, distance, op);
    }

    /** Make multiple cross section slices
     *
     * @param source: the source shape
     * @param dir: direction of the normal of the section plane
     * @param distances: distances to move the section plane for making slices
     * @param op: optional string to be encoded into topo naming for indicating
     *            the operation
     *
     * @return The original content of this TopoShape is discarded and replaced
     *         with the new shape. The function returns the TopoShape itself as
     *         a self reference so that multiple operations can be carried out
     *         for the same shape in the same line of code.
     */
    TopoShape& makeElementSlices(
        const TopoShape& source,
        const Base::Vector3d& dir,
        const std::vector<double>& distances,
        const char* op = nullptr
    );
    /** Make multiple cross section slices
     *
     * @param source: the source shape
     * @param dir: direction of the normal of the section plane
     * @param distances: distances to move the section plane for making slices
     * @param op: optional string to be encoded into topo naming for indicating
     *            the operation
     *
     * @return Return the new shape. The TopoShape itself is not modified.
     */
    TopoShape makeElementSlices(
        const Base::Vector3d& dir,
        const std::vector<double>& distances,
        const char* op = nullptr
    ) const
    {
        return TopoShape(0, Hasher).makeElementSlices(*this, dir, distances, op);
    }

    /* Make fillet shape
     *
     * @param source: the source shape
     * @param edges: the edges of the source shape where to make fillets
     * @param radius1: the radius of the beginning of the fillet
     * @param radius2: the radius of the ending of the fillet
     * @param op: optional string to be encoded into topo naming for indicating
     *            the operation
     *
     * @return The original content of this TopoShape is discarded and replaced
     *         with the new shape. The function returns the TopoShape itself as
     *         a self reference so that multiple operations can be carried out
     *         for the same shape in the same line of code.
     */
    TopoShape& makeElementFillet(
        const TopoShape& source,
        const std::vector<TopoShape>& edges,
        double radius1,
        double radius2,
        const char* op = nullptr
    );
    /* Make fillet shape
     *
     * @param source: the source shape
     * @param edges: the edges of the source shape where to make fillets
     * @param radius1: the radius of the beginning of the fillet
     * @param radius2: the radius of the ending of the fillet
     * @param op: optional string to be encoded into topo naming for indicating
     *            the operation
     *
     * @return Return the new shape. The TopoShape itself is not modified.
     */
    TopoShape makeElementFillet(
        const std::vector<TopoShape>& edges,
        double radius1,
        double radius2,
        const char* op = nullptr
    ) const
    {
        return TopoShape(0, Hasher).makeElementFillet(*this, edges, radius1, radius2, op);
    }

    /* Make chamfer shape
     *
     * @param source: the source shape
     * @param edges: the edges of the source shape where to make chamfers
     * @param radius1: the radius of the beginning of the chamfer
     * @param radius2: the radius of the ending of the chamfer
     * @param op: optional string to be encoded into topo naming for indicating
     *            the operation
     *
     * @return The original content of this TopoShape is discarded and replaced
     *         with the new shape. The function returns the TopoShape itself as
     *         a self reference so that multiple operations can be carried out
     *         for the same shape in the same line of code.
     */
    TopoShape& makeElementChamfer(
        const TopoShape& source,
        const std::vector<TopoShape>& edges,
        ChamferType chamferType,
        double radius1,
        double radius2,
        const char* op = nullptr,
        Flip flipDirection = Flip::none
    );
    /* Make chamfer shape
     *
     * @param source: the source shape
     * @param edges: the edges of the source shape where to make chamfers
     * @param radius1: the radius of the beginning of the chamfer
     * @param radius2: the radius of the ending of the chamfer
     * @param op: optional string to be encoded into topo naming for indicating
     *            the operation
     *
     * @return Return the new shape. The TopoShape itself is not modified.
     */
    TopoShape makeElementChamfer(
        const std::vector<TopoShape>& edges,
        ChamferType chamferType,
        double radius1,
        double radius2,
        const char* op = nullptr,
        Flip flipDirection = Flip::none
    ) const
    {
        return TopoShape(0, Hasher)
            .makeElementChamfer(*this, edges, chamferType, radius1, radius2, op, flipDirection);
    }

    /** Make a new shape with transformation
     *
     * @param source: input shape
     * @param mat: transformation matrix
     * @param op: optional string to be encoded into topo naming for indicating
     *            the operation
     * @param checkScale: whether to check if the transformation matrix
     *                    contains scaling factor.
     * @param copy: whether to perform deep copy of the shape. If noCopy, and
     *              checkScale, then the shape will be copied if there
     *              is scaling.
     *
     * @return Returns true if scaling is performed.
     *
     * The original content of this TopoShape is discarded and replaced with
     * the new transformed shape.
     */
    bool _makeElementTransform(
        const TopoShape& source,
        const Base::Matrix4D& mat,
        const char* op = nullptr,
        CheckScale checkScale = CheckScale::noScaleCheck,
        CopyType copy = CopyType::noCopy
    );

    /** Make a new shape with transformation
     *
     * @param source: input shape
     * @param mat: transformation matrix
     * @param op: optional string to be encoded into topo naming for indicating
     *            the operation
     * @param checkScale: whether to check if the transformation matrix
     *                    contains scaling factor.
     * @param copy: whether to perform deep copy of the shape. If noCopy, and
     *              checkScale, then the shape will be copied if there
     *              is scaling.
     *
     * @return The original content of this TopoShape is discarded and replaced
     *         with the new transformed shape. The function returns the
     *         TopoShape itself as a self reference so that multiple operations
     *         can be carried out for the same shape in the same line of code.
     */
    TopoShape& makeElementTransform(
        const TopoShape& source,
        const Base::Matrix4D& mat,
        const char* op = nullptr,
        CheckScale checkScale = CheckScale::noScaleCheck,
        CopyType copy = CopyType::noCopy
    )
    {
        _makeElementTransform(source, mat, op, checkScale, copy);
        return *this;
    }

    /** Make a new shape with transformation
     *
     * @param source: input shape
     * @param mat: transformation matrix
     * @param op: optional string to be encoded into topo naming for indicating
     *            the operation
     * @param checkScale: whether to check if the transformation matrix
     *                    contains scaling factor.
     * @param copy: whether to perform deep copy of the shape. If noCopy, and
     *              checkScale, then the shape will be copied if there
     *              is scaling.
     *
     * @return Return a new shape with transformation. The shape itself is not
     *         modified
     */
    TopoShape makeElementTransform(
        const Base::Matrix4D& mat,
        const char* op = nullptr,
        CheckScale checkScale = CheckScale::noScaleCheck,
        CopyType copy = CopyType::noCopy
    )
    {
        return TopoShape(Tag, Hasher).makeElementTransform(*this, mat, op, checkScale, copy);
    }

    /** Make a new shape with transformation
     *
     * @param source: input shape
     * @param trsf: OCCT transformation matrix
     * @param op: optional string to be encoded into topo naming for indicating
     *            the operation
     * @param copy: whether to perform deep copy of the shape.
     *
     * @return The original content of this TopoShape is discarded and replaced
     *         with the new transformed shape. The function returns the
     *         TopoShape itself as a self reference so that multiple operations
     *         can be carried out for the same shape in the same line of code.
     */
    TopoShape& makeElementTransform(
        const TopoShape& shape,
        const gp_Trsf& trsf,
        const char* op = nullptr,
        CopyType copy = CopyType::noCopy
    );

    /** Make a new shape with transformation
     *
     * @param source: input shape
     * @param trsf: OCCT transformation matrix
     * @param op: optional string to be encoded into topo naming for indicating
     *            the operation
     * @param copy: whether to perform deep copy of the shape.
     *
     * @return Return a new shape with transformation. The shape itself is not
     *         modified
     */
    TopoShape makeElementTransform(
        const gp_Trsf& trsf,
        const char* op = nullptr,
        CopyType copy = CopyType::noCopy
    )
    {
        return TopoShape(Tag, Hasher).makeElementTransform(*this, trsf, op, copy);
    }

    /* Make draft shape
     *
     * @param source: the source shape
     * @param faces: the faces of the source shape to make draft faces
     * @param pullDirection: the pulling direction for making the draft
     * @param angle: the angle of the draft
     * @param neutralPlane: the neutral plane used as a reference to decide pulling direction
     * @param retry: whether to keep going by skipping faces that failed to create draft
     * @param op: optional string to be encoded into topo naming for indicating
     *            the operation
     *
     * @return The original content of this TopoShape is discarded and replaced
     *         with the new shape. The function returns the TopoShape itself as
     *         a self reference so that multiple operations can be carried out
     *         for the same shape in the same line of code.
     */
    TopoShape& makeElementDraft(
        const TopoShape& source,
        const std::vector<TopoShape>& faces,
        const gp_Dir& pullDirection,
        double angle,
        const gp_Pln& neutralPlane,
        bool retry = true,
        const char* op = nullptr
    );
    /* Make draft shape
     *
     * @param source: the source shape
     * @param faces: the faces of the source shape to make draft faces
     * @param pullDirection: the pulling direction for making the draft
     * @param angle: the angle of the draft
     * @param neutralPlane: the neutral plane used as a reference to decide pulling direction
     * @param retry: whether to keep going by skipping faces that failed to create draft
     * @param op: optional string to be encoded into topo naming for indicating
     *            the operation
     *
     * @return Return the new shape. The TopoShape itself is not modified.
     */
    TopoShape makeElementDraft(
        const std::vector<TopoShape>& faces,
        const gp_Dir& pullDirection,
        double angle,
        const gp_Pln& neutralPlane,
        bool retry = true,
        const char* op = nullptr
    ) const
    {
        return TopoShape(0, Hasher)
            .makeElementDraft(*this, faces, pullDirection, angle, neutralPlane, retry, op);
    }

    /* Make a shell using this shape
     * @param silent: whether to throw exception on failure
     * @param op: optional string to be encoded into topo naming for indicating
     *            the operation
     *
     * @return The original content of this TopoShape is discarded and replaced
     *         with the new shape. The function returns the TopoShape itself as
     *         a self reference so that multiple operations can be carried out
     *         for the same shape in the same line of code.
     */
    TopoShape& makeElementShell(bool silent = true, const char* op = nullptr);

    /* Make a shell with input wires
     *
     * @param wires: input wires
     * @param silent: whether to throw exception on failure
     * @param op: optional string to be encoded into topo naming for indicating
     *            the operation
     *
     * @return The original content of this TopoShape is discarded and replaced
     *         with the new shape. The function returns the TopoShape itself as
     *         a self reference so that multiple operations can be carried out
     *         for the same shape in the same line of code.
     */
    TopoShape& makeElementShellFromWires(
        const std::vector<TopoShape>& wires,
        bool silent = true,
        const char* op = nullptr
    );
    /* Make a shell with input wires
     *
     * @param wires: input wires
     * @param silent: whether to throw exception on failure
     * @param op: optional string to be encoded into topo naming for indicating
     *            the operation
     *
     * @return Return the new shape. The TopoShape itself is not modified.
     */
    TopoShape& makeElementShellFromWires(bool silent = true, const char* op = nullptr)
    {
        return makeElementShellFromWires(getSubTopoShapes(TopAbs_WIRE), silent, op);
    }

    /** Make a planar face with the input wires or edges
     *
     * @param shapes: input shapes. Can be either edges, wires, or compound of
     *                those two types
     * @param op: optional string to be encoded into topo naming for indicating
     *            the operation
     * @param maker: optional type name of the face maker. If not given,
     *               default to "Part::FaceMakerBullseye"
     * @param plane: optional plane of the face.
     *
     * @return The function creates a planar face. The original content of this
     *         TopoShape is discarded and replaced with the new shape. The
     *         function returns the TopoShape itself as a reference so that
     *         multiple operations can be carried out for the same shape in the
     *         same line of code.
     */
    TopoShape& makeElementFace(
        const std::vector<TopoShape>& shapes,
        const char* op = nullptr,
        const char* maker = nullptr,
        const gp_Pln* plane = nullptr
    );
    /** Make a planar face with the input wire or edge
     *
     * @param shape: input shape. Can be either edge, wire, or compound of
     *               those two types
     * @param op: optional string to be encoded into topo naming for indicating
     *            the operation
     * @param maker: optional type name of the face maker. If not given,
     *               default to "Part::FaceMakerBullseye"
     * @param plane: optional plane of the face.
     *
     * @return The function creates a planar face. The original content of this
     *         TopoShape is discarded and replaced with the new shape. The
     *         function returns the TopoShape itself as a reference so that
     *         multiple operations can be carried out for the same shape in the
     *         same line of code.
     */
    TopoShape& makeElementFace(
        const TopoShape& shape,
        const char* op = nullptr,
        const char* maker = nullptr,
        const gp_Pln* plane = nullptr
    );
    /** Make a planar face using this shape
     *
     * @param op: optional string to be encoded into topo naming for indicating
     *            the operation
     * @param maker: optional type name of the face maker. If not given,
     *               default to "Part::FaceMakerBullseye"
     * @param plane: optional plane of the face.
     *
     * @return The function returns a new planar face made using the wire or edge
     *         inside this shape. The shape itself is not modified.
     */
    TopoShape makeElementFace(
        const char* op = nullptr,
        const char* maker = nullptr,
        const gp_Pln* plane = nullptr
    ) const
    {
        return TopoShape(0, Hasher).makeElementFace(*this, op, maker, plane);
    }

    /** Make a face with BSpline (or Bezier) surface
     *
     * @param shapes: input shapes of any type, but only edges inside the shape
     *                will be used.
     * @param style: surface filling style. @sa FillingStyle
     * @param keepBezier: whether to create Bezier surface if the input edge
     *                    has Bezier curve.
     * @param op: optional string to be encoded into topo naming for indicating
     *            the operation
     *
     * @return The function creates a face with either BSpline or Bezier
     *         surface. The original content of this TopoShape is discarded and
     *         replaced with the new shape. The function returns the TopoShape
     *         itself as a self reference so that multiple operations can be
     *         carried out for the same shape in the same line of code.
     */
    TopoShape& makeElementBSplineFace(
        const std::vector<TopoShape>& input,
        FillingStyle style = FillingStyle::stretch,
        bool keepBezier = false,
        const char* op = nullptr
    );
    /** Make a face with BSpline (or Bezier) surface
     *
     * @param shape: input shape of any type, but only edges inside the shape
     *               will be used.
     * @param style: surface filling style. @sa FillingStyle
     * @param keepBezier: whether to create Bezier surface if the input edge
     *                    has Bezier curve.
     * @param op: optional string to be encoded into topo naming for indicating
     *            the operation
     *
     * @return The function creates a face with either BSpline or Bezier
     *         surface. The original content of this TopoShape is discarded and
     *         replaced with the new shape. The function returns the TopoShape
     *         itself as a self reference so that multiple operations can be
     *         carried out for the same shape in the same line of code.
     */
    TopoShape& makeElementBSplineFace(
        const TopoShape& input,
        FillingStyle style = FillingStyle::stretch,
        bool keepBezier = false,
        const char* op = nullptr
    );
    /** Make a face with BSpline (or Bezier) surface
     *
     * @param style: surface filling style. @sa FillingStyle
     * @param keepBezier: whether to create Bezier surface if the input edge
     *                    has Bezier curve.
     * @param op: optional string to be encoded into topo naming for indicating
     *            the operation
     *
     * @return The function returns a new face with either BSpline or Bezier
     *         surface. The shape itself is not modified.
     */
    TopoShape makeElementBSplineFace(
        FillingStyle style = FillingStyle::stretch,
        bool keepBezier = false,
        const char* op = nullptr
    )
    {
        return TopoShape(0, Hasher).makeElementBSplineFace(*this, style, keepBezier, op);
    }


    struct BRepFillingParams;

    /** Provides information about the continuity of a curve.
     *  Corresponds to OCCT type GeomAbs_Shape
     */
    enum class Continuity
    {
        /// Only geometric continuity
        C0,
        /** for each point on the curve, the tangent vectors 'on the right' and 'on
         *  the left' are collinear with the same orientation.
         */
        G1,
        /** Continuity of the first derivative. The 'C1' curve is also 'G1' but, in
         *  addition, the tangent vectors 'on the right' and 'on the left' are equal.
         */
        C1,

        /** For each point on the curve, the normalized normal vectors 'on the
         *  right' and 'on the left' are equal.
         */
        G2,

        /// Continuity of the second derivative.
        C2,

        /// Continuity of the third derivative.
        C3,

        /** Continuity of the N-th derivative, whatever is the value given for N
         * (infinite order of continuity). Also provides information about the
         * continuity of a surface.
         */
        CN,
    };

    /** Make a non-planar filled face with boundary and/or constraint edge/wire
     *
     * @param shapes: input shapes of any type. The function will automatically
     *                discover connected and closed edges to be used as the
     *                boundary of the new face. Any other vertex, edge,
     *                and/or face will be used as constraints to fine tune the
     *                surface generation.
     * @param params: @sa BRepFillingParams
     * @param op: optional string to be encoded into topo naming for indicating
     *            the operation
     *
     * @return The function creates a face with BSpline surface. The original
     *         content of this TopoShape is discarded and replaced with the new
     *         shape. The function returns the TopoShape itself as a self
     *         reference so that multiple operations can be carried out for the
     *         same shape in the same line of code.
     *
     * @sa OCCT BRepOffsetAPI_MakeFilling
     */
    TopoShape& makeElementFilledFace(
        const std::vector<TopoShape>& shapes,
        const BRepFillingParams& params,
        const char* op = nullptr
    );


    /** Make a solid using shells or CompSolid
     *
     * @param shape: input shape of either a shell, a compound of shells, or a
     *               CompSolid.
     * @param op: optional string to be encoded into topo naming for indicating
     *            the operation
     *
     * @return The function produces a solid. The original content of this
     *         TopoShape is discarded and replaced with the new shape. The
     *         function returns the TopoShape itself as a self reference so
     *         that multiple operations can be carried out for the same shape
     *         in the same line of code.
     */
    TopoShape& makeElementSolid(const TopoShape& shape, const char* op = nullptr);
    /** Make a solid using this shape
     *
     * @param op: optional string to be encoded into topo naming for indicating
     *            the operation
     *
     * @return The function returns a new solid using the shell or CompSolid
     *         inside this shape. The shape itself is not modified.
     */
    TopoShape makeElementSolid(const char* op = nullptr) const
    {
        return TopoShape(0, Hasher).makeElementSolid(*this, op);
    }


    /** Generic shape making with mapped element name from shape history
     *
     * @param mkShape: OCCT shape maker.
     * @param sources: list of source shapes.
     * @param op: optional string to be encoded into topo naming for indicating
     *            the operation
     *
     * @return The original content of this TopoShape is discarded and replaced
     *         with the new shape built by the shape maker. The function
     *         returns the TopoShape itself as a self reference so that
     *         multiple operations can be carried out for the same shape in the
     *         same line of code.
     */
    TopoShape& makeElementShape(
        BRepBuilderAPI_MakeShape& mkShape,
        const std::vector<TopoShape>& sources,
        const char* op = nullptr
    );
    /** Generic shape making with mapped element name from shape history
     *
     * @param mkShape: OCCT shape maker.
     * @param source: source shape.
     * @param op: optional string to be encoded into topo naming for indicating
     *            the operation
     *
     * @return The original content of this TopoShape is discarded and replaced
     *         with the new shape built by the shape maker. The function
     *         returns the TopoShape itself as a self reference so that
     *         multiple operations can be carried out for the same shape in the
     *         same line of code.
     */
    TopoShape& makeElementShape(
        BRepBuilderAPI_MakeShape& mkShape,
        const TopoShape& source,
        const char* op = nullptr
    );
    /** Generic shape making with mapped element name from shape history
     *
     * @param mkShape: OCCT shape maker.
     * @param op: optional string to be encoded into topo naming for indicating
     *            the operation
     *
     * @return Returns the new shape built by the shape maker with mappend element
     *         name generated using this shape as the source. The shape itself
     *         is not modified.
     */
    TopoShape makeElementShape(BRepBuilderAPI_MakeShape& mkShape, const char* op = nullptr) const
    {
        return TopoShape(0, Hasher).makeElementShape(mkShape, *this, op);
    }

    /** Specialized shape making for BRepBuilderAPI_MakePrism with mapped element name
     *
     * @param mkShape: OCCT shape maker.
     * @param sources: list of source shapes.
     * @param op: optional string to be encoded into topo naming for indicating
     *            the operation
     *
     * @return The original content of this TopoShape is discarded and replaced
     *         with the new shape built by the shape maker. The function
     *         returns the TopoShape itself as a self reference so that
     *         multiple operations can be carried out for the same shape in the
     *         same line of code.
     */
    TopoShape& makeElementShape(
        BRepFeat_MakePrism& mkShape,
        const std::vector<TopoShape>& sources,
        const TopoShape& uptoface,
        const char* op
    );

    /* Toponaming migration, February 2014:
     * Note that the specialized versions of makeElementShape for operations that do not
     * inherit from BRepBuilderAPI_MakeShape  ( like BRepBuilderAPI_Sewing ) have been removed.
     * Rather than restore them, code that calls them should be changed to call
     * makeShapeWithElementMap directly.  For example:
     * makeElementShape(sewer, sources)
     * makeShapeWithElementMap(sewer.SewedShape(), MapperSewing(sewer), sources, OpCodes::Sewing);
     * Note that if op exists in the method, it should be checked for null and overridden with
     * the appropriate operation if so.
     */

    friend class TopoShapeCache;

private:
    // Cache storage
    mutable std::shared_ptr<TopoShapeCache> _parentCache;
    mutable std::shared_ptr<TopoShapeCache> _cache;
    mutable TopLoc_Location _subLocation;

    /** Helper class to ensure synchronization of element map and cache
     *
     * It exposes constant methods of OCCT TopoDS_Shape unchanged, and wraps all
     * non-constant method to auto-clear the element names in the owner TopoShape
     */
    class ShapeProtector: public TopoDS_Shape
    {
    public:
        using TopoDS_Shape::TopoDS_Shape;
        using TopoDS_Shape::operator=;

        explicit ShapeProtector(TopoShape& owner)
            : _owner(&owner)
        {}

        ShapeProtector(TopoShape& owner, const TopoDS_Shape& shape)
            : TopoDS_Shape(shape)
            , _owner(&owner)
        {}

        void Nullify()
        {
            if (!this->IsNull()) {
                _owner->resetElementMap();
                _owner->_cache.reset();
                _owner->_parentCache.reset();
            }
        }

        const TopLoc_Location& Location() const
        {
            // Some platforms do not support "using TopoDS_Shape::Location" here because of an
            // ambiguous lookup, so implement it manually.
            return TopoDS_Shape::Location();
        }

        void Location(const TopLoc_Location& Loc)
        {
            // Location does not affect element map or cache
            TopoShape::locate(*dynamic_cast<TopoDS_Shape*>(this), Loc);
        }

        void Move(const TopLoc_Location& position)
        {
            // Move does not affect element map or cache
            TopoShape::move(*dynamic_cast<TopoDS_Shape*>(this), position);
        }

        using TopoDS_Shape::Orientation;
        void Orientation(const TopAbs_Orientation Orient)
        {
            _owner->flushElementMap();
            TopoDS_Shape::Orientation(Orient);
            if (_owner->_cache) {
                _owner->initCache();
            }
        }

        void Reverse()
        {
            _owner->flushElementMap();
            TopoDS_Shape::Reverse();
            if (_owner->_cache) {
                _owner->initCache();
            }
        }

        void Complement()
        {
            _owner->flushElementMap();
            TopoDS_Shape::Complement();
            if (_owner->_cache) {
                _owner->initCache();
            }
        }

        void Compose(const TopAbs_Orientation Orient)
        {
            _owner->flushElementMap();
            TopoDS_Shape::Compose(Orient);
            if (_owner->_cache) {
                _owner->initCache();
            }
        }

        void EmptyCopy()
        {
            _owner->flushElementMap();
            TopoDS_Shape::EmptyCopy();
            if (_owner->_cache) {
                _owner->initCache();
            }
        }

        void TShape(const Handle(TopoDS_TShape) & T)
        {
            _owner->flushElementMap();
            TopoDS_Shape::TShape(T);
            if (_owner->_cache) {
                _owner->initCache();
            }
        }

        TopoShape* _owner;
    };

    ShapeProtector _Shape;

private:
    // Helper methods
    static std::vector<Data::ElementMap::MappedChildElements> createChildMap(
        size_t count,
        const std::vector<TopoShape>& shapes,
        const char* op
    );

    void setupChild(
        Data::ElementMap::MappedChildElements& child,
        TopAbs_ShapeEnum elementType,
        const TopoShape& topoShape,
        size_t shapeCount,
        const char* op
    );
    void mapSubElementForShape(const TopoShape& other, const char* op);
    void mapSubElementTypeForShape(
        const TopoShape& other,
        TopAbs_ShapeEnum type,
        const char* op,
        int count,
        bool forward,
        bool& warned
    );
    void mapCompoundSubElements(const std::vector<TopoShape>& shapes, const char* op);

    /** Given a set of edges, return a sorted list of connected edges
     *
     * @param edges: (input/output) input list of shapes. Must be of type edge.
     *               On return, the returned connected edges will be removed
     *               from this list. You can repeated call this function to find
     *               all wires.
     * @param keepOrder: whether to respect the order of the input edges
     * @param tol: tolerance for checking the distance of two vertex to decide
     *             if two edges are connected
     * @return Return a list of ordered connected edges.
     */
    static std::deque<TopoShape> sortEdges(
        std::list<TopoShape>& edges,
        bool keepOrder = false,
        double tol = 0.0
    );
    static TopoShape reverseEdge(const TopoShape& edge);
};

/** Shape mapper for generic BRepBuilderAPI_MakeShape derived class
 *
 * Uses BRepBuilderAPI_MakeShape::Modified/Generated() function to extract
 * shape history for generating mapped element names
 */
struct PartExport MapperMaker: TopoShape::Mapper
{
    BRepBuilderAPI_MakeShape& maker;
    explicit MapperMaker(BRepBuilderAPI_MakeShape& maker)
        : maker(maker)
    {}
    const std::vector<TopoDS_Shape>& modified(const TopoDS_Shape& s) const override;
    const std::vector<TopoDS_Shape>& generated(const TopoDS_Shape& s) const override;
};

struct PartExport MapperSewing: TopoShape::Mapper
{
    BRepBuilderAPI_Sewing& maker;
    explicit MapperSewing(BRepBuilderAPI_Sewing& maker)
        : maker(maker)
    {}
    const std::vector<TopoDS_Shape>& modified(const TopoDS_Shape& s) const override;
};

/** Shape mapper for BRepTools_History
 *
 * Uses BRepTools_History::Modified/Generated() function to extract
 * shape history for generating mapped element names
 */
struct PartExport MapperHistory: TopoShape::Mapper
{
    Handle(BRepTools_History) history;
    explicit MapperHistory(const Handle(BRepTools_History) & history);
    explicit MapperHistory(const Handle(BRepTools_ReShape) & reshape);
    explicit MapperHistory(ShapeFix_Root& fix);
    const std::vector<TopoDS_Shape>& modified(const TopoDS_Shape& s) const override;
    const std::vector<TopoDS_Shape>& generated(const TopoDS_Shape& s) const override;
};

}  // namespace Part
