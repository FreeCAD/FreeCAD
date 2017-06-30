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
// $Id: G4PVPlacement.cc,v 1.16 2007/04/11 07:56:38 gcosmo Exp $
// GEANT4 tag $Name: geant4-09-03-patch-01 $
//
// 
// class G4PVPlacement Implementation
//
// ----------------------------------------------------------------------

///#include "G4PVPlacement.hh"
///#include "G4AffineTransform.hh"
///#include "G4UnitsTable.hh"
///#include "G4LogicalVolume.hh"
///#include "G4VSolid.hh"

#include "G4PVPlacement.hh"
#include "../management/G4AffineTransform.hh"
///#include "G4UnitsTable.hh"
#include "../management/G4LogicalVolume.hh"
#include "../management/G4VSolid.hh"
/**/
// ----------------------------------------------------------------------
// Constructor
//
G4PVPlacement::G4PVPlacement( G4RotationMatrix *pRot,
                        const G4ThreeVector &tlate,
                        const G4String& pName,
                              G4LogicalVolume *pLogical,
                              G4VPhysicalVolume *pMother,
                              G4bool pMany,
                              G4int pCopyNo,
                              G4bool pSurfChk )
  : G4VPhysicalVolume(pRot,tlate,pName,pLogical,pMother),
    fmany(pMany), fallocatedRotM(false), fcopyNo(pCopyNo)
{
  if (pMother)
  {
    G4LogicalVolume* motherLogical = pMother->GetLogicalVolume();
    if (pLogical == motherLogical)
    {
 //     G4Exception("G4PVPlacement::G4PVPlacement()", "InvalidSetup",
 //                 FatalException, "Cannot place a volume inside itself!");
    }
    SetMotherLogical(motherLogical);
    motherLogical->AddDaughter(this);
    if (pSurfChk) { CheckOverlaps(); }
  }
}
/**/
// ----------------------------------------------------------------------
// Constructor
//
G4PVPlacement::G4PVPlacement( const G4Transform3D &Transform3D,
                              const G4String& pName,
                                    G4LogicalVolume *pLogical,
                                    G4VPhysicalVolume *pMother,
                                    G4bool pMany,
                                    G4int pCopyNo,
                                    G4bool pSurfChk )
  : G4VPhysicalVolume(NewPtrRotMatrix(Transform3D.getRotation().inverse()),
                      Transform3D.getTranslation(),pName,pLogical,pMother),
    fmany(pMany), fcopyNo(pCopyNo)
{
  fallocatedRotM = (GetRotation() != 0);
  if (pMother)
  {
    G4LogicalVolume* motherLogical = pMother->GetLogicalVolume();
///    if (pLogical == motherLogical)
///      G4Exception("G4PVPlacement::G4PVPlacement()", "InvalidSetup",
///                  FatalException, "Cannot place a volume inside itself!");
    SetMotherLogical(motherLogical);
    motherLogical->AddDaughter(this);
    if (pSurfChk) { CheckOverlaps(); }
  }
}
/**/
// ----------------------------------------------------------------------
// Constructor
//
// The logical volume of the mother is utilised (not the physical)
//
G4PVPlacement::G4PVPlacement( G4RotationMatrix *pRot,
                        const G4ThreeVector &tlate,
                              G4LogicalVolume *pCurrentLogical,
                        const G4String& pName,
                              G4LogicalVolume *pMotherLogical,
                              G4bool pMany,
                              G4int pCopyNo,
                              G4bool pSurfChk )
  : G4VPhysicalVolume(pRot,tlate,pName,pCurrentLogical,0),
    fmany(pMany), fallocatedRotM(false), fcopyNo(pCopyNo)
{
  if (pCurrentLogical == pMotherLogical)
  {
 //   G4Exception("G4PVPlacement::G4PVPlacement()", "InvalidSetup",
 //               FatalException, "Cannot place a volume inside itself!");
  }
  SetMotherLogical(pMotherLogical);
  if (pMotherLogical) { pMotherLogical->AddDaughter(this); }
  if ((pSurfChk) && (pMotherLogical)) { CheckOverlaps(); }
}

