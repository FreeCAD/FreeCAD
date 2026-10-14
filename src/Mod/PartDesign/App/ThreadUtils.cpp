// SPDX-License-Identifier: LGPL-2.1-or-later

/****************************************************************************
 *                                                                          *
 *   Copyright (c) 2026 Caio Venâncio <caio.venancio784@gmail.com>          *
 *                                                                          *
 *   This file is part of FreeCAD.                                          *
 *                                                                          *
 *   FreeCAD is free software: you can redistribute it and/or modify it     *
 *   under the terms of the GNU Lesser General Public License as            *
 *   published by the Free Software Foundation, either version 2.1 of the   *
 *   License, or (at your option) any later version.                        *
 *                                                                          *
 *   FreeCAD is distributed in the hope that it will be useful, but         *
 *   WITHOUT ANY WARRANTY; without even the implied warranty of             *
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU       *
 *   Lesser General Public License for more details.                        *
 *                                                                          *
 *   You should have received a copy of the GNU Lesser General Public       *
 *   License along with FreeCAD. If not, see                                *
 *   <https://www.gnu.org/licenses/>.                                       *
 *                                                                          *
 ***************************************************************************/

#include <algorithm>
#include <cctype>
#include <iterator>
#include <cmath>
#include <numbers>
#include <limits>
#include <sstream>
#include <stdexcept>
#include <map>
#include <vector>

#include <QObject>

#include <BRep_Tool.hxx>
#include <BRepTools.hxx>
#include <BRepAdaptor_Surface.hxx>
#include <BRepBndLib.hxx>
#include <Geom_ConicalSurface.hxx>
#include <Geom_CylindricalSurface.hxx>
#include <Geom_Plane.hxx>
#include <Geom_Surface.hxx>
#include <Geom_TrimmedCurve.hxx>
#include <GeomAPI_IntCS.hxx>
#include <GeomAbs_SurfaceType.hxx>
#include <gp_Circ.hxx>
#include <gp_Cone.hxx>
#include <gp_Cylinder.hxx>
#include <gp_Dir.hxx>
#include <TopoDS.hxx>
#include <TopoDS_Face.hxx>
#include <TopoDS_Vertex.hxx>
#include <TopoDS_Wire.hxx>
#include <BRepBuilderAPI_MakeWire.hxx>
#include <BRepBuilderAPI_MakeEdge.hxx>
#include <BRepBuilderAPI_MakeFace.hxx>
#include <BRepBuilderAPI_MakeVertex.hxx>
#include <BRepBuilderAPI_Copy.hxx>
#include <BRepBuilderAPI_Transform.hxx>
#include <gp_Ax3.hxx>
#include <BRepTools_WireExplorer.hxx>
#include <GC_MakeArcOfCircle.hxx>
#include <BRepBuilderAPI_MakeSolid.hxx>
#include <BRepClass3d_SolidClassifier.hxx>
#include <BRepAlgoAPI_Cut.hxx>
#include <BRepPrimAPI_MakeCylinder.hxx>
#include <BRepPrimAPI_MakeCone.hxx>
#include <BRepAlgoAPI_Common.hxx>
#include <BRepAdaptor_Curve.hxx>
#include <IntCurvesFace_ShapeIntersector.hxx>
#include <Standard_Failure.hxx>
#include <GeomAPI_ExtremaCurveCurve.hxx>
#include <Geom_Line.hxx>
#include <GeomAdaptor_Curve.hxx>
#include <Precision.hxx>

#include <App/VarSet.h>
#include <App/PropertyLinks.h>
#include <App/DocumentObject.h>
#include "App/Document.h"
#include <Mod/Part/App/TopoShape.h>
#include <Mod/Part/App/TopoShapeOpCode.h>
#include <Mod/Part/App/Part2DObject.h>
#include <Mod/Part/App/FaceMakerCheese.h>
#include <Mod/Part/App/Tools.h>
#include <Base/Tools.h>
#include <Base/Console.h>
#include <Mod/Spreadsheet/App/Sheet.h>
#include <App/PropertyGeo.h>

#include "Feature.h"
#include "ThreadUtils.h"

using namespace PartDesign;

namespace
{
constexpr double threadDefinitionTolerance = 0.001;
constexpr double minorDiameterFuseOverlap = 0.01;

constexpr bool useExtendedWhitworthRoot = true;

bool isWhitworthThread(const std::string& threadType)
{
    return threadType == "BSP" || threadType == "BSW" || threadType == "BSF";
}

const ThreadUtils::ThreadDefinition& checkedThreadDefinition(
    const std::vector<ThreadUtils::ThreadDefinition>& definitions,
    int threadType
)
{
    if (threadType < 0 || static_cast<size_t>(threadType) >= definitions.size()) {
        throw Base::IndexError(QT_TRANSLATE_NOOP("Exception", "Thread type out of range"));
    }
    return definitions[threadType];
}

double parseThreadValue(const std::string& value)
{
    try {
        const double number = std::stod(value);
        if (std::isfinite(number)) {
            return number;
        }
    }
    catch (const std::invalid_argument&) {
    }
    catch (const std::out_of_range&) {
    }
    throw Base::ValueError(
        QT_TRANSLATE_NOOP("Exception", "Thread error: Invalid numeric value in thread definition")
    );
}
}  // namespace

const char* ThreadUtils::ThreadClass_None_Enums[] = {"None", nullptr};

// ISO 965-1:2013 ISO general purpose metric screw threads - Tolerances - Part 1
// Table 1 - Fundamentral deviations for internal threads ...
// reproduced in: https://www.accu.co.uk/en/p/134-iso-metric-thread-tolerances [retrieved: 2021-01-11]
const double ThreadUtils::ThreadClass_ISOmetric_data[ThreadClass_ISOmetric_data_size_utils][2] = {
    {0.2, 0.017},  {0.25, 0.018}, {0.3, 0.018},  {0.35, 0.019}, {0.4, 0.019},
    {0.45, 0.020}, {0.5, 0.020},  {0.6, 0.021},  {0.7, 0.022},  {0.75, 0.022},
    {0.8, 0.024},  {1.0, 0.026},  {1.25, 0.028}, {1.5, 0.032},  {1.75, 0.034},
    {2.0, 0.038},  {2.5, 0.042},  {3.0, 0.048},  {3.5, 0.053},  {4.0, 0.060},
    {4.5, 0.063},  {5.0, 0.071},  {5.5, 0.075},  {6.0, 0.080},  {8.0, 0.100}
};

const double ThreadUtils::ThreadRunout[ThreadRunout_size_utils][2] = {
    {0.2, 1.3},  {0.25, 1.5}, {0.3, 1.8},  {0.35, 2.1}, {0.4, 2.3},  {0.45, 2.6},
    {0.5, 2.8},  {0.6, 3.4},  {0.7, 3.8},  {0.75, 4.0}, {0.8, 4.2},  {1.0, 5.1},
    {1.25, 6.2}, {1.5, 7.3},  {1.75, 8.3}, {2.0, 9.3},  {2.5, 11.2}, {3.0, 13.1},
    {3.5, 15.2}, {4.0, 16.8}, {4.5, 18.4}, {5.0, 20.8}, {5.5, 22.4}, {6.0, 24.0}
};

const char* ThreadUtils::ThreadDirectionEnums[] = {"Right", "Left", nullptr};


