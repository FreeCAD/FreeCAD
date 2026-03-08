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

#pragma once

#include <map>
#include <vector>

#include <QString>

#include <Inventor/SbColor.h>
#include <Inventor/nodes/SoCoordinate3.h>
#include <Inventor/nodes/SoDrawStyle.h>
#include <Inventor/nodes/SoFont.h>
#include <Inventor/nodes/SoGroup.h>
#include <Inventor/nodes/SoLineSet.h>
#include <Inventor/nodes/SoMarkerSet.h>
#include <Inventor/nodes/SoMaterial.h>
#include <Inventor/nodes/SoPickStyle.h>
#include <Inventor/nodes/SoText2.h>
#include <Inventor/nodes/SoTranslation.h>

#include <Base/Color.h>
#include <Gui/ViewParams.h>
#include <Gui/Inventor/SmSwitchboard.h>
#include <Mod/Sketcher/App/GeoList.h>

#include "ViewProviderSketchGeometryExtension.h"


namespace Part
{
class Geometry;
}

namespace SketcherGui
{

/** @brief      Struct for storing local drawing parameters
 *
 * Parameters based on user preferenced are auto loaded by EditCoinManager observer nested class.
 */
struct DrawingParameters
{
    /// Defines the number of segments to use to draw a curved edge
    int curvedEdgeCountSegments;

    /** @name Rendering Heights - virtual height introduced in the scenegraph to determine what is
     * drawn on top of what*/
    //@{
    const float zEdit = 0.002f;        // Height used by temporal edit curves
    const float zCross = 0.002f;       // Height used by the Axes
    const float zInfo = 0.004f;        // Height used by the Overlay information layer
    const float zLowLines = 0.005f;    // Height used for bottom rendered lines
    const float zMidLines = 0.006f;    // Height used for in-the-middle rendered lines
    const float zHighLines = 0.007f;   // Height used for on top rendered lines
    const float zHighLine = 0.008f;    // Height for highlighted lines (selected/preselected)
    const float zConstr = 0.004f;      // Height for rendering constraints
    const float zRootPoint = 0.010f;   // Height used for rendering the root point
    const float zLowPoints = 0.011f;   // Height used for bottom rendered points
    const float zMidPoints = 0.012f;   // Height used for mid rendered points
    const float zHighPoints = 0.013f;  // Height used for top rendered points
    const float zHighlight = 0.014f;   // Height for highlighted points (selected/preselected)
    const float zText = 0.014f;        // Height for rendered text
    //@}

    /// Different categories of geometries that can be selected by the user to be rendered on top,
    /// in the middle or in the bottom
    enum class GeometryRendering
    {
        NormalGeometry = 1,
        Construction = 2,
        ExternalGeometry = 3
    };

    /** @name Rendering Order - defining what should be rendered on top and in the middle*/
    //@{
    GeometryRendering topRenderingGeometry = GeometryRendering::NormalGeometry;
    GeometryRendering midRenderingGeometry = GeometryRendering::Construction;
    //@}

    /** @name Rendering Coin Colors **/
    //@{
    static SbColor InformationColor;                       // Information Overlay Color
    static SbColor CrossColorH;                            // Color for the Horizontal Axis
    static SbColor CrossColorV;                            // Color for the Vertical Axis
    static SbColor InvalidSketchColor;                     // Color for rendering an invalid sketch
    static SbColor FullyConstrainedColor;                  // Color for a fully constrained sketch
    static SbColor FullyConstraintInternalAlignmentColor;  // Color for fully constrained internal
                                                           // alignment geometry
    static SbColor InternalAlignedGeoColor;  // Color for non-fully constrained internal geometry
    static SbColor FullyConstraintElementColor;  // Color for a fully constrained element
    static SbColor CurveColor;                   // Color for curves
    static SbColor PreselectColor;               // Color used for preselection
    static SbColor PreselectSelectedColor;  // Color used for preselection when geometry is already
                                            // selected
    static SbColor SelectColor;             // Color used for selected geometry
    static SbColor CurveExternalColor;      // Color used for external geometry
    static SbColor CurveExternalDefiningColor;  // Color used for external defining geometry
    static SbColor CurveDraftColor;             // Color used for construction geometry
    static SbColor FullyConstraintConstructionElementColor;  // Color used for a fully constrained
                                                             // construction element
    static SbColor ConstrDimColor;            // Color used for a dimensional constraints
    static SbColor ConstrIcoColor;            // Color used for constraint icons
    static SbColor NonDrivingConstrDimColor;  // Color used for non-driving (reference) dimensional
                                              // constraints
    static SbColor ExprBasedConstrDimColor;  // Color used for expression based dimensional constraints
    static SbColor DeactivatedConstrDimColor;  // Color used for deactivated dimensional constraints
    static SbColor CursorTextColor;            // Color used by the edit mode cursor
    //@}

