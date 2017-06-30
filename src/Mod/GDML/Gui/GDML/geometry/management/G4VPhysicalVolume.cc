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
// $Id: G4VPhysicalVolume.cc,v 1.14 2007/04/11 08:00:12 gcosmo Exp $
// GEANT4 tag $Name: geant4-09-03-patch-01 $
//
// 
// class G4VPhysicalVolume Implementation
//
// --------------------------------------------------------------------

#include "G4VPhysicalVolume.hh"

#include "G4PhysicalVolumeStore.hh"
#include "G4LogicalVolume.hh"

// Constructor: init parameters and register in Store
//
G4VPhysicalVolume::G4VPhysicalVolume( G4RotationMatrix *pRot,
                                const G4ThreeVector &tlate,
                                const G4String& pName,
                                      G4LogicalVolume* pLogical,
                                      G4VPhysicalVolume* )
                                      //G4LogicalVolume* pv_cadgdml)//CAD-GDML not to use G4PVPlacement
  : frot(pRot), ftrans(tlate), flogical(pLogical),
    fname(pName),flmother(0)
{//flmother(0)
	//BEGIN CAD-GDML
	//SetMotherLogical(pv_cadgdml);
  //if (pv_cadgdml) { pv_cadgdml->AddDaughter(this); }
	//END CAD-GDML
  	
  G4PhysicalVolumeStore::Register(this);
}

// Fake default constructor - sets only member data and allocates memory
//                            for usage restricted to object persistency.
//
G4VPhysicalVolume::G4VPhysicalVolume( __void__& )
  : frot(0), flogical(0), fname(""), flmother(0)
{
  // Register to store
  //
  G4PhysicalVolumeStore::Register(this);
}

// Destructor -  remove from Store
//
G4VPhysicalVolume::~G4VPhysicalVolume() 
{
  G4PhysicalVolumeStore::DeRegister(this);
}

G4int G4VPhysicalVolume::GetMultiplicity() const
{
  return 1;
}

G4RotationMatrix* G4VPhysicalVolume::GetObjectRotation() const
{
  static G4RotationMatrix  aRotM; 
  static G4RotationMatrix  IdentityRM;  // Never changed (from "1")
  G4RotationMatrix* retval; 

  // Insure against frot being a null pointer
  if(frot)
  {
    aRotM= frot->inverse();
    retval= &aRotM;
  }
  else
  {
    retval= &IdentityRM;
  }
  return retval;
}

// Only implemented for placed and parameterised volumes.
// Not required for replicas.
//

G4bool G4VPhysicalVolume::CheckOverlaps(G4int, G4double, G4bool)
{
  return false;
}