const std::vector<ThreadUtils::ThreadDescription> ThreadUtils::threadDescription[] = {
    {
        {"---", 6.0, 0.0, 0.0},
    },

    {
        {"M1x0.25", 1.0, 0.25, 0.75},    {"M1.1x0.25", 1.1, 0.25, 0.85},
        {"M1.2x0.25", 1.2, 0.25, 0.95},  {"M1.4x0.3", 1.4, 0.30, 1.10},
        {"M1.6x0.35", 1.6, 0.35, 1.25},  {"M1.8x0.35", 1.8, 0.35, 1.45},
        {"M2x0.4", 2.0, 0.40, 1.60},     {"M2.2x0.45", 2.2, 0.45, 1.75},
        {"M2.5x0.45", 2.5, 0.45, 2.05},  {"M3x0.5", 3.0, 0.50, 2.50},
        {"M3.5x0.6", 3.5, 0.60, 2.90},   {"M4x0.7", 4.0, 0.70, 3.30},
        {"M4.5x0.75", 4.5, 0.75, 3.70},  {"M5x0.8", 5.0, 0.80, 4.20},
        {"M6x1.0", 6.0, 1.00, 5.00},     {"M7x1.0", 7.0, 1.00, 6.00},
        {"M8x1.25", 8.0, 1.25, 6.80},    {"M9x1.25", 9.0, 1.25, 7.80},
        {"M10x1.5", 10.0, 1.50, 8.50},   {"M11x1.5", 11.0, 1.50, 9.50},
        {"M12x1.75", 12.0, 1.75, 10.20}, {"M14x2.0", 14.0, 2.00, 12.00},
        {"M16x2.0", 16.0, 2.00, 14.00},  {"M18x2.5", 18.0, 2.50, 15.50},
        {"M20x2.5", 20.0, 2.50, 17.50},  {"M22x2.5", 22.0, 2.50, 19.50},
        {"M24x3.0", 24.0, 3.00, 21.00},  {"M27x3.0", 27.0, 3.00, 24.00},
        {"M30x3.5", 30.0, 3.50, 26.50},  {"M33x3.5", 33.0, 3.50, 29.50},
        {"M36x4.0", 36.0, 4.00, 32.00},  {"M39x4.0", 39.0, 4.00, 35.00},
        {"M42x4.5", 42.0, 4.50, 37.50},  {"M45x4.5", 45.0, 4.50, 40.50},
        {"M48x5.0", 48.0, 5.00, 43.00},  {"M52x5.0", 52.0, 5.00, 47.00},
        {"M56x5.5", 56.0, 5.50, 50.50},  {"M60x5.5", 60.0, 5.50, 54.50},
        {"M64x6.0", 64.0, 6.00, 58.00},  {"M68x6.0", 68.0, 6.00, 62.00},
    },
    {{"M1x0.2", 1.0, 0.20, 0.80},      {"M1.1x0.2", 1.1, 0.20, 0.90},
     {"M1.2x0.2", 1.2, 0.20, 1.00},    {"M1.4x0.2", 1.4, 0.20, 1.20},
     {"M1.6x0.2", 1.6, 0.20, 1.40},    {"M1.8x0.2", 1.8, 0.20, 1.60},
     {"M2x0.25", 2.0, 0.25, 1.75},     {"M2.2x0.25", 2.2, 0.25, 1.95},
     {"M2.5x0.35", 2.5, 0.35, 2.15},   {"M3x0.35", 3.0, 0.35, 2.65},
     {"M3.5x0.35", 3.5, 0.35, 3.15},   {"M4x0.5", 4.0, 0.50, 3.50},
     {"M4.5x0.5", 4.5, 0.50, 4.00},    {"M5x0.5", 5.0, 0.50, 4.50},
     {"M5.5x0.5", 5.5, 0.50, 5.00},    {"M6x0.75", 6.0, 0.75, 5.25},
     {"M7x0.75", 7.0, 0.75, 6.25},     {"M8x0.75", 8.0, 0.75, 7.25},
     {"M8x1.0", 8.0, 1.00, 7.00},      {"M9x0.75", 9.0, 0.75, 8.25},
     {"M9x1.0", 9.0, 1.00, 8.00},      {"M10x0.75", 10.0, 0.75, 9.25},
     {"M10x1.0", 10.0, 1.00, 9.00},    {"M10x1.25", 10.0, 1.25, 8.75},
     {"M11x0.75", 11.0, 0.75, 10.25},  {"M11x1.0", 11.0, 1.00, 10.00},
     {"M12x1.0", 12.0, 1.00, 11.00},   {"M12x1.25", 12.0, 1.25, 10.75},
     {"M12x1.5", 12.0, 1.50, 10.50},   {"M14x1.0", 14.0, 1.00, 13.00},
     {"M14x1.25", 14.0, 1.25, 12.75},  {"M14x1.5", 14.0, 1.50, 12.50},
     {"M15x1.0", 15.0, 1.00, 14.00},   {"M15x1.5", 15.0, 1.50, 13.50},
     {"M16x1.0", 16.0, 1.00, 15.00},   {"M16x1.5", 16.0, 1.50, 14.50},
     {"M17x1.0", 17.0, 1.00, 16.00},   {"M17x1.5", 17.0, 1.50, 15.50},
     {"M18x1.0", 18.0, 1.00, 17.00},   {"M18x1.5", 18.0, 1.50, 16.50},
     {"M18x2.0", 18.0, 2.00, 16.00},   {"M20x1.0", 20.0, 1.00, 19.00},
     {"M20x1.5", 20.0, 1.50, 18.50},   {"M20x2.0", 20.0, 2.00, 18.00},
     {"M22x1.0", 22.0, 1.00, 21.00},   {"M22x1.5", 22.0, 1.50, 20.50},
     {"M22x2.0", 22.0, 2.00, 20.00},   {"M24x1.0", 24.0, 1.00, 23.00},
     {"M24x1.5", 24.0, 1.50, 22.50},   {"M24x2.0", 24.0, 2.00, 22.00},
     {"M25x1.0", 25.0, 1.00, 24.00},   {"M25x1.5", 25.0, 1.50, 23.50},
     {"M25x2.0", 25.0, 2.00, 23.00},   {"M27x1.0", 27.0, 1.00, 26.00},
     {"M27x1.5", 27.0, 1.50, 25.50},   {"M27x2.0", 27.0, 2.00, 25.00},
     {"M28x1.0", 28.0, 1.00, 27.00},   {"M28x1.5", 28.0, 1.50, 26.50},
     {"M28x2.0", 28.0, 2.00, 26.00},   {"M30x1.0", 30.0, 1.00, 29.00},
     {"M30x1.5", 30.0, 1.50, 28.50},   {"M30x2.0", 30.0, 2.00, 28.00},
     {"M30x3.0", 30.0, 3.00, 27.00},   {"M32x1.5", 32.0, 1.50, 30.50},
     {"M32x2.0", 32.0, 2.00, 30.00},   {"M33x1.5", 33.0, 1.50, 31.50},
     {"M33x2.0", 33.0, 2.00, 31.00},   {"M33x3.0", 33.0, 3.00, 30.00},
     {"M35x1.5", 35.0, 1.50, 33.50},   {"M35x2.0", 35.0, 2.00, 33.00},
     {"M36x1.5", 36.0, 1.50, 34.50},   {"M36x2.0", 36.0, 2.00, 34.00},
     {"M36x3.0", 36.0, 3.00, 33.00},   {"M39x1.5", 39.0, 1.50, 37.50},
     {"M39x2.0", 39.0, 2.00, 37.00},   {"M39x3.0", 39.0, 3.00, 36.00},
     {"M40x1.5", 40.0, 1.50, 38.50},   {"M40x2.0", 40.0, 2.00, 38.00},
     {"M40x3.0", 40.0, 3.00, 37.00},   {"M42x1.5", 42.0, 1.50, 40.50},
     {"M42x2.0", 42.0, 2.00, 40.00},   {"M42x3.0", 42.0, 3.00, 39.00},
     {"M42x4.0", 42.0, 4.00, 38.00},   {"M45x1.5", 45.0, 1.50, 43.50},
     {"M45x2.0", 45.0, 2.00, 43.00},   {"M45x3.0", 45.0, 3.00, 42.00},
     {"M45x4.0", 45.0, 4.00, 41.00},   {"M48x1.5", 48.0, 1.50, 46.50},
     {"M48x2.0", 48.0, 2.00, 46.00},   {"M48x3.0", 48.0, 3.00, 45.00},
     {"M48x4.0", 48.0, 4.00, 44.00},   {"M50x1.5", 50.0, 1.50, 48.50},
     {"M50x2.0", 50.0, 2.00, 48.00},   {"M50x3.0", 50.0, 3.00, 47.00},
     {"M52x1.5", 52.0, 1.50, 50.50},   {"M52x2.0", 52.0, 2.00, 50.00},
     {"M52x3.0", 52.0, 3.00, 49.00},   {"M52x4.0", 52.0, 4.00, 48.00},
     {"M55x1.5", 55.0, 1.50, 53.50},   {"M55x2.0", 55.0, 2.00, 53.00},
     {"M55x3.0", 55.0, 3.00, 52.00},   {"M55x4.0", 55.0, 4.00, 51.00},
     {"M56x1.5", 56.0, 1.50, 54.50},   {"M56x2.0", 56.0, 2.00, 54.00},
     {"M56x3.0", 56.0, 3.00, 53.00},   {"M56x4.0", 56.0, 4.00, 52.00},
     {"M58x1.5", 58.0, 1.50, 56.50},   {"M58x2.0", 58.0, 2.00, 56.00},
     {"M58x3.0", 58.0, 3.00, 55.00},   {"M58x4.0", 58.0, 4.00, 54.00},
     {"M60x1.5", 60.0, 1.50, 58.50},   {"M60x2.0", 60.0, 2.00, 58.00},
     {"M60x3.0", 60.0, 3.00, 57.00},   {"M60x4.0", 60.0, 4.00, 56.00},
     {"M62x1.5", 62.0, 1.50, 60.50},   {"M62x2.0", 62.0, 2.00, 60.00},
     {"M62x3.0", 62.0, 3.00, 59.00},   {"M62x4.0", 62.0, 4.00, 58.00},
     {"M64x1.5", 64.0, 1.50, 62.50},   {"M64x2.0", 64.0, 2.00, 62.00},
     {"M64x3.0", 64.0, 3.00, 61.00},   {"M64x4.0", 64.0, 4.00, 60.00},
     {"M65x1.5", 65.0, 1.50, 63.50},   {"M65x2.0", 65.0, 2.00, 63.00},
     {"M65x3.0", 65.0, 3.00, 62.00},   {"M65x4.0", 65.0, 4.00, 61.00},
     {"M68x1.5", 68.0, 1.50, 66.50},   {"M68x2.0", 68.0, 2.00, 66.00},
     {"M68x3.0", 68.0, 3.00, 65.00},   {"M68x4.0", 68.0, 4.00, 64.00},
     {"M70x1.5", 70.0, 1.50, 68.50},   {"M70x2.0", 70.0, 2.00, 68.00},
     {"M70x3.0", 70.0, 3.00, 67.00},   {"M70x4.0", 70.0, 4.00, 66.00},
     {"M70x6.0", 70.0, 6.00, 64.00},   {"M72x1.5", 72.0, 1.50, 70.50},
     {"M72x2.0", 72.0, 2.00, 70.00},   {"M72x3.0", 72.0, 3.00, 69.00},
     {"M72x4.0", 72.0, 4.00, 68.00},   {"M72x6.0", 72.0, 6.00, 66.00},
     {"M75x1.5", 75.0, 1.50, 73.50},   {"M75x2.0", 75.0, 2.00, 73.00},
     {"M75x3.0", 75.0, 3.00, 72.00},   {"M75x4.0", 75.0, 4.00, 71.00},
     {"M75x6.0", 75.0, 6.00, 69.00},   {"M76x1.5", 76.0, 1.50, 74.50},
     {"M76x2.0", 76.0, 2.00, 74.00},   {"M76x3.0", 76.0, 3.00, 73.00},
     {"M76x4.0", 76.0, 4.00, 72.00},   {"M76x6.0", 76.0, 6.00, 70.00},
     {"M80x1.5", 80.0, 1.50, 78.50},   {"M80x2.0", 80.0, 2.00, 78.00},
     {"M80x3.0", 80.0, 3.00, 77.00},   {"M80x4.0", 80.0, 4.00, 76.00},
     {"M80x6.0", 80.0, 6.00, 74.00},   {"M85x2.0", 85.0, 2.00, 83.00},
     {"M85x3.0", 85.0, 3.00, 82.00},   {"M85x4.0", 85.0, 4.00, 81.00},
     {"M85x6.0", 85.0, 6.00, 79.00},   {"M90x2.0", 90.0, 2.00, 88.00},
     {"M90x3.0", 90.0, 3.00, 87.00},   {"M90x4.0", 90.0, 4.00, 86.00},
     {"M90x6.0", 90.0, 6.00, 84.00},   {"M95x2.0", 95.0, 2.00, 93.00},
     {"M95x3.0", 95.0, 3.00, 92.00},   {"M95x4.0", 95.0, 4.00, 91.00},
     {"M95x6.0", 95.0, 6.00, 89.00},   {"M100x2.0", 100.0, 2.00, 98.00},
     {"M100x3.0", 100.0, 3.00, 97.00}, {"M100x4.0", 100.0, 4.00, 96.00},
     {"M100x6.0", 100.0, 6.00, 94.00}},
    {
        {"#1", 1.854, 0.397, 1.50},      {"#2", 2.184, 0.454, 1.85},
        {"#3", 2.515, 0.529, 2.10},      {"#4", 2.845, 0.635, 2.35},
        {"#5", 3.175, 0.635, 2.65},      {"#6", 3.505, 0.794, 2.85},
        {"#8", 4.166, 0.794, 3.50},      {"#10", 4.826, 1.058, 3.90},
        {"#12", 5.486, 1.058, 4.50},     {"1/4", 6.350, 1.270, 5.10},
        {"5/16", 7.938, 1.411, 6.60},    {"3/8", 9.525, 1.588, 8.00},
        {"7/16", 11.113, 1.814, 9.40},   {"1/2", 12.700, 1.954, 10.80},
        {"9/16", 14.288, 2.117, 12.20},  {"5/8", 15.875, 2.309, 13.50},
        {"3/4", 19.050, 2.540, 16.50},   {"7/8", 22.225, 2.822, 19.50},
        {"1", 25.400, 3.175, 22.25},     {"1 1/8", 28.575, 3.628, 25.00},
        {"1 1/4", 31.750, 3.628, 28.00}, {"1 3/8", 34.925, 4.233, 30.75},
        {"1 1/2", 38.100, 4.233, 34.00}, {"1 3/4", 44.450, 5.080, 39.50},
        {"2", 50.800, 5.644, 45.00},     {"2 1/4", 57.150, 5.644, 51.50},
        {"2 1/2", 63.500, 6.350, 57.00}, {"2 3/4", 69.850, 6.350, 63.50},
        {"3", 76.200, 6.350, 70.00},     {"3 1/4", 82.550, 6.350, 76.50},
        {"3 1/2", 88.900, 6.350, 83.00}, {"3 3/4", 95.250, 6.350, 89.00},
        {"4", 101.600, 6.350, 95.50},
    },
    {
        {"#0", 1.524, 0.317, 1.20},      {"#1", 1.854, 0.353, 1.55},
        {"#2", 2.184, 0.397, 1.85},      {"#3", 2.515, 0.454, 2.10},
        {"#4", 2.845, 0.529, 2.40},      {"#5", 3.175, 0.577, 2.70},
        {"#6", 3.505, 0.635, 2.95},      {"#8", 4.166, 0.706, 3.50},
        {"#10", 4.826, 0.794, 4.10},     {"#12", 5.486, 0.907, 4.70},
        {"1/4", 6.350, 0.907, 5.50},     {"5/16", 7.938, 1.058, 6.90},
        {"3/8", 9.525, 1.058, 8.50},     {"7/16", 11.113, 1.270, 9.90},
        {"1/2", 12.700, 1.270, 11.50},   {"9/16", 14.288, 1.411, 12.90},
        {"5/8", 15.875, 1.411, 14.50},   {"3/4", 19.050, 1.588, 17.50},
        {"7/8", 22.225, 1.814, 20.40},   {"1", 25.400, 2.117, 23.25},
        {"1 1/8", 28.575, 2.117, 26.50}, {"1 3/16", 30.163, 1.588, 28.58},
        {"1 1/4", 31.750, 2.117, 29.50}, {"1 3/8", 34.925, 2.117, 32.75},
        {"1 1/2", 38.100, 2.117, 36.00},
    },
    {
        {"#12", 5.486, 0.794, 4.80},      {"1/4", 6.350, 0.794, 5.70},
        {"5/16", 7.938, 0.794, 7.25},     {"3/8", 9.525, 0.794, 8.85},
        {"7/16", 11.113, 0.907, 10.35},   {"1/2", 12.700, 0.907, 11.80},
        {"9/16", 14.288, 1.058, 13.40},   {"5/8", 15.875, 1.058, 15.00},
        {"11/16", 17.462, 1.058, 16.60},  {"3/4", 19.050, 1.270, 18.00},
        {"13/16", 20.638, 1.270, 19.60},  {"7/8", 22.225, 1.270, 21.15},
        {"15/16", 23.812, 1.270, 22.70},  {"1", 25.400, 1.270, 24.30},
        {"1 1/16", 26.988, 1.411, 25.80}, {"1 1/8", 28.575, 1.411, 27.35},
        {"1 1/4", 31.750, 1.411, 30.55},  {"1 5/16", 33.338, 1.411, 32.10},
        {"1 3/8", 34.925, 1.411, 33.70},  {"1 7/16", 36.512, 1.411, 35.30},
        {"1 1/2", 38.100, 1.411, 36.90},  {"1 9/16", 39.688, 1.411, 38.55},
        {"1 5/8", 41.275, 1.411, 40.10},  {"1 11/16", 42.862, 1.411, 41.60},
    },
    {
        {"1/16", 7.938, 0.941, 0.0},    {"1/8", 10.287, 0.941, 0.0},
        {"1/4", 13.716, 1.411, 0.0},    {"3/8", 17.145, 1.411, 0.0},
        {"1/2", 21.336, 1.814, 0.0},    {"3/4", 26.670, 1.814, 0.0},
        {"1", 33.401, 2.209, 0.0},      {"1 1/4", 42.164, 2.209, 0.0},
        {"1 1/2", 48.260, 2.209, 0.0},  {"2", 60.325, 2.209, 0.0},
        {"2 1/2", 73.025, 3.175, 0.0},  {"3", 88.900, 3.175, 0.0},
        {"3 1/2", 101.600, 3.175, 0.0}, {"4", 114.300, 3.175, 0.0},
        {"5", 141.300, 3.175, 0.0},     {"6", 168.275, 3.175, 0.0},
        {"8", 219.075, 3.175, 0.0},     {"10", 273.050, 3.175, 0.0},
        {"12", 323.850, 3.175, 0.0},
    },
    {
        {"1/16", 7.723, 0.907, 6.6},     {"1/8", 9.728, 0.907, 8.8},
        {"1/4", 13.157, 1.337, 11.8},    {"3/8", 16.662, 1.337, 15.25},
        {"1/2", 20.955, 1.814, 19.00},   {"5/8", 22.911, 1.814, 21.00},
        {"3/4", 26.441, 1.814, 24.50},   {"7/8", 30.201, 1.814, 28.25},
        {"1", 33.249, 2.309, 30.75},     {"1 1/8", 37.897, 2.309, 0.0},
        {"1 1/4", 41.910, 2.309, 39.50}, {"1 1/2", 47.803, 2.309, 45.50},
        {"1 3/4", 53.743, 2.309, 51.00}, {"2", 59.614, 2.309, 57.00},
        {"2 1/4", 65.710, 2.309, 0.0},   {"2 1/2", 75.184, 2.309, 0.0},
        {"2 3/4", 81.534, 2.309, 0.0},   {"3", 87.884, 2.309, 0.0},
        {"3 1/2", 100.330, 2.309, 0.0},  {"4", 113.030, 2.309, 0.0},
        {"4 1/2", 125.730, 2.309, 0.0},  {"5", 138.430, 2.309, 0.0},
        {"5 1/2", 151.130, 2.309, 0.0},  {"6", 163.830, 2.309, 0.0},
    },
    {
        {"1/8", 3.175, 0.635, 2.55},     {"3/16", 4.762, 1.058, 3.70},
        {"1/4", 6.350, 1.270, 5.10},     {"5/16", 7.938, 1.411, 6.50},
        {"3/8", 9.525, 1.588, 7.90},     {"7/16", 11.113, 1.814, 9.30},
        {"1/2", 12.700, 2.117, 10.50},   {"9/16", 14.290, 2.117, 12.10},
        {"5/8", 15.876, 2.309, 13.50},   {"11/16", 17.463, 2.309, 15.00},
        {"3/4", 19.051, 2.540, 16.25},   {"7/8", 22.226, 2.822, 19.25},
        {"1", 25.400, 3.175, 22.00},     {"1 1/8", 28.576, 3.629, 24.75},
        {"1 1/4", 31.751, 3.629, 28.00}, {"1 1/2", 38.100, 4.233, 33.50},
        {"1 3/4", 44.452, 5.080, 39.00}, {"2", 50.802, 5.644, 44.50},
        {"2 1/4", 57.152, 6.350, 0.0},   {"2 1/2", 63.502, 6.350, 0.0},
        {"2 3/4", 69.853, 7.257, 0.0},   {"3", 76.203, 7.257, 0.0},
        {"3 1/4", 82.553, 7.815, 0.0},   {"3 1/2", 88.903, 7.815, 0.0},
        {"3 3/4", 95.254, 8.467, 0.0},   {"4", 101.604, 8.467, 0.0},
        {"4 1/2", 114.304, 8.835, 0.0},  {"5", 127.005, 9.236, 0.0},
        {"5 1/2", 139.705, 9.676, 0.0},  {"6", 152.406, 10.16, 0.0},
    },
    {
        {"3/16", 4.763, 0.794, 4.00},    {"7/32", 5.558, 0.907, 4.60},
        {"1/4", 6.350, 0.977, 5.30},     {"9/32", 7.142, 0.977, 6.10},
        {"5/16", 7.938, 1.154, 6.80},    {"3/8", 9.525, 1.270, 8.30},
        {"7/16", 11.113, 1.411, 9.70},   {"1/2", 12.700, 1.588, 11.10},
        {"9/16", 14.288, 1.588, 12.70},  {"5/8", 15.875, 1.814, 14.00},
        {"11/16", 17.463, 1.814, 15.50}, {"3/4", 19.050, 2.116, 16.75},
        {"7/8", 22.225, 2.309, 19.75},   {"1", 25.400, 2.540, 22.75},
        {"1 1/8", 28.575, 2.822, 25.50}, {"1 1/4", 31.750, 2.822, 28.50},
        {"1 3/8", 34.925, 3.175, 31.50}, {"1 1/2", 38.100, 3.175, 34.50},
        {"1 5/8", 41.275, 3.175, 0.0},   {"1 3/4", 44.450, 3.629, 0.0},
        {"2", 50.800, 3.629, 0.0},       {"2 1/4", 57.150, 4.233, 0.0},
        {"2 1/2", 63.500, 4.233, 0.0},   {"2 3/4", 69.850, 4.233, 0.0},
        {"3", 76.200, 5.080, 0.0},       {"3 1/4", 82.550, 5.080, 0.0},
        {"3 1/2", 88.900, 5.644, 0.0},   {"3 3/4", 95.250, 5.644, 0.0},
        {"4", 101.600, 5.644, 0.0},      {"4 1/4", 107.950, 6.350, 0.0},
    },
    // ISO 4570:2002
    {
        {"5v1", 5.334, 0.705, 0},   {"5v2", 5.370, 1.058, 0},   {"6v1", 6.160, 0.800, 0},
        {"8v1", 7.798, 0.794, 0},   {"9v1", 9.525, 0.794, 0},   {"10v2", 10.414, 0.907, 0},
        {"12v1", 12.319, 0.977, 0}, {"13v1", 12.700, 1.270, 0}, {"8v2", 7.938, 1.058, 0},
        {"10v1", 9.800, 1.000, 0},  {"11v1", 11.113, 1.270, 0}, {"13v2", 12.700, 0.794, 0},
        {"15v1", 15.137, 1.000, 0}, {"16v1", 15.875, 0.941, 0}, {"17v1", 17.137, 1.000, 0},
        {"17v2", 17.463, 1.058, 0}, {"17v3", 17.463, 1.588, 0}, {"19v1", 19.050, 1.588, 0},
        {"20v1", 20.642, 1.000, 0},
    }
};

