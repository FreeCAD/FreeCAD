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

#ifndef TechDraw_DrawViewDimension_h_
#define TechDraw_DrawViewDimension_h_

#include <App/DocumentObject.h>
#include <Base/UnitsApi.h>
#include <Mod/Part/App/PropertyTopoShapeList.h>

#include <Mod/TechDraw/TechDrawGlobal.h>

#include "DimensionGeometry.h"
#include "DimensionReferences.h"
#include "DrawUtil.h"
#include "DrawView.h"
#include "DrawViewPart.h"

class TopoDS_Shape;

namespace Measure
{
class Measurement;
}
namespace TechDraw
{
class DrawViewPart;
class DimensionFormatter;
class GeometryMatcher;
class DimensionAutoCorrect;

class TechDrawExport DrawViewDimension: public TechDraw::DrawView
{
    PROPERTY_HEADER_WITH_OVERRIDE(TechDraw::DrawViewDimension);

public:
    // keep this enum synchronized with TypeEnums
    enum DimensionType
    {
        Distance,
        DistanceX,
        DistanceY,
        DistanceZ,
        Radius,
        Diameter,
        Angle,
        Angle3Pt
    };

    /// Constructor
    DrawViewDimension();
    ~DrawViewDimension() override;

    App::PropertyEnumeration MeasureType;   // True/Projected
    App::PropertyLinkSubList References2D;  // Points to Projection SubFeatures
    App::PropertyLinkSubList References3D;  // Points to 3D Geometry SubFeatures
    App::PropertyEnumeration Type;          // DistanceX, DistanceY, Diameter, etc.

    App::PropertyBool TheoreticalExact;
    App::PropertyBool Inverted;
    App::PropertyString FormatSpec;
    App::PropertyString FormatSpecOverTolerance;
    App::PropertyString FormatSpecUnderTolerance;
    App::PropertyBool Arbitrary;
    App::PropertyBool ArbitraryTolerances;
    App::PropertyBool EqualTolerance;
    App::PropertyQuantityConstraint OverTolerance;
    App::PropertyQuantityConstraint UnderTolerance;

    App::PropertyBool AngleOverride;
    App::PropertyAngle LineAngle;
    App::PropertyAngle ExtensionAngle;

    Part::PropertyTopoShapeList SavedGeometry;
    App::PropertyVectorList BoxCorners;

    enum RefType
    {
        invalidRef,
        oneEdge,
        twoEdge,
        twoVertex,
        vertexEdge,
        threeVertex,
        extent
    };


    short mustExecute() const override;
    virtual bool has2DReferences() const;
    virtual bool has3DReferences() const;
    bool hasOverUnderTolerance() const;

    App::DocumentObjectExecReturn* execute() override;

    const char* getViewProviderName() const override
    {
        return "TechDrawGui::ViewProviderDimension";
    }
    // return PyObject as DrawViewDimensionPy
    PyObject* getPyObject() override;

    virtual std::string getFormattedToleranceValue(int partial);
    virtual std::pair<std::string, std::string> getFormattedToleranceValues(int partial = 0);
    virtual std::string getFormattedDimensionValue(int partial = 0);
    virtual std::string
    formatValue(qreal value, QString qFormatSpec, int partial = 0, bool isDim = true);

    virtual bool haveTolerance();

    virtual double getDimValue();
    QStringList getPrefixSuffixSpec(QString fSpec);

    virtual DrawViewPart* getViewPart() const;
    QRectF getRect() const override
    {
        return {0, 0, 1, 1};
    }                                // pretend dimensions always fit!
    virtual int getRefType() const;  // Vertex-Vertex, Edge, Edge-Edge
    static int
    getRefTypeSubElements(const std::vector<std::string>&);  // Vertex-Vertex, Edge, Edge-Edge

