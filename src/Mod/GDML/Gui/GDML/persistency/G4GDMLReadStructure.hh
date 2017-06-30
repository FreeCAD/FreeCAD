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
// $Id: G4GDMLReadStructure.hh,v 1.28 2009/09/24 15:04:34 gcosmo Exp $
// GEANT4 tag $Name: geant4-09-03-patch-01 $
//
//
// class G4GDMLReadStructure
//
// Class description:
//
// GDML class for import of structures.

// History:
// - Created.                                  Zoltan Torzsok, November 2007
// -------------------------------------------------------------------------

#ifndef _G4GDMLREADSTRUCTURE_INCLUDED_
#define _G4GDMLREADSTRUCTURE_INCLUDED_

///#include "G4Types.hh"
#include "../global/G4Types.hh"
///#include "geomdefs.hh"
#include "../global/geomdefs.hh"

#include "G4GDMLReadParamvol.hh"

class G4AssemblyVolume;
class G4LogicalVolume;
class G4VPhysicalVolume;

struct G4GDMLAuxPairType
{
   G4String type;
   G4double value;
};

typedef std::vector<G4GDMLAuxPairType> G4GDMLAuxListType;
typedef std::map<const G4LogicalVolume*,G4GDMLAuxListType> G4GDMLAuxMapType;
typedef std::map<G4String, G4AssemblyVolume*> G4GDMLAssemblyMapType;

class G4GDMLReadStructure : public G4GDMLReadParamvol
{

 public:

   G4GDMLReadStructure();
   virtual ~G4GDMLReadStructure();

   G4VPhysicalVolume* GetPhysvol(const G4String&) const;
   ///
   G4LogicalVolume* GetVolume(const G4String&) const;
   G4AssemblyVolume* GetAssembly(const G4String&) const;
   G4GDMLAuxListType GetVolumeAuxiliaryInformation(const G4LogicalVolume* const);
   G4VPhysicalVolume* GetWorldVolume(const G4String&);
   const G4GDMLAuxMapType* GetAuxMap() const;

   virtual void VolumeRead(const xercesc::DOMElement* const);
   virtual void Volume_contentRead(const xercesc::DOMElement* const);
   virtual void StructureRead(const xercesc::DOMElement* const);

 protected:

   G4GDMLAuxPairType AuxiliaryRead(const xercesc::DOMElement* const);
   void AssemblyRead(const xercesc::DOMElement* const);
   void DivisionvolRead(const xercesc::DOMElement* const);
   ///G4LogicalVolume* FileRead(const xercesc::DOMElement* const);
   void PhysvolRead(const xercesc::DOMElement* const,
                    G4AssemblyVolume* assembly=0);
   void ReplicavolRead(const xercesc::DOMElement* const, G4int number);
   void ReplicaRead(const xercesc::DOMElement* const replicaElement,
                    G4LogicalVolume* logvol,G4int number);
   ///EAxis AxisRead(const xercesc::DOMElement* const axisElement);
   G4double QuantityRead(const xercesc::DOMElement* const readElement);
   void BorderSurfaceRead(const xercesc::DOMElement* const);
   void SkinSurfaceRead(const xercesc::DOMElement* const);

 protected:

   G4GDMLAuxMapType auxMap;
   G4GDMLAssemblyMapType assemblyMap;
   G4LogicalVolume *pMotherLogical;

};

#endif
