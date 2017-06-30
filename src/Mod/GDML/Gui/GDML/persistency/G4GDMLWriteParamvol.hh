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
// $Id: G4GDMLWriteParamvol.hh,v 1.13 2009/04/24 15:34:20 gcosmo Exp $
// GEANT4 tag $Name: geant4-09-03-patch-01 $
//
//
// class G4GDMLWriteParamvol
//
// Class description:
//
// GDML class for writing parameterised entities dimensions.

// History:
// - Created.                                  Zoltan Torzsok, November 2007
// -------------------------------------------------------------------------

#ifndef _G4GDMLWRITEPARAMVOL_INCLUDED_
#define _G4GDMLWRITEPARAMVOL_INCLUDED_

#include "G4GDMLWriteSetup.hh"

class G4Box;
class G4Trd;
class G4Trap;
class G4Tubs;
class G4Cons;
class G4Sphere;
class G4Orb;
class G4Torus;
class G4Para;
class G4Hype;
class G4VPhysicalVolume;

class G4GDMLWriteParamvol : public G4GDMLWriteSetup
{

 public:

   virtual void ParamvolWrite(xercesc::DOMElement*,
                              const G4VPhysicalVolume* const);
   virtual void ParamvolAlgorithmWrite(xercesc::DOMElement* paramvolElement,
                              const G4VPhysicalVolume* const paramvol);

 protected:

   G4GDMLWriteParamvol();
   virtual ~G4GDMLWriteParamvol();

   void Box_dimensionsWrite(xercesc::DOMElement*, const G4Box* const);
   void Trd_dimensionsWrite(xercesc::DOMElement*, const G4Trd* const);
   void Trap_dimensionsWrite(xercesc::DOMElement*, const G4Trap* const);
   void Tube_dimensionsWrite(xercesc::DOMElement*, const G4Tubs* const);
   void Cone_dimensionsWrite(xercesc::DOMElement*, const G4Cons* const);
   void Sphere_dimensionsWrite(xercesc::DOMElement*, const G4Sphere* const);
   void Orb_dimensionsWrite(xercesc::DOMElement*, const G4Orb* const);
   void Torus_dimensionsWrite(xercesc::DOMElement*, const G4Torus* const);
   void Para_dimensionsWrite(xercesc::DOMElement*, const G4Para* const);
   void Hype_dimensionsWrite(xercesc::DOMElement*, const G4Hype* const);
   void ParametersWrite(xercesc::DOMElement*,
                        const G4VPhysicalVolume* const, const G4int&);

};

#endif
