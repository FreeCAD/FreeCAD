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
#include <Inventor/nodes/SoCoordinate3.h>
#include <Inventor/nodes/SoDrawStyle.h>
#include <Inventor/nodes/SoLineSet.h>
#include <Inventor/nodes/SoMarkerSet.h>
#include <Inventor/nodes/SoMaterial.h>
#include <Inventor/nodes/SoSeparator.h>
#endif  // #ifndef _PreComp_

#include <Gui/Inventor/MarkerBitmaps.h>
#include <Gui/Inventor/SmSwitchboard.h>
#include <Mod/Sketcher/App/Constraint.h>
#include <Mod/Sketcher/App/GeoEnum.h>
#include <Mod/Sketcher/App/GeoList.h>
#include <Mod/Sketcher/App/GeometryFacade.h>
#include <Mod/Sketcher/App/SolverGeometryExtension.h>

#include "EditModeGeometryCoinConverter.h"
#include "EditModeGeometryCoinManager.h"
#include "ViewProviderSketchCoinAttorney.h"


using namespace SketcherGui;
using namespace Sketcher;

//**************************** EditModeGeometryCoinManager class ******************************

EditModeGeometryCoinManager::EditModeGeometryCoinManager(
    ViewProviderSketch& vp,
    DrawingParameters& drawingParams,
    GeometryLayerParameters& geometryLayerParams,
    AnalysisResults& analysisResultStruct,
    EditModeScenegraphNodes& editModeScenegraph,
    CoinMapping& coinMap)
    : viewProvider(vp)
    , drawingParameters(drawingParams)
    , geometryLayerParameters(geometryLayerParams)
    , analysisResults(analysisResultStruct)
    , editModeScenegraphNodes(editModeScenegraph)
    , coinMapping(coinMap)
{}

EditModeGeometryCoinManager::~EditModeGeometryCoinManager()
{}

void EditModeGeometryCoinManager::processGeometry(const GeoListFacade& geolistfacade)
{
    // enable all layers
    editModeScenegraphNodes.PointsGroup->enable.setNum(geometryLayerParameters.getCoinLayerCount());
    editModeScenegraphNodes.CurvesGroup->enable.setNum(geometryLayerParameters.getCoinLayerCount());
    SbBool* swsp = editModeScenegraphNodes.PointsGroup->enable.startEditing();
    SbBool* swsc = editModeScenegraphNodes.CurvesGroup->enable.startEditing();

    auto setEnableLayer = [swsp, swsc](int l, bool enabled) {
        swsp[l] = enabled;  // layer defaults to enabled
        swsc[l] = enabled;  // layer defaults to enabled
    };

    auto layersconfigurations = viewProvider.VisualLayerList.getValues();

    for (auto l = 0; l < geometryLayerParameters.getCoinLayerCount(); l++) {
        setEnableLayer(l, layersconfigurations[l].isVisible());
    }

    editModeScenegraphNodes.PointsGroup->enable.finishEditing();
    editModeScenegraphNodes.CurvesGroup->enable.finishEditing();

    // Define the coin nodes that will be filled in with the geometry layers
    GeometryLayerNodes geometrylayernodes {editModeScenegraphNodes.PointsMaterials,
                                           editModeScenegraphNodes.PointsCoordinate,
                                           editModeScenegraphNodes.CurvesMaterials,
                                           editModeScenegraphNodes.CurvesCoordinate,
                                           editModeScenegraphNodes.CurveSet};

    // process geometry layers
    EditModeGeometryCoinConverter gcconv(viewProvider,
                                         geometrylayernodes,
                                         drawingParameters,
                                         geometryLayerParameters,
                                         coinMapping);

    gcconv.convert(geolistfacade);

    // set cross coordinates
    editModeScenegraphNodes.RootCrossSet->numVertices.set1Value(0, 2);
    editModeScenegraphNodes.RootCrossSet->numVertices.set1Value(1, 2);

    analysisResults.combRepresentationScale = gcconv.getCombRepresentationScale();
    analysisResults.boundingBoxMagnitudeOrder =
        exp(ceil(log(std::abs(gcconv.getBoundingBoxMaxMagnitude()))));
    analysisResults.bsplineGeoIds = gcconv.getBSplineGeoIds();
    analysisResults.arcGeoIds = gcconv.getArcGeoIds();
}

