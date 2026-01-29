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

#pragma once

#include <App/DocumentObject.h>
#include <Base/UnitsApi.h>
#include <Mod/Part/App/PropertyTopoShapeList.h>

#include <Mod/TechDraw/TechDrawGlobal.h>

#include "DimensionFormatter.h"
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
class GeometryMatcher;
class DimensionAutoCorrect;
using DF = DimensionFormatter;

//TODO: Cyclic dependency issue with DimensionFormatter

//NOLINTBEGIN
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
    App::PropertyBool UseActualArea;

    App::PropertyBool ShowUnits;
//NOLINTEND

    enum class RefType
    {
        invalidRef,
        oneEdge,
        twoEdge,
        twoVertex,
        vertexEdge,
        threeVertex,
        oneFace,
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

    virtual std::string getFormattedToleranceValue(DF::Format partial);
    virtual std::pair<std::string, std::string> getFormattedToleranceValues(DF::Format partial = DF::Format::UNALTERED);
    virtual std::string getFormattedDimensionValue(DF::Format partial = DF::Format::UNALTERED);
    virtual std::string formatValue(qreal value,
                                    const QString& qFormatSpec,
                                    DF::Format partial = DF::Format::UNALTERED,
                                    bool isDim = true);

    virtual bool haveTolerance();

    virtual double getDimValue();
    virtual double getTrueDimValue() const;
    virtual double getProjectedDimValue() const;

    QStringList getPrefixSuffixSpec(const QString& fSpec);

    virtual DrawViewPart* getViewPart() const;
    QRectF getRect() const override
    {
        return {0, 0, 1, 1};
    }                                // pretend dimensions always fit!
    virtual RefType getRefType() const;  // Vertex-Vertex, Edge, Edge-Edge
    static RefType
    getRefTypeSubElements(const std::vector<std::string>&);  // Vertex-Vertex, Edge, Edge-Edge

    void setReferences2d(const ReferenceVector& refs);
    void setReferences3d(const ReferenceVector& refs);
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


    virtual pointPair getLinearPoints() const;

    virtual void setLinearPoints(Base::Vector3d point0, Base::Vector3d point1)
    {
        m_linearPoints.first(point0);
        m_linearPoints.second(point1);
    };
    virtual void setLinearPoints(const pointPair& newPair)
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
    areaPoint getAreaPoint()
    {
        return m_areaPoint;
    }

    bool leaderIntersectsArc(Base::Vector3d s, Base::Vector3d pointOnCircle);

    bool isMultiValueSchema() const;

    pointPair getArrowPositions();
    void saveArrowPositions(const Base::Vector2d positions[]);  //NOLINT

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

    static std::vector<Part::TopoShape> getEdges(const Part::TopoShape& inShape);
    static std::vector<Part::TopoShape> getVertexes(const Part::TopoShape& inShape);
    static double getArcAngle(Base::Vector3d center, Base::Vector3d startPoint, Base::Vector3d endPoint);

    // autocorrect support methods
    void saveFeatureBox();
    Base::BoundBox3d getSavedBox() const;
    Base::BoundBox3d getFeatureBox() const;

    static double getActualArea(const TopoDS_Face& face);
    static double getFilledArea(const TopoDS_Face& face);
    static Base::Vector3d getFaceCenter(const TopoDS_Face& face);

protected:
    void handleChangedPropertyType(Base::XMLReader& reader, const char* typeName, App::Property* propss) override;
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
    virtual arcPoints arcPointsFromBaseGeom(const BaseGeomPtr& base);
    virtual arcPoints arcPointsFromEdge(const TopoDS_Edge& occEdge);

    virtual anglePoints getAnglePointsTwoEdges(ReferenceVector references);
    virtual anglePoints getAnglePointsThreeVerts(ReferenceVector references);

    virtual areaPoint getAreaParameters(ReferenceVector references);

    double
    dist2Segs(Base::Vector3d s1, Base::Vector3d e1, Base::Vector3d s2, Base::Vector3d e2) const;
    pointPair closestPoints(const TopoDS_Shape& s1, const TopoDS_Shape& s2) const;

    void resetLinear();
    void resetAngular();
    void resetArc();
    void resetArea();

    bool okToProceed();
    void updateSavedGeometry();

    bool validateReferenceForm() const;
    bool autocorrectReferences();

private:
    Measure::Measurement* measurement;
    static const char* TypeEnums[];         //NOLINT
    static const char* MeasureTypeEnums[];  //NOLINT
    void dumpRefs2D(const char* text) const;
    // Dimension "geometry"
    pointPair m_linearPoints;
    pointPair m_arrowPositions;
    arcPoints m_arcPoints;
    anglePoints m_anglePoints;
    areaPoint m_areaPoint;
    bool m_hasGeometry;

    friend class DimensionFormatter;
    DimensionFormatter* m_formatter;
    GeometryMatcher* m_matcher;
    DimensionAutoCorrect* m_corrector;

    bool m_referencesCorrect {false};

    std::set<std::string> m_3dObjectCache;
};

}  // namespace TechDraw