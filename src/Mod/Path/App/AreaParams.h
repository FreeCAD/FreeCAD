/****************************************************************************
 *   Copyright (c) 2017 Zheng, Lei (realthunder) <realthunder.dev@gmail.com>*
 *                                                                          *
 *   This file is part of the FreeCAD CAx development system.               *
 *                                                                          *
 *   This library is free software; you can redistribute it and/or          *
 *   modify it under the terms of the GNU Library General Public            *
 *   License as published by the Free Software Foundation; either           *
 *   version 2 of the License, or (at your option) any later version.       *
 *                                                                          *
 *   This library  is distributed in the hope that it will be useful,       *
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of         *
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the          *
 *   GNU Library General Public License for more details.                   *
 *                                                                          *
 *   You should have received a copy of the GNU Library General Public      *
 *   License along with this library; see the file COPYING.LIB. If not,     *
 *   write to the Free Software Foundation, Inc., 59 Temple Place,          *
 *   Suite 330, Boston, MA  02111-1307, USA                                 *
 *                                                                          *
 ****************************************************************************/

#ifndef PATH_AreaParams_H
#define PATH_AreaParams_H

// deifne this to enable offset algo selection
// #define AREA_OFFSET_ALGO

/** \file
 * Parameters definition for Path::Area and its companion 
 * See \ref ParamPage "here" for details of parameter definition.
 */

#include "ParamsHelper.h"

/** clipper fill type */
#define AREA_CLIPPER_FILL_TYPE \
    (NonZero)(EvenOdd)(Positive)(Negative),(ClipperLib::PolyFillType,ClipperLib::pft)

/** Paramerters of clipper fill types */
#define AREA_PARAMS_CLIPPER_FILL \
    ((enum2,subject_fill,SubjectFill,0,\
        "ClipperLib subject fill type. \nSee https://goo.gl/5pYQQP",AREA_CLIPPER_FILL_TYPE))\
    ((enum2,clip_fill,ClipFill,0,\
        "ClipperLib clip fill type. \nSee https://goo.gl/5pYQQP",AREA_CLIPPER_FILL_TYPE))

/** Deflection parameter */
#define AREA_PARAMS_DEFLECTION \
    ((double,deflection,Deflection,0.01,\
        "Deflection for non circular curve discretization. It also also used for\n"\
        "discretizing circular wires when you 'Explode' the shape for wire operations"))

/** Base parameters */
#define AREA_PARAMS_BASE \
    ((enum,fill,Fill,2,"Fill the output wires to make a face. \n"\
        "Auto means make a face if any of the children has a face.",(None)(Face)(Auto)))\
    ((enum,coplanar,Coplanar,2,\
        "Specifies the way to check coplanar. 'Force' will discard non coplaner shapes,\n"\
        "but 'Check' only gives warning.",(None)(Check)(Force)))\
    ((bool,reorient,Reorient,true,\
        "Re-orient closed wires in wire only shapes so that inner wires become holes."))\
    ((bool,explode,Explode,false,\
        "If true, Area will explode the first shape into disconnected open edges, \n"\
        "with all curves discretized, so that later operations like 'Difference' \n"\
        "behave like wire cutting. Without exploding, 'Difference' in ClipperLib\n"\
        "behave like face cutting."))\
    ((enum,open_mode,OpenMode,0,\
        "Specify how to handle open wires. 'None' means combin without openeration.\n"\
        "'Edges' means separate to edges before Union. ClipperLib seems to have an.\n"\
        "urge to close open wires.",(None)(Union)(Edges)))\
    AREA_PARAMS_DEFLECTION \
    AREA_PARAMS_CLIPPER_FILL 

#define AREA_PARAMS_FIT_ARCS \
    ((bool,fit_arcs,FitArcs,true,"Enable arc fitting"))

