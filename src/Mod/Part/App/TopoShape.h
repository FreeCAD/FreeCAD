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
#include <App/StringHasher.h>

class BRepBuilderAPI_MakeShape;
class BRepBuilderAPI_Sewing;
class BRepOffsetAPI_ThruSections;
class gp_Ax1;
class gp_Ax2;
class gp_Vec;
class gp_Trsf;
class gp_GTrsf;

namespace Part
{

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
    TopoDS_Shape getSubShape(const char* Type) const;
    TopoDS_Shape getSubShape(TopAbs_ShapeEnum type, int idx) const;
    TopoShape getSubTopoShape(const char *Type) const;
    TopoShape getSubTopoShape(TopAbs_ShapeEnum type, int idx) const;
    std::vector<TopoShape> getSubTopoShapes(TopAbs_ShapeEnum type=TopAbs_SHAPE) const;
    unsigned long countSubShapes(const char* Type) const;
    unsigned long countSubShapes(TopAbs_ShapeEnum type) const;
    bool hasSubShape(const char *Type) const;
    bool hasSubShape(TopAbs_ShapeEnum type) const;
    /// get the Topo"sub"Shape with the given name
    PyObject * getPySubShape(const char* Type) const;

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
    void importBrep(std::istream&);
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
    TopoDS_Shape section(TopoDS_Shape) const;
    TopoDS_Shape section(const std::vector<TopoDS_Shape>&, Standard_Real tolerance = 0.0) const;
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
    void transformShape(const Base::Matrix4D&, bool copy, bool checkScale=false);
    TopoDS_Shape mirror(const gp_Ax2&) const;
    TopoDS_Shape toNurbs() const;
    TopoDS_Shape replaceShape(const std::vector< std::pair<TopoDS_Shape,TopoDS_Shape> >& s) const;
    TopoDS_Shape removeShape(const std::vector<TopoDS_Shape>& s) const;
    void sewShape();
    bool fix(double, double, double);
    bool removeInternalWires(double);
    TopoDS_Shape removeSplitter() const;
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
    
    TopoShape &makECompound(const std::vector<TopoShape> &shapes, bool appendTag=true, 
            const char *op=0, bool force=true);

    TopoShape &makEWires(const std::vector<TopoShape> &shapes, const char *op=0, bool fix=false, double tol=0.0);
    TopoShape &makEWires(const TopoShape &shape, const char *op=0, bool fix=false, double tol=0.0);
    TopoShape makEWires(const char *op=0, bool fix=false, double tol=0.0) const {
        return TopoShape(Tag).makEWires(*this,op,fix,tol);
    }

    TopoShape &makEFace(const std::vector<TopoShape> &shapes, const char *op=0, const char *maker=0);
    TopoShape &makEFace(const TopoShape &shape, const char *op=0, const char *maker=0);
    TopoShape makEFace(const char *op=0, const char *maker=0) const {
        return TopoShape(Tag).makEFace(*this,op,maker);
    }

    TopoShape &makEFilledFace(const std::vector<TopoShape> &shapes, const TopoShape &surface,
            const char *op=0, bool appendTag=true);

    TopoShape &makESolid(const TopoShape &shape, const char *op=0, bool appendTag=false);
    TopoShape makESolid(const char *op=0, bool appendTag=false) const {
        return TopoShape(Tag).makESolid(*this,op,appendTag);
    }

    TopoShape &makEShape(BRepBuilderAPI_MakeShape &mkShape, 
            const std::vector<TopoShape> &sources, const char *op=0, bool appendTag=true);
    TopoShape &makEShape(BRepBuilderAPI_MakeShape &mkShape, 
            const TopoShape &source, const char *op=0, bool appendTag=false);
    TopoShape makEShape(BRepBuilderAPI_MakeShape &mkShape, const char *op=0, bool appendTag=false) const {
        return TopoShape(Tag).makEShape(mkShape,*this,op,appendTag);
    }

