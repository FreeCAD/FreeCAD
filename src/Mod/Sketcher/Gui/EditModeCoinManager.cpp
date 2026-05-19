// SPDX-License-Identifier: LGPL-2.1-or-later

/***************************************************************************
 *   Copyright (c) 2021 Abdullah Tahiri <abdullah.tahiri.yo@gmail.com>     *
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

#include <algorithm>
#include <cmath>
#include <functional>
#include <limits>
#include <memory>

#include <Inventor/SbVec3f.h>
#include <Inventor/SoPickedPoint.h>
#include <Inventor/lists/SoPickedPointList.h>
#include <Inventor/details/SoDetail.h>
#include <Inventor/details/SoLineDetail.h>
#include <Inventor/details/SoPointDetail.h>
#include <Inventor/nodes/SoCoordinate3.h>
#include <Inventor/nodes/SoDrawStyle.h>
#include <Inventor/nodes/SoFont.h>
#include <Inventor/nodes/SoGroup.h>
#include <Inventor/nodes/SoLineSet.h>
#include <Inventor/nodes/SoMarkerSet.h>
#include <Inventor/nodes/SoMaterial.h>
#include <Inventor/nodes/SoPickStyle.h>
#include <Inventor/nodes/SoSeparator.h>
#include <Inventor/nodes/SoText2.h>
#include <Inventor/nodes/SoTranslation.h>

#include <Base/Converter.h>
#include <Base/Exception.h>
#include <Gui/Inventor/MarkerBitmaps.h>
#include <Gui/Inventor/SoFCBoundingBox.h>
#include <Gui/Utilities.h>
#include <Mod/Part/App/Geometry.h>
#include <Mod/Sketcher/App/Constraint.h>
#include <Mod/Sketcher/App/GeoList.h>

#include "EditModeCoinManager.h"
#include "EditModeConstraintCoinManager.h"
#include "EditModeGeometryCoinConverter.h"
#include "EditModeGeometryCoinManager.h"
#include "EditModeInformationOverlayCoinConverter.h"
#include "Utils.h"
#include "ViewProviderSketch.h"
#include "ViewProviderSketchCoinAttorney.h"

using namespace SketcherGui;
using namespace Sketcher;

namespace
{
struct ScreenPreselectionPolicy
{
    static constexpr float PointMarkerHitPaddingPx = 5.0F;
    static constexpr float EndpointPointHitRadiusBonusPx = 2.0F;
    static constexpr float PointHoverHysteresisPx = 2.0F;
    static constexpr float EdgeHitPaddingPx = 2.0F;
};

float distanceSquaredToSegment(const SbVec2f& point, const SbVec2f& segmentStart, const SbVec2f& segmentEnd)
{
    float segmentX = segmentEnd[0] - segmentStart[0];
    float segmentY = segmentEnd[1] - segmentStart[1];
    float segmentLengthSquared = segmentX * segmentX + segmentY * segmentY;
    if (segmentLengthSquared <= std::numeric_limits<float>::epsilon()) {
        float dx = point[0] - segmentStart[0];
        float dy = point[1] - segmentStart[1];
        return dx * dx + dy * dy;
    }

    float projection = ((point[0] - segmentStart[0]) * segmentX
                        + (point[1] - segmentStart[1]) * segmentY)
        / segmentLengthSquared;
    projection = std::clamp(projection, 0.0F, 1.0F);

    float closestX = segmentStart[0] + projection * segmentX;
    float closestY = segmentStart[1] + projection * segmentY;
    float dx = point[0] - closestX;
    float dy = point[1] - closestY;
    return dx * dx + dy * dy;
}

// Screen-space point/edge preselection should be resolved in one place so the fallback policy is
// explicit and independent of raw Coin hit ordering.
struct GeometryScreenPreselector
{
    GeometryScreenPreselector(
        const SketcherGui::GeometryLayerParameters& geometryLayerParams,
        const SketcherGui::EditModeScenegraphNodes& scenegraphNodes,
        SketcherGui::CoinMapping& coinMap,
        const SketcherGui::DrawingParameters& drawingParams,
        const SketcherGui::GeoList& geoListArg,
        std::function<SbVec2f(const SbVec3f&)> screenProjectorArg
    )
        : geometryLayerParameters(geometryLayerParams)
        , editModeScenegraphNodes(scenegraphNodes)
        , coinMapping(coinMap)
        , drawingParameters(drawingParams)
        , geolist(geoListArg)
        , projectToScreen(std::move(screenProjectorArg))
    {}

    bool detectHoveredPointPreselection(
        int hoveredPointIndex,
        const SbVec2s& cursorPos,
        SketcherGui::EditModeCoinManager::PreselectionResult& result
    )
    {
        if (hoveredPointIndex == SketcherGui::EditModeCoinManager::PreselectionResult::InvalidPoint) {
            return false;
        }

        SketcherGui::MultiFieldId pointId = coinMapping.getIndexLayer(hoveredPointIndex);
        if (pointId == SketcherGui::MultiFieldId::Invalid) {
            return false;
        }

        float distanceSquared = 0.0F;
        return detectPointDiskPreselection(
            pointId.fieldIndex,
            pointId.layerId,
            cursorPos,
            ScreenPreselectionPolicy::PointHoverHysteresisPx,
            distanceSquared,
            result
        );
    }

    bool detectNearbyPointPreselection(
        const SbVec2s& cursorPos,
        SketcherGui::EditModeCoinManager::PreselectionResult& result
    )
    {
        float bestDistanceSquared = std::numeric_limits<float>::max();
        bool found = false;

        for (int layerIndex = 0; layerIndex < geometryLayerParameters.getCoinLayerCount();
             ++layerIndex) {
            SoCoordinate3* coords = editModeScenegraphNodes.PointsCoordinate[layerIndex];
            if (!coords) {
                continue;
            }

            int pointCount = coords->point.getNum();
            for (int pointIndex = 0; pointIndex < pointCount; ++pointIndex) {
                int vertexId = coinMapping.getPointVertexId(pointIndex, layerIndex);
                if (vertexId < 0) {
                    continue;
                }

                SketcherGui::EditModeCoinManager::PreselectionResult candidate;
                float distanceSquared = 0.0F;
                if (!detectPointDiskPreselection(
                        pointIndex,
                        layerIndex,
                        cursorPos,
                        0.0F,
                        distanceSquared,
                        candidate
                    )) {
                    continue;
                }

                if (distanceSquared > bestDistanceSquared) {
                    continue;
                }

                result = candidate;
                bestDistanceSquared = distanceSquared;
                found = true;
            }
        }

        return found;
    }

    bool detectNearbyCurvePreselection(
        const SbVec2s& cursorPos,
        SketcherGui::EditModeCoinManager::PreselectionResult& result
    )
    {
        float bestDistanceSquared = std::numeric_limits<float>::max();
        bool found = false;
        SbVec2f cursorPoint(static_cast<float>(cursorPos[0]), static_cast<float>(cursorPos[1]));

        for (int layerIndex = 0; layerIndex < geometryLayerParameters.getCoinLayerCount();
             ++layerIndex) {
            for (int subLayerIndex = 0; subLayerIndex < geometryLayerParameters.getSubLayerCount();
                 ++subLayerIndex) {
                if (static_cast<int>(editModeScenegraphNodes.CurvesCoordinate.size()) <= layerIndex
                    || static_cast<int>(editModeScenegraphNodes.CurvesCoordinate[layerIndex].size())
                        <= subLayerIndex
                    || static_cast<int>(editModeScenegraphNodes.CurveSet.size()) <= layerIndex
                    || static_cast<int>(editModeScenegraphNodes.CurveSet[layerIndex].size())
                        <= subLayerIndex) {
                    continue;
                }

                SoCoordinate3* coords
                    = editModeScenegraphNodes.CurvesCoordinate[layerIndex][subLayerIndex];
                SoLineSet* curveSet = editModeScenegraphNodes.CurveSet[layerIndex][subLayerIndex];
                if (!coords || !curveSet) {
                    continue;
                }

                const SbVec3f* curveValues = coords->point.getValues(0);
                int curveCount = curveSet->numVertices.getNum();
                if (!curveValues || curveCount <= 0) {
                    continue;
                }

                float curveHitRadius = getCurveHitRadius(subLayerIndex);
                float curveHitRadiusSquared = curveHitRadius * curveHitRadius;
                int vertexOffset = 0;

                for (int curveIndex = 0; curveIndex < curveCount; ++curveIndex) {
                    int vertexCount = curveSet->numVertices[curveIndex];
                    if (!coinMapping.isValidCurveId(curveIndex, layerIndex, subLayerIndex)) {
                        vertexOffset += std::max(vertexCount, 0);
                        continue;
                    }

                    if (vertexCount < 2) {
                        vertexOffset += std::max(vertexCount, 0);
                        continue;
                    }

                    float bestCurveDistanceSquared = std::numeric_limits<float>::max();
                    int bestSegmentStart = -1;
                    for (int segmentIndex = 0; segmentIndex < vertexCount - 1; ++segmentIndex) {
                        int currentVertexIndex = vertexOffset + segmentIndex;
                        SbVec2f segmentStart = projectToScreen(curveValues[currentVertexIndex]);
                        SbVec2f segmentEnd = projectToScreen(curveValues[currentVertexIndex + 1]);
                        float distanceSquared
                            = distanceSquaredToSegment(cursorPoint, segmentStart, segmentEnd);
                        if (distanceSquared > curveHitRadiusSquared
                            || distanceSquared >= bestCurveDistanceSquared) {
                            continue;
                        }

                        bestCurveDistanceSquared = distanceSquared;
                        bestSegmentStart = currentVertexIndex;
                    }

                    vertexOffset += vertexCount;
                    if (bestSegmentStart < 0 || bestCurveDistanceSquared >= bestDistanceSquared) {
                        continue;
                    }

                    int geoIndex = coinMapping.getCurveGeoId(curveIndex, layerIndex, subLayerIndex);
                    const SbVec3f& startPoint = curveValues[bestSegmentStart];
                    const SbVec3f& endPoint = curveValues[bestSegmentStart + 1];
                    SbVec2f startScreen = projectToScreen(startPoint);
                    SbVec2f endScreen = projectToScreen(endPoint);

                    float segmentX = endScreen[0] - startScreen[0];
                    float segmentY = endScreen[1] - startScreen[1];
                    float segmentLengthSquared = segmentX * segmentX + segmentY * segmentY;
                    float interpolation = 0.0F;
                    if (segmentLengthSquared > std::numeric_limits<float>::epsilon()) {
                        interpolation = ((cursorPoint[0] - startScreen[0]) * segmentX
                                         + (cursorPoint[1] - startScreen[1]) * segmentY)
                            / segmentLengthSquared;
                        interpolation = std::clamp(interpolation, 0.0F, 1.0F);
                    }

                    result.clear();
                    result.Kind = SketcherGui::EditModeCoinManager::PreselectionResult::HitKind::Edge;
                    result.GeoIndex = geoIndex;
                    result.setPickedPoint(
                        Base::Vector3d(
                            startPoint[0] + (endPoint[0] - startPoint[0]) * interpolation,
                            startPoint[1] + (endPoint[1] - startPoint[1]) * interpolation,
                            startPoint[2] + (endPoint[2] - startPoint[2]) * interpolation
                        )
                    );
                    bestDistanceSquared = bestCurveDistanceSquared;
                    found = true;
                }
            }
        }

        return found;
    }

private:
    bool detectPointDiskPreselection(
        int pointIndex,
        int layerIndex,
        const SbVec2s& cursorPos,
        float extraRadiusPx,
        float& distanceSquared,
        SketcherGui::EditModeCoinManager::PreselectionResult& result
    )
    {
        if (!coinMapping.isValidPointId(pointIndex, layerIndex)
            || static_cast<int>(editModeScenegraphNodes.PointsCoordinate.size()) <= layerIndex) {
            return false;
        }

        SoCoordinate3* coords = editModeScenegraphNodes.PointsCoordinate[layerIndex];
        if (!coords || pointIndex >= coords->point.getNum()) {
            return false;
        }

        const SbVec3f* pointValues = coords->point.getValues(0);
        if (!pointValues) {
            return false;
        }

        float pointHitRadius = getPointHitRadius(pointIndex, layerIndex, extraRadiusPx);
        SbVec2f screenPoint = projectToScreen(pointValues[pointIndex]);
        float dx = static_cast<float>(cursorPos[0]) - screenPoint[0];
        float dy = static_cast<float>(cursorPos[1]) - screenPoint[1];
        distanceSquared = dx * dx + dy * dy;
        if (distanceSquared > pointHitRadius * pointHitRadius) {
            return false;
        }

        result.clear();
        result.Kind = SketcherGui::EditModeCoinManager::PreselectionResult::HitKind::Point;
        result.PointIndex = coinMapping.getPointVertexId(pointIndex, layerIndex);
        result.setPickedPoint(
            Base::Vector3d(
                pointValues[pointIndex][0],
                pointValues[pointIndex][1],
                pointValues[pointIndex][2]
            )
        );
        return true;
    }

    float getPointHitRadius(int pointIndex, int layerIndex, float extraRadiusPx) const
    {
        float markerExtent = static_cast<float>(drawingParameters.markerSize);
        if (static_cast<int>(editModeScenegraphNodes.PointsDrawStyle.size()) > layerIndex
            && editModeScenegraphNodes.PointsDrawStyle[layerIndex]) {
            markerExtent = std::max(
                markerExtent,
                editModeScenegraphNodes.PointsDrawStyle[layerIndex]->pointSize.getValue()
            );
        }

        int pointGeoId = coinMapping.getPointGeoId(pointIndex, layerIndex);
        Sketcher::PointPos pointPos = coinMapping.getPointPosId(pointIndex, layerIndex);
        const Part::Geometry* geometry = geolist.getGeometryFromGeoId(pointGeoId);
        bool isEndpointOfNonPointGeometry = geometry
            && geometry->getTypeId() != Part::GeomPoint::getClassTypeId()
            && (pointPos == Sketcher::PointPos::start || pointPos == Sketcher::PointPos::end);

        float pointHitRadius = markerExtent * 0.5f
            + ScreenPreselectionPolicy::PointMarkerHitPaddingPx + extraRadiusPx;
        if (isEndpointOfNonPointGeometry) {
            pointHitRadius += ScreenPreselectionPolicy::EndpointPointHitRadiusBonusPx;
        }

        return pointHitRadius;
    }

    float getCurveHitRadius(int subLayerIndex) const
    {
        SoDrawStyle* drawStyle = editModeScenegraphNodes.CurvesDrawStyle;
        if (geometryLayerParameters.isConstructionSubLayer(subLayerIndex)) {
            drawStyle = editModeScenegraphNodes.CurvesConstructionDrawStyle;
        }
        else if (geometryLayerParameters.isInternalSubLayer(subLayerIndex)) {
            drawStyle = editModeScenegraphNodes.CurvesInternalDrawStyle;
        }
        else if (geometryLayerParameters.isExternalSubLayer(subLayerIndex)) {
            drawStyle = editModeScenegraphNodes.CurvesExternalDrawStyle;
        }
        else if (geometryLayerParameters.isExternalDefiningSubLayer(subLayerIndex)) {
            drawStyle = editModeScenegraphNodes.CurvesExternalDefiningDrawStyle;
        }

        float lineWidth = drawStyle ? drawStyle->lineWidth.getValue() : 1.0F;
        return std::max(3.0F, lineWidth * 0.5F + ScreenPreselectionPolicy::EdgeHitPaddingPx);
    }

    const SketcherGui::GeometryLayerParameters& geometryLayerParameters;
    const SketcherGui::EditModeScenegraphNodes& editModeScenegraphNodes;
    SketcherGui::CoinMapping& coinMapping;
    const SketcherGui::DrawingParameters& drawingParameters;
    const SketcherGui::GeoList& geolist;
    const std::function<SbVec2f(const SbVec3f&)> projectToScreen;
};
}  // namespace

void EditModeCoinManager::PreselectionResult::setPickedPoint(const Base::Vector3d& point)
{
    PickedPoint = point;
}

void EditModeCoinManager::PreselectionResult::setPickedPoint(const SoPickedPoint* point)
{
    setPickedPoint(Base::convertTo<Base::Vector3d>(point->getPoint()));
}

//**************************** ParameterObserver nested class ******************************
EditModeCoinManager::ParameterObserver::ParameterObserver(EditModeCoinManager& client)
    : Client(client)
{
    initParameters();
    subscribeToParameters();
}

EditModeCoinManager::ParameterObserver::~ParameterObserver()
{
    unsubscribeToParameters();
}

void EditModeCoinManager::ParameterObserver::initParameters()
{
    // static map to avoid substantial if/else branching
    //
    // key->first               => String of parameter,
    // key->second              => Update function to be called for the parameter,
    str2updatefunction = {
        {"SegmentsPerGeometry",
         [this](const std::string& param) { updateCurvedEdgeCountSegmentsParameter(param); }},
        {"BSplineDegreeVisible",
         [this](const std::string& param) {
             updateOverlayVisibilityParameter<OverlayVisibilityParameter::BSplineDegree>(param);
         }},
        {"BSplineControlPolygonVisible",
         [this](const std::string& param) {
             updateOverlayVisibilityParameter<OverlayVisibilityParameter::BSplineControlPolygonVisible>(
                 param
             );
         }},
        {"BSplineCombVisible",
         [this](const std::string& param) {
             updateOverlayVisibilityParameter<OverlayVisibilityParameter::BSplineCombVisible>(param);
         }},
        {"BSplineKnotMultiplicityVisible",
         [this](const std::string& param) {
             updateOverlayVisibilityParameter<OverlayVisibilityParameter::BSplineKnotMultiplicityVisible>(
                 param
             );
         }},
        {"BSplinePoleWeightVisible",
         [this](const std::string& param) {
             updateOverlayVisibilityParameter<OverlayVisibilityParameter::BSplinePoleWeightVisible>(
                 param
             );
         }},
        {"ArcCircleHelperVisible",
         [this](const std::string& param) {
             updateOverlayVisibilityParameter<OverlayVisibilityParameter::ArcCircleHelperVisible>(
                 param
             );
         }},
        {"TopRenderGeometryId",
         [this](const std::string& param) { updateLineRenderingOrderParameters(param); }},
        {"MidRenderGeometryId",
         [this](const std::string& param) { updateLineRenderingOrderParameters(param); }},
        {"HideUnits",
         [this](const std::string& param) { updateConstraintPresentationParameters(param); }},
        {"ShowDimensionalName",
         [this](const std::string& param) { updateConstraintPresentationParameters(param); }},
        {"DimensionalStringFormat",
         [this](const std::string& param) { updateConstraintPresentationParameters(param); }},
        {"ViewScalingFactor", [this](const std::string&) { Client.updateElementSizeParameters(); }},
        {"MarkerSize", [this](const std::string&) { Client.updateElementSizeParameters(); }},
        {"EditSketcherFontName", [this](const std::string&) { Client.updateElementSizeParameters(); }},
        {"EditSketcherFontSize", [this](const std::string&) { Client.updateElementSizeParameters(); }},
        {"ConstraintIconHitPadding",
         [this](const std::string&) { Client.updateElementSizeParameters(); }},
        {"EdgeWidth",
         [this, &drawingParameters = Client.drawingParameters](const std::string& param) {
             updateWidth(drawingParameters.CurveWidth, param, 2);
         }},
        {"ConstructionWidth",
         [this, &drawingParameters = Client.drawingParameters](const std::string& param) {
             updateWidth(drawingParameters.ConstructionWidth, param, 2);
         }},
        {"InternalWidth",
         [this, &drawingParameters = Client.drawingParameters](const std::string& param) {
             updateWidth(drawingParameters.InternalWidth, param, 2);
         }},
        {"ExternalWidth",
         [this, &drawingParameters = Client.drawingParameters](const std::string& param) {
             updateWidth(drawingParameters.ExternalWidth, param, 2);
         }},
        {"ExternalDefiningWidth",
         [this, &drawingParameters = Client.drawingParameters](const std::string& param) {
             updateWidth(drawingParameters.ExternalDefiningWidth, param, 2);
         }},
        {"EdgePattern",
         [this, &drawingParameters = Client.drawingParameters](const std::string& param) {
             updatePattern(drawingParameters.CurvePattern, param, 0b1111111111111111);
         }},
        {"ConstructionPattern",
         [this, &drawingParameters = Client.drawingParameters](const std::string& param) {
             updatePattern(drawingParameters.ConstructionPattern, param, 0b1111110011111100);
         }},
        {"InternalPattern",
         [this, &drawingParameters = Client.drawingParameters](const std::string& param) {
             updatePattern(drawingParameters.InternalPattern, param, 0b1111110011111100);
         }},
        {"ExternalPattern",
         [this, &drawingParameters = Client.drawingParameters](const std::string& param) {
             updatePattern(drawingParameters.ExternalPattern, param, 0b1111110011111100);
         }},
        {"ExternalDefiningPattern",
         [this, &drawingParameters = Client.drawingParameters](const std::string& param) {
             updatePattern(drawingParameters.ExternalDefiningPattern, param, 0b1111111111111111);
         }},
        {"EditedEdgeColor",
         [this, drawingParameters = Client.drawingParameters](const std::string& param) {
             updateColor(drawingParameters.CurveColor, param);
         }},
        {"ConstructionColor",
         [this, drawingParameters = Client.drawingParameters](const std::string& param) {
             updateColor(drawingParameters.CurveDraftColor, param);
         }},
        {"InternalAlignedGeoColor",
         [this, drawingParameters = Client.drawingParameters](const std::string& param) {
             updateColor(drawingParameters.InternalAlignedGeoColor, param);
         }},
        {"FullyConstraintElementColor",
         [this, drawingParameters = Client.drawingParameters](const std::string& param) {
             updateColor(drawingParameters.FullyConstraintElementColor, param);
         }},
        {"FullyConstraintConstructionElementColor",
         [this, drawingParameters = Client.drawingParameters](const std::string& param) {
             updateColor(drawingParameters.FullyConstraintConstructionElementColor, param);
         }},
        {"FullyConstraintInternalAlignmentColor",
         [this, drawingParameters = Client.drawingParameters](const std::string& param) {
             updateColor(drawingParameters.FullyConstraintInternalAlignmentColor, param);
         }},
        {"FullyConstraintElementColor",
         [this, drawingParameters = Client.drawingParameters](const std::string& param) {
             updateColor(drawingParameters.FullyConstraintElementColor, param);
         }},
        {"InvalidSketchColor",
         [this, drawingParameters = Client.drawingParameters](const std::string& param) {
             updateColor(drawingParameters.InvalidSketchColor, param);
         }},
        {"FullyConstrainedColor",
         [this, drawingParameters = Client.drawingParameters](const std::string& param) {
             updateColor(drawingParameters.FullyConstrainedColor, param);
         }},
        {"ConstrainedDimColor",
         [this, drawingParameters = Client.drawingParameters](const std::string& param) {
             updateColor(drawingParameters.ConstrDimColor, param);
         }},
        {"ConstrainedIcoColor",
         [this, drawingParameters = Client.drawingParameters](const std::string& param) {
             updateColor(drawingParameters.ConstrIcoColor, param);
         }},
        {"NonDrivingConstrDimColor",
         [this, drawingParameters = Client.drawingParameters](const std::string& param) {
             updateColor(drawingParameters.NonDrivingConstrDimColor, param);
         }},
        {"ExprBasedConstrDimColor",
         [this, drawingParameters = Client.drawingParameters](const std::string& param) {
             updateColor(drawingParameters.ExprBasedConstrDimColor, param);
         }},
        {"DeactivatedConstrDimColor",
         [this, drawingParameters = Client.drawingParameters](const std::string& param) {
             updateColor(drawingParameters.DeactivatedConstrDimColor, param);
         }},
        {"ExternalColor",
         [this, drawingParameters = Client.drawingParameters](const std::string& param) {
             updateColor(drawingParameters.CurveExternalColor, param);
         }},
        {"ExternalDefiningColor",
         [this, drawingParameters = Client.drawingParameters](const std::string& param) {
             updateColor(drawingParameters.CurveExternalDefiningColor, param);
         }},
        {"HighlightColor",
         [this, drawingParameters = Client.drawingParameters](const std::string& param) {
             updateColor(drawingParameters.PreselectColor, param);
         }},
        {"SelectionColor",
         [this, drawingParameters = Client.drawingParameters](const std::string& param) {
             updateColor(drawingParameters.SelectColor, param);
         }},
        {"CursorTextColor",
         [this, drawingParameters = Client.drawingParameters](const std::string& param) {
             updateColor(drawingParameters.CursorTextColor, param);
         }},
        {"UserSchema", [this](const std::string& param) { updateUnit(param); }},
    };

    for (auto& val : str2updatefunction) {
        auto string = val.first;
        auto function = val.second;

        function(string);
    }
}

void EditModeCoinManager::ParameterObserver::updateCurvedEdgeCountSegmentsParameter(
    const std::string& parametername
)
{
    ParameterGrp::handle hGrp = App::GetApplication().GetParameterGroupByPath(
        "User parameter:BaseApp/Preferences/View"
    );
    int stdcountsegments = hGrp->GetInt(parametername.c_str(), 50);
    // value cannot be smaller than 6
    if (stdcountsegments < 6) {
        stdcountsegments = 6;
    }

    Client.drawingParameters.curvedEdgeCountSegments = stdcountsegments;
}

void EditModeCoinManager::ParameterObserver::updateLineRenderingOrderParameters(
    const std::string& parametername
)
{
    (void)parametername;

    ParameterGrp::handle hGrpp = App::GetApplication().GetParameterGroupByPath(
        "User parameter:BaseApp/Preferences/Mod/Sketcher/General"
    );

    Client.drawingParameters.topRenderingGeometry = DrawingParameters::GeometryRendering(
        hGrpp->GetInt("TopRenderGeometryId", 1)
    );
    Client.drawingParameters.midRenderingGeometry = DrawingParameters::GeometryRendering(
        hGrpp->GetInt("MidRenderGeometryId", 2)
    );
}

void EditModeCoinManager::ParameterObserver::updateConstraintPresentationParameters(
    const std::string& parametername
)
{
    (void)parametername;

    ParameterGrp::handle hGrpskg = App::GetApplication().GetParameterGroupByPath(
        "User parameter:BaseApp/Preferences/Mod/Sketcher"
    );

    Client.constraintParameters.bHideUnits = hGrpskg->GetBool("HideUnits", false);
    Client.constraintParameters.bShowDimensionalName = hGrpskg->GetBool("ShowDimensionalName", true);
    Client.constraintParameters.sDimensionalStringFormat = QString::fromStdString(
        hGrpskg->GetASCII("DimensionalStringFormat", "%N = %V")
    );
}

template<EditModeCoinManager::ParameterObserver::OverlayVisibilityParameter visibilityparameter>
void EditModeCoinManager::ParameterObserver::updateOverlayVisibilityParameter(
    const std::string& parametername
)
{
    ParameterGrp::handle hGrpsk = App::GetApplication().GetParameterGroupByPath(
        "User parameter:BaseApp/Preferences/Mod/Sketcher/General"
    );

    if constexpr (visibilityparameter == OverlayVisibilityParameter::BSplineDegree) {
        Client.overlayParameters.bSplineDegreeVisible = hGrpsk->GetBool(parametername.c_str(), true);
    }
    else if constexpr (visibilityparameter == OverlayVisibilityParameter::BSplineControlPolygonVisible) {
        Client.overlayParameters.bSplineControlPolygonVisible
            = hGrpsk->GetBool(parametername.c_str(), true);
    }
    else if constexpr (visibilityparameter == OverlayVisibilityParameter::BSplineCombVisible) {
        Client.overlayParameters.bSplineCombVisible = hGrpsk->GetBool(parametername.c_str(), true);
    }
    else if constexpr (
        visibilityparameter == OverlayVisibilityParameter::BSplineKnotMultiplicityVisible
    ) {
        Client.overlayParameters.bSplineKnotMultiplicityVisible
            = hGrpsk->GetBool(parametername.c_str(), true);
    }
    else if constexpr (visibilityparameter == OverlayVisibilityParameter::BSplinePoleWeightVisible) {
        Client.overlayParameters.bSplinePoleWeightVisible
            = hGrpsk->GetBool(parametername.c_str(), true);
    }
    else if constexpr (visibilityparameter == OverlayVisibilityParameter::ArcCircleHelperVisible) {
        Client.overlayParameters.arcCircleHelperVisible = hGrpsk->GetBool(parametername.c_str(), false);
    }

    Client.overlayParameters.visibleInformationChanged = true;
}

void EditModeCoinManager::ParameterObserver::updateWidth(
    int& width,
    const std::string& parametername,
    int def
)
{
    ParameterGrp::handle hGrp = App::GetApplication().GetParameterGroupByPath(
        "User parameter:BaseApp/Preferences/Mod/Sketcher/View"
    );

    width = hGrp->GetInt(parametername.c_str(), def);

    Client.updateInventorWidths();
}

void EditModeCoinManager::ParameterObserver::updatePattern(
    unsigned int& pattern,
    const std::string& parametername,
    unsigned int def
)
{
    ParameterGrp::handle hGrp = App::GetApplication().GetParameterGroupByPath(
        "User parameter:BaseApp/Preferences/Mod/Sketcher/View"
    );

    pattern = hGrp->GetInt(parametername.c_str(), def);

    Client.updateInventorPatterns();
}

void EditModeCoinManager::ParameterObserver::updateColor(SbColor& sbcolor, const std::string& parametername)
{
    ParameterGrp::handle hGrp = App::GetApplication().GetParameterGroupByPath(
        "User parameter:BaseApp/Preferences/View"
    );

    float transparency = 0.f;
    unsigned long color = (unsigned long)(sbcolor.getPackedValue());
    color = hGrp->GetUnsigned(parametername.c_str(), color);
    sbcolor.setPackedValue((uint32_t)color, transparency);

    Client.updateInventorColors();
}

void EditModeCoinManager::ParameterObserver::updateUnit(const std::string& parametername)
{
    Q_UNUSED(parametername);
    // Nothing to do because we only need Client.redrawViewProvider(); that is already called in
    // OnChange.
}

void EditModeCoinManager::ParameterObserver::subscribeToParameters()
{
    try {
        ParameterGrp::handle hGrp = App::GetApplication().GetParameterGroupByPath(
            "User parameter:BaseApp/Preferences/View"
        );
        hGrp->Attach(this);

        ParameterGrp::handle hGrpsk = App::GetApplication().GetParameterGroupByPath(
            "User parameter:BaseApp/Preferences/Mod/Sketcher/General"
        );
        hGrpsk->Attach(this);

        ParameterGrp::handle hGrpskg = App::GetApplication().GetParameterGroupByPath(
            "User parameter:BaseApp/Preferences/Mod/Sketcher"
        );
        hGrpskg->Attach(this);

        ParameterGrp::handle hGrpu = App::GetApplication().GetParameterGroupByPath(
            "User parameter:BaseApp/Preferences/Units"
        );
        hGrpu->Attach(this);
    }
    catch (const Base::ValueError& e) {  // ensure that if parameter strings are not well-formed,
                                         // the exception is not propagated
        Base::Console()
            .developerError("EditModeCoinManager", "Malformed parameter string: %s\n", e.what());
    }
}

void EditModeCoinManager::ParameterObserver::unsubscribeToParameters()
{
    try {
        ParameterGrp::handle hGrp = App::GetApplication().GetParameterGroupByPath(
            "User parameter:BaseApp/Preferences/View"
        );
        hGrp->Detach(this);

        ParameterGrp::handle hGrpsk = App::GetApplication().GetParameterGroupByPath(
            "User parameter:BaseApp/Preferences/Mod/Sketcher/General"
        );
        hGrpsk->Detach(this);

        ParameterGrp::handle hGrpskg = App::GetApplication().GetParameterGroupByPath(
            "User parameter:BaseApp/Preferences/Mod/Sketcher"
        );
        hGrpskg->Detach(this);

        ParameterGrp::handle hGrpu = App::GetApplication().GetParameterGroupByPath(
            "User parameter:BaseApp/Preferences/Units"
        );
        hGrpu->Detach(this);
    }
    catch (const Base::ValueError& e) {  // ensure that if parameter strings are not well-formed,
                                         // the program is not terminated when calling the noexcept
                                         // destructor.
        Base::Console()
            .developerError("EditModeCoinManager", "Malformed parameter string: %s\n", e.what());
    }
}

void EditModeCoinManager::ParameterObserver::OnChange(
    Base::Subject<const char*>& rCaller,
    const char* sReason
)
{
    (void)rCaller;

    auto key = str2updatefunction.find(sReason);
    if (key != str2updatefunction.end()) {
        auto string = key->first;
        auto function = key->second;

        function(string);

        Client.redrawViewProvider();  // redraw with non-temporal geometry
    }
}

//**************************** EditModeCoinManager class ******************************

EditModeCoinManager::EditModeCoinManager(ViewProviderSketch& vp)
    : viewProvider(vp)
{

    pEditModeConstraintCoinManager = std::make_unique<EditModeConstraintCoinManager>(
        viewProvider,
        drawingParameters,
        geometryLayerParameters,
        constraintParameters,
        editModeScenegraphNodes,
        coinMapping
    );

    pEditModeGeometryCoinManager = std::make_unique<EditModeGeometryCoinManager>(
        viewProvider,
        drawingParameters,
        geometryLayerParameters,
        analysisResults,
        editModeScenegraphNodes,
        coinMapping
    );
    // Create Edit Mode Scenograph
    createEditModeInventorNodes();

    // Create parameter observer and initialise watched parameters
    pObserver = std::make_unique<EditModeCoinManager::ParameterObserver>(*this);
}

EditModeCoinManager::~EditModeCoinManager()
{
    Gui::coinRemoveAllChildren(editModeScenegraphNodes.EditRoot);
    ViewProviderSketchCoinAttorney::removeNodeFromRoot(viewProvider, editModeScenegraphNodes.EditRoot);
    editModeScenegraphNodes.EditRoot->unref();
}

/***** Temporary edit curves and markers *****/

