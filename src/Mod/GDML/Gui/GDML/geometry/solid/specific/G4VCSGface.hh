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
// $Id: G4VCSGface.hh,v 1.9 2008/05/15 11:41:59 gcosmo Exp $
// GEANT4 tag $Name: geant4-09-03-patch-01 $
//
// 
// --------------------------------------------------------------------
// GEANT 4 class header file
//
//
// G4VCSGface
//
// Class description:
//
//   Definition of the virtual base class G4VCSGface, one side (or face)
//   of a CSG-like solid. It should be possible to build a CSG entirely out of
//   connecting CSG faces.
//
//   Each face has an inside and outside surface, the former represents
//   the inside of the volume, the latter, the outside.
//
//   Virtual members:
//
//  -------------------------------------------------------------------
//      Intersect( const G4ThreeVector &p, const G4ThreeVector &v,
//                 G4bool outGoing, G4double surfTolerance,
//                 G4double &distance, G4double &distFromSurface,
//                 G4ThreeVector &normal, G4bool &allBehind );
//
//          p               - (in) position
//          v               - (in) direction (assumed to be a unit vector)
//          outgoing        - (in) true, to consider only inside surfaces
//                                 false, to consider only outside surfaces
//          distance        - (out) distance to intersection
//          distFromSurface - (out) distance from surface (along surface normal),
//                            < 0 if the point is in front of the surface
//          normal          - (out) normal of surface at intersection point
//          allBehind       - (out) true, if entire surface is behind normal
//
//          return value = true if there is an intersection,
//                         false if there is no intersection
//                               (all output arguments undefined)
//
//   Determine the distance along a line to the face.
//
//  -------------------------------------------------------------------
//   Distance( const G4ThreeVector &p, const G4bool outgoing );
//
//      p         - (in) position
//      outgoing  - (in) true, to consider only inside surfaces
//                             false, to consider only outside surfaces
//
//      return value = distance to closest surface satisifying requirements
//                         or kInfinity if no such surface exists
//
//   Determine the distance of a point from either the inside or outside
//   surfaces of the face.
//
//  -------------------------------------------------------------------
//       Inside( const G4ThreeVector &p, const G4double tolerance, 
//               G4double *bestDistance );
//
//      p            - (in) position
//      tolerance    - (in) tolerance defining the bounds of the "kSurface",
//                          nominally equal to kCarTolerance/2
//      bestDistance - (out) distance to closest surface (in or out)
//
//      return value = kInside if the point is closest to the inside surface
//                     kOutside if the point is closest to the outside surface
//                     kSurface if the point is withing tolerance of the surface
//
//   Determine whether a point is inside, outside, or on the surface of
//   the face.
//
//  -------------------------------------------------------------------
//       Normal( const G4ThreeVector &p,  G4double *bestDistance );
//
//       p            - (in) position
//       bestDistance - (out) distance to closest surface (in or out)
//
//       return value = the normal of the surface nearest the point
//
//   Return normal of surface closest to the point.
//
//  -------------------------------------------------------------------
//   Extent( const G4ThreeVector axis );
//
//       axis    - (in) unit vector defining direction
//
//       return value = the largest point along the given axis of the
//                      the face's extent.
//
//  -------------------------------------------------------------------
//       CalculateExtent( const EAxis pAxis,
//                        const G4VoxelLimit &pVoxelLimit,
//                        const G4AffineTransform &pTransform,
//                        G4double &min, G4double &max )
//
//           pAxis       - (in) The x,y, or z axis in which to check
//                              the shapes 3D extent against
//           pVoxelLimit - (in) Limits along x, y, and/or z axes
//           pTransform  - (in) A coordinate transformation on which
//                              to apply to the shape before testing
//           min         - (out) If the face has any point on its
//                               surface after tranformation and limits
//                               along pAxis that is smaller than the value
//                               of min, than it is used to replace min.
//                               Undefined if the return value is false.
//           max         - (out) Same as min, except for the largest
//                               point.
//                               Undefined if the return value is false.
//
//           return value = true if anything remains of the face
//
//   Calculate the extent of the face for the voxel navigator.
//   In analogy with CalculateExtent for G4VCSGfaceted, this is
//   done in the following steps:
//
//          1. Transform the face using pTranform, an arbitrary 3D 
//             rotation/offset/reflection
//          2. Clip the face to those boundaries as specified in
//             pVoxelLimit. This may include limits in any number
//             of x, y, or z axes.
//          3. For each part of the face that remains (there could
//             be many separate pieces in general):
//                4. Check to see if the piece overlaps the currently
//                   existing limits along axis pAxis. For 
//                   pVoxelLimit.IsLimited(pAxis) = false, there are
//                   no limits.
//                5. For a piece that does overlap, update min/max
//                   accordingly (within confines of pre-existing
//                   limits) along the direction pAxis.
//          6. If min/max were updated, return true
//                           
//  -------------------------------------------------------------------
//       G3VCSGface *Clone()
//
//   This method is invoked by G4CSGfaceted during the copy constructor
//   or the assignment operator. Its purpose is to return a pointer
//   (of type G4VCSGface) to a duplicate copy of the face.
//   The implementation is straight forward for inherited classes. Example:
//
//   G4VCSGface G4PolySideFace::Clone() { return new G4PolySideFace(*this); }
//
//   Of course, this assumes the copy constructor of G4PolySideFace is
//   correctly implemented.
//
//   Implementation notes:
//   * distance.
//        The meaning of distance includes the boundaries of the face.
//        For example, for a rectangular, planer face:
//
//               A   |  B           | C
//                   |              |
//              -------+--------------+-----
//               D   |  I           | E
//                   |              |
//              -------+--------------+-----
//               F   |  G           | H
//                   |              |
//       
//        A, C, F, and H: closest distance is the distance to
//        the adjacent corner.
//
//        B, D, E, and G: closest distance is the distance to
//        the adjacent line.
//
//        I: normal distance to plane
//
//        For non-planer faces, one can use the normal to decide when
//        a point falls off the edge and then act accordingly.
//
//
//   Usage:
//
//   A CSG shape can be defined by putting together any number of generic
//   faces, as long as the faces cover the entire surface of the shape
//   without overlapping.
//
//   G4VSolid::CalculateExtent
//
//   Define unit vectors along the specified transform axis.
//   Use the inverse of the specified coordinate transformation to rotate
//   these unit vectors. Loop over each face, call face->Extent, and save
//   the maximum value.
//
//   G4VSolid::Inside
//
//   To decide if a point is inside, outside, or on the surface of the shape,
//   loop through all faces, and find the answer from face->Inside which gives
//   a value of "bestDistance" smaller than any other. While looping, if any
//   face->Inside returns kSurface, this value can be returned immediately.
//
//  EInside answer;
//  G4VCSGface *face = faces;
//  G4double best = kInfinity;
//  do {
//    G4double distance;
//    EInside result = (*face)->Inside( p, kCarTolerance/2, distance );
//    if (result == kSurface) return kSurface;
//    if (distance < best) {
//      best = distance;
//      answer = result;
//    }
//  } while( ++face < faces + numFaces );
//
//  return(answer);
//
//   G4VSolid::SurfaceNormal
//
//   Loop over all faces, call face->Normal, and return the normal to the face 
//   that is closest to the point.
//
//   G4VSolid::DistanceToIn(p)
//
//   Loop over all faces, invoking face->Distance with outgoing = false,
//   and save the answer that is smallest.
//
//   G4VSolid::DistanceToIn(p,v)
//
//   Loop over all faces, invoking face->Intersect with outgoing = false,
//   and save the answer that is smallest.
//
//   G4VSolid::DistanceToOut(p)
//
//   Loop over all faces, invoking face->Distance with outgoing = true,
//   and save the answer that is smallest.
//
//   G4VSolid::DistanceToOut(p,v)
//
//   Loop over all faces, invoking face->Intersect with outgoing = true,
//   and save the answer that is smallest. If there is more than one answer,
//   or if allBehind is false for the one answer, return validNorm as false.

