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
// $Id: G4IntersectionSolid.cc,v 1.30 2006/11/08 09:37:41 gcosmo Exp $
// GEANT4 tag $Name: geant4-09-03-patch-01 $
//
// Implementation of methods for the class G4IntersectionSolid
//
// History:
//
// 17.02.05 V.Grichine: bug was fixed in DistanceToIn(p,v) based on algorithm
//                      proposed by Dino Bazzacco <dino.bazzacco@pd.infn.it>
// 29.05.01 V.Grichine: bug was fixed in DistanceToIn(p,v)
// 16.03.01 V.Grichine: modifications in CalculateExtent() and Inside()
// 29.07.99 V.Grichine: modifications in DistanceToIn(p,v)
// 12.09.98 V.Grichine: first implementation
//
// --------------------------------------------------------------------

#include "G4IntersectionSolid.hh"

#include <sstream>
/*
#include "G4VoxelLimits.hh"
#include "G4VPVParameterisation.hh"

#include "G4VGraphicsScene.hh"
#include "G4Polyhedron.hh"
#include "G4NURBS.hh"
*/
///#include "G4VoxelLimits.hh"
#include "../../management/G4VPVParameterisation.hh"

///#include "G4VGraphicsScene.hh"
#include "../../../graphics_reps/G4Polyhedron.hh"
///#include "G4NURBS.hh"
// #include "G4NURBSbox.hh"

/////////////////////////////////////////////////////////////////////
//
// Transfer all data members to G4BooleanSolid which is responsible
// for them. pName will be in turn sent to G4VSolid
//

G4IntersectionSolid::G4IntersectionSolid( const G4String& pName,
                                                G4VSolid* pSolidA ,
                                                G4VSolid* pSolidB   )
  : G4BooleanSolid(pName,pSolidA,pSolidB)
{
} 

///////////////////////////////////////////////////////////////////
//

G4IntersectionSolid::G4IntersectionSolid( const G4String& pName,
                                                G4VSolid* pSolidA,
                                                G4VSolid* pSolidB,
                                                G4RotationMatrix* rotMatrix,
                                          const G4ThreeVector& transVector  )
  : G4BooleanSolid(pName,pSolidA,pSolidB,rotMatrix,transVector)
{
}

//////////////////////////////////////////////////////////////////
//
// 
 
G4IntersectionSolid::G4IntersectionSolid( const G4String& pName,
                                                G4VSolid* pSolidA,
                                                G4VSolid* pSolidB,
                                          const G4Transform3D& transform )
  : G4BooleanSolid(pName,pSolidA,pSolidB,transform)
{
} 

//////////////////////////////////////////////////////////////////
//
// Fake default constructor - sets only member data and allocates memory
//                            for usage restricted to object persistency.

G4IntersectionSolid::G4IntersectionSolid( __void__& a )
  : G4BooleanSolid(a)
{
}

///////////////////////////////////////////////////////////////
//
//

G4IntersectionSolid::~G4IntersectionSolid()
{
}

///////////////////////////////////////////////////////////////
//
//
     
G4bool 
G4IntersectionSolid::CalculateExtent(const EAxis pAxis,
                                     const G4VoxelLimits& pVoxelLimit,
                                     const G4AffineTransform& pTransform,
                                           G4double& pMin,
                                           G4double& pMax) const 
{
  G4bool   retA, retB, out;
  G4double minA, minB, maxA, maxB; 

  retA = fPtrSolidA
          ->CalculateExtent( pAxis, pVoxelLimit, pTransform, minA, maxA);
  retB = fPtrSolidB
          ->CalculateExtent( pAxis, pVoxelLimit, pTransform, minB, maxB);

  if( retA && retB )
  {
    pMin = std::max( minA, minB ); 
    pMax = std::min( maxA, maxB );
    out  = (pMax > pMin); // true;
#ifdef G4BOOLDEBUG
    // G4cout.precision(16);
    // G4cout<<"pMin = "<<pMin<<"; pMax = "<<pMax<<G4endl;
#endif
  }
  else out = false;

  return out; // It exists in this slice only if both exist in it.
}
 