void EditModeCoinManager::drawEditMarkers(
    const std::vector<Base::Vector2d>& EditMarkers,
    unsigned int augmentationlevel
)
{
    // determine marker size
    int augmentedmarkersize = drawingParameters.markerSize;

    auto supportedsizes = Gui::Inventor::MarkerBitmaps::getSupportedSizes("CIRCLE_LINE");

    const auto defaultmarker = std::ranges::find(supportedsizes, drawingParameters.markerSize);

    if (defaultmarker != supportedsizes.end()) {

        if (const auto validAugmentationLevels = std::distance(defaultmarker, supportedsizes.end());
            augmentationlevel >= validAugmentationLevels) {
            augmentationlevel = validAugmentationLevels - 1;
        }

        augmentedmarkersize = *std::next(defaultmarker, augmentationlevel);
    }

    editModeScenegraphNodes.EditMarkerSet->markerIndex.startEditing();
    editModeScenegraphNodes.EditMarkerSet->markerIndex
        = Gui::Inventor::MarkerBitmaps::getMarkerIndex("CIRCLE_LINE", augmentedmarkersize);

    // add the points to set
    editModeScenegraphNodes.EditMarkersCoordinate->point.setNum(EditMarkers.size());
    editModeScenegraphNodes.EditMarkersMaterials->diffuseColor.setNum(EditMarkers.size());
    SbVec3f* verts = editModeScenegraphNodes.EditMarkersCoordinate->point.startEditing();
    SbColor* color = editModeScenegraphNodes.EditMarkersMaterials->diffuseColor.startEditing();

    int i = 0;  // setting up the line set
    for (std::vector<Base::Vector2d>::const_iterator it = EditMarkers.begin();
         it != EditMarkers.end();
         ++it, i++) {
        verts[i].setValue(
            it->x,
            it->y,
            ViewProviderSketchCoinAttorney::getViewOrientationFactor(viewProvider)
                * drawingParameters.zEdit
        );
        color[i] = drawingParameters.InformationColor;
    }

    editModeScenegraphNodes.EditMarkersCoordinate->point.finishEditing();
    editModeScenegraphNodes.EditMarkersMaterials->diffuseColor.finishEditing();
    editModeScenegraphNodes.EditMarkerSet->markerIndex.finishEditing();
}

