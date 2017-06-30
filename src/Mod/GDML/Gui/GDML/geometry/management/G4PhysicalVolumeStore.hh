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
// $Id: G4PhysicalVolumeStore.hh,v 1.16 2008/07/10 09:41:20 gcosmo Exp $
// GEANT4 tag $Name: geant4-09-03-patch-01 $
//
// class G4PhysicalVolume
//
// Class description:
//
// Container for all solids, with functionality derived from
// std::vector<T>. The class is a `singleton', in that only
// one can exist, and access is provided via the static method
// G4PhysicalVolumeStore::GetInstance()
//
// All solids should be registered with G4PhysicalVolumeStore, and removed on
// their destruction. Intended principally for UI browser. The underlying
// container initially has a capacity of 100.
//
// If much additional functionality is added, should consider containment
// instead of inheritance for std::vector<T>
//
// Member data:
//
// static G4PhysicalVolumeStore*
//   - Ptr to the single G4PhysicalVolumeStore.

// History:
// 18.04.01 G.Cosmo Migrated to STL vector
// 25.07.95 P.Kent  Initial version
// --------------------------------------------------------------------
#ifndef G4PHYSICALVOLUMESTORE_HH
#define G4PHYSICALVOLUMESTORE_HH

#include <vector>

#include "G4VPhysicalVolume.hh"
///#include "G4VStoreNotifier.hh"

class G4PhysicalVolumeStore : public std::vector<G4VPhysicalVolume*>
{
  public:  // with description

    static void Register(G4VPhysicalVolume* pSolid);
      // Add the volume to the collection.
    static void DeRegister(G4VPhysicalVolume* pSolid);
      // Remove the volume from the collection.
    static G4PhysicalVolumeStore* GetInstance();
      // Get a ptr to the unique G4PhysicalVolumeStore, creating it if necessary.
///    static void SetNotifier(G4VStoreNotifier* pNotifier);
      // Assign a notifier for allocation/deallocation of the physical volumes.
    static void Clean();
      // Delete all physical volumes from the store. Mother logical volumes
      // are automatically notified and have their daughters de-registered.

    G4VPhysicalVolume* GetVolume(const G4String& name,
                                 G4bool verbose=true) const;
      // Return the pointer of the first volume in the collection having
      // that name.

    virtual ~G4PhysicalVolumeStore();
      // Destructor: takes care to delete allocated physical volumes.

  protected:

    G4PhysicalVolumeStore();

  private:

    static G4PhysicalVolumeStore* fgInstance;
///    static G4VStoreNotifier* fgNotifier;
    static G4bool locked;
};

#endif