const char* ThreadUtils::DepthTypeEnums[]
    = {"Dimension", "ThroughAll", "UpToGeometry", "UpToFirst", nullptr};

const char* ThreadUtils::ThreadTypeEnums[] = {
    "None",
    "ISOMetricProfile",
    "ISOMetricFineProfile",
    "UNC",
    "UNF",
    "UNEF",
    "NPT",
    "BSP",
    "BSW",
    "BSF",
    "ISOTyre",
    nullptr
};

const char* ThreadUtils::ThreadTypeNameEnums[] = {
    "None",
    "ISO metric regular",
    "ISO metric fine",
    "UTS coarse",
    "UTS fine",
    "UTS extra fine",
    "ANSI pipes",
    "ISO/BSP pipes",
    "BSW whitworth",
    "BSF whitworth fine",
    "ISO tyre valves",
    nullptr
};

int threadTypeFromString(const std::string& type)
{
    const auto begin = std::begin(ThreadUtils::ThreadTypeNameEnums);
    const auto end = std::end(ThreadUtils::ThreadTypeNameEnums) - 1;
    const auto found = std::find(begin, end, type);
    return found == end ? 0 : static_cast<int>(found - begin);
}

std::vector<std::string> ThreadUtils::getThreadTypeEnums()
{
    std::vector<std::string> result;


    const auto& definitions = getThreadDefinitions();

    for (const auto& definition : definitions) {
        result.push_back(definition.id);
    }

    return result;
}

std::vector<std::string> ThreadUtils::getThreadTypeNameEnums() const
{
    std::vector<std::string> result;


    const auto& definitions = getThreadDefinitions();

    for (const auto& definition : definitions) {
        result.push_back(definition.name);
    }

    return result;
}

std::vector<std::string> ThreadUtils::getThreadDiameters(const int threadType) const
{
    const auto& definition = checkedThreadDefinition(getThreadDefinitions(), threadType);
    std::set<double> uniqueDiameters;
    const size_t rowCount = std::min(definition.sizes.size(), definition.pitches.size());
    for (size_t row = 0; row < rowCount; ++row) {
        if (definition.sizes[row].empty() || definition.pitches[row].empty()) {
            continue;
        }
        parseThreadValue(definition.pitches[row]);
        uniqueDiameters.insert(parseThreadValue(definition.sizes[row]));
    }
    if (uniqueDiameters.empty()) {
        return {"6.0"};
    }

    std::vector<std::string> designations;
    designations.reserve(uniqueDiameters.size());

    for (double diameter : uniqueDiameters) {
        std::ostringstream oss;

        oss << std::noshowpoint << diameter << " mm";
        designations.push_back(oss.str());
    }
    return designations;
}


std::vector<std::string> ThreadUtils::getThreadMinorDiameters(const int threadType)
{
    const auto& definition = checkedThreadDefinition(getThreadDefinitions(), threadType);
    std::set<double> uniqueMinorDiameters;
    const size_t rowCount = std::min(definition.sizes.size(), definition.pitches.size());
    for (size_t row = 0; row < rowCount; ++row) {
        if (definition.sizes[row].empty() || definition.pitches[row].empty()) {
            continue;
        }
        uniqueMinorDiameters.insert(getMinorDiameterForRow(definition, row, false));
    }

    if (uniqueMinorDiameters.empty()) {
        return {"6.0 mm"};
    }

    std::vector<std::string> designations;
    designations.reserve(uniqueMinorDiameters.size());

    for (double diameter : uniqueMinorDiameters) {
        std::ostringstream oss;
        oss << std::noshowpoint << diameter << " mm";
        designations.push_back(oss.str());
    }

    return designations;
}

std::optional<double> ThreadUtils::estimateKnownMinorDiameter(
    const std::string& threadDefinitionId,
    double majorDiameter,
    double pitch
) const
{
    const double majorRadius = majorDiameter / 2.0;
    double rootRadius;

    if (threadDefinitionId == "BSP" || threadDefinitionId == "BSW" || threadDefinitionId == "BSF") {
        const double triangleHeight = 0.960491 * pitch;
        rootRadius = majorRadius - (2.0 * triangleHeight / 3.0);
    }
    else if (
        threadDefinitionId == "ISOMetricProfile" || threadDefinitionId == "ISOMetricFineProfile"
        || threadDefinitionId == "UNC" || threadDefinitionId == "UNF" || threadDefinitionId == "UNEF"
        || threadDefinitionId == "NPT" || threadDefinitionId == "ISOTyre"
    ) {
        const double triangleHeight = std::sqrt(3.0) / 2.0 * pitch;
        const double profileHeight = 5.0 * triangleHeight / 8.0;
        rootRadius = majorRadius - profileHeight;
    }
    else {
        return std::nullopt;
    }

    if (rootRadius < 0.0) {
        rootRadius = 0.0;
    }

    return 2.0 * rootRadius;
}

ThreadUtils::ResolvedThreadSelection ThreadUtils::resolveThreadSelection(
    int threadType,
    int sizeIndex,
    int pitchIndex
) const
{
    const auto& definition = checkedThreadDefinition(getThreadDefinitions(), threadType);

    const std::vector<std::string> diameters = getThreadDiameters(threadType);
    if (sizeIndex < 0 || static_cast<size_t>(sizeIndex) >= diameters.size()) {
        throw Base::IndexError(QT_TRANSLATE_NOOP("Exception", "Thread size out of range"));
    }

    const std::vector<std::string> pitches = getThreadPitches(threadType, sizeIndex);
    if (pitchIndex < 0 || static_cast<size_t>(pitchIndex) >= pitches.size()) {
        throw Base::IndexError(QT_TRANSLATE_NOOP("Exception", "Thread pitch out of range"));
    }

    const double selectedDiameter = parseThreadValue(diameters[sizeIndex]);
    const double selectedPitch = parseThreadValue(pitches[pitchIndex]);
    const size_t rowCount = std::min(definition.sizes.size(), definition.pitches.size());
    for (size_t row = 0; row < rowCount; ++row) {
        if (definition.sizes[row].empty() || definition.pitches[row].empty()) {
            continue;
        }

        const double rowDiameter = parseThreadValue(definition.sizes[row]);
        const double rowPitch = parseThreadValue(definition.pitches[row]);
        if (std::abs(rowDiameter - selectedDiameter) < threadDefinitionTolerance
            && std::abs(rowPitch - selectedPitch) < threadDefinitionTolerance) {
            return {&definition, row, selectedDiameter, selectedPitch};
        }
    }

    throw Base::ValueError(
        QT_TRANSLATE_NOOP("Exception", "Selected thread size and pitch were not found")
    );
}

std::optional<Part::TopoShape> ThreadUtils::prepareThreadProfile(
    int threadType,
    int sizeIndex,
    int pitchIndex,
    bool isInternalThread,
    double majorRadius,
    const gp_Vec& xDir,
    const gp_Vec& zDir
) const
{
    const ResolvedThreadSelection selection = resolveThreadSelection(threadType, sizeIndex, pitchIndex);
    const ThreadDefinition& definition = *selection.definition;

    const ThreadDefinition::Profile* profile = nullptr;
    if (!isInternalThread && definition.externalProfileStatus == ThreadDefinition::ProfileStatus::Valid
        && definition.externalProfile) {
        profile = &*definition.externalProfile;
    }
    else if (definition.profileStatus == ThreadDefinition::ProfileStatus::Valid && definition.profile) {
        profile = &*definition.profile;
    }

    if (!profile) {
        return std::nullopt;
    }

    if (selection.pitch <= Precision::Confusion()) {
        throw Base::ValueError(
            QT_TRANSLATE_NOOP("Exception", "Thread pitch must be greater than zero")
        );
    }
    if (xDir.Magnitude() <= Precision::Confusion() || zDir.Magnitude() <= Precision::Confusion()) {
        throw Base::ValueError(
            QT_TRANSLATE_NOOP("Exception", "Thread profile directions must be non-zero")
        );
    }

    const gp_Dir radialDirection(xDir);
    const gp_Dir axialDirection(zDir);
    if (std::abs(radialDirection.Dot(axialDirection)) > Precision::Angular()) {
        throw Base::ValueError(
            QT_TRANSLATE_NOOP("Exception", "Thread profile directions must be perpendicular")
        );
    }
    const gp_Dir normalDirection(radialDirection.Crossed(axialDirection));

    const double pitch = selection.pitch;
    gp_Trsf transform;
    transform.SetValues(
        pitch * radialDirection.X(),
        pitch * axialDirection.X(),
        pitch * normalDirection.X(),
        majorRadius * radialDirection.X(),
        pitch * radialDirection.Y(),
        pitch * axialDirection.Y(),
        pitch * normalDirection.Y(),
        majorRadius * radialDirection.Y(),
        pitch * radialDirection.Z(),
        pitch * axialDirection.Z(),
        pitch * normalDirection.Z(),
        majorRadius * radialDirection.Z()
    );

    Part::TopoShape transformed;
    transformed.makeElementTransform(profile->wire, transform, nullptr, Part::CopyType::copy);
    if (transformed.isNull() || transformed.shapeType() != TopAbs_WIRE || !transformed.isValid()
        || !transformed.isClosed()) {
        throw Base::CADKernelError(
            QT_TRANSLATE_NOOP("Exception", "Thread profile transformation failed")
        );
    }

    return transformed;
}

double ThreadUtils::getMinorDiameterForRow(
    const ThreadDefinition& definition,
    size_t row,
    bool preferExternalProfile
) const
{
    if (row >= definition.sizes.size() || row >= definition.pitches.size()) {
        throw Base::IndexError(QT_TRANSLATE_NOOP("Exception", "Thread definition row out of range"));
    }

    if (row < definition.minorDiameters.size() && !definition.minorDiameters[row].empty()) {
        try {
            const double value = std::abs(parseThreadValue(definition.minorDiameters[row]));
            const double majorDiameter = parseThreadValue(definition.sizes[row]);
            if (value <= Precision::Confusion() || value >= majorDiameter) {
                throw Base::ValueError(
                    "Thread definition '" + definition.id + "' contains an invalid minor diameter"
                );
            }
            return value;
        }
        catch (const Base::ValueError&) {
            throw;
        }
        catch (const std::exception&) {
            throw Base::ValueError(
                "Thread definition '" + definition.id + "' contains an invalid minor diameter"
            );
        }
    }

    const double majorDiameter = parseThreadValue(definition.sizes[row]);
    const double pitch = parseThreadValue(definition.pitches[row]);
    if (majorDiameter <= Precision::Confusion() || pitch <= Precision::Confusion()) {
        throw Base::ValueError(
            "Thread definition '" + definition.id
            + "' must contain positive major diameter and pitch values"
        );
    }

    std::optional<double> minorDiameter
        = estimateKnownMinorDiameter(definition.id, majorDiameter, pitch);

    if (!minorDiameter) {
        const ThreadDefinition::Profile* profile = nullptr;
        if (preferExternalProfile
            && definition.externalProfileStatus == ThreadDefinition::ProfileStatus::Valid
            && definition.externalProfile) {
            profile = &*definition.externalProfile;
        }
        else if (
            definition.profileStatus == ThreadDefinition::ProfileStatus::Valid && definition.profile
        ) {
            profile = &*definition.profile;
        }

        if (!profile) {
            throw Base::ValueError(
                "Thread definition '" + definition.id
                + "' has no minor diameter, known family formula, or valid thread profile"
            );
        }

        const double profileDepth = (profile->maxX - profile->minX) * pitch;
        minorDiameter = majorDiameter - 2.0 * profileDepth;
    }

    *minorDiameter += minorDiameterFuseOverlap;
    if (*minorDiameter <= Precision::Confusion() || *minorDiameter >= majorDiameter) {
        throw Base::ValueError(
            "Thread definition '" + definition.id + "' produces an invalid minor diameter"
        );
    }

    return *minorDiameter;
}

double ThreadUtils::getMinorDiameter(const int threadType, const int size, const int pitch) const
{
    const ResolvedThreadSelection selection = resolveThreadSelection(threadType, size, pitch);
    return getMinorDiameterForRow(*selection.definition, selection.row, true);
}

std::vector<std::string> ThreadUtils::getThreadPitches(const int threadType, const int threadDiameter) const
{
    const auto& definition = checkedThreadDefinition(getThreadDefinitions(), threadType);
    std::vector<std::string> pitches;
    const auto diameters = getThreadDiameters(threadType);
    if (threadDiameter < 0 || static_cast<size_t>(threadDiameter) >= diameters.size()) {
        throw Base::IndexError(QT_TRANSLATE_NOOP("Exception", "Thread size out of range"));
    }
    const double targetDiameter = parseThreadValue(diameters[threadDiameter]);
    const size_t rowCount = std::min(definition.sizes.size(), definition.pitches.size());
    for (size_t row = 0; row < rowCount; ++row) {
        if (definition.sizes[row].empty() || definition.pitches[row].empty()) {
            continue;
        }
        if (std::abs(parseThreadValue(definition.sizes[row]) - targetDiameter)
            < threadDefinitionTolerance) {
            parseThreadValue(definition.pitches[row]);
            std::ostringstream oss;
            oss << std::noshowpoint << definition.pitches[row] << " mm";
            pitches.push_back(oss.str());
        }
    }
    if (pitches.empty()) {
        return {"0.0"};
    }
    return pitches;
}

double ThreadUtils::getThreadPitch(const int threadType, const int threadDiameter, const int threadPitch) const
{
    const auto pitches = getThreadPitches(threadType, threadDiameter);
    if (threadPitch < 0 || static_cast<size_t>(threadPitch) >= pitches.size()) {
        return 0.0;
    }

    const double pitch = parseThreadValue(pitches[threadPitch]);
    if (pitch <= 0.0) {
        throw Base::ValueError(QT_TRANSLATE_NOOP(
            "Exception",
            "Thread error: Thread pitch must be finite and greater than zero"
        ));
    }
    return pitch;
}

std::vector<std::string> ThreadUtils::getThreadClasses(const int threadType, const bool isInternal) const
{
    const auto& definition = checkedThreadDefinition(getThreadDefinitions(), threadType);
    const auto& clearances = isInternal ? definition.internalClearances
                                        : definition.externalClearances;
    if (clearances.empty()) {
        return {"None"};
    }

    return clearances;
}

bool ThreadUtils::isThreadConical(const int threadType) const
{
    return checkedThreadDefinition(getThreadDefinitions(), threadType).isConical;
}

std::string ThreadUtils::getThreadDesignations(
    const int threadType,
    const int threadDiameter,
    const int threadPitch
)
{
    try {
        const ResolvedThreadSelection selection
            = resolveThreadSelection(threadType, threadDiameter, threadPitch);
        if (selection.row < selection.definition->designations.size()
            && !selection.definition->designations[selection.row].empty()) {
            return selection.definition->designations[selection.row];
        }
    }
    catch (const Base::Exception&) {
    }

    return "---";
}

double ThreadUtils::getThreadProfileAngle()
{
    return 90 - 1.79;
}

enum class FaceType
{
    Invalid,
    Cylinder,
    Cone
};

static FaceType getFaceType(const TopoDS_Face& face)
{
    if (face.IsNull()) {
        return FaceType::Invalid;
    }

    BRepAdaptor_Surface surface(face);

    switch (surface.GetType()) {
        case GeomAbs_Cylinder:
            return FaceType::Cylinder;

        case GeomAbs_Cone:
            return FaceType::Cone;

        default:
            return FaceType::Invalid;
    }
}

static gp_Pnt toPnt(const gp_Vec& dir)
{
    return {dir.X(), dir.Y(), dir.Z()};
}

