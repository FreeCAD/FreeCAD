//
// ********************************************************************
// * License and Disclaimer                                           *
// *                                                                  *
// * The  Geant4 software  is  copyright of the Copyright Holders  of *
// * the Geant4 Collaboration.  It is provided  under  the terms  and *
// * conditions of the Geant4 Software License,  included in the file *
// * LICENSE and available at  http://cern.ch/geant4/license .  These *
// * include a list of copyright holders.                             *
// *                                                                  *
// * Neither the authors of this software system, nor their employing *
// * institutes,nor the agencies providing financial support for this *
// * work  make  any representation or  warranty, express or implied, *
// * regarding  this  software system or assume any liability for its *
// * use.  Please see the license in the file  LICENSE  and URL above *
// * for the full disclaimer and the limitation of liability.         *
// *                                                                  *
// * This  code  implementation is the result of  the  scientific and *
// * technical work of the GEANT4 collaboration.                      *
// * By using,  copying,  modifying or  distributing the software (or *
// * any work based  on the software)  you  agree  to acknowledge its *
// * use  in  resulting  scientific  publications,  and indicate your *
// * acceptance of all terms of the Geant4 Software license.          *
// ********************************************************************
//
//
// $Id: G4VSolid.hh,v 1.29 2008/09/10 13:18:42 gcosmo Exp $
// GEANT4 tag $Name: geant4-09-03-patch-01 $
//
// 
// class G4VSolid
//
// Class description:
//
// Abstract base class for solids, physical shapes that can be tracked through.
// Each solid has a name, and the constructors and destructors automatically
// add and subtract them from the G4SolidStore, a singleton `master' List
// of available solids.
//
// This class defines, but does not implement, functions to compute
// distances to/from the shape. Functions are also defined
// to check whether a point is inside the shape, to return the
// surface normal of the shape at a given point, and to compute
// the extent of the shape. [see descriptions below]
//
// Some protected/private utility functions are implemented for the 
// clipping of regions for the computation of a solid's extent. Note that
// the clipping mechanism is presently inefficient.
// 
// Some visualization/graphics functions are also defined.
//
// Member Data:
//
// G4String fshapeName
//   - Name for this solid.

// History:
// 12.04.00 J.Allison     Implemented GetExtent() in terms of CalculateExtent()
// 17.06.98 J.Apostolakis Added pure virtual function GetEntityType()
// 26.07.96 P.Kent        Added ComputeDimensions for replication mechanism
// 27.03.96 J.Allison     Methods for visualisation 
// 30.06.95 P.Kent        Initial version, no scoping or visualisation functions
// --------------------------------------------------------------------
#ifndef G4VSOLID_HH
#define G4VSOLID_HH

///#include "G4Types.hh"
///#include "G4String.hh"
///#include "geomdefs.hh"
#include "../../global/G4Types.hh"
#include "../../global/G4String.hh"
#include "../../global/geomdefs.hh"

class G4AffineTransform;
class G4VoxelLimits;

class G4VPVParameterisation;
class G4VPhysicalVolume;

class G4VGraphicsScene;
class G4Polyhedron;
class G4NURBS;
class G4VisExtent;
class G4DisplacedSolid;

///#include "G4ThreeVector.hh"
#include "../../global/G4ThreeVector.hh"
#include <vector>

typedef std::vector<G4ThreeVector> G4ThreeVectorList;
typedef G4String   G4GeometryType;

class G4VSolid
{
  public:  // with description 

    G4VSolid(const G4String& name);
      // Creates a new shape, with the supplied name. No provision is made
      // for sharing a common name amongst multiple classes.
    virtual ~G4VSolid();
      // Default destructor.

    inline G4bool operator==( const G4VSolid& s ) const;
      // Return true only if addresses are the same.
/*
    friend std::ostream& operator<< ( std::ostream& os, const G4VSolid& e );
      // Streaming operator, using DumpInfo().
*/
    inline G4String GetName() const;
      // Returns the current shape's name.
    inline void SetName(const G4String& name);
      // Sets the current shape's name.

    inline G4double GetTolerance() const;
      // Returns the cached geometrical tolerance.

    virtual G4bool CalculateExtent(const EAxis pAxis,
				   const G4VoxelLimits& pVoxelLimit,
				   const G4AffineTransform& pTransform,
				   G4double& pMin, G4double& pMax) const = 0;
      // Calculate the minimum and maximum extent of the solid, when under the
      // specified transform, and within the specified limits. If the solid
      // is not intersected by the region, return false, else return true.

