/***************************************************************************
 *   Copyright (c) 2011 Juergen Riegel <FreeCAD@juergen-riegel.net>        *
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
# include <Bnd_Box.hxx>
# include <gp_Dir.hxx>
# include <gp_Circ.hxx>
# include <gp_Pln.hxx>
# include <BRep_Builder.hxx>
# include <BRepAdaptor_Surface.hxx>
# include <BRepBndLib.hxx>
# include <BRepPrimAPI_MakePrism.hxx>
# include <BRepFeat_MakePrism.hxx>
# include <BRepBuilderAPI_Copy.hxx>
# include <BRepBuilderAPI_MakeFace.hxx>
# include <BRepBuilderAPI_MakeEdge.hxx>
# include <BRepBuilderAPI_MakeWire.hxx>
# include <BRepBuilderAPI_Transform.hxx>
# include <BRepPrimAPI_MakeRevol.hxx>
# include <Geom_Plane.hxx>
# include <Geom_Circle.hxx>
# include <Geom2d_Curve.hxx>
# include <TopoDS.hxx>
# include <TopoDS_Face.hxx>
# include <TopoDS_Wire.hxx>
# include <TopExp_Explorer.hxx>
# include <TopExp.hxx>
# include <BRepAlgoAPI_Cut.hxx>
# include <BRepAlgoAPI_Fuse.hxx>
# include <Standard_Version.hxx>
# include <QCoreApplication>
#endif


#include <Base/Placement.h>
#include <Base/Exception.h>
#include <Base/Tools.h>
#include <Base/FileInfo.h>
#include <App/Document.h>
#include <App/Application.h>
#include <Base/Reader.h>
#include <Mod/Part/App/TopoShape.h>

#include "json.hpp"
#include "FeatureHole.h"

namespace PartDesign {

/* TRANSLATOR PartDesign::Hole */

const char* Hole::DepthTypeEnums[]                   = { "Dimension", "ThroughAll", /*, "UpToFirst", */ NULL };
const char* Hole::ThreadTypeEnums[]                  = { "None", "ISOMetricProfile", "ISOMetricFineProfile", "UNC", "UNF", "UNEF", NULL};
const char* Hole::ClearanceMetricEnums[]             = { "Standard", "Close", "Wide", NULL};
const char* Hole::ClearanceUTSEnums[]                = { "Normal", "Close", "Loose", NULL };
const char* Hole::DrillPointEnums[]                  = { "Flat", "Angled", NULL};

/* "None" profile */

const char* Hole::HoleCutType_None_Enums[]           = { "None", "Counterbore", "Countersink", NULL };
const char* Hole::ThreadSize_None_Enums[]            = { "None", NULL };
const char* Hole::ThreadClass_None_Enums[]           = { "None", NULL };

/* Sources:
  http://www.engineeringtoolbox.com/metric-threads-d_777.html
  http://www.metalmart.com/tools/miscellaneous-guides/standard-drill-size/

*/

