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

#include <memory>
#include <iostream>
#include <climits>
#include <deque>
#include <iosfwd>
#include <list>
#include <unordered_map>
#include <unordered_set>

#include <TopoDS_Compound.hxx>
#include <TopoDS_Wire.hxx>
#include <TopTools_ListOfShape.hxx>
#include <TopTools_IndexedMapOfShape.hxx>

#include <App/ComplexGeoData.h>
#include <Base/Exception.h>
#include <Mod/Part/PartGlobal.h>

#include <QVector>

class BRepBuilderAPI_MakeShape;
class BRepTools_History;
class BRepTools_ReShape;
class ShapeFix_Root;
class BRepBuilderAPI_Sewing;
class BRepOffsetAPI_ThruSections;
class BRepFeat_MakePrism;
class BRepOffsetAPI_MakePipeShell;
class BRepOffsetAPI_DraftAngle;
class BRepPrimAPI_MakeHalfSpace; 
class gp_Ax1;
class gp_Ax2;
class gp_Pln;
class gp_Vec;
class gp_Trsf;
class gp_GTrsf;
class gp_Pln;
class gp_Dir;

namespace App {
class Color;
}

namespace Part
{

struct ShapeHasher;
class TopoShape;
typedef std::unordered_map<TopoShape, TopoShape, ShapeHasher, ShapeHasher> TopoShapeMap;

/* A special sub-class to indicate null shapes
 */
class PartExport NullShapeException : public Base::ValueError
{
public:
   /// Construction
   NullShapeException();
   NullShapeException(const char * sMessage);
   NullShapeException(const std::string& sMessage);
   /// Destruction
   virtual ~NullShapeException() throw() {}
};

/* A special sub-class to indicate boolean failures
 */
class PartExport BooleanException : public Base::CADKernelError
{
public:
   /// Construction
   BooleanException();
   BooleanException(const char * sMessage);
   BooleanException(const std::string& sMessage);
   /// Destruction
   virtual ~BooleanException() throw() {}
};

/** The representation for a CAD Shape
 */
class PartExport TopoShape : public Data::ComplexGeoData
{
    TYPESYSTEM_HEADER();

public:
    explicit TopoShape(long Tag=0,
                       App::StringHasherRef hasher=App::StringHasherRef(), 
                       const TopoDS_Shape &shape=TopoDS_Shape());

    TopoShape(const TopoDS_Shape&,
              long Tag=0,
              App::StringHasherRef hasher=App::StringHasherRef());

    TopoShape(const TopoShape&);
    ~TopoShape();

    void setShape(const TopoDS_Shape& shape, bool resetElementMap=true);

    inline void setShape(const TopoShape& shape) {
        *this = shape;
    }

    inline const TopoDS_Shape& getShape() const {
        return this->_Shape;
    }

    void operator = (const TopoShape&);

    bool operator == (const TopoShape &other) const {
        return _Shape.getShape().IsEqual(other._Shape);
    }

    virtual bool isSame (const Data::ComplexGeoData &other) const;

    /** @name Placement control */
    //@{
    /// set the transformation of the CasCade Shape
    void setTransform(const Base::Matrix4D& rclTrf);
    /// set the transformation of the CasCade Shape
    void setShapePlacement(const Base::Placement& rclTrf);
    /// get the transformation of the CasCade Shape
    Base::Placement getShapePlacement(void) const;
    /// get the transformation of the CasCade Shape
    Base::Matrix4D getTransform(void) const;
    /// Bound box from the CasCade shape
    Base::BoundBox3d getBoundBox(void)const;
    virtual bool getCenterOfGravity(Base::Vector3d& center) const;
    virtual bool getRotation(Base::Rotation& rot) const;
    static void convertTogpTrsf(const Base::Matrix4D& mtrx, gp_Trsf& trsf);
    static void convertToMatrix(const gp_Trsf& trsf, Base::Matrix4D& mtrx);
    static Base::Matrix4D convert(const gp_Trsf& trsf);
    static gp_Trsf convert(const Base::Matrix4D& mtrx);
    //@}

    /** @name Subelement management */
    //@{
    /** Sub type list
     *  List of different subelement types
     *  it is NOT a list of the subelements itself
     */
    virtual const std::vector<const char*>& getElementTypes(void) const;
    virtual unsigned long countSubElements(const char* Type) const;
    /// get the subelement by type and number
    virtual Data::Segment* getSubElement(const char* Type, unsigned long) const;
    /// get the subelement by name
    virtual Data::Segment* getSubElementByName(const char* Name) const;
    /** Get lines from segment */
    virtual void getLinesFromSubElement(
        const Data::Segment*,
        std::vector<Base::Vector3d> &Points,
        std::vector<Line> &lines) const;
    /** Get faces from segment */
    virtual void getFacesFromSubElement(
        const Data::Segment*,
        std::vector<Base::Vector3d> &Points,
        std::vector<Base::Vector3d> &PointNormals,
        std::vector<Facet> &faces) const;
    //@}
    /// get the Topo"sub"Shape with the given name
    TopoDS_Shape getSubShape(const char* Type, bool silent=false) const;
    TopoDS_Shape getSubShape(TopAbs_ShapeEnum type, int idx, bool silent=false) const;
    TopoShape getSubTopoShape(const char *Type, bool silent=false) const;
    TopoShape getSubTopoShape(TopAbs_ShapeEnum type, int idx, bool silent=false) const;
    std::vector<TopoShape> getSubTopoShapes(TopAbs_ShapeEnum type=TopAbs_SHAPE, TopAbs_ShapeEnum avoid=TopAbs_SHAPE) const;
    std::vector<TopoDS_Shape> getSubShapes(TopAbs_ShapeEnum type=TopAbs_SHAPE, TopAbs_ShapeEnum avoid=TopAbs_SHAPE) const;
    std::vector<TopoShape> getOrderedEdges(bool mapElement=true) const;
    std::vector<TopoShape> getOrderedVertexes(bool mapElement=true) const;
    unsigned long countSubShapes(const char* Type) const;
    unsigned long countSubShapes(TopAbs_ShapeEnum type) const;
    bool hasSubShape(const char *Type) const;
    bool hasSubShape(TopAbs_ShapeEnum type) const;
    /// get the Topo"sub"Shape with the given name
    PyObject * getPySubShape(const char* Type, bool silent=false) const;
    PyObject * getPyObject();
    void setPyObject(PyObject*);

    /** @name Save/restore */
    //@{
    void Save (Base::Writer &writer) const;
    void Restore(Base::XMLReader &reader);

    void SaveDocFile (Base::Writer &writer) const;
    void RestoreDocFile(Base::Reader &reader);
    unsigned int getMemSize (void) const;
    //@}

    /** @name Input/Output */
    //@{
    void read(const char *FileName);
    void write(const char *FileName) const;
    void dump(std::ostream& out) const;
    void importIges(const char *FileName);
    void importStep(const char *FileName);
    void importBrep(const char *FileName);
    void importBrep(std::istream&, int indicator=1);
    void importBinary(std::istream&);
    void exportIges(const char *FileName) const;
    void exportStep(const char *FileName) const;
    void exportBrep(const char *FileName) const;
    void exportBrep(std::ostream&) const;
    void exportBinary(std::ostream&);
    void exportStl (const char *FileName, double deflection) const;
    void exportFaceSet(double, double, const std::vector<App::Color>&, std::ostream&) const;
    void exportLineSet(std::ostream&) const;
    //@}

    /** @name Query*/
    //@{
    bool isNull() const;
    bool isValid() const;
    bool analyze(bool runBopCheck, std::ostream&) const;
    bool isClosed() const;
    bool isCoplanar(const TopoShape &other, double tol=-1, double atol=-1) const;
    bool findPlane(gp_Pln &pln, double tol=-1, double atol=-1) const;
    /// Returns true if the expansion of the shape is infinite, false otherwise
    bool isInfinite() const;
    /// Check if this shape is a single linear edge, works on BSplineCurve and BezierCurve
    bool isLinearEdge(Base::Vector3d *dir = nullptr, Base::Vector3d *base = nullptr) const;
    /// Check if this shape is a single planar face, works on BSplineSurface and BezierSurface
    bool isPlanarFace(double tol=1e-7) const;
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
    TopoDS_Shape section(TopoDS_Shape, Standard_Boolean approximate=Standard_False) const;
    TopoDS_Shape section(const std::vector<TopoDS_Shape>&, Standard_Real tolerance = 0.0, Standard_Boolean approximate=Standard_False) const;
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
    TopoDS_Shape generalFuse(const std::vector<TopoDS_Shape> &sOthers, Standard_Real tolerance, std::vector<TopTools_ListOfShape>* mapInOut = nullptr) const;
    //@}

