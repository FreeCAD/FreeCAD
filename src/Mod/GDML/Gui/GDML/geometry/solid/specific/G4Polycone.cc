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
// $Id: G4Polycone.cc,v 1.43 2008/05/15 13:45:15 gcosmo Exp $
// GEANT4 tag $Name: geant4-09-03-patch-01 $
//
// 
// --------------------------------------------------------------------
// GEANT 4 class source file
//
//
// G4Polycone.cc
//
// Implementation of a CSG polycone
//
// --------------------------------------------------------------------

#include "G4Polycone.hh"

#include "G4PolyconeSide.hh"
#include "G4PolyPhiFace.hh"

///#include "Randomize.hh"

///#include "G4Polyhedron.hh"
#include "../../../graphics_reps/G4Polyhedron.hh"
///#include "G4EnclosingCylinder.hh"
#include "G4EnclosingCylinder.hh"
///#include "G4ReduciblePolygon.hh"
#include "G4ReduciblePolygon.hh"
///#include "G4VPVParameterisation.hh"
#include "../../management/G4VPVParameterisation.hh"

//For twopi...
#include "../../../CLHEP/units/PhysicalConstants.h"
using namespace CLHEP;

//
// Constructor (GEANT3 style parameters)
//  
G4Polycone::G4Polycone( const G4String& name, 
                              G4double phiStart,
                              G4double phiTotal,
                              G4int numZPlanes,
                        const G4double zPlane[],
                        const G4double rInner[],
                        const G4double rOuter[]  )
  : G4VCSGfaceted( name ), genericPcon(false)
{
  //
  // Some historical ugliness
  //
  original_parameters = new G4PolyconeHistorical();
  
  original_parameters->Start_angle = phiStart;
  original_parameters->Opening_angle = phiTotal;
  original_parameters->Num_z_planes = numZPlanes;
  original_parameters->Z_values = new G4double[numZPlanes];
  original_parameters->Rmin = new G4double[numZPlanes];
  original_parameters->Rmax = new G4double[numZPlanes];

  G4int i;
  for (i=0; i<numZPlanes; i++)
  {
    if (( i < numZPlanes-1) && ( zPlane[i] == zPlane[i+1] ))
    {
      if( (rInner[i]   > rOuter[i+1])
        ||(rInner[i+1] > rOuter[i])   )
      {
/*        DumpInfo();
        G4cerr << "ERROR - G4Polycone::G4Polycone()"
               << G4endl
               << "        Segments are not contiguous !" << G4endl
               << "        rMin[" << i << "] = " << rInner[i]
               << " -- rMax[" << i+1 << "] = " << rOuter[i+1] << G4endl
               << "        rMin[" << i+1 << "] = " << rInner[i+1]
               << " -- rMax[" << i << "] = " << rOuter[i] << G4endl;
        G4Exception("G4Polycone::G4Polycone()", "InvalidSetup", FatalException, 
                    "Cannot create a Polycone with no contiguous segments.");
*/      }
    } 
    original_parameters->Z_values[i] = zPlane[i];
    original_parameters->Rmin[i] = rInner[i];
    original_parameters->Rmax[i] = rOuter[i];
  }

  //
  // Build RZ polygon using special PCON/PGON GEANT3 constructor
  //
  G4ReduciblePolygon *rz =
    new G4ReduciblePolygon( rInner, rOuter, zPlane, numZPlanes );
  
  //
  // Do the real work
  //
  Create( phiStart, phiTotal, rz );
  
  delete rz;
}


//
// Constructor (generic parameters)
//
G4Polycone::G4Polycone( const G4String& name, 
                              G4double phiStart,
                              G4double phiTotal,
                              G4int    numRZ,
                        const G4double r[],
                        const G4double z[]   )
  : G4VCSGfaceted( name ), genericPcon(true)
{
  G4ReduciblePolygon *rz = new G4ReduciblePolygon( r, z, numRZ );
  
  Create( phiStart, phiTotal, rz );
  
  // Set original_parameters struct for consistency
  //
  SetOriginalParameters();
  
  delete rz;
}