const Hole::ThreadDescription Hole::threadDescription[][171] =
{
    /* None */
    {
        { "---", 	6.0, 0.0, 0.0 },
    },

    /* ISO metric regular */
    /* ISO metric threaded core hole diameters according to ISO 2306 */
    // {name, thread diameter, thread pitch, core hole diameter}
    {
        { "M1", 	1.0,    0.25,   0.75 },
        { "M1.1", 	1.1,    0.25,   0.85 },
        { "M1.2", 	1.2,    0.25,   0.95 },
        { "M1.4", 	1.4,    0.30,   1.10 },
        { "M1.6", 	1.6,    0.35,   1.25 },
        { "M1.8", 	1.8,    0.35,   1.45 },
        { "M2", 	2.0,    0.40,   1.60 },
        { "M2.2", 	2.2,    0.45,   1.75 },
        { "M2.5", 	2.5,    0.45,   2.05 },
        { "M3", 	3.0,    0.50,   2.50 },
        { "M3.5", 	3.5,    0.60,   2.90 },
        { "M4", 	4.0,    0.70,   3.30 },
        { "M4.5", 	4.5,    0.75,   3.70 },
        { "M5", 	5.0,    0.80,   4.20 },
        { "M6", 	6.0,    1.00,   5.00 },
        { "M7", 	7.0,    1.00,   6.00 },
        { "M8", 	8.0,    1.25,   6.80 },
        { "M9", 	9.0,    1.25,   7.80 },
        { "M10", 	10.0,   1.50,   8.50 },
        { "M11", 	11.0,   1.50,   9.50 },
        { "M12", 	12.0,   1.75,   10.20 },
        { "M14", 	14.0,   2.00,   12.00 },
        { "M16", 	16.0,   2.00,   14.00 },
        { "M18", 	18.0,   2.50,   15.50 },
        { "M20", 	20.0,   2.50,   17.50 },
        { "M22", 	22.0,   2.50,   19.50 },
        { "M24", 	24.0,   3.00,   21.00 },
        { "M27", 	27.0,   3.00,   24.00 },
        { "M30", 	30.0,   3.50,   26.50 },
        { "M33", 	33.0,   3.50,   29.50 },
        { "M36", 	36.0,   4.00,   32.00 },
        { "M39", 	39.0,   4.00,   35.00 },
        { "M42", 	42.0,   4.50,   37.50 },
        { "M45", 	45.0,   4.50,   40.50 },
        { "M48", 	48.0,   5.00,   43.00 },
        { "M52", 	52.0,   5.00,   47.00 },
        { "M56", 	56.0,   5.50,   50.50 },
        { "M60", 	60.0,   5.50,   54.50 },
        { "M64", 	64.0,   6.00,   58.00 },
        { "M68", 	68.0,   6.00,   62.00 },
     },
    /* ISO metric fine (core hole entry is calculated exactly by diameter - pitch) */
    {
        { "M1x0.2",     1.0, 0.20,  0.80 },
        { "M1.1x0.2",   1.1, 0.20,  0.90 },
        { "M1.2x0.2",   1.2, 0.20,  1.00 },
        { "M1.4x0.2",   1.4, 0.20,  1.20 },
        { "M1.6x0.2",   1.6, 0.20,  1.40 },
        { "M1.8x0.2",   1.8, 0.20,  1.60 },
        { "M2x0.25",    2.0, 0.25,  1.75 },
        { "M2.2x0.25",  2.2, 0.25,  1.95 },
        { "M2.5x0.35",  2.5, 0.35,  2.15 },
        { "M3x0.35",    3.0, 0.35,  2.65 },
        { "M3.5x0.35",  3.5, 0.35,  3.15 },
        { "M4x0.5",     4.0, 0.50,  3.50 },
        { "M4.5x0.5",   4.5, 0.50,  4.00 },
        { "M5x0.5",     5.0, 0.50,  4.50 },
        { "M5.5x0.5",   5.5, 0.50,  5.00 },
        { "M6x0.75",    6.0, 0.75,  5.25 },
        { "M7x0.75",    7.0, 0.75,  6.25 },
        { "M8x0.75",    8.0, 0.75,  7.25 },
        { "M8x1.0",     8.0, 1.00,  7.00 },
        { "M9x0.75",    9.0, 0.75,  8.25 },
        { "M9x1.0",     9.0, 1.00,  8.00 },
        { "M10x0.75",  10.0, 0.75,  9.25 },
        { "M10x1.0",   10.0, 1.00,  9.00 },
        { "M10x1.25",  10.0, 1.25,  8.75 },
        { "M11x0.75",  11.0, 0.75, 10.25 },
        { "M11x1.0",   11.0, 1.00, 10.00 },
        { "M12x1.0",   12.0, 1.00, 11.00 },
        { "M12x1.25",  12.0, 1.25, 10.75 },
        { "M12x1.5",   12.0, 1.50, 10.50 },
        { "M14x1.0",   14.0, 1.00, 13.00 },
        { "M14x1.25",  14.0, 1.25, 12.75 },
        { "M14x1.5",   14.0, 1.50, 12.50 },
        { "M15x1.0",   15.0, 1.00, 14.00 },
        { "M15x1.5",   15.0, 1.50, 13.50 },
        { "M16x1.0",   16.0, 1.00, 15.00 },
        { "M16x1.5",   16.0, 1.50, 14.50 },
        { "M17x1.0",   17.0, 1.00, 16.00 },
        { "M17x1.5",   17.0, 1.50, 15.50 },
        { "M18x1.0",   18.0, 1.00, 17.00 },
        { "M18x1.5",   18.0, 1.50, 16.50 },
        { "M18x2.0",   18.0, 2.00, 16.00 },
        { "M20x1.0",   20.0, 1.00, 19.00 },
        { "M20x1.5",   20.0, 1.50, 18.50 },
        { "M20x2.0",   20.0, 2.00, 18.00 },
        { "M22x1.0",   22.0, 1.00, 21.00 },
        { "M22x1.5",   22.0, 1.50, 20.50 },
        { "M22x2.0",   22.0, 2.00, 20.00 },
        { "M24x1.0",   24.0, 1.00, 23.00 },
        { "M24x1.5",   24.0, 1.50, 22.50 },
        { "M24x2.0",   24.0, 2.00, 22.00 },
        { "M25x1.0",   25.0, 1.00, 24.00 },
        { "M25x1.5",   25.0, 1.50, 23.50 },
        { "M25x2.0",   25.0, 2.00, 23.00 },
        { "M27x1.0",   27.0, 1.00, 26.00 },
        { "M27x1.5",   27.0, 1.50, 25.50 },
        { "M27x2.0",   27.0, 2.00, 25.00 },
        { "M28x1.0",   28.0, 1.00, 27.00 },
        { "M28x1.5",   28.0, 1.50, 26.50 },
        { "M28x2.0",   28.0, 2.00, 26.00 },
        { "M30x1.0",   30.0, 1.00, 29.00 },
        { "M30x1.5",   30.0, 1.50, 28.50 },
        { "M30x2.0",   30.0, 2.00, 28.00 },
        { "M30x3.0",   30.0, 3.00, 27.00 },
        { "M32x1.5",   32.0, 1.50, 30.50 },
        { "M32x2.0",   32.0, 2.00, 30.00 },
        { "M33x1.5",   33.0, 1.50, 31.50 },
        { "M33x2.0",   33.0, 2.00, 31.00 },
        { "M33x3.0",   33.0, 3.00, 30.00 },
        { "M35x1.5",   35.0, 1.50, 33.50 },
        { "M35x2.0",   35.0, 2.00, 33.00 },
        { "M36x1.5",   36.0, 1.50, 34.50 },
        { "M36x2.0",   36.0, 2.00, 34.00 },
        { "M36x3.0",   36.0, 3.00, 33.00 },
        { "M39x1.5",   39.0, 1.50, 37.50 },
        { "M39x2.0",   39.0, 2.00, 37.00 },
        { "M39x3.0",   39.0, 3.00, 36.00 },
        { "M40x1.5",   40.0, 1.50, 38.50 },
        { "M40x2.0",   40.0, 2.00, 38.00 },
        { "M40x3.0",   40.0, 3.00, 37.00 },
        { "M42x1.5",   42.0, 1.50, 40.50 },
        { "M42x2.0",   42.0, 2.00, 40.00 },
        { "M42x3.0",   42.0, 3.00, 39.00 },
        { "M42x4.0",   42.0, 4.00, 38.00 },
        { "M45x1.5",   45.0, 1.50, 43.50 },
        { "M45x2.0",   45.0, 2.00, 43.00 },
        { "M45x3.0",   45.0, 3.00, 42.00 },
        { "M45x4.0",   45.0, 4.00, 41.00 },
        { "M48x1.5",   48.0, 1.50, 46.50 },
        { "M48x2.0",   48.0, 2.00, 46.00 },
        { "M48x3.0",   48.0, 3.00, 45.00 },
        { "M48x4.0",   48.0, 4.00, 44.00 },
        { "M50x1.5",   50.0, 1.50, 48.50 },
        { "M50x2.0",   50.0, 2.00, 48.00 },
        { "M50x3.0",   50.0, 3.00, 47.00 },
        { "M52x1.5",   52.0, 1.50, 50.50 },
        { "M52x2.0",   52.0, 2.00, 50.00 },
        { "M52x3.0",   52.0, 3.00, 49.00 },
        { "M52x4.0",   52.0, 4.00, 48.00 },
        { "M55x1.5",   55.0, 1.50, 53.50 },
        { "M55x2.0",   55.0, 2.00, 53.00 },
        { "M55x3.0",   55.0, 3.00, 52.00 },
        { "M55x4.0",   55.0, 4.00, 51.00 },
        { "M56x1.5",   56.0, 1.50, 54.50 },
        { "M56x2.0",   56.0, 2.00, 54.00 },
        { "M56x3.0",   56.0, 3.00, 53.00 },
        { "M56x4.0",   56.0, 4.00, 52.00 },
        { "M58x1.5",   58.0, 1.50, 56.50 },
        { "M58x2.0",   58.0, 2.00, 56.00 },
        { "M58x3.0",   58.0, 3.00, 55.00 },
        { "M58x4.0",   58.0, 4.00, 54.00 },
        { "M60x1.5",   60.0, 1.50, 58.50 },
        { "M60x2.0",   60.0, 2.00, 58.00 },
        { "M60x3.0",   60.0, 3.00, 57.00 },
        { "M60x4.0",   60.0, 4.00, 56.00 },
        { "M62x1.5",   62.0, 1.50, 60.50 },
        { "M62x2.0",   62.0, 2.00, 60.00 },
        { "M62x3.0",   62.0, 3.00, 59.00 },
        { "M62x4.0",   62.0, 4.00, 58.00 },
        { "M64x1.5",   64.0, 1.50, 62.50 },
        { "M64x2.0",   64.0, 2.00, 62.00 },
        { "M64x3.0",   64.0, 3.00, 61.00 },
        { "M64x4.0",   64.0, 4.00, 60.00 },
        { "M65x1.5",   65.0, 1.50, 63.50 },
        { "M65x2.0",   65.0, 2.00, 63.00 },
        { "M65x3.0",   65.0, 3.00, 62.00 },
        { "M65x4.0",   65.0, 4.00, 61.00 },
        { "M68x1.5",   68.0, 1.50, 66.50 },
        { "M68x2.0",   68.0, 2.00, 66.00 },
        { "M68x3.0",   68.0, 3.00, 65.00 },
        { "M68x4.0",   68.0, 4.00, 64.00 },
        { "M70x1.5",   70.0, 1.50, 68.50 },
        { "M70x2.0",   70.0, 2.00, 68.00 },
        { "M70x3.0",   70.0, 3.00, 67.00 },
        { "M70x4.0",   70.0, 4.00, 66.00 },
        { "M70x6.0",   70.0, 6.00, 64.00 },
        { "M72x1.5",   72.0, 1.50, 70.50 },
        { "M72x2.0",   72.0, 2.00, 70.00 },
        { "M72x3.0",   72.0, 3.00, 69.00 },
        { "M72x4.0",   72.0, 4.00, 68.00 },
        { "M72x6.0",   72.0, 6.00, 66.00 },
        { "M75x1.5",   75.0, 1.50, 73.50 },
        { "M75x2.0",   75.0, 2.00, 73.00 },
        { "M75x3.0",   75.0, 3.00, 72.00 },
        { "M75x4.0",   75.0, 4.00, 71.00 },
        { "M75x6.0",   75.0, 6.00, 69.00 },
        { "M76x1.5",   76.0, 1.50, 74.50 },
        { "M76x2.0",   76.0, 2.00, 74.00 },
        { "M76x3.0",   76.0, 3.00, 73.00 },
        { "M76x4.0",   76.0, 4.00, 72.00 },
        { "M76x6.0",   76.0, 6.00, 70.00 },
        { "M80x1.5",   80.0, 1.50, 78.50 },
        { "M80x2.0",   80.0, 2.00, 78.00 },
        { "M80x3.0",   80.0, 3.00, 77.00 },
        { "M80x4.0",   80.0, 4.00, 76.00 },
        { "M80x6.0",   80.0, 6.00, 74.00 },
        { "M85x2.0",   85.0, 2.00, 83.00 },
        { "M85x3.0",   85.0, 3.00, 82.00 },
        { "M85x4.0",   85.0, 4.00, 81.00 },
        { "M85x6.0",   85.0, 6.00, 79.00 },
        { "M90x2.0",   90.0, 2.00, 88.00 },
        { "M90x3.0",   90.0, 3.00, 87.00 },
        { "M90x4.0",   90.0, 4.00, 86.00 },
        { "M90x6.0",   90.0, 6.00, 84.00 },
        { "M95x2.0",   95.0, 2.00, 93.00 },
        { "M95x3.0",   95.0, 3.00, 92.00 },
        { "M95x4.0",   95.0, 4.00, 91.00 },
        { "M95x6.0",   95.0, 6.00, 89.00 },
        { "M100x2.0", 100.0, 2.00, 98.00 },
        { "M100x3.0", 100.0, 3.00, 97.00 },
        { "M100x4.0", 100.0, 4.00, 96.00 },
        { "M100x6.0", 100.0, 6.00, 94.00 }
     },
    /* UNC */
    {
        { "#1",         1.854,  0.397,     1.50 },
        { "#2",         2.184,  0.454,     1.85 },
        { "#3",         2.515,  0.529,     2.10 },
        { "#4",         2.845,  0.635,     2.35 },
        { "#5",         3.175,  0.635,     2.65 },
        { "#6",         3.505,  0.794,     2.85 },
        { "#8",         4.166,  0.794,     3.50 },
        { "#10",        4.826,  1.058,     3.90 },
        { "#12",        5.486,  1.058,     4.50 },
        { "1/4",        6.350,  1.270,     5.10 },
        { "5/16",       7.938,  1.411,     6.60 },
        { "3/8",        9.525,  1.588,     8.00 },
        { "7/16",       11.113, 1.814,     9.40 },
        { "1/2",        12.700, 1.954,    10.80 },
        { "9/16",       14.288, 2.117,    12.20 },
        { "5/8",        15.875, 2.309,    13.50 },
        { "3/4",        19.050, 2.540,    16.50 },
        { "7/8",        22.225, 2.822,    19.50 },
        { "1",          25.400, 3.175,    22.25 },
        { "1 1/8",      28.575, 3.628,    25.00 },
        { "1 1/4",      31.750, 3.628,    28.00 },
        { "1 3/8",      34.925, 4.233,    30.75 },
        { "1 1/2",      38.100, 4.233,    34.00 },
        { "1 3/4",      44.450, 5.080,    39.50 },
        { "2",          50.800, 5.644,    45.00 },
        { "2 1/4",      57.150, 5.644,    51.50 },
        { "2 1/2",      63.500, 6.350,    57.00 },
        { "2 3/4",      69.850, 6.350,    63.50 },
        { "3",          76.200, 6.350,    70.00 },
        { "3 1/4",      82.550, 6.350,    76.50 },
        { "3 1/2",      88.900, 6.350,    83.00 },
        { "3 3/4",      95.250, 6.350,    89.00 },
        { "4",          101.600, 6.350,   95.50 },
    },
    /* UNF */
    {
        { "#0",         1.524,  0.317,    1.20 },
        { "#1",         1.854,  0.353,    1.55 },
        { "#2",         2.184,  0.397,    1.85 },
        { "#3",         2.515,  0.454,    2.10 },
        { "#4",         2.845,  0.529,    2.40 },
        { "#5",         3.175,  0.577,    2.70 },
        { "#6",         3.505,  0.635,    2.95 },
        { "#8",         4.166,  0.706,    3.50 },
        { "#10",        4.826,  0.794,    4.10 },
        { "#12",        5.486,  0.907,    4.70 },
        { "1/4",        6.350,  0.907,    5.50 },
        { "5/16",       7.938,  1.058,    6.90 },
        { "3/8",        9.525,  1.058,    8.50 },
        { "7/16",       11.113, 1.270,    9.90 },
        { "1/2",        12.700, 1.270,    11.50 },
        { "9/16",       14.288, 1.411,    12.90 },
        { "5/8",        15.875, 1.411,    14.50 },
        { "3/4",        19.050, 1.588,    17.50 },
        { "7/8",        22.225, 1.814,    20.40 },
        { "1",          25.400, 2.117,    23.25 },
        { "1 1/8",      28.575, 2.117,    26.50 },
        { "1 1/4",      31.750, 2.117,    29.50 },
        { "1 3/8",      34.925, 2.117,    32.75 },
        { "1 1/2",      38.100, 2.117,    36.00 },
    }    ,
    /* UNEF */
    {
        { "#12",        5.486,  0.794,    4.80 },
        { "1/4",        6.350,  0.794,    5.70 },
        { "5/16",       7.938,  0.794,    7.25 },
        { "3/8",        9.525,  0.794,    8.85 },
        { "7/16",       11.113, 0.907,   10.35 },
        { "1/2",        12.700, 0.907,   11.80 },
        { "9/16",       14.288, 1.058,   13.40 },
        { "5/8",        15.875, 1.058,   15.00 },
        { "11/16",      17.462, 1.058,   16.60 },
        { "3/4",        19.050, 1.270,   18.00 },
        { "13/16",      20.638, 1.270,   19.60 },
        { "7/8",        22.225, 1.270,   21.15 },
        { "15/16",      23.812, 1.270,   22.70 },
        { "1",          25.400, 1.270,   24.30 },
        { "1 1/16",     26.988, 1.411,   25.80 },
        { "1 1/8",      28.575, 1.411,   27.35 },
        { "1 1/4",      31.750, 1.411,   30.55 },
        { "1 5/16",     33.338, 1.411,   32.10 },
        { "1 3/8",      34.925, 1.411,   33.70 },
        { "1 7/16",     36.512, 1.411,   35.30 },
        { "1 1/2",      38.100, 1.411,   36.90 },
        { "1 9/16",     39.688, 1.411,   38.55 },
        { "1 5/8",      41.275, 1.411,   40.10 },
        { "1 11/16",    42.862, 1.411,   41.60 },
    }

};

