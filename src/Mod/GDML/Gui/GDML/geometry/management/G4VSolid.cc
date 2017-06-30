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
// $Id: G4VSolid.cc,v 1.39 2008/09/23 13:07:41 gcosmo Exp $
// GEANT4 tag $Name: geant4-09-03-patch-01 $
//
// class G4VSolid
//
// Implementation for solid base class
//
// History:
//
//  06.12.02 V.Grichine, restored original conditions in ClipPolygon()
//  10.05.02 V.Grichine, ClipPolygon(): clip only other axis and limited voxels
//  15.04.02 V.Grichine, bug fixed in ClipPolygon(): clip only one axis
//  13.03.02 V.Grichine, cosmetics of voxel limit functions  
//  15.11.00 D.Williams, V.Grichine, fix in CalculateClippedPolygonExtent()
//  10.07.95 P.Kent, Added == operator, solid Store entry
//  30.06.95 P.Kent, Created.
// --------------------------------------------------------------------
//#include "../../../PreCompiled.h"
//#include <Base/Console.h>

///#include "G4VSolid.hh"
///#include "G4SolidStore.hh"
///#include "globals.hh"
#include "G4VSolid.hh"
#include "G4SolidStore.hh"
#include "../../global/globals.hh"
/*
#include "Randomize.hh"
#include "G4GeometryTolerance.hh"

#include "G4VoxelLimits.hh"
#include "G4AffineTransform.hh"
#include "G4VisExtent.hh"
*/
///#include "Randomize.hh"
#include "../../global/G4GeometryTolerance.hh"

#include "G4VoxelLimits.hh"
#include "G4AffineTransform.hh"
///#include "G4VisExtent.hh"

//////////////////////////////////////////////////////////////////////////
//
// Constructor
//  - Copies name
//  - Add ourselves to solid Store

G4VSolid::G4VSolid(const G4String& name)
  : fshapeName(name)
{
 ///   kCarTolerance = G4GeometryTolerance::GetInstance()->GetSurfaceTolerance();
	//GDML very bad
	//kCarTolerance = 0.001;
	//Base::Console().Message("G4VSolid=%s\n",name.c_str());
	//Base::Console().Message("G4VSolid=%s\n",fshapeName.c_str());

    // Register to store
    //
    G4SolidStore::GetInstance()->Register(this);
}
/**/
//////////////////////////////////////////////////////////////////////////
//
// Copy constructor
//

G4VSolid::G4VSolid(const G4VSolid& rhs)
  : kCarTolerance(rhs.kCarTolerance), fshapeName(rhs.fshapeName)
{
    // Register to store
    //
    G4SolidStore::GetInstance()->Register(this);
}
/**/
//////////////////////////////////////////////////////////////////////////
//
// Fake default constructor - sets only member data and allocates memory
//                            for usage restricted to object persistency.
//
G4VSolid::G4VSolid( __void__& )
  : fshapeName("")
{
    // Register to store
    //
    G4SolidStore::GetInstance()->Register(this);
}

//////////////////////////////////////////////////////////////////////////
//
// Destructor (virtual)
// - Remove ourselves from solid Store

G4VSolid::~G4VSolid()
{
    G4SolidStore::GetInstance()->DeRegister(this);
}
/*
//////////////////////////////////////////////////////////////////////////
//
// Assignment operator

G4VSolid& G4VSolid::operator = (const G4VSolid& rhs) 
{
   // Check assignment to self
   //
   if (this == &rhs)  { return *this; }

   // Copy data
   //
   kCarTolerance = rhs.kCarTolerance;
   fshapeName = rhs.fshapeName;

   return *this;
}  

//////////////////////////////////////////////////////////////////////////
//
// Streaming operator dumping solid contents

std::ostream& operator<< ( std::ostream& os, const G4VSolid& e )
{
    return e.StreamInfo(os);
}

//////////////////////////////////////////////////////////////////////////
//
// Throw exception if ComputeDimensions called for illegal derived class

void G4VSolid::ComputeDimensions(G4VPVParameterisation*,
                                 const G4int,
                                 const G4VPhysicalVolume*)
{
    G4cerr << "ERROR - Illegal call to G4VSolid::ComputeDimensions()" << G4endl
           << "        Method not overloaded by derived class !" << G4endl;
    G4Exception("G4VSolid::ComputeDimensions()", "NotApplicable",
                FatalException, "Illegal call to case class.");
}
*/
//////////////////////////////////////////////////////////////////////////
//
// Throw exception (warning) for solids not implementing the method
/*
G4ThreeVector G4VSolid::GetPointOnSurface() const
{
///   G4cerr << "WARNING - G4VSolid::GetPointOnSurface()" << G4endl
///           << "          Not implemented for solid: "
///           << this->GetEntityType() << " !" << G4endl;
///    G4Exception("G4VSolid::GetPointOnSurface()", "NotImplemented",
///        JustWarning, "Not implemented for this solid ! Returning origin.");
    return G4ThreeVector(0,0,0);
}*/

