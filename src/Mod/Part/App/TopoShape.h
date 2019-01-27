/***************************************************************************
 *   Copyright (c) Jürgen Riegel          (juergen.riegel@web.de) 2002     *
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
#include <TopoDS_Compound.hxx>
#include <TopoDS_Wire.hxx>
#include <TopTools_ListOfShape.hxx>
#include <TopTools_IndexedMapOfShape.hxx>
#include <App/ComplexGeoData.h>
#include <Base/Exception.h>

class BRepBuilderAPI_MakeShape;
class BRepBuilderAPI_Sewing;
class BRepOffsetAPI_ThruSections;
class BRepFeat_MakePrism;
class BRepOffsetAPI_MakePipeShell;
class BRepOffsetAPI_DraftAngle;
class BRepPrimAPI_MakeHalfSpace; 
class gp_Ax1;
class gp_Ax2;
class gp_Vec;
class gp_Trsf;
class gp_GTrsf;
class gp_Pln;
class gp_Dir;

namespace Part
{

/* A special sub-class to indicate null shapes
 */
class PartExport NullShapeException : public Base::ValueError
{
public:
   /// Construction
   NullShapeException();
   NullShapeException(const char * sMessage);
   NullShapeException(const std::string& sMessage);
   /// Construction
   NullShapeException(const NullShapeException &inst);
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
   /// Construction
   BooleanException(const BooleanException &inst);
   /// Destruction
   virtual ~BooleanException() throw() {}
};

class PartExport ShapeSegment : public Data::Segment
{
    TYPESYSTEM_HEADER();

public:
    ShapeSegment(const TopoDS_Shape &ShapeIn):Shape(ShapeIn){}
    ShapeSegment(){}
    virtual std::string getName() const;

    TopoDS_Shape Shape;
};



/** The representation for a CAD Shape
 */
class PartExport TopoShape : public Data::ComplexGeoData
{
    TYPESYSTEM_HEADER();

public:
    explicit TopoShape(long Tag=0, App::StringHasherRef hasher=App::StringHasherRef(), 
            const TopoDS_Shape &shape=TopoDS_Shape());
    TopoShape(const TopoDS_Shape&);
    TopoShape(const TopoShape&);
    ~TopoShape();

    inline void setShape(const TopoDS_Shape& shape, bool resetElementMap=true) {
        _Shape = shape;
        if(resetElementMap) {
            Hasher = App::StringHasherRef();
            this->resetElementMap();
        }
    }

    inline void setShape(const TopoShape& shape) {
        *this = shape;
    }

    inline const TopoDS_Shape& getShape() const {
        return this->_Shape;
    }

    void operator = (const TopoShape&);