    TopoShape &makEShape(BRepBuilderAPI_Sewing &mkShape, 
            const std::vector<TopoShape> &source, const char *op=0, bool appendTag=true);
    TopoShape &makEShape(BRepBuilderAPI_Sewing &mkShape, 
            const TopoShape &source, const char *op=0, bool appendTag=true);
    TopoShape makEShape(BRepBuilderAPI_Sewing &mkShape, const char *op=0, bool appendTag=true) const {
        return TopoShape(Tag).makEShape(mkShape,*this,op,appendTag);
    }

    TopoShape &makEShape(BRepOffsetAPI_ThruSections &mkShape, 
            const std::vector<TopoShape> &source, const char *op=0, bool appendTag=true);
    TopoShape &makEShape(BRepOffsetAPI_ThruSections &mkShape, 
            const TopoShape &source, const char *op=0, bool appendTag=true);
    TopoShape makEShape(BRepOffsetAPI_ThruSections &mkShape, const char *op=0, bool appendTag=true) const {
        return TopoShape(Tag).makEShape(mkShape,*this,op,appendTag);
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
            const std::vector<TopoShape> &sources, const char *op=0, bool appendTag=true);

    TopoShape &makEShape(const char *maker, const std::vector<TopoShape> &shapes, 
            const char *op=0, bool appendTag=true, double tol = 0.0);
    TopoShape &makEShape(const char *maker, const TopoShape &shape, const char *op=0, 
            bool appendTag=false, double tol = 0.0);
    TopoShape makEShape(const char *maker, const char *op=0, bool appendTag=false, double tol = 0.0) const {
        return TopoShape(Tag).makEShape(maker,*this,op,appendTag,tol);
    }

    TopoShape &makETransform(const TopoShape &shape, const Base::Matrix4D &mat, const char *op=0,
            bool appendTag=false, bool checkScale=false);
    TopoShape &makETransform(const TopoShape &shape, const gp_Trsf &mat, const char *op=0,
            bool appendTag=false, bool checkScale=false) {
        return makETransform(shape,convert(mat),op,appendTag,checkScale);
    }
    TopoShape makETransform(const Base::Matrix4D &mat, const char *op=0,
            bool appendTag=false, bool checkScale=false) const {
        return TopoShape(Tag).makETransform(*this,mat,op,appendTag,checkScale);
    }
    TopoShape makETransform(const gp_Trsf &mat, const char *op=0,
            bool appendTag=false, bool checkScale=false) const {
        return TopoShape(Tag).makETransform(*this,mat,op,appendTag,checkScale);
    }

    TopoShape &makEGTransform(const TopoShape &shape, const Base::Matrix4D &mat, 
            const char *op=0, bool appendTag=false);
    TopoShape makEGTransform(const Base::Matrix4D &mat, const char *op=0, bool appendTag=false) const {
        return TopoShape(Tag).makEGTransform(*this,mat,op,appendTag);
    }

    TopoShape &makECopy(const TopoShape &shape, const char *op=0, bool appendTag=true);
    TopoShape makECopy(const char *op=0, bool appendTag=true) const {
        return TopoShape(Tag).makECopy(*this,op,appendTag);
    }

    TopoShape &makERefine(const TopoShape &shape, const char *op=0, bool no_fail=true);
    TopoShape makERefine(const char *op=0, bool no_fail=true) const {
        return TopoShape(Tag).makERefine(*this,op,no_fail);
    }

    TopoShape &makEThickSolid(const TopoShape &shape, const std::vector<TopoShape> &faces, 
            double offset, double tol, bool intersection = false, bool selfInter = false,
            short offsetMode = 0, short join = 0, const char *op=0, bool appendTag=false);
    TopoShape makEThickSolid(const std::vector<TopoShape> &faces, 
            double offset, double tol, bool intersection = false, bool selfInter = false,
            short offsetMode = 0, short join = 0, const char *op=0, bool appendTag=false) const {
        return TopoShape(Tag).makEThickSolid(*this,faces,offset,tol,intersection,selfInter,
                offsetMode,join,op,appendTag);
    }

