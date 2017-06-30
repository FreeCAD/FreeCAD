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
// $Id: G4Element.hh,v 1.27 2009/09/19 14:13:03 vnivanch Exp $
// GEANT4 tag $Name: geant4-09-03-patch-01 $
//

// class description
//
// An element is a chemical element either directly defined in terms of
// its charactaristics: its name, symbol,
//                      Z (effective atomic number)
//                      N (effective number of nucleons)
//                      A (effective mass of a mole)
// or in terms of a collection of constituent isotopes with specified 
// relative abundance (i.e. fraction of nb of atomes per volume).
//
// Quantities, with physical meaning or not, which are constant in a given 
// element are computed and stored here as Derived data members.
//
// The class contains as a private static member the table of defined
// elements (an ordered vector of elements).
//
// Elements can be assembled singly or in mixtures into materials used
// in volume definitions via the G4Material class.
//

//....oooOO0OOooo........oooOO0OOooo........oooOO0OOooo........oooOO0OOooo......

// 09-07-96, new data members added by L.Urban
// 17-01-97, aesthetic rearrangement, M.Maire
// 20-01-97, Tsai formula for the rad length, M.Maire
// 21-01-97, remove mixture flag, M.Maire
// 24-01-97, new data member: fTaul
//           new method: ComputeIonisationPara, M.Maire
// 20-03-97, corrected initialization of pointers, M.Maire
// 27-06-97, new function GetIsotope(int), M.Maire
// 24-02-98, fWeightVector becomes fRelativeAbundanceVector
// 27-04-98, atomic shell stuff, V. Grichine
// 09-07-98, Ionisation parameters removed from the class, M.Maire
// 04-08-98, new method GetElement(elementName), M.Maire
// 16-11-98, Subshell -> Shell, mma
// 30-03-01, suppression of the warning message in GetElement
// 17-07-01, migration to STL, M. Verderi
// 13-09-01, stl migration. Suppression of the data member fIndexInTable
// 14-09-01, fCountUse: nb of materials which use this element
// 26-02-02, fIndexInTable renewed 
// 01-04-05, new data member fIndexZ to count the number of elements with same Z
// 17-10-06: Add Get/Set fNaturalAbandances (V.Ivanchenko)
// 17.09.09, add fNbOfShellElectrons and methods (V. Grichine)
//....oooOO0OOooo........oooOO0OOooo........oooOO0OOooo........oooOO0OOooo......

#ifndef G4ELEMENT_HH
#define G4ELEMENT_HH

//#include "globals.hh"
#include "../global/globals.hh"
#include <vector>
//#include "G4ios.hh"
#include "../global/G4ios.hh"
#include "G4Isotope.hh"
/*#include "G4AtomicShells.hh"
#include "G4IonisParamElm.hh"*/
#include "G4IsotopeVector.hh"
#include "G4ElementTable.hh"

//....oooOO0OOooo........oooOO0OOooo........oooOO0OOooo........oooOO0OOooo......

class G4Element
{
public:  // with description

  //
  // Constructor to Build an element directly; no reference to isotopes
  //
  G4Element(const G4String& name,		//its name
            const G4String& symbol,		//its symbol
                  G4double  Zeff,		//atomic number
                  G4double  Aeff);		//mass of mole
                  
  //
  // Constructor to Build an element from isotopes via AddIsotope
  //
  G4Element(const G4String& name,		//its name
            const G4String& symbol,		//its symbol
            G4int nbIsotopes);			//nb of isotopes

  //
  // Add an isotope to the element
  // 
  void AddIsotope(G4Isotope* isotope,			//isotope 
                  G4double   RelativeAbundance);	//fraction of nb of 
                  					//atomes per volume
  virtual ~G4Element();
  
  //
  // retrieval methods
  //
  const G4String& GetName()   const {return fName;}
  const G4String& GetSymbol() const {return fSymbol;}
  G4double GetZ()      const {return fZeff;}     //atomic number
  G4double GetN()      const {return fNeff;}     //number of nucleons
  G4double GetA()      const {return fAeff;}     //mass of a mole
  G4bool   GetNaturalAbandancesFlag();

  void     SetNaturalAbandancesFlag(G4bool);
  
  //the number of atomic shells in this element:
  //
  G4int GetNbOfAtomicShells() const {return fNbOfAtomicShells;}
  
