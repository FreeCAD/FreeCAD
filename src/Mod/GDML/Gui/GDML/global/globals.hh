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
// $Id: globals.hh,v 1.27 2006/06/29 19:03:51 gunter Exp $
// GEANT4 tag $Name: geant4-09-03-patch-01 $
//
//
// Global Constants and typedefs
//
// History:
// 30.06.95 P.Kent - Created
// 16.02.96 G.Cosmo - Added inclusion of "templates.hh"
// 03.03.96 M.Maire - Added inclusion of "G4PhysicalConstants.hh"
// 08.11.96 G.Cosmo - Added cbrt() definition and G4ApplicationState enum type
// 29.11.96 G.Cosmo - Added typedef of HepBoolean to G4bool
// 22.10.97 M.Maire - Moved PhysicalConstants at the end of the file
// 04.12.97 G.Cosmo,E.Tcherniaev - Migrated to CLHEP
// 26.08.98 J.Allison,E.Tcherniaev - Introduced min/max/sqr/abs functions
// 22.09.98 G.Cosmo - Removed min/max/sqr/abs functions and replaced with
//                    inclusion of CLHEP/config/TemplateFunctions.h for CLHEP-1.3
// 15.12.99 G.Garcia - Included min, max definitions for NT with ISO standard
// 15.06.01 G.Cosmo - Removed cbrt() definition

#ifndef GLOBALS_HH
#define GLOBALS_HH

#include "G4ios.hh"

#ifndef FALSE
  #define FALSE 0
#endif
#ifndef TRUE
  #define TRUE 1
#endif

#include <algorithm>  // Retrieve definitions of min/max

// Include base types
#include "G4Types.hh"

// Get definition of G4String
#include "G4String.hh"

// Includes some additional definitions: sqr, G4SwapPtr, G4SwapObj.
#include "templates.hh"

// Includes Physical Constants and System of Units
///#include "../CLHEP/units/SystemOfUnits.h"
///#include "../CLHEP/units/PhysicalConstants.h"

// Global error function
#include "G4ExceptionSeverity.hh"
/*
void G4Exception(const char* issure,
                 const char* errorCode,
                             G4ExceptionSeverity severity,
                 const char* comments);
				 */
void G4Exception(const char* s=0);
void G4Exception(std::string s);
void G4Exception(G4String s);

#endif /* GLOBALS_HH */