    /** Sweeping */
    //@{
    TopoDS_Shape makePipe(const TopoDS_Shape& profile) const;
    TopoDS_Shape makePipeShell(const TopTools_ListOfShape& profiles, const Standard_Boolean make_solid,
        const Standard_Boolean isFrenet = Standard_False, int transition=0) const;
    TopoDS_Shape makePrism(const gp_Vec&) const;
    ///revolve shape. Note: isSolid is deprecated (instead, use some Part::FaceMaker to make a face, first).
    TopoDS_Shape revolve(const gp_Ax1&, double d, Standard_Boolean isSolid=Standard_False) const;
    TopoDS_Shape makeSweep(const TopoDS_Shape& profile, double, int) const;
    TopoDS_Shape makeTube(double radius, double tol, int cont, int maxdeg, int maxsegm) const;
    TopoDS_Shape makeTorus(Standard_Real radius1, Standard_Real radius2,
        Standard_Real angle1, Standard_Real angle2, Standard_Real angle3,
        Standard_Boolean isSolid=Standard_True) const;
    TopoDS_Shape makeHelix(Standard_Real pitch, Standard_Real height,
        Standard_Real radius, Standard_Real angle=0,
        Standard_Boolean left=Standard_False, Standard_Boolean style=Standard_False) const;
    TopoDS_Shape makeLongHelix(Standard_Real pitch, Standard_Real height,
        Standard_Real radius, Standard_Real angle=0,
        Standard_Boolean left=Standard_False) const;
    TopoDS_Shape makeSpiralHelix(Standard_Real radiusbottom, Standard_Real radiustop,
        Standard_Real height, Standard_Real nbturns=1, Standard_Real breakperiod=1,
        Standard_Boolean left=Standard_False) const;
    TopoDS_Shape makeThread(Standard_Real pitch, Standard_Real depth,
        Standard_Real height, Standard_Real radius) const;
    TopoDS_Shape makeLoft(const TopTools_ListOfShape& profiles, Standard_Boolean isSolid,
        Standard_Boolean isRuled, Standard_Boolean isClosed = Standard_False, Standard_Integer maxDegree = 5) const;
    TopoDS_Shape makeOffsetShape(double offset, double tol,
        bool intersection = false, bool selfInter = false,
        short offsetMode = 0, short join = 0, bool fill = false) const;
    TopoDS_Shape makeOffset2D(double offset, short joinType = 0,
        bool fill = false, bool allowOpenResult = false, bool intersection = false) const;
    TopoDS_Shape makeThickSolid(const TopTools_ListOfShape& remFace,
        double offset, double tol,
        bool intersection = false, bool selfInter = false,
        short offsetMode = 0, short join = 0) const;
    //@}

    /** @name Manipulation*/
    //@{
    void transformGeometry(const Base::Matrix4D &rclMat);
    TopoDS_Shape transformGShape(const Base::Matrix4D&, bool copy = false) const;
    /** Transform shape
     * 
     * @param mat: transformation matrix
     * @param copy: whether to copy the shape before transformation
     * @param checkScale: whether to check for non-uniform scaling. If not and
     * there is non-uniform scaling, exception will be raised. If non-uniform
     * scaling is detected, it will call makEGTransform() to do the
     * transformation.
     *
     * @return Return true only if non-uniform scaling is detected.
     */
    bool transformShape(const Base::Matrix4D &mat, bool copy, bool checkScale=false);
    TopoDS_Shape mirror(const gp_Ax2&) const;
    TopoDS_Shape toNurbs() const;
    TopoDS_Shape replaceShape(const std::vector< std::pair<TopoDS_Shape,TopoDS_Shape> >& s) const;
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

    /** Mesh this shape with the given accuracy
     *
     * Triagulate this shape with a given accuracy stored internally.
     *
     * @param linearDeflection: linear deflection.
     * @param angluarDeflection: angular deflection.
     * @param parallel: if true, shape will be meshed in parallel.
     * @param relative: if true, then deflection used for discretization of each
     *        edge will be <theLinDeflection> * <size of edge>. Deflection used
     *        for the faces will be the maximum deflection of their edges.
     */
    void meshShape(double linearDeflection=0.0,
                   double angularDeflection=0.0,
                   bool parallel = true,
                   bool relative = false) const;

    /** @name Getting basic geometric entities */
    //@{
    /** Get points from object with given accuracy */
    virtual void getPoints(std::vector<Base::Vector3d> &Points,
        std::vector<Base::Vector3d> &Normals,
        float Accuracy, uint16_t flags=0) const;
    virtual void getFaces(std::vector<Base::Vector3d> &Points,std::vector<Facet> &faces,
        float Accuracy, uint16_t flags=0) const;
    void setFaces(const std::vector<Base::Vector3d> &Points,
                  const std::vector<Facet> &faces, double tolerance=1.0e-06);
    void getDomains(std::vector<Domain>&) const;
    //@}

    /** @name Element name mapping aware shape maker 
     *
     * These functions are implemented in TopoShapeEx.cpp
     */
    //@{
    
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
    static std::deque<TopoShape> sortEdges(std::list<TopoShape> &edges, bool keepOrder=false, double tol=0.0);

    /** Make a compound shape
     * 
     * @param shapes: input shapes
     * @param op: optional string to be encoded into topo naming for indicating
     *            the operation
     * @param force: if true and there is only one input shape, then return
     *               that shape instead.  If false, then always return a
     *               compound, even if there is no input shape. 
     *
     * @return The original content of this TopoShape is discarded and replaced
     *         with the new shape. The function returns the TopoShape itself as
     *         a reference so that multiple operations can be carried out for
     *         the same shape in the same line of code.
     */
    TopoShape &makECompound(const std::vector<TopoShape> &shapes, const char *op=nullptr, bool force=true);

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
    TopoShape &makEWires(const std::vector<TopoShape> &shapes,
                         const char *op=nullptr,
                         double tol=0.0,
                         bool shared=false,
                         TopoShapeMap *output=nullptr);

    /** Make a compound of wires by connecting input edges 
     *
     * @param shape: input shape. Can be any type of shape. Edges will be
     *               extracted for building wires.
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
    TopoShape &makEWires(const TopoShape &shape,
                         const char *op=nullptr,
                         double tol=0.0,
                         bool shared=false,
                         TopoShapeMap *output=nullptr);

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
     * @return Same as makEWires() but respects the order of the input edges.
     *         The function produces either a wire or a compound of wires. The
     *         original content of this TopoShape is discarded and replaced
     *         with the new shape. The function returns the TopoShape itself as
     *         a reference so that multiple operations can be carried out for
     *         the same shape in the same line of code.
     */
    TopoShape &makEOrderedWires(const std::vector<TopoShape> &shapes,
                                const char *op=nullptr,
                                double tol=0.0,
                                TopoShapeMap *output=nullptr);

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
    TopoShape makEWires(const char *op=nullptr,
                        double tol=0.0,
                        bool shared=false,
                        TopoShapeMap *output=nullptr) const
    {
        return TopoShape(0,Hasher).makEWires(*this,op,tol,shared,output);
    }

    /** Make a planar face with the input wires or edges 
     *
     * @param shapes: input shapes. Can be either edges, wires, or compound of
     *                those two types
     * @param op: optional string to be encoded into topo naming for indicating
     *            the operation
     * @param maker: optional type name of the face maker. If not given,
     *               default to "Part::FaceMakerBullseye"
     * @param pln: optional plane of the face.
     *
     * @return The function creates a planar face. The original content of this
     *         TopoShape is discarded and replaced with the new shape. The
     *         function returns the TopoShape itself as a reference so that
     *         multiple operations can be carried out for the same shape in the
     *         same line of code.
     */
    TopoShape &makEFace(const std::vector<TopoShape> &shapes,
                        const char *op = nullptr,
                        const char *maker = nullptr,
                        const gp_Pln *pln = nullptr);
    /** Make a planar face with the input wire or edge 
     *
     * @param shape: input shape. Can be either edge, wire, or compound of
     *               those two types
     * @param op: optional string to be encoded into topo naming for indicating
     *            the operation
     * @param maker: optional type name of the face maker. If not given,
     *               default to "Part::FaceMakerBullseye"
     * @param pln: optional plane of the face.
     *
     * @return The function creates a planar face. The original content of this
     *         TopoShape is discarded and replaced with the new shape. The
     *         function returns the TopoShape itself as a reference so that
     *         multiple operations can be carried out for the same shape in the
     *         same line of code.
     */
    TopoShape &makEFace(const TopoShape &shape,
                        const char *op = nullptr,
                        const char *maker = nullptr,
                        const gp_Pln *pln = nullptr);
    /** Make a planar face using this shape 
     *
     * @param op: optional string to be encoded into topo naming for indicating
     *            the operation
     * @param maker: optional type name of the face maker. If not given,
     *               default to "Part::FaceMakerBullseye"
     * @param pln: optional plane of the face.
     *
     * @return The function returns a new planar face made using the wire or edge
     *         inside this shape. The shape itself is not modified.
     */
    TopoShape makEFace(const char *op = nullptr,
                       const char *maker = nullptr,
                       const gp_Pln *pln = nullptr) const {
        return TopoShape(0,Hasher).makEFace(*this,op,maker,pln);
    }

