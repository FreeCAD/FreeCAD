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
#include <App/Document.h>
#include <Base/Reader.h>
#include <Mod/Part/App/TopoShape.h>

#include "FeatureHole.h"

using namespace PartDesign;

/* TRANSLATOR PartDesign::Hole */

const char* Hole::DepthTypeEnums[]                   = { "Dimension", "ThroughAll", /*, "UpToFirst", */ NULL };
const char* Hole::ThreadTypeEnums[]                  = { "None", "ISOMetricProfile", "ISOMetricFineProfile", "UNC", "UNF", "UNEF", NULL};
const char* Hole::ThreadFitEnums[]                   = { "Standard", "Close", NULL};
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
        { "---", 	6.0, 0.0 },
    },

    /* ISO metric coarse */
    {
        { "M1.60", 	1.60,       0.35 },
        { "M2.00", 	2.00,       0.40 },
        { "M2.50", 	2.50,       0.45 },
        { "M3", 	3.00,       0.50 },
        { "M3.50", 	3.50,       0.60 },
        { "M4", 	4.00,       0.70 },
        { "M5", 	5.00,       0.80 },
        { "M6", 	6.00,       1.00 },
        { "M8", 	8.00,       1.25 },
        { "M10", 	10.00,      1.50 },
        { "M12", 	12.00,      1.75 },
        { "M14", 	14.00,      2.00 },
        { "M16", 	16.00,      2.00 },
        { "M20", 	20.00,      2.50 },
        { "M22", 	22.00,      2.50 },
        { "M24", 	24.00,      3.00 },
        { "M27", 	27.00,      3.00 },
        { "M30", 	30.00,      3.50 },
        { "M36", 	36.00,      4.00 },
        { "M42", 	42.00,      4.50 },
        { "M48", 	48.00,      5.00 },
        { "M56", 	56.00,      5.50 },
        { "M64", 	64.00,      6.00 },
        { "M68", 	68.00,      6.00 },
     },
    /* ISO metric fine */
    {
        { "M1.0x0.2", 	1.0,    0.20 },
        { "M1.1x0.2", 	1.1,    0.20 },
        { "M1.2x0.2", 	1.2,    0.20 },
        { "M1.4z0.2", 	1.4,    0.20 },
        { "M1.6x0.2", 	1.6,    0.20 },
        { "M1.8x0.2", 	1.8,    0.20 },
        { "M2x0.25", 	2.0,    0.25 },
        { "M2.2x0.25", 	2.2,    0.25 },
        { "M2.5x0.35", 	2.5,    0.35 },
        { "M3x0.35", 	3.0,    0.35 },
        { "M3.5x0.35",  3.5,    0.35 },
        { "M4x0.5", 	4.0,    0.50 },
        { "M4.5x0.5", 	4.5,    0.50 },
        { "M5x0.5", 	5.0,    0.50 },
        { "M5.5x0.5", 	5.5,    0.50 },
        { "M6x0.75", 	6.0,    0.75 },
        { "M7x0.75", 	7.0,    0.75 },
        { "M8x0.75", 	8.0,    0.75 },
        { "M8x1.0", 	8.0,    1.00 },
        { "M9x0.75", 	9.0,    0.75 },
        { "M9x1",       9.0,    1.00 },
        { "M10x0.75", 	10.0,   0.75 },
        { "M10x1",      10.0,   1.00 },
        { "M10x1.25", 	10.0,   1.25 },
        { "M11x0.75", 	11.0,   0.75 },
        { "M11x1",      11.0,   1.00 },
        { "M12x1",      12.0,   1.00 },
        { "M12x1.25", 	12.0,   1.25 },
        { "M12x1.5", 	12.0,   1.50 },
        { "M14x1.0", 	14.0,   1.00 },
        { "M14x1.25", 	14.0,   1.25 },
        { "M14x1.5", 	14.0,   1.50 },
        { "M15x1",      15.0,   1.00 },
        { "M15x1.5", 	15.0,   1.50 },
        { "M16x1",      16.0,   1.00 },
        { "M16x1.5", 	16.0,   1.50 },
        { "M17x1.0", 	17.0,   1.00 },
        { "M17x1.5", 	17.0,   1.50 },
        { "M18x1.0", 	18.0,   1.00 },
        { "M18x1.5", 	18.0,   1.50 },
        { "M18x2.0", 	18.0,   2.00 },
        { "M20x1.0", 	20.0,   1.00 },
        { "M20x1.5", 	20.0,   1.50 },
        { "M20x2.0", 	20.0,   2.00 },
        { "M22x1.0", 	22.0,   1.00 },
        { "M22x1.5", 	22.0,   1.50 },
        { "M22x2.0", 	22.0,   2.00 },
        { "M24x1.0", 	24.0,   1.00 },
        { "M24x1.5", 	24.0,   1.50 },
        { "M24x2.0", 	24.0,   2.00 },
        { "M25x1.0", 	25.0,   1.00 },
        { "M25x1.5", 	25.0,   1.50 },
        { "M25x2.0", 	25.0,   2.00 },
        { "M27x1.0", 	27.0,   1.00 },
        { "M27x1.5", 	27.0,   1.50 },
        { "M27x2.0", 	27.0,   2.00 },
        { "M28x1.0", 	28.0,   1.00 },
        { "M28x1.5", 	28.0,   1.50 },
        { "M28x2.0", 	28.0,   2.00 },
        { "M30x1.0", 	30.0,   1.00 },
        { "M30x1.5", 	30.0,   1.50 },
        { "M30x2.0", 	30.0,   2.00 },
        { "M30x3.0", 	30.0,   3.00 },
        { "M32x1.5", 	32.0,   1.50 },
        { "M32x2.0", 	32.0,   2.00 },
        { "M33x1.5", 	33.0,   1.50 },
        { "M33x2.0", 	33.0,   2.00 },
        { "M33x3.0", 	33.0,   3.00 },
        { "M35x1.5", 	35.0,   1.50 },
        { "M35x2.0", 	35.0,   2.00 },
        { "M36x1.5", 	36.0,   1.50 },
        { "M36x2.0", 	36.0,   2.00 },
        { "M36x3.0", 	36.0,   3.00 },
        { "M39x1.5", 	39.0,   1.50 },
        { "M39x2.0", 	39.0,   2.00 },
        { "M39x3.0", 	39.0,   3.00 },
        { "M40x1.5", 	40.0,   1.50 },
        { "M40x2.0", 	40.0,   2.00 },
        { "M40x3.0", 	40.0,   3.00 },
        { "M42x1.5", 	42.0,   1.50 },
        { "M42x2.0", 	42.0,   2.00 },
        { "M42x3.0", 	42.0,   3.00 },
        { "M42x4.0", 	42.0,   4.00 },
        { "M45x1.5", 	45.0,   1.50 },
        { "M45x2.0", 	45.0,   2.00 },
        { "M45x3.0", 	45.0,   3.00 },
        { "M45x4.0", 	45.0,   4.00 },
        { "M48x1.5", 	48.0,   1.50 },
        { "M48x2.0", 	48.0,   2.00 },
        { "M48x3.0", 	48.0,   3.00 },
        { "M48x4.0", 	48.0,   4.00 },
        { "M50x1.5", 	50.0,   1.50 },
        { "M50x2.0", 	50.0,   2.00 },
        { "M50x3.0", 	50.0,   3.00 },
        { "M52x1.5", 	52.0,   1.50 },
        { "M52x2.0", 	52.0,   2.00 },
        { "M52x3.0", 	52.0,   3.00 },
        { "M52x4.0", 	52.0,   4.00 },
        { "M55x1.5", 	55.0,   1.50 },
        { "M55x2.0", 	55.0,   2.00 },
        { "M55x3.0", 	55.0,   3.00 },
        { "M55x4.0", 	55.0,   4.00 },
        { "M56x1.5", 	56.0,   1.50 },
        { "M56x2.0", 	56.0,   2.00 },
        { "M56x3.0", 	56.0,   3.00 },
        { "M56x4.0", 	56.0,   4.00 },
        { "M58x1.5", 	58.0,   1.50 },
        { "M58x2.0", 	58.0,   2.00 },
        { "M58x3.0", 	58.0,   3.00 },
        { "M58x4.0", 	58.0,   4.00 },
        { "M60x1.5", 	60.0,   1.50 },
        { "M60x2.0", 	60.0,   2.00 },
        { "M60x3.0", 	60.0,   3.00 },
        { "M60x4.0", 	60.0,   4.00 },
        { "M62x1.5", 	62.0,   1.50 },
        { "M62x2.0", 	62.0,   2.00 },
        { "M62x3.0", 	62.0,   3.00 },
        { "M62x4.0", 	62.0,   4.00 },
        { "M64x1.5", 	64.0,   1.50 },
        { "M64x2.0", 	64.0,   2.00 },
        { "M64x3.0", 	64.0,   3.00 },
        { "M64x4.0", 	64.0,   4.00 },
        { "M65x1.5", 	65.0,   1.50 },
        { "M65x2.0", 	65.0,   2.00 },
        { "M65x3.0", 	65.0,   3.00 },
        { "M65x4.0", 	65.0,   4.00 },
        { "M68x1.5", 	68.0,   1.50 },
        { "M68x2.0", 	68.0,   2.00 },
        { "M68x3.0", 	68.0,   3.00 },
        { "M68x4.0", 	68.0,   4.00 },
        { "M70x1.5", 	70.0,   1.50 },
        { "M70x2.0", 	70.0,   2.00 },
        { "M70x3.0", 	70.0,   3.00 },
        { "M70x4.0", 	70.0,   4.00 },
        { "M70x6.0", 	70.0,   6.00 },
        { "M72x1.5", 	72.0,   1.50 },
        { "M72x2.0", 	72.0,   2.00 },
        { "M72x3.0", 	72.0,   3.00 },
        { "M72x4.0", 	72.0,   4.00 },
        { "M72x6.0", 	72.0,   6.00 },
        { "M75x1.5", 	75.0,   1.50 },
        { "M75x2.0", 	75.0,   2.00 },
        { "M75x3.0", 	75.0,   3.00 },
        { "M75x4.0", 	75.0,   4.00 },
        { "M75x6.0", 	75.0,   6.00 },
        { "M76x1.5", 	76.0,   1.50 },
        { "M76x2.0", 	76.0,   2.00 },
        { "M76x3.0", 	76.0,   3.00 },
        { "M76x4.0", 	76.0,   4.00 },
        { "M76x6.0", 	76.0,   6.00 },
        { "M80x1.5", 	80.0,   1.50 },
        { "M80x2.0", 	80.0,   2.00 },
        { "M80x3.0", 	80.0,   3.00 },
        { "M80x4.0", 	80.0,   4.00 },
        { "M80x6.0", 	80.0,   6.00 },
        { "M85x2.0", 	85.0,   2.00 },
        { "M85x3.0", 	85.0,   3.00 },
        { "M85x4.0", 	85.0,   4.00 },
        { "M85x6.0", 	85.0,   6.00 },
        { "M90x2.0", 	90.0,   2.00 },
        { "M90x3.0", 	90.0,   3.00 },
        { "M90x4.0", 	90.0,   4.00 },
        { "M90x6.0", 	90.0,   6.00 },
        { "M95x2.0", 	95.0,   2.00 },
        { "M95x3.0", 	95.0,   3.00 },
        { "M95x4.0", 	95.0,   4.00 },
        { "M95x6.0", 	95.0,   6.00 },
        { "M100x2.0", 	100.0,  2.00 },
        { "M100x3.0", 	100.0,  3.00 },
        { "M100x4.0", 	100.0,  4.00 },
        { "M100x6.0", 	100.0,  6.00 }
     },
    /* UNC */
    {
        { "#1",         1.8542, 0.3969 },
        { "#2",         2.1844, 0.4536 },
        { "#3",         2.5146, 0.5292 },
        { "#4",         2.8448, 0.6350 },
        { "#5",         3.1750, 0.6350 },
        { "#6",         3.5052, 0.7938 },
        { "#8",         4.1656, 0.7938 },
        { "#10",        4.8260, 1.0583 },
        { "#12",        5.4864, 1.0583 },
        { "1/4",        6.3500, 1.2700 },
        { "5/16",       7.9375, 1.4111 },
        { "3/8",        9.5250, 1.5875 },
        { "7/16",       11.1125, 1.8143 },
        { "1/2",        12.7000, 1.9538 },
        { "9/16",       14.2875, 2.1167 },
        { "5/8",        15.8750, 2.3091 },
        { "3/4",        19.0500, 2.5400 },
        { "7/8",        22.2250, 2.8222 },
        { "1",          25.4000, 3.1750 },
    },
    /* UNF */
    {
        { "#0",         1.5240, 0.3175 },
        { "#1",         1.8542, 0.3528 },
        { "#2",         2.1844, 0.3969 },
        { "#3",         2.5146, 0.4536 },
        { "#4",         2.8448, 0.5292 },
        { "#5",         3.1750, 0.5773 },
        { "#6",         3.5052, 0.6350 },
        { "#8",         4.1656, 0.7056 },
        { "#10",        4.8260, 0.7938 },
        { "#12",        5.4864, 0.9071 },
        { "1/4",        6.3500, 0.9071 },
        { "5/16",       7.9375, 1.0583 },
        { "3/8",        9.5250, 1.0583 },
        { "7/16",       11.1125, 1.2700 },
        { "1/2",        12.7000, 1.2700 },
        { "9/16",       14.2875, 1.4111 },
        { "5/8",        15.8750, 1.4111 },
        { "3/4",        19.0500, 1.5875 },
        { "7/8",        22.2250, 1.8143 },
        { "1",          25.4000, 2.1167 },
    }    ,
    /* UNEF */
    {
        { "#12",        5.4864, 0.7938 },
        { "1/4",        6.3500, 0.7938 },
        { "5/16",       7.9375, 0.7938 },
        { "3/8",        9.5250, 0.7938 },
        { "7/16",       11.1125, 0.9071 },
        { "1/2",        12.7000, 0.9071 },
        { "9/16",       14.2875, 1.0583 },
        { "5/8",        15.8750, 1.0583 },
        { "3/4",        19.0500, 1.2700 },
        { "7/8",        22.2250, 1.2700 },
        { "1",          25.4000, 1.2700 },
    }

};

