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
// $Id: meshdefs.hh,v 1.7 2006/06/29 18:33:17 gunter Exp $
// GEANT4 tag $Name: geant4-09-03-patch-01 $
//
//
// Tube/Cone Meshing constants for extent calculations

// History:
// 13.08.95 P.Kent Created separate file
// --------------------------------------------------------------------
#ifndef MESHDEFS_HH
#define MESHDEFS_HH

///#include "G4Types.hh"
///#include <CLHEP/Units/PhysicalConstants.h>
#include "../../global/G4Types.hh"
#include "../../CLHEP/units/PhysicalConstants.h"
using namespace CLHEP;

const G4double kMeshAngleDefault=(pi/4); // Angle for mesh `wedges' in rads
                                 // Works best when simple fraction of pi/2

const G4int kMinMeshSections=3;	 // Min wedges+1 to make
const G4int kMaxMeshSections=37; // max wedges+1 to make
                                 // =>10 degrees/wedge for complete tube

#endif
