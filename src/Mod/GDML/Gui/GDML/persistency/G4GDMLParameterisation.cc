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
// $Id: G4GDMLParameterisation.cc,v 1.11 2008/07/16 15:46:34 gcosmo Exp $
// GEANT4 tag $Name: geant4-09-03-patch-01 $
//
// class G4GDMLParameterisation Implementation
//
// History:
// - Created.                                  Zoltan Torzsok, November 2007
// -------------------------------------------------------------------------

#include "G4GDMLParameterisation.hh"

/*
G4int G4GDMLParameterisation::GetSize() const
{
   return (G4int)parameterList.size();
}

void G4GDMLParameterisation::AddParameter(const PARAMETER& newParameter)
{
   parameterList.push_back(newParameter);
}

void G4GDMLParameterisation::
ComputeTransformation(const G4int index,G4VPhysicalVolume* physvol) const
{
   physvol->SetTranslation(parameterList[index].position);
   physvol->SetRotation(parameterList[index].pRot);
}

void G4GDMLParameterisation::
ComputeDimensions(G4Box& box,const G4int index,const G4VPhysicalVolume*) const
{
   box.SetXHalfLength(parameterList[index].dimension[0]);
   box.SetYHalfLength(parameterList[index].dimension[1]);
   box.SetZHalfLength(parameterList[index].dimension[2]);
}

void G4GDMLParameterisation::
ComputeDimensions(G4Trd& trd,const G4int index,const G4VPhysicalVolume*) const
{
   trd.SetXHalfLength1(parameterList[index].dimension[0]);
   trd.SetXHalfLength2(parameterList[index].dimension[1]);
   trd.SetYHalfLength1(parameterList[index].dimension[2]);
   trd.SetYHalfLength2(parameterList[index].dimension[3]);
   trd.SetZHalfLength(parameterList[index].dimension[4]);
}

void G4GDMLParameterisation::
ComputeDimensions(G4Trap& trap,const G4int index,const G4VPhysicalVolume*) const
{
   trap.SetAllParameters(parameterList[index].dimension[0], // Dz
                         parameterList[index].dimension[1], // Theta
                         parameterList[index].dimension[2], // Phi
                         parameterList[index].dimension[3], // Dy1
                         parameterList[index].dimension[4], // Dx1
                         parameterList[index].dimension[5], // Dx2
                         parameterList[index].dimension[6], // pAlp1,
                         parameterList[index].dimension[7], // pDy2,
                         parameterList[index].dimension[8], // pDx3,
                         parameterList[index].dimension[9], // pDx4,
                         parameterList[index].dimension[10]); // pAlp2
}

void G4GDMLParameterisation::
ComputeDimensions(G4Tubs& tubs,const G4int index,const G4VPhysicalVolume*) const
{
   tubs.SetInnerRadius(parameterList[index].dimension[0]);
   tubs.SetOuterRadius(parameterList[index].dimension[1]);
   tubs.SetZHalfLength(parameterList[index].dimension[2]);
   tubs.SetStartPhiAngle(parameterList[index].dimension[3]);
   tubs.SetDeltaPhiAngle(parameterList[index].dimension[4]);
}

void G4GDMLParameterisation::
ComputeDimensions(G4Cons& cons,const G4int index,const G4VPhysicalVolume*) const
{
   cons.SetInnerRadiusMinusZ(parameterList[index].dimension[0]);
   cons.SetOuterRadiusMinusZ(parameterList[index].dimension[1]);
   cons.SetInnerRadiusPlusZ(parameterList[index].dimension[2]);
   cons.SetOuterRadiusPlusZ(parameterList[index].dimension[3]);
   cons.SetZHalfLength(parameterList[index].dimension[4]);
   cons.SetStartPhiAngle(parameterList[index].dimension[5]);
   cons.SetDeltaPhiAngle(parameterList[index].dimension[6]);
}

void G4GDMLParameterisation::
ComputeDimensions(G4Sphere& sphere,const G4int index,const G4VPhysicalVolume*) const
{
   sphere.SetInsideRadius(parameterList[index].dimension[0]);
   sphere.SetOuterRadius(parameterList[index].dimension[1]);
   sphere.SetStartPhiAngle(parameterList[index].dimension[2]);
   sphere.SetDeltaPhiAngle(parameterList[index].dimension[3]);
   sphere.SetStartThetaAngle(parameterList[index].dimension[4]);
   sphere.SetDeltaThetaAngle(parameterList[index].dimension[5]);
}

void G4GDMLParameterisation::
ComputeDimensions(G4Orb& orb,const G4int index,const G4VPhysicalVolume*) const
{
   orb.SetRadius(parameterList[index].dimension[0]);
}

void G4GDMLParameterisation::
ComputeDimensions(G4Torus& torus,const G4int index,const G4VPhysicalVolume*) const
{
   torus.SetAllParameters(parameterList[index].dimension[0], // pRmin
                          parameterList[index].dimension[1], // pRmax
			  parameterList[index].dimension[2], // pRtor
                          parameterList[index].dimension[3], // pSPhi
			  parameterList[index].dimension[4]); // pDPhi
}

void G4GDMLParameterisation::
ComputeDimensions(G4Para& para,const G4int index,const G4VPhysicalVolume*) const
{
   para.SetXHalfLength(parameterList[index].dimension[0]);
   para.SetYHalfLength(parameterList[index].dimension[1]);
   para.SetZHalfLength(parameterList[index].dimension[2]);
   para.SetAlpha(parameterList[index].dimension[3]);
   para.SetTanAlpha(std::tan(parameterList[index].dimension[3]));
   para.SetThetaAndPhi(parameterList[index].dimension[4],parameterList[index].dimension[5]);
}

void G4GDMLParameterisation::
ComputeDimensions(G4Hype& hype,const G4int index,const G4VPhysicalVolume*) const
{
   hype.SetInnerRadius(parameterList[index].dimension[0]);
   hype.SetOuterRadius(parameterList[index].dimension[1]);
   hype.SetZHalfLength(parameterList[index].dimension[4]);
   hype.SetInnerStereo(parameterList[index].dimension[2]);
   hype.SetOuterStereo(parameterList[index].dimension[3]);
}

void G4GDMLParameterisation::
ComputeDimensions(G4Polycone&,const G4int,const G4VPhysicalVolume*) const
{
   G4Exception("G4GDMLParameterisation::ComputeDimensions()",
               "InvalidSetup", FatalException,
               "Parameterisation of G4Polycone not implemented yet. Sorry!");
}

void G4GDMLParameterisation::
ComputeDimensions(G4Polyhedra&,const G4int,const G4VPhysicalVolume*) const
{
   G4Exception("G4GDMLParameterisation::ComputeDimensions()",
               "InvalidSetup", FatalException,
               "Parameterisation of G4Polyhedra not implemented yet. Sorry!");
}
*/