    TopoShape &makERevolve(const TopoShape &base, const gp_Ax1& axis, double d, 
            const char *face_maker=0, const char *op=0, bool appendTag=false);
    TopoShape makERevolve(const gp_Ax1& axis, double d, 
            const char *face_maker=0, const char *op=0, bool appendTag=false) const {
        return TopoShape(Tag).makERevolve(*this,axis,d,face_maker,op,appendTag);
    }

    TopoShape &makEPrism(const TopoShape &base, const gp_Vec& vec, const char *op=0, bool appendTag=false);
    TopoShape makEPrism(const gp_Vec& vec, const char *op=0, bool appendTag=false) const {
        return TopoShape(Tag).makEPrism(*this,vec,op,appendTag);
    }

    TopoShape &makEOffset(const TopoShape &shape, double offset, double tol,
            bool intersection = false, bool selfInter = false, short offsetMode = 0, 
            short join = 0, bool fill = false, const char *op=0, bool appendTag=false);
    TopoShape makEOffset(double offset, double tol, bool intersection = false, bool selfInter = false, 
            short offsetMode=0, short join=0, bool fill=false, const char *op=0, bool appendTag=false) const {
        return TopoShape(Tag).makEOffset(*this,offset,tol,intersection,selfInter,
                offsetMode,join,fill,op,appendTag);
    }

    TopoShape &makEOffset2D(const TopoShape &shape, double offset, short joinType=0, bool fill=false, 
            bool allowOpenResult=false, bool intersection=false, const char *op=0, bool appendTag=false);
    TopoShape makEOffset2D(double offset, short joinType=0, bool fill=false, bool allowOpenResult=false, 
            bool intersection=false, const char *op=0, bool appendTag=false) const {
        return TopoShape(Tag).makEOffset2D(*this,offset,joinType,fill,allowOpenResult,intersection,
                op,appendTag);
    }

    TopoShape &makEPipeShell(const std::vector<TopoShape> &shapes, const Standard_Boolean make_solid,
            const Standard_Boolean isFrenet, int transition=0, const char *op=0, bool appendTag=true);

    TopoShape &makELoft(const std::vector<TopoShape> &shapes,
            Standard_Boolean isSolid, Standard_Boolean isRuled, Standard_Boolean isClosed=Standard_False,
            Standard_Integer maxDegree=5, const char *op=0, bool appendTag=true);

    TopoShape &makERuledSurface(const std::vector<TopoShape> &shapes, int orientation=0,
            const char *op=0, bool appendTag=true);

    TopoShape &makEMirror(const TopoShape &shape, const gp_Ax2&, const char *op=0, bool appendTag=false);
    TopoShape makEMirror(const gp_Ax2& ax, const char *op=0, bool appendTag=false) const {
        return TopoShape(Tag).makEMirror(*this,ax,op,appendTag);
    }

    TopoShape &makESlice(const TopoShape &shape, const Base::Vector3d&, double, const char *op=0);
    TopoShape makESlice(const Base::Vector3d& dir, double d, const char *op=0) const {
        return TopoShape(Tag).makESlice(*this,dir,d,op);
    }
    TopoShape &makESlice(const TopoShape &shape, const Base::Vector3d&, 
            const std::vector<double>&, const char *op=0);
    TopoShape makESlice(const Base::Vector3d &dir, const std::vector<double> &d, const char *op=0) const {
        return TopoShape(Tag).makESlice(*this,dir,d,op);
    }

    TopoShape &makEFillet(const TopoShape &shape, const std::vector<TopoShape> &edges, 
            double radius1, double radius2, const char *op=0, bool appendTag=false);
    TopoShape makEFillet(const std::vector<TopoShape> &edges, 
            double radius1, double radius2, const char *op=0, bool appendTag=false) const {
        return TopoShape(Tag).makEFillet(*this,edges,radius1,radius2,op,appendTag);
    }

    TopoShape &makEChamfer(const TopoShape &shape, const std::vector<TopoShape> &edges, 
            double radius1, double radius2, const char *op=0, bool appendTag=false);
    TopoShape makEChamfer(const std::vector<TopoShape> &edges, 
            double radius1, double radius2, const char *op=0, bool appendTag=false) const {
        return TopoShape(Tag).makEChamfer(*this,edges,radius1,radius2,op,appendTag);
    }