/* ISO coarse metric enums */
const char* Hole::HoleCutType_ISOmetric_Enums[]  = { "None", "Counterbore", "Countersink", "Cheesehead", "Countersink socket screw", "Cap screw", NULL};
const char* Hole::ThreadSize_ISOmetric_Enums[]   = { "M1.60", "M2",  "M2.50", "M3",
                                                     "M3.50", "M4",  "M5",    "M6",
                                                     "M8", 	  "M10", "M12",   "M14",
                                                     "M16",   "M20", "M22",   "M24",
                                                     "M27",   "M30", "M36",   "M42",
                                                     "M48",   "M56", "M64",   "M68", NULL };
const char* Hole::ThreadClass_ISOmetric_Enums[]  = { "4G", "4H", "5G", "5H", "6G", "6H", "7G", "7H","8G", "8H", NULL };

const char* Hole::HoleCutType_ISOmetricfine_Enums[]  = { "None", "Counterbore", "Countersink", "Cheesehead", "Countersink socket screw", "Cap screw", NULL};
const char* Hole::ThreadSize_ISOmetricfine_Enums[]   = {
    "M1.0x0.2",    "M1.1x0.2",    "M1.2x0.2",    "M1.4z0.2",
    "M1.6x0.2",    "M1.8x0.2",    "M2x0.25",     "M2.2x0.25",
    "M2.5x0.35",   "M3x0.35",     "M3.5x0.35",
    "M4x0.5",      "M4.5x0.5",    "M5x0.5",      "M5.5x0.5",
    "M6x0.75",     "M7x0.75",     "M8x0.75",     "M8x1.0",
    "M9x0.75",     "M9x 1",       "M10x0.75",    "M10x1",
    "M10x1.25",    "M11x0.75",    "M11x1",       "M12x1",
    "M12x1.25",    "M12x1.5",     "M14x1.0",     "M14x1.25",
    "M14x1.5",     "M15x1",       "M15x1.5",     "M16x1",
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
                                               "1/4", "5/16", "3/8", "7/16", "1/2",
                                               "9/16", "5/8", "3/4", "7/8", "1", NULL };