double ThreadUtils::getThreadClassClearance(
    int threadType,
    int threadSize,
    const App::PropertyEnumeration& threadClass
) const
{
    const auto& definition = checkedThreadDefinition(getThreadDefinitions(), threadType);
    const std::string className = threadClass.isValid() ? threadClass.getValueAsString() : "";
    if (className.size() < 2 || (className[1] != 'G' && className[1] != 'g')) {
        return 0.0;
    }
    const int legacyType = threadTypeFromString(definition.name);
    if (legacyType < 0 || static_cast<size_t>(legacyType) >= std::size(threadDescription)
        || threadSize < 0 || static_cast<size_t>(threadSize) >= threadDescription[legacyType].size()) {
        throw Base::IndexError(QT_TRANSLATE_NOOP("Exception", "Thread size out of range"));
    }
    const double pitch = threadDescription[legacyType][threadSize].pitch;
    for (const auto& data : ThreadClass_ISOmetric_data) {
        if (pitch <= data[0]) {
            return data[1];
        }
    }

    return 0.0;
}

void ThreadUtils::rotateToNormal(
    const gp_Dir& helixAxis,
    const gp_Dir& normalAxis,
    TopoDS_Shape& helixShape
) const
{
    const auto getRotationAxis =
        [](const gp_Dir& dir1, const gp_Dir& dir2, gp_Dir& dir3, double& angle) {
            if (dir1.IsEqual(dir2, Precision::Angular())) {
                return false;
            }

            angle = acos(dir1 * dir2);
            if (dir1.IsOpposite(dir2, Precision::Angular())) {
                gp_XYZ xyz(dir1.XYZ());
                if (fabs(xyz.X()) <= fabs(xyz.Y()) && fabs(xyz.X()) <= fabs(xyz.Z())) {
                    xyz.SetX(1.0);
                }
                else if (fabs(xyz.Y()) <= fabs(xyz.X()) && fabs(xyz.Y()) <= fabs(xyz.Z())) {
                    xyz.SetY(1.0);
                }
                else {
                    xyz.SetZ(1.0);
                }
                dir3 = dir1.Crossed(gp_Dir(xyz));
            }
            else {
                dir3 = dir1.Crossed(dir2);
            }
            return true;
        };
    double angle;
    gp_Dir rotAxis;
    if (getRotationAxis(helixAxis, normalAxis, rotAxis, angle)) {
        const gp_Pnt origin(0.0, 0.0, 0.0);
        gp_Trsf mov = helixShape.Location().Transformation();
        mov.SetRotation(gp_Ax1(origin, rotAxis), angle);
        const TopLoc_Location loc2(mov);
        helixShape.Move(loc2);
    }
}

Part::TopoShape ThreadUtils::makeLegacyThreadProfile(
    const std::string& threadType,
    double majorRadius,
    double pitch,
    const gp_Vec& xDir,
    const gp_Vec& zDir
) const
{
    constexpr double marginZ = 0.001;
    BRepBuilderAPI_MakeWire wireBuilder;

    if (threadType == "BSP" || threadType == "BSW" || threadType == "BSF") {
        const double height = 0.960491 * pitch;
        const double radius = 0.137329 * pitch;
        const double marginX = std::tan(Base::toRadians(62.5)) * marginZ;
        const double crestArcX = majorRadius - radius * 0.58284013094;

        const gp_Pnt p1 = toPnt((majorRadius - 5.0 * height / 6.0 + marginX) * xDir + marginZ * zDir);
        const gp_Pnt p2 = toPnt(crestArcX * xDir + 3.0 * pitch / 8.0 * zDir);
        const gp_Pnt crest = toPnt(majorRadius * xDir + pitch / 2.0 * zDir);
        const gp_Pnt p3 = toPnt(crestArcX * xDir + 5.0 * pitch / 8.0 * zDir);
        const gp_Pnt p4 = toPnt(
            (majorRadius - 5.0 * height / 6.0 + marginX) * xDir + (pitch - marginZ) * zDir
        );

        wireBuilder.Add(BRepBuilderAPI_MakeEdge(p1, p2).Edge());
        const Handle(Geom_TrimmedCurve) crestArc = GC_MakeArcOfCircle(p2, crest, p3).Value();
        wireBuilder.Add(BRepBuilderAPI_MakeEdge(crestArc).Edge());
        wireBuilder.Add(BRepBuilderAPI_MakeEdge(p3, p4).Edge());
        wireBuilder.Add(BRepBuilderAPI_MakeEdge(p4, p1).Edge());
    }
    else {
        const double height = std::sqrt(3.0) / 2.0 * pitch;
        const double profileDepth = 7.0 * height / 8.0;
        const double marginX = std::tan(Base::toRadians(60.0)) * marginZ;

        const gp_Pnt p1 = toPnt((majorRadius - profileDepth + marginX) * xDir + marginZ * zDir);
        const gp_Pnt p2 = toPnt(majorRadius * xDir + 7.0 * pitch / 16.0 * zDir);
        const gp_Pnt p3 = toPnt(majorRadius * xDir + 9.0 * pitch / 16.0 * zDir);
        const gp_Pnt p4 = toPnt(
            (majorRadius - profileDepth + marginX) * xDir + (pitch - marginZ) * zDir
        );

        wireBuilder.Add(BRepBuilderAPI_MakeEdge(p1, p2).Edge());
        if (threadType == "ISOTyre") {
            const gp_Pnt crest = toPnt((majorRadius + pitch / 32.0) * xDir + pitch / 2.0 * zDir);
            const Handle(Geom_TrimmedCurve) crestArc = GC_MakeArcOfCircle(p2, crest, p3).Value();
            wireBuilder.Add(BRepBuilderAPI_MakeEdge(crestArc).Edge());
        }
        else {
            wireBuilder.Add(BRepBuilderAPI_MakeEdge(p2, p3).Edge());
        }
        wireBuilder.Add(BRepBuilderAPI_MakeEdge(p3, p4).Edge());
        wireBuilder.Add(BRepBuilderAPI_MakeEdge(p4, p1).Edge());
    }

    wireBuilder.Build();
    if (!wireBuilder.IsDone()) {
        throw Base::CADKernelError(
            QT_TRANSLATE_NOOP("Exception", "Legacy thread profile could not be built")
        );
    }

    Part::TopoShape profile(wireBuilder.Wire());
    if (!profile.isValid() || !profile.isClosed()) {
        throw Base::CADKernelError(QT_TRANSLATE_NOOP("Exception", "Legacy thread profile is invalid"));
    }
    return profile;
}

Part::TopoShape ThreadUtils::makeExtendedWhitworthThreadProfile(
    double majorRadius,
    double pitch,
    const gp_Vec& xDir,
    const gp_Vec& zDir
) const
{
    constexpr double marginZ = 0.001;
    constexpr double tangentOffsetFactor = 0.58284013094;

    const double height = 0.960491 * pitch;
    const double arcRadius = 0.137329 * pitch;

    const double sharpRootRadius = majorRadius - 5.0 * height / 6.0;
    const double finishedRootRadius = majorRadius - 2.0 * height / 3.0;
    const double crestTangentRadius = majorRadius - arcRadius * tangentOffsetFactor;
    const double rootTangentRadius = finishedRootRadius + arcRadius * tangentOffsetFactor;
    const double rootCircleCenterRadius = finishedRootRadius + arcRadius;
    const double hiddenTailRadius = sharpRootRadius + std::tan(Base::toRadians(62.5)) * marginZ;

    if (hiddenTailRadius >= finishedRootRadius - Precision::Confusion()) {
        throw Base::ValueError(
            QT_TRANSLATE_NOOP("Exception", "Whitworth thread profile has no room for root overlap")
        );
    }

    const auto profilePoint = [&xDir, &zDir](double radius, double axial) {
        return toPnt(radius * xDir + axial * zDir);
    };

    const double rootTangentAngle = std::atan2(pitch / 8.0, rootTangentRadius - rootCircleCenterRadius);
    const double lowerRootMidAngle = (std::numbers::pi + rootTangentAngle) / 2.0;
    const double upperRootMidAngle = -lowerRootMidAngle;
    const auto rootArcPoint = [&](double centerAxial, double angle) {
        return profilePoint(
            rootCircleCenterRadius + arcRadius * std::cos(angle),
            centerAxial + arcRadius * std::sin(angle)
        );
    };

    const gp_Pnt rootStart = profilePoint(finishedRootRadius, 0.0);
    const gp_Pnt lowerRootMid = rootArcPoint(0.0, lowerRootMidAngle);
    const gp_Pnt lowerRootTangent = profilePoint(rootTangentRadius, pitch / 8.0);
    const gp_Pnt lowerCrestTangent = profilePoint(crestTangentRadius, 3.0 * pitch / 8.0);
    const gp_Pnt crest = profilePoint(majorRadius, pitch / 2.0);
    const gp_Pnt upperCrestTangent = profilePoint(crestTangentRadius, 5.0 * pitch / 8.0);
    const gp_Pnt upperRootTangent = profilePoint(rootTangentRadius, 7.0 * pitch / 8.0);
    const gp_Pnt upperRootMid = rootArcPoint(pitch, upperRootMidAngle);
    const gp_Pnt rootEnd = profilePoint(finishedRootRadius, pitch);
    const gp_Pnt hiddenTailEnd = profilePoint(hiddenTailRadius, pitch);
    const gp_Pnt hiddenTailStart = profilePoint(hiddenTailRadius, 0.0);

    BRepBuilderAPI_MakeWire wireBuilder;
    const Handle(Geom_TrimmedCurve)
        lowerRootArc = GC_MakeArcOfCircle(rootStart, lowerRootMid, lowerRootTangent).Value();
    wireBuilder.Add(BRepBuilderAPI_MakeEdge(lowerRootArc).Edge());
    wireBuilder.Add(BRepBuilderAPI_MakeEdge(lowerRootTangent, lowerCrestTangent).Edge());

    const Handle(Geom_TrimmedCurve)
        crestArc = GC_MakeArcOfCircle(lowerCrestTangent, crest, upperCrestTangent).Value();
    wireBuilder.Add(BRepBuilderAPI_MakeEdge(crestArc).Edge());
    wireBuilder.Add(BRepBuilderAPI_MakeEdge(upperCrestTangent, upperRootTangent).Edge());

    const Handle(Geom_TrimmedCurve)
        upperRootArc = GC_MakeArcOfCircle(upperRootTangent, upperRootMid, rootEnd).Value();
    wireBuilder.Add(BRepBuilderAPI_MakeEdge(upperRootArc).Edge());

    wireBuilder.Add(BRepBuilderAPI_MakeEdge(rootEnd, hiddenTailEnd).Edge());
    wireBuilder.Add(BRepBuilderAPI_MakeEdge(hiddenTailEnd, hiddenTailStart).Edge());
    wireBuilder.Add(BRepBuilderAPI_MakeEdge(hiddenTailStart, rootStart).Edge());
    wireBuilder.Build();

    if (!wireBuilder.IsDone()) {
        throw Base::CADKernelError(
            QT_TRANSLATE_NOOP("Exception", "Extended Whitworth thread profile could not be built")
        );
    }

    Part::TopoShape profile(wireBuilder.Wire());
    if (!profile.isValid() || !profile.isClosed()) {
        throw Base::CADKernelError(
            QT_TRANSLATE_NOOP("Exception", "Extended Whitworth thread profile is invalid")
        );
    }

    return profile;
}

ThreadUtils::ThreadEndTrims ThreadUtils::calculateThreadEndTrims(
    const TopoDS_Wire& normalizedProfile,
    double pitch,
    const NormalizedThreadSupport& nearSupport,
    const NormalizedThreadSupport& farSupport
)
{
    constexpr double halfPeriod = 0.5;

    if (pitch <= Precision::Confusion()) {
        throw Base::ValueError(
            QT_TRANSLATE_NOOP("Exception", "Thread pitch must be greater than zero")
        );
    }
    if (normalizedProfile.IsNull()) {
        throw Base::ValueError(QT_TRANSLATE_NOOP("Exception", "Thread profile must not be null"));
    }

    const Part::TopoShape profile(normalizedProfile);
    if (!profile.isValid() || !profile.isClosed()) {
        throw Base::ValueError(
            QT_TRANSLATE_NOOP("Exception", "Thread profile must be a valid closed wire")
        );
    }

    const Base::BoundBox3d bounds = profile.getBoundBox();
    const double normalizedTolerance
        = std::max(threadDefinitionTolerance, Precision::Confusion() / pitch);
    if (std::abs(bounds.MinZ) > normalizedTolerance || std::abs(bounds.MaxZ) > normalizedTolerance
        || bounds.MinY < -normalizedTolerance || bounds.MaxY > 1.0 + normalizedTolerance) {
        throw Base::ValueError(QT_TRANSLATE_NOOP(
            "Exception",
            "Thread profile must be normalized in the XY plane with Y in the range 0..1"
        ));
    }

    const auto findIntersections = [&](const NormalizedThreadSupport& support) {
        if (!std::isfinite(support.xAtYZero) || !std::isfinite(support.slope)) {
            throw Base::ValueError(
                QT_TRANSLATE_NOOP("Exception", "Thread support line contains invalid values")
            );
        }

        const gp_Pln supportPlane(gp_Pnt(support.xAtYZero, 0.0, 0.0), gp_Dir(1.0, -support.slope, 0.0));
        const Handle(Geom_Plane) supportSurface = new Geom_Plane(supportPlane);
        std::vector<double> phases;

        for (BRepTools_WireExplorer explorer(normalizedProfile); explorer.More(); explorer.Next()) {
            Standard_Real firstParameter;
            Standard_Real lastParameter;
            const Handle(Geom_Curve)
                curve = BRep_Tool::Curve(explorer.Current(), firstParameter, lastParameter);
            if (curve.IsNull()) {
                continue;
            }

            const Handle(Geom_TrimmedCurve)
                trimmedCurve = new Geom_TrimmedCurve(curve, firstParameter, lastParameter);
            const GeomAPI_IntCS intersections(trimmedCurve, supportSurface);
            if (!intersections.IsDone()) {
                throw Base::CADKernelError(
                    QT_TRANSLATE_NOOP("Exception", "Thread profile/support intersection failed")
                );
            }
            if (intersections.NbSegments() > 0) {
                throw Base::ValueError(QT_TRANSLATE_NOOP(
                    "Exception",
                    "Thread profile is coincident with the support surface"
                ));
            }

            for (Standard_Integer pointIndex = 1; pointIndex <= intersections.NbPoints();
                 ++pointIndex) {
                const gp_Pnt point = intersections.Point(pointIndex);
                if (std::abs(point.Z()) <= normalizedTolerance && point.Y() >= -normalizedTolerance
                    && point.Y() <= 1.0 + normalizedTolerance) {
                    phases.push_back(std::clamp(point.Y(), 0.0, 1.0));
                }
            }
        }

        std::sort(phases.begin(), phases.end());
        phases.erase(
            std::unique(
                phases.begin(),
                phases.end(),
                [normalizedTolerance](double lhs, double rhs) {
                    return std::abs(lhs - rhs) <= normalizedTolerance;
                }
            ),
            phases.end()
        );
        return phases;
    };

    const std::vector<double> nearIntersections = findIntersections(nearSupport);
    const std::vector<double> farIntersections = findIntersections(farSupport);

    const auto nearIt = std::find_if(
        nearIntersections.begin(),
        nearIntersections.end(),
        [normalizedTolerance](double phase) {
            return phase > normalizedTolerance && phase < halfPeriod;
        }
    );
    const auto farIt = std::find_if(
        farIntersections.rbegin(),
        farIntersections.rend(),
        [normalizedTolerance](double phase) {
            return phase < 1.0 - normalizedTolerance && phase > halfPeriod;
        }
    );
    if (nearIt == nearIntersections.end() || farIt == farIntersections.rend()) {
        throw Base::ValueError(QT_TRANSLATE_NOOP(
            "Exception",
            "Thread profile does not cross the support surface at both ends"
        ));
    }

    return {*nearIt * pitch, (1.0 - *farIt) * pitch};
}

static double getThreadTaperSlope(bool tapered, double taperedAngle)
{
    if (!tapered) {
        return 0.0;
    }
    if (!std::isfinite(taperedAngle) || taperedAngle <= 0.0 || taperedAngle >= 180.0) {
        throw Base::ValueError(QT_TRANSLATE_NOOP(
            "Exception",
            "Thread error: Tapered angle must be between 0 and 180 degrees"
        ));
    }
    return std::tan(Base::toRadians(90.0 - taperedAngle));
}

