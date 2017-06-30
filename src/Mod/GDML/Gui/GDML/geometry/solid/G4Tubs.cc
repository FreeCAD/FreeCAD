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
// $Id: G4Tubs.cc,v 1.79 2009/06/30 10:10:11 gcosmo Exp $
// GEANT4 tag $Name: geant4-09-03-patch-01 $
//
// 
// class G4Tubs
//
// History:
//
// 02.08.07 T.Nikitina: bug fixed in DistanceToOut(p,v,..) for negative value under sqrt
//                      for the case: p on the surface and v is tangent to the surface
// 11.05.07 T.Nikitina: bug fixed in DistanceToOut(p,v,..) for phi < 2pi
// 03.05.05 V.Grichine: SurfaceNormal(p) according to J. Apostolakis proposal
// 16.03.05 V.Grichine: SurfaceNormal(p) with edges/corners for boolean
// 20.07.01 V.Grichine: bug fixed in Inside(p)
// 20.02.01 V.Grichine: bug fixed in Inside(p) and CalculateExtent was 
//                      simplified base on G4Box::CalculateExtent
// 07.12.00 V.Grichine: phi-section algorithm was changed in Inside(p)
// 28.11.00 V.Grichine: bug fixed in Inside(p)
// 31.10.00 V.Grichine: assign sr, sphi in Distance ToOut(p,v,...)
// 08.08.00 V.Grichine: more stable roots of 2-equation in DistanceToOut(p,v,..)
// 02.08.00 V.Grichine: point is outside check in Distance ToOut(p)
// 17.05.00 V.Grichine: bugs (#76,#91) fixed in Distance ToOut(p,v,...)
// 31.03.00 V.Grichine: bug fixed in Inside(p)
// 19.11.99 V.Grichine: side = kNull in DistanceToOut(p,v,...)
// 13.10.99 V.Grichine: bugs fixed in DistanceToIn(p,v) 
// 28.05.99 V.Grichine: bugs fixed in DistanceToOut(p,v,...)
// 25.05.99 V.Grichine: bugs fixed in DistanceToIn(p,v) 
// 23.03.99 V.Grichine: bug fixed in DistanceToIn(p,v) 
// 09.10.98 V.Grichine: modifications in DistanceToOut(p,v,...)
// 18.06.98 V.Grichine: n-normalisation in DistanceToOut(p,v)
// 
// 1994-95  P.Kent:     implementation
//
/////////////////////////////////////////////////////////////////////////

#include "G4Tubs.hh"
/*
#include "G4VoxelLimits.hh"
#include "G4AffineTransform.hh"
#include "G4GeometryTolerance.hh"
#include "G4VPVParameterisation.hh"

#include "Randomize.hh"

#include "meshdefs.hh"

#include "G4VGraphicsScene.hh"
#include "G4Polyhedron.hh"
#include "G4NURBS.hh"
#include "G4NURBStube.hh"
#include "G4NURBScylinder.hh"
#include "G4NURBStubesector.hh"
*/
#include "../management/G4VoxelLimits.hh"
#include "../management/G4AffineTransform.hh"
#include "../../global/G4GeometryTolerance.hh"
#include "../management/G4VPVParameterisation.hh"

///#include "Randomize.hh"

#include "../management/meshdefs.hh"

///#include "G4VGraphicsScene.hh"
#include "../../graphics_reps/G4Polyhedron.hh"
///#include "G4NURBS.hh"
///#include "G4NURBStube.hh"
///#include "G4NURBScylinder.hh"
///#include "G4NURBStubesector.hh"

////using namespace CLHEP;

/////////////////////////////////////////////////////////////////////////
//
// Constructor - check parameters, convert angles so 0<sphi+dpshi<=2_PI
//             - note if pdphi>2PI then reset to 2PI

G4Tubs::G4Tubs( const G4String &pName,
                      G4double pRMin, G4double pRMax,
                      G4double pDz,
                      G4double pSPhi, G4double pDPhi )
  : G4CSGSolid(pName), fSPhi(0), fDPhi(0)
{

///  kRadTolerance = G4GeometryTolerance::GetInstance()->GetRadialTolerance();
///  kAngTolerance = G4GeometryTolerance::GetInstance()->GetAngularTolerance();
///BEGIN CAD-GDML
///see checkangle at the end of the function
///kRadTolerance = 0.01;
///kAngTolerance = 0.1;
///END CAD-GDML

  if (pDz>0) // Check z-len
  {
    fDz = pDz ;
  }
  else
  {
///    G4cerr << "ERROR - G4Tubs()::G4Tubs()" << G4endl
///           << "        Negative Z half-length (" << pDz << ") in solid: "
///           << GetName() << G4endl;
///    G4Exception("G4Tubs::G4Tubs()", "InvalidSetup", FatalException,
///                "Invalid Z half-length");
  }
  if ( (pRMin < pRMax) && (pRMin >= 0) ) // Check radii
  {
    fRMin = pRMin ; 
    fRMax = pRMax ;
  }
  else
  {
///    G4cerr << "ERROR - G4Tubs()::G4Tubs()" << G4endl
///           << "        Invalid values for radii in solid " << GetName()
///           << G4endl
///           << "        pRMin = " << pRMin << ", pRMax = " << pRMax << G4endl;
///    G4Exception("G4Tubs::G4Tubs()", "InvalidSetup", FatalException,
///                "Invalid radii.");
  }

  // Check angles
  CheckPhiAngles(pSPhi, pDPhi);
}

///////////////////////////////////////////////////////////////////////
//
// Fake default constructor - sets only member data and allocates memory
//                            for usage restricted to object persistency.
//
G4Tubs::G4Tubs( __void__& a )
  : G4CSGSolid(a)
{
}

//////////////////////////////////////////////////////////////////////////
//
// Destructor

G4Tubs::~G4Tubs()
{
}
/*
/////////////////////////////////////////////////////////////////////////
//
// Dispatch to parameterisation for replication mechanism dimension
// computation & modification.

void G4Tubs::ComputeDimensions(       G4VPVParameterisation* p,
                                const G4int n,
                                const G4VPhysicalVolume* pRep )
{
  p->ComputeDimensions(*this,n,pRep) ;
}
*/
////////////////////////////////////////////////////////////////////////
//
// Calculate extent under transform and specified limit

