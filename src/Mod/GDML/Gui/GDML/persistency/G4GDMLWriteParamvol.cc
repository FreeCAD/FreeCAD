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
// $Id: G4GDMLWriteParamvol.cc,v 1.25 2009/04/24 15:34:20 gcosmo Exp $
// GEANT4 tag $Name: geant4-09-03-patch-01 $
//
// class G4GDMLParamVol Implementation
//
// Original author: Zoltan Torzsok, November 2007
//
// --------------------------------------------------------------------

#include "G4GDMLWriteParamvol.hh"

///#include "G4Box.hh"
#include "../geometry/solid/G4Box.hh"
//#include "G4Trd.hh"
//#include "G4Trap.hh"
///#include "G4Tubs.hh"
#include "../geometry/solid/G4Tubs.hh"
/*#include "G4Cons.hh"
#include "G4Sphere.hh"
#include "G4Orb.hh"
#include "G4Torus.hh"
#include "G4Para.hh"
#include "G4Hype.hh"*/
///#include "G4LogicalVolume.hh"
///#include "G4VPhysicalVolume.hh"
#include "../geometry/management/G4LogicalVolume.hh"
#include "../geometry/management/G4VPhysicalVolume.hh"
//#include "G4PVParameterised.hh"
//
#include "../geometry/management/G4VPVParameterisation.hh"

G4GDMLWriteParamvol::
G4GDMLWriteParamvol() : G4GDMLWriteSetup()
{
}

G4GDMLWriteParamvol::
~G4GDMLWriteParamvol()
{
}