    /// Filling style when making a BSpline face
    enum FillingStyle {
        /// The style with the flattest patches
        FillingStyle_Strech,
        /// A rounded style of patch with less depth than those of Curved
        FillingStyle_Coons,
        /// The style with the most rounded patches
        FillingStyle_Curved,
    };

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
    TopoShape &makEBSplineFace(const std::vector<TopoShape> &input,
                               FillingStyle style = FillingStyle_Strech,
                               bool keepBezier = false,
                               const char *op=nullptr);
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
    TopoShape &makEBSplineFace(const TopoShape &input,
                               FillingStyle style = FillingStyle_Strech,
                               bool keepBezier = false,
                               const char *op=nullptr);
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
    TopoShape makEBSplineFace(FillingStyle style = FillingStyle_Strech,
                              bool keepBezier = false,
                              const char *op=nullptr)
    {
        return TopoShape(0,Hasher).makEBSplineFace(*this, style, keepBezier, op);
    }
    
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

    /** Make a non-planar filled face with boundary and/or constraint edge/wire 
     *
     * @param shapes: input shapes of any type. The function will automatically
     *                discover connected and closed edges to be used as the
     *                boundary of the the new face. Any other vertex, edge,
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
    TopoShape &makEFilledFace(const std::vector<TopoShape> &shapes,
                              const BRepFillingParams &params,
                              const char *op=nullptr);

    /** Make a solid using shells or CompSolid 
     *
     * @param shapes: input shapes of either shells or CompSolid.
     * @param op: optional string to be encoded into topo naming for indicating
     *            the operation
     *
     * @return The function produces a solid. The original content of this
     *         TopoShape is discarded and replaced with the new shape. The
     *         function returns the TopoShape itself as a self reference so
     *         that multiple operations can be carried out for the same shape
     *         in the same line of code.
     */
    TopoShape &makESolid(const std::vector<TopoShape> &shapes, const char *op=nullptr);
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
    TopoShape &makESolid(const TopoShape &shape, const char *op=nullptr);
    /** Make a solid using this shape 
     *
     * @param op: optional string to be encoded into topo naming for indicating
     *            the operation
     *
     * @return The function returns a new solid using the shell or CompSolid
     *         inside this shape. The shape itself is not modified.
     */
    TopoShape makESolid(const char *op=nullptr) const {
        return TopoShape(0,Hasher).makESolid(*this,op);
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
    TopoShape &makEShape(BRepBuilderAPI_MakeShape &mkShape, 
            const std::vector<TopoShape> &sources, const char *op=nullptr);
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
    TopoShape &makEShape(BRepBuilderAPI_MakeShape &mkShape, 
            const TopoShape &source, const char *op=nullptr);
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
    TopoShape makEShape(BRepBuilderAPI_MakeShape &mkShape, const char *op=nullptr) const {
        return TopoShape(0,Hasher).makEShape(mkShape,*this,op);
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
    TopoShape &makEShape(BRepBuilderAPI_Sewing &mkShape, 
            const std::vector<TopoShape> &sources, const char *op=nullptr);
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
    TopoShape &makEShape(BRepBuilderAPI_Sewing &mkShape, 
            const TopoShape &source, const char *op=nullptr);
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
    TopoShape makEShape(BRepBuilderAPI_Sewing &mkShape, const char *op=nullptr) const {
        return TopoShape(0,Hasher).makEShape(mkShape,*this,op);
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
    TopoShape &makEShape(BRepOffsetAPI_ThruSections &mkShape, 
            const std::vector<TopoShape> &sources, const char *op=nullptr);
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
    TopoShape &makEShape(BRepOffsetAPI_ThruSections &mkShape, 
            const TopoShape &source, const char *op=nullptr);
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
    TopoShape makEShape(BRepOffsetAPI_ThruSections &mkShape, const char *op=nullptr) const {
        return TopoShape(0,Hasher).makEShape(mkShape,*this,op);
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
    TopoShape &makEShape(BRepOffsetAPI_MakePipeShell &mkShape, 
            const std::vector<TopoShape> &sources, const char *op=nullptr);

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
    TopoShape &makEShape(BRepPrimAPI_MakeHalfSpace &mkShape, 
            const TopoShape &source, const char *op=nullptr);
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
    TopoShape makEShape(BRepPrimAPI_MakeHalfSpace  &mkShape, const char *op=nullptr) const {
        return TopoShape(0,Hasher).makEShape(mkShape,*this,op);
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
    TopoShape &makEShape(BRepFeat_MakePrism &mkShape,
            const std::vector<TopoShape> &sources, const TopoShape &uptoface, const char *op);

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
    TopoShape &makESHAPE(const TopoDS_Shape &shape, const Mapper &mapper, 
            const std::vector<TopoShape> &sources, const char *op=nullptr);

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
    TopoShape &makEBoolean(const char *maker, const std::vector<TopoShape> &sources,
            const char *op=nullptr, double tol = 0.0);
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
    TopoShape &makEBoolean(const char *maker, const TopoShape &source,
            const char *op=nullptr, double tol = 0.0);

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
    TopoShape makEBoolean(const char *maker, const char *op=nullptr, double tol = 0.0) const {
        return TopoShape(0,Hasher).makEBoolean(maker,*this,op,tol);
    }

    /** Make a new shape with transformation
     *
     * @param source: input shape
     * @param mat: transformation matrix
     * @param op: optional string to be encoded into topo naming for indicating
     *            the operation
     * @param checkScale: whether to check if the transformation matrix
     *                    contains scaling factor.
     * @param copy: whether to perform deep copy of the shape. If false, and
     *              checkScale is true, then the shape will be copied if there
     *              is scaling.
     *
     * @return Returns true if scaling is performed.
     *
     * The original content of this TopoShape is discarded and replaced with
     * the new transformed shape.
     */
    bool _makETransform(const TopoShape &source, const Base::Matrix4D &mat,
            const char *op=nullptr, bool checkScale=false, bool copy=false);

    /** Make a new shape with transformation
     *
     * @param source: input shape
     * @param mat: transformation matrix
     * @param op: optional string to be encoded into topo naming for indicating
     *            the operation
     * @param checkScale: whether to check if the transformation matrix
     *                    contains scaling factor.
     * @param copy: whether to perform deep copy of the shape. If false, and
     *              checkScale is true, then the shape will be copied if there
     *              is scaling.
     *
     * @return The original content of this TopoShape is discarded and replaced
     *         with the new transformed shape. The function returns the
     *         TopoShape itself as a self reference so that multiple operations
     *         can be carried out for the same shape in the same line of code.
     */
    TopoShape &makETransform(const TopoShape &source, const Base::Matrix4D &mat,
            const char *op=nullptr, bool checkScale=false, bool copy=false) {
        _makETransform(source,mat,op,checkScale,copy);
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
     * @param copy: whether to perform deep copy of the shape. If false, and
     *              checkScale is true, then the shape will be copied if there
     *              is scaling.
     *
     * @return Return a new shape with transformation. The shape itself is not
     *         modified
     */
    TopoShape makETransform(const Base::Matrix4D &mat, const char *op=nullptr, 
            bool checkScale=false, bool copy=false) const {
        return TopoShape(Tag,Hasher).makETransform(*this,mat,op,checkScale,copy);
    }

    /** Make a new shape with transformation
     *
     * @param source: input shape
     * @param trsf: OCCT transformation matrix
     * @param op: optional string to be encoded into topo naming for indicating
     *            the operation
     * @param checkScale: whether to check if the transformation matrix
     *                    contains scaling factor.
     * @param copy: whether to perform deep copy of the shape. If false, and
     *              checkScale is true, then the shape will be copied if there
     *              is scaling.
     *
     * @return The original content of this TopoShape is discarded and replaced
     *         with the new transformed shape. The function returns the
     *         TopoShape itself as a self reference so that multiple operations
     *         can be carried out for the same shape in the same line of code.
     */
    TopoShape &makETransform(const TopoShape &shape, const gp_Trsf &trsf, 
            const char *op=nullptr, bool copy=false);

    /** Make a new shape with transformation
     *
     * @param source: input shape
     * @param trsf: OCCT transformation matrix
     * @param op: optional string to be encoded into topo naming for indicating
     *            the operation
     * @param checkScale: whether to check if the transformation matrix
     *                    contains scaling factor.
     * @param copy: whether to perform deep copy of the shape. If false, and
     *              checkScale is true, then the shape will be copied if there
     *              is scaling.
     *
     * @return Return a new shape with transformation. The shape itself is not
     *         modified
     */
    TopoShape makETransform(const gp_Trsf &trsf, const char *op=nullptr, bool copy=false) const {
        return TopoShape(Tag,Hasher).makETransform(*this,trsf,op,copy);
    }

    /** @name Convenience function to transform the shape
     */
    //@{

    /** Move the shape to a new location
     *
     * @param loc: location
     *
     * The location is applied in addition to any current transformation of the shape
     */
    void move(const TopLoc_Location &loc) {
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
    TopoShape moved(const TopLoc_Location &loc) const {
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
    void move(const gp_Trsf &trsf) {
        move(_Shape._Shape, trsf);
    }
    /** Return a new transformed shape
     *
     * @param trsf: OCCT transformation (must not have scale)
     *
     * @return Return a shallow copy of the shape transformed to the new
     *         location that is applied in addition to any current
     *         transformation of the shape
     */
    TopoShape moved(const gp_Trsf &trsf) const {
        return moved(_Shape._Shape, trsf);
    }
    /** Set a new location for the shape
     *
     * @param loc: shape location
     *
     * Any previous location of the shape is discarded before applying the
     * input location
     */
    void locate(const TopLoc_Location &loc) {
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
    TopoShape located(const TopLoc_Location &loc) const {
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
    void locate(const gp_Trsf &trsf) {
        located(_Shape._Shape, trsf);
    }
    /** Set a new transformation for the shape
     *
     * @param trsf: OCCT transformation (must not have scale)
     *
     * Any previous transformation of the shape is discarded before applying
     * the input transformation.
     */
    TopoShape located(const gp_Trsf &trsf) const {
        return located(_Shape._Shape, trsf);
    }

    static TopoDS_Shape &move(TopoDS_Shape &s, const TopLoc_Location &);
    static TopoDS_Shape moved(const TopoDS_Shape &s, const TopLoc_Location &);
    static TopoDS_Shape &move(TopoDS_Shape &s, const gp_Trsf &);
    static TopoDS_Shape moved(const TopoDS_Shape &s, const gp_Trsf &);
    static TopoDS_Shape &locate(TopoDS_Shape &s, const TopLoc_Location &loc);
    static TopoDS_Shape located(const TopoDS_Shape &s, const TopLoc_Location &);
    static TopoDS_Shape &locate(TopoDS_Shape &s, const gp_Trsf &);
    static TopoDS_Shape located(const TopoDS_Shape &s, const gp_Trsf &);
    //@}

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
    TopoShape &makEGTransform(const TopoShape &source, const Base::Matrix4D &mat, 
            const char *op=nullptr, bool copy=false);

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
    TopoShape makEGTransform(const Base::Matrix4D &mat, const char *op=nullptr, bool copy=false) const {
        return TopoShape(Tag,Hasher).makEGTransform(*this,mat,op,copy);
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
    TopoShape &makECopy(const TopoShape &source, const char *op=nullptr, bool copyGeom=true, bool copyMesh=false);

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
    TopoShape makECopy(const char *op=nullptr, bool copyGeom=true, bool copyMesh=false) const {
        return TopoShape(Tag,Hasher).makECopy(*this,op,copyGeom,copyMesh);
    }

    /** Refine the input shape by merging faces/edges that share the same geometry
     *
     * @param source: input shape
     * @param op: optional string to be encoded into topo naming for indicating
     *            the operation
     * @param no_fail: if true, throw exception if failed to refine. Or else,
     *                 the shape remains untouched if failed.
     *
     * @return The original content of this TopoShape is discarded and replaced
     *         with the refined shape. The function returns the TopoShape
     *         itself as a self reference so that multiple operations can be
     *         carried out for the same shape in the same line of code.
     */
    TopoShape &makERefine(const TopoShape &source, const char *op=nullptr, bool no_fail=true);

    /** Refine the input shape by merging faces/edges that share the same geometry
     *
     * @param source: input shape
     * @param op: optional string to be encoded into topo naming for indicating
     *            the operation
     * @param no_fail: if true, throw exception if failed to refine. Or else,
     *                 the shape remains untouched if failed.
     *
     * @return Return a refined shape. The shape itself is not modified
     */
    TopoShape makERefine(const char *op=nullptr, bool no_fail=true) const {
        return TopoShape(Tag,Hasher).makERefine(*this,op,no_fail);
    }

    /// Defines how to fill the holes that may appear after offset two adjacent faces
    enum class JoinType
    {
        Arc,
        Tangent,
        Intersection,
    };

    /** Make a hollowed solid by removing some faces from a given solid
     *
     * @param source: input shape
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
    TopoShape &makEThickSolid(const TopoShape &source, const std::vector<TopoShape> &faces, 
            double offset, double tol, bool intersection = false, bool selfInter = false,
            short offsetMode = 0, JoinType join = JoinType::Arc, const char *op=nullptr);

    /** Make a hollowed solid by removing some faces from a given solid
     *
     * @param source: input shape
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
    TopoShape makEThickSolid(const std::vector<TopoShape> &faces, 
            double offset, double tol, bool intersection = false, bool selfInter = false,
            short offsetMode = 0, JoinType join = JoinType::Arc, const char *op=nullptr) const {
        return TopoShape(0,Hasher).makEThickSolid(*this,faces,offset,tol,intersection,selfInter,
                offsetMode,join,op);
    }

    /** Make reolved shell around a basis shape
     *
     * @param base: the base shape
     * @param axis: the reolving axis
     * @param d: rotation angle in degree
     * @param face_maker: optional type name of the the maker used to make a
     *                    face from basis shape
     * @param op: optional string to be encoded into topo naming for indicating
     *            the operation
     *
     * @return The original content of this TopoShape is discarded and replaced
     *         with the new shape. The function returns the TopoShape itself as
     *         a self reference so that multiple operations can be carried out
     *         for the same shape in the same line of code.
     */
    TopoShape &makERevolve(const TopoShape &base, const gp_Ax1& axis, double d, 
            const char *face_maker=0, const char *op=nullptr);

    /** Make reolved shell around a basis shape
     *
     * @param base: the basis shape
     * @param axis: the reolving axis
     * @param d: rotation angle in degree
     * @param face_maker: optional type name of the the maker used to make a
     *                    face from basis shape
     * @param op: optional string to be encoded into topo naming for indicating
     *            the operation
     *
     * @return Return the generated new shape. The TopoShape itself is not modified.
     */
    TopoShape makERevolve(const gp_Ax1& axis, double d, 
            const char *face_maker=nullptr, const char *op=nullptr) const {
        return TopoShape(0,Hasher).makERevolve(*this,axis,d,face_maker,op);
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
    TopoShape &makEPrism(const TopoShape &base, const gp_Vec& vec, const char *op=nullptr);

    /** Make a prism that is a linear sweep of this shape
     *
     * @param vec: vector defines the sweep direction
     * @param op: optional string to be encoded into topo naming for indicating
     *            the operation
     *
     * @return Return the generated new shape. The TopoShape itself is not modified.
     */
    TopoShape makEPrism(const gp_Vec& vec, const char *op=nullptr) const {
        return TopoShape(0,Hasher).makEPrism(*this,vec,op);
    }

    /// Operation mode for makEPrismUntil()
    enum PrismMode {
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
    TopoShape &makEPrismUntil(const TopoShape &base, 
                              const TopoShape& profile,
                              const TopoShape& supportFace,
                              const TopoShape& upToFace,
                              const gp_Dir& direction,
                              PrismMode mode,
                              Standard_Boolean checkLimits = Standard_True,
                              const char *op=nullptr);

    /** Make a prism based on this shape that is either depression or protrusion of a profile shape up to a given face
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
    TopoShape makEPrismUntil(const TopoShape& profile,
                             const TopoShape& supportFace,
                             const TopoShape& upToFace,
                             const gp_Dir& direction,
                             PrismMode mode,
                             Standard_Boolean checkLimits = Standard_True,
                             const char *op=nullptr) const
    {
        return TopoShape(0,Hasher).makEPrismUntil(*this,
                                                  profile,
                                                  supportFace,
                                                  upToFace,
                                                  direction,
                                                  mode,
                                                  checkLimits,
                                                  op);
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
    TopoShape &makEOffset(const TopoShape &source, double offset, double tol,
            bool intersection = false, bool selfInter = false, short offsetMode = 0, 
            JoinType join = JoinType::Arc, bool fill = false, const char *op=nullptr);

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
    TopoShape makEOffset(double offset, double tol, bool intersection = false, bool selfInter = false, 
            short offsetMode=0, JoinType join=JoinType::Arc, bool fill=false, const char *op=nullptr) const {
        return TopoShape(0,Hasher).makEOffset(*this,offset,tol,intersection,selfInter,
                offsetMode,join,fill,op);
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
    TopoShape &makEOffset2D(const TopoShape &source, double offset, JoinType join=JoinType::Arc, bool fill=false, 
            bool allowOpenResult=false, bool intersection=false, const char *op=nullptr);
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
    TopoShape makEOffset2D(double offset, JoinType join=JoinType::Arc, bool fill=false, bool allowOpenResult=false, 
            bool intersection=false, const char *op=nullptr) const {
        return TopoShape(0,Hasher).makEOffset2D(*this,offset,join,fill,allowOpenResult,intersection,op);
    }

    /** Make a 2D offset of face with separate control for outer and inner (hole) wires
     *
     * @param source: source shape of any type, but only faces inside will be used
     * @param offset: distance to offset for outer wires of the faces
     * @param innerOffset: distance to offset for inner wires of the faces
     * @param join: join type of outer wire. Only support JoinType::Arc and JoinType::Intersection.
     * @param innerJoin: join type of inner wire. Only support JoinType::Arc and JoinType::Intersection.
     * @param op: optional string to be encoded into topo naming for indicating
     *            the operation
     *
     * @return The original content of this TopoShape is discarded and replaced
     *         with the new shape. The function returns the TopoShape itself as
     *         a self reference so that multiple operations can be carried out
     *         for the same shape in the same line of code.
     */
    TopoShape &makEOffsetFace(const TopoShape &source,
                              double offset,
                              double innerOffset,
                              JoinType join = JoinType::Arc, 
                              JoinType innerJoin = JoinType::Arc, 
                              const char *op = nullptr);

    /** Make a 2D offset of face with separate control for outer and inner (hole) wires
     *
     * @param source: source shape of any type, but only faces inside will be used
     * @param offset: distance to offset for outer wires of the faces
     * @param innerOffset: distance to offset for inner wires of the faces
     * @param join: join type of outer wire. Only support JoinType::Arc and JoinType::Intersection.
     * @param innerJoin: join type of inner wire. Only support JoinType::Arc and JoinType::Intersection.
     * @param op: optional string to be encoded into topo naming for indicating
     *            the operation
     *
     * @return Return the new shape. The TopoShape itself is not modified.
     */
    TopoShape makEOffsetFace(double offset,
                             double innerOffset,
                             JoinType join = JoinType::Arc,
                             JoinType innerJoin = JoinType::Arc,
                             const char *op = nullptr) const
    {
        return TopoShape(0,Hasher).makEOffsetFace(*this,offset,innerOffset,join,innerJoin,op);
    }

    /// Option to manage discontinuity in pipe sweeping
    enum class TransitionMode {
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
    TopoShape &makEPipeShell(const std::vector<TopoShape> &sources, const Standard_Boolean makeSolid,
                             const Standard_Boolean isFrenet, TransitionMode transition=TransitionMode::Transformed,
                             const char *op=nullptr, double tol3d=0.0, double tolBound=0.0, double tolAngluar=0.0);

    /** Make an evolved shape
     *
     * An evolved shape is built from a planar spine (face or wire) and a
     * profile (wire). The evolved shape is the unlooped sweep (pipe) of the
     * profile along the spine. Self-intersections are removed.
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
    TopoShape &makEEvolve(const TopoShape &spine, const TopoShape &profile, JoinType join=JoinType::Arc,
            bool axeProf=true, bool solid=false, bool profOnSpine=false, double tol=0.0, const char *op=nullptr);

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
    TopoShape makEEvolve(const TopoShape &profile, JoinType join=JoinType::Arc,
            bool axeProf=true, bool solid=false, bool profOnSpine=false, double tol=0.0, const char *op=nullptr)
    {
        return TopoShape(0,Hasher).makEEvolve(*this, profile, join, axeProf, solid, profOnSpine, tol, op);
    }

    /** Make an loft that is a shell or solid passing through a set of sections in a given sequence
     *
     * @param sources: the source shapes. The first shape is used as the spine,
     *                 and the rest as sections. The sections can be of any
     *                 type, but only wires are used. The first and last
     *                 section may be vertex.
     * @param isSolid: whether to make a solid
     * @param isRuled: If true, then the faces generated between the edges of
     *                 two consecutive section wires are ruled surfaces. If
     *                 false, then they are smoothed out by approximation
     * @param isClosed: If true, then the first section is duplicated to close
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
    TopoShape &makELoft(const std::vector<TopoShape> &sources,
            Standard_Boolean isSolid, Standard_Boolean isRuled, Standard_Boolean isClosed=Standard_False,
            Standard_Integer maxDegree=5, const char *op=nullptr);

    /** Make a ruled surface
     *
     * @param sources: the source shapes, each of which must contain either a
     *                 single edge or a single wire.
     * @param orientation: 
     * @param isSolid: whether to make a solid
     * @param isRuled: If true, then the faces generated between the edges of
     *                 two consecutive section wires are ruled surfaces. If
     *                 false, then they are smoothed out by approximation
     * @param isClosed: If true, then the first section is duplicated to close
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
    TopoShape &makERuledSurface(const std::vector<TopoShape> &source, int orientation=0, const char *op=nullptr);

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
    TopoShape &makEMirror(const TopoShape &source, const gp_Ax2& axis, const char *op=nullptr);
    /** Make a mirrored shape
     *
     * @param source: the source shape
     * @param axis: the axis for mirroring
     * @param op: optional string to be encoded into topo naming for indicating
     *            the operation
     *
     * @return Return the new shape. The TopoShape itself is not modified.
     */
    TopoShape makEMirror(const gp_Ax2& ax, const char *op=nullptr) const {
        return TopoShape(0,Hasher).makEMirror(*this,ax,op);
    }

    /** Make a cross section slice
     *
     * @param source: the source shape
     * @param dir: direction of the normal of the section plane
     * @param d: distance to move the section plane
     * @param op: optional string to be encoded into topo naming for indicating
     *            the operation
     *
     * @return The original content of this TopoShape is discarded and replaced
     *         with the new shape. The function returns the TopoShape itself as
     *         a self reference so that multiple operations can be carried out
     *         for the same shape in the same line of code.
     */
    TopoShape &makESlice(const TopoShape &source, const Base::Vector3d& dir, double d, const char *op=nullptr);
    /** Make a cross section slice
     *
     * @param source: the source shape
     * @param dir: direction of the normal of the section plane
     * @param d: distance to move the section plane
     * @param op: optional string to be encoded into topo naming for indicating
     *            the operation
     *
     * @return Return the new shape. The TopoShape itself is not modified.
     */
    TopoShape makESlice(const Base::Vector3d& dir, double d, const char *op=nullptr) const {
        return TopoShape(0,Hasher).makESlice(*this,dir,d,op);
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
    TopoShape &makESlices(const TopoShape &source, const Base::Vector3d& dir,
            const std::vector<double> &distances, const char *op=nullptr);
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
    TopoShape makESlices(const Base::Vector3d &dir, const std::vector<double> &distances, const char *op=nullptr) const {
        return TopoShape(0,Hasher).makESlices(*this,dir,distances,op);
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
    TopoShape &makEFillet(const TopoShape &source, const std::vector<TopoShape> &edges, 
            double radius1, double radius2, const char *op=nullptr);
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
    TopoShape makEFillet(const std::vector<TopoShape> &edges, 
            double radius1, double radius2, const char *op=nullptr) const {
        return TopoShape(0,Hasher).makEFillet(*this,edges,radius1,radius2,op);
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
    TopoShape &makEChamfer(const TopoShape &source, const std::vector<TopoShape> &edges, 
            double radius1, double radius2, const char *op=nullptr, bool flipDirection=false, bool asAngle=false);
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
    TopoShape makEChamfer(const std::vector<TopoShape> &edges, 
            double radius1, double radius2, const char *op=nullptr, bool flipDirection=false, bool asAngle=false) const {
        return TopoShape(0,Hasher).makEChamfer(*this,edges,radius1,radius2,op,flipDirection,asAngle);
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
    TopoShape &makEDraft(const TopoShape &source, const std::vector<TopoShape> &faces, 
           const gp_Dir &pullDirection, double angle, const gp_Pln &neutralPlane,
           bool retry=true, const char *op=nullptr);
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
    TopoShape makEDraft(const std::vector<TopoShape> &faces, 
           const gp_Dir &pullDirection, double angle, const gp_Pln &neutralPlane,
           bool retry=true, const char *op=nullptr) const {
        return TopoShape(0,Hasher).makEDraft(*this,faces,pullDirection,angle,neutralPlane,retry,op);
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
    TopoShape &makEShell(bool silent=true, const char *op=nullptr);

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
    TopoShape &makEShellFromWires(const std::vector<TopoShape> &wires, bool silent=true, const char *op=nullptr);
    /* Make a shell with input wires
     *
     * @param wires: input wires
     * @param silent: whether to throw exception on failure
     * @param op: optional string to be encoded into topo naming for indicating
     *            the operation
     *
     * @return Return the new shape. The TopoShape itself is not modified.
     */
    TopoShape &makEShellFromWires(bool silent=true, const char *op=nullptr) {
        return makEShellFromWires(getSubTopoShapes(TopAbs_WIRE), silent, op);
    }

    /* Make a shape with some subshapes replaced
     *
     * @param source: the source shape
     * @param s: replacement mapping the existing sub shape of source to new shapes
     *
     * @return The original content of this TopoShape is discarded and replaced
     *         with the new shape. The function returns the TopoShape itself as
     *         a self reference so that multiple operations can be carried out
     *         for the same shape in the same line of code.
     */
    TopoShape &replacEShape(const TopoShape &source, const std::vector<std::pair<TopoShape,TopoShape> > &s);
    /* Make a new shape using this shape with some subshapes replaced by others
     *
     * @param s: replacement mapping the existing sub shape of source to new shapes
     *
     * @return Return the new shape. The TopoShape itself is not modified.
     */
    TopoShape replacEShape(const std::vector<std::pair<TopoShape,TopoShape> > &s) const {
        return TopoShape(0,Hasher).replacEShape(*this,s);
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
    TopoShape &removEShape(const TopoShape &source, const std::vector<TopoShape>& s);
    /* Make a new shape using this shape with some subshapes removed
     *
     * @param s: the subshapes to be removed
     *
     * @return Return the new shape. The TopoShape itself is not modified.
     */
    TopoShape removEShape(const std::vector<TopoShape>& s) const {
        return TopoShape(0,Hasher).removEShape(*this,s);
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
    TopoShape &makEGeneralFuse(const std::vector<TopoShape> &sources, 
            std::vector<std::vector<TopoShape> > &modified, double tol=0, const char *op=nullptr);

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
    TopoShape &makEFuse(const std::vector<TopoShape> &sources, const char *op=nullptr, double tol=0);
    /** Make a fusion of this shape and an input shape
     *
     * @param source: the source shape
     * @param op: optional string to be encoded into topo naming for indicating
     *            the operation
     * @param tol: tolerance for the fusion
     *
     * @return Return the new shape. The TopoShape itself is not modified.
     */
    TopoShape makEFuse(const TopoShape &source, const char *op=nullptr, double tol=0) const {
        return TopoShape(0,Hasher).makEFuse({*this,source},op,tol);
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
    TopoShape &makECut(const std::vector<TopoShape> &sources, const char *op=nullptr, double tol=0);
    /** Make a boolean cut of this shape with an input shape
     *
     * @param source: the source shape
     * @param op: optional string to be encoded into topo naming for indicating
     *            the operation
     * @param tol: tolerance for the fusion
     *
     * @return Return the new shape. The TopoShape itself is not modified.
     */
    TopoShape makECut(const TopoShape &source, const char *op=nullptr, double tol=0) const {
        return TopoShape(0,Hasher).makECut({*this,source},op,tol);
    }

    /** Try to simplify geometry of any linear/planar subshape to line/plane
     *
     * @return Return true if the shape is modified
     */
    bool linearize(bool face, bool edge);

    //@}

    /** @name Element name mapping helper functions
     *
     * These functions are implemented in TopoShapeEx.cpp
     */
    //@{

    static const std::string &modPostfix();
    static const std::string &genPostfix();
    static const std::string &modgenPostfix();
    static const std::string &upperPostfix();
    static const std::string &lowerPostfix();

    void mapSubElement(const TopoShape &other,const char *op=nullptr, bool forceHasher=false);
    void mapSubElement(const std::vector<TopoShape> &shapes, const char *op=nullptr);
    void mapSubElementsTo(std::vector<TopoShape> &shapes, const char *op=nullptr) const;
    void copyElementMap(const TopoShape &other, const char *op=nullptr);

    bool canMapElement(const TopoShape &other) const;

    void cacheRelatedElements(const Data::MappedName & name,
                              bool sameType,
                              const QVector<Data::MappedElement> & names) const;

    bool getRelatedElementsCached(const Data::MappedName & name,
                                  bool sameType,
                                  QVector<Data::MappedElement> &names) const;

    virtual std::string getElementMapVersion() const;
    virtual bool checkElementMapVersion(const char * ver) const;

    virtual void flushElementMap() const;

    virtual Data::ElementMapPtr resetElementMap(
            Data::ElementMapPtr elementMap=Data::ElementMapPtr());

    virtual unsigned long getElementMapReserve() const;
    bool hasPendingElementMap() const;

    virtual std::vector<Data::IndexedName> getHigherElements(const char *element,
                                                             bool silent=false) const;

    Data::MappedName setElementComboName(const Data::IndexedName & element, 
                                         const std::vector<Data::MappedName> &names,
                                         const char *marker=nullptr,
                                         const char *op=nullptr,
                                         const Data::ElementIDRefs *sids=nullptr);

    std::vector<Data::MappedName> decodeElementComboName(const Data::IndexedName &element,
                                                         const Data::MappedName &name,
                                                         const char *marker=nullptr,
                                                         std::string *postfix = nullptr) const;

    virtual void reTagElementMap(long tag, App::StringHasherRef hasher, const char *postfix=nullptr);

    long isElementGenerated(const Data::MappedName &name, int depth=1) const;
    //@}


    /** @name sub shape cached functions
     *
     * Mapped element names introduces some overhead when getting sub shapes
     * from a shape These functions use internal caches for sub-shape maps to
     * improve performance
     */
    //@{
    void initCache(int reset=0, const char *file=nullptr, int line=0) const;
    int findShape(const TopoDS_Shape &subshape) const;
    TopoDS_Shape findShape(const char *name) const;
    TopoDS_Shape findShape(TopAbs_ShapeEnum type, int idx) const;
    int findAncestor(const TopoDS_Shape &subshape, TopAbs_ShapeEnum type) const;
    TopoDS_Shape findAncestorShape(const TopoDS_Shape &subshape, TopAbs_ShapeEnum type) const;
    std::vector<int> findAncestors(const TopoDS_Shape &subshape, TopAbs_ShapeEnum type) const;
    std::vector<TopoDS_Shape> findAncestorsShapes(const TopoDS_Shape &subshape, TopAbs_ShapeEnum type) const;
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
    std::vector<TopoShape> searchSubShape(const TopoShape &subshape,
                                          std::vector<std::string> *names=nullptr,
                                          bool checkGeometry=true,
                                          double tol=1e-7, double atol=1e-12) const;
    //@}

    /** @name Convenience function to obtain the shape type and the corresponding string name
     */
    //@{
    static TopAbs_ShapeEnum shapeType(const char *type,bool silent=false);
    static TopAbs_ShapeEnum shapeType(char type,bool silent=false);
    TopAbs_ShapeEnum shapeType(bool silent=false) const;
    static const std::string &shapeName(TopAbs_ShapeEnum type,bool silent=false);
    const std::string &shapeName(bool silent=false) const;
    static std::pair<TopAbs_ShapeEnum,int> shapeTypeAndIndex(const char *name);
    static std::pair<TopAbs_ShapeEnum,int> shapeTypeAndIndex(const Data::IndexedName &name);
    //@}

    class Cache;
    friend class Cache;

protected:
    virtual Data::MappedName renameDuplicateElement(int index,
                                                    const Data::IndexedName & element, 
                                                    const Data::IndexedName & element2,
                                                    const Data::MappedName & name,
                                                    Data::ElementIDRefs &sids);

private:

    /** Helper class to ensure synchronization of element map and cache
     *
     * It exposes constant method of OCCT TopoDS_Shape unchanged, and wrap all
     * non-constant method to auto clear the element names in the owner TopoShape
     */
    class ShapeProtector
    {
    public:
        ShapeProtector(TopoShape & master)
            : master(master)
        {}

        ShapeProtector(TopoShape & master, const TopoDS_Shape & shape)
            : master(master), _Shape(shape)
        {}

        const TopoDS_Shape & getShape() const {
            return _Shape;
        }
        operator const TopoDS_Shape & () const {
            return _Shape;
        }

        // Pass through all const interface of TopoDS_Shape

        Standard_Boolean IsNull() const { return _Shape.IsNull(); }
        const TopLoc_Location& Location() const { return _Shape.Location(); }
        TopoDS_Shape Located (const TopLoc_Location& Loc) const {
            return TopoShape::located(_Shape, Loc);
        }
        TopAbs_Orientation Orientation() const { return _Shape.Orientation(); }
        TopoDS_Shape Oriented (const TopAbs_Orientation Or) const { return _Shape.Oriented(Or); }
        const Handle(TopoDS_TShape)& TShape() const { return _Shape.TShape(); }
        TopAbs_ShapeEnum ShapeType() const { return _Shape.ShapeType(); }
        Standard_Boolean Free() const { return _Shape.Free(); }
        Standard_Boolean Locked() const { return _Shape.Locked(); }
        Standard_Boolean Modified() const { return _Shape.Modified(); }
        Standard_Boolean Checked() const { return _Shape.Checked(); }
        Standard_Boolean Orientable() const { return _Shape.Orientable(); }
        Standard_Boolean Closed() const { return _Shape.Closed(); }
        Standard_Boolean Infinite() const { return _Shape.Infinite(); }
        Standard_Boolean Convex() const { return _Shape.Convex(); }
        TopoDS_Shape Moved (const TopLoc_Location& position) const {
            return TopoShape::moved(_Shape, position);
        }
        TopoDS_Shape Reversed() const { return _Shape.Reversed(); }
        TopoDS_Shape Complemented() const { return _Shape.Complemented(); }
        TopoDS_Shape Composed (const TopAbs_Orientation Orient) const { return _Shape.Composed(Orient); }
        Standard_Boolean IsPartner (const TopoDS_Shape& other) const { return _Shape.IsPartner(other); }
        Standard_Boolean IsSame (const TopoDS_Shape& other) const { return _Shape.IsSame(other); }
        Standard_Boolean IsEqual (const TopoDS_Shape& other) const { return _Shape.IsEqual(other); }
        Standard_Boolean operator == (const TopoDS_Shape& other) const { return _Shape == other; }
        Standard_Boolean IsNotEqual (const TopoDS_Shape& other) const { return _Shape.IsNotEqual(other); }
        Standard_Boolean operator != (const TopoDS_Shape& other) const { return _Shape != other; }
        Standard_Integer HashCode (const Standard_Integer Upper) const { return _Shape.HashCode(Upper); }
        TopoDS_Shape EmptyCopied() const { return _Shape.EmptyCopied(); }

        // Flag setters, probably not going to affect element map or cache. Pass
        // through for now.

        void Free (const Standard_Boolean F) { _Shape.Free(F); }
        void Locked (const Standard_Boolean F) { _Shape.Locked(F); }
        void Modified (const Standard_Boolean M) { _Shape.Modified(M); }
        void Checked (const Standard_Boolean C) { _Shape.Checked(C); }
        void Orientable (const Standard_Boolean C) { _Shape.Orientable(C); }
        void Closed (const Standard_Boolean C) { _Shape.Closed(C); }
        void Infinite (const Standard_Boolean C) { _Shape.Infinite(C); }
        void Convex (const Standard_Boolean C) { _Shape.Convex(C); }
    
        // Sync master TopoShape element map and cache on all non-const interface

        void Nullify() {
            master.resetElementMap();
            master._Cache.reset();
            master._ParentCache.reset();
        }
    
        void Location (const TopLoc_Location& Loc) {
            // Location does not affect element map or cache
            TopoShape::locate(_Shape, Loc);
        }
    
        void Orientation (const TopAbs_Orientation Orient) {
            master.flushElementMap();
            _Shape.Orientation(Orient);
            if (master._Cache) master.initCache();
        }
    
        void Move (const TopLoc_Location& position) {
            // Move does not affect element map or cache
            TopoShape::move(_Shape, position);
        }
    
        void Reverse() {
            master.flushElementMap();
            _Shape.Reverse();
            if (master._Cache) master.initCache();
        }
    
        void Complement() {
            master.flushElementMap();
            _Shape.Complement();
            if (master._Cache) master.initCache();
        }
    
        void Compose (const TopAbs_Orientation Orient) {
            master.flushElementMap();
            _Shape.Compose(Orient);
            if (master._Cache) master.initCache();
        }
    
        void EmptyCopy() {
            master.flushElementMap();
            _Shape.EmptyCopy();
            if (master._Cache) master.initCache();
        }
    
        void TShape (const Handle(TopoDS_TShape)& T) {
            master.flushElementMap();
            _Shape.TShape(T);
            if (master._Cache) master.initCache();
        }
    private:
        TopoShape & master;
        TopoDS_Shape _Shape;
        friend class TopoShape;
    };
    friend class ShapeProtector;

    ShapeProtector _Shape;
    mutable std::shared_ptr<Cache> _Cache;
    mutable std::shared_ptr<Cache> _ParentCache;
    mutable TopLoc_Location _SubLocation;
};

} //namespace Part


namespace std {

template<> 
struct hash<Part::TopoShape> {
    typedef Part::TopoShape argument_type;
    typedef std::size_t result_type;
    inline result_type operator()(argument_type const& s) const {
        return s.getShape().HashCode(INT_MAX);
    }
};

template<> 
struct hash<TopoDS_Shape> {
    typedef TopoDS_Shape argument_type;
    typedef std::size_t result_type;
    inline result_type operator()(argument_type const& s) const {
        return s.HashCode(INT_MAX);
    }
};

} //namespace std


namespace Part {

/// Shape hasher that ignore orientation
struct ShapeHasher {
    inline size_t operator()(const TopoShape &s) const {
        return s.getShape().HashCode(INT_MAX);
    }
    inline size_t operator()(const TopoDS_Shape &s) const {
        return s.HashCode(INT_MAX);
    }
    inline bool operator()(const TopoShape &a, const TopoShape &b) const {
        return a.getShape().IsSame(b.getShape());
    }
    inline bool operator()(const TopoDS_Shape &a, const TopoDS_Shape &b) const {
        return a.IsSame(b);
    }
    template <class T>
    static inline void hash_combine(std::size_t& seed, const T& v)
    {
        // copied from boost::hash_combine
        std::hash<T> hasher;
        seed ^= hasher(v) + 0x9e3779b9 + (seed<<6) + (seed>>2);
    }
    inline size_t operator()(const std::pair<TopoShape, TopoShape> &s) const {
        size_t res = s.first.getShape().HashCode(INT_MAX);
        hash_combine(res, s.second.getShape().HashCode(INT_MAX));
        return res;
    }
    inline size_t operator()(const std::pair<TopoDS_Shape, TopoDS_Shape> &s) const {
        size_t res = s.first.HashCode(INT_MAX);
        hash_combine(res, s.second.HashCode(INT_MAX));
        return res;
    }
    inline bool operator()(const std::pair<TopoShape, TopoShape> &a,
                           const std::pair<TopoShape, TopoShape> &b) const {
        return a.first.getShape().IsSame(b.first.getShape())
            && a.second.getShape().IsSame(b.second.getShape());
    }
    inline bool operator()(const std::pair<TopoDS_Shape, TopoDS_Shape> &a,
                           const std::pair<TopoDS_Shape, TopoDS_Shape> &b) const {
        return a.first.IsSame(b.first)
            && a.second.IsSame(b.second);
    }
};

/** Shape mapper for generic BRepBuilderAPI_MakeShape derived class
 *
 * Uses BRepBuilderAPI_MakeShape::Modified/Generated() function to extract
 * shape history for generating mapped element names
 */
struct PartExport MapperMaker: TopoShape::Mapper {
    BRepBuilderAPI_MakeShape &maker;
    MapperMaker(BRepBuilderAPI_MakeShape &maker)
        :maker(maker)
    {}
    virtual const std::vector<TopoDS_Shape> &modified(const TopoDS_Shape &s) const override;
    virtual const std::vector<TopoDS_Shape> &generated(const TopoDS_Shape &s) const override;
};

/** Shape mapper for BRepTools_History
 *
 * Uses BRepTools_History::Modified/Generated() function to extract
 * shape history for generating mapped element names
 */
struct PartExport MapperHistory: TopoShape::Mapper {
    Handle(BRepTools_History) history;
    MapperHistory(const Handle(BRepTools_History) &history);
    MapperHistory(const Handle(BRepTools_ReShape) &reshape);
    MapperHistory(ShapeFix_Root &fix);
    virtual const std::vector<TopoDS_Shape> &modified(const TopoDS_Shape &s) const override;
    virtual const std::vector<TopoDS_Shape> &generated(const TopoDS_Shape &s) const override;
};

/** Shape mapper for user defined shape mapping
 */
struct PartExport ShapeMapper: TopoShape::Mapper {

    /** Populate mapping from a source shape to a list of shape
     *
     * @param generated: whether the shape is generated
     * @param src: source shape
     * @param dst: a list of sub shapes in the new shape
     *
     * The source will be expanded into sub shapes of faces, edges and vertices
     * before being inserted into the map.
     */
    void populate(bool generated, const TopoShape &src, const TopTools_ListOfShape &dst);
    /** Populate mapping from a source sub shape to a list of shape
     *
     * @param generated: whether the shape is generated
     * @param src: a list of sub shapes in the source shape
     * @param dst: a list of sub shapes in the new shape
     *
     * The source will be expanded into sub shapes of faces, edges and vertices
     * before being inserted into the map.
     */
    void populate(bool generated, const TopTools_ListOfShape &src, const TopTools_ListOfShape &dst);

    /** Populate mapping from a source sub shape to a list of shape
     *
     * @param generated: whether the shape is generated
     * @param src: a list of sub shapes in the source shape
     * @param dst: a list of sub shapes in the new shape
     *
     * The source will be expanded into sub shapes of faces, edges and vertices
     * before being inserted into the map.
     */
    void populate(bool generated, const std::vector<TopoShape> &src, const std::vector<TopoShape> &dst)
    {
        for(auto &s : src)
            populate(generated,s,dst);
    }

    /** Populate mapping from a source sub shape to a list of shape
     *
     * @param generated: whether the shape is generated
     * @param src: a sub shape of the source shape
     * @param dst: a list of sub shapes in the new shape
     *
     * The source will be expanded into sub shapes of faces, edges and vertices
     * before being inserted into the map.
     */
    void populate(bool generated, const TopoShape &src, const std::vector<TopoShape> &dst)
    {
        if(src.isNull())
            return;
        std::vector<TopoDS_Shape> dstShapes;
        for(auto &d : dst)
            expand(d.getShape(), dstShapes);
        insert(generated, src.getShape(), dstShapes);
    }

    /** Expand a shape into faces, edges and vertices
     * @params d: shape to expand
     * @param shapes: output sub shapes of faces, edges and vertices
     */
    void expand(const TopoDS_Shape &d, std::vector<TopoDS_Shape> &shapes);

    /** Insert a map entry from a sub shape in the source to a list of sub shapes in the new shape
     *
     * @params generated: whether the sub shapes are generated or modified
     * @param s: a sub shape in the source
     * @param d: a list of sub shapes in the new shape
     */
    void insert(bool generated, const TopoDS_Shape &s, const std::vector<TopoDS_Shape> &d);

    /** Insert a map entry from a sub shape in the source to a sub shape in the new shape
     *
     * @params generated: whether the sub shapes are generated or modified
     * @param s: a sub shape in the source
     * @param d: a list of sub shapes in the new shape
     */
    void insert(bool generated, const TopoDS_Shape &s, const TopoDS_Shape &d);

    virtual const std::vector<TopoDS_Shape> &generated(const TopoDS_Shape &s) const override {
        auto iter = _generated.find(s);
        if(iter != _generated.end())
            return iter->second.shapes;
        return _res;
    }

    virtual const std::vector<TopoDS_Shape> &modified(const TopoDS_Shape &s) const override {
        auto iter = _modified.find(s);
        if(iter != _modified.end())
            return iter->second.shapes;
        return _res;
    }

    std::vector<TopoShape> shapes;
    std::unordered_set<TopoDS_Shape,ShapeHasher,ShapeHasher> shapeSet;

    struct ShapeValue {
        std::vector<TopoDS_Shape> shapes;
        std::unordered_set<TopoDS_Shape,ShapeHasher,ShapeHasher> shapeSet;
    };
    typedef std::unordered_map<TopoDS_Shape, ShapeValue,ShapeHasher,ShapeHasher> ShapeMap;
    ShapeMap _generated;
    std::unordered_set<TopoDS_Shape,ShapeHasher,ShapeHasher> _generatedShapes;
    ShapeMap _modified;
    std::unordered_set<TopoDS_Shape,ShapeHasher,ShapeHasher> _modifiedShapes;
};

/** Generic shape mapper from a given source to an output shape
 */
struct PartExport GenericShapeMapper: ShapeMapper {
    /// Populate the map with a given source shape to an output shape
    void init(const TopoShape &src, const TopoDS_Shape &dst);
};

/// Parameters for TopoShape::makEFilledFace()
struct PartExport TopoShape::BRepFillingParams {
    /** Optional initial surface to begin the construction of the surface for the filled face.
     *
     *  It is useful if the surface resulting from construction for the
     *  algorithm is likely to be complex. The support surface of the face
     *  under construction is computed by a deformation of Surf which satisfies
     *  the given constraints. The set of bounding edges defines the wire of
     *  the face. If no initial surface is given, the algorithm computes it
     *  automatically. If the set of edges is not connected (Free constraint),
     *  missing edges are automatically computed. Important: the initial
     *  surface must have orthogonal local coordinates, i.e. partial
     *  derivatives dS/du and dS/dv must be orthogonal at each point of
     *  surface. If this condition breaks, distortions of resulting surface are
     *  possible
     */
    TopoShape surface;
    /** Optional map from input edge to continutity order. The default
     *  continuity order is TopoShape::Continuity::C0.
     */
    std::unordered_map<TopoDS_Shape, TopoShape::Continuity, ShapeHasher, ShapeHasher> orders;
    /// Optional map from input shape to face used as support
    std::unordered_map<TopoDS_Shape, TopoDS_Shape, ShapeHasher, ShapeHasher> supports;
    /// Optional begin index to the input shapes to be used as the boundary of the filled face.
    int boundary_begin = -1;
    /// Optional end index (last index + 1) to the input shapes to be used as the boundary of the filled face.
    int boundary_end = -1;
    /// The energe minimizing criterion degree;
    unsigned int degree = 3;
    /// The number of points on the curve NbPntsOnCur
    unsigned int ptsoncurve = 15;
    /// The number of iterations NbIter
    unsigned int numiter = 2;
    /// The Boolean Anisotropie
    bool anisotropy = false;
    /// The 2D tolerance Tol2d
    double tol2d = 1e-5;
    /// The 3D tolerance Tol3d
    double tol3d = 1e-4;
    /// The angular tolerance TolAng
    double tolG1 = 0.01;
    /// The tolerance for curvature TolCur
    double tolG2 = 0.1;
    /// The highest polynomial degree MaxDeg
    unsigned int maxdeg = 8;
    /** The greatest number of segments MaxSeg.
     *
     * If the Boolean Anistropie is true, the algorithm's performance is better
     * in cases where the ratio of the length U and the length V indicate a
     * great difference between the two. In other words, when the surface is,
     * for example, extremely long.
     */
    unsigned int maxseg = 9;
};

class PartExport ShapeSegment : public Data::Segment
{
    TYPESYSTEM_HEADER();

public:
    ShapeSegment(const TopoShape &ShapeIn):Shape(ShapeIn){}
    ShapeSegment(){}
    virtual std::string getName() const;

    TopoShape Shape;
};

} // namespace Part

#endif // PART_TOPOSHAPE_H