static void validateThreadRadii(double firstRadius, double lastRadius)
{
    if (!std::isfinite(firstRadius) || !std::isfinite(lastRadius)
        || std::min(firstRadius, lastRadius) <= Precision::Confusion()) {
        throw Base::ValueError(QT_TRANSLATE_NOOP(
            "Exception",
            "Thread error: Thread geometry reaches the cone apex or contains an invalid radius"
        ));
    }
}

ThreadUtils::ThreadEndTrims ThreadUtils::getThreadEndTrims(
    int threadType,
    int sizeIndex,
    int pitchIndex,
    bool isInternalThread,
    double supportDiameter,
    double usefulThreadLength,
    bool tapered,
    double taperedAngle,
    bool useCustomThreadClearance,
    double customThreadClearance,
    App::PropertyEnumeration& threadClass,
    bool referenceAtStart,
    double supportTaperSlope
) const
{
    const ResolvedThreadSelection selection = resolveThreadSelection(threadType, sizeIndex, pitchIndex);
    const ThreadDefinition& definition = *selection.definition;

    const ThreadDefinition::Profile* selectedProfile = nullptr;
    if (!isInternalThread && definition.externalProfileStatus == ThreadDefinition::ProfileStatus::Valid
        && definition.externalProfile) {
        selectedProfile = &*definition.externalProfile;
    }
    else if (definition.profileStatus == ThreadDefinition::ProfileStatus::Valid && definition.profile) {
        selectedProfile = &*definition.profile;
    }

    if (!std::isfinite(supportDiameter) || !std::isfinite(usefulThreadLength)
        || !std::isfinite(supportTaperSlope) || supportDiameter <= Precision::Confusion()
        || usefulThreadLength <= Precision::Confusion()) {
        throw Base::ValueError(QT_TRANSLATE_NOOP(
            "Exception",
            "Thread support diameter and useful length must be greater than zero"
        ));
    }

    constexpr double referenceMajorRadius = 0.0;
    const gp_Vec radialDirection(1.0, 0.0, 0.0);
    const gp_Vec axialDirection(0.0, 1.0, 0.0);

    Part::TopoShape normalizedProfile;
    if (selectedProfile) {
        normalizedProfile = selectedProfile->wire;
    }
    else {
        Part::TopoShape physicalProfile;
        if (useExtendedWhitworthRoot && isWhitworthThread(definition.id)) {
            physicalProfile = makeExtendedWhitworthThreadProfile(
                referenceMajorRadius,
                selection.pitch,
                radialDirection,
                axialDirection
            );
        }
        else {
            physicalProfile = makeLegacyThreadProfile(
                definition.id,
                referenceMajorRadius,
                selection.pitch,
                radialDirection,
                axialDirection
            );
        }

        gp_Trsf normalization;
        normalization.SetScale(gp_Pnt(0.0, 0.0, 0.0), 1.0 / selection.pitch);
        normalizedProfile
            .makeElementTransform(physicalProfile, normalization, nullptr, Part::CopyType::copy);
    }

    const double clearance = useCustomThreadClearance
        ? customThreadClearance / 2.0
        : getThreadClassClearance(threadType, sizeIndex, threadClass) / 2.0;
    const double correctedMajorRadius = selection.majorDiameter / 2.0
        + (isInternalThread ? clearance : -clearance);
    const double supportRadius = supportDiameter / 2.0;
    const double taperSlope = getThreadTaperSlope(tapered, taperedAngle);
    const double farMajorRadius = correctedMajorRadius
        - (referenceAtStart ? usefulThreadLength * taperSlope : 0.0);
    const double nearMajorRadius = farMajorRadius + usefulThreadLength * taperSlope;
    const double farSupportRadius = supportRadius + usefulThreadLength * supportTaperSlope;
    validateThreadRadii(nearMajorRadius, farMajorRadius);
    validateThreadRadii(supportRadius, farSupportRadius);
    const double farSupportOffset = (farSupportRadius - farMajorRadius) / selection.pitch;
    const double nearSupportOffset = (supportRadius - nearMajorRadius) / selection.pitch;

    const NormalizedThreadSupport nearSupport {nearSupportOffset, -taperSlope};
    const NormalizedThreadSupport farSupport {farSupportOffset, -taperSlope};
    return calculateThreadEndTrims(
        TopoDS::Wire(normalizedProfile.getShape()),
        selection.pitch,
        nearSupport,
        farSupport
    );
}


TopoDS_Shape ThreadUtils::makeThread(
    const gp_Vec& xDir,
    const gp_Vec& zDir,
    double length,
    const int threadType,  // TODO: change to ThreadTypeIndex
    const int threadSize,  // TODO: change to ThreadDiameterIndex
    const int threadPitch,
    const int leftHanded,
    App::PropertyEnumeration& threadClass,
    const bool isInternalThread,
    const bool tapered,
    const double taperedAngle,
    const bool useCustomThreadClearance,
    const double customThreadClearance,
    const double farEndTrim,
    const double usefulThreadLength
)
{
    if (threadType < 0) {
        throw Base::IndexError(QT_TRANSLATE_NOOP("Exception", "Thread type out of range"));
    }
    if (threadSize < 0) {
        throw Base::IndexError(QT_TRANSLATE_NOOP("Exception", "Thread size out of range"));
    }
    if (threadPitch < 0) {
        throw Base::IndexError(QT_TRANSLATE_NOOP("Exception", "Thread pitch out of range"));
    }


    // https://en.wikipedia.org/wiki/File:ISO_and_UTS_Thread_Dimensions.svg
    const std::vector<std::string> currentThreads = ThreadUtils::getThreadTypeNameEnums();
    if (static_cast<size_t>(threadType) >= currentThreads.size()) {
        throw Base::IndexError(QT_TRANSLATE_NOOP("Exception", "Thread type out of range"));
    }

    const std::vector<std::string> threadDiameters = getThreadDiameters(threadType);
    if (static_cast<size_t>(threadSize) >= threadDiameters.size()) {
        throw Base::IndexError(QT_TRANSLATE_NOOP("Exception", "Thread size out of range"));
    }

    const std::vector<std::string> threadPitches = getThreadPitches(threadType, threadSize);
    if (static_cast<size_t>(threadPitch) >= threadPitches.size()) {
        throw Base::IndexError(QT_TRANSLATE_NOOP("Exception", "Thread pitch out of range"));
    }

    const std::string& currentThread = currentThreads[threadType];
    const int currentThreadTypeIndex = threadTypeFromString(currentThread);
    double majorRadius = parseThreadValue(threadDiameters[threadSize]) / 2.0;
    const double pitch = getThreadPitch(threadType, threadSize, threadPitch);
    if (pitch <= Precision::Confusion()) {
        throw Base::ValueError(
            QT_TRANSLATE_NOOP("Exception", "Thread pitch must be greater than zero")
        );
    }

    double clearance;
    if (useCustomThreadClearance) {
        clearance = customThreadClearance / 2;
    }
    else {
        clearance = getThreadClassClearance(threadType, threadSize, threadClass) / 2;
    }

    double correctedMajorRadius;
    if (isInternalThread) {
        correctedMajorRadius = majorRadius + clearance;
    }
    else {
        correctedMajorRadius = majorRadius - clearance;
    }

    if (!std::isfinite(farEndTrim) || farEndTrim < 0.0) {
        throw Base::ValueError(
            QT_TRANSLATE_NOOP("Exception", "Thread far-end trim must not be negative")
        );
    }
    if (!std::isfinite(length) || length <= Precision::Confusion()
        || !std::isfinite(usefulThreadLength) || usefulThreadLength < 0.0
        || usefulThreadLength + farEndTrim > length + Precision::Confusion()) {
        throw Base::ValueError(
            QT_TRANSLATE_NOOP("Exception", "Thread error: Invalid generated or useful thread length")
        );
    }
    const double taperSlope = getThreadTaperSlope(tapered, taperedAngle);
    if (usefulThreadLength > 0.0) {
        const double nominalDepth = usefulThreadLength + pitch;
        validateThreadRadii(majorRadius, majorRadius - nominalDepth * taperSlope);
        validateThreadRadii(correctedMajorRadius, correctedMajorRadius - nominalDepth * taperSlope);
    }
    const double radialExtension = (usefulThreadLength + farEndTrim) * taperSlope;
    majorRadius -= radialExtension;
    correctedMajorRadius -= radialExtension;
    validateThreadRadii(majorRadius, majorRadius + length * taperSlope);
    validateThreadRadii(correctedMajorRadius, correctedMajorRadius + length * taperSlope);

    const std::string threadTypeStr = ThreadTypeEnums[currentThreadTypeIndex];

    Part::TopoShape threadProfile;
    const auto preparedProfile = prepareThreadProfile(
        threadType,
        threadSize,
        threadPitch,
        isInternalThread,
        correctedMajorRadius,
        xDir,
        zDir
    );
    if (preparedProfile) {
        threadProfile = *preparedProfile;
    }
    else {
        if (isWhitworthThread(threadTypeStr)) {

            if (useExtendedWhitworthRoot) {
                threadProfile
                    = makeExtendedWhitworthThreadProfile(correctedMajorRadius, pitch, xDir, zDir);
            }
            else {
                threadProfile
                    = makeLegacyThreadProfile(threadTypeStr, correctedMajorRadius, pitch, xDir, zDir);
            }
        }
        else {
            threadProfile
                = makeLegacyThreadProfile(threadTypeStr, correctedMajorRadius, pitch, xDir, zDir);
        }
    }
    const TopoDS_Wire threadWire = TopoDS::Wire(threadProfile.getShape());

    gp_Trsf profileToLocal;
    profileToLocal.SetTransformation(gp_Ax3(gp_Pnt(0.0, 0.0, 0.0), gp_Dir(zDir), gp_Dir(xDir)));
    const TopoDS_Shape localProfile
        = BRepBuilderAPI_Transform(threadWire, profileToLocal, true).Shape();
    Bnd_Box profileBounds;
    BRepBndLib::Add(localProfile, profileBounds);
    double minX, minY, minZ, maxX, maxY, maxZ;
    profileBounds.Get(minX, minY, minZ, maxX, maxY, maxZ);
    validateThreadRadii(minX, minX + length * taperSlope);

    const double threadDepth = length;
    const double helixLength = threadDepth;
    const double helixAngle = tapered ? 90 - taperedAngle : 0.0;

    TopoDS_Shape helix
        = TopoShape().makeLongHelix(pitch, helixLength, majorRadius, helixAngle, leftHanded);

    const gp_Pnt origin(0.0, 0.0, 0.0);
    const gp_Dir helixAxis(0.0, 0.0, 1.0);
    const gp_Dir rotationAxis(1.0, 0.0, 0.0);

    gp_Trsf mov;
    mov.SetRotation(gp_Ax1(origin, rotationAxis), std::numbers::pi);
    const TopLoc_Location loc1(mov);
    helix.Move(loc1);

    rotateToNormal(helixAxis, zDir, helix);

    BRepOffsetAPI_MakePipeShell pipeShellBuilder(TopoDS::Wire(helix));
    pipeShellBuilder.SetTolerance(Precision::Confusion());
    pipeShellBuilder.SetTransitionMode(BRepBuilderAPI_Transformed);
    pipeShellBuilder.SetMode(true);
    pipeShellBuilder.Add(threadWire);
    if (!pipeShellBuilder.IsReady()) {
        throw Base::CADKernelError(QT_TRANSLATE_NOOP("Exception", "Error: Thread could not be built"));
    }
    const TopoDS_Shape shell = pipeShellBuilder.Shape();

    TopTools_ListOfShape simulatedSections;
    pipeShellBuilder.Simulate(2, simulatedSections);
    std::vector<TopoDS_Wire> frontWires, backWires;
    frontWires.push_back(TopoDS::Wire(simulatedSections.First()));
    backWires.push_back(TopoDS::Wire(simulatedSections.Last()));
    const TopoDS_Shape front = Part::FaceMakerCheese::makeFace(frontWires);
    const TopoDS_Shape back = Part::FaceMakerCheese::makeFace(backWires);

    BRepBuilderAPI_Sewing sewer;
    sewer.SetTolerance(Precision::Confusion());
    sewer.Add(front);
    sewer.Add(back);
    sewer.Add(shell);
    sewer.Perform();

    BRepBuilderAPI_MakeSolid solidBuilder;
    solidBuilder.Add(TopoDS::Shell(sewer.SewedShape()));
    if (!solidBuilder.IsDone()) {
        throw Base::CADKernelError(QT_TRANSLATE_NOOP("Exception", "Error: Result is not a solid"));
    }
    TopoDS_Shape result = solidBuilder.Shape();

    BRepClass3d_SolidClassifier solidClassifier(result);
    solidClassifier.PerformInfinitePoint(Precision::Confusion());
    if (solidClassifier.State() == TopAbs_IN) {
        result.Reverse();
    }

    return result;
}


static gp_Ax1 getLateralFaceAxis(const TopoDS_Face& face)
{
    if (face.IsNull()) {
        throw Base::ValueError("LateralFace does not reference a valid shape/face.");
    }

    BRepAdaptor_Surface surface(face);

    switch (surface.GetType()) {
        case GeomAbs_Cylinder:
            return surface.Cylinder().Axis();
        case GeomAbs_Cone:
            return surface.Cone().Axis();
        default:
            throw Base::ValueError("The selected face must be cylindrical or conical.");
    }
}

static TopoDS_Face getSelectedFace(const App::PropertyLinkSub& faceProp)
{
    App::DocumentObject* obj = faceProp.getValue();
    if (!obj || !obj->getNameInDocument()) {
        return TopoDS_Face();
    }

    const std::vector<std::string>& subs = faceProp.getSubValues();
    if (subs.size() != 1 || subs.front().empty()) {
        return TopoDS_Face();
    }

    auto feature = dynamic_cast<Part::Feature*>(obj);
    if (!feature) {
        return TopoDS_Face();
    }

    const Part::TopoShape& topoShape = feature->Shape.getShape();

    if (topoShape.isNull()) {
        return TopoDS_Face();
    }

    TopoDS_Shape subShape;
    try {
        subShape = topoShape.getSubShape(subs.front().c_str());
    }
    catch (const Standard_Failure& e) {
        Base::Console().warning("getSelectedFace: OCCT exception: %s\n", e.GetMessageString());
        return TopoDS_Face();
    }
    catch (const Base::Exception& e) {
        Base::Console().warning("getSelectedFace: %s\n", e.what());
        return TopoDS_Face();
    }

    if (subShape.IsNull() || subShape.ShapeType() != TopAbs_FACE) {
        return TopoDS_Face();
    }

    return TopoDS::Face(subShape);
}

double ThreadUtils::getConicalAngle(const App::PropertyLinkSub& lateralFace)
{
    const TopoDS_Face face = getSelectedFace(lateralFace);
    if (getFaceType(face) == FaceType::Cone) {
        return BRepAdaptor_Surface(face).Cone().SemiAngle();
    }

    return 0.0;
}

double ThreadUtils::getCylinderDiameter(const TopoDS_Face& face)
{
    if (face.IsNull()) {
        return -1.0;
    }
    BRepAdaptor_Surface surface(face);
    return surface.GetType() == GeomAbs_Cylinder ? 2.0 * surface.Cylinder().Radius() : -1.0;
}

static double getLateralFaceDiameterAtPoint(const TopoDS_Face& face, const gp_Pnt& point)
{
    const gp_Ax1 axis = getLateralFaceAxis(face);
    BRepAdaptor_Surface surface(face);
    if (surface.GetType() == GeomAbs_Cylinder) {
        return 2.0 * surface.Cylinder().Radius();
    }

    const gp_Cone cone = surface.Cone();
    const double axialDistance = gp_Vec(axis.Location(), point).Dot(gp_Vec(axis.Direction()));
    const double radius = cone.RefRadius() + axialDistance * std::tan(cone.SemiAngle());
    if (!std::isfinite(radius) || radius <= Precision::Confusion()) {
        throw Base::ValueError("Thread error: Invalid cone radius at the thread start.");
    }
    return 2.0 * radius;
}