const double Hole::metricHoleDiameters[36][4] =
{
    /* ISO metric clearance hole diameters according to ISO 273 */
    // {screw diameter, close, standard, coarse}
        { 1.0,      1.1,    1.2,    1.3},
        { 1.2,      1.3,    1.4,    1.5},
        { 1.4,      1.5,    1.6,    1.8},
        { 1.6,      1.7,    1.8,    2.0},
        { 1.8,      2.0,    2.1,    2.2},
        { 2.0,      2.2,    2.4,    2.6},
        { 2.5,      2.7,    2.9,    3.1},
        { 3.0,      3.2,    3.4,    3.6},
        { 3.5,      3.7,    3.9,    4.2},
        { 4.0,      4.3,    4.5,    4.8},
        { 4.5,      4.8,    5.0,    5.3},
        { 5.0,      5.3,    5.5,    5.8},
        { 6.0,      6.4,    6.6,    7.0},
        { 7.0,      7.4,    7.6,    8.0},
        { 8.0,      8.4,    9.0,    10.0},
        // 9.0 undefined
        { 10.0,     10.5,   11.0,   12.0},
        // 11.0 undefined
        { 12.0,     13.0,   13.5,   14.5},
        { 14.0,     15.0,   15.5,   16.5},
        { 16.0,     17.0,  	17.5,   18.5},
        { 18.0,     19.0,  	20.0,   21.0},
        { 20.0,     21.0,  	22.0,   24.0},
        { 22.0,     23.0,  	24.0,   26.0},
        { 24.0,     25.0,  	26.0,   28.0},
        { 27.0,     28.0,  	30.0,   32.0},
        { 30.0,     31.0,  	33.0,   35.0},
        { 33.0,     34.0,  	36.0,   38.0},
        { 36.0,     37.0,  	39.0,   42.0},
        { 39.0,     40.0,  	42.0,   45.0},
        { 42.0,     43.0,  	45.0,   48.0},
        { 45.0,     46.0,  	48.0,   52.0},
        { 48.0,     50.0,  	52.0,   56.0},
        { 52.0,     54.0,  	56.0,   62.0},
        { 56.0,     58.0,  	62.0,   66.0},
        { 60.0,     62.0,  	66.0,   70.0},
        { 64.0,     66.0,  	70.0,   74.0},
        { 68.0,     70.0,  	77.0,   78.0}
};

const Hole::UTSClearanceDefinition Hole::UTSHoleDiameters[22] =
{
    /* UTS clearance hole diameters according to ASME B18.2.8 */
    // for information: the norm defines a drill bit number (that is in turn standardized in another ASME norm).
    // as result the norm defines a minimal clearance which is the diameter of that drill bit.
    // we use here this minimal clearance as the theoretical exact hole diameter as this is also done in the ISO norm.
    // {screw class, close, normal, loose}
        { "#0",     1.7,  1.9,  2.4 },
        { "#1",     2.1,  2.3,  2.6 },
        { "#2",     2.4,  2.6,  2.9 },
        { "#3",     2.7,  2.9,  3.3 },
        { "#4",     3.0,  3.3,  3.7 },
        { "#5",     3.6,  4.0,  4.4 },
        { "#6",     3.9,  4.3,  4.7 },
        { "#8",     4.6,  5.0,  5.4 },
        { "#10",    5.2,  5.6,  6.0 },
        // "#12" not defined
        { "1/4",    6.8,  7.1,  7.5 },
        { "5/16",   8.3,  8.7,  9.1 },
        { "3/8",    9.9, 10.3, 10.7 },
        { "7/16",  11.5, 11.9, 12.3 },
        { "1/2",   13.5, 14.3, 15.5 },
        // "9/16" not defined
        { "5/8",   16.7, 17.5, 18.6 },
        { "3/4",   19.8, 20.6, 23.0 },
        { "7/8",   23.0, 23.8, 26.2 },
        { "1",     26.2, 27.8, 29.4 },
        { "1 1/8", 29.4, 31.0, 33.3 },
        { "1 1/4", 32.5, 34.1, 36.5 },
        { "1 3/8", 36.5, 38.1, 40.9 },
        { "1 1/2", 39.7, 41.3, 44.0 }
};

/* ISO coarse metric enums */
std::vector<std::string> Hole::HoleCutType_ISOmetric_Enums  = { "None", "Counterbore", "Countersink", "Cheesehead (deprecated)", "Countersink socket screw (deprecated)", "Cap screw (deprecated)" };
const char* Hole::ThreadSize_ISOmetric_Enums[]   = { "M1",   "M1.1", "M1.2", "M1.4", "M1.6",
                                                     "M1.8", "M2",   "M2.2", "M2.5", "M3",
                                                     "M3.5", "M4",   "M4.5", "M5",   "M6",
                                                     "M7",   "M8",   "M9",   "M10",  "M11",
                                                     "M12",  "M14",  "M16",  "M18",  "M20",
                                                     "M22",  "M24",  "M27",  "M30",  "M33",
                                                     "M36",  "M39",  "M42",  "M45",  "M48",
                                                     "M52",  "M56",  "M60",  "M64",  "M68",  NULL };
const char* Hole::ThreadClass_ISOmetric_Enums[]  = { "4G", "4H", "5G", "5H", "6G", "6H", "7G", "7H","8G", "8H", NULL };

std::vector<std::string> Hole::HoleCutType_ISOmetricfine_Enums  = { "None", "Counterbore", "Countersink", "Cheesehead (deprecated)", "Countersink socket screw (deprecated)", "Cap screw (deprecated)" };
const char* Hole::ThreadSize_ISOmetricfine_Enums[]   = {
    "M1x0.2",      "M1.1x0.2",    "M1.2x0.2",    "M1.4x0.2",
    "M1.6x0.2",    "M1.8x0.2",    "M2x0.25",     "M2.2x0.25",
    "M2.5x0.35",   "M3x0.35",     "M3.5x0.35",
    "M4x0.5",      "M4.5x0.5",    "M5x0.5",      "M5.5x0.5",
    "M6x0.75",     "M7x0.75",     "M8x0.75",     "M8x1.0",
    "M9x0.75",     "M9x1.0",      "M10x0.75",    "M10x1.0",
    "M10x1.25",    "M11x0.75",    "M11x1.0",     "M12x1.0",
    "M12x1.25",    "M12x1.5",     "M14x1.0",     "M14x1.25",
    "M14x1.5",     "M15x1.0",     "M15x1.5",     "M16x1.0",
    "M16x1.5",     "M17x1.0",     "M17x1.5",     "M18x1.0",
    "M18x1.5",     "M18x2.0",     "M20x1.0",     "M20x1.5",
    "M20x2.0",     "M22x1.0",     "M22x1.5",     "M22x2.0",
    "M24x1.0",     "M24x1.5",     "M24x2.0",     "M25x1.0",
    "M25x1.5",     "M25x2.0",     "M27x1.0",     "M27x1.5",
    "M27x2.0",     "M28x1.0",     "M28x1.5",     "M28x2.0",
    "M30x1.0",     "M30x1.5",     "M30x2.0",     "M30x3.0",
    "M32x1.5",     "M32x2.0",     "M33x1.5",     "M33x2.0",
    "M33x3.0",     "M35x1.5",     "M35x2.0",     "M36x1.5",
    "M36x2.0",     "M36x3.0",     "M39x1.5",     "M39x2.0",
    "M39x3.0",     "M40x1.5",     "M40x2.0",     "M40x3.0",
    "M42x1.5",     "M42x2.0",     "M42x3.0",     "M42x4.0",
    "M45x1.5",     "M45x2.0",     "M45x3.0",     "M45x4.0",
    "M48x1.5",     "M48x2.0",     "M48x3.0",     "M48x4.0",
    "M50x1.5",     "M50x2.0",     "M50x3.0",     "M52x1.5",
    "M52x2.0",     "M52x3.0",     "M52x4.0",     "M55x1.5",
    "M55x2.0",     "M55x3.0",     "M55x4.0",     "M56x1.5",
    "M56x2.0",     "M56x3.0",     "M56x4.0",     "M58x1.5",
    "M58x2.0",     "M58x3.0",     "M58x4.0",     "M60x1.5",
    "M60x2.0",     "M60x3.0",     "M60x4.0",     "M62x1.5",
    "M62x2.0",     "M62x3.0",     "M62x4.0",     "M64x1.5",
    "M64x2.0",     "M64x3.0",     "M64x4.0",     "M65x1.5",
    "M65x2.0",     "M65x3.0",     "M65x4.0",     "M68x1.5",
    "M68x2.0",     "M68x3.0",     "M68x4.0",     "M70x1.5",
    "M70x2.0",     "M70x3.0",     "M70x4.0",     "M70x6.0",
    "M72x1.5",     "M72x2.0",     "M72x3.0",     "M72x4.0",
    "M72x6.0",     "M75x1.5",     "M75x2.0",     "M75x3.0",
    "M75x4.0",     "M75x6.0",     "M76x1.5",     "M76x2.0",
    "M76x3.0",     "M76x4.0",     "M76x6.0",     "M80x1.5",
    "M80x2.0",     "M80x3.0",     "M80x4.0",     "M80x6.0",
    "M85x2.0",     "M85x3.0",     "M85x4.0",     "M85x6.0",
    "M90x2.0",     "M90x3.0",     "M90x4.0",     "M90x6.0",
    "M95x2.0",     "M95x3.0",     "M95x4.0",     "M95x6.0",
    "M100x2.0",    "M100x3.0",    "M100x4.0",    "M100x6.0", NULL };
const char* Hole::ThreadClass_ISOmetricfine_Enums[]  = { "4G", "4H", "5G", "5H", "6G", "6H", "7G", "7H","8G", "8H", NULL };

/* Details from https://en.wikipedia.org/wiki/Unified_Thread_Standard */

/* UTS coarse */
const char* Hole::HoleCutType_UNC_Enums[]  = { "None", "Counterbore", "Countersink", NULL};
const char* Hole::ThreadSize_UNC_Enums[]   = { "#1", "#2", "#3", "#4", "#5", "#6",
                                               "#8",  "#10", "#12",
                                               "1/4", "5/16", "3/8", "7/16", "1/2", "9/16",
                                               "5/8", "3/4", "7/8", "1", "1 1/8", "1 1/4",
                                               "1 3/8", "1 1/2", "1 3/4", "2", "2 1/4",
                                               "2 1/2", "2 3/4", "3", "3 1/4", "3 1/2",
                                               "3 3/4", "4", NULL };
const char* Hole::ThreadClass_UNC_Enums[]  = { "1B", "2B", "3B", NULL };

/* UTS fine */
const char* Hole::HoleCutType_UNF_Enums[]  = { "None", "Counterbore", "Countersink", NULL};
const char* Hole::ThreadSize_UNF_Enums[]   = { "#0", "#1", "#2", "#3", "#4", "#5", "#6",
                                               "#8", "#10", "#12",
                                               "1/4", "5/16", "3/8", "7/16", "1/2", "9/16",
                                               "5/8", "3/4", "7/8", "1", "1 1/8", "1 1/4",
                                               "1 3/8", "1 1/2", NULL };
const char* Hole::ThreadClass_UNF_Enums[]  = { "1B", "2B", "3B", NULL };

/* UTS extrafine */
const char* Hole::HoleCutType_UNEF_Enums[] = { "None", "Counterbore", "Countersink", NULL};
const char* Hole::ThreadSize_UNEF_Enums[]  = { "#12", "1/4", "5/16", "3/8", "7/16", "1/2",
                                               "9/16", "5/8", "11/16", "3/4", "13/16", "7/8",
                                               "15/16", "1", "1 1/16", "1 1/8", "1 1/4",
                                               "1 5/16", "1 3/8", "1 7/16", "1 1/2", "1 9/16",
                                               "1 5/8", "1 11/16", NULL };
const char* Hole::ThreadClass_UNEF_Enums[] = { "1B", "2B", "3B", NULL };

const char* Hole::ThreadDirectionEnums[]  = { "Right", "Left", NULL};

PROPERTY_SOURCE(PartDesign::Hole, PartDesign::ProfileBased)

