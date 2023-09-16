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

#include "PreCompiled.h"
#ifndef _PreComp_
#include <memory>

#include <Inventor/SbVec3f.h>
#include <Inventor/SoPickedPoint.h>
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
#endif  // #ifndef _PreComp_

#include <Base/Exception.h>
#include <Gui/Inventor/MarkerBitmaps.h>
#include <Gui/SoFCBoundingBox.h>
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
         [this](const std::string& param) {
             updateCurvedEdgeCountSegmentsParameter(param);
         }},
        {"BSplineDegreeVisible",
         [this](const std::string& param) {
             updateOverlayVisibilityParameter<OverlayVisibilityParameter::BSplineDegree>(param);
         }},
        {"BSplineControlPolygonVisible",
         [this](const std::string& param) {
             updateOverlayVisibilityParameter<
                 OverlayVisibilityParameter::BSplineControlPolygonVisible>(param);
         }},
        {"BSplineCombVisible",
         [this](const std::string& param) {
             updateOverlayVisibilityParameter<OverlayVisibilityParameter::BSplineCombVisible>(
                 param);
         }},
        {"BSplineKnotMultiplicityVisible",
         [this](const std::string& param) {
             updateOverlayVisibilityParameter<
                 OverlayVisibilityParameter::BSplineKnotMultiplicityVisible>(param);
         }},
        {"BSplinePoleWeightVisible",
         [this](const std::string& param) {
             updateOverlayVisibilityParameter<OverlayVisibilityParameter::BSplinePoleWeightVisible>(
                 param);
         }},
        {"ArcCircleHelperVisible",
         [this](const std::string& param) {
             updateOverlayVisibilityParameter<OverlayVisibilityParameter::ArcCircleHelperVisible>(
                 param);
         }},
        {"TopRenderGeometryId",
         [this](const std::string& param) {
             updateLineRenderingOrderParameters(param);
         }},
        {"MidRenderGeometryId",
         [this](const std::string& param) {
             updateLineRenderingOrderParameters(param);
         }},
        {"HideUnits",
         [this](const std::string& param) {
             updateConstraintPresentationParameters(param);
         }},
        {"ShowDimensionalName",
         [this](const std::string& param) {
             updateConstraintPresentationParameters(param);
         }},
        {"DimensionalStringFormat",
         [this](const std::string& param) {
             updateConstraintPresentationParameters(param);
         }},
        {"ViewScalingFactor",
         [this](const std::string& param) {
             updateElementSizeParameters(param);
         }},
        {"MarkerSize",
         [this](const std::string& param) {
             updateElementSizeParameters(param);
         }},
        {"EditSketcherFontSize",
         [this](const std::string& param) {
             updateElementSizeParameters(param);
         }},
        {"CreateLineColor",
         [this, drawingParameters = Client.drawingParameters](const std::string& param) {
             updateColor(drawingParameters.CreateCurveColor, param);
         }},
        {"EditedVertexColor",
         [this, drawingParameters = Client.drawingParameters](const std::string& param) {
             updateColor(drawingParameters.VertexColor, param);
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
        {"FullyConstraintConstructionPointColor",
         [this, drawingParameters = Client.drawingParameters](const std::string& param) {
             updateColor(drawingParameters.FullyConstraintConstructionPointColor, param);
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
        {"UserSchema",
         [this](const std::string& param) {
             updateUnit(param);
         }},
    };

    for (auto& val : str2updatefunction) {
        auto string = val.first;
        auto function = val.second;

        function(string);
    }
}

void EditModeCoinManager::ParameterObserver::updateCurvedEdgeCountSegmentsParameter(
    const std::string& parametername)
{
    ParameterGrp::handle hGrp =
        App::GetApplication().GetParameterGroupByPath("User parameter:BaseApp/Preferences/View");
    int stdcountsegments = hGrp->GetInt(parametername.c_str(), 50);
    // value cannot be smaller than 6
    if (stdcountsegments < 6) {
        stdcountsegments = 6;
    }

    Client.drawingParameters.curvedEdgeCountSegments = stdcountsegments;
}

void EditModeCoinManager::ParameterObserver::updateLineRenderingOrderParameters(
    const std::string& parametername)
{
    (void)parametername;

    ParameterGrp::handle hGrpp = App::GetApplication().GetParameterGroupByPath(
        "User parameter:BaseApp/Preferences/Mod/Sketcher/General");

    Client.drawingParameters.topRenderingGeometry =
        DrawingParameters::GeometryRendering(hGrpp->GetInt("TopRenderGeometryId", 1));
    Client.drawingParameters.midRenderingGeometry =
        DrawingParameters::GeometryRendering(hGrpp->GetInt("MidRenderGeometryId", 2));
}

void EditModeCoinManager::ParameterObserver::updateConstraintPresentationParameters(
    const std::string& parametername)
{
    (void)parametername;

    ParameterGrp::handle hGrpskg = App::GetApplication().GetParameterGroupByPath(
        "User parameter:BaseApp/Preferences/Mod/Sketcher");

    Client.constraintParameters.bHideUnits = hGrpskg->GetBool("HideUnits", false);
    Client.constraintParameters.bShowDimensionalName =
        hGrpskg->GetBool("ShowDimensionalName", false);
    Client.constraintParameters.sDimensionalStringFormat =
        QString::fromStdString(hGrpskg->GetASCII("DimensionalStringFormat", "%N = %V"));
}

template<EditModeCoinManager::ParameterObserver::OverlayVisibilityParameter visibilityparameter>
void EditModeCoinManager::ParameterObserver::updateOverlayVisibilityParameter(
    const std::string& parametername)
{
    ParameterGrp::handle hGrpsk = App::GetApplication().GetParameterGroupByPath(
        "User parameter:BaseApp/Preferences/Mod/Sketcher/General");

    if constexpr (visibilityparameter == OverlayVisibilityParameter::BSplineDegree) {
        Client.overlayParameters.bSplineDegreeVisible =
            hGrpsk->GetBool(parametername.c_str(), true);
    }
    else if constexpr (visibilityparameter
                       == OverlayVisibilityParameter::BSplineControlPolygonVisible) {
        Client.overlayParameters.bSplineControlPolygonVisible =
            hGrpsk->GetBool(parametername.c_str(), true);
    }
    else if constexpr (visibilityparameter == OverlayVisibilityParameter::BSplineCombVisible) {
        Client.overlayParameters.bSplineCombVisible = hGrpsk->GetBool(parametername.c_str(), true);
    }
    else if constexpr (visibilityparameter
                       == OverlayVisibilityParameter::BSplineKnotMultiplicityVisible) {
        Client.overlayParameters.bSplineKnotMultiplicityVisible =
            hGrpsk->GetBool(parametername.c_str(), true);
    }
    else if constexpr (visibilityparameter
                       == OverlayVisibilityParameter::BSplinePoleWeightVisible) {
        Client.overlayParameters.bSplinePoleWeightVisible =
            hGrpsk->GetBool(parametername.c_str(), true);
    }
    else if constexpr (visibilityparameter == OverlayVisibilityParameter::ArcCircleHelperVisible) {
        Client.overlayParameters.arcCircleHelperVisible =
            hGrpsk->GetBool(parametername.c_str(), false);
    }

    Client.overlayParameters.visibleInformationChanged = true;
}

void EditModeCoinManager::ParameterObserver::updateElementSizeParameters(
    const std::string& parametername)
{
    (void)parametername;

    // Add scaling to Constraint icons
    ParameterGrp::handle hGrp =
        App::GetApplication().GetParameterGroupByPath("User parameter:BaseApp/Preferences/View");

    double viewScalingFactor = hGrp->GetFloat("ViewScalingFactor", 1.0);
    viewScalingFactor = Base::clamp<double>(viewScalingFactor, 0.5, 5.0);

    int markersize = hGrp->GetInt("MarkerSize", 7);

    int defaultFontSizePixels =
        Client.defaultApplicationFontSizePixels();  // returns height in pixels, not points

    int sketcherfontSize = hGrp->GetInt("EditSketcherFontSize", defaultFontSizePixels);

    int dpi = Client.getApplicationLogicalDPIX();

    // simple scaling factor for hardcoded pixel values in the Sketcher
    Client.drawingParameters.pixelScalingFactor = viewScalingFactor * dpi
        / 96;  // 96 ppi is the standard pixel density for which pixel quantities were calculated

    // About sizes:
    // SoDatumLabel takes the size in points, not in pixels. This is because it uses QFont
    // internally. Coin, at least our coin at this time, takes pixels, not points.
    //
    // DPI considerations:
    // With hdpi monitors, the coin font labels do not respect the size passed in pixels:
    // https://forum.freecad.org/viewtopic.php?f=3&t=54347&p=467610#p467610
    // https://forum.freecad.org/viewtopic.php?f=10&t=49972&start=40#p467471
    //
    // Because I (abdullah) have  96 dpi logical, 82 dpi physical, and I see a 35px font setting for
    // a "1" in a datum label as 34px, and I see kilsore and Elyas screenshots showing 41px and 61px
    // in higher resolution monitors for the same configuration, I think that coin pixel size has to
    // be corrected by the logical dpi of the monitor. The rationale is that: a) it obviously needs
    // dpi correction, b) with physical dpi, the ratio of representation between kilsore and me is
    // too far away.
    //
    // This means that the following correction does not have a documented basis, but appears
    // necessary so that the Sketcher is usable in HDPI monitors.

    Client.drawingParameters.coinFontSize =
        std::lround(sketcherfontSize * 96.0f / dpi);  // this is in pixels
    Client.drawingParameters.labelFontSize = std::lround(
        sketcherfontSize * 72.0f / dpi);  // this is in points, as SoDatumLabel uses points
    Client.drawingParameters.constraintIconSize = std::lround(0.8 * sketcherfontSize);

    // For marker size the global default is used.
    //
    // Rationale:
    // -> Other WBs use the default value as is
    // -> If a user has a HDPI, he will eventually change the value for the other WBs
    // -> If we correct the value here in addition, we would get two times a resize
    Client.drawingParameters.markerSize = markersize;

    Client.updateInventorNodeSizes();
}

void EditModeCoinManager::ParameterObserver::updateColor(SbColor& sbcolor,
                                                         const std::string& parametername)
{
    ParameterGrp::handle hGrp =
        App::GetApplication().GetParameterGroupByPath("User parameter:BaseApp/Preferences/View");

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
            "User parameter:BaseApp/Preferences/View");
        hGrp->Attach(this);

        ParameterGrp::handle hGrpsk = App::GetApplication().GetParameterGroupByPath(
            "User parameter:BaseApp/Preferences/Mod/Sketcher/General");
        hGrpsk->Attach(this);

        ParameterGrp::handle hGrpskg = App::GetApplication().GetParameterGroupByPath(
            "User parameter:BaseApp/Preferences/Mod/Sketcher");
        hGrpskg->Attach(this);

        ParameterGrp::handle hGrpu = App::GetApplication().GetParameterGroupByPath(
            "User parameter:BaseApp/Preferences/Units");
        hGrpu->Attach(this);
    }
    catch (const Base::ValueError& e) {  // ensure that if parameter strings are not well-formed,
                                         // the exception is not propagated
        Base::Console().DeveloperError("EditModeCoinManager",
                                       "Malformed parameter string: %s\n",
                                       e.what());
    }
}

