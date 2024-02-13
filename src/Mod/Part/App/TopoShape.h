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

#ifndef PART_TOPOSHAPE_H
#define PART_TOPOSHAPE_H

#include <iosfwd>
#include <list>

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

namespace App
{
class Color;
}

namespace Part
{

struct ShapeHasher;
class TopoShape;
class TopoShapeCache;
typedef std::unordered_map<TopoShape, TopoShape, ShapeHasher, ShapeHasher> TopoShapeMap;

/* A special sub-class to indicate null shapes
 */
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
    TYPESYSTEM_HEADER_WITH_OVERRIDE();

public:
    explicit ShapeSegment(const TopoDS_Shape& ShapeIn)
        : Shape(ShapeIn)
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

/// Behavior of refines when a problem arises; either leave the shape untouched or throw an exception.
/// This replaces a boolean parameter in the original Toponaming branch by realthunder..
enum class RefineFail
{
    shapeUntouched,
    throwException
};

/** The representation for a CAD Shape
 */
class PartExport TopoShape: public Data::ComplexGeoData
{
    TYPESYSTEM_HEADER_WITH_OVERRIDE();

public:
    TopoShape(long Tag=0,
              App::StringHasherRef hasher=App::StringHasherRef(),
              const TopoDS_Shape &shape=TopoDS_Shape());  // Cannot be made explicit
    TopoShape(const TopoDS_Shape&,
              long Tag=0,
              App::StringHasherRef hasher=App::StringHasherRef());  // Cannot be made explicit
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

    /** @name Placement control */
    //@{
    /// set the transformation of the CasCade Shape
    void setTransform(const Base::Matrix4D& rclTrf) override;
    /// set the transformation of the CasCade Shape
    void setShapePlacement(const Base::Placement& rclTrf);
    /// get the transformation of the CasCade Shape
    Base::Placement getShapePlacement() const;
    /// get the transformation of the CasCade Shape
    Base::Matrix4D getTransform() const override;
    /// Bound box from the CasCade shape
    Base::BoundBox3d getBoundBox() const override;
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
    void getLinesFromSubShape(const TopoDS_Shape& shape,
                              std::vector<Base::Vector3d>& vertices,
                              std::vector<Line>& lines) const;
    void getFacesFromDomains(const std::vector<Domain>& domains,
                             std::vector<Base::Vector3d>& vertices,
                             std::vector<Facet>& faces) const;

public:
    /// Get the standard accuracy to be used with getPoints, getLines or getFaces
    double getAccuracy() const override;
    /** Get points from object with given accuracy */
    void getPoints(std::vector<Base::Vector3d>& Points,
                   std::vector<Base::Vector3d>& Normals,
                   double Accuracy,
                   uint16_t flags = 0) const override;
    /** Get lines from object with given accuracy */
    void getLines(std::vector<Base::Vector3d>& Points,
                  std::vector<Line>& lines,
                  double Accuracy,
                  uint16_t flags = 0) const override;
    void getFaces(std::vector<Base::Vector3d>& Points,
                  std::vector<Facet>& faces,
                  double Accuracy,
                  uint16_t flags = 0) const override;
    void setFaces(const std::vector<Base::Vector3d>& Points,
                  const std::vector<Facet>& faces,
                  double tolerance = 1.0e-06);
    void getDomains(std::vector<Domain>&) const;
    //@}

