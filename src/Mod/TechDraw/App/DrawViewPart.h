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

namespace TechDrawGeometry
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

    App::PropertyStringList  CosmeticVertexList;

    std::vector<TechDraw::DrawHatch*> getHatches(void) const;
    std::vector<TechDraw::DrawGeomHatch*> getGeomHatches(void) const;
    std::vector<TechDraw::DrawViewDimension*> getDimensions() const;
    std::vector<TechDraw::DrawViewBalloon*> getBalloons() const;

    //TODO: are there use-cases for Python access to TechDrawGeometry???

    const std::vector<TechDrawGeometry::Vertex *> & getVertexGeometry() const;
    const std::vector<TechDrawGeometry::BaseGeom  *> & getEdgeGeometry() const;
    const std::vector<TechDrawGeometry::BaseGeom  *> getVisibleFaceEdges() const;
    const std::vector<TechDrawGeometry::Face *> & getFaceGeometry() const;

    bool hasGeometry(void) const;
    TechDrawGeometry::GeometryObject* getGeometryObject(void) const { return geometryObject; }

    TechDrawGeometry::BaseGeom* getProjEdgeByIndex(int idx) const;               //get existing geom for edge idx in projection
    TechDrawGeometry::Vertex* getProjVertexByIndex(int idx) const;               //get existing geom for vertex idx in projection
    std::vector<TechDrawGeometry::BaseGeom*> getProjFaceByIndex(int idx) const;  //get edges for face idx in projection

    virtual Base::BoundBox3d getBoundingBox() const;
    double getBoxX(void) const;
    double getBoxY(void) const;
    virtual QRectF getRect() const;
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

    virtual short mustExecute() const;
//    virtual void onDocumentRestored() override;

    bool handleFaces(void);
    bool showSectionEdges(void);

    /** @name methods override Feature */
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
    bool isUnsetting(void) { return nowUnsetting; }
    
    gp_Pln getProjPlane(void) const;
    virtual std::vector<TopoDS_Wire> getWireForFace(int idx) const;
    virtual TopoDS_Shape getSourceShape(void) const; 
    virtual std::vector<TopoDS_Shape> getShapesFromObject(App::DocumentObject* docObj) const; 
    virtual TopoDS_Shape getSourceShapeFused(void) const; 
    bool isIso(void) const;

    virtual int addRandomVertex(Base::Vector3d pos);
    const std::vector<TechDraw::CosmeticVertex*> & getCosmeticVertex(void) const { return cosmoVertex; }
    TechDraw::CosmeticVertex* getCosmeticVertexByIndex(int idx) const;
    TechDraw::CosmeticVertex* getCosmeticVertexByLink(int idx) const;
    void clearCV(void);

protected:
    TechDrawGeometry::GeometryObject *geometryObject;
    Base::BoundBox3d bbox;

    void onChanged(const App::Property* prop);
    virtual void unsetupObject();

    virtual TechDrawGeometry::GeometryObject*  buildGeometryObject(TopoDS_Shape shape, gp_Ax2 viewAxis);
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

    //Cosmetics
    std::vector<TechDraw::CosmeticVertex*> cosmoVertex;
    void rebuildCosmoVertex(void);

private:
    bool nowUnsetting;
    bool on1;
/*    bool m_restoreComplete;*/

};

typedef App::FeaturePythonT<DrawViewPart> DrawViewPartPython;

} //namespace TechDraw

#endif  // #ifndef _DrawViewPart_h_
