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

#include <BRep_Tool.hxx>
#include <BRepTools.hxx>
#include <BRepAdaptor_Surface.hxx>
#include <Geom_ConicalSurface.hxx>
#include <Geom_CylindricalSurface.hxx>
#include <Geom_Surface.hxx>
#include <GeomAbs_SurfaceType.hxx>
#include <gp_Circ.hxx>
#include <gp_Cylinder.hxx>
#include <gp_Dir.hxx>
#include <TopExp_Explorer.hxx>
#include <TopoDS.hxx>
#include <TopoDS_Face.hxx>
#include <TopoDS_Wire.hxx>
#include <BRepBuilderAPI_MakeWire.hxx>
#include <BRepBuilderAPI_MakeEdge.hxx>
#include <GC_MakeArcOfCircle.hxx>
#include <BRepBuilderAPI_MakeSolid.hxx>
#include <BRepClass3d_SolidClassifier.hxx>
#include <BRepAlgoAPI_Cut.hxx>
#include <BRepPrimAPI_MakeCylinder.hxx>

#include <App/VarSet.h>
#include <App/PropertyLinks.h>
#include <App/DocumentObject.h>
#include "App/Document.h"
#include <Mod/Part/App/TopoShape.h>
#include <Mod/Part/App/FaceMakerCheese.h>
#include <Mod/Part/App/Tools.h>
#include <Base/Tools.h>
#include <Base/Console.h>
#include <QObject>
#include "Feature.h"

#include <numbers>
#include <iostream>

#include "ThreadUtils.h"

using namespace PartDesign;

/* "None" profile */
const char* ThreadUtils::ThreadClass_None_Enums[] = {"None", nullptr};

/* ISO coarse metric enums */
const char* ThreadUtils::ThreadClass_ISOmetric_Enums[]
    = {"4G", "4H", "5G", "5H", "6G", "6H", "7G", "7H", "8G", "8H", nullptr};

const char* ThreadUtils::ThreadClass_ISOmetricfine_Enums[]
    = {"4G", "4H", "5G", "5H", "6G", "6H", "7G", "7H", "8G", "8H", nullptr};

// ISO 965-1:2013 ISO general purpose metric screw threads - Tolerances - Part 1
// Table 1 - Fundamentral deviations for internal threads ...
// reproduced in: https://www.accu.co.uk/en/p/134-iso-metric-thread-tolerances [retrieved: 2021-01-11]
const double ThreadUtils::ThreadClass_ISOmetric_data[ThreadClass_ISOmetric_data_size_utils][2] = {
    //  Pitch    G
    {0.2, 0.017},  {0.25, 0.018}, {0.3, 0.018},  {0.35, 0.019}, {0.4, 0.019},
    {0.45, 0.020}, {0.5, 0.020},  {0.6, 0.021},  {0.7, 0.022},  {0.75, 0.022},
    {0.8, 0.024},  {1.0, 0.026},  {1.25, 0.028}, {1.5, 0.032},  {1.75, 0.034},
    {2.0, 0.038},  {2.5, 0.042},  {3.0, 0.048},  {3.5, 0.053},  {4.0, 0.060},
    {4.5, 0.063},  {5.0, 0.071},  {5.5, 0.075},  {6.0, 0.080},  {8.0, 0.100}
};

/* According to DIN 76-1 (Thread run-outs and thread undercuts - Part 1: For ISO metric threads in
 * accordance with DIN 13-1) */
const double ThreadUtils::ThreadRunout[ThreadRunout_size_utils][2] = {
    //  Pitch    e1
    {0.2, 1.3},  {0.25, 1.5}, {0.3, 1.8},  {0.35, 2.1}, {0.4, 2.3},  {0.45, 2.6},
    {0.5, 2.8},  {0.6, 3.4},  {0.7, 3.8},  {0.75, 4.0}, {0.8, 4.2},  {1.0, 5.1},
    {1.25, 6.2}, {1.5, 7.3},  {1.75, 8.3}, {2.0, 9.3},  {2.5, 11.2}, {3.0, 13.1},
    {3.5, 15.2}, {4.0, 16.8}, {4.5, 18.4}, {5.0, 20.8}, {5.5, 22.4}, {6.0, 24.0}
};

/* Details from https://en.wikipedia.org/wiki/Unified_Thread_Standard */

/* UTS coarse */
const char* ThreadUtils::ThreadClass_UNC_Enums[] = {"1B", "2B", "3B", nullptr};

/* UTS fine */
const char* ThreadUtils::ThreadClass_UNF_Enums[] = {"1B", "2B", "3B", nullptr};

/* UTS extrafine */
const char* ThreadUtils::ThreadClass_UNEF_Enums[] = {"1B", "2B", "3B", nullptr};

/* BSW */
const char* ThreadUtils::ThreadClass_BSW_Enums[] = {"Medium", "Normal", nullptr};

/* BSF */
const char* ThreadUtils::ThreadClass_BSF_Enums[] = {"Medium", "Normal", nullptr};

const char* ThreadUtils::ThreadDirectionEnums[] = {"Right", "Left", nullptr};


const std::vector<ThreadUtils::ThreadDescription> ThreadUtils::threadDescription[] = {
    /* None */
    {
        {"---", 6.0, 0.0, 0.0},
    },

    /* ISO metric regular */
    /* ISO metric threaded Tap-Drill diameters according to ISO 2306 */
    // {name, thread diameter, thread pitch, Tap-Drill diameter}
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
    /* ISO metric fine (drill = diameter - pitch) */
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
    /* UNC */
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
    /* UNF */
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
    /* UNEF */
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
    /* NPT National pipe threads */
    // Asme B1.20.1
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
    /* BSP */
    // Parallel - ISO 228-1
    // Tapered  - ISO 7-1
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
    /* BSW */
    // BS 84 Basic sizes
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
    /* BSF */
    // BS 84 Basic sizes
    // BS 1157 for drill sizes
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
    /* ISO Tyre valve threads */
    // ISO 4570:2002
    // Ordered as the standard
    {
        {"5v1", 5.334, 0.705, 0},  // Schrader internal
        {"5v2", 5.370, 1.058, 0},  // Presta cap
        {"6v1", 6.160, 0.800, 0},  // Presta body
        {"8v1", 7.798, 0.794, 0},  // Schrader external
        {"9v1", 9.525, 0.794, 0},   {"10v2", 10.414, 0.907, 0}, {"12v1", 12.319, 0.977, 0},
        {"13v1", 12.700, 1.270, 0}, {"8v2", 7.938, 1.058, 0},   {"10v1", 9.800, 1.000, 0},
        {"11v1", 11.113, 1.270, 0}, {"13v2", 12.700, 0.794, 0}, {"15v1", 15.137, 1.000, 0},
        {"16v1", 15.875, 0.941, 0}, {"17v1", 17.137, 1.000, 0}, {"17v2", 17.463, 1.058, 0},
        {"17v3", 17.463, 1.588, 0}, {"19v1", 19.050, 1.588, 0}, {"20v1", 20.642, 1.000, 0},
    }
};

