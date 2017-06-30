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
// $Id: G4GDMLParameterisation.hh,v 1.10 2008/07/16 15:46:33 gcosmo Exp $
// GEANT4 tag $Name: geant4-09-03-patch-01 $
//
//
// class G4GDMLParameterisation
//
// Class description:
//
// GDML class for interpretation of parameterisations.

// History:
// - Created.                                  Zoltan Torzsok, November 2007
// -------------------------------------------------------------------------

#ifndef _G4GDMLPARAMETERISATION_INCLUDED_
#define _G4GDMLPARAMETERISATION_INCLUDED_

#include "../geometry/management/G4VPVParameterisation.hh"
/*
#include "G4VPhysicalVolume.hh"
#include "G4ThreeVector.hh"
#include "G4Box.hh"
#include "G4Trd.hh"
#include "G4Trap.hh"
#include "G4Cons.hh"
#include "G4Sphere.hh"
#include "G4Orb.hh"
#include "G4Torus.hh"
#include "G4Para.hh"
#include "G4Hype.hh"
#include "G4Tubs.hh"
#include "G4Polycone.hh"
#include "G4Polyhedra.hh"
#include "G4RotationMatrix.hh"
#include "G4ThreeVector.hh"
*/
#include <vector>

//class G4GDMLParameterisation : public G4VPVParameterisation
/*
class G4GDMLParameterisation 
{

 public:

   struct PARAMETER
   {
      G4RotationMatrix* pRot;
      G4ThreeVector position;
      G4double dimension[16];

      PARAMETER() { memset(dimension,0,sizeof(dimension)); }
   };

   G4int GetSize() const;
   void  AddParameter(const PARAMETER&);

 private:

   void ComputeTransformation(const G4int,G4VPhysicalVolume*) const;
   void ComputeDimensions(G4Box&,const G4int,const G4VPhysicalVolume*) const;
   void ComputeDimensions(G4Trd&,const G4int,const G4VPhysicalVolume*) const;
   void ComputeDimensions(G4Trap&,const G4int,const G4VPhysicalVolume*) const;
   void ComputeDimensions(G4Cons&,const G4int,const G4VPhysicalVolume*) const;
   void ComputeDimensions(G4Sphere&,const G4int,const G4VPhysicalVolume*) const;
   void ComputeDimensions(G4Orb&,const G4int,const G4VPhysicalVolume*) const;
   void ComputeDimensions(G4Torus&,const G4int,const G4VPhysicalVolume*) const;
   void ComputeDimensions(G4Para&,const G4int,const G4VPhysicalVolume*) const;
   void ComputeDimensions(G4Hype&,const G4int,const G4VPhysicalVolume*) const;
   void ComputeDimensions(G4Tubs&,const G4int,const G4VPhysicalVolume*) const;
   void ComputeDimensions(G4Polycone&,const G4int,const G4VPhysicalVolume*) const;
   void ComputeDimensions(G4Polyhedra&,const G4int,const G4VPhysicalVolume*) const;

 private:

   std::vector<PARAMETER> parameterList;

};
*/
#endif
