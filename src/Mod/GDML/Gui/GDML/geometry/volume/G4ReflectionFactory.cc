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
// $Id: G4ReflectionFactory.cc,v 1.9 2008/11/13 09:33:20 gcosmo Exp $
// GEANT4 tag $Name: geant4-09-03-patch-01 $
//
// 
// Class G4ReflectionFactory Implementation
//
// Decomposition of a general transformation
// that can include reflection in a "reflection-free" transformation:
// 
// x(inM') = TG*x(inM)         TG - general transformation
//         = T*(R*x(inM))      T  - "reflection-free" transformation
//         = T* x(inReflM)   
//
// Daughters transformation:
// When a volume V containing daughter D with transformation TD
// is placed in mother M with a general tranformation TGV,
// the TGV is decomposed,
// new reflected volume ReflV containing a new daughter ReflD
// with reflected transformation ReflTD is created:
// 
// x(inV) = TD * x(inD);
// x(inM) = TGV * x(inV) 
//        = TV * R * x(inV) 
//        = TV * R * TD * x(inD)
//        = TV * R*TD*R-1 * R*x(inD)
//        = TV * ReflTD * x(inReflD)

// Author: Ivana Hrivnacova, 16.10.2001  (Ivana.Hrivnacova@cern.ch)
// --------------------------------------------------------------------

///#include "G4ReflectionFactory.hh"
///#include "G4ReflectedSolid.hh"
///#include "G4Region.hh" 
///#include "G4LogicalVolume.hh"  
///#include "G4PVPlacement.hh"  
///#include "G4PVReplica.hh"  
///#include "G4VPVDivisionFactory.hh"
///#include "G4GeometryTolerance.hh"

#include "G4ReflectionFactory.hh"
#include "../management/G4ReflectedSolid.hh"
///#include "G4Region.hh" 
#include "../management/G4LogicalVolume.hh"  
#include "G4PVPlacement.hh"  
///#include "G4PVReplica.hh"  
///#include "G4VPVDivisionFactory.hh"
///#include "G4GeometryTolerance.hh"


#include "../../../PreCompiled.h"
#include <Base/Console.h>


G4ReflectionFactory* G4ReflectionFactory::fInstance = 0;
const G4String  G4ReflectionFactory::fDefaultNameExtension = "_refl";
const G4Scale3D G4ReflectionFactory::fScale = G4ScaleZ3D(-1.0);

//_____________________________________________________________________________

G4ReflectionFactory* G4ReflectionFactory::Instance() 
{
  // Static singleton access method.
  // ---

  if (!fInstance) { fInstance = new G4ReflectionFactory(); }

  return fInstance;
}  

//_____________________________________________________________________________
/**/
G4ReflectionFactory::G4ReflectionFactory()
  : fVerboseLevel(0),
    fNameExtension(fDefaultNameExtension)    
{
  // Protected singleton constructor.
  // ---

//  fScalePrecision = 10.
//                  * G4GeometryTolerance::GetInstance()->GetSurfaceTolerance();
  fInstance = this;
}
/**/
//_____________________________________________________________________________

G4ReflectionFactory::~G4ReflectionFactory()
{
  delete fInstance;
}

//
// public methods
//

//_____________________________________________________________________________

