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
// $Id: G4LogicalVolume.hh,v 1.27 2008/07/10 09:40:08 gcosmo Exp $
// GEANT4 tag $Name: geant4-09-03-patch-01 $
//
// 
// class G4LogicalVolume
//
// Class description:
//
// Represents a leaf node or unpositioned subtree in the geometry hierarchy.
// Logical volumes are named, and may have daughters ascribed to them.
// They are responsible for retrieval of the physical and tracking attributes
// of the physical volume that it represents: solid, material, magnetic field,
// and optionally, user limits, sensitive detectors, regions, biasing weights.
//
// Get and Set functionality is provided for all attributes, but note that
// most set functions should not be used  when the geometry is `closed'.
// As a  further development, `Guard' checks can be added to ensure
// only legal operations at tracking time.
//
// On construction, solid, material and name must be specified.
//
// Daughters are ascribed and managed by means of a simple
// GetNoDaughters,Get/SetDaughter(n),AddDaughter interface.
//
// Smart voxels as used for tracking optimisation. They're also an attribute.
//
// Logical volumes self register to the logical volume Store on construction,
// and deregister on destruction.
//
// NOTE: This class is currently *NOT* subclassed, since not meant to
//       act as a base class. Therefore, the destructor is NOT virtual.
//
// Data members:
//
//    std::vector<G4VPhysicalVolume*> fDaughters
//    - Vector of daughters. Given initial size of 0.
//    G4FieldManager* fFieldManager
//    - Pointer (possibly 0) to (magnetic or other) field manager object.
//    G4Material* fMaterial
//    - Pointer to material at this node.
//    G4String fName
//    - Name of logical volume.
//    G4VSensitiveDetector *fSensitiveDetector
//    - Pointer (possibly 0) to `Hit' object.
//    G4VSolid* fSolid
//    - Pointer to solid.
//    G4UserLimits* fUserLimits
//    - Pointer (possibly 0) to user Step limit object for this node.
//    G4SmartVoxelHeader* fVoxel
//    - Pointer (possibly 0) to optimisation info objects.
//    G4bool fOptimise
//    - Flag to identify if optimisation should be applied or not.
//    G4bool fRootRegion
//    - Flag to identify if the logical volume is a root region.
//    G4double fSmartless
//    - Quality for optimisation, average number of voxels to be spent
//      per content.
//    const G4VisAttributes* fVisAttributes
//    - Pointer (possibly 0) to visualization attributes.
//    G4Region* fRegion
//    - Pointer to the cuts region (if any)
//    G4MaterialCutsCouple* fCutsCouple
//    - Pointer (possibly 0) to associated production cuts.
//    G4double fBiasWeight
//    - Weight used in the event biasing technique.
//
// Following data members has been moved to G4Region - M.Asai (Aug/18/2005)
//    G4FastSimulationManager* fFastSimulationManager
//    - Pointer (possibly 0) to G4FastSimulationManager object.
//    G4bool fIsEnvelope
//    - Flags if the Logical Volume is an envelope for a FastSimulationManager.

// History:
// 12.11.04 G.Cosmo: Added GetMass() method for computing mass of the tree
// 24.09.02 G.Cosmo: Added flags and accessors for region cuts handling
// 17.05.02 G.Cosmo: Added IsToOptimise() method and related flag
// 18.04.01 G.Cosmo: Migrated to STL vector
// 12.02.99 S.Giani: Added user defined optimisation quality
// 09.11.98 J.Apostolakis:  Changed G4MagneticField to G4FieldManager
// 09.11.98 M.Verderi, J.Apostolakis: Added BiasWeight member and accessors
// 10.20.97 P.M.DeFreitas: Added pointer to a FastSimulation
//          J.Apostolakis: & flag to indicate if it is an Envelope for it
// 19.11.96 J.Allison: Replaced G4Visible with explicit const G4VisAttributes*
// 19.08.96 P.Kent: Split -> hh/icc/cc files; G4VSensitiveDetector change
// 11.07.95 P.Kent: Initial version.
// ------------------------------------------------------------------------
#ifndef G4LOGICALVOLUME_HH
#define G4LOGICALVOLUME_HH

//#include "G4Types.hh"
#include "../../global/G4Types.hh"
///#include "G4Region.hh"           // Required by inline methods
#include "G4VPhysicalVolume.hh"  // Need operator == for vector fdaughters
#include <vector>
#include <assert.h>

