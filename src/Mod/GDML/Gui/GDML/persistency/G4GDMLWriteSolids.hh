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
// $Id: G4GDMLWriteSolids.hh,v 1.38 2009/04/24 15:34:20 gcosmo Exp $
// GEANT4 tag $Name: geant4-09-03-patch-01 $
//
//
// class G4GDMLWriteSolids
//
// Class description:
//
// GDML class for writing solids.

// History:
// - Created.                                  Zoltan Torzsok, November 2007
// -------------------------------------------------------------------------

#ifndef _G4GDMLWRITESOLIDS_INCLUDED_
#define _G4GDMLWRITESOLIDS_INCLUDED_

//#include "G4Types.hh"
#include "../global/G4Types.hh"

#include "G4GDMLWriteMaterials.hh"

class G4BooleanSolid;
class G4Box;
class G4Cons;
class G4EllipticalCone;
class G4Ellipsoid;
class G4EllipticalTube;
class G4ExtrudedSolid;
class G4Hype;
class G4Orb;
class G4Para;
class G4Paraboloid;
class G4Polycone;
class G4Polyhedra;
class G4Sphere;
class G4TessellatedSolid;
class G4Tet;
class G4Torus;
class G4Trap;
class G4Trd;
class G4Tubs;
class G4TwistedBox;
class G4TwistedTrap;
class G4TwistedTrd;
class G4TwistedTubs;
class G4VSolid;
class G4OpticalSurface;

class G4GDMLWriteSolids : public G4GDMLWriteMaterials
{
  public:

   virtual void AddSolid(const G4VSolid* const);
   virtual void SolidsWrite(xercesc::DOMElement*);

  protected:

   G4GDMLWriteSolids();
   virtual ~G4GDMLWriteSolids();

   void BooleanWrite(xercesc::DOMElement*, const G4BooleanSolid* const);
   void BoxWrite(xercesc::DOMElement*, const G4Box* const);
   void ConeWrite(xercesc::DOMElement*, const G4Cons* const);
   void ElconeWrite(xercesc::DOMElement*, const G4EllipticalCone* const);
   void EllipsoidWrite(xercesc::DOMElement*, const G4Ellipsoid* const);
   void EltubeWrite(xercesc::DOMElement*, const G4EllipticalTube* const);
   void XtruWrite(xercesc::DOMElement*, const G4ExtrudedSolid* const);
   void HypeWrite(xercesc::DOMElement*, const G4Hype* const);
   void OrbWrite(xercesc::DOMElement*, const G4Orb* const);
   void ParaWrite(xercesc::DOMElement*, const G4Para* const);
   void ParaboloidWrite(xercesc::DOMElement*, const G4Paraboloid* const);
   void PolyconeWrite(xercesc::DOMElement*, const G4Polycone* const);
   void PolyhedraWrite(xercesc::DOMElement*, const G4Polyhedra* const);
   void SphereWrite(xercesc::DOMElement*, const G4Sphere* const);
   void TessellatedWrite(xercesc::DOMElement*, const G4TessellatedSolid* const);
   void TetWrite(xercesc::DOMElement*, const G4Tet* const);
   void TorusWrite(xercesc::DOMElement*, const G4Torus* const);
   void TrapWrite(xercesc::DOMElement*, const G4Trap* const);
   void TrdWrite(xercesc::DOMElement*, const G4Trd* const);
   void TubeWrite(xercesc::DOMElement*, const G4Tubs* const);
   void TwistedboxWrite(xercesc::DOMElement*, const G4TwistedBox* const);
   void TwistedtrapWrite(xercesc::DOMElement*, const G4TwistedTrap* const);
   void TwistedtrdWrite(xercesc::DOMElement*, const G4TwistedTrd* const);
   void TwistedtubsWrite(xercesc::DOMElement*, const G4TwistedTubs* const);
   void ZplaneWrite(xercesc::DOMElement*, const G4double&,
                    const G4double&, const G4double&);
   void OpticalSurfaceWrite(xercesc::DOMElement*,
                    const G4OpticalSurface* const);

  protected:

   std::vector<const G4VSolid*> solidList;
   xercesc::DOMElement* solidsElement;
   static const G4int maxTransforms = 8; // Constant for limiting the number
                                         // of displacements/reflections
                                         // applied to a single solid
};

#endif
