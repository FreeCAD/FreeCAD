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
// $Id: G4GDMLWriteStructure.hh,v 1.39 2009/04/24 15:34:20 gcosmo Exp $
// GEANT4 tag $Name: geant4-09-03-patch-01 $
//
//
// class G4GDMLWriteStructure
//
// Class description:
//
// GDML class for export of structures.

// History:
// - Created.                                  Zoltan Torzsok, November 2007
// -------------------------------------------------------------------------

#ifndef _G4GDMLWRITESTRUCTURE_INCLUDED_
#define _G4GDMLWRITESTRUCTURE_INCLUDED_

//#include "G4Types.hh"
#include "../global/G4Types.hh"
//#include "G4Transform3D.hh"
#include "../global/G4Transform3D.hh"

#include "G4GDMLWriteParamvol.hh"

class G4LogicalVolume;
class G4VPhysicalVolume;
class G4PVDivision;
class G4LogicalBorderSurface;
class G4LogicalSkinSurface;
class G4OpticalSurface;
class G4SurfaceProperty;

class G4GDMLWriteStructure : public G4GDMLWriteParamvol
{

 public:

   G4GDMLWriteStructure();
   virtual ~G4GDMLWriteStructure();

   virtual void StructureWrite(xercesc::DOMElement*);

 protected:

   void DivisionvolWrite(xercesc::DOMElement*, const G4PVDivision* const);
   void PhysvolWrite(xercesc::DOMElement*,const G4VPhysicalVolume* const topVol,
                   const G4Transform3D& transform, const G4String& moduleName);
   void ReplicavolWrite(xercesc::DOMElement*, const G4VPhysicalVolume* const);
   G4Transform3D TraverseVolumeTree(const G4LogicalVolume* const topVol,
                                    const G4int depth);
   void SurfacesWrite();
   void BorderSurfaceCache(const G4LogicalBorderSurface* const);
   void SkinSurfaceCache(const G4LogicalSkinSurface* const);
   //const G4LogicalBorderSurface* GetBorderSurface(const G4VPhysicalVolume* const);
   //const G4LogicalSkinSurface* GetSkinSurface(const G4LogicalVolume* const);
   G4bool FindOpticalSurface(const G4SurfaceProperty*);

 protected:

   xercesc::DOMElement* structureElement;
   std::vector<xercesc::DOMElement*> borderElementVec;
   std::vector<xercesc::DOMElement*> skinElementVec;

 private:  // cache for optical surfaces...

   std::vector<const G4OpticalSurface*> opt_vec;
};

#endif