void EditModeCoinManager::drawEdit(const std::vector<Base::Vector2d>& EditCurve, GeometryCreationMode mode)
{
    editModeScenegraphNodes.EditCurveSet->numVertices.setNum(1);
    editModeScenegraphNodes.EditCurvesCoordinate->point.setNum(EditCurve.size());
    editModeScenegraphNodes.EditCurvesMaterials->diffuseColor.setNum(EditCurve.size());
    SbVec3f* verts = editModeScenegraphNodes.EditCurvesCoordinate->point.startEditing();
    int32_t* index = editModeScenegraphNodes.EditCurveSet->numVertices.startEditing();
    SbColor* color = editModeScenegraphNodes.EditCurvesMaterials->diffuseColor.startEditing();

    setEditDrawStyle(mode);

    int i = 0;  // setting up the line set
    for (std::vector<Base::Vector2d>::const_iterator it = EditCurve.begin(); it != EditCurve.end();
         ++it, i++) {
        verts[i].setValue(
            it->x,
            it->y,
            ViewProviderSketchCoinAttorney::getViewOrientationFactor(viewProvider)
                * drawingParameters.zEdit
        );
        switch (mode) {
            case GeometryCreationMode::Normal:
                color[i] = drawingParameters.CurveColor;
                break;
            case GeometryCreationMode::Construction:
                color[i] = drawingParameters.CurveDraftColor;
                break;
        }
    }

    index[0] = EditCurve.size();
    editModeScenegraphNodes.EditCurvesCoordinate->point.finishEditing();
    editModeScenegraphNodes.EditCurveSet->numVertices.finishEditing();
    editModeScenegraphNodes.EditCurvesMaterials->diffuseColor.finishEditing();
}

