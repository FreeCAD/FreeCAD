// SPDX-License-Identifier: LGPL-2.1-or-later

/****************************************************************************
 *   Copyright (c) 2018 Zheng Lei (realthunder) <realthunder.dev@gmail.com> *
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

#pragma once

#include <Mod/Part/PartGlobal.h>

/** Definition of commonly used TopoShape operational code
 *
 * The operational code (op code) is encoded into the mapped element name to
 * provide more context and meaning to the  shape history when tracing it back
 * to its predecessors. Some op codes can be passed to the generalized shape-
 * making API TopoShape::makEBoolean() to make a shape.
 */
namespace Part
{

class PartExport OpCodes
{
public:
    /** Element name encoding scheme version number
     *
     * Increase this version if there is major change in encoding scheme.
     * Opening a document containing a mismatched version number will cause the
     * element map to be regenerated after recompute
     */

    static constexpr const int Version = 15;

    /** @name Op codes that are also accepted by TopoShape::makEBoolean() */
    //@{
    static constexpr const char* Fuse = "FUS";
    static constexpr const char* Cut = "CUT";
    static constexpr const char* Common = "CMN";
    static constexpr const char* Section = "SEC";
    static constexpr const char* Xor = "XOR";
    static constexpr const char* Compound = "CMP";
    static constexpr const char* Compsolid = "CSD";
    static constexpr const char* Pipe = "PIP";
    static constexpr const char* Shell = "SHL";
    static constexpr const char* Wire = "WIR";
    //@}

    static constexpr const char* Tag = "TAG";
    static constexpr const char* Copy = "CPY";
    static constexpr const char* Transform = "XFM";
    static constexpr const char* Gtransform = "GFM";
    static constexpr const char* Face = "FAC";
    static constexpr const char* FilledFace = "FFC";
    static constexpr const char* Extrude = "XTR";
    static constexpr const char* GeneralFuse = "GFS";
    static constexpr const char* Refine = "RFI";
    static constexpr const char* Boolean = "BOL";
    static constexpr const char* Slice = "SLC";
    static constexpr const char* Maker = "MAK";
    static constexpr const char* Fillet = "FLT";
    static constexpr const char* Chamfer = "CHF";
    static constexpr const char* Thicken = "THK";
    static constexpr const char* Offset = "OFS";
    static constexpr const char* Offset2D = "OFF";
    static constexpr const char* Revolve = "RVL";
    static constexpr const char* Loft = "LFT";
    static constexpr const char* Sweep = "SWP";
    static constexpr const char* PipeShell = "PSH";
    static constexpr const char* ShellFill = "SHF";
    static constexpr const char* Solid = "SLD";
    static constexpr const char* RuledSurface = "RSF";
    static constexpr const char* Mirror = "MIR";
    static constexpr const char* Sketch = "SKT";
    static constexpr const char* SketchExport = "SKE";
    static constexpr const char* Shapebinder = "BND";
    static constexpr const char* ThruSections = "TRU";
    static constexpr const char* Sewing = "SEW";
    static constexpr const char* Prism = "PSM";
    static constexpr const char* Draft = "DFT";
    static constexpr const char* HalfSpace = "HSP";
    static constexpr const char* BSplineFace = "BSF";
    static constexpr const char* Split = "SPT";
    static constexpr const char* Evolve = "EVO";
};

}  // namespace Part