SbColor EditModeGeometryCoinManager::constrainedPointColor(bool constrainedElement,
                                                           const GeometryFacade* geom)
{
    if (geom && geom->isInternalAligned()) {
        if (constrainedElement) {
            return drawingParameters.FullyConstraintInternalAlignmentColor;
        }
        else {
            return drawingParameters.InternalAlignedGeoColor;
        }
    }
    else {
        if (geom
            && !(geom->isGeoType(Part::GeomPoint::getClassTypeId()) && !geom->getConstruction())) {
            if (constrainedElement) {
                return drawingParameters.FullyConstraintConstructionPointColor;
            }
            else {
                return drawingParameters.VertexColor;
            }
        }
        else {  // this is a defined GeomPoint
            if (constrainedElement) {
                return drawingParameters.FullyConstraintElementColor;
            }
            else {
                return drawingParameters.CurveColor;
            }
        }
    }
}

void EditModeGeometryCoinManager::setPointColors(SbColor* pcolor,
                                                 int PtNum,
                                                 bool issketchinvalid,
                                                 const GeoListFacade& geolistfacade,
                                                 int l)
{
    auto isFullyConstraintElement = [](const GeometryFacade* geom) {
        if (geom) {
            if (geom->hasExtension(Sketcher::SolverGeometryExtension::getClassTypeId())) {

                auto solvext = std::static_pointer_cast<const Sketcher::SolverGeometryExtension>(
                    geom->getExtension(Sketcher::SolverGeometryExtension::getClassTypeId()).lock());

                return (solvext->getGeometry()
                        == Sketcher::SolverGeometryExtension::FullyConstraint);
            }
        }
        return false;
    };
    // colors of the point set
    if (issketchinvalid) {
        for (int i = 0; i < PtNum; i++) {
            pcolor[i] = drawingParameters.InvalidSketchColor;
        }
    }
    else {

        for (int i = 0; i < PtNum; i++) {
            if (!(i == 0 && l == 0)
                && ViewProviderSketchCoinAttorney::isSketchFullyConstrained(
                    viewProvider)) {  // root point is not coloured
                pcolor[i] = drawingParameters.FullyConstrainedColor;
            }
            else {
                int GeoId = coinMapping.getPointGeoId(i, l);
                auto geom = geolistfacade.getGeometryFacadeFromGeoId(GeoId);

                bool constrainedElement = isFullyConstraintElement(geom);

                pcolor[i] = constrainedPointColor(constrainedElement, geom);
            }
        }
    }
}


void EditModeGeometryCoinManager::setPointHeights(SbVec3f* pverts,
                                                  int PtNum,
                                                  const GeoListFacade& geolistfacade,
                                                  int l,
                                                  int viewOrientationFactor,
                                                  float zNormPoint,
                                                  float zConstrPoint)
{
    // update rendering height of points

    float x, y, z;

    for (int i = 0; i < PtNum; i++) {  // 0 is the origin
        if (i == 0 && l == 0) {        // reset root point to lowest
            pverts[i].setValue(0, 0, viewOrientationFactor * drawingParameters.zRootPoint);
        }
        else {
            pverts[i].getValue(x, y, z);
            auto geom = geolistfacade.getGeometryFacadeFromGeoId(coinMapping.getPointGeoId(i, l));

            if (geom) {
                if (geom->getConstruction()) {
                    pverts[i].setValue(x, y, viewOrientationFactor * zConstrPoint);
                }
                else {
                    pverts[i].setValue(x, y, viewOrientationFactor * zNormPoint);
                }
            }
        }
    }
}