void G4GDMLWriteParamvol::
Box_dimensionsWrite(xercesc::DOMElement* parametersElement,
                    const G4Box* const box)
{
   xercesc::DOMElement* box_dimensionsElement = NewElement("box_dimensions");
   box_dimensionsElement->
     setAttributeNode(NewAttribute("x",2.0*box->GetXHalfLength()/mm));
   box_dimensionsElement->
     setAttributeNode(NewAttribute("y",2.0*box->GetYHalfLength()/mm));
   box_dimensionsElement->
     setAttributeNode(NewAttribute("z",2.0*box->GetZHalfLength()/mm));
   box_dimensionsElement->
     setAttributeNode(NewAttribute("lunit","mm"));
   parametersElement->appendChild(box_dimensionsElement);
}
/*
void G4GDMLWriteParamvol::
Trd_dimensionsWrite(xercesc::DOMElement* parametersElement,
                    const G4Trd* const trd)
{
   xercesc::DOMElement* trd_dimensionsElement = NewElement("trd_dimensions");
   trd_dimensionsElement->
     setAttributeNode(NewAttribute("x1",2.0*trd->GetXHalfLength1()/mm));
   trd_dimensionsElement->
     setAttributeNode(NewAttribute("x2",2.0*trd->GetXHalfLength2()/mm));
   trd_dimensionsElement->
     setAttributeNode(NewAttribute("y1",2.0*trd->GetYHalfLength1()/mm));
   trd_dimensionsElement->
     setAttributeNode(NewAttribute("y2",2.0*trd->GetYHalfLength2()/mm));
   trd_dimensionsElement->
     setAttributeNode(NewAttribute("z",2.0*trd->GetZHalfLength()/mm));
   trd_dimensionsElement->
     setAttributeNode(NewAttribute("lunit","mm"));
   parametersElement->appendChild(trd_dimensionsElement);
}

void G4GDMLWriteParamvol::
Trap_dimensionsWrite(xercesc::DOMElement* parametersElement,
                     const G4Trap* const trap)
{
   const G4ThreeVector simaxis = trap->GetSymAxis();
   const G4double phi = (simaxis.z() != 1.0)
                      ? (std::atan(simaxis.y()/simaxis.x())) : (0.0);
   const G4double theta = std::acos(simaxis.z());
   const G4double alpha1 = std::atan(trap->GetTanAlpha1());
   const G4double alpha2 = std::atan(trap->GetTanAlpha2());

   xercesc::DOMElement* trap_dimensionsElement = NewElement("trap");
   trap_dimensionsElement->
     setAttributeNode(NewAttribute("z",2.0*trap->GetZHalfLength()/mm));
   trap_dimensionsElement->
     setAttributeNode(NewAttribute("theta",theta/degree));
   trap_dimensionsElement->
     setAttributeNode(NewAttribute("phi",phi/degree));
   trap_dimensionsElement->
     setAttributeNode(NewAttribute("y1",2.0*trap->GetYHalfLength1()/mm));
   trap_dimensionsElement->
     setAttributeNode(NewAttribute("x1",2.0*trap->GetXHalfLength1()/mm));
   trap_dimensionsElement->
     setAttributeNode(NewAttribute("x2",2.0*trap->GetXHalfLength2()/mm));
   trap_dimensionsElement->
     setAttributeNode(NewAttribute("alpha1",alpha1/degree));
   trap_dimensionsElement->
     setAttributeNode(NewAttribute("y2",2.0*trap->GetYHalfLength2()/mm));
   trap_dimensionsElement->
     setAttributeNode(NewAttribute("x3",2.0*trap->GetXHalfLength3()/mm));
   trap_dimensionsElement->
     setAttributeNode(NewAttribute("x4",2.0*trap->GetXHalfLength4()/mm));
   trap_dimensionsElement->
     setAttributeNode(NewAttribute("alpha2",alpha2/degree));
   trap_dimensionsElement->
     setAttributeNode(NewAttribute("aunit","deg"));
   trap_dimensionsElement->
     setAttributeNode(NewAttribute("lunit","mm"));
   parametersElement->appendChild(trap_dimensionsElement);
}
*/
void G4GDMLWriteParamvol::
Tube_dimensionsWrite(xercesc::DOMElement* parametersElement,
                     const G4Tubs* const tube)
{
   xercesc::DOMElement* tube_dimensionsElement = NewElement("tube_dimensions");
   tube_dimensionsElement->
     setAttributeNode(NewAttribute("InR",tube->GetInnerRadius()/mm));
   tube_dimensionsElement->
     setAttributeNode(NewAttribute("OutR",tube->GetOuterRadius()/mm));
   tube_dimensionsElement->
     setAttributeNode(NewAttribute("hz",2.0*tube->GetZHalfLength()/mm));
   tube_dimensionsElement->
     setAttributeNode(NewAttribute("StartPhi",tube->GetStartPhiAngle()/degree));
   tube_dimensionsElement->
     setAttributeNode(NewAttribute("DeltaPhi",tube->GetDeltaPhiAngle()/degree));
   tube_dimensionsElement->
     setAttributeNode(NewAttribute("aunit","deg"));
   tube_dimensionsElement->
     setAttributeNode(NewAttribute("lunit","mm"));
   parametersElement->appendChild(tube_dimensionsElement);
}

