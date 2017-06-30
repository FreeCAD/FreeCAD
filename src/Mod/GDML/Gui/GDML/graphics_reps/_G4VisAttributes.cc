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
// $Id: G4VisAttributes.cc,v 1.16 2007/01/05 14:12:13 allison Exp $
// GEANT4 tag $Name: geant4-09-03-patch-01 $
//
// 
// John Allison  23rd October 1996

#include "G4VisAttributes.hh"

#include "G4AttValue.hh"

G4VisAttributes::G4VisAttributes ():
fVisible             (true),
fDaughtersInvisible  (false),
fColour              (G4Colour ()),
fLineStyle           (unbroken),
fLineWidth           (1.),
fForceDrawingStyle   (false),
fForceAuxEdgeVisible (false),
fForcedLineSegmentsPerCircle (0),  // <=0 means not forced.
fStartTime           (-DBL_MAX),
fEndTime             (DBL_MAX),
fAttValues           (0),
fAttDefs             (0)
{}

G4VisAttributes::G4VisAttributes (G4bool visibility):
fVisible             (visibility),
fDaughtersInvisible  (false),
fColour              (G4Colour ()),
fLineStyle           (unbroken),
fLineWidth           (1.),
fForceDrawingStyle   (false),
fForceAuxEdgeVisible (false),
fForcedLineSegmentsPerCircle (0),  // <=0 means not forced.
fStartTime           (-DBL_MAX),
fEndTime             (DBL_MAX),
fAttValues           (0),
fAttDefs             (0)
{}

G4VisAttributes::G4VisAttributes (const G4Colour& colour):
fVisible             (true),
fDaughtersInvisible  (false),
fColour              (colour),
fLineStyle           (unbroken),
fLineWidth           (1.),
fForceDrawingStyle   (false),
fForceAuxEdgeVisible (false),
fForcedLineSegmentsPerCircle (0),  // <=0 means not forced.
fStartTime           (-DBL_MAX),
fEndTime             (DBL_MAX),
fAttValues           (0),
fAttDefs             (0)
{}

G4VisAttributes::G4VisAttributes (G4bool visibility,
				  const G4Colour& colour):
fVisible            (visibility),
fDaughtersInvisible (false),
fColour             (colour),
fLineStyle          (unbroken),
fLineWidth          (1.),
fForceDrawingStyle  (false),
fForcedLineSegmentsPerCircle (0),  // <=0 means not forced.
fStartTime           (-DBL_MAX),
fEndTime             (DBL_MAX),
fAttValues          (0),
fAttDefs            (0)
{}

const G4VisAttributes  G4VisAttributes::Invisible = G4VisAttributes (false);

const std::vector<G4AttValue>* G4VisAttributes::CreateAttValues () const {
  // Create an expendable copy on the heap...
  return new std::vector<G4AttValue>(*fAttValues);
}

void G4VisAttributes::SetForceLineSegmentsPerCircle (G4int nSegments) {
  const G4int nSegmentsMin = 12;
  if (nSegments > 0 && nSegments < nSegmentsMin) {
    nSegments = nSegmentsMin;
    G4cout <<
      "G4VisAttributes::SetForcedLineSegmentsPerCircle: attempt to set the"
      "\nnumber of line segements per circle < " << nSegmentsMin
         << "; forced to " << nSegments << G4endl;
  }
  fForcedLineSegmentsPerCircle = nSegments;
}

std::ostream& operator << (std::ostream& os, const G4VisAttributes& a) {
  
  os << "G4VisAttributes: ";
  if (&a){
    if (!a.fVisible) os << "in";
    os << "visible, daughters ";
    if (a.fDaughtersInvisible) os << "in";
    os << "visible, colour: " << a.fColour;
    os << "\n  linestyle: ";
    switch (a.fLineStyle) {
    case G4VisAttributes::unbroken:
      os << "solid"; break;
    case G4VisAttributes::dashed:
      os << "dashed"; break;
    case G4VisAttributes::dotted: os << "dotted"; break;
    default: os << "unrecognised"; break;
    }
    os << ", line width: " << a.fLineWidth;
    os << "\n  drawing style: ";
    if (a.fForceDrawingStyle) {
      os << "forced to: ";
      switch (a.fForcedStyle) {
      case G4VisAttributes::wireframe:
	os << "wireframe"; break;
      case G4VisAttributes::solid:
	os << "solid"; break;
      default: os << "unrecognised"; break;
      }
    }
    else {
      os << "not forced";
    }
    os << ", auxiliary edge visibility: ";
    if (!a.fForceAuxEdgeVisible) {
      os << "not ";
    }
    os << "forced";
    os << "\n  line segments per circle: ";
    if (a.fForcedLineSegmentsPerCircle > 0) {
      os << "forced to " << a.fForcedLineSegmentsPerCircle;
    } else {
      os << "not forced.";
    }
    os << "\n  time range: (" << a.fStartTime << ',' << a.fEndTime << ')';
    os << "\n  G4AttValue pointer is ";
    if (a.fAttValues) {
      os << "non-";
    }
    os << "zero";      
    os << ", G4AttDef pointer is ";
    if (a.fAttDefs) {
      os << "non-";
    }
    os << "zero";      
  } 
  else os << " zero G4VisAttributes pointer";
  return os;
}

G4bool G4VisAttributes::operator != (const G4VisAttributes& a) const {

  if (
      (fVisible            != a.fVisible)            ||
      (fDaughtersInvisible != a.fDaughtersInvisible) ||
      (fColour             != a.fColour)             ||
      (fLineStyle          != a.fLineStyle)          ||
      (fLineWidth          != a.fLineWidth)          ||
      (fForceDrawingStyle  != a.fForceDrawingStyle)  ||
      (fForceAuxEdgeVisible!= a.fForceAuxEdgeVisible)||
      (fForcedLineSegmentsPerCircle != a.fForcedLineSegmentsPerCircle) ||
      (fStartTime          != a.fStartTime)          ||
      (fEndTime            != a.fEndTime)            ||
      (fAttValues          != a.fAttValues)          ||
      (fAttDefs            != a.fAttDefs)
      )
    return true;

  if (fForceDrawingStyle) {
    if (fForcedStyle != a.fForcedStyle) return true;
  }

  return false;
}

G4bool G4VisAttributes::operator == (const G4VisAttributes& a) const {
  return !(G4VisAttributes::operator != (a));
}