G4PhysicalVolumesPair
G4ReflectionFactory::Place( const G4Transform3D& transform3D,
                            const G4String&      name,
                                  G4LogicalVolume* LV,
                                  G4LogicalVolume* motherLV,
                                  G4bool  isMany, 
                                  G4int   copyNo,
                                  G4bool  surfCheck)
{
  // Evaluates the passed transformation; if it contains reflection
  // it performs its decomposition, creates new reflected solid and
  // logical volume (or retrieves them from a map if the reflected
  // objects were already created), transforms the daughters (if present)
  // and place it in the given mother.
  // The result is a pair of physical volumes;
  // the second physical volume is a placement in a reflected mother
  // - or 0 if mother LV was not reflected.
  // ---

  if (fVerboseLevel>0)
  {
///    G4cout << "Place " << name << " lv " << LV << " "
///           << LV->GetName() << G4endl;
  }  

  // decompose transformation
  G4Scale3D     scale;
  G4Rotate3D    rotation;
  G4Translate3D translation;

  transform3D.getDecomposition(scale, rotation, translation);
  G4Transform3D pureTransform3D = translation * rotation;
  
  //PrintTransform(transform3D);
  //PrintTransform(pureTransform3D);

  // check that scale correspond to fScale
  //
  CheckScale(scale);
  
  //
  // reflection IS NOT present in transform3D 
  //

  if (! IsReflection(scale))
  {
///    if (fVerboseLevel>0)
 ///     G4cout << "Scale positive" << G4endl;

    G4VPhysicalVolume* pv1
      =  new G4PVPlacement(pureTransform3D, LV, name,
                           motherLV, isMany, copyNo, surfCheck);
 
    G4VPhysicalVolume* pv2 = 0;
    if (G4LogicalVolume* reflMotherLV = GetReflectedLV(motherLV))
    {
      // if mother was reflected
      // reflect this LV and place it in reflected mother
      
      pv2 = new G4PVPlacement(fScale * (pureTransform3D * fScale.inverse()),
                              ReflectLV(LV, surfCheck), name, reflMotherLV,
                              isMany, copyNo, surfCheck);
    }
    
    return G4PhysicalVolumesPair(pv1, pv2);            
  }         
           
  //
  //  reflection IS present in transform3D
  //

///  if (fVerboseLevel>0)
///    G4cout << "scale negative" << G4endl;

  G4VPhysicalVolume* pv1
    = new G4PVPlacement(pureTransform3D, ReflectLV(LV, surfCheck), name,
                        motherLV, isMany, copyNo, surfCheck);

  G4VPhysicalVolume* pv2 = 0;
  if (G4LogicalVolume* reflMotherLV = GetReflectedLV(motherLV))
  {

    // if mother was reflected
    // place the refLV consituent in reflected mother

    pv2 =  new G4PVPlacement(fScale * (pureTransform3D * fScale.inverse()),
                             LV, name, reflMotherLV, isMany, copyNo, surfCheck);
  }

  return G4PhysicalVolumesPair(pv1, pv2);  
}           

