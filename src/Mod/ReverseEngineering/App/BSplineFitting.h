// SPDX-License-Identifier: LGPL-2.1-or-later
// SPDX-FileNotice: Part of the FreeCAD project.

/******************************************************************************
 *                                                                            *
 *   © 2015 Werner Mayer <wmayer@users.sourceforge.net>                       *
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


#ifndef REEN_BSPLINEFITTING_H
#define REEN_BSPLINEFITTING_H

#if defined(HAVE_PCL_OPENNURBS)
# include <vector>

# include <Base/Vector3D.h>
# include <Geom_BSplineSurface.hxx>


namespace Reen
{

class BSplineFitting
{
public:
    BSplineFitting(const std::vector<Base::Vector3f>&);
    Handle(Geom_BSplineSurface) perform();

    void setIterations(unsigned);
    void setOrder(unsigned);
    void setRefinement(unsigned);
    void setInteriorSmoothness(double);
    void setInteriorWeight(double);
    void setBoundarySmoothness(double);
    void setBoundaryWeight(double);

private:
    std::vector<Base::Vector3f> myPoints;
    unsigned myIterations;
    unsigned myOrder;
    unsigned myRefinement;
    double myInteriorSmoothness;
    double myInteriorWeight;
    double myBoundarySmoothness;
    double myBoundaryWeight;
};

}  // namespace Reen

#endif  // HAVE_PCL_OPENNURBS

#endif  // REEN_BSPLINEFITTING_H
