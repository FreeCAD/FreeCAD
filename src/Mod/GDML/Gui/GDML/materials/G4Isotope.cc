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
// $Id: G4Isotope.cc,v 1.22 2008/08/11 11:53:11 vnivanch Exp $
// GEANT4 tag $Name: geant4-09-03-patch-01 $
//
//....oooOO0OOooo........oooOO0OOooo........oooOO0OOooo........oooOO0OOooo......

// 26.06.96: Code uses operators (+=, *=, ++, -> etc.) correctly, P. Urban
// 29.01.97: Forbidden to create Isotope with Z<1 or N<Z, M.Maire
// 03.05.01: flux.precision(prec) at begin/end of operator<<
// 17.07.01: migration to STL. M. Verderi.
// 13.09.01: suppression of the data member fIndexInTable
// 14.09.01: fCountUse: nb of elements which use this isotope 
// 26.02.02: fIndexInTable renewed
// 17.10.06: if fA is not defined in the constructor, it is computed from
//           NistManager v.Ivanchenko

//....oooOO0OOooo........oooOO0OOooo........oooOO0OOooo........oooOO0OOooo......

#include "G4Isotope.hh"
//#include "G4NistManager.hh"
#include <iomanip>

///BEGIN CAD-GDML
#include "../CLHEP/units/SystemOfUnits.h"
//#include "../CLHEP/units/PhysicalConstants.h"
using namespace CLHEP;
///END CAD-GDML

//....oooOO0OOooo........oooOO0OOooo........oooOO0OOooo........oooOO0OOooo......

G4IsotopeTable G4Isotope::theIsotopeTable;

//....oooOO0OOooo........oooOO0OOooo........oooOO0OOooo........oooOO0OOooo......

// Create an isotope
//
G4Isotope::G4Isotope(const G4String& Name, G4int Z, G4int N, G4double A)
: fName(Name), fZ(Z), fN(N), fA(A), fCountUse(0)
{
/*  if (Z<1) G4Exception
    (" ERROR! It is not allowed to create an Isotope with Z < 1" );

  if (N<Z) G4Exception
    (" ERROR! Attempt to create an Isotope with N < Z !!!" );
 */   
/*  if (A<=DBL_MIN) {
    fA = (G4NistManager::Instance()->GetAtomicMass(Z,N))*g/(mole*amu_c2);  
  }*/
  theIsotopeTable.push_back(this);
  fIndexInTable = theIsotopeTable.size() - 1;
}

//....oooOO0OOooo........oooOO0OOooo........oooOO0OOooo........oooOO0OOooo......

// Fake default constructor - sets only member data and allocates memory
//                            for usage restricted to object persistency

G4Isotope::G4Isotope(__void__&)
  : fZ(0), fN(0), fA(0), fCountUse(0)
{
}

//....oooOO0OOooo........oooOO0OOooo........oooOO0OOooo........oooOO0OOooo......

G4Isotope::~G4Isotope()
{
  /*
  if (fCountUse != 0)
    G4cout << "--> warning from ~G4Isotope(): the isotope " << fName
           << " is still referenced by " << fCountUse << " G4Elements \n" 
	   << G4endl;
  */     
  //remove this isotope from theIsotopeTable
  theIsotopeTable[fIndexInTable] = 0;
}  

//....oooOO0OOooo........oooOO0OOooo........oooOO0OOooo........oooOO0OOooo......

G4Isotope::G4Isotope(G4Isotope& right)
{
  *this = right;
  
  //insert this new isotope in table
  theIsotopeTable.push_back(this);
  fIndexInTable = theIsotopeTable.size() - 1;
}

//....oooOO0OOooo........oooOO0OOooo........oooOO0OOooo........oooOO0OOooo......

G4Isotope & G4Isotope::operator=(const G4Isotope& right)
{
  if (this != &right)
  {
    fName = right.fName;
    fZ = right.fZ;
    fN = right.fN;
    fA = right.fA;
  }
  return *this;
}

//....oooOO0OOooo........oooOO0OOooo........oooOO0OOooo........oooOO0OOooo......

G4int G4Isotope::operator==(const G4Isotope &right) const
{
  return (this == (G4Isotope *) &right);
}

//....oooOO0OOooo........oooOO0OOooo........oooOO0OOooo........oooOO0OOooo......

G4int G4Isotope::operator!=(const G4Isotope &right) const
{
  return (this != (G4Isotope *) &right);
}

//....oooOO0OOooo........oooOO0OOooo........oooOO0OOooo........oooOO0OOooo......

std::ostream& operator<<(std::ostream& flux, G4Isotope* isotope)
{
  std::ios::fmtflags mode = flux.flags();
  flux.setf(std::ios::fixed,std::ios::floatfield);
  G4long prec = flux.precision(3);
    
  flux
    << " Isotope: " << std::setw(5) << isotope->fName 
    << "   Z = " << std::setw(2)    << isotope->fZ 
    << "   N = " << std::setw(3)    << isotope->fN
    << "   A = " << std::setw(6) << std::setprecision(2) 
    << (isotope->fA)/(g/mole) << " g/mole";

  flux.precision(prec);       
  flux.setf(mode,std::ios::floatfield);       
  return flux;
}

//....oooOO0OOooo........oooOO0OOooo........oooOO0OOooo........oooOO0OOooo......

 std::ostream& operator<<(std::ostream& flux, G4Isotope& isotope)
{
  flux << &isotope;        
  return flux;
}

//....oooOO0OOooo........oooOO0OOooo........oooOO0OOooo........oooOO0OOooo......
     
std::ostream& operator<<(std::ostream& flux, G4IsotopeTable IsotopeTable)
{
 //Dump info for all known isotopes
   flux 
     << "\n***** Table : Nb of isotopes = " << IsotopeTable.size() 
     << " *****\n" << G4endl;
        
   for (size_t i=0; i<IsotopeTable.size(); i++)
     flux << IsotopeTable[i] << G4endl;

   return flux;
}
      
//....oooOO0OOooo........oooOO0OOooo........oooOO0OOooo........oooOO0OOooo......

const G4IsotopeTable* G4Isotope::GetIsotopeTable()
{
  return &theIsotopeTable;
}

//....oooOO0OOooo........oooOO0OOooo........oooOO0OOooo........oooOO0OOooo......

size_t G4Isotope::GetNumberOfIsotopes()
{
  return theIsotopeTable.size();
}

//....oooOO0OOooo........oooOO0OOooo........oooOO0OOooo........oooOO0OOooo......

G4Isotope* G4Isotope::GetIsotope(G4String isotopeName, G4bool warning)
{  
  // search the isotope by its name 
  for (size_t J=0 ; J<theIsotopeTable.size() ; J++)
   {
    if (theIsotopeTable[J]->GetName() == isotopeName)
      return theIsotopeTable[J];
   }
   
  // the isotope does not exist in the table
  if (warning) {
/*  G4cout << "\n---> warning from G4Isotope::GetIsotope(). The isotope: "
         << isotopeName << " does not exist in the table. Return NULL pointer."
	 << G4endl;
*/  }     
  return 0;          
}

//....oooOO0OOooo........oooOO0OOooo........oooOO0OOooo........oooOO0OOooo......
