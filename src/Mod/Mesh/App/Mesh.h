/***************************************************************************
 *   Copyright (c) Jürgen Riegel <juergen.riegel@web.de>                   *
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


#ifndef MESH_MESH_H
#define MESH_MESH_H

#include <vector>
#include <list>
#include <set>
#include <string>
#include <map>

#include <Base/Matrix.h>
#include <Base/Vector3D.h>

#include <App/PropertyStandard.h>
#include <App/PropertyGeo.h>
#include <App/ComplexGeoData.h>

#include "Core/MeshKernel.h"
#include "Core/MeshIO.h"
#include "Core/Iterator.h"
#include "MeshPoint.h"
#include "Facet.h"
#include "Segment.h"

namespace Py {
class List;
}

namespace Base {
  class Polygon2d;
  class ViewProjMethod;
}

namespace MeshCore {
class AbstractPolygonTriangulator;
}

namespace Mesh
{

class MeshObject;
class MeshExport MeshSegment : public Data::Segment
{
    TYPESYSTEM_HEADER();

public:
    virtual std::string getName() const {
        return "MeshSegment";
    }

    Base::Reference<MeshObject> mesh;
    std::unique_ptr<Mesh::Segment> segment;
};

/**
 * The MeshObject class provides an interface for the underlying MeshKernel class and
 * most of its algorithm on it.
 * @note Each instance of MeshObject has its own instance of a MeshKernel so it's not possible
 * that several instances of MeshObject manage one instance of MeshKernel.
 */
class MeshExport MeshObject : public Data::ComplexGeoData
{
    TYPESYSTEM_HEADER();

public:
    enum GeometryType {PLANE, CYLINDER, SPHERE};
    enum CutType {INNER, OUTER};

    // typedef needed for cross-section
    typedef std::pair<Base::Vector3f, Base::Vector3f> TPlane;
    typedef std::list<std::vector<Base::Vector3f> > TPolylines;

    MeshObject();
    explicit MeshObject(const MeshCore::MeshKernel& Kernel);
    explicit MeshObject(const MeshCore::MeshKernel& Kernel, const Base::Matrix4D &Mtrx);
    MeshObject(const MeshObject&);
    virtual ~MeshObject();

    void operator = (const MeshObject&);

    /** @name Subelement management */
    //@{
    /** Sub type list
     *  List of different subelement types
     *  its NOT a list of the subelements itself
     */
    virtual std::vector<const char*> getElementTypes() const;
    virtual unsigned long countSubElements(const char* Type) const;
    /// get the subelement by type and number
    virtual Data::Segment* getSubElement(const char* Type, unsigned long) const;
    /** Get faces from segment */
    virtual void getFacesFromSubElement(
        const Data::Segment*,
        std::vector<Base::Vector3d> &Points,
        std::vector<Base::Vector3d> &PointNormals,
        std::vector<Facet> &faces) const;
    //@}

    void setTransform(const Base::Matrix4D& rclTrf);
    Base::Matrix4D getTransform() const;
    void transformGeometry(const Base::Matrix4D &rclMat);

    /**
     * Swaps the content of \a Kernel and the internal mesh kernel.
     */
    void swap(MeshCore::MeshKernel& Kernel);
    void swap(MeshObject& mesh);

    /** @name Querying */
    //@{
    std::string representation() const;
    std::string topologyInfo() const;
    unsigned long countPoints() const;
    unsigned long countFacets() const;
    unsigned long countEdges () const;
    unsigned long countSegments() const;
    bool isSolid() const;
    Base::Vector3d getPoint(PointIndex) const;
    MeshPoint getMeshPoint(PointIndex) const;
    Mesh::Facet getMeshFacet(FacetIndex) const;
    double getSurface() const;
    double getVolume() const;
    /** Get points from object with given accuracy */
    virtual void getPoints(std::vector<Base::Vector3d> &Points,
        std::vector<Base::Vector3d> &Normals,
        float Accuracy, uint16_t flags=0) const;
    virtual void getFaces(std::vector<Base::Vector3d> &Points,std::vector<Facet> &Topo,
        float Accuracy, uint16_t flags=0) const;
    std::vector<PointIndex> getPointsFromFacets(const std::vector<FacetIndex>& facets) const;
    //@}

    void setKernel(const MeshCore::MeshKernel& m);
    MeshCore::MeshKernel& getKernel()
    { return _kernel; }
    const MeshCore::MeshKernel& getKernel() const
    { return _kernel; }

    virtual Base::BoundBox3d getBoundBox() const;
    virtual bool getCenterOfGravity(Base::Vector3d& center) const;

    /** @name I/O */
    //@{
    // Implemented from Persistence
    unsigned int getMemSize () const;
    void Save (Base::Writer &writer) const;
    void SaveDocFile (Base::Writer &writer) const;
    void Restore(Base::XMLReader &reader);
    void RestoreDocFile(Base::Reader &reader);
    void save(const char* file,MeshCore::MeshIO::Format f=MeshCore::MeshIO::Undefined,
        const MeshCore::Material* mat = nullptr,
        const char* objectname = nullptr) const;
    void save(std::ostream&,MeshCore::MeshIO::Format f,
        const MeshCore::Material* mat = nullptr,
        const char* objectname = nullptr) const;
    bool load(const char* file, MeshCore::Material* mat = nullptr);
    bool load(std::istream&, MeshCore::MeshIO::Format f, MeshCore::Material* mat = nullptr);
    // Save and load in internal format
    void save(std::ostream&) const;
    void load(std::istream&);
    //@}

