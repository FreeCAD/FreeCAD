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
// $Id: G4ReflectionFactory.hh,v 1.4 2008/11/13 09:33:20 gcosmo Exp $
// GEANT4 tag $Name: geant4-09-03-patch-01 $
//
// 
// class G4ReflectionFactory
//
// Class description:
//
// Class providing functions for volumes placements with a general
// transfomation that can contain reflection.
// Reflection is then applied to a solid: a new G4ReflectedSolid
// instance is created and is placed with a transformation containing
// pure rotation and translation only.
// The pair of constituent and reflected logical volumes is
// considered as a generalized logical volume that is addressed
// by user specifying the constituent logical volume.
//
// Decomposition of a general transformation that can include reflection
// in a "reflection-free" transformation:
// 
// x(inM') = TG*x(inM)         TG - general transformation
//         = T*(R*x(inM))      T  - "reflection-free" transformation
//         = T* x(inReflM)   
//
// Daughters transformation:
// When a volume V containing daughter D with transformation TD
// is placed in mother M with a general tranformation TGV,
// the TGV is decomposed. New reflected volume ReflV containing
// a new daughter ReflD with reflected transformation ReflTD is created:
// 
// x(inV) = TD * x(inD);
// x(inM) = TGV * x(inV) 
//        = TV * R * x(inV) 
//        = TV * R * TD * x(inD)
//        = TV * R*TD*R-1 * R*x(inD)
//        = TV * ReflTD * x(inReflD)

// Author: Ivana Hrivnacova, 16.10.2001  (Ivana.Hrivnacova@cern.ch)
// --------------------------------------------------------------------
#ifndef G4_REFLECTION_FACTORY_HH
#define G4_REFLECTION_FACTORY_HH

///#include "G4Types.hh"
///#include "G4Transform3D.hh"
///#include "geomdefs.hh"
#include "../../global/G4Types.hh"
#include "../../global/G4Transform3D.hh"
#include "../../global/geomdefs.hh"

#include <map>

class G4VPhysicalVolume;
class G4LogicalVolume;
class G4VSolid;
class G4VPVDivisionFactory;

typedef std::pair<G4VPhysicalVolume*,
                  G4VPhysicalVolume*> G4PhysicalVolumesPair;  
typedef std::map<G4LogicalVolume*, G4LogicalVolume*,  
                 std::less<G4LogicalVolume*> > G4ReflectedVolumesMap;

class G4ReflectionFactory 
{
    typedef G4ReflectedVolumesMap::const_iterator LogicalVolumesMapIterator;

  public:  // with description
  
    virtual ~G4ReflectionFactory();
      // Virtual destructor.

    static G4ReflectionFactory* Instance();
      // Gets pointer to the instance of the singleton.

    G4PhysicalVolumesPair Place(const G4Transform3D& transform3D,
                                const G4String&  name,
                                      G4LogicalVolume* LV,
                                      G4LogicalVolume* motherLV,
                                      G4bool isMany, 
                                      G4int  copyNo,
                                      G4bool surfCheck=false);
      // Evaluates the passed transformation; if it contains reflection
      // it performs its decomposition, creates new reflected solid and
      // logical volume (or retrieves them from a map if the reflected
      // objects were already created), transforms the daughters (if present)
      // and place it in the given mother.
      // The result is a pair of physical volumes;
      // the second physical volume is a placement in a reflected mother
      // or 0 if mother LV was not reflected.

    G4PhysicalVolumesPair Replicate(const G4String& name, 
                                          G4LogicalVolume* LV,
                                          G4LogicalVolume* motherLV,
                                          EAxis axis, 
                                          G4int nofReplicas, 
                                          G4double width,
                                          G4double offset=0);
      // Creates replica in the given mother.
      // The result is a pair of physical volumes;
      // the second physical volume is a replica in a reflected mother
      // or 0 if mother LV was not reflected.

    G4PhysicalVolumesPair Divide(const G4String& name, 
                                       G4LogicalVolume* LV,
                                       G4LogicalVolume* motherLV,
                                       EAxis axis, 
                                       G4int nofDivisions, 
                                       G4double width,
                                       G4double offset);
    G4PhysicalVolumesPair Divide(const G4String& name, 
                                       G4LogicalVolume* LV,
                                       G4LogicalVolume* motherLV,
                                       EAxis axis, 
                                       G4int nofDivisions, 
                                       G4double offset);
    G4PhysicalVolumesPair Divide(const G4String& name, 
                                       G4LogicalVolume* LV,
                                       G4LogicalVolume* motherLV,
                                       EAxis axis, 
                                       G4double width,
                                       G4double offset);
      // Creates division in the given mother.
      // The result is a pair of physical volumes;
      // the second physical volume is a division in a reflected mother
      // or 0 if mother LV was not reflected.

