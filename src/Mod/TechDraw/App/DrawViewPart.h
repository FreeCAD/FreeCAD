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
#include <App/PropertyUnits.h>
#include <App/FeaturePython.h>

#include <Base/BoundBox.h>

#include "PropertyGeomFormatList.h"
#include "PropertyCenterLineList.h"
#include "PropertyCosmeticEdgeList.h"
#include "PropertyCosmeticVertexList.h"
#include "DrawView.h"

class gp_Pnt;
class gp_Pln;
class gp_Ax2;
//class TopoDS_Edge;
//class TopoDS_Vertex;
//class TopoDS_Wire;
class TopoDS_Shape;

namespace App
{
class Part;
}

namespace TechDraw
{
class GeometryObject;
class Vertex;
class BaseGeom;
class Face;
}

namespace TechDraw {
class DrawHatch;
class DrawGeomHatch;
class DrawViewDimension;
class DrawProjectSplit;
class DrawViewSection;
class DrawViewDetail;
class DrawViewBalloon;
class CosmeticVertex;
class CosmeticEdge;
class CenterLine;
class GeomFormat;
}

namespace TechDraw
{

class DrawViewSection;

class TechDrawExport DrawViewPart : public DrawView
{
    PROPERTY_HEADER_WITH_OVERRIDE(TechDraw::DrawViewPart);

public:
    DrawViewPart(void);
    virtual ~DrawViewPart();

    App::PropertyLinkList     Source;
    App::PropertyVector       Direction;  //TODO: Rename to YAxisDirection or whatever this actually is  (ProjectionDirection)
    App::PropertyBool         Perspective;
    App::PropertyDistance     Focus;

    App::PropertyBool   CoarseView;
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

    TechDraw::PropertyCosmeticVertexList CosmeticVertexes;
    TechDraw::PropertyCosmeticEdgeList CosmeticEdges;
    TechDraw::PropertyCenterLineList  CenterLines;
    TechDraw::PropertyGeomFormatList  GeomFormats;

    virtual short mustExecute() const override;
    virtual void onDocumentRestored() override;
    virtual App::DocumentObjectExecReturn *execute(void) override;
    virtual const char* getViewProviderName(void) const override {
        return "TechDrawGui::ViewProviderViewPart";
    }
    virtual PyObject *getPyObject(void) override;

    std::vector<TechDraw::DrawHatch*> getHatches(void) const;
    std::vector<TechDraw::DrawGeomHatch*> getGeomHatches(void) const;
    std::vector<TechDraw::DrawViewDimension*> getDimensions() const;
    std::vector<TechDraw::DrawViewBalloon*> getBalloons() const;

    const std::vector<TechDraw::Vertex *> getVertexGeometry() const;
    const std::vector<TechDraw::BaseGeom  *> & getEdgeGeometry() const;
    const std::vector<TechDraw::BaseGeom  *> getVisibleFaceEdges() const;
    const std::vector<TechDraw::Face *> & getFaceGeometry() const;

    bool hasGeometry(void) const;
    TechDraw::GeometryObject* getGeometryObject(void) const { return geometryObject; }

    TechDraw::BaseGeom* getGeomByIndex(int idx) const;               //get existing geom for edge idx in projection
    TechDraw::Vertex* getProjVertexByIndex(int idx) const;           //get existing geom for vertex idx in projection
    TechDraw::Vertex* getProjVertexByCosTag(std::string cosTag);
    std::vector<TechDraw::BaseGeom*> getFaceEdgesByIndex(int idx) const;  //get edges for face idx in projection

    virtual Base::BoundBox3d getBoundingBox() const;
    double getBoxX(void) const;
    double getBoxY(void) const;
    virtual QRectF getRect() const override;
    virtual std::vector<DrawViewSection*> getSectionRefs() const;                    //are there ViewSections based on this ViewPart?
    virtual std::vector<DrawViewDetail*> getDetailRefs() const;
    const Base::Vector3d& getUDir(void) const {return uDir;}                       //paperspace X
    const Base::Vector3d& getVDir(void) const {return vDir;}                       //paperspace Y
    const Base::Vector3d& getWDir(void) const {return wDir;}                       //paperspace Z
    virtual const Base::Vector3d& getCentroid(void) const {return shapeCentroid;}
    Base::Vector3d projectPoint(const Base::Vector3d& pt) const;
    virtual gp_Ax2 getViewAxis(const Base::Vector3d& pt,
                               const Base::Vector3d& direction,
                               const bool flip=true) const;

    bool handleFaces(void);
    bool showSectionEdges(void);

    bool isUnsetting(void) { return nowUnsetting; }
    
