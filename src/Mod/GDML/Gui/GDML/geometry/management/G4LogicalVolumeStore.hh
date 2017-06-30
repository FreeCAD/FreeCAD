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
// $Id: G4LogicalVolumeStore.hh,v 1.13 2007/04/10 10:13:50 gcosmo Exp $
// GEANT4 tag $Name: geant4-09-03-patch-01 $
//
// class G4LogicalVolumeStore
//
// Class description:
//
// Container for all LogicalVolumes, with functionality derived from
// std::vector<T>. The class is a `singleton', in that only
// one can exist, and access is provided via the static function
// G4LogicalVolumeStore::GetInstance()
//
// All LogicalVolumes should be registered with G4LogicalVolumeStore,
// and removed on their destruction. Intended principally for UI browser.
// The underlying container initially has a capacity of 100.
//
// If much additional functionality is added, should consider containment
// instead of inheritance for std::vector<T>.
//
// Member data:
//
// static G4LogicalVolumeStore* fgInstance
//   - Ptr to the single G4LogicalVolumeStore.

// History:
// 18.04.01 G.Cosmo Migrated to STL vector
// 10.07.95 P.Kent  Initial version
// --------------------------------------------------------------------
#ifndef G4LOGICALVOLUMESTORE_HH
#define G4LOGICALVOLUMESTORE_HH

#include <vector>

#include "G4LogicalVolume.hh"
///#include "G4VStoreNotifier.hh"

class G4LogicalVolumeStore : public std::vector<G4LogicalVolume*>
{
  public:  // with description

    static void Register(G4LogicalVolume* pVolume);
      // Add the logical volume to the collection.
    static void DeRegister(G4LogicalVolume* pVolume);
      // Remove the logical volume from the collection.
    static G4LogicalVolumeStore* GetInstance();
      // Get a ptr to the unique G4LogicalVolumeStore, creating it if necessary.
///    static void SetNotifier(G4VStoreNotifier* pNotifier);
      // Assign a notifier for allocation/deallocation of the logical volumes.
    static void Clean();
      // Delete all volumes from the store.

    G4LogicalVolume* GetVolume(const G4String& name, G4bool verbose=true) const;
      // Return the pointer of the first volume in the collection having
      // that name.

    virtual ~G4LogicalVolumeStore();
      // Destructor: takes care to delete allocated logical volumes.

  protected:

    G4LogicalVolumeStore();

  private:

    static G4LogicalVolumeStore* fgInstance;
///    static G4VStoreNotifier* fgNotifier;
    static G4bool locked;
};

#endif
