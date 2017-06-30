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
// $Id: G4EnclosingCylinder.hh,v 1.8 2006/06/29 18:46:58 gunter Exp $
// GEANT4 tag $Name: geant4-09-03-patch-01 $
//
//
// --------------------------------------------------------------------
// GEANT 4 class header file
//
// G4EnclosingCylinder
//
// Class description:
//
//   Definition of a utility class for quickly deciding if a point
//   is clearly outside a polyhedra or polycone or deciding if
//   a trajectory is clearly going to miss those shapes.

// Author: 
//   David C. Williams (davidw@scipp.ucsc.edu)
// --------------------------------------------------------------------
#ifndef G4EnclosingCylinder_hh
#define G4EnclosingCylinder_hh

///#include "G4Types.hh"
///#include "geomdefs.hh"
///#include "G4ThreeVector.hh"
#include "../../../global/G4Types.hh"
#include "../../../global/geomdefs.hh"
#include "../../../global/G4ThreeVector.hh"

class G4ReduciblePolygon;

class G4EnclosingCylinder
{
  public:  // with description

    G4EnclosingCylinder( const G4ReduciblePolygon *rz,
                               G4bool phiIsOpen, 
                               G4double startPhi, G4double totalPhi );
    ~G4EnclosingCylinder();
  
    G4bool MustBeOutside( const G4ThreeVector &p ) const;
      // Decide very rapidly if the point is outside the cylinder.
      // If one is not certain, return false.

    G4bool ShouldMiss( const G4ThreeVector &p, const G4ThreeVector &v ) const;
      // Decide very rapidly if the trajectory is going to miss the cylinder.
      // If one is not sure, return false.

  public:  // without description

    G4EnclosingCylinder(__void__&);
      // Fake default constructor for usage restricted to direct object
      // persistency for clients requiring preallocation of memory for
      // persistifiable objects.

  protected:

    G4double radius;    // radius of our cylinder
    G4double zLo, zHi;  // z extent
  
    G4bool    phiIsOpen; // true if there is a phi segment
    G4double  startPhi,  // for isPhiOpen==true, starting of phi segment
              totalPhi;  // for isPhiOpen==true, size of phi segment

    G4double rx1, ry1,
             dx1, dy1;
    G4double rx2, ry2,
             dx2, dy2;
     
    G4bool   concave;  // true, if x/y cross section is concave
    
};

#endif