void EditModeCoinManager::ParameterObserver::unsubscribeToParameters()
{
    try {
        ParameterGrp::handle hGrp = App::GetApplication().GetParameterGroupByPath(
            "User parameter:BaseApp/Preferences/View");
        hGrp->Detach(this);

        ParameterGrp::handle hGrpsk = App::GetApplication().GetParameterGroupByPath(
            "User parameter:BaseApp/Preferences/Mod/Sketcher/General");
        hGrpsk->Detach(this);

        ParameterGrp::handle hGrpskg = App::GetApplication().GetParameterGroupByPath(
            "User parameter:BaseApp/Preferences/Mod/Sketcher");
        hGrpskg->Detach(this);

        ParameterGrp::handle hGrpu = App::GetApplication().GetParameterGroupByPath(
            "User parameter:BaseApp/Preferences/Units");
        hGrpu->Detach(this);
    }
    catch (const Base::ValueError&
               e) {  // ensure that if parameter strings are not well-formed, the program is not
                     // terminated when calling the noexcept destructor.
        Base::Console().DeveloperError("EditModeCoinManager",
                                       "Malformed parameter string: %s\n",
                                       e.what());
    }
}

void EditModeCoinManager::ParameterObserver::OnChange(Base::Subject<const char*>& rCaller,
                                                      const char* sReason)
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

    pEditModeConstraintCoinManager =
        std::make_unique<EditModeConstraintCoinManager>(viewProvider,
                                                        drawingParameters,
                                                        geometryLayerParameters,
                                                        constraintParameters,
                                                        editModeScenegraphNodes,
                                                        coinMapping);

    pEditModeGeometryCoinManager =
        std::make_unique<EditModeGeometryCoinManager>(viewProvider,
                                                      drawingParameters,
                                                      geometryLayerParameters,
                                                      analysisResults,
                                                      editModeScenegraphNodes,
                                                      coinMapping);
    // Create Edit Mode Scenograph
    createEditModeInventorNodes();

    // Create parameter observer and initialise watched parameters
    pObserver = std::make_unique<EditModeCoinManager::ParameterObserver>(*this);
}