void EditModeCoinManager::drawEdit(
    const std::list<std::vector<Base::Vector2d>>& list,
    GeometryCreationMode mode
)
{
    int ncoords = 0;

    for (const auto& v : list) {
        ncoords += v.size();
    }

    editModeScenegraphNodes.EditCurveSet->numVertices.setNum(list.size());
    editModeScenegraphNodes.EditCurvesCoordinate->point.setNum(ncoords);
    editModeScenegraphNodes.EditCurvesMaterials->diffuseColor.setNum(ncoords);
    SbVec3f* verts = editModeScenegraphNodes.EditCurvesCoordinate->point.startEditing();
    int32_t* index = editModeScenegraphNodes.EditCurveSet->numVertices.startEditing();
    SbColor* color = editModeScenegraphNodes.EditCurvesMaterials->diffuseColor.startEditing();

    setEditDrawStyle(mode);

    int coordindex = 0;
    int indexindex = 0;
    for (const auto& v : list) {
        for (const auto& p : v) {
            verts[coordindex].setValue(
                p.x,
                p.y,
                ViewProviderSketchCoinAttorney::getViewOrientationFactor(viewProvider)
                    * drawingParameters.zEdit
            );

            switch (mode) {
                case GeometryCreationMode::Normal:
                    color[coordindex] = drawingParameters.CurveColor;
                    break;
                case GeometryCreationMode::Construction:
                    color[coordindex] = drawingParameters.CurveDraftColor;
                    break;
            }

            coordindex++;
        }
        index[indexindex] = v.size();
        indexindex++;
    }

    editModeScenegraphNodes.EditCurvesCoordinate->point.finishEditing();
    editModeScenegraphNodes.EditCurveSet->numVertices.finishEditing();
    editModeScenegraphNodes.EditCurvesMaterials->diffuseColor.finishEditing();
}