Hole::Hole()
{
    addSubType = FeatureAddSub::Subtractive;

    readCutDefinitions();

    ADD_PROPERTY_TYPE(Threaded, (false), "Hole", App::Prop_None, "Threaded");

    ADD_PROPERTY_TYPE(ModelActualThread, (false), "Hole", App::Prop_None, "Model actual thread");
    ADD_PROPERTY_TYPE(ThreadPitch, (0.0), "Hole", App::Prop_None, "Thread pitch");
    ADD_PROPERTY_TYPE(ThreadAngle, (0.0), "Hole", App::Prop_None, "Thread angle");
    ADD_PROPERTY_TYPE(ThreadCutOffInner, (0.0), "Hole", App::Prop_None, "Thread CutOff Inner");
    ADD_PROPERTY_TYPE(ThreadCutOffOuter, (0.0), "Hole", App::Prop_None, "Thread CutOff Outer");

    ADD_PROPERTY_TYPE(ThreadType, (0L), "Hole", App::Prop_None, "Thread type");
    ThreadType.setEnums(ThreadTypeEnums);

    ADD_PROPERTY_TYPE(ThreadSize, (0L), "Hole", App::Prop_None, "Thread size");
    ThreadSize.setEnums(ThreadSize_None_Enums);

    ADD_PROPERTY_TYPE(ThreadClass, (0L), "Hole", App::Prop_None, "Thread class");
    ThreadClass.setEnums(ThreadClass_None_Enums);

    ADD_PROPERTY_TYPE(ThreadFit, (0L), "Hole", App::Prop_None, "Clearance hole fit");
    ThreadFit.setEnums(ClearanceMetricEnums);

    ADD_PROPERTY_TYPE(Diameter, (6.0), "Hole", App::Prop_None, "Diameter");

    ADD_PROPERTY_TYPE(ThreadDirection, (0L), "Hole", App::Prop_None, "Thread direction");
    ThreadDirection.setEnums(ThreadDirectionEnums);
    ThreadDirection.setReadOnly(true);

    ADD_PROPERTY_TYPE(HoleCutType, (0L), "Hole", App::Prop_None, "Head cut type");
    HoleCutType.setEnums(HoleCutType_None_Enums);

    ADD_PROPERTY_TYPE(HoleCutCustomValues, (false), "Hole", App::Prop_None, "Custom cut values");

    ADD_PROPERTY_TYPE(HoleCutDiameter, (0.0), "Hole", App::Prop_None, "Head cut diameter");

    ADD_PROPERTY_TYPE(HoleCutDepth, (0.0), "Hole", App::Prop_None, "Head cut deth");

    ADD_PROPERTY_TYPE(HoleCutCountersinkAngle, (90.0), "Hole", App::Prop_None, "Head cut countersink angle");

    ADD_PROPERTY_TYPE(DepthType, (0L), "Hole", App::Prop_None, "Type");
    DepthType.setEnums(DepthTypeEnums);

    ADD_PROPERTY_TYPE(Depth, (25.0), "Hole", App::Prop_None, "Length");

    ADD_PROPERTY_TYPE(DrillPoint, (1L), "Hole", App::Prop_None, "Drill point type");
    DrillPoint.setEnums(DrillPointEnums);

    ADD_PROPERTY_TYPE(DrillPointAngle, (118.0), "Hole", App::Prop_None, "Drill point angle");
    ADD_PROPERTY_TYPE(DrillForDepth, ((long)0), "Hole", App::Prop_None,
        "The size of the drill point will be taken into\n account for the depth of blind holes");

    ADD_PROPERTY_TYPE(Tapered, (false),"Hole",  App::Prop_None, "Tapered");

    ADD_PROPERTY_TYPE(TaperedAngle, (90.0), "Hole", App::Prop_None, "Tapered angle");
}

void Hole::updateHoleCutParams()
{
    std::string holeCutTypeStr = HoleCutType.getValueAsString();

    // there is no cut, thus return
    if (holeCutTypeStr == "None")
        return;

    if (ThreadType.getValue() < 0) {
        throw Base::IndexError("Thread type out of range");
        return;
    }

    // get diameter and size
    double diameterVal = Diameter.getValue();

    // handle thread types
    std::string threadTypeStr = ThreadType.getValueAsString();
    if (threadTypeStr == "ISOMetricProfile" || threadTypeStr == "ISOMetricFineProfile") {
        if (ThreadSize.getValue() < 0) {
            throw Base::IndexError("Thread size out of range");
            return;
        }

        std::string threadSizeStr = ThreadSize.getValueAsString();

        // we don't update for these settings but we need to set a value for new holes
        // furthermore we must assure the hole cut diameter is not <= the hole diameter
        // if we have a cut but the values are zero, we assume it is a new hole
        // we take in this case the values from the norm ISO 4762 or ISO 10642
        if (holeCutTypeStr == "Counterbore") {
            // read ISO 4762 values
            const CutDimensionSet& counter = find_cutDimensionSet(threadTypeStr, "ISO 4762");
            const CounterBoreDimension& dimen = counter.get_bore(threadSizeStr);
            if (HoleCutDiameter.getValue() == 0.0 || HoleCutDiameter.getValue() <= diameterVal) {
                // there is no norm defining counterbores for all sizes, thus we need to use the
                // same fallback as for the case HoleCutTypeMap.count(key)
                if (dimen.diameter != 0.0) {
                    HoleCutDiameter.setValue(dimen.diameter);
                    HoleCutDepth.setValue(dimen.depth);
                }
                else {
                    // valid values for visual feedback
                    HoleCutDiameter.setValue(Diameter.getValue() + 0.1);
                    HoleCutDepth.setValue(0.1);
                }
            }
            if (HoleCutDepth.getValue() == 0.0)
                HoleCutDepth.setValue(dimen.depth);
            HoleCutDiameter.setReadOnly(false);
            HoleCutDepth.setReadOnly(false);
            HoleCutCountersinkAngle.setReadOnly(true);
        }
        else if (holeCutTypeStr == "Countersink") {
            // read ISO 10642 values
            const CutDimensionSet& counter = find_cutDimensionSet(threadTypeStr, "ISO 10642");
            if (HoleCutDiameter.getValue() == 0.0 || HoleCutDiameter.getValue() <= diameterVal) {
                const CounterSinkDimension& dimen = counter.get_sink(threadSizeStr);
                if (dimen.diameter != 0.0) {
                    HoleCutDiameter.setValue(dimen.diameter);
                } 
                else {
                    HoleCutDiameter.setValue(Diameter.getValue() + 0.1);
                }
                HoleCutCountersinkAngle.setValue(counter.angle);
            }
            if (HoleCutCountersinkAngle.getValue() == 0.0) {
                HoleCutCountersinkAngle.setValue(counter.angle);
            }
            HoleCutDiameter.setReadOnly(false);
            HoleCutDepth.setReadOnly(false);
            HoleCutCountersinkAngle.setReadOnly(false);
        }

        // cut definition
        CutDimensionKey key { threadTypeStr, holeCutTypeStr };
        if (HoleCutTypeMap.count(key)) {
            const CutDimensionSet &counter = find_cutDimensionSet(key);
            if (counter.cut_type == CutDimensionSet::Counterbore) {
                // disable HoleCutCountersinkAngle and reset it to ISO's default
                HoleCutCountersinkAngle.setValue(90.0);
                HoleCutCountersinkAngle.setReadOnly(true);
                const CounterBoreDimension &dimen = counter.get_bore(threadSizeStr);
                if (dimen.thread == "None") {
                    // valid values for visual feedback
                    HoleCutDiameter.setValue(Diameter.getValue() + 0.1);
                    HoleCutDepth.setValue(0.1);
                    // we force custom values since there are no normed ones
                    HoleCutCustomValues.setReadOnly(true);
                    // important to set only if not already true, to avoid loop call of updateHoleCutParams()
                    if (!HoleCutCustomValues.getValue()) {
                        HoleCutCustomValues.setValue(true);
                        HoleCutDiameter.setReadOnly(false);
                        HoleCutDepth.setReadOnly(false);
                    }
                } else {
                    // set normed values if not overwritten or if previously there
                    // were no normed values available and thus HoleCutCustomValues is checked and read-only
                    if (!HoleCutCustomValues.getValue()
                        || (HoleCutCustomValues.getValue() && HoleCutCustomValues.isReadOnly())) {
                        HoleCutDiameter.setValue(dimen.diameter);
                        HoleCutDepth.setValue(dimen.depth);
                        HoleCutDiameter.setReadOnly(true);
                        HoleCutDepth.setReadOnly(true);
                        if (HoleCutCustomValues.getValue() && HoleCutCustomValues.isReadOnly())
                            HoleCutCustomValues.setValue(false);
                    }
                    else {
                        HoleCutDiameter.setReadOnly(false);
                        HoleCutDepth.setReadOnly(false);
                    }
                    HoleCutCustomValues.setReadOnly(false);
                }
            } else if (counter.cut_type == CutDimensionSet::Countersink) {
                const CounterSinkDimension &dimen = counter.get_sink(threadSizeStr);
                if (dimen.thread == "None") {
                    // valid values for visual feedback
                    HoleCutDiameter.setValue(Diameter.getValue() + 0.1);
                    // there might be an angle of zero (if no norm exists for the size)
                    if (HoleCutCountersinkAngle.getValue() == 0.0) {
                        HoleCutCountersinkAngle.setValue(counter.angle);
                    }
                    // we force custom values since there are no normed ones
                    HoleCutCustomValues.setReadOnly(true);
                    // important to set only if not already true, to avoid loop call of updateHoleCutParams()
                    if (!HoleCutCustomValues.getValue()) {
                        HoleCutCustomValues.setValue(true);
                        HoleCutDiameter.setReadOnly(false);
                        HoleCutDepth.setReadOnly(false);
                        HoleCutCountersinkAngle.setReadOnly(false);
                    }
                } else {
                    // set normed values if not overwritten or if previously there
                    // were no normed values available and thus HoleCutCustomValues is checked and read-only
                    if (!HoleCutCustomValues.getValue()
                        || (HoleCutCustomValues.getValue() && HoleCutCustomValues.isReadOnly())) {
                        HoleCutDiameter.setValue(dimen.diameter);
                        HoleCutDiameter.setReadOnly(true);
                        HoleCutDepth.setReadOnly(true);
                        HoleCutCountersinkAngle.setValue(counter.angle);
                        HoleCutCountersinkAngle.setReadOnly(true);
                        if (HoleCutCustomValues.getValue() && HoleCutCustomValues.isReadOnly())
                            HoleCutCustomValues.setValue(false);
                    }
                    else {
                        HoleCutDiameter.setReadOnly(false);
                        HoleCutDepth.setReadOnly(false);
                        HoleCutCountersinkAngle.setReadOnly(false);
                    }
                    HoleCutCustomValues.setReadOnly(false);
                }
            }
        }

        // handle legacy types but don’t change user settings for
        // user defined None, Counterbore and Countersink
        // handle legacy types but don’t change user settings for
        // user defined None, Counterbore and Countersink
        else if (holeCutTypeStr == "Cheesehead (deprecated)") {
            HoleCutDiameter.setValue(diameterVal * 1.6);
            HoleCutDepth.setValue(diameterVal * 0.6);
            HoleCutDiameter.setReadOnly(false);
            HoleCutDepth.setReadOnly(false);
        }
        else if (holeCutTypeStr == "Countersink socket screw (deprecated)") {
            HoleCutDiameter.setValue(diameterVal * 2.0);
            HoleCutDepth.setValue(diameterVal * 0.0);
            if (HoleCutCountersinkAngle.getValue() == 0.0) {
                HoleCutCountersinkAngle.setValue(90.0);
            }
            HoleCutDiameter.setReadOnly(false);
            HoleCutDepth.setReadOnly(false);
            HoleCutCountersinkAngle.setReadOnly(false);
        }
        else if (holeCutTypeStr == "Cap screw (deprecated)") {
            HoleCutDiameter.setValue(diameterVal * 1.5);
            HoleCutDepth.setValue(diameterVal * 1.25);
            HoleCutDiameter.setReadOnly(false);
            HoleCutDepth.setReadOnly(false);
        }
    }
    else { // we have an UTS profile or none

        // we don't update for these settings but we need to set a value for new holes
        // furthermore we must assure the hole cut diameter is not <= the hole diameter
        // if we have a cut but the values are zero, we assume it is a new hole
        // we use rules of thumbs as proposal
        if (holeCutTypeStr == "Counterbore") {
            if (HoleCutDiameter.getValue() == 0.0 || HoleCutDiameter.getValue() <= diameterVal) {
                HoleCutDiameter.setValue(diameterVal * 1.6);
                HoleCutDepth.setValue(diameterVal * 0.9);
            }
            if (HoleCutDepth.getValue() == 0.0)
                HoleCutDepth.setValue(diameterVal * 0.9);
            HoleCutDiameter.setReadOnly(false);
            HoleCutDepth.setReadOnly(false);
        }
        else if (holeCutTypeStr == "Countersink") {
            if (HoleCutDiameter.getValue() == 0.0 || HoleCutDiameter.getValue() <= diameterVal) {
                HoleCutDiameter.setValue(diameterVal * 1.7);
                // 82 degrees for UTS, 90 otherwise
                if (threadTypeStr != "None")       
                    HoleCutCountersinkAngle.setValue(82.0);
                else
                    HoleCutCountersinkAngle.setValue(90.0);
            }
            if (HoleCutCountersinkAngle.getValue() == 0.0) {
                if (threadTypeStr != "None")
                    HoleCutCountersinkAngle.setValue(82.0);
                else
                    HoleCutCountersinkAngle.setValue(90.0);
            }
            HoleCutDiameter.setReadOnly(false);
            HoleCutDepth.setReadOnly(false);
            HoleCutCountersinkAngle.setReadOnly(false);
        }
    }
}