    virtual EInside Inside(const G4ThreeVector& p) const = 0;
      // Returns kOutside if the point at offset p is outside the shapes
      // boundaries plus Tolerance/2, kSurface if the point is <= Tolerance/2
      // from a surface, otherwise kInside.

    virtual G4ThreeVector SurfaceNormal(const G4ThreeVector& p) const = 0;
      // Returns the outwards pointing unit normal of the shape for the
      // surface closest to the point at offset p.

    virtual G4double DistanceToIn(const G4ThreeVector& p,
                                  const G4ThreeVector& v) const = 0;
      // Return the distance along the normalised vector v to the shape,
      // from the point at offset p. If there is no intersection, return
      // kInfinity. The first intersection resulting from `leaving' a
      // surface/volume is discarded. Hence, it is tolerant of points on
      // the surface of the shape.

    virtual G4double DistanceToIn(const G4ThreeVector& p) const = 0;
      // Calculate the distance to the nearest surface of a shape from an
      // outside point. The distance can be an underestimate.

    virtual G4double DistanceToOut(const G4ThreeVector& p,
				   const G4ThreeVector& v,
				   const G4bool calcNorm=false,
				   G4bool *validNorm=0,
				   G4ThreeVector *n=0) const = 0;
      // Return the distance along the normalised vector v to the shape,
      // from a point at an offset p inside or on the surface of the shape.
      // Intersections with surfaces, when the point is < Tolerance/2 from a
      // surface must be ignored.
      // If calcNorm==true:
      //    validNorm set true if the solid lies entirely behind or on the
      //              exiting surface.
      //    n set to exiting outwards normal vector (undefined Magnitude).
      //    validNorm set to false if the solid does not lie entirely behind
      //              or on the exiting surface
      // If calcNorm==false:
      //    validNorm and n are unused.
      //
      // Must be called as solid.DistanceToOut(p,v) or by specifying all
      // the parameters.

    virtual G4double DistanceToOut(const G4ThreeVector& p) const = 0;
      // Calculate the distance to the nearest surface of a shape from an
      // inside point. The distance can be an underestimate.

/*
    virtual void ComputeDimensions(G4VPVParameterisation* p,
	                           const G4int n,
                                   const G4VPhysicalVolume* pRep);
      // Throw exception if ComputeDimensions called frrom an illegal
      // derived class.

    virtual G4double GetCubicVolume();
      // Returns an estimation of the solid volume in internal units.
      // This method may be overloaded by derived classes to compute the
      // exact geometrical quantity for solids where this is possible,
      // or anyway to cache the computed value.
      // Note: the computed value is NOT cached.

    virtual G4double GetSurfaceArea();
      // Return an estimation of the solid surface area in internal units.
      // This method may be overloaded by derived classes to compute the
      // exact geometrical quantity for solids where this is possible,
      // or anyway to cache the computed value.
      // Note: the computed value is NOT cached.
*/
    virtual G4GeometryType  GetEntityType() const = 0;
      // Provide identification of the class of an object.
      // (required for persistency and STEP interface)

 ///   virtual G4ThreeVector GetPointOnSurface() const;
      // Returns a random point located on the surface of the solid.
/*
    virtual std::ostream& StreamInfo(std::ostream& os) const = 0;
      // Dumps contents of the solid to a stream.
    inline void DumpInfo() const;
      // Dumps contents of the solid to the standard output.

    // Visualization functions

    virtual void DescribeYourselfTo (G4VGraphicsScene& scene) const = 0;
      // A "double dispatch" function which identifies the solid
      // to the graphics scene.
    virtual G4VisExtent   GetExtent        () const;
      // Provide extent (bounding box) as possible hint to the graphics view.
*/    virtual G4Polyhedron* CreatePolyhedron () const;
/*    virtual G4NURBS*      CreateNURBS      () const;
      // Create a G4Polyhedron/G4NURBS/...  (It is the caller's responsibility
      // to delete it).  A null pointer means "not created".
*/    virtual G4Polyhedron* GetPolyhedron () const;
/*      // Smart access function - creates on request and stores for future
      // access.  A null pointer means "not available".

    virtual const G4VSolid* GetConstituentSolid(G4int no) const;
    virtual       G4VSolid* GetConstituentSolid(G4int no);
      // If the solid is made up from a Boolean operation of two solids,
      // return the "no" solid. If the solid is not a "Boolean", return 0.

    virtual const G4DisplacedSolid* GetDisplacedSolidPtr() const; 
    virtual       G4DisplacedSolid* GetDisplacedSolidPtr(); 
      // If the solid is a "G4DisplacedSolid", return a self pointer
      // else return 0.
*/
  public:  // without description