SbColor EditModeGeometryCoinManager::setVertexes(int GeoId,
                                                 const GeometryFacade* geom,
                                                 int indexes,
                                                 int highLine,
                                                 int extLine,
                                                 int normLine,
                                                 int constrLine,
                                                 SbVec3f* verts,
                                                 bool preselected,
                                                 bool constrainedElement,
                                                 bool issketchinvalid)
{
    bool selected = ViewProviderSketchCoinAttorney::isCurveSelected(viewProvider, GeoId);

    SbColor ret;
    int vertZ;
    if (selected && preselected) {
        ret = drawingParameters.PreselectSelectedColor;
        vertZ = highLine;
    }
    else if (selected) {
        ret = drawingParameters.SelectColor;
        vertZ = highLine;
    }
    else if (preselected) {
        ret = drawingParameters.PreselectColor;
        vertZ = highLine;
    }
    else if (GeoId <= Sketcher::GeoEnum::RefExt) {  // external Geometry
        ret = drawingParameters.CurveExternalColor;
        vertZ = extLine;
    }
    else if (issketchinvalid) {
        ret = drawingParameters.InvalidSketchColor;
        vertZ = normLine;
    }
    else if (geom && geom->getConstruction()) {
        if (geom->isInternalAligned()) {
            if (constrainedElement) {
                ret = drawingParameters.FullyConstraintInternalAlignmentColor;
            }
            else {
                ret = drawingParameters.InternalAlignedGeoColor;
            }
        }
        else {
            if (constrainedElement) {
                ret = drawingParameters.FullyConstraintConstructionElementColor;
            }
            else {
                ret = drawingParameters.CurveDraftColor;
            }
        }
        vertZ = constrLine;
    }
    else if (ViewProviderSketchCoinAttorney::isSketchFullyConstrained(viewProvider)) {
        ret = drawingParameters.FullyConstrainedColor;
        vertZ = normLine;
    }
    else if (constrainedElement) {
        ret = drawingParameters.FullyConstraintElementColor;
        vertZ = normLine;
    }
    else {
        ret = drawingParameters.CurveColor;
        vertZ = normLine;
    }

    float x, y, z;
    int j = 0;  // vertexindex

    for (int k = j; j < k + indexes; j++) {
        verts[j].getValue(x, y, z);
        verts[j] = SbVec3f(x, y, vertZ);
    }
    return ret;
}

SbColor EditModeGeometryCoinManager::layer0Color(int preselectcross)
{
    if (ViewProviderSketchCoinAttorney::isCurveSelected(viewProvider, Sketcher::GeoEnum::HAxis)) {
        return drawingParameters.SelectColor;
    }
    else if (preselectcross == 1) {  // cross only in layer 0
        return drawingParameters.PreselectColor;
    }
    else {
        return drawingParameters.CrossColorH;
    }

    if (ViewProviderSketchCoinAttorney::isCurveSelected(viewProvider, Sketcher::GeoEnum::VAxis)) {
        return drawingParameters.SelectColor;
    }
    else if (preselectcross == 2) {
        return drawingParameters.PreselectColor;
    }
    else {
        return drawingParameters.CrossColorV;
    }
}

