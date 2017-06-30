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
// $Id: G4GDMLReadMaterials.hh,v 1.11 2009/04/24 15:34:20 gcosmo Exp $
// GEANT4 tag $Name: geant4-09-03-patch-01 $
//
//
// class G4GDMLReadMaterials
//
// Class description:
//
// GDML class for loading isotopes, elements and materials according to
// specifications in Geant4.

// History:
// - Created.                                  Zoltan Torzsok, November 2007
// -------------------------------------------------------------------------

#ifndef _G4GDMLREADMATERIALS_INCLUDED_
#define _G4GDMLREADMATERIALS_INCLUDED_

///#include "G4Types.hh"
#include "../global/G4Types.hh"
#include "G4GDMLReadDefine.hh"

class G4Element;
class G4Isotope;
class G4Material;

class G4GDMLReadMaterials : public G4GDMLReadDefine 
{

 public:

   G4Element* GetElement(const G4String&, G4bool verbose=true) const;
   G4Isotope* GetIsotope(const G4String&, G4bool verbose=true) const;
   G4Material* GetMaterial(const G4String&, G4bool verbose=true) const;

   virtual void MaterialsRead(const xercesc::DOMElement* const);

 protected:

   G4GDMLReadMaterials();
   virtual ~G4GDMLReadMaterials();

   G4double AtomRead(const xercesc::DOMElement* const);
   G4int CompositeRead(const xercesc::DOMElement* const,G4String&);
   G4double DRead(const xercesc::DOMElement* const);
   G4double PRead(const xercesc::DOMElement* const);
   G4double TRead(const xercesc::DOMElement* const);
   void ElementRead(const xercesc::DOMElement* const);
   G4double FractionRead(const xercesc::DOMElement* const,G4String&);
   void IsotopeRead(const xercesc::DOMElement* const);
   void MaterialRead(const xercesc::DOMElement* const);
   void MixtureRead(const xercesc::DOMElement* const,G4Element*);
   void MixtureRead(const xercesc::DOMElement* const,G4Material*);
   void PropertyRead(const xercesc::DOMElement* const,G4Material*);

};

#endif