///////////////////////////////////////////////////////////////////////////
// 
// Calculate the maximum and minimum extents of the polygon described
// by the vertices: pSectionIndex->pSectionIndex+1->
//                   pSectionIndex+2->pSectionIndex+3->pSectionIndex
// in the List pVertices
//
// If the minimum is <pMin pMin is set to the new minimum
// If the maximum is >pMax pMax is set to the new maximum
//
// No modifications are made to pVertices
//

void G4VSolid::ClipCrossSection(       G4ThreeVectorList* pVertices,
                                 const G4int pSectionIndex,
                                 const G4VoxelLimits& pVoxelLimit,
                                 const EAxis pAxis, 
                                       G4double& pMin, G4double& pMax) const
{

  G4ThreeVectorList polygon;
  polygon.reserve(4);
  polygon.push_back((*pVertices)[pSectionIndex]);
  polygon.push_back((*pVertices)[pSectionIndex+1]);
  polygon.push_back((*pVertices)[pSectionIndex+2]);
  polygon.push_back((*pVertices)[pSectionIndex+3]);
  //  G4cout<<"ClipCrossSection: 0-1-2-3"<<G4endl;
  CalculateClippedPolygonExtent(polygon,pVoxelLimit,pAxis,pMin,pMax);
  return;
}

//////////////////////////////////////////////////////////////////////////////////
//
// Calculate the maximum and minimum extents of the polygons
// joining the CrossSections at pSectionIndex->pSectionIndex+3 and
//                              pSectionIndex+4->pSectionIndex7
//
// in the List pVertices, within the boundaries of the voxel limits pVoxelLimit
//
// If the minimum is <pMin pMin is set to the new minimum
// If the maximum is >pMax pMax is set to the new maximum
//
// No modifications are made to pVertices

void G4VSolid::ClipBetweenSections(      G4ThreeVectorList* pVertices,
                                   const G4int pSectionIndex,
                                   const G4VoxelLimits& pVoxelLimit,
                                   const EAxis pAxis, 
                                         G4double& pMin, G4double& pMax) const
{
  G4ThreeVectorList polygon;
  polygon.reserve(4);
  polygon.push_back((*pVertices)[pSectionIndex]);
  polygon.push_back((*pVertices)[pSectionIndex+4]);
  polygon.push_back((*pVertices)[pSectionIndex+5]);
  polygon.push_back((*pVertices)[pSectionIndex+1]);
  // G4cout<<"ClipBetweenSections: 0-4-5-1"<<G4endl;
  CalculateClippedPolygonExtent(polygon,pVoxelLimit,pAxis,pMin,pMax);
  polygon.clear();

  polygon.push_back((*pVertices)[pSectionIndex+1]);
  polygon.push_back((*pVertices)[pSectionIndex+5]);
  polygon.push_back((*pVertices)[pSectionIndex+6]);
  polygon.push_back((*pVertices)[pSectionIndex+2]);
  // G4cout<<"ClipBetweenSections: 1-5-6-2"<<G4endl;
  CalculateClippedPolygonExtent(polygon,pVoxelLimit,pAxis,pMin,pMax);
  polygon.clear();

  polygon.push_back((*pVertices)[pSectionIndex+2]);
  polygon.push_back((*pVertices)[pSectionIndex+6]);
  polygon.push_back((*pVertices)[pSectionIndex+7]);
  polygon.push_back((*pVertices)[pSectionIndex+3]);
  //  G4cout<<"ClipBetweenSections: 2-6-7-3"<<G4endl;
  CalculateClippedPolygonExtent(polygon,pVoxelLimit,pAxis,pMin,pMax);
  polygon.clear();

  polygon.push_back((*pVertices)[pSectionIndex+3]);
  polygon.push_back((*pVertices)[pSectionIndex+7]);
  polygon.push_back((*pVertices)[pSectionIndex+4]);
  polygon.push_back((*pVertices)[pSectionIndex]);
  //  G4cout<<"ClipBetweenSections: 3-7-4-0"<<G4endl;
  CalculateClippedPolygonExtent(polygon,pVoxelLimit,pAxis,pMin,pMax);
  return;
}