//
// Create
//
// Generic create routine, called by each constructor after
// conversion of arguments
//
void G4Polycone::Create( G4double phiStart,
                         G4double phiTotal,
                         G4ReduciblePolygon *rz    )
{
  //
  // Perform checks of rz values
  //
  if (rz->Amin() < 0.0)
  {
/*    G4cerr << "ERROR - G4Polycone::Create(): " << GetName() << G4endl
           << "        All R values must be >= 0 !"
           << G4endl;
    G4Exception("G4Polycone::Create()", "InvalidSetup", FatalException,
                "Illegal input parameters.");*/
  }
    
  G4double rzArea = rz->Area();
  if (rzArea < -kCarTolerance)
    rz->ReverseOrder();

  else if (rzArea < -kCarTolerance)
  {
 /*   G4cerr << "ERROR - G4Polycone::Create(): " << GetName() << G4endl
           << "        R/Z cross section is zero or near zero: "
           << rzArea << G4endl;
    G4Exception("G4Polycone::Create()", "InvalidSetup", FatalException,
                "Illegal input parameters.");*/
  }
    
  if ( (!rz->RemoveDuplicateVertices( kCarTolerance ))
    || (!rz->RemoveRedundantVertices( kCarTolerance ))     ) 
  {
 /*   G4cerr << "ERROR - G4Polycone::Create(): " << GetName() << G4endl
           << "        Too few unique R/Z values !"
           << G4endl;
    G4Exception("G4Polycone::Create()", "InvalidSetup", FatalException,
                "Illegal input parameters.");*/
  }

  if (rz->CrossesItself(1/kInfinity)) 
  {
/*    G4cerr << "ERROR - G4Polycone::Create(): " << GetName() << G4endl
           << "        R/Z segments cross !"
           << G4endl;
    G4Exception("G4Polycone::Create()", "InvalidSetup", FatalException,
                "Illegal input parameters.");*/
  }

  numCorner = rz->NumVertices();

  //
  // Phi opening? Account for some possible roundoff, and interpret
  // nonsense value as representing no phi opening
  //
  if (phiTotal <= 0 || phiTotal > twopi-1E-10)
  {
    phiIsOpen = false;
    startPhi = 0;
    endPhi = twopi;
  }
  else
  {
    phiIsOpen = true;
    
    //
    // Convert phi into our convention
    //
    startPhi = phiStart;
    while( startPhi < 0 ) startPhi += twopi;
    
    endPhi = phiStart+phiTotal;
    while( endPhi < startPhi ) endPhi += twopi;
  }
  
  //
  // Allocate corner array. 
  //
  corners = new G4PolyconeSideRZ[numCorner];

  //
  // Copy corners
  //
  G4ReduciblePolygonIterator iterRZ(rz);
  
  G4PolyconeSideRZ *next = corners;
  iterRZ.Begin();
  do
  {
    next->r = iterRZ.GetA();
    next->z = iterRZ.GetB();
  } while( ++next, iterRZ.Next() );
  
  //
  // Allocate face pointer array
  //
  numFace = phiIsOpen ? numCorner+2 : numCorner;
  faces = new G4VCSGface*[numFace];
  
  //
  // Construct conical faces
  //
  // But! Don't construct a face if both points are at zero radius!
  //
  G4PolyconeSideRZ *corner = corners,
                   *prev = corners + numCorner-1,
                   *nextNext;
  G4VCSGface  **face = faces;
  do
  {
    next = corner+1;
    if (next >= corners+numCorner) next = corners;
    nextNext = next+1;
    if (nextNext >= corners+numCorner) nextNext = corners;
    
    if (corner->r < 1/kInfinity && next->r < 1/kInfinity) continue;
    
    //
    // We must decide here if we can dare declare one of our faces
    // as having a "valid" normal (i.e. allBehind = true). This
    // is never possible if the face faces "inward" in r.
    //
    G4bool allBehind;
    if (corner->z > next->z)
    {
      allBehind = false;
    }
    else
    {
      //
      // Otherwise, it is only true if the line passing
      // through the two points of the segment do not
      // split the r/z cross section
      //
      allBehind = !rz->BisectedBy( corner->r, corner->z,
                 next->r, next->z, kCarTolerance );
    }
    
    *face++ = new G4PolyconeSide( prev, corner, next, nextNext,
                startPhi, endPhi-startPhi, phiIsOpen, allBehind );
  } while( prev=corner, corner=next, corner > corners );
  
  if (phiIsOpen)
  {
    //
    // Construct phi open edges
    //
    *face++ = new G4PolyPhiFace( rz, startPhi, 0, endPhi  );
    *face++ = new G4PolyPhiFace( rz, endPhi,   0, startPhi );
  }
  
  //
  // We might have dropped a face or two: recalculate numFace
  //
  numFace = face-faces;
  
  //
  // Make enclosingCylinder
  //
  enclosingCylinder =
    new G4EnclosingCylinder( rz, phiIsOpen, phiStart, phiTotal );
}


//
// Fake default constructor - sets only member data and allocates memory
//                            for usage restricted to object persistency.
//
G4Polycone::G4Polycone( __void__& a )
  : G4VCSGfaceted(a), genericPcon(false), corners(0),
    original_parameters(0), enclosingCylinder(0)
{
}


//
// Destructor
//
G4Polycone::~G4Polycone()
{
  delete [] corners;
  
  if (original_parameters) delete original_parameters;
  if (enclosingCylinder) delete enclosingCylinder;
}


//
// Copy constructor
//
G4Polycone::G4Polycone( const G4Polycone &source )
  : G4VCSGfaceted( source )
{
  CopyStuff( source );
}


//
// Assignment operator
//
const G4Polycone &G4Polycone::operator=( const G4Polycone &source )
{
  if (this == &source) return *this;
  
  G4VCSGfaceted::operator=( source );
  
  delete [] corners;
  if (original_parameters) delete original_parameters;
  
  delete enclosingCylinder;
  
  CopyStuff( source );
  
  return *this;
}


