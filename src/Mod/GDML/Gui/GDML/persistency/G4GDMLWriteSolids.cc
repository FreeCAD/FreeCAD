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
// $Id: G4GDMLWriteSolids.cc,v 1.65 2009/04/24 15:34:20 gcosmo Exp $
// GEANT4 tag $Name: geant4-09-03-patch-01 $
//
// class G4GDMLWriteSolids Implementation
//
// Original author: Zoltan Torzsok, November 2007
//
// --------------------------------------------------------------------

#include "G4GDMLWriteSolids.hh"

//#include "G4BooleanSolid.hh"
///#include "G4Box.hh"
#include "../geometry/solid/G4Box.hh"
/*#include "G4Cons.hh"
#include "G4Ellipsoid.hh"
#include "G4EllipticalCone.hh"
#include "G4EllipticalTube.hh"
#include "G4ExtrudedSolid.hh"
#include "G4Hype.hh"
#include "G4Orb.hh"
#include "G4Para.hh"
#include "G4Paraboloid.hh"
#include "G4IntersectionSolid.hh"
#include "G4Polycone.hh"
#include "G4Polyhedra.hh"
#include "G4ReflectedSolid.hh"
#include "G4Sphere.hh"
#include "G4SubtractionSolid.hh"
#include "G4TessellatedSolid.hh"
#include "G4Tet.hh"
#include "G4Torus.hh"
#include "G4Trap.hh"
#include "G4Trd.hh"*/
///#include "G4Tubs.hh"
#include "../geometry/solid/G4Tubs.hh"
/*#include "G4TwistedBox.hh"
#include "G4TwistedTrap.hh"
#include "G4TwistedTrd.hh"
#include "G4TwistedTubs.hh"
#include "G4UnionSolid.hh"
#include "G4OpticalSurface.hh"
#include "G4SurfaceProperty.hh"*/

G4GDMLWriteSolids::
G4GDMLWriteSolids() : G4GDMLWriteMaterials()
{
}

