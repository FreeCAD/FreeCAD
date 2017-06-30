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
// $Id: G4MaterialPropertyVector.cc,v 1.17 2009/04/21 15:35:45 gcosmo Exp $
// GEANT4 tag $Name: geant4-09-03-patch-01 $
//
// 
////////////////////////////////////////////////////////////////////////
// G4MaterialPropertyVector Class Implementation
////////////////////////////////////////////////////////////////////////
//
// File:        G4MaterialPropertyVector.cc
// Version:     1.0
// Created:     1996-02-08
// Author:      Juliet Armstrong
// Updated:     1997-03-25 by Peter Gumplinger
//              > cosmetics (only)
// mail:        gum@triumf.ca
//
////////////////////////////////////////////////////////////////////////

#include "G4MaterialPropertyVector.hh"

// = operator
// ----------
//
G4MaterialPropertyVector&
G4MaterialPropertyVector::operator =(const G4MaterialPropertyVector& right)
{
  if (this == &right)  { return *this; }

  // clear the vector of current contents

  MPV.clear();

  // create an actual copy (instead of the shallow copy that the
  // assignment operator defaults to for G4RWTPtrSortedVector)

  NumEntries = 0;
  CurrentEntry = -1;

  for (G4int i = 0 ; i < right.NumEntries; i++)
  {
    G4MPVEntry *newElement = new G4MPVEntry(right.GetEntry(i));
    MPV.push_back(newElement);
    NumEntries++;
  }

  return *this;
}

/////////////////
// Constructors
/////////////////

G4MaterialPropertyVector::
G4MaterialPropertyVector(G4double *PhotonEnergies,
                         G4double *PropertyValues,
                         G4int     NumElements)
{
  NumEntries = 0;
  CurrentEntry = -1;

  // create a vector filling it with the values
  // from PhotonEnergies[] and PropertyValues[] 

  for(G4int i = 0; i < NumElements; i++)
  {
    AddElement(PhotonEnergies[i], PropertyValues[i]);
  }
}

G4MaterialPropertyVector::
G4MaterialPropertyVector(const G4MaterialPropertyVector &right)
{
  // create an actual copy (instead of the shallow copy that the
  // assignment operator defaults to for G4RWTPtrSortedVector)

  NumEntries = 0;
  CurrentEntry = -1;

  for (G4int i = 0 ; i < right.NumEntries; i++)
  {
    G4MPVEntry *newElement = new G4MPVEntry(right.GetEntry(i));
    MPV.push_back(newElement);
    NumEntries++;
  }
}

////////////////
// Destructor
////////////////

G4MaterialPropertyVector::~G4MaterialPropertyVector()
{
  MPV.clear();
}

////////////
// Methods
////////////

void G4MaterialPropertyVector::RemoveElement(G4double aPhotonEnergy)
{
  G4MPVEntry *newElement;
  G4MPVEntry *success=0;

  newElement = new G4MPVEntry(aPhotonEnergy, DBL_MAX);

  std::vector<G4MPVEntry*>::iterator i;
  for (i = MPV.begin(); i != MPV.end(); i++)
  {
    if (**i == *newElement) { success = *i; break; }
  }
  //  success = MPV.remove(newElement);

  if(success == 0)
  {
///    G4Exception("G4MaterialPropertyVector::RemoveElement()", "NotFound",
///                FatalException, "Element not found !");
    return;
  }
  else
  {
    MPV.erase(i); // remove done here.
  }

  NumEntries--;
}

G4double G4MaterialPropertyVector::GetProperty(G4double aPhotonEnergy) const
{
  G4MPVEntry *target, *temp; 
  G4int left, right;
  G4double ratio1, ratio2, pmright, pmleft, InterpolatedValue;
 
  /////////////////////////
  // Establish table range 
  /////////////////////////

  G4double PMmin   = MPV.front()->GetPhotonEnergy(); 
  G4double minProp = MPV.front()->GetProperty(); 
  G4double PMmax   = MPV.back() ->GetPhotonEnergy();
  G4double maxProp = MPV.back() ->GetProperty();

  ///////////////////////////////////////////
  // Does value fall outside range of table?
  ///////////////////////////////////////////

  if (aPhotonEnergy < PMmin) 
  {
///    G4Exception("G4MaterialPropertyVector::GetProperty()", "OutOfRange",
///                JustWarning, "Attempt to retrieve property below range !");
    return minProp; 
  } 

        if (aPhotonEnergy > PMmax)
  {
///    G4Exception("G4MaterialPropertyVector::GetProperty()", "OutOfRange",
///                JustWarning, "Attempt to retrieve property above range !");
    return maxProp;
  } 
  
  target = new G4MPVEntry(aPhotonEnergy, 0.0);

  temp = 0;
  //temp = MPV.find(target);
  std::vector<G4MPVEntry*>::const_iterator i;
  for (i = MPV.begin(); i != MPV.end(); i++)
  {
    if (**i == *target)  { temp = *i; break; }
  }
  if (temp != 0)
  {
    ////////////////////////
    // Return actual value
    ////////////////////////

    G4double retval = temp->GetProperty();
    delete target;
    return retval;
  }
  else
  {
    //////////////////////////////
    // Return interpolated value 
    //////////////////////////////

    GetAdjacentBins(aPhotonEnergy, &left, &right);

    pmleft = MPV[left]->GetPhotonEnergy();
    pmright = MPV[right]->GetPhotonEnergy();
    ratio1 = (aPhotonEnergy-pmleft)/(pmright-pmleft);
    ratio2 = 1 - ratio1;
    InterpolatedValue = MPV[left]->GetProperty()*ratio2 + 
                        MPV[right]->GetProperty()*ratio1;
    delete target;
    return InterpolatedValue;
  }  
}