void Hole::updateDiameterParam()
{
    // Diameter parameter depends on Threaded, ThreadType, ThreadSize, and ThreadFit

    int threadType = ThreadType.getValue();
    int threadSize = ThreadSize.getValue();
    if (threadType < 0) {
        // When restoring the feature it might be in an inconsistent state.
        // So, just silently ignore it instead of throwing an exception.
        if (isRestoring())
            return;
        throw Base::IndexError("Thread type out of range");
    }
    if (threadSize < 0) {
        // When restoring the feature it might be in an inconsistent state.
        // So, just silently ignore it instead of throwing an exception.
        if (isRestoring())
            return;
        throw Base::IndexError("Thread size out of range");
    }
    double diameter = threadDescription[threadType][threadSize].diameter;
    double pitch = threadDescription[threadType][threadSize].pitch;

    if (threadType == 0)
        return;

    if (Threaded.getValue()) {
        if (std::string(ThreadType.getValueAsString()) != "None") {
            double h = pitch * sqrt(3) / 2;

            // Basic profile for ISO and UTS threads
            ThreadPitch.setValue(pitch);
            ThreadAngle.setValue(60);
            ThreadCutOffInner.setValue(h/8);
            ThreadCutOffOuter.setValue(h/4);
        }

        if (ModelActualThread.getValue()) {
            pitch = ThreadPitch.getValue();
        }

        // use normed diameters if possible
        std::string threadType = ThreadType.getValueAsString();
        if (threadType == "ISOMetricProfile" || threadType == "UNC"
            || threadType == "UNF" || threadType == "UNEF") {
            diameter = threadDescription[ThreadType.getValue()][ThreadSize.getValue()].CoreHole;
        }     
        // if nothing available, we must calculate
        else {
            // this fits exactly the definition for ISO metric fine
            diameter = diameter - pitch;
        }
    }
    else { // we have a clearance hole
        bool found = false;
        std::string threadType = ThreadType.getValueAsString();
        // UTS and metric have a different clearance hole set
        if (threadType == "ISOMetricProfile" || threadType == "ISOMetricFineProfile") {
            int MatrixRowSizeMetric = sizeof(metricHoleDiameters) / sizeof(metricHoleDiameters[0]);
            switch (ThreadFit.getValue()) {
            case 0: /* standard fit */
                // read diameter out of matrix
                for (int i = 0; i < MatrixRowSizeMetric; i++) {
                    if (metricHoleDiameters[i][0] == diameter) {
                        diameter = metricHoleDiameters[i][2];
                        found = true;
                        break;
                    }
                }
                // if nothing is defined (e.g. for M2.2, M9 and M11), we must calculate
                // we use the factors defined for M5 in the metricHoleDiameters list
                if (!found) {
                    diameter = diameter * 1.10;
                }
                break;
            case 1: /* close fit */
                // read diameter out of matrix
                for (int i = 0; i < MatrixRowSizeMetric; i++) {
                    if (metricHoleDiameters[i][0] == diameter) {
                        diameter = metricHoleDiameters[i][1];
                        found = true;
                        break;
                    }
                }
                // if nothing was found, we must calculate
                if (!found) {
                    diameter = diameter * 1.06;
                }
                break;
            case 2: /* wide fit */
                // read diameter out of matrix
                for (int i = 0; i < MatrixRowSizeMetric; i++) {
                    if (metricHoleDiameters[i][0] == diameter) {
                        diameter = metricHoleDiameters[i][3];
                        found = true;
                        break;
                    }
                }
                // if nothing was found, we must calculate
                if (!found) {
                    diameter = diameter * 1.16;
                }
                break;
            default:
                throw Base::IndexError("Thread fit out of range");
            }
        }
        else if (threadType == "UNC" || threadType == "UNF" || threadType == "UNEF") {
            std::string ThreadSizeString = ThreadSize.getValueAsString();
            int MatrixRowSizeUTS = sizeof(UTSHoleDiameters) / sizeof(UTSHoleDiameters[0]);
            switch (ThreadFit.getValue()) {
            case 0: /* normal fit */
                // read diameter out of matrix
                for (int i = 0; i < MatrixRowSizeUTS; i++) {
                    if (UTSHoleDiameters[i].designation == ThreadSizeString) {
                        diameter = UTSHoleDiameters[i].normal;
                        found = true;
                        break;
                    }
                }
                // if nothing was found (if "#12" or "9/16"), we must calculate
                // // we use the factors defined for "3/8" in the UTSHoleDiameters list
                if (!found) {
                    diameter = diameter * 1.08;
                }
                break;
            case 1: /* close fit */
                // read diameter out of matrix
                for (int i = 0; i < MatrixRowSizeUTS; i++) {
                    if (UTSHoleDiameters[i].designation == ThreadSizeString) {
                        diameter = UTSHoleDiameters[i].close;
                        found = true;
                        break;
                    }
                }
                // if nothing was found, we must calculate
                if (!found) {
                    diameter = diameter * 1.04;
                }
                break;
            case 2: /* loose fit */
                // read diameter out of matrix
                for (int i = 0; i < MatrixRowSizeUTS; i++) {
                    if (UTSHoleDiameters[i].designation == ThreadSizeString) {
                        diameter = UTSHoleDiameters[i].loose;
                        found = true;
                        break;
                    }
                }
                // if nothing was found, we must calculate
                if (!found) {
                    diameter = diameter * 1.12;
                }
                break;
            default:
                throw Base::IndexError("Thread fit out of range");
            }
        }
        else {
            switch (ThreadFit.getValue()) {
            case 0: /* normal fit */
                // we must calculate
                if (!found) {
                    diameter = diameter * 1.1;
                }
                break;
            case 1: /* close fit */
                if (!found) {
                    diameter = diameter * 1.05;
                }
                break;
            case 2: /* loose fit */
                if (!found) {
                    diameter = diameter * 1.15;
                }
                break;
            default:
                throw Base::IndexError("Thread fit out of range");
            }
        }
    }
    Diameter.setValue(diameter);
}

