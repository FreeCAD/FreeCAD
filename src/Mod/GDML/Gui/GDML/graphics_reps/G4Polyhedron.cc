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
// $Id: G4Polyhedron.cc,v 1.23 2008/04/14 08:50:23 gcosmo Exp $
// GEANT4 tag $Name: geant4-09-03-patch-01 $

#include "G4Polyhedron.hh"

G4Polyhedron::G4Polyhedron ()///:
///  fNumberOfRotationStepsAtTimeOfCreation (fNumberOfRotationSteps)
{}

G4Polyhedron::~G4Polyhedron () {}

G4Polyhedron::G4Polyhedron (const HepPolyhedron& from)
  : HepPolyhedron(from)
{
  fNumberOfRotationStepsAtTimeOfCreation =
    from.fNumberOfRotationSteps;
}

G4PolyhedronBox::G4PolyhedronBox (G4double dx, G4double dy, G4double dz):
  G4Polyhedron (HepPolyhedronBox (dx, dy, dz)) {}

G4PolyhedronBox::~G4PolyhedronBox () {}
/*
G4PolyhedronCone::G4PolyhedronCone (G4double Rmn1, G4double Rmx1, 
				    G4double Rmn2, G4double Rmx2, G4double Dz):
  G4Polyhedron (HepPolyhedronCone (Rmn1, Rmx1, Rmn2, Rmx2, Dz)) {}

G4PolyhedronCone::~G4PolyhedronCone () {}
*/
G4PolyhedronCons::G4PolyhedronCons (G4double Rmn1, G4double Rmx1, 
				    G4double Rmn2, G4double Rmx2, G4double Dz,
				    G4double Phi1, G4double Dphi):
  G4Polyhedron (HepPolyhedronCons (Rmn1, Rmx1, Rmn2, Rmx2, Dz, Phi1, Dphi)) {}

G4PolyhedronCons::~G4PolyhedronCons () {}
/*
G4PolyhedronPara::G4PolyhedronPara (G4double Dx, G4double Dy, G4double Dz,
				    G4double Alpha, G4double Theta,
				    G4double Phi):
  G4Polyhedron (HepPolyhedronPara (Dx, Dy, Dz, Alpha, Theta, Phi)) {}

G4PolyhedronPara::~G4PolyhedronPara () {}
*/
G4PolyhedronPcon::G4PolyhedronPcon (G4double phi, G4double dphi, G4int nz,
				    const G4double *z,
				    const G4double *rmin,
				    const G4double *rmax):
  G4Polyhedron (HepPolyhedronPcon (phi, dphi, nz, z, rmin, rmax)) {}

G4PolyhedronPcon::~G4PolyhedronPcon () {}
/*
G4PolyhedronPgon::G4PolyhedronPgon (G4double phi, G4double dphi, G4int npdv,
				    G4int nz,
				    const G4double *z,
				    const G4double *rmin,
				    const G4double *rmax):
  G4Polyhedron (HepPolyhedronPgon (phi, dphi, npdv, nz, z, rmin, rmax)) {}

G4PolyhedronPgon::~G4PolyhedronPgon () {}

G4PolyhedronSphere::G4PolyhedronSphere (G4double rmin, G4double rmax,
					G4double phi, G4double dphi,
					G4double the, G4double dthe):
  G4Polyhedron (HepPolyhedronSphere (rmin, rmax, phi, dphi, the, dthe)) {}

G4PolyhedronSphere::~G4PolyhedronSphere () {}

G4PolyhedronTorus::G4PolyhedronTorus (G4double rmin, G4double rmax,
				      G4double rtor,
				      G4double phi, G4double dphi):
  G4Polyhedron (HepPolyhedronTorus (rmin, rmax, rtor, phi, dphi)) {}

G4PolyhedronTorus::~G4PolyhedronTorus () {}
*/
G4PolyhedronTrap::G4PolyhedronTrap (G4double Dz, G4double Theta, G4double Phi,
				    G4double Dy1,
				    G4double Dx1, G4double Dx2, G4double Alp1,
				    G4double Dy2,
				    G4double Dx3, G4double Dx4, G4double Alp2):
  G4Polyhedron (HepPolyhedronTrap (Dz, Theta, Phi, Dy1, Dx1, Dx2, Alp1,
				   Dy2, Dx3, Dx4, Alp2)) {}

G4PolyhedronTrap::~G4PolyhedronTrap () {}
/*
G4PolyhedronTrd1::G4PolyhedronTrd1 (G4double Dx1, G4double Dx2,
				    G4double Dy, G4double Dz):
  G4Polyhedron (HepPolyhedronTrd1 (Dx1, Dx2, Dy, Dz)) {}

G4PolyhedronTrd1::~G4PolyhedronTrd1 () {}
*/
G4PolyhedronTrd2::G4PolyhedronTrd2 (G4double Dx1, G4double Dx2,
				    G4double Dy1, G4double Dy2, G4double Dz):
  G4Polyhedron (HepPolyhedronTrd2 (Dx1, Dx2, Dy1, Dy2, Dz)) {}

G4PolyhedronTrd2::~G4PolyhedronTrd2 () {}
/*
G4PolyhedronTube::G4PolyhedronTube (G4double Rmin, G4double Rmax, G4double Dz):
  G4Polyhedron (HepPolyhedronTube (Rmin, Rmax, Dz)) {}

G4PolyhedronTube::~G4PolyhedronTube () {}
*/
G4PolyhedronTubs::G4PolyhedronTubs (G4double Rmin, G4double Rmax, G4double Dz, 
				    G4double Phi1, G4double Dphi):
  G4Polyhedron (HepPolyhedronTubs (Rmin, Rmax, Dz, Phi1, Dphi)) {}

G4PolyhedronTubs::~G4PolyhedronTubs () {}
/*
G4PolyhedronParaboloid::G4PolyhedronParaboloid (G4double r1, G4double r2,
                                                G4double dz, G4double sPhi,
                                                G4double dPhi):
  G4Polyhedron (HepPolyhedronParaboloid(r1, r2, dz, sPhi, dPhi)) {}

G4PolyhedronParaboloid::~G4PolyhedronParaboloid () {}

G4PolyhedronHype::G4PolyhedronHype (G4double r1, G4double r2, G4double tan1,
                                    G4double tan2, G4double halfZ):
  G4Polyhedron (HepPolyhedronHype(r1, r2, tan1, tan2, halfZ)) {}

G4PolyhedronHype::~G4PolyhedronHype () {}

G4PolyhedronEllipsoid::G4PolyhedronEllipsoid (G4double ax, G4double by,
                                              G4double cz, 
					      G4double zCut1, G4double zCut2):
  G4Polyhedron (HepPolyhedronEllipsoid (ax, by, cz, zCut1, zCut2)) {}

G4PolyhedronEllipsoid::~G4PolyhedronEllipsoid () {}

G4PolyhedronEllipticalCone::G4PolyhedronEllipticalCone (G4double ax,
                                                        G4double ay,
                                                        G4double h, 
					                G4double zCut1):
  G4Polyhedron (HepPolyhedronEllipticalCone (ax, ay, h, zCut1)) {}

G4PolyhedronEllipticalCone::~G4PolyhedronEllipticalCone () {}

std::ostream& operator<<(std::ostream& os, const G4Polyhedron& polyhedron)
{
  os << "G4Polyhedron: "
     << (const G4Visible&)polyhedron << '\n'
     << (const HepPolyhedron&)polyhedron;
  return os;
}
*/