G4bool G4Tubs::CalculateExtent( const EAxis              pAxis,
                                const G4VoxelLimits&     pVoxelLimit,
                                const G4AffineTransform& pTransform,
                                      G4double&          pMin, 
                                      G4double&          pMax    ) const
{
		//CAD-GDML
	return true;
/*
  if ( (!pTransform.IsRotated()) && (fDPhi == twopi) && (fRMin == 0) )
  {
    // Special case handling for unrotated solid tubes
    // Compute x/y/z mins and maxs fro bounding box respecting limits,
    // with early returns if outside limits. Then switch() on pAxis,
    // and compute exact x and y limit for x/y case
      
    G4double xoffset, xMin, xMax;
    G4double yoffset, yMin, yMax;
    G4double zoffset, zMin, zMax;

    G4double diff1, diff2, maxDiff, newMin, newMax;
    G4double xoff1, xoff2, yoff1, yoff2, delta;

    xoffset = pTransform.NetTranslation().x();
    xMin = xoffset - fRMax;
    xMax = xoffset + fRMax;

    if (pVoxelLimit.IsXLimited())
    {
      if ( (xMin > pVoxelLimit.GetMaxXExtent())
        || (xMax < pVoxelLimit.GetMinXExtent()) )
      {
        return false;
      }
      else
      {
        if (xMin < pVoxelLimit.GetMinXExtent())
        {
          xMin = pVoxelLimit.GetMinXExtent();
        }
        if (xMax > pVoxelLimit.GetMaxXExtent())
        {
          xMax = pVoxelLimit.GetMaxXExtent();
        }
      }
    }
    yoffset = pTransform.NetTranslation().y();
    yMin    = yoffset - fRMax;
    yMax    = yoffset + fRMax;

    if ( pVoxelLimit.IsYLimited() )
    {
      if ( (yMin > pVoxelLimit.GetMaxYExtent())
        || (yMax < pVoxelLimit.GetMinYExtent()) )
      {
        return false;
      }
      else
      {
        if (yMin < pVoxelLimit.GetMinYExtent())
        {
          yMin = pVoxelLimit.GetMinYExtent();
        }
        if (yMax > pVoxelLimit.GetMaxYExtent())
        {
          yMax=pVoxelLimit.GetMaxYExtent();
        }
      }
    }
    zoffset = pTransform.NetTranslation().z();
    zMin    = zoffset - fDz;
    zMax    = zoffset + fDz;

    if ( pVoxelLimit.IsZLimited() )
    {
      if ( (zMin > pVoxelLimit.GetMaxZExtent())
        || (zMax < pVoxelLimit.GetMinZExtent()) )
      {
        return false;
      }
      else
      {
        if (zMin < pVoxelLimit.GetMinZExtent())
        {
          zMin = pVoxelLimit.GetMinZExtent();
        }
        if (zMax > pVoxelLimit.GetMaxZExtent())
        {
          zMax = pVoxelLimit.GetMaxZExtent();
        }
      }
    }
    switch ( pAxis )  // Known to cut cylinder
    {
      case kXAxis :
      {
        yoff1 = yoffset - yMin;
        yoff2 = yMax    - yoffset;

        if ( (yoff1 >= 0) && (yoff2 >= 0) ) // Y limits cross max/min x
        {                                   // => no change
          pMin = xMin;
          pMax = xMax;
        }
        else
        {
          // Y limits don't cross max/min x => compute max delta x,
          // hence new mins/maxs

          delta   = fRMax*fRMax - yoff1*yoff1;
          diff1   = (delta>0.) ? std::sqrt(delta) : 0.;
          delta   = fRMax*fRMax - yoff2*yoff2;
          diff2   = (delta>0.) ? std::sqrt(delta) : 0.;
          maxDiff = (diff1 > diff2) ? diff1:diff2;
          newMin  = xoffset - maxDiff;
          newMax  = xoffset + maxDiff;
          pMin    = (newMin < xMin) ? xMin : newMin;
          pMax    = (newMax > xMax) ? xMax : newMax;
        }    
        break;
      }
      case kYAxis :
      {
        xoff1 = xoffset - xMin;
        xoff2 = xMax - xoffset;

        if ( (xoff1 >= 0) && (xoff2 >= 0) ) // X limits cross max/min y
        {                                   // => no change
          pMin = yMin;
          pMax = yMax;
        }
        else
        {
          // X limits don't cross max/min y => compute max delta y,
          // hence new mins/maxs

          delta   = fRMax*fRMax - xoff1*xoff1;
          diff1   = (delta>0.) ? std::sqrt(delta) : 0.;
          delta   = fRMax*fRMax - xoff2*xoff2;
          diff2   = (delta>0.) ? std::sqrt(delta) : 0.;
          maxDiff = (diff1 > diff2) ? diff1 : diff2;
          newMin  = yoffset - maxDiff;
          newMax  = yoffset + maxDiff;
          pMin    = (newMin < yMin) ? yMin : newMin;
          pMax    = (newMax > yMax) ? yMax : newMax;
        }
        break;
      }
      case kZAxis:
      {
        pMin = zMin;
        pMax = zMax;
        break;
      }
      default:
        break;
    }
    pMin -= kCarTolerance;
    pMax += kCarTolerance;
    return true;
  }
  else // Calculate rotated vertex coordinates
  {
    G4int i, noEntries, noBetweenSections4;
    G4bool existsAfterClip = false;
    G4ThreeVectorList* vertices = CreateRotatedVertices(pTransform);

    pMin =  kInfinity;
    pMax = -kInfinity;

    noEntries = vertices->size();
    noBetweenSections4 = noEntries - 4;
    
    for ( i = 0 ; i < noEntries ; i += 4 )
    {
      ClipCrossSection(vertices, i, pVoxelLimit, pAxis, pMin, pMax);
    }
    for ( i = 0 ; i < noBetweenSections4 ; i += 4 )
    {
      ClipBetweenSections(vertices, i, pVoxelLimit, pAxis, pMin, pMax);
    }
    if ( (pMin != kInfinity) || (pMax != -kInfinity) )
    {
      existsAfterClip = true;
      pMin -= kCarTolerance; // Add 2*tolerance to avoid precision troubles
      pMax += kCarTolerance;
    }
    else
    {
      // Check for case where completely enveloping clipping volume
      // If point inside then we are confident that the solid completely
      // envelopes the clipping volume. Hence set min/max extents according
      // to clipping volume extents along the specified axis.

      G4ThreeVector clipCentre(
             (pVoxelLimit.GetMinXExtent()+pVoxelLimit.GetMaxXExtent())*0.5,
             (pVoxelLimit.GetMinYExtent()+pVoxelLimit.GetMaxYExtent())*0.5,
             (pVoxelLimit.GetMinZExtent()+pVoxelLimit.GetMaxZExtent())*0.5 );
        
      if ( Inside(pTransform.Inverse().TransformPoint(clipCentre)) != kOutside )
      {
        existsAfterClip = true;
        pMin            = pVoxelLimit.GetMinExtent(pAxis);
        pMax            = pVoxelLimit.GetMaxExtent(pAxis);
      }
    }
    delete vertices;
    return existsAfterClip;
  }
  */
}


///////////////////////////////////////////////////////////////////////////
//
// Return whether point inside/outside/on surface

