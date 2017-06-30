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
// $Id: G4PolyPhiFace.cc,v 1.15 2008/05/15 11:41:59 gcosmo Exp $
// GEANT4 tag $Name: geant4-09-03-patch-01 $
//
// 
// --------------------------------------------------------------------
// GEANT 4 class source file
//
//
// G4PolyPhiFace.cc
//
// Implementation of the face that bounds a polycone or polyhedra at
// its phi opening.
//
// --------------------------------------------------------------------

#include "G4PolyPhiFace.hh"
///#include "G4ClippablePolygon.hh"
#include "G4ReduciblePolygon.hh"
///#include "G4AffineTransform.hh"
#include "../../management/G4AffineTransform.hh"
///#include "G4SolidExtentList.hh"
///#include "G4GeometryTolerance.hh"
#include "../../../global/G4GeometryTolerance.hh"

///#include "Randomize.hh"
///#include "G4TwoVector.hh"
#include "../../../global/G4TwoVector.hh"

//
// Constructor
//
// Points r,z should be supplied in clockwise order in r,z. For example:
//
//                [1]---------[2]         ^ R
//                 |           |          |
//                 |           |          +--> z
//                [0]---------[3]
//
G4PolyPhiFace::G4PolyPhiFace( const G4ReduciblePolygon *rz,
                                    G4double phi, 
                                    G4double deltaPhi,
                                    G4double phiOther )
{
  kCarTolerance = G4GeometryTolerance::GetInstance()->GetSurfaceTolerance();
  fSurfaceArea = 0.;  

  numEdges = rz->NumVertices();
  
  rMin = rz->Amin();
  rMax = rz->Amax();
  zMin = rz->Bmin();
  zMax = rz->Bmax();

  //
  // Is this the "starting" phi edge of the two?
  //
  G4bool start = (phiOther > phi);
  
  //
  // Build radial vector
  //
  radial = G4ThreeVector( std::cos(phi), std::sin(phi), 0.0 );
  
  //
  // Build normal
  //
  G4double zSign = start ? 1 : -1;
  normal = G4ThreeVector( zSign*radial.y(), -zSign*radial.x(), 0 );
  
  //
  // Is allBehind?
  //
  allBehind = (zSign*(std::cos(phiOther)*radial.y() - std::sin(phiOther)*radial.x()) < 0);
  
  //
  // Adjacent edges
  //
  G4double midPhi = phi + (start ? +0.5 : -0.5)*deltaPhi;
  G4double cosMid = std::cos(midPhi), 
                 sinMid = std::sin(midPhi);

  //
  // Allocate corners
  //
  corners = new G4PolyPhiFaceVertex[numEdges];
  //
  // Fill them
  //
  G4ReduciblePolygonIterator iterRZ(rz);
  
  G4PolyPhiFaceVertex *corn = corners;
  G4PolyPhiFaceVertex *helper=corners;

  iterRZ.Begin();
  do
  {
    corn->r = iterRZ.GetA();
    corn->z = iterRZ.GetB();
    corn->x = corn->r*radial.x();
    corn->y = corn->r*radial.y();

    // Add pointer on prev corner
    //
    if( corn == corners )
      { corn->prev = corners+numEdges-1;}
    else
      { corn->prev = helper; }

    // Add pointer on next corner
    //
    if( corn < corners+numEdges-1 )
      { corn->next = corn+1;}
    else
      { corn->next = corners; }

    helper = corn;
  } while( ++corn, iterRZ.Next() );

  //
  // Allocate edges
  //
  edges = new G4PolyPhiFaceEdge[numEdges];

  //
  // Fill them
  //
  G4double rFact = std::cos(0.5*deltaPhi);
  G4double rFactNormalize = 1.0/std::sqrt(1.0+rFact*rFact);

  G4PolyPhiFaceVertex *prev = corners+numEdges-1,
                      *here = corners;
  G4PolyPhiFaceEdge   *edge = edges;
  do
  {
    G4ThreeVector sideNorm;
    
    edge->v0 = prev;
    edge->v1 = here;

    G4double dr = here->r - prev->r,
             dz = here->z - prev->z;
                         
    edge->length = std::sqrt( dr*dr + dz*dz );

    edge->tr = dr/edge->length;
    edge->tz = dz/edge->length;
    
    if ((here->r < DBL_MIN) && (prev->r < DBL_MIN))
    {
      //
      // Sigh! Always exceptions!
      // This edge runs at r==0, so its adjoing surface is not a
      // PolyconeSide or PolyhedraSide, but the opposite PolyPhiFace.
      //
      G4double zSignOther = start ? -1 : 1;
      sideNorm = G4ThreeVector(  zSignOther*std::sin(phiOther), 
                                -zSignOther*std::cos(phiOther), 0 );
    }
    else
    {
      sideNorm = G4ThreeVector( edge->tz*cosMid,
                                edge->tz*sinMid,
                               -edge->tr*rFact );
      sideNorm *= rFactNormalize;
    }
    sideNorm += normal;
    
    edge->norm3D = sideNorm.unit();
  } while( edge++, prev=here, ++here < corners+numEdges );

  //
  // Go back and fill in corner "normals"
  //
  G4PolyPhiFaceEdge *prevEdge = edges+numEdges-1;
  edge = edges;
  do
  {
    //
    // Calculate vertex 2D normals (on the phi surface)
    //
    G4double rPart = prevEdge->tr + edge->tr;
    G4double zPart = prevEdge->tz + edge->tz;
    G4double norm = std::sqrt( rPart*rPart + zPart*zPart );
    G4double rNorm = +zPart/norm;
    G4double zNorm = -rPart/norm;
    
    edge->v0->rNorm = rNorm;
    edge->v0->zNorm = zNorm;
    
    //
    // Calculate the 3D normals.
    //
    // Find the vector perpendicular to the z axis
    // that defines the plane that contains the vertex normal
    //
    G4ThreeVector xyVector;
    
    if (edge->v0->r < DBL_MIN)
    {
      //
      // This is a vertex at r==0, which is a special
      // case. The normal we will construct lays in the
      // plane at the center of the phi opening.
      //
      // We also know that rNorm < 0
      //
      G4double zSignOther = start ? -1 : 1;
      G4ThreeVector normalOther(  zSignOther*std::sin(phiOther), 
                                 -zSignOther*std::cos(phiOther), 0 );
                
      xyVector = - normal - normalOther;
    }
    else
    {
      //
      // This is a vertex at r > 0. The plane
      // is the average of the normal and the
      // normal of the adjacent phi face
      //
      xyVector = G4ThreeVector( cosMid, sinMid, 0 );
      if (rNorm < 0)
        xyVector -= normal;
      else
        xyVector += normal;
    }
    
    //
    // Combine it with the r/z direction from the face
    //
    edge->v0->norm3D = rNorm*xyVector.unit() + G4ThreeVector( 0, 0, zNorm );
  } while(  prevEdge=edge, ++edge < edges+numEdges );
  
  //
  // Build point on surface
  //
  G4double rAve = 0.5*(rMax-rMin),
           zAve = 0.5*(zMax-zMin);
  surface = G4ThreeVector( rAve*radial.x(), rAve*radial.y(), zAve );
}


