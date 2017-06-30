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
// $Id: G4GDMLReadParamvol.hh,v 1.8 2009/04/24 15:34:20 gcosmo Exp $
// GEANT4 tag $Name: geant4-09-03-patch-01 $
//
//
// class G4GDMLReadParamvol
//
// Class description:
//
// GDML class for importing parameterised geometrical entities.

// History:
// - Created.                                  Zoltan Torzsok, November 2007
// -------------------------------------------------------------------------

#ifndef _G4GDMLREADPARAMVOL_INCLUDED_
#define _G4GDMLREADPARAMVOL_INCLUDED_

#include "G4GDMLReadSetup.hh"
#include "G4GDMLParameterisation.hh"

class G4LogicalVolume;

class G4GDMLReadParamvol : public G4GDMLReadSetup
{

 public:
	 
///   virtual void ParamvolRead(const xercesc::DOMElement* const,G4LogicalVolume*);

   virtual void Paramvol_contentRead(const xercesc::DOMElement* const);

 protected:

   G4GDMLReadParamvol();
   virtual ~G4GDMLReadParamvol();
/*
   void Box_dimensionsRead(const xercesc::DOMElement* const,
                                 G4GDMLParameterisation::PARAMETER&);
   void Trd_dimensionsRead(const xercesc::DOMElement* const,
                                 G4GDMLParameterisation::PARAMETER&);
   void Trap_dimensionsRead(const xercesc::DOMElement* const,
                                 G4GDMLParameterisation::PARAMETER&);
   void Tube_dimensionsRead(const xercesc::DOMElement* const,
                                 G4GDMLParameterisation::PARAMETER&);
   void Cone_dimensionsRead(const xercesc::DOMElement* const,
                                 G4GDMLParameterisation::PARAMETER&);
   void Sphere_dimensionsRead(const xercesc::DOMElement* const,
                                 G4GDMLParameterisation::PARAMETER&);
   void Orb_dimensionsRead(const xercesc::DOMElement* const,
                                 G4GDMLParameterisation::PARAMETER&);
   void Torus_dimensionsRead(const xercesc::DOMElement* const,
                                 G4GDMLParameterisation::PARAMETER&);
   void Para_dimensionsRead(const xercesc::DOMElement* const,
                                 G4GDMLParameterisation::PARAMETER&);
   void Hype_dimensionsRead(const xercesc::DOMElement* const,
                                 G4GDMLParameterisation::PARAMETER&);

   void ParameterisedRead(const xercesc::DOMElement* const);

   void ParametersRead(const xercesc::DOMElement* const);

 protected:

   G4GDMLParameterisation* parameterisation;
   */
};

#endif
