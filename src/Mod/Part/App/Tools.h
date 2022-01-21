/***************************************************************************
 *   Copyright (c) 2011 Werner Mayer <wmayer[at]users.sourceforge.net>     *
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


#ifndef PART_TOOLS_H
#define PART_TOOLS_H

#include <Base/Converter.h>
#include <gp_Pnt.hxx>
#include <gp_Vec.hxx>
#include <gp_Dir.hxx>
#include <gp_XYZ.hxx>
#include <Geom_Surface.hxx>
#include <Poly_Polygon3D.hxx>
#include <Poly_Triangle.hxx>
#include <Poly_Triangulation.hxx>
#include <TColStd_ListOfTransient.hxx>
#include <TColgp_Array1OfDir.hxx>
#include <TopLoc_Location.hxx>
#include <TopoDS_Edge.hxx>
#include <TopoDS_Face.hxx>
#include <vector>
#include <Mod/Part/PartGlobal.h>

class gp_Lin;
class gp_Pln;

namespace Base {
// Specialization for gp_Pnt
template <>
struct vec_traits<gp_Pnt> {
    typedef gp_Pnt vec_type;
    typedef double float_type;
    vec_traits(const vec_type& v) : v(v){}
    inline std::tuple<float_type,float_type,float_type> get() const {
        return std::make_tuple(v.X(), v.Y(), v.Z());
    }
private:
    const vec_type& v;
};
// Specialization for gp_Vec
template <>
struct vec_traits<gp_Vec> {
    typedef gp_Vec vec_type;
    typedef double float_type;
    vec_traits(const vec_type& v) : v(v){}
    inline std::tuple<float_type,float_type,float_type> get() const {
        return std::make_tuple(v.X(), v.Y(), v.Z());
    }
private:
    const vec_type& v;
};
// Specialization for gp_Dir
template <>
struct vec_traits<gp_Dir> {
    typedef gp_Dir vec_type;
    typedef double float_type;
    vec_traits(const vec_type& v) : v(v){}
    inline std::tuple<float_type,float_type,float_type> get() const {
        return std::make_tuple(v.X(), v.Y(), v.Z());
    }
private:
    const vec_type& v;
};
// Specialization for gp_XYZ
template <>
struct vec_traits<gp_XYZ> {
    typedef gp_XYZ vec_type;
    typedef double float_type;
    vec_traits(const vec_type& v) : v(v){}
    inline std::tuple<float_type,float_type,float_type> get() const {
        return std::make_tuple(v.X(), v.Y(), v.Z());
    }
private:
    const vec_type& v;
};
}

namespace Part
{

PartExport
void closestPointsOnLines(const gp_Lin& lin1, const gp_Lin& lin2, gp_Pnt &p1, gp_Pnt &p2);
PartExport
bool intersect(const gp_Pln& pln1, const gp_Pln& pln2, gp_Lin& lin);
PartExport
bool tangentialArc(const gp_Pnt& p0, const gp_Vec& v0, const gp_Pnt& p1, gp_Pnt& c, gp_Dir& a);

class PartExport Tools
{
public:
    Handle(Geom_Surface) makeSurface (const TColStd_ListOfTransient& theBoundaries,
                                     const Standard_Real theTol,
                                     const Standard_Integer theNbPnts,
                                     const Standard_Integer theNbIter,
                                     const Standard_Integer theMaxDeg);
    /*!
     * @brief getTriangulation
     * The indexes of the triangles are adjusted to the points vector.
     * @param face
     * @param points
     * @param facets
     * @return true if a triangulation exists or false otherwise
     */
    static bool getTriangulation(const TopoDS_Face& face, std::vector<gp_Pnt>& points, std::vector<Poly_Triangle>& facets);
    /*!
     * \brief getPolygonOnTriangulation
     * Get the polygon of edge.
     * \note \a edge must belong to face.
     * \param edge
     * \param face
     * \param points
     * \return true if a triangulation exists or false otherwise
     */
    static bool getPolygonOnTriangulation(const TopoDS_Edge& edge, const TopoDS_Face& face, std::vector<gp_Pnt>& points);
    /*!
     * \brief getPolygon3D
     * \param edge
     * \param points
     * \return true if a polygon exists or false otherwise
     */
    static bool getPolygon3D(const TopoDS_Edge& edge, std::vector<gp_Pnt>& points);
    /*!
     * \brief getPointNormals
     * Calculate the point normals of the given triangulation.
     * \param points
     * \param facets
     * \param normals
     */
    static void getPointNormals(const std::vector<gp_Pnt>& points, const std::vector<Poly_Triangle>& facets, std::vector<gp_Vec>& vertexnormals);
    /*!
     * \brief getPointNormals
     * Computes the more accurate surface normals for the points. If the calculation for a point fails then the precomputed
     * point normal of the triangulation is used.
     * \param points
     * \param face
     * \param vertexnormals
     */
    static void getPointNormals(const std::vector<gp_Pnt>& points, const TopoDS_Face& face, std::vector<gp_Vec>& vertexnormals);
    /*!
     * \brief getPointNormals
     * Computes the exact surface normals for the points by using the UV coordinates of the mesh vertexes.
     * \param face
     * \param aPoly
     * \param vertexnormals
     */
    static void getPointNormals(const TopoDS_Face& face, Handle(Poly_Triangulation) aPoly, TColgp_Array1OfDir& normals);
    /*!
     * \brief getPointNormals
     * Computes the exact surface normals for the points by using the UV coordinates of the mesh vertexes.
     * \param face
     * \param aPoly
     * \param vertexnormals
     */
    static void getPointNormals(const TopoDS_Face& face, Handle(Poly_Triangulation) aPoly, std::vector<gp_Vec>& normals);
    /*!
     * \brief applyTransformationOnNormals
     * Apply the transformation to the vectors
     * \param loc
     * \param normals
     */
    static void applyTransformationOnNormals(const TopLoc_Location& loc, std::vector<gp_Vec>& normals);
    /*!
     * \brief triangulationOfInfinite
     * Returns the triangulation of the face of the tessellated shape. In case the face has infinite lengths
     * the triangulation of a limited parameter range is computed.
     * \param edge
     * \param loc
     */
    static Handle (Poly_Triangulation) triangulationOfFace(const TopoDS_Face& face);
    /*!
     * \brief polygonOfEdge
     * Returns the polygon of the edge of the tessellated shape. In case the edge has infinite length
     * the polygon of a limited parameter range is computed.
     * \param edge
     * \param loc
     */
    static Handle(Poly_Polygon3D) polygonOfEdge(const TopoDS_Edge& edge, TopLoc_Location& loc);
    /*!
     * \brief getNormal
     * Returns the normal at the given parameters on the surface and the state of the calculation
     * \param surf
     * \param u
     * \param v
     * \param tol
     * \param dir
     * \param done
     */
    static void getNormal(const Handle(Geom_Surface)& surf, double u, double v, const Standard_Real tol, gp_Dir& dir, Standard_Boolean& done);
     /* \brief getNormal
     * Returns the normal at the given parameters on the face and the state of the calculation.
     * The orientation is taken into account
     * \param face
     * \param u
     * \param v
     * \param tol
     * \param dir
     * \param done
     */
    static void getNormal(const TopoDS_Face& face, double u, double v, const Standard_Real tol, gp_Dir& dir, Standard_Boolean& done);
};

} //namespace Part


#endif // PART_TOOLS_H
