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
// the GEANT4 collaboration.
//
// By copying, distributing or modifying the Program (or any work
// based on the Program) you indicate your acceptance of this statement,
// and all its terms.
//
// $Id: G4VCSGfaceted.cc,v 1.26 2009/05/08 14:29:56 gcosmo Exp $
// GEANT4 tag $Name: geant4-09-03-patch-01 $
//
// 
// --------------------------------------------------------------------
// GEANT 4 class source file
//
//
// G4VCSGfaceted.cc
//
// Implementation of the virtual class of a CSG type shape that is built
// entirely out of G4VCSGface faces.
//
// --------------------------------------------------------------------

#include "G4VCSGfaceted.hh"
#include "G4VCSGface.hh"
///#include "G4SolidExtentList.hh"

///#include "G4VoxelLimits.hh"
///#include "G4AffineTransform.hh"
#include "../../management/G4VoxelLimits.hh"
#include "../../management/G4AffineTransform.hh"

//#include "Randomize.hh"

///#include "G4Polyhedron.hh"   
#include "../../../graphics_reps/G4Polyhedron.hh"   
///#include "G4VGraphicsScene.hh"
///#include "G4NURBS.hh"
///#include "G4NURBSbox.hh"
///#include "G4VisExtent.hh"

//
// Constructor
//
G4VCSGfaceted::G4VCSGfaceted( const G4String& name )
  : G4VSolid(name),
    numFace(0), faces(0), fCubicVolume(0.), fSurfaceArea(0.), fpPolyhedron(0),
    fStatistics(1000000), fCubVolEpsilon(0.001), fAreaAccuracy(-1.)
{
}


//
// Fake default constructor - sets only member data and allocates memory
//                            for usage restricted to object persistency.
//
G4VCSGfaceted::G4VCSGfaceted( __void__& a )
  : G4VSolid(a),
    numFace(0), faces(0), fCubicVolume(0.), fSurfaceArea(0.), fpPolyhedron(0),
    fStatistics(1000000), fCubVolEpsilon(0.001), fAreaAccuracy(-1.)
{
}

//
// Destructor
//
G4VCSGfaceted::~G4VCSGfaceted()
{
  DeleteStuff();
  delete fpPolyhedron;
}


//
// Copy constructor
//
G4VCSGfaceted::G4VCSGfaceted( const G4VCSGfaceted &source )
  : G4VSolid( source )
{
  CopyStuff( source );
}


//
// Assignment operator
//
const G4VCSGfaceted &G4VCSGfaceted::operator=( const G4VCSGfaceted &source )
{
  if (&source == this) { return *this; }
  
  DeleteStuff();
  CopyStuff( source );
  
  return *this;
}


//
// CopyStuff (protected)
//
// Copy the contents of source
//
void G4VCSGfaceted::CopyStuff( const G4VCSGfaceted &source )
{
  numFace = source.numFace;
  if (numFace == 0) { return; }    // odd, but permissable?
  
  faces = new G4VCSGface*[numFace];
  
  G4VCSGface **face = faces,
       **sourceFace = source.faces;
  do
  {
    *face = (*sourceFace)->Clone();
  } while( ++sourceFace, ++face < faces+numFace );
  fCubicVolume = source.fCubicVolume;
  fpPolyhedron = source.fpPolyhedron;
}


//
// DeleteStuff (protected)
//
// Delete all allocated objects
//
void G4VCSGfaceted::DeleteStuff()
{
  if (numFace)
  {
    G4VCSGface **face = faces;
    do
    {
      delete *face;
    } while( ++face < faces + numFace );

    delete [] faces;
  }
}


//
// CalculateExtent
//
G4bool G4VCSGfaceted::CalculateExtent( const EAxis axis,
                                       const G4VoxelLimits &voxelLimit,
                                       const G4AffineTransform &transform,
                                             G4double &min,
                                             G4double &max ) const
{/*
  G4SolidExtentList  extentList( axis, voxelLimit );

  //
  // Loop over all faces, checking min/max extent as we go.
  //
  G4VCSGface **face = faces;
  do
  {
    (*face)->CalculateExtent( axis, voxelLimit, transform, extentList );
  } while( ++face < faces + numFace );
  
  //
  // Return min/max value
  //
  return extentList.GetExtent( min, max );
  */return true;
}


//
// Inside
//
// It could be a good idea to override this virtual
// member to add first a simple test (such as spherical
// test or whatnot) and to call this version only if
// the simplier test fails.
//
EInside G4VCSGfaceted::Inside( const G4ThreeVector &p ) const
{
  EInside answer=kOutside;
  G4VCSGface **face = faces;
  G4double best = kInfinity;
  do
  {
    G4double distance;
    EInside result = (*face)->Inside( p, kCarTolerance/2, &distance );
    if (result == kSurface) { return kSurface; }
    if (distance < best)
    {
      best = distance;
      answer = result;
    }
  } while( ++face < faces + numFace );

  return answer;
}


