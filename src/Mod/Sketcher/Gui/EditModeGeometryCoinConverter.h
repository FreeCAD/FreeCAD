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

#include <vector>

#include "ViewProviderSketch.h"


namespace Base
{
template<typename T>
class Vector3;

class Vector2d;
}  // namespace Base

namespace Sketcher
{
enum ConstraintType : int;
enum class PointPos : int;
}  // namespace Sketcher

namespace Part
{
class Geometry;
}

namespace SketcherGui
{
class ViewProviderSketch;
struct GeometryLayerNodes;
struct DrawingParameters;
class GeometryLayerParameters;
struct CoinMapping;

/** @brief      Class for creating the Geometry layer into coin nodes
 *  @details
 * Responsibility:
 * To create and update GeometryLayer nodes provided as constructor parameter
 * for the provided geometry, taking into account the drawing parameters provided as
 * constructor parameters.
 *
 * Interface:
 * A single entry point convert(), performing the following flow:
 *
 * [Geometry] => Analysis => construct drawing elements => Create mappings GeoId coin => populate
 * coin nodes
 *
 * Analysis performs analysis such as maximum boundingbox magnitude of all geometries and maximum
 * curvature of BSplines
 */
class EditModeGeometryCoinConverter
{
    // These internal private classes are used to parametrize the conversion of geometry into points
    // and line sets (see template method convert)
private:
    enum class PointsMode
    {
        InsertSingle,
        InsertStartEnd,
        InsertStartEndMid,
        InsertMidOnly
    };

    enum class CurveMode
    {
        NoCurve,
        StartEndPointsOnly,
        ClosedCurve,
        OpenCurve
    };

    enum class AnalyseMode
    {
        BoundingBoxMagnitude,
        BoundingBoxMagnitudeAndBSplineCurvature
    };

public:
    /** Constructs an GeometryCoinConverter responsible for
     * generating the points and line sets for drawing the geometry
     * defined by a GeometryLayer into the coin nodes provided by
     * GeometryLayerNodes.
     *
     * @param geometrylayernodes: The coin nodes to be populated with
     * the geometry
     *
     * @param drawingparameters: Parameters for drawing the overlay information
     */
    EditModeGeometryCoinConverter(
        ViewProviderSketch& vp,
        GeometryLayerNodes& geometrylayernodes,
        DrawingParameters& drawingparameters,
        GeometryLayerParameters& geometryLayerParams,
        CoinMapping& coinMap
    );

    /**
     * converts the geometry defined by GeometryLayer into the coin nodes.
     *
     * @param geometry: the geometry to be processed
     */
    void convert(const Sketcher::GeoListFacade& geolistfacade);

    /**
     * returns the maximum of the vertical and horizontal magnitudes of the
     * coordinates of the points and lines added to coin by this layer (local responsibility).
     */
    float getBoundingBoxMaxMagnitude();

    /**
     * returns the Comb representation scale that should be used to represent
     * the B-Splines of this layer (local responsibility).
     */
    double getCombRepresentationScale();

    /**
     * returns the GeoIds of BSpline geometries
     */
    auto getBSplineGeoIds()
    {
        return std::move(bsplineGeoIds);
    }

    /**
     * returns the GeoIds of Arc geometries
     */
    auto getArcGeoIds()
    {
        return std::move(arcGeoIds);
    }

private:
    template<typename GeoType, PointsMode pointmode, CurveMode curvemode, AnalyseMode analysemode>
    void convert(
        const Sketcher::GeometryFacade* geometryfacade,
        [[maybe_unused]] int geoId,
        [[maybe_unused]] int subLayerId = 0
    );

private:
    /// Reference to ViewProviderSketch in order to access the public and the Attorney Interface
    ViewProviderSketch& viewProvider;

    GeometryLayerNodes& geometryLayerNodes;

    std::vector<std::vector<Base::Vector3d>> Points;
    std::vector<std::vector<std::vector<Base::Vector3d>>> Coords;
    std::vector<std::vector<std::vector<unsigned int>>> Index;

    // temporal counters, one per layer
    std::vector<int> pointCounter;

    // temporal global vertex counter
    int vertexCounter = 0;

    // Parameters
    DrawingParameters& drawingParameters;
    GeometryLayerParameters& geometryLayerParameters;
    // Mappings coin geoId
    CoinMapping& coinMapping;

    // measurements
    float boundingBoxMaxMagnitude = 100;
    double combrepscale = 0;  // the repscale that would correspond to this comb based only on this
                              // calculation.
    std::vector<int> bsplineGeoIds;
    std::vector<int> arcGeoIds;
};


}  // namespace SketcherGui