    gp_Pln getProjPlane(void) const;
    virtual std::vector<TopoDS_Wire> getWireForFace(int idx) const;

    virtual TopoDS_Shape getSourceShape(void) const; 
/*    virtual std::vector<TopoDS_Shape> getShapesFromObject(App::DocumentObject* docObj) const; */
    virtual TopoDS_Shape getSourceShapeFused(void) const; 
/*    std::vector<TopoDS_Shape> extractDrawableShapes(TopoDS_Shape shapeIn) const;*/

    bool isIso(void) const;

    virtual int addCosmeticVertex(Base::Vector3d pos);
    virtual int addCosmeticVertex(CosmeticVertex* cv);
    std::string addCosmeticVertexSS(Base::Vector3d pos);
    virtual void removeCosmeticVertex(TechDraw::CosmeticVertex* cv);
    virtual void removeCosmeticVertex(int idx);
    virtual void removeCosmeticVertex(std::string tagString);
    virtual void removeCosmeticVertex(std::vector<std::string> delTags);

    int getCosmeticVertexIndex(std::string tagString);
    TechDraw::CosmeticVertex* getCosmeticVertex(std::string tagString) const;
    TechDraw::CosmeticVertex* getCosmeticVertexByIndex(int idx) const;
    TechDraw::CosmeticVertex* getCosmeticVertexByGeom(int idx) const;
    void clearCosmeticVertexes(void); 
    void addCosmeticVertexesToGeom(void);
    void add1CosmeticVertexToGeom(int iCV);
    int add1CVToGV(int iCV);
    int add1CVToGV(std::string tag);

    virtual int addCosmeticEdge(Base::Vector3d start, Base::Vector3d end);
    virtual int addCosmeticEdge(TopoDS_Edge e);
    virtual int addCosmeticEdge(TechDraw::CosmeticEdge*);
    virtual void removeCosmeticEdge(TechDraw::CosmeticEdge* ce);
    virtual void removeCosmeticEdge(int idx);
    virtual void removeCosmeticEdge(std::string delTag);
    virtual void removeCosmeticEdge(std::vector<std::string> delTags);
    TechDraw::CosmeticEdge* getCosmeticEdge(std::string tagString) const;
    TechDraw::CosmeticEdge* getCosmeticEdgeByIndex(int idx) const;
    TechDraw::CosmeticEdge* getCosmeticEdgeByGeom(int idx) const;
    int getCosmeticEdgeIndex(TechDraw::CosmeticEdge* ce) const;
    void clearCosmeticEdges(void);
    void addCosmeticEdgesToGeom(void);

    virtual int addCenterLine(TechDraw::CenterLine*);
    virtual void removeCenterLine(TechDraw::CenterLine* cl);
    virtual void removeCenterLine(int idx);
    void removeCenterLine(std::string delTag);
    void removeCenterLine(std::vector<std::string> delTags);
    TechDraw::CenterLine* getCenterLineByIndex(int idx) const;
    TechDraw::CenterLine* getCenterLineByGeom(int idx) const;
    void replaceCenterLine(int idx, TechDraw::CenterLine* cl);
    void replaceCenterLineByGeom(int geomIndex, TechDraw::CenterLine* cl);
    void clearCenterLines(void);
    void addCenterLinesToGeom(void);

    int addGeomFormat(TechDraw::GeomFormat* gf);
    virtual void removeGeomFormat(int idx);
    TechDraw::GeomFormat* getGeomFormatByIndex(int idx) const;
    TechDraw::GeomFormat* getGeomFormatByGeom(int idx) const;
    void clearGeomFormats(void);

    void dumpVerts(const std::string text);
    void dumpCosVerts(const std::string text);

protected:
    TechDraw::GeometryObject *geometryObject;
    Base::BoundBox3d bbox;

    virtual void onChanged(const App::Property* prop) override;
    virtual void unsetupObject() override;

    virtual TechDraw::GeometryObject*  buildGeometryObject(TopoDS_Shape shape, gp_Ax2 viewAxis);
    void extractFaces();

    //Projection parameter space
    virtual void saveParamSpace(const Base::Vector3d& direction, const Base::Vector3d& xAxis=Base::Vector3d(0.0,0.0,0.0));
    Base::Vector3d uDir;                       //paperspace X
    Base::Vector3d vDir;                       //paperspace Y
    Base::Vector3d wDir;                       //paperspace Z
    Base::Vector3d shapeCentroid;
    void getRunControl(void);
    
    bool m_sectionEdges;
    bool m_handleFaces;

private:
    bool nowUnsetting;

};

typedef App::FeaturePythonT<DrawViewPart> DrawViewPartPython;

} //namespace TechDraw

#endif  // #ifndef _DrawViewPart_h_