    TopoShape &replacEShape(const TopoShape &shape, const std::vector<std::pair<TopoShape,TopoShape> > &s);
    TopoShape replacEShape(const std::vector<std::pair<TopoShape,TopoShape> > &s) const {
        return TopoShape(Tag).replacEShape(*this,s);
    }
        
    TopoShape &removEShape(const TopoShape &shape, const std::vector<TopoShape>& s);
    TopoShape removEShape(const std::vector<TopoShape>& s) const {
        return TopoShape(Tag).removEShape(*this,s);
    }

    TopoShape &makEGeneralFuse(const std::vector<TopoShape> &shapes, 
            std::vector<std::vector<TopoShape> > &modified, double tol=0, const char *op=0, bool appendTag=true);

    static const std::string &modPostfix();
    //@}

    /** @name Element name mapping helper functions
     *
     * These functions are implemented in TopoShapeEx.cpp
     */
    //@{
    void mapSubElement(TopAbs_ShapeEnum type,
            const TopoShape &other,const char *op=0,bool mapAll=true,bool appendTag=false);
    void mapSubElement(TopAbs_ShapeEnum type, const TopTools_IndexedMapOfShape &shapeMap, 
            const TopoShape &other,const char *op=0,bool mapAll=true,bool appendTag=false);
    void mapSubElement(TopAbs_ShapeEnum type, const TopoShape &other,
            const TopTools_IndexedMapOfShape &otherMap,const char *op=0,bool mapAll=true,bool appendTag=false);
    void mapSubElement(TopAbs_ShapeEnum type, const TopTools_IndexedMapOfShape &shapeMap, const TopoShape &other,
            const TopTools_IndexedMapOfShape &otherMap,const char *op=0,bool mapAll=true,bool appendTag=false);
    void mapSubElement(TopAbs_ShapeEnum type, const std::vector<TopoShape> &shapes, 
            const char *op=0, bool mapAll=true,bool appendTag=false);
    void mapSubElementsTo(TopAbs_ShapeEnum type, std::vector<TopoShape> &shapes, 
            const char *op=0, bool mapAll=true,bool appendTag=false) const;

    bool canMapElement() const{
        return !isNull() && (Tag!=0 || getElementMapSize()!=0);
    }

    bool canMapElement(const TopoShape &other) const{
        return !isNull() && other.canMapElement();
    }

    virtual std::string getElementMapVersion() const override;
    //@}

    static TopAbs_ShapeEnum shapeEnum(const char *type);
    static const std::string &shapeName(TopAbs_ShapeEnum type);

public:
    /** Shape tag 
     *
     * A very loosely controlled tag, which is why it is made public.  Its main
     * purpose is not for unique identification, but as a way for the user to
     * disambiguate input shape and generate mappable topological names. It is
     * the user's job to assign sensible tags before generating element maps.
     * The default value is 0, and will disable auto element mapping.
     *
     * A very simple example, for a compound C created by shape A and B.
     * TopoShape will map A's Edge1 and B's Edge1 to C as 
     *
     *      <A.Tag>_Edge1
     *      <B.Tag>_Edge1
     *
     * If C is a fusion, then for elements that are unmodified, it will be same
     * as compound. For modified of generated elements, it will be some thing
     * like
     *
     *      <A.Tag>_Face1_<B.Tag>_Face2
     *
     * If A and/or B has its own element mapping, then the trailing _EdgeX or
     * _FaceX will be replaced by those mappings.
     *
     * Now, you may be thinking that if this process continues, the mapped
     * element name will grow without control. The caller can pass a
     * App::StringHasher when calling Data::ComplexGeoData::setElementName(),
     * which can hash on the mapped name and shorten it to a integer index.
     * App::Document provides a persistent StringHasher.
     */
    mutable long Tag;

private:
    TopoDS_Shape _Shape;
};

} //namespace Part


#endif // PART_TOPOSHAPE_H