G4GDMLWriteSolids::
~G4GDMLWriteSolids()
{
}
/*
void G4GDMLWriteSolids::
BooleanWrite(xercesc::DOMElement* solidsElement,
             const G4BooleanSolid* const boolean)
{
   G4int displaced=0;

   G4String tag("undefined");
   if (dynamic_cast<const G4IntersectionSolid*>(boolean))
     { tag = "intersection"; } else
   if (dynamic_cast<const G4SubtractionSolid*>(boolean))
     { tag = "subtraction"; } else
   if (dynamic_cast<const G4UnionSolid*>(boolean))
     { tag = "union"; }
   
   G4VSolid* firstPtr = const_cast<G4VSolid*>(boolean->GetConstituentSolid(0));
   G4VSolid* secondPtr = const_cast<G4VSolid*>(boolean->GetConstituentSolid(1));
   
   G4ThreeVector firstpos,firstrot,pos,rot;

   // Solve possible displacement of referenced solids!
   //
   while (true)
   {
      if ( displaced>8 )
      {
        G4String ErrorMessage = "The referenced solid '"
                              + firstPtr->GetName() +
                              + "in the Boolean shape '" +
                              + boolean->GetName() +
                              + "' was displaced too many times!";
        G4Exception("G4GDMLWriteSolids::BooleanWrite()",
                    "InvalidSetup", FatalException, ErrorMessage);
      }

      if (G4DisplacedSolid* disp = dynamic_cast<G4DisplacedSolid*>(firstPtr))
      {
         firstpos += disp->GetObjectTranslation();
         firstrot += firstrot + GetAngles(disp->GetObjectRotation());
         firstPtr = disp->GetConstituentMovedSolid();
         displaced++;
         continue;
      }
      break;
   }
   displaced = 0;
   while (true)
   {
      if ( displaced>maxTransforms )
      {
        G4String ErrorMessage = "The referenced solid '"
                              + secondPtr->GetName() +
                              + "in the Boolean shape '" +
                              + boolean->GetName() +
                              + "' was displaced too many times!";
        G4Exception("G4GDMLWriteSolids::BooleanWrite()",
                    "InvalidSetup", FatalException, ErrorMessage);
      }

      if (G4DisplacedSolid* disp = dynamic_cast<G4DisplacedSolid*>(secondPtr))
      {
         pos += disp->GetObjectTranslation();
         rot += GetAngles(disp->GetObjectRotation());
         secondPtr = disp->GetConstituentMovedSolid();
         displaced++;
         continue;
      }
      break;
   }

   AddSolid(firstPtr);   // At first add the constituent solids!
   AddSolid(secondPtr);

   const G4String& name = GenerateName(boolean->GetName(),boolean);
   const G4String& firstref = GenerateName(firstPtr->GetName(),firstPtr);
   const G4String& secondref = GenerateName(secondPtr->GetName(),secondPtr);

   xercesc::DOMElement* booleanElement = NewElement(tag);
   booleanElement->setAttributeNode(NewAttribute("name",name));
   xercesc::DOMElement* firstElement = NewElement("first");
   firstElement->setAttributeNode(NewAttribute("ref",firstref));
   booleanElement->appendChild(firstElement);
   xercesc::DOMElement* secondElement = NewElement("second");
   secondElement->setAttributeNode(NewAttribute("ref",secondref));
   booleanElement->appendChild(secondElement);
   solidsElement->appendChild(booleanElement);
     // Add the boolean solid AFTER the constituent solids!

   if ( (std::fabs(pos.x()) > kLinearPrecision)
     || (std::fabs(pos.y()) > kLinearPrecision)
     || (std::fabs(pos.z()) > kLinearPrecision) )
   {
     PositionWrite(booleanElement,name+"_pos",pos);
   }

   if ( (std::fabs(rot.x()) > kAngularPrecision)
     || (std::fabs(rot.y()) > kAngularPrecision)
     || (std::fabs(rot.z()) > kAngularPrecision) )
   {
     RotationWrite(booleanElement,name+"_rot",rot);
   }

   if ( (std::fabs(firstpos.x()) > kLinearPrecision)
     || (std::fabs(firstpos.y()) > kLinearPrecision)
     || (std::fabs(firstpos.z()) > kLinearPrecision) )
   {
     FirstpositionWrite(booleanElement,name+"_fpos",firstpos);
   }

   if ( (std::fabs(firstrot.x()) > kAngularPrecision)
     || (std::fabs(firstrot.y()) > kAngularPrecision)
     || (std::fabs(firstrot.z()) > kAngularPrecision) )
   {
     FirstrotationWrite(booleanElement,name+"_frot",firstrot);
   }
}
*/
void G4GDMLWriteSolids::
BoxWrite(xercesc::DOMElement* solidsElement, const G4Box* const box)
{
   const G4String& name = GenerateName(box->GetName(),box);

   xercesc::DOMElement* boxElement = NewElement("box");
   boxElement->setAttributeNode(NewAttribute("name",name));
   boxElement->setAttributeNode(NewAttribute("x",2.0*box->GetXHalfLength()/mm));
   boxElement->setAttributeNode(NewAttribute("y",2.0*box->GetYHalfLength()/mm));
   boxElement->setAttributeNode(NewAttribute("z",2.0*box->GetZHalfLength()/mm));
   boxElement->setAttributeNode(NewAttribute("lunit","mm"));
   solidsElement->appendChild(boxElement);
}
/*
void G4GDMLWriteSolids::
ConeWrite(xercesc::DOMElement* solidsElement, const G4Cons* const cone)
{
   const G4String& name = GenerateName(cone->GetName(),cone);

   xercesc::DOMElement* coneElement = NewElement("cone");
   coneElement->setAttributeNode(NewAttribute("name",name));
   coneElement->
     setAttributeNode(NewAttribute("rmin1",cone->GetInnerRadiusMinusZ()/mm));
   coneElement->
     setAttributeNode(NewAttribute("rmax1",cone->GetOuterRadiusMinusZ()/mm));
   coneElement->
     setAttributeNode(NewAttribute("rmin2",cone->GetInnerRadiusPlusZ()/mm));
   coneElement->
     setAttributeNode(NewAttribute("rmax2",cone->GetOuterRadiusPlusZ()/mm));
   coneElement->
     setAttributeNode(NewAttribute("z",2.0*cone->GetZHalfLength()/mm));
   coneElement->
     setAttributeNode(NewAttribute("startphi",cone->GetStartPhiAngle()/degree));
   coneElement->
     setAttributeNode(NewAttribute("deltaphi",cone->GetDeltaPhiAngle()/degree));
   coneElement->setAttributeNode(NewAttribute("aunit","deg"));
   coneElement->setAttributeNode(NewAttribute("lunit","mm"));
   solidsElement->appendChild(coneElement);
}

void G4GDMLWriteSolids::
ElconeWrite(xercesc::DOMElement* solidsElement,
            const G4EllipticalCone* const elcone)
{
   const G4String& name = GenerateName(elcone->GetName(),elcone);

   xercesc::DOMElement* elconeElement = NewElement("elcone");
   elconeElement->setAttributeNode(NewAttribute("name",name));
   elconeElement->setAttributeNode(NewAttribute("dx",elcone->GetSemiAxisX()/mm));
   elconeElement->setAttributeNode(NewAttribute("dy",elcone->GetSemiAxisY()/mm));
   elconeElement->setAttributeNode(NewAttribute("zmax",elcone->GetZMax()/mm));
   elconeElement->setAttributeNode(NewAttribute("zcut",elcone->GetZTopCut()/mm));
   elconeElement->setAttributeNode(NewAttribute("lunit","mm"));
   solidsElement->appendChild(elconeElement);
}

void G4GDMLWriteSolids::
EllipsoidWrite(xercesc::DOMElement* solidsElement,
               const G4Ellipsoid* const ellipsoid)
{
   const G4String& name = GenerateName(ellipsoid->GetName(),ellipsoid);

   xercesc::DOMElement* ellipsoidElement = NewElement("ellipsoid");
   ellipsoidElement->setAttributeNode(NewAttribute("name",name));
   ellipsoidElement->
     setAttributeNode(NewAttribute("ax",ellipsoid->GetSemiAxisMax(0)/mm));
   ellipsoidElement->
     setAttributeNode(NewAttribute("by",ellipsoid->GetSemiAxisMax(1)/mm));
   ellipsoidElement->
     setAttributeNode(NewAttribute("cz",ellipsoid->GetSemiAxisMax(2)/mm));
   ellipsoidElement->
     setAttributeNode(NewAttribute("zcut1",ellipsoid->GetZBottomCut()/mm));
   ellipsoidElement->
     setAttributeNode(NewAttribute("zcut2",ellipsoid->GetZTopCut()/mm));
   ellipsoidElement->
     setAttributeNode(NewAttribute("lunit","mm"));
   solidsElement->appendChild(ellipsoidElement);
}

void G4GDMLWriteSolids::
EltubeWrite(xercesc::DOMElement* solidsElement,
            const G4EllipticalTube* const eltube)
{
   const G4String& name = GenerateName(eltube->GetName(),eltube);

   xercesc::DOMElement* eltubeElement = NewElement("eltube");
   eltubeElement->setAttributeNode(NewAttribute("name",name));
   eltubeElement->setAttributeNode(NewAttribute("dx",eltube->GetDx()/mm));
   eltubeElement->setAttributeNode(NewAttribute("dy",eltube->GetDy()/mm));
   eltubeElement->setAttributeNode(NewAttribute("dz",eltube->GetDz()/mm));
   eltubeElement->setAttributeNode(NewAttribute("lunit","mm"));
   solidsElement->appendChild(eltubeElement);
}

void G4GDMLWriteSolids::
XtruWrite(xercesc::DOMElement* solidsElement,
          const G4ExtrudedSolid* const xtru)
{
   const G4String& name = GenerateName(xtru->GetName(),xtru);

   xercesc::DOMElement* xtruElement = NewElement("xtru");
   xtruElement->setAttributeNode(NewAttribute("name",name));
   xtruElement->setAttributeNode(NewAttribute("lunit","mm"));
   solidsElement->appendChild(xtruElement);

   const G4int NumVertex = xtru->GetNofVertices();
   
   for (G4int i=0;i<NumVertex;i++)
   {
      xercesc::DOMElement* twoDimVertexElement = NewElement("twoDimVertex");
      xtruElement->appendChild(twoDimVertexElement);

      const G4TwoVector& vertex = xtru->GetVertex(i);

      twoDimVertexElement->setAttributeNode(NewAttribute("x",vertex.x()/mm));
      twoDimVertexElement->setAttributeNode(NewAttribute("y",vertex.y()/mm));
   }

   const G4int NumSection = xtru->GetNofZSections();

   for (G4int i=0;i<NumSection;i++)
   {
      xercesc::DOMElement* sectionElement = NewElement("section");
      xtruElement->appendChild(sectionElement);

      const G4ExtrudedSolid::ZSection section = xtru->GetZSection(i);

      sectionElement->setAttributeNode(NewAttribute("zOrder",i));
      sectionElement->setAttributeNode(NewAttribute("zPosition",section.fZ/mm));
      sectionElement->
        setAttributeNode(NewAttribute("xOffset",section.fOffset.x()/mm));
      sectionElement->
        setAttributeNode(NewAttribute("yOffset",section.fOffset.y()/mm));
      sectionElement->
        setAttributeNode(NewAttribute("scalingFactor",section.fScale));
   }
}

void G4GDMLWriteSolids::
HypeWrite(xercesc::DOMElement* solidsElement, const G4Hype* const hype)
{
   const G4String& name = GenerateName(hype->GetName(),hype);

   xercesc::DOMElement* hypeElement = NewElement("hype");
   hypeElement->setAttributeNode(NewAttribute("name",name));
   hypeElement->setAttributeNode(NewAttribute("rmin",
                hype->GetInnerRadius()/mm));
   hypeElement->setAttributeNode(NewAttribute("rmax",
                hype->GetOuterRadius()/mm));
   hypeElement->setAttributeNode(NewAttribute("inst",
                hype->GetInnerStereo()/degree));
   hypeElement->setAttributeNode(NewAttribute("outst",
                hype->GetOuterStereo()/degree));
   hypeElement->setAttributeNode(NewAttribute("z",
                2.0*hype->GetZHalfLength()/mm));
   hypeElement->setAttributeNode(NewAttribute("aunit","deg"));
   hypeElement->setAttributeNode(NewAttribute("lunit","mm"));
   solidsElement->appendChild(hypeElement);
}

void G4GDMLWriteSolids::
OrbWrite(xercesc::DOMElement* solidsElement, const G4Orb* const orb)
{
   const G4String& name = GenerateName(orb->GetName(),orb);

   xercesc::DOMElement* orbElement = NewElement("orb");
   orbElement->setAttributeNode(NewAttribute("name",name));
   orbElement->setAttributeNode(NewAttribute("r",orb->GetRadius()/mm));
   orbElement->setAttributeNode(NewAttribute("lunit","mm"));
   solidsElement->appendChild(orbElement);
}

void G4GDMLWriteSolids::
ParaWrite(xercesc::DOMElement* solidsElement, const G4Para* const para)
{
   const G4String& name = GenerateName(para->GetName(),para);

   const G4ThreeVector simaxis = para->GetSymAxis();
   const G4double alpha = std::atan(para->GetTanAlpha());
   const G4double theta = std::acos(simaxis.z());
   const G4double phi = (simaxis.z() != 1.0)
                      ? (std::atan(simaxis.y()/simaxis.x())) : (0.0);

   xercesc::DOMElement* paraElement = NewElement("para");
   paraElement->setAttributeNode(NewAttribute("name",name));
   paraElement->setAttributeNode(NewAttribute("x",
                2.0*para->GetXHalfLength()/mm));
   paraElement->setAttributeNode(NewAttribute("y",
                2.0*para->GetYHalfLength()/mm));
   paraElement->setAttributeNode(NewAttribute("z",
                2.0*para->GetZHalfLength()/mm));
   paraElement->setAttributeNode(NewAttribute("alpha",alpha/degree));
   paraElement->setAttributeNode(NewAttribute("theta",theta/degree));
   paraElement->setAttributeNode(NewAttribute("phi",phi/degree));
   paraElement->setAttributeNode(NewAttribute("aunit","deg"));
   paraElement->setAttributeNode(NewAttribute("lunit","mm"));
   solidsElement->appendChild(paraElement);
}

void G4GDMLWriteSolids::
ParaboloidWrite(xercesc::DOMElement* solidsElement,
                const G4Paraboloid* const paraboloid)
{
   const G4String& name = GenerateName(paraboloid->GetName(),paraboloid);

   xercesc::DOMElement* paraboloidElement = NewElement("paraboloid");
   paraboloidElement->setAttributeNode(NewAttribute("name",name));
   paraboloidElement->setAttributeNode(NewAttribute("rlo",
                      paraboloid->GetRadiusMinusZ()/mm));
   paraboloidElement->setAttributeNode(NewAttribute("rhi",
                      paraboloid->GetRadiusPlusZ()/mm));
   paraboloidElement->setAttributeNode(NewAttribute("dz",
                      paraboloid->GetZHalfLength()/mm));
   paraboloidElement->setAttributeNode(NewAttribute("lunit","mm"));
   solidsElement->appendChild(paraboloidElement);
}

void G4GDMLWriteSolids::
PolyconeWrite(xercesc::DOMElement* solidsElement,
              const G4Polycone* const polycone)
{
   const G4String& name = GenerateName(polycone->GetName(),polycone);

   xercesc::DOMElement* polyconeElement = NewElement("polycone");
   polyconeElement->setAttributeNode(NewAttribute("name",name));
   polyconeElement->setAttributeNode(NewAttribute("startphi",
                    polycone->GetOriginalParameters()->Start_angle/degree));
   polyconeElement->setAttributeNode(NewAttribute("deltaphi",
                    polycone->GetOriginalParameters()->Opening_angle/degree));
   polyconeElement->setAttributeNode(NewAttribute("aunit","deg"));
   polyconeElement->setAttributeNode(NewAttribute("lunit","mm"));
   solidsElement->appendChild(polyconeElement);

   const size_t num_zplanes = polycone->GetOriginalParameters()->Num_z_planes;
   const G4double* z_array = polycone->GetOriginalParameters()->Z_values;
   const G4double* rmin_array = polycone->GetOriginalParameters()->Rmin;
   const G4double* rmax_array = polycone->GetOriginalParameters()->Rmax;

   for (size_t i=0; i<num_zplanes; i++)
   {
      ZplaneWrite(polyconeElement,z_array[i],rmin_array[i],rmax_array[i]);
   }
}

void G4GDMLWriteSolids::
PolyhedraWrite(xercesc::DOMElement* solidsElement,
               const G4Polyhedra* const polyhedra)
{
   const G4String& name = GenerateName(polyhedra->GetName(),polyhedra);

   xercesc::DOMElement* polyhedraElement = NewElement("polyhedra");
   polyhedraElement->setAttributeNode(NewAttribute("name",name));
   polyhedraElement->setAttributeNode(NewAttribute("startphi",
                     polyhedra->GetOriginalParameters()->Start_angle/degree));
   polyhedraElement->setAttributeNode(NewAttribute("deltaphi",
                     polyhedra->GetOriginalParameters()->Opening_angle/degree));
   polyhedraElement->setAttributeNode(NewAttribute("numsides",
                     polyhedra->GetOriginalParameters()->numSide));
   polyhedraElement->setAttributeNode(NewAttribute("aunit","deg"));
   polyhedraElement->setAttributeNode(NewAttribute("lunit","mm"));
   solidsElement->appendChild(polyhedraElement);

   const size_t num_zplanes = polyhedra->GetOriginalParameters()->Num_z_planes;
   const G4double* z_array = polyhedra->GetOriginalParameters()->Z_values;
   const G4double* rmin_array = polyhedra->GetOriginalParameters()->Rmin;
   const G4double* rmax_array = polyhedra->GetOriginalParameters()->Rmax;

   const G4double convertRad =
         std::cos(0.5*polyhedra->GetOriginalParameters()->Opening_angle
       / polyhedra->GetOriginalParameters()->numSide);

   for (size_t i=0;i<num_zplanes;i++)
   {
      ZplaneWrite(polyhedraElement,z_array[i],
                  rmin_array[i]*convertRad, rmax_array[i]*convertRad);
   }
}

void G4GDMLWriteSolids::
SphereWrite(xercesc::DOMElement* solidsElement, const G4Sphere* const sphere)
{
   const G4String& name = GenerateName(sphere->GetName(),sphere);

   xercesc::DOMElement* sphereElement = NewElement("sphere");
   sphereElement->setAttributeNode(NewAttribute("name",name));
   sphereElement->setAttributeNode(NewAttribute("rmin",
                  sphere->GetInsideRadius()/mm));
   sphereElement->setAttributeNode(NewAttribute("rmax",
                  sphere->GetOuterRadius()/mm));
   sphereElement->setAttributeNode(NewAttribute("startphi",
                  sphere->GetStartPhiAngle()/degree));
   sphereElement->setAttributeNode(NewAttribute("deltaphi",
                  sphere->GetDeltaPhiAngle()/degree));
   sphereElement->setAttributeNode(NewAttribute("starttheta",
                  sphere->GetStartThetaAngle()/degree));
   sphereElement->setAttributeNode(NewAttribute("deltatheta",
                  sphere->GetDeltaThetaAngle()/degree));
   sphereElement->setAttributeNode(NewAttribute("aunit","deg"));
   sphereElement->setAttributeNode(NewAttribute("lunit","mm"));
   solidsElement->appendChild(sphereElement);
}

void G4GDMLWriteSolids::
TessellatedWrite(xercesc::DOMElement* solidsElement,
                 const G4TessellatedSolid* const tessellated)
{
   const G4String& solid_name = tessellated->GetName();
   const G4String& name = GenerateName(solid_name, tessellated);

   xercesc::DOMElement* tessellatedElement = NewElement("tessellated");
   tessellatedElement->setAttributeNode(NewAttribute("name",name));
   tessellatedElement->setAttributeNode(NewAttribute("aunit","deg"));
   tessellatedElement->setAttributeNode(NewAttribute("lunit","mm"));
   solidsElement->appendChild(tessellatedElement);

   std::map<G4ThreeVector, G4String> vertexMap;

   const size_t NumFacets = tessellated->GetNumberOfFacets();
   size_t NumVertex = 0;
   
   for (size_t i=0;i<NumFacets;i++)
   {
      const G4VFacet* facet = tessellated->GetFacet(i);
      const size_t NumVertexPerFacet = facet->GetNumberOfVertices();

      G4String FacetTag;
      
      if (NumVertexPerFacet==3) { FacetTag="triangular"; } else
      if (NumVertexPerFacet==4) { FacetTag="quadrangular"; }
      else
      {
        G4Exception("G4GDMLWriteSolids::TessellatedWrite()", "InvalidSetup",
                    FatalException, "Facet should contain 3 or 4 vertices!");
      }

      xercesc::DOMElement* facetElement = NewElement(FacetTag);
      tessellatedElement->appendChild(facetElement);

      for (size_t j=0; j<NumVertexPerFacet; j++)
      {
         std::stringstream name_stream;
         std::stringstream ref_stream;

         name_stream << "vertex" << (j+1);
         ref_stream << solid_name << "_v" << NumVertex;

         const G4String& name = name_stream.str();  // facet's tag variable
         G4String ref = ref_stream.str();     // vertex tag to be associated

         // Now search for the existance of the current vertex in the
         // map of cached vertices. If existing, do NOT store it as
         // position in the GDML file, so avoiding duplication; otherwise
         // cache it in the local map and add it as position in the
         // "define" section of the GDML file.

         const G4ThreeVector& vertex = facet->GetVertex(j);

         if(vertexMap.find(vertex) != vertexMap.end())  // Vertex is cached
         {
           ref = vertexMap[vertex];     // Set the proper tag for it
         }
         else                                           // Vertex not found
         {
           vertexMap.insert(std::make_pair(vertex,ref)); // Cache vertex and ...
           AddPosition(ref, vertex);    // ... add it to define section!
           NumVertex++;
         }

         // Now create association of the vertex with its facet
         //
         facetElement->setAttributeNode(NewAttribute(name,ref));
      }
   }
}

void G4GDMLWriteSolids::
TetWrite(xercesc::DOMElement* solidsElement, const G4Tet* const tet)
{
   const G4String& solid_name = tet->GetName();
   const G4String& name = GenerateName(solid_name, tet);

   std::vector<G4ThreeVector> vertexList = tet->GetVertices();

   xercesc::DOMElement* tetElement = NewElement("tet");
   tetElement->setAttributeNode(NewAttribute("name",name));
   tetElement->setAttributeNode(NewAttribute("vertex1",solid_name+"_v1"));
   tetElement->setAttributeNode(NewAttribute("vertex2",solid_name+"_v2"));
   tetElement->setAttributeNode(NewAttribute("vertex3",solid_name+"_v3"));
   tetElement->setAttributeNode(NewAttribute("vertex4",solid_name+"_v4"));
   tetElement->setAttributeNode(NewAttribute("lunit","mm"));
   solidsElement->appendChild(tetElement);

   AddPosition(solid_name+"_v1",vertexList[0]);
   AddPosition(solid_name+"_v2",vertexList[1]);
   AddPosition(solid_name+"_v3",vertexList[2]);
   AddPosition(solid_name+"_v4",vertexList[3]);
}

void G4GDMLWriteSolids::
TorusWrite(xercesc::DOMElement* solidsElement, const G4Torus* const torus)
{
   const G4String& name = GenerateName(torus->GetName(),torus);

   xercesc::DOMElement* torusElement = NewElement("torus");
   torusElement->setAttributeNode(NewAttribute("name",name));
   torusElement->setAttributeNode(NewAttribute("rmin",torus->GetRmin()/mm));
   torusElement->setAttributeNode(NewAttribute("rmax",torus->GetRmax()/mm));
   torusElement->setAttributeNode(NewAttribute("rtor",torus->GetRtor()/mm));
   torusElement->
     setAttributeNode(NewAttribute("startphi",torus->GetSPhi()/degree));
   torusElement->
     setAttributeNode(NewAttribute("deltaphi",torus->GetDPhi()/degree));
   torusElement->setAttributeNode(NewAttribute("aunit","deg"));
   torusElement->setAttributeNode(NewAttribute("lunit","mm"));
   solidsElement->appendChild(torusElement);
}

void G4GDMLWriteSolids::
TrapWrite(xercesc::DOMElement* solidsElement, const G4Trap* const trap)
{
   const G4String& name = GenerateName(trap->GetName(),trap);

   const G4ThreeVector& simaxis = trap->GetSymAxis();
   const G4double phi = (simaxis.z() != 1.0)
                      ? (std::atan(simaxis.y()/simaxis.x())) : (0.0);
   const G4double theta = std::acos(simaxis.z());
   const G4double alpha1 = std::atan(trap->GetTanAlpha1());
   const G4double alpha2 = std::atan(trap->GetTanAlpha2());

   xercesc::DOMElement* trapElement = NewElement("trap");
   trapElement->setAttributeNode(NewAttribute("name",name));
   trapElement->setAttributeNode(NewAttribute("z",
                2.0*trap->GetZHalfLength()/mm));
   trapElement->setAttributeNode(NewAttribute("theta",theta/degree));
   trapElement->setAttributeNode(NewAttribute("phi",phi/degree));
   trapElement->setAttributeNode(NewAttribute("y1",
                2.0*trap->GetYHalfLength1()/mm));
   trapElement->setAttributeNode(NewAttribute("x1",
                2.0*trap->GetXHalfLength1()/mm));
   trapElement->setAttributeNode(NewAttribute("x2",
                2.0*trap->GetXHalfLength2()/mm));
   trapElement->setAttributeNode(NewAttribute("alpha1",alpha1/degree));
   trapElement->setAttributeNode(NewAttribute("y2",
                2.0*trap->GetYHalfLength2()/mm));
   trapElement->setAttributeNode(NewAttribute("x3",
                2.0*trap->GetXHalfLength3()/mm));
   trapElement->setAttributeNode(NewAttribute("x4",
                2.0*trap->GetXHalfLength4()/mm));
   trapElement->setAttributeNode(NewAttribute("alpha2",alpha2/degree));
   trapElement->setAttributeNode(NewAttribute("aunit","deg"));
   trapElement->setAttributeNode(NewAttribute("lunit","mm"));
   solidsElement->appendChild(trapElement);
}

void G4GDMLWriteSolids::
TrdWrite(xercesc::DOMElement* solidsElement, const G4Trd* const trd)
{
   const G4String& name = GenerateName(trd->GetName(),trd);

   xercesc::DOMElement* trdElement = NewElement("trd");
   trdElement->setAttributeNode(NewAttribute("name",name));
   trdElement->setAttributeNode(NewAttribute("x1",
               2.0*trd->GetXHalfLength1()/mm));
   trdElement->setAttributeNode(NewAttribute("x2",
               2.0*trd->GetXHalfLength2()/mm));
   trdElement->setAttributeNode(NewAttribute("y1",
               2.0*trd->GetYHalfLength1()/mm));
   trdElement->setAttributeNode(NewAttribute("y2",
               2.0*trd->GetYHalfLength2()/mm));
   trdElement->setAttributeNode(NewAttribute("z",
               2.0*trd->GetZHalfLength()/mm));
   trdElement->setAttributeNode(NewAttribute("lunit","mm"));
   solidsElement->appendChild(trdElement);
}
*/
void G4GDMLWriteSolids::
TubeWrite(xercesc::DOMElement* solidsElement, const G4Tubs* const tube)
{
   const G4String& name = GenerateName(tube->GetName(),tube);

   xercesc::DOMElement* tubeElement = NewElement("tube");
   tubeElement->setAttributeNode(NewAttribute("name",name));
   tubeElement->setAttributeNode(NewAttribute("rmin",
                tube->GetInnerRadius()/mm));
   tubeElement->setAttributeNode(NewAttribute("rmax",
                tube->GetOuterRadius()/mm));
   tubeElement->setAttributeNode(NewAttribute("z",
                2.0*tube->GetZHalfLength()/mm));
   tubeElement->setAttributeNode(NewAttribute("startphi",
                tube->GetStartPhiAngle()/degree));
   tubeElement->setAttributeNode(NewAttribute("deltaphi",
                tube->GetDeltaPhiAngle()/degree));
   tubeElement->setAttributeNode(NewAttribute("aunit","deg"));
   tubeElement->setAttributeNode(NewAttribute("lunit","mm"));
   solidsElement->appendChild(tubeElement);
}
/*
void G4GDMLWriteSolids::
TwistedboxWrite(xercesc::DOMElement* solidsElement,
                const G4TwistedBox* const twistedbox)
{
   const G4String& name = GenerateName(twistedbox->GetName(),twistedbox);

   xercesc::DOMElement* twistedboxElement = NewElement("twistedbox");
   twistedboxElement->setAttributeNode(NewAttribute("name",name));
   twistedboxElement->setAttributeNode(NewAttribute("x",
                      2.0*twistedbox->GetXHalfLength()/mm));
   twistedboxElement->setAttributeNode(NewAttribute("y",
                      2.0*twistedbox->GetYHalfLength()/mm));
   twistedboxElement->setAttributeNode(NewAttribute("z",
                      2.0*twistedbox->GetZHalfLength()/mm));
   twistedboxElement->setAttributeNode(NewAttribute("PhiTwist",
                      twistedbox->GetPhiTwist()/degree));
   twistedboxElement->setAttributeNode(NewAttribute("aunit","deg"));
   twistedboxElement->setAttributeNode(NewAttribute("lunit","mm"));
   solidsElement->appendChild(twistedboxElement);
}

void G4GDMLWriteSolids::
TwistedtrapWrite(xercesc::DOMElement* solidsElement,
                 const G4TwistedTrap* const twistedtrap)
{
   const G4String& name = GenerateName(twistedtrap->GetName(),twistedtrap);

   xercesc::DOMElement* twistedtrapElement = NewElement("twistedtrap");
   twistedtrapElement->setAttributeNode(NewAttribute("name",name));
   twistedtrapElement->setAttributeNode(NewAttribute("y1",
                       2.0*twistedtrap->GetY1HalfLength()/mm));
   twistedtrapElement->setAttributeNode(NewAttribute("x1",
                       2.0*twistedtrap->GetX1HalfLength()/mm));
   twistedtrapElement->setAttributeNode(NewAttribute("x2",
                       2.0*twistedtrap->GetX2HalfLength()/mm));
   twistedtrapElement->setAttributeNode(NewAttribute("y2",
                       2.0*twistedtrap->GetY2HalfLength()/mm));
   twistedtrapElement->setAttributeNode(NewAttribute("x3",
                       2.0*twistedtrap->GetX3HalfLength()/mm));
   twistedtrapElement->setAttributeNode(NewAttribute("x4",
                       2.0*twistedtrap->GetX4HalfLength()/mm));
   twistedtrapElement->setAttributeNode(NewAttribute("z",
                       2.0*twistedtrap->GetZHalfLength()/mm));
   twistedtrapElement->setAttributeNode(NewAttribute("Alph",
                       twistedtrap->GetTiltAngleAlpha()/degree));
   twistedtrapElement->setAttributeNode(NewAttribute("Theta",
                       twistedtrap->GetPolarAngleTheta()/degree));
   twistedtrapElement->setAttributeNode(NewAttribute("Phi",
                       twistedtrap->GetAzimuthalAnglePhi()/degree));
   twistedtrapElement->setAttributeNode(NewAttribute("PhiTwist",
                       twistedtrap->GetPhiTwist()/degree));
   twistedtrapElement->setAttributeNode(NewAttribute("aunit","deg"));
   twistedtrapElement->setAttributeNode(NewAttribute("lunit","mm"));
   
   solidsElement->appendChild(twistedtrapElement);
}

void G4GDMLWriteSolids::
TwistedtrdWrite(xercesc::DOMElement* solidsElement,
                const G4TwistedTrd* const twistedtrd)
{
   const G4String& name = GenerateName(twistedtrd->GetName(),twistedtrd);

   xercesc::DOMElement* twistedtrdElement = NewElement("twistedtrd");
   twistedtrdElement->setAttributeNode(NewAttribute("name",name));
   twistedtrdElement->setAttributeNode(NewAttribute("x1",
                      2.0*twistedtrd->GetX1HalfLength()/mm));
   twistedtrdElement->setAttributeNode(NewAttribute("x2",
                      2.0*twistedtrd->GetX2HalfLength()/mm));
   twistedtrdElement->setAttributeNode(NewAttribute("y1",
                      2.0*twistedtrd->GetY1HalfLength()/mm));
   twistedtrdElement->setAttributeNode(NewAttribute("y2",
                      2.0*twistedtrd->GetY2HalfLength()/mm));
   twistedtrdElement->setAttributeNode(NewAttribute("z",
                      2.0*twistedtrd->GetZHalfLength()/mm));
   twistedtrdElement->setAttributeNode(NewAttribute("PhiTwist",
                      twistedtrd->GetPhiTwist()/degree));
   twistedtrdElement->setAttributeNode(NewAttribute("aunit","deg"));
   twistedtrdElement->setAttributeNode(NewAttribute("lunit","mm"));
   solidsElement->appendChild(twistedtrdElement);
}

void G4GDMLWriteSolids::
TwistedtubsWrite(xercesc::DOMElement* solidsElement,
                 const G4TwistedTubs* const twistedtubs)
{
   const G4String& name = GenerateName(twistedtubs->GetName(),twistedtubs);

   xercesc::DOMElement* twistedtubsElement = NewElement("twistedtubs");
   twistedtubsElement->setAttributeNode(NewAttribute("name",name));
   twistedtubsElement->setAttributeNode(NewAttribute("twistedangle",
                       twistedtubs->GetPhiTwist()/degree));
   twistedtubsElement->setAttributeNode(NewAttribute("endinnerrad",
                       twistedtubs->GetInnerRadius()/mm));
   twistedtubsElement->setAttributeNode(NewAttribute("endouterrad",
                       twistedtubs->GetOuterRadius()/mm));
   twistedtubsElement->setAttributeNode(NewAttribute("zlen",
                       2.0*twistedtubs->GetZHalfLength()/mm));
   twistedtubsElement->setAttributeNode(NewAttribute("phi",
                       twistedtubs->GetDPhi()/degree));
   twistedtubsElement->setAttributeNode(NewAttribute("aunit","deg"));
   twistedtubsElement->setAttributeNode(NewAttribute("lunit","mm"));
   solidsElement->appendChild(twistedtubsElement);
}

void G4GDMLWriteSolids::
ZplaneWrite(xercesc::DOMElement* element, const G4double& z,
            const G4double& rmin, const G4double& rmax)
{
   xercesc::DOMElement* zplaneElement = NewElement("zplane");
   zplaneElement->setAttributeNode(NewAttribute("z",z/mm));
   zplaneElement->setAttributeNode(NewAttribute("rmin",rmin/mm));
   zplaneElement->setAttributeNode(NewAttribute("rmax",rmax/mm));
   element->appendChild(zplaneElement);
}

void G4GDMLWriteSolids::
OpticalSurfaceWrite(xercesc::DOMElement* solidsElement,
                    const G4OpticalSurface* const surf)
{
   xercesc::DOMElement* optElement = NewElement("opticalsurface");
   G4OpticalSurfaceModel smodel = surf->GetModel();
   G4double sval = (smodel==glisur) ? surf->GetPolish() : surf->GetSigmaAlpha();

   optElement->setAttributeNode(NewAttribute("name", surf->GetName()));
   optElement->setAttributeNode(NewAttribute("model", smodel));
   optElement->setAttributeNode(NewAttribute("finish", surf->GetFinish()));
   optElement->setAttributeNode(NewAttribute("type", surf->GetType()));
   optElement->setAttributeNode(NewAttribute("value", sval));

   solidsElement->appendChild(optElement);
}
*/
void G4GDMLWriteSolids::SolidsWrite(xercesc::DOMElement* gdmlElement)
{
   //G4cout << "G4GDML: Writing solids..." << G4endl;

   solidsElement = NewElement("solids");
   gdmlElement->appendChild(solidsElement);

   solidList.clear();
}

