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
// $Id: G4VoxelLimits.cc,v 1.11 2006/06/29 18:34:11 gunter Exp $
// GEANT4 tag $Name: geant4-09-03-patch-01 $
//
// class G4VoxelLimits
// 
// Implementation
//
// History:
//
// 14.03.02 V. Grichine, cosmetics
// 13.07.95 P.Kent Initial version
// --------------------------------------------------------------------

#include "G4VoxelLimits.hh"

///#include "G4ios.hh"
#include "../../global/G4ios.hh"

///////////////////////////////////////////////////////////////////////////
//
// Empty constructor and destructor
//

G4VoxelLimits::G4VoxelLimits()
 : fxAxisMin(-kInfinity),fxAxisMax(kInfinity),
   fyAxisMin(-kInfinity),fyAxisMax(kInfinity),
   fzAxisMin(-kInfinity),fzAxisMax(kInfinity)
{
}

G4VoxelLimits::~G4VoxelLimits()
{
}

///////////////////////////////////////////////////////////////////////////
//
// Further restrict limits
// No checks for illegal restrictions
//

void G4VoxelLimits::AddLimit( const EAxis pAxis, 
                              const G4double pMin,
                              const G4double pMax )
{
  if ( pAxis == kXAxis )
  {
    if ( pMin > fxAxisMin ) fxAxisMin = pMin ;    
    if ( pMax < fxAxisMax ) fxAxisMax = pMax ;    
  }
  else if ( pAxis == kYAxis )
  {
    if ( pMin > fyAxisMin ) fyAxisMin = pMin ;    
    if ( pMax < fyAxisMax ) fyAxisMax = pMax ;
  }
  else
  { 
    assert( pAxis == kZAxis ) ;

    if ( pMin > fzAxisMin ) fzAxisMin = pMin ;
    if ( pMax < fzAxisMax ) fzAxisMax = pMax ;
  }
}

///////////////////////////////////////////////////////////////////////////
//
// ClipToLimits
//
// Clip the line segment pStart->pEnd to the volume described by the
// current limits. Return true if the line remains after clipping,
// else false, and leave the vectors in an undefined state.
//
// Process:
//
// Use Cohen-Sutherland clipping in 3D
// [Fundamentals of Interactive Computer Graphics,Foley & Van Dam]
//

