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

#ifndef REEN_BSPLINEFITTING_H
#define REEN_BSPLINEFITTING_H

#if defined(HAVE_PCL_OPENNURBS)
#include <vector>

#include <Base/Vector3D.h>
#include <Geom_BSplineSurface.hxx>


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