const char* Hole::ThreadClass_UNC_Enums[]  = { "1B", "2B", "3B", NULL };

/* UTS fine */
const char* Hole::HoleCutType_UNF_Enums[]  = { "None", "Counterbore", "Countersink", NULL};
const char* Hole::ThreadSize_UNF_Enums[]   = { "#1", "#2", "#3", "#4", "#5", "#6",
                                               "#8", "#10", "#12",
                                               "1/4", "5/16", "3/8", "7/16", "1/2",
                                               "9/16", "5/8", "3/4", "7/8", "1", NULL };
const char* Hole::ThreadClass_UNF_Enums[]  = { "1B", "2B", "3B", NULL };

/* UTS extrafine */
const char* Hole::HoleCutType_UNEF_Enums[] = { "None", "Counterbore", "Countersink", NULL};
const char* Hole::ThreadSize_UNEF_Enums[]  = { "#12", "1/4", "5/16", "3/8", "7/16", "1/2",
                                               "9/16", "5/8", "3/4", "7/8", "1", NULL };
const char* Hole::ThreadClass_UNEF_Enums[] = { "1B", "2B", "3B", NULL };

const char* Hole::ThreadDirectionEnums[]  = { "Right", "Left", NULL};

PROPERTY_SOURCE(PartDesign::Hole, PartDesign::ProfileBased)

