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
// $Id: G4EnclosingCylinder.cc,v 1.10 2007/05/11 13:54:29 gcosmo Exp $
// GEANT4 tag $Name: geant4-09-03-patch-01 $
//
// 
// --------------------------------------------------------------------
// GEANT 4 class source file
//
//
// G4EnclosingCylinder.cc
//
// Implementation of a utility class for a quick check of geometry.
//
// --------------------------------------------------------------------

#include "G4EnclosingCylinder.hh"
#include "G4ReduciblePolygon.hh"
///#include "G4GeometryTolerance.hh"
#include "../../../global/G4GeometryTolerance.hh"

//for pi
#include "../../../CLHEP/units/PhysicalConstants.h"
using namespace CLHEP;

//
// Constructor
//
G4EnclosingCylinder::G4EnclosingCylinder( const G4ReduciblePolygon *rz,
                                                G4bool thePhiIsOpen, 
                                                G4double theStartPhi,
                                                G4double theTotalPhi )
  : startPhi(theStartPhi), totalPhi(theTotalPhi),
    rx1(0.), ry1(0.), dx1(0.), dy1(0.),
    rx2(0.), ry2(0.), dx2(0.), dy2(0.),     
    concave(theTotalPhi > pi)
{
  //
  // Obtain largest r and smallest and largest z
  //
  radius = rz->Amax();
  zHi = rz->Bmax();
  zLo = rz->Bmin();

  G4double kCarTolerance = G4GeometryTolerance::GetInstance()
                           ->GetSurfaceTolerance();
  //
  // Save phi info
  //
  phiIsOpen = thePhiIsOpen;
  if ( phiIsOpen )
  {    
    rx1 = std::cos(startPhi);
    ry1 = std::sin(startPhi);
    dx1 = +ry1*10*kCarTolerance;
    dy1 = -rx1*10*kCarTolerance;
    
    rx2 = std::cos(startPhi+totalPhi);
    ry2 = std::sin(startPhi+totalPhi);
    dx2 = -ry2*10*kCarTolerance;
    dy2 = +rx2*10*kCarTolerance;
  }
  
  //
  // Add safety
  //
  radius += 10*kCarTolerance;
  zLo    -= 10*kCarTolerance;
  zHi    += 10*kCarTolerance;
}

//
// Fake default constructor - sets only member data and allocates memory
//                            for usage restricted to object persistency.
//
G4EnclosingCylinder::G4EnclosingCylinder( __void__& )
{
}

//
// Destructor
//
G4EnclosingCylinder::~G4EnclosingCylinder()
{
}


//
// Outside
//
// Decide very rapidly if the point is outside the cylinder
//
// If one is not certain, return false
//
G4bool G4EnclosingCylinder::MustBeOutside( const G4ThreeVector &p ) const
{
  if (p.perp() > radius) return true;
  if (p.z() < zLo) return true;
  if (p.z() > zHi) return true;

  if (phiIsOpen)
  {
    if (concave)
    {
      if ( ((p.x()-dx1)*ry1 - (p.y()-dy1)*rx1) < 0) return false;
      if ( ((p.x()-dx2)*ry2 - (p.y()-dy2)*rx2) > 0) return false;
    }
    else
    {
      if ( ((p.x()-dx1)*ry1 - (p.y()-dy1)*rx1) > 0) return true;
      if ( ((p.x()-dx2)*ry2 - (p.y()-dy2)*rx2) < 0) return true;
    }
  }
  
  return false;
}
    
    
//
// Misses
//
// Decide very rapidly if the trajectory is going to miss the cylinder
//
// If one is not sure, return false
//
G4bool G4EnclosingCylinder::ShouldMiss( const G4ThreeVector &p,
                                        const G4ThreeVector &v ) const
{
  if (!MustBeOutside(p)) return false;
  
  G4double cross = p.x()*v.y() - p.y()*v.x();
  if (cross > radius) return true;
  
  if (p.perp() > radius)
  {
    G4double dot = p.x()*v.x() + p.y()*v.y();
    if (dot > 0) return true;
  }

  return false;
}