void G4GDMLWriteSolids::AddSolid(const G4VSolid* const solidPtr)
{
   for (size_t i=0; i<solidList.size(); i++)   // Check if solid is
   {                                           // already in the list!
      if (solidList[i] == solidPtr)  { return; }
   }

   solidList.push_back(solidPtr);

//   if (const G4BooleanSolid* const booleanPtr
//     = dynamic_cast<const G4BooleanSolid*>(solidPtr))
//     { BooleanWrite(solidsElement,booleanPtr); } else
   if (const G4Box* const boxPtr
     = dynamic_cast<const G4Box*>(solidPtr))
     { BoxWrite(solidsElement,boxPtr); } else
/*   if (const G4Cons* const conePtr
     = dynamic_cast<const G4Cons*>(solidPtr))
     { ConeWrite(solidsElement,conePtr); } else
   if (const G4EllipticalCone* const elconePtr
     = dynamic_cast<const G4EllipticalCone*>(solidPtr))
     { ElconeWrite(solidsElement,elconePtr); } else
   if (const G4Ellipsoid* const ellipsoidPtr
     = dynamic_cast<const G4Ellipsoid*>(solidPtr))
     { EllipsoidWrite(solidsElement,ellipsoidPtr); } else
   if (const G4EllipticalTube* const eltubePtr
     = dynamic_cast<const G4EllipticalTube*>(solidPtr))
     { EltubeWrite(solidsElement,eltubePtr); } else
   if (const G4ExtrudedSolid* const xtruPtr
     = dynamic_cast<const G4ExtrudedSolid*>(solidPtr))
     { XtruWrite(solidsElement,xtruPtr); } else
   if (const G4Hype* const hypePtr
     = dynamic_cast<const G4Hype*>(solidPtr))
     { HypeWrite(solidsElement,hypePtr); } else
   if (const G4Orb* const orbPtr
     = dynamic_cast<const G4Orb*>(solidPtr))
     { OrbWrite(solidsElement,orbPtr); } else
   if (const G4Para* const paraPtr
     = dynamic_cast<const G4Para*>(solidPtr))
     { ParaWrite(solidsElement,paraPtr); } else
   if (const G4Paraboloid* const paraboloidPtr
     = dynamic_cast<const G4Paraboloid*>(solidPtr))
     { ParaboloidWrite(solidsElement,paraboloidPtr); } else
   if (const G4Polycone* const polyconePtr
     = dynamic_cast<const G4Polycone*>(solidPtr))
     { PolyconeWrite(solidsElement,polyconePtr); } else
   if (const G4Polyhedra* const polyhedraPtr
     = dynamic_cast<const G4Polyhedra*>(solidPtr))
     { PolyhedraWrite(solidsElement,polyhedraPtr); } else
   if (const G4Sphere* const spherePtr
     = dynamic_cast<const G4Sphere*>(solidPtr))
     { SphereWrite(solidsElement,spherePtr); } else
   if (const G4TessellatedSolid* const tessellatedPtr
     = dynamic_cast<const G4TessellatedSolid*>(solidPtr))
     { TessellatedWrite(solidsElement,tessellatedPtr); } else
   if (const G4Tet* const tetPtr
     = dynamic_cast<const G4Tet*>(solidPtr))
     { TetWrite(solidsElement,tetPtr); } else
   if (const G4Torus* const torusPtr
     = dynamic_cast<const G4Torus*>(solidPtr))
     { TorusWrite(solidsElement,torusPtr); } else
   if (const G4Trap* const trapPtr
     = dynamic_cast<const G4Trap*>(solidPtr))
     { TrapWrite(solidsElement,trapPtr); } else
   if (const G4Trd* const trdPtr
     = dynamic_cast<const G4Trd*>(solidPtr))
     { TrdWrite(solidsElement,trdPtr); } else*/
   if (const G4Tubs* const tubePtr
     = dynamic_cast<const G4Tubs*>(solidPtr))
     { TubeWrite(solidsElement,tubePtr); }/* else
   if (const G4TwistedBox* const twistedboxPtr
     = dynamic_cast<const G4TwistedBox*>(solidPtr))
     { TwistedboxWrite(solidsElement,twistedboxPtr); } else
   if (const G4TwistedTrap* const twistedtrapPtr
     = dynamic_cast<const G4TwistedTrap*>(solidPtr))
     { TwistedtrapWrite(solidsElement,twistedtrapPtr); } else
   if (const G4TwistedTrd* const twistedtrdPtr
     = dynamic_cast<const G4TwistedTrd*>(solidPtr))
     { TwistedtrdWrite(solidsElement,twistedtrdPtr); } else
   if (const G4TwistedTubs* const twistedtubsPtr
     = dynamic_cast<const G4TwistedTubs*>(solidPtr))
     { TwistedtubsWrite(solidsElement,twistedtubsPtr); }*/
   else
   {
     G4String error_msg = "Unknown solid: " + solidPtr->GetName()
                        + "; Type: " + solidPtr->GetEntityType();
///     G4Exception("G4GDMLWriteSolids::AddSolid()", "WriteError",
///                 FatalException, error_msg);
   }
}
