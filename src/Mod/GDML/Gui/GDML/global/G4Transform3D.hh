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
// $Id: G4Transform3D.hh,v 1.5 2006/06/29 18:59:16 gunter Exp $
// GEANT4 tag $Name: geant4-09-03-patch-01 $
//
#ifndef G4TRANSFORM3D_HH
#define G4TRANSFORM3D_HH

#include "globals.hh"
///#include <CLHEP/Geometry/Transform3D.h>
#include "../CLHEP/geometry/Transform3D.h"

typedef HepGeom::Transform3D G4Transform3D;

typedef HepGeom::Rotate3D G4Rotate3D;
typedef HepGeom::RotateX3D G4RotateX3D;
typedef HepGeom::RotateY3D G4RotateY3D;
typedef HepGeom::RotateZ3D G4RotateZ3D;

typedef HepGeom::Translate3D G4Translate3D;
typedef HepGeom::TranslateX3D G4TranslateX3D;
typedef HepGeom::TranslateY3D G4TranslateY3D;
typedef HepGeom::TranslateZ3D G4TranslateZ3D;

typedef HepGeom::Reflect3D G4Reflect3D;
typedef HepGeom::ReflectX3D G4ReflectX3D;
typedef HepGeom::ReflectY3D G4ReflectY3D;
typedef HepGeom::ReflectZ3D G4ReflectZ3D;

typedef HepGeom::Scale3D G4Scale3D;
typedef HepGeom::ScaleX3D G4ScaleX3D;
typedef HepGeom::ScaleY3D G4ScaleY3D;
typedef HepGeom::ScaleZ3D G4ScaleZ3D;

#endif /* G4TRANSFORM3D_HH */