double ThreadUtils::getLateralFaceDiameter(const App::PropertyLinkSub& lateralFace)
{
    const TopoDS_Face face = getSelectedFace(lateralFace);
    return getLateralFaceDiameterAtPoint(
        face,
        getThreadStartPoint(lateralFace, getLateralFaceAxis(face).Direction())
    );
}

double ThreadUtils::getLateralFaceDiameter(
    const App::PropertyLinkSub& lateralFace,
    const App::PropertyLinkSub& startPlane
)
{
    return getLateralFaceDiameterAtPoint(
        getSelectedFace(lateralFace),
        getThreadStartPoint(lateralFace, startPlane)
    );
}

bool ThreadUtils::isConicalFace(const App::PropertyLinkSub& lateralFace) const
{
    return getFaceType(getSelectedFace(lateralFace)) == FaceType::Cone;
}

int ThreadUtils::findNearestThreadSize(const int threadType, const double diameter)
{
    const std::vector<std::string> threadDiameters = getThreadDiameters(threadType);

    if (threadDiameters.empty()) {
        return -1;
    }

    int bestIndex = 0;
    double bestDistance = std::abs(parseThreadValue(threadDiameters[0]) - diameter);

    for (size_t i = 1; i < threadDiameters.size(); ++i) {

        const double currentDiameter = parseThreadValue(threadDiameters[i]);

        const double distance = std::abs(currentDiameter - diameter);

        if (distance < bestDistance) {
            bestDistance = distance;
            bestIndex = static_cast<int>(i);
        }
    }

    return bestIndex;
}
std::optional<ThreadUtils::ThreadSizeSelection> ThreadUtils::findNearestMinorThreadSize(
    const int threadType,
    const double diameter
) const
{
    const auto& definitions = getThreadDefinitions();
    if (threadType < 0 || static_cast<size_t>(threadType) >= definitions.size()) {
        return std::nullopt;
    }

    const auto& definition = definitions[threadType];
    const std::vector<std::string> diameters = getThreadDiameters(threadType);
    std::optional<ThreadSizeSelection> bestSelection;
    double bestDistance = std::numeric_limits<double>::max();

    const size_t rowCount = std::min(definition.sizes.size(), definition.pitches.size());
    for (size_t row = 0; row < rowCount; ++row) {
        if (definition.sizes[row].empty() || definition.pitches[row].empty()) {
            continue;
        }

        const double majorDiameter = parseThreadValue(definition.sizes[row]);
        const double rowPitch = parseThreadValue(definition.pitches[row]);

        int sizeIndex = -1;
        for (size_t i = 0; i < diameters.size(); ++i) {
            if (std::abs(parseThreadValue(diameters[i]) - majorDiameter) < threadDefinitionTolerance) {
                sizeIndex = static_cast<int>(i);
                break;
            }
        }
        if (sizeIndex < 0) {
            continue;
        }

        const std::vector<std::string> pitches = getThreadPitches(threadType, sizeIndex);
        int pitchIndex = -1;
        for (size_t i = 0; i < pitches.size(); ++i) {
            if (std::abs(parseThreadValue(pitches[i]) - rowPitch) < threadDefinitionTolerance) {
                pitchIndex = static_cast<int>(i);
                break;
            }
        }
        if (pitchIndex < 0) {
            continue;
        }

        const double minorDiameter = getMinorDiameterForRow(definition, row, false);
        const double distance = std::abs(minorDiameter - diameter);
        if (distance < bestDistance) {
            bestDistance = distance;
            bestSelection = ThreadSizeSelection {sizeIndex, pitchIndex, minorDiameter};
        }
    }

    return bestSelection;
}

bool ThreadUtils::isInternalFace(const App::PropertyLinkSub& faceProp, const TopoDS_Shape& solid)
{
    if (solid.IsNull()) {
        return false;
    }

    const TopoDS_Face face = getSelectedFace(faceProp);

    if (face.IsNull()) {
        return false;
    }

    BRepAdaptor_Surface surface(face);

    const GeomAbs_SurfaceType surfaceType = surface.GetType();


    if (surfaceType != GeomAbs_Cylinder && surfaceType != GeomAbs_Cone) {

        return false;
    }


    gp_Ax1 axis;

    if (surfaceType == GeomAbs_Cylinder) {

        const gp_Cylinder cylinder = surface.Cylinder();

        axis = cylinder.Axis();
    }
    else if (surfaceType == GeomAbs_Cone) {

        const gp_Cone cone = surface.Cone();

        axis = cone.Axis();
    }


    Standard_Real u1, u2, v1, v2;

    BRepTools::UVBounds(face, u1, u2, v1, v2);

    const Standard_Real u = (u1 + u2) * 0.5;

    const Standard_Real v = (v1 + v2) * 0.5;

    gp_Pnt point;
    gp_Vec du;
    gp_Vec dv;

    surface.D1(u, v, point, du, dv);


    gp_Vec normal = du.Crossed(dv);

    if (normal.Magnitude() < Precision::Confusion()) {

        return false;
    }

    normal.Normalize();

    if (face.Orientation() == TopAbs_REVERSED) {

        normal.Reverse();
    }


    gp_Vec axisDirection = axis.Direction();

    axisDirection.Normalize();

    const gp_Vec axisToPoint(axis.Location(), point);

    const Standard_Real axialDistance = axisToPoint.Dot(axisDirection);

    const gp_Pnt axisPoint = axis.Location().Translated(axisDirection * axialDistance);

    gp_Vec radial(axisPoint, point);

    if (radial.Magnitude() < Precision::Confusion()) {

        return false;
    }

    radial.Normalize();


    const double dot = normal.Dot(radial);

    constexpr double tolerance = 1e-9;

    if (dot < -tolerance) {
        return true;
    }

    if (dot > tolerance) {
        return false;
    }

    return false;
}

App::DocumentObjectExecReturn* ThreadUtils::validateParameters(const App::PropertyLinkSub& lateralFace)
{
    const TopoDS_Face threadedFace = getSelectedFace(lateralFace);

    if (threadedFace.IsNull()) {
        return new App::DocumentObjectExecReturn(
            QT_TRANSLATE_NOOP("Exception", "Please select a cylindrical or conical face.")
        );
    }

    if (getFaceType(threadedFace) == FaceType::Invalid) {
        return new App::DocumentObjectExecReturn(
            QT_TRANSLATE_NOOP("Exception", "The selected face must be cylindrical or conical")
        );
    }

    return App::DocumentObject::StdReturn;
}

gp_Vec ThreadUtils::computePerpendicular(const gp_Vec& zDir) const
{
    gp_Vec xDir;

    if (std::abs(zDir.Z() - zDir.X()) > Precision::Confusion()) {
        xDir = gp_Vec(zDir.Z(), 0, -zDir.X());
    }
    else if (std::abs(zDir.Z() - zDir.Y()) > Precision::Confusion()) {
        xDir = gp_Vec(zDir.Y(), -zDir.X(), 0);
    }
    else {
        xDir = gp_Vec(0, -zDir.Z(), zDir.Y());
    }

    xDir.Normalize();
    return xDir;
}

gp_Pnt ThreadUtils::getThreadAxisOrigin(const App::PropertyLinkSub& lateralFace)
{
    return getLateralFaceAxis(getSelectedFace(lateralFace)).Location();
}

gp_Vec ThreadUtils::getThreadZAxis(const App::PropertyLinkSub& lateralFace) const
{
    if (lateralFace.getValue() == nullptr) {
        throw Base::ValueError("LateralFace property is null or not assigned.");
    }

    return gp_Vec(getLateralFaceAxis(getSelectedFace(lateralFace)).Direction());
}

double ThreadUtils::getThroughAllLength(TopoShape base) const
{
    Bnd_Box box;

    if (!base.isNull()) {
        BRepBndLib::Add(base.getShape(), box);
    }

    box.SetGap(0.0);

    return 2.02 * sqrt(box.SquareExtent());
}

ThreadUtils::ThreadLibrary::ThreadLibrary()
{}

void ThreadUtils::ThreadLibrary::readThreadDefinitions()
{
    const std::vector<std::string> dirs {
        App::Application::getResourceDir() + "Mod/PartDesign/Resources/Thread",
        App::Application::getUserAppDataDir() + "PartDesign/Thread"
    };


    definitions.clear();

    for (const auto& dir : dirs) {
        const std::vector<Base::FileInfo> files {Base::FileInfo(dir).getDirectoryContent()};

        for (const auto& file : files) {
            if (file.extension() == "FCStd") {
                try {
                    auto definition = readThreadDefinition(file);

                    if (!definition) {
                        Base::Console().error(
                            "Failed to read thread definition: %s\n",
                            file.filePath().c_str()
                        );
                        continue;
                    }

                    definitions.push_back(std::move(*definition));
                }
                catch (const Base::Exception& e) {
                    Base::Console()
                        .error("Failed reading '%s': %s\n", file.filePath().c_str(), e.what());
                }
                catch (const std::exception& e) {
                    Base::Console()
                        .error("Failed reading '%s': %s\n", file.filePath().c_str(), e.what());
                }
            }
        }
    }

    // Match Hole's standard order and keep newly discovered standards at the end.
    const auto begin = std::begin(ThreadUtils::ThreadTypeEnums);
    const auto end = std::end(ThreadUtils::ThreadTypeEnums) - 1;
    std::stable_sort(
        definitions.begin(),
        definitions.end(),
        [begin, end](const ThreadDefinition& lhs, const ThreadDefinition& rhs) {
            return std::find(begin, end, lhs.id) < std::find(begin, end, rhs.id);
        }
    );
}

std::optional<ThreadUtils::ThreadDefinition> ThreadUtils::ThreadLibrary::readThreadDefinition(
    const Base::FileInfo& file
)
{
    auto* oldDoc = App::GetApplication().getActiveDocument();


    auto* doc = App::GetApplication().openDocument(file.filePath().c_str(), {false, true});

    if (!doc) {
        throw Base::RuntimeError("Unable to open thread definition.");
    }

    auto* currentDoc = App::GetApplication().getActiveDocument();


    try {
        auto definition = readThreadDocument(doc, file.filePath());

        App::GetApplication().closeDocument(doc->getName());

        if (oldDoc) {
            App::GetApplication().setActiveDocument(oldDoc->getName());
        }

        if (!definition) {
            return std::nullopt;
        }

        definition->file = file.filePath();

        return definition;
    }
    catch (...) {
        App::GetApplication().closeDocument(doc->getName());

        if (oldDoc) {
            App::GetApplication().setActiveDocument(oldDoc->getName());
        }

        throw;
    }
}

std::optional<ThreadUtils::ThreadDefinition> ThreadUtils::ThreadLibrary::readThreadDocument(
    App::Document* doc,
    const std::string& source
) const
{
    if (!doc) {
        return std::nullopt;
    }

    auto metadata = ThreadUtils::findMetadata(doc);

    if (!metadata) {
        return std::nullopt;
    }

    ThreadDefinition definition = *metadata;

    ThreadUtils::findThreadProfiles(doc, definition, source);

    findSpreadsheets(doc, definition);


    return definition;
}

namespace
{

double getSignedProfileArea(const TopoDS_Wire& wire)
{
    constexpr int samplesPerEdge = 32;
    std::vector<gp_Pnt> points;

    for (BRepTools_WireExplorer explorer(wire); explorer.More(); explorer.Next()) {
        TopoDS_Edge edge = TopoDS::Edge(explorer.Current());
        BRepAdaptor_Curve curve(edge);
        const double first = curve.FirstParameter();
        const double last = curve.LastParameter();
        const bool reversed = edge.Orientation() == TopAbs_REVERSED;

        for (int i = 0; i < samplesPerEdge; ++i) {
            const double ratio = static_cast<double>(i) / samplesPerEdge;
            const double parameter = reversed ? last - ratio * (last - first)
                                              : first + ratio * (last - first);
            points.push_back(curve.Value(parameter));
        }
    }

    if (points.size() < 3) {
        return 0.0;
    }

    double twiceArea = 0.0;
    for (size_t i = 0; i < points.size(); ++i) {
        const gp_Pnt& current = points[i];
        const gp_Pnt& next = points[(i + 1) % points.size()];
        twiceArea += current.X() * next.Y() - next.X() * current.Y();
    }
    return twiceArea / 2.0;
}

std::optional<ThreadUtils::ThreadDefinition::Profile> readThreadProfile(
    App::Document* doc,
    const char* objectName,
    ThreadUtils::ThreadDefinition::ProfileStatus& status,
    std::string& diagnostic
)
{
    using Profile = ThreadUtils::ThreadDefinition::Profile;
    using ProfileStatus = ThreadUtils::ThreadDefinition::ProfileStatus;

    status = ProfileStatus::Missing;
    diagnostic.clear();

    auto* object = doc->getObject(objectName);
    if (!object) {
        diagnostic = std::string("Object '") + objectName + "' was not found";
        return std::nullopt;
    }

    auto* sketch = dynamic_cast<Part::Part2DObject*>(object);
    if (!sketch) {
        status = ProfileStatus::Invalid;
        diagnostic = std::string("Object '") + objectName + "' is not a 2D sketch object";
        return std::nullopt;
    }

    if (sketch->isError()) {
        status = ProfileStatus::Invalid;
        diagnostic = std::string("Object '") + objectName + "' failed to recompute";
        return std::nullopt;
    }

    if (!sketch->Placement.getValue().isIdentity(Precision::Confusion())) {
        status = ProfileStatus::Invalid;
        diagnostic = std::string("Object '") + objectName + "' must be placed on the XY plane";
        return std::nullopt;
    }

    Part::TopoShape shape = sketch->Shape.getShape();
    if (shape.isNull()) {
        status = ProfileStatus::Invalid;
        diagnostic = std::string("Object '") + objectName + "' has no profile geometry";
        return std::nullopt;
    }

    if (shape.shapeType() != TopAbs_WIRE) {
        status = ProfileStatus::Invalid;
        diagnostic = std::string("Object '") + objectName
            + "' must contain exactly one connected wire without loose geometry";
        return std::nullopt;
    }

    if (!shape.isValid()) {
        status = ProfileStatus::Invalid;
        diagnostic = std::string("Object '") + objectName + "' contains invalid geometry";
        return std::nullopt;
    }

    if (!shape.isClosed()) {
        status = ProfileStatus::Invalid;
        diagnostic = std::string("Object '") + objectName + "' must be a closed wire";
        return std::nullopt;
    }

    const Base::BoundBox3d bounds = shape.getBoundBox();
    const double tolerance = Precision::Confusion();
    if (bounds.MinZ < -tolerance || bounds.MaxZ > tolerance) {
        status = ProfileStatus::Invalid;
        diagnostic = std::string("Object '") + objectName + "' must be planar in XY";
        return std::nullopt;
    }

    if (bounds.MinY < -tolerance || bounds.MaxY > 1.0 + tolerance) {
        status = ProfileStatus::Invalid;
        diagnostic = std::string("Object '") + objectName
            + "' must use normalized axial coordinates in the range 0..1";
        return std::nullopt;
    }

    if (bounds.MinX > tolerance || bounds.MaxX < -tolerance) {
        status = ProfileStatus::Invalid;
        diagnostic = std::string("Object '") + objectName
            + "' must reach X = 0 at the major-radius reference";
        return std::nullopt;
    }

    if (bounds.LengthX() <= tolerance || bounds.LengthY() <= tolerance) {
        status = ProfileStatus::Invalid;
        diagnostic = std::string("Object '") + objectName + "' has a degenerate profile";
        return std::nullopt;
    }

    const TopoDS_Wire wire = TopoDS::Wire(shape.getShape());
    if (getSignedProfileArea(wire) <= tolerance * tolerance) {
        status = ProfileStatus::Invalid;
        diagnostic = std::string("Object '") + objectName
            + "' must be oriented counter-clockwise in the XY plane";
        return std::nullopt;
    }

    BRepBuilderAPI_Copy copy(wire, Standard_True, Standard_False);
    if (!copy.IsDone() || copy.Shape().IsNull()) {
        status = ProfileStatus::Invalid;
        diagnostic = std::string("Object '") + objectName + "' could not be copied";
        return std::nullopt;
    }

    Profile profile {Part::TopoShape(copy.Shape()), bounds.MinX, bounds.MaxX, bounds.MinY, bounds.MaxY};
    status = ProfileStatus::Valid;
    return profile;
}

}  // namespace

