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
# include <Inventor/nodes/SoSeparator.h>
# include <Inventor/nodes/SoGroup.h>
# include <Inventor/nodes/SoSwitch.h>
# include <Inventor/nodes/SoMaterial.h>
# include <Inventor/nodes/SoCoordinate3.h>
# include <Inventor/nodes/SoLineSet.h>
# include <Inventor/nodes/SoFont.h>

# include <Inventor/nodes/SoMarkerSet.h>
# include <Inventor/nodes/SoTranslation.h>
# include <Inventor/nodes/SoText2.h>
# include <Inventor/nodes/SoPickStyle.h>
# include <Inventor/nodes/SoDrawStyle.h>
# include <Inventor/SoPickedPoint.h>
# include <Inventor/details/SoPointDetail.h>
# include <Inventor/details/SoDetail.h>
# include <Inventor/details/SoLineDetail.h>

# include <Inventor/nodes/SoAnnotation.h>
# include <Inventor/nodes/SoImage.h>
# include <Inventor/nodes/SoInfo.h>

# include <Inventor/actions/SoRayPickAction.h>

# include <Inventor/SbVec3f.h>
# include <Inventor/SbImage.h>

# include <memory>
#endif  // #ifndef _PreComp_

#include <Gui/Inventor/SmSwitchboard.h>

#include <Mod/Part/App/Geometry.h>
#include <Mod/Sketcher/App/GeometryFacade.h>
#include <Mod/Sketcher/App/SolverGeometryExtension.h>
#include <Mod/Sketcher/App/GeoEnum.h>
#include <Mod/Sketcher/App/Constraint.h>
#include <Mod/Sketcher/App/GeoList.h>

#include <Base/Exception.h>
#include <Base/Tools2D.h>
#include <Base/UnitsApi.h>
#include <Gui/Utilities.h>
#include <Base/Converter.h>
#include <Base/Tools.h>

#include <Base/Vector3D.h>

#include <App/ObjectIdentifier.h>

#include <Gui/SoFCBoundingBox.h>
#include <Gui/BitmapFactory.h>
#include <Gui/Inventor/MarkerBitmaps.h>
#include <Gui/Tools.h>

#include <qpainter.h>

#include "SoZoomTranslation.h"
#include "SoDatumLabel.h"

#include "EditModeInformationOverlayCoinConverter.h"

#include "EditModeGeometryCoinConverter.h"

#include "ViewProviderSketch.h"

#include "ViewProviderSketchCoinAttorney.h"

#include "EditModeConstraintCoinManager.h"

#include "EditModeGeometryCoinManager.h"

using namespace SketcherGui;
using namespace Sketcher;

//**************************** EditModeGeometryCoinManager class ******************************

EditModeGeometryCoinManager::EditModeGeometryCoinManager(   ViewProviderSketch &vp,
                                                            DrawingParameters & drawingParams,
                                                            GeometryLayerParameters & geometryLayerParams,
                                                            AnalysisResults & analysisResultStruct,
                                                            EditModeScenegraphNodes & editModeScenegraph,
                                                            CoinMapping & coinMap):
    viewProvider(vp),
    drawingParameters(drawingParams),
    geometryLayerParameters(geometryLayerParams),
    analysisResults(analysisResultStruct),
    editModeScenegraphNodes(editModeScenegraph),
    coinMapping(coinMap)
{}

EditModeGeometryCoinManager::~EditModeGeometryCoinManager()
{}

