/***************************************************************************
 *   Copyright (c) 2013 Luke Parry <l.parry@warwick.ac.uk>                 *
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

#ifndef _TechDraw_DrawViewDimension_h_
#define _TechDraw_DrawViewDimension_h_
#include <tuple>

# include <App/DocumentObject.h>
# include <App/FeaturePython.h>
# include <App/PropertyLinks.h>

#include "DrawView.h"

class TopoDS_Shape;

namespace Measure {
class Measurement;
}
namespace TechDraw
{
class DrawViewPart;

struct DimRef {
    DrawViewPart* part;
    std::string   sub;
};

typedef std::pair<Base::Vector3d,Base::Vector3d> pointPair;

struct anglePoints
{
    pointPair ends;
    Base::Vector3d vertex;

    anglePoints()
    {
        ends.first  = Base::Vector3d(0.0,0.0,0.0);
        ends.second = Base::Vector3d(0.0,0.0,0.0);
        vertex      = Base::Vector3d(0.0,0.0,0.0);
    }
};

struct arcPoints
{
    bool isArc;
    double radius;
    Base::Vector3d center;
    pointPair onCurve;
    pointPair arcEnds;
    Base::Vector3d midArc;
    bool arcCW;

    arcPoints() 
    {
         isArc = false;
         radius = 0.0;
         center         = Base::Vector3d(0.0,0.0,0.0);
         onCurve.first  = Base::Vector3d(0.0,0.0,0.0);
         onCurve.second = Base::Vector3d(0.0,0.0,0.0);
         arcEnds.first  = Base::Vector3d(0.0,0.0,0.0);
         arcEnds.second = Base::Vector3d(0.0,0.0,0.0);
         midArc         = Base::Vector3d(0.0,0.0,0.0);
         arcCW = false;
    }

};

class TechDrawExport DrawViewDimension : public TechDraw::DrawView
{
    PROPERTY_HEADER_WITH_OVERRIDE(TechDraw::DrawViewDimension);

public:
    /// Constructor
    DrawViewDimension();
    virtual ~DrawViewDimension();

    App::PropertyEnumeration       MeasureType;                        //True/Projected
    App::PropertyLinkSubList       References2D;                       //Points to Projection SubFeatures
    App::PropertyLinkSubList       References3D;                       //Points to 3D Geometry SubFeatures
    App::PropertyEnumeration       Type;                               //DistanceX,DistanceY,Diameter, etc

    App::PropertyBool              TheoreticalExact;
    App::PropertyBool              Inverted;
    App::PropertyString            FormatSpec;
    App::PropertyBool              Arbitrary;
    App::PropertyFloat             OverTolerance;
    App::PropertyFloat             UnderTolerance;

    enum RefType{
            invalidRef,
            oneEdge,
            twoEdge,
            twoVertex,
            vertexEdge,
            threeVertex
        };


    short mustExecute() const override;
    virtual bool has2DReferences(void) const;
    virtual bool has3DReferences(void) const;
    bool hasTolerance(void) const;

    /** @name methods override Feature */
    //@{
    /// recalculate the Feature
    virtual App::DocumentObjectExecReturn *execute(void) override;
    //@}

    /// returns the type name of the ViewProvider
    virtual const char* getViewProviderName(void) const override {
        return "TechDrawGui::ViewProviderDimension";
    }
    //return PyObject as DrawViewDimensionPy
    virtual PyObject *getPyObject(void) override;

    virtual std::string getFormatedValue(int partial = 0);
    virtual double getDimValue();
    virtual DrawViewPart* getViewPart() const;
    virtual QRectF getRect() const override { return QRectF(0,0,1,1);}          //pretend dimensions always fit!
    static int getRefType1(const std::string s);
    static int getRefType2(const std::string s1, const std::string s2);
    static int getRefType3(const std::string g1,
                           const std::string g2,
                           const std::string g3);
    virtual int getRefType() const;                                   //Vertex-Vertex, Edge, Edge-Edge
    void setAll3DMeasurement();
    void clear3DMeasurements(void);
    virtual bool checkReferences2D(void) const;
    pointPair getLinearPoints(void) {return m_linearPoints; }
    arcPoints getArcPoints(void) {return m_arcPoints; }
    anglePoints getAnglePoints(void) {return m_anglePoints; }
    bool leaderIntersectsArc(Base::Vector3d s, Base::Vector3d pointOnCircle);

    bool isMultiValueSchema(void) const;

    pointPair getArrowPositions(void);
    void saveArrowPositions(const Base::Vector2d positions[]);

protected:
    virtual void onChanged(const App::Property* prop) override;
    virtual void onDocumentRestored() override;
    bool showUnits() const;
    bool useDecimals() const;
    std::string getPrefix() const;
    std::string getDefaultFormatSpec() const;
    virtual pointPair getPointsOneEdge();
    virtual pointPair getPointsTwoEdges();
    virtual pointPair getPointsTwoVerts();
    virtual pointPair getPointsEdgeVert();

protected:
    Measure::Measurement *measurement;
    double dist2Segs(Base::Vector3d s1,
                     Base::Vector3d e1,
                     Base::Vector3d s2,
                     Base::Vector3d e2) const;
    pointPair closestPoints(TopoDS_Shape s1,
                            TopoDS_Shape s2) const;
    pointPair   m_linearPoints;
    pointPair   m_arrowPositions;

private:
    static const char* TypeEnums[];
    static const char* MeasureTypeEnums[];
    void dumpRefs2D(const char* text) const;
    //Dimension "geometry"
/*    pointPair   m_linearPoints;*/
    arcPoints   m_arcPoints;
    anglePoints m_anglePoints;
    bool        m_hasGeometry;
};

} //namespace TechDraw
#endif