//
// Diagnose
//
// Throw an exception if something is found inconsistent with
// the solid.
//
// For debugging purposes only
//
void G4PolyPhiFace::Diagnose( G4VSolid *owner )
{
  G4PolyPhiFaceVertex   *corner = corners;
  do
  {
    G4ThreeVector test(corner->x, corner->y, corner->z);
    test -= 1E-6*corner->norm3D;
    
 /*   if (owner->Inside(test) != kInside) 
      G4Exception( "G4PolyPhiFace::Diagnose()", "InvalidSetup",
                   FatalException, "Bad vertex normal found." );*/
  } while( ++corner < corners+numEdges );
}


//
// Fake default constructor - sets only member data and allocates memory
//                            for usage restricted to object persistency.
//
G4PolyPhiFace::G4PolyPhiFace( __void__&)
  : edges(0), corners(0)
{
}


//
// Destructor
//
G4PolyPhiFace::~G4PolyPhiFace()
{
  delete [] edges;
  delete [] corners;
}


//
// Copy constructor
//
G4PolyPhiFace::G4PolyPhiFace( const G4PolyPhiFace &source )
  : G4VCSGface()
{
  CopyStuff( source );
}


//
// Assignment operator
//
G4PolyPhiFace& G4PolyPhiFace::operator=( const G4PolyPhiFace &source )
{
  if (this == &source) return *this;

  delete [] edges;
  delete [] corners;
  
  CopyStuff( source );
  
  return *this;
}


