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

#ifndef SKETCHERGUI_EditModeGeometryCoinManager_H
#define SKETCHERGUI_EditModeGeometryCoinManager_H

#include <functional>
#include <vector>

#include <Mod/Sketcher/App/GeoList.h>

#include "EditModeCoinManagerParameters.h"


class SbVec3f;
class SoRayPickAction;
class SoPickedPoint;
class SbVec3s;

namespace Base
{
template<typename T>
class Vector3;

class Vector2d;

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

using GeoList = Sketcher::GeoList;
using GeoListFacade = Sketcher::GeoListFacade;

/** @brief      Class for managing the Edit mode coin nodes of ViewProviderSketch relating to
 * geometry.
 *  @details
 *
 * EditModeGeometryCoinManager is a helper of EditModeCoinManager specialised in geometry
 * management.
 *
 * Three main functions are delegated to it:
 * 1. Creation of Edit mode coin nodes to handle Geometry representation.
 * 2. Converting Sketcher geometry into Coin information.
 * 3. Updating the Geometry colors.
 *
 * Internally, EditModeGeometryCoinManager uses yet another class for geometry conversion,
 * GeometryCoinConverter.
 *
 */
class SketcherGuiExport EditModeGeometryCoinManager
{

public:
    explicit EditModeGeometryCoinManager(ViewProviderSketch& vp,
                                         DrawingParameters& drawingParams,
                                         GeometryLayerParameters& geometryLayerParams,
                                         AnalysisResults& analysisResultStruct,
                                         EditModeScenegraphNodes& editModeScenegraph,
                                         CoinMapping& coinMap);
    ~EditModeGeometryCoinManager();


    // This function populates the coin nodes with the information of the current geometry
    void processGeometry(const GeoListFacade& geolistfacade);

    void updateGeometryColor(const GeoListFacade& geolistfacade, bool issketchinvalid);

    void updateGeometryLayersConfiguration();

    /** @name coin nodes creation*/
    void createEditModeInventorNodes();
    //@}

private:
    void createGeometryRootNodes();
    void emptyGeometryRootNodes();
    void createEditModePointInventorNodes();
    void createEditModeCurveInventorNodes();

private:
    ViewProviderSketch& viewProvider;

    DrawingParameters& drawingParameters;
    GeometryLayerParameters& geometryLayerParameters;
    AnalysisResults& analysisResults;

    EditModeScenegraphNodes& editModeScenegraphNodes;

    CoinMapping& coinMapping;
};


}  // namespace SketcherGui


#endif  // SKETCHERGUI_EditModeGeometryCoinManager_H