EInside G4Tubs::Inside( const G4ThreeVector& p ) const
{
  G4double r2,pPhi,tolRMin,tolRMax;
  EInside in = kOutside ;
  static const G4double halfCarTolerance=kCarTolerance*0.5;
  static const G4double halfRadTolerance=kRadTolerance*0.5;
  static const G4double halfAngTolerance=kAngTolerance*0.5;

  if (std::fabs(p.z()) <= fDz - halfCarTolerance)
  {
    r2 = p.x()*p.x() + p.y()*p.y() ;

    if (fRMin) { tolRMin = fRMin + halfRadTolerance ; }
    else       { tolRMin = 0 ; }

    tolRMax = fRMax - halfRadTolerance ;
      
    if ((r2 >= tolRMin*tolRMin) && (r2 <= tolRMax*tolRMax))
    {
      if ( fPhiFullTube )
      {
        in = kInside ;
      }
      else
      {
        // Try inner tolerant phi boundaries (=>inside)
        // if not inside, try outer tolerant phi boundaries

        if ((tolRMin==0)&&(p.x()<=halfCarTolerance)&&(p.y()<=halfCarTolerance))
        {
          in=kSurface;
        }
        else
        {
          pPhi = std::atan2(p.y(),p.x()) ;
          if ( pPhi < -halfAngTolerance )  { pPhi += twopi; } // 0<=pPhi<2pi

          if ( fSPhi >= 0 )
          {
            if ( (std::abs(pPhi) < halfAngTolerance)
              && (std::abs(fSPhi + fDPhi - twopi) < halfAngTolerance) )
            { 
              pPhi += twopi ; // 0 <= pPhi < 2pi
            }
            if ( (pPhi >= fSPhi + halfAngTolerance)
              && (pPhi <= fSPhi + fDPhi - halfAngTolerance) )
            {
              in = kInside ;
            }
            else if ( (pPhi >= fSPhi - halfAngTolerance)
                   && (pPhi <= fSPhi + fDPhi + halfAngTolerance) )
            {
              in = kSurface ;
            }
          }
          else  // fSPhi < 0
          {
            if ( (pPhi <= fSPhi + twopi - halfAngTolerance)
              && (pPhi >= fSPhi + fDPhi  + halfAngTolerance) ) {;} //kOutside
            else if ( (pPhi <= fSPhi + twopi + halfAngTolerance)
                   && (pPhi >= fSPhi + fDPhi  - halfAngTolerance) )
            {
              in = kSurface ;
            }
            else
            {
              in = kInside ;
            }
          }
        }                    
      }
    }
    else  // Try generous boundaries
    {
      tolRMin = fRMin - halfRadTolerance ;
      tolRMax = fRMax + halfRadTolerance ;

      if ( tolRMin < 0 )  { tolRMin = 0; }

      if ( (r2 >= tolRMin*tolRMin) && (r2 <= tolRMax*tolRMax) )
      {
        if (fPhiFullTube || (r2 <=halfRadTolerance*halfRadTolerance) )
        {                        // Continuous in phi or on z-axis
          in = kSurface ;
        }
        else // Try outer tolerant phi boundaries only
        {
          pPhi = std::atan2(p.y(),p.x()) ;

          if ( pPhi < -halfAngTolerance)  { pPhi += twopi; } // 0<=pPhi<2pi
          if ( fSPhi >= 0 )
          {
            if ( (std::abs(pPhi) < halfAngTolerance)
              && (std::abs(fSPhi + fDPhi - twopi) < halfAngTolerance) )
            { 
              pPhi += twopi ; // 0 <= pPhi < 2pi
            }
            if ( (pPhi >= fSPhi - halfAngTolerance)
              && (pPhi <= fSPhi + fDPhi + halfAngTolerance) )
            {
              in = kSurface ;
            }
          }
          else  // fSPhi < 0
          {
            if ( (pPhi <= fSPhi + twopi - halfAngTolerance)
              && (pPhi >= fSPhi + fDPhi + halfAngTolerance) ) {;} // kOutside
            else
            {
              in = kSurface ;
            }
          }
        }
      }
    }
  }
  else if (std::fabs(p.z()) <= fDz + halfCarTolerance)
  {                                          // Check within tolerant r limits
    r2      = p.x()*p.x() + p.y()*p.y() ;
    tolRMin = fRMin - halfRadTolerance ;
    tolRMax = fRMax + halfRadTolerance ;

    if ( tolRMin < 0 )  { tolRMin = 0; }

    if ( (r2 >= tolRMin*tolRMin) && (r2 <= tolRMax*tolRMax) )
    {
      if (fPhiFullTube || (r2 <=halfRadTolerance*halfRadTolerance))
      {                        // Continuous in phi or on z-axis
        in = kSurface ;
      }
      else // Try outer tolerant phi boundaries
      {
        pPhi = std::atan2(p.y(),p.x()) ;

        if ( pPhi < -halfAngTolerance )  { pPhi += twopi; }  // 0<=pPhi<2pi
        if ( fSPhi >= 0 )
        {
          if ( (std::abs(pPhi) < halfAngTolerance)
            && (std::abs(fSPhi + fDPhi - twopi) < halfAngTolerance) )
          { 
            pPhi += twopi ; // 0 <= pPhi < 2pi
          }
          if ( (pPhi >= fSPhi - halfAngTolerance)
            && (pPhi <= fSPhi + fDPhi + halfAngTolerance) )
          {
            in = kSurface;
          }
        }
        else  // fSPhi < 0
        {
          if ( (pPhi <= fSPhi + twopi - halfAngTolerance)
            && (pPhi >= fSPhi + fDPhi  + halfAngTolerance) ) {;}
          else
          {
            in = kSurface ;
          }
        }      
      }
    }
  }
  return in;
}

///////////////////////////////////////////////////////////////////////////
//
// Return unit normal of surface closest to p
// - note if point on z axis, ignore phi divided sides
// - unsafe if point close to z axis a rmin=0 - no explicit checks

G4ThreeVector G4Tubs::SurfaceNormal( const G4ThreeVector& p ) const
{
  G4int noSurfaces = 0;
  G4double rho, pPhi;
  G4double distZ, distRMin, distRMax;
  G4double distSPhi = kInfinity, distEPhi = kInfinity;

  static const G4double halfCarTolerance = 0.5*kCarTolerance;
  static const G4double halfAngTolerance = 0.5*kAngTolerance;

  G4ThreeVector norm, sumnorm(0.,0.,0.);
  G4ThreeVector nZ = G4ThreeVector(0, 0, 1.0);
  G4ThreeVector nR, nPs, nPe;

  rho = std::sqrt(p.x()*p.x() + p.y()*p.y());

  distRMin = std::fabs(rho - fRMin);
  distRMax = std::fabs(rho - fRMax);
  distZ    = std::fabs(std::fabs(p.z()) - fDz);

  if (!fPhiFullTube)    // Protected against (0,0,z) 
  {
    if ( rho > halfCarTolerance )
    {
      pPhi = std::atan2(p.y(),p.x());
    
      if(pPhi  < fSPhi- halfCarTolerance)           { pPhi += twopi; }
      else if(pPhi > fSPhi+fDPhi+ halfCarTolerance) { pPhi -= twopi; }

      distSPhi = std::fabs(pPhi - fSPhi);       
      distEPhi = std::fabs(pPhi - fSPhi - fDPhi); 
    }
    else if( !fRMin )
    {
      distSPhi = 0.; 
      distEPhi = 0.; 
    }
    nPs = G4ThreeVector(std::sin(fSPhi),-std::cos(fSPhi),0);
    nPe = G4ThreeVector(-std::sin(fSPhi+fDPhi),std::cos(fSPhi+fDPhi),0);
  }
  if ( rho > halfCarTolerance ) { nR = G4ThreeVector(p.x()/rho,p.y()/rho,0); }

  if( distRMax <= halfCarTolerance )
  {
    noSurfaces ++;
    sumnorm += nR;
  }
  if( fRMin && (distRMin <= halfCarTolerance) )
  {
    noSurfaces ++;
    sumnorm -= nR;
  }
  if( fDPhi < twopi )   
  {
    if (distSPhi <= halfAngTolerance)  
    {
      noSurfaces ++;
      sumnorm += nPs;
    }
    if (distEPhi <= halfAngTolerance)  
    {
      noSurfaces ++;
      sumnorm += nPe;
    }
  }
  if (distZ <= halfCarTolerance)  
  {
    noSurfaces ++;
    if ( p.z() >= 0.)  { sumnorm += nZ; }
    else               { sumnorm -= nZ; }
  }
  if ( noSurfaces == 0 )
  {
#ifdef G4CSGDEBUG
    G4Exception("G4Tube::SurfaceNormal(p)", "Notification",
                JustWarning, "Point p is not on surface !?" );
    G4cout.precision(20);
    G4cout<< "G4Tubs::SN ( "<<p.x()<<", "<<p.y()<<", "<<p.z()<<" ); "
          << G4endl << G4endl;
#endif 
     norm = ApproxSurfaceNormal(p);
  }
  else if ( noSurfaces == 1 )  { norm = sumnorm; }
  else                         { norm = sumnorm.unit(); }

  return norm;
}

/////////////////////////////////////////////////////////////////////////////
//
// Algorithm for SurfaceNormal() following the original specification
// for points not on the surface