EditModeCoinManager::~EditModeCoinManager()
{
    Gui::coinRemoveAllChildren(editModeScenegraphNodes.EditRoot);
    ViewProviderSketchCoinAttorney::removeNodeFromRoot(viewProvider,
                                                       editModeScenegraphNodes.EditRoot);
    editModeScenegraphNodes.EditRoot->unref();
}

/***** Temporary edit curves and markers *****/

void EditModeCoinManager::drawEditMarkers(const std::vector<Base::Vector2d>& EditMarkers,
                                          unsigned int augmentationlevel)
{
    // determine marker size
    int augmentedmarkersize = drawingParameters.markerSize;

    auto supportedsizes = Gui::Inventor::MarkerBitmaps::getSupportedSizes("CIRCLE_LINE");

    auto defaultmarker =
        std::find(supportedsizes.begin(), supportedsizes.end(), drawingParameters.markerSize);

    if (defaultmarker != supportedsizes.end()) {
        auto validAugmentationLevels = std::distance(defaultmarker, supportedsizes.end());

        if (augmentationlevel >= validAugmentationLevels) {
            augmentationlevel = validAugmentationLevels - 1;
        }

        augmentedmarkersize = *std::next(defaultmarker, augmentationlevel);
    }

    editModeScenegraphNodes.EditMarkerSet->markerIndex.startEditing();
    editModeScenegraphNodes.EditMarkerSet->markerIndex =
        Gui::Inventor::MarkerBitmaps::getMarkerIndex("CIRCLE_LINE", augmentedmarkersize);

    // add the points to set
    editModeScenegraphNodes.EditMarkersCoordinate->point.setNum(EditMarkers.size());
    editModeScenegraphNodes.EditMarkersMaterials->diffuseColor.setNum(EditMarkers.size());
    SbVec3f* verts = editModeScenegraphNodes.EditMarkersCoordinate->point.startEditing();
    SbColor* color = editModeScenegraphNodes.EditMarkersMaterials->diffuseColor.startEditing();

    int i = 0;  // setting up the line set
    for (std::vector<Base::Vector2d>::const_iterator it = EditMarkers.begin();
         it != EditMarkers.end();
         ++it, i++) {
        verts[i].setValue(it->x,
                          it->y,
                          ViewProviderSketchCoinAttorney::getViewOrientationFactor(viewProvider)
                              * drawingParameters.zEdit);
        color[i] = drawingParameters.InformationColor;
    }

    editModeScenegraphNodes.EditMarkersCoordinate->point.finishEditing();
    editModeScenegraphNodes.EditMarkersMaterials->diffuseColor.finishEditing();
    editModeScenegraphNodes.EditMarkerSet->markerIndex.finishEditing();
}

