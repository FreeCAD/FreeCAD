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
// $Id: G4GeometryTolerance.hh,v 1.2 2007/06/18 13:26:40 gcosmo Exp $
// GEANT4 tag $Name: geant4-09-03-patch-01 $
//
// --------------------------------------------------------------------
// GEANT 4 class header file 
//
// Class Description:
//
// A singleton class for computation and storage of the tolerance values
// used by the geometry modeler for precision on boundaries.
// The Cartesian tolerance defines the thickness of the surface of the
// geometrical shapes for their inner funtions and for the tracking. The
// tolerance must be greater than the largest mathematical error from the
// shape distance calculation functions. The tolerance is centred on the
// surface, e.g. the Inside() method of a solid uses a tolerance dx +/- kTol/2.
// The Cartesian tolerance can either be set to a fixed value (1E-9 mm)
// or to a value computed on the basis of the maximum extent of the
// world volume assigned through the G4GeometryManager at the beginning
// of the application -before- any geometrical object is created.

//      ---------------- G4GeometryTolerance ----------------
//
// Author: G.Cosmo (CERN), October 2006
// ------------------------------------------------------------

#ifndef G4GeometryTolerance_hh
#define G4GeometryTolerance_hh

#include "G4Types.hh"

class G4GeometryTolerance
{
  friend class G4GeometryManager;

  public:  // with description

    static G4GeometryTolerance* GetInstance();
      // Get a pointer to the unique G4GeometryTolerance,
      // creating it if necessary and setting the tolerances.
    G4double GetSurfaceTolerance() const;
      // Returns the current Cartesian tolerance of a surface.
    G4double GetAngularTolerance() const;
      // Returns the current angular tolerance.
    G4double GetRadialTolerance() const;
      // Returns the current radial tolerance.

  protected:

    static void SetSurfaceTolerance(G4double worldExtent);
      // Sets the Cartesian surface tolerance to a value computed
      // from the maximum extent of the world volume. This method
      // can be called only once, and is done only through the
      // G4GeometryManager class.

    G4GeometryTolerance();
    ~G4GeometryTolerance();
      // Protected constructor and destructor.

  private:

    static G4GeometryTolerance* fInstance;
    static G4double fCarTolerance;
    static G4double fAngTolerance;
    static G4double fRadTolerance;
    static G4bool fInitialised;
 };

#endif // G4GeometryTolerance_hh