G4ThreeVector G4Tubs::ApproxSurfaceNormal( const G4ThreeVector& p ) const
{ ENorm side ;
  G4ThreeVector norm ;
  G4double rho, phi ;
  G4double distZ, distRMin, distRMax, distSPhi, distEPhi, distMin ;

  rho = std::sqrt(p.x()*p.x() + p.y()*p.y()) ;

  distRMin = std::fabs(rho - fRMin) ;
  distRMax = std::fabs(rho - fRMax) ;
  distZ    = std::fabs(std::fabs(p.z()) - fDz) ;

  if (distRMin < distRMax) // First minimum
  {
    if ( distZ < distRMin )
    {
       distMin = distZ ;
       side    = kNZ ;
    }
    else
    {
      distMin = distRMin ;
      side    = kNRMin   ;
    }
  }
  else
  {
    if ( distZ < distRMax )
    {
      distMin = distZ ;
      side    = kNZ   ;
    }
    else
    {
      distMin = distRMax ;
      side    = kNRMax   ;
    }
  }   
  if (!fPhiFullTube  &&  rho ) // Protected against (0,0,z) 
  {
    phi = std::atan2(p.y(),p.x()) ;

    if ( phi < 0 )  { phi += twopi; }

    if ( fSPhi < 0 )
    {
      distSPhi = std::fabs(phi - (fSPhi + twopi))*rho ;
    }
    else
    {
      distSPhi = std::fabs(phi - fSPhi)*rho ;
    }
    distEPhi = std::fabs(phi - fSPhi - fDPhi)*rho ;
                                      
    if (distSPhi < distEPhi) // Find new minimum
    {
      if ( distSPhi < distMin )
      {
        side = kNSPhi ;
      }
    }
    else
    {
      if ( distEPhi < distMin )
      {
        side = kNEPhi ;
      }
    }
  }    
  switch ( side )
  {
    case kNRMin : // Inner radius
    {                      
      norm = G4ThreeVector(-p.x()/rho, -p.y()/rho, 0) ;
      break ;
    }
    case kNRMax : // Outer radius
    {                  
      norm = G4ThreeVector(p.x()/rho, p.y()/rho, 0) ;
      break ;
    }
    case kNZ : //    + or - dz
    {                              
      if ( p.z() > 0 )  { norm = G4ThreeVector(0,0,1) ; }
      else              { norm = G4ThreeVector(0,0,-1); }
      break ;
    }
    case kNSPhi:
    {
      norm = G4ThreeVector(std::sin(fSPhi), -std::cos(fSPhi), 0) ;
      break ;
    }
    case kNEPhi:
    {
      norm = G4ThreeVector(-std::sin(fSPhi+fDPhi), std::cos(fSPhi+fDPhi), 0) ;
      break;
    }
    default:
    {
 ///     DumpInfo();
 ///     G4Exception("G4Tubs::ApproxSurfaceNormal()", "Notification", JustWarning,
 ///                 "Undefined side for valid surface normal to solid.");
      break ;
    }    
  }                
  return norm;
}

////////////////////////////////////////////////////////////////////
//
//
// Calculate distance to shape from outside, along normalised vector
// - return kInfinity if no intersection, or intersection distance <= tolerance
//
// - Compute the intersection with the z planes 
//        - if at valid r, phi, return
//
// -> If point is outer outer radius, compute intersection with rmax
//        - if at valid phi,z return
//
// -> Compute intersection with inner radius, taking largest +ve root
//        - if valid (in z,phi), save intersction
//
//    -> If phi segmented, compute intersections with phi half planes
//        - return smallest of valid phi intersections and
//          inner radius intersection
//
// NOTE:
// - 'if valid' implies tolerant checking of intersection points