void EditModeGeometryCoinManager::processGeometry(const GeoListFacade & geolistfacade)
{
    // enable all layers
    editModeScenegraphNodes.PointsGroup->enable.setNum(geometryLayerParameters.CoinLayers);
    editModeScenegraphNodes.CurvesGroup->enable.setNum(geometryLayerParameters.CoinLayers);
    SbBool *swsp = editModeScenegraphNodes.PointsGroup->enable.startEditing();
    SbBool *swsc = editModeScenegraphNodes.CurvesGroup->enable.startEditing();

    for(int l=0; l<geometryLayerParameters.CoinLayers; l++){
        swsp[l] = true; // layer defaults to enabled
        swsc[l] = true; // layer defaults to enabled
    }

    editModeScenegraphNodes.PointsGroup->enable.finishEditing();
    editModeScenegraphNodes.CurvesGroup->enable.finishEditing();

    // Define the coin nodes that will be filled in with the geometry layers
    GeometryLayerNodes geometrylayernodes {
        editModeScenegraphNodes.PointsMaterials,
        editModeScenegraphNodes.PointsCoordinate,
        editModeScenegraphNodes.CurvesMaterials,
        editModeScenegraphNodes.CurvesCoordinate,
        editModeScenegraphNodes.CurveSet
    };

    // process geometry layers
    EditModeGeometryCoinConverter gcconv(geometrylayernodes, drawingParameters, geometryLayerParameters, coinMapping);

    gcconv.convert(geolistfacade);

    // set cross coordinates
    editModeScenegraphNodes.RootCrossSet->numVertices.set1Value(0,2);
    editModeScenegraphNodes.RootCrossSet->numVertices.set1Value(1,2);

    analysisResults.combRepresentationScale = gcconv.getCombRepresentationScale();
    analysisResults.boundingBoxMagnitudeOrder = exp(ceil(log(std::abs(gcconv.getBoundingBoxMaxMagnitude()))));
    analysisResults.bsplineGeoIds = gcconv.getBSplineGeoIds();
}