void EditModeCoinManager::drawLineExtensionAutoConstraintHint(
    const std::vector<Base::Vector2d>& HintCurve
)
{
    editModeScenegraphNodes.LineExtensionAutoConstraintHintSet->numVertices.setNum(1);
    editModeScenegraphNodes.LineExtensionAutoConstraintHintCoordinate->point.setNum(HintCurve.size());
    editModeScenegraphNodes.LineExtensionAutoConstraintHintMaterials->diffuseColor.setNum(
        HintCurve.size()
    );

    SbVec3f* verts
        = editModeScenegraphNodes.LineExtensionAutoConstraintHintCoordinate->point.startEditing();
    int32_t* index
        = editModeScenegraphNodes.LineExtensionAutoConstraintHintSet->numVertices.startEditing();
    SbColor* color = editModeScenegraphNodes.LineExtensionAutoConstraintHintMaterials->diffuseColor
                         .startEditing();

    editModeScenegraphNodes.LineExtensionAutoConstraintHintDrawStyle->lineWidth
        = editModeScenegraphNodes.InformationDrawStyle->lineWidth;
    editModeScenegraphNodes.LineExtensionAutoConstraintHintDrawStyle->linePattern
        = editModeScenegraphNodes.CurvesConstructionDrawStyle->linePattern;
    editModeScenegraphNodes.LineExtensionAutoConstraintHintDrawStyle->linePatternScaleFactor
        = editModeScenegraphNodes.CurvesConstructionDrawStyle->linePatternScaleFactor;

    int i = 0;
    for (const auto& point : HintCurve) {
        verts[i].setValue(
            point.x,
            point.y,
            ViewProviderSketchCoinAttorney::getViewOrientationFactor(viewProvider)
                * drawingParameters.zEdit
        );
        color[i] = drawingParameters.InformationColor;
        ++i;
    }

    index[0] = HintCurve.size();

    editModeScenegraphNodes.LineExtensionAutoConstraintHintCoordinate->point.finishEditing();
    editModeScenegraphNodes.LineExtensionAutoConstraintHintSet->numVertices.finishEditing();
    editModeScenegraphNodes.LineExtensionAutoConstraintHintMaterials->diffuseColor.finishEditing();
}

void EditModeCoinManager::setPositionText(const Base::Vector2d& Pos, const SbString& text)
{
    editModeScenegraphNodes.textX->string = text;
    editModeScenegraphNodes.textPos->translation = SbVec3f(
        Pos.x,
        Pos.y,
        ViewProviderSketchCoinAttorney::getViewOrientationFactor(viewProvider) * drawingParameters.zText
    );
}

void EditModeCoinManager::setPositionText(const Base::Vector2d& Pos)
{
    if (showCursorCoords()) {
        SbString text;
        std::string xString = lengthToDisplayFormat(Pos.x, 1);
        std::string yString = lengthToDisplayFormat(Pos.y, 1);
        text.sprintf(" (%s, %s)", xString.c_str(), yString.c_str());
        setPositionText(Pos, text);
    }
}

void EditModeCoinManager::resetPositionText()
{
    editModeScenegraphNodes.textX->string = "";
}

void EditModeCoinManager::setAxisPickStyle(bool on)
{
    if (on) {
        editModeScenegraphNodes.pickStyleAxes->style = SoPickStyle::SHAPE;
    }
    else {
        editModeScenegraphNodes.pickStyleAxes->style = SoPickStyle::UNPICKABLE;
    }
}

EditModeCoinManager::PreselectionResult EditModeCoinManager::detectConstraintPreselection(
    const SoPickedPointList& points,
    const SbVec2s& cursorPos
)
{
    PreselectionResult result;
    Base::Vector3d pickedPoint;

    result.ConstrIndices
        = pEditModeConstraintCoinManager->detectPreselectionConstr(cursorPos, &pickedPoint);
    if (!result.ConstrIndices.empty()) {
        result.Kind = PreselectionResult::HitKind::Constraint;
        result.setPickedPoint(pickedPoint);
        return result;
    }

    for (int i = 0; i < points.getLength(); ++i) {
        SoPickedPoint* point = points[i];
        if (!point) {
            continue;
        }

        result.ConstrIndices
            = pEditModeConstraintCoinManager->detectPreselectionConstr(point, cursorPos);
        if (result.ConstrIndices.empty()) {
            continue;
        }

        result.Kind = PreselectionResult::HitKind::Constraint;
        result.setPickedPoint(point);
        return result;
    }

    return result;
}

bool EditModeCoinManager::detectOriginPreselection(const SoPickedPoint* point, PreselectionResult& result)
{
    SoPath* path = point->getPath();
    SoNode* tail = path->getTail();
    if (tail != editModeScenegraphNodes.OriginPointSet) {
        return false;
    }

    const SoDetail* pointDetail = point->getDetail(editModeScenegraphNodes.OriginPointSet);
    if (!pointDetail || pointDetail->getTypeId() != SoPointDetail::getClassTypeId()) {
        return false;
    }

    result.Kind = PreselectionResult::HitKind::Axis;
    result.Cross = PreselectionResult::Axes::RootPoint;
    result.setPickedPoint(point);
    return true;
}

bool EditModeCoinManager::detectPointPreselection(
    const SoPickedPoint* point,
    int layerIndex,
    PreselectionResult& result
)
{
    SoPath* path = point->getPath();
    SoNode* tail = path->getTail();
    if (tail != editModeScenegraphNodes.PointSet[layerIndex]) {
        return false;
    }

    const SoDetail* pointDetail = point->getDetail(editModeScenegraphNodes.PointSet[layerIndex]);
    if (!pointDetail || pointDetail->getTypeId() != SoPointDetail::getClassTypeId()) {
        return false;
    }

    int pointIndex = static_cast<const SoPointDetail*>(pointDetail)->getCoordinateIndex();
    result.PointIndex = coinMapping.getPointVertexId(pointIndex, layerIndex);
    if (result.PointIndex == -1) {
        result.Kind = PreselectionResult::HitKind::Axis;
        result.Cross = PreselectionResult::Axes::RootPoint;
    }
    else {
        result.Kind = PreselectionResult::HitKind::Point;
    }

    result.setPickedPoint(point);
    return true;
}

bool EditModeCoinManager::detectCurvePreselection(
    const SoPickedPoint* point,
    int layerIndex,
    PreselectionResult& result
)
{
    SoPath* path = point->getPath();
    SoNode* tail = path->getTail();

    for (int subLayerIndex = 0; subLayerIndex < geometryLayerParameters.getSubLayerCount();
         ++subLayerIndex) {
        if (tail != editModeScenegraphNodes.CurveSet[layerIndex][subLayerIndex]) {
            continue;
        }

        const SoDetail* curveDetail = point->getDetail(
            editModeScenegraphNodes.CurveSet[layerIndex][subLayerIndex]
        );
        if (!curveDetail || curveDetail->getTypeId() != SoLineDetail::getClassTypeId()) {
            return false;
        }

        int curveIndex = static_cast<const SoLineDetail*>(curveDetail)->getLineIndex();
        result.GeoIndex = coinMapping.getCurveGeoId(curveIndex, layerIndex, subLayerIndex);
        result.Kind = PreselectionResult::HitKind::Edge;
        result.setPickedPoint(point);
        return true;
    }

    return false;
}

bool EditModeCoinManager::detectPointPreselection(
    const SoPickedPointList& points,
    PreselectionResult& result
)
{
    for (int i = 0; i < points.getLength(); ++i) {
        SoPickedPoint* point = points[i];
        if (!point) {
            continue;
        }

        for (int layerIndex = 0; layerIndex < geometryLayerParameters.getCoinLayerCount();
             ++layerIndex) {
            PreselectionResult candidate;
            if (!detectPointPreselection(point, layerIndex, candidate)) {
                continue;
            }

            if (candidate.Kind == PreselectionResult::HitKind::Point) {
                result = candidate;
                return true;
            }
        }
    }

    return false;
}

bool EditModeCoinManager::detectCurvePreselection(
    const SoPickedPointList& points,
    PreselectionResult& result
)
{
    for (int i = 0; i < points.getLength(); ++i) {
        SoPickedPoint* point = points[i];
        if (!point) {
            continue;
        }

        for (int layerIndex = 0; layerIndex < geometryLayerParameters.getCoinLayerCount();
             ++layerIndex) {
            if (detectCurvePreselection(point, layerIndex, result)) {
                return true;
            }
        }
    }

    return false;
}

