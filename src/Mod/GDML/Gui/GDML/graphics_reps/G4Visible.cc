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
// $Id: G4Visible.cc,v 1.17 2006/11/07 11:53:16 allison Exp $
// GEANT4 tag $Name: geant4-09-03-patch-01 $
//
// 
// John Allison  30th October 1996
// Base class for all things visible, i.e., which have Vis Attributes.

#include "G4Visible.hh"
///#include "G4VisAttributes.hh"
///#include "G4ios.hh"
#include "../global/G4ios.hh"

G4Visible::G4Visible ()///:
///  fpVisAttributes (0),
///  fAllocatedVisAttributes (false)
{}

G4Visible::G4Visible (const G4Visible& visible){
/*  fAllocatedVisAttributes = visible.fAllocatedVisAttributes;
  if (fAllocatedVisAttributes)
    fpVisAttributes = new G4VisAttributes(visible.fpVisAttributes);
  else fpVisAttributes = visible.fpVisAttributes;*/
}
/*
G4Visible::G4Visible (const G4VisAttributes* pVA):
  fpVisAttributes (pVA),
  fAllocatedVisAttributes (false)
{}
*/
G4Visible::~G4Visible () {
///  if (fAllocatedVisAttributes) delete fpVisAttributes;
}

G4Visible& G4Visible::operator= (const G4Visible& rhs) {
/*  fAllocatedVisAttributes = rhs.fAllocatedVisAttributes;
  if (fAllocatedVisAttributes)
    fpVisAttributes = new G4VisAttributes(rhs.fpVisAttributes);
  else fpVisAttributes = rhs.fpVisAttributes;*/
  return *this;
}
/*
void G4Visible::SetVisAttributes (const G4VisAttributes& VA) {
  // Allocate G4VisAttributes on the heap in case the user specifies a
  // short-lived VA for a long-lived G4Visible.  Flag so that it can
  // be deleted in the destructor.
  // First delete any G4VisAttributes already on the heap...
  if (fAllocatedVisAttributes) delete fpVisAttributes;
  fpVisAttributes = new G4VisAttributes(VA);
  fAllocatedVisAttributes = true;
}


void G4Visible::SetVisAttributes (const G4VisAttributes* pVA) {
  // First delete any G4VisAttributes already on the heap...
  if (fAllocatedVisAttributes) delete fpVisAttributes;
  fpVisAttributes = pVA;
  fAllocatedVisAttributes = false;
}

G4bool G4Visible::operator != (const G4Visible& right) const {
  return *fpVisAttributes != *right.fpVisAttributes;
}

std::ostream& operator << (std::ostream& os, const G4Visible& v) {
  if (v.fpVisAttributes) return os << *(v.fpVisAttributes);
  else return os << "No Visualization Attributes";
}
*/