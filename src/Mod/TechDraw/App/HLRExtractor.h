// SPDX-License-Identifier: LGPL-2.1-or-later
// SPDX-FileCopyrightText: 2026 Morten Vajhøj
// SPDX-FileNotice: Part of the FreeCAD project.

/******************************************************************************
 *                                                                            *
 *   FreeCAD is free software: you can redistribute it and/or modify          *
 *   it under the terms of the GNU Lesser General Public License as           *
 *   published by the Free Software Foundation, either version 2.1            *
 *   of the License, or (at your option) any later version.                   *
 *                                                                            *
 *   FreeCAD is distributed in the hope that it will be useful,               *
 *   but WITHOUT ANY WARRANTY; without even the implied warranty              *
 *   of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.                  *
 *   See the GNU Lesser General Public License for more details.              *
 *                                                                            *
 *   You should have received a copy of the GNU Lesser General Public         *
 *   License along with FreeCAD. If not, see https://www.gnu.org/licenses     *
 *                                                                            *
 ******************************************************************************/

#pragma once


#include <Mod/TechDraw/TechDrawGlobal.h>

#include <vector>

#include <Standard_Handle.hxx>
#include <TopoDS_Edge.hxx>
#include <TopoDS_Shape.hxx>

class HLRBRep_Algo;
class HLRBRep_Data;
class HLRBRep_EdgeData;
class HLRBRep_PolyAlgo;
class gp_XY;

enum class HLREdgeType
{
    Iso = 1,
    Outline = 2,
    Smooth = 3,
    Seam = 4,
    Hard = 5
};

struct HLREdge
{
    TopoDS_Edge edge;
    TopoDS_Shape sourceShape;
    std::string sourceType = "";
};

class HLRExtractor
{
public:
    HLRExtractor(const Handle(HLRBRep_Algo) & algo);

    std::vector<HLREdge> extract(HLREdgeType type, bool visible);
    TopoDS_Shape toCompound(const std::vector<HLREdge>& hlrEdges);

private:
    void extractFace(bool visible,
                  HLREdgeType type,
                  int iFace,
                  Handle(HLRBRep_Data) & data,
                  std::vector<HLREdge>& result);

    void extractEdge(bool visible,
                  bool inFace,
                  HLREdgeType type,
                  int iEdge,
                  HLRBRep_EdgeData& edge,
                  std::vector<HLREdge>& result);

    void findSource(int iEdge, HLREdge& hlrEdge);

    Handle(HLRBRep_Algo) m_algo;
};

class PolyHLRExtractor
{
public:
    PolyHLRExtractor(const Handle(HLRBRep_PolyAlgo) & algo);

    std::vector<HLREdge> extract(HLREdgeType type, bool visible) const;
    static TopoDS_Shape toCompound(const std::vector<HLREdge>& hlrEdges);

private:
    struct Segment
    {
        HLREdge hlrEdge;
        bool visible = false;
        bool reg1 = false;
        bool regn = false;
        bool outl = false;
        bool intl = false;
    };

    void update();

    void extractSegment(Segment segment, bool visible, const gp_XY& start, const gp_XY& finish);

    static void findSource(const TopoDS_Shape& original, HLREdge& hlrEdge);

    Handle(HLRBRep_PolyAlgo) m_algo;
    std::vector<Segment> m_segments;
};