    /** @name Subelement management */
    //@{
    /// Unlike \ref getTypeAndIndex() this function only handles the supported
    /// element types.
    static std::pair<std::string, unsigned long> getElementTypeAndIndex(const char* Name);
    /** Sub type list
     *  List of different subelement types
     *  it is NOT a list of the subelements itself
     */
    std::vector<const char*> getElementTypes() const override;
    unsigned long countSubElements(const char* Type) const override;
    /// get the subelement by type and number
    Data::Segment* getSubElement(const char* Type, unsigned long) const override;
    /** Get lines from segment */
    void getLinesFromSubElement(const Data::Segment*,
                                std::vector<Base::Vector3d>& Points,
                                std::vector<Line>& lines) const override;
    /** Get faces from segment */
    void getFacesFromSubElement(const Data::Segment*,
                                std::vector<Base::Vector3d>& Points,
                                std::vector<Base::Vector3d>& PointNormals,
                                std::vector<Facet>& faces) const override;
    //@}
    /// get the Topo"sub"Shape with the given name
    TopoDS_Shape getSubShape(const char* Type, bool silent = false) const;
    TopoDS_Shape getSubShape(TopAbs_ShapeEnum type, int idx, bool silent = false) const;
    std::vector<TopoShape> getSubTopoShapes(TopAbs_ShapeEnum type = TopAbs_SHAPE) const;
    std::vector<TopoDS_Shape> getSubShapes(TopAbs_ShapeEnum type = TopAbs_SHAPE) const;
    unsigned long countSubShapes(const char* Type) const;
    unsigned long countSubShapes(TopAbs_ShapeEnum type) const;
    bool hasSubShape(const char* Type) const;
    bool hasSubShape(TopAbs_ShapeEnum type) const;
    /// get the Topo"sub"Shape with the given name
    PyObject* getPySubShape(const char* Type, bool silent = false) const;
    PyObject* getPyObject() override;
    void setPyObject(PyObject*) override;

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
    void exportFaceSet(double, double, const std::vector<App::Color>&, std::ostream&) const;
    void exportLineSet(std::ostream&) const;
    //@}

    /** @name Query*/
    //@{
    bool isNull() const;
    bool isValid() const;
    bool analyze(bool runBopCheck, std::ostream&) const;
    bool isClosed() const;
    bool isCoplanar(const TopoShape& other, double tol = -1) const;
    bool findPlane(gp_Pln& plane, double tol = -1) const;
    /// Returns true if the expansion of the shape is infinite, false otherwise
    bool isInfinite() const;
    /// Checks whether the shape is a planar face
    bool isPlanar(double tol = 1.0e-7) const;
    //@}

    /** @name Boolean operation*/
    //@{
    TopoDS_Shape cut(TopoDS_Shape) const;
    TopoDS_Shape cut(const std::vector<TopoDS_Shape>&, Standard_Real tolerance = 0.0) const;
    TopoDS_Shape common(TopoDS_Shape) const;
    TopoDS_Shape common(const std::vector<TopoDS_Shape>&, Standard_Real tolerance = 0.0) const;
    TopoDS_Shape fuse(TopoDS_Shape) const;
    TopoDS_Shape fuse(const std::vector<TopoDS_Shape>&, Standard_Real tolerance = 0.0) const;
    TopoDS_Shape oldFuse(TopoDS_Shape) const;
    TopoDS_Shape section(TopoDS_Shape, Standard_Boolean approximate = Standard_False) const;
    TopoDS_Shape section(const std::vector<TopoDS_Shape>&,
                         Standard_Real tolerance = 0.0,
                         Standard_Boolean approximate = Standard_False) const;
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
    TopoDS_Shape generalFuse(const std::vector<TopoDS_Shape>& sOthers,
                             Standard_Real tolerance,
                             std::vector<TopTools_ListOfShape>* mapInOut = nullptr) const;
    //@}