// Author:
//   David C. Williams (davidw@scipp.ucsc.edu)
// --------------------------------------------------------------------
#ifndef G4VCSGface_hh
#define G4VCSGface_hh

///#include "G4Types.hh"
#include "../../../global/G4Types.hh"
///#include "G4ThreeVector.hh"
#include "../../../global/G4ThreeVector.hh"
///#include "geomdefs.hh"
#include "../../../global/geomdefs.hh"
///#include "G4VSolid.hh"
#include "../../management/G4VSolid.hh"

class G4VoxelLimits;
class G4AffineTransform;
class G4SolidExtentList;

class G4VCSGface
{
  public:  // with description

  G4VCSGface() {}
  virtual ~G4VCSGface() {}
  
  virtual G4bool Intersect( const G4ThreeVector &p, const G4ThreeVector &v,  
                            G4bool outgoing, G4double surfTolerance,
                            G4double &distance, G4double &distFromSurface,
                            G4ThreeVector &normal, G4bool &allBehind ) = 0;

  virtual G4double Distance( const G4ThreeVector &p, G4bool outgoing ) = 0;
  
  virtual EInside Inside( const G4ThreeVector &p, G4double tolerance, 
                          G4double *bestDistance ) = 0;
    
  virtual G4ThreeVector Normal( const G4ThreeVector &p,
                                G4double *bestDistance ) = 0;

  virtual G4double Extent( const G4ThreeVector axis ) = 0;
  
  virtual void CalculateExtent( const EAxis axis, 
                                const G4VoxelLimits &voxelLimit,
                                const G4AffineTransform &tranform,
                                G4SolidExtentList &extentList       ) = 0;

  virtual G4VCSGface* Clone() = 0;

  virtual G4double SurfaceArea( ) = 0;
  virtual G4ThreeVector GetPointOnFace() = 0;
};

#endif
