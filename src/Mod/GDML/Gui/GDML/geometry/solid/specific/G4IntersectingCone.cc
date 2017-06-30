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
// $Id: G4IntersectingCone.cc,v 1.12 2008/04/28 08:59:47 gcosmo Exp $
// GEANT4 tag $Name: geant4-09-03-patch-01 $
//
// 
// --------------------------------------------------------------------
// GEANT 4 class source file
//
//
// G4IntersectingCone.cc
//
// Implementation of a utility class which calculates the intersection
// of an arbitrary line with a fixed cone
// --------------------------------------------------------------------

#include "G4IntersectingCone.hh"
///#include "G4GeometryTolerance.hh"
#include "../../../global/G4GeometryTolerance.hh"

//
// Constructor
//
G4IntersectingCone::G4IntersectingCone( const G4double r[2],
                                        const G4double z[2] )
{ 


  half_kCarTolerance = 0.5 * G4GeometryTolerance::GetInstance()
                             ->GetSurfaceTolerance(); 
  //
  // What type of cone are we?
  //
  type1 = (std::fabs(z[1]-z[0]) > std::fabs(r[1]-r[0]));
  
  if (type1)
  {
    B = (r[1]-r[0])/(z[1]-z[0]);      // tube like
    A = 0.5*( r[1]+r[0] - B*(z[1]+z[0]) );
  }
  else
  {
    B = (z[1]-z[0])/(r[1]-r[0]);      // disk like
    A = 0.5*( z[1]+z[0] - B*(r[1]+r[0]) );
  }
  //
  // Calculate extent
  //
  if (r[0] < r[1])
  {
    rLo = r[0]-half_kCarTolerance; rHi = r[1]+half_kCarTolerance;
  }
  else
  {
    rLo = r[1]-half_kCarTolerance; rHi = r[0]+half_kCarTolerance;
  }
  
  if (z[0] < z[1])
  {
    zLo = z[0]-half_kCarTolerance; zHi = z[1]+half_kCarTolerance;
  }
  else
  {
    zLo = z[1]-half_kCarTolerance; zHi = z[0]+half_kCarTolerance;
  }
}


//
// Fake default constructor - sets only member data and allocates memory
//                            for usage restricted to object persistency.
//
G4IntersectingCone::G4IntersectingCone( __void__& )
{
}


//
// Destructor
//
G4IntersectingCone::~G4IntersectingCone()
{
}


//
// HitOn
//
// Check r or z extent, as appropriate, to see if the point is possibly
// on the cone.
//
G4bool G4IntersectingCone::HitOn( const G4double r,
                                  const G4double z )
{
  //
  // Be careful! The inequalities cannot be "<=" and ">=" here without
  // punching a tiny hole in our shape!
  //
  if (type1)
  {
    if (z < zLo || z > zHi) return false;
  }
  else
  {
    if (r < rLo || r > rHi) return false;
  }

  return true;
}


//
// LineHitsCone
//
// Calculate the intersection of a line with our conical surface, ignoring
// any phi division
//
G4int G4IntersectingCone::LineHitsCone( const G4ThreeVector &p,
                                        const G4ThreeVector &v,
                                              G4double *s1, G4double *s2 )
{
  if (type1)
  {
    return LineHitsCone1( p, v, s1, s2 );
  }
  else
  {
    return LineHitsCone2( p, v, s1, s2 );
  }
}