G4double G4MaterialPropertyVector::GetPhotonEnergy(G4double aProperty) const
{
  //         ***NB*** 
  // Assumes that the property is an increasing function of photon energy
  // (e.g. refraction index)
  //         ***NB*** 
  //
  // Returns the photon energy corresponding to the property value passed in.
  // If several photon energy values correspond to the value passed in, the
  // function returns the first photon energy in the vector that corresponds
  // to that value. 

  G4int left, right, mid;
  G4double ratio1, ratio2, pright, pleft;

  //////////////////////////
  // Establish Table range
  //////////////////////////

  G4double PropMin = MPV.front()->GetProperty();
  G4double PMmin   = MPV.front()->GetPhotonEnergy();
  G4double PropMax = MPV.back() ->GetProperty();
  G4double PMmax   = MPV.back() ->GetPhotonEnergy();

  ///////////////////////////////////////////
  // Does value fall outside range of table?
  ///////////////////////////////////////////

  if (aProperty < PropMin) 
  {
/*    G4Exception("G4MaterialPropertyVector::GetPhotonEnergy()",
                "OutOfRange", JustWarning,
                "Attempt to retrieve photon energy out of range");
*/    return PMmin;
  }

  if (aProperty > PropMax)
  {
/*    G4Exception("G4MaterialPropertyVector::GetPhotonEnergy()",
                "OutOfRange", JustWarning,
                "Attempt to retrieve photon energy out of range");
*/    return PMmax;
  }

  //////////////////////////////
  // Return interpolated value
  //////////////////////////////

  left = 0;
  right = MPV.size(); // was .entries()

  // find values in bins on either side of aProperty 
  
  do
  {
    mid = (left + right)/2;
    if (MPV[mid]->GetProperty() == aProperty)
    {
      // Get first photon energy value in vector that 
      // corresponds to property value  

      while ((mid-1 >= 0) && (MPV[mid-1]->GetProperty() == aProperty))
      {
          mid--;
      }
      return MPV[mid]->GetPhotonEnergy();
    }
    if (MPV[mid]->GetProperty() < aProperty)
    {
      left = mid;
    }
    else
    {
      right = mid;
    }
  } while ((right - left) > 1);

  pleft = MPV[left]->GetProperty();
  pright = MPV[right]->GetProperty();
  ratio1 = (aProperty - pleft) / (pright - pleft);
  ratio2 = 1 - ratio1;

  return  (MPV[left]->GetPhotonEnergy()*ratio2 +
           MPV[right]->GetPhotonEnergy()*ratio1);
}

void G4MaterialPropertyVector::DumpVector()
{
  if (MPV.empty())
  {
/*    G4Exception("G4MaterialPropertyVector::DumpVector()", "EmptyVector",
                JustWarning, "Nothing to dump. Vector is empty !");
  */}
  else
  {
    for (G4int i = 0; i < NumEntries; i++)
    {
    //  G4cout << "MPV["<< i << "]: ";
      MPV[i]->DumpEntry();
    }
    //G4cout << " Done DumpVector of " << NumEntries << " entries." << G4endl;
  }
} 

void G4MaterialPropertyVector::GetAdjacentBins(G4double aPhotonEnergy,
                                               G4int *left, G4int *right) const
{
  G4int mid;

  *left = 0;
  *right = (MPV.size() - 1); // was .entries()

  // find values in bins on either side of aPhotonEnergy

  do
  {
    mid = (*left + *right)/2;
    if (MPV[mid]->GetPhotonEnergy() < aPhotonEnergy) 
    {
      *left = mid;
    }
    else
    {
      *right = mid;
    }
  } while ((*right - *left) > 1);
}