    /** @name Rendering sizes (also to support HDPI monitors) **/
    //@{
    double pixelScalingFactor = 1.0;  // Scaling factor to be used for pixels
    int coinFontSize = 17;            // Font size to be used by coin
    int labelFontSize = 17;  // Font size to be used by SoDatumLabel, which uses a QPainter and a
                             // QFont internally
    int constraintIconSize = 15;  // Size of constraint icons
    int markerSize = 7;           // Size used for markers

    int CurveWidth = 2;             // width of normal edges
    int ConstructionWidth = 1;      // width of construction edges
    int InternalWidth = 1;          // width of internal edges
    int ExternalWidth = 1;          // width of external edges
    int ExternalDefiningWidth = 1;  // width of external defining edges

    unsigned int CurvePattern = 0b1111111111111111;             // pattern of normal edges
    unsigned int ConstructionPattern = 0b1111110011111100;      // pattern of construction edges
    unsigned int InternalPattern = 0b1111110011111100;          // pattern of internal edges
    unsigned int ExternalPattern = 0b1111110011111100;          // pattern of external edges
    unsigned int ExternalDefiningPattern = 0b1111111111111111;  // pattern of external defining edges
    //@}

    DrawingParameters()
    {
        unsigned long colorLong;
        Base::Color color;

        colorLong = Gui::ViewParams::instance()->getAxisXColor();
        color = Base::Color(static_cast<uint32_t>(colorLong));
        CrossColorH = SbColor(color.r, color.g, color.b);

        colorLong = Gui::ViewParams::instance()->getAxisYColor();
        color = Base::Color(static_cast<uint32_t>(colorLong));
        CrossColorV = SbColor(color.r, color.g, color.b);
    }
};

/** @brief      Struct for storing references to the scenegraph nodes necessary for geometry layers
 */
struct GeometryLayerNodes
{
    /** @name Point nodes*/
    //@{
    std::vector<SoMaterial*>& PointsMaterials;      // The materials for the points/vertices
    std::vector<SoCoordinate3*>& PointsCoordinate;  // The coordinates of the points/vertices
    //@}

    /** @name Curve nodes*/
    //@{
    std::vector<std::vector<SoMaterial*>>& CurvesMaterials;
    std::vector<std::vector<SoCoordinate3*>>& CurvesCoordinate;
    std::vector<std::vector<SoLineSet*>>& CurveSet;
    //@}
};

/** @brief
 * Helper class to store together a field index of a coin multifield object and the coin geometry
 * layer to which it belongs.
 *
 * @details
 *
 * @warning the layer is * not * the logical layer (the one of GeometryFacade), but the coin layer.
 * See GeometryLayerParameters.
 *
 * Overloaded operators and specialisation of std::less enable it to be used in containers including
 * ordered containers.
 */
class MultiFieldId
{
public:
    explicit constexpr MultiFieldId(int fieldindex = -1, int layerid = 0, int geotypeid = 0)
        : fieldIndex(fieldindex)
        , layerId(layerid)
        , geoTypeId(geotypeid)
    {}

    MultiFieldId(const MultiFieldId&) = default;
    MultiFieldId& operator=(const MultiFieldId&) = default;

    MultiFieldId(MultiFieldId&&) = default;
    MultiFieldId& operator=(MultiFieldId&&) = default;