//
// SurfaceNormal
//
G4ThreeVector G4VCSGfaceted::SurfaceNormal( const G4ThreeVector& p ) const
{
  G4ThreeVector answer;
  G4VCSGface **face = faces;
  G4double best = kInfinity;
  do
  {
    G4double distance;
    G4ThreeVector normal = (*face)->Normal( p, &distance );
    if (distance < best)
    {
      best = distance;
      answer = normal;
    }
  } while( ++face < faces + numFace );

  return answer;
}


//
// DistanceToIn(p,v)
//
G4double G4VCSGfaceted::DistanceToIn( const G4ThreeVector &p,
                                      const G4ThreeVector &v ) const
{
  G4double distance = kInfinity;
  G4double distFromSurface = kInfinity;
  G4VCSGface *bestFace=0;
  G4VCSGface **face = faces;
  do
  {
    G4double   faceDistance,
               faceDistFromSurface;
    G4ThreeVector   faceNormal;
    G4bool    faceAllBehind;
    if ((*face)->Intersect( p, v, false, kCarTolerance/2,
                faceDistance, faceDistFromSurface,
                faceNormal, faceAllBehind ) )
    {
      //
      // Intersecting face
      //
      if (faceDistance < distance)
      {
        distance = faceDistance;
        distFromSurface = faceDistFromSurface;
        bestFace = *face;
        if (distFromSurface <= 0) { return 0; }
      }
    }
  } while( ++face < faces + numFace );
  
  if (distance < kInfinity && distFromSurface<kCarTolerance/2)
  {
    if (bestFace->Distance(p,false) < kCarTolerance/2)  { distance = 0; }
  }

  return distance;
}


//
// DistanceToIn(p)
//
G4double G4VCSGfaceted::DistanceToIn( const G4ThreeVector &p ) const
{
  return DistanceTo( p, false );
}


//
// DistanceToOut(p,v)
//
G4double G4VCSGfaceted::DistanceToOut( const G4ThreeVector &p,
                                       const G4ThreeVector &v,
                                       const G4bool calcNorm,
                                             G4bool *validNorm,
                                             G4ThreeVector *n ) const
{
  G4bool allBehind = true;
  G4double distance = kInfinity;
  G4double distFromSurface = kInfinity;
  G4ThreeVector normal;
  G4VCSGface *bestFace=0;
  
  G4VCSGface **face = faces;
  do
  {
    G4double  faceDistance,
              faceDistFromSurface;
    G4ThreeVector  faceNormal;
    G4bool    faceAllBehind;
    if ((*face)->Intersect( p, v, true, kCarTolerance/2,
                faceDistance, faceDistFromSurface,
                faceNormal, faceAllBehind ) )
    {
      //
      // Intersecting face
      //
      if ( (distance < kInfinity) || (!faceAllBehind) )  { allBehind = false; }
      if (faceDistance < distance)
      {
        distance = faceDistance;
        distFromSurface = faceDistFromSurface;
        normal = faceNormal;
        bestFace = *face;
        if (distFromSurface <= 0)  { break; }
      }
    }
  } while( ++face < faces + numFace );
  
  if (distance < kInfinity)
  {
    if (distFromSurface <= 0)
    {
      distance = 0;
    }
    else if (distFromSurface<kCarTolerance/2)
    {
      if (bestFace->Distance(p,true) < kCarTolerance/2)  { distance = 0; }
    }

    if (calcNorm)
    {
      *validNorm = allBehind;
      *n = normal;
    }
  }
  else
  { 
    if (Inside(p) == kSurface)  { distance = 0; }
    if (calcNorm)  { *validNorm = false; }
  }

  return distance;
}


//
// DistanceToOut(p)
//
G4double G4VCSGfaceted::DistanceToOut( const G4ThreeVector &p ) const
{
  return DistanceTo( p, true );
}


//
// DistanceTo
//
// Protected routine called by DistanceToIn and DistanceToOut
//
G4double G4VCSGfaceted::DistanceTo( const G4ThreeVector &p,
                                    const G4bool outgoing ) const
{
  G4VCSGface **face = faces;
  G4double best = kInfinity;
  do
  {
    G4double distance = (*face)->Distance( p, outgoing );
    if (distance < best)  { best = distance; }
  } while( ++face < faces + numFace );

  return (best < 0.5*kCarTolerance) ? 0 : best;
}


//
// DescribeYourselfTo
//
void G4VCSGfaceted::DescribeYourselfTo( G4VGraphicsScene& scene ) const
{
//   scene.AddSolid( *this );
}