/*
//_____________________________________________________________________________

G4PhysicalVolumesPair
G4ReflectionFactory::Replicate(const G4String& name, 
                                     G4LogicalVolume* LV,
                                     G4LogicalVolume* motherLV,
                                     EAxis axis, 
                                     G4int nofReplicas, 
                                     G4double width,
                                     G4double offset)
{
  // Creates replica in given mother.
  // The result is a pair of physical volumes;
  // the second physical volume is a replica in a reflected mother
  // - or 0 if mother LV was not reflected.
  // ---

  if (fVerboseLevel>0)
  {
    G4cout << "Replicate " << name << " lv " << LV << " " 
           << LV->GetName() << G4endl;
  }  

  G4VPhysicalVolume* pv1
    = new G4PVReplica(name, LV, motherLV, axis, nofReplicas, width, offset);
 
  G4VPhysicalVolume* pv2 = 0;
  if (G4LogicalVolume* reflMotherLV = GetReflectedLV(motherLV))
  {
    // if mother was reflected
    // reflect the LV and replicate it in reflected mother
    
    pv2 = new G4PVReplica(name, ReflectLV(LV), reflMotherLV, 
                          axis, nofReplicas, width, offset); 
  }
    
  return G4PhysicalVolumesPair(pv1, pv2);            
}         
        
//_____________________________________________________________________________

G4PhysicalVolumesPair
G4ReflectionFactory::Divide(const G4String& name, 
                                  G4LogicalVolume* LV,
                                  G4LogicalVolume* motherLV,
                                  EAxis axis, 
                                  G4int nofDivisions, 
                                  G4double width,
                                  G4double offset)
{
  // Creates division in the given mother.
  // The result is a pair of physical volumes;
  // the second physical volume is a division in a reflected mother
  // or 0 if mother LV was not reflected.
  // ---

  if (fVerboseLevel>0)
  {
    G4cout << "Divide " << name << " lv " << LV << " " 
           << LV->GetName() << G4endl;
  }  

  G4VPVDivisionFactory* divisionFactory = GetPVDivisionFactory();

  G4VPhysicalVolume* pv1 = divisionFactory
      ->CreatePVDivision(name, LV, motherLV, axis, nofDivisions, width, offset);
 
  G4VPhysicalVolume* pv2 = 0;
  if (G4LogicalVolume* reflMotherLV = GetReflectedLV(motherLV))
  {
    // if mother was reflected
    // reflect the LV and replicate it in reflected mother
    
    pv2 = divisionFactory->CreatePVDivision(name, ReflectLV(LV), reflMotherLV, 
                                            axis, nofDivisions, width, offset); 
  }
    
  return G4PhysicalVolumesPair(pv1, pv2);            
}         
        
             
//_____________________________________________________________________________

G4PhysicalVolumesPair
G4ReflectionFactory::Divide(const G4String& name, 
                                  G4LogicalVolume* LV,
                                  G4LogicalVolume* motherLV,
                                  EAxis axis, 
                                  G4int nofDivisions, 
                                  G4double offset)
{
  // Creates division in the given mother.
  // The result is a pair of physical volumes;
  // the second physical volume is a division in a reflected mother
  // or 0 if mother LV was not reflected.
  // ---

  if (fVerboseLevel>0)
  {
    G4cout << "Divide " << name << " lv " << LV << " " 
           << LV->GetName() << G4endl;
  }  

  G4VPVDivisionFactory* divisionFactory = GetPVDivisionFactory();

  G4VPhysicalVolume* pv1 = divisionFactory
      ->CreatePVDivision(name, LV, motherLV, axis, nofDivisions, offset);
 
  G4VPhysicalVolume* pv2 = 0;
  if (G4LogicalVolume* reflMotherLV = GetReflectedLV(motherLV))
  {
    // if mother was reflected
    // reflect the LV and replicate it in reflected mother
    
    pv2 = divisionFactory->CreatePVDivision(name, ReflectLV(LV), reflMotherLV, 
                                            axis, nofDivisions, offset); 
  }
    
  return G4PhysicalVolumesPair(pv1, pv2);            
}         
        
             
//_____________________________________________________________________________

G4PhysicalVolumesPair
G4ReflectionFactory::Divide(const G4String& name, 
                                  G4LogicalVolume* LV,
                                  G4LogicalVolume* motherLV,
                                  EAxis axis, 
                                  G4double width,
                                  G4double offset)
{
  // Creates division in the given mother.
  // The result is a pair of physical volumes;
  // the second physical volume is a division in a reflected mother
  // or 0 if mother LV was not reflected.
  // ---

  if (fVerboseLevel>0)
  {
    G4cout << "Divide " << name << " lv " << LV << " " 
           << LV->GetName() << G4endl;
  }  

  G4VPVDivisionFactory* divisionFactory = GetPVDivisionFactory();

  G4VPhysicalVolume* pv1 = divisionFactory
    -> CreatePVDivision(name, LV, motherLV, axis, width, offset);
 
  G4VPhysicalVolume* pv2 = 0;
  if (G4LogicalVolume* reflMotherLV = GetReflectedLV(motherLV))
  {
    // if mother was reflected
    // reflect the LV and replicate it in reflected mother
    
    pv2 = divisionFactory->CreatePVDivision(name, ReflectLV(LV), reflMotherLV, 
                                            axis, width, offset); 
  }
    
  return G4PhysicalVolumesPair(pv1, pv2);            
}         
        
*/             
//
// private methods
//

//_____________________________________________________________________________

G4LogicalVolume* G4ReflectionFactory::ReflectLV(G4LogicalVolume* LV,
                                                G4bool surfCheck) 
{
  // Gets/creates the reflected solid and logical volume
  // and copies + transforms LV daughters.
  // ---

  G4LogicalVolume* refLV = GetReflectedLV(LV);

  if (!refLV)
  {

    // create new (reflected) objects
    //
    refLV = CreateReflectedLV(LV);
        
    // process daughters  
    //
    ReflectDaughters(LV, refLV, surfCheck);
/*
    // check if to be set as root region
    //
    if (LV->IsRootRegion())
    {
       LV->GetRegion()->AddRootLogicalVolume(refLV);
    }
	*/
  }

  return refLV;
}

//_____________________________________________________________________________

G4LogicalVolume* G4ReflectionFactory::CreateReflectedLV(G4LogicalVolume* LV) 
{
  // Creates the reflected solid and logical volume
  // and add the logical volumes pair in the maps.
  // ---

  // consistency check
  //
  if (fReflectedLVMap.find(LV) != fReflectedLVMap.end())
  {
/*    G4cerr << "ERROR - G4ReflectionFactory::CreateReflectedLV(..): "
           << LV->GetName() << G4endl
           << "        Cannot be applied to an already reflected volume !"
           << G4endl;
    G4Exception("G4ReflectionFactory::CreateReflectedLV(..)",
                "NotApplicable", FatalException,
                "Cannot be applied to a volume already reflected.");
*/  }        
              
  G4VSolid* refSolid 
    = new G4ReflectedSolid(LV->GetSolid()->GetName() + fNameExtension,
                           LV->GetSolid(), fScale);
  

  
  Base::Console().Message(LV->GetSolid()->GetName().c_str());




  G4LogicalVolume* refLV
    = new G4LogicalVolume(refSolid, 
                          LV->GetMaterial(),                 
                          LV->GetName() + fNameExtension,
                          LV->GetFieldManager(),
                          LV->GetSensitiveDetector(),
                          LV->GetUserLimits());
  refLV->SetVisAttributes(LV->GetVisAttributes());  // vis-attributes
  refLV->SetBiasWeight(LV->GetBiasWeight());        // biasing weight
/*  if (LV->IsRegion())
  {
    refLV->SetRegion(LV->GetRegion());              // set a region in case
  }
*/
  fConstituentLVMap[LV] = refLV;
  fReflectedLVMap[refLV] = LV;

  return refLV;        
}