///////////////////////////////////////////////////////////////////////////////
//
// Calculate the maximum and minimum extents of the convex polygon pPolygon
// along the axis pAxis, within the limits pVoxelLimit
//

void
G4VSolid::CalculateClippedPolygonExtent(G4ThreeVectorList& pPolygon,
                                  const G4VoxelLimits& pVoxelLimit,
                                  const EAxis pAxis, 
                                        G4double& pMin,
                                        G4double& pMax) const
{
  G4int noLeft,i;
  G4double component;


  ClipPolygon(pPolygon,pVoxelLimit,pAxis);
  noLeft = pPolygon.size();

  if ( noLeft )
  {
    //  G4cout<<G4endl;
    for (i=0;i<noLeft;i++)
    {
      component = pPolygon[i].operator()(pAxis);
      //  G4cout <<i<<"\t"<<component<<G4endl;
 
      if (component < pMin) 
      { 
        //  G4cout <<i<<"\t"<<"Pmin = "<<component<<G4endl;
        pMin = component;      
      }
      if (component > pMax)
      {  
        //  G4cout <<i<<"\t"<<"PMax = "<<component<<G4endl;
        pMax = component;  
      }    
    }
    //  G4cout<<G4endl;
  }
  // G4cout<<"pMin = "<<pMin<<"\t"<<"pMax = "<<pMax<<G4endl;
}

/////////////////////////////////////////////////////////////////////////////
//
// Clip the convex polygon described by the vertices at
// pSectionIndex ->pSectionIndex+3 within pVertices to the limits pVoxelLimit
//
// Set pMin to the smallest
//
// Calculate the extent of the polygon along pAxis, when clipped to the
// limits pVoxelLimit. If the polygon exists after clippin, set pMin to
// the polygon's minimum extent along the axis if <pMin, and set pMax to
// the polygon's maximum extent along the axis if >pMax.
//
// The polygon is described by a set of vectors, where each vector represents
// a vertex, so that the polygon is described by the vertex sequence:
//   0th->1st 1st->2nd 2nd->... nth->0th
//
// Modifications to the polygon are made
//
// NOTE: Execessive copying during clipping

void G4VSolid::ClipPolygon(      G4ThreeVectorList& pPolygon,
                           const G4VoxelLimits& pVoxelLimit,
                           const EAxis                        ) const
{
  G4ThreeVectorList outputPolygon;

  if ( pVoxelLimit.IsLimited() )
  {
    if (pVoxelLimit.IsXLimited() ) // && pAxis != kXAxis)
    {
      G4VoxelLimits simpleLimit1;
      simpleLimit1.AddLimit(kXAxis,pVoxelLimit.GetMinXExtent(),kInfinity);
      //  G4cout<<"MinXExtent()"<<G4endl;
      ClipPolygonToSimpleLimits(pPolygon,outputPolygon,simpleLimit1);
   
      pPolygon.clear();

      if ( !outputPolygon.size() )  return;

      G4VoxelLimits simpleLimit2;
      //  G4cout<<"MaxXExtent()"<<G4endl;
      simpleLimit2.AddLimit(kXAxis,-kInfinity,pVoxelLimit.GetMaxXExtent());
      ClipPolygonToSimpleLimits(outputPolygon,pPolygon,simpleLimit2);

      if ( !pPolygon.size() )       return;
      else                          outputPolygon.clear();
    }
    if ( pVoxelLimit.IsYLimited() ) // && pAxis != kYAxis)
    {
      G4VoxelLimits simpleLimit1;
      simpleLimit1.AddLimit(kYAxis,pVoxelLimit.GetMinYExtent(),kInfinity);
      ClipPolygonToSimpleLimits(pPolygon,outputPolygon,simpleLimit1);

      // Must always clear pPolygon - for clip to simpleLimit2 and in case of
      // early exit

      pPolygon.clear();

      if ( !outputPolygon.size() )  return;

      G4VoxelLimits simpleLimit2;
      simpleLimit2.AddLimit(kYAxis,-kInfinity,pVoxelLimit.GetMaxYExtent());
      ClipPolygonToSimpleLimits(outputPolygon,pPolygon,simpleLimit2);

      if ( !pPolygon.size() )       return;
      else                          outputPolygon.clear();
    }
    if ( pVoxelLimit.IsZLimited() ) // && pAxis != kZAxis)
    {
      G4VoxelLimits simpleLimit1;
      simpleLimit1.AddLimit(kZAxis,pVoxelLimit.GetMinZExtent(),kInfinity);
      ClipPolygonToSimpleLimits(pPolygon,outputPolygon,simpleLimit1);

      // Must always clear pPolygon - for clip to simpleLimit2 and in case of
      // early exit

      pPolygon.clear();

      if ( !outputPolygon.size() )  return;

      G4VoxelLimits simpleLimit2;
      simpleLimit2.AddLimit(kZAxis,-kInfinity,pVoxelLimit.GetMaxZExtent());
      ClipPolygonToSimpleLimits(outputPolygon,pPolygon,simpleLimit2);

      // Return after final clip - no cleanup
    }
  }
}