    inline bool operator==(const MultiFieldId& obj) const
    {
        return this->fieldIndex == obj.fieldIndex && this->layerId == obj.layerId
            && this->geoTypeId == obj.geoTypeId;
    }

    inline bool operator!=(const MultiFieldId& obj) const
    {
        return this->fieldIndex != obj.fieldIndex || this->layerId != obj.layerId
            || this->geoTypeId != obj.geoTypeId;
    }

    int fieldIndex = -1;
    int layerId = 0;
    int geoTypeId = 0;

    static const MultiFieldId Invalid;
};


}  // namespace SketcherGui

namespace std
{
template<>
struct less<SketcherGui::MultiFieldId>
{
    bool operator()(const SketcherGui::MultiFieldId& lhs, const SketcherGui::MultiFieldId& rhs) const
    {
        return (lhs.layerId != rhs.layerId)
            ? (lhs.layerId < rhs.layerId)
            : (static_cast<int>(lhs.fieldIndex) < static_cast<int>(rhs.fieldIndex));
    }
};
}  // namespace std


namespace SketcherGui
{

/** @brief
 * Helper class to store geometry layers configuration
 *
 * CoinLayers is the number of * Coin * Geometry Layers. This
 * is *not* necessarily the number of Geometry layers (logical layers).
 *
 * Logical layers (as stored in GeometryFacade) define a grouping of geometries.
 * However, this does not mean that they are represented in coin in different layers.
 * Only when there is a reason for such mapping is done so. For example, when the
 * geometry have different drawing parameters.
 *
 * This means that there may be:
 * 1. A N:N relationship between logical layers and coin layers.
 * 2. A M:N relationship between logical layers and coin layers (M<N), including a 1:N relationship.
 */
class GeometryLayerParameters
{

public:
    GeometryLayerParameters()
    {
        reset();
    }

private:
    enum CoinLayer
    {
        Default = 0
    };


public:
    enum class SubLayer
    {
        Normal = 0,
        Construction = 1,
        Internal = 2,
        External = 3,
        ExternalDefining = 4
    };

    void reset()
    {
        CoinLayers = 1;
    }

    inline int getSafeCoinLayer(int coinlayer)
    {
        if (coinlayer < CoinLayers) {
            return coinlayer;
        }

        return Default;
    }

    int inline getCoinLayerCount() const
    {
        return CoinLayers;
    }

    void setCoinLayerCount(int layernumber)
    {
        CoinLayers = layernumber;
    }

    int inline getSubLayerCount() const
    {
        return SubLayers;
    }

    int getSubLayerIndex(const int geoId, const Sketcher::GeometryFacade* geom) const;

    bool isNormalSubLayer(int t) const
    {
        return t == static_cast<int>(SubLayer::Normal);
    }

    bool isConstructionSubLayer(int t) const
    {
        return t == static_cast<int>(SubLayer::Construction);
    }

    bool isInternalSubLayer(int t) const
    {
        return t == static_cast<int>(SubLayer::Internal);
    }

    bool isExternalSubLayer(int t) const
    {
        return t == static_cast<int>(SubLayer::External);
    }

    bool isExternalDefiningSubLayer(int t) const
    {
        return t == static_cast<int>(SubLayer::ExternalDefining);
    }


private:
    int CoinLayers = 1;  // defaults to a single Coin Geometry Layer.
    int SubLayers = 5;   // Normal, Construction, Internal, External.
};

/** @brief     Struct to hold the results of analysis performed on geometry
 */
struct AnalysisResults
{                                         // TODO: This needs to be refactored
    double combRepresentationScale = 0;   // used for information overlay (BSpline comb)
    float boundingBoxMagnitudeOrder = 0;  // used for grid extension
    std::vector<int> bsplineGeoIds;       // used for information overlay
    std::vector<int> arcGeoIds;
};

/** @brief      Struct adapted to store the parameters necessary to create and update
 *  the information overlay layer.
 */
struct OverlayParameters
{
    bool rebuildInformationLayer = false;
    bool visibleInformationChanged = true;
    double currentBSplineCombRepresentationScale = 0;