/////////////////////////////////////////////////////
//
// Touching ? Empty intersection ?

EInside G4IntersectionSolid::Inside(const G4ThreeVector& p) const
{
  EInside positionA = fPtrSolidA->Inside(p) ;

  if( positionA == kOutside ) return kOutside ;

  EInside positionB = fPtrSolidB->Inside(p) ;
  
  if(positionA == kInside && positionB == kInside)
  {
    return kInside ;
  }
  else
  {
    if((positionA == kInside && positionB == kSurface) ||
       (positionB == kInside && positionA == kSurface) ||
       (positionA == kSurface && positionB == kSurface)   )
    {
      return kSurface ;
    }
    else
    {
      return kOutside ;
    }
  }
}

//////////////////////////////////////////////////////////////
//

G4ThreeVector 
G4IntersectionSolid::SurfaceNormal( const G4ThreeVector& p ) const 
{
  G4ThreeVector normal;
  EInside insideA, insideB;
  
  insideA= fPtrSolidA->Inside(p);
  insideB= fPtrSolidB->Inside(p);

#ifdef G4BOOLDEBUG
  if( (insideA == kOutside) || (insideB == kOutside) )
  {
    G4cout << "WARNING - Invalid call in "
           << "G4IntersectionSolid::SurfaceNormal(p)" << G4endl
           << "  Point p is outside !" << G4endl;
    G4cout << "          p = " << p << G4endl;
    G4cerr << "WARNING - Invalid call in "
           << "G4IntersectionSolid::SurfaceNormal(p)" << G4endl
           << "  Point p is outside !" << G4endl;
    G4cerr << "          p = " << p << G4endl;
  }
#endif

  // OLD: if(fPtrSolidA->DistanceToOut(p) <= fPtrSolidB->DistanceToOut(p) ) 

  // On the surface of both is difficult ... treat it like on A now!
  //
  // if( (insideA == kSurface) && (insideB == kSurface) )
  //    normal= fPtrSolidA->SurfaceNormal(p) ;
  // else 
  if( insideA == kSurface )
    {
      normal= fPtrSolidA->SurfaceNormal(p) ;
    }
  else if( insideB == kSurface )
    {
      normal= fPtrSolidB->SurfaceNormal(p) ;
    } 
    // We are on neither surface, so we should generate an exception
  else
    {
      if(fPtrSolidA->DistanceToOut(p) <= fPtrSolidB->DistanceToOut(p) ) 
   normal= fPtrSolidA->SurfaceNormal(p) ;   
      else
   normal= fPtrSolidB->SurfaceNormal(p) ;   
#ifdef G4BOOLDEBUG
      G4cout << "WARNING - Invalid call in "
             << "G4IntersectionSolid::SurfaceNormal(p)" << G4endl
             << "  Point p is out of surface !" << G4endl;
      G4cout << "          p = " << p << G4endl;
      G4cerr << "WARNING - Invalid call in "
             << "G4IntersectionSolid::SurfaceNormal(p)" << G4endl
             << "  Point p is out of surface !" << G4endl;
      G4cerr << "          p = " << p << G4endl;
#endif
    }

  return normal;
}

/////////////////////////////////////////////////////////////
//
// The same algorithm as in DistanceToIn(p)