//
// CopyStuff
//
void G4Polycone::CopyStuff( const G4Polycone &source )
{
  //
  // Simple stuff
  //
  startPhi  = source.startPhi;
  endPhi    = source.endPhi;
  phiIsOpen  = source.phiIsOpen;
  numCorner  = source.numCorner;
  genericPcon= source.genericPcon;

  //
  // The corner array
  //
  corners = new G4PolyconeSideRZ[numCorner];
  
  G4PolyconeSideRZ  *corn = corners,
        *sourceCorn = source.corners;
  do
  {
    *corn = *sourceCorn;
  } while( ++sourceCorn, ++corn < corners+numCorner );
  
  //
  // Original parameters
  //
  if (source.original_parameters)
  {
    original_parameters =
      new G4PolyconeHistorical( *source.original_parameters );
  }
  
  //
  // Enclosing cylinder
  //
  enclosingCylinder = new G4EnclosingCylinder( *source.enclosingCylinder );
}


//
// Reset
//
G4bool G4Polycone::Reset()
{
  if (genericPcon)
  {
/*    G4cerr << "Solid " << GetName() << " built using generic construct."
           << G4endl << "Not applicable to the generic construct !" << G4endl;
    G4Exception("G4Polycone::Reset()", "NotApplicableConstruct",
                JustWarning, "Parameters NOT resetted.");*/
    return 1;
  }

  //
  // Clear old setup
  //
  G4VCSGfaceted::DeleteStuff();
  delete [] corners;
  delete enclosingCylinder;

  //
  // Rebuild polycone
  //
  G4ReduciblePolygon *rz =
    new G4ReduciblePolygon( original_parameters->Rmin,
                            original_parameters->Rmax,
                            original_parameters->Z_values,
                            original_parameters->Num_z_planes );
  Create( original_parameters->Start_angle,
          original_parameters->Opening_angle, rz );
  delete rz;

  return 0;
}


//
// Inside
//
// This is an override of G4VCSGfaceted::Inside, created in order
// to speed things up by first checking with G4EnclosingCylinder.
//
EInside G4Polycone::Inside( const G4ThreeVector &p ) const
{
  //
  // Quick test
  //
  if (enclosingCylinder->MustBeOutside(p)) return kOutside;

  //
  // Long answer
  //
  return G4VCSGfaceted::Inside(p);
}


//
// DistanceToIn
//
// This is an override of G4VCSGfaceted::Inside, created in order
// to speed things up by first checking with G4EnclosingCylinder.
//
G4double G4Polycone::DistanceToIn( const G4ThreeVector &p,
                                   const G4ThreeVector &v ) const
{
  //
  // Quick test
  //
  if (enclosingCylinder->ShouldMiss(p,v))
    return kInfinity;
  
  //
  // Long answer
  //
  return G4VCSGfaceted::DistanceToIn( p, v );
}


//
// DistanceToIn
//
G4double G4Polycone::DistanceToIn( const G4ThreeVector &p ) const
{
  return G4VCSGfaceted::DistanceToIn(p);
}


//
// ComputeDimensions
//
void G4Polycone::ComputeDimensions(       G4VPVParameterisation* p,
                                    const G4int n,
                                    const G4VPhysicalVolume* pRep )
{
  p->ComputeDimensions(*this,n,pRep);
}

//
// GetEntityType
//
G4GeometryType  G4Polycone::GetEntityType() const
{
  return G4String("G4Polycone");
}

//
// Stream object contents to an output stream
//
std::ostream& G4Polycone::StreamInfo( std::ostream& os ) const
{
  os << "-----------------------------------------------------------\n"
     << "    *** Dump for solid - " << GetName() << " ***\n"
     << "    ===================================================\n"
     << " Solid type: G4Polycone\n"
     << " Parameters: \n"
     << "    starting phi angle : " << startPhi/degree << " degrees \n"
     << "    ending phi angle   : " << endPhi/degree << " degrees \n";
  G4int i=0;
  if (!genericPcon)
  {
    G4int numPlanes = original_parameters->Num_z_planes;
    os << "    number of Z planes: " << numPlanes << "\n"
       << "              Z values: \n";
    for (i=0; i<numPlanes; i++)
    {
      os << "              Z plane " << i << ": "
         << original_parameters->Z_values[i] << "\n";
    }
    os << "              Tangent distances to inner surface (Rmin): \n";
    for (i=0; i<numPlanes; i++)
    {
      os << "              Z plane " << i << ": "
         << original_parameters->Rmin[i] << "\n";
    }
    os << "              Tangent distances to outer surface (Rmax): \n";
    for (i=0; i<numPlanes; i++)
    {
      os << "              Z plane " << i << ": "
         << original_parameters->Rmax[i] << "\n";
    }
  }
  os << "    number of RZ points: " << numCorner << "\n"
     << "              RZ values (corners): \n";
     for (i=0; i<numCorner; i++)
     {
       os << "                         "
          << corners[i].r << ", " << corners[i].z << "\n";
     }
  os << "-----------------------------------------------------------\n";

  return os;
}