//
// CopyStuff (protected)
//
void G4PolyPhiFace::CopyStuff( const G4PolyPhiFace &source )
{
  //
  // The simple stuff
  //
  numEdges  = source.numEdges;
  normal    = source.normal;
  radial    = source.radial;
  surface   = source.surface;
  rMin    = source.rMin;
  rMax    = source.rMax;
  zMin    = source.zMin;
  zMax    = source.zMax;
  allBehind  = source.allBehind;

  kCarTolerance = source.kCarTolerance;
  fSurfaceArea = source.fSurfaceArea;

  //
  // Corner dynamic array
  //
  corners = new G4PolyPhiFaceVertex[numEdges];
  G4PolyPhiFaceVertex *corn = corners,
                      *sourceCorn = source.corners;
  do
  {
    *corn = *sourceCorn;
  } while( ++sourceCorn, ++corn < corners+numEdges );
  
  //
  // Edge dynamic array
  //
  edges = new G4PolyPhiFaceEdge[numEdges];

  G4PolyPhiFaceVertex *prev = corners+numEdges-1,
                      *here = corners;
  G4PolyPhiFaceEdge   *edge = edges,
                      *sourceEdge = source.edges;
  do
  {
    *edge = *sourceEdge;
    edge->v0 = prev;
    edge->v1 = here;
  } while( ++sourceEdge, ++edge, prev=here, ++here < corners+numEdges );
}


//
// Intersect
//
G4bool G4PolyPhiFace::Intersect( const G4ThreeVector &p,
                                 const G4ThreeVector &v,
                                       G4bool outgoing,
                                       G4double surfTolerance,
                                       G4double &distance,
                                       G4double &distFromSurface,
                                       G4ThreeVector &aNormal,
                                       G4bool &isAllBehind )
{
  G4double normSign = outgoing ? +1 : -1;
  
  //
  // These don't change
  //
  isAllBehind = allBehind;
  aNormal = normal;

  //
  // Correct normal? Here we have straight sides, and can safely ignore
  // intersections where the dot product with the normal is zero.
  //
  G4double dotProd = normSign*normal.dot(v);
  
  if (dotProd <= 0) return false;

  //
  // Calculate distance to surface. If the side is too far
  // behind the point, we must reject it.
  //
  G4ThreeVector ps = p - surface;
  distFromSurface = -normSign*ps.dot(normal);
    
  if (distFromSurface < -surfTolerance) return false;

  //
  // Calculate precise distance to intersection with the side
  // (along the trajectory, not normal to the surface)
  //
  distance = distFromSurface/dotProd;

  //
  // Calculate intersection point in r,z
  //
  G4ThreeVector ip = p + distance*v;
  
  G4double r = radial.dot(ip);
  
  //
  // And is it inside the r/z extent?
  //
  return InsideEdgesExact( r, ip.z(), normSign, p, v );
}


//
// Distance
//
G4double G4PolyPhiFace::Distance( const G4ThreeVector &p, G4bool outgoing )
{
  G4double normSign = outgoing ? +1 : -1;
  //
  // Correct normal? 
  //
  G4ThreeVector ps = p - surface;
  G4double distPhi = -normSign*normal.dot(ps);
  
  if (distPhi < -0.5*kCarTolerance) 
    return kInfinity;
  else if (distPhi < 0)
    distPhi = 0.0;
  
  //
  // Calculate projected point in r,z
  //
  G4double r = radial.dot(p);
  
  //
  // Are we inside the face?
  //
  G4double distRZ2;
  
  if (InsideEdges( r, p.z(), &distRZ2, 0 ))
  {
    //
    // Yup, answer is just distPhi
    //
    return distPhi;
  }
  else
  {
    //
    // Nope. Penalize by distance out
    //
    return std::sqrt( distPhi*distPhi + distRZ2 );
  }
}  
  

