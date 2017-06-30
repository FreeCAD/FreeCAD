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
// $Id: G4LogicalVolume.cc,v 1.34 2009/09/24 13:22:57 gcosmo Exp $
// GEANT4 tag $Name: geant4-09-03-patch-01 $
//
// 
// class G4LogicalVolume Implementation
//
// History:
// 01.03.05 G.Santin: Added flag for optional propagation of GetMass()
// 17.05.02 G.Cosmo: Added flag for optional optimisation
// 12.02.99 S.Giani: Default initialization of voxelization quality
// 04.08.97 P.M.DeFreitas: Added methods for parameterised simulation 
// 19.08.96 P.Kent: Modified for G4VSensitive Detector
// 11.07.95 P.Kent: Initial version
// --------------------------------------------------------------------

#include "G4LogicalVolume.hh"
#include "G4LogicalVolumeStore.hh"
#include "G4VSolid.hh"
///
#include "../../materials/G4Material.hh"
///
#include "G4VPVParameterisation.hh"
///#include "G4VisAttributes.hh"

///#include "G4UnitsTable.hh"

// ********************************************************************
// Constructor - sets member data and adds to logical Store,
//               voxel pointer for optimisation set to 0 by default.
//               Initialises daughter vector to 0 length.
// ********************************************************************
//

G4LogicalVolume::G4LogicalVolume( G4VSolid* pSolid,
                                  G4Material* pMaterial,
                            const G4String& name,
                                  G4FieldManager* pFieldMgr,
                                  G4VSensitiveDetector* pSDetector,
                                  G4UserLimits* pULimits,
                                  G4bool optimise )
 : fDaughters(0,(G4VPhysicalVolume*)0), fFieldManager(pFieldMgr),
   fVoxel(0), fOptimise(optimise), fRootRegion(false), fLock(false),
   fSmartless(2.), fMass(0.), fVisAttributes(0),  fCutsCouple(0)
/// : fDaughters(0,(G4VPhysicalVolume*)0), fFieldManager(pFieldMgr),
///   fVoxel(0), fOptimise(optimise), fRootRegion(false), fLock(false),
///   fSmartless(2.), fMass(0.), fVisAttributes(0), fRegion(0), fCutsCouple(0)
{
  SetSolid(pSolid);
  SetMaterial(pMaterial);
  SetName(name);
  ///SetSensitiveDetector(pSDetector);
  ///SetUserLimits(pULimits);    
  //
  // Add to store
  //
  G4LogicalVolumeStore::Register(this);
}

// ********************************************************************
// Fake default constructor - sets only member data and allocates memory
//                            for usage restricted to object persistency.
// ********************************************************************
//
G4LogicalVolume::G4LogicalVolume( __void__& )
/// : fDaughters(0,(G4VPhysicalVolume*)0), fFieldManager(0),
///   fMaterial(0), fName(""), fSensitiveDetector(0), fSolid(0), fUserLimits(0),
///   fVoxel(0), fOptimise(true), fRootRegion(false), fLock(false), fSmartless(2.),
///   fMass(0.), fVisAttributes(0), fRegion(0), fCutsCouple(0), fBiasWeight(0.)
 : fDaughters(0,(G4VPhysicalVolume*)0), fFieldManager(0),
   fMaterial(0), fName(""), fSensitiveDetector(0), fSolid(0), fUserLimits(0),
   fVoxel(0), fOptimise(true), fRootRegion(false), fLock(false), fSmartless(2.),
   fMass(0.), fVisAttributes(0), fCutsCouple(0), fBiasWeight(0.)
{
  // Add to store
  //
  G4LogicalVolumeStore::Register(this);
}

// ********************************************************************
// Destructor - Removes itself from solid Store
// NOTE: Not virtual
// ********************************************************************
//
G4LogicalVolume::~G4LogicalVolume()
{
///  if (!fLock && fRootRegion)  // De-register root region first if not locked
///  {                           // and flagged as root logical-volume
///    fRegion->RemoveRootLogicalVolume(this, true);
///  }
  G4LogicalVolumeStore::DeRegister(this);
}

// ********************************************************************
// SetFieldManager
// ********************************************************************
//
void
G4LogicalVolume::SetFieldManager(G4FieldManager* pNewFieldMgr,
                                 G4bool          forceAllDaughters) 
{
  fFieldManager = pNewFieldMgr;

  G4int NoDaughters = GetNoDaughters();
  while ( (NoDaughters--)>0 )
  {
    G4LogicalVolume* DaughterLogVol; 
    DaughterLogVol = GetDaughter(NoDaughters)->GetLogicalVolume();
    if ( forceAllDaughters || (DaughterLogVol->GetFieldManager() == 0) )
    {
      DaughterLogVol->SetFieldManager(pNewFieldMgr, forceAllDaughters);
    }
  }
}