    void  SetVerboseLevel(G4int verboseLevel);
    G4int GetVerboseLevel() const;
      // Sets/gets verbosity level.

    void     SetVolumesNameExtension(const G4String& nameExtension);
    const G4String& GetVolumesNameExtension() const;
      // Returns the name extension for the reflected solids
      // and logical volumes.

    void     SetScalePrecision(G4double scaleValue);
    G4double GetScalePrecision() const;
      // Sets/gets precision factor for the scale consistency check
      // The default value is set to 10*kCarTolerance.

    G4LogicalVolume* GetConstituentLV(G4LogicalVolume* reflLV) const;
      // Returns the consituent volume of the given reflected volume,
      // 0 if the given reflected volume was not found.

    G4LogicalVolume* GetReflectedLV(G4LogicalVolume* lv) const;
      // Returns the reflected volume of the given consituent volume,
      // 0 if the given volume was not reflected.

    G4bool IsConstituent(G4LogicalVolume* lv) const;
      // Returns true if the given volume has been already reflected
      // (is in the map of constituent volumes).

    G4bool IsReflected(G4LogicalVolume* lv) const;
      // Returns true if the given volume is a reflected volume
      // (is in the map reflected  volumes).

    const G4ReflectedVolumesMap& GetReflectedVolumesMap() const;
      // Returns a handle to the internal map of volumes which have
      // been reflected, after that placement or replication is performed.

  protected:  

    G4ReflectionFactory();
      // Protected singleton constructor.

    G4ReflectionFactory(const G4ReflectionFactory&);
    G4ReflectionFactory& operator=(const G4ReflectionFactory&);
      // Disabled copy constructor and assignment operator.
 
  private:  

    G4LogicalVolume*   ReflectLV(G4LogicalVolume* LV, G4bool surfCheck=false);
      // Gets/creates the reflected solid and logical volume
      // and copies + transforms LV daughters.

    G4LogicalVolume*   CreateReflectedLV(G4LogicalVolume* LV);
      // Creates the reflected solid and logical volume
      // and add the logical volumes pair in the maps.

    void ReflectDaughters(G4LogicalVolume* LV,
                          G4LogicalVolume* refLV, G4bool surfCheck=false);
      // Reflects daughters recursively.

    void ReflectPVPlacement(G4VPhysicalVolume* PV,
                            G4LogicalVolume* refLV, G4bool surfCheck=false);
      // Copies and transforms daughter of PVPlacement type of
      // a constituent volume into a reflected volume. 

    void ReflectPVReplica(G4VPhysicalVolume* PV, G4LogicalVolume* refLV);
      // Copies and transforms daughter of PVReplica type of
      // a constituent volume into a reflected volume. 

    void ReflectPVDivision(G4VPhysicalVolume* PV, G4LogicalVolume* refLV);
      // Copies and transforms daughter of PVDivision type of
      // a constituent volume into a reflected volume. 

    void ReflectPVParameterised(G4VPhysicalVolume* PV,
                                G4LogicalVolume* refLV, G4bool surfCheck=false);
      // Not implemented yet.
      // Should copy and transform daughter of PVReplica type of
      // a constituent volume into a reflected volume. 

    G4bool IsReflection(const G4Scale3D& scale) const;
      // Returns true if the scale is negative, false otherwise.

    void CheckScale(const G4Scale3D& scale) const;
      // Checks if scale correspond to fScale, if not gives exception.

    G4VPVDivisionFactory* GetPVDivisionFactory() const;
      // Checks if the division factory is instanciated,
      // if not gives exception.

    void PrintConstituentLVMap();  
      // Temporary - for debugging purpose.
  
  private:

    static G4ReflectionFactory* fInstance;
    static const G4String       fDefaultNameExtension;
    static const G4Scale3D      fScale;
    G4double                    fScalePrecision;

    G4int              fVerboseLevel;
    G4String           fNameExtension;
    G4ReflectedVolumesMap  fConstituentLVMap;
    G4ReflectedVolumesMap  fReflectedLVMap;
};

#endif