Hole::Hole()
{
    addSubType = FeatureAddSub::Subtractive;

    ADD_PROPERTY_TYPE(Threaded, ((long)0), "Hole", App::Prop_None, "Threaded");

    ADD_PROPERTY_TYPE(ModelActualThread, ((long)0), "Hole", App::Prop_None, "Model actual thread");
    ADD_PROPERTY_TYPE(ThreadPitch, ((long)0), "Hole", App::Prop_None, "Thread pitch");
    ADD_PROPERTY_TYPE(ThreadAngle, ((long)0), "Hole", App::Prop_None, "Thread angle");
    ADD_PROPERTY_TYPE(ThreadCutOffInner, ((long)0), "Hole", App::Prop_None, "Thread CutOff Inner");
    ADD_PROPERTY_TYPE(ThreadCutOffOuter, ((long)0), "Hole", App::Prop_None, "Thread CutOff Outer");

    ADD_PROPERTY_TYPE(ThreadType, ((long)0), "Hole", App::Prop_None, "Thread type");
    ThreadType.setEnums(ThreadTypeEnums);

    ADD_PROPERTY_TYPE(ThreadSize, ((long)0), "Hole", App::Prop_None, "Thread size");
    ThreadSize.setEnums(ThreadSize_None_Enums);

    ADD_PROPERTY_TYPE(ThreadClass, ((long)0), "Hole", App::Prop_None, "Thread class");
    ThreadClass.setEnums(ThreadClass_None_Enums);

    ADD_PROPERTY_TYPE(ThreadFit, ((long)0), "Hole", App::Prop_None, "Thread fit");
    ThreadFit.setEnums(ThreadFitEnums);

    ADD_PROPERTY_TYPE(Diameter, (6.0), "Hole", App::Prop_None, "Diameter");

    ADD_PROPERTY_TYPE(ThreadDirection, ((long)0), "Hole", App::Prop_None, "Thread direction");
    ThreadDirection.setEnums(ThreadDirectionEnums);

    ADD_PROPERTY_TYPE(HoleCutType, ((long)0), "Hole", App::Prop_None, "Head cut type");
    HoleCutType.setEnums(HoleCutType_None_Enums);

    ADD_PROPERTY_TYPE(HoleCutDiameter, (0.0), "Hole", App::Prop_None, "Head cut diameter");

    ADD_PROPERTY_TYPE(HoleCutDepth, (0.0), "Hole", App::Prop_None, "Head cut deth");

    ADD_PROPERTY_TYPE(HoleCutCountersinkAngle, (90.0), "Hole", App::Prop_None, "Head cut countersink angle");

    ADD_PROPERTY_TYPE(DepthType, ((long)0), "Hole", App::Prop_None, "Type");
    DepthType.setEnums(DepthTypeEnums);

    ADD_PROPERTY_TYPE(Depth, (25.0), "Hole", App::Prop_None, "Length");

    ADD_PROPERTY_TYPE(DrillPoint, ((long)1), "Hole", App::Prop_None, "Drill point type");
    DrillPoint.setEnums(DrillPointEnums);

    ADD_PROPERTY_TYPE(DrillPointAngle, (118.0), "Hole", App::Prop_None, "Drill point angle");

    ADD_PROPERTY_TYPE(Tapered, ((bool)false),"Hole",  App::Prop_None, "Tapered");

    ADD_PROPERTY_TYPE(TaperedAngle, (90.0), "Hole", App::Prop_None, "Tapered angle");
}