// Forward declarations
//
class G4FieldManager;
class G4Material;
class G4VSensitiveDetector;
class G4VSolid;
class G4UserLimits;
class G4SmartVoxelHeader;
class G4VisAttributes;
class G4FastSimulationManager;
class G4MaterialCutsCouple;

class G4LogicalVolume
{
  typedef std::vector<G4VPhysicalVolume*> G4PhysicalVolumeList;

  public:  // with description
  
    G4LogicalVolume(G4VSolid* pSolid,
                    G4Material* pMaterial,
              const G4String& name,
                    G4FieldManager* pFieldMgr=0,
                    G4VSensitiveDetector* pSDetector=0,
                    G4UserLimits* pULimits=0,
                    G4bool optimise=true);
      // Constructor. The solid and material pointer must be non null.
      // The parameters for field, detector and user limits are optional.
      // The volume also enters itself into the logical volume Store.
      // Optimisation of the geometry (voxelisation) for the volume
      // hierarchy is applied by default. For parameterised volumes in
      // the hierarchy, optimisation is -always- applied.

    ~G4LogicalVolume();
      // Destructor. Removes the logical volume from the logical volume Store.
      // NOT virtual, since not meant to act as base class.

    inline G4String GetName() const;
    inline void SetName(const G4String& pName);
      // Returns and sets the name of the logical volume.

    inline G4int GetNoDaughters() const;
      // Returns the number of daughters (0 to n).
    inline G4VPhysicalVolume* GetDaughter(const G4int i) const;
      // Returns the ith daughter. Note numbering starts from 0,
      // and no bounds checking is performed.
    inline void AddDaughter(G4VPhysicalVolume* p);
      // Adds the volume p as a daughter of the current logical volume.
    inline G4bool IsDaughter(const G4VPhysicalVolume* p) const;
      // Returns true if the volume p is a daughter of the current
      // logical volume.
    G4bool IsAncestor(const G4VPhysicalVolume* p) const;
      // Returns true if the volume p is part of the hierarchy of
      // volumes established by the current logical volume. Scans
      // recursively the volume tree.
    inline void RemoveDaughter(const G4VPhysicalVolume* p);
      // Removes the volume p from the List of daughter of the current
      // logical volume.
    inline void ClearDaughters();
      // Clears the list of daughters. Used by the phys-volume store when
      // the geometry tree is cleared, since modified at run-time.
    G4int TotalVolumeEntities() const;
      // Returns the total number of physical volumes (replicated or placed)
      // in the tree represented by the current logical volume.


    inline G4VSolid* GetSolid() const;
    inline void SetSolid(G4VSolid *pSolid);
      // Gets and sets the current solid.

    inline G4Material* GetMaterial() const;
    inline void SetMaterial(G4Material *pMaterial);
      // Gets and sets the current material.
    inline void UpdateMaterial(G4Material *pMaterial);
      // Sets material and corresponding MaterialCutsCouple.
      // This method is invoked by G4Navigator while it is navigating through 
      // material parameterization.
///    G4double GetMass(G4bool forced=false, G4bool propagate=true,
///                     G4Material* parMaterial=0);
      // Returns the mass of the logical volume tree computed from the
      // estimated geometrical volume of each solid and material associated
      // to the logical volume and (by default) to its daughters.
      // NOTE: the computation may require a considerable amount of time,
      //       depending from the complexity of the geometry tree.
      //       The returned value is cached and can be used for successive
      //       calls (default), unless recomputation is forced by providing
      //       'true' for the boolean argument in input. Computation should
      //       be forced if the geometry setup has changed after the previous
      //       call. By setting the 'propagate' boolean flag to 'false' the 
      //       method returns the mass of the present logical volume only 
      //       (subtracted for the volume occupied by the daughter volumes).
      //       An optional argument to specify a material is also provided.

    inline G4FieldManager* GetFieldManager() const;
      // Gets current FieldManager.
    void SetFieldManager(G4FieldManager *pFieldMgr, G4bool forceToAllDaughters); 
      // Sets FieldManager and propagates it:
      //  i) only to daughters with G4FieldManager = 0
      //     if forceToAllDaughters=false
      // ii) to all daughters
      //     if forceToAllDaughters=true

    inline G4VSensitiveDetector* GetSensitiveDetector() const;
      // Gets current SensitiveDetector.
    inline void SetSensitiveDetector(G4VSensitiveDetector *pSDetector);
      // Sets SensitiveDetector (can be 0).