//
// GetPointOnCone
//
// Auxiliary method for Get Point On Surface
//
G4ThreeVector G4Polycone::GetPointOnCone(G4double fRmin1, G4double fRmax1,
                                         G4double fRmin2, G4double fRmax2,
                                         G4double zOne,   G4double zTwo,
                                         G4double& totArea) const
{ 
	/*
  // declare working variables
  //
  G4double Aone, Atwo, Afive, phi, zRand, fDPhi, fSPhi, cosu, sinu;
  G4double rRand1, chose, rone, rtwo, qone, qtwo,
           fDz = std::fabs((zTwo-zOne)/2.);
  G4ThreeVector point, offset;
  offset = G4ThreeVector(0.,0.,0.5*(zTwo+zOne));
  fSPhi = startPhi; fDPhi = endPhi - startPhi;
  rone = (fRmax1-fRmax2)/(2.*fDz); 
  rtwo = (fRmin1-fRmin2)/(2.*fDz);
  if(fRmax1==fRmax2){qone=0.;}
  else{
    qone = fDz*(fRmax1+fRmax2)/(fRmax1-fRmax2);
  }
  if(fRmin1==fRmin2){qtwo=0.;}
  else{
    qtwo = fDz*(fRmin1+fRmin2)/(fRmin1-fRmin2);
   }
  Aone   = 0.5*fDPhi*(fRmax2 + fRmax1)*(sqr(fRmin1-fRmin2)+sqr(zTwo-zOne));       
  Atwo   = 0.5*fDPhi*(fRmin2 + fRmin1)*(sqr(fRmax1-fRmax2)+sqr(zTwo-zOne));
  Afive  = fDz*(fRmax1-fRmin1+fRmax2-fRmin2);
  totArea = Aone+Atwo+2.*Afive;
  
  phi  = RandFlat::shoot(startPhi,endPhi);
  cosu = std::cos(phi);
  sinu = std::sin(phi);


  if( (startPhi == 0) && (endPhi == twopi) ) { Afive = 0; }
  chose = RandFlat::shoot(0.,Aone+Atwo+2.*Afive);
  if( (chose >= 0) && (chose < Aone) )
  {
    if(fRmax1 != fRmax2)
    {
      zRand = RandFlat::shoot(-1.*fDz,fDz); 
      point = G4ThreeVector (rone*cosu*(qone-zRand),
                             rone*sinu*(qone-zRand), zRand);
      
     
    }
    else
    {
      point = G4ThreeVector(fRmax1*cosu, fRmax1*sinu,
                            RandFlat::shoot(-1.*fDz,fDz));
     
    }
  }
  else if(chose >= Aone && chose < Aone + Atwo)
  {
    if(fRmin1 != fRmin2)
      { 
      zRand = RandFlat::shoot(-1.*fDz,fDz); 
      point = G4ThreeVector (rtwo*cosu*(qtwo-zRand),
                             rtwo*sinu*(qtwo-zRand), zRand);
      
    }
    else
    {
      point = G4ThreeVector(fRmin1*cosu, fRmin1*sinu,
                            RandFlat::shoot(-1.*fDz,fDz));
     
    }
  }
  else if( (chose >= Aone + Atwo + Afive) && (chose < Aone + Atwo + 2.*Afive) )
  {
    zRand  = RandFlat::shoot(-1.*fDz,fDz); 
    rRand1 = RandFlat::shoot(fRmin2-((zRand-fDz)/(2.*fDz))*(fRmin1-fRmin2),
                             fRmax2-((zRand-fDz)/(2.*fDz))*(fRmax1-fRmax2)); 
    point =  G4ThreeVector (rRand1*std::cos(startPhi),
                            rRand1*std::sin(startPhi), zRand);
  }
  else
  { 
    zRand  = RandFlat::shoot(-1.*fDz,fDz); 
    rRand1 = RandFlat::shoot(fRmin2-((zRand-fDz)/(2.*fDz))*(fRmin1-fRmin2),
                             fRmax2-((zRand-fDz)/(2.*fDz))*(fRmax1-fRmax2)); 
    point  = G4ThreeVector (rRand1*std::cos(endPhi),
                            rRand1*std::sin(endPhi), zRand);
   
  }
  return point+offset;*/
	return G4ThreeVector(0.,0.,0.);
}