//
// Inside
//
EInside G4PolyPhiFace::Inside( const G4ThreeVector &p,
                                     G4double tolerance, 
                                     G4double *bestDistance )
{
  //
  // Get distance along phi, which if negative means the point
  // is nominally inside the shape.
  //
  G4ThreeVector ps = p - surface;
  G4double distPhi = normal.dot(ps);
  
  //
  // Calculate projected point in r,z
  //
  G4double r = radial.dot(p);
  
  //
  // Are we inside the face?
  //
  G4double distRZ2;
  G4PolyPhiFaceVertex *base3Dnorm;
  G4ThreeVector      *head3Dnorm;
  
  if (InsideEdges( r, p.z(), &distRZ2, &base3Dnorm, &head3Dnorm ))
  {
    //
    // Looks like we're inside. Distance is distance in phi.
    //
    *bestDistance = std::fabs(distPhi);
    
    //
    // Use distPhi to decide fate
    //
    if (distPhi < -tolerance) return kInside;
    if (distPhi <  tolerance) return kSurface;
    return kOutside;
  }
  else
  {
    //
    // We're outside the extent of the face,
    // so the distance is penalized by distance from edges in RZ
    //
    *bestDistance = std::sqrt( distPhi*distPhi + distRZ2 );
    
    //
    // Use edge normal to decide fate
    //
    G4ThreeVector cc( base3Dnorm->r*radial.x(),
          base3Dnorm->r*radial.y(),
          base3Dnorm->z );
    cc = p - cc;
    G4double normDist = head3Dnorm->dot(cc);
    if ( distRZ2 > tolerance*tolerance )
    {
      //
      // We're far enough away that kSurface is not possible
      //
      return normDist < 0 ? kInside : kOutside;
    }
    
    if (normDist < -tolerance) return kInside;
    if (normDist <  tolerance) return kSurface;
    return kOutside;
  }
}  


//
// Normal
//
// This virtual member is simple for our planer shape,
// which has only one normal
//
G4ThreeVector G4PolyPhiFace::Normal( const G4ThreeVector &p,
                                           G4double *bestDistance )
{
  //
  // Get distance along phi, which if negative means the point
  // is nominally inside the shape.
  //
  G4double distPhi = normal.dot(p);

  //
  // Calculate projected point in r,z
  //
  G4double r = radial.dot(p);
  
  //
  // Are we inside the face?
  //
  G4double distRZ2;
  
  if (InsideEdges( r, p.z(), &distRZ2, 0 ))
  {
    //
    // Yup, answer is just distPhi
    //
    *bestDistance = std::fabs(distPhi);
  }
  else
  {
    //
    // Nope. Penalize by distance out
    //
    *bestDistance = std::sqrt( distPhi*distPhi + distRZ2 );
  }
  
  return normal;
}


//
// Extent
//
// This actually isn't needed by polycone or polyhedra...
//
G4double G4PolyPhiFace::Extent( const G4ThreeVector axis )
{
  G4double max = -kInfinity;
  
  G4PolyPhiFaceVertex *corner = corners;
  do
  {
    G4double here = axis.x()*corner->r*radial.x()
            + axis.y()*corner->r*radial.y()
            + axis.z()*corner->z;
    if (here > max) max = here;
  } while( ++corner < corners + numEdges );
  
  return max;
}  


//
// CalculateExtent
//
// See notes in G4VCSGface
//
void G4PolyPhiFace::CalculateExtent( const EAxis axis, 
                                     const G4VoxelLimits &voxelLimit,
                                     const G4AffineTransform &transform,
                                           G4SolidExtentList &extentList )
{
	/*
  //
  // Construct a (sometimes big) clippable polygon, 
  //
  // Perform the necessary transformations while doing so
  //
  G4ClippablePolygon polygon;
  
  G4PolyPhiFaceVertex *corner = corners;
  do
  {
    G4ThreeVector point( 0, 0, corner->z );
    point += radial*corner->r;
    
    polygon.AddVertexInOrder( transform.TransformPoint( point ) );
  } while( ++corner < corners + numEdges );
  
  //
  // Clip away
  //
  if (polygon.PartialClip( voxelLimit, axis ))
  {
    //
    // Add it to the list
    //
    polygon.SetNormal( transform.TransformAxis(normal) );
    extentList.AddSurface( polygon );
  }*/
}