void EditModeGeometryCoinManager::updateGeometryLayerColor(const GeoListFacade& geolistfacade,
                                                           bool issketchinvalid,
                                                           SbColor* crosscolor,
                                                           int viewOrientationFactor,
                                                           int l)
{
    auto isFullyConstraintElement = [](const GeometryFacade* geom) {
        if (geom) {
            if (geom->hasExtension(Sketcher::SolverGeometryExtension::getClassTypeId())) {

                auto solvext = std::static_pointer_cast<const Sketcher::SolverGeometryExtension>(
                    geom->getExtension(Sketcher::SolverGeometryExtension::getClassTypeId()).lock());

                return (solvext->getGeometry()
                        == Sketcher::SolverGeometryExtension::FullyConstraint);
            }
        }
        return false;
    };

    int PtNum = editModeScenegraphNodes.PointsMaterials[l]->diffuseColor.getNum();
    SbColor* pcolor = editModeScenegraphNodes.PointsMaterials[l]->diffuseColor.startEditing();
    int CurvNum = editModeScenegraphNodes.CurvesMaterials[l]->diffuseColor.getNum();
    SbColor* color = editModeScenegraphNodes.CurvesMaterials[l]->diffuseColor.startEditing();

    SbVec3f* verts = editModeScenegraphNodes.CurvesCoordinate[l]->point.startEditing();
    SbVec3f* pverts = editModeScenegraphNodes.PointsCoordinate[l]->point.startEditing();

    auto getRenderHeight = [this](DrawingParameters::GeometryRendering renderingtype,
                                  float toprendering,
                                  float midrendering,
                                  float lowrendering) {
        if (drawingParameters.topRenderingGeometry == renderingtype) {
            return toprendering;
        }
        else if (drawingParameters.midRenderingGeometry == renderingtype) {
            return midrendering;
        }
        else {
            return lowrendering;
        }
    };

    float zNormPoint = getRenderHeight(DrawingParameters::GeometryRendering::NormalGeometry,
                                       drawingParameters.zHighPoints,
                                       drawingParameters.zLowPoints,
                                       drawingParameters.zLowPoints);

    float zConstrPoint = getRenderHeight(DrawingParameters::GeometryRendering::Construction,
                                         drawingParameters.zHighPoints,
                                         drawingParameters.zLowPoints,
                                         drawingParameters.zLowPoints);
    setPointColors(pcolor, PtNum, issketchinvalid, geolistfacade, l);

    setPointHeights(pverts,
                    PtNum,
                    geolistfacade,
                    l,
                    viewOrientationFactor,
                    zNormPoint,
                    zConstrPoint);

    auto preselectpoint = ViewProviderSketchCoinAttorney::getPreselectPoint(viewProvider);
    auto preselectcross = ViewProviderSketchCoinAttorney::getPreselectCross(viewProvider);
    auto preselectcurve = ViewProviderSketchCoinAttorney::getPreselectCurve(viewProvider);

    auto raisePoint = [](SbVec3f& point, float height) {
        float x, y, z;
        point.getValue(x, y, z);
        point.setValue(x, y, height);
    };

    MultiFieldId preselectpointmfid;

    if (preselectcross == 0) {
        if (l == 0) {  // cross only in layer 0
            pcolor[0] = drawingParameters.PreselectColor;
        }
    }
    else if (preselectpoint != -1) {
        preselectpointmfid = coinMapping.getIndexLayer(preselectpoint);
        if (MultiFieldId::Invalid != preselectpointmfid && preselectpointmfid.layerId == l
            && preselectpointmfid.fieldIndex < PtNum) {

            pcolor[preselectpointmfid.fieldIndex] = drawingParameters.PreselectColor;

            raisePoint(pverts[preselectpointmfid.fieldIndex],
                       viewOrientationFactor * drawingParameters.zHighlight);
        }
    }

    ViewProviderSketchCoinAttorney::executeOnSelectionPointSet(
        viewProvider,
        [pcolor,
         pverts,
         PtNum,
         preselectpointmfid,
         layerId = l,
         &coinMapping = coinMapping,
         drawingParameters = this->drawingParameters,
         raisePoint,
         viewOrientationFactor](const int i) {
            auto pointindex = coinMapping.getIndexLayer(i);
            if (layerId == pointindex.layerId && pointindex.fieldIndex >= 0
                && pointindex.fieldIndex < PtNum) {
                pcolor[pointindex.fieldIndex] = (preselectpointmfid == pointindex)
                    ? drawingParameters.PreselectSelectedColor
                    : drawingParameters.SelectColor;

                raisePoint(pverts[pointindex.fieldIndex],
                           viewOrientationFactor * drawingParameters.zHighlight);
            }
        });

    // update colors and rendering height of the curves

    float zNormLine = getRenderHeight(DrawingParameters::GeometryRendering::NormalGeometry,
                                      drawingParameters.zHighLines,
                                      drawingParameters.zMidLines,
                                      drawingParameters.zLowLines);

    float zConstrLine = getRenderHeight(DrawingParameters::GeometryRendering::Construction,
                                        drawingParameters.zHighLines,
                                        drawingParameters.zMidLines,
                                        drawingParameters.zLowLines);

    float zExtLine = getRenderHeight(DrawingParameters::GeometryRendering::ExternalGeometry,
                                     drawingParameters.zHighLines,
                                     drawingParameters.zMidLines,
                                     drawingParameters.zLowLines);

    for (int i = 0; i < CurvNum; i++) {
        int GeoId = coinMapping.getCurveGeoId(i, l);
        auto geom = geolistfacade.getGeometryFacadeFromGeoId(GeoId);
        // CurvId has several vertices associated to 1 material
        // edit->CurveSet->numVertices => [i] indicates number of vertex for line i.
        int indexes = (editModeScenegraphNodes.CurveSet[l]->numVertices[i]);
        bool preselected = (preselectcurve == GeoId);
        bool constrainedElement = isFullyConstraintElement(geom);

        color[i] = setVertexes(GeoId,
                               geom,
                               indexes,
                               viewOrientationFactor * drawingParameters.zHighLine,
                               viewOrientationFactor * zExtLine,
                               viewOrientationFactor * zNormLine,
                               viewOrientationFactor * zConstrLine,
                               verts,
                               preselected,
                               constrainedElement,
                               issketchinvalid);
    }
    // colors of the cross
    if (l == 0) {  // only in layer 0
        crosscolor[0] = layer0Color(preselectcross);
    }
    // end editing
    editModeScenegraphNodes.CurvesMaterials[l]->diffuseColor.finishEditing();
    editModeScenegraphNodes.PointsMaterials[l]->diffuseColor.finishEditing();
    editModeScenegraphNodes.CurvesCoordinate[l]->point.finishEditing();
    editModeScenegraphNodes.CurveSet[l]->numVertices.finishEditing();
}