////////////////////////////////////////////////////////////////////////////
//
// pVoxelLimits must be only limited along one axis, and either the maximum
// along the axis must be +kInfinity, or the minimum -kInfinity

void
G4VSolid::ClipPolygonToSimpleLimits( G4ThreeVectorList& pPolygon,
                                     G4ThreeVectorList& outputPolygon,
                               const G4VoxelLimits& pVoxelLimit       ) const
{
  G4int i;
  G4int noVertices=pPolygon.size();
  G4ThreeVector vEnd,vStart;

  for (i = 0 ; i < noVertices ; i++ )
  {
    vStart = pPolygon[i];
    // G4cout << "i = " << i << G4endl;
    if ( i == noVertices-1 )    vEnd = pPolygon[0];
    else                        vEnd = pPolygon[i+1];

    if ( pVoxelLimit.Inside(vStart) )
    {
      if (pVoxelLimit.Inside(vEnd))
      {
        // vStart and vEnd inside -> output end point
        //
        outputPolygon.push_back(vEnd);
      }
      else
      {
        // vStart inside, vEnd outside -> output crossing point
        //
        // G4cout << "vStart inside, vEnd outside" << G4endl;
        pVoxelLimit.ClipToLimits(vStart,vEnd);
        outputPolygon.push_back(vEnd);
      }    
    }
    else
    {
      if (pVoxelLimit.Inside(vEnd))
      {
        // vStart outside, vEnd inside -> output inside section
        //
        // G4cout << "vStart outside, vEnd inside" << G4endl;
        pVoxelLimit.ClipToLimits(vStart,vEnd);
        outputPolygon.push_back(vStart);
        outputPolygon.push_back(vEnd);  
      }
      else  // Both point outside -> no output
      {
        // outputPolygon.push_back(vStart);
        // outputPolygon.push_back(vEnd);  
      }
    }
  }
}
/*
const G4VSolid* G4VSolid::GetConstituentSolid(G4int) const
{ return 0; } 

G4VSolid* G4VSolid::GetConstituentSolid(G4int)
{ return 0; } 

const G4DisplacedSolid* G4VSolid::GetDisplacedSolidPtr() const
{ return 0; } 

G4DisplacedSolid* G4VSolid::GetDisplacedSolidPtr() 
{ return 0; } 

G4VisExtent G4VSolid::GetExtent () const 
{
  G4VisExtent extent;
  G4VoxelLimits voxelLimits;  // Defaults to "infinite" limits.
  G4AffineTransform affineTransform;
  G4double vmin, vmax;
  CalculateExtent(kXAxis,voxelLimits,affineTransform,vmin,vmax);
  extent.SetXmin (vmin);
  extent.SetXmax (vmax);
  CalculateExtent(kYAxis,voxelLimits,affineTransform,vmin,vmax);
  extent.SetYmin (vmin);
  extent.SetYmax (vmax);
  CalculateExtent(kZAxis,voxelLimits,affineTransform,vmin,vmax);
  extent.SetZmin (vmin);
  extent.SetZmax (vmax);
  return extent;
}
*/
G4Polyhedron* G4VSolid::CreatePolyhedron () const
{
  return 0;
}
/*
G4NURBS* G4VSolid::CreateNURBS () const
{
  return 0;
}
*/
G4Polyhedron* G4VSolid::GetPolyhedron () const
{
  return 0;
}
/*
////////////////////////////////////////////////////////////////
//
// Returns an estimation of the solid volume in internal units.
// The number of statistics and error accuracy is fixed.
// This method may be overloaded by derived classes to compute the
// exact geometrical quantity for solids where this is possible.
// or anyway to cache the computed value.
// This implementation does NOT cache the computed value.

G4double G4VSolid::GetCubicVolume()
{
  G4int cubVolStatistics = 1000000;
  G4double cubVolEpsilon = 0.001;
  return EstimateCubicVolume(cubVolStatistics, cubVolEpsilon);
}
*/
////////////////////////////////////////////////////////////////
//
// Calculate cubic volume based on Inside() method.
// Accuracy is limited by the second argument or the statistics
// expressed by the first argument.
// Implementation is courtesy of Vasiliki Despoina Mitsou,
// University of Athens.

