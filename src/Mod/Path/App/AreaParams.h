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

/** Base parameters */
#define AREA_PARAMS_BASE \
    ((enum,fill,Fill,2,"Fill the output wires to make a face. \n"\
        "Auto means make a face if any of the children has a face.",(None)(Face)(Auto)))\
    ((enum,coplanar,Coplanar,2,"Specifies the way to check coplanar.\n"\
                               "'Force' will discard non coplaner shapes, but 'Check' only gives warning.",\
                               (None)(Check)(Force)))\
    ((bool,reorder,Reorder,false,"Re-orient closed wires in wire only shapes so that inner wires become holes."))\
    ((enum,open_mode,OpenMode,0,"Specify how to handle open wires.\n"\
                        "'None' means combin without openeration.\n"\
                        "'Edges' means separate to edges before Union.\n"\
                        "ClipperLib seems to have an urge to close open wires.",(None)(Union)(Edges)))\
    AREA_PARAMS_CLIPPER_FILL \
    ((double,deflection,Deflection,0.01,"Deflection for non circular curve discretization"))

/** libarea algorithm option parameters */
#define AREA_PARAMS_CAREA \
    ((double,tolerance,Tolerance,Precision::Confusion(),"Point coincidence tolerance"))\
    ((bool,fit_arcs,FitArcs,true,"Enable arc fitting"))\
    ((bool,clipper_simple,Simplify,false,\
        "Simplify polygons after operation. See https://goo.gl/Mh9XK1"))\
    ((double,clipper_clean_distance,CleanDistance,0.0,\
        "Clean polygon smaller than this distance. See https://goo.gl/jox3JY"))\
    ((double,accuracy,Accuracy,0.01,"Arc fitting accuracy"))\
    ((double,units,Unit,1.0,"Scaling factor for convertion to inch"))\
    ((short,min_arc_points,MinArcPoints,4,"Minimum segments for arc discretization"))\
    ((short,max_arc_points,MaxArcPoints,100,"Maximum segments for arc discretization"))\
    ((double,clipper_scale,ClipperScale,10000.0,\
        "ClipperLib operate on intergers. This is the scale factor to \nconvert floating points."))

/** Pocket parameters 
 *
 * These parameters cooresponds to CAreaPocketParams in libarea
 * */
#define AREA_PARAMS_POCKET \
    ((enum,mode,PocketMode,1,"Selects the pocket toolpath pattern",(None)(ZigZag)(Offset)(Spiral)(ZigZagOffset)))\
	((double,tool_radius,ToolRadius,1.0,"Tool radius for pocketing"))\
	((double,extra_offset,PocketExtraOffset,0.0,"Extra offset for pocketing"))\
	((double,stepover,PocketStepover,0.0,"Cutter diameter to step over on each pass. If =0, use ToolRadius."))\
	((bool,from_center,FromCenter,true,"Start pocketing from center"))\
	((double,zig_angle,ZigAngle,45,"Zig angle in degree"))

#define AREA_PARAMS_POCKET_CONF \
    ((bool,thicken,Thicken,false,"Thicken the resulting wires with ToolRadius"))

/** Operation code */
#define AREA_PARAMS_OPCODE \
    ((enum2,op,Operation,0,"Boolean operation",\
        (Union)(Difference)(Intersection)(Xor),(ClipperLib::ClipType,ClipperLib::ct)))

/** Offset parameters */
#define AREA_PARAMS_OFFSET \
    ((double,offset,Offset,0.0,"Offset value, positive for expansion, negative for shrinking"))\
    ((long,extra_pass,ExtraPass,0,"Number of extra offset pass to generate."))\
    ((double,stepover,Stepover,0.0,"Cutter diameter to step over on each pass. If =0, use Offset"))

/** Section parameters */
#define AREA_PARAMS_SECTION \
    ((long,count,SectionCount,0,"Number of sections to generate. -1 means full sections."))\
    ((double,stepdown,Stepdown,1.0,"Step down distance for each section"))\
    ((double,offset,SectionOffset,0.0,"Offset for the first section"))

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

#endif //PATH_AreaParam_H
