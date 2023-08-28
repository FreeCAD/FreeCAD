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


class gp_Ax1;
class gp_Ax2;
class gp_Pln;
class gp_Vec;

namespace App {
class Color;
}

namespace Part
{

/* A special sub-class to indicate null shapes
 */
class PartExport NullShapeException : public Base::ValueError
{
public:
   /// Construction
   NullShapeException();
   explicit NullShapeException(const char * sMessage);
   explicit NullShapeException(const std::string& sMessage);
   /// Destruction
   ~NullShapeException() noexcept override = default;
};

/* A special sub-class to indicate boolean failures
 */
class PartExport BooleanException : public Base::CADKernelError
{
public:
   /// Construction
   BooleanException();
   explicit BooleanException(const char * sMessage);
   explicit BooleanException(const std::string& sMessage);
   /// Destruction
   ~BooleanException() noexcept override = default;
};

class PartExport ShapeSegment : public Data::Segment
{
    TYPESYSTEM_HEADER_WITH_OVERRIDE();

public:
    ShapeSegment(const TopoDS_Shape &ShapeIn):Shape(ShapeIn){}
    ShapeSegment() = default;
    std::string getName() const override;

    TopoDS_Shape Shape;
};



/** The representation for a CAD Shape
 */
class PartExport TopoShape : public Data::ComplexGeoData
{
    TYPESYSTEM_HEADER_WITH_OVERRIDE();

public:
    TopoShape();
    TopoShape(const TopoDS_Shape&);//explicit bombs
    TopoShape(const TopoShape&);
    ~TopoShape() override;

    inline void setShape(const TopoDS_Shape& shape) {
        this->_Shape = shape;
    }

    inline const TopoDS_Shape& getShape() const {
        return this->_Shape;
    }

    void operator = (const TopoShape&);

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
    Base::BoundBox3d getBoundBox()const override;
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
    void getLinesFromSubShape(const TopoDS_Shape& shape, std::vector<Base::Vector3d>& vertices, std::vector<Line>& lines) const;
    void getFacesFromDomains(const std::vector<Domain>& domains, std::vector<Base::Vector3d>& vertices, std::vector<Facet>& faces) const;

public:
    /// Get the standard accuracy to be used with getPoints, getLines or getFaces
    double getAccuracy() const override;
    /** Get points from object with given accuracy */
    void getPoints(std::vector<Base::Vector3d> &Points,
        std::vector<Base::Vector3d> &Normals,
        double Accuracy, uint16_t flags=0) const override;
    /** Get lines from object with given accuracy */
    void getLines(std::vector<Base::Vector3d> &Points,std::vector<Line> &lines,
        double Accuracy, uint16_t flags=0) const override;
    void getFaces(std::vector<Base::Vector3d> &Points,std::vector<Facet> &faces,
        double Accuracy, uint16_t flags=0) const override;
    void setFaces(const std::vector<Base::Vector3d> &Points,
                  const std::vector<Facet> &faces, double tolerance=1.0e-06);
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
    void getLinesFromSubElement(
        const Data::Segment*,
        std::vector<Base::Vector3d> &Points,
        std::vector<Line> &lines) const override;
    /** Get faces from segment */
    void getFacesFromSubElement(
        const Data::Segment*,
        std::vector<Base::Vector3d> &Points,
        std::vector<Base::Vector3d> &PointNormals,
        std::vector<Facet> &faces) const override;
    //@}
    /// get the Topo"sub"Shape with the given name
    TopoDS_Shape getSubShape(const char* Type, bool silent=false) const;
    TopoDS_Shape getSubShape(TopAbs_ShapeEnum type, int idx, bool silent=false) const;
    std::vector<TopoShape> getSubTopoShapes(TopAbs_ShapeEnum type=TopAbs_SHAPE) const;
    std::vector<TopoDS_Shape> getSubShapes(TopAbs_ShapeEnum type=TopAbs_SHAPE) const;
    unsigned long countSubShapes(const char* Type) const;
    unsigned long countSubShapes(TopAbs_ShapeEnum type) const;
    bool hasSubShape(const char *Type) const;
    bool hasSubShape(TopAbs_ShapeEnum type) const;
    /// get the Topo"sub"Shape with the given name
    PyObject * getPySubShape(const char* Type, bool silent=false) const;
    PyObject * getPyObject() override;
    void setPyObject(PyObject*) override;

    /** @name Save/restore */
    //@{
    void Save (Base::Writer &writer) const override;
    void Restore(Base::XMLReader &reader) override;

