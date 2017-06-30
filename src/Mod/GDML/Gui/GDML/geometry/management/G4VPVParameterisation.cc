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
// $Id: G4VPVParameterisation.cc,v 1.8 2007/07/16 08:40:13 gcosmo Exp $
// GEANT4 tag $Name: geant4-09-03-patch-01 $
//
// Default implementations for Parameterisations that do not
// parameterise solid and/or material.
// --------------------------------------------------------------------

#include "G4VPVParameterisation.hh"
#include "G4VPhysicalVolume.hh"
#include "G4LogicalVolume.hh"
//#include "G4VVolumeMaterialScanner.hh"

G4VPVParameterisation::G4VPVParameterisation()
{
}

G4VPVParameterisation::~G4VPVParameterisation()
{
}

G4VSolid*
G4VPVParameterisation::ComputeSolid(const G4int,
                                    G4VPhysicalVolume *pPhysicalVol) 
{
  return pPhysicalVol->GetLogicalVolume()->GetSolid();
}
       
G4Material*
G4VPVParameterisation::ComputeMaterial(const G4int,
                                       G4VPhysicalVolume *pPhysicalVol,
				       const G4VTouchable *) 
{
  return pPhysicalVol->GetLogicalVolume()->GetMaterial();
}


G4bool G4VPVParameterisation::IsNested() const
{
  return false;
}

G4VVolumeMaterialScanner* 
G4VPVParameterisation::GetMaterialScanner()
{
  return 0;
}
//   These enable material scan for nested parameterisations
