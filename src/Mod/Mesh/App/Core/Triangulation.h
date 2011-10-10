/***************************************************************************
 *   Copyright (c) 2005 Werner Mayer <wmayer[at]users.sourceforge.net>     *
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


#ifndef MESH_TRIANGULATION_H
#define MESH_TRIANGULATION_H

#include "Elements.h"
#include <Base/Vector3D.h>

namespace MeshCore
{
class MeshKernel;

class MeshExport AbstractPolygonTriangulator
{
public:
    AbstractPolygonTriangulator();
    virtual ~AbstractPolygonTriangulator();

    /** Sets the polygon to be triangulated. */
    void SetPolygon(const std::vector<Base::Vector3f>& raclPoints);
    /** Get the polygon points to be triangulated. The points may be
     * projected onto its average plane.
     */
    std::vector<Base::Vector3f> GetPolygon() const;
    /** The triangulation algorithm may create new points when
     * calling Triangulate(). This method returns these added
     * points.
     * @note: Make sure to have called ProjectOntoSurface() before using
     * this method if you want the surface points the new points are lying on.
     */
    std::vector<Base::Vector3f> AddedPoints() const;
    /** Computes the best-fit plane and returns a transformation matrix
     * built out of the axes of the plane.
     */
    Base::Matrix4D GetTransformToFitPlane() const;
    /** If the points of the polygon set by SetPolygon() doesn't lie in a 
     * plane this method can be used to project the points in a common plane.
     */
    std::vector<Base::Vector3f> ProjectToFitPlane();
    /** Computes the triangulation of a polygon. The resulting facets can
     * be accessed by GetTriangles() or GetFacets().
     */
    bool TriangulatePolygon();
    /** If points were added then we get the 3D points by projecting the added
     * 2D points onto a surface which fits into the given points.
     */
    virtual void ProjectOntoSurface(const std::vector<Base::Vector3f>&);
    /** Returns the geometric triangles of the polygon. */
    const std::vector<MeshGeomFacet>& GetTriangles() const { return _triangles;}
    /** Returns the topologic facets of the polygon. */
    const std::vector<MeshFacet>& GetFacets() const { return _facets;}
    /** Returns the length of the polygon */
    float GetLength() const;
    /** Get information about the pol<gons that were processed.
     * It returns an array of the number of edges for each closed
     * polygon.
     */
    std::vector<unsigned long> GetInfo() const;
    void Discard();

protected:
    /** Computes the triangulation of a polygon. The resulting facets can
     * be accessed by GetTriangles() or GetFacets().
     */
    virtual bool Triangulate() = 0;
    void Done();

protected:
    bool                        _discard;
    Base::Matrix4D              _inverse;
    std::vector<Base::Vector3f> _points;
    std::vector<Base::Vector3f> _newpoints;
    std::vector<MeshGeomFacet>  _triangles;
    std::vector<MeshFacet>      _facets;
    std::vector<unsigned long>  _info;
};

/**
 * The EarClippingTriangulator embeds an efficient algorithm to triangulate
 * polygons taken from http://www.flipcode.com/files/code/triangulate.cpp.
 */
class MeshExport EarClippingTriangulator : public AbstractPolygonTriangulator
{
public:
    EarClippingTriangulator();
    ~EarClippingTriangulator();

protected:
    bool Triangulate();

private:
    /**
    * Static class to triangulate any contour/polygon (without holes) efficiently.
    * The original code snippet was submitted to FlipCode.com by John W. Ratcliff 
    * (jratcliff@verant.com) on July 22, 2000.
    * The original vector of 2d points is replaced by a vector of 3d points where the
    * z-ccordinate is ignored. This is because the algorithm is often used for 3d points 
    * projected to a common plane. The result vector of 2d points is replaced by an 
    * array of indices to the points of the polygon.
    */
    class Triangulate
    {
    public:
        // triangulate a contour/polygon, places results in STL vector
        // as series of triangles.indicating the points
        static bool Process(const std::vector<Base::Vector3f> &contour,
            std::vector<unsigned long> &result);

        // compute area of a contour/polygon
        static float Area(const std::vector<Base::Vector3f> &contour);

        // decide if point Px/Py is inside triangle defined by
        // (Ax,Ay) (Bx,By) (Cx,Cy)
        static bool InsideTriangle(float Ax, float Ay, float Bx, float By,
            float Cx, float Cy, float Px, float Py);

        static bool _invert;
    private:
        static bool Snip(const std::vector<Base::Vector3f> &contour,
            int u,int v,int w,int n,int *V);
    };
};

class MeshExport QuasiDelaunayTriangulator : public EarClippingTriangulator
{
public:
    QuasiDelaunayTriangulator();
    ~QuasiDelaunayTriangulator();

protected:
    bool Triangulate();
};

class MeshExport DelaunayTriangulator : public AbstractPolygonTriangulator
{
public:
    DelaunayTriangulator();
    ~DelaunayTriangulator();

protected:
    bool Triangulate();
};

class MeshExport FlatTriangulator : public AbstractPolygonTriangulator
{
public:
    FlatTriangulator();
    ~FlatTriangulator();

    void ProjectOntoSurface(const std::vector<Base::Vector3f>&);

protected:
    bool Triangulate();
};

class MeshExport ConstraintDelaunayTriangulator : public AbstractPolygonTriangulator
{
public:
    ConstraintDelaunayTriangulator(float area);
    ~ConstraintDelaunayTriangulator();

protected:
    bool Triangulate();

private:
    float fMaxArea;
};

class MeshExport Triangulator : public AbstractPolygonTriangulator
{
public:
    Triangulator(const MeshKernel&);
    ~Triangulator();

protected:
    bool Triangulate();

    const MeshKernel& _kernel;
};

} // namespace MeshCore


#endif  // MESH_TRIANGULATION_H 
