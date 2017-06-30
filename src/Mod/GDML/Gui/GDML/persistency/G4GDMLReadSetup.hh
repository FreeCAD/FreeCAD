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
// $Id: G4GDMLReadSetup.hh,v 1.6 2009/03/24 15:47:33 gcosmo Exp $
// GEANT4 tag $Name: geant4-09-03-patch-01 $
//
//
// class G4GDMLReadSetup
//
// Class description:
//
// GDML class for geometry setup of solids.

// History:
// - Created.                                  Zoltan Torzsok, November 2007
// -------------------------------------------------------------------------

#ifndef _G4GDMLREADSETUP_INCLUDED_
#define _G4GDMLREADSETUP_INCLUDED_

#include <map>

//#include "G4String.hh"
#include "../global/G4String.hh"

#include "G4GDMLReadSolids.hh"

class G4GDMLReadSetup : public G4GDMLReadSolids
{
 public:

   G4String GetSetup(const G4String&);

   virtual void SetupRead(const xercesc::DOMElement* const element);

 protected:

   G4GDMLReadSetup();
   virtual ~G4GDMLReadSetup();

 protected:

   std::map<G4String,G4String> setupMap;
};

#endif