    /** Sweeping */
    //@{
    TopoDS_Shape makePipe(const TopoDS_Shape& profile) const;
    TopoDS_Shape makePipeShell(const TopTools_ListOfShape& profiles,
                               const Standard_Boolean make_solid,
                               const Standard_Boolean isFrenet = Standard_False,
                               int transition = 0) const;
    TopoDS_Shape makePrism(const gp_Vec&) const;
    /// revolve shape. Note: isSolid is deprecated (instead, use some Part::FaceMaker to make a
    /// face, first).
    TopoDS_Shape revolve(const gp_Ax1&, double d, Standard_Boolean isSolid = Standard_False) const;
    TopoDS_Shape makeSweep(const TopoDS_Shape& profile, double, int) const;
    TopoDS_Shape makeTube(double radius, double tol, int cont, int maxdeg, int maxsegm) const;
    TopoDS_Shape makeTorus(Standard_Real radius1,
                           Standard_Real radius2,
                           Standard_Real angle1,
                           Standard_Real angle2,
                           Standard_Real angle3,
                           Standard_Boolean isSolid = Standard_True) const;
    TopoDS_Shape makeHelix(Standard_Real pitch,
                           Standard_Real height,
                           Standard_Real radius,
                           Standard_Real angle = 0,
                           Standard_Boolean left = Standard_False,
                           Standard_Boolean style = Standard_False) const;
    TopoDS_Shape makeLongHelix(Standard_Real pitch,
                               Standard_Real height,
                               Standard_Real radius,
                               Standard_Real angle = 0,
                               Standard_Boolean left = Standard_False) const;
    TopoDS_Shape makeSpiralHelix(Standard_Real radiusbottom,
                                 Standard_Real radiustop,
                                 Standard_Real height,
                                 Standard_Real nbturns = 1,
                                 Standard_Real breakperiod = 1,
                                 Standard_Boolean left = Standard_False) const;
    TopoDS_Shape makeThread(Standard_Real pitch,
                            Standard_Real depth,
                            Standard_Real height,
                            Standard_Real radius) const;
    TopoDS_Shape makeLoft(const TopTools_ListOfShape& profiles,
                          Standard_Boolean isSolid,
                          Standard_Boolean isRuled,
                          Standard_Boolean isClosed = Standard_False,
                          Standard_Integer maxDegree = 5) const;
    TopoDS_Shape makeOffsetShape(double offset,
                                 double tol,
                                 bool intersection = false,
                                 bool selfInter = false,
                                 short offsetMode = 0,
                                 short join = 0,
                                 bool fill = false) const;
    TopoDS_Shape makeOffset2D(double offset,
                              short joinType = 0,
                              bool fill = false,
                              bool allowOpenResult = false,
                              bool intersection = false) const;
    TopoDS_Shape makeThickSolid(const TopTools_ListOfShape& remFace,
                                double offset,
                                double tol,
                                bool intersection = false,
                                bool selfInter = false,
                                short offsetMode = 0,
                                short join = 0) const;
    //@}

    /** @name Manipulation*/
    //@{
    void transformGeometry(const Base::Matrix4D& rclMat) override;
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
    enum SplitWireReorient {
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
     * @param inner: optional output of inner wires
     * @param reorient: wire reorientation, see SplitWireReorient
     *
     * @return Return the outer wire
     */
    TopoShape splitWires(std::vector<TopoShape> *inner = nullptr,
                         SplitWireReorient reorient = Reorient) const;

    /** @name Element name mapping aware shape maker
     *
     * To be complete in next batch of patches
     */
    //@{
    TopoShape&
    makeCompound(const std::vector<TopoShape>& shapes, const char* op = nullptr, bool force = true);

    TopoShape&
    makeWires(const TopoShape& shape, const char* op = nullptr, bool fix = false, double tol = 0.0);
    TopoShape makeWires(const char* op = nullptr, bool fix = false, double tol = 0.0) const
    {
        return TopoShape().makeWires(*this, op, fix, tol);
    }
    TopoShape& makeFace(const std::vector<TopoShape>& shapes,
                        const char* op = nullptr,
                        const char* maker = nullptr);
    TopoShape&
    makeFace(const TopoShape& shape, const char* op = nullptr, const char* maker = nullptr);
    TopoShape makeFace(const char* op = nullptr, const char* maker = nullptr) const
    {
        return TopoShape().makeFace(*this, op, maker);
    }
    bool _makeTransform(const TopoShape& shape,
                        const Base::Matrix4D& mat,
                        const char* op = nullptr,
                        bool checkScale = false,
                        bool copy = false);

    TopoShape& makeTransform(const TopoShape& shape,
                             const Base::Matrix4D& mat,
                             const char* op = nullptr,
                             bool checkScale = false,
                             bool copy = false)
    {
        _makeTransform(shape, mat, op, checkScale, copy);
        return *this;
    }
    TopoShape makeTransform(const Base::Matrix4D& mat,
                            const char* op = nullptr,
                            bool checkScale = false,
                            bool copy = false) const
    {
        return TopoShape().makeTransform(*this, mat, op, checkScale, copy);
    }

    TopoShape& makeTransform(const TopoShape& shape,
                             const gp_Trsf& trsf,
                             const char* op = nullptr,
                             bool copy = false);
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

    TopoShape& makeGTransform(const TopoShape& shape,
                              const Base::Matrix4D& mat,
                              const char* op = nullptr,
                              bool copy = false);
    TopoShape
    makeGTransform(const Base::Matrix4D& mat, const char* op = nullptr, bool copy = false) const
    {
        return TopoShape().makeGTransform(*this, mat, op, copy);
    }
    
