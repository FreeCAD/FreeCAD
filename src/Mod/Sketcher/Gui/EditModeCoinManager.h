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

#include <cstdint>
#include <functional>
#include <optional>
#include <vector>

#include <App/Application.h>
#include <Base/Vector3D.h>
#include <Mod/Sketcher/App/GeoList.h>
#include "GeometryCreationMode.h"

#include "EditModeCoinManagerParameters.h"


class SbVec3f;
class SbVec2s;
class SoRayPickAction;
class SoPickedPoint;
class SoPickedPointList;
class SbVec3s;

namespace Base
{
template<typename T>
class Vector3;

class Vector2d;

class BoundBox2d;

class Placement;
}  // namespace Base

namespace Part
{
class Geometry;
}

namespace Sketcher
{
class Constraint;
class PropertyConstraintList;
};  // namespace Sketcher

namespace SketcherGui
{

class ViewProviderSketch;
class EditModeConstraintCoinManager;
class EditModeGeometryCoinManager;
class LazyExternalGeometryLayer;

using GeoList = Sketcher::GeoList;
using GeoListFacade = Sketcher::GeoListFacade;

/** @brief      Helper class for managing the Coin nodes of ViewProviderSketch.
 *  @details
 *
 * Given the substantial amount of code involved in coin node management, EditModeCoinManager
 * further delegates on other specialised helper classes. Some of them share the
 * ViewProviderSketchCoinAttorney, which defines the maximum coupling and minimum encapsulation.
 *
 * The most important such delegates are: EditModeGeometryCoinManager and
 * EditModeConstraintCoinManager.
 *
 * EditModeCoinManager takes over the responsibility of creating the Coin (Inventor) scenegraph
 * and modifying it, including all the drawing of geometry, constraints and overlay layer. This
 * is an exclusive responsibility under the Single Responsibility Principle.
 *
 * EditModeCoinManager exposes a public interface to be used by ViewProviderSketch. Where,
 * EditModeCoinManager needs special access to facilities of ViewProviderSketch in order to fulfil
 * its responsibility, this access is defined by ViewProviderSketchCoinAttorney.
 *
 * EditModeCoinManager is responsible, under the Single Responsibility Principle, to manage the coin
 * EditRoot node. This node is ref-ed on creation and unref-ed on destruction to ensure that its
 * lifetime matches the one of EditModeCoinManager.
 *
 * EditRoot is added on request to pcRoot by ViewProviderSketch. The node pcRoot belongs, under the
 * Single Responsibility Principle, to ViewProviderSketch. EditModeCoinManager delegates addition
 * and removal of child notes of EditRoot to specialised helper classes.
 *
 * EditModeCoinManager is designed to define the span of time in which ViewProviderSketch is in edit
 * mode.
 *
 * In addition to the scenegraph, EditModeCoinManager is responsible for keeping any necessary
 * mapping between indices used at ViewProviderSketch level, and internal indexing used by
 * EditModeCoinManager and its subclasses.
 */
class SketcherGuiExport EditModeCoinManager
{
    /** @brief      Class for monitoring changes in parameters affecting drawing and coin node
     * generation
     *  @details
     *
     * This nested class is a helper responsible for attaching to the parameters relevant for
     * EditModeCoinManager and its helpers, initialising the EditModeCoinManager to the current
     * configuration and handle in real time any change to their values.
     */
    class ParameterObserver: public ParameterGrp::ObserverType
    {
    private:
        enum class OverlayVisibilityParameter
        {
            BSplineDegree,
            BSplineControlPolygonVisible,
            BSplineCombVisible,
            BSplineKnotMultiplicityVisible,
            BSplinePoleWeightVisible,
            ArcCircleHelperVisible
        };

    public:
        explicit ParameterObserver(EditModeCoinManager& client);
        ~ParameterObserver() override;

        void subscribeToParameters();

        void unsubscribeToParameters();

        /** Observer for parameter group. */
        void OnChange(Base::Subject<const char*>& rCaller, const char* sReason) override;

    private:
        void initParameters();
        void updateCurvedEdgeCountSegmentsParameter(const std::string& parametername);
        void updateLineRenderingOrderParameters(const std::string& parametername);
        void updateAxisTransparencyParameter(const std::string& parametername);
        void updateOccludedAxisTransparencyParameter(const std::string& parametername);
        void updateConstraintPresentationParameters(const std::string& parametername);
        void updateWidth(int& width, const std::string& parametername, int def);
        void updatePattern(unsigned int& pattern, const std::string& pname, unsigned int def);
        void updateColor(SbColor& sbcolor, const std::string& parametername);
        void updateUnit(const std::string& parametername);