/**/
// ----------------------------------------------------------------------
// Constructor
//
G4PVPlacement::G4PVPlacement( const G4Transform3D &Transform3D,
                                    G4LogicalVolume *pCurrentLogical,
                              const G4String& pName,
                                    G4LogicalVolume *pMotherLogical,
                                    G4bool pMany,
                                    G4int pCopyNo,
                                    G4bool pSurfChk )
  : G4VPhysicalVolume(0,Transform3D.getTranslation(),pName,pCurrentLogical,0),
    fmany(pMany), fcopyNo(pCopyNo)
{
  if (pCurrentLogical == pMotherLogical)
  {
///    G4Exception("G4PVPlacement::G4PVPlacement()", "InvalidSetup",
///                FatalException, "Cannot place a volume inside itself!");
  }
  SetRotation( NewPtrRotMatrix(Transform3D.getRotation().inverse()) );
  fallocatedRotM = (GetRotation() != 0);
  SetMotherLogical(pMotherLogical);
  if (pMotherLogical) { pMotherLogical->AddDaughter(this); }
  if ((pSurfChk) && (pMotherLogical)) { CheckOverlaps(); }
}

// ----------------------------------------------------------------------
// Fake default constructor - sets only member data and allocates memory
//                            for usage restricted to object persistency.
//
G4PVPlacement::G4PVPlacement( __void__& a )
  : G4VPhysicalVolume(a)
{
}

// ----------------------------------------------------------------------
// Destructor
//
G4PVPlacement::~G4PVPlacement()
{
  if( fallocatedRotM ){ delete frot; }
}

// ----------------------------------------------------------------------
// IsMany
//
G4bool G4PVPlacement::IsMany() const
{
  return fmany; 
}

// ----------------------------------------------------------------------
// GetCopyNo
//
G4int G4PVPlacement::GetCopyNo() const
{
  return fcopyNo;
}

// ----------------------------------------------------------------------
// SetCopyNo
//
void G4PVPlacement::SetCopyNo(G4int newCopyNo)
{
  fcopyNo= newCopyNo;
}

// ----------------------------------------------------------------------
// IsReplicated
//
G4bool G4PVPlacement::IsReplicated() const
{
  return false;
}

// ----------------------------------------------------------------------
// IsParameterised
//
G4bool G4PVPlacement::IsParameterised() const
{
  return false;
}

// ----------------------------------------------------------------------
// GetParameterisation
//
/**/
G4VPVParameterisation* G4PVPlacement::GetParameterisation() const
{
  return 0;
}
/**/
// ----------------------------------------------------------------------
// GetReplicationData
//
void G4PVPlacement::
GetReplicationData( EAxis&, G4int&, G4double&, G4double&, G4bool& ) const
{
  // No-operations
}

// ----------------------------------------------------------------------
// IsRegularRepeatedStructure
//
// This is for specialised repeated volumes (replicas, parameterised vol.)
//
G4bool G4PVPlacement::IsRegularStructure() const
{
  return false;
}           

// ----------------------------------------------------------------------
// IsRegularRepeatedStructure
//
// This is for specialised repeated volumes (replicas, parameterised vol.)
//
G4int G4PVPlacement::GetRegularStructureId() const
{
  return 0;  
}           

