/***************************************************************************
 *   Copyright (c) Jürgen Riegel          (juergen.riegel@web.de) 2007     *
 *   Copyright (c) Luke Parry             (l.parry@warwick.ac.uk) 2013     *
 *   Copyright (c) WandererFan            (wandererfan@gmail.com) 2016     *
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

#ifndef _DrawViewPart_h_
#define _DrawViewPart_h_

#include <TopoDS_Edge.hxx>
#include <TopoDS_Vertex.hxx>
#include <TopoDS_Wire.hxx>

#include <App/DocumentObject.h>
#include <App/PropertyLinks.h>
#include "DrawView.h"
#include <App/FeaturePython.h>

#include <Base/BoundBox.h>

class gp_Pnt;

namespace TechDrawGeometry
{
class GeometryObject;
class Vertex;
class BaseGeom;
class Face;
}

namespace TechDraw {
class DrawHatch;
}

namespace TechDraw
{
class DrawViewSection;

class TechDrawExport DrawViewPart : public DrawView
{
    PROPERTY_HEADER(TechDraw::DrawViewPart);

public:
    DrawViewPart(void);
    virtual ~DrawViewPart();

    App::PropertyLink   Source;                                        //Part Feature
    App::PropertyVector Direction;  //TODO: Rename to YAxisDirection or whatever this actually is  (ProjectionDirection)
    App::PropertyVector XAxisDirection;
    App::PropertyBool   ShowHiddenLines;
    App::PropertyBool   ShowSmoothLines;
    App::PropertyBool   ShowSeamLines;
    App::PropertyFloat  LineWidth;
    App::PropertyFloat  HiddenWidth;
    App::PropertyBool   ShowCenters;
    App::PropertyFloat  CenterScale;
    App::PropertyFloatConstraint  Tolerance;
    App::PropertyBool   HorizCenterLine;
    App::PropertyBool   VertCenterLine;

    App::PropertyBool   ShowSectionLine;
    App::PropertyBool   HorizSectionLine;     //true(horiz)/false(vert)
    App::PropertyBool   ArrowUpSection;       //true(up/right)/false(down/left)
    App::PropertyString SymbolSection;


    std::vector<TechDraw::DrawHatch*> getHatches(void) const;

    //TODO: are there use-cases for Python access to TechDrawGeometry???

    const std::vector<TechDrawGeometry::Vertex *> & getVertexGeometry() const;
    const std::vector<TechDrawGeometry::BaseGeom  *> & getEdgeGeometry() const;
    const std::vector<TechDrawGeometry::Face *> & getFaceGeometry() const;
    bool hasGeometry(void) const;

    TechDrawGeometry::BaseGeom* getProjEdgeByIndex(int idx) const;               //get existing geom for edge idx in projection
    TechDrawGeometry::Vertex* getProjVertexByIndex(int idx) const;               //get existing geom for vertex idx in projection
    std::vector<TechDrawGeometry::BaseGeom*> getProjFaceByIndex(int idx) const;  //get edges for face idx in projection
    virtual Base::BoundBox3d getBoundingBox() const;
    double getBoxX(void) const;
    double getBoxY(void) const;
    virtual QRectF getRect() const;
    virtual DrawViewSection* getSectionRef() const;                    //is there a ViewSection based on this ViewPart?
    Base::Vector3d getUDir(void)  {return uDir;}                       //paperspace X
    Base::Vector3d getVDir(void)  {return vDir;}                       //paperspace Y
    Base::Vector3d getWDir(void)  {return wDir;}                       //paperspace Z

    short mustExecute() const;

    /** @name methods overide Feature */
    //@{
    /// recalculate the Feature
    virtual App::DocumentObjectExecReturn *execute(void);
    //@}

    /// returns the type name of the ViewProvider
    virtual const char* getViewProviderName(void) const {
        return "TechDrawGui::ViewProviderViewPart";
    }
    //return PyObject as DrawViewPartPy
    virtual PyObject *getPyObject(void);

    void dumpVertexes(const char* text, const TopoDS_Shape& s);
    void dumpEdge(char* label, int i, TopoDS_Edge e);
    void dump1Vertex(const char* label, const TopoDS_Vertex& v);
    void countFaces(const char* label, const TopoDS_Shape& s);
    void countWires(const char* label, const TopoDS_Shape& s);
    void countEdges(const char* label, const TopoDS_Shape& s);
    Base::Vector3d getValidXDir() const;
    Base::Vector3d projectPoint(Base::Vector3d pt) const;

protected:
    TechDrawGeometry::GeometryObject *geometryObject;
    Base::BoundBox3d bbox;

    void onChanged(const App::Property* prop);
    void buildGeometryObject(TopoDS_Shape shape, gp_Pnt& center);
    void extractFaces();

    bool isOnEdge(TopoDS_Edge e, TopoDS_Vertex v, bool allowEnds = false);
    std::vector<TopoDS_Edge> splitEdge(std::vector<TopoDS_Vertex> splitPoints, TopoDS_Edge e);
    double simpleMinDist(TopoDS_Shape s1, TopoDS_Shape s2);

    //Projection parameter space
    void saveParamSpace(Base::Vector3d direction,
                        Base::Vector3d xAxis);
    Base::Vector3d uDir;                       //paperspace X
    Base::Vector3d vDir;                       //paperspace Y
    Base::Vector3d wDir;                       //paperspace Z
    Base::Vector3d shapeCentroid;

private:
    static App::PropertyFloatConstraint::Constraints floatRange;

};

typedef App::FeaturePythonT<DrawViewPart> DrawViewPartPython;

} //namespace TechDraw

#endif  // #ifndef _DrawViewPart_h_