G4double 
G4IntersectionSolid::DistanceToIn( const G4ThreeVector& p,
                                   const G4ThreeVector& v  ) const 
{
  G4double dist = 0.0;
  if( Inside(p) == kInside )
  {
#ifdef G4BOOLDEBUG
    G4cout << "WARNING - Invalid call in "
           << "G4IntersectionSolid::DistanceToIn(p,v)" << G4endl
           << "  Point p is inside !" << G4endl;
    G4cout << "          p = " << p << G4endl;
    G4cout << "          v = " << v << G4endl;
    G4cerr << "WARNING - Invalid call in "
           << "G4IntersectionSolid::DistanceToIn(p,v)" << G4endl
           << "  Point p is inside !" << G4endl;
    G4cerr << "          p = " << p << G4endl;
    G4cerr << "          v = " << v << G4endl;
#endif
  }
  else // if( Inside(p) == kSurface ) 
  {
    EInside wA = fPtrSolidA->Inside(p);
    EInside wB = fPtrSolidB->Inside(p);

    G4ThreeVector pA = p,  pB = p;
    G4double      dA = 0., dA1=0., dA2=0.;
    G4double      dB = 0., dB1=0., dB2=0.;
    G4bool        doA = true, doB = true;

    while(true) 
    {
      if(doA) 
      {
        // find next valid range for A

        dA1 = 0.;

        if( wA != kInside ) 
        {
          dA1 = fPtrSolidA->DistanceToIn(pA, v);

          if( dA1 == kInfinity )   return kInfinity;
        
          pA += dA1*v;
        }
        dA2 = dA1 + fPtrSolidA->DistanceToOut(pA, v);
      }
      dA1 += dA;
      dA2 += dA;

      if(doB) 
      {
        // find next valid range for B

        dB1 = 0.;
        if(wB != kInside) 
        {
          dB1 = fPtrSolidB->DistanceToIn(pB, v);

          if(dB1 == kInfinity)   return kInfinity;
        
          pB += dB1*v;
        }
        dB2 = dB1 + fPtrSolidB->DistanceToOut(pB, v);
      }
      dB1 += dB;
      dB2 += dB;

       // check if they overlap

      if( dA1 < dB1 ) 
      {
        if( dB1 < dA2 )  return dB1;

        dA   = dA2;
        pA   = p + dA*v;  // continue from here
        wA   = kSurface;
        doA  = true;
        doB  = false;
      }
      else 
      {
        if( dA1 < dB2 )  return dA1;

        dB   = dB2;
        pB   = p + dB*v;  // continue from here
        wB   = kSurface;
        doB  = true;
        doA  = false;
      }
    }
  }
  return dist ;  
}

////////////////////////////////////////////////////////
//
// Approximate nearest distance from the point p to the intersection of
// two solids

G4double 
G4IntersectionSolid::DistanceToIn( const G4ThreeVector& p) const 
{
#ifdef G4BOOLDEBUG
  if( Inside(p) == kInside )
  {
    G4cout << "WARNING - Invalid call in "
           << "G4IntersectionSolid::DistanceToIn(p)" << G4endl
           << "  Point p is inside !" << G4endl;
    G4cout << "          p = " << p << G4endl;
    G4cerr << "WARNING - Invalid call in "
           << "G4IntersectionSolid::DistanceToIn(p)" << G4endl
           << "  Point p is inside !" << G4endl;
    G4cerr << "          p = " << p << G4endl;
  }
#endif
  EInside sideA = fPtrSolidA->Inside(p) ;
  EInside sideB = fPtrSolidB->Inside(p) ;
  G4double dist=0.0 ;

  if( sideA != kInside && sideB  != kOutside )
  {
    dist = fPtrSolidA->DistanceToIn(p) ;
  }
  else
  {
    if( sideB != kInside  && sideA != kOutside )
    {
      dist = fPtrSolidB->DistanceToIn(p) ;
    }
    else
    {
      dist =  std::min(fPtrSolidA->DistanceToIn(p),
                    fPtrSolidB->DistanceToIn(p) ) ; 
    }
  }
  return dist ;
}

//////////////////////////////////////////////////////////
//
// The same algorithm as DistanceToOut(p)

