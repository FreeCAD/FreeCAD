// SPDX-License-Identifier: LGPL-2.1-or-later

/***************************************************************************
 *   Copyright (c) 2012 Werner Mayer <wmayer[at]users.sourceforge.net>     *
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

#pragma once

#include <vector>

#include <Base/Vector3D.h>


namespace Points
{
class PointKernel;
}
namespace Mesh
{
class MeshObject;
}
namespace pcl
{
struct PolygonMesh;
}

namespace Reen
{

class MeshConversion
{
public:
    static void convert(const pcl::PolygonMesh&, Mesh::MeshObject&);
};

class SurfaceTriangulation
{
public:
    SurfaceTriangulation(const Points::PointKernel&, Mesh::MeshObject&);
    /** \brief Set the number of k nearest neighbors to use for the normal estimation.
     * \param[in] k the number of k-nearest neighbors
     */
    void perform(int ksearch);
    /** \brief Pass the normals to the points given in the constructor.
     * \param[in] normals the normals to the given points.
     */
    void perform(const std::vector<Base::Vector3f>& normals);

    /** \brief Set the multiplier of the nearest neighbor distance to obtain the final search radius
     * for each point (this will make the algorithm adapt to different point densities in the
     * cloud). \param[in] mu the multiplier
     */
    inline void setMu(double mu)
    {
        this->mu = mu;
    }

    /** \brief Set the sphere radius that is to be used for determining the k-nearest neighbors used
     * for triangulating. \param[in] radius the sphere radius that is to contain all k-nearest
     * neighbors \note This distance limits the maximum edge length!
     */
    inline void setSearchRadius(double radius)
    {
        this->searchRadius = radius;
    }

private:
    const Points::PointKernel& myPoints;
    Mesh::MeshObject& myMesh;
    double mu;
    double searchRadius;
};

class PoissonReconstruction
{
public:
    PoissonReconstruction(const Points::PointKernel&, Mesh::MeshObject&);
    /** \brief Set the number of k nearest neighbors to use for the normal estimation.
     * \param[in] k the number of k-nearest neighbors
     */
    void perform(int ksearch = 5);
    /** \brief Pass the normals to the points given in the constructor.
     * \param[in] normals the normals to the given points.
     */
    void perform(const std::vector<Base::Vector3f>& normals);

    /** \brief Set the maximum depth of the tree that will be used for surface reconstruction.
     * \note Running at depth d corresponds to solving on a voxel grid whose resolution is no larger
     * than 2^d x 2^d x 2^d. Note that since the reconstructor adapts the octree to the sampling
     * density, the specified reconstruction depth is only an upper bound. \param[in] depth the
     * depth parameter
     */
    inline void setDepth(int depth)
    {
        this->depth = depth;
    }

    /** \brief Set the depth at which a block Gauss-Seidel solver is used to solve the Laplacian
     * equation \note Using this parameter helps reduce the memory overhead at the cost of a small
     * increase in reconstruction time. (In practice, we have found that for reconstructions of
     * depth 9 or higher a subdivide depth of 7 or 8 can greatly reduce the memory usage.)
     * \param[in] solver_divide the given parameter value
     */
    inline void setSolverDivide(int solverDivide)
    {
        this->solverDivide = solverDivide;
    }

    /** \brief Set the minimum number of sample points that should fall within an octree node as the
     * octree construction is adapted to sampling density \note For noise-free samples, small values
     * in the range [1.0 - 5.0] can be used. For more noisy samples, larger values in the range
     * [15.0 - 20.0] may be needed to provide a smoother, noise-reduced, reconstruction. \param[in]
     * samples_per_node the given parameter value
     */
    inline void setSamplesPerNode(float samplesPerNode)
    {
        this->samplesPerNode = samplesPerNode;
    }

private:
    const Points::PointKernel& myPoints;
    Mesh::MeshObject& myMesh;
    int depth;
    int solverDivide;
    float samplesPerNode;
};

class GridReconstruction
{
public:
    GridReconstruction(const Points::PointKernel&, Mesh::MeshObject&);
    /** \brief Set the number of k nearest neighbors to use for the normal estimation.
     * \param[in] k the number of k-nearest neighbors
     */
    void perform(int ksearch = 5);
    /** \brief Pass the normals to the points given in the constructor.
     * \param[in] normals the normals to the given points.
     */
    void perform(const std::vector<Base::Vector3f>& normals);

private:
    const Points::PointKernel& myPoints;
    Mesh::MeshObject& myMesh;
};

class ImageTriangulation
{
public:
    ImageTriangulation(int width, int height, const Points::PointKernel&, Mesh::MeshObject&);
    void perform();

private:
    int width, height;
    const Points::PointKernel& myPoints;
    Mesh::MeshObject& myMesh;
};

class MarchingCubesRBF
{
public:
    MarchingCubesRBF(const Points::PointKernel&, Mesh::MeshObject&);
    /** \brief Set the number of k nearest neighbors to use for the normal estimation.
     * \param[in] k the number of k-nearest neighbors
     */
    void perform(int ksearch = 5);
    /** \brief Pass the normals to the points given in the constructor.
     * \param[in] normals the normals to the given points.
     */
    void perform(const std::vector<Base::Vector3f>& normals);

private:
    const Points::PointKernel& myPoints;
    Mesh::MeshObject& myMesh;
};

class MarchingCubesHoppe
{
public:
    MarchingCubesHoppe(const Points::PointKernel&, Mesh::MeshObject&);
    /** \brief Set the number of k nearest neighbors to use for the normal estimation.
     * \param[in] k the number of k-nearest neighbors
     */
    void perform(int ksearch = 5);
    /** \brief Pass the normals to the points given in the constructor.
     * \param[in] normals the normals to the given points.
     */
    void perform(const std::vector<Base::Vector3f>& normals);

private:
    const Points::PointKernel& myPoints;
    Mesh::MeshObject& myMesh;
};

}  // namespace Reen
