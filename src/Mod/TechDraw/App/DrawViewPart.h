/***************************************************************************
 *   Copyright (c) 2007 Jürgen Riegel <juergen.riegel@web.de>              *
 *   Copyright (c) 2013 Luke Parry <l.parry@warwick.ac.uk>                 *
 *   Copyright (c) 2016 WandererFan <wandererfan@gmail.com>                *
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
#include "CosmeticExtension.h"
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

class TechDrawExport DrawViewPart : public DrawView, public CosmeticExtension
{
    PROPERTY_HEADER_WITH_EXTENSIONS(TechDraw::DrawViewPart);

public:
    DrawViewPart(void);
    virtual ~DrawViewPart();

    App::PropertyLinkList     Source;
    App::PropertyXLinkList    XSource;
    App::PropertyVector       Direction;  //TODO: Rename to YAxisDirection or whatever this actually is  (ProjectionDirection)
    App::PropertyVector       XDirection;
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

    const std::vector<TechDraw::VertexPtr> getVertexGeometry() const;
    const std::vector<TechDraw::BaseGeom*> getEdgeGeometry() const;
    const std::vector<TechDraw::BaseGeom*> getVisibleFaceEdges() const;
    const std::vector<TechDraw::FacePtr> getFaceGeometry() const;

    bool hasGeometry(void) const;
    TechDraw::GeometryObject* getGeometryObject(void) const { return geometryObject; }

    TechDraw::BaseGeom* getGeomByIndex(int idx) const;               //get existing geom for edge idx in projection
    TechDraw::VertexPtr getProjVertexByIndex(int idx) const;           //get existing geom for vertex idx in projection
    TechDraw::VertexPtr getProjVertexByCosTag(std::string cosTag);
    std::vector<TechDraw::BaseGeom*> getFaceEdgesByIndex(int idx) const;  //get edges for face idx in projection

    virtual Base::BoundBox3d getBoundingBox() const;
    double getBoxX(void) const;
    double getBoxY(void) const;
    virtual QRectF getRect() const override;
    virtual std::vector<DrawViewSection*> getSectionRefs() const;       //are there ViewSections based on this ViewPart?
    virtual std::vector<DrawViewDetail*> getDetailRefs() const;


    virtual Base::Vector3d projectPoint(const Base::Vector3d& pt,
                                        bool invert = true) const;
    virtual BaseGeom* projectEdge(const TopoDS_Edge& e) const;

    virtual gp_Ax2 getViewAxis(const Base::Vector3d& pt,
                               const Base::Vector3d& direction,
                               const bool flip=true) const;
    virtual gp_Ax2 getProjectionCS(Base::Vector3d pt) const;
    virtual Base::Vector3d getXDirection(void) const;       //don't use XDirection.getValue()
    virtual Base::Vector3d getOriginalCentroid(void) const;
    virtual Base::Vector3d getLegacyX(const Base::Vector3d& pt,
                                      const Base::Vector3d& axis,
                                      const bool flip = true)  const;


    bool handleFaces(void);

    bool isUnsetting(void) { return nowUnsetting; }
    
    virtual std::vector<TopoDS_Wire> getWireForFace(int idx) const;

    virtual TopoDS_Shape getSourceShape(void) const; 
    virtual TopoDS_Shape getSourceShapeFused(void) const; 
    virtual std::vector<TopoDS_Shape> getSourceShape2d(void) const;


    bool isIso(void) const;

    void clearCosmeticVertexes(void); 
    void refreshCVGeoms(void);
    void addCosmeticVertexesToGeom(void);
    int add1CVToGV(std::string tag);
    int getCVIndex(std::string tag);

    void clearCosmeticEdges(void); 
    void refreshCEGeoms(void);
    void addCosmeticEdgesToGeom(void);
    int add1CEToGE(std::string tag);

    void clearCenterLines(void); 
    void refreshCLGeoms(void);
    void addCenterLinesToGeom(void);
    int add1CLToGE(std::string tag);

    void clearGeomFormats(void);

    void dumpVerts(const std::string text);
    void dumpCosVerts(const std::string text);
    void dumpCosEdges(const std::string text);

    std::string addReferenceVertex(Base::Vector3d v);
    void addReferencesToGeom(void);
    void removeReferenceVertex(std::string tag);
    void updateReferenceVert(std::string tag, Base::Vector3d loc2d);
    void removeAllReferencesFromGeom();
    void resetReferenceVerts();

    std::vector<App::DocumentObject*> getAllSources(void) const;


protected:
    bool checkXDirection(void) const;

    TechDraw::GeometryObject *geometryObject;
    Base::BoundBox3d bbox;

    virtual void onChanged(const App::Property* prop) override;
    virtual void unsetupObject() override;

    virtual TechDraw::GeometryObject*  buildGeometryObject(TopoDS_Shape shape, gp_Ax2 viewAxis); //const??
    virtual TechDraw::GeometryObject*  makeGeometryForShape(TopoDS_Shape shape);   //const??
    void partExec(TopoDS_Shape shape);
    virtual void addShapes2d(void);

    void extractFaces();

    Base::Vector3d shapeCentroid;
    void getRunControl(void);

    bool m_handleFaces;

    TopoDS_Shape m_saveShape;    //TODO: make this a Property.  Part::TopoShapeProperty??
    Base::Vector3d m_saveCentroid;   //centroid before centering shape in origin

    void handleChangedPropertyName(Base::XMLReader &reader, const char* TypeName, const char* PropName) override;

    bool prefHardViz(void);
    bool prefSeamViz(void);
    bool prefSmoothViz(void);
    bool prefIsoViz(void);
    bool prefHardHid(void);
    bool prefSeamHid(void);
    bool prefSmoothHid(void);
    bool prefIsoHid(void);
    int  prefIsoCount(void);

    std::vector<TechDraw::VertexPtr> m_referenceVerts;

private:
    bool nowUnsetting;

};

typedef App::FeaturePythonT<DrawViewPart> DrawViewPartPython;

} //namespace TechDraw

#endif  // #ifndef _DrawViewPart_h_