    void setReferences2d(ReferenceVector refs);
    void setReferences3d(ReferenceVector refs);
    ReferenceVector getReferences2d() const;
    ReferenceVector getReferences3d() const;
    bool hasGoodReferences() const
    {
        return m_referencesCorrect;
    }

    void setAll3DMeasurement();
    void clear3DMeasurements();
    virtual bool checkReferences2D() const;
    bool hasBroken3dReferences() const;


    virtual pointPair getLinearPoints() const
    {
        return m_linearPoints;
    }
    virtual void setLinearPoints(Base::Vector3d point0, Base::Vector3d point1)
    {
        m_linearPoints.first(point0);
        m_linearPoints.second(point1);
    };
    virtual void setLinearPoints(pointPair newPair)
    {
        m_linearPoints = newPair;
    }
    arcPoints getArcPoints()
    {
        return m_arcPoints;
    }
    anglePoints getAnglePoints()
    {
        return m_anglePoints;
    }

    bool leaderIntersectsArc(Base::Vector3d s, Base::Vector3d pointOnCircle);

    bool isMultiValueSchema() const;

    pointPair getArrowPositions();
    void saveArrowPositions(const Base::Vector2d positions[]);

    bool showUnits() const;
    bool useDecimals() const;
    bool isExtentDim() const;
    virtual ReferenceVector getEffectiveReferences() const;

    GeometryMatcher* getMatcher() const
    {
        return m_matcher;
    }
    DimensionAutoCorrect* getCorrector() const
    {
        return m_corrector;
    }

    // these should probably be static as they don't use the dimension at all
    std::vector<Part::TopoShape> getEdges(const Part::TopoShape& inShape);
    std::vector<Part::TopoShape> getVertexes(const Part::TopoShape& inShape);

    // autocorrect support methods
    void saveFeatureBox();
    Base::BoundBox3d getSavedBox();
    Base::BoundBox3d getFeatureBox();

protected:
    void handleChangedPropertyType(Base::XMLReader&, const char*, App::Property*) override;
    void Restore(Base::XMLReader& reader) override;
    void onChanged(const App::Property* prop) override;
    void onDocumentRestored() override;
    std::string getPrefixForDimType() const;
    std::string getDefaultFormatSpec(bool isToleranceFormat = false) const;
    virtual pointPair getPointsOneEdge(ReferenceVector references);
    virtual pointPair getPointsTwoEdges(ReferenceVector references);
    virtual pointPair getPointsTwoVerts(ReferenceVector references);
    virtual pointPair getPointsEdgeVert(ReferenceVector references);

    virtual arcPoints getArcParameters(ReferenceVector references);
    virtual arcPoints arcPointsFromBaseGeom(BaseGeomPtr base);
    virtual arcPoints arcPointsFromEdge(TopoDS_Edge occEdge);

    virtual anglePoints getAnglePointsTwoEdges(ReferenceVector references);
    virtual anglePoints getAnglePointsThreeVerts(ReferenceVector references);

    Measure::Measurement* measurement;
    double
    dist2Segs(Base::Vector3d s1, Base::Vector3d e1, Base::Vector3d s2, Base::Vector3d e2) const;
    pointPair closestPoints(TopoDS_Shape s1, TopoDS_Shape s2) const;

    void resetLinear();
    void resetAngular();
    void resetArc();

    bool okToProceed();
    void updateSavedGeometry();

private:
    static const char* TypeEnums[];
    static const char* MeasureTypeEnums[];
    void dumpRefs2D(const char* text) const;
    // Dimension "geometry"
    pointPair m_linearPoints;
    pointPair m_arrowPositions;
    arcPoints m_arcPoints;
    anglePoints m_anglePoints;
    bool m_hasGeometry;

    friend class DimensionFormatter;
    DimensionFormatter* m_formatter;
    GeometryMatcher* m_matcher;
    DimensionAutoCorrect* m_corrector;

    bool m_referencesCorrect {false};

    std::set<std::string> m_3dObjectCache;
};

}  // namespace TechDraw
#endif