G4double 
G4IntersectionSolid::DistanceToOut( const G4ThreeVector& p,
                                    const G4ThreeVector& v,
                                    const G4bool calcNorm,
                                          G4bool *validNorm,
                                          G4ThreeVector *n      ) const 
{
  G4bool         validNormA, validNormB;
  G4ThreeVector  nA, nB;

#ifdef G4BOOLDEBUG
  if( Inside(p) == kOutside )
  {
    G4cout << "Position:"  << G4endl << G4endl;
    G4cout << "p.x() = "   << p.x()/mm << " mm" << G4endl;
    G4cout << "p.y() = "   << p.y()/mm << " mm" << G4endl;
    G4cout << "p.z() = "   << p.z()/mm << " mm" << G4endl << G4endl;
    G4cout << "Direction:" << G4endl << G4endl;
    G4cout << "v.x() = "   << v.x() << G4endl;
    G4cout << "v.y() = "   << v.y() << G4endl;
    G4cout << "v.z() = "   << v.z() << G4endl << G4endl;
    G4cout << "WARNING - Invalid call in "
           << "G4IntersectionSolid::DistanceToOut(p,v)" << G4endl
           << "  Point p is outside !" << G4endl;
    G4cout << "          p = " << p << G4endl;
    G4cout << "          v = " << v << G4endl;
    G4cerr << "WARNING - Invalid call in "
           << "G4IntersectionSolid::DistanceToOut(p,v)" << G4endl
           << "  Point p is outside !" << G4endl;
    G4cerr << "          p = " << p << G4endl;
    G4cerr << "          v = " << v << G4endl;
  }
#endif
  G4double distA = fPtrSolidA->DistanceToOut(p,v,calcNorm,&validNormA,&nA) ;
  G4double distB = fPtrSolidB->DistanceToOut(p,v,calcNorm,&validNormB,&nB) ;

  G4double dist = std::min(distA,distB) ; 

  if( calcNorm )
  {
    if ( distA < distB )
    {
       *validNorm = validNormA;
       *n =         nA;
    }
    else
    {   
       *validNorm = validNormB;
       *n =         nB;
    }
  }

  return dist ; 
}

//////////////////////////////////////////////////////////////
//
// Inverted algorithm of DistanceToIn(p)

G4double 
G4IntersectionSolid::DistanceToOut( const G4ThreeVector& p ) const 
{
#ifdef G4BOOLDEBUG
  if( Inside(p) == kOutside )
  {
    G4cout << "WARNING - Invalid call in "
           << "G4IntersectionSolid::DistanceToOut(p)" << G4endl
           << "  Point p is outside !" << G4endl;
    G4cout << "          p = " << p << G4endl;
    G4cerr << "WARNING - Invalid call in "
           << "G4IntersectionSolid::DistanceToOut(p)" << G4endl
           << "  Point p is outside !" << G4endl;
    G4cerr << "          p = " << p << G4endl;
  }
#endif

  return std::min(fPtrSolidA->DistanceToOut(p),
                  fPtrSolidB->DistanceToOut(p) ) ; 

}

//////////////////////////////////////////////////////////////
//
//

void 
G4IntersectionSolid::ComputeDimensions( G4VPVParameterisation*,
                                  const G4int,
                                        const G4VPhysicalVolume* ) 
{
}

/////////////////////////////////////////////////
//
//                    

G4GeometryType G4IntersectionSolid::GetEntityType() const 
{
  return G4String("G4IntersectionSolid");
}

/////////////////////////////////////////////////
//
//                    

void 
G4IntersectionSolid::DescribeYourselfTo ( G4VGraphicsScene& scene ) const 
{
///  scene.AddSolid (*this);
}

////////////////////////////////////////////////////
//
//

G4Polyhedron* 
G4IntersectionSolid::CreatePolyhedron () const 
{
  G4Polyhedron* pA = fPtrSolidA->GetPolyhedron();
  G4Polyhedron* pB = fPtrSolidB->GetPolyhedron();
  if (pA && pB)
  {
    G4Polyhedron* resultant = new G4Polyhedron (pA->intersect(*pB));
    return resultant;
  }
  else
  {
    std::ostringstream oss;
    oss << "Solid - " << GetName()
        << " - one of the Boolean components has no" << G4endl
        << " corresponding polyhedron. Returning NULL !";
///    G4Exception("G4IntersectionSolid::CreatePolyhedron()", "InvalidSetup",
///                JustWarning, oss.str().c_str());
    return 0;
  }
}

/////////////////////////////////////////////////////////
//
//

G4NURBS*      
G4IntersectionSolid::CreateNURBS      () const 
{
  // Take into account boolean operation - see CreatePolyhedron.
  // return new G4NURBSbox (1.0, 1.0, 1.0);
  return 0;
}