void Hole::onChanged(const App::Property *prop)
{
    if (prop == &ThreadType) {
        std::string type, holeCutTypeStr;
        if (ThreadType.isValid())
            type = ThreadType.getValueAsString();
        if (HoleCutType.isValid())
            holeCutTypeStr = HoleCutType.getValueAsString();

        if (type == "None" ) {
            ThreadSize.setEnums(ThreadSize_None_Enums);
            ThreadClass.setEnums(ThreadClass_None_Enums);
            HoleCutType.setEnums(HoleCutType_None_Enums);
            Threaded.setReadOnly(true);
            ThreadSize.setReadOnly(true);
            ThreadFit.setReadOnly(true);
            ThreadClass.setReadOnly(true);
            Diameter.setReadOnly(false);
            Threaded.setValue(0);
        }
        else if ( type == "ISOMetricProfile" ) {
            ThreadSize.setEnums(ThreadSize_ISOmetric_Enums);
            ThreadClass.setEnums(ThreadClass_ISOmetric_Enums);
            HoleCutType.setEnums(HoleCutType_ISOmetric_Enums);
            ThreadFit.setEnums(ClearanceMetricEnums);
            Threaded.setReadOnly(false);
            ThreadSize.setReadOnly(false);
            // thread class and direction are only sensible if threaded
            // fit only sensible if not threaded
            ThreadFit.setReadOnly(Threaded.getValue());
            ThreadClass.setReadOnly(!Threaded.getValue());
            Diameter.setReadOnly(true);
        }
        else if ( type == "ISOMetricFineProfile" ) {
            ThreadSize.setEnums(ThreadSize_ISOmetricfine_Enums);
            ThreadClass.setEnums(ThreadClass_ISOmetricfine_Enums);
            HoleCutType.setEnums(HoleCutType_ISOmetricfine_Enums);
            ThreadFit.setEnums(ClearanceMetricEnums);
            Threaded.setReadOnly(false);
            ThreadSize.setReadOnly(false);
            // thread class and direction are only sensible if threaded
            // fit only sensible if not threaded
            ThreadFit.setReadOnly(Threaded.getValue());
            ThreadClass.setReadOnly(!Threaded.getValue());
            Diameter.setReadOnly(true);
        }
        else if ( type == "UNC" ) {
            ThreadSize.setEnums(ThreadSize_UNC_Enums);
            ThreadClass.setEnums(ThreadClass_UNC_Enums);
            HoleCutType.setEnums(HoleCutType_UNC_Enums);
            ThreadFit.setEnums(ClearanceUTSEnums);
            Threaded.setReadOnly(false);
            ThreadSize.setReadOnly(false);
            // thread class and direction are only sensible if threaded
            // fit only sensible if not threaded
            ThreadFit.setReadOnly(Threaded.getValue());
            ThreadClass.setReadOnly(!Threaded.getValue());
            Diameter.setReadOnly(true);
        }
        else if ( type == "UNF" ) {
            ThreadSize.setEnums(ThreadSize_UNF_Enums);
            ThreadClass.setEnums(ThreadClass_UNF_Enums);
            HoleCutType.setEnums(HoleCutType_UNF_Enums);
            ThreadFit.setEnums(ClearanceUTSEnums);
            Threaded.setReadOnly(false);
            ThreadSize.setReadOnly(false);
            // thread class and direction are only sensible if threaded
            // fit only sensible if not threaded
            ThreadFit.setReadOnly(Threaded.getValue());
            ThreadClass.setReadOnly(!Threaded.getValue());
            Diameter.setReadOnly(true);
        }
        else if ( type == "UNEF" ) {
            ThreadSize.setEnums(ThreadSize_UNEF_Enums);
            ThreadClass.setEnums(ThreadClass_UNEF_Enums);
            HoleCutType.setEnums(HoleCutType_UNEF_Enums);
            ThreadFit.setEnums(ClearanceUTSEnums);
            Threaded.setReadOnly(false);
            ThreadSize.setReadOnly(false);
            // thread class and direction are only sensible if threaded
            // fit only sensible if not threaded
            ThreadFit.setReadOnly(Threaded.getValue());
            ThreadClass.setReadOnly(!Threaded.getValue());;
            Diameter.setReadOnly(true);
        }

        if (holeCutTypeStr == "None") {
            HoleCutCustomValues.setReadOnly(true);
            HoleCutDiameter.setReadOnly(true);
            HoleCutDepth.setReadOnly(true);
            HoleCutCountersinkAngle.setReadOnly(true);
        }
        else if (holeCutTypeStr == "Counterbore") {
            HoleCutCustomValues.setReadOnly(true);
            HoleCutDiameter.setReadOnly(false);
            HoleCutDepth.setReadOnly(false);
            HoleCutCountersinkAngle.setReadOnly(true);
        }
        else if (holeCutTypeStr == "Countersink") {
            HoleCutCustomValues.setReadOnly(true);
            HoleCutDiameter.setReadOnly(false);
            HoleCutDepth.setReadOnly(false);
            HoleCutCountersinkAngle.setReadOnly(false);
        }
        else { // screw definition
            HoleCutCustomValues.setReadOnly(false);
            if (HoleCutCustomValues.getValue()) {
                HoleCutDiameter.setReadOnly(false);
                HoleCutDepth.setReadOnly(false);
                // we must not set HoleCutCountersinkAngle here because the info if this can
                // be enabled is first available in updateHoleCutParams and thus handled there
                updateHoleCutParams();
            }
            else {
                HoleCutDiameter.setReadOnly(true);
                HoleCutDepth.setReadOnly(true);
                HoleCutCountersinkAngle.setReadOnly(true);
            }
        }

        // Signal changes to these
        ProfileBased::onChanged(&ThreadSize);
        ProfileBased::onChanged(&ThreadClass);
        ProfileBased::onChanged(&HoleCutType);
        ProfileBased::onChanged(&Threaded);

        bool v = (type != "None") || !Threaded.getValue() || !ModelActualThread.getValue();
        ThreadPitch.setReadOnly(v);
        ThreadAngle.setReadOnly(v);
        ThreadCutOffInner.setReadOnly(v);
        ThreadCutOffOuter.setReadOnly(v);

        // Diameter parameter depends on this
        if (type != "None" )
            updateDiameterParam();
    }
    else if (prop == &Threaded) {
        std::string type(ThreadType.getValueAsString());

        // thread class and direction are only sensible if threaded
        // fit only sensible if not threaded
        if (Threaded.getValue()) {
            ThreadClass.setReadOnly(false);
            ThreadDirection.setReadOnly(false);
            ThreadFit.setReadOnly(true);
            ModelActualThread.setReadOnly(true); // For now set this one to read only
        }
        else {
            ThreadClass.setReadOnly(true);
            ThreadDirection.setReadOnly(true);
            if (type == "None")
                ThreadFit.setReadOnly(true);
            else
                ThreadFit.setReadOnly(false);
            ModelActualThread.setReadOnly(true);
        }

        // Diameter parameter depends on this
        updateDiameterParam();
    }
    else if (prop == &ModelActualThread) {
        bool v =(!ModelActualThread.getValue()) ||
                (Threaded.isReadOnly()) ||
                (std::string(ThreadType.getValueAsString()) != "None");

        ThreadPitch.setReadOnly(v);
        ThreadAngle.setReadOnly(v);
        ThreadCutOffInner.setReadOnly(v);
        ThreadCutOffOuter.setReadOnly(v);
    }
    else if (prop == &DrillPoint) {
        if (DrillPoint.getValue() == 1) {
            DrillPointAngle.setReadOnly(false);
            DrillForDepth.setReadOnly(false);
        }
        else {
            DrillPointAngle.setReadOnly(true);
            DrillForDepth.setReadOnly(true);
        }
    }
    else if (prop == &Tapered) {
        if (Tapered.getValue())
            TaperedAngle.setReadOnly(false);
        else
            TaperedAngle.setReadOnly(true);
    }
    else if (prop == &ThreadSize) {
        updateDiameterParam();
        // updateHoleCutParams() will later automatically be called because updateDiameterParam() changes &Diameter
    }
    else if (prop == &ThreadFit) {
        updateDiameterParam();
    }
    else if (prop == &Diameter) {
        // a changed diameter means we also need to check the hole cut
        // because the hole cut diameter must not be <= than the diameter
        updateHoleCutParams();
    }
    else if (prop == &HoleCutType) {
        ProfileBased::onChanged(&HoleCutDiameter);
        ProfileBased::onChanged(&HoleCutDepth);
        ProfileBased::onChanged(&HoleCutCountersinkAngle);

        // the read-only states are set in updateHoleCutParams()
        updateHoleCutParams();
    }
    else if (prop == &HoleCutCustomValues) {
        // when going back to standardized values, we must recalculate
        // also to find out if HoleCutCountersinkAngle can be ReadOnly
        // both an also the read-only states is done in updateHoleCutParams()
        updateHoleCutParams();
    }
    else if (prop == &DepthType) {
        Depth.setReadOnly((std::string(DepthType.getValueAsString()) != "Dimension"));
        DrillPoint.setReadOnly((std::string(DepthType.getValueAsString()) != "Dimension"));
        DrillPointAngle.setReadOnly((std::string(DepthType.getValueAsString()) != "Dimension"));
        DrillForDepth.setReadOnly((std::string(DepthType.getValueAsString()) != "Dimension"));
    }
    ProfileBased::onChanged(prop);
}

/**
  * Computes 2D intersection between the lines (pa1, pa2) and (pb1, pb2).
  * The lines are assumed to be crossing, and it is an error
  * to specify parallel lines.
  * Only the x and y coordinates of the points are used to compute the 2D intersection.
  *
  * The result are the x and y coordinate of the intersection point.
  */
static void computeIntersection(gp_Pnt pa1, gp_Pnt pa2, gp_Pnt pb1, gp_Pnt pb2, double & x, double & y)
{
    double vx1 = pa1.X() - pa2.X();
    double vy1 = pa1.Y() - pa2.Y();
    double vx2 = pb1.X() - pb2.X();
    double vy2 = pb1.Y() - pb2.Y();
    double x1 = pa1.X();
    double y1 = pa1.Y();
    double x2 = pb1.X();
    double y2 = pb1.Y();

    /* Solve the system
        x1 + t1 * vx1 = x2 + t2 * vx2
        y1 + t1 * vy1 = y2 + t2 * vy2

        =>

       [ vx1    -vx2 ] [ t1 ] = [ x2 - x1 ]
       [ vy1    -vy2 ] [ t2 ] = [ y2 - y1 ]

       =>

       [ t1 ] = f * [ -vy2  vx2 ] [ x2 - x1 ]
       [ t2 ] =     [ -vy1  vx1 ] [ y2 - y1 ]

     */

    assert( ( ( vx1 * - vy2 ) - ( -vx2 * vy1 ) ) != 0 );

    double f = 1 / ( ( vx1 * - vy2 ) - ( -vx2 * vy1 ) );

    double t1 = -vy2 * f * ( x2 - x1 ) + vx2 * f * ( y2 - y1 );

#ifdef _DEBUG
    double t2 = -vy1 * f * ( x2 - x1 ) + vx1 * f * ( y2 - y1 );

    assert( ( x1 + t1 * vx1 ) - ( x2 + t2 * vx2 ) < 1e-6 );
    assert( ( y1 + t1 * vy1 ) - ( y2 + t2 * vy2 ) < 1e-6 );
#endif

    x = x1 + t1 * vx1;
    y = y1 + t1 * vy1;
}

short Hole::mustExecute() const
{
    if ( ThreadType.isTouched() ||
         Threaded.isTouched() ||
         ModelActualThread.isTouched() ||
         ThreadPitch.isTouched() ||
         ThreadAngle.isTouched() ||
         ThreadCutOffInner.isTouched() ||
         ThreadCutOffOuter.isTouched() ||
         ThreadSize.isTouched() ||
         ThreadClass.isTouched() ||
         ThreadFit.isTouched() ||
         Diameter.isTouched() ||
         ThreadDirection.isTouched() ||
         HoleCutType.isTouched() ||
         HoleCutDiameter.isTouched() ||
         HoleCutDepth.isTouched() ||
         HoleCutCountersinkAngle.isTouched() ||
         DepthType.isTouched() ||
         Depth.isTouched() ||
         DrillPoint.isTouched() ||
         DrillPointAngle.isTouched() ||
         Tapered.isTouched() ||
         TaperedAngle.isTouched() )
        return 1;
    return ProfileBased::mustExecute();
}

void Hole::Restore(Base::XMLReader &reader)
{
    ProfileBased::Restore(reader);

    updateProps();
}

void Hole::updateProps()
{
    onChanged(&Threaded);
    onChanged(&ModelActualThread);
    onChanged(&ThreadPitch);
    onChanged(&ThreadAngle);
    onChanged(&ThreadCutOffInner);
    onChanged(&ThreadCutOffOuter);
    onChanged(&ThreadType);
    onChanged(&ThreadSize);
    onChanged(&ThreadClass);
    onChanged(&ThreadFit);
    onChanged(&Diameter);
    onChanged(&ThreadDirection);
    onChanged(&HoleCutType);
    onChanged(&HoleCutDiameter);
    onChanged(&HoleCutDepth);
    onChanged(&HoleCutCountersinkAngle);
    onChanged(&DepthType);
    onChanged(&Depth);
    onChanged(&DrillPoint);
    onChanged(&DrillPointAngle);
    onChanged(&Tapered);
    onChanged(&TaperedAngle);
}

static gp_Pnt toPnt(gp_Vec dir)
{
    return gp_Pnt(dir.X(), dir.Y(), dir.Z());
}

