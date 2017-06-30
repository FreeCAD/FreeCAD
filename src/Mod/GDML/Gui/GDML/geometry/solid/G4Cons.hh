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
// $Id: G4Cons.hh,v 1.22 2009/03/31 09:56:24 gcosmo Exp $
// GEANT4 tag $Name: geant4-09-03-patch-01 $
//
//
// --------------------------------------------------------------------
// GEANT 4 class header file
//
// G4Cons
//
// Class description:
//
//   A G4Cons is, in the general case, a Phi segment of a cone, with
//   half-length fDz, inner and outer radii specified at -fDz and +fDz.
//   The Phi segment is described by a starting fSPhi angle, and the
//   +fDPhi delta angle for the shape.
//   If the delta angle is >=2*pi, the shape is treated as continuous
//   in Phi
//
//   Member Data:
//
//  fRmin1  inside radius at  -fDz
//  fRmin2  inside radius at  +fDz
//  fRmax1  outside radius at -fDz
//  fRmax2  outside radius at +fDz
//  fDz  half length in z
//
//  fSPhi  starting angle of the segment in radians
//  fDPhi  delta angle of the segment in radians
//
//  fPhiFullCone   Boolean variable used for indicate the Phi Section
//
//   Note:
//      Internally fSPhi & fDPhi are adjusted so that fDPhi<=2PI,
//      and fDPhi+fSPhi<=2PI. This enables simpler comparisons to be
//      made with (say) Phi of a point.

// History:
// 19.3.94 P.Kent: Old C++ code converted to tolerant geometry
// 13.9.96 V.Grichine: Final modifications to commit
// --------------------------------------------------------------------

#ifndef G4Cons_HH
#define G4Cons_HH

#include "G4CSGSolid.hh"
//begin CAD-GDML
//for twopi decalration into G4Cons.icc
#include "../../CLHEP/units/PhysicalConstants.h"
using namespace CLHEP;
//END CAD-GDML


class G4Cons : public G4CSGSolid
{
  public:  // with description

    G4Cons(const G4String& pName,
                 G4double pRmin1, G4double pRmax1,
                 G4double pRmin2, G4double pRmax2,
                 G4double pDz,
                 G4double pSPhi, G4double pDPhi);
      //
      // Constructs a cone with the given name and dimensions

   ~G4Cons() ;
      //
      // Destructor

    // Accessors

    inline G4double GetInnerRadiusMinusZ() const;
    inline G4double GetOuterRadiusMinusZ() const;
    inline G4double GetInnerRadiusPlusZ()  const;
    inline G4double GetOuterRadiusPlusZ()  const;
    inline G4double GetZHalfLength()       const;
    inline G4double GetStartPhiAngle()     const;
    inline G4double GetDeltaPhiAngle()     const;
  
    // Modifiers

    inline void SetInnerRadiusMinusZ (G4double Rmin1 );
    inline void SetOuterRadiusMinusZ (G4double Rmax1 );
    inline void SetInnerRadiusPlusZ  (G4double Rmin2 );
    inline void SetOuterRadiusPlusZ  (G4double Rmax2 );
    inline void SetZHalfLength       (G4double newDz );
    inline void SetStartPhiAngle     (G4double newSPhi, G4bool trig=true);
    inline void SetDeltaPhiAngle     (G4double newDPhi);

    // Other methods for solid

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

    G4double DistanceToIn (const G4ThreeVector& p,
                           const G4ThreeVector& v) const;
    G4double DistanceToIn (const G4ThreeVector& p) const;
    G4double DistanceToOut(const G4ThreeVector& p,
                           const G4ThreeVector& v,
                           const G4bool calcNorm=G4bool(false),
                                 G4bool *validNorm=0,
                                 G4ThreeVector *n=0) const;             
    G4double DistanceToOut(const G4ThreeVector& p) const;

    G4GeometryType GetEntityType() const;
        
    G4ThreeVector GetPointOnSurface() const; 
        
    std::ostream& StreamInfo(std::ostream& os) const;

    // Visualisation functions

    void          DescribeYourselfTo( G4VGraphicsScene& scene ) const;
    G4Polyhedron* CreatePolyhedron() const;
    G4NURBS*      CreateNURBS() const;

  public:  // without description
       
    G4Cons(__void__&);
      //
      // Fake default constructor for usage restricted to direct object
      // persistency for clients requiring preallocation of memory for
      // persistifiable objects.

    //  Old access functions

    inline G4double    GetRmin1() const;
    inline G4double    GetRmax1() const;
    inline G4double    GetRmin2() const;
    inline G4double    GetRmax2() const;
    inline G4double    GetDz()    const;
    inline G4double    GetSPhi() const;
    inline G4double    GetDPhi() const;

  private:
 
    G4ThreeVectorList*
    CreateRotatedVertices(const G4AffineTransform& pTransform) const;
  
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

    G4ThreeVector ApproxSurfaceNormal(const G4ThreeVector& p) const;
      //
      // Algorithm for SurfaceNormal() following the original
      // specification for points not on the surface

  private:

    // Used by distanceToOut
    //
    enum ESide {kNull,kRMin,kRMax,kSPhi,kEPhi,kPZ,kMZ};
  
    // used by normal
    //
    enum ENorm {kNRMin,kNRMax,kNSPhi,kNEPhi,kNZ};

    G4double kRadTolerance, kAngTolerance;
      //
      // Radial and angular tolerances

    G4double fRmin1, fRmin2, fRmax1, fRmax2, fDz, fSPhi, fDPhi;
      //
      // Radial and angular dimensions

    G4double sinCPhi, cosCPhi, cosHDPhiOT, cosHDPhiIT,
             sinSPhi, cosSPhi, sinEPhi, cosEPhi;
      //
      // Cached trigonometric values

    G4bool fPhiFullCone;
      //
      // Flag for identification of section or full cone
};

#include "G4Cons.icc"

#endif