    // Parameters (auto loaded by EditCoinManager observer nested class)
    bool bSplineDegreeVisible;
    bool bSplineControlPolygonVisible;
    bool bSplineCombVisible;
    bool bSplineKnotMultiplicityVisible;
    bool bSplinePoleWeightVisible;
    bool arcCircleHelperVisible;
};

/** @brief      Struct adapted to store the parameters necessary to create and update
 *  constraints.
 */
struct ConstraintParameters
{
    bool bHideUnits;            // whether units should be hidden or not
    bool bShowDimensionalName;  // whether the name of dimensional constraints should be shown or not
    QString sDimensionalStringFormat;  // how to code strings of dimensional constraints
};

/** @brief      Helper struct adapted to store the pointer to edit mode scenegraph objects.
 */
struct EditModeScenegraphNodes
{
    /** @name Point nodes*/
    //@{
    SoSeparator* EditRoot;
    SmSwitchboard* PointsGroup;
    std::vector<SoMaterial*> PointsMaterials;
    std::vector<SoCoordinate3*> PointsCoordinate;
    std::vector<SoDrawStyle*> PointsDrawStyle;
    std::vector<SoMarkerSet*> PointSet;
    //@}

    /** @name Origin Point nodes*/
    //@{
    SoMaterial* OriginPointMaterial;
    SoCoordinate3* OriginPointCoordinate;
    SoMarkerSet* OriginPointSet;
    SoDrawStyle* OriginPointDrawStyle;
    //@}

    /** @name Curve nodes*/
    //@{
    SmSwitchboard* CurvesGroup;
    std::vector<std::vector<SoMaterial*>> CurvesMaterials;
    std::vector<std::vector<SoCoordinate3*>> CurvesCoordinate;
    std::vector<std::vector<SoLineSet*>> CurveSet;
    SoDrawStyle* CurvesDrawStyle;
    SoDrawStyle* CurvesConstructionDrawStyle;
    SoDrawStyle* CurvesInternalDrawStyle;
    SoDrawStyle* CurvesExternalDrawStyle;
    SoDrawStyle* CurvesExternalDefiningDrawStyle;
    SoDrawStyle* HiddenCurvesDrawStyle;
    //@}

    /** @name Axes nodes*/
    //@{
    SoMaterial* RootCrossMaterials;
    SoCoordinate3* RootCrossCoordinate;
    SoLineSet* RootCrossSet;
    SoDrawStyle* RootCrossDrawStyle;
    //@}

    /** @name Temporal edit curve nodes - For geometry creation */
    //@{
    SoMaterial* EditCurvesMaterials;
    SoCoordinate3* EditCurvesCoordinate;
    SoLineSet* EditCurveSet;
    SoDrawStyle* EditCurvesDrawStyle;
    SoPickStyle* pickStyleAxes;
    //@}

    /** @name Temporal edit markers nodes- For operation rendering, such as trimming green circles*/
    //@{
    SoMaterial* EditMarkersMaterials;
    SoCoordinate3* EditMarkersCoordinate;
    SoMarkerSet* EditMarkerSet;
    SoDrawStyle* EditMarkersDrawStyle;
    //@}

    /** @name Temporal edit text nodes*/
    //@{
    SoText2* textX;
    SoTranslation* textPos;
    SoFont* textFont;
    SoMaterial* textMaterial;
    //@}

    /** @name Constraint nodes*/
    //@{
    SmSwitchboard* constrGroup;
    SoPickStyle* constrGrpSelect;
    SoDrawStyle* ConstraintDrawStyle;
    //@}

    /** @name Information Overlay Layer nodes*/
    //@{
    SoGroup* infoGroup;
    SoDrawStyle* InformationDrawStyle;
    //@}
};

/** @brief      Helper struct containing index conversions (mappings) between
 * {GeoId, PointPos} and MF indices per layer, sublayers, and VertexId and MF indices per layer.
 *
 * These are updated with every draw of the scenegraph.
 */
struct CoinMapping
{