/*
void G4GDMLWriteParamvol::
Cone_dimensionsWrite(xercesc::DOMElement* parametersElement,
                     const G4Cons* const cone)
{
   xercesc::DOMElement* cone_dimensionsElement = NewElement("cone_dimensions");
   cone_dimensionsElement->
     setAttributeNode(NewAttribute("rmin1",cone->GetInnerRadiusMinusZ()/mm));
   cone_dimensionsElement->
     setAttributeNode(NewAttribute("rmax1",cone->GetOuterRadiusMinusZ()/mm));
   cone_dimensionsElement->
     setAttributeNode(NewAttribute("rmin2",cone->GetInnerRadiusPlusZ()/mm));
   cone_dimensionsElement->
     setAttributeNode(NewAttribute("rmax2",cone->GetOuterRadiusPlusZ()/mm));
   cone_dimensionsElement->
     setAttributeNode(NewAttribute("z",2.0*cone->GetZHalfLength()/mm));
   cone_dimensionsElement->
     setAttributeNode(NewAttribute("startphi",cone->GetStartPhiAngle()/degree));
   cone_dimensionsElement->
     setAttributeNode(NewAttribute("deltaphi",cone->GetDeltaPhiAngle()/degree));
   cone_dimensionsElement->
     setAttributeNode(NewAttribute("aunit","deg"));
   cone_dimensionsElement->
     setAttributeNode(NewAttribute("lunit","mm"));
   parametersElement->appendChild(cone_dimensionsElement);
}

void G4GDMLWriteParamvol::
Sphere_dimensionsWrite(xercesc::DOMElement* parametersElement,
                       const G4Sphere* const sphere)
{
   xercesc::DOMElement* sphere_dimensionsElement =
                        NewElement("sphere_dimensions");
   sphere_dimensionsElement->setAttributeNode(NewAttribute("rmin",
                             sphere->GetInsideRadius()/mm));
   sphere_dimensionsElement->setAttributeNode(NewAttribute("rmax",
                             sphere->GetOuterRadius()/mm));
   sphere_dimensionsElement->setAttributeNode(NewAttribute("startphi",
                             sphere->GetStartPhiAngle()/degree));
   sphere_dimensionsElement->setAttributeNode(NewAttribute("deltaphi",
                             sphere->GetDeltaPhiAngle()/degree));
   sphere_dimensionsElement->setAttributeNode(NewAttribute("starttheta",
                             sphere->GetStartThetaAngle()/degree));
   sphere_dimensionsElement->setAttributeNode(NewAttribute("deltatheta",
                             sphere->GetDeltaThetaAngle()/degree));
   sphere_dimensionsElement->setAttributeNode(NewAttribute("aunit","deg"));
   sphere_dimensionsElement->setAttributeNode(NewAttribute("lunit","mm"));
   parametersElement->appendChild(sphere_dimensionsElement);
}

void G4GDMLWriteParamvol::
Orb_dimensionsWrite(xercesc::DOMElement* parametersElement,
                    const G4Orb* const orb)
{
   xercesc::DOMElement* orb_dimensionsElement = NewElement("orb_dimensions");
   orb_dimensionsElement->setAttributeNode(NewAttribute("r",
                          orb->GetRadius()/mm));
   orb_dimensionsElement->setAttributeNode(NewAttribute("lunit","mm"));
   parametersElement->appendChild(orb_dimensionsElement);
}

void G4GDMLWriteParamvol::
Torus_dimensionsWrite(xercesc::DOMElement* parametersElement,
                      const G4Torus* const torus)
{
   xercesc::DOMElement* torus_dimensionsElement =
                        NewElement("torus_dimensions");
   torus_dimensionsElement->
     setAttributeNode(NewAttribute("rmin",torus->GetRmin()/mm));
   torus_dimensionsElement->
     setAttributeNode(NewAttribute("rmax",torus->GetRmax()/mm));
   torus_dimensionsElement->
     setAttributeNode(NewAttribute("rtor",torus->GetRtor()/mm));
   torus_dimensionsElement->
     setAttributeNode(NewAttribute("startphi",torus->GetSPhi()/degree));
   torus_dimensionsElement->
     setAttributeNode(NewAttribute("deltaphi",torus->GetDPhi()/degree));
   torus_dimensionsElement->
     setAttributeNode(NewAttribute("aunit","deg"));
   torus_dimensionsElement->
     setAttributeNode(NewAttribute("lunit","mm"));
   parametersElement->appendChild(torus_dimensionsElement);
}

void G4GDMLWriteParamvol::
Para_dimensionsWrite(xercesc::DOMElement* parametersElement,
                     const G4Para* const para)
{
   const G4ThreeVector simaxis = para->GetSymAxis();
   const G4double alpha = std::atan(para->GetTanAlpha());
   const G4double theta = std::acos(simaxis.z());
   const G4double phi = (simaxis.z() != 1.0)
                      ? (std::atan(simaxis.y()/simaxis.x())) : (0.0);

   xercesc::DOMElement* para_dimensionsElement = NewElement("para_dimensions");
   para_dimensionsElement->
     setAttributeNode(NewAttribute("x",2.0*para->GetXHalfLength()/mm));
   para_dimensionsElement->
     setAttributeNode(NewAttribute("y",2.0*para->GetYHalfLength()/mm));
   para_dimensionsElement->
     setAttributeNode(NewAttribute("z",2.0*para->GetZHalfLength()/mm));
   para_dimensionsElement->
     setAttributeNode(NewAttribute("alpha",alpha/degree));
   para_dimensionsElement->
     setAttributeNode(NewAttribute("theta",theta/degree));
   para_dimensionsElement->
     setAttributeNode(NewAttribute("phi",phi/degree));
   para_dimensionsElement->
     setAttributeNode(NewAttribute("aunit","deg"));
   para_dimensionsElement->
     setAttributeNode(NewAttribute("lunit","mm"));
   parametersElement->appendChild(para_dimensionsElement);
}

void G4GDMLWriteParamvol::
Hype_dimensionsWrite(xercesc::DOMElement* parametersElement,
                     const G4Hype* const hype)
{
   xercesc::DOMElement* hype_dimensionsElement = NewElement("hype_dimensions");
   hype_dimensionsElement->
     setAttributeNode(NewAttribute("rmin",hype->GetInnerRadius()/mm));
   hype_dimensionsElement->
     setAttributeNode(NewAttribute("rmax",hype->GetOuterRadius()/mm));
   hype_dimensionsElement->
     setAttributeNode(NewAttribute("inst",hype->GetInnerStereo()/degree));
   hype_dimensionsElement->
     setAttributeNode(NewAttribute("outst",hype->GetOuterStereo()/degree));
   hype_dimensionsElement->
     setAttributeNode(NewAttribute("z",2.0*hype->GetZHalfLength()/mm));
   hype_dimensionsElement->
     setAttributeNode(NewAttribute("aunit","deg"));
   hype_dimensionsElement->
     setAttributeNode(NewAttribute("lunit","mm"));
   parametersElement->appendChild(hype_dimensionsElement);
}
*/
/**/
void G4GDMLWriteParamvol::
ParametersWrite(xercesc::DOMElement* paramvolElement,
                const G4VPhysicalVolume* const paramvol,const G4int& index)
{
   paramvol->GetParameterisation()
     ->ComputeTransformation(index, const_cast<G4VPhysicalVolume*>(paramvol));
   G4ThreeVector Angles;
   G4String name = GenerateName(paramvol->GetName(),paramvol);
   std::stringstream os; 
   os.precision(15);
   os << index;     
   G4String sncopie = os.str(); 

   xercesc::DOMElement* parametersElement = NewElement("parameters");
   parametersElement->setAttributeNode(NewAttribute("number",index+1));

   PositionWrite(parametersElement, name+sncopie+"_pos",
                 paramvol->GetObjectTranslation());
   Angles=GetAngles(paramvol->GetObjectRotationValue());
   if (Angles.mag2()>DBL_EPSILON)
   {
     RotationWrite(parametersElement, name+sncopie+"_rot",
                   GetAngles(paramvol->GetObjectRotationValue()));
   }
   paramvolElement->appendChild(parametersElement);

   G4VSolid* solid = paramvol->GetLogicalVolume()->GetSolid();

   if (G4Box* box = dynamic_cast<G4Box*>(solid))
   {
      paramvol->GetParameterisation()->ComputeDimensions(*box,index,
                const_cast<G4VPhysicalVolume*>(paramvol));
      Box_dimensionsWrite(parametersElement,box);
   } /*else
   if (G4Trd* trd = dynamic_cast<G4Trd*>(solid))
   {
      paramvol->GetParameterisation()->ComputeDimensions(*trd,index,
                const_cast<G4VPhysicalVolume*>(paramvol));
      Trd_dimensionsWrite(parametersElement,trd);
   } else
   if (G4Trap* trap = dynamic_cast<G4Trap*>(solid))
   {
      paramvol->GetParameterisation()->ComputeDimensions(*trap,index,
                const_cast<G4VPhysicalVolume*>(paramvol));
      Trap_dimensionsWrite(parametersElement,trap);
   }*/ else
   if (G4Tubs* tube = dynamic_cast<G4Tubs*>(solid))
   {
      paramvol->GetParameterisation()->ComputeDimensions(*tube,index,
                const_cast<G4VPhysicalVolume*>(paramvol));
      Tube_dimensionsWrite(parametersElement,tube);
   }/* else
   if (G4Cons* cone = dynamic_cast<G4Cons*>(solid))
   {
      paramvol->GetParameterisation()->ComputeDimensions(*cone,index,
                const_cast<G4VPhysicalVolume*>(paramvol));
      Cone_dimensionsWrite(parametersElement,cone);
   } else
   if (G4Sphere* sphere = dynamic_cast<G4Sphere*>(solid))
   {
      paramvol->GetParameterisation()->ComputeDimensions(*sphere,index,
                const_cast<G4VPhysicalVolume*>(paramvol));
      Sphere_dimensionsWrite(parametersElement,sphere);
   } else
   if (G4Orb* orb = dynamic_cast<G4Orb*>(solid))
   {
      paramvol->GetParameterisation()->ComputeDimensions(*orb,index,
                const_cast<G4VPhysicalVolume*>(paramvol));
      Orb_dimensionsWrite(parametersElement,orb);
   } else
   if (G4Torus* torus = dynamic_cast<G4Torus*>(solid))
   {
      paramvol->GetParameterisation()->ComputeDimensions(*torus,index,
                const_cast<G4VPhysicalVolume*>(paramvol));
      Torus_dimensionsWrite(parametersElement,torus);
   } else
   if (G4Para* para = dynamic_cast<G4Para*>(solid))
   {
      paramvol->GetParameterisation()->ComputeDimensions(*para,index,
                const_cast<G4VPhysicalVolume*>(paramvol));
      Para_dimensionsWrite(parametersElement,para);
   } else
   if (G4Hype* hype = dynamic_cast<G4Hype*>(solid))
   {
      paramvol->GetParameterisation()->ComputeDimensions(*hype,index,
                const_cast<G4VPhysicalVolume*>(paramvol));
      Hype_dimensionsWrite(parametersElement,hype);
   }*/
   else
   {
/*     G4String error_msg = "Solid '" + solid->GetName()
                        + "' cannot be used in parameterised volume!";
     G4Exception("G4GDMLWriteParamvol::ParametersWrite()",
                 "InvalidSetup", FatalException, error_msg);
*/   }
}
/**/
void G4GDMLWriteParamvol::
ParamvolWrite(xercesc::DOMElement* volumeElement,
              const G4VPhysicalVolume* const paramvol)
{/**/
   const G4String volumeref =
                  GenerateName(paramvol->GetLogicalVolume()->GetName(),
                               paramvol->GetLogicalVolume());
   xercesc::DOMElement* paramvolElement = NewElement("paramvol");
   paramvolElement->setAttributeNode(NewAttribute("ncopies",
                                     paramvol->GetMultiplicity()));
   xercesc::DOMElement* volumerefElement = NewElement("volumeref");
   volumerefElement->setAttributeNode(NewAttribute("ref",volumeref));

   xercesc::DOMElement* algorithmElement =
     NewElement("parameterised_position_size");
   paramvolElement->appendChild(volumerefElement);
   paramvolElement->appendChild(algorithmElement);
   ParamvolAlgorithmWrite(algorithmElement,paramvol);
   volumeElement->appendChild(paramvolElement);/**/
}

void G4GDMLWriteParamvol::
ParamvolAlgorithmWrite(xercesc::DOMElement* paramvolElement,
                       const G4VPhysicalVolume* const paramvol)
{/**/
   const G4String volumeref =
                  GenerateName(paramvol->GetLogicalVolume()->GetName(),
                               paramvol->GetLogicalVolume());
  
   const G4int parameterCount = paramvol->GetMultiplicity();

   for (G4int i=0; i<parameterCount; i++)
   {
     ParametersWrite(paramvolElement,paramvol,i);
   }/**/
}