void EditModeCoinManager::drawEdit(const std::vector<Base::Vector2d>& EditCurve)
{
    editModeScenegraphNodes.EditCurveSet->numVertices.setNum(1);
    editModeScenegraphNodes.EditCurvesCoordinate->point.setNum(EditCurve.size());
    editModeScenegraphNodes.EditCurvesMaterials->diffuseColor.setNum(EditCurve.size());
    SbVec3f* verts = editModeScenegraphNodes.EditCurvesCoordinate->point.startEditing();
    int32_t* index = editModeScenegraphNodes.EditCurveSet->numVertices.startEditing();
    SbColor* color = editModeScenegraphNodes.EditCurvesMaterials->diffuseColor.startEditing();

    int i = 0;  // setting up the line set
    for (std::vector<Base::Vector2d>::const_iterator it = EditCurve.begin(); it != EditCurve.end();
         ++it, i++) {
        verts[i].setValue(it->x,
                          it->y,
                          ViewProviderSketchCoinAttorney::getViewOrientationFactor(viewProvider)
                              * drawingParameters.zEdit);
        color[i] = drawingParameters.CreateCurveColor;
    }

    index[0] = EditCurve.size();
    editModeScenegraphNodes.EditCurvesCoordinate->point.finishEditing();
    editModeScenegraphNodes.EditCurveSet->numVertices.finishEditing();
    editModeScenegraphNodes.EditCurvesMaterials->diffuseColor.finishEditing();
}