    void SaveDocFile (Base::Writer &writer) const override;
    void RestoreDocFile(Base::Reader &reader) override;
    unsigned int getMemSize () const override;
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
    void exportBinary(std::ostream&) const;
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
    bool isCoplanar(const TopoShape &other, double tol=-1) const;
    bool findPlane(gp_Pln &pln, double tol=-1) const;
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
    void transformGeometry(const Base::Matrix4D &rclMat) override;
    TopoDS_Shape transformGShape(const Base::Matrix4D&, bool copy = false) const;
    bool transformShape(const Base::Matrix4D&, bool copy, bool checkScale=false);
    TopoDS_Shape mirror(const gp_Ax2&) const;
    TopoDS_Shape toNurbs() const;
    TopoDS_Shape replaceShape(const std::vector< std::pair<TopoDS_Shape,TopoDS_Shape> >& s) const;
    TopoDS_Shape removeShape(const std::vector<TopoDS_Shape>& s) const;
    void sewShape(double tolerance = 1.0e-06);
    bool fix(double, double, double);
    bool removeInternalWires(double);
    TopoDS_Shape removeSplitter() const;
    TopoDS_Shape defeaturing(const std::vector<TopoDS_Shape>& s) const;
    TopoDS_Shape makeShell(const TopoDS_Shape&) const;
    //@}

    /** @name Element name mapping aware shape maker
     *
     * To be complete in next batch of patches
     */
    //@{
    TopoShape &makeCompound(const std::vector<TopoShape> &shapes, const char *op=nullptr, bool force=true);

    TopoShape &makeWires(const TopoShape &shape, const char *op=nullptr, bool fix=false, double tol=0.0);
    TopoShape makeWires(const char *op=nullptr, bool fix=false, double tol=0.0) const {
        return TopoShape().makeWires(*this,op,fix,tol);
    }
    TopoShape &makeFace(const std::vector<TopoShape> &shapes, const char *op=nullptr, const char *maker=nullptr);
    TopoShape &makeFace(const TopoShape &shape, const char *op=nullptr, const char *maker=nullptr);
    TopoShape makeFace(const char *op=nullptr, const char *maker=nullptr) const {
        return TopoShape().makeFace(*this,op,maker);
    }
    bool _makeTransform(const TopoShape &shape, const Base::Matrix4D &mat,
            const char *op=nullptr, bool checkScale=false, bool copy=false);

    TopoShape &makeTransform(const TopoShape &shape, const Base::Matrix4D &mat,
            const char *op=nullptr, bool checkScale=false, bool copy=false) {
        _makeTransform(shape,mat,op,checkScale,copy);
        return *this;
    }
    TopoShape makeTransform(const Base::Matrix4D &mat, const char *op=nullptr,
            bool checkScale=false, bool copy=false) const {
        return TopoShape().makeTransform(*this,mat,op,checkScale,copy);
    }

    TopoShape &makeTransform(const TopoShape &shape, const gp_Trsf &trsf,
            const char *op=nullptr, bool copy=false);
    TopoShape makeTransform(const gp_Trsf &trsf, const char *op=nullptr, bool copy=false) const {
        return TopoShape().makeTransform(*this,trsf,op,copy);
    }

    void move(const TopLoc_Location &loc) {
        _Shape.Move(loc);
    }
    TopoShape moved(const TopLoc_Location &loc) const {
        TopoShape ret(*this);
        ret._Shape.Move(loc);
        return ret;
    }

    TopoShape &makeGTransform(const TopoShape &shape, const Base::Matrix4D &mat,
            const char *op=nullptr, bool copy=false);
    TopoShape makeGTransform(const Base::Matrix4D &mat, const char *op=nullptr, bool copy=false) const {
        return TopoShape().makeGTransform(*this,mat,op,copy);
    }

    TopoShape &makeRefine(const TopoShape &shape, const char *op=nullptr, bool no_fail=true);
    TopoShape makeRefine(const char *op=nullptr, bool no_fail=true) const {
        return TopoShape().makeRefine(*this,op,no_fail);
    }
    //@}

    static TopAbs_ShapeEnum shapeType(const char *type,bool silent=false);
    static TopAbs_ShapeEnum shapeType(char type,bool silent=false);
    TopAbs_ShapeEnum shapeType(bool silent=false) const;
    static const std::string &shapeName(TopAbs_ShapeEnum type,bool silent=false);
    const std::string &shapeName(bool silent=false) const;
    static std::pair<TopAbs_ShapeEnum,int> shapeTypeAndIndex(const char *name);
private:
    TopoDS_Shape _Shape;
};

} //namespace Part


#endif // PART_TOPOSHAPE_H
