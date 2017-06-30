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
// $Id: G4VCSGfaceted.hh,v 1.17 2008/05/15 13:45:15 gcosmo Exp $
// GEANT4 tag $Name: geant4-09-03-patch-01 $
//
// 
// --------------------------------------------------------------------
// GEANT 4 class header file
//
//
// G4VCSGfaceted
//
// Class description:
//
//   Virtual class defining CSG-like type shape that is built entire
//   of G4CSGface faces.

// Author:
//   David C. Williams (davidw@scipp.ucsc.edu)
// --------------------------------------------------------------------

#ifndef G4VCSGfaceted_hh
#define G4VCSGfaceted_hh

///#include "G4VSolid.hh"
#include "../../management/G4VSolid.hh"

class G4VCSGface;
class G4VisExtent;

class G4VCSGfaceted : public G4VSolid 
{
  public:  // with description

    G4VCSGfaceted( const G4String& name );
    virtual ~G4VCSGfaceted();
  
    G4VCSGfaceted( const G4VCSGfaceted &source );
    const G4VCSGfaceted &operator=( const G4VCSGfaceted &source );
  
    virtual G4bool CalculateExtent( const EAxis pAxis,
                                    const G4VoxelLimits& pVoxelLimit,
                                    const G4AffineTransform& pTransform,
                                          G4double& pmin,G4double& pmax ) const;
  
    virtual EInside Inside( const G4ThreeVector& p ) const;

    virtual G4ThreeVector SurfaceNormal( const G4ThreeVector& p ) const;

    virtual G4double DistanceToIn( const G4ThreeVector& p,
                                   const G4ThreeVector& v ) const;
    virtual G4double DistanceToIn( const G4ThreeVector& p ) const;
    virtual G4double DistanceToOut( const G4ThreeVector& p,
                                    const G4ThreeVector& v,
                                    const G4bool calcNorm=false,
                                          G4bool *validNorm=0,
                                          G4ThreeVector *n=0 ) const;
    virtual G4double DistanceToOut( const G4ThreeVector& p ) const;

    virtual G4GeometryType GetEntityType() const;

    virtual std::ostream& StreamInfo(std::ostream& os) const;

    virtual G4Polyhedron* CreatePolyhedron() const = 0;

    virtual void DescribeYourselfTo( G4VGraphicsScene& scene ) const;

 ///   virtual G4VisExtent GetExtent() const;

    virtual G4Polyhedron* GetPolyhedron () const;

    G4int GetCubVolStatistics() const;
    G4double GetCubVolEpsilon() const;
    void SetCubVolStatistics(G4int st);
    void SetCubVolEpsilon(G4double ep);
    G4int GetAreaStatistics() const;
    G4double GetAreaAccuracy() const;
    void SetAreaStatistics(G4int st);
    void SetAreaAccuracy(G4double ep);

    virtual G4double GetCubicVolume();
      // Returns an estimation of the geometrical cubic volume of the
      // solid. Caches the computed value once computed the first time.
    virtual G4double GetSurfaceArea();
      // Returns an estimation of the geometrical surface area of the
      // solid. Caches the computed value once computed the first time.

  public:  // without description

    G4VCSGfaceted(__void__&);
      // Fake default constructor for usage restricted to direct object
      // persistency for clients requiring preallocation of memory for
      // persistifiable objects.

  protected:  // without description

    G4int    numFace;
    G4VCSGface **faces;
    G4double fCubicVolume;
    G4double fSurfaceArea;
    mutable G4Polyhedron* fpPolyhedron;

    virtual G4double DistanceTo( const G4ThreeVector &p,
                                 const G4bool outgoing ) const;

    G4ThreeVector GetPointOnSurfaceGeneric()const;
      // Returns a random point located on the surface of the solid 
      // in case of generic Polycone or generic Polyhedra.

    void CopyStuff( const G4VCSGfaceted &source );
    void DeleteStuff();

  private:

    G4int    fStatistics;
    G4double fCubVolEpsilon;
    G4double fAreaAccuracy;
      // Statistics, error accuracy for volume estimation.

};

#endif