//
//-------------------------------------------------------
  
  
//
// InsideEdgesExact
//
// Decide if the point in r,z is inside the edges of our face,
// **but** do so consistently with other faces.
//
// This routine has functionality similar to InsideEdges, but uses
// an algorithm to decide if a trajectory falls inside or outside the
// face that uses only the trajectory p,v values and the three dimensional
// points representing the edges of the polygon. The objective is to plug up
// any leaks between touching G4PolyPhiFaces (at r==0) and any other face
// that uses the same convention.
//
// See: "Computational Geometry in C (Second Edition)"
// http://cs.smith.edu/~orourke/
//
G4bool G4PolyPhiFace::InsideEdgesExact( G4double r, G4double z,
                                        G4double normSign,
                                  const G4ThreeVector &p,
                                  const G4ThreeVector &v )
{
  //
  // Quick check of extent
  //
  if ( (r < rMin-kCarTolerance)
    || (r > rMax+kCarTolerance) ) return false;
       
  if ( (z < zMin-kCarTolerance)
    || (z > zMax+kCarTolerance) ) return false;
  
  //
  // Exact check: loop over all vertices
  //
  G4double qx = p.x() + v.x(),
           qy = p.y() + v.y(),
           qz = p.z() + v.z();

  G4int answer = 0;
  G4PolyPhiFaceVertex *corn = corners, 
                      *prev = corners+numEdges-1;

  G4double cornZ, prevZ;
  
  prevZ = ExactZOrder( z, qx, qy, qz, v, normSign, prev );
  do
  {
    //
    // Get z order of this vertex, and compare to previous vertex
    //
    cornZ = ExactZOrder( z, qx, qy, qz, v, normSign, corn );
    
    if (cornZ < 0)
    {
      if (prevZ < 0) continue;
    }
    else if (cornZ > 0)
    {
      if (prevZ > 0) continue;
    }
    else
    {
      //
      // By chance, we overlap exactly (within precision) with 
      // the current vertex. Continue if the same happened previously
      // (e.g. the previous vertex had the same z value)
      //
      if (prevZ == 0) continue;
      
      //
      // Otherwise, to decide what to do, we need to know what is
      // coming up next. Specifically, we need to find the next vertex
      // with a non-zero z order.
      //
      // One might worry about infinite loops, but the above conditional
      // should prevent it
      //
      G4PolyPhiFaceVertex *next = corn;
      G4double nextZ;
      do
      {
        next++;
        if (next == corners+numEdges) next = corners;

        nextZ = ExactZOrder( z, qx, qy, qz, v, normSign, next );
      } while( nextZ == 0 );
      
      //
      // If we won't be changing direction, go to the next vertex
      //
      if (nextZ*prevZ < 0) continue;
    }
  
      
    //
    // We overlap in z with the side of the face that stretches from
    // vertex "prev" to "corn". On which side (left or right) do
    // we lay with respect to this segment?
    //  
    G4ThreeVector qa( qx - prev->x, qy - prev->y, qz - prev->z ),
                  qb( qx - corn->x, qy - corn->y, qz - corn->z );

    G4double aboveOrBelow = normSign*qa.cross(qb).dot(v);
    
    if (aboveOrBelow > 0) 
      answer++;
    else if (aboveOrBelow < 0)
      answer--;
    else
    {
      //
      // A precisely zero answer here means we exactly
      // intersect (within roundoff) the edge of the face.
      // Return true in this case.
      //
      return true;
    }
  } while( prevZ = cornZ, prev=corn, ++corn < corners+numEdges );
  
//  G4int fanswer = std::abs(answer);
//  if (fanswer==1 || fanswer>2) {
//    G4cerr << "G4PolyPhiFace::InsideEdgesExact: answer is "
//           << answer << G4endl;
//  }

  return answer!=0;
}
  
  
//
// InsideEdges (don't care aboud distance)
//
// Decide if the point in r,z is inside the edges of our face
//
// This routine can be made a zillion times quicker by implementing
// better code, for example:
//
//    int pnpoly(int npol, float *xp, float *yp, float x, float y)
//    {
//      int i, j, c = 0;
//      for (i = 0, j = npol-1; i < npol; j = i++) {
//        if ((((yp[i]<=y) && (y<yp[j])) ||
//             ((yp[j]<=y) && (y<yp[i]))) &&
//            (x < (xp[j] - xp[i]) * (y - yp[i]) / (yp[j] - yp[i]) + xp[i]))
//
//          c = !c;
//      }
//      return c;
//    }
//
// See "Point in Polyon Strategies", Eric Haines [Graphic Gems IV]  pp. 24-46
//
// My algorithm below is rather unique, but is based on code needed to
// calculate the distance to the shape. I left it in here because ...
// well ... to test it better.
//
G4bool G4PolyPhiFace::InsideEdges( G4double r, G4double z )
{
  //
  // Quick check of extent
  //
  if ( r < rMin || r > rMax ) return false;
  if ( z < zMin || z > zMax ) return false;
  
  //
  // More thorough check
  //
  G4double notUsed;
  
  return InsideEdges( r, z, &notUsed, 0 );
}


