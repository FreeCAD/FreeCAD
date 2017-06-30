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
// $Id: G4DisplacedSolid.cc,v 1.27 2006/06/29 18:43:41 gunter Exp $
// GEANT4 tag $Name: geant4-09-03-patch-01 $
//
// Implementation for G4DisplacedSolid class for boolean 
// operations between other solids
//
// History:
//
// 28.10.98 V.Grichine: created
// 14.11.99 V.Grichine: modifications in CalculateExtent(...) method
// 22.11.00 V.Grichine: new set methods for matrix/vectors
//
// --------------------------------------------------------------------


/*#include "G4VoxelLimits.hh"

#include "G4VPVParameterisation.hh"

#include "G4VGraphicsScene.hh"
#include "G4Polyhedron.hh"
#include "G4NURBS.hh"
// #include "G4NURBSbox.hh"
*/
///#include "G4VoxelLimits.hh"

#include "../../management/G4VPVParameterisation.hh"

///#include "G4VGraphicsScene.hh"
#include "../../../graphics_reps/G4Polyhedron.hh"
///#include "G4NURBS.hh"
// #include "G4NURBSbox.hh"

//BEGIN CAD-GDML
#include "G4DisplacedSolid.hh"
//END CAD-GDML

////////////////////////////////////////////////////////////////
//
// Constructor for transformation like rotation of frame then translation 
// in new frame. It is similar to 1st constractor in G4PVPlacement

G4DisplacedSolid::G4DisplacedSolid( const G4String& pName,
                                          G4VSolid* pSolid ,
                                          G4RotationMatrix* rotMatrix,
                                    const G4ThreeVector& transVector    )
  : G4VSolid(pName), fpPolyhedron(0)
{
  fPtrSolid = pSolid ;
  fPtrTransform = new G4AffineTransform(rotMatrix,transVector) ;
  fPtrTransform->Invert() ;
  fDirectTransform = new G4AffineTransform(rotMatrix,transVector) ;
}

/////////////////////////////////////////////////////////////////////////////////
//
// Constructor

G4DisplacedSolid::G4DisplacedSolid( const G4String& pName,
                                          G4VSolid* pSolid ,
                                    const G4Transform3D& transform  )
  : G4VSolid(pName), fpPolyhedron(0)
{
  fPtrSolid = pSolid ;
  fDirectTransform = new G4AffineTransform(transform.getRotation().inverse(),
                                           transform.getTranslation()) ;

  fPtrTransform    = new G4AffineTransform(transform.getRotation().inverse(),
                                           transform.getTranslation()) ;
  fPtrTransform->Invert() ;
}

///////////////////////////////////////////////////////////////////
//
// Constructor for use with creation of Transient object
// from Persistent object

G4DisplacedSolid::G4DisplacedSolid( const G4String& pName,
                                          G4VSolid* pSolid ,
                                    const G4AffineTransform directTransform )
  : G4VSolid(pName), fpPolyhedron(0)
{
  fPtrSolid = pSolid ;
  fDirectTransform = new G4AffineTransform( directTransform );
  fPtrTransform    = new G4AffineTransform( directTransform.Inverse() ) ; 
}

///////////////////////////////////////////////////////////////////
//
// Fake default constructor - sets only member data and allocates memory
//                            for usage restricted to object persistency.

G4DisplacedSolid::G4DisplacedSolid( __void__& a )
  : G4VSolid(a), fPtrSolid(0), fPtrTransform(0),
    fDirectTransform(0), fpPolyhedron(0)
{
}

///////////////////////////////////////////////////////////////////
//
// Destructor

G4DisplacedSolid::~G4DisplacedSolid() 
{
  CleanTransformations();
  delete fpPolyhedron;
}

G4GeometryType G4DisplacedSolid::GetEntityType() const 
{
  return G4String("G4DisplacedSolid");
}

void G4DisplacedSolid::CleanTransformations()
{
  if(fPtrTransform)
  {
    delete fPtrTransform;  fPtrTransform=0;
    delete fDirectTransform;  fDirectTransform=0;
  }
}

const G4DisplacedSolid* G4DisplacedSolid::GetDisplacedSolidPtr() const   
{
  return this;
}

G4DisplacedSolid* G4DisplacedSolid::GetDisplacedSolidPtr() 
{
  return this;
}

G4VSolid* G4DisplacedSolid::GetConstituentMovedSolid() const
{ 
  return fPtrSolid; 
} 

