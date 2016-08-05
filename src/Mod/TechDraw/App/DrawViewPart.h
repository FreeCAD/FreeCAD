/***************************************************************************
 *   Copyright (c) Jürgen Riegel          (juergen.riegel@web.de) 2007     *
 *   Copyright (c) Luke Parry             (l.parry@warwick.ac.uk) 2013     *
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
struct WalkerEdge;
}

namespace TechDraw
{

class TechDrawExport DrawViewPart : public DrawView
{
    PROPERTY_HEADER(TechDraw::DrawViewPart);

public:
    /// Constructor
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

protected:
    TechDrawGeometry::GeometryObject *geometryObject;
    Base::BoundBox3d bbox;

    void onChanged(const App::Property* prop);
    Base::Vector3d getValidXDir() const;
    void buildGeometryObject(TopoDS_Shape shape, gp_Pnt& center);
    void extractFaces();
    std::vector<TopoDS_Wire> sortWiresBySize(std::vector<TopoDS_Wire>& w, bool reverse = false);
    class wireCompare;

    bool isOnEdge(TopoDS_Edge e, TopoDS_Vertex v, bool allowEnds = false);
    std::vector<TopoDS_Edge> splitEdge(std::vector<TopoDS_Vertex> splitPoints, TopoDS_Edge e);
    double simpleMinDist(TopoDS_Shape s1, TopoDS_Shape s2);
    bool isSamePoint(TopoDS_Vertex v1, TopoDS_Vertex v2);
    int findUniqueVert(TopoDS_Vertex vx, std::vector<TopoDS_Vertex> &uniqueVert);
    std::vector<TopoDS_Vertex> makeUniqueVList(std::vector<TopoDS_Edge> edges);
    std::vector<WalkerEdge>    makeWalkerEdges(std::vector<TopoDS_Edge> edges,
                                               std::vector<TopoDS_Vertex> verts);
    TopoDS_Wire makeCleanWire(std::vector<TopoDS_Edge> edges, double tol = 0.10);


private:
    static App::PropertyFloatConstraint::Constraints floatRange;

};

typedef App::FeaturePythonT<DrawViewPart> DrawViewPartPython;

} //namespace TechDraw

#endif  // #ifndef _DrawViewPart_h_
