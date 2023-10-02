/***************************************************************************
 *   Copyright (c) 2009 Jürgen Riegel <juergen.riegel@web.de>              *
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

#ifndef _RobotAlgos_h_
#define _RobotAlgos_h_

#include "kdl_cp/frames_io.hpp"
#include <Base/Placement.h>
#include <Base/Vector3D.h>
#include <Mod/Robot/RobotGlobal.h>


namespace Robot
{

/** Algo class for projecting shapes and creating SVG output of it
 */
class RobotExport RobotAlgos
{

public:
    /// Constructor
    RobotAlgos();
    virtual ~RobotAlgos();

    void Test();
};

inline KDL::Frame toFrame(const Base::Placement& To)
{
    return KDL::Frame(KDL::Rotation::Quaternion(To.getRotation()[0],
                                                To.getRotation()[1],
                                                To.getRotation()[2],
                                                To.getRotation()[3]),
                      KDL::Vector(To.getPosition()[0], To.getPosition()[1], To.getPosition()[2]));
}
inline Base::Placement toPlacement(const KDL::Frame& To)
{
    double x, y, z, w;
    To.M.GetQuaternion(x, y, z, w);
    return Base::Placement(Base::Vector3d(To.p[0], To.p[1], To.p[2]), Base::Rotation(x, y, z, w));
}

}  // namespace Robot


#endif