G4double G4VSolid::EstimateCubicVolume(G4int nStat, G4double epsilon) const
{
  G4int iInside=0;
  G4double px,py,pz,minX,maxX,minY,maxY,minZ,maxZ,volume;
  G4bool yesno;
  G4ThreeVector p;
  EInside in;

  // values needed for CalculateExtent signature

  G4VoxelLimits limit;                // Unlimited
  G4AffineTransform origin;

  // min max extents of pSolid along X,Y,Z

  yesno = this->CalculateExtent(kXAxis,limit,origin,minX,maxX);
  yesno = this->CalculateExtent(kYAxis,limit,origin,minY,maxY);
  yesno = this->CalculateExtent(kZAxis,limit,origin,minZ,maxZ);

  // limits

  if(nStat < 100)    nStat   = 100;
  if(epsilon > 0.01) epsilon = 0.01;

  for(G4int i = 0; i < nStat; i++ )
  {
///    px = minX+(maxX-minX)*G4UniformRand();
///    py = minY+(maxY-minY)*G4UniformRand();
///    pz = minZ+(maxZ-minZ)*G4UniformRand();
    px = minX+(maxX-minX)*rand()/(double)RAND_MAX;
    py = minY+(maxY-minY)*rand()/(double)RAND_MAX;
    pz = minZ+(maxZ-minZ)*rand()/(double)RAND_MAX;
    p  = G4ThreeVector(px,py,pz);
    in = this->Inside(p);
    if(in != kOutside) iInside++;    
  }
  volume = (maxX-minX)*(maxY-minY)*(maxZ-minZ)*iInside/nStat;
  return volume;
}
/*
////////////////////////////////////////////////////////////////
//
// Returns an estimation of the solid surface area in internal units.
// The number of statistics and error accuracy is fixed.
// This method may be overloaded by derived classes to compute the
// exact geometrical quantity for solids where this is possible.
// or anyway to cache the computed value.
// This implementation does NOT cache the computed value.

G4double G4VSolid::GetSurfaceArea()
{
  G4int stat = 1000000;
  G4double ell = -1.;
  return EstimateSurfaceArea(stat,ell);
}

////////////////////////////////////////////////////////////////
//
// Estimate surface area based on Inside(), DistanceToIn(), and
// DistanceToOut() methods. Accuracy is limited by the statistics
// defined by the first argument. Implemented by Mikhail Kosov.
*/
G4double G4VSolid::EstimateSurfaceArea(G4int nStat, G4double ell) const
{
  G4int inside=0;
  G4double px,py,pz,minX,maxX,minY,maxY,minZ,maxZ,surf;
  G4bool yesno;
  G4ThreeVector p;
  EInside in;

  // values needed for CalculateExtent signature

  G4VoxelLimits limit;                // Unlimited
  G4AffineTransform origin;

  // min max extents of pSolid along X,Y,Z

  yesno = this->CalculateExtent(kXAxis,limit,origin,minX,maxX);
  yesno = this->CalculateExtent(kYAxis,limit,origin,minY,maxY);
  yesno = this->CalculateExtent(kZAxis,limit,origin,minZ,maxZ);

  // limits

  if(nStat < 100) { nStat = 100; }

  G4double dX=maxX-minX;
  G4double dY=maxY-minY;
  G4double dZ=maxZ-minZ;
  if(ell<=0.)          // Automatic definition of skin thickness
  {
    G4double minval=dX;
    if(dY<dX) { minval=dY; }
    if(dZ<minval) { minval=dZ; }
    ell=.01*minval;
  }

  G4double dd=2*ell;
  minX-=ell; minY-=ell; minZ-=ell; dX+=dd; dY+=dd; dZ+=dd;

  for(G4int i = 0; i < nStat; i++ )
  {
///	px = minX+dX*G4UniformRand();
///    py = minY+dY*G4UniformRand();
///    pz = minZ+dZ*G4UniformRand();
	px = minX+dX*rand()/(double)RAND_MAX;
    py = minY+dY*rand()/(double)RAND_MAX;
    pz = minZ+dZ*rand()/(double)RAND_MAX;
    p  = G4ThreeVector(px,py,pz);
    in = this->Inside(p);
    if(in != kOutside)
    {
      if  (DistanceToOut(p)<ell) { inside++; }
    }
    else if(DistanceToIn(p)<ell) { inside++; }
  }
  // @@ The conformal correction can be upgraded
  surf = dX*dY*dZ*inside/dd/nStat;
  return surf;
}