    /** @name Manipulation */
    //@{
    void addFacet(const MeshCore::MeshGeomFacet& facet);
    void addFacets(const std::vector<MeshCore::MeshGeomFacet>& facets);
    void addFacets(const std::vector<MeshCore::MeshFacet> &facets,
                   bool checkManifolds);
    void addFacets(const std::vector<MeshCore::MeshFacet> &facets,
                   const std::vector<Base::Vector3f>& points,
                   bool checkManifolds);
    void addFacets(const std::vector<Data::ComplexGeoData::Facet> &facets,
                   const std::vector<Base::Vector3d>& points,
                   bool checkManifolds);
    void setFacets(const std::vector<MeshCore::MeshGeomFacet>& facets);
    void setFacets(const std::vector<Data::ComplexGeoData::Facet> &facets,
                   const std::vector<Base::Vector3d>& points);
    /**
     * Combines two independent mesh objects.
     * @note The mesh object we want to add must not overlap or intersect with
     * this mesh object.
     */
    void addMesh(const MeshObject&);
    /**
     * Combines two independent mesh objects.
     * @note The mesh object we want to add must not overlap or intersect with
     * this mesh object.
     */
    void addMesh(const MeshCore::MeshKernel&);
    void deleteFacets(const std::vector<FacetIndex>& removeIndices);
    void deletePoints(const std::vector<PointIndex>& removeIndices);
    std::vector<std::vector<FacetIndex> > getComponents() const;
    unsigned long countComponents() const;
    void removeComponents(unsigned long);
    /**
     * Checks for the given facet indices what will be the degree for each point
     * when these facets are removed from the mesh kernel.
     * The point degree information is stored in \a point_degree. The return value
     * gives the number of points which will have a degree of zero.
     */
    unsigned long getPointDegree(const std::vector<FacetIndex>& facets,
        std::vector<PointIndex>& point_degree) const;
    void fillupHoles(unsigned long, int, MeshCore::AbstractPolygonTriangulator&);
    void offset(float fSize);
    void offsetSpecial2(float fSize);
    void offsetSpecial(float fSize, float zmax, float zmin);
    /// clears the Mesh
    void clear();
    void transformToEigenSystem();
    Base::Matrix4D getEigenSystem(Base::Vector3d& v) const;
    void movePoint(PointIndex, const Base::Vector3d& v);
    void setPoint(PointIndex, const Base::Vector3d& v);
    void smooth(int iterations, float d_max);
    void decimate(float fTolerance, float fReduction);
    void decimate(int targetSize);
    Base::Vector3d getPointNormal(PointIndex) const;
    std::vector<Base::Vector3d> getPointNormals() const;
    void crossSections(const std::vector<TPlane>&, std::vector<TPolylines> &sections,
                       float fMinEps = 1.0e-2f, bool bConnectPolygons = false) const;
    void cut(const Base::Polygon2d& polygon, const Base::ViewProjMethod& proj, CutType);
    void trim(const Base::Polygon2d& polygon, const Base::ViewProjMethod& proj, CutType);
    void trimByPlane(const Base::Vector3f& base, const Base::Vector3f& normal);
    //@}

    /** @name Selection */
    //@{
    void deleteSelectedFacets();
    void deleteSelectedPoints();
    void addFacetsToSelection(const std::vector<FacetIndex>&) const;
    void addPointsToSelection(const std::vector<PointIndex>&) const;
    void removeFacetsFromSelection(const std::vector<FacetIndex>&) const;
    void removePointsFromSelection(const std::vector<PointIndex>&) const;
    unsigned long countSelectedFacets() const;
    bool hasSelectedFacets() const;
    unsigned long countSelectedPoints() const;
    bool hasSelectedPoints() const;
    void getFacetsFromSelection(std::vector<FacetIndex>&) const;
    void getPointsFromSelection(std::vector<PointIndex>&) const;
    void clearFacetSelection() const;
    void clearPointSelection() const;
    //@}

    /** @name Boolean operations */
    //@{
    MeshObject* unite(const MeshObject&) const;
    MeshObject* intersect(const MeshObject&) const;
    MeshObject* subtract(const MeshObject&) const;
    MeshObject* inner(const MeshObject&) const;
    MeshObject* outer(const MeshObject&) const;
    std::vector< std::vector<Base::Vector3f> > section(const MeshObject&, bool connectLines, float fMinDist) const;
    //@}

