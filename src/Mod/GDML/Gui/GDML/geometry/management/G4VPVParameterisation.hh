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
// $Id: G4VPVParameterisation.hh,v 1.13 2007/07/16 08:40:13 gcosmo Exp $
// GEANT4 tag $Name: geant4-09-03-patch-01 $
//
// class G4VPVParamterisation
//
// Class description:
//
// Parameterisation class, able to compute the transformation and
// (indirectly) the dimensions of parameterised volumes, given a
// replication number.

// History:
// 25.07.96 P.Kent        Initial stub version
// 20.09.96 V.Grichine    Modifications for G4Trap/Cons/Sphere
// 31.10.96 V.Grichine    Modifications for G4Torus/Para
// 17.02.98 J.Apostolakis Allowing the parameterisation of Solid type
// --------------------------------------------------------------------
#ifndef G4VPVPARAMETERISATION_HH
#define G4VPVPARAMETERISATION_HH

#include "../../global/G4Types.hh"
//#include "G4VVolumeMaterialScanner.hh"

class G4VPhysicalVolume;
class G4VTouchable; 
class G4VSolid;
class G4Material;

// CSG Entities which may be parameterised/replicated
//
class G4Box;
class G4Tubs;
class G4Trd;
class G4Trap;
class G4Cons;
class G4Sphere;
class G4Orb;
class G4Torus;
class G4Para;
class G4Polycone;
class G4Polyhedra;
class G4Hype;

class G4VVolumeMaterialScanner; 

class G4VPVParameterisation
{
  public:

    G4VPVParameterisation();
    virtual ~G4VPVParameterisation();

  public:  // with description

    virtual void ComputeTransformation(const G4int,
                                       G4VPhysicalVolume * ) const = 0;

    virtual G4VSolid*   ComputeSolid(const G4int, G4VPhysicalVolume *);
				       
    virtual G4Material* ComputeMaterial(const G4int repNo, 
                                        G4VPhysicalVolume *currentVol,
                                        const G4VTouchable *parentTouch=0);
       //  Refined method, enabling nested parameterisations

    virtual G4bool IsNested() const;
    virtual G4VVolumeMaterialScanner* GetMaterialScanner(); 
       //   These enable material scan for nested parameterisations

    virtual void ComputeDimensions(G4Box &,
                                   const G4int,
                                   const G4VPhysicalVolume *) const {}

    virtual void ComputeDimensions(G4Tubs &,
                                   const G4int,
                                   const G4VPhysicalVolume *) const {}

    virtual void ComputeDimensions(G4Trd &,
                                   const G4int,
                                   const G4VPhysicalVolume *) const {}
	
    virtual void ComputeDimensions(G4Trap &,
                                   const G4int,
                                   const G4VPhysicalVolume *) const {}
	
    virtual void ComputeDimensions(G4Cons &,
                                   const G4int,
                                   const G4VPhysicalVolume *) const {}

    virtual void ComputeDimensions(G4Sphere &,
                                   const G4int,
                                   const G4VPhysicalVolume *) const {}

    virtual void ComputeDimensions(G4Orb &,
                                   const G4int,
                                   const G4VPhysicalVolume *) const {}

    virtual void ComputeDimensions(G4Torus &,
                                   const G4int,
                                   const G4VPhysicalVolume *) const {}

    virtual void ComputeDimensions(G4Para &,
                                   const G4int,
                                   const G4VPhysicalVolume *) const {}

    virtual void ComputeDimensions(G4Polycone &,
                                   const G4int,
                                   const G4VPhysicalVolume *) const {}

    virtual void ComputeDimensions(G4Polyhedra &,
                                   const G4int,
                                   const G4VPhysicalVolume *) const {}

    virtual void ComputeDimensions(G4Hype &,
                                   const G4int,
                                   const G4VPhysicalVolume *) const {}
};

#endif