//_____________________________________________________________________________

void G4ReflectionFactory::ReflectDaughters(G4LogicalVolume* LV, 
                                           G4LogicalVolume* refLV,
                                           G4bool surfCheck)
{
  // Reflects daughters recursively.
  // ---

  if (fVerboseLevel>0)
  {
///    G4cout << "G4ReflectionFactory::ReflectDaughters(): " 
///           << LV->GetNoDaughters() << " of " << LV->GetName() << G4endl;
  }     

  for (G4int i=0; i<LV->GetNoDaughters(); i++)
  {
    G4VPhysicalVolume* dPV = LV->GetDaughter(i);
    
//    if (! dPV->IsReplicated())
    {
      ReflectPVPlacement(dPV, refLV, surfCheck); 
    }  
/*    else if (! dPV->GetParameterisation())
    {
      ReflectPVReplica(dPV, refLV); 
    }  
    else if (G4VPVDivisionFactory::Instance() &&
             G4VPVDivisionFactory::Instance()->IsPVDivision(dPV))
    {
      ReflectPVDivision(dPV, refLV); 
    }  
    else
    {
      ReflectPVParameterised(dPV, refLV, surfCheck); 
    }*/
  }
}

//_____________________________________________________________________________

void G4ReflectionFactory::ReflectPVPlacement(G4VPhysicalVolume* dPV, 
                                             G4LogicalVolume* refLV,
                                             G4bool surfCheck)
{
  // Copies and transforms daughter of PVPlacement type of
  // a constituent volume into a reflected volume. 
  // ---

  G4LogicalVolume* dLV = dPV->GetLogicalVolume();

  // update daughter transformation
  //
  G4Transform3D dt(dPV->GetObjectRotationValue(), dPV->GetObjectTranslation());
  dt = fScale * (dt * fScale.inverse());

  G4LogicalVolume* refDLV;
  
///  if (fVerboseLevel>0) 
///    G4cout << "Daughter: " << dPV << "  " << dLV->GetName();
  
  if (!IsReflected(dLV))
  {

///    if (fVerboseLevel>0) 
///      G4cout << " will be reflected." << G4endl;

    // get reflected volume if already created    //
    refDLV = GetReflectedLV(dLV); 

    if (!refDLV)
    {
      // create new daughter solid and logical volume
      //
      refDLV = CreateReflectedLV(dLV); 
  
      // recursive call
      //
      ReflectDaughters(dLV, refDLV, surfCheck);   
    }  

    // create new daughter physical volume
    // with updated transformation

    new G4PVPlacement(dt, refDLV, dPV->GetName(), refLV, 
                      dPV->IsMany(), dPV->GetCopyNo(), surfCheck); 

  } 
  else
  {
 ///   if (fVerboseLevel>0) 
 ///     G4cout << " will be reconstitued." << G4endl;

    refDLV = GetConstituentLV(dLV); 

    new G4PVPlacement(dt, refDLV, dPV->GetName(), refLV, 
                      dPV->IsMany(), dPV->GetCopyNo(), surfCheck); 
  }       
}    
/*
//_____________________________________________________________________________

void G4ReflectionFactory::ReflectPVReplica(G4VPhysicalVolume* dPV, 
                                           G4LogicalVolume* refLV)
{
  // Copies and transforms daughter of PVReplica type of
  // a constituent volume into a reflected volume. 
  // ---

  G4LogicalVolume* dLV = dPV->GetLogicalVolume();

  // get replication data
  //
  EAxis axis;
  G4int nofReplicas;
  G4double width;
  G4double offset;
  G4bool consuming;

  dPV->GetReplicationData(axis, nofReplicas, width, offset, consuming);

  G4LogicalVolume* refDLV;
  
  if (fVerboseLevel>0) 
    G4cout << "Daughter: " << dPV << "  " << dLV->GetName();
  
  if (!IsReflected(dLV))
  {
    if (fVerboseLevel>0) 
      G4cout << " will be reflected." << G4endl;

    // get reflected volume if already created
    //
    refDLV = GetReflectedLV(dLV); 

    if (!refDLV)
    {
      // create new daughter solid and logical volume
      //
      refDLV = CreateReflectedLV(dLV); 
  
      // recursive call
      //
      ReflectDaughters(dLV, refDLV); 
    }     
        
    // create new daughter replica
    //
    new G4PVReplica(dPV->GetName(), refDLV, refLV, 
                    axis, nofReplicas, width, offset);
  }
  else
  {
    if (fVerboseLevel>0) 
      G4cout << " will be reconstitued." << G4endl;

    refDLV = GetConstituentLV(dLV); 

    new G4PVReplica(dPV->GetName(), refDLV, refLV, 
                    axis, nofReplicas, width, offset); 
  }       
}

//_____________________________________________________________________________

void G4ReflectionFactory::ReflectPVDivision(G4VPhysicalVolume* dPV, 
                                            G4LogicalVolume* refLV)
{
  // Copies and transforms daughter of PVDivision type of
  // a constituent volume into a reflected volume. 
  // ---

  G4VPVDivisionFactory* divisionFactory = GetPVDivisionFactory();

  G4LogicalVolume* dLV = dPV->GetLogicalVolume();

  // get parameterisation data
  //
  G4VPVParameterisation* param = dPV->GetParameterisation();

  G4LogicalVolume* refDLV;
  
  if (fVerboseLevel>0) 
    G4cout << "Daughter: " << dPV << "  " << dLV->GetName();
  
  if (!IsReflected(dLV))
  {
    if (fVerboseLevel>0) 
      G4cout << " will be reflected." << G4endl;

    // get reflected volume if already created
    //
    refDLV = GetReflectedLV(dLV); 

    if (!refDLV)
    {
      // create new daughter solid and logical volume
      //
      refDLV = CreateReflectedLV(dLV); 
  
      // recursive call
      //
      ReflectDaughters(dLV, refDLV); 
    }     
        
    // create new daughter replica
    //
    divisionFactory->CreatePVDivision(dPV->GetName(), refDLV, refLV, param);
  }
  else
  {
    if (fVerboseLevel>0) 
      G4cout << " will be reconstitued." << G4endl;

    refDLV = GetConstituentLV(dLV); 

    divisionFactory->CreatePVDivision(dPV->GetName(), refDLV, refLV, param); 
  }       
}

//_____________________________________________________________________________

void G4ReflectionFactory::ReflectPVParameterised(G4VPhysicalVolume* dPV, 
                                                 G4LogicalVolume*, G4bool)
{
  // Not implemented.
  // Should copy and transform daughter of PVReplica type of
  // a constituent volume into a reflected volume. 
  // ---

  G4cerr << "ERROR - G4ReflectionFactory::ReflectPVParameterised(...): "
         << dPV->GetName() << G4endl
         << "        Reflection of parameterised volumes "
         << "is not yet implemented." << G4endl;
  G4Exception("G4ReflectionFactory::ReflectPVParameterised(...)",
              "NotImplemented", FatalException,
              "Sorry, not yet implemented.");
}
*/
//_____________________________________________________________________________