//
// InsideEdges (care about distance)
//
// Decide if the point in r,z is inside the edges of our face
//
G4bool G4PolyPhiFace::InsideEdges( G4double r, G4double z,
                                   G4double *bestDist2, 
                                   G4PolyPhiFaceVertex **base3Dnorm, 
                                   G4ThreeVector **head3Dnorm )
{
  G4double bestDistance2 = kInfinity;
  G4bool   answer = 0;
  
  G4PolyPhiFaceEdge *edge = edges;
  do
  {
    G4PolyPhiFaceVertex *testMe;
    //
    // Get distance perpendicular to the edge
    //
    G4double dr = (r-edge->v0->r), dz = (z-edge->v0->z);

    G4double distOut = dr*edge->tz - dz*edge->tr;
    G4double distance2 = distOut*distOut;
    if (distance2 > bestDistance2) continue;        // No hope!

    //
    // Check to see if normal intersects edge within the edge's boundary
    //
    G4double s = dr*edge->tr + dz*edge->tz;

    //
    // If it doesn't, penalize distance2 appropriately
    //
    if (s < 0)
    {
      distance2 += s*s;
      testMe = edge->v0;
    }
    else if (s > edge->length)
    {
      G4double s2 = s-edge->length;
      distance2 += s2*s2;
      testMe = edge->v1;
    }
    else
    {
      testMe = 0;
    }
    
    //
    // Closest edge so far?
    //
    if (distance2 < bestDistance2)
    {
      bestDistance2 = distance2;
      if (testMe)
      {
        G4double distNorm = dr*testMe->rNorm + dz*testMe->zNorm;
        answer = (distNorm <= 0);
        if (base3Dnorm)
        {
          *base3Dnorm = testMe;
          *head3Dnorm = &testMe->norm3D;
        }
      }
      else
      {
        answer = (distOut <= 0);                        
        if (base3Dnorm)
        {
          *base3Dnorm = edge->v0;
          *head3Dnorm = &edge->norm3D;
        }
      }
    }
  } while( ++edge < edges + numEdges );
  
  *bestDist2 = bestDistance2;
  return answer;
}

//
// Calculation of Surface Area of a Triangle 
// In the same time Random Point in Triangle is given
//
G4double G4PolyPhiFace::SurfaceTriangle( G4ThreeVector p1,
                                         G4ThreeVector p2,
                                         G4ThreeVector p3,
                                         G4ThreeVector *p4 )
{
  G4ThreeVector v, w;
  
  v = p3 - p1;
  w = p1 - p2;
///  G4double lambda1 = G4UniformRand();
  G4double lambda1 = rand()/(double)RAND_MAX;// rand()/(double)RAND_MAX;
///  G4double lambda2 = lambda1*G4UniformRand();
  G4double lambda2 = lambda1*rand()/(double)RAND_MAX;
 
  *p4=p2 + lambda1*w + lambda2*v;
  return 0.5*(v.cross(w)).mag();
}

//
// Compute surface area
//
G4double G4PolyPhiFace::SurfaceArea()
{
  if ( fSurfaceArea==0. ) { Triangulate(); }
  return fSurfaceArea;
}

//
// Return random point on face
//
G4ThreeVector G4PolyPhiFace::GetPointOnFace()
{
  Triangulate();
  return surface_point;
}

//
// Auxiliary Functions used for Finding the PointOnFace using Triangulation
//

//
// Calculation of 2*Area of Triangle with Sign
//
G4double G4PolyPhiFace::Area2( G4TwoVector a,
                               G4TwoVector b,
                               G4TwoVector c )
{
  return ((b.x()-a.x())*(c.y()-a.y())-
          (c.x()-a.x())*(b.y()-a.y()));
}

//
// Boolean function for sign of Surface
//
G4bool G4PolyPhiFace::Left( G4TwoVector a,
                            G4TwoVector b,
                            G4TwoVector c )
{
  return Area2(a,b,c)>0;
}

//
// Boolean function for sign of Surface
//
G4bool G4PolyPhiFace::LeftOn( G4TwoVector a,
                              G4TwoVector b,
                              G4TwoVector c )
{
  return Area2(a,b,c)>=0;
}

//
// Boolean function for sign of Surface
//
G4bool G4PolyPhiFace::Collinear( G4TwoVector a,
                                 G4TwoVector b,
                                 G4TwoVector c )
{
  return Area2(a,b,c)==0;
}

//
// Boolean function for finding "Proper" Intersection
// That means Intersection of two lines segments (a,b) and (c,d)
// 
G4bool G4PolyPhiFace::IntersectProp( G4TwoVector a,
                                     G4TwoVector b,
                                     G4TwoVector c, G4TwoVector d )
{
  if( Collinear(a,b,c) || Collinear(a,b,d)||
      Collinear(c,d,a) || Collinear(c,d,b) )  { return false; }

  G4bool Positive;
  Positive = !(Left(a,b,c))^!(Left(a,b,d));
  return Positive && (!Left(c,d,a)^!Left(c,d,b));
}