// ********************************************************************
// IsAncestor
//
// Finds out if the current logical volume is an ancestor of a given 
// physical volume
// ********************************************************************
//
G4bool
G4LogicalVolume::IsAncestor(const G4VPhysicalVolume* aVolume) const
{
  G4bool isDaughter = IsDaughter(aVolume);
  if (!isDaughter)
  {
    for (G4PhysicalVolumeList::const_iterator itDau = fDaughters.begin();
         itDau != fDaughters.end(); itDau++)
    {
      isDaughter = (*itDau)->GetLogicalVolume()->IsAncestor(aVolume);
      if (isDaughter)  break;
    }
  }
  return isDaughter;
}

// ********************************************************************
// TotalVolumeEntities
//
// Returns the total number of physical volumes (replicated or placed)
// in the tree represented by the current logical volume.
// ********************************************************************
//
G4int G4LogicalVolume::TotalVolumeEntities() const
{
  G4int vols = 1;
  for (G4PhysicalVolumeList::const_iterator itDau = fDaughters.begin();
       itDau != fDaughters.end(); itDau++)
  {
    G4VPhysicalVolume* physDaughter = (*itDau);
    vols += physDaughter->GetMultiplicity()
           *physDaughter->GetLogicalVolume()->TotalVolumeEntities();
  }
  return vols;
}
/*
// ********************************************************************
// GetMass
//
// Returns the mass of the logical volume tree computed from the
// estimated geometrical volume of each solid and material associated
// to the logical volume and its daughters.
// NOTE: the computation may require considerable amount of time,
//       depending from the complexity of the geometry tree.
//       The returned value is cached and can be used for successive
//       calls (default), unless recomputation is forced by providing
//       'true' for the boolean argument in input. Computation should
//       be forced if the geometry setup has changed after the previous
//       call. By setting the 'propagate' boolean flag to 'false' the 
//       method returns the mass of the present logical volume only 
//       (subtracted for the volume occupied by the daughter volumes).
//       The extra argument 'parMaterial' is internally used to
//       consider cases of geometrical parameterisations by material.
// ********************************************************************
//
G4double G4LogicalVolume::GetMass(G4bool forced,
                                  G4bool propagate,
                                  G4Material* parMaterial)
{
  // Return the cached non-zero value, if not forced
  //
  if ( (fMass) && (!forced) ) return fMass;

  // Global density and computed mass associated to the logical
  // volume without considering its daughters
  //
  G4Material* logMaterial = parMaterial ? parMaterial : fMaterial;
  if (!logMaterial)
  {
    G4cerr << "ERROR - G4LogicalVolume::GetMass()" << G4endl
           << "        No material is associated to the logical volume: "
           << fName << " !  Sorry, cannot compute the mass ..." << G4endl;
///    G4Exception("G4LogicalVolume::GetMass()", "InvalidSetup", FatalException,
///		"No material associated to the logical volume !");
  }
  if (!fSolid)
  {
    G4cerr << "ERROR - G4LogicalVolume::GetMass()" << G4endl
           << "        No solid is associated to the logical volume: "
           << fName << " !  Sorry, cannot compute the mass ..." << G4endl;
///    G4Exception("G4LogicalVolume::GetMass()", "InvalidSetup", FatalException,
///		"No solid associated to the logical volume !");
  }
  G4double globalDensity = logMaterial->GetDensity();
  fMass = fSolid->GetCubicVolume() * globalDensity;

  // For each daughter in the tree, subtract the mass occupied
  // and if required by the propagate flag, add the real daughter's
  // one computed recursively

  for (G4PhysicalVolumeList::const_iterator itDau = fDaughters.begin();
       itDau != fDaughters.end(); itDau++)
  {
    G4VPhysicalVolume* physDaughter = (*itDau);
    G4LogicalVolume* logDaughter = physDaughter->GetLogicalVolume();
    G4double subMass=0.;
    G4VSolid* daughterSolid = 0;
    G4Material* daughterMaterial = 0;

    // Compute the mass to subtract and to add for each daughter
    // considering its multiplicity (i.e. replicated or not) and
    // eventually its parameterisation (by solid and/or by material)
    //
    for (G4int i=0; i<physDaughter->GetMultiplicity(); i++)
    {
      G4VPVParameterisation*
        physParam = physDaughter->GetParameterisation();
      if (physParam)
      {
        daughterSolid = physParam->ComputeSolid(i, physDaughter);
        daughterSolid->ComputeDimensions(physParam, i, physDaughter);
        daughterMaterial = physParam->ComputeMaterial(i, physDaughter);
      }
      else
      {
        daughterSolid = logDaughter->GetSolid();
        daughterMaterial = logDaughter->GetMaterial();
      }
      subMass = daughterSolid->GetCubicVolume() * globalDensity;

      // Subtract the daughter's portion for the mass and, if required,
      // add the real daughter's mass computed recursively
      //
      fMass -= subMass;
      if (propagate)
      {
        fMass += logDaughter->GetMass(true, true, daughterMaterial);
      }
    }
  }

  return fMass;
}
*/
/*
void G4LogicalVolume::SetVisAttributes (const G4VisAttributes& VA)
{
  fVisAttributes = new G4VisAttributes(VA);
}
*/