/*
//
// GetExtent
//
// Define the sides of the box into which our solid instance would fit.
//
G4VisExtent G4VCSGfaceted::GetExtent() const 
{
  static const G4ThreeVector xMax(1,0,0), xMin(-1,0,0),
                             yMax(0,1,0), yMin(0,-1,0),
                             zMax(0,0,1), zMin(0,0,-1);
  static const G4ThreeVector *axes[6] =
     { &xMin, &xMax, &yMin, &yMax, &zMin, &zMax };
  
  G4double answers[6] =
     {-kInfinity, -kInfinity, -kInfinity, -kInfinity, -kInfinity, -kInfinity};

  G4VCSGface **face = faces;
  do
  {    
    const G4ThreeVector **axis = axes+5 ;
    G4double *answer = answers+5;
    do
    {
      G4double testFace = (*face)->Extent( **axis );
      if (testFace > *answer)  { *answer = testFace; }
    }
    while( --axis, --answer >= answers );
    
  } while( ++face < faces + numFace );
  
    return G4VisExtent( -answers[0], answers[1], 
                        -answers[2], answers[3],
                        -answers[4], answers[5]  );
}
*/

//
// GetEntityType
//
G4GeometryType G4VCSGfaceted::GetEntityType() const
{
  return G4String("G4CSGfaceted");
}


//
// Stream object contents to an output stream
//
std::ostream& G4VCSGfaceted::StreamInfo( std::ostream& os ) const
{
  os << "-----------------------------------------------------------\n"
     << "    *** Dump for solid - " << GetName() << " ***\n"
     << "    ===================================================\n"
     << " Solid type: G4VCSGfaceted\n"
     << " Parameters: \n"
     << "    number of faces: " << numFace << "\n"
     << "-----------------------------------------------------------\n";

  return os;
}


//
// GetCubVolStatistics
//
G4int G4VCSGfaceted::GetCubVolStatistics() const
{
  return fStatistics;
}


//
// GetCubVolEpsilon
//
G4double G4VCSGfaceted::GetCubVolEpsilon() const
{
  return fCubVolEpsilon;
}


//
// SetCubVolStatistics
//
void G4VCSGfaceted::SetCubVolStatistics(G4int st)
{
  fCubicVolume=0.;
  fStatistics=st;
}


//
// SetCubVolEpsilon
//
void G4VCSGfaceted::SetCubVolEpsilon(G4double ep)
{
  fCubicVolume=0.;
  fCubVolEpsilon=ep;
}


//
// GetAreaStatistics
//
G4int G4VCSGfaceted::GetAreaStatistics() const
{
  return fStatistics;
}


//
// GetAreaAccuracy
//
G4double G4VCSGfaceted::GetAreaAccuracy() const
{
  return fAreaAccuracy;
}


//
// SetAreaStatistics
//
void G4VCSGfaceted::SetAreaStatistics(G4int st)
{
  fSurfaceArea=0.;
  fStatistics=st;
}


//
// SetAreaAccuracy
//
void G4VCSGfaceted::SetAreaAccuracy(G4double ep)
{
  fSurfaceArea=0.;
  fAreaAccuracy=ep;
}


//
// GetCubicVolume
//
G4double G4VCSGfaceted::GetCubicVolume()
{
  if(fCubicVolume != 0.) {;}
  else   { fCubicVolume = EstimateCubicVolume(fStatistics,fCubVolEpsilon); }
  return fCubicVolume;
}


//
// GetSurfaceArea
//
G4double G4VCSGfaceted::GetSurfaceArea()
{
  if(fSurfaceArea != 0.) {;}
  else   { fSurfaceArea = EstimateSurfaceArea(fStatistics,fAreaAccuracy); }
  return fSurfaceArea;
}


//
// GetPolyhedron
//
G4Polyhedron* G4VCSGfaceted::GetPolyhedron () const
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


//
// GetPointOnSurfaceGeneric proportional to Areas of faces
// in case of GenericPolycone or GenericPolyhedra
//
G4ThreeVector G4VCSGfaceted::GetPointOnSurfaceGeneric( ) const
{
  // Preparing variables
  //
  G4ThreeVector answer=G4ThreeVector(0.,0.,0.);
  G4VCSGface **face = faces;
  G4double area = 0;
  G4int i;
  std::vector<G4double> areas; 

  // First step: calculate surface areas
  //
  do
  {
    G4double result = (*face)->SurfaceArea( );
    areas.push_back(result);
    area=area+result;
  } while( ++face < faces + numFace );

  // Second Step: choose randomly one surface
  //
  G4VCSGface **face1 = faces;
//  G4double chose = area*G4UniformRand();//rand()/(double)RAND_MAX;
  G4double chose = area*rand()/(double)RAND_MAX;
  G4double Achose1, Achose2;
  Achose1=0; Achose2=0.; 
  i=0;

  do
  {
    Achose2+=areas[i];
    if(chose>=Achose1 && chose<Achose2)
    {
      G4ThreeVector point;
      point= (*face1)->GetPointOnFace();
      return point;
    }
    i++;
    Achose1=Achose2;
  } while( ++face1 < faces + numFace );

  return answer;
}