void EditModeGeometryCoinManager::updateGeometryColor(const GeoListFacade& geolistfacade,
                                                      bool issketchinvalid)
{
    // Update Colors

    SbColor* crosscolor = editModeScenegraphNodes.RootCrossMaterials->diffuseColor.startEditing();
    auto viewOrientationFactor =
        ViewProviderSketchCoinAttorney::getViewOrientationFactor(viewProvider);

    for (auto l = 0; l < geometryLayerParameters.getCoinLayerCount(); l++) {
        updateGeometryLayerColor(geolistfacade,
                                 issketchinvalid,
                                 crosscolor,
                                 viewOrientationFactor,
                                 l);
    }

    editModeScenegraphNodes.RootCrossMaterials->diffuseColor.finishEditing();
}

void EditModeGeometryCoinManager::updateGeometryLayersConfiguration()
{
    // Several cases:
    // 1) The number of layers have changed
    // 2) The number of layers is the same, but the configuration needs to be updated

    // TODO: Quite some room for improvement here:
    geometryLayerParameters.setCoinLayerCount(viewProvider.VisualLayerList.getSize());

    emptyGeometryRootNodes();
    createEditModePointInventorNodes();
    createEditModeCurveInventorNodes();
}

auto concat(std::string string, int i)
{
    return string + std::to_string(i);
};


void EditModeGeometryCoinManager::createEditModeInventorNodes()
{
    createGeometryRootNodes();

    geometryLayerParameters.setCoinLayerCount(viewProvider.VisualLayerList.getSize());

    createEditModePointInventorNodes();

    createEditModeCurveInventorNodes();
}

void EditModeGeometryCoinManager::createGeometryRootNodes()
{
    // stuff for the points ++++++++++++++++++++++++++++++++++++++
    editModeScenegraphNodes.PointsGroup = new SmSwitchboard;
    editModeScenegraphNodes.EditRoot->addChild(editModeScenegraphNodes.PointsGroup);

    // stuff for the Curves +++++++++++++++++++++++++++++++++++++++
    editModeScenegraphNodes.CurvesGroup = new SmSwitchboard;
    editModeScenegraphNodes.EditRoot->addChild(editModeScenegraphNodes.CurvesGroup);
}

void EditModeGeometryCoinManager::emptyGeometryRootNodes()
{
    Gui::coinRemoveAllChildren(editModeScenegraphNodes.PointsGroup);
    Gui::coinRemoveAllChildren(editModeScenegraphNodes.CurvesGroup);
}