    /** @name Placement control */
    //@{
    /// set the transformation of the CasCade Shape
    void setTransform(const Base::Matrix4D& rclTrf);
    /// set the transformation of the CasCade Shape
    void setPlacement(const Base::Placement& rclTrf);
    /// get the transformation of the CasCade Shape
    Base::Matrix4D getTransform(void) const;
    /// get the transformation of the CasCade Shape
    Base::Placement getPlacemet(void) const;
    /// Bound box from the CasCade shape
    Base::BoundBox3d getBoundBox(void)const;
    virtual bool getCenterOfGravity(Base::Vector3d& center) const;
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
    virtual std::vector<const char*> getElementTypes(void) const;
    virtual unsigned long countSubElements(const char* Type) const;
    /// get the subelement by type and number
    virtual Data::Segment* getSubElement(const char* Type, unsigned long) const;
    /** Get lines from segment */
    virtual void getLinesFromSubelement(
        const Data::Segment*,
        std::vector<Base::Vector3d> &Points,
        std::vector<Line> &lines) const;
    /** Get faces from segment */
    virtual void getFacesFromSubelement(
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
    std::vector<TopoShape> getSubTopoShapes(TopAbs_ShapeEnum type=TopAbs_SHAPE) const;
    std::vector<TopoDS_Shape> getSubShapes(TopAbs_ShapeEnum type=TopAbs_SHAPE) const;
    unsigned long countSubShapes(const char* Type) const;
    unsigned long countSubShapes(TopAbs_ShapeEnum type) const;
    bool hasSubShape(const char *Type) const;
    bool hasSubShape(TopAbs_ShapeEnum type) const;
    /// get the Topo"sub"Shape with the given name
    PyObject * getPySubShape(const char* Type, bool silent=false) const;

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
    void exportFaceSet(double, double, std::ostream&) const;
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
    TopoDS_Shape makeHelix(Standard_Real pitch, Standard_Real height,
        Standard_Real radius, Standard_Real angle=0,
        Standard_Boolean left=Standard_False, Standard_Boolean style=Standard_False) const;
    TopoDS_Shape makeLongHelix(Standard_Real pitch, Standard_Real height,
        Standard_Real radius, Standard_Real angle=0,
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
    TopoDS_Shape transformGShape(const Base::Matrix4D&) const;
    /** Transform shape
     * 
     * @param mat: transformation matrix
     * @param copy: whether to copy the shape before trasnformation
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
    void sewShape();
    bool fix(double, double, double);
    bool removeInternalWires(double);
    TopoDS_Shape removeSplitter() const;
    TopoDS_Shape defeaturing(const std::vector<TopoDS_Shape>& s) const;
    TopoDS_Shape makeShell(const TopoDS_Shape&) const;
    //@}

    /** @name Getting basic geometric entities */
    //@{
    /** Get points from object with given accuracy */
    virtual void getPoints(std::vector<Base::Vector3d> &Points,
        std::vector<Base::Vector3d> &Normals,
        float Accuracy, uint16_t flags=0) const;
    virtual void getFaces(std::vector<Base::Vector3d> &Points,std::vector<Facet> &faces,
        float Accuracy, uint16_t flags=0) const;
    void setFaces(const std::vector<Base::Vector3d> &Points,
                  const std::vector<Facet> &faces, float Accuracy=1.0e-06);
    void getDomains(std::vector<Domain>&) const;
    //@}

    /** @name Element name mapping aware shape maker 
     *
     * These functions are implemented in TopoShapeEx.cpp
     */
    //@{
    
    TopoShape &makECompound(const std::vector<TopoShape> &shapes, const char *op=0, bool force=true);

    TopoShape &makEWires(const std::vector<TopoShape> &shapes, const char *op=0, bool fix=false, double tol=0.0);
    TopoShape &makEWires(const TopoShape &shape, const char *op=0, bool fix=false, double tol=0.0);
    TopoShape makEWires(const char *op=0, bool fix=false, double tol=0.0) const {
        return TopoShape(0,Hasher).makEWires(*this,op,fix,tol);
    }

    TopoShape &makEFace(const std::vector<TopoShape> &shapes, const char *op=0, const char *maker=0);
    TopoShape &makEFace(const TopoShape &shape, const char *op=0, const char *maker=0);
    TopoShape makEFace(const char *op=0, const char *maker=0) const {
        return TopoShape(0,Hasher).makEFace(*this,op,maker);
    }

    TopoShape &makEFilledFace(const std::vector<TopoShape> &shapes, const TopoShape &surface, const char *op=0);

    TopoShape &makESolid(const std::vector<TopoShape> &shapes, const char *op=0);
    TopoShape &makESolid(const TopoShape &shape, const char *op=0);
    TopoShape makESolid(const char *op=0) const {
        return TopoShape(0,Hasher).makESolid(*this,op);
    }

    TopoShape &makEShape(BRepBuilderAPI_MakeShape &mkShape, 
            const std::vector<TopoShape> &sources, const char *op=0);
    TopoShape &makEShape(BRepBuilderAPI_MakeShape &mkShape, 
            const TopoShape &source, const char *op=0);
    TopoShape makEShape(BRepBuilderAPI_MakeShape &mkShape, const char *op=0) const {
        return TopoShape(0,Hasher).makEShape(mkShape,*this,op);
    }

    TopoShape &makEShape(BRepBuilderAPI_Sewing &mkShape, 
            const std::vector<TopoShape> &source, const char *op=0);
    TopoShape &makEShape(BRepBuilderAPI_Sewing &mkShape, 
            const TopoShape &source, const char *op=0);
    TopoShape makEShape(BRepBuilderAPI_Sewing &mkShape, const char *op=0) const {
        return TopoShape(0,Hasher).makEShape(mkShape,*this,op);
    }

    TopoShape &makEShape(BRepOffsetAPI_ThruSections &mkShape, 
            const std::vector<TopoShape> &source, const char *op=0);
    TopoShape &makEShape(BRepOffsetAPI_ThruSections &mkShape, 
            const TopoShape &source, const char *op=0);
    TopoShape makEShape(BRepOffsetAPI_ThruSections &mkShape, const char *op=0) const {
        return TopoShape(0,Hasher).makEShape(mkShape,*this,op);
    }

    TopoShape &makEShape(BRepOffsetAPI_MakePipeShell &mkShape, 
            const std::vector<TopoShape> &source, const char *op=0);

    TopoShape &makEShape(BRepPrimAPI_MakeHalfSpace &mkShape, 
            const TopoShape &source, const char *op=0);
    TopoShape makEShape(BRepPrimAPI_MakeHalfSpace  &mkShape, const char *op=0) const {
        return TopoShape(0,Hasher).makEShape(mkShape,*this,op);
    }

    TopoShape &makEShape(BRepFeat_MakePrism &mkShape, const TopoShape &source,
            const char *op=0);
    TopoShape makEShape(BRepFeat_MakePrism &mkShape,const char *op=0) const {
        return TopoShape(0,Hasher).makEShape(mkShape,*this,op);
    }

    struct Mapper {
        virtual std::vector<TopoDS_Shape> generated(const TopoDS_Shape &) const {
            return std::vector<TopoDS_Shape>();
        }
        virtual std::vector<TopoDS_Shape> modified(const TopoDS_Shape &) const {
            return std::vector<TopoDS_Shape>();
        }
    };
    TopoShape &makESHAPE(const TopoDS_Shape &shape, const Mapper &mapper, 
            const std::vector<TopoShape> &sources, const char *op=0);

    TopoShape &makEShape(const char *maker, const std::vector<TopoShape> &shapes, 
            const char *op=0, double tol = 0.0);
    TopoShape &makEShape(const char *maker, const TopoShape &shape, const char *op=0, double tol = 0.0);
    TopoShape makEShape(const char *maker, const char *op=0, double tol = 0.0) const {
        return TopoShape(0,Hasher).makEShape(maker,*this,op,tol);
    }

    bool _makETransform(const TopoShape &shape, const Base::Matrix4D &mat,
            const char *op=0, bool checkScale=false, bool copy=false);

    TopoShape &makETransform(const TopoShape &shape, const Base::Matrix4D &mat,
            const char *op=0, bool checkScale=false, bool copy=false) {
        _makETransform(shape,mat,op,checkScale,copy);
        return *this;
    }
    TopoShape makETransform(const Base::Matrix4D &mat, const char *op=0, 
            bool checkScale=false, bool copy=false) const {
        return TopoShape(Tag,Hasher).makETransform(*this,mat,op,checkScale,copy);
    }

    TopoShape &makETransform(const TopoShape &shape, const gp_Trsf &trsf, 
            const char *op=0, bool copy=false);
    TopoShape makETransform(const gp_Trsf &trsf, const char *op=0, bool copy=false) const {
        return TopoShape(Tag,Hasher).makETransform(*this,trsf,op,copy);
    }

    void move(const TopLoc_Location &loc) {
        _Shape.Move(loc);
    }
    TopoShape moved(const TopLoc_Location &loc) const {
        TopoShape ret(*this);
        ret._Shape.Move(loc);
        return ret;
    }

    TopoShape &makEGTransform(const TopoShape &shape, const Base::Matrix4D &mat, 
            const char *op=0, bool copy=false);
    TopoShape makEGTransform(const Base::Matrix4D &mat, const char *op=0, bool copy=false) const {
        return TopoShape(Tag,Hasher).makEGTransform(*this,mat,op,copy);
    }

    TopoShape &makECopy(const TopoShape &shape, const char *op=0, bool copyGeom=true, bool copyMesh=false);
    TopoShape makECopy(const char *op=0, bool copyGeom=true, bool copyMesh=false) const {
        return TopoShape(Tag,Hasher).makECopy(*this,op,copyGeom,copyMesh);
    }

    TopoShape &makERefine(const TopoShape &shape, const char *op=0, bool no_fail=true);
    TopoShape makERefine(const char *op=0, bool no_fail=true) const {
        return TopoShape(Tag,Hasher).makERefine(*this,op,no_fail);
    }

    TopoShape &makEThickSolid(const TopoShape &shape, const std::vector<TopoShape> &faces, 
            double offset, double tol, bool intersection = false, bool selfInter = false,
            short offsetMode = 0, short join = 0, const char *op=0);
    TopoShape makEThickSolid(const std::vector<TopoShape> &faces, 
            double offset, double tol, bool intersection = false, bool selfInter = false,
            short offsetMode = 0, short join = 0, const char *op=0) const {
        return TopoShape(0,Hasher).makEThickSolid(*this,faces,offset,tol,intersection,selfInter,
                offsetMode,join,op);
    }

    TopoShape &makERevolve(const TopoShape &base, const gp_Ax1& axis, double d, 
            const char *face_maker=0, const char *op=0);
    TopoShape makERevolve(const gp_Ax1& axis, double d, 
            const char *face_maker=0, const char *op=0) const {
        return TopoShape(0,Hasher).makERevolve(*this,axis,d,face_maker,op);
    }

    TopoShape &makEPrism(const TopoShape &base, const gp_Vec& vec, const char *op=0);
    TopoShape makEPrism(const gp_Vec& vec, const char *op=0) const {
        return TopoShape(0,Hasher).makEPrism(*this,vec,op);
    }

    TopoShape &makEOffset(const TopoShape &shape, double offset, double tol,
            bool intersection = false, bool selfInter = false, short offsetMode = 0, 
            short join = 0, bool fill = false, const char *op=0);
    TopoShape makEOffset(double offset, double tol, bool intersection = false, bool selfInter = false, 
            short offsetMode=0, short join=0, bool fill=false, const char *op=0) const {
        return TopoShape(0,Hasher).makEOffset(*this,offset,tol,intersection,selfInter,
                offsetMode,join,fill,op);
    }

    TopoShape &makEOffset2D(const TopoShape &shape, double offset, short joinType=0, bool fill=false, 
            bool allowOpenResult=false, bool intersection=false, const char *op=0);
    TopoShape makEOffset2D(double offset, short joinType=0, bool fill=false, bool allowOpenResult=false, 
            bool intersection=false, const char *op=0) const {
        return TopoShape(0,Hasher).makEOffset2D(*this,offset,joinType,fill,allowOpenResult,intersection,op);
    }

    TopoShape &makEPipeShell(const std::vector<TopoShape> &shapes, const Standard_Boolean make_solid,
            const Standard_Boolean isFrenet, int transition=0, const char *op=0);

    TopoShape &makELoft(const std::vector<TopoShape> &shapes,
            Standard_Boolean isSolid, Standard_Boolean isRuled, Standard_Boolean isClosed=Standard_False,
            Standard_Integer maxDegree=5, const char *op=0);

    TopoShape &makERuledSurface(const std::vector<TopoShape> &shapes, int orientation=0, const char *op=0);

    TopoShape &makEMirror(const TopoShape &shape, const gp_Ax2&, const char *op=0);
    TopoShape makEMirror(const gp_Ax2& ax, const char *op=0) const {
        return TopoShape(0,Hasher).makEMirror(*this,ax,op);
    }

    TopoShape &makESlice(const TopoShape &shape, const Base::Vector3d&, double, const char *op=0);
    TopoShape makESlice(const Base::Vector3d& dir, double d, const char *op=0) const {
        return TopoShape(0,Hasher).makESlice(*this,dir,d,op);
    }
    TopoShape &makESlice(const TopoShape &shape, const Base::Vector3d&, 
            const std::vector<double>&, const char *op=0);
    TopoShape makESlice(const Base::Vector3d &dir, const std::vector<double> &d, const char *op=0) const {
        return TopoShape(0,Hasher).makESlice(*this,dir,d,op);
    }

    TopoShape &makEFillet(const TopoShape &shape, const std::vector<TopoShape> &edges, 
            double radius1, double radius2, const char *op=0);
    TopoShape makEFillet(const std::vector<TopoShape> &edges, 
            double radius1, double radius2, const char *op=0) const {
        return TopoShape(0,Hasher).makEFillet(*this,edges,radius1,radius2,op);
    }

    TopoShape &makEChamfer(const TopoShape &shape, const std::vector<TopoShape> &edges, 
            double radius1, double radius2, const char *op=0);
    TopoShape makEChamfer(const std::vector<TopoShape> &edges, 
            double radius1, double radius2, const char *op=0) const {
        return TopoShape(0,Hasher).makEChamfer(*this,edges,radius1,radius2,op);
    }

    TopoShape &makEDraft(const TopoShape &shape, const std::vector<TopoShape> &faces, 
           const gp_Dir &pullDirection, double angle, const gp_Pln &neutralPlane,
           bool retry=true, const char *op=0);
    TopoShape makEDraft(const std::vector<TopoShape> &faces, 
           const gp_Dir &pullDirection, double angle, const gp_Pln &neutralPlane,
           bool retry=true, const char *op=0) const {
        return TopoShape(0,Hasher).makEDraft(*this,faces,pullDirection,angle,neutralPlane,retry,op);
    }

    TopoShape &replacEShape(const TopoShape &shape, const std::vector<std::pair<TopoShape,TopoShape> > &s);
    TopoShape replacEShape(const std::vector<std::pair<TopoShape,TopoShape> > &s) const {
        return TopoShape(0,Hasher).replacEShape(*this,s);
    }
        
    TopoShape &removEShape(const TopoShape &shape, const std::vector<TopoShape>& s);
    TopoShape removEShape(const std::vector<TopoShape>& s) const {
        return TopoShape(0,Hasher).removEShape(*this,s);
    }

    TopoShape &makEGeneralFuse(const std::vector<TopoShape> &shapes, 
            std::vector<std::vector<TopoShape> > &modified, double tol=0, const char *op=0);

    TopoShape &makEFuse(const std::vector<TopoShape> &shapes, const char *op=0, double tol=0);
    TopoShape makEFuse(const TopoShape &shape, const char *op=0, double tol=0) const {
        return TopoShape(0,Hasher).makEFuse({*this,shape},op,tol);
    }

    TopoShape &makECut(const std::vector<TopoShape> &shapes, const char *op=0, double tol=0);
    TopoShape makECut(const TopoShape &shape, const char *op=0, double tol=0) const {
        return TopoShape(0,Hasher).makECut({*this,shape},op,tol);
    }

    static const std::string &modPostfix();
    static const std::string &genPostfix();
    static const std::string &modgenPostfix();
    static const std::string &upperPostfix();
    static const std::string &lowerPostfix();
    //@}

    /** @name Element name mapping helper functions
     *
     * These functions are implemented in TopoShapeEx.cpp
     */
    //@{
    void mapSubElement(const TopoShape &other,const char *op=0, bool forceHasher=false);
    void mapSubElement(const std::vector<TopoShape> &shapes, const char *op=0) {
        for(auto &shape : shapes)
            mapSubElement(shape,op);
    }
    void mapSubElementsTo(std::vector<TopoShape> &shapes, const char *op=0) const {
        for(auto &shape : shapes)
            shape.mapSubElement(*this,op);
    }

    bool canMapElement(const TopoShape &other) const;

    std::vector<std::pair<std::string,std::string> > getRelatedElements(
            const char *name, bool sameType=true) const;

    void cacheRelatedElements(const char *name, bool sameType,
            const std::vector<std::pair<std::string,std::string> > &names) const;

    bool getRelatedElementsCached(const char *name, bool sameType,
            std::vector<std::pair<std::string,std::string> > &names) const;

    virtual std::string getElementMapVersion() const override;

    const char *setElementComboName(const char *element, 
            const std::vector<std::string> &names, const char *marker=0, const char *op=0);

    virtual void reTagElementMap(long tag, App::StringHasherRef hasher, const char *postfix=0) override;
    //@}


    /** @name sub shape cached functions
     *
     * These functions uses internal caches for sub shape maps to imporve performance
     */
    //@{
    void initCache(int reset=0, const char *file=0, int line=0) const;
    int findShape(const TopoDS_Shape &subshape) const;
    TopoDS_Shape findShape(const char *name) const;
    TopoDS_Shape findShape(TopAbs_ShapeEnum type, int idx) const;
    int findAncestor(const TopoDS_Shape &subshape, TopAbs_ShapeEnum type) const;
    TopoDS_Shape findAncestorShape(const TopoDS_Shape &subshape, TopAbs_ShapeEnum type) const;
    std::vector<int> findAncestors(const TopoDS_Shape &subshape, TopAbs_ShapeEnum type) const;
    std::vector<TopoDS_Shape> findAncestorsShapes(const TopoDS_Shape &subshape, TopAbs_ShapeEnum type) const;
    //@}

    static TopAbs_ShapeEnum shapeType(const char *type,bool silent=false);
    static TopAbs_ShapeEnum shapeType(char type,bool silent=false);
    TopAbs_ShapeEnum shapeType(bool silent=false) const;
    static const std::string &shapeName(TopAbs_ShapeEnum type,bool silent=false);
    const std::string &shapeName(bool silent=false) const;
    static std::pair<TopAbs_ShapeEnum,int> shapeTypeAndIndex(const char *name);

private:
    TopoDS_Shape _Shape;

    class Cache;
    mutable std::shared_ptr<Cache> _Cache;
};

} //namespace Part


#endif // PART_TOPOSHAPE_H