G4LogicalVolume*
G4ReflectionFactory::GetConstituentLV(G4LogicalVolume* reflLV) const
{              
  // Returns the consituent volume of the given reflected volume,
  // 0 if the given reflected volume was not found.
  // ---

  LogicalVolumesMapIterator it = fReflectedLVMap.find(reflLV);

  if (it == fReflectedLVMap.end()) return 0;

  return (*it).second;
}        

//_____________________________________________________________________________

G4LogicalVolume*
G4ReflectionFactory::GetReflectedLV(G4LogicalVolume* lv) const
{              
  // Returns the reflected volume of the given consituent volume,
  // 0 if the given volume was not reflected.
  // ---

  LogicalVolumesMapIterator it = fConstituentLVMap.find(lv);

  if (it == fConstituentLVMap.end()) return 0;

  return (*it).second;
}        
/*
//_____________________________________________________________________________

G4bool G4ReflectionFactory::IsConstituent(G4LogicalVolume* lv) const
{
  // Returns true if the given volume has been already reflected
  // (is in the map of constituent volumes).
  // ---

  return (fConstituentLVMap.find(lv) != fConstituentLVMap.end());
}  

//_____________________________________________________________________________
*/
G4bool G4ReflectionFactory::IsReflected(G4LogicalVolume* lv) const
{
  // Returns true if the given volume is a reflected volume
  // (is in the map reflected  volumes).
  // ---

  return (fReflectedLVMap.find(lv) != fReflectedLVMap.end());
}  