bool EditModeCoinManager::detectGeometryPreselection(
    const SoPickedPointList& points,
    const SbVec2s& cursorPos,
    int hoveredPointIndex,
    PreselectionResult& result
)
{
    const GeoList geolist = ViewProviderSketchCoinAttorney::getGeoList(viewProvider);
    auto projectToScreen = [this](const SbVec3f& point) {
        return ViewProviderSketchCoinAttorney::getScreenCoordinates(viewProvider, point);
    };

    GeometryScreenPreselector screenPreselector {
        geometryLayerParameters,
        editModeScenegraphNodes,
        coinMapping,
        drawingParameters,
        geolist,
        projectToScreen
    };

    if (detectPointPreselection(points, result)) {
        return true;
    }

    if (screenPreselector.detectHoveredPointPreselection(hoveredPointIndex, cursorPos, result)) {
        return true;
    }

    if (screenPreselector.detectNearbyPointPreselection(cursorPos, result)) {
        return true;
    }

    if (screenPreselector.detectNearbyCurvePreselection(cursorPos, result)) {
        return true;
    }

    if (detectCurvePreselection(points, result)) {
        return true;
    }

    return false;
}

bool EditModeCoinManager::detectAxisPreselection(const SoPickedPoint* point, PreselectionResult& result)
{
    SoPath* path = point->getPath();
    SoNode* tail = path->getTail();
    if (tail != editModeScenegraphNodes.RootCrossSet) {
        return false;
    }

    const SoDetail* crossDetail = point->getDetail(editModeScenegraphNodes.RootCrossSet);
    if (!crossDetail || crossDetail->getTypeId() != SoLineDetail::getClassTypeId()) {
        return false;
    }

    int crossIndex = static_cast<const SoLineDetail*>(crossDetail)->getLineIndex();
    if (crossIndex == 0) {
        result.Cross = PreselectionResult::Axes::HorizontalAxis;
    }
    else if (crossIndex == 1) {
        result.Cross = PreselectionResult::Axes::VerticalAxis;
    }

    result.Kind = PreselectionResult::HitKind::Axis;
    result.setPickedPoint(point);
    return true;
}

bool EditModeCoinManager::detectAxisPreselection(
    const SoPickedPointList& points,
    PreselectionResult& result
)
{
    for (int i = 0; i < points.getLength(); ++i) {
        SoPickedPoint* point = points[i];
        if (!point) {
            continue;
        }

        if (detectOriginPreselection(point, result)) {
            return true;
        }

        for (int layerIndex = 0; layerIndex < geometryLayerParameters.getCoinLayerCount();
             ++layerIndex) {
            PreselectionResult candidate;
            if (!detectPointPreselection(point, layerIndex, candidate)) {
                continue;
            }

            if (candidate.Kind == PreselectionResult::HitKind::Axis) {
                result = candidate;
                return true;
            }
        }

        if (detectAxisPreselection(point, result)) {
            return true;
        }
    }

    return false;
}

EditModeCoinManager::PreselectionResult EditModeCoinManager::detectPreselection(
    const SoPickedPointList& points,
    const SbVec2s& cursorPos,
    int hoveredPointIndex
)
{
    PreselectionResult result;

    result = detectConstraintPreselection(points, cursorPos);
    if (result.hasWinner()) {
        return result;
    }

    if (detectGeometryPreselection(points, cursorPos, hoveredPointIndex, result)) {
        return result;
    }

    if (detectAxisPreselection(points, result)) {
        return result;
    }

    return result;
}

SoGroup* EditModeCoinManager::getSelectedConstraints()
{
    SoGroup* group = new SoGroup();
    group->ref();

    for (int i = 0; i < editModeScenegraphNodes.constrGroup->getNumChildren(); i++) {
        if (ViewProviderSketchCoinAttorney::isConstraintSelected(viewProvider, i)) {
            SoSeparator* sep = pEditModeConstraintCoinManager->getConstraintIdSeparator(i);
            if (sep) {
                group->addChild(sep);
            }
        }
    }

    return group;
}

/***** update coin nodes *****/

void EditModeCoinManager::processGeometryConstraintsInformationOverlay(
    const GeoListFacade& geolistfacade,
    bool rebuildinformationlayer
)
{
    overlayParameters.rebuildInformationLayer = rebuildinformationlayer;

    pEditModeGeometryCoinManager->processGeometry(geolistfacade);

    updateOverlayParameters();

    processGeometryInformationOverlay(geolistfacade);

    pEditModeConstraintCoinManager->processConstraints(geolistfacade);
}

void EditModeCoinManager::updateOverlayParameters()
{
    if ((analysisResults.combRepresentationScale
         > (2 * overlayParameters.currentBSplineCombRepresentationScale))
        || (analysisResults.combRepresentationScale
            < (overlayParameters.currentBSplineCombRepresentationScale / 2))) {
        overlayParameters.currentBSplineCombRepresentationScale = analysisResults.combRepresentationScale;
    }
}

void EditModeCoinManager::processGeometryInformationOverlay(const GeoListFacade& geolistfacade)
{
    if (overlayParameters.rebuildInformationLayer) {
        // every time we start with empty information overlay
        Gui::coinRemoveAllChildren(editModeScenegraphNodes.infoGroup);
    }

    auto ioconv = EditModeInformationOverlayCoinConverter(
        viewProvider,
        editModeScenegraphNodes.infoGroup,
        overlayParameters,
        drawingParameters
    );

    // geometry information layer for bsplines, as they need a second round now that max curvature
    // is known
    for (auto geoid : analysisResults.bsplineGeoIds) {
        const Part::Geometry* geo = geolistfacade.getGeometryFromGeoId(geoid);

        ioconv.convert(geo, geoid);
    }
    for (auto geoid : analysisResults.arcGeoIds) {
        const Part::Geometry* geo = geolistfacade.getGeometryFromGeoId(geoid);
        ioconv.convert(geo, geoid);
    }


    overlayParameters.visibleInformationChanged = false;  // just updated
}

void EditModeCoinManager::updateAxesLength(const Base::BoundBox2d& bb)
{
    auto zCrossH = ViewProviderSketchCoinAttorney::getViewOrientationFactor(viewProvider)
        * drawingParameters.zCross;
    editModeScenegraphNodes.RootCrossCoordinate->point.set1Value(0, SbVec3f(bb.MinX, 0.0f, zCrossH));
    editModeScenegraphNodes.RootCrossCoordinate->point.set1Value(1, SbVec3f(bb.MaxX, 0.0f, zCrossH));
    editModeScenegraphNodes.RootCrossCoordinate->point.set1Value(2, SbVec3f(0.0f, bb.MinY, zCrossH));
    editModeScenegraphNodes.RootCrossCoordinate->point.set1Value(3, SbVec3f(0.0f, bb.MaxY, zCrossH));
}

void EditModeCoinManager::updateColor()
{
    auto geolistfacade = ViewProviderSketchCoinAttorney::getGeoListFacade(viewProvider);

    updateColor(geolistfacade);
}

void EditModeCoinManager::updateColor(const GeoListFacade& geolistfacade)
{
    bool sketchinvalid = ViewProviderSketchCoinAttorney::isSketchInvalid(viewProvider);

    pEditModeGeometryCoinManager->updateGeometryColor(geolistfacade, sketchinvalid);

    // update constraint color

    auto constraints = ViewProviderSketchCoinAttorney::getConstraints(viewProvider);

    if (ViewProviderSketchCoinAttorney::haveConstraintsInvalidGeometry(viewProvider)) {
        return;
    }

    pEditModeConstraintCoinManager->updateConstraintColor(constraints);
}

void EditModeCoinManager::setConstraintSelectability(bool enabled /* = true */)
{
    pEditModeConstraintCoinManager->setConstraintSelectability(enabled);
}


void EditModeCoinManager::updateGeometryLayersConfiguration()
{
    pEditModeGeometryCoinManager->updateGeometryLayersConfiguration();
}