        template<OverlayVisibilityParameter visibilityparameter>
        void updateOverlayVisibilityParameter(const std::string& parametername);

    private:
        std::map<std::string, std::function<void(const std::string&)>> str2updatefunction;
        EditModeCoinManager& Client;
    };

public:
    /** @brief This struct defines the information provided to other classes about preselection.
     *
     * @details
     *
     * PointIndex: Only Positive values corresponding to VertexId (are positive for both normal and
     * external geometry) GeoIndex: Same values as GeoId of GeoElementId, except for axes which are
     * not included. -1 represents an invalid curve.
     *
     * In other words valid values are 0,1,2,... for normal geometry and -3,-4,-5,... for external
     * geometry
     *
     * Cross: Axes and RootPoint values as defined in the enum class.
     *
     */
    struct PreselectionResult
    {
        enum class HitKind : std::int8_t
        {
            None = -1,
            Point = 0,
            Edge = 1,
            Axis = 2,
            Constraint = 3
        };

        enum class ConstraintHitKind : std::uint8_t
        {
            None,
            Icon,
            DatumLabel
        };

        enum SpecialValues
        {
            InvalidPoint = -1,
            InvalidCurve = -1,
            ExternalCurve = -3,
            InvalidLazyExternalId = InvalidPoint
        };

        enum class Axes
        {
            None = -1,
            RootPoint = 0,
            HorizontalAxis = 1,
            VerticalAxis = 2
        };

        HitKind Kind = HitKind::None;
        int PointIndex = InvalidPoint;
        int GeoIndex = InvalidCurve;  // valid values are 0,1,2,... for normal geometry and
                                      // -3,-4,-5,... for external geometry
        Axes Cross = Axes::None;
        int LazyExternalId = InvalidLazyExternalId;
        bool LazyExternalVertex = false;
        std::set<int> ConstrIndices;
        ConstraintHitKind ConstraintKind = ConstraintHitKind::None;
        std::optional<Base::Vector3d> PickedPoint;

        [[nodiscard]] inline bool hasWinner() const
        {
            return Kind != HitKind::None || LazyExternalId != InvalidLazyExternalId;
        }

        [[nodiscard]] inline bool hasPickedPoint() const
        {
            return PickedPoint.has_value();
        }

        [[nodiscard]] inline const Base::Vector3d& pickedPoint() const
        {
            return PickedPoint.value();
        }

        void setPickedPoint(const Base::Vector3d& point);
        void setPickedPoint(const SoPickedPoint* point);

        inline void clear()
        {
            Kind = HitKind::None;
            PointIndex = InvalidPoint;
            GeoIndex = InvalidCurve;
            Cross = Axes::None;
            LazyExternalId = InvalidLazyExternalId;
            LazyExternalVertex = false;
            ConstrIndices.clear();
            ConstraintKind = ConstraintHitKind::None;
            PickedPoint.reset();
        }
    };

    struct PreselectionCandidates
    {
        std::vector<PreselectionResult> Items;
    };

public:
    explicit EditModeCoinManager(ViewProviderSketch& vp);
    ~EditModeCoinManager();

    /** @name Temporary edit curves and markers */
    //@{
    void drawEditMarkers(const std::vector<Base::Vector2d>& EditMarkers, unsigned int augmentationlevel);
    void drawEdit(const std::vector<Base::Vector2d>& EditCurve, GeometryCreationMode mode);
    void drawEdit(const std::list<std::vector<Base::Vector2d>>& list, GeometryCreationMode mode);
    void drawLineExtensionAutoConstraintHint(const std::vector<Base::Vector2d>& HintCurve);
    void setPositionText(const Base::Vector2d& Pos, const SbString& txt);
    void setPositionText(const Base::Vector2d& Pos);
    void resetPositionText();
    void setAxisPickStyle(bool on);
    //@}

    /** @name handle preselection and selection of points */
    //@{
    PreselectionResult detectPreselection(
        const SoPickedPointList& points,
        const SbVec2s& cursorPos,
        int hoveredPointIndex = PreselectionResult::InvalidPoint
    );
    /// The client is responsible for unref-ing the SoGroup to release the memory.
    SoGroup* getSelectedConstraints();
    //@}