//
// Boolean function for determining if Point c is between a and b
// For the tree points(a,b,c) on the same line
//
G4bool G4PolyPhiFace::Between( G4TwoVector a, G4TwoVector b, G4TwoVector c )
{
  if( !Collinear(a,b,c) ) { return false; }

  if(a.x()!=b.x())
  {
    return ((a.x()<=c.x())&&(c.x()<=b.x()))||
           ((a.x()>=c.x())&&(c.x()>=b.x()));
  }
  else
  {
    return ((a.y()<=c.y())&&(c.y()<=b.y()))||
           ((a.y()>=c.y())&&(c.y()>=b.y()));
  }
}

//
// Boolean function for finding Intersection "Proper" or not
// Between two line segments (a,b) and (c,d)
//
G4bool G4PolyPhiFace::Intersect( G4TwoVector a,
                                 G4TwoVector b,
                                 G4TwoVector c, G4TwoVector d )
{
 if( IntersectProp(a,b,c,d) )
   { return true; }
 else if( Between(a,b,c)||
          Between(a,b,d)||
          Between(c,d,a)||
          Between(c,d,b) )
   { return true; }
 else
   { return false; }
}

//
// Boolean Diagonalie help to determine 
// if diagonal s of segment (a,b) is convex or reflex
//
G4bool G4PolyPhiFace::Diagonalie( G4PolyPhiFaceVertex *a,
                                  G4PolyPhiFaceVertex *b )
{
  G4PolyPhiFaceVertex   *corner = triangles;
  G4PolyPhiFaceVertex   *corner_next=triangles;

  // For each Edge (corner,corner_next) 
  do
  {
    corner_next=corner->next;

    // Skip edges incident to a of b
    //
    if( (corner!=a)&&(corner_next!=a)
      &&(corner!=b)&&(corner_next!=b) )
    {
       G4TwoVector rz1,rz2,rz3,rz4;
       rz1 = G4TwoVector(a->r,a->z);
       rz2 = G4TwoVector(b->r,b->z);
       rz3 = G4TwoVector(corner->r,corner->z);
       rz4 = G4TwoVector(corner_next->r,corner_next->z);
       if( Intersect(rz1,rz2,rz3,rz4) ) { return false; }
    }
    corner=corner->next;   
   
  } while( corner != triangles );

  return true;
}

//
// Boolean function that determine if b is Inside Cone (a0,a,a1)
// being a the center of the Cone
//
G4bool G4PolyPhiFace::InCone( G4PolyPhiFaceVertex *a, G4PolyPhiFaceVertex *b )
{
  // a0,a and a1 are consecutive vertices
  //
  G4PolyPhiFaceVertex *a0,*a1;
  a1=a->next;
  a0=a->prev;

  G4TwoVector arz,arz0,arz1,brz;
  arz=G4TwoVector(a->r,a->z);arz0=G4TwoVector(a0->r,a0->z);
  arz1=G4TwoVector(a1->r,a1->z);brz=G4TwoVector(b->r,b->z);
    
  
  if(LeftOn(arz,arz1,arz0))  // If a is convex vertex
  {
    return Left(arz,brz,arz0)&&Left(brz,arz,arz1);
  }
  else                       // Else a is reflex
  {
    return !( LeftOn(arz,brz,arz1)&&LeftOn(brz,arz,arz0));
  }
}

//
// Boolean function finding if Diagonal is possible
// inside Polycone or PolyHedra
//
G4bool G4PolyPhiFace::Diagonal( G4PolyPhiFaceVertex *a, G4PolyPhiFaceVertex *b )
{ 
  return InCone(a,b) && InCone(b,a) && Diagonalie(a,b);
}

//
// Initialisation for Triangulisation by ear tips
// For details see "Computational Geometry in C" by Joseph O'Rourke
//
void G4PolyPhiFace::EarInit()
{
  G4PolyPhiFaceVertex   *corner = triangles;
  G4PolyPhiFaceVertex *c_prev,*c_next;
  
  do
  {
     // We need to determine three consecutive vertices
     //
     c_next=corner->next;
     c_prev=corner->prev; 

     // Calculation of ears
     //
     corner->ear=Diagonal(c_prev,c_next);   
     corner=corner->next;

  } while( corner!=triangles );
}