// ----------------------------------------------------------------------
// CheckOverlaps
//
/*
G4bool G4PVPlacement::CheckOverlaps(G4int res, G4double tol, G4bool verbose)
{
  if (res<=0) { return false; }

  G4VSolid* solid = GetLogicalVolume()->GetSolid();
  G4LogicalVolume* motherLog = GetMotherLogical();
  if (!motherLog) { return false; }

  G4VSolid* motherSolid = motherLog->GetSolid();

  if (verbose)
  {
    G4cout << "Checking overlaps for volume " << GetName() << " ... ";
  }

  // Create the transformation from daughter to mother
  //
  G4AffineTransform Tm( GetRotation(), GetTranslation() );

  for (G4int n=0; n<res; n++)
  {
    // Generate a random point on the solid's surface
    //
    G4ThreeVector point = solid->GetPointOnSurface();

    // Transform the generated point to the mother's coordinate system
    //
    G4ThreeVector mp = Tm.TransformPoint(point);

    // Checking overlaps with the mother volume
    //
    if (motherSolid->Inside(mp)==kOutside)
    {
      G4double distin = motherSolid->DistanceToIn(mp);
      if (distin > tol)
      {
        G4cout << G4endl;
        G4cout << "WARNING - G4PVPlacement::CheckOverlaps()" << G4endl
               << "          Overlap is detected for volume "
               << GetName() << G4endl
               << "          with its mother volume "
               << motherLog->GetName() << G4endl
               << "          at mother local point " << mp << ", "
               << "overlapping by at least: " << G4BestUnit(distin, "Length")
               << G4endl;
        G4Exception("G4PVPlacement::CheckOverlaps()", "InvalidSetup",
                    JustWarning, "Overlap with mother volume !");
        return true;
      }
    }

    // Checking overlaps with each 'sister' volume
    //
    for (G4int i=0; i<motherLog->GetNoDaughters(); i++)
    {
      G4VPhysicalVolume* daughter = motherLog->GetDaughter(i);

      if (daughter == this) { continue; }

      // Create the transformation for daughter volume and transform point
      //
      G4AffineTransform Td( daughter->GetRotation(),
                            daughter->GetTranslation() );
      G4ThreeVector md = Td.Inverse().TransformPoint(mp);

      G4VSolid* daughterSolid = daughter->GetLogicalVolume()->GetSolid();
      if (daughterSolid->Inside(md)==kInside)
      {
        G4double distout = daughterSolid->DistanceToOut(md);
        if (distout > tol)
        {
          G4cout << G4endl;
          G4cout << "WARNING - G4PVPlacement::CheckOverlaps()" << G4endl
                 << "          Overlap is detected for volume "
                 << GetName() << G4endl
                 << "          with " << daughter->GetName() << " volume's"
                 << G4endl
                 << "          local point " << md << ", "
                 << "overlapping by at least: " << G4BestUnit(distout,"Length")
                 << G4endl;
          G4Exception("G4PVPlacement::CheckOverlaps()", "InvalidSetup",
                      JustWarning, "Overlap with volume already placed !");
          return true;
        }
      }

      // Now checking that 'sister' volume is not totally included and
      // overlapping. Do it only once, for the first point generated
      //
      if (n==0)
      {
        // Generate a single point on the surface of the 'sister' volume
        // and verify that the point is NOT inside the current volume

        G4ThreeVector dPoint = daughterSolid->GetPointOnSurface();

        // Transform the generated point to the mother's coordinate system
        // and finally to current volume's coordinate system
        //
        G4ThreeVector mp2 = Td.TransformPoint(dPoint);
        G4ThreeVector ms = Tm.Inverse().TransformPoint(mp2);

        if (solid->Inside(ms)==kInside)
        {
           G4cout << G4endl;
           G4cout << "WARNING - G4PVPlacement::CheckOverlaps()" << G4endl
                  << "          Overlap is detected for volume "
                  << GetName() << G4endl
                  << "          apparently fully encapsulating volume "
                  << daughter->GetName() << G4endl
                  << "          at the same level !" << G4endl;
           G4Exception("G4PVPlacement::CheckOverlaps()", "InvalidSetup",
                       JustWarning, "Overlap with volume already placed !");
          return true;
        }
      }
    }
  }

  if (verbose)
  {
    G4cout << "OK! " << G4endl;
  }

  return false;
}
*/
// ----------------------------------------------------------------------
// NewPtrRotMatrix
//
// Auxiliary function for 2nd & 4th constructors (those with G4Transform3D)
// Creates a new rotation matrix on the heap (using "new") and copies its
// argument into it.
//
// NOTE: Ownership of the returned pointer is left to the caller !
//       No entity is currently responsible to delete this memory. 
//
G4RotationMatrix*
G4PVPlacement::NewPtrRotMatrix(const G4RotationMatrix &RotMat)
{
  G4RotationMatrix *pRotMatrix; 
  if ( RotMat.isIdentity() )
  {
     pRotMatrix = 0;
  }
  else
  {
     pRotMatrix = new G4RotationMatrix(RotMat);
  }
  // fallocatedRotM= ! (RotMat.isIdentity());
    
  return pRotMatrix;
}