G4double G4Tubs::DistanceToIn( const G4ThreeVector& p,
                               const G4ThreeVector& v  ) const
{
  G4double snxt = kInfinity ;      // snxt = default return value
  G4double tolORMin2, tolIRMax2 ;  // 'generous' radii squared
  G4double tolORMax2, tolIRMin2, tolODz, tolIDz ;
  const G4double dRmax = 100.*fRMax;

  static const G4double halfCarTolerance = 0.5*kCarTolerance;
  static const G4double halfRadTolerance = 0.5*kRadTolerance;

  // Intersection point variables
  //
  G4double Dist, s, xi, yi, zi, rho2, inum, iden, cosPsi, Comp ;
  G4double t1, t2, t3, b, c, d ;     // Quadratic solver variables 
  
  // Calculate tolerant rmin and rmax

  if (fRMin > kRadTolerance)
  {
    tolORMin2 = (fRMin - halfRadTolerance)*(fRMin - halfRadTolerance) ;
    tolIRMin2 = (fRMin + halfRadTolerance)*(fRMin + halfRadTolerance) ;
  }
  else
  {
    tolORMin2 = 0.0 ;
    tolIRMin2 = 0.0 ;
  }
  tolORMax2 = (fRMax + halfRadTolerance)*(fRMax + halfRadTolerance) ;
  tolIRMax2 = (fRMax - halfRadTolerance)*(fRMax - halfRadTolerance) ;

  // Intersection with Z surfaces

  tolIDz = fDz - halfCarTolerance ;
  tolODz = fDz + halfCarTolerance ;

  if (std::fabs(p.z()) >= tolIDz)
  {
    if ( p.z()*v.z() < 0 )    // at +Z going in -Z or visa versa
    {
      s = (std::fabs(p.z()) - fDz)/std::fabs(v.z()) ;   // Z intersect distance

      if(s < 0.0)  { s = 0.0; }

      xi   = p.x() + s*v.x() ;                // Intersection coords
      yi   = p.y() + s*v.y() ;
      rho2 = xi*xi + yi*yi ;

      // Check validity of intersection

      if ((tolIRMin2 <= rho2) && (rho2 <= tolIRMax2))
      {
        if (!fPhiFullTube && rho2)
        {
          // Psi = angle made with central (average) phi of shape
          //
          inum   = xi*cosCPhi + yi*sinCPhi ;
          iden   = std::sqrt(rho2) ;
          cosPsi = inum/iden ;
          if (cosPsi >= cosHDPhiIT)  { return s ; }
        }
        else
        {
          return s ;
        }
      }
    }
    else
    {
      if ( snxt<halfCarTolerance )  { snxt=0; }
      return snxt ;  // On/outside extent, and heading away
                     // -> cannot intersect
    }
  }

  // -> Can not intersect z surfaces
  //
  // Intersection with rmax (possible return) and rmin (must also check phi)
  //
  // Intersection point (xi,yi,zi) on line x=p.x+t*v.x etc.
  //
  // Intersects with x^2+y^2=R^2
  //
  // Hence (v.x^2+v.y^2)t^2+ 2t(p.x*v.x+p.y*v.y)+p.x^2+p.y^2-R^2=0
  //            t1                t2                t3

  t1 = 1.0 - v.z()*v.z() ;
  t2 = p.x()*v.x() + p.y()*v.y() ;
  t3 = p.x()*p.x() + p.y()*p.y() ;

  if ( t1 > 0 )        // Check not || to z axis
  {
    b = t2/t1 ;
    c = t3 - fRMax*fRMax ;
    if ((t3 >= tolORMax2) && (t2<0))   // This also handles the tangent case
    {
      // Try outer cylinder intersection
      //          c=(t3-fRMax*fRMax)/t1;

      c /= t1 ;
      d = b*b - c ;

      if (d >= 0)  // If real root
      {
        s = -b - std::sqrt(d) ;
        if (s >= 0)  // If 'forwards'
        {
          if ( s>dRmax ) // Avoid rounding errors due to precision issues on
          {              // 64 bits systems. Split long distances and recompute
            G4double fTerm = s-std::fmod(s,dRmax);
            s = fTerm + DistanceToIn(p+fTerm*v,v);
          } 
          // Check z intersection
          //
          zi = p.z() + s*v.z() ;
          if (std::fabs(zi)<=tolODz)
          {
            // Z ok. Check phi intersection if reqd
            //
            if (fPhiFullTube)
            {
              return s ;
            }
            else
            {
              xi     = p.x() + s*v.x() ;
              yi     = p.y() + s*v.y() ;
              cosPsi = (xi*cosCPhi + yi*sinCPhi)/fRMax ;
              if (cosPsi >= cosHDPhiIT)  { return s ; }
            }
          }  //  end if std::fabs(zi)
        }    //  end if (s>=0)
      }      //  end if (d>=0)
    }        //  end if (r>=fRMax)
    else 
    {
      // Inside outer radius :
      // check not inside, and heading through tubs (-> 0 to in)

      if ((t3 > tolIRMin2) && (t2 < 0) && (std::fabs(p.z()) <= tolIDz))
      {
        // Inside both radii, delta r -ve, inside z extent

        if (!fPhiFullTube)
        {
          inum   = p.x()*cosCPhi + p.y()*sinCPhi ;
          iden   = std::sqrt(t3) ;
          cosPsi = inum/iden ;
          if (cosPsi >= cosHDPhiIT)
          {
            // In the old version, the small negative tangent for the point
            // on surface was not taken in account, and returning 0.0 ...
            // New version: check the tangent for the point on surface and 
            // if no intersection, return kInfinity, if intersection instead
            // return s.
            //
            c = t3-fRMax*fRMax; 
            if ( c<=0.0 )
            {
              return 0.0;
            }
            else
            {
              c = c/t1 ;
              d = b*b-c;
              if ( d>=0.0 )
              {
                snxt = c/(-b+std::sqrt(d)); // using safe solution
                                            // for quadratic equation 
                if ( snxt < halfCarTolerance ) { snxt=0; }
                return snxt ;
              }      
              else
              {
                return kInfinity;
              }
            }
          } 
        }
        else
        {   
          // In the old version, the small negative tangent for the point
          // on surface was not taken in account, and returning 0.0 ...
          // New version: check the tangent for the point on surface and 
          // if no intersection, return kInfinity, if intersection instead
          // return s.
          //
          c = t3 - fRMax*fRMax; 
          if ( c<=0.0 )
          {
            return 0.0;
          }
          else
          {
            c = c/t1 ;
            d = b*b-c;
            if ( d>=0.0 )
            {
              snxt= c/(-b+std::sqrt(d)); // using safe solution
                                         // for quadratic equation 
              if ( snxt < halfCarTolerance ) { snxt=0; }
              return snxt ;
            }      
            else
            {
              return kInfinity;
            }
          }
        } // end if   (!fPhiFullTube)
      }   // end if   (t3>tolIRMin2)
    }     // end if   (Inside Outer Radius) 
    if ( fRMin )    // Try inner cylinder intersection
    {
      c = (t3 - fRMin*fRMin)/t1 ;
      d = b*b - c ;
      if ( d >= 0.0 )  // If real root
      {
        // Always want 2nd root - we are outside and know rmax Hit was bad
        // - If on surface of rmin also need farthest root

        s = -b + std::sqrt(d) ;
        if (s >= -halfCarTolerance)  // check forwards
        {
          // Check z intersection
          //
          if(s < 0.0)  { s = 0.0; }
          if ( s>dRmax ) // Avoid rounding errors due to precision issues seen
          {              // 64 bits systems. Split long distances and recompute
            G4double fTerm = s-std::fmod(s,dRmax);
            s = fTerm + DistanceToIn(p+fTerm*v,v);
          } 
          zi = p.z() + s*v.z() ;
          if (std::fabs(zi) <= tolODz)
          {
            // Z ok. Check phi
            //
            if ( fPhiFullTube )
            {
              return s ; 
            }
            else
            {
              xi     = p.x() + s*v.x() ;
              yi     = p.y() + s*v.y() ;
              cosPsi = (xi*cosCPhi + yi*sinCPhi)/fRMin ;
              if (cosPsi >= cosHDPhiIT)
              {
                // Good inner radius isect
                // - but earlier phi isect still possible

                snxt = s ;
              }
            }
          }        //    end if std::fabs(zi)
        }          //    end if (s>=0)
      }            //    end if (d>=0)
    }              //    end if (fRMin)
  }

  // Phi segment intersection
  //
  // o Tolerant of points inside phi planes by up to kCarTolerance*0.5
  //
  // o NOTE: Large duplication of code between sphi & ephi checks
  //         -> only diffs: sphi -> ephi, Comp -> -Comp and half-plane
  //            intersection check <=0 -> >=0
  //         -> use some form of loop Construct ?
  //
  if ( !fPhiFullTube )
  {
    // First phi surface (Starting phi)
    //
    Comp    = v.x()*sinSPhi - v.y()*cosSPhi ;
                    
    if ( Comp < 0 )  // Component in outwards normal dirn
    {
      Dist = (p.y()*cosSPhi - p.x()*sinSPhi) ;

      if ( Dist < halfCarTolerance )
      {
        s = Dist/Comp ;

        if (s < snxt)
        {
          if ( s < 0 )  { s = 0.0; }
          zi = p.z() + s*v.z() ;
          if ( std::fabs(zi) <= tolODz )
          {
            xi   = p.x() + s*v.x() ;
            yi   = p.y() + s*v.y() ;
            rho2 = xi*xi + yi*yi ;

            if ( ( (rho2 >= tolIRMin2) && (rho2 <= tolIRMax2) )
              || ( (rho2 >  tolORMin2) && (rho2 <  tolIRMin2)
                && ( v.y()*cosSPhi - v.x()*sinSPhi >  0 )
                && ( v.x()*cosSPhi + v.y()*sinSPhi >= 0 )     )
              || ( (rho2 > tolIRMax2) && (rho2 < tolORMax2)
                && (v.y()*cosSPhi - v.x()*sinSPhi > 0)
                && (v.x()*cosSPhi + v.y()*sinSPhi < 0) )    )
            {
              // z and r intersections good
              // - check intersecting with correct half-plane
              //
              if ((yi*cosCPhi-xi*sinCPhi) <= halfCarTolerance) { snxt = s; }
            }
          }
        }
      }    
    }
      
    // Second phi surface (Ending phi)

    Comp    = -(v.x()*sinEPhi - v.y()*cosEPhi) ;
        
    if (Comp < 0 )  // Component in outwards normal dirn
    {
      Dist = -(p.y()*cosEPhi - p.x()*sinEPhi) ;

      if ( Dist < halfCarTolerance )
      {
        s = Dist/Comp ;

        if (s < snxt)
        {
          if ( s < 0 )  { s = 0; }
          zi = p.z() + s*v.z() ;
          if ( std::fabs(zi) <= tolODz )
          {
            xi   = p.x() + s*v.x() ;
            yi   = p.y() + s*v.y() ;
            rho2 = xi*xi + yi*yi ;
            if ( ( (rho2 >= tolIRMin2) && (rho2 <= tolIRMax2) )
                || ( (rho2 > tolORMin2)  && (rho2 < tolIRMin2)
                  && (v.x()*sinEPhi - v.y()*cosEPhi >  0)
                  && (v.x()*cosEPhi + v.y()*sinEPhi >= 0) )
                || ( (rho2 > tolIRMax2) && (rho2 < tolORMax2)
                  && (v.x()*sinEPhi - v.y()*cosEPhi > 0)
                  && (v.x()*cosEPhi + v.y()*sinEPhi < 0) ) )
            {
              // z and r intersections good
              // - check intersecting with correct half-plane
              //
              if ( (yi*cosCPhi-xi*sinCPhi) >= 0 ) { snxt = s; }
            }                         //?? >=-halfCarTolerance
          }
        }
      }
    }         //  Comp < 0
  }           //  !fPhiFullTube 
  if ( snxt<halfCarTolerance )  { snxt=0; }
  return snxt ;
}
 
//////////////////////////////////////////////////////////////////
//
// Calculate distance to shape from outside, along normalised vector
// - return kInfinity if no intersection, or intersection distance <= tolerance
//
// - Compute the intersection with the z planes 
//        - if at valid r, phi, return
//
// -> If point is outer outer radius, compute intersection with rmax
//        - if at valid phi,z return
//
// -> Compute intersection with inner radius, taking largest +ve root
//        - if valid (in z,phi), save intersction
//
//    -> If phi segmented, compute intersections with phi half planes
//        - return smallest of valid phi intersections and
//          inner radius intersection
//
// NOTE:
// - Precalculations for phi trigonometry are Done `just in time'
// - `if valid' implies tolerant checking of intersection points
//   Calculate distance (<= actual) to closest surface of shape from outside
// - Calculate distance to z, radial planes
// - Only to phi planes if outside phi extent
// - Return 0 if point inside

G4double G4Tubs::DistanceToIn( const G4ThreeVector& p ) const
{
  G4double safe=0.0, rho, safe1, safe2, safe3 ;
  G4double safePhi, cosPsi ;

  rho   = std::sqrt(p.x()*p.x() + p.y()*p.y()) ;
  safe1 = fRMin - rho ;
  safe2 = rho - fRMax ;
  safe3 = std::fabs(p.z()) - fDz ;

  if ( safe1 > safe2 ) { safe = safe1; }
  else                 { safe = safe2; }
  if ( safe3 > safe )  { safe = safe3; }

  if ( (!fPhiFullTube) && (rho) )
  {
    // Psi=angle from central phi to point
    //
    cosPsi = (p.x()*cosCPhi + p.y()*sinCPhi)/rho ;
    
    if ( cosPsi < std::cos(fDPhi*0.5) )
    {
      // Point lies outside phi range

      if ( (p.y()*cosCPhi - p.x()*sinCPhi) <= 0 )
      {
        safePhi = std::fabs(p.x()*sinSPhi - p.y()*cosSPhi) ;
      }
      else
      {
        safePhi = std::fabs(p.x()*sinEPhi - p.y()*cosEPhi) ;
      }
      if ( safePhi > safe )  { safe = safePhi; }
    }
  }
  if ( safe < 0 )  { safe = 0; }
  return safe ;
}

