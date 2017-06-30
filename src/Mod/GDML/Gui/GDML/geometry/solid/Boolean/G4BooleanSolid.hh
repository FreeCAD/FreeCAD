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
// $Id: G4BooleanSolid.hh,v 1.15 2006/10/19 15:34:49 gcosmo Exp $
// GEANT4 tag $Name: geant4-09-03-patch-01 $
//
//
// class G4BooleanSolid
//
// Class description:
//
// Abstract base class for solids created by boolean operations
// between other solids.

// History:
//
// 10.09.98 V.Grichine, created
//
// --------------------------------------------------------------------
#ifndef G4BOOLEANSOLID_HH
#define G4BOOLEANSOLID_HH
/*
#include "G4DisplacedSolid.hh"

#include "G4ThreeVector.hh"
#include "G4RotationMatrix.hh"
#include "G4Transform3D.hh"
*/
#include "G4DisplacedSolid.hh"

#include "../../../global/G4ThreeVector.hh"
#include "../../management/G4RotationMatrix.hh"
#include "../../../global/G4Transform3D.hh"


class G4VSolid;

class G4BooleanSolid : public G4VSolid
{
  public:  // with description
 
    G4BooleanSolid( const G4String& pName,
                          G4VSolid* pSolidA ,
                          G4VSolid* pSolidB   );

    G4BooleanSolid( const G4String& pName,
                          G4VSolid* pSolidA ,
                          G4VSolid* pSolidB,
                          G4RotationMatrix* rotMatrix,
                    const G4ThreeVector& transVector    );

    G4BooleanSolid( const G4String& pName,
                          G4VSolid* pSolidA ,
                          G4VSolid* pSolidB , 
                    const G4Transform3D& transform   );

    virtual ~G4BooleanSolid();

    virtual const G4VSolid* GetConstituentSolid(G4int no) const;
    virtual       G4VSolid* GetConstituentSolid(G4int no);
      // If Solid is made up from a Boolean operation of two solids,
      // return the corresponding solid (for no=0 and 1).
      // If the solid is not a "Boolean", return 0.

    inline G4double GetCubicVolume();
    inline G4double GetSurfaceArea();

    virtual G4GeometryType  GetEntityType() const;
    virtual G4Polyhedron* GetPolyhedron () const;

    std::ostream& StreamInfo(std::ostream& os) const;

    inline G4int GetCubVolStatistics() const;
    inline G4double GetCubVolEpsilon() const;
    inline void SetCubVolStatistics(G4int st);
    inline void SetCubVolEpsilon(G4double ep);

    inline G4int GetAreaStatistics() const;
    inline G4double GetAreaAccuracy() const;
    inline void SetAreaStatistics(G4int st);
    inline void SetAreaAccuracy(G4double ep);

  ///  G4ThreeVector GetPointOnSurface() const;

  public:  // without description

    G4BooleanSolid(__void__&);
      // Fake default constructor for usage restricted to direct object
      // persistency for clients requiring preallocation of memory for
      // persistifiable objects.

  protected:
  
    G4VSolid* fPtrSolidA;
    G4VSolid* fPtrSolidB;

  private:

    G4BooleanSolid(const G4BooleanSolid&);
    G4BooleanSolid& operator=(const G4BooleanSolid&);
      // Private copy constructor and assignment operator.

  private:

    G4int    fStatistics;
    G4double fCubVolEpsilon;
    G4double fAreaAccuracy;
    G4double fCubicVolume;
    G4double fSurfaceArea;

    mutable G4Polyhedron* fpPolyhedron;

    G4bool  createdDisplacedSolid;
      // If & only if this object created it, it must delete it

} ;

#include "G4BooleanSolid.icc"

#endif
