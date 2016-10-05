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
#include <App/PropertyStandard.h>
#include "DrawView.h"
#include <App/FeaturePython.h>

#include <Base/BoundBox.h>

//#include "GeometryObject.h"

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
struct splitPoint {
    int i;
    Base::Vector3d v;
    double param;
};
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
    App::PropertyBool   SeamVisible;
    App::PropertyBool   SmoothVisible;
    //App::PropertyBool   OutlinesVisible;
    App::PropertyBool   IsoVisible;

    App::PropertyBool   HardHidden;
    App::PropertyBool   SmoothHidden;
    App::PropertyBool   SeamHidden;
    //App::PropertyBool   OutlinesHidden;
    App::PropertyBool   IsoHidden;
    App::PropertyInteger  IsoCount;

    App::PropertyFloat  LineWidth;
    App::PropertyFloat  HiddenWidth;
    App::PropertyFloat  IsoWidth;
    App::PropertyBool   ArcCenterMarks;
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
    const std::vector<TechDrawGeometry::BaseGeom  *> getVisibleFaceEdges() const;
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
    const Base::Vector3d& getUDir(void) const {return uDir;}                       //paperspace X
    const Base::Vector3d& getVDir(void) const {return vDir;}                       //paperspace Y
    const Base::Vector3d& getWDir(void) const {return wDir;}                       //paperspace Z
    const Base::Vector3d& getCentroid(void) const {return shapeCentroid;}
    Base::Vector3d getValidXDir() const;
    Base::Vector3d projectPoint(const Base::Vector3d& pt) const;

    virtual short mustExecute() const;

    bool handleFaces(void);
    bool showSectionEdges(void);

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

protected:
    TechDrawGeometry::GeometryObject *geometryObject;
    Base::BoundBox3d bbox;

    void onChanged(const App::Property* prop);
    void buildGeometryObject(TopoDS_Shape shape, gp_Pnt& center);
    void extractFaces();

    bool isOnEdge(TopoDS_Edge e, TopoDS_Vertex v, double& param, bool allowEnds = false);
    std::vector<TopoDS_Edge> splitEdges(std::vector<TopoDS_Edge> orig, std::vector<splitPoint> splits);
    std::vector<TopoDS_Edge> split1Edge(TopoDS_Edge e, std::vector<splitPoint> splitPoints);
    double simpleMinDist(TopoDS_Shape s1, TopoDS_Shape s2); //const;   //probably sb static or DrawUtil

    //Projection parameter space
    void saveParamSpace(const Base::Vector3d& direction,
                        const Base::Vector3d& xAxis);
    Base::Vector3d uDir;                       //paperspace X
    Base::Vector3d vDir;                       //paperspace Y
    Base::Vector3d wDir;                       //paperspace Z
    Base::Vector3d shapeCentroid;
    std::vector<splitPoint> sortSplits(std::vector<splitPoint>& s, bool ascend);
    static bool splitCompare(const splitPoint& p1, const splitPoint& p2);
    static bool splitEqual(const splitPoint& p1, const splitPoint& p2);
    void getRunControl(void);

    long int m_interAlgo;
    bool m_sectionEdges;
    bool m_handleFaces;

private:
    static App::PropertyFloatConstraint::Constraints floatRange;

};

typedef App::FeaturePythonT<DrawViewPart> DrawViewPartPython;

} //namespace TechDraw

#endif  // #ifndef _DrawViewPart_h_