//////////////////////////////////////////////////////////////////////////////
//
// Calculate distance to surface of shape from `inside', allowing for tolerance
// - Only Calc rmax intersection if no valid rmin intersection

G4double G4Tubs::DistanceToOut( const G4ThreeVector& p,
                                const G4ThreeVector& v,
                                const G4bool calcNorm,
                                      G4bool *validNorm,
                                      G4ThreeVector *n    ) const
{  
  ESide side=kNull , sider=kNull, sidephi=kNull ;
  G4double snxt, sr=kInfinity, sphi=kInfinity, pdist ;
  G4double deltaR, t1, t2, t3, b, c, d2, roMin2 ;

  static const G4double halfCarTolerance = kCarTolerance*0.5;
  static const G4double halfAngTolerance = kAngTolerance*0.5;
 
  // Vars for phi intersection:

  G4double pDistS, compS, pDistE, compE, sphi2, xi, yi, vphi, roi2 ;
 
  // Z plane intersection

  if (v.z() > 0 )
  {
    pdist = fDz - p.z() ;
    if ( pdist > halfCarTolerance )
    {
      snxt = pdist/v.z() ;
      side = kPZ ;
    }
    else
    {
      if (calcNorm)
      {
        *n         = G4ThreeVector(0,0,1) ;
        *validNorm = true ;
      }
      return snxt = 0 ;
    }
  }
  else if ( v.z() < 0 )
  {
    pdist = fDz + p.z() ;

    if ( pdist > halfCarTolerance )
    {
      snxt = -pdist/v.z() ;
      side = kMZ ;
    }
    else
    {
      if (calcNorm)
      {
        *n         = G4ThreeVector(0,0,-1) ;
        *validNorm = true ;
      }
      return snxt = 0.0 ;
    }
  }
  else
  {
    snxt = kInfinity ;    // Travel perpendicular to z axis
    side = kNull;
  }

  // Radial Intersections
  //
  // Find intersection with cylinders at rmax/rmin
  // Intersection point (xi,yi,zi) on line x=p.x+t*v.x etc.
  //
  // Intersects with x^2+y^2=R^2
  //
  // Hence (v.x^2+v.y^2)t^2+ 2t(p.x*v.x+p.y*v.y)+p.x^2+p.y^2-R^2=0
  //
  //            t1                t2                    t3

  t1   = 1.0 - v.z()*v.z() ;      // since v normalised
  t2   = p.x()*v.x() + p.y()*v.y() ;
  t3   = p.x()*p.x() + p.y()*p.y() ;

  if ( snxt > 10*(fDz+fRMax) )  { roi2 = 2*fRMax*fRMax; }
  else  { roi2 = snxt*snxt*t1 + 2*snxt*t2 + t3; }        // radius^2 on +-fDz

  if ( t1 > 0 ) // Check not parallel
  {
    // Calculate sr, r exit distance
     
    if ( (t2 >= 0.0) && (roi2 > fRMax*(fRMax + kRadTolerance)) )
    {
      // Delta r not negative => leaving via rmax

      deltaR = t3 - fRMax*fRMax ;

      // NOTE: Should use rho-fRMax<-kRadTolerance*0.5
      // - avoid sqrt for efficiency

      if ( deltaR < -kRadTolerance*fRMax )
      {
        b     = t2/t1 ;
        c     = deltaR/t1 ;
        d2    = b*b-c;
        if( d2 >= 0 ) { sr = -b + std::sqrt(d2); }
        else          { sr = 0.; }
        sider = kRMax ;
      }
      else
      {
        // On tolerant boundary & heading outwards (or perpendicular to)
        // outer radial surface -> leaving immediately

        if ( calcNorm ) 
        {
          *n         = G4ThreeVector(p.x()/fRMax,p.y()/fRMax,0) ;
          *validNorm = true ;
        }
        return snxt = 0 ; // Leaving by rmax immediately
      }
    }             
    else if ( t2 < 0. ) // i.e.  t2 < 0; Possible rmin intersection
    {
      roMin2 = t3 - t2*t2/t1 ; // min ro2 of the plane of movement 

      if ( fRMin && (roMin2 < fRMin*(fRMin - kRadTolerance)) )
      {
        deltaR = t3 - fRMin*fRMin ;
        b      = t2/t1 ;
        c      = deltaR/t1 ;
        d2     = b*b - c ;

        if ( d2 >= 0 )   // Leaving via rmin
        {
          // NOTE: SHould use rho-rmin>kRadTolerance*0.5
          // - avoid sqrt for efficiency

          if (deltaR > kRadTolerance*fRMin)
          {
            sr    = -b-std::sqrt(d2) ;
            sider = kRMin ;
          }
          else
          {
            if ( calcNorm ) { *validNorm = false; }  // Concave side
            return snxt = 0.0;
          }
        }
        else    // No rmin intersect -> must be rmax intersect
        {
          deltaR = t3 - fRMax*fRMax ;
          c     = deltaR/t1 ;
          d2    = b*b-c;
          if( d2 >=0. )
          {
            sr     = -b + std::sqrt(d2) ;
            sider  = kRMax ;
          }
          else // Case: On the border+t2<kRadTolerance
               //       (v is perpendicular to the surface)
          {
            if (calcNorm)
            {
              *n = G4ThreeVector(p.x()/fRMax,p.y()/fRMax,0) ;
              *validNorm = true ;
            }
            return snxt = 0.0;
          }
        }
      }
      else if ( roi2 > fRMax*(fRMax + kRadTolerance) )
           // No rmin intersect -> must be rmax intersect
      {
        deltaR = t3 - fRMax*fRMax ;
        b      = t2/t1 ;
        c      = deltaR/t1;
        d2     = b*b-c;
        if( d2 >= 0 )
        {
          sr     = -b + std::sqrt(d2) ;
          sider  = kRMax ;
        }
        else // Case: On the border+t2<kRadTolerance
             //       (v is perpendicular to the surface)
        {
          if (calcNorm)
          {
            *n = G4ThreeVector(p.x()/fRMax,p.y()/fRMax,0) ;
            *validNorm = true ;
          }
          return snxt = 0.0;
        }
      }
    }
    
    // Phi Intersection

    if ( !fPhiFullTube )
    {
      // add angle calculation with correction 
      // of the difference in domain of atan2 and Sphi
      //
      vphi = std::atan2(v.y(),v.x()) ;
     
      if ( vphi < fSPhi - halfAngTolerance  )             { vphi += twopi; }
      else if ( vphi > fSPhi + fDPhi + halfAngTolerance ) { vphi -= twopi; }


      if ( p.x() || p.y() )  // Check if on z axis (rho not needed later)
      {
        // pDist -ve when inside

        pDistS = p.x()*sinSPhi - p.y()*cosSPhi ;
        pDistE = -p.x()*sinEPhi + p.y()*cosEPhi ;

        // Comp -ve when in direction of outwards normal

        compS   = -sinSPhi*v.x() + cosSPhi*v.y() ;
        compE   =  sinEPhi*v.x() - cosEPhi*v.y() ;
       
        sidephi = kNull;
        
        if( ( (fDPhi <= pi) && ( (pDistS <= halfCarTolerance)
                              && (pDistE <= halfCarTolerance) ) )
         || ( (fDPhi >  pi) && !((pDistS >  halfCarTolerance)
                              && (pDistE >  halfCarTolerance) ) )  )
        {
          // Inside both phi *full* planes
          
          if ( compS < 0 )
          {
            sphi = pDistS/compS ;
            
            if (sphi >= -halfCarTolerance)
            {
              xi = p.x() + sphi*v.x() ;
              yi = p.y() + sphi*v.y() ;
              
              // Check intersecting with correct half-plane
              // (if not -> no intersect)
              //
              if( (std::abs(xi)<=kCarTolerance)&&(std::abs(yi)<=kCarTolerance) )
              {
                sidephi = kSPhi;
                if (((fSPhi-halfAngTolerance)<=vphi)
                   &&((fSPhi+fDPhi+halfAngTolerance)>=vphi))
                {
                  sphi = kInfinity;
                }
              }
              else if ( yi*cosCPhi-xi*sinCPhi >=0 )
              {
                sphi = kInfinity ;
              }
              else
              {
                sidephi = kSPhi ;
                if ( pDistS > -halfCarTolerance )
                {
                  sphi = 0.0 ; // Leave by sphi immediately
                }    
              }       
            }
            else
            {
              sphi = kInfinity ;
            }
          }
          else
          {
            sphi = kInfinity ;
          }

          if ( compE < 0 )
          {
            sphi2 = pDistE/compE ;
            
            // Only check further if < starting phi intersection
            //
            if ( (sphi2 > -halfCarTolerance) && (sphi2 < sphi) )
            {
              xi = p.x() + sphi2*v.x() ;
              yi = p.y() + sphi2*v.y() ;
              
              if ((std::abs(xi)<=kCarTolerance)&&(std::abs(yi)<=kCarTolerance))
              {
                // Leaving via ending phi
                //
                if( !((fSPhi-halfAngTolerance <= vphi)
                     &&(fSPhi+fDPhi+halfAngTolerance >= vphi)) )
                {
                  sidephi = kEPhi ;
                  if ( pDistE <= -halfCarTolerance )  { sphi = sphi2 ; }
                  else                                { sphi = 0.0 ;   }
                }
              } 
              else    // Check intersecting with correct half-plane 

              if ( (yi*cosCPhi-xi*sinCPhi) >= 0)
              {
                // Leaving via ending phi
                //
                sidephi = kEPhi ;
                if ( pDistE <= -halfCarTolerance ) { sphi = sphi2 ; }
                else                               { sphi = 0.0 ;   }
              }
            }
          }
        }
        else
        {
          sphi = kInfinity ;
        }
      }
      else
      {
        // On z axis + travel not || to z axis -> if phi of vector direction
        // within phi of shape, Step limited by rmax, else Step =0
               
        if ( (fSPhi - halfAngTolerance <= vphi)
           && (vphi <= fSPhi + fDPhi + halfAngTolerance ) )
        {
          sphi = kInfinity ;
        }
        else
        {
          sidephi = kSPhi ; // arbitrary 
          sphi    = 0.0 ;
        }
      }
      if (sphi < snxt)  // Order intersecttions
      {
        snxt = sphi ;
        side = sidephi ;
      }
    }
    if (sr < snxt)  // Order intersections
    {
      snxt = sr ;
      side = sider ;
    }
  }
  if (calcNorm)
  {
    switch(side)
    {
      case kRMax:
        // Note: returned vector not normalised
        // (divide by fRMax for unit vector)
        //
        xi = p.x() + snxt*v.x() ;
        yi = p.y() + snxt*v.y() ;
        *n = G4ThreeVector(xi/fRMax,yi/fRMax,0) ;
        *validNorm = true ;
        break ;

      case kRMin:
        *validNorm = false ;  // Rmin is inconvex
        break ;

      case kSPhi:
        if ( fDPhi <= pi )
        {
          *n         = G4ThreeVector(sinSPhi,-cosSPhi,0) ;
          *validNorm = true ;
        }
        else
        {
          *validNorm = false ;
        }
        break ;

      case kEPhi:
        if (fDPhi <= pi)
        {
          *n = G4ThreeVector(-sinEPhi,cosEPhi,0) ;
          *validNorm = true ;
        }
        else
        {
          *validNorm = false ;
        }
        break ;

      case kPZ:
        *n         = G4ThreeVector(0,0,1) ;
        *validNorm = true ;
        break ;

      case kMZ:
        *n         = G4ThreeVector(0,0,-1) ;
        *validNorm = true ;
        break ;

      default:
/*        G4cout.precision(16) ;
        G4cout << G4endl ;
        DumpInfo();
        G4cout << "Position:"  << G4endl << G4endl ;
        G4cout << "p.x() = "   << p.x()/mm << " mm" << G4endl ;
        G4cout << "p.y() = "   << p.y()/mm << " mm" << G4endl ;
        G4cout << "p.z() = "   << p.z()/mm << " mm" << G4endl << G4endl ;
        G4cout << "Direction:" << G4endl << G4endl ;
        G4cout << "v.x() = "   << v.x() << G4endl ;
        G4cout << "v.y() = "   << v.y() << G4endl ;
        G4cout << "v.z() = "   << v.z() << G4endl << G4endl ;
        G4cout << "Proposed distance :" << G4endl << G4endl ;
        G4cout << "snxt = "    << snxt/mm << " mm" << G4endl << G4endl ;
        G4Exception("G4Tubs::DistanceToOut(p,v,..)","Notification",JustWarning,
                    "Undefined side for valid surface normal to solid.");
*/        break ;
    }
  }
  if ( snxt<halfCarTolerance )  { snxt=0 ; }

  return snxt ;
}

