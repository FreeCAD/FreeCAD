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
// $Id: G4VPhysicalVolume.hh,v 1.17 2007/04/11 08:00:12 gcosmo Exp $
// GEANT4 tag $Name: geant4-09-03-patch-01 $
//
//
// class G4VPhysicalVolume
//
// Class description:
//
// This is an Abstract Base class for the representation of positioned volume.  
// The volume is placed within a mother volume,  relative to its coordinate 
// system.  Either a single positioned volume or many positioned volume can 
// be represented by a particular G4VPhysicalVolume.

// History:
// 09.11.99 J.Apostolakis  Added GetObjectRotationValue() method & redid comments.
// 28.08.96 P.Kent Replaced transform by rotmat + vector
// 25.07.96 P.Kent Modified interface for new `Replica' capable geometry 
// 24.07.95 P.Kent First non-stub version
// --------------------------------------------------------------------
#ifndef G4VPHYSICALVOLUME_HH
#define G4VPHYSICALVOLUME_HH

///#include "G4Types.hh"
///#include "G4String.hh"
#include "../../global/G4Types.hh"
#include "../../global/G4String.hh"

///#include "geomdefs.hh"
#include "../../global/geomdefs.hh"

///#include "G4RotationMatrix.hh"
///#include "G4ThreeVector.hh"
#include "../management/G4RotationMatrix.hh"
#include "../../global/G4ThreeVector.hh"

class G4LogicalVolume;
class G4VPVParameterisation;

class G4VPhysicalVolume
{
  public:  // with description

    G4VPhysicalVolume(G4RotationMatrix *pRot,
                const G4ThreeVector &tlate,
                const G4String &pName,
                      G4LogicalVolume *pLogical,
                      G4VPhysicalVolume *pMother);//before
//                      G4LogicalVolume *pMother);//CAD-GDML
      // Initialise volume, positioned in a frame which is rotated by *pRot, 
      // relative to the coordinate system of the mother volume pMother.
      // The center of the object is then placed at tlate in the new
      // coordinates. If pRot=0 the volume is unrotated with respect to its
      // mother. The physical volume is added to the mother's logical volume.
      //
      // Must be called by all subclasses. pMother must point to a valid parent
      // volume, except in the case of the world/top volume, when it =0.
      // 
      // The constructor also registers volume with physical volume Store.
      // Note that the Store may be removed or dynamically built in future
      // because of memory constraints.

    virtual ~G4VPhysicalVolume();
      // Destructor, will be subclassed. Removes volume from volume Store.

    inline G4bool operator == (const G4VPhysicalVolume& p) const;
      // Equality defined by equal addresses only.

    // Access functions

      // The following are accessor functions that make a distinction
      // between whether the rotation/translation is being made for the
      // frame or the object/volume that is being placed.
      // (They are the inverse of each other).
    G4RotationMatrix* GetObjectRotation() const;              //  Obsolete 
    inline G4RotationMatrix  GetObjectRotationValue() const;  //  Replacement
    inline G4ThreeVector  GetObjectTranslation() const;
      // Return the rotation/translation of the Object relative to the mother.
    inline const G4RotationMatrix* GetFrameRotation() const;
    inline G4ThreeVector  GetFrameTranslation() const;
      // Return the rotation/translation of the Frame used to position 
      // this volume in its mother volume (opposite of object rot/trans).

    // Older access functions, that do not distinguish between frame/object!

    inline const G4ThreeVector& GetTranslation() const;
    inline const G4RotationMatrix* GetRotation() const;
      // Old access functions, that do not distinguish between frame/object!
      // They return the translation/rotation of the volume.

    // Set functions

    inline void SetTranslation(const G4ThreeVector &v);
    inline G4RotationMatrix* GetRotation();
    inline void SetRotation(G4RotationMatrix*);
      // NOT INTENDED FOR GENERAL USE.
      // Non constant versions of above. Used to change transformation
      // for replication/parameterisation mechanism.

    inline G4LogicalVolume* GetLogicalVolume() const;
      // Return the associated logical volume.
    inline void SetLogicalVolume(G4LogicalVolume *pLogical);
      // Set the logical volume. Must not be called when geometry closed.

    inline G4LogicalVolume* GetMotherLogical() const;
      // Return the current mother logical volume pointer.
    inline void SetMotherLogical(G4LogicalVolume *pMother);
      // Set the mother logical volume. Must not be called when geometry closed.

    inline const G4String& GetName() const;
      // Return the volume's name.
    inline void SetName(const G4String& pName);
      // Set the volume's name.

    virtual G4int GetMultiplicity() const;
      // Returns number of object entities (1 for normal placements,
      // n for replicas or parameterised).

    // Functions required of subclasses
 //modification so that G4VPhysicalVolume is not abstract
    virtual G4bool IsMany() const = 0;
      // Return true if the volume is MANY (not implemented yet).
    virtual G4int GetCopyNo() const = 0;
      // Return the volumes copy number.
/*    virtual void  SetCopyNo(G4int CopyNo) = 0;
      // Set the volumes copy number.
    virtual G4bool IsReplicated() const = 0;
      // Return true if replicated (single object instance represents
      // many real volumes), else false.
    virtual G4bool IsParameterised() const = 0;
      // Return true if parameterised (single object instance represents
      // many real parameterised volumes), else false.
*/
///
	virtual G4VPVParameterisation* GetParameterisation() const = 0;
 /*     // Return replicas parameterisation object (able to compute dimensions
      // and transformations of replicas), or NULL if not applicable.
    virtual void GetReplicationData(EAxis& axis,
                                    G4int& nReplicas,
                                    G4double& width,
                                    G4double& offset,
                                    G4bool& consuming) const = 0;
      // Return replication information. No-op for no replicated volumes.
    virtual G4bool  IsRegularStructure() const = 0;
      // Returns true if the underlying volume structure is regular.
    virtual G4int  GetRegularStructureId() const = 0;
      // Returns non-zero code in case the underlying volume structure 
      //  is regular, voxel-like.  Value is id for structure type.
      //  If non-zero the volume is a candidate for specialised 
      //  navigation such as 'nearest neighbour' directly on volumes.
*/    virtual G4bool CheckOverlaps(G4int res=1000, G4double tol=0.,
                                 G4bool verbose=true);
      // Verifies if the placed volume is overlapping with existing
      // daughters or with the mother volume. Provides default resolution
      // for the number of points to be generated and verified.
      // Concrete implementation is done and required only for placed and
      // parameterised volumes. Returns true if the volume is overlapping.

  public:  // without description

    G4VPhysicalVolume(__void__&);
      // Fake default constructor for usage restricted to direct object
      // persistency for clients requiring preallocation of memory for
      // persistifiable objects.

  private:

    G4VPhysicalVolume(const G4VPhysicalVolume&);
    G4VPhysicalVolume& operator=(const G4VPhysicalVolume&);
      // Private copy constructor and assignment operator.

  protected:

    G4RotationMatrix *frot;
    G4ThreeVector ftrans;

  private:

    G4LogicalVolume *flogical;   // The logical volume representing the
                                 // physical and tracking attributes of
                                 // the volume
    G4String fname;              // The name of the volume
    G4LogicalVolume   *flmother; // The current mother logical volume
};

#include "G4VPhysicalVolume.icc"

#endif