    G4VSolid(__void__&);
      // Fake default constructor for usage restricted to direct object
      // persistency for clients requiring preallocation of memory for
      // persistifiable objects.

    G4VSolid(const G4VSolid& rhs);
    G4VSolid& operator=(const G4VSolid& rhs); 
      // Copy constructor and assignment operator.

  protected:  // with description

    void CalculateClippedPolygonExtent(G4ThreeVectorList& pPolygon,
				       const G4VoxelLimits& pVoxelLimit,
				       const EAxis pAxis, 
				       G4double& pMin, G4double& pMax) const;
      // Calculate the maximum and minimum extents of the convex polygon
      // pPolygon along the axis pAxis, within the limits pVoxelLimit.
      //
      // If the minimum is <pMin pMin is set to the new minimum.
      // If the maximum is >pMax pMax is set to the new maximum.
      //
      // Modifications to pPolygon are made - it is left in an undefined state.

    void ClipCrossSection(G4ThreeVectorList* pVertices,
			  const G4int pSectionIndex,
			  const G4VoxelLimits& pVoxelLimit,
			  const EAxis pAxis, 
			  G4double& pMin, G4double& pMax) const;
      // Calculate the maximum and minimum extents of the polygon described
      // by the vertices: pSectionIndex->pSectionIndex+1->
      //                  pSectionIndex+2->pSectionIndex+3->pSectionIndex
      // in the List pVertices.
      //
      // If the minimum is <pMin pMin is set to the new minimum.
      // If the maximum is >pMax pMax is set to the new maximum.
      //
      // No modifications are made to pVertices.

    void ClipBetweenSections(G4ThreeVectorList* pVertices,
			     const G4int pSectionIndex,
			     const G4VoxelLimits& pVoxelLimit,
			     const EAxis pAxis, 
			     G4double& pMin, G4double& pMax) const;
      // Calculate the maximum and minimum extents of the polygons
      // joining the CrossSections at pSectionIndex->pSectionIndex+3 and
      //                              pSectionIndex+4->pSectionIndex7
      // in the List pVertices, within the boundaries of the voxel limits
      // pVoxelLimit.
      //
      // If the minimum is <pMin pMin is set to the new minimum.
      // If the maximum is >pMax pMax is set to the new maximum.
      //
      // No modifications are made to pVertices.

    void ClipPolygon(      G4ThreeVectorList& pPolygon,
		     const G4VoxelLimits& pVoxelLimit,
                     const EAxis              pAxis      ) const;
      // Clip the specified convex polygon to the given limits, where
      // the polygon is described by the vertices at (0),(1),...,(n),(0) in
      // pPolygon. 
      // If the polygon is completely clipped away, the polygon is cleared.


    G4double EstimateCubicVolume(G4int nStat, G4double epsilon) const;
      // Calculate cubic volume based on Inside() method.
      // Accuracy is limited by the second argument or the statistics
      // expressed by the first argument.

    G4double EstimateSurfaceArea(G4int nStat, G4double ell) const;
      // Calculate surface area only based on Inside() method.
      // Accuracy is limited by the second argument or the statistics
      // expressed by the first argument.

  protected:

    G4double kCarTolerance;      // Cached geometrical tolerance

  private:

    void ClipPolygonToSimpleLimits(G4ThreeVectorList& pPolygon,
				   G4ThreeVectorList& outputPolygon,
			     const G4VoxelLimits&     pVoxelLimit   ) const;
      // Clip the specified convex polygon to the given limits, storing the
      // result in outputPolygon. The voxel limits must be limited in one
      // *plane* only: This is achieved by having only x or y or z limits,
      // and either the minimum or maximum limit set to -+kInfinity
      // respectively.

    G4String fshapeName;	 // Name
};

#include "G4VSolid.icc"

#endif