//////////////////////////////////////////////////////////////////////////
//
// Calculate distance (<=actual) to closest surface of shape from inside

G4double G4Tubs::DistanceToOut( const G4ThreeVector& p ) const
{
  G4double safe=0.0, rho, safeR1, safeR2, safeZ, safePhi ;
  rho = std::sqrt(p.x()*p.x() + p.y()*p.y()) ;

#ifdef G4CSGDEBUG
  if( Inside(p) == kOutside )
  {
    G4cout.precision(16) ;
    G4cout << G4endl ;
    DumpInfo();
    G4cout << "Position:"  << G4endl << G4endl ;
    G4cout << "p.x() = "   << p.x()/mm << " mm" << G4endl ;
    G4cout << "p.y() = "   << p.y()/mm << " mm" << G4endl ;
    G4cout << "p.z() = "   << p.z()/mm << " mm" << G4endl << G4endl ;
    G4Exception("G4Tubs::DistanceToOut(p)", "Notification", JustWarning, 
                 "Point p is outside !?");
  }
#endif

  if ( fRMin )
  {
    safeR1 = rho   - fRMin ;
    safeR2 = fRMax - rho ;
 
    if ( safeR1 < safeR2 ) { safe = safeR1 ; }
    else                   { safe = safeR2 ; }
  }
  else
  {
    safe = fRMax - rho ;
  }
  safeZ = fDz - std::fabs(p.z()) ;

  if ( safeZ < safe )  { safe = safeZ ; }

  // Check if phi divided, Calc distances closest phi plane
  //
  if ( !fPhiFullTube )
  {
    if ( p.y()*cosCPhi-p.x()*sinCPhi <= 0 )
    {
      safePhi = -(p.x()*sinSPhi - p.y()*cosSPhi) ;
    }
    else
    {
      safePhi = (p.x()*sinEPhi - p.y()*cosEPhi) ;
    }
    if (safePhi < safe)  { safe = safePhi ; }
  }
  if ( safe < 0 )  { safe = 0 ; }

  return safe ;  
}

/////////////////////////////////////////////////////////////////////////
//
// Create a List containing the transformed vertices
// Ordering [0-3] -fDz cross section
//          [4-7] +fDz cross section such that [0] is below [4],
//                                             [1] below [5] etc.
// Note:
//  Caller has deletion resposibility
//  Potential improvement: For last slice, use actual ending angle
//                         to avoid rounding error problems.