    inline G4UserLimits* GetUserLimits() const;
    inline void SetUserLimits(G4UserLimits *pULimits);
      // Gets and sets current UserLimits.

    inline G4SmartVoxelHeader* GetVoxelHeader() const;
    inline void SetVoxelHeader(G4SmartVoxelHeader *pVoxel);
      // Gets and sets current VoxelHeader.
    
    inline G4double GetSmartless() const;
    inline void SetSmartless(G4double s);
      // Gets and sets user defined optimisation quality.

    inline G4bool IsToOptimise() const;
      // Replies if geometry optimisation (voxelisation) is to be
      // applied for this volume hierarchy.
    inline void SetOptimisation(G4bool optim);
      // Specifies if to apply or not geometry optimisation to this
      // volume hierarchy. Note that for parameterised volumes in the
      // hierarchy, optimisation is always applied. 
/*
    inline G4bool IsRootRegion() const;
      // Replies if the logical volume represents a root region or not.
    inline void SetRegionRootFlag(G4bool rreg);
      // Sets/unsets the volume as a root region for cuts.
    inline G4bool IsRegion() const;
      // Replies if the logical volume is part of a cuts region or not.
    inline void SetRegion(G4Region* reg);
      // Sets/unsets the volume as cuts region.
    inline G4Region* GetRegion() const;
      // Return the region to which the volume belongs, if any.
    inline void PropagateRegion();
      // Propagates region pointer to daughters.
*/
    inline const G4MaterialCutsCouple* GetMaterialCutsCouple() const;
    inline void SetMaterialCutsCouple(G4MaterialCutsCouple* cuts);
      // Accessors for production cuts.

    G4bool operator == (const G4LogicalVolume& lv) const;
      // Equality defined by address only.
      // Returns true if objects are at same address, else false.

    inline const G4VisAttributes* GetVisAttributes () const;
    inline void  SetVisAttributes (const G4VisAttributes* pVA);
///    void  SetVisAttributes (const G4VisAttributes& VA);
      // Gets and sets visualization attributes. A copy of 'VA' on the heap
      // will be made in the case the call with a const reference is used.

    inline G4FastSimulationManager* GetFastSimulationManager () const;
      // Gets current FastSimulationManager pointer if exists, otherwise null.

    inline void SetBiasWeight (G4double w);
    inline G4double GetBiasWeight() const;
      // Sets and gets bias weight.

  public:  // without description

    G4LogicalVolume(__void__&);
      // Fake default constructor for usage restricted to direct object
      // persistency for clients requiring preallocation of memory for
      // persistifiable objects.

    inline void Lock();
      // Set lock identifier for final deletion of entity.

  private:

    G4LogicalVolume(const G4LogicalVolume&);
    G4LogicalVolume& operator=(const G4LogicalVolume&);
      // Private copy-constructor and assignment operator.

  private:

    // Data members:   

    G4PhysicalVolumeList fDaughters;
      // Vector of daughters. Given initial size of 0.
    G4FieldManager* fFieldManager;
      // Pointer (possibly 0) to (magnetic or other) field manager object.
    G4Material* fMaterial;
      // Pointer to material at this node.
    G4String fName;
      // Name of logical volume.
    G4VSensitiveDetector* fSensitiveDetector;
      // Pointer (possibly 0) to `Hit' object.
    G4VSolid* fSolid;
      // Pointer to solid.
    G4UserLimits* fUserLimits;
      // Pointer (possibly 0) to user Step limit object for this node.
    G4SmartVoxelHeader* fVoxel;
      // Pointer (possibly 0) to optimisation info objects.
    G4bool fOptimise;
      // Flag to identify if optimisation should be applied or not.
    G4bool fRootRegion;
      // Flag to identify if the logical volume is a root region.
    G4bool fLock;
      // Flag to identify if entity is locked for final deletion.
    G4double fSmartless;
      // Quality for optimisation, average number of voxels to be spent
      // per content.
    G4double fMass;
      // Mass of the logical volume tree.
    const G4VisAttributes* fVisAttributes;
      // Pointer (possibly 0) to visualization attributes.
///    G4Region* fRegion;
      // Pointer to the cuts region (if any)
    G4MaterialCutsCouple* fCutsCouple;
      // Pointer (possibly 0) to associated production cuts.
    G4double fBiasWeight;
      // Weight used in the event biasing technique.
};

#include "G4LogicalVolume.icc"

#endif
