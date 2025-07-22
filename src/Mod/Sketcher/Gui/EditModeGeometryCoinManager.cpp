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
#include "Mod/Sketcher/App/ExternalGeometryFacade.h"


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
    editModeScenegraphNodes.CurvesGroup->enable.setNum(
        geometryLayerParameters.getCoinLayerCount() * geometryLayerParameters.getSubLayerCount());
    SbBool* swsp = editModeScenegraphNodes.PointsGroup->enable.startEditing();
    SbBool* swsc = editModeScenegraphNodes.CurvesGroup->enable.startEditing();

    auto layersconfigurations = viewProvider.VisualLayerList.getValues();

    for (auto l = 0; l < geometryLayerParameters.getCoinLayerCount(); l++) {
        auto enabled = layersconfigurations[l].isVisible();

        swsp[l] = enabled;
        int slCount = geometryLayerParameters.getSubLayerCount();
        for (int t = 0; t < slCount; t++) {
            swsc[l * slCount + t] = enabled;
        }
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

void EditModeGeometryCoinManager::updateGeometryColor(const GeoListFacade& geolistfacade,
                                                      bool issketchinvalid)
{
    // Lambdas for convenience retrieval of geometry information
    auto isDefinedGeomPoint = [&geolistfacade](int GeoId, Sketcher::PointPos PosId) {
        auto geom = geolistfacade.getGeometryFacadeFromGeoId(GeoId);
        if (geom) {
            bool isStartOrEnd =
                PosId == Sketcher::PointPos::start || PosId == Sketcher::PointPos::end;
            return isStartOrEnd && !geom->getConstruction();
        }
        return false;
    };

    auto isExternalDefiningGeomPoint = [&geolistfacade](int GeoId) {
        auto geom = geolistfacade.getGeometryFacadeFromGeoId(GeoId);
        if (geom) {
            auto egf = ExternalGeometryFacade::getFacade(geom->clone());
            auto ref = egf->getRef();
            return egf->testFlag(ExternalGeometryExtension::Defining);
        }
        return false;
    };

    auto isCoincident = [&](int GeoId, Sketcher::PointPos PosId) {
        const std::vector<Sketcher::Constraint*>& constraints =
            ViewProviderSketchCoinAttorney::getConstraints(viewProvider);
        for (auto& constr : constraints) {
            if (constr->Type == Coincident
                || (constr->Type == Tangent && constr->FirstPos != Sketcher::PointPos::none)
                || (constr->Type == Perpendicular && constr->FirstPos != Sketcher::PointPos::none
                    && constr->SecondPos != Sketcher::PointPos::none)) {
                if ((constr->First == GeoId && constr->FirstPos == PosId)
                    || (constr->Second == GeoId && constr->SecondPos == PosId)) {
                    return true;
                }
            }
        }
        return false;
    };

    auto isInternalAlignedGeom = [&geolistfacade](int GeoId) {
        auto geom = geolistfacade.getGeometryFacadeFromGeoId(GeoId);
        if (geom) {
            return geom->isInternalAligned();
        }
        return false;
    };

    auto isFullyConstraintElement = [&geolistfacade](int GeoId) {
        auto geom = geolistfacade.getGeometryFacadeFromGeoId(GeoId);

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

    bool sketchFullyConstrained =
        ViewProviderSketchCoinAttorney::isSketchFullyConstrained(viewProvider);

    // Update Colors

    SbColor* crosscolor = editModeScenegraphNodes.RootCrossMaterials->diffuseColor.startEditing();
    auto viewOrientationFactor =
        ViewProviderSketchCoinAttorney::getViewOrientationFactor(viewProvider);

    for (auto l = 0; l < geometryLayerParameters.getCoinLayerCount(); l++) {
        float x, y, z;
        int PtNum = editModeScenegraphNodes.PointsMaterials[l]->diffuseColor.getNum();
        SbColor* pcolor = editModeScenegraphNodes.PointsMaterials[l]->diffuseColor.startEditing();
        SbVec3f* pverts = editModeScenegraphNodes.PointsCoordinate[l]->point.startEditing();

        // colors of the point set
        for (int i = 0; i < PtNum; i++) {
            if (!coinMapping.isValidPointId(i, l)) {
                continue;
            }

            int GeoId = coinMapping.getPointGeoId(i, l);
            Sketcher::PointPos PosId = coinMapping.getPointPosId(i, l);
            bool isExternal = GeoId < -1;

            if (isExternal) {
                if (isCoincident(GeoId, PosId) && !issketchinvalid) {
                    pcolor[i] = drawingParameters.ConstrIcoColor;
                }
                else {
                    pcolor[i] = isExternalDefiningGeomPoint(GeoId)
                        ? drawingParameters.CurveExternalDefiningColor
                        : drawingParameters.CurveExternalColor;
                }
            }
            else if (issketchinvalid) {
                pcolor[i] = drawingParameters.InvalidSketchColor;
            }
            else if (!(i == 0 && l == 0) && sketchFullyConstrained) {
                // root point is not coloured nor external
                pcolor[i] = drawingParameters.FullyConstrainedColor;
            }
            else {
                bool constrainedElement = isFullyConstraintElement(GeoId);

                if (isInternalAlignedGeom(GeoId)) {
                    if (constrainedElement) {
                        pcolor[i] = drawingParameters.FullyConstraintInternalAlignmentColor;
                    }
                    else {
                        if (isCoincident(GeoId, PosId)) {
                            pcolor[i] = drawingParameters.ConstrIcoColor;
                        }
                        else {
                            pcolor[i] = drawingParameters.InternalAlignedGeoColor;
                        }
                    }
                }
                else {
                    if (!isDefinedGeomPoint(GeoId, PosId)) {
                        if (constrainedElement) {
                            pcolor[i] = drawingParameters.FullyConstraintConstructionElementColor;
                        }
                        else {
                            if (isCoincident(GeoId, PosId)) {
                                pcolor[i] = drawingParameters.ConstrIcoColor;
                            }
                            else {
                                pcolor[i] = drawingParameters.CurveDraftColor;
                            }
                        }
                    }
                    else {  // this is a defined GeomPoint
                        if (constrainedElement) {
                            pcolor[i] = drawingParameters.FullyConstraintElementColor;
                        }
                        else {
                            if (isCoincident(GeoId, PosId)) {
                                pcolor[i] = drawingParameters.ConstrIcoColor;
                            }
                            else {
                                pcolor[i] = drawingParameters.CurveColor;
                            }
                        }
                    }
                }
            }
        }

        // update rendering height of points

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
                                           drawingParameters.zMidPoints,
                                           drawingParameters.zMidPoints);

        float zConstrPoint = getRenderHeight(DrawingParameters::GeometryRendering::Construction,
                                             drawingParameters.zHighPoints,
                                             drawingParameters.zMidPoints,
                                             drawingParameters.zMidPoints);

        for (int i = 0; i < PtNum; i++) {  // 0 is the origin
            if (i == 0 && l == 0) {        // reset root point to lowest
                pverts[i].setValue(0, 0, viewOrientationFactor * drawingParameters.zRootPoint);
            }
            else {
                if (!coinMapping.isValidPointId(i, l)) {
                    continue;
                }

                int GeoId = coinMapping.getPointGeoId(i, l);
                Sketcher::PointPos PosId = coinMapping.getPointPosId(i, l);
                pverts[i].getValue(x, y, z);
                auto geom = geolistfacade.getGeometryFacadeFromGeoId(GeoId);
                bool isExternal = GeoId < -1;

                if (geom) {
                    z = viewOrientationFactor * zNormPoint;

                    if (isCoincident(GeoId, PosId)) {
                        z = viewOrientationFactor * drawingParameters.zLowPoints;
                    }
                    else {
                        if (isExternal || isInternalAlignedGeom(GeoId)) {
                            z = viewOrientationFactor * drawingParameters.zRootPoint;
                        }
                        else if (geom->getConstruction()) {
                            z = viewOrientationFactor * zConstrPoint;
                        }
                    }
                    pverts[i].setValue(x, y, z);
                }
            }
        }

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

        for (auto t = 0; t < geometryLayerParameters.getSubLayerCount(); t++) {
            int CurvNum = editModeScenegraphNodes.CurvesMaterials[l][t]->diffuseColor.getNum();
            SbColor* color =
                editModeScenegraphNodes.CurvesMaterials[l][t]->diffuseColor.startEditing();
            SbVec3f* verts = editModeScenegraphNodes.CurvesCoordinate[l][t]->point.startEditing();

            int j = 0;  // vertexindex
            for (int i = 0; i < CurvNum; i++) {
                if (!coinMapping.isValidCurveId(i, l, t)) {
                    continue;
                }

                int GeoId = coinMapping.getCurveGeoId(i, l, t);
                // CurvId has several vertices associated to 1 material
                // edit->CurveSet->numVertices => [i] indicates number of vertex for line i.
                int indexes = (editModeScenegraphNodes.CurveSet[l][t]->numVertices[i]);

                bool selected =
                    ViewProviderSketchCoinAttorney::isCurveSelected(viewProvider, GeoId);
                bool preselected = (preselectcurve == GeoId);
                bool constrainedElement = isFullyConstraintElement(GeoId);
                bool isExternal = GeoId < -1;

                if (selected || preselected) {
                    color[i] = selected ? (preselected ? drawingParameters.PreselectSelectedColor
                                                       : drawingParameters.SelectColor)
                                        : drawingParameters.PreselectColor;

                    for (int k = j; j < k + indexes; j++) {
                        verts[j].getValue(x, y, z);
                        verts[j] =
                            SbVec3f(x, y, viewOrientationFactor * drawingParameters.zHighLine);
                    }
                }
                else if (isExternal) {
                    auto geom = geolistfacade.getGeometryFacadeFromGeoId(GeoId);
                    auto egf = ExternalGeometryFacade::getFacade(geom->clone());
                    auto ref = egf->getRef();
                    if (egf->testFlag(ExternalGeometryExtension::Missing)) {
                        color[i] = drawingParameters.InvalidSketchColor;
                    }
                    else {
                        color[i] = egf->testFlag(ExternalGeometryExtension::Defining)
                            ? drawingParameters.CurveExternalDefiningColor
                            : drawingParameters.CurveExternalColor;
                    }
                    for (int k = j; j < k + indexes; j++) {
                        verts[j].getValue(x, y, z);
                        verts[j] = SbVec3f(x, y, viewOrientationFactor * zExtLine);
                    }
                }
                else {
                    if (issketchinvalid) {
                        color[i] = drawingParameters.InvalidSketchColor;

                        for (int k = j; j < k + indexes; j++) {
                            verts[j].getValue(x, y, z);
                            verts[j] = SbVec3f(x, y, viewOrientationFactor * zNormLine);
                        }
                    }
                    else if (geometryLayerParameters.isConstructionSubLayer(t)) {
                        if (constrainedElement) {
                            color[i] = drawingParameters.FullyConstraintConstructionElementColor;
                        }
                        else {
                            color[i] = drawingParameters.CurveDraftColor;
                        }

                        for (int k = j; j < k + indexes; j++) {
                            verts[j].getValue(x, y, z);
                            verts[j] = SbVec3f(x, y, viewOrientationFactor * zConstrLine);
                        }
                    }
                    else if (geometryLayerParameters.isInternalSubLayer(t)) {
                        if (constrainedElement) {
                            color[i] = drawingParameters.FullyConstraintInternalAlignmentColor;
                        }
                        else {
                            color[i] = drawingParameters.InternalAlignedGeoColor;
                        }

                        for (int k = j; j < k + indexes; j++) {
                            verts[j].getValue(x, y, z);
                            verts[j] = SbVec3f(x, y, viewOrientationFactor * zConstrLine);
                        }
                    }
                    else {
                        if (sketchFullyConstrained) {
                            color[i] = drawingParameters.FullyConstrainedColor;
                        }
                        else if (constrainedElement) {
                            color[i] = drawingParameters.FullyConstraintElementColor;
                        }
                        else {
                            color[i] = drawingParameters.CurveColor;
                        }

                        for (int k = j; j < k + indexes; j++) {
                            verts[j].getValue(x, y, z);
                            verts[j] = SbVec3f(x, y, viewOrientationFactor * zNormLine);
                        }
                    }
                }
            }

            editModeScenegraphNodes.CurvesMaterials[l][t]->diffuseColor.finishEditing();
            editModeScenegraphNodes.CurvesCoordinate[l][t]->point.finishEditing();
            editModeScenegraphNodes.CurveSet[l][t]->numVertices.finishEditing();
        }

        // colors of the cross
        if (l == 0) {  // only in layer 0
            if (ViewProviderSketchCoinAttorney::isCurveSelected(viewProvider,
                                                                Sketcher::GeoEnum::HAxis)) {
                crosscolor[0] = drawingParameters.SelectColor;
            }
            else if (preselectcross == 1) {  // cross only in layer 0
                crosscolor[0] = drawingParameters.PreselectColor;
            }
            else {
                crosscolor[0] = drawingParameters.CrossColorH;
            }

            if (ViewProviderSketchCoinAttorney::isCurveSelected(viewProvider,
                                                                Sketcher::GeoEnum::VAxis)) {
                crosscolor[1] = drawingParameters.SelectColor;
            }
            else if (preselectcross == 2) {
                crosscolor[1] = drawingParameters.PreselectColor;
            }
            else {
                crosscolor[1] = drawingParameters.CrossColorV;
            }
        }

        editModeScenegraphNodes.PointsMaterials[l]->diffuseColor.finishEditing();
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
    editModeScenegraphNodes.CurvesDrawStyle = new SoDrawStyle;
    editModeScenegraphNodes.CurvesDrawStyle->setName("CurvesDrawStyle");
    editModeScenegraphNodes.CurvesDrawStyle->lineWidth =
        drawingParameters.CurveWidth * drawingParameters.pixelScalingFactor;
    editModeScenegraphNodes.CurvesDrawStyle->linePattern = drawingParameters.CurvePattern;
    editModeScenegraphNodes.CurvesDrawStyle->linePatternScaleFactor = 2;

    editModeScenegraphNodes.CurvesConstructionDrawStyle = new SoDrawStyle;
    editModeScenegraphNodes.CurvesConstructionDrawStyle->setName("CurvesConstructionDrawStyle");
    editModeScenegraphNodes.CurvesConstructionDrawStyle->lineWidth =
        drawingParameters.ConstructionWidth * drawingParameters.pixelScalingFactor;
    editModeScenegraphNodes.CurvesConstructionDrawStyle->linePattern =
        drawingParameters.ConstructionPattern;
    editModeScenegraphNodes.CurvesConstructionDrawStyle->linePatternScaleFactor = 2;

    editModeScenegraphNodes.CurvesInternalDrawStyle = new SoDrawStyle;
    editModeScenegraphNodes.CurvesInternalDrawStyle->setName("CurvesInternalDrawStyle");
    editModeScenegraphNodes.CurvesInternalDrawStyle->lineWidth =
        drawingParameters.InternalWidth * drawingParameters.pixelScalingFactor;
    editModeScenegraphNodes.CurvesInternalDrawStyle->linePattern =
        drawingParameters.InternalPattern;
    editModeScenegraphNodes.CurvesInternalDrawStyle->linePatternScaleFactor = 2;

    editModeScenegraphNodes.CurvesExternalDrawStyle = new SoDrawStyle;
    editModeScenegraphNodes.CurvesExternalDrawStyle->setName("CurvesExternalDrawStyle");
    editModeScenegraphNodes.CurvesExternalDrawStyle->lineWidth =
        drawingParameters.ExternalWidth * drawingParameters.pixelScalingFactor;
    editModeScenegraphNodes.CurvesExternalDrawStyle->linePattern =
        drawingParameters.ExternalPattern;
    editModeScenegraphNodes.CurvesExternalDrawStyle->linePatternScaleFactor = 2;

    editModeScenegraphNodes.CurvesExternalDefiningDrawStyle = new SoDrawStyle;
    editModeScenegraphNodes.CurvesExternalDefiningDrawStyle->setName(
        "CurvesExternalDefiningDrawStyle");
    editModeScenegraphNodes.CurvesExternalDefiningDrawStyle->lineWidth =
        drawingParameters.ExternalDefiningWidth * drawingParameters.pixelScalingFactor;
    editModeScenegraphNodes.CurvesExternalDefiningDrawStyle->linePattern =
        drawingParameters.ExternalDefiningPattern;
    editModeScenegraphNodes.CurvesExternalDefiningDrawStyle->linePatternScaleFactor = 2;

    for (int i = 0; i < geometryLayerParameters.getCoinLayerCount(); i++) {
        editModeScenegraphNodes.CurvesMaterials.emplace_back();
        editModeScenegraphNodes.CurvesCoordinate.emplace_back();
        editModeScenegraphNodes.CurveSet.emplace_back();
        for (int t = 0; t < geometryLayerParameters.getSubLayerCount(); t++) {
            SoSeparator* sep = new SoSeparator;
            sep->ref();

            auto somaterial = new SoMaterial;
            somaterial->setName(concat("CurvesMaterials", i * 10 + t).c_str());
            editModeScenegraphNodes.CurvesMaterials[i].push_back(somaterial);
            sep->addChild(editModeScenegraphNodes.CurvesMaterials[i][t]);

            auto MtlBind = new SoMaterialBinding;
            MtlBind->setName(concat("CurvesMaterialsBinding", i * 10 + t).c_str());
            MtlBind->value = SoMaterialBinding::PER_FACE;
            sep->addChild(MtlBind);

            auto coords = new SoCoordinate3;
            coords->setName(concat("CurvesCoordinate", i * 10 + t).c_str());
            editModeScenegraphNodes.CurvesCoordinate[i].push_back(coords);
            sep->addChild(editModeScenegraphNodes.CurvesCoordinate[i][t]);

            if (geometryLayerParameters.isConstructionSubLayer(t)) {
                sep->addChild(editModeScenegraphNodes.CurvesConstructionDrawStyle);
            }
            else if (geometryLayerParameters.isInternalSubLayer(t)) {
                sep->addChild(editModeScenegraphNodes.CurvesInternalDrawStyle);
            }
            else if (geometryLayerParameters.isExternalSubLayer(t)) {
                sep->addChild(editModeScenegraphNodes.CurvesExternalDrawStyle);
            }
            else if (geometryLayerParameters.isExternalDefiningSubLayer(t)) {
                sep->addChild(editModeScenegraphNodes.CurvesExternalDefiningDrawStyle);
            }
            else {
                sep->addChild(editModeScenegraphNodes.CurvesDrawStyle);
            }

            auto solineset = new SoLineSet;
            solineset->setName(concat("CurvesLineSet", i * 10 + t).c_str());
            editModeScenegraphNodes.CurveSet[i].push_back(solineset);
            sep->addChild(editModeScenegraphNodes.CurveSet[i][t]);

            editModeScenegraphNodes.CurvesGroup->addChild(sep);
            sep->unref();
        }
    }
}