//
// GetPointOnTubs
//
// Auxiliary method for GetPoint On Surface
//
G4ThreeVector G4Polycone::GetPointOnTubs(G4double fRMin, G4double fRMax,
                                         G4double zOne,  G4double zTwo,
                                         G4double& totArea) const
{ 
	/*
  G4double xRand,yRand,zRand,phi,cosphi,sinphi,chose,
           aOne,aTwo,aFou,rRand,fDz,fSPhi,fDPhi;
  fDz = std::fabs(0.5*(zTwo-zOne));
  fSPhi = startPhi;
  fDPhi = endPhi-startPhi;
  
  aOne = 2.*fDz*fDPhi*fRMax;
  aTwo = 2.*fDz*fDPhi*fRMin;
  aFou = 2.*fDz*(fRMax-fRMin);
  totArea = aOne+aTwo+2.*aFou;
  phi    = RandFlat::shoot(startPhi,endPhi);
  cosphi = std::cos(phi);
  sinphi = std::sin(phi);
  rRand  = RandFlat::shoot(fRMin,fRMax);
 
  if(startPhi == 0 && endPhi == twopi) 
    aFou = 0;
  
  chose  = RandFlat::shoot(0.,aOne+aTwo+2.*aFou);
  if( (chose >= 0) && (chose < aOne) )
  {
    xRand = fRMax*cosphi;
    yRand = fRMax*sinphi;
    zRand = RandFlat::shoot(-1.*fDz,fDz);
    return G4ThreeVector(xRand, yRand, zRand+0.5*(zTwo+zOne));
  }
  else if( (chose >= aOne) && (chose < aOne + aTwo) )
  {
    xRand = fRMin*cosphi;
    yRand = fRMin*sinphi;
    zRand = RandFlat::shoot(-1.*fDz,fDz);
    return G4ThreeVector(xRand, yRand, zRand+0.5*(zTwo+zOne));
  }
  else if( (chose >= aOne+aTwo) && (chose <aOne+aTwo+aFou) )
  {
    xRand = rRand*std::cos(fSPhi+fDPhi);
    yRand = rRand*std::sin(fSPhi+fDPhi);
    zRand = RandFlat::shoot(-1.*fDz,fDz);
    return G4ThreeVector(xRand, yRand, zRand+0.5*(zTwo+zOne));
  }

  // else

  xRand = rRand*std::cos(fSPhi+fDPhi);
  yRand = rRand*std::sin(fSPhi+fDPhi);
  zRand = RandFlat::shoot(-1.*fDz,fDz);
  return G4ThreeVector(xRand, yRand, zRand+0.5*(zTwo+zOne));
  */return G4ThreeVector(0.,0.,0.);
}


//
// GetPointOnRing
//
// Auxiliary method for GetPoint On Surface
//
G4ThreeVector G4Polycone::GetPointOnRing(G4double fRMin1, G4double fRMax1,
                                         G4double fRMin2,G4double fRMax2,
                                         G4double zOne) const
{/*
  G4double xRand,yRand,phi,cosphi,sinphi,rRand1,rRand2,A1,Atot,rCh;
  
  phi    = RandFlat::shoot(startPhi,endPhi);
  cosphi = std::cos(phi);
  sinphi = std::sin(phi);

  if(fRMin1==fRMin2)
  {
    rRand1 = fRMin1; A1=0.;
  }
  else
  {
    rRand1 = RandFlat::shoot(fRMin1,fRMin2);
    A1=std::abs(fRMin2*fRMin2-fRMin1*fRMin1);
  }
  if(fRMax1==fRMax2)
  {
    rRand2=fRMax1; Atot=A1;
  }
  else
  {
    rRand2 = RandFlat::shoot(fRMax1,fRMax2);
    Atot   = A1+std::abs(fRMax2*fRMax2-fRMax1*fRMax1);
  }
  rCh   = RandFlat::shoot(0.,Atot);
 
  if(rCh>A1) { rRand1=rRand2; }
  
  xRand = rRand1*cosphi;
  yRand = rRand1*sinphi;

  return G4ThreeVector(xRand, yRand, zOne);*/
	return G4ThreeVector(0.,0.,0.);
}


//
// GetPointOnCut
//
// Auxiliary method for Get Point On Surface
//
G4ThreeVector G4Polycone::GetPointOnCut(G4double fRMin1, G4double fRMax1,
                                        G4double fRMin2, G4double fRMax2,
                                        G4double zOne,  G4double zTwo,
                                        G4double& totArea) const
{  /* if(zOne==zTwo)
    {
      return GetPointOnRing(fRMin1, fRMax1,fRMin2,fRMax2,zOne);
    }
    if( (fRMin1 == fRMin2) && (fRMax1 == fRMax2) )
    {
      return GetPointOnTubs(fRMin1, fRMax1,zOne,zTwo,totArea);
    }
    return GetPointOnCone(fRMin1,fRMax1,fRMin2,fRMax2,zOne,zTwo,totArea);
	*/
	return G4ThreeVector(0.,0.,0.);
}