    /** Refine the input shape by merging faces/edges that share the same geometry
     *
     * @param source: input shape
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
    TopoShape& makeElementRefine(const TopoShape& source,
                                 const char* op = nullptr,
                                 RefineFail no_fail = RefineFail::throwException);

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
    TopoShape makeElementRefine(const char* op = nullptr,
                                RefineFail no_fail = RefineFail::throwException) const
    {
        return TopoShape(Tag, Hasher).makeElementRefine(*this, op, no_fail);
    }


    TopoShape& makeRefine(const TopoShape& shape,
                          const char* op = nullptr,
                          RefineFail no_fail = RefineFail::throwException);

    TopoShape makeRefine(const char* op = nullptr,
                         RefineFail no_fail = RefineFail::throwException) const
    {
        return TopoShape().makeRefine(*this, op, no_fail);
    }
    //@}

    static TopAbs_ShapeEnum shapeType(const char* type, bool silent = false);
    static TopAbs_ShapeEnum shapeType(char type, bool silent = false);
    TopAbs_ShapeEnum shapeType(bool silent = false) const;
    static const std::string& shapeName(TopAbs_ShapeEnum type, bool silent = false);
    const std::string& shapeName(bool silent = false) const;
    static std::pair<TopAbs_ShapeEnum, int> shapeTypeAndIndex(const char* name);

    Data::MappedName setElementComboName(const Data::IndexedName & element, 
                                         const std::vector<Data::MappedName> &names,
                                         const char *marker=nullptr,
                                         const char *op=nullptr,
                                         const Data::ElementIDRefs *sids=nullptr);

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
    std::vector<TopoDS_Shape> findAncestorsShapes(const TopoDS_Shape& subshape,
                                                  TopAbs_ShapeEnum type) const;
    /** Search sub shape
     *
     * unlike findShape(), the input shape does not have to be an actual
     * sub-shape of this shape. The sub-shape is searched by shape geometry
     *
     * @param subshape: a sub shape to search
     * @param names: optional output of found sub shape indexed based name
     * @param checkGeometry: whether to compare shape geometry
     * @param tol: tolerance to check coincident vertices
     * @param atol: tolerance to check for same angles
     */
    // TODO: Implement this method and its tests later in Toponaming Phase 3.
    // std::vector<TopoShape> searchSubShape(const TopoShape &subshape,
    //                                      std::vector<std::string> *names=nullptr,
    //                                      bool checkGeometry=true,
    //                                      double tol=1e-7, double atol=1e-12) const;
    //@}

    void copyElementMap(const TopoShape & topoShape, const char *op=nullptr);
    bool canMapElement(const TopoShape &other) const;
    void mapSubElement(const TopoShape &other,const char *op=nullptr, bool forceHasher=false);
    void mapSubElement(const std::vector<TopoShape> &shapes, const char *op=nullptr);
    void mapSubElementsTo(std::vector<TopoShape>& shapes, const char* op = nullptr) const;
    bool hasPendingElementMap() const;

