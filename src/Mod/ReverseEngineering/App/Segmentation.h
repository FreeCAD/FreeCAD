/***************************************************************************
 *   Copyright (c) 2016 Werner Mayer <wmayer[at]users.sourceforge.net>     *
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

#ifndef REEN_SEGMENTATION_H
#define REEN_SEGMENTATION_H

#include <list>
#include <vector>

#include <Base/Vector3D.h>


namespace Points
{
class PointKernel;
}

namespace Reen
{

class Segmentation
{
public:
    Segmentation(const Points::PointKernel&, std::list<std::vector<int>>& clusters);
    /** \brief Set the number of k nearest neighbors to use for the normal estimation.
     * \param[in] k the number of k-nearest neighbors
     */
    void perform(int ksearch);

private:
    const Points::PointKernel& myPoints;
    std::list<std::vector<int>>& myClusters;
};

class NormalEstimation
{
public:
    explicit NormalEstimation(const Points::PointKernel&);
    /** \brief Set the number of k nearest neighbors to use for the feature estimation.
     * \param[in] k the number of k-nearest neighbors
     */
    inline void setKSearch(int k)
    {
        kSearch = k;
    }

    /** \brief Set the sphere radius that is to be used for determining the nearest neighbors used
     * for the feature estimation. \param[in] radius the sphere radius used as the maximum distance
     * to consider a point a neighbor
     */
    inline void setSearchRadius(double radius)
    {
        searchRadius = radius;
    }

    /** \brief Perform the normal estimation.
     * \param[out] the estimated normals
     */
    void perform(std::vector<Base::Vector3d>& normals);

private:
    const Points::PointKernel& myPoints;
    int kSearch;
    double searchRadius;
};

}  // namespace Reen

#endif  // REEN_SEGMENTATION_H