//
// LineHitsCone1
//
// Calculate the intersections of a line with a conical surface. Only
// suitable if zPlane[0] != zPlane[1].
//
// Equation of a line:
//
//       x = x0 + s*tx      y = y0 + s*ty      z = z0 + s*tz
//
// Equation of a conical surface:
//
//       x**2 + y**2 = (A + B*z)**2
//
// Solution is quadratic:
//
//  a*s**2 + b*s + c = 0
//
// where:
//
//  a = x0**2 + y0**2 - (A + B*z0)**2
//
//  b = 2*( x0*tx + y0*ty - (A*B - B*B*z0)*tz)
//
//  c = tx**2 + ty**2 - (B*tz)**2
//
// Notice, that if a < 0, this indicates that the two solutions (assuming
// they exist) are in opposite cones (that is, given z0 = -A/B, one z < z0
// and the other z > z0). For our shapes, the invalid solution is one
// which produces A + Bz < 0, or the one where Bz is smallest (most negative).
// Since Bz = B*s*tz, if B*tz > 0, we want the largest s, otherwise,
// the smaller.
//
// If there are two solutions on one side of the cone, we want to make
// sure that they are on the "correct" side, that is A + B*z0 + s*B*tz >= 0.
//
// If a = 0, we have a linear problem: s = c/b, which again gives one solution.
// This should be rare.
//
// For b*b - 4*a*c = 0, we also have one solution, which is almost always
// a line just grazing the surface of a the cone, which we want to ignore. 
// However, there are two other, very rare, possibilities:
// a line intersecting the z axis and either:
//       1. At the same angle std::atan(B) to just miss one side of the cone, or
//       2. Intersecting the cone apex (0,0,-A/B)
// We *don't* want to miss these! How do we identify them? Well, since
// this case is rare, we can at least swallow a little more CPU than we would
// normally be comfortable with. Intersection with the z axis means
// x0*ty - y0*tx = 0. Case (1) means a==0, and we've already dealt with that
// above. Case (2) means a < 0.
//
// Now: x0*tx + y0*ty = 0 in terms of roundoff error. We can write:
//             Delta = x0*tx + y0*ty
//             b = 2*( Delta - (A*B + B*B*z0)*tz )
// For:
//             b*b - 4*a*c = epsilon
// where epsilon is small, then:
//             Delta = epsilon/2/B
// 
G4int G4IntersectingCone::LineHitsCone1( const G4ThreeVector &p,
                                         const G4ThreeVector &v,
                                               G4double *s1, G4double *s2 )
{
  G4double x0 = p.x(), y0 = p.y(), z0 = p.z();
  G4double tx = v.x(), ty = v.y(), tz = v.z();

  G4double a = tx*tx + ty*ty - sqr(B*tz);
  G4double b = 2*( x0*tx + y0*ty - (A*B + B*B*z0)*tz);
  G4double c = x0*x0 + y0*y0 - sqr(A + B*z0);
  
  G4double radical = b*b - 4*a*c;
 
  if (radical < -1E-6*std::fabs(b))  { return 0; }    // No solution
  
  if (radical < 1E-6*std::fabs(b))
  {
    //
    // The radical is roughly zero: check for special, very rare, cases
    //
    if (std::fabs(a) > 1/kInfinity)
      {
      if(B==0.) { return 0; }
      if ( std::fabs(x0*ty - y0*tx) < std::fabs(1E-6/B) )
      {
         *s1 = -0.5*b/a;
         return 1;
      }
      return 0;
    }
  }
  else
  {
    radical = std::sqrt(radical);
  }
  
  if (a > 1/kInfinity)
  {
    G4double sa, sb, q = -0.5*( b + (b < 0 ? -radical : +radical) );
    sa = q/a;
    sb = c/q;
    if (sa < sb) { *s1 = sa; *s2 = sb; } else { *s1 = sb; *s2 = sa; }
    if (A + B*(z0+(*s1)*tz) < 0)  { return 0; }
    return 2;
  }
  else if (a < -1/kInfinity)
  {
    G4double sa, sb, q = -0.5*( b + (b < 0 ? -radical : +radical) );
    sa = q/a;
    sb = c/q;
    *s1 = (B*tz > 0)^(sa > sb) ? sb : sa;
    return 1;
  }
  else if (std::fabs(b) < 1/kInfinity)
  {
    return 0;
  }
  else
  {
    *s1 = -c/b;
    if (A + B*(z0+(*s1)*tz) < 0)  { return 0; }
    return 1;
  }
}

  
//
// LineHitsCone2
//
// See comments under LineHitsCone1. In this routine, case2, we have:
//
//   Z = A + B*R
//
// The solution is still quadratic:
//
//  a = tz**2 - B*B*(tx**2 + ty**2)
//
//  b = 2*( (z0-A)*tz - B*B*(x0*tx+y0*ty) )
//
//  c = ( (z0-A)**2 - B*B*(x0**2 + y0**2) )
//
// The rest is much the same, except some details.
//
// a > 0 now means we intersect only once in the correct hemisphere.
//
// a > 0 ? We only want solution which produces R > 0. 
// since R = (z0+s*tz-A)/B, for tz/B > 0, this is the largest s
//          for tz/B < 0, this is the smallest s
// thus, same as in case 1 ( since sign(tz/B) = sign(tz*B) )
//
G4int G4IntersectingCone::LineHitsCone2( const G4ThreeVector &p,
                                         const G4ThreeVector &v,
                                               G4double *s1, G4double *s2 )
{
  G4double x0 = p.x(), y0 = p.y(), z0 = p.z();
  G4double tx = v.x(), ty = v.y(), tz = v.z();
  
  
  // Special case which might not be so rare: B = 0 (precisely)
  //
  if (B==0)
  {
    if (std::fabs(tz) < 1/kInfinity)  { return 0; }
    
    *s1 = (A-z0)/tz;
    return 1;
  }

  G4double B2 = B*B;

  G4double a = tz*tz - B2*(tx*tx + ty*ty);
  G4double b = 2*( (z0-A)*tz - B2*(x0*tx + y0*ty) );
  G4double c = sqr(z0-A) - B2*( x0*x0 + y0*y0 );
  
  G4double radical = b*b - 4*a*c;
 
  if (radical < -1E-6*std::fabs(b)) { return 0; }   // No solution
  
  if (radical < 1E-6*std::fabs(b))
  {
    //
    // The radical is roughly zero: check for special, very rare, cases
    //
    if (std::fabs(a) > 1/kInfinity)
    {
      if ( std::fabs(x0*ty - y0*tx) < std::fabs(1E-6/B) )
      {
        *s1 = -0.5*b/a;
        return 1;
      }
      return 0;
    }
  }
  else
  {
    radical = std::sqrt(radical);
  }
  
  if (a < -1/kInfinity)
  {
    G4double sa, sb, q = -0.5*( b + (b < 0 ? -radical : +radical) );
    sa = q/a;
    sb = c/q;
    if (sa < sb) { *s1 = sa; *s2 = sb; } else { *s1 = sb; *s2 = sa; }
    if ((z0 + (*s1)*tz  - A)/B < 0)  { return 0; }
    return 2;
  }
  else if (a > 1/kInfinity)
  {
    G4double sa, sb, q = -0.5*( b + (b < 0 ? -radical : +radical) );
    sa = q/a;
    sb = c/q;
    *s1 = (tz*B > 0)^(sa > sb) ? sb : sa;
    return 1;
  }
  else if (std::fabs(b) < 1/kInfinity)
  {
    return 0;
  }
  else
  {
    *s1 = -c/b;
    if ((z0 + (*s1)*tz  - A)/B < 0)  { return 0; }
    return 1;
  }
}