    /** @name Topological operations */
    //@{
    void refine();
    void removeNeedles(float);
    void optimizeTopology(float);
    void optimizeEdges();
    void splitEdges();
    void splitEdge(FacetIndex, FacetIndex, const Base::Vector3f&);
    void splitFacet(FacetIndex, const Base::Vector3f&, const Base::Vector3f&);
    void swapEdge(FacetIndex, FacetIndex);
    void collapseEdge(FacetIndex, FacetIndex);
    void collapseFacet(FacetIndex);
    void collapseFacets(const std::vector<FacetIndex>&);
    void insertVertex(FacetIndex, const Base::Vector3f& v);
    void snapVertex(FacetIndex, const Base::Vector3f& v);
    //@}

    /** @name Mesh validation */
    //@{
    unsigned long countNonUniformOrientedFacets() const;
    void flipNormals();
    void harmonizeNormals();
    void validateIndices();
    void validateCaps(float fMaxAngle, float fSplitFactor);
    void validateDeformations(float fMaxAngle, float fEps);
    void validateDegenerations(float fEps);
    void removeDuplicatedPoints();
    void removeDuplicatedFacets();
    bool hasNonManifolds() const;
    bool hasInvalidNeighbourhood() const;
    bool hasPointsOutOfRange() const;
    bool hasFacetsOutOfRange() const;
    bool hasCorruptedFacets() const;
    void removeNonManifolds();
    void removeNonManifoldPoints();
    bool hasSelfIntersections() const;
    void removeSelfIntersections();
    void removeSelfIntersections(const std::vector<FacetIndex>&);
    void removeFoldsOnSurface();
    void removeFullBoundaryFacets();
    bool hasInvalidPoints() const;
    void removeInvalidPoints();
    void mergeFacets();
    bool hasPointsOnEdge() const;
    void removePointsOnEdge(bool fillBoundary);
    //@}

    /** @name Mesh segments */
    //@{
    void addSegment(const Segment&);
    void addSegment(const std::vector<FacetIndex>&);
    const Segment& getSegment(unsigned long) const;
    Segment& getSegment(unsigned long);
    MeshObject* meshFromSegment(const std::vector<FacetIndex>&) const;
    std::vector<Segment> getSegmentsOfType(GeometryType, float dev, unsigned long minFacets) const;
    //@}

    /** @name Primitives */
    //@{
    static MeshObject* createMeshFromList(Py::List& list);
    static MeshObject* createSphere(float, int);
    static MeshObject* createEllipsoid(float, float, int);
    static MeshObject* createCylinder(float, float, int, float, int);
    static MeshObject* createCone(float, float, float, int, float, int);
    static MeshObject* createTorus(float, float, int);
    static MeshObject* createCube(float, float, float);
    static MeshObject* createCube(float, float, float, float);
    static MeshObject* createCube(const Base::BoundBox3d&);
    //@}

public:
    class MeshExport const_point_iterator
    {
    public:
        const_point_iterator(const MeshObject*, PointIndex index);
        const_point_iterator(const const_point_iterator& pi);
        ~const_point_iterator();

        const_point_iterator& operator=(const const_point_iterator& fi);
        const MeshPoint& operator*();
        const MeshPoint* operator->();
        bool operator==(const const_point_iterator& fi) const;
        bool operator!=(const const_point_iterator& fi) const;
        const_point_iterator& operator++();
        const_point_iterator& operator--();
    private:
        void dereference();
        const MeshObject* _mesh;
        MeshPoint _point;
        MeshCore::MeshPointIterator _p_it;
    };

    class MeshExport const_facet_iterator
    {
    public:
        const_facet_iterator(const MeshObject*, FacetIndex index);
        const_facet_iterator(const const_facet_iterator& fi);
        ~const_facet_iterator();

        const_facet_iterator& operator=(const const_facet_iterator& fi);
        Mesh::Facet& operator*();
        Mesh::Facet* operator->();
        bool operator==(const const_facet_iterator& fi) const;
        bool operator!=(const const_facet_iterator& fi) const;
        const_facet_iterator& operator++();
        const_facet_iterator& operator--();
    private:
        void dereference();
        const MeshObject* _mesh;
        Mesh::Facet _facet;
        MeshCore::MeshFacetIterator _f_it;
    };

    /** @name Iterator */
    //@{
    const_point_iterator points_begin() const
    { return const_point_iterator(this, 0); }
    const_point_iterator points_end() const
    { return const_point_iterator(this, countPoints()); }

    const_facet_iterator facets_begin() const
    { return const_facet_iterator(this, 0); }
    const_facet_iterator facets_end() const
    { return const_facet_iterator(this, countFacets()); }

    typedef std::vector<Segment>::const_iterator const_segment_iterator;
    const_segment_iterator segments_begin() const
    { return _segments.begin(); }
    const_segment_iterator segments_end() const
    { return _segments.end(); }
    //@}

    // friends
    friend class Segment;

private:
    void deletedFacets(const std::vector<FacetIndex>& remFacets);
    void updateMesh(const std::vector<FacetIndex>&) const;
    void updateMesh() const;
    void swapKernel(MeshCore::MeshKernel& m, const std::vector<std::string>& g);
    void copySegments(const MeshObject&);
    void swapSegments(MeshObject&);

private:
    Base::Matrix4D _Mtrx;
    MeshCore::MeshKernel _kernel;
    std::vector<Segment> _segments;
    static float Epsilon;
};

} // namespace Mesh


#endif // MESH_MESH_H