//
// GetPointOnSurface
//
G4ThreeVector G4Polycone::GetPointOnSurface() const
{
	/*
  if (!genericPcon)  // Polycone by faces
  {
    G4double Area=0,totArea=0,Achose1=0,Achose2=0,phi,cosphi,sinphi,rRand;
    G4int i=0;
    G4int numPlanes = original_parameters->Num_z_planes;
  
    phi = RandFlat::shoot(startPhi,endPhi);
    cosphi = std::cos(phi);
    sinphi = std::sin(phi);

    rRand = RandFlat::shoot(original_parameters->Rmin[0],
                            original_parameters->Rmax[0]);
  
    std::vector<G4double> areas;       // (numPlanes+1);
    std::vector<G4ThreeVector> points; // (numPlanes-1);
  
    areas.push_back(pi*(sqr(original_parameters->Rmax[0])
                       -sqr(original_parameters->Rmin[0])));

    for(i=0; i<numPlanes-1; i++)
    {
      Area = (original_parameters->Rmin[i]+original_parameters->Rmin[i+1])
           * std::sqrt(sqr(original_parameters->Rmin[i]
                      -original_parameters->Rmin[i+1])+
                       sqr(original_parameters->Z_values[i+1]
                      -original_parameters->Z_values[i]));

      Area += (original_parameters->Rmax[i]+original_parameters->Rmax[i+1])
            * std::sqrt(sqr(original_parameters->Rmax[i]
                       -original_parameters->Rmax[i+1])+
                        sqr(original_parameters->Z_values[i+1]
                       -original_parameters->Z_values[i]));

      Area *= 0.5*(endPhi-startPhi);
    
      if(startPhi==0.&& endPhi == twopi)
      {
        Area += std::fabs(original_parameters->Z_values[i+1]
                         -original_parameters->Z_values[i])*
                         (original_parameters->Rmax[i]
                         +original_parameters->Rmax[i+1]
                         -original_parameters->Rmin[i]
                         -original_parameters->Rmin[i+1]);
      }
      areas.push_back(Area);
      totArea += Area;
    }
  
    areas.push_back(pi*(sqr(original_parameters->Rmax[numPlanes-1])-
                        sqr(original_parameters->Rmin[numPlanes-1])));
  
    totArea += (areas[0]+areas[numPlanes]);
    G4double chose = RandFlat::shoot(0.,totArea);

    if( (chose>=0.) && (chose<areas[0]) )
    {
      return G4ThreeVector(rRand*cosphi, rRand*sinphi,
                           original_parameters->Z_values[0]);
    }
  
    for (i=0; i<numPlanes-1; i++)
    {
      Achose1 += areas[i];
      Achose2 = (Achose1+areas[i+1]);
      if(chose>=Achose1 && chose<Achose2)
      {
        return GetPointOnCut(original_parameters->Rmin[i],
                             original_parameters->Rmax[i],
                             original_parameters->Rmin[i+1],
                             original_parameters->Rmax[i+1],
                             original_parameters->Z_values[i],
                             original_parameters->Z_values[i+1], Area);
      }
    }

    rRand = RandFlat::shoot(original_parameters->Rmin[numPlanes-1],
                            original_parameters->Rmax[numPlanes-1]);
  
    return G4ThreeVector(rRand*cosphi,rRand*sinphi,
                         original_parameters->Z_values[numPlanes-1]);  

  }
  else  // Generic Polycone
  {
    return GetPointOnSurfaceGeneric();  
  }
  */
	return G4ThreeVector(0.,0.,0.);
}