    /** Helper class to return the generated and modified shape given an input shape
     *
     * Shape history information is extracted using OCCT APIs
     * BRepBuilderAPI_MakeShape::Generated/Modified(). However, there is often
     * some glitches in various derived class. So we use this class as an
     * abstraction, and create various derived classes to deal with the glitches.
     */
    struct PartExport Mapper {
        /// Helper vector for temporary storage of both generated and modified shapes
        mutable std::vector<TopoDS_Shape> _res;
        virtual ~Mapper() {}
        /// Return a list of shape generated from the given input shape
        virtual const std::vector<TopoDS_Shape> &generated(const TopoDS_Shape &) const {
            return _res;
        }
        /// Return a list of shape modified from the given input shape
        virtual const std::vector<TopoDS_Shape> &modified(const TopoDS_Shape &) const {
            return _res;
        }
    };

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
    TopoShape &makeShapeWithElementMap(const TopoDS_Shape &shape,
                                       const Mapper &mapper,
                                       const std::vector<TopoShape> &sources,
                                       const char *op=nullptr);
    /**
     * When given a single shape to create a compound, two results are possible: either to simply
     * return the shape as given, or to force it to be placed in a Compound.
     */
    enum class SingleShapeCompoundCreationPolicy {
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
    TopoShape& makeElementCompound(const std::vector<TopoShape>& shapes,
                                   const char* op = nullptr,
                                   SingleShapeCompoundCreationPolicy policy =
                                       SingleShapeCompoundCreationPolicy::forceCompound);


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
    TopoShape& makeElementWires(const std::vector<TopoShape>& shapes,
                                const char* op = nullptr,
                                double tol = 0.0,
                                ConnectionPolicy policy = ConnectionPolicy::mergeWithTolerance,
                                TopoShapeMap* output = nullptr);


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
    TopoShape& makeElementWires(const TopoShape& shape,
                                const char* op = nullptr,
                                double tol = 0.0,
                                ConnectionPolicy policy = ConnectionPolicy::mergeWithTolerance,
                                TopoShapeMap* output = nullptr);

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
    TopoShape& makeElementOrderedWires(const std::vector<TopoShape>& shapes,
                                       const char* op = nullptr,
                                       double tol = 0.0,
                                       TopoShapeMap* output = nullptr);

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
    TopoShape makeElementWires(const char* op = nullptr,
                               double tol = 0.0,
                               ConnectionPolicy policy = ConnectionPolicy::mergeWithTolerance,
                               TopoShapeMap* output = nullptr) const
    {
        return TopoShape(0, Hasher).makeElementWires(*this, op, tol, policy, output);
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
    TopoShape& makeElementCopy(const TopoShape& source,
                               const char* op = nullptr,
                               bool copyGeom = true,
                               bool copyMesh = false);

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
    TopoShape
    makeElementCopy(const char* op = nullptr, bool copyGeom = true, bool copyMesh = false) const
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
    TopoShape& makeElementBoolean(const char* maker,
                                  const std::vector<TopoShape>& sources,
                                  const char* op = nullptr,
                                  double tol = 0.0);
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
    TopoShape& makeElementBoolean(const char* maker,
                                  const TopoShape& source,
                                  const char* op = nullptr,
                                  double tol = 0.0);

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
    TopoShape
    makeElementBoolean(const char* maker, const char* op = nullptr, double tol = 0.0) const
    {
        return TopoShape(0, Hasher).makeElementBoolean(maker, *this, op, tol);
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
    TopoShape& makeElementShellFromWires(const std::vector<TopoShape>& wires,
                                         bool silent = true,
                                         const char* op = nullptr);
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

    TopoShape& makeElementFace(const std::vector<TopoShape>& shapes,
                               const char* op = nullptr,
                               const char* maker = nullptr,
                               const gp_Pln* plane = nullptr);
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
    TopoShape& makeElementFace(const TopoShape& shape,
                               const char* op = nullptr,
                               const char* maker = nullptr,
                               const gp_Pln* plane = nullptr);
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
    TopoShape makeElementFace(const char* op = nullptr,
                              const char* maker = nullptr,
                              const gp_Pln* plane = nullptr) const
    {
        return TopoShape(0, Hasher).makeElementFace(*this, op, maker, plane);
    }

    /// Filling style when making a BSpline face
    enum class FillingStyle
    {
        /// The style with the flattest patches
        Stretch,
        /// A rounded style of patch with less depth than those of Curved
        Coons,
        /// The style with the most rounded patches
        Curved,
    };

    struct BRepFillingParams;

    /** Provides information about the continuity of a curve.
     *  Corresponds to OCCT type GeomAbs_Shape
     */
    enum class Continuity {
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
    TopoShape& makeElementShape(BRepBuilderAPI_MakeShape& mkShape,
                                const std::vector<TopoShape>& sources,
                                const char* op = nullptr);
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
    TopoShape& makeElementShape(BRepBuilderAPI_MakeShape& mkShape,
                                const TopoShape& source,
                                const char* op = nullptr);
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

    /** Specialized shape making for BRepBuilderAPI_Sewing with mapped element name
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
    TopoShape& makeElementShape(BRepBuilderAPI_Sewing& mkShape,
                                const std::vector<TopoShape>& sources,
                                const char* op = nullptr);
    /** Specialized shape making for BRepBuilderAPI_Sewing with mapped element name
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
    TopoShape& makeElementShape(BRepBuilderAPI_Sewing& mkShape,
                                const TopoShape& source,
                                const char* op = nullptr);
    /** Specialized shape making for BRepBuilderAPI_Sewing with mapped element name
     *
     * @param mkShape: OCCT shape maker.
     * @param op: optional string to be encoded into topo naming for indicating
     *            the operation
     *
     * @return Returns the new shape built by the shape maker with mappend element
     *         name generated using this shape as the source. The shape itself
     *         is not modified.
     */
    TopoShape makeElementShape(BRepBuilderAPI_Sewing& mkShape, const char* op = nullptr) const
    {
        return TopoShape(0, Hasher).makeElementShape(mkShape, *this, op);
    }

    /** Specialized shape making for BRepBuilderAPI_ThruSections with mapped element name
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
    TopoShape& makeElementShape(BRepOffsetAPI_ThruSections& mkShape,
                                const std::vector<TopoShape>& sources,
                                const char* op = nullptr);
    /** Specialized shape making for BRepBuilderAPI_Sewing with mapped element name
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
    TopoShape& makeElementShape(BRepOffsetAPI_ThruSections& mkShape,
                                const TopoShape& source,
                                const char* op = nullptr);
    /** Specialized shape making for BRepBuilderAPI_Sewing with mapped element name
     *
     * @param mkShape: OCCT shape maker.
     * @param op: optional string to be encoded into topo naming for indicating
     *            the operation
     *
     * @return Returns the new shape built by the shape maker with mappend element
     *         name generated using this shape as the source. The shape itself
     *         is not modified.
     */
    TopoShape makeElementShape(BRepOffsetAPI_ThruSections& mkShape, const char* op = nullptr) const
    {
        return TopoShape(0, Hasher).makeElementShape(mkShape, *this, op);
    }

    /** Specialized shape making for BRepBuilderAPI_MakePipeShell with mapped element name
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
    TopoShape& makeElementShape(BRepOffsetAPI_MakePipeShell& mkShape,
                                const std::vector<TopoShape>& sources,
                                const char* op = nullptr);

    /** Specialized shape making for BRepBuilderAPI_MakeHalfSpace with mapped element name
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
    TopoShape& makeElementShape(BRepPrimAPI_MakeHalfSpace& mkShape,
                                const TopoShape& source,
                                const char* op = nullptr);
    /** Specialized shape making for BRepBuilderAPI_MakeHalfSpace with mapped element name
     *
     * @param mkShape: OCCT shape maker.
     * @param op: optional string to be encoded into topo naming for indicating
     *            the operation
     *
     * @return Returns the new shape built by the shape maker with mappend element
     *         name generated using this shape as the source. The shape itself
     *         is not modified.
     */
    TopoShape makeElementShape(BRepPrimAPI_MakeHalfSpace& mkShape, const char* op = nullptr) const
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
    TopoShape& makeElementShape(BRepFeat_MakePrism& mkShape,
                                const std::vector<TopoShape>& sources,
                                const TopoShape& uptoface,
                                const char* op);

    /** Helper class to return the generated and modified shape given an input shape
     *
     * Shape history information is extracted using OCCT APIs
     * BRepBuilderAPI_MakeShape::Generated/Modified(). However, there is often
     * some glitches in various derived class. So we use this class as an
     * abstraction, and create various derived classes to deal with the glitches.
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

        explicit ShapeProtector(TopoShape & owner)
            : _owner(&owner)
        {}

        ShapeProtector(TopoShape & owner, const TopoDS_Shape & shape)
            : TopoDS_Shape(shape), _owner(&owner)
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
    static std::vector<Data::ElementMap::MappedChildElements>
    createChildMap(size_t count, const std::vector<TopoShape>& shapes, const char* op);

    void setupChild(Data::ElementMap::MappedChildElements& child,
                    TopAbs_ShapeEnum elementType,
                    const TopoShape& topoShape,
                    size_t shapeCount,
                    const char* op);
    void mapSubElementForShape(const TopoShape& other, const char* op);
    void mapSubElementTypeForShape(const TopoShape& other,
                                   TopAbs_ShapeEnum type,
                                   const char* op,
                                   int count,
                                   bool forward,
                                   bool& warned);
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
    static std::deque<TopoShape>
    sortEdges(std::list<TopoShape>& edges, bool keepOrder = false, double tol = 0.0);
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

#endif  // PART_TOPOSHAPE_H