G4bool G4VoxelLimits::ClipToLimits( G4ThreeVector& pStart,
                                    G4ThreeVector& pEnd      ) const
{
  G4int sCode, eCode ;
  G4bool remainsAfterClip ;
    
  // Determine if line is trivially inside (both outcodes==0) or outside
  // (logical AND of outcodes !=0)

  sCode = OutCode(pStart) ;
  eCode = OutCode(pEnd)   ;

  if ( sCode & eCode )
  {
    // Trivially outside, no intersection with region

    remainsAfterClip = false;
  }
  else if ( sCode == 0 && eCode == 0 )
  {
    // Trivially inside, no intersections

    remainsAfterClip = true ;
  }
  else
  {
    // Line segment *may* cut volume boundaries
    // At most, one end point is inside

    G4double x1, y1, z1, x2, y2, z2 ;

    x1 = pStart.x() ;
    y1 = pStart.y() ;
    z1 = pStart.z() ;

    x2 = pEnd.x() ;
    y2 = pEnd.y() ;
    z2 = pEnd.z() ;
    /*
    if( std::abs(x1-x2) < kCarTolerance*kCarTolerance)
    {
      G4cout<<"x1 = "<<x1<<"\t"<<"x2 = "<<x2<<G4endl; 
    }   
    if( std::abs(y1-y2) < kCarTolerance*kCarTolerance)
    {
      G4cout<<"y1 = "<<y1<<"\t"<<"y2 = "<<y2<<G4endl; 
    }   
    if( std::abs(z1-z2) < kCarTolerance*kCarTolerance)
    {
      G4cout<<"z1 = "<<z1<<"\t"<<"z2 = "<<z2<<G4endl; 
    } 
    */  
    while ( sCode != eCode )
    {
      // Copy vectors to work variables x1-z1,x2-z2
      // Ensure x1-z1 lies outside volume, swapping vectors and outcodes
      // if necessary

      if ( sCode )
      {
        if ( sCode & 0x01 )  // Clip against fxAxisMin
        {
          z1 += (fxAxisMin-x1)*(z2-z1)/(x2-x1);
          y1 += (fxAxisMin-x1)*(y2-y1)/(x2-x1);
          x1  = fxAxisMin;
        }
        else if ( sCode & 0x02 ) // Clip against fxAxisMax
        {
          z1 += (fxAxisMax-x1)*(z2-z1)/(x2-x1);
          y1 += (fxAxisMax-x1)*(y2-y1)/(x2-x1);
          x1  = fxAxisMax ;
        }
        else if ( sCode & 0x04 )  // Clip against fyAxisMin
        {
          x1 += (fyAxisMin-y1)*(x2-x1)/(y2-y1);
          z1 += (fyAxisMin-y1)*(z2-z1)/(y2-y1);
          y1  = fyAxisMin;
        }
        else if ( sCode & 0x08 )  // Clip against fyAxisMax
        {
          x1 += (fyAxisMax-y1)*(x2-x1)/(y2-y1);
          z1 += (fyAxisMax-y1)*(z2-z1)/(y2-y1);
          y1  = fyAxisMax;
        }
        else if ( sCode & 0x10 )  // Clip against fzAxisMin
        {
          x1 += (fzAxisMin-z1)*(x2-x1)/(z2-z1);
          y1 += (fzAxisMin-z1)*(y2-y1)/(z2-z1);
          z1  = fzAxisMin;
        }
        else if ( sCode & 0x20 )  // Clip against fzAxisMax
        {
          x1 += (fzAxisMax-z1)*(x2-x1)/(z2-z1);
          y1 += (fzAxisMax-z1)*(y2-y1)/(z2-z1);
          z1  = fzAxisMax;
        }
      }
      if ( eCode )  // Clip 2nd end: repeat of 1st, but 1<>2
      {
        if ( eCode & 0x01 )  // Clip against fxAxisMin
        {
          z2 += (fxAxisMin-x2)*(z1-z2)/(x1-x2);
          y2 += (fxAxisMin-x2)*(y1-y2)/(x1-x2);
          x2  = fxAxisMin;
        }
        else if ( eCode & 0x02 )  // Clip against fxAxisMax
        {
          z2 += (fxAxisMax-x2)*(z1-z2)/(x1-x2);
          y2 += (fxAxisMax-x2)*(y1-y2)/(x1-x2);
          x2  = fxAxisMax;
        }
        else if ( eCode & 0x04 )  // Clip against fyAxisMin
        {
          x2 += (fyAxisMin-y2)*(x1-x2)/(y1-y2);
          z2 += (fyAxisMin-y2)*(z1-z2)/(y1-y2);
          y2  = fyAxisMin;
        }
        else if (eCode&0x08)  // Clip against fyAxisMax
        {
          x2 += (fyAxisMax-y2)*(x1-x2)/(y1-y2);
          z2 += (fyAxisMax-y2)*(z1-z2)/(y1-y2);
          y2  = fyAxisMax;
        }
        else if ( eCode & 0x10 )  // Clip against fzAxisMin
        {
          x2 += (fzAxisMin-z2)*(x1-x2)/(z1-z2);
          y2 += (fzAxisMin-z2)*(y1-y2)/(z1-z2);
          z2  = fzAxisMin;
        }
        else if ( eCode & 0x20 )  // Clip against fzAxisMax
        {
          x2 += (fzAxisMax-z2)*(x1-x2)/(z1-z2);
          y2 += (fzAxisMax-z2)*(y1-y2)/(z1-z2);
          z2  = fzAxisMax;
        }
      }
      //  G4endl; G4cout<<"x1 = "<<x1<<"\t"<<"x2 = "<<x2<<G4endl<<G4endl;
      pStart = G4ThreeVector(x1,y1,z1);
      pEnd   = G4ThreeVector(x2,y2,z2);
      sCode  = OutCode(pStart);
      eCode  = OutCode(pEnd);
    }
    if ( sCode == 0 && eCode == 0 ) remainsAfterClip = true;
    else                            remainsAfterClip = false;
  }
  return remainsAfterClip;
}

////////////////////////////////////////////////////////////////////////////
//
// Calculate the `outcode' for the specified vector:
// The following bits are set:
//   0      pVec.x()<fxAxisMin && IsXLimited()
//   1      pVec.x()>fxAxisMax && IsXLimited()
//   2      pVec.y()<fyAxisMin && IsYLimited()
//   3      pVec.y()>fyAxisMax && IsYLimited()
//   4      pVec.z()<fzAxisMin && IsZLimited()
//   5      pVec.z()>fzAxisMax && IsZLimited()
//

G4int G4VoxelLimits::OutCode( const G4ThreeVector& pVec ) const
{
  G4int code = 0 ;                // The outcode

  if ( IsXLimited() )
  {
    if ( pVec.x() < fxAxisMin ) code |= 0x01 ;
    if ( pVec.x() > fxAxisMax ) code |= 0x02 ;
  }
  if ( IsYLimited() )
  {
    if ( pVec.y() < fyAxisMin ) code |= 0x04 ;
    if ( pVec.y() > fyAxisMax ) code |= 0x08 ;
  }
  if (IsZLimited())
  {
    if ( pVec.z() < fzAxisMin ) code |= 0x10 ;
    if ( pVec.z() > fzAxisMax ) code |= 0x20 ;
  }
  return code;
}

///////////////////////////////////////////////////////////////////////////////

std::ostream& operator << (std::ostream& os, const G4VoxelLimits& pLim)
{
    os << "{";
    if (pLim.IsXLimited())
        {
            os << "(" << pLim.GetMinXExtent() 
               << "," << pLim.GetMaxXExtent() << ") ";
        }
    else
        {
            os << "(-,-) ";
        }
    if (pLim.IsYLimited())
        {
            os << "(" << pLim.GetMinYExtent() 
               << "," << pLim.GetMaxYExtent() << ") ";
        }
    else
        {
            os << "(-,-) ";
        }
    if (pLim.IsZLimited())
        {
            os << "(" << pLim.GetMinZExtent()
               << "," << pLim.GetMaxZExtent() << ")";
        }
    else
        {
            os << "(-,-)";
        }
    os << "}";
    return os;
}