void EditModeCoinManager::createEditModeInventorNodes()
{
    // 1 - Create the edit root node
    editModeScenegraphNodes.EditRoot = new SoSeparator;
    editModeScenegraphNodes.EditRoot->ref();  // Node is unref in the destructor of EditModeCoinManager
    editModeScenegraphNodes.EditRoot->setName("Sketch_EditRoot");
    ViewProviderSketchCoinAttorney::addNodeToRoot(viewProvider, editModeScenegraphNodes.EditRoot);
    editModeScenegraphNodes.EditRoot->renderCaching = SoSeparator::OFF;

    // Create Geometry Coin nodes ++++++++++++++++++++++++++++++++++++++
    pEditModeGeometryCoinManager->createEditModeInventorNodes();

    // stuff for the RootCross lines +++++++++++++++++++++++++++++++++++++++
    SoGroup* crossRoot = new Gui::SoSkipBoundingGroup;
    editModeScenegraphNodes.pickStyleAxes = new SoPickStyle();
    editModeScenegraphNodes.pickStyleAxes->style = SoPickStyle::SHAPE;
    crossRoot->addChild(editModeScenegraphNodes.pickStyleAxes);
    editModeScenegraphNodes.EditRoot->addChild(crossRoot);
    auto MtlBind = new SoMaterialBinding;
    MtlBind->setName("RootCrossMaterialBinding");
    MtlBind->value = SoMaterialBinding::PER_FACE;
    crossRoot->addChild(MtlBind);

    editModeScenegraphNodes.RootCrossDrawStyle = new SoDrawStyle;
    editModeScenegraphNodes.RootCrossDrawStyle->setName("RootCrossDrawStyle");
    editModeScenegraphNodes.RootCrossDrawStyle->lineWidth = 2 * drawingParameters.pixelScalingFactor;
    crossRoot->addChild(editModeScenegraphNodes.RootCrossDrawStyle);

    editModeScenegraphNodes.RootCrossMaterials = new SoMaterial;
    editModeScenegraphNodes.RootCrossMaterials->setName("RootCrossMaterials");
    editModeScenegraphNodes.RootCrossMaterials->diffuseColor.set1Value(0, drawingParameters.CrossColorH);
    editModeScenegraphNodes.RootCrossMaterials->diffuseColor.set1Value(1, drawingParameters.CrossColorV);
    crossRoot->addChild(editModeScenegraphNodes.RootCrossMaterials);

    editModeScenegraphNodes.RootCrossCoordinate = new SoCoordinate3;
    editModeScenegraphNodes.RootCrossCoordinate->setName("RootCrossCoordinate");
    crossRoot->addChild(editModeScenegraphNodes.RootCrossCoordinate);

    editModeScenegraphNodes.RootCrossSet = new SoLineSet;
    editModeScenegraphNodes.RootCrossSet->setName("RootCrossLineSet");
    crossRoot->addChild(editModeScenegraphNodes.RootCrossSet);

    // stuff for the Origin Point
    SoGroup* originPointRoot = new Gui::SoSkipBoundingGroup;
    originPointRoot->setName("OriginPointRoot_SkipBBox");
    editModeScenegraphNodes.EditRoot->addChild(originPointRoot);

    editModeScenegraphNodes.OriginPointMaterial = new SoMaterial;
    editModeScenegraphNodes.OriginPointMaterial->setName("OriginPointMaterial");
    originPointRoot->addChild(editModeScenegraphNodes.OriginPointMaterial);

    editModeScenegraphNodes.OriginPointDrawStyle = new SoDrawStyle;
    editModeScenegraphNodes.OriginPointDrawStyle->setName("OriginPointDrawStyle");
    editModeScenegraphNodes.OriginPointDrawStyle->pointSize = 8
        * drawingParameters.pixelScalingFactor;
    originPointRoot->addChild(editModeScenegraphNodes.OriginPointDrawStyle);

    editModeScenegraphNodes.OriginPointCoordinate = new SoCoordinate3;
    editModeScenegraphNodes.OriginPointCoordinate->setName("OriginPointCoordinate");
    // A default position, which will be updated later
    editModeScenegraphNodes.OriginPointCoordinate->point.set1Value(0, SbVec3f(0.0f, 0.0f, 0.0f));
    originPointRoot->addChild(editModeScenegraphNodes.OriginPointCoordinate);

    editModeScenegraphNodes.OriginPointSet = new SoMarkerSet;
    editModeScenegraphNodes.OriginPointSet->setName("OriginPointSet");
    editModeScenegraphNodes.OriginPointSet->markerIndex
        = Gui::Inventor::MarkerBitmaps::getMarkerIndex("CIRCLE_FILLED", drawingParameters.markerSize);
    originPointRoot->addChild(editModeScenegraphNodes.OriginPointSet);

    // stuff for the EditCurves +++++++++++++++++++++++++++++++++++++++
    SoSeparator* editCurvesRoot = new SoSeparator;
    editModeScenegraphNodes.EditRoot->addChild(editCurvesRoot);
    editModeScenegraphNodes.EditCurvesMaterials = new SoMaterial;
    editModeScenegraphNodes.EditCurvesMaterials->setName("EditCurvesMaterials");
    editCurvesRoot->addChild(editModeScenegraphNodes.EditCurvesMaterials);

    editModeScenegraphNodes.EditCurvesCoordinate = new SoCoordinate3;
    editModeScenegraphNodes.EditCurvesCoordinate->setName("EditCurvesCoordinate");
    editCurvesRoot->addChild(editModeScenegraphNodes.EditCurvesCoordinate);

    editModeScenegraphNodes.EditCurvesDrawStyle = new SoDrawStyle;
    editModeScenegraphNodes.EditCurvesDrawStyle->setName("EditCurvesDrawStyle");
    editModeScenegraphNodes.EditCurvesDrawStyle->lineWidth = 3
        * drawingParameters.pixelScalingFactor;  // Default value will be overridden in drawEdit()
    editCurvesRoot->addChild(editModeScenegraphNodes.EditCurvesDrawStyle);

    editModeScenegraphNodes.EditCurveSet = new SoLineSet;
    editModeScenegraphNodes.EditCurveSet->setName("EditCurveLineSet");
    editCurvesRoot->addChild(editModeScenegraphNodes.EditCurveSet);

    SoSeparator* autoConstraintExtensionHintRoot = new SoSeparator;
    editModeScenegraphNodes.EditRoot->addChild(autoConstraintExtensionHintRoot);

    SoPickStyle* autoConstraintExtensionHintPickStyle = new SoPickStyle;
    autoConstraintExtensionHintPickStyle->style = SoPickStyle::UNPICKABLE;
    autoConstraintExtensionHintRoot->addChild(autoConstraintExtensionHintPickStyle);

    editModeScenegraphNodes.LineExtensionAutoConstraintHintMaterials = new SoMaterial;
    editModeScenegraphNodes.LineExtensionAutoConstraintHintMaterials->setName(
        "LineExtensionAutoConstraintHintMaterials"
    );
    autoConstraintExtensionHintRoot->addChild(
        editModeScenegraphNodes.LineExtensionAutoConstraintHintMaterials
    );

    editModeScenegraphNodes.LineExtensionAutoConstraintHintCoordinate = new SoCoordinate3;
    editModeScenegraphNodes.LineExtensionAutoConstraintHintCoordinate->setName(
        "LineExtensionAutoConstraintHintCoordinate"
    );
    autoConstraintExtensionHintRoot->addChild(
        editModeScenegraphNodes.LineExtensionAutoConstraintHintCoordinate
    );

    editModeScenegraphNodes.LineExtensionAutoConstraintHintDrawStyle = new SoDrawStyle;
    editModeScenegraphNodes.LineExtensionAutoConstraintHintDrawStyle->setName(
        "LineExtensionAutoConstraintHintDrawStyle"
    );
    autoConstraintExtensionHintRoot->addChild(
        editModeScenegraphNodes.LineExtensionAutoConstraintHintDrawStyle
    );

    editModeScenegraphNodes.LineExtensionAutoConstraintHintSet = new SoLineSet;
    editModeScenegraphNodes.LineExtensionAutoConstraintHintSet->setName(
        "LineExtensionAutoConstraintHintLineSet"
    );
    autoConstraintExtensionHintRoot->addChild(
        editModeScenegraphNodes.LineExtensionAutoConstraintHintSet
    );

    // stuff for the EditMarkers +++++++++++++++++++++++++++++++++++++++
    SoSeparator* editMarkersRoot = new SoSeparator;
    editModeScenegraphNodes.EditRoot->addChild(editMarkersRoot);
    editModeScenegraphNodes.EditMarkersMaterials = new SoMaterial;
    editModeScenegraphNodes.EditMarkersMaterials->setName("EditMarkersMaterials");
    editMarkersRoot->addChild(editModeScenegraphNodes.EditMarkersMaterials);

    editModeScenegraphNodes.EditMarkersCoordinate = new SoCoordinate3;
    editModeScenegraphNodes.EditMarkersCoordinate->setName("EditMarkersCoordinate");
    editMarkersRoot->addChild(editModeScenegraphNodes.EditMarkersCoordinate);

    editModeScenegraphNodes.EditMarkersDrawStyle = new SoDrawStyle;
    editModeScenegraphNodes.EditMarkersDrawStyle->setName("EditMarkersDrawStyle");
    editModeScenegraphNodes.EditMarkersDrawStyle->pointSize = 8
        * drawingParameters.pixelScalingFactor;
    editMarkersRoot->addChild(editModeScenegraphNodes.EditMarkersDrawStyle);

    editModeScenegraphNodes.EditMarkerSet = new SoMarkerSet;
    editModeScenegraphNodes.EditMarkerSet->setName("EditMarkerSet");
    editModeScenegraphNodes.EditMarkerSet->markerIndex
        = Gui::Inventor::MarkerBitmaps::getMarkerIndex("CIRCLE_LINE", drawingParameters.markerSize);
    editMarkersRoot->addChild(editModeScenegraphNodes.EditMarkerSet);

    // stuff for the edit coordinates ++++++++++++++++++++++++++++++++++++++
    SoSeparator* Coordsep = new SoSeparator();
    SoPickStyle* ps = new SoPickStyle();
    ps->style.setValue(SoPickStyle::UNPICKABLE);
    Coordsep->addChild(ps);
    Coordsep->setName("CoordSeparator");
    // no caching for frequently-changing data structures
    Coordsep->renderCaching = SoSeparator::OFF;

    editModeScenegraphNodes.textMaterial = new SoMaterial;
    editModeScenegraphNodes.textMaterial->setName("CoordTextMaterials");
    editModeScenegraphNodes.textMaterial->diffuseColor = drawingParameters.CursorTextColor;
    Coordsep->addChild(editModeScenegraphNodes.textMaterial);

    editModeScenegraphNodes.textFont = new SoFont();
    editModeScenegraphNodes.textFont->name.setValue("Helvetica");
    editModeScenegraphNodes.textFont->size.setValue(drawingParameters.coinFontSize);

    Coordsep->addChild(editModeScenegraphNodes.textFont);

    editModeScenegraphNodes.textPos = new SoTranslation();
    Coordsep->addChild(editModeScenegraphNodes.textPos);

    editModeScenegraphNodes.textX = new SoText2();
    editModeScenegraphNodes.textX->justification = SoText2::LEFT;
    editModeScenegraphNodes.textX->string = "";
    Coordsep->addChild(editModeScenegraphNodes.textX);
    editModeScenegraphNodes.EditRoot->addChild(Coordsep);

    // coin nodes for the constraints +++++++++++++++++++++++++++++++++++++++++++++++++++
    pEditModeConstraintCoinManager->createEditModeInventorNodes();

    // group node for the Geometry information visual +++++++++++++++++++++++++++++++++++
    MtlBind = new SoMaterialBinding;
    MtlBind->setName("InformationMaterialBinding");
    MtlBind->value = SoMaterialBinding::OVERALL;
    editModeScenegraphNodes.EditRoot->addChild(MtlBind);

    // use small line width for the information visual
    editModeScenegraphNodes.InformationDrawStyle = new SoDrawStyle;
    editModeScenegraphNodes.InformationDrawStyle->setName("InformationDrawStyle");
    editModeScenegraphNodes.InformationDrawStyle->lineWidth = 1
        * drawingParameters.pixelScalingFactor;
    editModeScenegraphNodes.EditRoot->addChild(editModeScenegraphNodes.InformationDrawStyle);

    // add the group where all the information entity has its SoSeparator
    editModeScenegraphNodes.infoGroup = new SoGroup();
    editModeScenegraphNodes.infoGroup->setName("InformationGroup");
    editModeScenegraphNodes.EditRoot->addChild(editModeScenegraphNodes.infoGroup);
}