    /** @name update coin nodes*/
    void processGeometryConstraintsInformationOverlay(
        const GeoListFacade& geolistfacade,
        bool rebuildinformationlayer
    );

    void updateVirtualSpace();
    void drawLazyExternalGeometryLayer(LazyExternalGeometryLayer& layer);

    /// Draw all constraint icons
    /*! Except maybe the radius and lock ones? */
    void drawConstraintIcons();

    // This specific overload is to use a specific geometry list, which may be a temporal one
    void drawConstraintIcons(const GeoListFacade& geolistfacade);

    void updateGeometryLayersConfiguration();
    //@}

    /** @name coin node access*/
    SoSeparator* getRootEditNode();
    //@}

    /** @name update coin colors*/
    //@{
    void updateColor();
    void updateColor(const GeoListFacade& geolistfacade);  // overload to be used with temporal geometry.
    //@}

    /** @name change constraints selectability*/
    //@{
    void setConstraintSelectability(bool enabled = true);
    //@}

    // Updates the Axes extension to span the specified area.
    void updateAxesLength(const Base::BoundBox2d& bb);

    void updateElementSizeParameters();

private:
    PreselectionResult detectConstraintPreselection(
        const SoPickedPointList& points,
        const SbVec2s& cursorPos
    );
    bool detectOriginPreselection(const SoPickedPoint* point, PreselectionResult& result);
    bool detectGeometryPreselection(
        const SoPickedPointList& points,
        const SbVec2s& cursorPos,
        int hoveredPointIndex,
        PreselectionResult& result
    );
    bool detectPointPreselection(const SoPickedPointList& points, PreselectionResult& result);
    bool detectCurvePreselection(const SoPickedPointList& points, PreselectionResult& result);
    bool detectAxisPreselection(const SoPickedPointList& points, PreselectionResult& result);
    bool detectPointPreselection(const SoPickedPoint* point, int layerIndex, PreselectionResult& result);
    bool detectCurvePreselection(const SoPickedPoint* point, int layerIndex, PreselectionResult& result);
    bool detectAxisPreselection(const SoPickedPoint* point, PreselectionResult& result);
    PreselectionCandidates collectPreselectionCandidates(
        const SoPickedPointList& points,
        const SbVec2s& cursorPos,
        int hoveredPointIndex
    );
    PreselectionResult resolvePreselectionCandidates(const PreselectionCandidates& candidates) const;

    // This function populates the coin nodes with the information of the current geometry
    void processGeometry(const GeoListFacade& geolistfacade);

    // This function populates the geometry information layer of coin. It requires the analysis
    // information gathered during the processGeometry step, so it is not possible to run both in
    // parallel.
    void processGeometryInformationOverlay(const GeoListFacade& geolistfacade);

    // updates the parameters to be used for the Overlay information layer
    void updateOverlayParameters();

    void updateGeometryColor(const GeoListFacade& geolistfacade, bool issketchinvalid);

    // causes the ViewProvider to draw
    void redrawViewProvider();

    void setEditDrawStyle(GeometryCreationMode mode);

    int defaultApplicationFontSizePixels() const;

    double getDevicePixelRatio() const;

    int getApplicationLogicalDPIX() const;

    void updateInventorNodeSizes();

    void updateInventorColors();

    void updateInventorPatterns();

    void updateInventorWidths();

    /** @name coin nodes creation*/
    void createEditModeInventorNodes();
    //@}

private:
    /// Reference to ViewProviderSketch in order to access the public and the Attorney Interface
    ViewProviderSketch& viewProvider;
    /// Observer to track all the needed parameters.
    std::unique_ptr<EditModeCoinManager::ParameterObserver> pObserver;

    DrawingParameters drawingParameters;
    AnalysisResults analysisResults;
    OverlayParameters overlayParameters;
    ConstraintParameters constraintParameters;
    GeometryLayerParameters geometryLayerParameters;

    /// The pointers to Coin Scenegraph
    EditModeScenegraphNodes editModeScenegraphNodes;

    /// Mapping between external and internal indices
    CoinMapping coinMapping;

    // Coin Helpers
    std::unique_ptr<EditModeConstraintCoinManager> pEditModeConstraintCoinManager;
    std::unique_ptr<EditModeGeometryCoinManager> pEditModeGeometryCoinManager;
};


}  // namespace SketcherGui