App::DocumentObjectExecReturn *Hole::execute(void)
{
    Part::Feature* profile = 0;
    TopoDS_Shape profileshape;
    try {
        profile = getVerifiedObject();
        profileshape = getVerifiedFace();
    } catch (const Base::Exception& e) {
        return new App::DocumentObjectExecReturn(e.what());
    }

    // Find the base shape
    TopoDS_Shape base;
    try {
        base = getBaseShape();
    }
    catch (const Base::Exception&) {
        std::string text(QT_TR_NOOP("The requested feature cannot be created. The reason may be that:\n"
                                    "  - the active Body does not contain a base shape, so there is no\n"
                                    "  material to be removed;\n"
                                    "  - the selected sketch does not belong to the active Body."));
        return new App::DocumentObjectExecReturn(text);
    }

    try {
        std::string method(DepthType.getValueAsString());
        double length = 0.0;

        this->positionByPrevious();
        TopLoc_Location invObjLoc = this->getLocation().Inverted();

        base.Move(invObjLoc);

        if (profileshape.IsNull())
            return new App::DocumentObjectExecReturn("Hole error: Creating a face from sketch failed");
        profileshape.Move(invObjLoc);

        /* Build the prototype hole */

        // Get vector normal to profile
        Base::Vector3d  SketchVector = getProfileNormal();
        if (Reversed.getValue())
            SketchVector *= -1.0;

        // Define this as zDir
        gp_Vec zDir(SketchVector.x, SketchVector.y, SketchVector.z);
        zDir.Transform(invObjLoc.Transformation());

        // Define xDir
        gp_Vec xDir;

        /* Compute xDir normal to zDir */
        if (std::abs(zDir.Z() - zDir.X()) > Precision::Confusion())
            xDir = gp_Vec(zDir.Z(), 0, -zDir.X());
        else if (std::abs(zDir.Z() - zDir.Y()) > Precision::Confusion())
            xDir = gp_Vec(zDir.Y(), -zDir.X(), 0);
        else
            xDir = gp_Vec(0, -zDir.Z(), zDir.Y());

        // Normalize xDir; this is needed as the computation above does not necessarily give a unit-length vector.
        xDir.Normalize();

        if ( method == "Dimension" )
            length = Depth.getValue();
        else if ( method == "UpToFirst" ) {
            /* FIXME */
        }
        else if ( method == "ThroughAll" ) {
            // Use a large (10m), but finite length
            length = 1e4;
        }
        else
            return new App::DocumentObjectExecReturn("Hole error: Unsupported length specification");

        if (length <= 0.0)
            return new App::DocumentObjectExecReturn("Hole error: Invalid hole depth");

        BRepBuilderAPI_MakeWire mkWire;
        const std::string holeCutType = HoleCutType.getValueAsString();
        const std::string threadType = ThreadType.getValueAsString();
        bool isCountersink = (holeCutType == "Countersink" ||
              holeCutType == "Countersink socket screw (deprecated)" ||
              isDynamicCountersink(threadType, holeCutType));
        bool isCounterbore = (holeCutType == "Counterbore" ||
              holeCutType == "Cheesehead (deprecated)" ||
              holeCutType == "Cap screw (deprecated)" ||
              isDynamicCounterbore(threadType, holeCutType));
        double TaperedAngleVal = Tapered.getValue() ? Base::toRadians( TaperedAngle.getValue() ) : Base::toRadians(90.0);
        double radiusBottom = Diameter.getValue() / 2.0 - length / tan(TaperedAngleVal);
        double radius = Diameter.getValue() / 2.0;
        double holeCutRadius = HoleCutDiameter.getValue() / 2.0;
        gp_Pnt firstPoint(0, 0, 0);
        gp_Pnt lastPoint(0, 0, 0);
        double lengthCounter = 0.0;
        double xPosCounter = 0.0;
        double zPosCounter = 0.0;

        if (TaperedAngleVal <= 0.0 || TaperedAngleVal > Base::toRadians( 180.0 ) )
            return new App::DocumentObjectExecReturn("Hole error: Invalid taper angle");

        if ( isCountersink ) {
            double countersinkAngle = Base::toRadians( HoleCutCountersinkAngle.getValue() / 2.0 );

            if ( countersinkAngle <= 0 || countersinkAngle > Base::toRadians( 180.0 ) )
                return new App::DocumentObjectExecReturn("Hole error: Invalid countersink angle");

            if (holeCutRadius < radius)
                return new App::DocumentObjectExecReturn("Hole error: Hole cut diameter too small");

            // Top point
            gp_Pnt newPoint = toPnt(holeCutRadius * xDir);
            mkWire.Add( BRepBuilderAPI_MakeEdge(lastPoint, newPoint) );
            lastPoint = newPoint;

            computeIntersection(gp_Pnt( holeCutRadius, 0, 0 ),
                                gp_Pnt( holeCutRadius - sin( countersinkAngle ), -cos( countersinkAngle ), 0 ),
                                gp_Pnt( radius, 0, 0 ),
                                gp_Pnt( radiusBottom, -length, 0), xPosCounter, zPosCounter);
            if (-length > zPosCounter)
                return new App::DocumentObjectExecReturn("Hole error: Invalid countersink");

            lengthCounter = zPosCounter;

            newPoint = toPnt(xPosCounter * xDir + zPosCounter * zDir);
            mkWire.Add( BRepBuilderAPI_MakeEdge( lastPoint, newPoint ) );
            lastPoint = newPoint;
        }
        else if ( isCounterbore ) {
            double holeCutDepth = HoleCutDepth.getValue();

            if (holeCutDepth <= 0.0)
                return new App::DocumentObjectExecReturn("Hole error: Hole cut depth must be greater than zero");

            if (holeCutDepth > length)
                return new App::DocumentObjectExecReturn("Hole error: Hole cut depth must be less than hole depth");

            if (holeCutRadius < radius)
                return new App::DocumentObjectExecReturn("Hole error: Hole cut diameter too small");

            // Top point
            gp_Pnt newPoint = toPnt(holeCutRadius * xDir);
            mkWire.Add(BRepBuilderAPI_MakeEdge(lastPoint, newPoint));
            lastPoint = newPoint;

            // Bottom of counterbore
            newPoint = toPnt(holeCutRadius * xDir - holeCutDepth * zDir);
            mkWire.Add(BRepBuilderAPI_MakeEdge(lastPoint, newPoint));
            lastPoint = newPoint;

            // Compute intersection of tapered edge and line at bottom of counterbore hole
            computeIntersection(gp_Pnt( 0, -holeCutDepth, 0 ),
                                gp_Pnt( holeCutRadius, -holeCutDepth, 0 ),
                                gp_Pnt( radius, 0, 0 ),
                                gp_Pnt( radiusBottom, length, 0 ), xPosCounter, zPosCounter);

            lengthCounter = zPosCounter;
            newPoint = toPnt(xPosCounter * xDir + zPosCounter * zDir);
            mkWire.Add(BRepBuilderAPI_MakeEdge(lastPoint, newPoint));
            lastPoint = newPoint;
        }
        else {
            gp_Pnt newPoint = toPnt(radius * xDir);
            mkWire.Add(BRepBuilderAPI_MakeEdge(lastPoint, newPoint));
            lastPoint = newPoint;
            lengthCounter = 0.0;
        }

        std::string drillPoint = DrillPoint.getValueAsString();
        double xPosDrill = 0.0;
        double zPosDrill = 0.0;
        if (drillPoint == "Flat") {
            gp_Pnt newPoint = toPnt(radiusBottom * xDir + -length * zDir);
            mkWire.Add(BRepBuilderAPI_MakeEdge(lastPoint, newPoint));
            lastPoint = newPoint;

            newPoint = toPnt(-length * zDir);
            mkWire.Add( BRepBuilderAPI_MakeEdge( lastPoint, newPoint ) );
            lastPoint = newPoint;
        }
        else if (drillPoint == "Angled") {
            double drillPointAngle = Base::toRadians( ( 180.0 - DrillPointAngle.getValue() ) / 2.0 );
            gp_Pnt newPoint;
            bool isDrillForDepth = DrillForDepth.getValue();

            // the angle is in any case > 0 and < 90 but nevertheless this safeguard:
            if ( drillPointAngle <= 0.0 || drillPointAngle >= Base::toRadians( 180.0 ) )
                return new App::DocumentObjectExecReturn("Hole error: Invalid drill point angle");

            // if option to take drill point size into account
            // the next wire point is the intersection of the drill edge and the hole edge
            if (isDrillForDepth) {
                computeIntersection(gp_Pnt(0, -length, 0),
                    gp_Pnt(radius, radius * tan(drillPointAngle) - length, 0),
                    gp_Pnt(radius, 0, 0),
                    gp_Pnt(radiusBottom, -length, 0), xPosDrill, zPosDrill);
                if (zPosDrill > 0 || zPosDrill >= lengthCounter)
                    return new App::DocumentObjectExecReturn("Hole error: Invalid drill point");

                newPoint = toPnt(xPosDrill * xDir + zPosDrill * zDir);
                mkWire.Add(BRepBuilderAPI_MakeEdge(lastPoint, newPoint));
                lastPoint = newPoint;

                newPoint = toPnt(-length * zDir);
                mkWire.Add(BRepBuilderAPI_MakeEdge(lastPoint, newPoint));
                lastPoint = newPoint;
            }
            else {
                xPosDrill = radiusBottom;
                zPosDrill = -length;

                newPoint = toPnt(xPosDrill * xDir + zPosDrill * zDir);
                mkWire.Add(BRepBuilderAPI_MakeEdge(lastPoint, newPoint));
                lastPoint = newPoint;

                // the end point is the size of the drill tip
                newPoint = toPnt((-length - radius * tan(drillPointAngle)) * zDir);
                mkWire.Add(BRepBuilderAPI_MakeEdge(lastPoint, newPoint));
                lastPoint = newPoint;
            }
        }

        mkWire.Add( BRepBuilderAPI_MakeEdge(lastPoint, firstPoint) );

        TopoDS_Wire wire = mkWire.Wire();

        TopoDS_Face face = BRepBuilderAPI_MakeFace(wire);

        double angle = Base::toRadians<double>(360.0);
        BRepPrimAPI_MakeRevol RevolMaker(face, gp_Ax1(firstPoint, zDir), angle);

        TopoDS_Shape protoHole;
        if (RevolMaker.IsDone()) {
            protoHole = RevolMaker.Shape();

            if (protoHole.IsNull())
                return new App::DocumentObjectExecReturn("Hole error: Resulting shape is empty");
        }
        else
            return new App::DocumentObjectExecReturn("Hole error: Could not revolve sketch");

#if 0
        // Make thread

        // This code is work in progress; making threas in OCC is not very easy, so
        // this work is postponed until later
        if (ModelActualThread.getValue()) {
            BRepBuilderAPI_MakeWire mkThreadWire;
            double z = 0;
            double d_min = Diameter.getValue() + ThreadCutOffInner.getValue();
            double d_maj = Diameter.getValue() - ThreadCutOffInner.getValue();
            int i = 0;

            firstPoint = toPnt(xDir * d_min);

            mkThreadWire.Add(BRepBuilderAPI_MakeEdge(gp_Pnt(0, 0, 0), firstPoint));
            while (z < length) {
                double z1 = i * ThreadPitch.getValue() + ThreadPitch.getValue() * 0.1;
                double z2 = i * ThreadPitch.getValue() + ThreadPitch.getValue() * 0.45;
                double z3 = i * ThreadPitch.getValue() + ThreadPitch.getValue() * 0.55;
                double z4 = i * ThreadPitch.getValue() + ThreadPitch.getValue() * 0.9;

                gp_Pnt p2 = toPnt(xDir * d_min - zDir * z1);
                gp_Pnt p3 = toPnt(xDir * d_maj - zDir * z2);
                gp_Pnt p4 = toPnt(xDir * d_maj - zDir * z3);
                gp_Pnt p5 = toPnt(xDir * d_min - zDir * z4);

                mkThreadWire.Add(BRepBuilderAPI_MakeEdge(firstPoint, p2));
                mkThreadWire.Add(BRepBuilderAPI_MakeEdge(p2, p3));
                mkThreadWire.Add(BRepBuilderAPI_MakeEdge(p3, p4));
                mkThreadWire.Add(BRepBuilderAPI_MakeEdge(p4, p5));
                firstPoint = p5;

                ++i;
                z += ThreadPitch.getValue();
            }
            mkThreadWire.Add(BRepBuilderAPI_MakeEdge(firstPoint, toPnt(-z * zDir)));
            mkThreadWire.Add(BRepBuilderAPI_MakeEdge(toPnt(-z * zDir), gp_Pnt(0, 0, 0)));

            TopoDS_Wire threadWire = mkThreadWire.Wire();

            TopoDS_Face threadFace = BRepBuilderAPI_MakeFace(threadWire);

            //TopoDS_Wire helix = TopoShape::makeHelix(ThreadPitch.getValue(), ThreadPitch.getValue(), Diameter.getValue());

            double angle = Base::toRadians<double>(360.0);
            BRepPrimAPI_MakeRevol RevolMaker2(threadFace, gp_Ax1(gp_Pnt(0,0,0), zDir), angle);

            //TopoDS_Shape protoHole;
            if (RevolMaker2.IsDone()) {
                protoHole = RevolMaker2.Shape();

                if (protoHole.IsNull())
                    return new App::DocumentObjectExecReturn("Hole: Resulting shape is empty");
            }
            else
                return new App::DocumentObjectExecReturn("Hole: Could not revolve sketch!");
        }
#endif

        BRep_Builder builder;
        TopoDS_Compound holes;
        builder.MakeCompound(holes);

        TopTools_IndexedMapOfShape edgeMap;
        TopExp::MapShapes(profileshape, TopAbs_EDGE, edgeMap);
        for ( int i=1 ; i<=edgeMap.Extent() ; i++ ) {
            Standard_Real c_start;
            Standard_Real c_end;
            TopoDS_Edge edge = TopoDS::Edge(edgeMap(i));
            Handle(Geom_Curve) c = BRep_Tool::Curve(edge, c_start, c_end);

            // Circle?
            if (c->DynamicType() != STANDARD_TYPE(Geom_Circle))
                continue;

            Handle(Geom_Circle) circle = Handle(Geom_Circle)::DownCast(c);

            const gp_Pnt& loc = circle->Axis().Location();

            gp_Trsf sketchTransformation;
            gp_Trsf localSketchTransformation;
            Base::Placement SketchPos = profile->Placement.getValue();
            Base::Matrix4D mat = SketchPos.toMatrix();
            sketchTransformation.SetValues(
                        mat[0][0], mat[0][1], mat[0][2], mat[0][3],
                        mat[1][0], mat[1][1], mat[1][2], mat[1][3],
                        mat[2][0], mat[2][1], mat[2][2], mat[2][3]
#if OCC_VERSION_HEX < 0x060800
                        , 0.00001, 0.00001
#endif
                    ); //precision was removed in OCCT CR0025194
            localSketchTransformation.SetTranslation( gp_Pnt( 0, 0, 0 ),
                                                      gp_Pnt(loc.X(), loc.Y(), loc.Z()) );

            TopoDS_Shape copy = protoHole;
            BRepBuilderAPI_Transform transformer(copy, localSketchTransformation );

            copy = transformer.Shape();
            BRepAlgoAPI_Cut mkCut( base, copy );
            if (!mkCut.IsDone())
                return new App::DocumentObjectExecReturn("Hole: Cut out of base feature failed");

            TopoDS_Shape result = mkCut.Shape();

            // We have to get the solids (fuse sometimes creates compounds)
            base = getSolid(result);
            if (base.IsNull())
                return new App::DocumentObjectExecReturn("Hole: Resulting shape is not a solid");
            base = refineShapeIfActive(base);
            builder.Add(holes, transformer.Shape() );
        }

        // Do not apply a placement to the AddSubShape property (#0003547)
        //holes.Move( this->getLocation().Inverted() );

        // set the subtractive shape property for later usage in e.g. pattern
        this->AddSubShape.setValue( holes );

        remapSupportShape(base);

        int solidCount = countSolids(base);
        if (solidCount > 1) {
            return new App::DocumentObjectExecReturn("Hole: Result has multiple solids. This is not supported at this time.");
        }

        this->Shape.setValue(base);

        return App::DocumentObject::StdReturn;
    }
    catch (Standard_Failure& e) {
        if (std::string(e.GetMessageString()) == "TopoDS::Face" &&
            (std::string(DepthType.getValueAsString()) == "UpToFirst" || std::string(DepthType.getValueAsString()) == "UpToFace"))
            return new App::DocumentObjectExecReturn("Could not create face from sketch.\n"
                "Intersecting sketch entities or multiple faces in a sketch are not allowed "
                "for making a pocket up to a face.");
        else
            return new App::DocumentObjectExecReturn(e.GetMessageString());
    }
    catch (Base::Exception& e) {
        return new App::DocumentObjectExecReturn(e.what());
    }
}