void Hole::updateHoleCutParams()
{
    std::string threadType = ThreadType.getValueAsString();

    if (threadType == "ISOMetricProfile" || threadType == "ISOMetricFineProfile") {
        std::string holeCutType = HoleCutType.getValueAsString();
        if (ThreadType.getValue() < 0)
            throw Base::IndexError("Thread type out of range");
        if (ThreadSize.getValue() < 0)
            throw Base::IndexError("Thread size out of range");
        double diameter = PartDesign::Hole::threadDescription[ThreadType.getValue()][ThreadSize.getValue()].diameter;
        double f = 1.0;
        double depth = 0;

        if (holeCutType == "Counterbore") {
            f = 2.0;
            depth = 0.6;
        }
        else if (holeCutType == "Countersink") {
            f = 2.0;
            depth = 0;
        }
        else if (holeCutType == "Cheesehead") {
            f = 1.6;
            depth = 0.6;
        }
        else if (holeCutType == "Countersink socket screw") {
            f = 2.0;
            depth = 0;
        }
        else if (holeCutType == "Cap screw") {
            f = 1.5;
            depth = 1.25;
        }
        HoleCutDiameter.setValue(diameter * f);
        HoleCutDepth.setValue(diameter * depth);
    }
}

void Hole::updateDiameterParam()
{
    // Diameter parameter depends on Threaded, ThreadType, ThreadSize, and ThreadFit

    int threadType = ThreadType.getValue();
    int threadSize = ThreadSize.getValue();
    if (threadType < 0)
        throw Base::IndexError("Thread type out of range");
    if (threadSize < 0)
        throw Base::IndexError("Thread size out of range");
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

        /* Use thread tap diameter, normall D - pitch */
        diameter = diameter - pitch;
    }
    else {
        switch ( ThreadFit.getValue() ) {
        case 0: /* standard */
            diameter = ( 5 * ( (int)( ( diameter * 110 ) / 5 ) ) ) / 100.0;
            break;
        case 1: /* close */
            diameter = ( 5 * ( (int)( ( diameter * 105 ) / 5 ) ) ) / 100.0;
            break;
        default:
            assert( 0 );
        }
    }
    Diameter.setValue(diameter);
}

