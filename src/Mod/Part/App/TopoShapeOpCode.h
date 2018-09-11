/****************************************************************************
 *   Copyright (c) 2018 Zheng, Lei (realthunder) <realthunder.dev@gmail.com>*
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

#ifndef PART_TOPOSHAPE_OPCODE_H
#define PART_TOPOSHAPE_OPCODE_H

// Registration of TopoShape op code used for topological naming.
//
// These are put there just for convenience. Currently, there is no mandatory
// rules for op code naming.

#define TOPOP_TAG       "TAG"
#define TOPOP_COPY      "CPY"
#define TOPOP_TRANSFORM "XFM"
#define TOPOP_GTRANSFORM "GFM"
#define TOPOP_COMPOUND  "CMP"
#define TOPOP_COMPSOLID "CSD"
#define TOPOP_FACE      "FAC"
#define TOPOP_FILLED_FACE "FFC"
#define TOPOP_WIRE      "WIR"
#define TOPOP_EXTRUDE   "XTR"
#define TOPOP_FUSE      "FUS"
#define TOPOP_GENERAL_FUSE "GFS"
#define TOPOP_REFINE    "RFI"
#define TOPOP_BOOLEAN   "BOL"
#define TOPOP_CUT       "CUT"
#define TOPOP_COMMON    "CMN"
#define TOPOP_SECTION   "SEC"
#define TOPOP_SLICE     "SLC"
#define TOPOP_MAKER     "MAK"
#define TOPOP_FILLET    "FLT"
#define TOPOP_CHAMFER   "CHF"
#define TOPOP_THICKEN   "THK"
#define TOPOP_OFFSET    "OFS"
#define TOPOP_OFFSET2D  "OFF"
#define TOPOP_REVOLVE   "RVL"
#define TOPOP_LOFT      "LFT"
#define TOPOP_SWEEP     "SWP"
#define TOPOP_PIPE      "PIP"
#define TOPOP_PIPE_SHELL "PSH"
#define TOPOP_SHELL     "SHL"
#define TOPOP_SOLID     "SLD"
#define TOPOP_RULED_SURFACE "RSF"
#define TOPOP_MIRROR    "MIR"
#define TOPOP_SKETCH    "SKT"
#define TOPOP_SKETCH_EXPORT "SKE"
#define TOPOP_SHAPEBINDER "BND"
#define TOPOP_THRU_SECTIONS "TRU"
#define TOPOP_SEWING    "SEW"
#define TOPOP_PRISM     "PSM"
#define TOPOP_DRAFT     "DFT"
#define TOPOP_HALF_SPACE "HSP"

#endif