/////////////////////////////////////////////////////////////////////////////

G4AffineTransform  G4DisplacedSolid::GetTransform() const
{
  G4AffineTransform aTransform = *fPtrTransform;
  return aTransform;
}

void G4DisplacedSolid::SetTransform(G4AffineTransform& transform) 
{
  fPtrTransform = &transform ;
  fpPolyhedron = 0;
}

//////////////////////////////////////////////////////////////////////////////

G4AffineTransform  G4DisplacedSolid::GetDirectTransform() const
{
  G4AffineTransform aTransform= *fDirectTransform;
  return aTransform;
}

void G4DisplacedSolid::SetDirectTransform(G4AffineTransform& transform) 
{
  fDirectTransform = &transform ;
  fpPolyhedron = 0;
}

/////////////////////////////////////////////////////////////////////////////

G4RotationMatrix G4DisplacedSolid::GetFrameRotation() const
{
  G4RotationMatrix InvRotation= fDirectTransform->NetRotation();
  return InvRotation;
}

void G4DisplacedSolid::SetFrameRotation(const G4RotationMatrix& matrix)
{
  fDirectTransform->SetNetRotation(matrix);
  fpPolyhedron = 0;
}

/////////////////////////////////////////////////////////////////////////////

G4ThreeVector  G4DisplacedSolid::GetFrameTranslation() const
{
  return fPtrTransform->NetTranslation();
}

void G4DisplacedSolid::SetFrameTranslation(const G4ThreeVector& vector)
{
  fPtrTransform->SetNetTranslation(vector);
  fpPolyhedron = 0;
}

///////////////////////////////////////////////////////////////

G4RotationMatrix G4DisplacedSolid::GetObjectRotation() const
{
  G4RotationMatrix Rotation= fPtrTransform->NetRotation();
  return Rotation;
}

void G4DisplacedSolid::SetObjectRotation(const G4RotationMatrix& matrix)
{
  fPtrTransform->SetNetRotation(matrix);
  fpPolyhedron = 0;
}

///////////////////////////////////////////////////////////////////////

G4ThreeVector  G4DisplacedSolid::GetObjectTranslation() const
{
  return fDirectTransform->NetTranslation();
}

void G4DisplacedSolid::SetObjectTranslation(const G4ThreeVector& vector)
{
  fDirectTransform->SetNetTranslation(vector);
  fpPolyhedron = 0;
}

///////////////////////////////////////////////////////////////
//
//
     
G4bool 
G4DisplacedSolid::CalculateExtent( const EAxis pAxis,
                                   const G4VoxelLimits& pVoxelLimit,
                                   const G4AffineTransform& pTransform,
                                         G4double& pMin, 
                                         G4double& pMax           ) const 
{
  G4AffineTransform sumTransform ;
  sumTransform.Product(*fDirectTransform,pTransform) ;
  return fPtrSolid->CalculateExtent(pAxis,pVoxelLimit,sumTransform,pMin,pMax) ;
}
 
/////////////////////////////////////////////////////
//
// 

EInside G4DisplacedSolid::Inside(const G4ThreeVector& p) const
{
  G4ThreeVector newPoint = fPtrTransform->TransformPoint(p) ;
  return fPtrSolid->Inside(newPoint) ; 
}

//////////////////////////////////////////////////////////////
//
//

G4ThreeVector 
G4DisplacedSolid::SurfaceNormal( const G4ThreeVector& p ) const 
{
  G4ThreeVector newPoint = fPtrTransform->TransformPoint(p) ;
  G4ThreeVector normal = fPtrSolid->SurfaceNormal(newPoint) ; 
  return fDirectTransform->TransformAxis(normal) ;
}

/////////////////////////////////////////////////////////////
//
// The same algorithm as in DistanceToIn(p)

G4double 
G4DisplacedSolid::DistanceToIn( const G4ThreeVector& p,
                                const G4ThreeVector& v  ) const 
{    
  G4ThreeVector newPoint = fPtrTransform->TransformPoint(p) ;
  G4ThreeVector newDirection = fPtrTransform->TransformAxis(v) ;
  return fPtrSolid->DistanceToIn(newPoint,newDirection) ;   
}

////////////////////////////////////////////////////////
//
// Approximate nearest distance from the point p to the intersection of
// two solids

