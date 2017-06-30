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
// $Id: G4IntersectingCone.hh,v 1.11 2008/04/28 08:59:47 gcosmo Exp $
// GEANT4 tag $Name: geant4-09-03-patch-01 $
//
// 
// --------------------------------------------------------------------
// GEANT 4 class header file
//
//
// G4IntersectingCone
//
// Class description:
//
//   Utility class which calculates the intersection
//   of an arbitrary line with a fixed cone

// Author: 
//   David C. Williams (davidw@scipp.ucsc.edu)
// --------------------------------------------------------------------
#ifndef G4IntersectingCone_hh
#define G4IntersectingCone_hh

///#include "G4Types.hh"
///#include "geomdefs.hh"
///#include "G4ThreeVector.hh"
#include "../../../global/G4Types.hh"
#include "../../../global/geomdefs.hh"
#include "../../../global/G4ThreeVector.hh"

class G4IntersectingCone
{
  public:

    G4IntersectingCone( const G4double r[2], const G4double z[2] );
    virtual ~G4IntersectingCone();
  
    G4int LineHitsCone( const G4ThreeVector &p, const G4ThreeVector &v,
                              G4double *s1, G4double *s2 );
  
    G4bool HitOn( const G4double r, const G4double z );
  
    inline G4double RLo() const { return rLo; }
    inline G4double RHi() const { return rHi; }
    inline G4double ZLo() const { return zLo; }
    inline G4double ZHi() const { return zHi; }
  
  public:  // without description

    G4IntersectingCone(__void__&);
      // Fake default constructor for usage restricted to direct object
      // persistency for clients requiring preallocation of memory for
      // persistifiable objects.


  protected:

    G4double zLo, zHi,  // Z bounds of side
             rLo, rHi;  // R bounds of side

    G4bool   type1;    // True if cone is type 1
                       //  (std::abs(z1-z2)>std::abs(r1-r2))
    G4double A, B;     // Cone radius parameter:
                       //  type 1: r = A + B*z
                       //  type 2: z = A + B*r

    G4double half_kCarTolerance;  // half of Surface Thickness

    G4int LineHitsCone1( const G4ThreeVector &p, const G4ThreeVector &v,
                               G4double *s1, G4double *s2 );
    G4int LineHitsCone2( const G4ThreeVector &p, const G4ThreeVector &v,
                               G4double *s1, G4double *s2 );
};

#endif