  //the binding energy of the shell, ground shell index=0
  //
  G4double GetAtomicShell(G4int index) const;

  //the number of electrons at the shell, ground shell index=0
  //
  G4int GetNbOfShellElectrons(G4int index) const;
    
  //number of isotopes constituing this element:
  //
  size_t GetNumberOfIsotopes() const {return fNumberOfIsotopes;}
   
  //vector of pointers to isotopes constituing this element:
  //
  G4IsotopeVector* GetIsotopeVector() const {return theIsotopeVector;}
    
  //vector of relative abundance of each isotope:
  //
  G4double* GetRelativeAbundanceVector() const 
                   {return fRelativeAbundanceVector;}
    
  const G4Isotope* GetIsotope(G4int iso) const 
                   {return (*theIsotopeVector)[iso];}

  //the (static) Table of Elements:
  //
  static 
  const  G4ElementTable* GetElementTable();
  
  static 
  size_t GetNumberOfElements();
  
  //the index of this element in the Table:
  //
  size_t GetIndex() const {return fIndexInTable;}
    
  //return pointer to an element, given its name:
  //
  static
  G4Element* GetElement(G4String name, G4bool warning=true);
  
  //count number of materials which use this element
  //
  G4int GetCountUse() const {return fCountUse;}
  void  increaseCountUse()  {fCountUse++;}
  void  decreaseCountUse()  {fCountUse--;}
  
  //count elements with same Z
  //
  G4int GetIndexZ() const {return fIndexZ;}
        
  //Coulomb correction factor:
  //
  G4double GetfCoulomb() const {return fCoulomb;}
   
  //Tsai formula for the radiation length:
  //
  G4double GetfRadTsai() const {return fRadTsai;}
    
  //pointer to ionisation parameters:
  //
///  G4IonisParamElm* GetIonisation() const {return fIonisation;}
    
  // printing methods
  //    
  friend std::ostream& operator<<(std::ostream&, G4Element*);    
  friend std::ostream& operator<<(std::ostream&, G4Element&);    
  friend std::ostream& operator<<(std::ostream&, G4ElementTable);

public:  // without description

  G4int operator==(const G4Element&) const;
  G4int operator!=(const G4Element&) const;

  G4Element(__void__&);
    // Fake default constructor for usage restricted to direct object
    // persistency for clients requiring preallocation of memory for
    // persistifiable objects.

  void SetName(const G4String& name)  {fName=name;}

private:

  G4Element(G4Element&);
  const G4Element & operator=(const G4Element&);
       
private:

  void InitializePointers();
  void ComputeDerivedQuantities();
  void ComputeCoulombFactor();
  void ComputeLradTsaiFactor();

private:

  //
  // Basic data members (which define an Element)
  //
  G4String fName;              // name
  G4String fSymbol;            // symbol
  G4double fZeff;              // Effective atomic number
  G4double fNeff;              // Effective number of nucleons
  G4double fAeff;              // Effective mass of a mole
    
  G4int fNbOfAtomicShells;     // number  of atomic shells
  G4double* fAtomicShells ;    // Pointer to atomic shell binding energies
  G4int* fNbOfShellElectrons; // pointer to the number of subshell electrons
    
  // Isotope vector contains constituent isotopes of the element   
  size_t fNumberOfIsotopes;    // Number of isotopes added to the element
  G4IsotopeVector* theIsotopeVector;
  G4double* fRelativeAbundanceVector;     // Fraction nb of atomes per volume
                                          // for each constituent
  G4int fCountUse;          // nb of materials which use this element
  G4int fIndexZ;            // index for elements with same Z
    
  // Set up the static Table of Elements
  static G4ElementTable theElementTable;
  size_t fIndexInTable;
  G4bool fNaturalAbandances;

  //
  // Derived data members (computed from the basic data members)
  //
  G4double fCoulomb;             // Coulomb correction factor
  G4double fRadTsai;             // Tsai formula for the radiation length
///  G4IonisParamElm* fIonisation;  // Pointer to ionisation parameters
};

inline G4bool G4Element::GetNaturalAbandancesFlag()
{
  return fNaturalAbandances;
}

inline void G4Element::SetNaturalAbandancesFlag(G4bool val)
{
  fNaturalAbandances = val;
}

//....oooOO0OOooo........oooOO0OOooo........oooOO0OOooo........oooOO0OOooo......

#endif