void EditModeCoinManager::drawEdit(const std::list<std::vector<Base::Vector2d>>& list)
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

    int coordindex = 0;
    int indexindex = 0;
    for (const auto& v : list) {
        for (const auto& p : v) {
            verts[coordindex].setValue(
                p.x,
                p.y,
                ViewProviderSketchCoinAttorney::getViewOrientationFactor(viewProvider)
                    * drawingParameters.zEdit);
            color[coordindex] = drawingParameters.CreateCurveColor;
            coordindex++;
        }
        index[indexindex] = v.size();
        indexindex++;
    }

    editModeScenegraphNodes.EditCurvesCoordinate->point.finishEditing();
    editModeScenegraphNodes.EditCurveSet->numVertices.finishEditing();
    editModeScenegraphNodes.EditCurvesMaterials->diffuseColor.finishEditing();
}

void EditModeCoinManager::setPositionText(const Base::Vector2d& Pos, const SbString& text)
{
    editModeScenegraphNodes.textX->string = text;
    editModeScenegraphNodes.textPos->translation =
        SbVec3f(Pos.x,
                Pos.y,
                ViewProviderSketchCoinAttorney::getViewOrientationFactor(viewProvider)
                    * drawingParameters.zText);
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

EditModeCoinManager::PreselectionResult
EditModeCoinManager::detectPreselection(SoPickedPoint* Point, const SbVec2s& cursorPos)
{
    EditModeCoinManager::PreselectionResult result;

    if (!Point) {
        return result;
    }

    // Base::Console().Log("Point pick\n");
    SoPath* path = Point->getPath();
    SoNode* tail = path->getTail();  // Tail is directly the node containing points and curves

    for (int l = 0; l < geometryLayerParameters.getCoinLayerCount(); l++) {
        // checking for a hit in the points
        if (tail == editModeScenegraphNodes.PointSet[l]) {
            const SoDetail* point_detail = Point->getDetail(editModeScenegraphNodes.PointSet[l]);
            if (point_detail && point_detail->getTypeId() == SoPointDetail::getClassTypeId()) {
                // get the index
                int pindex = static_cast<const SoPointDetail*>(point_detail)->getCoordinateIndex();
                result.PointIndex = coinMapping.getPointVertexId(
                    pindex,
                    l);  // returns -1 for root, global VertexId for the rest of vertices.

                if (result.PointIndex == -1) {
                    result.Cross = PreselectionResult::Axes::RootPoint;
                }

                return result;
            }
        }

        // checking for a hit in the curves
        if (tail == editModeScenegraphNodes.CurveSet[l]) {
            const SoDetail* curve_detail = Point->getDetail(editModeScenegraphNodes.CurveSet[l]);
            if (curve_detail && curve_detail->getTypeId() == SoLineDetail::getClassTypeId()) {
                // get the index
                int curveIndex = static_cast<const SoLineDetail*>(curve_detail)->getLineIndex();
                result.GeoIndex = coinMapping.getCurveGeoId(curveIndex, l);

                return result;
            }
        }
    }
    // checking for a hit in the axes
    if (tail == editModeScenegraphNodes.RootCrossSet) {
        const SoDetail* cross_detail = Point->getDetail(editModeScenegraphNodes.RootCrossSet);
        if (cross_detail && cross_detail->getTypeId() == SoLineDetail::getClassTypeId()) {
            // get the index (reserve index 0 for root point)
            int CrossIndex = static_cast<const SoLineDetail*>(cross_detail)->getLineIndex();

            if (CrossIndex == 0) {
                result.Cross = PreselectionResult::Axes::HorizontalAxis;
            }
            else if (CrossIndex == 1) {
                result.Cross = PreselectionResult::Axes::VerticalAxis;
            }

            return result;
        }
    }
    // checking if a constraint is hit
    result.ConstrIndices =
        pEditModeConstraintCoinManager->detectPreselectionConstr(Point, cursorPos);

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
    bool rebuildinformationlayer)
{
    overlayParameters.rebuildInformationLayer = rebuildinformationlayer;

    pEditModeGeometryCoinManager->processGeometry(geolistfacade);

    updateOverlayParameters();

    processGeometryInformationOverlay(geolistfacade);

    updateAxesLength();

    pEditModeConstraintCoinManager->processConstraints(geolistfacade);
}

void EditModeCoinManager::updateOverlayParameters()
{
    if ((analysisResults.combRepresentationScale
         > (2 * overlayParameters.currentBSplineCombRepresentationScale))
        || (analysisResults.combRepresentationScale
            < (overlayParameters.currentBSplineCombRepresentationScale / 2))) {
        overlayParameters.currentBSplineCombRepresentationScale =
            analysisResults.combRepresentationScale;
    }
}

void EditModeCoinManager::processGeometryInformationOverlay(const GeoListFacade& geolistfacade)
{
    if (overlayParameters.rebuildInformationLayer) {
        // every time we start with empty information overlay
        Gui::coinRemoveAllChildren(editModeScenegraphNodes.infoGroup);
    }

    auto ioconv = EditModeInformationOverlayCoinConverter(viewProvider,
                                                          editModeScenegraphNodes.infoGroup,
                                                          overlayParameters,
                                                          drawingParameters);

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

void EditModeCoinManager::updateAxesLength()
{
    auto zCrossH = ViewProviderSketchCoinAttorney::getViewOrientationFactor(viewProvider)
        * drawingParameters.zCross;
    editModeScenegraphNodes.RootCrossCoordinate->point.set1Value(
        0,
        SbVec3f(-analysisResults.boundingBoxMagnitudeOrder, 0.0f, zCrossH));
    editModeScenegraphNodes.RootCrossCoordinate->point.set1Value(
        1,
        SbVec3f(analysisResults.boundingBoxMagnitudeOrder, 0.0f, zCrossH));
    editModeScenegraphNodes.RootCrossCoordinate->point.set1Value(
        2,
        SbVec3f(0.0f, -analysisResults.boundingBoxMagnitudeOrder, zCrossH));
    editModeScenegraphNodes.RootCrossCoordinate->point.set1Value(
        3,
        SbVec3f(0.0f, analysisResults.boundingBoxMagnitudeOrder, zCrossH));
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
    editModeScenegraphNodes.EditRoot
        ->ref();  // Node is unref in the destructor of EditModeCoinManager
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
    editModeScenegraphNodes.RootCrossDrawStyle->lineWidth =
        2 * drawingParameters.pixelScalingFactor;
    crossRoot->addChild(editModeScenegraphNodes.RootCrossDrawStyle);

    editModeScenegraphNodes.RootCrossMaterials = new SoMaterial;
    editModeScenegraphNodes.RootCrossMaterials->setName("RootCrossMaterials");
    editModeScenegraphNodes.RootCrossMaterials->diffuseColor.set1Value(
        0,
        drawingParameters.CrossColorH);
    editModeScenegraphNodes.RootCrossMaterials->diffuseColor.set1Value(
        1,
        drawingParameters.CrossColorV);
    crossRoot->addChild(editModeScenegraphNodes.RootCrossMaterials);

    editModeScenegraphNodes.RootCrossCoordinate = new SoCoordinate3;
    editModeScenegraphNodes.RootCrossCoordinate->setName("RootCrossCoordinate");
    crossRoot->addChild(editModeScenegraphNodes.RootCrossCoordinate);

    editModeScenegraphNodes.RootCrossSet = new SoLineSet;
    editModeScenegraphNodes.RootCrossSet->setName("RootCrossLineSet");
    crossRoot->addChild(editModeScenegraphNodes.RootCrossSet);

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
    editModeScenegraphNodes.EditCurvesDrawStyle->lineWidth =
        3 * drawingParameters.pixelScalingFactor;
    editCurvesRoot->addChild(editModeScenegraphNodes.EditCurvesDrawStyle);

    editModeScenegraphNodes.EditCurveSet = new SoLineSet;
    editModeScenegraphNodes.EditCurveSet->setName("EditCurveLineSet");
    editCurvesRoot->addChild(editModeScenegraphNodes.EditCurveSet);

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
    editModeScenegraphNodes.EditMarkersDrawStyle->pointSize =
        8 * drawingParameters.pixelScalingFactor;
    editMarkersRoot->addChild(editModeScenegraphNodes.EditMarkersDrawStyle);

    editModeScenegraphNodes.EditMarkerSet = new SoMarkerSet;
    editModeScenegraphNodes.EditMarkerSet->setName("EditMarkerSet");
    editModeScenegraphNodes.EditMarkerSet->markerIndex =
        Gui::Inventor::MarkerBitmaps::getMarkerIndex("CIRCLE_LINE", drawingParameters.markerSize);
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
    editModeScenegraphNodes.InformationDrawStyle->lineWidth =
        1 * drawingParameters.pixelScalingFactor;
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

int EditModeCoinManager::getApplicationLogicalDPIX() const
{
    return ViewProviderSketchCoinAttorney::getApplicationLogicalDPIX(viewProvider);
}

void EditModeCoinManager::updateInventorNodeSizes()
{
    auto layersconfiguration = viewProvider.VisualLayerList.getValues();

    for (int l = 0; l < geometryLayerParameters.getCoinLayerCount(); l++) {
        editModeScenegraphNodes.PointsDrawStyle[l]->pointSize =
            8 * drawingParameters.pixelScalingFactor;
        editModeScenegraphNodes.PointSet[l]->markerIndex =
            Gui::Inventor::MarkerBitmaps::getMarkerIndex("CIRCLE_FILLED",
                                                         drawingParameters.markerSize);
        editModeScenegraphNodes.CurvesDrawStyle[l]->lineWidth =
            layersconfiguration[l].getLineWidth() * drawingParameters.pixelScalingFactor;
    }

    editModeScenegraphNodes.RootCrossDrawStyle->lineWidth =
        2 * drawingParameters.pixelScalingFactor;
    editModeScenegraphNodes.EditCurvesDrawStyle->lineWidth =
        3 * drawingParameters.pixelScalingFactor;
    editModeScenegraphNodes.EditMarkersDrawStyle->pointSize =
        8 * drawingParameters.pixelScalingFactor;
    editModeScenegraphNodes.EditMarkerSet->markerIndex =
        Gui::Inventor::MarkerBitmaps::getMarkerIndex("CIRCLE_LINE", drawingParameters.markerSize);
    editModeScenegraphNodes.ConstraintDrawStyle->lineWidth =
        1 * drawingParameters.pixelScalingFactor;
    editModeScenegraphNodes.InformationDrawStyle->lineWidth =
        1 * drawingParameters.pixelScalingFactor;

    editModeScenegraphNodes.textFont->size.setValue(drawingParameters.coinFontSize);

    pEditModeConstraintCoinManager->rebuildConstraintNodes();
}

void EditModeCoinManager::updateInventorColors()
{
    editModeScenegraphNodes.RootCrossMaterials->diffuseColor.set1Value(
        0,
        drawingParameters.CrossColorH);
    editModeScenegraphNodes.RootCrossMaterials->diffuseColor.set1Value(
        1,
        drawingParameters.CrossColorV);
    editModeScenegraphNodes.textMaterial->diffuseColor = drawingParameters.CursorTextColor;
}

/************************ Edit node access ************************/

SoSeparator* EditModeCoinManager::getRootEditNode()
{
    return editModeScenegraphNodes.EditRoot;
}