//
// CreatePolyhedron
//
G4Polyhedron* G4Polycone::CreatePolyhedron() const
{ 
  //
  // This has to be fixed in visualization. Fake it for the moment.
  // 
  if (!genericPcon)
  {
    return new G4PolyhedronPcon( original_parameters->Start_angle,
                                 original_parameters->Opening_angle,
                                 original_parameters->Num_z_planes,
                                 original_parameters->Z_values,
                                 original_parameters->Rmin,
                                 original_parameters->Rmax );
  }
  else
  {
    // The following code prepares for:
    // HepPolyhedron::createPolyhedron(int Nnodes, int Nfaces,
    //                                  const double xyz[][3],
    //                                  const int faces_vec[][4])
    // Here is an extract from the header file HepPolyhedron.h:
    /**
     * Creates user defined polyhedron.
     * This function allows to the user to define arbitrary polyhedron.
     * The faces of the polyhedron should be either triangles or planar
     * quadrilateral. Nodes of a face are defined by indexes pointing to
     * the elements in the xyz array. Numeration of the elements in the
     * array starts from 1 (like in fortran). The indexes can be positive
     * or negative. Negative sign means that the corresponding edge is
     * invisible. The normal of the face should be directed to exterior
     * of the polyhedron. 
     * 
     * @param  Nnodes number of nodes
     * @param  Nfaces number of faces
     * @param  xyz    nodes
     * @param  faces_vec  faces (quadrilaterals or triangles)
     * @return status of the operation - is non-zero in case of problem
     */
    const G4int numSide =
          G4int(G4Polyhedron::GetNumberOfRotationSteps()
                * (endPhi - startPhi) / twopi) + 1;
    G4int nNodes;
    G4int nFaces;
    typedef G4double double3[3];
    double3* xyz;
    typedef G4int int4[4];
    int4* faces_vec;
    if (phiIsOpen)
    {
      // Triangulate open ends. Simple ear-chopping algorithm...
      // I'm not sure how robust this algorithm is (J.Allison).
      //
      std::vector<G4bool> chopped(numCorner, false);
      std::vector<G4int*> triQuads;
      G4int remaining = numCorner;
      G4int iStarter = 0;
      while (remaining >= 3)
      {
        // Find unchopped corners...
        //
        G4int A = -1, B = -1, C = -1;
        G4int iStepper = iStarter;
        do
        {
          if (A < 0)      { A = iStepper; }
          else if (B < 0) { B = iStepper; }
          else if (C < 0) { C = iStepper; }
          do
          {
            if (++iStepper >= numCorner) { iStepper = 0; }
          }
          while (chopped[iStepper]);
        }
        while (C < 0 && iStepper != iStarter);

        // Check triangle at B is pointing outward (an "ear").
        // Sign of z cross product determines...
        //
        G4double BAr = corners[A].r - corners[B].r;
        G4double BAz = corners[A].z - corners[B].z;
        G4double BCr = corners[C].r - corners[B].r;
        G4double BCz = corners[C].z - corners[B].z;
        if (BAr * BCz - BAz * BCr < kCarTolerance)
        {
          G4int* tq = new G4int[3];
          tq[0] = A + 1;
          tq[1] = B + 1;
          tq[2] = C + 1;
          triQuads.push_back(tq);
          chopped[B] = true;
          --remaining;
        }
        else
        {
          do
          {
            if (++iStarter >= numCorner) { iStarter = 0; }
          }
          while (chopped[iStarter]);
        }
      }
      // Transfer to faces...
      //
      nNodes = (numSide + 1) * numCorner;
      nFaces = numSide * numCorner + 2 * triQuads.size();
      faces_vec = new int4[nFaces];
      G4int iface = 0;
      G4int addition = numCorner * numSide;
      G4int d = numCorner - 1;
      for (G4int iEnd = 0; iEnd < 2; ++iEnd)
      {
        for (size_t i = 0; i < triQuads.size(); ++i)
        {
          // Negative for soft/auxiliary/normally invisible edges...
          //
          G4int a, b, c;
          if (iEnd == 0)
          {
            a = triQuads[i][0];
            b = triQuads[i][1];
            c = triQuads[i][2];
          }
          else
          {
            a = triQuads[i][0] + addition;
            b = triQuads[i][2] + addition;
            c = triQuads[i][1] + addition;
          }
          G4int ab = std::abs(b - a);
          G4int bc = std::abs(c - b);
          G4int ca = std::abs(a - c);
          faces_vec[iface][0] = (ab == 1 || ab == d)? a: -a;
          faces_vec[iface][1] = (bc == 1 || bc == d)? b: -b;
          faces_vec[iface][2] = (ca == 1 || ca == d)? c: -c;
          faces_vec[iface][3] = 0;
          ++iface;
        }
      }

      // Continue with sides...

      xyz = new double3[nNodes];
      const G4double dPhi = (endPhi - startPhi) / numSide;
      G4double phi = startPhi;
      G4int ixyz = 0;
      for (G4int iSide = 0; iSide < numSide; ++iSide)
      {
        for (G4int iCorner = 0; iCorner < numCorner; ++iCorner)
        {
          xyz[ixyz][0] = corners[iCorner].r * std::cos(phi);
          xyz[ixyz][1] = corners[iCorner].r * std::sin(phi);
          xyz[ixyz][2] = corners[iCorner].z;
          if (iSide == 0)   // startPhi
          {
            if (iCorner < numCorner - 1)
            {
              faces_vec[iface][0] = ixyz + 1;
              faces_vec[iface][1] = -(ixyz + numCorner + 1);
              faces_vec[iface][2] = ixyz + numCorner + 2;
              faces_vec[iface][3] = ixyz + 2;
            }
            else
            {
              faces_vec[iface][0] = ixyz + 1;
              faces_vec[iface][1] = -(ixyz + numCorner + 1);
              faces_vec[iface][2] = ixyz + 2;
              faces_vec[iface][3] = ixyz - numCorner + 2;
            }
          }
          else if (iSide == numSide - 1)   // endPhi
          {
            if (iCorner < numCorner - 1)
              {
                faces_vec[iface][0] = ixyz + 1;
                faces_vec[iface][1] = ixyz + numCorner + 1;
                faces_vec[iface][2] = ixyz + numCorner + 2;
                faces_vec[iface][3] = -(ixyz + 2);
              }
            else
              {
                faces_vec[iface][0] = ixyz + 1;
                faces_vec[iface][1] = ixyz + numCorner + 1;
                faces_vec[iface][2] = ixyz + 2;
                faces_vec[iface][3] = -(ixyz - numCorner + 2);
              }
          }
          else
          {
            if (iCorner < numCorner - 1)
              {
                faces_vec[iface][0] = ixyz + 1;
                faces_vec[iface][1] = -(ixyz + numCorner + 1);
                faces_vec[iface][2] = ixyz + numCorner + 2;
                faces_vec[iface][3] = -(ixyz + 2);
              }
              else
              {
                faces_vec[iface][0] = ixyz + 1;
                faces_vec[iface][1] = -(ixyz + numCorner + 1);
                faces_vec[iface][2] = ixyz + 2;
                faces_vec[iface][3] = -(ixyz - numCorner + 2);
              }
            }
            ++iface;
            ++ixyz;
        }
        phi += dPhi;
      }

      // Last corners...

      for (G4int iCorner = 0; iCorner < numCorner; ++iCorner)
      {
        xyz[ixyz][0] = corners[iCorner].r * std::cos(phi);
        xyz[ixyz][1] = corners[iCorner].r * std::sin(phi);
        xyz[ixyz][2] = corners[iCorner].z;
        ++ixyz;
      }
    }
    else  // !phiIsOpen - i.e., a complete 360 degrees.
    {
      nNodes = numSide * numCorner;
      nFaces = numSide * numCorner;;
      xyz = new double3[nNodes];
      faces_vec = new int4[nFaces];
      const G4double dPhi = (endPhi - startPhi) / numSide;
      G4double phi = startPhi;
      G4int ixyz = 0, iface = 0;
      for (G4int iSide = 0; iSide < numSide; ++iSide)
      {
        for (G4int iCorner = 0; iCorner < numCorner; ++iCorner)
        {
          xyz[ixyz][0] = corners[iCorner].r * std::cos(phi);
          xyz[ixyz][1] = corners[iCorner].r * std::sin(phi);
          xyz[ixyz][2] = corners[iCorner].z;

          if (iSide < numSide - 1)
          {
            if (iCorner < numCorner - 1)
            {
              faces_vec[iface][0] = ixyz + 1;
              faces_vec[iface][1] = -(ixyz + numCorner + 1);
              faces_vec[iface][2] = ixyz + numCorner + 2;
              faces_vec[iface][3] = -(ixyz + 2);
            }
            else
            {
              faces_vec[iface][0] = ixyz + 1;
              faces_vec[iface][1] = -(ixyz + numCorner + 1);
              faces_vec[iface][2] = ixyz + 2;
              faces_vec[iface][3] = -(ixyz - numCorner + 2);
            }
          }
          else   // Last side joins ends...
          {
            if (iCorner < numCorner - 1)
            {
              faces_vec[iface][0] = ixyz + 1;
              faces_vec[iface][1] = -(ixyz + numCorner - nFaces + 1);
              faces_vec[iface][2] = ixyz + numCorner - nFaces + 2;
              faces_vec[iface][3] = -(ixyz + 2);
            }
            else
            {
              faces_vec[iface][0] = ixyz + 1;
              faces_vec[iface][1] = -(ixyz - nFaces + numCorner + 1);
              faces_vec[iface][2] = ixyz - nFaces + 2;
              faces_vec[iface][3] = -(ixyz - numCorner + 2);
            }
          }
          ++ixyz;
          ++iface;
        }
        phi += dPhi;
      }
    }
    G4Polyhedron* polyhedron = new G4Polyhedron;
    G4int problem = polyhedron->createPolyhedron(nNodes, nFaces, xyz, faces_vec);
    delete faces_vec;
    delete xyz;
    if (problem)
    {
      std::ostringstream oss;
      oss << "Problem creating G4Polyhedron for: " << GetName();
 /*     G4Exception("G4Polycone::CreatePolyhedron()", "BadPolyhedron",
                  JustWarning, oss.str().c_str());*/
      delete polyhedron;
      return 0;
    }
    else
    {
      return polyhedron;
    }
  }
}


