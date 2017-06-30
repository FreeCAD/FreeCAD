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
// $Id: G4VoxelLimits.hh,v 1.9 2006/06/29 18:33:13 gunter Exp $
// GEANT4 tag $Name: geant4-09-03-patch-01 $
//
// class G4VoxelLimits
//
// Class description:
//
// Represents limitation/restrictions of space, where restrictions
// are only made perpendicular to the cartesian axes.
//
//
// Member data:
//
// G4double fxAxisMin,fxAxisMax
// G4double fyAxisMin,fyAxisMax
// G4double fzAxisMin,fzAxisMax
//   - The min and max values along each axis. +-kInfinity if not restricted.
//
//
// Notes:
//
// Beware no break statements after returns in switch(pAxis)s.

// History:
// 13.07.95 P.Kent Initial version.
// --------------------------------------------------------------------
#ifndef G4VOXELLIMITS_HH
#define G4VOXELLIMITS_HH
/*
#include "G4Types.hh"
#include "geomdefs.hh"

#include "G4ThreeVector.hh"
*/
#include "../../global/G4Types.hh"
#include "../../global/geomdefs.hh"

#include "../../global/G4ThreeVector.hh"

#include <assert.h>

class G4VoxelLimits
{
  public: // with description
  
    G4VoxelLimits();
      // Constructor - initialise to be unlimited. Volume unrestricted.

    ~G4VoxelLimits();
      // Destructor. No actions.

    void AddLimit(const EAxis pAxis, const G4double pMin,const G4double pMax);
      // Restrict the volume to between specified min and max along the
      // given axis. Cartesian axes only, pMin<=pMax.

    G4double GetMaxXExtent() const;
      // Return maximum x extent.
    G4double GetMaxYExtent() const;
      // Return maximum y extent.
    G4double GetMaxZExtent() const;
      // Return maximum z extent.

    G4double GetMinXExtent() const;
      // Return minimum x extent.
    G4double GetMinYExtent() const;
      // Return minimum y extent.
    G4double GetMinZExtent() const;
      // Return minimum z extent.

    G4double GetMaxExtent(const EAxis pAxis) const;
      // Return maximum extent of volume along specified axis.
    G4double GetMinExtent(const EAxis pAxis) const;
      // Return minimum extent of volume along specified axis.

    G4bool IsXLimited() const;
      // Return true if the x axis is limited.
    G4bool IsYLimited() const;
      // Return true if the y axis is limited.
    G4bool IsZLimited() const;
      // Return true if the z axis is limited.

    G4bool IsLimited() const;
      // Return true if limited along any axis
    G4bool IsLimited(const EAxis pAxis) const;
      // Return true if the specified axis is restricted/limited.

    G4bool ClipToLimits(G4ThreeVector& pStart,G4ThreeVector& pEnd) const;
      // Clip the line segment pStart->pEnd to the volume described by the
      // current limits. Return true if the line remains after clipping,
      // else false, and leave the vectors in an undefined state.

    G4bool Inside(const G4ThreeVector& pVec) const;
      // Return true if the specified vector is inside/on boundaries of limits.

    G4int OutCode(const G4ThreeVector& pVec) const;
      // Calculate the `outcode' for the specified vector.
      // Intended for use during clipping against the limits
      // The bits are set given the following conditions:
      //   0      pVec.x()<fxAxisMin && IsXLimited()
      //   1      pVec.x()>fxAxisMax && IsXLimited()
      //   2      pVec.y()<fyAxisMin && IsYLimited()
      //   3      pVec.y()>fyAxisMax && IsYLimited()
      //   4      pVec.z()<fzAxisMin && IsZLimited()
      //   5      pVec.z()>fzAxisMax && IsZLimited()

  private:

    G4double fxAxisMin,fxAxisMax;
    G4double fyAxisMin,fyAxisMax;
    G4double fzAxisMin,fzAxisMax;
};

#include "G4VoxelLimits.icc"

std::ostream& operator << (std::ostream& os, const G4VoxelLimits& pLim);
  // Print the limits to the stream in the form:
  //  "{(xmin,xmax) (ymin,ymax) (zmin,zmax)}"
  // Replace (xmin,xmax) by (-,-)  when not limited.

#endif