G4double 
G4DisplacedSolid::DistanceToIn( const G4ThreeVector& p ) const 
{
  G4ThreeVector newPoint = fPtrTransform->TransformPoint(p) ;
  return fPtrSolid->DistanceToIn(newPoint) ;   
}

//////////////////////////////////////////////////////////
//
// The same algorithm as DistanceToOut(p)

G4double 
G4DisplacedSolid::DistanceToOut( const G4ThreeVector& p,
                                 const G4ThreeVector& v,
                                 const G4bool calcNorm,
                                       G4bool *validNorm,
                                       G4ThreeVector *n   ) const 
{
  G4ThreeVector solNorm ; 
  G4ThreeVector newPoint = fPtrTransform->TransformPoint(p) ;
  G4ThreeVector newDirection = fPtrTransform->TransformAxis(v) ;
  G4double dist = fPtrSolid->DistanceToOut(newPoint,newDirection,
                                           calcNorm,validNorm,&solNorm) ;
  if(calcNorm)
  { 
    *n = fDirectTransform->TransformAxis(solNorm) ;
  }
  return dist ;  
}

//////////////////////////////////////////////////////////////
//
// Inverted algorithm of DistanceToIn(p)

G4double 
G4DisplacedSolid::DistanceToOut( const G4ThreeVector& p ) const 
{
  G4ThreeVector newPoint = fPtrTransform->TransformPoint(p) ;
  return fPtrSolid->DistanceToOut(newPoint) ;   
}

//////////////////////////////////////////////////////////////
//
//

void 
G4DisplacedSolid::ComputeDimensions(       G4VPVParameterisation*,
                                     const G4int,
                                     const G4VPhysicalVolume* ) 
{
/*  DumpInfo();
  G4Exception("G4DisplacedSolid::ComputeDimensions()",
                "NotApplicable", FatalException,
                "Method not applicable in this context!");
				*/
}

//////////////////////////////////////////////////////////////////////////
//
// Returns a point (G4ThreeVector) randomly and uniformly selected
// on the solid surface
//
/*
G4ThreeVector G4DisplacedSolid::GetPointOnSurface() const
{
  G4ThreeVector p =  fPtrSolid->GetPointOnSurface();
  return fDirectTransform->TransformPoint(p);
}
*/
//////////////////////////////////////////////////////////////////////////
//
// Stream object contents to an output stream

std::ostream& G4DisplacedSolid::StreamInfo(std::ostream& os) const
{
	/*
  os << "-----------------------------------------------------------\n"
     << "    *** Dump for Displaced solid - " << GetName() << " ***\n"
     << "    ===================================================\n"
     << " Solid type: " << GetEntityType() << "\n"
     << " Parameters of constituent solid: \n"
     << "===========================================================\n";
  fPtrSolid->StreamInfo(os);
  os << "===========================================================\n"
     << " Transformations: \n"
     << "    Direct transformation - translation : \n"
     << "           " << fDirectTransform->NetTranslation() << "\n"
     << "                          - rotation    : \n"
     << "           ";
  fDirectTransform->NetRotation().print(os);
  os << "\n"
     << "===========================================================\n";
*/
  return os;
}

//////////////////////////////////////////////////////////////////////////
//
//                    

void 
G4DisplacedSolid::DescribeYourselfTo ( G4VGraphicsScene& scene ) const 
{
///  scene.AddSolid (*this);
}

//////////////////////////////////////////////////////////////////////////
//
//

G4Polyhedron* 
G4DisplacedSolid::CreatePolyhedron () const 
{
  G4Polyhedron* polyhedron = fPtrSolid->CreatePolyhedron();
  polyhedron
    ->Transform(G4Transform3D(GetObjectRotation(),GetObjectTranslation()));
  return polyhedron;
}

//////////////////////////////////////////////////////////////////////////
//
//

G4NURBS*      
G4DisplacedSolid::CreateNURBS () const 
{
  // Take into account local transformation - see CreatePolyhedron.
  // return fPtrSolid->CreateNURBS() ;
  return 0;
}

//////////////////////////////////////////////////////////////////////////
//
//

G4Polyhedron* G4DisplacedSolid::GetPolyhedron () const
{
  if (!fpPolyhedron ||
      fpPolyhedron->GetNumberOfRotationStepsAtTimeOfCreation() !=
      fpPolyhedron->GetNumberOfRotationSteps())
    {
      delete fpPolyhedron;
      fpPolyhedron = CreatePolyhedron();
    }
  return fpPolyhedron;
}