void Hole::addCutType(const CutDimensionSet& dimensions)
{
    const CutDimensionSet::ThreadType thread = dimensions.thread_type;
    const std::string &name = dimensions.name;

    std::vector<std::string> *list;
    switch(thread) {
    case CutDimensionSet::Metric:
        HoleCutTypeMap.emplace(CutDimensionKey("ISOMetricProfile", name), dimensions);
        list = &HoleCutType_ISOmetric_Enums;
        break;
    case CutDimensionSet::MetricFine:
        HoleCutTypeMap.emplace(CutDimensionKey("ISOMetricFineProfile", name), dimensions);
        list = &HoleCutType_ISOmetricfine_Enums;
        break;
    default:
        return;
    }
    // add the collected lists of JSON definitions to the lists
    // if a name doesn't already exist in the list
    if (std::all_of(list->begin(), list->end(),
                [name](const std::string &x){ return x != name; }))
        list->push_back(name);
}

bool Hole::isDynamicCounterbore(const std::string &thread,
      const std::string &holeCutType)
{
    CutDimensionKey key { thread, holeCutType };
    return HoleCutTypeMap.count(key) &&
        HoleCutTypeMap.find(key)->second.cut_type == CutDimensionSet::Counterbore;
}

bool Hole::isDynamicCountersink(const std::string &thread,
      const std::string &holeCutType)
{
    CutDimensionKey key { thread, holeCutType };
    return HoleCutTypeMap.count(key) &&
        HoleCutTypeMap.find(key)->second.cut_type == CutDimensionSet::Countersink;
}

/*
 * Counter Dimensions
 */

const Hole::CounterBoreDimension Hole::CounterBoreDimension::nothing { "None", 0.0, 0.0 };
const Hole::CounterSinkDimension Hole::CounterSinkDimension::nothing { "None", 0.0 };

Hole::CutDimensionKey::CutDimensionKey(const std::string &t, const std::string &c) :
    thread_type { t }, cut_name { c }
{
}

bool Hole::CutDimensionKey::operator<(const CutDimensionKey &b) const
{
    return thread_type < b.thread_type ||
                         (thread_type == b.thread_type && cut_name < b.cut_name);
}

const Hole::CutDimensionSet& Hole::find_cutDimensionSet(const std::string &t,
      const std::string &c) {
    return HoleCutTypeMap.find(CutDimensionKey(t, c))->second;
}

const Hole::CutDimensionSet& Hole::find_cutDimensionSet(const CutDimensionKey &k)
{
    return HoleCutTypeMap.find(k)->second;
}

Hole::CutDimensionSet::CutDimensionSet(const std::string &nme,
      std::vector<CounterBoreDimension> &&d, CutType cut, ThreadType thread, double a) :
    bore_data{ std::move(d) }, cut_type{ cut }, thread_type{thread}, name{nme}, angle{a}
{
}

Hole::CutDimensionSet::CutDimensionSet(const std::string &nme,
      std::vector<CounterSinkDimension> &&d, CutType cut, ThreadType thread, double a) :
    sink_data{ std::move(d) }, cut_type{ cut }, thread_type{thread}, name{nme}, angle{a}
{
}

const Hole::CounterBoreDimension &Hole::CutDimensionSet::get_bore(const std::string &t) const
{
    auto i = std::find_if(bore_data.begin(), bore_data.end(),
          [t](const Hole::CounterBoreDimension &x) { return x.thread == t; } );
    if (i == bore_data.end())
        return CounterBoreDimension::nothing;
    else
        return *i;
}

const Hole::CounterSinkDimension &Hole::CutDimensionSet::get_sink(const std::string &t) const
{
    auto i = std::find_if(sink_data.begin(), sink_data.end(),
          [t](const Hole::CounterSinkDimension &x) { return x.thread == t; } );
    if (i == sink_data.end())
        return CounterSinkDimension::nothing;
    else
        return *i;
}

void from_json(const nlohmann::json &j, Hole::CounterBoreDimension &t)
{
    t.thread = j["thread"].get<std::string>();
    t.diameter = j["diameter"].get<double>();
    t.depth = j["depth"].get<double>();
}

void from_json(const nlohmann::json &j, Hole::CounterSinkDimension &t)
{
    t.thread = j["thread"].get<std::string>();
    t.diameter = j["diameter"].get<double>();
}

void from_json(const nlohmann::json &j, Hole::CutDimensionSet &t)
{
    t.name = j["name"].get<std::string>();

    std::string  thread_type_string = j["thread_type"].get<std::string>();
    if (thread_type_string == "metric")
        t.thread_type = Hole::CutDimensionSet::Metric;
    else if (thread_type_string == "metricfine")
        t.thread_type = Hole::CutDimensionSet::MetricFine;
    else
        throw Base::IndexError(std::string("Thread type ‘") + thread_type_string + "’ unsupported");

    std::string  cut_type_string = j["cut_type"].get<std::string>();
    if (cut_type_string == "counterbore") {
        t.cut_type = Hole::CutDimensionSet::Counterbore;
        t.bore_data = j["data"].get<std::vector<Hole::CounterBoreDimension> >();
        t.angle = 0.0;
    } else if (cut_type_string == "countersink") {
        t.cut_type = Hole::CutDimensionSet::Countersink;
        t.sink_data = j["data"].get<std::vector<Hole::CounterSinkDimension> >();
        t.angle = j["angle"].get<double>();
    }
    else
        throw Base::IndexError(std::string("Cut type ‘") + cut_type_string + "’ unsupported");

    t.name = j["name"].get<std::string>();
}

void Hole::readCutDefinitions()
{
    const char subpath[] = "Mod/PartDesign/Resources/Hole";
    std::vector<std::string> dirs {
        ::App::Application::getResourceDir() + subpath,
        ::App::Application::getUserAppDataDir() + subpath,
    };

    std::clog << "Looking for thread definitions in: ";
    for (auto &i : dirs)
        std::clog << i << " ";
    std::clog << "\n";
    for (auto &dir : dirs) {
        std::vector<::Base::FileInfo> files { ::Base::FileInfo(dir).getDirectoryContent() };
        for (const auto &f : files) {
            if (f.extension() == "json") {
                try {
                    std::ifstream input(f.filePath());
                    nlohmann::json j;
                    input >> j;
                    CutDimensionSet screwtype = j.get<CutDimensionSet>();
                    addCutType(screwtype);
                }
                catch(std::exception &e) {
                    std::cerr << "Failed reading ‘" << f.filePath() << "’ with: "<< e.what() << "\n";
                }
            }
        }
    }
}

} // namespace PartDesign