void EditModeCoinManager::redrawViewProvider()
{
    viewProvider.draw(false, false);
}
void EditModeCoinManager::setEditDrawStyle(GeometryCreationMode mode)
{
    SoDrawStyle* toCopy = nullptr;

    switch (mode) {
        case GeometryCreationMode::Normal:
            toCopy = editModeScenegraphNodes.CurvesDrawStyle;
            break;
        case GeometryCreationMode::Construction:
            toCopy = editModeScenegraphNodes.CurvesConstructionDrawStyle;
            break;
    }

    editModeScenegraphNodes.EditCurvesDrawStyle->lineWidth = toCopy->lineWidth;
    editModeScenegraphNodes.EditCurvesDrawStyle->linePattern = toCopy->linePattern;
    editModeScenegraphNodes.EditCurvesDrawStyle->linePatternScaleFactor = toCopy->linePatternScaleFactor;
}

void EditModeCoinManager::updateElementSizeParameters()
{
    // Add scaling to Constraint icons
    ParameterGrp::handle hGrp = App::GetApplication().GetParameterGroupByPath(
        "User parameter:BaseApp/Preferences/View"
    );

    double viewScalingFactor = hGrp->GetFloat("ViewScalingFactor", 1.0);
    viewScalingFactor = std::clamp<double>(viewScalingFactor, 0.5, 5.0);

    int markerSize = hGrp->GetInt("MarkerSize", 7);

    drawingParameters.labelFontName = QString::fromStdString(
        hGrp->GetASCII("EditSketcherFontName", "")
    );

    int defaultFontSizePixels = defaultApplicationFontSizePixels();  // returns height in pixels,
                                                                     // not points

    int sketcherfontSize = hGrp->GetInt("EditSketcherFontSize", defaultFontSizePixels);
    int constraintSymbolSizePref = hGrp->GetInt("ConstraintSymbolSize", defaultFontSizePixels);
    drawingParameters.constraintIconHitPaddingPx = hGrp->GetInt("ConstraintIconHitPadding", 3);

    double dpi = getApplicationLogicalDPIX();
    double devicePixelRatio = getDevicePixelRatio();

    // simple scaling factor for hardcoded pixel values in the Sketcher
    drawingParameters.pixelScalingFactor = devicePixelRatio * viewScalingFactor;

    // About sizes:
    // SoDatumLabel takes the size in points, not in pixels. This is because it uses QFont
    // internally. Coin, at least our coin at this time, takes pixels, not points.

    drawingParameters.coinFontSize = std::lround(
        sketcherfontSize * devicePixelRatio
    );  // in pixels (Coin uses pixels)
    drawingParameters.labelFontSize = std::lround(
        sketcherfontSize * devicePixelRatio * 72.0f / dpi
    );  // in points (SoDatumLabel uses points)
    drawingParameters.constraintIconSize = constraintSymbolSizePref;

    auto supportedsizes = Gui::Inventor::MarkerBitmaps::getSupportedSizes("CIRCLE_LINE");
    auto scaledMarkerSize = std::lround(markerSize * devicePixelRatio);
    auto const it = std::lower_bound(supportedsizes.begin(), supportedsizes.end(), scaledMarkerSize);
    if (it != supportedsizes.end()) {
        scaledMarkerSize = *it;
    }
    else {
        // This is a quick and dirty fix for https://github.com/FreeCAD/FreeCAD/issues/22010
        //
        // Basically if we want to use a bigger marker size than available, we use the biggest one
        // available. This is not a good way to fix the issue - we should ensure that the marker
        // size that the user requests is actually available - this, however, requires more
        // significant changes to the code.
        scaledMarkerSize = *supportedsizes.rbegin();
    }
    drawingParameters.markerSize = scaledMarkerSize;

    updateInventorNodeSizes();
}

/************************ Delegated constraint public interface **********/

// public function that triggers drawing of most constraint icons
void EditModeCoinManager::drawConstraintIcons()
{
    pEditModeConstraintCoinManager->drawConstraintIcons();
}

void EditModeCoinManager::drawConstraintIcons(const GeoListFacade& geolistfacade)
{
    pEditModeConstraintCoinManager->drawConstraintIcons(geolistfacade);
}

void EditModeCoinManager::updateVirtualSpace()
{
    pEditModeConstraintCoinManager->updateVirtualSpace();
}

/************************ Resizing of coin nodes ************************/

int EditModeCoinManager::defaultApplicationFontSizePixels() const
{
    return ViewProviderSketchCoinAttorney::defaultApplicationFontSizePixels(viewProvider);
}

double EditModeCoinManager::getDevicePixelRatio() const
{
    return ViewProviderSketchCoinAttorney::getDevicePixelRatio(viewProvider);
}

int EditModeCoinManager::getApplicationLogicalDPIX() const
{
    return ViewProviderSketchCoinAttorney::getApplicationLogicalDPIX(viewProvider);
}

void EditModeCoinManager::updateInventorNodeSizes()
{
    auto layersconfiguration = viewProvider.VisualLayerList.getValues();

    updateInventorWidths();

    for (int l = 0; l < geometryLayerParameters.getCoinLayerCount(); l++) {
        editModeScenegraphNodes.PointsDrawStyle[l]->pointSize = 8
            * drawingParameters.pixelScalingFactor;
        editModeScenegraphNodes.PointSet[l]->markerIndex = Gui::Inventor::MarkerBitmaps::getMarkerIndex(
            "CIRCLE_FILLED",
            drawingParameters.markerSize
        );
    }

    editModeScenegraphNodes.OriginPointDrawStyle->pointSize = 8
        * drawingParameters.pixelScalingFactor;
    editModeScenegraphNodes.OriginPointSet->markerIndex
        = Gui::Inventor::MarkerBitmaps::getMarkerIndex("CIRCLE_FILLED", drawingParameters.markerSize);

    editModeScenegraphNodes.RootCrossDrawStyle->lineWidth = 2 * drawingParameters.pixelScalingFactor;
    editModeScenegraphNodes.EditCurvesDrawStyle->lineWidth = 3 * drawingParameters.pixelScalingFactor;
    editModeScenegraphNodes.EditMarkersDrawStyle->pointSize = 8
        * drawingParameters.pixelScalingFactor;
    editModeScenegraphNodes.EditMarkerSet->markerIndex
        = Gui::Inventor::MarkerBitmaps::getMarkerIndex("CIRCLE_LINE", drawingParameters.markerSize);
    editModeScenegraphNodes.ConstraintDrawStyle->lineWidth = 1 * drawingParameters.pixelScalingFactor;
    editModeScenegraphNodes.InformationDrawStyle->lineWidth = 1
        * drawingParameters.pixelScalingFactor;

    editModeScenegraphNodes.textFont->size.setValue(drawingParameters.coinFontSize);

    pEditModeConstraintCoinManager->rebuildConstraintNodes();
}

void EditModeCoinManager::updateInventorWidths()
{
    editModeScenegraphNodes.CurvesDrawStyle->lineWidth = drawingParameters.CurveWidth
        * drawingParameters.pixelScalingFactor;
    editModeScenegraphNodes.CurvesConstructionDrawStyle->lineWidth = drawingParameters.ConstructionWidth
        * drawingParameters.pixelScalingFactor;
    editModeScenegraphNodes.CurvesInternalDrawStyle->lineWidth = drawingParameters.InternalWidth
        * drawingParameters.pixelScalingFactor;
    editModeScenegraphNodes.CurvesExternalDrawStyle->lineWidth = drawingParameters.ExternalWidth
        * drawingParameters.pixelScalingFactor;
    editModeScenegraphNodes.CurvesExternalDefiningDrawStyle->lineWidth
        = drawingParameters.ExternalDefiningWidth * drawingParameters.pixelScalingFactor;
}

void EditModeCoinManager::updateInventorPatterns()
{
    editModeScenegraphNodes.CurvesDrawStyle->linePattern = drawingParameters.CurvePattern;
    editModeScenegraphNodes.CurvesConstructionDrawStyle->linePattern
        = drawingParameters.ConstructionPattern;
    editModeScenegraphNodes.CurvesInternalDrawStyle->linePattern = drawingParameters.InternalPattern;
    editModeScenegraphNodes.CurvesExternalDrawStyle->linePattern = drawingParameters.ExternalPattern;
    editModeScenegraphNodes.CurvesExternalDefiningDrawStyle->linePattern
        = drawingParameters.ExternalDefiningPattern;
}

void EditModeCoinManager::updateInventorColors()
{
    editModeScenegraphNodes.RootCrossMaterials->diffuseColor.set1Value(0, drawingParameters.CrossColorH);
    editModeScenegraphNodes.RootCrossMaterials->diffuseColor.set1Value(1, drawingParameters.CrossColorV);
    editModeScenegraphNodes.textMaterial->diffuseColor = drawingParameters.CursorTextColor;
}

/************************ Edit node access ************************/

SoSeparator* EditModeCoinManager::getRootEditNode()
{
    return editModeScenegraphNodes.EditRoot;
}