/** libarea algorithm option parameters */
#define AREA_PARAMS_CAREA \
    ((double,tolerance,Tolerance,Precision::Confusion(),"Point coincidence tolerance"))\
    AREA_PARAMS_FIT_ARCS \
    ((bool,clipper_simple,Simplify,false,\
        "Simplify polygons after operation. See https://goo.gl/Mh9XK1"))\
    ((double,clipper_clean_distance,CleanDistance,0.0,\
        "Clean polygon smaller than this distance. See https://goo.gl/jox3JY"))\
    ((double,accuracy,Accuracy,0.01,"Arc fitting accuracy"))\
    ((double,units,Unit,1.0,"Scaling factor for convertion to inch"))\
    ((short,min_arc_points,MinArcPoints,4,"Minimum segments for arc discretization"))\
    ((short,max_arc_points,MaxArcPoints,100,"Maximum segments for arc discretization"))\
    ((double,clipper_scale,ClipperScale,10000.0,\
        "ClipperLib operate on intergers. This is the scale factor to convert\n"\
        "floating points."))

/** Pocket parameters 
 *
 * These parameters cooresponds to CAreaPocketParams in libarea
 * */
#define AREA_PARAMS_POCKET \
    ((enum,mode,PocketMode,0,"Selects the pocket toolpath pattern",(None)(ZigZag)(Offset)(Spiral)(ZigZagOffset)))\
	((double,tool_radius,ToolRadius,1.0,"Tool radius for pocketing"))\
	((double,extra_offset,PocketExtraOffset,0.0,"Extra offset for pocketing"))\
	((double,stepover,PocketStepover,0.0,"Cutter diameter to step over on each pass. If =0, use ToolRadius."))\
	((bool,from_center,FromCenter,true,"Start pocketing from center"))\
	((double,zig_angle,ZigAngle,45,"Zig angle in degree"))

#define AREA_PARAMS_POCKET_CONF \
    ((bool,thicken,Thicken,false,"Thicken the resulting wires with ToolRadius"))

/** Operation code */
#define AREA_PARAMS_OPCODE \
    ((enum,op,Operation,0,"Boolean operation.\n"\
        "For the first four operations, see https://goo.gl/Gj8RUu.\n"\
        "'Compound' means no operation, normally used to do Area.sortWires().",\
        (Union)(Difference)(Intersection)(Xor)(Compound)))

/** Offset parameters */
#define AREA_PARAMS_OFFSET \
    ((double,offset,Offset,0.0,"Offset value, positive for expansion, negative for shrinking"))\
    ((long,extra_pass,ExtraPass,0,"Number of extra offset pass to generate."))\
    ((double,stepover,Stepover,0.0,"Cutter diameter to step over on each pass. If =0, use Offset"))

#define AREA_PARAMS_SECTION_EXTRA \
    ((enum,mode,SectionMode,2,"Section offset coordinate mode.\n"\
        "'Absolute' means the absolute Z height to start section.\n"\
        "'BoundBox' means relative Z height to the bounding box of all the children shape. Only\n"\
        "positive value is allowed, which specifies the offset below the top Z of the bounding box.\n"\
        "Note that OCC has trouble getting the minimumi bounding box of some solids, particually\n"\
        "those with non-planar surface.\n"\
        "'Workplane' means relative to workplane.",\
        (Absolute)(BoundBox)(Workplane)))

/** Section parameters */
#define AREA_PARAMS_SECTION \
    ((long,count,SectionCount,0,"Number of sections to generate. -1 means full sections."))\
    ((double,stepdown,Stepdown,1.0,"Step down distance for each section"))\
    ((double,offset,SectionOffset,0.0,"Offset for the first section"))\
    AREA_PARAMS_SECTION_EXTRA

#ifdef AREA_OFFSET_ALGO
#   define AREA_PARAMS_OFFSET_ALGO \
    ((enum,algo,Algo,0,"Offset algorithm type",(Clipper)(libarea)))
#else
#   define AREA_PARAMS_OFFSET_ALGO
#endif

