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
// $Id: G4Trd.hh,v 1.16 2006/10/19 15:33:37 gcosmo Exp $
// GEANT4 tag $Name: geant4-09-03-patch-01 $
//
// 
// --------------------------------------------------------------------
// GEANT 4 class header file
//
//
// G4Trd
//
// Class description:
//
//   A G4Trd is a trapezoid with the x and y dimensions varying along z
//   functions:
//
//   Member Data:
//
//     fDx1    Half-length along x at the surface positioned at -dz
//     fDx2    Half-length along x at the surface positioned at +dz
//     fDy1    Half-length along y at the surface positioned at -dz
//     fDy2    Half-length along y at the surface positioned at +dz
//     fDz     Half-length along z axis

// History:
// 12.01.95 P.Kent: Old prototype code converted to thick geometry
// 17.02.95 P.Kent: Exiting normal return
// 19.08.96 P.Kent, V.Grichine: Fs in accordance with G4Box
// 21.04.97 J.Apostolakis: Added Set Methods
// 19.11.99 V.Grichine: kUndefined was added to Eside enum 
// --------------------------------------------------------------------

#ifndef G4TRD_HH
#define G4TRD_HH

#include "G4CSGSolid.hh"

class G4Trd : public G4CSGSolid 
{
  public:  // with description

    G4Trd( const G4String& pName,
                 G4double pdx1, G4double pdx2,
                 G4double pdy1, G4double pdy2,
                 G4double pdz );
      //
      // Constructs a trapezoid with name, and half lengths

    virtual ~G4Trd();
      //
      // Destructor

    // Accessors

    inline G4double GetXHalfLength1() const;
    inline G4double GetXHalfLength2() const;
    inline G4double GetYHalfLength1() const;
    inline G4double GetYHalfLength2() const;
    inline G4double GetZHalfLength()  const;

    // Modifiers

    inline void SetXHalfLength1(G4double val);
    inline void SetXHalfLength2(G4double val);
    inline void SetYHalfLength1(G4double val);
    inline void SetYHalfLength2(G4double val);
    inline void SetZHalfLength(G4double val);

    // Methods of solid

    inline G4double GetCubicVolume();
    inline G4double GetSurfaceArea();

    void ComputeDimensions(       G4VPVParameterisation* p,
                            const G4int n,
                            const G4VPhysicalVolume* pRep );

    G4bool CalculateExtent( const EAxis pAxis,
                            const G4VoxelLimits& pVoxelLimit,
                            const G4AffineTransform& pTransform,
                                  G4double& pMin, G4double& pMax ) const;

    EInside Inside( const G4ThreeVector& p ) const;

    G4ThreeVector SurfaceNormal( const G4ThreeVector& p ) const;

    G4double DistanceToIn( const G4ThreeVector& p,
                           const G4ThreeVector& v ) const;

    G4double DistanceToIn( const G4ThreeVector& p ) const;

    G4double DistanceToOut( const G4ThreeVector& p,
                            const G4ThreeVector& v,
                            const G4bool calcNorm=false,
                                  G4bool *validNorm=0,
                                  G4ThreeVector *n=0 ) const;

    G4double DistanceToOut( const G4ThreeVector& p ) const;

    void CheckAndSetAllParameters ( G4double pdx1, G4double pdx2,
                                    G4double pdy1, G4double pdy2,
                                    G4double pdz );

    void SetAllParameters ( G4double pdx1, G4double pdx2,
                            G4double pdy1, G4double pdy2,
                            G4double pdz );

    G4GeometryType GetEntityType() const;

    G4ThreeVector GetPointOnSurface() const; 

    std::ostream& StreamInfo( std::ostream& os ) const;

    // Visualisation functions

    void          DescribeYourselfTo (G4VGraphicsScene& scene) const;
    G4Polyhedron* CreatePolyhedron   () const;
    G4NURBS*      CreateNURBS        () const;

  public:  // without description

    G4Trd(__void__&);
      // Fake default constructor for usage restricted to direct object
      // persistency for clients requiring preallocation of memory for
      // persistifiable objects.

  protected:  // without description

    G4ThreeVectorList*
    CreateRotatedVertices( const G4AffineTransform& pTransform ) const;
      //
      // Creates the List of transformed vertices in the format required
      // for G4CSGSolid:: ClipCrossSection and ClipBetweenSections

    G4double fDx1,fDx2,fDy1,fDy2,fDz;

    // Codes for faces (kPX=plus x face,kMY= minus y face etc)

    enum ESide {kUndefined, kPX,kMX,kPY,kMY,kPZ,kMZ};

  private:

    G4ThreeVector ApproxSurfaceNormal( const G4ThreeVector& p ) const;
      // Algorithm for SurfaceNormal() following the original
      // specification for points not on the surface
};

#include "G4Trd.icc"

#endif