void ThreadUtils::findThreadProfiles(
    App::Document* doc,
    ThreadDefinition& definition,
    const std::string& source
)
{
    if (!doc) {
        definition.profileStatus = ThreadDefinition::ProfileStatus::Invalid;
        definition.profileDiagnostic = "Document is null";
        Base::Console().warning(
            "Thread definition '%s': %s. Using the legacy profile.\n",
            source.c_str(),
            definition.profileDiagnostic.c_str()
        );
        return;
    }

    doc->recompute();

    definition.profile = readThreadProfile(
        doc,
        "ThreadProfile",
        definition.profileStatus,
        definition.profileDiagnostic
    );
    if (definition.profileStatus != ThreadDefinition::ProfileStatus::Valid) {
        Base::Console().warning(
            "Thread definition '%s': %s. Using the legacy profile.\n",
            source.c_str(),
            definition.profileDiagnostic.c_str()
        );
    }
    else {
        definition.sketches.emplace_back("ThreadProfile");
    }

    definition.externalProfile = readThreadProfile(
        doc,
        "ExternalThreadProfile",
        definition.externalProfileStatus,
        definition.externalProfileDiagnostic
    );
    if (definition.externalProfileStatus == ThreadDefinition::ProfileStatus::Invalid) {
        Base::Console().warning(
            "Thread definition '%s': %s. The common profile will be used for external threads.\n",
            source.c_str(),
            definition.externalProfileDiagnostic.c_str()
        );
    }
    else if (definition.externalProfileStatus == ThreadDefinition::ProfileStatus::Valid) {
        definition.sketches.emplace_back("ExternalThreadProfile");
    }
}


std::optional<ThreadUtils::ThreadDefinition> ThreadUtils::findMetadata(App::Document* doc)
{

    ThreadUtils::ThreadDefinition definition;

    if (!doc) {
        Base::Console().error("Document is null.\n");
        return std::nullopt;
    }

    auto* obj = doc->getObject("VarSet");
    if (!obj) {
        Base::Console().error("Object 'VarSet' was not found.\n");
        return std::nullopt;
    }

    auto* varset = dynamic_cast<App::VarSet*>(obj);
    if (!varset) {
        Base::Console().error("Object 'VarSet' has invalid type.\n");
        return std::nullopt;
    }

    auto* propId = varset->getPropertyByName("id");
    if (!propId) {
        Base::Console().error("Property 'id' was not found in VarSet.\n");
        return std::nullopt;
    }

    auto* idProp = dynamic_cast<App::PropertyString*>(propId);
    if (!idProp) {
        Base::Console().error("Property 'id' is not string.\n");
        return std::nullopt;
    }

    definition.id = idProp->getValue();

    if (definition.id.empty()) {
        Base::Console().error("Thread resource property 'id' is empty.\n");
        return std::nullopt;
    }

    auto* propName = varset->getPropertyByName("name");
    if (!propName) {
        Base::Console().error("Property 'name' was not found in VarSet.\n");
        return std::nullopt;
    }

    auto* nameProp = dynamic_cast<App::PropertyString*>(propName);
    if (!nameProp) {
        Base::Console().error("Property 'name' is not string.\n");
        return std::nullopt;
    }

    definition.name = nameProp->getValue();

    if (definition.name.empty()) {
        Base::Console().error("Thread resource property 'name' is empty.\n");
        return std::nullopt;
    }

    auto* propDepth = varset->getPropertyByName("Depth");
    if (!propDepth) {
        Base::Console().error("Property 'Depth' was not found in VarSet.\n");
        return std::nullopt;
    }

    auto* depthProp = dynamic_cast<App::PropertyInteger*>(propDepth);
    if (!depthProp) {
        Base::Console().error("Property 'Depth' is not integer.\n");
        return std::nullopt;
    }

    definition.depthType = depthProp->getValue();


    auto* propIsConical = varset->getPropertyByName("Conical");
    if (propIsConical) {
        auto* isConicalProp = dynamic_cast<App::PropertyBool*>(propIsConical);
        if (isConicalProp) {
            definition.isConical = isConicalProp->getValue();
        }
        else {
            Base::Console().log(
                "Property 'isConical' found in VarSet but is not bool. Defaulting to false.\n"
            );
            definition.isConical = false;
        }
    }
    else {
        definition.isConical = false;
    }

    return definition;
}


void ThreadUtils::ThreadLibrary::findSpreadsheets(App::Document* doc, ThreadDefinition& definition) const
{
    std::vector<std::string> sizes;
    std::vector<std::string> minorDiameters;
    std::vector<std::string> pitches;
    std::vector<std::string> designations;
    std::vector<std::string> tapDrills;
    std::vector<std::string> internalClearances;
    std::vector<std::string> externalClearances;

    const std::map<std::string, std::vector<std::string>*> columnTargets = {
        {"designation", &designations},
        {"size", &sizes},
        {"minordiameter", &minorDiameters},
        {"pitch", &pitches},
        {"tapdrill", &tapDrills},
        {"internalclearance", &internalClearances},
        {"externalclearance", &externalClearances}
    };

    auto sanitizeString = [](std::string str, bool lower) -> std::string {
        str.erase(std::remove(str.begin(), str.end(), '\''), str.end());
        str.erase(std::remove(str.begin(), str.end(), '\"'), str.end());
        if (lower) {
            std::transform(str.begin(), str.end(), str.begin(), [](unsigned char ch) {
                return static_cast<char>(std::tolower(ch));
            });
        }
        return str;
    };

    for (auto* docObj : doc->getObjects()) {
        if (docObj && docObj->getTypeId().isDerivedFrom(Spreadsheet::Sheet::getClassTypeId())) {
            auto* sheet = static_cast<Spreadsheet::Sheet*>(docObj);
            auto [start, end] = sheet->getUsedRange();
            if (!start.isValid() || !end.isValid()) {
                continue;
            }

            std::map<std::string, int> headerToCol;

            for (int col = start.col(); col <= end.col(); ++col) {
                if (auto* cell = sheet->getCell(App::CellAddress(0, col))) {
                    std::string headerName;
                    if (cell->getStringContent(headerName) && !headerName.empty()) {
                        std::string cleanHeader = sanitizeString(headerName, true);
                        headerToCol[cleanHeader] = col;
                    }
                }
            }

            std::map<std::string, int> resolvedColumns;
            for (const auto& [name, target] : columnTargets) {
                auto it = headerToCol.find(name);
                if (it != headerToCol.end()) {
                    resolvedColumns[name] = it->second;
                }
            }

            for (int row = 1; row <= end.row(); ++row) {
                // Collect a complete record before appending any row-associated fields.
                std::map<std::string, std::string> values;
                for (const auto& [name, target] : columnTargets) {
                    std::string value;
                    auto colIt = resolvedColumns.find(name);
                    if (colIt != resolvedColumns.end()) {
                        if (auto* cell = sheet->getCell(App::CellAddress(row, colIt->second))) {
                            cell->getStringContent(value);
                        }
                    }
                    values[name] = sanitizeString(value, false);
                }

                // Clearance columns contain independent enumeration options, not row data.
                for (const auto& name : {"internalclearance", "externalclearance"}) {
                    if (!values[name].empty()) {
                        columnTargets.at(name)->push_back(values[name]);
                    }
                }

                const bool hasRowData = std::any_of(values.begin(), values.end(), [](const auto& item) {
                    return item.first != "internalclearance" && item.first != "externalclearance"
                        && !item.second.empty();
                });
                if (!hasRowData) {
                    continue;
                }
                if (values["size"].empty() || values["pitch"].empty()) {
                    Base::Console().warning(
                        "Thread definition '%s', spreadsheet '%s', row %d: "
                        "missing size or pitch; skipping row.\n",
                        definition.id.c_str(),
                        sheet->getNameInDocument(),
                        row + 1
                    );
                    continue;
                }
                try {
                    if (parseThreadValue(values["size"]) <= 0.0
                        || parseThreadValue(values["pitch"]) <= 0.0) {
                        throw Base::ValueError("Size and pitch must be greater than zero");
                    }
                }
                catch (const Base::Exception& e) {
                    Base::Console().warning(
                        "Thread definition '%s', spreadsheet '%s', row %d: %s; skipping row.\n",
                        definition.id.c_str(),
                        sheet->getNameInDocument(),
                        row + 1,
                        e.what()
                    );
                    continue;
                }
                for (const auto& [name, target] : columnTargets) {
                    if (name != "internalclearance" && name != "externalclearance") {
                        target->push_back(values[name]);
                    }
                }
            }
        }
    }

    definition.designations = std::move(designations);
    definition.sizes = std::move(sizes);
    definition.minorDiameters = std::move(minorDiameters);
    definition.pitches = std::move(pitches);
    definition.tapDrills = std::move(tapDrills);
    definition.internalClearances = std::move(internalClearances);
    definition.externalClearances = std::move(externalClearances);
}

gp_Pnt ThreadUtils::getThreadStartPoint(const App::PropertyLinkSub& lateralFace, const gp_Dir& zDir) const
{
    const TopoDS_Face face = getSelectedFace(lateralFace);

    if (face.IsNull()) {
        throw Base::RuntimeError("Thread error: selected face is null");
    }

    const gp_Pnt axisOrigin = getLateralFaceAxis(face).Location();

    TopTools_IndexedMapOfShape vertices;
    TopExp::MapShapes(face, TopAbs_VERTEX, vertices);

    if (vertices.IsEmpty()) {
        throw Base::RuntimeError("Thread error: selected face has no vertices");
    }

    double minProjection = std::numeric_limits<double>::max();
    bool found = false;

    for (int i = 1; i <= vertices.Extent(); ++i) {
        const TopoDS_Vertex vertex = TopoDS::Vertex(vertices(i));
        const gp_Pnt point = BRep_Tool::Pnt(vertex);

        const gp_Vec toPoint(axisOrigin, point);
        const double projection = toPoint.Dot(gp_Vec(zDir));

        if (projection < minProjection) {
            minProjection = projection;
            found = true;
        }
    }

    if (!found) {
        throw Base::RuntimeError("Thread error: could not determine thread start point");
    }

    const gp_Pnt startPoint = axisOrigin.Translated(minProjection * gp_Vec(zDir));

    return startPoint;
}


Part::TopoShape ThreadUtils::reduceExternalThreadBase(
    Part::TopoShape base,
    const App::PropertyLinkSub& lateralFace,
    double majorDiameter,
    double minorDiameter,
    double length,
    const gp_Pnt& startPoint,
    bool tapered,
    double taperedAngle
)
{
    if (!std::isfinite(majorDiameter) || !std::isfinite(minorDiameter) || !std::isfinite(length)
        || length <= Precision::Confusion() || majorDiameter <= Precision::Confusion()
        || minorDiameter <= Precision::Confusion() || minorDiameter >= majorDiameter) {
        throw Base::ValueError(
            QT_TRANSLATE_NOOP("Exception", "Invalid major or minor thread diameter")
        );
    }

    gp_Vec zDir = getThreadZAxis(lateralFace);
    zDir.Normalize();

    const gp_Ax2 axis(startPoint, gp_Dir(zDir));

    const double majorRadius = majorDiameter / 2.0;
    const double minorRadius = minorDiameter / 2.0;

    const bool conicalSupport = isConicalFace(lateralFace);
    const double outerSlope = conicalSupport ? std::tan(getConicalAngle(lateralFace)) : 0.0;
    const double innerSlope = conicalSupport ? -getThreadTaperSlope(tapered, taperedAngle) : 0.0;
    const double outerEndRadius = majorRadius + length * outerSlope;
    const double innerEndRadius = minorRadius + length * innerSlope;
    validateThreadRadii(majorRadius, outerEndRadius);
    validateThreadRadii(minorRadius, innerEndRadius);
    if (innerEndRadius >= outerEndRadius - Precision::Confusion()) {
        throw Base::ValueError(
            QT_TRANSLATE_NOOP("Exception", "Thread error: Reduced thread base exceeds the support radius")
        );
    }

    const auto makeVolume = [&](double firstRadius, double lastRadius) -> TopoDS_Shape {
        if (std::abs(lastRadius - firstRadius) <= Precision::Confusion()) {
            return BRepPrimAPI_MakeCylinder(axis, firstRadius, length).Shape();
        }
        return BRepPrimAPI_MakeCone(axis, firstRadius, lastRadius, length).Shape();
    };
    const TopoDS_Shape outer = makeVolume(majorRadius, outerEndRadius);
    const TopoDS_Shape inner = makeVolume(minorRadius, innerEndRadius);

    BRepAlgoAPI_Cut ring(outer, inner);
    ring.Build();

    if (!ring.IsDone() || ring.Shape().IsNull()) {
        throw Base::RuntimeError("Thread error: failed to create thread reduction volume");
    }

    Part::TopoShape result;
    result.makeElementBoolean(Part::OpCodes::Cut, {base, Part::TopoShape(ring.Shape())}, nullptr, 0.0);

    if (result.isNull()) {
        throw Base::RuntimeError("Thread error: failed to reduce external thread base");
    }

    return result;
}


gp_Dir ThreadUtils::getThreadAxisDir(const App::PropertyLinkSub& lateralFace)
{
    return getLateralFaceAxis(getSelectedFace(lateralFace)).Direction();
}


static TopoDS_Shape getSelectedSubShape(const App::PropertyLinkSub& prop)
{
    App::DocumentObject* obj = prop.getValue();
    if (!obj) {
        return TopoDS_Shape();
    }

    const std::vector<std::string>& subs = prop.getSubValues();

    if (obj->getTypeId().isDerivedFrom(Base::Type::fromName("Part::DatumPoint"))
        || obj->getTypeId().isDerivedFrom(Base::Type::fromName("Part::DatumLine"))
        || obj->getTypeId().isDerivedFrom(Base::Type::fromName("Part::DatumPlane"))
        || obj->getTypeId().isDerivedFrom(Base::Type::fromName("PartDesign::Datum"))
        || obj->getTypeId().isDerivedFrom(Base::Type::fromName("PartDesign::Point"))
        || obj->getTypeId().isDerivedFrom(Base::Type::fromName("PartDesign::Line"))
        || obj->getTypeId().isDerivedFrom(Base::Type::fromName("PartDesign::Plane"))) {

        const App::Property* rawPlacement = obj->getPropertyByName("Placement");
        if (!rawPlacement
            || !rawPlacement->getTypeId().isDerivedFrom(App::PropertyPlacement::getClassTypeId())) {
            return TopoDS_Shape();
        }

        const auto* propPlacement = static_cast<const App::PropertyPlacement*>(rawPlacement);
        const Base::Placement trsf = propPlacement->getValue();
        const Base::Vector3d pos = trsf.getPosition();

        if (obj->getTypeId().isDerivedFrom(Base::Type::fromName("Part::DatumPoint"))
            || obj->getTypeId().isDerivedFrom(Base::Type::fromName("PartDesign::Point"))) {
            BRepBuilderAPI_MakeVertex mkVertex(gp_Pnt(pos.x, pos.y, pos.z));
            return mkVertex.Vertex();
        }

        if (obj->getTypeId().isDerivedFrom(Base::Type::fromName("Part::DatumLine"))
            || obj->getTypeId().isDerivedFrom(Base::Type::fromName("PartDesign::Line"))) {
            const Base::Rotation rot = trsf.getRotation();
            Base::Vector3d dir(0, 0, 1);
            rot.multVec(dir, dir);

            const gp_Pnt pnt(pos.x, pos.y, pos.z);
            const gp_Dir direction(dir.x, dir.y, dir.z);
            const gp_Lin line(pnt, direction);

            BRepBuilderAPI_MakeEdge mkEdge(line, -1000.0, 1000.0);
            return mkEdge.Edge();
        }

        if (obj->getTypeId().isDerivedFrom(Base::Type::fromName("Part::DatumPlane"))
            || obj->getTypeId().isDerivedFrom(Base::Type::fromName("PartDesign::Plane"))) {
            const Base::Rotation rot = trsf.getRotation();
            Base::Vector3d normal(0, 0, 1);
            rot.multVec(normal, normal);

            const gp_Pnt pnt(pos.x, pos.y, pos.z);
            const gp_Dir dirNormal(normal.x, normal.y, normal.z);
            const gp_Pln plane(pnt, dirNormal);

            BRepBuilderAPI_MakeFace mkFace(plane, -1000.0, 1000.0, -1000.0, 1000.0);
            return mkFace.Face();
        }
    }

    auto* feature = dynamic_cast<Part::Feature*>(obj);
    if (!feature) {
        return TopoDS_Shape();
    }

    const Part::TopoShape& topoShape = feature->Shape.getShape();
    if (topoShape.isNull()) {
        return TopoDS_Shape();
    }

    if (subs.empty() || subs.front().empty()) {
        return topoShape.getShape();
    }

    TopoDS_Shape subShape;
    try {
        subShape = topoShape.getSubShape(subs.front().c_str());
    }
    catch (const Standard_Failure& e) {
        Base::Console().warning("getSelectedSubShape: OCCT exception: %s\n", e.GetMessageString());
        return TopoDS_Shape();
    }
    catch (const Base::Exception& e) {
        Base::Console().warning("getSelectedSubShape: %s\n", e.what());
        return TopoDS_Shape();
    }

    if (subShape.IsNull()) {
        return TopoDS_Shape();
    }

    return subShape;
}


