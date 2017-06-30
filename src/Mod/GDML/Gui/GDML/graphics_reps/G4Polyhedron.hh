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
// $Id: G4Polyhedron.hh,v 1.21 2008/04/14 08:50:23 gcosmo Exp $
// GEANT4 tag $Name: geant4-09-03-patch-01 $

#ifndef G4POLYHEDRON_HH
#define G4POLYHEDRON_HH

// Class Description:
// G4Polyhedron is an intermediate class between G4 and visualization
// systems. It is intended to provide some service like:
//   - polygonization of the G4 shapes with triangulization
//     (quadrilaterization) of complex polygons;
//   - calculation of normals for faces and vertices.
//
// Inherits from HepPolyhedron, to which reference should be made for
// functionality.
//
// Public constructors:
//   G4PolyhedronBox(dx,dy,dz)            - create G4Polyhedron for G4 Box;
//   G4PolyhedronTrd1(dx1,dx2,dy,dz)      - create G4Polyhedron for G4 Trd1;
//   G4PolyhedronTrd2(dx1,dx2,dy1,dy2,dz) - create G4Polyhedron for G4 Trd2;
//   G4PolyhedronTrap(dz,theta,phi,
//                    h1,bl1,tl1,alp1,
//                    h2,bl2,tl2,alp2)    - create G4Polyhedron for G4 Trap;
//   G4PolyhedronPara(dx,dy,dz,
//                    alpha,theta,phi)    - create G4Polyhedron for G4 Para;
//
//   G4PolyhedronTube(rmin,rmax,dz)       - create G4Polyhedron for G4 Tube;
//   G4PolyhedronTubs(rmin,rmax,dz,
//                    phi1,dphi)          - create G4Polyhedron for G4 Tubs;
//   G4PolyhedronCone(rmin1,rmax1,
//                    rmin2,rmax2,dz)     - create G4Polyhedron for G4 Cone;
//   G4PolyhedronCons(rmin1,rmax1,
//                    rmin2,rmax2,dz,
//                    phi1,dphi)          - create G4Polyhedron for G4 Cons;
//
//   G4PolyhedronPgon(phi,dphi,npdv,nz,
//                    z(*),rmin(*),rmax(*)) - create G4Polyhedron for G4 Pgon;
//   G4PolyhedronPcon(phi,dphi,nz,
//                    z(*),rmin(*),rmax(*)) - create G4Polyhedron for G4 Pcon;
//
//   G4PolyhedronSphere(rmin,rmax,
//                      phi,dphi,the,dthe)  - create G4Polyhedron for Sphere;
//   G4PolyhedronTorus(rmin,rmax,rtor,
//                     phi,dphi)            - create G4Polyhedron for Torus;
//   G4PolyhedronEllipsoid(dx,dy,dz,
//                     zcut1,zcut2)         - create G4Polyhedron for Ellipsoid;
//
// Public functions inherited from HepPolyhedron (this list might be
// incomplete):
//   GetNoVertices()  - returns number of vertices
//   GetNoFacets()    - returns number of faces
//   GetNextVertexIndex(index, edgeFlag) - get vertex indeces of the
//                      quadrilaterals in order; returns false when
//                      finished each face;
//   GetVertex(index) - returns vertex by index;
//   GetNextVertex(vertex, edgeFlag) - get vertices with edge visibility
//                      of the quadrilaterals in order;
//                      returns false when finished each face;
//   GetNextVertex(vertex, edgeFlag, normal) - get vertices with edge
//                      visibility and normal of the quadrilaterals
//                      in order; returns false when finished each face;
//   GetNextNormal(normal) - get normals of each face in order;
//                      returns false when finished all faces;
//   GetNextUnitNormal(normal) - get normals of unit length of each face
//                      in order; returns false when finished all faces;
//   GetNextEdgeIndeces(i1, i2, edgeFlag) - get indeces of the next edge;
//                      returns false for the last edge;
//   GetNextEdge(p1, p2, edgeFlag) - get next edge;
//                      returns false for the last edge;
//   SetNumberOfRotationSteps(G4int n) - Set number of steps for whole circle;

// History:
// 21st February 2000  Evgeni Chernaev, John Allison
// - Re-written to inherit HepPolyhedron.
//
// 11.03.05 J.Allison
// - Added fNumberOfRotationStepsAtTimeOfCreation and access method.
//   (NumberOfRotationSteps is also called number of sides per circle or
//   line segments per circle - see
//   /vis/viewer/set/lineSegmentsPerCircle.)
// 20.06.05 G.Cosmo
// - Added G4PolyhedronEllipsoid.
// 09.03.06 J.Allison
// - Added operator<<.

///#include "globals.hh"
#include "../global/globals.hh"
///#include "HepPolyhedron.h"
#include "HepPolyhedron.h"
#include "G4Visible.hh"

///
class G4Polyhedron : public HepPolyhedron, public G4Visible {
//class G4Polyhedron : public G4Visible {
public:
  G4Polyhedron ();
  ///
  G4Polyhedron (const HepPolyhedron& from);
  // Use compiler defaults for copy contructor and assignment.  (They
  // invoke their counterparts in HepPolyhedron and G4Visible.)
  virtual ~G4Polyhedron ();

