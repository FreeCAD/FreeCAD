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
// $Id: G4GDMLWriteSetup.cc,v 1.13 2009/04/24 15:34:20 gcosmo Exp $
// GEANT4 tag $Name: geant4-09-03-patch-01 $
//
// class G4GDMLWriteSetup Implementation
//
// Original author: Zoltan Torzsok, November 2007
//
// --------------------------------------------------------------------

#include "G4GDMLWriteSetup.hh"

///#include "G4LogicalVolume.hh"
#include "../geometry/management/G4LogicalVolume.hh"

G4GDMLWriteSetup::G4GDMLWriteSetup() : G4GDMLWriteSolids()
{
}

G4GDMLWriteSetup::~G4GDMLWriteSetup()
{
}

void G4GDMLWriteSetup::SetupWrite(xercesc::DOMElement* gdmlElement,
                                  const G4LogicalVolume* const logvol)
{
   //G4cout << "G4GDML: Writing setup..." << G4endl;

   const G4String worldref = GenerateName(logvol->GetName(),logvol);

   xercesc::DOMElement* setupElement = NewElement("setup");
   setupElement->setAttributeNode(NewAttribute("version","1.0"));
   setupElement->setAttributeNode(NewAttribute("name","Default"));
   xercesc::DOMElement* worldElement = NewElement("world");
   worldElement->setAttributeNode(NewAttribute("ref",worldref));
   setupElement->appendChild(worldElement);
   gdmlElement->appendChild(setupElement);
}
