/***************************************************************************
 *   Copyright (c) 2005 Imetric 3D GmbH                                    *
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


#ifndef MESH_EVALUATION_H
#define MESH_EVALUATION_H

#include <list>
#include <math.h>

#include "MeshKernel.h"
#include "Visitor.h"

namespace MeshCore {

/**
 * The MeshEvaluation class checks the mesh kernel for correctness with respect to a
 * certain criterion, such as manifoldness, self-intersections, etc.
 * The passed mesh kernel is read-only and cannot be modified.
 * @see MeshEvalTopology
 * @see MeshEvalGeometry
 * The class itself is abstract, hence the method Evaluate() must be implemented 
 * by subclasses.
 */
class MeshExport MeshEvaluation
{
public:
  MeshEvaluation (const MeshKernel &rclB) : _rclMesh(rclB) {}
  virtual ~MeshEvaluation () {}

  /**
   * Evaluates the mesh kernel with respect to certain criteria. Must be reimplemented by every 
   * subclass. This pure virtual function returns false if the mesh kernel is invalid according
   * to this criterion and true if the mesh kernel is correct. 
   */
  virtual bool Evaluate () = 0;

protected:
  const MeshKernel& _rclMesh; /**< Mesh kernel */
};

// ----------------------------------------------------

/**
 * The MeshValidation class tries to make a mesh kernel valid with respect to a
 * certain criterion, such as manifoldness, self-intersections, etc.
 * The passed mesh kernel can be modified to fix the errors.
 * The class itself is abstract, hence the method Fixup() must be implemented 
 * by subclasses.
 */
class MeshExport MeshValidation
{
public:
  MeshValidation (MeshKernel &rclB) : _rclMesh(rclB) {}
  virtual ~MeshValidation () {}

  /**
   * This function attempts to change the mesh kernel to be valid according to the checked 
   * criterion: True is returned if the errors could be fixed, false otherwise. 
   */
  virtual bool Fixup() = 0;

protected:
  MeshKernel& _rclMesh; /**< Mesh kernel */
};

// ----------------------------------------------------

/**
 * This class searches for nonuniform orientation of neighboured facets.
 * @author Werner Mayer
 */
class MeshExport MeshOrientationVisitor : public MeshFacetVisitor
{
public:
    MeshOrientationVisitor();

    /** Returns false after the first inconsistence is found, true otherwise. */
    bool Visit (const MeshFacet &, const MeshFacet &, unsigned long , unsigned long );
    bool HasNonUnifomOrientedFacets() const;

private:
    bool _nonuniformOrientation;
};

/**
 * This class searches for inconsistent orientation of neighboured facets.
 * Note: The 'TMP0' flag for facets must be resetted before using this class.
 * @author Werner Mayer
 */
class MeshExport MeshOrientationCollector : public MeshOrientationVisitor
{
public:
    MeshOrientationCollector(std::vector<unsigned long>& aulIndices,
                             std::vector<unsigned long>& aulComplement);

    /** Returns always true and collects the indices with wrong orientation. */
    bool Visit (const MeshFacet &, const MeshFacet &, unsigned long , unsigned long);

private:
    std::vector<unsigned long>& _aulIndices;
    std::vector<unsigned long>& _aulComplement;
};

/**
 * @author Werner Mayer
 */
class MeshExport MeshSameOrientationCollector : public MeshOrientationVisitor
{
public:
    MeshSameOrientationCollector(std::vector<unsigned long>& aulIndices);
    /** Returns always true and collects the indices with wrong orientation. */
    bool Visit (const MeshFacet &, const MeshFacet &, unsigned long , unsigned long);

private:
    std::vector<unsigned long>& _aulIndices;
};

/**
 * The MeshEvalOrientation class checks the mesh kernel for consistent facet normals.
 * @author Werner Mayer
 */
class MeshExport MeshEvalOrientation : public MeshEvaluation
{
public:
    MeshEvalOrientation (const MeshKernel& rclM);
    ~MeshEvalOrientation();
    bool Evaluate ();
    std::vector<unsigned long> GetIndices() const;

private:
    unsigned long HasFalsePositives(const std::vector<unsigned long>&) const;
};