  G4int GetNumberOfRotationStepsAtTimeOfCreation() const {
    return fNumberOfRotationStepsAtTimeOfCreation;
  }
private:
  G4int fNumberOfRotationStepsAtTimeOfCreation;
};

class G4PolyhedronBox: public G4Polyhedron {
public:
  G4PolyhedronBox (G4double dx, G4double dy, G4double dz);
  virtual ~G4PolyhedronBox ();
};
/*
class G4PolyhedronCone: public G4Polyhedron {
public:
  G4PolyhedronCone (G4double Rmn1, G4double Rmx1, 
                    G4double Rmn2, G4double Rmx2, G4double Dz);
  virtual ~G4PolyhedronCone (); 
};
*/
class G4PolyhedronCons: public G4Polyhedron {
public:
  G4PolyhedronCons (G4double Rmn1, G4double Rmx1, 
                    G4double Rmn2, G4double Rmx2, G4double Dz,
                    G4double Phi1, G4double Dphi);
  virtual ~G4PolyhedronCons ();
};
/*
class G4PolyhedronPara: public G4Polyhedron {
public:
  G4PolyhedronPara (G4double Dx, G4double Dy, G4double Dz,
                    G4double Alpha, G4double Theta, G4double Phi);
  virtual ~G4PolyhedronPara ();
};
*/
class G4PolyhedronPcon: public G4Polyhedron {
public:
  G4PolyhedronPcon (G4double phi, G4double dphi, G4int nz,
                    const G4double *z,
                    const G4double *rmin,
                    const G4double *rmax);
  virtual ~G4PolyhedronPcon ();
};
/*
class G4PolyhedronPgon: public G4Polyhedron {
public:
  G4PolyhedronPgon (G4double phi, G4double dphi, G4int npdv, G4int nz,
                    const G4double *z,
                    const G4double *rmin,
                    const G4double *rmax);
  virtual ~G4PolyhedronPgon ();
};

class G4PolyhedronSphere: public G4Polyhedron {
public:
  G4PolyhedronSphere (G4double rmin, G4double rmax,
                      G4double phi, G4double dphi,
                      G4double the, G4double dthe);
  virtual ~G4PolyhedronSphere ();
};

class G4PolyhedronTorus: public G4Polyhedron {
public:
  G4PolyhedronTorus (G4double rmin, G4double rmax, G4double rtor,
                    G4double phi, G4double dphi);
  virtual ~G4PolyhedronTorus ();
};
*/
class G4PolyhedronTrap: public G4Polyhedron {
public:
  G4PolyhedronTrap (G4double Dz, G4double Theta, G4double Phi,
                    G4double Dy1,
                    G4double Dx1, G4double Dx2, G4double Alp1,
                    G4double Dy2,
                    G4double Dx3, G4double Dx4, G4double Alp2);
  virtual ~G4PolyhedronTrap ();
};
/*
class G4PolyhedronTrd1: public G4Polyhedron {
public:
  G4PolyhedronTrd1 (G4double Dx1, G4double Dx2,
                    G4double Dy, G4double Dz);
  virtual ~G4PolyhedronTrd1 ();
};
*/
class G4PolyhedronTrd2: public G4Polyhedron {
public:
  G4PolyhedronTrd2 (G4double Dx1, G4double Dx2,
                    G4double Dy1, G4double Dy2, G4double Dz);
  virtual ~G4PolyhedronTrd2 ();
};
/*
class G4PolyhedronTube: public G4Polyhedron {
public:
  G4PolyhedronTube (G4double Rmin, G4double Rmax, G4double Dz);
  virtual ~G4PolyhedronTube ();
};
*/
class G4PolyhedronTubs: public G4Polyhedron {
public:
  G4PolyhedronTubs (G4double Rmin, G4double Rmax, G4double Dz, 
                    G4double Phi1, G4double Dphi);
  virtual ~G4PolyhedronTubs ();
};
/*
class G4PolyhedronParaboloid: public G4Polyhedron {
 public:
  G4PolyhedronParaboloid(G4double r1, G4double r2, G4double dz,
                         G4double sPhi, G4double dPhi);
  virtual ~G4PolyhedronParaboloid ();
};

class G4PolyhedronHype: public G4Polyhedron {
 public:
  G4PolyhedronHype(G4double r1, G4double r2, G4double tan1,
                   G4double tan2, G4double halfZ);
  virtual ~G4PolyhedronHype ();
};

class G4PolyhedronEllipsoid : public G4Polyhedron {
 public:
  G4PolyhedronEllipsoid(G4double dx, G4double dy, G4double dz, 
                        G4double zcut1, G4double zcut2);
  virtual ~G4PolyhedronEllipsoid ();
};

class G4PolyhedronEllipticalCone : public G4Polyhedron {
 public:
  G4PolyhedronEllipticalCone(G4double dx, G4double dy, G4double z, 
                             G4double zcut1);
  virtual ~G4PolyhedronEllipticalCone ();
};

std::ostream& operator<<(std::ostream& os, const G4Polyhedron&);
*/
#endif /* G4POLYHEDRON_HH */