G4ThreeVectorList*
G4Tubs::CreateRotatedVertices( const G4AffineTransform& pTransform ) const
{
  G4ThreeVectorList* vertices ;
  G4ThreeVector vertex0, vertex1, vertex2, vertex3 ;
  G4double meshAngle, meshRMax, crossAngle,
           cosCrossAngle, sinCrossAngle, sAngle;
  G4double rMaxX, rMaxY, rMinX, rMinY, meshRMin ;
  G4int crossSection, noCrossSections;

  // Compute no of cross-sections necessary to mesh tube
  //
  noCrossSections = G4int(fDPhi/kMeshAngleDefault) + 1 ;

  if ( noCrossSections < kMinMeshSections )
  {
    noCrossSections = kMinMeshSections ;
  }
  else if (noCrossSections>kMaxMeshSections)
  {
    noCrossSections = kMaxMeshSections ;
  }
  // noCrossSections = 4 ;

  meshAngle = fDPhi/(noCrossSections - 1) ;
  // meshAngle = fDPhi/(noCrossSections) ;

  meshRMax  = (fRMax+100*kCarTolerance)/std::cos(meshAngle*0.5) ;
  meshRMin = fRMin - 100*kCarTolerance ; 
 
  // If complete in phi, set start angle such that mesh will be at fRMax
  // on the x axis. Will give better extent calculations when not rotated.

  if (fPhiFullTube && (fSPhi == 0) )  { sAngle = -meshAngle*0.5 ; }
  else                                { sAngle =  fSPhi ; }
    
  vertices = new G4ThreeVectorList();
  vertices->reserve(noCrossSections*4);
    
  if ( vertices )
  {
    for (crossSection = 0 ; crossSection < noCrossSections ; crossSection++ )
    {
      // Compute coordinates of cross section at section crossSection

      crossAngle    = sAngle + crossSection*meshAngle ;
      cosCrossAngle = std::cos(crossAngle) ;
      sinCrossAngle = std::sin(crossAngle) ;

      rMaxX = meshRMax*cosCrossAngle ;
      rMaxY = meshRMax*sinCrossAngle ;

      if(meshRMin <= 0.0)
      {
        rMinX = 0.0 ;
        rMinY = 0.0 ;
      }
      else
      {
        rMinX = meshRMin*cosCrossAngle ;
        rMinY = meshRMin*sinCrossAngle ;
      }
      vertex0 = G4ThreeVector(rMinX,rMinY,-fDz) ;
      vertex1 = G4ThreeVector(rMaxX,rMaxY,-fDz) ;
      vertex2 = G4ThreeVector(rMaxX,rMaxY,+fDz) ;
      vertex3 = G4ThreeVector(rMinX,rMinY,+fDz) ;

      vertices->push_back(pTransform.TransformPoint(vertex0)) ;
      vertices->push_back(pTransform.TransformPoint(vertex1)) ;
      vertices->push_back(pTransform.TransformPoint(vertex2)) ;
      vertices->push_back(pTransform.TransformPoint(vertex3)) ;
    }
  }
  else
  {
/*    DumpInfo();
    G4Exception("G4Tubs::CreateRotatedVertices()",
                "FatalError", FatalException,
                "Error in allocation of vertices. Out of memory !");
*/  }
  return vertices ;
}

//////////////////////////////////////////////////////////////////////////
//
// Stream object contents to an output stream

G4GeometryType G4Tubs::GetEntityType() const
{
  return G4String("G4Tubs");
}

//////////////////////////////////////////////////////////////////////////
//
// Stream object contents to an output stream

std::ostream& G4Tubs::StreamInfo( std::ostream& os ) const
{
  os << "-----------------------------------------------------------\n"
     << "    *** Dump for solid - " << GetName() << " ***\n"
     << "    ===================================================\n"
     << " Solid type: G4Tubs\n"
     << " Parameters: \n"
     << "    inner radius : " << fRMin/mm << " mm \n"
     << "    outer radius : " << fRMax/mm << " mm \n"
     << "    half length Z: " << fDz/mm << " mm \n"
     << "    starting phi : " << fSPhi/degree << " degrees \n"
     << "    delta phi    : " << fDPhi/degree << " degrees \n"
     << "-----------------------------------------------------------\n";

  return os;
}

/////////////////////////////////////////////////////////////////////////
//
// GetPointOnSurface

G4ThreeVector G4Tubs::GetPointOnSurface() const
{
	//BEGIN CAD GDML
	//no time to add RandFlat::shoot
	return G4ThreeVector  (0., 0., 0.);
	//END CAD GDML
	/*
  G4double xRand, yRand, zRand, phi, cosphi, sinphi, chose,
           aOne, aTwo, aThr, aFou;
  G4double rRand;

  aOne = 2.*fDz*fDPhi*fRMax;
  aTwo = 2.*fDz*fDPhi*fRMin;
  aThr = 0.5*fDPhi*(fRMax*fRMax-fRMin*fRMin);
  aFou = 2.*fDz*(fRMax-fRMin);

  phi    = RandFlat::shoot(fSPhi, fSPhi+fDPhi);
  cosphi = std::cos(phi);
  sinphi = std::sin(phi);

  rRand  = RandFlat::shoot(fRMin,fRMax);
  
  if( (fSPhi == 0) && (fDPhi == twopi) ) { aFou = 0; }
  
  chose  = RandFlat::shoot(0.,aOne+aTwo+2.*aThr+2.*aFou);

  if( (chose >=0) && (chose < aOne) )
  {
    xRand = fRMax*cosphi;
    yRand = fRMax*sinphi;
    zRand = RandFlat::shoot(-1.*fDz,fDz);
    return G4ThreeVector  (xRand, yRand, zRand);
  }
  else if( (chose >= aOne) && (chose < aOne + aTwo) )
  {
    xRand = fRMin*cosphi;
    yRand = fRMin*sinphi;
    zRand = RandFlat::shoot(-1.*fDz,fDz);
    return G4ThreeVector  (xRand, yRand, zRand);
  }
  else if( (chose >= aOne + aTwo) && (chose < aOne + aTwo + aThr) )
  {
    xRand = rRand*cosphi;
    yRand = rRand*sinphi;
    zRand = fDz;
    return G4ThreeVector  (xRand, yRand, zRand);
  }
  else if( (chose >= aOne + aTwo + aThr) && (chose < aOne + aTwo + 2.*aThr) )
  {
    xRand = rRand*cosphi;
    yRand = rRand*sinphi;
    zRand = -1.*fDz;
    return G4ThreeVector  (xRand, yRand, zRand);
  }
  else if( (chose >= aOne + aTwo + 2.*aThr)
        && (chose < aOne + aTwo + 2.*aThr + aFou) )
  {
    xRand = rRand*std::cos(fSPhi);
    yRand = rRand*std::sin(fSPhi);
    zRand = RandFlat::shoot(-1.*fDz,fDz);
    return G4ThreeVector  (xRand, yRand, zRand);
  }
  else
  {
    xRand = rRand*std::cos(fSPhi+fDPhi);
    yRand = rRand*std::sin(fSPhi+fDPhi);
    zRand = RandFlat::shoot(-1.*fDz,fDz);
    return G4ThreeVector  (xRand, yRand, zRand);
  }
  */
}
/*
///////////////////////////////////////////////////////////////////////////
//
// Methods for visualisation

void G4Tubs::DescribeYourselfTo ( G4VGraphicsScene& scene ) const 
{
  scene.AddSolid (*this) ;
}
*/
G4Polyhedron* G4Tubs::CreatePolyhedron () const 
{
  return new G4PolyhedronTubs (fRMin, fRMax, fDz, fSPhi, fDPhi) ;
}
/*
G4NURBS* G4Tubs::CreateNURBS () const 
{
  G4NURBS* pNURBS ;
  if (fRMin != 0) 
  {
    if (fPhiFullTube) 
    {
      pNURBS = new G4NURBStube (fRMin,fRMax,fDz) ;
    }
    else 
    {
      pNURBS = new G4NURBStubesector (fRMin,fRMax,fDz,fSPhi,fSPhi+fDPhi) ;
    }
  }
  else 
  {
    if (fPhiFullTube) 
    {
      pNURBS = new G4NURBScylinder (fRMax,fDz) ;
    }
    else 
    {
      const G4double epsilon = 1.e-4 ; // Cylinder sector not yet available!
      pNURBS = new G4NURBStubesector (epsilon,fRMax,fDz,fSPhi,fSPhi+fDPhi) ;
    }
  }
  return pNURBS ;
}
*/