/**
 * The MeshFixOrientation class harmonizes the facet normals of the passed mesh kernel.
 * @author Werner Mayer
 */
class MeshExport MeshFixOrientation : public MeshValidation
{
public:
    MeshFixOrientation (MeshKernel& rclM);
    ~MeshFixOrientation();
    bool Fixup();
};

// ----------------------------------------------------

/**
 * The MeshEvalSolid class checks if the mesh represents a solid.
 * @author Werner Mayer
 */
class MeshExport MeshEvalSolid : public MeshEvaluation
{
public:
  MeshEvalSolid (const MeshKernel& rclM);
  ~MeshEvalSolid();
  bool Evaluate ();
};

// ----------------------------------------------------

/**
 * The MeshEvalTopology class checks for topologic correctness, i.e
 * that the mesh must not contain non-manifolds. E.g. an edge is regarded as
 * non-manifold if it is shared by more than two facets.
 * @note This check does not necessarily cover any degenerations.
 */
class MeshExport MeshEvalTopology : public MeshEvaluation
{
public:
    MeshEvalTopology (const MeshKernel &rclB) : MeshEvaluation(rclB) {}
    virtual ~MeshEvalTopology () {}
    virtual bool Evaluate ();

    void GetFacetManifolds (std::vector<unsigned long> &raclFacetIndList) const;
    unsigned long CountManifolds() const;
    const std::vector<std::pair<unsigned long, unsigned long> >& GetIndices() const { return nonManifoldList; }
    const std::list<std::vector<unsigned long> >& GetFacets() const { return nonManifoldFacets; }

protected:
    std::vector<std::pair<unsigned long, unsigned long> > nonManifoldList;
    std::list<std::vector<unsigned long> > nonManifoldFacets;
};

/**
 * The MeshFixTopology class tries to fix a few cases of non-manifolds.
 * @see MeshEvalTopology
 */
class MeshExport MeshFixTopology : public MeshValidation
{
public:
    MeshFixTopology (MeshKernel &rclB, const std::list<std::vector<unsigned long> >& mf)
      : MeshValidation(rclB), nonManifoldList(mf) {}
    virtual ~MeshFixTopology () {}
    bool Fixup();

    const std::vector<unsigned long>& GetDeletedFaces() const { return deletedFaces; }

protected:
    std::vector<unsigned long> deletedFaces;
    const std::list<std::vector<unsigned long> >& nonManifoldList;
};

// ----------------------------------------------------

/**
 * The MeshEvalPointManifolds class checks for non-manifold points.
 * A point is considered non-manifold if two sets of triangles share
 * the point but are not topologically connected over a common edge.
 * Such mesh defects can lead to some very ugly folds on the surface.
 */
class MeshExport MeshEvalPointManifolds : public MeshEvaluation
{
public:
    MeshEvalPointManifolds (const MeshKernel &rclB) : MeshEvaluation(rclB) {}
    virtual ~MeshEvalPointManifolds () {}
    virtual bool Evaluate ();

    void GetFacetIndices (std::vector<unsigned long> &facets) const;
    const std::list<std::vector<unsigned long> >& GetFacetIndices () const { return facetsOfNonManifoldPoints; }
    const std::vector<unsigned long>& GetIndices() const { return nonManifoldPoints; }
    unsigned long CountManifolds() const { return nonManifoldPoints.size(); }

protected:
    std::vector<unsigned long> nonManifoldPoints;
    std::list<std::vector<unsigned long> > facetsOfNonManifoldPoints;
};

// ----------------------------------------------------

/**
 * The MeshEvalSingleFacet class checks a special case of non-manifold edges as follows.
 * If an edge is shared by more than two facets and if all further facets causing this non-
 * manifold have only their neighbour facets set at this edge, i.e. they have no neighbours
 * at their other edges.
 * Such facets can just be removed from the mesh.
 */
class MeshExport MeshEvalSingleFacet : public MeshEvalTopology
{
public:
  MeshEvalSingleFacet (const MeshKernel &rclB) : MeshEvalTopology(rclB) {}
  virtual ~MeshEvalSingleFacet () {}
  bool Evaluate ();
};