const char* ThreadUtils::DepthTypeEnums[]
    = {"Dimension", "ThroughAll", "UpToGeometry", /*, "UpToFirst", */ nullptr};

const char* ThreadUtils::ThreadTypeEnums[] = {
    "None",              // 0
    "ISOMetricProfile",  // 1
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

const char* ThreadTypeName2Enums[] = {
    "ISO metric regular",
    "None",
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

// TODO: maybe change to lib for finding name in array
int threadTypeFromString(const std::string& type)
{
    for (int i = 0; ThreadUtils::ThreadTypeNameEnums[i] != nullptr; ++i) {
        if (type == ThreadUtils::ThreadTypeNameEnums[i]) {
            return i;
        }
    }

    return 0;  // None, if it is not in the enum
}

// TODO: it should be efficient
std::vector<std::string> ThreadUtils::getThreadTypeEnums()
{
    std::vector<std::string> result;

    // for (int i = 0; ThreadUtils::ThreadTypeEnums[i] != nullptr; ++i) {
    // result.push_back(std::string(ThreadUtils::ThreadTypeEnums[i]));
    // }

    const auto& definitions = getThreadDefinitions();

    for (const auto& definition : definitions) {
        result.push_back(definition.id);
    }

    return result;
}

std::vector<std::string> ThreadUtils::getThreadTypeNameEnums()
{
    std::vector<std::string> result;

    // for (int i = 0; ThreadUtils::ThreadTypeNameEnums[i] != nullptr; ++i) {
    //     result.push_back(std::string(ThreadUtils::ThreadTypeNameEnums[i]));
    // }

    const auto& definitions = getThreadDefinitions();

    for (const auto& definition : definitions) {
        result.push_back(definition.name);
    }

    return result;
}

std::vector<std::string> ThreadUtils::getThreadTypeName2Enums()
{
    std::vector<std::string> result;

    for (int i = 0; ThreadTypeName2Enums[i] != nullptr; ++i) {
        result.push_back(std::string(ThreadTypeName2Enums[i]));
    }

    // const auto& definitions = getThreadDefinitions();

    // for (const auto& definition : definitions) {
    //     result.push_back(definition.name);
    // }

    return result;
}

std::vector<std::string> ThreadUtils::getThreadDiameters(const int threadType)
{
    std::vector<std::string> currentThreads = ThreadUtils::getThreadTypeNameEnums();
    std::string currentThread = currentThreads[threadType];
    int currentThreadTypeIndex = threadTypeFromString(currentThread);
    // Base::Console().message("real int: %d\n", currentThreadTypeIndex);

    std::vector<std::string> sizes;  // diameters
    const auto& definitions = getThreadDefinitions();
    for (const auto& definition : definitions) {
        if (definition.name == currentThread) {
            sizes = definition.sizes;
            break;
        }
    }
    if (sizes.empty()) {
        return {"6.0"};
    }

    std::set<double> uniqueDiameters;

    // for (const auto& thread : ThreadUtils::threadDescription[currentThreadTypeIndex]) {
    for (const auto& size : sizes) {
        uniqueDiameters.insert(std::stod(size));
        // uniqueDiameters.insert(thread.diameter);
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
    std::vector<std::string> currentThreads = ThreadUtils::getThreadTypeNameEnums();
    std::string currentThread = currentThreads[threadType];
    int currentThreadTypeIndex = threadTypeFromString(currentThread);
    // Base::Console().message("real int: %d\n", currentThreadTypeIndex);

    std::vector<std::string> minorDiameters;  // diameters
    const auto& definitions = getThreadDefinitions();
    for (const auto& definition : definitions) {
        if (definition.name == currentThread) {
            minorDiameters = definition.minorDiameters;
            break;
        }
    }
    if (minorDiameters.empty()) {
        return {"6.0"};
    }

    std::set<double> uniqueMinorDiameters;

    // for (const auto& thread : ThreadUtils::threadDescription[currentThreadTypeIndex]) {
    for (const auto& minorDiameter : minorDiameters) {
        uniqueMinorDiameters.insert(std::stod(minorDiameter));
        // uniqueDiameters.insert(thread.diameter);
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

// TODO: return with threadclass in account
double ThreadUtils::getMinorDiameter(const int threadType, const int size)  // const int threadclass)
{
    std::vector<std::string> currentThreads = ThreadUtils::getThreadTypeNameEnums();
    std::string currentThread = currentThreads[threadType];
    int currentThreadTypeIndex = threadTypeFromString(currentThread);

    std::vector<std::string> minorDiameters;
    const auto& definitions = getThreadDefinitions();
    for (const auto& definition : definitions) {
        if (definition.name == currentThread) {
            minorDiameters = definition.minorDiameters;
            break;
        }
    }
    if (minorDiameters.empty()) {
        return 0.0;
    }

    // TODO: protect this against invalid access
    return std::abs(std::stod(minorDiameters[size]));
}

std::vector<std::string> ThreadUtils::getThreadPitches(const int threadType, const int threadDiameter)
{

    std::vector<std::string> currentThreads = ThreadUtils::getThreadTypeNameEnums();
    std::string currentThread = currentThreads[threadType];
    int currentThreadTypeIndex = threadTypeFromString(currentThread);
    std::vector<std::string> pitches;

    std::vector<std::string> diameters = getThreadDiameters(threadType);
    std::string targetDiameter = diameters[threadDiameter];

    double targeDiameterDouble = std::stod(targetDiameter);

    std::vector<std::string> pitchesDefinitions;
    std::vector<std::string> sizes;  // diameters
    const auto& definitions = getThreadDefinitions();
    for (const auto& definition : definitions) {
        if (definition.name == currentThread) {
            sizes = definition.sizes;
            pitchesDefinitions = definition.pitches;
            break;
        }
    }
    if (sizes.empty()) {
        return {"0.0"};
    }
    if (pitchesDefinitions.empty()) {
        return {"0.0"};
    }

    // get all pitches from a selected diameter
    // for (const auto& thread : ThreadUtils::threadDescription[currentThreadTypeIndex]) {
    //     if (std::abs(thread.diameter - targeDiameterDouble) < 0.001) {
    //         std::ostringstream oss;
    //         oss << std::noshowpoint << thread.pitch << " mm";
    //         pitches.push_back(oss.str());
    //     }
    // }

    // TODO: change the manual index
    size_t index = 0;
    for (const auto& size : sizes) {
        if (std::abs(std::stod(size) - targeDiameterDouble) < 0.001) {
            std::ostringstream oss;
            oss << std::noshowpoint << pitchesDefinitions[index] << " mm";
            pitches.push_back(oss.str());
        }
        ++index;
    }
    return pitches;
}

std::string ThreadUtils::getThreadDesignations(
    const int threadType,
    const int threadDiameter,
    const int threadPitch
)
{
    std::vector<std::string> currentThreads = ThreadUtils::getThreadTypeNameEnums();
    std::string currentThread = currentThreads[threadType];
    int currentThreadTypeIndex = threadTypeFromString(currentThread);


    std::vector<std::string> diameters = getThreadDiameters(threadType);
    std::string targetDiameter = diameters[threadDiameter];
    double targeDiameterDouble = std::stod(targetDiameter);

    std::vector<std::string> pitches = ThreadUtils::getThreadPitches(threadType, threadDiameter);
    std::string targetPitch = pitches[threadPitch];

    double targetPitchDouble = std::stod(targetPitch);

    std::vector<std::string> pitchesDefinitions;
    std::vector<std::string> sizes;  // diameters
    std::vector<std::string> designations;
    const auto& definitions = getThreadDefinitions();
    for (const auto& definition : definitions) {
        if (definition.name == currentThread) {
            sizes = definition.sizes;
            pitchesDefinitions = definition.pitches;
            designations = definition.designations;
            break;
        }
    }
    if (sizes.empty()) {
        return "---";
    }
    if (pitchesDefinitions.empty()) {
        return "---";
    }
    if (designations.empty()) {
        return "---";
    }

    // for (const auto& thread : ThreadUtils::threadDescription[currentThreadTypeIndex]) {
    //     if (std::abs(thread.diameter - targeDiameterDouble) < 0.001) {
    //         if (std::abs(thread.pitch - targetPitchDouble) < 0.001) {
    //             return thread.designation;
    //         }
    //     }
    // }

    // TODO: change the manual index
    size_t index = 0;
    for (const auto& size : sizes) {
        if (std::abs(std::stod(size) - targeDiameterDouble) < 0.001) {
            if (std::abs(std::stod(pitchesDefinitions[index]) - targetPitchDouble) < 0.001) {
                return designations[index];
            }
        }
        ++index;
    }

    return "---";
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

static gp_Pnt toPnt(gp_Vec dir)
{
    return {dir.X(), dir.Y(), dir.Z()};
}

double ThreadUtils::getThreadClassClearance(
    int threadType,
    int threadSize,
    App::PropertyEnumeration& ThreadClass
) const
{
    double pitch = threadDescription[threadType][threadSize].pitch;

    // Calculate how much clearance to add based on Thread tolerance class and pitch
    if (ThreadClass.getValueAsString()[1] == 'G') {
        for (auto it : ThreadClass_ISOmetric_data) {
            double p = it[0];
            if (pitch <= p) {
                return it[1];
            }
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
    auto getRotationAxis = [](const gp_Dir& dir1, const gp_Dir& dir2, gp_Dir& dir3, double& angle) {
        if (dir1.IsEqual(dir2, Precision::Angular())) {
            return false;
        }

        angle = acos(dir1 * dir2);
        if (dir1.IsOpposite(dir2, Precision::Angular())) {
            // Create a vector that is not parallel to dir1
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
    // rotate the helixAxis so that it is pointing in the normalAxis.
    double angle;
    gp_Dir rotAxis;
    if (getRotationAxis(helixAxis, normalAxis, rotAxis, angle)) {
        gp_Pnt origo(0.0, 0.0, 0.0);
        gp_Trsf mov = helixShape.Location().Transformation();
        mov.SetRotation(gp_Ax1(origo, rotAxis), angle);
        TopLoc_Location loc2(mov);
        helixShape.Move(loc2);
    }
}


TopoDS_Shape ThreadUtils::makeThread(
    const gp_Vec& xDir,
    const gp_Vec& zDir,
    double length,
    // const App::PropertyEnumeration& ThreadType,
    // const App::PropertyEnumeration& ThreadSize
    const int threadType,  // TODO: change to ThreadTypeIndex
    const int threadSize,  // TODO: change to ThreadDiameterIndex
    const int leftHanded,
    App::PropertyEnumeration& ThreadClass,
    const bool isInternalThread
)
{
    // Base::Console().message("Rmaj: %lf | RmajC: %lf | Pitch: %lf | clearance: %lf\n", Rmaj,
    // RmajC, Pitch, clearance); Base::Console().message("threadDepth: %lf | helixLength: %lf |
    // holeDepth: %lf\n", threadDepth, helixLength, holeDepth); int threadType =
    // ThreadType.getValue(); int threadSize = ThreadSize.getValue();
    if (threadType < 0) {
        throw Base::IndexError(QT_TRANSLATE_NOOP("Exception", "Thread type out of range"));
    }
    if (threadSize < 0) {
        throw Base::IndexError(QT_TRANSLATE_NOOP("Exception", "Thread size out of range"));
    }

    // if (xDir.Magnitude() <= Precision::Confusion()) {
    // throw Base::ValueError("Invalid xDir");
    // }

    // if (zDir.Magnitude() <= Precision::Confusion()) {
    // throw Base::ValueError("Invalid zDir");
    // }

    // bool leftHanded = (bool)ThreadDirection.getValue();
    // bool leftHanded = false;

    // Nomenclature and formulae according to Figure 1 of ISO 68-1
    // this is the same for all metric and UTS threads as stated here:
    // https://en.wikipedia.org/wiki/File:ISO_and_UTS_Thread_Dimensions.svg
    // Rmaj is half of the major diameter
    std::vector<std::string> currentThreads = ThreadUtils::getThreadTypeNameEnums();
    std::string currentThread = currentThreads[threadType];
    int currentThreadTypeIndex = threadTypeFromString(currentThread);
    double Rmaj = threadDescription[currentThreadTypeIndex][threadSize].diameter / 2;
    // double Pitch = getThreadPitch();
    double Pitch = threadDescription[currentThreadTypeIndex][threadSize].pitch;

    double clearance;  // clearance to be added on the diameter
    // if (UseCustomThreadClearance.getValue()) {
    // clearance = CustomThreadClearance.getValue() / 2;
    // }
    // else {
    clearance = getThreadClassClearance(threadType, threadSize, ThreadClass) / 2;
    // }
    double RmajC = Rmaj + clearance;
    double marginZ = 0.001;

    // start of comment
    BRepBuilderAPI_MakeWire mkThreadWire;
    double H;

    // std::string threadTypeStr = ThreadType.getValueAsString();
    std::vector<std::string> threadTypes = ThreadUtils::getThreadTypeEnums();
    std::string threadTypeStr = threadTypes[currentThreadTypeIndex];

    if (threadTypeStr == "BSP" || threadTypeStr == "BSW" || threadTypeStr == "BSF") {
        H = 0.960491 * Pitch;              // Height of Sharp V
        double radius = 0.137329 * Pitch;  // radius of the crest
        // construct the cross section going counter-clockwise
        // --------------
        // P    | p4
        // 5/8P |                p3
        //      |                         crest
        // 3/8P |                p2
        // 0    | p1
        // --------------
        //      | base-sharpV             Rmaj     H

        // the little adjustment of p1 and p4 is here to prevent coincidencies
        double marginX = std::tan(Base::toRadians(62.5)) * marginZ;

        gp_Pnt p1 = toPnt((RmajC - 5 * H / 6 + marginX) * xDir + marginZ * zDir);
        gp_Pnt p4 = toPnt((RmajC - 5 * H / 6 + marginX) * xDir + (Pitch - marginZ) * zDir);

        //     // Calculate positions for p2 and p3
        double p23x = RmajC - radius * 0.58284013094;

        gp_Pnt p2 = toPnt(p23x * xDir + 3 * Pitch / 8 * zDir);
        gp_Pnt p3 = toPnt(p23x * xDir + 5 * Pitch / 8 * zDir);
        gp_Pnt crest = toPnt((RmajC)*xDir + Pitch / 2 * zDir);

        mkThreadWire.Add(BRepBuilderAPI_MakeEdge(p1, p2).Edge());
        Handle(Geom_TrimmedCurve) arc1 = GC_MakeArcOfCircle(p2, crest, p3).Value();
        mkThreadWire.Add(BRepBuilderAPI_MakeEdge(arc1).Edge());
        mkThreadWire.Add(BRepBuilderAPI_MakeEdge(p3, p4).Edge());
        mkThreadWire.Add(BRepBuilderAPI_MakeEdge(p4, p1).Edge());
    }
    else {
        H = sqrt(3) / 2 * Pitch;  // height of fundamental triangle
        double h = 7 * H / 8;     // distance from Rmaj to the base
        // construct the cross section going counter-clockwise
        // pitch
        // --------------
        // P     | p4
        // 9/16P |                p3
        // 7/16P |                p2
        // 0     | p1
        // --------------
        //       | base-sharpV    Rmaj

        // the little adjustment of p1 and p4 is here to prevent coincidencies
        double marginX = std::tan(Base::toRadians(60.0)) * marginZ;
        gp_Pnt p1 = toPnt((RmajC - h + marginX) * xDir + marginZ * zDir);
        gp_Pnt p2 = toPnt((RmajC)*xDir + 7 * Pitch / 16 * zDir);
        gp_Pnt p3 = toPnt((RmajC)*xDir + 9 * Pitch / 16 * zDir);
        gp_Pnt p4 = toPnt((RmajC - h + marginX) * xDir + (Pitch - marginZ) * zDir);

        std::cout << "before mkThreadWire.add\n";
        mkThreadWire.Add(BRepBuilderAPI_MakeEdge(p1, p2).Edge());
        std::cout << "after mkThreadWire.add\n";
        if (threadTypeStr == "ISOTyre") {
            gp_Pnt crest = toPnt((RmajC + (Pitch / 32)) * xDir + Pitch / 2 * zDir);
            Handle(Geom_TrimmedCurve) arc1 = GC_MakeArcOfCircle(p2, crest, p3).Value();
            mkThreadWire.Add(BRepBuilderAPI_MakeEdge(arc1).Edge());
        }
        else {
            mkThreadWire.Add(BRepBuilderAPI_MakeEdge(p2, p3).Edge());
        }
        mkThreadWire.Add(BRepBuilderAPI_MakeEdge(p3, p4).Edge());
        mkThreadWire.Add(BRepBuilderAPI_MakeEdge(p4, p1).Edge());
    }

    mkThreadWire.Build();
    TopoDS_Wire threadWire = mkThreadWire.Wire();

    // create the helix path
    // double threadDepth = ThreadDepth.getValue();
    double threadDepth = length;
    double helixLength = threadDepth + Pitch / 2;
    // double holeDepth = Depth.getValue();
    double holeDepth = 4;
    // std::string threadDepthMethod(ThreadDepthType.getValueAsString());
    std::string threadDepthMethod("Dimension");
    // std::string depthMethod(DepthType.getValueAsString());
    std::string depthMethod("");
    if (threadDepthMethod != "Dimension") {
        if (depthMethod == "ThroughAll") {
            threadDepth = length;
            // ThreadDepth.setValue(threadDepth);
            helixLength = threadDepth + 2 * Pitch;
        }
        else if (threadDepthMethod == "Tapped (DIN76)") {
            // threadDepth = holeDepth - getThreadRunout();
            // ThreadDepth.setValue(threadDepth);
            helixLength = threadDepth + Pitch / 2;
        }
        else {  // Hole depth
            threadDepth = holeDepth;
            // ThreadDepth.setValue(threadDepth);
            helixLength = threadDepth + Pitch / 8;
        }
    }
    else {
        if (depthMethod == "Dimension") {
            // the thread must not be deeper than the hole
            // thus the max helixLength is holeDepth + P / 8;
            if (threadDepth > (holeDepth - Pitch / 2)) {
                helixLength = holeDepth + Pitch / 8;
            }
        }
    }
    // double helixAngle = Tapered.getValue() ? TaperedAngle.getValue() - 90 : 0.0;
    double helixAngle = 0.0;
    TopoDS_Shape helix = TopoShape().makeLongHelix(Pitch, helixLength, Rmaj, helixAngle, leftHanded);

    gp_Pnt origo(0.0, 0.0, 0.0);
    gp_Dir dir_axis1(0.0, 0.0, 1.0);  // pointing along the helix axis, as created.
    gp_Dir dir_axis2(1.0, 0.0, 0.0);  // pointing towards the helix start point, as created.

    // Reverse the direction of the helix. So that it goes into the material
    gp_Trsf mov;
    mov.SetRotation(gp_Ax1(origo, dir_axis2), std::numbers::pi);
    TopLoc_Location loc1(mov);
    helix.Move(loc1);

    // rotate the helix so that it is pointing in the zdir.
    rotateToNormal(dir_axis1, zDir, helix);

    // create the pipe shell
    BRepOffsetAPI_MakePipeShell mkPS(TopoDS::Wire(helix));
    mkPS.SetTolerance(Precision::Confusion());
    mkPS.SetTransitionMode(BRepBuilderAPI_Transformed);
    mkPS.SetMode(true);  // This is for frenet
    mkPS.Add(threadWire);
    if (!mkPS.IsReady()) {
        throw Base::CADKernelError(QT_TRANSLATE_NOOP("Exception", "Error: Thread could not be built"));
    }
    TopoDS_Shape shell = mkPS.Shape();

    // create faces at the ends of the pipe shell
    TopTools_ListOfShape sim;
    mkPS.Simulate(2, sim);
    std::vector<TopoDS_Wire> frontwires, backwires;
    frontwires.push_back(TopoDS::Wire(sim.First()));
    backwires.push_back(TopoDS::Wire(sim.Last()));
    // build the end faces
    TopoDS_Shape front = Part::FaceMakerCheese::makeFace(frontwires);
    TopoDS_Shape back = Part::FaceMakerCheese::makeFace(backwires);

    // sew the shell and end faces
    BRepBuilderAPI_Sewing sewer;
    sewer.SetTolerance(Precision::Confusion());
    sewer.Add(front);
    sewer.Add(back);
    sewer.Add(shell);
    sewer.Perform();

    // make the closed off shell into a solid
    BRepBuilderAPI_MakeSolid mkSolid;
    mkSolid.Add(TopoDS::Shell(sewer.SewedShape()));
    if (!mkSolid.IsDone()) {
        throw Base::CADKernelError(QT_TRANSLATE_NOOP("Exception", "Error: Result is not a solid"));
    }
    TopoDS_Shape result = mkSolid.Shape();

    // check if the algorithm has confused the inside and outside of the solid
    BRepClass3d_SolidClassifier SC(result);
    SC.PerformInfinitePoint(Precision::Confusion());
    if (SC.State() == TopAbs_IN) {
        result.Reverse();
    }

    // we are done
    return result;

    // TopoDS_Shape emptyTopoDS_Shape;
    // return emptyTopoDS_Shape;
}


static bool isCylindricalFace(const TopoDS_Face& face)
{
    if (face.IsNull()) {
        return false;
    }

    BRepAdaptor_Surface surface(face);

    return surface.GetType() == GeomAbs_Cylinder;
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
        Base::Console().warning("getSelectedFace: OCCT exception:...\n");
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

double ThreadUtils::getCylinderDiameter(const TopoDS_Face& face)
{
    Handle(Geom_Surface) surface = BRep_Tool::Surface(face);

    Handle(Geom_CylindricalSurface) cylinder = Handle(Geom_CylindricalSurface)::DownCast(surface);

    if (cylinder.IsNull()) {
        return -1.0;
    }

    return 2.0 * cylinder->Radius();
}

double ThreadUtils::getLateralFaceDiameter(const App::PropertyLinkSub& lateralFace)
{
    TopoDS_Face face = getSelectedFace(lateralFace);

    return getCylinderDiameter(face);
}

int ThreadUtils::findNearestThreadSize(const int threadType, const double diameter)
{
    std::vector<std::string> threadDiameters = getThreadDiameters(threadType);

    if (threadDiameters.empty()) {
        return -1;
    }

    int bestIndex = 0;
    double bestDistance = std::abs(std::stod(threadDiameters[0]) - diameter);

    for (size_t i = 1; i < threadDiameters.size(); ++i) {

        double currentDiameter = std::stod(threadDiameters[i]);

        double distance = std::abs(currentDiameter - diameter);

        if (distance < bestDistance) {
            bestDistance = distance;
            bestIndex = static_cast<int>(i);
        }
    }

    return bestIndex;
}
int ThreadUtils::findNearestMinorThreadSize(const int threadType, const double diameter)
{
    std::vector<std::string> threadDiameters = getThreadMinorDiameters(threadType);

    if (threadDiameters.empty()) {
        return -1;
    }

    int bestIndex = 0;
    double bestDistance = std::abs(std::stod(threadDiameters[0]) - diameter);

    for (size_t i = 1; i < threadDiameters.size(); ++i) {
        double currentDiameter = std::stod(threadDiameters[i]);
        double distance = std::abs(currentDiameter - diameter);

        Base::Console().message("i=%d\n", i);
        Base::Console().message("value=%s\n", threadDiameters[i]);
        Base::Console().message("distance=%lf\n", distance);
        Base::Console().message("bestDistance=%lf\n", bestDistance);

        if (distance < bestDistance) {
            bestDistance = distance;
            bestIndex = static_cast<int>(i);
        }
    }

    return bestIndex;
}

bool ThreadUtils::isInternalFace(const App::PropertyLinkSub& faceProp, const TopoDS_Shape& solid)
{
    if (solid.IsNull()) {
        return false;
    }

    TopoDS_Face face = getSelectedFace(faceProp);

    if (face.IsNull()) {
        return false;
    }

    BRepAdaptor_Surface surface(face);

    GeomAbs_SurfaceType surfaceType = surface.GetType();

    // -------------------------------------------------
    // Only cylindrical and conical surfaces are
    // currently supported.
    // -------------------------------------------------

    if (surfaceType != GeomAbs_Cylinder && surfaceType != GeomAbs_Cone) {

        return false;
    }

    // -------------------------------------------------
    // Get surface axis
    // -------------------------------------------------

    gp_Ax1 axis;

    if (surfaceType == GeomAbs_Cylinder) {

        gp_Cylinder cylinder = surface.Cylinder();

        axis = cylinder.Axis();
    }
    else if (surfaceType == GeomAbs_Cone) {

        gp_Cone cone = surface.Cone();

        axis = cone.Axis();
    }

    // -------------------------------------------------
    // Get a point in the middle of the face
    // -------------------------------------------------

    Standard_Real u1, u2, v1, v2;

    BRepTools::UVBounds(face, u1, u2, v1, v2);

    Standard_Real u = (u1 + u2) * 0.5;

    Standard_Real v = (v1 + v2) * 0.5;

    gp_Pnt point;
    gp_Vec du;
    gp_Vec dv;

    surface.D1(u, v, point, du, dv);

    // -------------------------------------------------
    // Calculate surface normal
    // -------------------------------------------------

    gp_Vec normal = du.Crossed(dv);

    if (normal.Magnitude() < Precision::Confusion()) {

        return false;
    }

    normal.Normalize();

    /*
     * D1() gives the geometric orientation of the
     * underlying surface. The topological face can
     * reverse that orientation.
     */
    if (face.Orientation() == TopAbs_REVERSED) {

        normal.Reverse();
    }

    // -------------------------------------------------
    // Calculate radial direction
    //
    // Project the point onto the surface axis and
    // construct the vector from the axis to the point.
    // -------------------------------------------------

    gp_Vec axisDirection = axis.Direction();

    axisDirection.Normalize();

    gp_Vec axisToPoint(axis.Location(), point);

    Standard_Real axialDistance = axisToPoint.Dot(axisDirection);

    gp_Pnt axisPoint = axis.Location().Translated(axisDirection * axialDistance);

    gp_Vec radial(axisPoint, point);

    if (radial.Magnitude() < Precision::Confusion()) {

        return false;
    }

    radial.Normalize();

    // -------------------------------------------------
    // Compare normal with radial direction
    // -------------------------------------------------

    double dot = normal.Dot(radial);

    constexpr double tolerance = 1e-9;

    if (dot < -tolerance) {
        return true;
    }

    if (dot > tolerance) {
        return false;
    }

    // Ambiguous / degenerate case.
    return false;
}

App::DocumentObjectExecReturn* ThreadUtils::validateParameters(const App::PropertyLinkSub& LateralFace)
{
    TopoDS_Face threadedFace = getSelectedFace(LateralFace);

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
    // Define xDir
    gp_Vec xDir;

    /* Compute xDir normal to zDir */
    if (std::abs(zDir.Z() - zDir.X()) > Precision::Confusion()) {
        xDir = gp_Vec(zDir.Z(), 0, -zDir.X());
    }
    else if (std::abs(zDir.Z() - zDir.Y()) > Precision::Confusion()) {
        xDir = gp_Vec(zDir.Y(), -zDir.X(), 0);
    }
    else {
        xDir = gp_Vec(0, -zDir.Z(), zDir.Y());
    }

    // Normalize xDir; this is needed as the computation above does not necessarily give
    // a unit-length vector.
    xDir.Normalize();
    return xDir;
}

gp_Pnt ThreadUtils::getThreadAxisOrigin(const App::PropertyLinkSub& LateralFace)
{
    TopoDS_Face threadedFace = getSelectedFace(LateralFace);
    Handle(Geom_Surface) surf = BRep_Tool::Surface(threadedFace);

    if (getFaceType(threadedFace) == FaceType::Cylinder) {
        Handle(Geom_CylindricalSurface) cyl = Handle(Geom_CylindricalSurface)::DownCast(surf);
        Base::Console().message("cyl->Position().Location().z(): %lf\n", cyl->Position().Location().Z());
        return cyl->Position().Location();
    }
    else if (getFaceType(threadedFace) == FaceType::Cone) {
        Handle(Geom_ConicalSurface) cone = Handle(Geom_ConicalSurface)::DownCast(surf);
        return cone->Position().Location();
    }

    throw Base::RuntimeError("Thread axis origin could not be calculated.");
}

gp_Vec ThreadUtils::getThreadZAxis(const App::PropertyLinkSub& LateralFace)
{
    TopoDS_Face threadedFace = getSelectedFace(LateralFace);
    Handle(Geom_Surface) surf = BRep_Tool::Surface(threadedFace);

    if (getFaceType(threadedFace) == FaceType::Cylinder) {
        Handle(Geom_CylindricalSurface) cyl = Handle(Geom_CylindricalSurface)::DownCast(surf);

        gp_Ax3 ax = cyl->Position();

        gp_Dir axis = ax.Direction();

        return gp_Vec(axis);
    }
    else if (getFaceType(threadedFace) == FaceType::Cone) {
        Handle(Geom_ConicalSurface) cone = Handle(Geom_ConicalSurface)::DownCast(surf);

        gp_Ax3 ax = cone->Position();

        return gp_Vec(ax.Direction());
    }

    throw Base::RuntimeError("zDir could not be calculated.");
}

double ThreadUtils::getThroughAllLength() const
{
    /* TODO */
    return 2.02;
}

ThreadUtils::ThreadLibrary::ThreadLibrary()
{}

void ThreadUtils::ThreadLibrary::readThreadDefinitions()
{
    std::vector<std::string> dirs {
        App::Application::getResourceDir() + "Mod/PartDesign/Resources/Thread",
        App::Application::getUserAppDataDir() + "PartDesign/Thread"
    };

    Base::Console().message("Dirs[0]: %s\n", dirs[0]);
    Base::Console().message("Dirs[1]: %s\n", dirs[1]);

    std::clog << "Looking for thread definitions in: ";
    for (const auto& dir : dirs) {
        std::clog << dir << " ";
    }
    std::clog << '\n';

    definitions.clear();

    for (const auto& dir : dirs) {
        std::vector<Base::FileInfo> files {Base::FileInfo(dir).getDirectoryContent()};

        Base::FileInfo fi(dir);
        Base::Console().message("dir exists: %d\n", fi.exists());
        Base::Console().message("dir is dir: %d\n", fi.isDir());

        for (const auto& file : files) {
            if (file.extension() == "FCStd") {
                try {
                    Base::Console().message("Filename: %s\n", file.fileName());
                    // readThreadDefinition(file);
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
                    std::cerr << "Failed reading '" << file.filePath() << "': " << e.what() << '\n';
                }
                catch (const std::exception& e) {
                    std::cerr << "Failed reading '" << file.filePath() << "': " << e.what() << '\n';
                }
            }
        }
    }
}

std::optional<ThreadUtils::ThreadDefinition> ThreadUtils::ThreadLibrary::readThreadDefinition(
    const Base::FileInfo& file
)
{
    auto* oldDoc = App::GetApplication().getActiveDocument();

    Base::Console().message("Before: %s\n", oldDoc ? oldDoc->getName() : "<none>");

    Base::Console().message("Reading a thread file!\n");
    auto* doc = App::GetApplication().openDocument(file.filePath().c_str(), {false, true});

    if (!doc) {
        throw Base::RuntimeError("Unable to open thread definition.");
    }

    auto* currentDoc = App::GetApplication().getActiveDocument();

    Base::Console().message("After open: %s\n", currentDoc ? currentDoc->getName() : "<none>");

    try {
        auto definition = readThreadDocument(doc);

        Base::Console().message("Closing a thread file!\n");
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
        Base::Console().message("Closing a thread file 2!\n");
        App::GetApplication().closeDocument(doc->getName());

        if (oldDoc) {
            App::GetApplication().setActiveDocument(oldDoc->getName());
        }

        throw;
    }
}

std::optional<ThreadUtils::ThreadDefinition> ThreadUtils::ThreadLibrary::readThreadDocument(
    App::Document* doc
)
{
    Base::Console().message("Handling data!\n");
    if (!doc) {
        return std::nullopt;
    }

    auto metadata = ThreadUtils::findMetadata(doc);

    if (!metadata) {
        return std::nullopt;
    }

    ThreadDefinition definition = *metadata;

    // localizar sketches

    // localizar spreadsheets
    findSpreadsheets(doc, definition);

    // registrar ThreadDefinition

    return definition;
}


std::optional<ThreadUtils::ThreadDefinition> ThreadUtils::findMetadata(App::Document* doc)
{
    Base::Console().message("Reading Metadata!\n");

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

    // id
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

    // name
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

    // depth
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

    Base::Console().message("Returning Metadata!\n");

    return definition;
}

#include <Mod/Spreadsheet/App/Sheet.h>

void ThreadUtils::ThreadLibrary::findSpreadsheets(App::Document* doc, ThreadDefinition& definition)
{
    Base::Console().message("Reading spreadsheets\n");
    std::vector<std::string> sizes;
    std::vector<std::string> minorDiameters;
    std::vector<std::string> pitches;
    std::vector<std::string> designations;
    std::vector<std::string> tapDrills;

    // App::DocumentObject* obj = doc->getObject("InternalSpreadsheet");
    App::DocumentObject* obj = doc->getObject("Spreadsheet");

    for (auto* obj : doc->getObjects()) {
        if (obj && obj->getTypeId().isDerivedFrom(Spreadsheet::Sheet::getClassTypeId())) {
            auto* sheet = static_cast<Spreadsheet::Sheet*>(obj);
            // usar sheet
            auto [start, end] = sheet->getUsedRange();

            Base::Console()
                .message("Start: %s | End: %s\n", start.toString().c_str(), end.toString().c_str());

            for (int row = 1; row <= end.row(); ++row) {
                // Column A - Designation
                if (auto* cell = sheet->getCell(App::CellAddress(row, 0))) {
                    std::string value;
                    if (cell->getStringContent(value) && !value.empty()) {
                        designations.push_back(value);
                    }
                }

                // Column B - Size
                if (auto* cell = sheet->getCell(App::CellAddress(row, 1))) {
                    std::string value;
                    if (cell->getStringContent(value) && !value.empty()) {
                        sizes.push_back(value);
                    }
                }

                // Column C - minorDiameter
                if (auto* cell = sheet->getCell(App::CellAddress(row, 2))) {
                    std::string value;
                    if (cell->getStringContent(value) && !value.empty()) {
                        minorDiameters.push_back(value);
                    }
                }

                // Column D - Pitch
                if (auto* cell = sheet->getCell(App::CellAddress(row, 3))) {
                    std::string value;
                    if (cell->getStringContent(value) && !value.empty()) {
                        pitches.push_back(value);
                    }
                }

                // Column E - Tap Drill
                if (auto* cell = sheet->getCell(App::CellAddress(row, 4))) {
                    std::string value;
                    if (cell->getStringContent(value) && !value.empty()) {
                        tapDrills.push_back(value);
                    }
                }
            }
        }
    }
    if (!obj) {
        Base::Console().message("No object found.\n");
        return;
    }

    Base::Console().message("designations.size: %d\n", designations.size());
    Base::Console().message("sizes.size: %d\n", sizes.size());
    Base::Console().message("pitches.size: %d\n", pitches.size());
    Base::Console().message("tapDrills.size: %d\n", tapDrills.size());
    definition.designations = std::move(designations);
    definition.sizes = std::move(sizes);
    definition.minorDiameters = std::move(minorDiameters);
    definition.pitches = std::move(pitches);
    definition.tapDrills = std::move(tapDrills);
}

gp_Pnt ThreadUtils::getThreadStartPoint(const App::PropertyLinkSub& lateralFace, const gp_Dir& zDir)
{
    TopoDS_Face face = getSelectedFace(lateralFace);

    if (face.IsNull()) {
        throw Base::RuntimeError("Thread error: selected face is null");
    }

    // Get the true cylindrical axis from the surface geometry
    BRepAdaptor_Surface surfAdapt(face);
    if (surfAdapt.GetType() != GeomAbs_Cylinder) {
        throw Base::RuntimeError("Thread error: selected face is not cylindrical");
    }

    gp_Ax1 cylAxis = surfAdapt.Cylinder().Axis();
    const gp_Pnt axisOrigin = cylAxis.Location();
    const gp_Dir axisDir = cylAxis.Direction();

    // Collect vertices belonging to the selected face, to find
    // the extremity along zDir (start of the thread)
    TopTools_IndexedMapOfShape vertices;
    TopExp::MapShapes(face, TopAbs_VERTEX, vertices);

    if (vertices.IsEmpty()) {
        throw Base::RuntimeError("Thread error: selected face has no vertices");
    }

    double minProjection = std::numeric_limits<double>::max();
    bool found = false;

    for (int i = 1; i <= vertices.Extent(); ++i) {
        const TopoDS_Vertex vertex = TopoDS::Vertex(vertices(i));
        gp_Pnt point = BRep_Tool::Pnt(vertex);

        // Project the vertex onto the axis direction, relative to axisOrigin
        // (not the world origin!) so we get the correct along-axis coordinate
        gp_Vec toPoint(axisOrigin, point);
        double projection = toPoint.Dot(gp_Vec(zDir));

        if (projection < minProjection) {
            minProjection = projection;
            found = true;
        }
    }

    if (!found) {
        throw Base::RuntimeError("Thread error: could not determine thread start point");
    }

    // Build the start point ON the axis, at the extremity found above
    gp_Pnt startPoint = axisOrigin.Translated(minProjection * gp_Vec(zDir));

    return startPoint;
}

Part::TopoShape ThreadUtils::reduceExternalThreadBase(
    Part::TopoShape base,
    const App::PropertyLinkSub& lateralFace,
    double majorDiameter,
    double minorDiameter,
    double length
)
{
    gp_Vec zDir = getThreadZAxis(lateralFace);
    zDir.Normalize();

    gp_Pnt startPoint = getThreadStartPoint(lateralFace, gp_Dir(zDir));
    Base::Console().message("getThreadStartPoint.Z(): %lf\n", startPoint.Z());
    // gp_Pnt startPoint = getThreadAxisOrigin(lateralFace);

    gp_Ax2 axis(startPoint, gp_Dir(zDir));

    double majorRadius = majorDiameter / 2.0;
    double minorRadius = minorDiameter / 2.0;

    BRepPrimAPI_MakeCylinder outer(axis, majorRadius, length);
    BRepPrimAPI_MakeCylinder inner(axis, minorRadius, length);
    if (outer.Shape().IsNull() || inner.Shape().IsNull()) {
        throw Base::RuntimeError("Thread error: failed to create thread reduction cylinders");
    }

    // Builds the ring
    BRepAlgoAPI_Cut ring(outer.Shape(), inner.Shape());

    ring.Build();

    if (!ring.IsDone()) {
        throw Base::RuntimeError("Thread error: failed to create thread reduction volume");
    }

    // Removes the ring
    BRepAlgoAPI_Cut result(base.getShape(), ring.Shape());

    result.Build();

    if (!result.IsDone() || result.Shape().IsNull()) {
        throw Base::RuntimeError("Thread error: failed to reduce external thread base");
    }

    return Part::TopoShape(result.Shape());
}

#include <BRep_Tool.hxx>
#include <BRepAdaptor_Surface.hxx>
#include <BRepAdaptor_Curve.hxx>
#include <GeomAbs_SurfaceType.hxx>
#include <IntCurvesFace_ShapeIntersector.hxx>
#include <Standard_Failure.hxx>

gp_Dir ThreadUtils::getThreadAxisDir(const App::PropertyLinkSub& LateralFace)
{
    TopoDS_Face threadedFace = getSelectedFace(LateralFace);
    Handle(Geom_Surface) surf = BRep_Tool::Surface(threadedFace);

    FaceType faceType = getFaceType(threadedFace);

    if (faceType == FaceType::Cylinder) {
        Handle(Geom_CylindricalSurface) cyl = Handle(Geom_CylindricalSurface)::DownCast(surf);
        return cyl->Position().Direction();
    }
    else if (faceType == FaceType::Cone) {
        Handle(Geom_ConicalSurface) cone = Handle(Geom_ConicalSurface)::DownCast(surf);
        return cone->Position().Direction();
    }

    throw Base::RuntimeError(
        "Thread axis direction could not be calculated (Face is neither Cylinder nor Cone)."
    );
}

#include <BRepBuilderAPI_MakeVertex.hxx>
#include <App/PropertyGeo.h>  // App::PropertyPlacement
#include "BRepBuilderAPI_MakeFace.hxx"

static TopoDS_Shape getSelectedSubShape(const App::PropertyLinkSub& prop)
{
    App::DocumentObject* obj = prop.getValue();
    if (!obj) {
        Base::Console().message("getSelectedSubShape: obj is null\n");
        return TopoDS_Shape();
    }

    Base::Console().message("getSelectedSubShape: obj name = %s\n", obj->getNameInDocument());
    Base::Console().message("getSelectedSubShape: obj type = %s\n", obj->getTypeId().getName());

    const std::vector<std::string>& subs = prop.getSubValues();
    Base::Console().message("getSelectedSubShape: subs size = %zu\n", subs.size());

    if (obj->getTypeId().isDerivedFrom(Base::Type::fromName("Part::DatumPoint"))
        || obj->getTypeId().isDerivedFrom(Base::Type::fromName("Part::DatumLine"))
        || obj->getTypeId().isDerivedFrom(Base::Type::fromName("Part::DatumPlane"))
        || obj->getTypeId().isDerivedFrom(Base::Type::fromName("PartDesign::Datum"))
        || obj->getTypeId().isDerivedFrom(Base::Type::fromName("PartDesign::Point"))
        || obj->getTypeId().isDerivedFrom(Base::Type::fromName("PartDesign::Line"))
        || obj->getTypeId().isDerivedFrom(Base::Type::fromName("PartDesign::Plane"))) {
        Base::Console().message("getSelectedSubShape: it's a Datum object\n");

        App::Property* rawPlacement = obj->getPropertyByName("Placement");
        if (!rawPlacement
            || !rawPlacement->getTypeId().isDerivedFrom(App::PropertyPlacement::getClassTypeId())) {
            Base::Console().message("getSelectedSubShape: Datum has no usable 'Placement' property\n");
            return TopoDS_Shape();
        }

        auto* propPlacement = static_cast<App::PropertyPlacement*>(rawPlacement);
        Base::Placement trsf = propPlacement->getValue();
        Base::Vector3d pos = trsf.getPosition();

        if (obj->getTypeId().isDerivedFrom(Base::Type::fromName("Part::DatumPoint"))
            || obj->getTypeId().isDerivedFrom(Base::Type::fromName("PartDesign::Point"))) {
            BRepBuilderAPI_MakeVertex mkVertex(gp_Pnt(pos.x, pos.y, pos.z));
            return mkVertex.Vertex();
        }

        if (obj->getTypeId().isDerivedFrom(Base::Type::fromName("Part::DatumLine"))
            || obj->getTypeId().isDerivedFrom(Base::Type::fromName("PartDesign::Line"))) {
            Base::Rotation rot = trsf.getRotation();
            Base::Vector3d dir(0, 0, 1);
            rot.multVec(dir, dir);

            gp_Pnt pnt(pos.x, pos.y, pos.z);
            gp_Dir direction(dir.x, dir.y, dir.z);
            gp_Lin line(pnt, direction);

            BRepBuilderAPI_MakeEdge mkEdge(line, -1000.0, 1000.0);
            return mkEdge.Edge();
        }

        if (obj->getTypeId().isDerivedFrom(Base::Type::fromName("Part::DatumPlane"))
            || obj->getTypeId().isDerivedFrom(Base::Type::fromName("PartDesign::Plane"))) {
            Base::Rotation rot = trsf.getRotation();
            Base::Vector3d normal(0, 0, 1);
            rot.multVec(normal, normal);

            gp_Pnt pnt(pos.x, pos.y, pos.z);
            gp_Dir dirNormal(normal.x, normal.y, normal.z);
            gp_Pln plane(pnt, dirNormal);

            BRepBuilderAPI_MakeFace mkFace(plane, -1000.0, 1000.0, -1000.0, 1000.0);
            return mkFace.Face();
        }
    }

    auto* feature = dynamic_cast<Part::Feature*>(obj);
    if (!feature) {
        Base::Console().message(
            "getSelectedSubShape: not a Part::Feature and not a recognized Datum\n"
        );
        return TopoDS_Shape();
    }

    const Part::TopoShape& topoShape = feature->Shape.getShape();
    if (topoShape.isNull()) {
        Base::Console().message("getSelectedSubShape: topoShape is null\n");
        return TopoDS_Shape();
    }

    if (subs.empty() || subs.front().empty()) {
        Base::Console().message("getSelectedSubShape: returning full shape\n");
        return topoShape.getShape();
    }

    TopoDS_Shape subShape;
    try {
        Base::Console().message("getSelectedSubShape: getting subshape '%s'\n", subs.front().c_str());
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
        Base::Console().message("getSelectedSubShape: subShape is null\n");
        return TopoDS_Shape();
    }

    return subShape;
}

#include <GeomAPI_ExtremaCurveCurve.hxx>
#include <Geom_Line.hxx>
#include <GeomAdaptor_Curve.hxx>
#include <BRep_Tool.hxx>
#include <Precision.hxx>
#include <vector>

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

    gp_Pnt intersection = lineOrigin.Translated(gp_Vec(lineDir) * t);

    const double distanceToPlane = plane.Distance(intersection);

    return intersection;
}

gp_Pnt ThreadUtils::getThreadStartPoint(
    const App::PropertyLinkSub& lateralFace,
    const App::PropertyLinkSub& startPlane
)
{
    // gp_Pnt axisOrigin = getThreadAxisOrigin(lateralFace);
    gp_Vec zDir = getThreadZAxis(lateralFace);
    zDir.Normalize();
    gp_Pnt axisOrigin = getThreadStartPoint(lateralFace, gp_Dir(zDir));

    gp_Dir axisDir = getThreadAxisDir(lateralFace);
    gp_Ax1 threadAxis(axisOrigin, axisDir);
    gp_Lin axisLine(threadAxis);

    auto* obj = startPlane.getValue();  // TODO: change for getSelectedFace
    if (obj == nullptr) {
        Base::Console().message("exit 1\n");
        return axisOrigin;
    }

    const TopoDS_Shape& shape = getSelectedSubShape(startPlane);  // check if TopoDS_Face == TopoDS_Shape

    if (shape.IsNull()) {
        Base::Console().message("exit 2\n");
        return axisOrigin;
    }

    switch (shape.ShapeType()) {
        case TopAbs_VERTEX: {
            Base::Console().message("It's a vertex\n");
            gp_Pnt pt = BRep_Tool::Pnt(TopoDS::Vertex(shape));
            if (axisLine.Distance(pt) > Precision::Confusion() * 10) {
                throw Base::ValueError(
                    QT_TRANSLATE_NOOP("Exception", "Selected point does not lie on the thread axis.")
                );
            }
            Base::Console().message("mandando o novo ponto.");
            return pt;
        }

        case TopAbs_EDGE: {
            BRepAdaptor_Curve curveAdaptor(TopoDS::Edge(shape));
            Base::Console().message("Sou EDGE, estou comecando\n");

            std::vector<gp_Pnt> intersections = findLineCurveIntersections(axisLine, curveAdaptor);

            if (intersections.empty()) {
                throw Base::ValueError("Selected edge does not intersect the thread axis.");
            }

            // TODO: find minor intersection later
            return intersections.front();
        }

        case TopAbs_FACE: {
            Base::Console().message("It's a face!\n");
            TopoDS_Face face = TopoDS::Face(shape);
            BRepAdaptor_Surface surfAdaptor(face);

            if (surfAdaptor.GetType() == GeomAbs_Plane) {
                gp_Pln plane = surfAdaptor.Plane();

                Base::Console().message("Getting intersection\n");
                gp_Pnt intersection = getPlaneLineIntersection(plane, axisLine);
                Base::Console().message("intersection.Z(): %lf\n", intersection.Z());

                // TODO: verify if point is on the face if it is needed.
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
                gp_Pnt pnt = intersector.Pnt(i);
                double dist = pnt.Distance(axisOrigin);

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
