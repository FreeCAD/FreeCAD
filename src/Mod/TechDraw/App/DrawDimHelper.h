/***************************************************************************
 *   Copyright (c) 2019 WandererFan <wandererfan@gmail.com>                *
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

#ifndef _DrawDimHelper_h_
#define _DrawDimHelper_h_

#include <Mod/TechDraw/TechDrawGlobal.h>

#include <Geom2d_Curve.hxx>

#include <string>
#include <Base/Vector3D.h>

class gp_Pnt2d;

namespace TechDraw
{
class BaseGeom;
class DrawViewPart;

class TechDrawExport hTrimCurve {
    public:
    hTrimCurve() : first(0.0), last(0.0) {}
    hTrimCurve(Handle(Geom2d_Curve) hCurveIn,
               double parm1,
               double parm2);
    ~hTrimCurve() {}

    Handle(Geom2d_Curve) hCurve;
    double first;
    double last;
};


/// Additional functions for working with Dimensions
class TechDrawExport DrawDimHelper {
    public:
    static void makeExtentDim(DrawViewPart* dvp,
//                              std::vector<TechDraw::BaseGeomPtr> selEdges,
                              std::vector<std::string> edgeNames,
                              int direction);
    static gp_Pnt2d findClosestPoint(std::vector<hTrimCurve> hCurve2dList,
                                   Handle(Geom2d_Curve) boundary);
    static TechDraw::DrawViewDimension* makeDistDim(DrawViewPart* dvp,
                                                    std::string dimType,
                                                    Base::Vector3d refMin,
                                                    Base::Vector3d refMax,
                                                    bool extent = false);
    static std::pair<Base::Vector3d, Base::Vector3d> minMax(DrawViewPart* dvp,
                                                            std::vector<std::string> edgeNames,
                                                            int direction);
};

} //end namespace TechDraw
#endif
