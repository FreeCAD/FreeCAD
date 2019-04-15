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

#include <iostream>
#include <TopoDS_Compound.hxx>
#include <TopoDS_Wire.hxx>
#include <TopTools_ListOfShape.hxx>
#include <App/ComplexGeoData.h>
#include <Base/Exception.h>

class gp_Ax1;
class gp_Ax2;
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
    TopoShape();
    TopoShape(const TopoDS_Shape&);
    TopoShape(const TopoShape&);
    ~TopoShape();

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
    unsigned long countSubShapes(const char* Type) const;
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
    void transformShape(const Base::Matrix4D&, bool copy);
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

private:
    TopoDS_Shape _Shape;
};

} //namespace Part


#endif // PART_TOPOSHAPE_H
