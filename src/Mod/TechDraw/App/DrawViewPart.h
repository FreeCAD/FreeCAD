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

#ifndef DrawViewPart_h_
#define DrawViewPart_h_

#include <Mod/TechDraw/TechDrawGlobal.h>

#include <QFuture>
#include <QFutureWatcher>
#include <QObject>

#include <TopoDS_Edge.hxx>
#include <TopoDS_Wire.hxx>

#include <App/DocumentObject.h>
#include <App/FeaturePython.h>
#include <App/PropertyLinks.h>
#include <App/PropertyUnits.h>
#include <Base/BoundBox.h>

#include "CosmeticExtension.h"
#include "DrawView.h"


class gp_Pnt;
class gp_Pln;
class gp_Ax2;
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
    DrawViewPart();
    ~DrawViewPart() override;

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

    short mustExecute() const override;
    void onDocumentRestored() override;
    App::DocumentObjectExecReturn *execute() override;
    const char* getViewProviderName() const override {
        return "TechDrawGui::ViewProviderViewPart";
    }
    PyObject *getPyObject() override;

    std::vector<TechDraw::DrawHatch*> getHatches() const;
    std::vector<TechDraw::DrawGeomHatch*> getGeomHatches() const;
    std::vector<TechDraw::DrawViewDimension*> getDimensions() const;
    std::vector<TechDraw::DrawViewBalloon*> getBalloons() const;

    const std::vector<TechDraw::VertexPtr> getVertexGeometry() const;
    const BaseGeomPtrVector getEdgeGeometry() const;
    const BaseGeomPtrVector getVisibleFaceEdges() const;
    const std::vector<TechDraw::FacePtr> getFaceGeometry() const;

    bool hasGeometry() const;
    TechDraw::GeometryObject* getGeometryObject() const { return geometryObject; }

    TechDraw::BaseGeomPtr getGeomByIndex(int idx) const;               //get existing geom for edge idx in projection
    TechDraw::VertexPtr getProjVertexByIndex(int idx) const;           //get existing geom for vertex idx in projection
    TechDraw::VertexPtr getProjVertexByCosTag(std::string cosTag);
    std::vector<TechDraw::BaseGeomPtr> getFaceEdgesByIndex(int idx) const;  //get edges for face idx in projection

    virtual Base::BoundBox3d getBoundingBox() const;
    double getBoxX() const;
    double getBoxY() const;
    QRectF getRect() const override;
    virtual std::vector<DrawViewSection*> getSectionRefs() const;       //are there ViewSections based on this ViewPart?
    virtual std::vector<DrawViewDetail*> getDetailRefs() const;


    virtual Base::Vector3d projectPoint(const Base::Vector3d& pt,
                                        bool invert = true) const;
    virtual BaseGeomPtr projectEdge(const TopoDS_Edge& e) const;

    virtual gp_Ax2 getViewAxis(const Base::Vector3d& pt,
                               const Base::Vector3d& direction,
                               const bool flip=true) const;
    virtual gp_Ax2 getProjectionCS(Base::Vector3d pt) const;
    virtual Base::Vector3d getXDirection() const;       //don't use XDirection.getValue()
    virtual Base::Vector3d getOriginalCentroid() const;
    virtual Base::Vector3d getCurrentCentroid() const;
    virtual Base::Vector3d getLegacyX(const Base::Vector3d& pt,
                                      const Base::Vector3d& axis,
                                      const bool flip = true)  const;


    bool handleFaces();

    bool isUnsetting() { return nowUnsetting; }

    virtual std::vector<TopoDS_Wire> getWireForFace(int idx) const;

    virtual TopoDS_Shape getSourceShape() const;
    virtual TopoDS_Shape getSourceShapeFused() const;
    virtual std::vector<TopoDS_Shape> getSourceShape2d() const;

    virtual void postHlrTasks(void);

    bool isIso() const;

    void clearCosmeticVertexes();
    void refreshCVGeoms();
    void addCosmeticVertexesToGeom();
    int add1CVToGV(std::string tag);
    int getCVIndex(std::string tag);

    void clearCosmeticEdges();
    void refreshCEGeoms();
    void addCosmeticEdgesToGeom();
    int add1CEToGE(std::string tag);

    void clearCenterLines();
    void refreshCLGeoms();
    void addCenterLinesToGeom();
    int add1CLToGE(std::string tag);

    void clearGeomFormats();

    void dumpVerts(const std::string text);
    void dumpCosVerts(const std::string text);
    void dumpCosEdges(const std::string text);

    std::string addReferenceVertex(Base::Vector3d v);
    void addReferencesToGeom();
    void removeReferenceVertex(std::string tag);
    void updateReferenceVert(std::string tag, Base::Vector3d loc2d);
    void removeAllReferencesFromGeom();
    void resetReferenceVerts();

    std::vector<App::DocumentObject*> getAllSources() const;

    bool waitingForFaces() const { return m_waitingForFaces; }
    void waitingForFaces(bool s) { m_waitingForFaces = s;}
    bool waitingForHlr() const { return m_waitingForHlr; }
    void waitingForHlr(bool s) { m_waitingForHlr = s; }
    virtual bool waitingForResult() const;

    void progressValueChanged(int v);

public Q_SLOTS:
    void onHlrFinished(void);
    void onFacesFinished(void);

protected:
    bool checkXDirection() const;

    TechDraw::GeometryObject *geometryObject;
    Base::BoundBox3d bbox;

    void onChanged(const App::Property* prop) override;
    void unsetupObject() override;

    virtual TechDraw::GeometryObject*  buildGeometryObject(TopoDS_Shape& shape, gp_Ax2& viewAxis);
    virtual TechDraw::GeometryObject*  makeGeometryForShape(TopoDS_Shape& shape);   //const??
    void partExec(TopoDS_Shape& shape);
    virtual void addShapes2d(void);

    void extractFaces();

    Base::Vector3d shapeCentroid;
    void getRunControl();

    bool m_handleFaces;

    TopoDS_Shape m_saveShape;    //TODO: make this a Property.  Part::TopoShapeProperty??
    Base::Vector3d m_saveCentroid;   //centroid before centering shape in origin

    void handleChangedPropertyName(Base::XMLReader &reader, const char* TypeName, const char* PropName) override;

    bool prefHardViz();
    bool prefSeamViz();
    bool prefSmoothViz();
    bool prefIsoViz();
    bool prefHardHid();
    bool prefSeamHid();
    bool prefSmoothHid();
    bool prefIsoHid();
    int  prefIsoCount();

    std::vector<TechDraw::VertexPtr> m_referenceVerts;

private:
    bool nowUnsetting;
    bool m_waitingForFaces;
    bool m_waitingForHlr;

    QMetaObject::Connection connectHlrWatcher;
    QFutureWatcher<void> m_hlrWatcher;
    QFuture<void> m_hlrFuture;
    QMetaObject::Connection connectFaceWatcher;
    QFutureWatcher<void> m_faceWatcher;
    QFuture<void> m_faceFuture;

};

typedef App::FeaturePythonT<DrawViewPart> DrawViewPartPython;

} //namespace TechDraw

#endif  // #ifndef DrawViewPart_h_