void Hole::onChanged(const App::Property *prop)
{
    if (prop == &ThreadType) {
        std::string type(ThreadType.getValueAsString());
        std::string holeCutType(HoleCutType.getValueAsString());

        if (type == "None" ) {
            ThreadSize.setEnums(ThreadSize_None_Enums);
            ThreadClass.setEnums(ThreadClass_None_Enums);
            HoleCutType.setEnums(HoleCutType_None_Enums);
            Threaded.setReadOnly(true);
            ThreadSize.setReadOnly(true);
            ThreadFit.setReadOnly(true);
            ThreadClass.setReadOnly(true);
            Diameter.setReadOnly(false);

            if (holeCutType == "None") {
                HoleCutDiameter.setReadOnly(true);
                HoleCutDepth.setReadOnly(true);
                HoleCutCountersinkAngle.setReadOnly(true);
            }
            else if (holeCutType == "Counterbore") {
                HoleCutDiameter.setReadOnly(false);
                HoleCutDepth.setReadOnly(false);
                HoleCutCountersinkAngle.setReadOnly(true);

            }
            else if (holeCutType == "Countersink") {
                HoleCutDiameter.setReadOnly(false);
                HoleCutDepth.setReadOnly(true);
                HoleCutCountersinkAngle.setReadOnly(false);
            }
            Threaded.setValue(0);
        }
        else if ( type == "ISOMetricProfile" ) {
            ThreadSize.setEnums(ThreadSize_ISOmetric_Enums);
            ThreadClass.setEnums(ThreadClass_ISOmetric_Enums);
            HoleCutType.setEnums(HoleCutType_ISOmetric_Enums);
            Threaded.setReadOnly(false);
            ThreadSize.setReadOnly(false);
            ThreadFit.setReadOnly(false);
            ThreadClass.setReadOnly(false);
            Diameter.setReadOnly(true);
            HoleCutDiameter.setReadOnly(true);
            HoleCutDepth.setReadOnly(true);
            HoleCutCountersinkAngle.setReadOnly(true);
        }
        else if ( type == "ISOMetricFineProfile" ) {
            ThreadSize.setEnums(ThreadSize_ISOmetricfine_Enums);
            ThreadClass.setEnums(ThreadClass_ISOmetricfine_Enums);
            HoleCutType.setEnums(HoleCutType_ISOmetricfine_Enums);
            Threaded.setReadOnly(false);
            ThreadSize.setReadOnly(false);
            ThreadFit.setReadOnly(false);
            ThreadClass.setReadOnly(false);
            Diameter.setReadOnly(true);
            HoleCutDiameter.setReadOnly(true);
            HoleCutDepth.setReadOnly(true);
            HoleCutCountersinkAngle.setReadOnly(true);
        }
        else if ( type == "UNC" ) {
            ThreadSize.setEnums(ThreadSize_UNC_Enums);
            ThreadClass.setEnums(ThreadClass_UNC_Enums);
            HoleCutType.setEnums(HoleCutType_UNC_Enums);
            Threaded.setReadOnly(false);
            ThreadSize.setReadOnly(false);
            ThreadFit.setReadOnly(false);
            ThreadClass.setReadOnly(false);
            Diameter.setReadOnly(true);

            if (holeCutType == "None") {
                HoleCutDiameter.setReadOnly(true);
                HoleCutDepth.setReadOnly(true);
                HoleCutCountersinkAngle.setReadOnly(true);
            }
            else if (holeCutType == "Counterbore") {
                HoleCutDiameter.setReadOnly(false);
                HoleCutDepth.setReadOnly(false);
                HoleCutCountersinkAngle.setReadOnly(true);

            }
            else if (holeCutType == "Countersink") {
                HoleCutDiameter.setReadOnly(false);
                HoleCutDepth.setReadOnly(true);
                HoleCutCountersinkAngle.setReadOnly(false);
            }
        }
        else if ( type == "UNF" ) {
            ThreadSize.setEnums(ThreadSize_UNF_Enums);
            ThreadClass.setEnums(ThreadClass_UNF_Enums);
            HoleCutType.setEnums(HoleCutType_UNF_Enums);
            Threaded.setReadOnly(false);
            ThreadSize.setReadOnly(false);
            ThreadFit.setReadOnly(false);
            ThreadClass.setReadOnly(false);
            Diameter.setReadOnly(true);

            if (holeCutType == "None") {
                HoleCutDiameter.setReadOnly(true);
                HoleCutDepth.setReadOnly(true);
                HoleCutCountersinkAngle.setReadOnly(true);
            }
            else if (holeCutType == "Counterbore") {
                HoleCutDiameter.setReadOnly(false);
                HoleCutDepth.setReadOnly(false);
                HoleCutCountersinkAngle.setReadOnly(true);

            }
            else if (holeCutType == "Countersink") {
                HoleCutDiameter.setReadOnly(false);
                HoleCutDepth.setReadOnly(true);
                HoleCutCountersinkAngle.setReadOnly(false);
            }
        }
        else if ( type == "UNEF" ) {
            ThreadSize.setEnums(ThreadSize_UNEF_Enums);
            ThreadClass.setEnums(ThreadClass_UNEF_Enums);
            HoleCutType.setEnums(HoleCutType_UNEF_Enums);
            Threaded.setReadOnly(false);
            ThreadSize.setReadOnly(false);
            ThreadFit.setReadOnly(false);
            ThreadClass.setReadOnly(false);
            Diameter.setReadOnly(true);

            if (holeCutType == "None") {
                HoleCutDiameter.setReadOnly(true);
                HoleCutDepth.setReadOnly(true);
                HoleCutCountersinkAngle.setReadOnly(true);
            }
            else if (holeCutType == "Counterbore") {
                HoleCutDiameter.setReadOnly(false);
                HoleCutDepth.setReadOnly(false);
                HoleCutCountersinkAngle.setReadOnly(true);

            }
            else if (holeCutType == "Countersink") {
                HoleCutDiameter.setReadOnly(false);
                HoleCutDepth.setReadOnly(true);
                HoleCutCountersinkAngle.setReadOnly(false);
            }
        }

        if (type == "ISOMetricProfile" || type == "ISOMetricFineProfile")
            HoleCutCountersinkAngle.setValue(90.0);
        else if (type == "UNC" || type == "UNF" || type == "UNEF")
            HoleCutCountersinkAngle.setValue(82.0);

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

        if (Threaded.getValue()) {
            ThreadDirection.setReadOnly(false);
            ThreadFit.setReadOnly(true);
            ModelActualThread.setReadOnly(true); // For now set this one to read only
        }
        else {
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
        if (DrillPoint.getValue() == 1)
            DrillPointAngle.setReadOnly(false);
        else
            DrillPointAngle.setReadOnly(true);
    }
    else if (prop == &Tapered) {
        if (Tapered.getValue())
            TaperedAngle.setReadOnly(false);
        else
            TaperedAngle.setReadOnly(true);
    }
    else if (prop == &ThreadSize) {
        updateDiameterParam();
        updateHoleCutParams();
    }
    else if (prop == &ThreadFit) {
        updateDiameterParam();
    }
    else if (prop == &HoleCutType) {
        std::string threadType = ThreadType.getValueAsString();
        std::string holeCutType = HoleCutType.getValueAsString();
        bool holeCutEnable = ( threadType != "ISOMetricProfile" &&
                threadType !="ISOMetricFineProfile" &&
                (holeCutType != "None"));

        HoleCutDiameter.setReadOnly(!holeCutEnable);

        if (holeCutType == "Countersink" || holeCutType == "Countersink socket screw")
            HoleCutDepth.setReadOnly(true);
        else
            HoleCutDepth.setReadOnly(!holeCutEnable);

        if (holeCutType != "Countersink" && holeCutType != "Countersink socket screw")
            HoleCutCountersinkAngle.setReadOnly(true);
        else
            HoleCutCountersinkAngle.setReadOnly(!holeCutEnable);

        updateHoleCutParams();
    }
    else if (prop == &DepthType) {
        Depth.setReadOnly((std::string(DepthType.getValueAsString()) != "Dimension"));
    }
    ProfileBased::onChanged(prop);
}

/**
  * Compute 2D intersection between the lines (pa1, pa2) and (pb1, pb2).
  * The lines are assumed to be crossing, and it is an error
  * to specify parallel lines.
  *
  * Only the x and y coordinates are used to compute the 2D intersection.
  *
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
    TopoShape profileshape;
    try {
        profile = getVerifiedObject();
        profileshape = getVerifiedFace();
    } catch (const Base::Exception& e) {
        return new App::DocumentObjectExecReturn(e.what());
    }

    // Find the base shape
    TopoShape base;
    try {
        base = getBaseShape();
    }
    catch (const Base::Exception&) {
        std::string text(QT_TR_NOOP("The requested feature cannot be created. The reason may be that:\n\n"
                                    "  \xe2\x80\xa2 the active Body does not contain a base shape, so there is no\n"
                                    "  material to be removed;\n"
                                    "  \xe2\x80\xa2 the selected sketch does not belong to the active Body."));
        return new App::DocumentObjectExecReturn(text);
    }

    try {
        std::string method(DepthType.getValueAsString());
        double length = 0.0;

        this->positionByPrevious();
        auto invObjLoc = this->getLocation().Inverted();

        base.move(invObjLoc);

        if (profileshape.isNull())
            return new App::DocumentObjectExecReturn("Pocket: Creating a face from sketch failed");
        profileshape.move(invObjLoc);

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
            return new App::DocumentObjectExecReturn("Hole: Unsupported length specification.");

        if (length <= 0)
            return new App::DocumentObjectExecReturn("Hole: Invalid hole depth");

        BRepBuilderAPI_MakeWire mkWire;
        std::string holeCutType = HoleCutType.getValueAsString();
        bool isCountersink = (holeCutType == "Countersink" || holeCutType == "Countersink socket screw");
        bool isCounterbore = (holeCutType == "Counterbore" || holeCutType == "Cheesehead" || holeCutType == "Cap screw");
        double hasTaperedAngle = Tapered.getValue() ? Base::toRadians( TaperedAngle.getValue() ) : Base::toRadians(90.0);
        double radiusBottom = Diameter.getValue() / 2.0 - length * cos( hasTaperedAngle );
        double radius = Diameter.getValue() / 2.0;
        double holeCutRadius = HoleCutDiameter.getValue() / 2.0;
        gp_Pnt firstPoint(0, 0, 0);
        gp_Pnt lastPoint(0, 0, 0);
        double length1 = 0;

        if ( hasTaperedAngle <= 0 || hasTaperedAngle > Base::toRadians( 180.0 ) )
            return new App::DocumentObjectExecReturn("Hole: Invalid taper angle.");

        if ( isCountersink ) {
            double x, z;
            double countersinkAngle = Base::toRadians( HoleCutCountersinkAngle.getValue() / 2.0 );

            if ( countersinkAngle <= 0 || countersinkAngle > Base::toRadians( 180.0 ) )
                return new App::DocumentObjectExecReturn("Hole: Invalid countersink angle.");

            if (holeCutRadius < radius)
                return new App::DocumentObjectExecReturn("Hole: Hole cut diameter too small.");

            // Top point
            gp_Pnt newPoint = toPnt(holeCutRadius * xDir);
            mkWire.Add( BRepBuilderAPI_MakeEdge(lastPoint, newPoint) );
            lastPoint = newPoint;

            computeIntersection(gp_Pnt( holeCutRadius, 0, 0 ),
                                gp_Pnt( holeCutRadius - sin( countersinkAngle ), -cos( countersinkAngle ), 0 ),
                                gp_Pnt( radius, 0, 0 ),
                                gp_Pnt( radiusBottom, -length, 0), x, z );
            if (-length > z)
                return new App::DocumentObjectExecReturn("Hole: Invalid countersink.");

            length1 = z;

            newPoint = toPnt(x * xDir + z * zDir);
            mkWire.Add( BRepBuilderAPI_MakeEdge( lastPoint, newPoint ) );
            lastPoint = newPoint;
        }
        else if ( isCounterbore ) {
            double holeCutDepth = HoleCutDepth.getValue();
            double x, z;

            if (holeCutDepth <= 0)
                return new App::DocumentObjectExecReturn("Hole: Hole cut depth must be greater than zero.");

            if (holeCutDepth > length)
                return new App::DocumentObjectExecReturn("Hole: Hole cut depth must be less than hole depth.");

            if (holeCutRadius < radius)
                return new App::DocumentObjectExecReturn("Hole: Hole cut diameter too small.");

            // Top point
            gp_Pnt newPoint = toPnt(holeCutRadius * xDir);
            mkWire.Add(BRepBuilderAPI_MakeEdge(lastPoint, newPoint));
            lastPoint = newPoint;

            // Bottom of counterbore
            newPoint = toPnt(holeCutRadius * xDir -holeCutDepth * zDir);
            mkWire.Add(BRepBuilderAPI_MakeEdge(lastPoint, newPoint));
            lastPoint = newPoint;

            // Compute intersection of tapered edge and line at bottom of counterbore hole
            computeIntersection(gp_Pnt( 0, -holeCutDepth, 0 ),
                                gp_Pnt( holeCutRadius, -holeCutDepth, 0 ),
                                gp_Pnt( radius, 0, 0 ),
                                gp_Pnt( radiusBottom, length, 0 ), x, z );

            length1 = z;
            newPoint = toPnt(x * xDir + z * zDir);
            mkWire.Add(BRepBuilderAPI_MakeEdge(lastPoint, newPoint));
            lastPoint = newPoint;
        }
        else {
            gp_Pnt newPoint = toPnt(radius * xDir);
            mkWire.Add(BRepBuilderAPI_MakeEdge(lastPoint, newPoint));
            lastPoint = newPoint;
            length1 = 0;
        }

        std::string drillPoint = DrillPoint.getValueAsString();
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
            double x, z;
            gp_Pnt newPoint;

            if ( drillPointAngle <= 0 || drillPointAngle > Base::toRadians( 180.0 ) )
                return new App::DocumentObjectExecReturn("Hole: Invalid drill point angle.");

            computeIntersection(gp_Pnt( 0, -length, 0 ),
                                gp_Pnt( cos( drillPointAngle ), -length + sin( drillPointAngle ), 0),
                                gp_Pnt(radius, 0, 0),
                                gp_Pnt(radiusBottom, -length, 0), x, z);

            if (z > 0 || z >= length1)
                return new App::DocumentObjectExecReturn("Hole: Invalid drill point.");

            newPoint = toPnt(x * xDir + z * zDir);
            mkWire.Add(BRepBuilderAPI_MakeEdge(lastPoint, newPoint));
            lastPoint = newPoint;

            newPoint = toPnt(-length * zDir);
            mkWire.Add(BRepBuilderAPI_MakeEdge(lastPoint, newPoint));
            lastPoint = newPoint;
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
                return new App::DocumentObjectExecReturn("Hole: Resulting shape is empty");
        }
        else
            return new App::DocumentObjectExecReturn("Hole: Could not revolve sketch!");

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

        std::vector<TopoShape> holes;

        TopoShape result(0,getDocument()->getStringHasher());

        TopTools_IndexedMapOfShape edgeMap;
        TopExp::MapShapes(profileshape.getShape(), TopAbs_EDGE, edgeMap);
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

            std::string name("Edge");
            name += std::to_string(i);
            const char *mapped = profileshape.getElementName(name.c_str(),true);
            if(mapped != name.c_str())
                name = std::string(mapped);

            TopoShape hole(profile->getID());
            hole.setShape(protoHole);
            // transform and generate element map.
            hole = hole.makETransform(localSketchTransformation,mapped);
            holes.push_back(hole);

            try {
                result.makECut({base,hole});
            }catch(Standard_Failure &) {
                std::string msg("Hole: Cut failed on profile edge ");
                msg += name;
                return new App::DocumentObjectExecReturn(msg.c_str());
            }
            base = getSolid(result);
            if (base.isNull()) {
                std::string msg("Hole: Cut produced non-solid on profile edge ");
                msg += name;
                return new App::DocumentObjectExecReturn(msg.c_str());
            }
        }

        // set the subtractive shape property for later usage in e.g. pattern
        this->AddSubShape.setValue(TopoShape().makECompound(holes));

        remapSupportShape(base.getShape());

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