void EditModeGeometryCoinManager::createEditModePointInventorNodes()
{
    for (int i = 0; i < geometryLayerParameters.getCoinLayerCount(); i++) {
        SoSeparator* sep = new SoSeparator;
        sep->ref();

        auto somaterial = new SoMaterial;
        editModeScenegraphNodes.PointsMaterials.push_back(somaterial);
        editModeScenegraphNodes.PointsMaterials[i]->setName(concat("PointsMaterials_", i).c_str());
        sep->addChild(editModeScenegraphNodes.PointsMaterials[i]);

        SoMaterialBinding* MtlBind = new SoMaterialBinding;
        MtlBind->setName(concat("PointsMaterialBinding", i).c_str());
        MtlBind->value = SoMaterialBinding::PER_VERTEX;
        sep->addChild(MtlBind);

        auto coords = new SoCoordinate3;
        editModeScenegraphNodes.PointsCoordinate.push_back(coords);
        editModeScenegraphNodes.PointsCoordinate[i]->setName(concat("PointsCoordinate", i).c_str());
        sep->addChild(editModeScenegraphNodes.PointsCoordinate[i]);

        auto drawstyle = new SoDrawStyle;
        editModeScenegraphNodes.PointsDrawStyle.push_back(drawstyle);
        editModeScenegraphNodes.PointsDrawStyle[i]->setName(concat("PointsDrawStyle", i).c_str());
        editModeScenegraphNodes.PointsDrawStyle[i]->pointSize =
            8 * drawingParameters.pixelScalingFactor;
        sep->addChild(editModeScenegraphNodes.PointsDrawStyle[i]);

        auto pointset = new SoMarkerSet;
        editModeScenegraphNodes.PointSet.push_back(pointset);
        editModeScenegraphNodes.PointSet[i]->setName(concat("PointSet", i).c_str());
        editModeScenegraphNodes.PointSet[i]->markerIndex =
            Gui::Inventor::MarkerBitmaps::getMarkerIndex("CIRCLE_FILLED",
                                                         drawingParameters.markerSize);
        sep->addChild(editModeScenegraphNodes.PointSet[i]);

        editModeScenegraphNodes.PointsGroup->addChild(sep);
        sep->unref();
    }
}

void EditModeGeometryCoinManager::createEditModeCurveInventorNodes()
{
    auto layersconfigurations = viewProvider.VisualLayerList.getValue();

    for (int i = 0; i < geometryLayerParameters.getCoinLayerCount(); i++) {
        SoSeparator* sep = new SoSeparator;
        sep->ref();

        auto somaterial = new SoMaterial;
        editModeScenegraphNodes.CurvesMaterials.push_back(somaterial);
        editModeScenegraphNodes.CurvesMaterials[i]->setName(concat("CurvesMaterials", i).c_str());
        sep->addChild(editModeScenegraphNodes.CurvesMaterials[i]);

        auto MtlBind = new SoMaterialBinding;
        MtlBind->setName(concat("CurvesMaterialsBinding", i).c_str());
        MtlBind->value = SoMaterialBinding::PER_FACE;
        sep->addChild(MtlBind);

        auto coords = new SoCoordinate3;
        editModeScenegraphNodes.CurvesCoordinate.push_back(coords);
        editModeScenegraphNodes.CurvesCoordinate[i]->setName(concat("CurvesCoordinate", i).c_str());
        sep->addChild(editModeScenegraphNodes.CurvesCoordinate[i]);

        auto drawstyle = new SoDrawStyle;
        editModeScenegraphNodes.CurvesDrawStyle.push_back(drawstyle);
        editModeScenegraphNodes.CurvesDrawStyle[i]->setName(concat("CurvesDrawStyle", i).c_str());

        editModeScenegraphNodes.CurvesDrawStyle[i]->lineWidth =
            layersconfigurations[i].getLineWidth() * drawingParameters.pixelScalingFactor;
        editModeScenegraphNodes.CurvesDrawStyle[i]->linePattern =
            layersconfigurations[i].getLinePattern();
        editModeScenegraphNodes.CurvesDrawStyle[i]->linePatternScaleFactor = 5;

        sep->addChild(editModeScenegraphNodes.CurvesDrawStyle[i]);

        auto solineset = new SoLineSet;
        editModeScenegraphNodes.CurveSet.push_back(solineset);
        editModeScenegraphNodes.CurveSet[i]->setName(concat("CurvesLineSet", i).c_str());
        sep->addChild(editModeScenegraphNodes.CurveSet[i]);

        editModeScenegraphNodes.CurvesGroup->addChild(sep);
        sep->unref();
    }
}
