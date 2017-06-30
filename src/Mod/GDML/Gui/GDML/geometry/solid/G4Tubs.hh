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
// $Id: G4Tubs.hh,v 1.22 2009/03/26 16:25:44 gcosmo Exp $
// GEANT4 tag $Name: geant4-09-03-patch-01 $
//
// 
// --------------------------------------------------------------------
// GEANT 4 class header file
//
// 
// G4Tubs
//
// Class description:
//
//   A tube or tube segment with curved sides parallel to
//   the z-axis. The tube has a specified half-length along
//   the z-axis, about which it is centered, and a given
//   minimum and maximum radius. A minimum radius of 0
//   corresponds to filled tube /cylinder. The tube segment is
//   specified by starting and delta angles for phi, with 0
//   being the +x axis, PI/2 the +y axis.
//   A delta angle of 2PI signifies a complete, unsegmented
//   tube/cylinder.
//
//   Member Data:
//
//   fRMin  Inner radius
//   fRMax  Outer radius
//   fDz  half length in z
//
//   fSPhi  The starting phi angle in radians,
//          adjusted such that fSPhi+fDPhi<=2PI, fSPhi>-2PI
//
//   fDPhi  Delta angle of the segment.
//
//   fPhiFullTube   Boolean variable used for indicate the Phi Section

// History:
// 10.08.95 P.Kent: General cleanup, use G4VSolid extent helper functions
//                  to CalculateExtent()
// 23.01.94 P.Kent: Converted to `tolerant' geometry
// 19.07.96 J.Allison: G4GraphicsScene - see G4Box
// 22.07.96 J.Allison: Changed SendPolyhedronTo to CreatePolyhedron
// --------------------------------------------------------------------

#ifndef G4TUBS_HH
#define G4TUBS_HH
///BEGIN CAD-GDML
#include "../../CLHEP/units/PhysicalConstants.h"
using namespace CLHEP;
///END CAD-GDML


#include "G4CSGSolid.hh"

class G4Tubs : public G4CSGSolid
{
  public:  // with description

    G4Tubs( const G4String& pName,
                  G4double pRMin,
                  G4double pRMax,
                  G4double pDz,
                  G4double pSPhi,
                  G4double pDPhi );
      //
      // Constructs a tubs with the given name and dimensions

   ~G4Tubs();
      //
      // Destructor

    // Accessors
    
    inline G4double GetInnerRadius   () const;
    inline G4double GetOuterRadius   () const;
    inline G4double GetZHalfLength   () const;
    inline G4double GetStartPhiAngle () const;
    inline G4double GetDeltaPhiAngle () const;

    // Modifiers

    inline void SetInnerRadius   (G4double newRMin);
    inline void SetOuterRadius   (G4double newRMax);
    inline void SetZHalfLength   (G4double newDz);
    inline void SetStartPhiAngle (G4double newSPhi, G4bool trig=true);
    inline void SetDeltaPhiAngle (G4double newDPhi);
    
    // Methods for solid

    inline G4double GetCubicVolume();
    inline G4double GetSurfaceArea();

    void ComputeDimensions(       G4VPVParameterisation* p,
                            const G4int n,
                            const G4VPhysicalVolume* pRep );

    G4bool CalculateExtent( const EAxis pAxis,
                            const G4VoxelLimits& pVoxelLimit,
                            const G4AffineTransform& pTransform,
                                  G4double& pmin, G4double& pmax ) const;

    EInside Inside( const G4ThreeVector& p ) const;

    G4ThreeVector SurfaceNormal( const G4ThreeVector& p ) const;

    G4double DistanceToIn(const G4ThreeVector& p, const G4ThreeVector& v) const;
    G4double DistanceToIn(const G4ThreeVector& p) const;
    G4double DistanceToOut(const G4ThreeVector& p, const G4ThreeVector& v,
                           const G4bool calcNorm=G4bool(false),
                                 G4bool *validNorm=0, G4ThreeVector *n=0) const;
    G4double DistanceToOut(const G4ThreeVector& p) const;

    G4GeometryType GetEntityType() const;

    G4ThreeVector GetPointOnSurface() const;

    std::ostream& StreamInfo( std::ostream& os ) const;

    // Visualisation functions

    void                DescribeYourselfTo ( G4VGraphicsScene& scene ) const;
    G4Polyhedron*       CreatePolyhedron   () const;
    G4NURBS*            CreateNURBS        () const;

  public:  // without description

    G4Tubs(__void__&);
      //
      // Fake default constructor for usage restricted to direct object
      // persistency for clients requiring preallocation of memory for
      // persistifiable objects.

    //  Older names for access functions

    inline G4double GetRMin() const;
    inline G4double GetRMax() const;
    inline G4double GetDz  () const;
    inline G4double GetSPhi() const;
    inline G4double GetDPhi() const;

  private:

    G4ThreeVectorList*
    CreateRotatedVertices( const G4AffineTransform& pTransform ) const;
      //
      // Creates the List of transformed vertices in the format required
      // for G4VSolid:: ClipCrossSection and ClipBetweenSections

    inline void Initialize();
      //
      // Reset relevant values to zero

    inline void CheckSPhiAngle(G4double sPhi);
    inline void CheckDPhiAngle(G4double dPhi);
    inline void CheckPhiAngles(G4double sPhi, G4double dPhi);
      //
      // Reset relevant flags and angle values

    inline void InitializeTrigonometry();
      //
      // Recompute relevant trigonometric values and cache them

    G4ThreeVector ApproxSurfaceNormal( const G4ThreeVector& p ) const;
      //
      // Algorithm for SurfaceNormal() following the original
      // specification for points not on the surface

  private:

    // Used by distanceToOut
    //
    enum ESide {kNull,kRMin,kRMax,kSPhi,kEPhi,kPZ,kMZ};

    // Used by normal
    //
    enum ENorm {kNRMin,kNRMax,kNSPhi,kNEPhi,kNZ};

    G4double kRadTolerance, kAngTolerance;
      //
      // Radial and angular tolerances

    G4double fRMin, fRMax, fDz, fSPhi, fDPhi;
      //
      // Radial and angular dimensions
   
    G4double sinCPhi, cosCPhi, cosHDPhiOT, cosHDPhiIT,
             sinSPhi, cosSPhi, sinEPhi, cosEPhi;
      //
      // Cached trigonometric values

    G4bool fPhiFullTube;
      //
      // Flag for identification of section or full tube
};

#include "G4Tubs.icc"

#endif