/**
 * The MeshFixSingleFacet class tries to fix a special case of non-manifolds.
 * @see MeshEvalSingleFacet
 */
class MeshExport MeshFixSingleFacet : public MeshValidation
{
public:
  MeshFixSingleFacet (MeshKernel &rclB, const std::vector<std::list<unsigned long> >& mf)
    : MeshValidation(rclB), _raclManifoldList(mf) {}
  virtual ~MeshFixSingleFacet () {}
  bool Fixup();

protected:
  const std::vector<std::list<unsigned long> >& _raclManifoldList;
};

// ----------------------------------------------------

/**
 * The MeshEvalSelfIntersection class checks the mesh for self intersection.
 * @author Werner Mayer
 */
class MeshExport MeshEvalSelfIntersection : public MeshEvaluation
{
public:
    MeshEvalSelfIntersection (const MeshKernel &rclB) : MeshEvaluation(rclB) {}
    virtual ~MeshEvalSelfIntersection () {}
    /// Evaluate the mesh and return if true if there are self intersections
    bool Evaluate ();
    /// collect all intersection lines
    void GetIntersections(const std::vector<std::pair<unsigned long, unsigned long> >&,
        std::vector<std::pair<Base::Vector3f, Base::Vector3f> >&) const;
    /// collect the index of all facets with self intersections
    void GetIntersections(std::vector<std::pair<unsigned long, unsigned long> >&) const;
};

/**
 * The MeshFixSelfIntersection class tries to fix self-intersections.
 * @see MeshEvalSingleFacet
 */
class MeshExport MeshFixSelfIntersection : public MeshValidation
{
public:
    MeshFixSelfIntersection (MeshKernel &rclB, const std::vector<std::pair<unsigned long, unsigned long> >& si)
        : MeshValidation(rclB), selfIntersectons(si) {}
    virtual ~MeshFixSelfIntersection () {}
    bool Fixup();

private:
    const std::vector<std::pair<unsigned long, unsigned long> >& selfIntersectons;
};

// ----------------------------------------------------

/**
 * The MeshEvalNeighbourhood class checks if the neighbourhood among the facets is
 * set correctly.
 * @author Werner Mayer
 */
class MeshExport MeshEvalNeighbourhood : public MeshEvaluation
{
public:
  MeshEvalNeighbourhood (const MeshKernel &rclB) : MeshEvaluation(rclB) {}
  ~MeshEvalNeighbourhood () {}
  bool Evaluate ();
  std::vector<unsigned long> GetIndices() const;
};

/**
 * The MeshFixNeighbourhood class fixes the neighbourhood of the facets.
 * @author Werner Mayer
 */
class MeshExport MeshFixNeighbourhood : public MeshValidation
{
public:
  MeshFixNeighbourhood (MeshKernel &rclB) : MeshValidation(rclB) {}
  ~MeshFixNeighbourhood () {}
  bool Fixup();
};

// ----------------------------------------------------

/**
 * The MeshEigensystem class actually does not try to check for or fix errors but
 * it provides methods to calculate the mesh's local coordinate system with the center
 * of gravity as origin.
 * The local coordinate system is computed this way that u has minimum and w has maximum 
 * expansion. The local coordinate system is right-handed.
 * @author Werner Mayer
 */
class MeshExport MeshEigensystem : public MeshEvaluation
{
public:
  MeshEigensystem (const MeshKernel &rclB);
  virtual ~MeshEigensystem () {}

  /** Returns the transformation matrix. */
  Base::Matrix4D Transform() const;
  /**
   * Returns the expansions in \a u, \a v and \a w of the bounding box.
   */
  Base::Vector3f GetBoundings() const;

  bool Evaluate();
  /** 
   * Calculates the local coordinate system defined by \a u, \a v, \a w
   * and \a c. 
   */
protected:
  void CalculateLocalSystem();

private:
  Base::Vector3f _cU, _cV, _cW, _cC; /**< Vectors that define the local coordinate system. */
  float _fU, _fV, _fW; /**< Expansion in \a u, \a v, and \a w direction of the transformed mesh. */
};

} // namespace MeshCore

#endif // MESH_EVALUATION_H
