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


namespace MeshCore
{
class MeshKernel;

class MeshExport TriangulationVerifier
{
public:
    TriangulationVerifier() = default;
    virtual ~TriangulationVerifier() = default;
    TriangulationVerifier(const TriangulationVerifier&) = delete;
    TriangulationVerifier(TriangulationVerifier&&) = delete;
    TriangulationVerifier& operator=(const TriangulationVerifier&) = delete;
    TriangulationVerifier& operator=(TriangulationVerifier&&) = delete;

    virtual bool Accept(const Base::Vector3f& n,
                        const Base::Vector3f& p1,
                        const Base::Vector3f& p2,
                        const Base::Vector3f& p3) const;
    virtual bool MustFlip(const Base::Vector3f& n1, const Base::Vector3f& n2) const;
};

class MeshExport TriangulationVerifierV2: public TriangulationVerifier
{
public:
    bool Accept(const Base::Vector3f& n,
                const Base::Vector3f& p1,
                const Base::Vector3f& p2,
                const Base::Vector3f& p3) const override;
    bool MustFlip(const Base::Vector3f& n1, const Base::Vector3f& n2) const override;
};

class MeshExport AbstractPolygonTriangulator
{
public:
    AbstractPolygonTriangulator();
    virtual ~AbstractPolygonTriangulator();

    AbstractPolygonTriangulator(const AbstractPolygonTriangulator&) = delete;
    AbstractPolygonTriangulator(AbstractPolygonTriangulator&&) = delete;
    AbstractPolygonTriangulator& operator=(const AbstractPolygonTriangulator&) = delete;
    AbstractPolygonTriangulator& operator=(AbstractPolygonTriangulator&&) = delete;

    /** Sets the polygon to be triangulated. */
    void SetPolygon(const std::vector<Base::Vector3f>& raclPoints);
    void SetIndices(const std::vector<PointIndex>& d)
    {
        _indices = d;
    }
    /** Set a verifier object that checks if the generated triangulation
     * can be accepted and added to the mesh kernel.
     * The triangulator takes ownership of the passed verifier.
     */
    void SetVerifier(TriangulationVerifier* v);
    TriangulationVerifier* GetVerifier() const;
    /** Usually the created faces use the indices of the polygon points
     * from [0, n]. If the faces should be appended to an existing mesh
     * they may need to be reindexed from the calling instance.
     * However, there may other algorithms that directly use the indices
     * of the mesh and thus do not need to be touched afterwards. In this
     * case the method should be reimplemented to return false.
     */
    virtual bool NeedsReindexing() const
    {
        return true;
    }
    /** Get the polygon points to be triangulated. The points may be
     * projected onto its average plane.
     */
    std::vector<Base::Vector3f> GetPolygon() const;
    /** The triangulation algorithm may create new points when
     * calling Triangulate(). This method returns these added
     * points.
     * @note: Make sure to have called PostProcessing() before using
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
    virtual void PostProcessing(const std::vector<Base::Vector3f>&);
    /** Returns the geometric triangles of the polygon. */
    const std::vector<MeshGeomFacet>& GetTriangles() const
    {
        return _triangles;
    }
    /** Returns the topologic facets of the polygon. */
    const std::vector<MeshFacet>& GetFacets() const
    {
        return _facets;
    }
    /** Returns the triangle to a given topologic facet. */
    virtual MeshGeomFacet GetTriangle(const MeshPointArray&, const MeshFacet&) const;
    /** Returns the length of the polygon */
    float GetLength() const;
    /** Get information about the polygons that were processed.
     * It returns an array of the number of edges for each closed
     * polygon.
     */
    std::vector<PointIndex> GetInfo() const;
    virtual void Discard();
    /** Resets some internals. The default implementation does nothing.*/
    virtual void Reset();

protected:
    /** Computes the triangulation of a polygon. The resulting facets can
     * be accessed by GetTriangles() or GetFacets().
     */
    virtual bool Triangulate() = 0;
    void Done();

protected:
    // NOLINTBEGIN
    bool _discard;
    Base::Matrix4D _inverse;
    std::vector<PointIndex> _indices;
    std::vector<Base::Vector3f> _points;
    std::vector<Base::Vector3f> _newpoints;
    std::vector<MeshGeomFacet> _triangles;
    std::vector<MeshFacet> _facets;
    std::vector<PointIndex> _info;
    TriangulationVerifier* _verifier;
    // NOLINTEND
};

/**
 * The EarClippingTriangulator embeds an efficient algorithm to triangulate
 * polygons taken from http://www.flipcode.com/files/code/triangulate.cpp.
 */
class MeshExport EarClippingTriangulator: public AbstractPolygonTriangulator
{
public:
    EarClippingTriangulator();

protected:
    bool Triangulate() override;

private:
    /**
     * Static class to triangulate any contour/polygon (without holes) efficiently.
     * The original code snippet was submitted to FlipCode.com by John W. Ratcliff
     * (jratcliff@verant.com) on July 22, 2000.
     * The original vector of 2d points is replaced by a vector of 3d points where the
     * z-coordinate is ignored. This is because the algorithm is often used for 3d points
     * projected to a common plane. The result vector of 2d points is replaced by an
     * array of indices to the points of the polygon.
     */
    class Triangulate
    {
    public:
        // triangulate a contour/polygon, places results in STL vector
        // as series of triangles.indicating the points
        static bool Process(const std::vector<Base::Vector3f>& contour,
                            std::vector<PointIndex>& result);

        // compute area of a contour/polygon
        static float Area(const std::vector<Base::Vector3f>& contour);

        // decide if point Px/Py is inside triangle defined by
        // (Ax,Ay) (Bx,By) (Cx,Cy)
        static bool InsideTriangle(float Ax,
                                   float Ay,
                                   float Bx,
                                   float By,
                                   float Cx,
                                   float Cy,
                                   float Px,
                                   float Py);

        static bool _invert;

    private:
        static bool
        Snip(const std::vector<Base::Vector3f>& contour, int u, int v, int w, int n, int* V);
    };
};

class MeshExport QuasiDelaunayTriangulator: public EarClippingTriangulator
{
public:
    QuasiDelaunayTriangulator();

protected:
    bool Triangulate() override;
};

class MeshExport DelaunayTriangulator: public AbstractPolygonTriangulator
{
public:
    DelaunayTriangulator();

protected:
    bool Triangulate() override;
};

class MeshExport FlatTriangulator: public AbstractPolygonTriangulator
{
public:
    FlatTriangulator();

    void PostProcessing(const std::vector<Base::Vector3f>&) override;

protected:
    bool Triangulate() override;
};

class MeshExport ConstraintDelaunayTriangulator: public AbstractPolygonTriangulator
{
public:
    explicit ConstraintDelaunayTriangulator(float area);

protected:
    bool Triangulate() override;

private:
    float fMaxArea;
};

#if 0
class MeshExport Triangulator : public AbstractPolygonTriangulator
{
public:
    Triangulator(const MeshKernel&, bool flat);
    ~Triangulator();
    void Discard();
    void Reset();

    bool NeedsReindexing() const { return false; }
    MeshGeomFacet GetTriangle(const MeshPointArray&, const MeshFacet&) const;
    void PostProcessing(const std::vector<Base::Vector3f>&);

protected:
    bool Triangulate();

    const MeshKernel& _kernel;
};
#endif

}  // namespace MeshCore


#endif  // MESH_TRIANGULATION_H