/** Offset configuration parameters */
#define AREA_PARAMS_OFFSET_CONF \
    AREA_PARAMS_OFFSET_ALGO \
    ((enum2,join_type,JoinType,0,"ClipperOffset join type. \nSee https://goo.gl/4odfQh",\
        (Round)(Square)(Miter),(ClipperLib::JoinType,ClipperLib::jt)))\
    ((enum2,end_type,EndType,0,"\nClipperOffset end type. See https://goo.gl/tj7gkX",\
        (OpenRound)(ClosedPolygon)(ClosedLine)(OpenSquare)(OpenButt),(ClipperLib::EndType,ClipperLib::et)))\
    ((double,miter_limit,MiterLimit,2.0,"Miter limit for joint type Miter. See https://goo.gl/K8xX9h"))\
    ((double,round_precision,RoundPreceision,0.0,\
        "Round joint precision. If =0, it defaults to Accuracy. \nSee https://goo.gl/4odfQh"))

#define AREA_PARAMS_MIN_DIST \
    ((double, min_dist, MinDistance, 0.0, \
        "minimum distance for the generated new wires. Wires maybe broken if the\n"\
        "algorithm see fits. Set to zero to disable wire breaking."))

/** Area wire sorting parameters */
#define AREA_PARAMS_SORT \
    ((enum, sort_mode, SortMode, 1, "Wire sorting mode to optimize travel distance.\n"\
        "'2D5' explode shapes into wires, and groups the shapes by its plane. The 'start' position\n"\
        "chooses the first plane to start. The algorithm will then sort within the plane and then\n"\
        "move on to the next nearest plane.\n"\
        "'3D' makes no assumption of planarity. The sorting is done across 3D space\n",\
        (None)(2D5)(3D)))\
    AREA_PARAMS_MIN_DIST

/** Area path generation parameters */
#define AREA_PARAMS_PATH \
    AREA_PARAMS_SORT \
    ((double, threshold, RetractThreshold, 0.0,\
        "If two wire's end points are separated within this threshold, they are consider\n"\
        "as connected. You may want to set this to the tool diameter to keep the tool down."))\
    ((double, height, RetractHeight, 0.0,"Tool retraction absolute height"))\
    ((double, clearance, Clearance, 0.0,\
        "When return from last retraction, this gives the pause height relative to the Z\n"\
        "value of the next move"))\
    ((double,segmentation,Segmentation,0.0,\
        "Break long curves into segments of this length. One use case is for PCB autolevel,\n"\
        "so that more correction points can be inserted"))

#define AREA_PARAMS_PATH_EXTRA \
    AREA_PARAMS_DEFLECTION \
    AREA_PARAMS_FIT_ARCS

#define AREA_PARAMS_PATH_CONF \
    AREA_PARAMS_PATH \
    AREA_PARAMS_PATH_EXTRA 

/** Group of all Area configuration parameters except CArea's*/
#define AREA_PARAMS_AREA \
    AREA_PARAMS_BASE \
    AREA_PARAMS_OFFSET \
    AREA_PARAMS_OFFSET_CONF \
    AREA_PARAMS_POCKET \
    AREA_PARAMS_POCKET_CONF \
    AREA_PARAMS_SECTION

/** Group of all Area configuration parameters */
#define AREA_PARAMS_CONF \
    AREA_PARAMS_CAREA \
    AREA_PARAMS_AREA

/** Group of all Area parameters */
#define AREA_PARAMS_ALL \
    AREA_PARAMS_CONF \
    AREA_PARAMS_OPCODE

#define AREA_PARAM_LOG_LEVEL (Error)(Warning)(Log)(Trace)
#define AREA_PARAMS_LOG_LEVEL \
    ((enum, log_level, LogLevel, 1, "Area log level", AREA_PARAM_LOG_LEVEL))

#define AREA_PARAMS_EXTRA_CONF \
    AREA_PARAMS_LOG_LEVEL 

#define AREA_PARAMS_STATIC_CONF \
    AREA_PARAMS_CONF \
    AREA_PARAMS_EXTRA_CONF

#endif //PATH_AreaParam_H