//
// CreateNURBS
//
G4NURBS *G4Polycone::CreateNURBS() const
{
  return 0;
}


//
// G4PolyconeHistorical stuff
//

G4PolyconeHistorical::G4PolyconeHistorical()
  : Z_values(0), Rmin(0), Rmax(0)
{
}

G4PolyconeHistorical::~G4PolyconeHistorical()
{
  delete [] Z_values;
  delete [] Rmin;
  delete [] Rmax;
}

G4PolyconeHistorical::
G4PolyconeHistorical( const G4PolyconeHistorical &source )
{
  Start_angle   = source.Start_angle;
  Opening_angle = source.Opening_angle;
  Num_z_planes  = source.Num_z_planes;
  
  Z_values  = new G4double[Num_z_planes];
  Rmin      = new G4double[Num_z_planes];
  Rmax      = new G4double[Num_z_planes];
  
  for( G4int i = 0; i < Num_z_planes; i++)
  {
    Z_values[i] = source.Z_values[i];
    Rmin[i]     = source.Rmin[i];
    Rmax[i]     = source.Rmax[i];
  }
}

G4PolyconeHistorical&
G4PolyconeHistorical::operator=( const G4PolyconeHistorical& right )
{
  if ( &right == this ) return *this;

  if (&right)
  {
    Start_angle   = right.Start_angle;
    Opening_angle = right.Opening_angle;
    Num_z_planes  = right.Num_z_planes;
  
    delete [] Z_values;
    delete [] Rmin;
    delete [] Rmax;
    Z_values  = new G4double[Num_z_planes];
    Rmin      = new G4double[Num_z_planes];
    Rmax      = new G4double[Num_z_planes];
  
    for( G4int i = 0; i < Num_z_planes; i++)
    {
      Z_values[i] = right.Z_values[i];
      Rmin[i]     = right.Rmin[i];
      Rmax[i]     = right.Rmax[i];
    }
  }
  return *this;
}
