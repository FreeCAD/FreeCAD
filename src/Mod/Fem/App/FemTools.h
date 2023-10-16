/***************************************************************************
 *   Copyright (c) 2015 Werner Mayer <wmayer[at]users.sourceforge.net>     *
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


#ifndef FEM_TOOLS_H
#define FEM_TOOLS_H

#include <Base/Vector3D.h>
#include <Mod/Fem/FemGlobal.h>
#include <gp_XYZ.hxx>

class TopoDS_Shape;
class TopoDS_Edge;
class TopoDS_Face;

namespace Fem
{

class FemExport Tools
{
public:
    /*!
     Get the direction of the shape. If the shape is a planar face
     then get its normal direction. If it's 'linear' then get its
     direction vector.
     @see isLinear
     @see isPlanar
     */
    static Base::Vector3d getDirectionFromShape(const TopoDS_Shape&);
    /*!
     Checks whether the curve of the edge is 'linear' which is the case
     for a line or a spline or Bezier curve with collinear control points.
     */
    static bool isLinear(const TopoDS_Edge&);
    /*!
     Checks whether the surface of the face is planar.
     */
    static bool isPlanar(const TopoDS_Face&);
    /*!
     It is assumed that the edge is 'linear'.
     The direction vector of the line is returned.
     @see isLinear
     */
    static gp_XYZ getDirection(const TopoDS_Edge&);
    /*!
     It is assumed that the face is 'planar'.
     The normal vector of the plane is returned.
     @see isPlanar
     */
    static gp_XYZ getDirection(const TopoDS_Face&);
    /*!
     function to determine 3rd-party binaries used by the FEM WB
     The result is either the full path if available or just the binary
     name if it was found in a system path
     */
    static std::string checkIfBinaryExists(std::string prefSection,
                                           std::string prefBinaryPath,
                                           std::string prefBinaryName);
};

}  // namespace Fem


#endif  // FEM_TOOLS_H