std::vector<gp_Pnt> ThreadUtils::findLineCurveIntersections(
    const gp_Lin& line,
    const BRepAdaptor_Curve& curve
)
{
    std::vector<gp_Pnt> intersections;

    Handle(Geom_Curve) geomCurve = curve.Curve().Curve();

    if (geomCurve.IsNull()) {
        return intersections;
    }

    Handle(Geom_Line) geomLine = new Geom_Line(line);

    GeomAPI_ExtremaCurveCurve extrema(geomLine, geomCurve);

    const Standard_Integer nbExtrema = extrema.NbExtrema();
    const double tolerance = Precision::Confusion() * 10.0;

    for (Standard_Integer i = 1; i <= nbExtrema; ++i) {

        gp_Pnt linePoint;
        gp_Pnt curvePoint;

        extrema.Points(i, linePoint, curvePoint);

        const double distance = linePoint.Distance(curvePoint);

        if (distance <= tolerance) {
            bool alreadyExists = false;

            for (const gp_Pnt& existing : intersections) {
                if (existing.Distance(curvePoint) <= tolerance) {
                    alreadyExists = true;
                    break;
                }
            }

            if (!alreadyExists) {
                intersections.push_back(curvePoint);
            }
        }
    }

    return intersections;
}

gp_Pnt ThreadUtils::getPlaneLineIntersection(const gp_Pln& plane, const gp_Lin& line)
{
    const gp_Pnt& lineOrigin = line.Location();
    const gp_Dir& lineDir = line.Direction();

    const gp_Pnt& planeOrigin = plane.Location();
    const gp_Dir& planeNormal = plane.Axis().Direction();

    const double dx = planeOrigin.X() - lineOrigin.X();
    const double dy = planeOrigin.Y() - lineOrigin.Y();
    const double dz = planeOrigin.Z() - lineOrigin.Z();

    const double denominator = planeNormal.X() * lineDir.X() + planeNormal.Y() * lineDir.Y()
        + planeNormal.Z() * lineDir.Z();

    if (Abs(denominator) <= Precision::Angular()) {
        throw Base::ValueError(
            QT_TRANSLATE_NOOP("Exception", "Thread axis is parallel to the selected plane.")
        );
    }

    const double numerator = planeNormal.X() * dx + planeNormal.Y() * dy + planeNormal.Z() * dz;

    const double t = numerator / denominator;

    const gp_Pnt intersection = lineOrigin.Translated(gp_Vec(lineDir) * t);

    const double distanceToPlane = plane.Distance(intersection);

    return intersection;
}

gp_Pnt ThreadUtils::getThreadStartPoint(
    const App::PropertyLinkSub& lateralFace,
    const App::PropertyLinkSub& startPlane
)
{
    gp_Vec zDir = getThreadZAxis(lateralFace);
    zDir.Normalize();
    const gp_Pnt axisOrigin = getThreadStartPoint(lateralFace, gp_Dir(zDir));

    const gp_Dir axisDir = getThreadAxisDir(lateralFace);
    const gp_Ax1 threadAxis(axisOrigin, axisDir);
    const gp_Lin axisLine(threadAxis);

    auto* obj = startPlane.getValue();
    if (obj == nullptr) {
        return axisOrigin;
    }

    const TopoDS_Shape& shape = getSelectedSubShape(startPlane);

    if (shape.IsNull()) {
        return axisOrigin;
    }

    switch (shape.ShapeType()) {
        case TopAbs_VERTEX: {
            const gp_Pnt pt = BRep_Tool::Pnt(TopoDS::Vertex(shape));
            if (axisLine.Distance(pt) > Precision::Confusion() * 10) {
                throw Base::ValueError(
                    QT_TRANSLATE_NOOP("Exception", "Selected point does not lie on the thread axis.")
                );
            }
            return pt;
        }

        case TopAbs_EDGE: {
            BRepAdaptor_Curve curveAdaptor(TopoDS::Edge(shape));

            const std::vector<gp_Pnt> intersections
                = findLineCurveIntersections(axisLine, curveAdaptor);

            if (intersections.empty()) {
                throw Base::ValueError("Selected edge does not intersect the thread axis.");
            }

            return intersections.front();
        }

        case TopAbs_FACE: {
            const TopoDS_Face face = TopoDS::Face(shape);
            BRepAdaptor_Surface surfAdaptor(face);

            if (surfAdaptor.GetType() == GeomAbs_Plane) {
                const gp_Pln plane = surfAdaptor.Plane();

                const gp_Pnt intersection = getPlaneLineIntersection(plane, axisLine);

                return intersection;
            }

            IntCurvesFace_ShapeIntersector intersector;
            intersector.Load(face, Precision::Confusion());
            intersector.Perform(axisLine, -1e5, 1e5);

            if (!intersector.IsDone() || intersector.NbPnt() == 0) {
                throw Base::ValueError("Selected face/surface does not intersect the thread axis.");
            }

            gp_Pnt bestPoint;
            double minDistance = std::numeric_limits<double>::max();

            for (int i = 1; i <= intersector.NbPnt(); ++i) {
                const gp_Pnt pnt = intersector.Pnt(i);
                const double dist = pnt.Distance(axisOrigin);

                if (dist < minDistance) {
                    minDistance = dist;
                    bestPoint = pnt;
                }
            }

            return bestPoint;
        }
    }

    throw Base::ValueError("Invalid start object selected for thread calculation.");
}

gp_Pnt ThreadUtils::getThreadFarPoint(const App::PropertyLinkSub& lateralFace, const gp_Dir& zDir) const
{
    const TopoDS_Face face = getSelectedFace(lateralFace);

    if (face.IsNull()) {
        throw Base::RuntimeError("Thread error: selected face is null");
    }

    const gp_Pnt axisOrigin = getLateralFaceAxis(face).Location();

    TopTools_IndexedMapOfShape vertices;
    TopExp::MapShapes(face, TopAbs_VERTEX, vertices);

    if (vertices.IsEmpty()) {
        throw Base::RuntimeError("Thread error: selected face has no vertices");
    }

    double maxProjection = std::numeric_limits<double>::lowest();
    bool found = false;

    for (int i = 1; i <= vertices.Extent(); ++i) {
        const TopoDS_Vertex vertex = TopoDS::Vertex(vertices(i));
        const gp_Pnt point = BRep_Tool::Pnt(vertex);

        const gp_Vec toPoint(axisOrigin, point);
        const double projection = toPoint.Dot(gp_Vec(zDir));

        if (projection > maxProjection) {
            maxProjection = projection;
            found = true;
        }
    }

    if (!found) {
        throw Base::RuntimeError("Thread error: could not determine thread end point");
    }

    const gp_Pnt endPoint = axisOrigin.Translated(maxProjection * gp_Vec(zDir));

    return endPoint;
}

gp_Pnt ThreadUtils::getThreadEndPoint(
    const App::PropertyLinkSub& lateralFace,
    const App::PropertyLinkSub& upToGeometry,
    const gp_Pnt& startPoint
)
{
    gp_Vec zDir = getThreadZAxis(lateralFace);
    zDir.Normalize();

    const gp_Dir axisDir = getThreadAxisDir(lateralFace);
    const gp_Ax1 threadAxis(startPoint, axisDir);
    const gp_Lin axisLine(threadAxis);

    auto* obj = upToGeometry.getValue();
    if (obj == nullptr) {
        return getThreadFarPoint(lateralFace, gp_Dir(zDir));
    }

    const TopoDS_Shape& shape = getSelectedSubShape(upToGeometry);

    if (shape.IsNull()) {
        return getThreadFarPoint(lateralFace, gp_Dir(zDir));
    }

    switch (shape.ShapeType()) {
        case TopAbs_VERTEX: {
            const gp_Pnt pt = BRep_Tool::Pnt(TopoDS::Vertex(shape));
            if (axisLine.Distance(pt) > Precision::Confusion() * 10) {
                throw Base::ValueError(
                    QT_TRANSLATE_NOOP("Exception", "Selected point does not lie on the thread axis.")
                );
            }
            return pt;
        }

        case TopAbs_EDGE: {
            BRepAdaptor_Curve curveAdaptor(TopoDS::Edge(shape));

            const std::vector<gp_Pnt> intersections
                = findLineCurveIntersections(axisLine, curveAdaptor);

            if (intersections.empty()) {
                throw Base::ValueError("Selected edge does not intersect the thread axis.");
            }

            gp_Pnt bestPoint;
            double minDistance = std::numeric_limits<double>::max();
            bool found = false;
            for (const gp_Pnt& pnt : intersections) {
                const double dist = gp_Vec(startPoint, pnt).Dot(gp_Vec(axisDir));
                if (dist > Precision::Confusion() && dist < minDistance) {
                    minDistance = dist;
                    bestPoint = pnt;
                    found = true;
                }
            }
            if (!found) {
                throw Base::ValueError(
                    "Selected end edge does not intersect the thread axis ahead of the start."
                );
            }
            return bestPoint;
        }

        case TopAbs_FACE: {
            const TopoDS_Face face = TopoDS::Face(shape);
            BRepAdaptor_Surface surfAdaptor(face);

            if (surfAdaptor.GetType() == GeomAbs_Plane) {
                const gp_Pln plane = surfAdaptor.Plane();
                const gp_Pnt intersection = getPlaneLineIntersection(plane, axisLine);
                return intersection;
            }

            IntCurvesFace_ShapeIntersector intersector;
            intersector.Load(face, Precision::Confusion());
            intersector.Perform(axisLine, -1e5, 1e5);

            if (!intersector.IsDone() || intersector.NbPnt() == 0) {
                throw Base::ValueError("Selected face/surface does not intersect the thread axis.");
            }

            gp_Pnt bestPoint;
            double minDistance = std::numeric_limits<double>::max();

            for (int i = 1; i <= intersector.NbPnt(); ++i) {
                const gp_Pnt pnt = intersector.Pnt(i);
                const double dist = gp_Vec(startPoint, pnt).Dot(gp_Vec(axisDir));

                if (dist > Precision::Confusion() && dist < minDistance) {
                    minDistance = dist;
                    bestPoint = pnt;
                }
            }
            if (minDistance == std::numeric_limits<double>::max()) {
                throw Base::ValueError(
                    "Selected end surface does not intersect the thread axis ahead of the start."
                );
            }

            return bestPoint;
        }
    }

    throw Base::ValueError("Invalid end object selected for thread calculation.");
}

double ThreadUtils::getUpToGeometryLength(
    const App::PropertyLinkSub& upToGeometry,
    const App::PropertyLinkSub& lateralFace,
    const App::PropertyLinkSub& startPlane
)
{
    const gp_Pnt startPoint = getThreadStartPoint(lateralFace, startPlane);
    const gp_Pnt endPoint = getThreadEndPoint(lateralFace, upToGeometry, startPoint);

    return gp_Vec(startPoint, endPoint).Dot(getThreadZAxis(lateralFace).Normalized());
}

double ThreadUtils::getUpToFirstLength(
    const App::PropertyLinkSub& lateralFace,
    const App::PropertyLinkSub& startFace,
    Part::TopoShape base,
    const int threadType,
    const int threadSize,
    const bool isInternalThread,
    const bool useCustomThreadClearance,
    const double customThreadClearance,
    App::PropertyEnumeration& threadClass,
    const bool tapered,
    const double taperedAngle
)
{
    const auto diameters = getThreadDiameters(threadType);
    if (threadSize < 0 || static_cast<size_t>(threadSize) >= diameters.size()) {
        throw Base::IndexError(QT_TRANSLATE_NOOP("Exception", "Thread size out of range"));
    }
    const double majorRadius = parseThreadValue(diameters[threadSize]) / 2.0;

    double clearance;
    if (useCustomThreadClearance) {
        clearance = customThreadClearance / 2.0;
    }
    else {
        clearance = getThreadClassClearance(threadType, threadSize, threadClass) / 2.0;
    }

    const double correctedMajorRadius = isInternalThread ? (majorRadius + clearance)
                                                         : (majorRadius - clearance);

    gp_Vec zDirVec = getThreadZAxis(lateralFace);
    zDirVec.Normalize();
    const gp_Dir zDir(zDirVec);

    const gp_Pnt startPoint = getThreadStartPoint(lateralFace, startFace);

    double guideLength = base.isNull() ? 1.0e6 : getThroughAllLength(base);
    if (guideLength <= Precision::Confusion()) {
        guideLength = 1.0e6;
    }

    const gp_Ax2 axis(startPoint, zDir);

    validateThreadRadii(correctedMajorRadius, correctedMajorRadius);
    const double radialSlope = -getThreadTaperSlope(tapered, taperedAngle);
    if (radialSlope < 0.0) {
        guideLength = std::min(guideLength, correctedMajorRadius / -radialSlope);
    }

    TopoDS_Shape guideShape;
    if (std::abs(guideLength * radialSlope) > Precision::Confusion()) {
        const double endRadius = std::max(0.0, correctedMajorRadius + guideLength * radialSlope);
        BRepPrimAPI_MakeCone mkCone(axis, correctedMajorRadius, endRadius, guideLength);
        guideShape = mkCone.Shape();
    }
    else {
        BRepPrimAPI_MakeCylinder mkCyl(axis, correctedMajorRadius, guideLength);
        guideShape = mkCyl.Shape();
    }

    if (guideShape.IsNull()) {
        Base::Console().error("getUpToFirstLength: guideShape is null!\n");
        throw Base::RuntimeError("Thread error: failed to build guide shape for UpToFirst calculation");
    }

    if (base.isNull() || base.getShape().IsNull()) {
        return guideLength;
    }

    BRepAlgoAPI_Common common(guideShape, base.getShape());
    common.Build();

    if (!common.IsDone() || common.Shape().IsNull()) {
        return guideLength;
    }

    const TopoDS_Shape overlap = common.Shape();

    TopTools_IndexedMapOfShape vertices;
    TopExp::MapShapes(overlap, TopAbs_VERTEX, vertices);

    if (vertices.IsEmpty()) {
        return guideLength;
    }

    double minProjection = std::numeric_limits<double>::max();
    bool found = false;

    for (int i = 1; i <= vertices.Extent(); ++i) {
        const gp_Pnt point = BRep_Tool::Pnt(TopoDS::Vertex(vertices(i)));
        const gp_Vec toPoint(startPoint, point);
        const double projection = toPoint.Dot(gp_Vec(zDir));

        if (projection > Precision::Confusion() && projection < minProjection) {
            minProjection = projection;
            found = true;
        }
    }

    if (!found) {
        return guideLength;
    }

    return minProjection;
}