//
// Triangulisation by ear tips for Polycone or Polyhedra
// For details see "Computational Geometry in C" by Joseph O'Rourke
//
void G4PolyPhiFace::Triangulate()
{ 
  // The copy of Polycone is made and this copy is reordered in order to 
  // have a list of triangles. This list is used for GetPointOnFace().

  G4PolyPhiFaceVertex *tri_help = new G4PolyPhiFaceVertex[numEdges];
  triangles = tri_help;
  G4PolyPhiFaceVertex *triang = triangles;

  std::vector<G4double> areas;
  std::vector<G4ThreeVector> points;
  G4double area=0.;
  G4PolyPhiFaceVertex *v0,*v1,*v2,*v3,*v4;
  v2=triangles;

  // Make copy for prev/next for triang=corners
  //
  G4PolyPhiFaceVertex *helper = corners;
  G4PolyPhiFaceVertex *helper2 = corners;
  do
  {
    triang->r = helper->r;
    triang->z = helper->z;
    triang->x = helper->x;
    triang->y= helper->y;

    // add pointer on prev corner
    //
    if( helper==corners )
      { triang->prev=triangles+numEdges-1; }
    else
      { triang->prev=helper2; }

    // add pointer on next corner
    //
    if( helper<corners+numEdges-1 )
      { triang->next=triang+1; }
    else
      { triang->next=triangles; }
    helper2=triang;
    helper=helper->next;
    triang=triang->next;

  } while( helper!=corners );

  EarInit();
 
  G4int n=numEdges;
  G4int i=0;
  G4ThreeVector p1,p2,p3,p4;
  const G4int max_n_loops=numEdges*10000; // protection against infinite loop

  // Each step of outer loop removes one ear
  //
  while(n>3)  // Inner loop searches for one ear
  {
    v2=triangles; 
    do
    {
      if(v2->ear)  // Ear found. Fill variables
      {
        // (v1,v3) is diagonal
        //
        v3=v2->next; v4=v3->next;
        v1=v2->prev; v0=v1->prev;
        
        // Calculate areas and points

        p1=G4ThreeVector((v2)->x,(v2)->y,(v2)->z);
        p2=G4ThreeVector((v1)->x,(v1)->y,(v1)->z);
        p3=G4ThreeVector((v3)->x,(v3)->y,(v3)->z);

        G4double result1 = SurfaceTriangle(p1,p2,p3,&p4 );
        points.push_back(p4);
        areas.push_back(result1);
        area=area+result1;

        // Update earity of diagonal endpoints
        //
        v1->ear=Diagonal(v0,v3);
        v3->ear=Diagonal(v1,v4);

        // Cut off the ear v2 
        // Has to be done for a copy and not for real PolyPhiFace
        //
        v1->next=v3;
        v3->prev=v1;
        triangles=v3; // In case the head was v2
        n--;
 
        break; // out of inner loop
      }        // end if ear found

      v2=v2->next;
    
    } while( v2!=triangles );

    i++;
    if(i>=max_n_loops)
    {
 /*     G4Exception( "G4PolyPhiFace::Triangulation()",
                   "Bad_Definition_of_Solid", FatalException,
                   "Maximum number of steps is reached for triangulation!" );*/
    }
  }   // end outer while loop

  if(v2->next)
  {
     // add last triangle
     //
     v2=v2->next;
     p1=G4ThreeVector((v2)->x,(v2)->y,(v2)->z);
     p2=G4ThreeVector((v2->next)->x,(v2->next)->y,(v2->next)->z);
     p3=G4ThreeVector((v2->prev)->x,(v2->prev)->y,(v2->prev)->z);
     G4double result1 = SurfaceTriangle(p1,p2,p3,&p4 );
     points.push_back(p4);
     areas.push_back(result1);
     area=area+result1;
  }
 
  // Surface Area is stored
  //
  fSurfaceArea = area;
  
  // Second Step: choose randomly one surface
  //
  ///G4double chose = area*G4UniformRand();// rand()/(double)RAND_MAX;
  G4double chose = area*rand()/(double)RAND_MAX;
   
  // Third Step: Get a point on choosen surface
  //
  G4double Achose1, Achose2;
  Achose1=0; Achose2=0.; 
  i=0;
  do 
  {
    Achose2+=areas[i];
    if(chose>=Achose1 && chose<Achose2)
    {
      G4ThreeVector point;
       point=points[i] ;
       surface_point=point;
       break;     
    }
    i++; Achose1=Achose2;
  } while( i<numEdges-2 );
 
  delete [] tri_help;
}