void EditModeGeometryCoinManager::updateGeometryColor(const GeoListFacade & geolistfacade, bool issketchinvalid)
{
    // Lambdas for convenience retrieval of geometry information
    auto isConstructionGeom = [&geolistfacade](int GeoId) {
        auto geom = geolistfacade.getGeometryFacadeFromGeoId(GeoId);
        if (geom)
            return geom->getConstruction();
        return false;
    };

    auto isDefinedGeomPoint = [&geolistfacade](int GeoId) {
        auto geom = geolistfacade.getGeometryFacadeFromGeoId(GeoId);
        if (geom)
            return geom->isGeoType(Part::GeomPoint::getClassTypeId()) && !geom->getConstruction();
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

        if(geom) {
            if(geom->hasExtension(Sketcher::SolverGeometryExtension::getClassTypeId())) {

                auto solvext = std::static_pointer_cast<const Sketcher::SolverGeometryExtension>(
                                    geom->getExtension(Sketcher::SolverGeometryExtension::getClassTypeId()).lock());

                return (solvext->getGeometry() == Sketcher::SolverGeometryExtension::FullyConstraint);
            }
        }
        return false;
    };

    // Update Colors

    SbColor *crosscolor = editModeScenegraphNodes.RootCrossMaterials->diffuseColor.startEditing();

    for(int l=0; l<geometryLayerParameters.CoinLayers; l++) {

        int PtNum = editModeScenegraphNodes.PointsMaterials[l]->diffuseColor.getNum();
        SbColor *pcolor = editModeScenegraphNodes.PointsMaterials[l]->diffuseColor.startEditing();
        int CurvNum = editModeScenegraphNodes.CurvesMaterials[l]->diffuseColor.getNum();
        SbColor *color = editModeScenegraphNodes.CurvesMaterials[l]->diffuseColor.startEditing();

        SbVec3f *verts = editModeScenegraphNodes.CurvesCoordinate[l]->point.startEditing();
        SbVec3f *pverts = editModeScenegraphNodes.PointsCoordinate[l]->point.startEditing();

        float x,y,z;

        // colors of the point set
        if( issketchinvalid ) {
            for (int  i=0; i < PtNum; i++)
                pcolor[i] = drawingParameters.InvalidSketchColor;
        }
        else {

            for (int  i=0; i < PtNum; i++) {
                if ( !(i == 0 && l == 0) && ViewProviderSketchCoinAttorney::isSketchFullyConstrained(viewProvider)) {// root point is not coloured
                    pcolor[i] = drawingParameters.FullyConstrainedColor;
                }
                else {
                    int GeoId = coinMapping.getPointGeoId(i, l);

                    bool constrainedElement = isFullyConstraintElement(GeoId);

                    if(isInternalAlignedGeom(GeoId)) {
                        if(constrainedElement)
                            pcolor[i] = drawingParameters.FullyConstraintInternalAlignmentColor;
                        else
                            pcolor[i] = drawingParameters.InternalAlignedGeoColor;
                    }
                    else {
                        if(!isDefinedGeomPoint(GeoId)) {
                            if(constrainedElement)
                                pcolor[i] = drawingParameters.FullyConstraintConstructionPointColor;
                            else
                                pcolor[i] = drawingParameters.VertexColor;
                        }
                        else { // this is a defined GeomPoint
                            if(constrainedElement)
                                pcolor[i] = drawingParameters.FullyConstraintElementColor;
                            else
                                pcolor[i] = drawingParameters.CurveColor;
                        }
                    }
                }
            }
        }

        // update rendering height of points

        auto getRenderHeight = [this](DrawingParameters::GeometryRendering renderingtype, float toprendering, float midrendering, float lowrendering) {
            if(drawingParameters.topRenderingGeometry == renderingtype)
                return toprendering;
            else if(drawingParameters.midRenderingGeometry == renderingtype)
                return midrendering;
            else
                return lowrendering;
        };

        float zNormPoint = getRenderHeight(DrawingParameters::GeometryRendering::NormalGeometry,
                                        drawingParameters.zHighPoints,
                                        drawingParameters.zLowPoints,
                                        drawingParameters.zLowPoints);

        float zConstrPoint = getRenderHeight(DrawingParameters::GeometryRendering::Construction,
                                        drawingParameters.zHighPoints,
                                        drawingParameters.zLowPoints,
                                        drawingParameters.zLowPoints);

        for (int  i=0; i < PtNum; i++) { // 0 is the origin
            if( i == 0 && l == 0 ) { // reset root point to lowest
                pverts[i].setValue(0, 0, drawingParameters.zRootPoint);
            }
            else {
                pverts[i].getValue(x,y,z);
                auto geom = geolistfacade.getGeometryFacadeFromGeoId(coinMapping.getPointGeoId(i, l));

                if(geom) {
                    if(geom->getConstruction())
                        pverts[i].setValue(x,y,zConstrPoint);
                    else
                        pverts[i].setValue(x,y,zNormPoint);
                }
            }
        }

        auto preselectpoint = ViewProviderSketchCoinAttorney::getPreselectPoint(viewProvider);
        auto preselectcross = ViewProviderSketchCoinAttorney::getPreselectCross(viewProvider);
        auto preselectcurve = ViewProviderSketchCoinAttorney::getPreselectCurve(viewProvider);

        MultiFieldId preselectpointmfid;

        if ( preselectcross == 0) {
            if(l == 0) // cross only in layer 0
                pcolor[0] = drawingParameters.PreselectColor;
        }
        else if (preselectpoint != -1) {
            preselectpointmfid = coinMapping.getIndexLayer(preselectpoint);
            if (MultiFieldId::Invalid != preselectpointmfid &&
                preselectpointmfid.layerId == l &&
                preselectpointmfid.fieldIndex < PtNum)
                pcolor[preselectpointmfid.fieldIndex] = drawingParameters.PreselectColor;
        }

        ViewProviderSketchCoinAttorney::executeOnSelectionPointSet(viewProvider,
            [pcolor, PtNum, preselectpointmfid, layerId = l, &coinMapping = coinMapping, drawingParameters = this->drawingParameters](const int i) {
                auto pointindex = coinMapping.getIndexLayer(i);
                if (layerId == pointindex.layerId && pointindex.fieldIndex >= 0 && pointindex.fieldIndex < PtNum) {
                    pcolor[pointindex.fieldIndex] = (preselectpointmfid == pointindex)
                        ? drawingParameters.PreselectSelectedColor : drawingParameters.SelectColor;
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

        int j=0; // vertexindex

        for (int  i=0; i < CurvNum; i++) {
            int GeoId = coinMapping.getCurveGeoId(i, l);
            // CurvId has several vertices associated to 1 material
            //edit->CurveSet->numVertices => [i] indicates number of vertex for line i.
            int indexes = (editModeScenegraphNodes.CurveSet[l]->numVertices[i]);

            bool selected = ViewProviderSketchCoinAttorney::isCurveSelected(viewProvider, GeoId);
            bool preselected = (preselectcurve == GeoId);

            bool constrainedElement = isFullyConstraintElement(GeoId);

            if (selected && preselected) {
                color[i] = drawingParameters.PreselectSelectedColor;
                for (int k=j; j<k+indexes; j++) {
                    verts[j].getValue(x,y,z);
                    verts[j] = SbVec3f(x,y,drawingParameters.zHighLine);
                }
            }
            else if (selected){
                color[i] = drawingParameters.SelectColor;
                for (int k=j; j<k+indexes; j++) {
                    verts[j].getValue(x,y,z);
                    verts[j] = SbVec3f(x,y,drawingParameters.zHighLine);
                }
            }
            else if (preselected){
                color[i] = drawingParameters.PreselectColor;
                for (int k=j; j<k+indexes; j++) {
                    verts[j].getValue(x,y,z);
                    verts[j] = SbVec3f(x,y,drawingParameters.zHighLine);
                }
            }
            else if (GeoId <= Sketcher::GeoEnum::RefExt) {  // external Geometry
                color[i] = drawingParameters.CurveExternalColor;
                for (int k=j; j<k+indexes; j++) {
                    verts[j].getValue(x,y,z);
                    verts[j] = SbVec3f(x,y,zExtLine);
                }
            }
            else if ( issketchinvalid ) {
                color[i] = drawingParameters.InvalidSketchColor;
                for (int k=j; j<k+indexes; j++) {
                    verts[j].getValue(x,y,z);
                    verts[j] = SbVec3f(x,y,zNormLine);
                }
            }
            else if (isConstructionGeom(GeoId)) {
                if(isInternalAlignedGeom(GeoId)) {
                    if(constrainedElement)
                        color[i] = drawingParameters.FullyConstraintInternalAlignmentColor;
                    else
                        color[i] = drawingParameters.InternalAlignedGeoColor;
                }
                else {
                    if(constrainedElement)
                        color[i] = drawingParameters.FullyConstraintConstructionElementColor;
                    else
                        color[i] = drawingParameters.CurveDraftColor;
                }

                for (int k=j; j<k+indexes; j++) {
                    verts[j].getValue(x,y,z);
                    verts[j] = SbVec3f(x,y,zConstrLine);
                }
            }
            else if (ViewProviderSketchCoinAttorney::isSketchFullyConstrained(viewProvider)) {
                color[i] = drawingParameters.FullyConstrainedColor;
                for (int k=j; j<k+indexes; j++) {
                    verts[j].getValue(x,y,z);
                    verts[j] = SbVec3f(x,y,zNormLine);
                }
            }
            else if (isFullyConstraintElement(GeoId)) {
                color[i] = drawingParameters.FullyConstraintElementColor;
                for (int k=j; j<k+indexes; j++) {
                    verts[j].getValue(x,y,z);
                    verts[j] = SbVec3f(x,y,zNormLine);
                }
            }
            else {
                color[i] = drawingParameters.CurveColor;
                for (int k=j; j<k+indexes; j++) {
                    verts[j].getValue(x,y,z);
                    verts[j] = SbVec3f(x,y,zNormLine);
                }
            }
        }

        // colors of the cross
        if ( l == 0 ) { // only in layer 0
            if (ViewProviderSketchCoinAttorney::isCurveSelected(viewProvider, Sketcher::GeoEnum::HAxis)) {
                    crosscolor[0] = drawingParameters.SelectColor;
            }
            else if (preselectcross == 1) { // cross only in layer 0
                    crosscolor[0] = drawingParameters.PreselectColor;
            }
            else {
                    crosscolor[0] = drawingParameters.CrossColorH;
            }

            if (ViewProviderSketchCoinAttorney::isCurveSelected(viewProvider, Sketcher::GeoEnum::VAxis)) {
                crosscolor[1] = drawingParameters.SelectColor;
            }
            else if (preselectcross == 2) {
                crosscolor[1] = drawingParameters.PreselectColor;
            }
            else {
                crosscolor[1] = drawingParameters.CrossColorV;
            }
        }

        // end editing
        editModeScenegraphNodes.CurvesMaterials[l]->diffuseColor.finishEditing();
        editModeScenegraphNodes.PointsMaterials[l]->diffuseColor.finishEditing();
        editModeScenegraphNodes.CurvesCoordinate[l]->point.finishEditing();
        editModeScenegraphNodes.CurveSet[l]->numVertices.finishEditing();
    }

    editModeScenegraphNodes.RootCrossMaterials->diffuseColor.finishEditing();
}


void EditModeGeometryCoinManager::createEditModeInventorNodes()
{
    // stuff for the points ++++++++++++++++++++++++++++++++++++++
    editModeScenegraphNodes.PointsGroup = new SmSwitchboard;
    editModeScenegraphNodes.EditRoot->addChild(editModeScenegraphNodes.PointsGroup);

    auto concat = [](std::string string, int i) {
        return string+std::to_string(i);
    };

    for(int i=0; i < geometryLayerParameters.CoinLayers; i++) {
        SoSeparator * sep = new SoSeparator;
        sep->ref();

        auto somaterial = new SoMaterial;
        editModeScenegraphNodes.PointsMaterials.push_back(somaterial);
        editModeScenegraphNodes.PointsMaterials[i]->setName(concat("PointsMaterials_",i).c_str());
        sep->addChild(editModeScenegraphNodes.PointsMaterials[i]);

        SoMaterialBinding *MtlBind = new SoMaterialBinding;
        MtlBind->setName(concat("PointsMaterialBinding",i).c_str());
        MtlBind->value = SoMaterialBinding::PER_VERTEX;
        sep->addChild(MtlBind);

        auto coords = new SoCoordinate3;
        editModeScenegraphNodes.PointsCoordinate.push_back(coords);
        editModeScenegraphNodes.PointsCoordinate[i]->setName(concat("PointsCoordinate",i).c_str());
        sep->addChild(editModeScenegraphNodes.PointsCoordinate[i]);

        auto drawstyle = new SoDrawStyle;
        editModeScenegraphNodes.PointsDrawStyle.push_back(drawstyle);
        editModeScenegraphNodes.PointsDrawStyle[i]->setName(concat("PointsDrawStyle",i).c_str());
        editModeScenegraphNodes.PointsDrawStyle[i]->pointSize = 8 * drawingParameters.pixelScalingFactor;
        sep->addChild(editModeScenegraphNodes.PointsDrawStyle[i]);

        auto pointset = new SoMarkerSet;
        editModeScenegraphNodes.PointSet.push_back(pointset);
        editModeScenegraphNodes.PointSet[i]->setName(concat("PointSet",i).c_str());
        editModeScenegraphNodes.PointSet[i]->markerIndex = Gui::Inventor::MarkerBitmaps::getMarkerIndex("CIRCLE_FILLED", drawingParameters.markerSize);
        sep->addChild(editModeScenegraphNodes.PointSet[i]);

        editModeScenegraphNodes.PointsGroup->addChild(sep);
        sep->unref();
    }

    // stuff for the Curves +++++++++++++++++++++++++++++++++++++++
    editModeScenegraphNodes.CurvesGroup = new SmSwitchboard;
    editModeScenegraphNodes.EditRoot->addChild(editModeScenegraphNodes.CurvesGroup);

    for(int i=0; i < geometryLayerParameters.CoinLayers; i++) {
        SoSeparator * sep = new SoSeparator;
        sep->ref();

        auto somaterial = new SoMaterial;
        editModeScenegraphNodes.CurvesMaterials.push_back(somaterial);
        editModeScenegraphNodes.CurvesMaterials[i]->setName(concat("CurvesMaterials",i).c_str());
        sep->addChild(editModeScenegraphNodes.CurvesMaterials[i]);

        auto MtlBind = new SoMaterialBinding;
        MtlBind->setName(concat("CurvesMaterialsBinding",i).c_str());
        MtlBind->value = SoMaterialBinding::PER_FACE;
        sep->addChild(MtlBind);

        auto coords = new SoCoordinate3;
        editModeScenegraphNodes.CurvesCoordinate.push_back(coords);
        editModeScenegraphNodes.CurvesCoordinate[i]->setName(concat("CurvesCoordinate",i).c_str());
        sep->addChild(editModeScenegraphNodes.CurvesCoordinate[i]);

        auto drawstyle = new SoDrawStyle;
        editModeScenegraphNodes.CurvesDrawStyle.push_back(drawstyle);
        editModeScenegraphNodes.CurvesDrawStyle[i]->setName(concat("CurvesDrawStyle",i).c_str());
        editModeScenegraphNodes.CurvesDrawStyle[i]->lineWidth = 3 * drawingParameters.pixelScalingFactor;

        /* Demo code to introduce a dashed line
           if(i == 1) {
            editModeScenegraphNodes.CurvesDrawStyle[i]->linePattern = 0x3CF2;
            editModeScenegraphNodes.CurvesDrawStyle[i]->linePatternScaleFactor = 5;
        }*/

        sep->addChild(editModeScenegraphNodes.CurvesDrawStyle[i]);

        auto solineset = new SoLineSet;
        editModeScenegraphNodes.CurveSet.push_back(solineset);
        editModeScenegraphNodes.CurveSet[i]->setName(concat("CurvesLineSet",i).c_str());
        sep->addChild(editModeScenegraphNodes.CurveSet[i]);

        editModeScenegraphNodes.CurvesGroup->addChild(sep);
        sep->unref();
    }

}
