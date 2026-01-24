// SPDX-License-Identifier: LGPL-2.1-or-later
// SPDX-FileNotice: Part of the FreeCAD project.

/******************************************************************************
 *                                                                            *
 *   © 2016 Werner Mayer <wmayer@users.sourceforge.net>                       *
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


#ifndef REEN_SAMPLECONSENSUS_H
#define REEN_SAMPLECONSENSUS_H

#include <vector>

#include <Base/Vector3D.h>


namespace Points
{
class PointKernel;
}

namespace Reen
{

class SampleConsensus
{
public:
    enum SacModel
    {
        SACMODEL_PLANE,
        SACMODEL_LINE,
        SACMODEL_CIRCLE2D,
        SACMODEL_CIRCLE3D,
        SACMODEL_SPHERE,
        SACMODEL_CYLINDER,
        SACMODEL_CONE,
        SACMODEL_TORUS,
    };
    SampleConsensus(SacModel sac, const Points::PointKernel&, const std::vector<Base::Vector3d>&);
    double perform(std::vector<float>& parameters, std::vector<int>& model);

private:
    SacModel mySac;
    const Points::PointKernel& myPoints;
    const std::vector<Base::Vector3d>& myNormals;
};

}  // namespace Reen

#endif  // REEN_SAMPLECONSENSUS_H
