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
// $Id: G4PhysicalVolumeStore.cc,v 1.20 2008/07/10 09:41:20 gcosmo Exp $
// GEANT4 tag $Name: geant4-09-03-patch-01 $
//
// G4PhysicalVolumeStore
//
// Implementation for singleton container
//
// History:
// 25.07.95 P.Kent Initial version
// --------------------------------------------------------------------

///#include "G4Types.hh"
///#include "G4PhysicalVolumeStore.hh"
///#include "G4GeometryManager.hh"
///#include "G4LogicalVolume.hh"
#include "../../global/G4Types.hh"
#include "G4PhysicalVolumeStore.hh"
///#include "G4GeometryManager.hh"
#include "G4LogicalVolume.hh"

// ***************************************************************************
// Static class variables
// ***************************************************************************
//
G4PhysicalVolumeStore* G4PhysicalVolumeStore::fgInstance = 0;
///G4VStoreNotifier* G4PhysicalVolumeStore::fgNotifier = 0;
G4bool G4PhysicalVolumeStore::locked = false;

// ***************************************************************************
// Protected constructor: Construct underlying container with
// initial size of 100 entries
// ***************************************************************************
//
G4PhysicalVolumeStore::G4PhysicalVolumeStore()
  : std::vector<G4VPhysicalVolume*>()
{
  reserve(100);
}

// ***************************************************************************
// Destructor
// ***************************************************************************
//
G4PhysicalVolumeStore::~G4PhysicalVolumeStore()
{
  Clean();
}

// ***************************************************************************
// Delete all elements from the store
// ***************************************************************************
//
void G4PhysicalVolumeStore::Clean()
{
  // Do nothing if geometry is closed
  //
/*  if (G4GeometryManager::GetInstance()->IsGeometryClosed())
  {
    G4cout << "WARNING - Attempt to delete the physical volume store"
           << " while geometry closed !" << G4endl;
    return;
  }
*/
  // Locks store for deletion of volumes. De-registration will be
  // performed at this stage. G4VPhysicalVolumes will not de-register
  // themselves.
  //
  locked = true;

  size_t i=0;
  G4PhysicalVolumeStore* store = GetInstance();
  std::vector<G4VPhysicalVolume*>::iterator pos;

#ifdef G4GEOMETRY_VOXELDEBUG
  G4cout << "Deleting Physical Volumes ... ";
#endif

  for(pos=store->begin(); pos!=store->end(); pos++)
  {
///    if (fgNotifier) { fgNotifier->NotifyDeRegistration(); }
    if (*pos) { delete *pos; }
    i++;
  }

#ifdef G4GEOMETRY_VOXELDEBUG
  if (store->size() < i-1)
    { G4cout << "No volumes deleted. Already deleted by user ?" << G4endl; }
  else
    { G4cout << i-1 << " volumes deleted !" << G4endl; }
#endif

  locked = false;
  store->clear();
}
/*
// ***************************************************************************
// Associate user notifier to the store
// ***************************************************************************
//
void G4PhysicalVolumeStore::SetNotifier(G4VStoreNotifier* pNotifier)
{
  GetInstance();
  fgNotifier = pNotifier;
}
*/
// ***************************************************************************
// Add Volume to container
// ***************************************************************************
//
void G4PhysicalVolumeStore::Register(G4VPhysicalVolume* pVolume)
{
  GetInstance()->push_back(pVolume);
///  if (fgNotifier) { fgNotifier->NotifyRegistration(); }
}

// ***************************************************************************
// Remove Volume from container and update the list of daughters
// of the mother's logical volume
// ***************************************************************************
//
void G4PhysicalVolumeStore::DeRegister(G4VPhysicalVolume* pVolume)
{
  if (!locked)    // Do not de-register if locked !
  {
///    if (fgNotifier) { fgNotifier->NotifyDeRegistration(); }
    G4LogicalVolume* motherLogical = pVolume->GetMotherLogical();
    if (motherLogical) { motherLogical->RemoveDaughter(pVolume); }
    for (iterator i=GetInstance()->begin(); i!=GetInstance()->end(); i++)
    {
      if (**i==*pVolume)
      {
        GetInstance()->erase(i);
        break;
      }
    }
  }
}

// ***************************************************************************
// Retrieve the first volume pointer in the container having that name
// ***************************************************************************
//
G4VPhysicalVolume*
G4PhysicalVolumeStore::GetVolume(const G4String& name, G4bool verbose) const
{
  for (iterator i=GetInstance()->begin(); i!=GetInstance()->end(); i++)
  {
    if ((*i)->GetName() == name) { return *i; }
  }
  if (verbose)
  {
///     G4cerr << "ERROR - G4PhysicalVolumeStore::GetVolume()" << G4endl
///            << "        Volume " << name << " NOT found in store !" << G4endl
///            << "        Returning NULL pointer." << G4endl;
///     G4Exception("G4PhysicalVolumeStore::GetVolume()", "InvalidQuery",
///                 JustWarning, "Volume NOT found in store !");
  }
  return 0;
}

// ***************************************************************************
// Return ptr to Store, setting if necessary
// ***************************************************************************
//
G4PhysicalVolumeStore* G4PhysicalVolumeStore::GetInstance()
{
  static G4PhysicalVolumeStore worldStore;
  if (!fgInstance)
  {
    fgInstance = &worldStore;
  }
  return fgInstance;
}