//_____________________________________________________________________________

G4bool G4ReflectionFactory::IsReflection(const G4Scale3D& scale) const
{
  // Returns true if the scale is negative, false otherwise.
  // ---

  if (scale(0,0)*scale(1,1)*scale(2,2) < 0.)
    return true;
  else 
    return false;  
}
/*
//_____________________________________________________________________________

const G4ReflectedVolumesMap&
G4ReflectionFactory::GetReflectedVolumesMap() const
{
  return fReflectedLVMap;
}

//_____________________________________________________________________________

void G4ReflectionFactory::PrintConstituentLVMap()
{
  // temporary - for debugging purpose
  // ---

  LogicalVolumesMapIterator it;
  for (it = fConstituentLVMap.begin(); it != fConstituentLVMap.end(); it++)
  {
    G4cout << "lv: " << (*it).first << "  lv_refl: " << (*it).second << G4endl;
  }
  G4cout << G4endl;
}  
*/
//_____________________________________________________________________________

void G4ReflectionFactory::CheckScale(const G4Scale3D& scale) const
{
  // Check if scale correspond to fScale,
  // if not give exception.
  // ---

  if (!IsReflection(scale)) return;
  
  G4double diff = 0.;
  for (G4int i=0; i<4; i++)
    for (G4int j=0; j<4; j++) 
      diff += std::abs(scale(i,j) - fScale(i,j));  

  if (diff > fScalePrecision)
  {
 /*   G4cerr << "ERROR - G4ReflectionFactory::CheckScale(..)" << G4endl
           << "        Unexpected scale. Difference: " << diff << G4endl;
    G4Exception("G4ReflectionFactory::CheckScale(..)",
                "WrongArgumentValue", FatalException,
                "Unexpected scale in input !");
*/  }
}    
/*
//_____________________________________________________________________________

G4VPVDivisionFactory* G4ReflectionFactory::GetPVDivisionFactory() const
{
  // Returns the G4PVDivisionFactory instance if it exists,
  // otherwise gives exception 
  // ---

  G4VPVDivisionFactory* divisionFactory = G4VPVDivisionFactory::Instance();
  if (!divisionFactory)
  {
     G4cerr << "ERROR - G4ReflectionFactory::GetPVDivisionFactory()" << G4endl
            << "        It has been requested to reflect divided volumes."
            << G4endl
            << "        In this case, it is required to instantiate a concrete"
            << G4endl
            << "        factory G4PVDivisionFactory in your program -before-"
            << G4endl
            << "        executing the reflection !" << G4endl;
     G4Exception("G4ReflectionFactory::GetPVDivisionFactory()",
                 "WrongSetup", FatalException,
                 "A concrete G4PVDivisionFactory instantiated is required !");
  }
  
  return divisionFactory;
}  

//_____________________________________________________________________________

void G4ReflectionFactory::SetScalePrecision(G4double scaleValue)
{
  fScalePrecision = scaleValue;
}

//_____________________________________________________________________________

G4double G4ReflectionFactory::GetScalePrecision() const
{
  return fScalePrecision;
}

//_____________________________________________________________________________

void G4ReflectionFactory::SetVerboseLevel(G4int verboseLevel)
{
  fVerboseLevel = verboseLevel;
}
          
//_____________________________________________________________________________

G4int G4ReflectionFactory::GetVerboseLevel() const
{
  return fVerboseLevel;
}

//_____________________________________________________________________________

void G4ReflectionFactory::SetVolumesNameExtension(const G4String& nameExtension)
{
  fNameExtension = nameExtension;
}
          
//_____________________________________________________________________________

const G4String& G4ReflectionFactory::GetVolumesNameExtension() const
{
  return fNameExtension;
}
      */    
/*
  // placement with decomposed transformation

  G4VPhysicalVolume* pv1
    =  new G4PVPlacement(new G4RotationMatrix(rotation.getRotation().inverse()),
                         translation.getTranslation(),
            refLV, name, motherLV, isMany, copyNo);
*/
