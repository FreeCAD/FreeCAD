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
// $Id: G4GeometryTolerance.cc,v 1.3 2008/02/15 07:29:51 gcosmo Exp $
// GEANT4 tag $Name: geant4-09-03-patch-01 $
//
// class G4GeometryTolerance
//
// Implementation
//
// Author:
// 30.10.06 - G.Cosmo, first implementation
// --------------------------------------------------------------------

#include "G4GeometryTolerance.hh"
#include "globals.hh"

//BEGIN CAD-GDML
#include "../CLHEP/units/SystemOfUnits.h"
using namespace CLHEP;
//END CAD-GDML

// ***************************************************************************
// Static class data
// ***************************************************************************
//
G4GeometryTolerance* G4GeometryTolerance::fInstance = 0;
G4bool G4GeometryTolerance::fInitialised = false;
G4double G4GeometryTolerance::fCarTolerance = 1E-9*mm;
G4double G4GeometryTolerance::fAngTolerance = 1E-9*rad;
G4double G4GeometryTolerance::fRadTolerance = 1E-9*mm;

// ***************************************************************************
// Constructor.
// ***************************************************************************
//
G4GeometryTolerance::G4GeometryTolerance()
{
}

// ***************************************************************************
// Empty destructor.
// ***************************************************************************
//
G4GeometryTolerance::~G4GeometryTolerance()
{
}

// ***************************************************************************
// Returns the instance of the singleton.
// Creates it in case it's called for the first time.
// ***************************************************************************
//
G4GeometryTolerance* G4GeometryTolerance::GetInstance()
{
  static G4GeometryTolerance theToleranceManager;
  if (!fInstance)
  {
    fInstance = &theToleranceManager;
  }
  return fInstance;    
}

// ***************************************************************************
// Accessors.
// ***************************************************************************
//
G4double G4GeometryTolerance::GetSurfaceTolerance() const
{
  return fCarTolerance;
}

G4double G4GeometryTolerance::GetAngularTolerance() const
{
  return fAngTolerance;
}

G4double G4GeometryTolerance::GetRadialTolerance() const
{
  return fRadTolerance;
}

// ***************************************************************************
// Sets the tolerance to a value computed on the basis of the world volume
// extent provided as argument. The method can be called only once.
// ***************************************************************************
//
void G4GeometryTolerance::SetSurfaceTolerance(G4double worldExtent)
{
  if (!fInitialised)
  {
    fCarTolerance = worldExtent*1E-11;
    fInitialised = true;
  }
  else
  {
/*    G4cout << "WARNING - G4GeometryTolerance::SetSurfaceTolerance()" << G4endl
           << "          Tolerance can only be set once. Currently set to: "
           << fCarTolerance/mm << " mm." << G4endl;
    G4Exception("G4GeometryTolerance::SetSurfaceTolerance()",
                "NotApplicable", JustWarning,
                "The tolerance has been already set!");
*/  }
}