    void clear()
    {
        for (size_t l = 0; l < CurvIdToGeoId.size(); ++l) {
            CurvIdToGeoId[l].clear();
        }
        CurvIdToGeoId.clear();
        PointIdToGeoId.clear();
        PointIdToPosId.clear();
        GeoElementId2SetId.clear();
        PointIdToVertexId.clear();
    };

    /// given the MF index of a curve and the coin layer in which it is drawn returns the GeoId of
    /// the curve
    int getCurveGeoId(int curveindex, int layerindex, int sublayerindex = 0)
    {
        return CurvIdToGeoId[layerindex][sublayerindex][curveindex];
    }

    bool isValidCurveId(int curveindex, int layerindex, int sublayerindex = 0) const
    {
        // clang-format off
        return static_cast<int>(CurvIdToGeoId.size()) > layerindex &&
               static_cast<int>(CurvIdToGeoId[layerindex].size()) > sublayerindex &&
               static_cast<int>(CurvIdToGeoId[layerindex][sublayerindex].size()) > curveindex;
        // clang-format on
    }

    /// given the MF index of a point and the coin layer in which it is drawn returns the GeoId of
    /// the point
    int getPointGeoId(int pointindex, int layerindex)
    {
        return PointIdToGeoId[layerindex][pointindex];
    }

    bool isValidPointId(int pointindex, int layerindex) const
    {
        // clang-format off
        return static_cast<int>(PointIdToGeoId.size()) > layerindex &&
               static_cast<int>(PointIdToGeoId[layerindex].size()) > pointindex;
        // clang-format on
    }

    /// given the MF index of a point and the coin layer in which it is drawn returns the PosId of
    /// the point
    Sketcher::PointPos getPointPosId(int pointindex, int layerindex)
    {
        return PointIdToPosId[layerindex][pointindex];
    }
    /// given the MF index of a point and the coin layer in which it is drawn returns the VertexId
    /// of the point
    int getPointVertexId(int pointindex, int layerindex)
    {
        return PointIdToVertexId[layerindex][pointindex];
    }

    /// given the {GeoId, PointPos} of a curve or point, returns MultiFieldId containing the MF
    /// index and the coin layer of the curve or point
    MultiFieldId getIndexLayer(int geoid, Sketcher::PointPos pos)
    {
        if (geoid == -1) {
            return MultiFieldId(-1, 0, 0);
        }

        auto indexit = GeoElementId2SetId.find(Sketcher::GeoElementId(geoid, pos));

        if (indexit != GeoElementId2SetId.end()) {
            return indexit->second;
        }

        return MultiFieldId::Invalid;
    }

    /// given the VertexId of a point, returns MultiFieldId containing the MF index and the coin
    /// layer of the point
    MultiFieldId getIndexLayer(int vertexId)
    {
        if (vertexId == -1) {
            return MultiFieldId(-1, 0, 0);
        }

        for (size_t l = 0; l < PointIdToVertexId.size(); l++) {

            if (auto indexit = std::ranges::find(PointIdToVertexId[l], vertexId);
                indexit != PointIdToVertexId[l].end()) {
                return MultiFieldId(std::distance(PointIdToVertexId[l].begin(), indexit), l);
            }
        }

        return MultiFieldId::Invalid;
    }

    //* These map a MF index (second index) within a coin layer (first index) for points or curves
    // to a GeoId */
    std::vector<std::vector<std::vector<int>>> CurvIdToGeoId;  // conversion of SoLineSet index to GeoId
    std::vector<std::vector<int>> PointIdToGeoId;  // conversion of SoCoordinate3 index to GeoId
    std::vector<std::vector<Sketcher::PointPos>> PointIdToPosId;  // SoCoordinate3 index to PosId

    //* This maps an MF index (second index) of a point within a coin layer (first index) to a
    // global VertexId */
    std::vector<std::vector<int>> PointIdToVertexId;

    /// This maps GeoElementId index {GeoId, PointPos} to a {coin layer and MF index} of a curve or
    /// point.
    std::map<Sketcher::GeoElementId, MultiFieldId> GeoElementId2SetId;
};

}  // namespace SketcherGui
