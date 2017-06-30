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
// $Id: G4Material.cc,v 1.42 2008/08/13 16:06:42 vnivanch Exp $
// GEANT4 tag $Name: geant4-09-03-patch-01 $
//
//....oooOO0OOooo........oooOO0OOooo........oooOO0OOooo........oooOO0OOooo......
//
// 26-06-96, Code uses operators (+=, *=, ++, -> etc.) correctly, P. Urban
// 10-07-96, new data members added by L.Urban
// 12-12-96, new data members added by L.Urban
// 20-01-97, aesthetic rearrangement. RadLength calculation modified.
//           Data members Zeff and Aeff REMOVED (i.e. passed to the Elements).
//           (local definition of Zeff in DensityEffect and FluctModel...)
//           Vacuum defined as a G4State. Mixture flag removed, M.Maire. 
// 29-01-97, State=Vacuum automatically set density=0 in the contructors.
//           Subsequent protections have been put in the calculation of 
//           MeanExcEnergy, ShellCorrectionVector, DensityEffect, M.Maire.
// 11-02-97, ComputeDensityEffect() rearranged, M.Maire.
// 20-03-97, corrected initialization of pointers, M.Maire.
// 28-05-98, the kState=kVacuum has been removed.
//           automatic check for a minimal density, M.Maire 
// 12-06-98, new method AddMaterial() allowing mixture of materials, M.Maire  
// 09-07-98, ionisation parameters removed from the class, M.Maire
// 05-10-98, change names: NumDensity -> NbOfAtomsPerVolume
// 18-11-98, new interface to SandiaTable
// 19-01-99  enlarge tolerance on test of coherence of gas conditions
// 19-07-99, Constructors with chemicalFormula added by V.Ivanchenko
// 16-01-01, Nuclear interaction length, M.Maire
// 12-03-01, G4bool fImplicitElement;
//           copy constructor and assignement operator revised (mma)
// 03-05-01, flux.precision(prec) at begin/end of operator<<
// 17-07-01, migration to STL. M. Verderi.
// 14-09-01, Suppression of the data member fIndexInTable
// 26-02-02, fIndexInTable renewed
// 16-04-02, G4Exception put in constructor with chemical formula
// 06-05-02, remove the check of the ideal gas state equation
// 06-08-02, remove constructors with chemical formula (mma)
// 22-01-04, proper STL handling of theElementVector (Hisaya)
// 30-03-05, warning in GetMaterial(materialName) 
// 09-03-06, minor change of printout (V.Ivanchenko) 
// 10-01-07, compute fAtomVector in the case of mass fraction (V.Ivanchenko) 
// 27-07-07, improve destructor (V.Ivanchenko) 
// 18-10-07, move definition of material index to InitialisePointers (V.Ivanchenko) 
// 13-08-08, do not use fixed size arrays (V.Ivanchenko) 
// 

// 
//....oooOO0OOooo........oooOO0OOooo........oooOO0OOooo........oooOO0OOooo......

#include "G4Material.hh"
//#include "G4UnitsTable.hh"
#include <iomanip>

///BEGIN CAD-GDML
#include "../CLHEP/units/PhysicalConstants.h"
using namespace CLHEP;
///END CAD-GDML

G4MaterialTable G4Material::theMaterialTable;

//....oooOO0OOooo........oooOO0OOooo........oooOO0OOooo........oooOO0OOooo......

// Constructor to create a material from scratch

G4Material::G4Material(const G4String& name, G4double z,
                       G4double a, G4double density, 
                       G4State state, G4double temp, G4double pressure)
  : fName(name)		       
{/**/
  InitializePointers();
    
  if (density < universe_mean_density)
     {// G4cerr << "--- Warning from G4Material::G4Material()"
       //       << " define a material with density=0 is not allowed. \n"
       //       << " The material " << name << " will be constructed with the"
       //       << " default minimal density: " << universe_mean_density/(g/cm3) 
       //       << "g/cm3" << G4endl;
       density = universe_mean_density;
     } 

  fDensity  = density;
  fState    = state;
  fTemp     = temp;
  fPressure = pressure;
  fChemicalFormula = " ";

  // Initialize theElementVector allocating one
  // element corresponding to this material
  maxNbComponents        = fNumberOfComponents = fNumberOfElements = 1;
  fArrayLength           = maxNbComponents;
  fImplicitElement       = true;
  theElementVector       = new G4ElementVector();
  theElementVector->push_back( new G4Element(name, " ", z, a));  
  fMassFractionVector    = new G4double[1];
  fMassFractionVector[0] = 1. ;
  
  (*theElementVector)[0] -> increaseCountUse();
  
  if (fState == kStateUndefined)
    {
     if (fDensity > kGasThreshold) fState = kStateSolid;
     else                          fState = kStateGas;
    }

  ComputeDerivedQuantities();/**/
}

//....oooOO0OOooo........oooOO0OOooo........oooOO0OOooo........oooOO0OOooo......

// Constructor to create a material from a List of constituents
// (elements and/or materials)  added with AddElement or AddMaterial

G4Material::G4Material(const G4String& name, G4double density,
                       G4int nComponents,
                       G4State state, G4double temp, G4double pressure)
  : fName(name)		       
{/**/
  InitializePointers();
    
  if (density < universe_mean_density)
    {//G4cerr << "--- Warning from G4Material::G4Material()"
     //       << " define a material with density=0 is not allowed. \n"
     //       << " The material " << name << " will be constructed with the"
      //      << " default minimal density: " << universe_mean_density/(g/cm3) 
      //      << "g/cm3" << G4endl;
     density = universe_mean_density;
    }
        
  fDensity  = density;
  fState    = state;
  fTemp     = temp;
  fPressure = pressure;
  fChemicalFormula = " ";
    
  maxNbComponents     = nComponents;
  fArrayLength        = maxNbComponents;
  fNumberOfComponents = fNumberOfElements = 0;
  fImplicitElement    = false;
  theElementVector    = new G4ElementVector();
  theElementVector->reserve(maxNbComponents);  
    
  if (fState == kStateUndefined) 
    {
      if (fDensity > kGasThreshold) fState = kStateSolid;
      else                          fState = kStateGas;
    }/**/
}

//....oooOO0OOooo........oooOO0OOooo........oooOO0OOooo........oooOO0OOooo......

// Fake default constructor - sets only member data and allocates memory
//                            for usage restricted to object persistency
/**/
G4Material::G4Material(__void__&)
  : fNumberOfComponents(0), fNumberOfElements(0), theElementVector(0), 
    fImplicitElement(false), fMassFractionVector(0), fAtomsVector(0), 
    fMaterialPropertiesTable(0), fIndexInTable(0), 
//    VecNbOfAtomsPerVolume(0), fIonisation(0), fSandiaTable(0)
    VecNbOfAtomsPerVolume(0)
{
}
/**/

//....oooOO0OOooo........oooOO0OOooo........oooOO0OOooo........oooOO0OOooo......

// AddElement -- composition by atom count

void G4Material::AddElement(G4Element* element, G4int nAtoms)
{   
  // initialization
  if ( fNumberOfElements == 0 ) {
     fAtomsVector        = new G4int   [fArrayLength];
     fMassFractionVector = new G4double[fArrayLength];
  }

  // filling ...
  if ( G4int(fNumberOfElements) < maxNbComponents ) {
     theElementVector->push_back(element);     
     fAtomsVector       [fNumberOfElements] = nAtoms;
     fNumberOfComponents = ++fNumberOfElements;
     element->increaseCountUse();
  } else {
/*    G4cerr << "G4Material::AddElement ERROR for " << fName << " nElement= " 
	   <<  fNumberOfElements << G4endl;
    G4Exception
    ("ERROR!!! - Attempt to add more than the declared number of elements.");
*/  }
  // filled.
  if ( G4int(fNumberOfElements) == maxNbComponents ) {     
     // compute proportion by mass
     size_t i=0;
     G4double Zmol(0.), Amol(0.);
     for (i=0;i<fNumberOfElements;i++) {
       Zmol +=  fAtomsVector[i]*(*theElementVector)[i]->GetZ();
       Amol +=  fAtomsVector[i]*(*theElementVector)[i]->GetA();
     }
     for (i=0;i<fNumberOfElements;i++) {
       fMassFractionVector[i] = fAtomsVector[i]
	                       *(*theElementVector)[i]->GetA()/Amol;
     }

     ComputeDerivedQuantities();
  }
}

//....oooOO0OOooo........oooOO0OOooo........oooOO0OOooo........oooOO0OOooo......

// AddElement -- composition by fraction of mass

void G4Material::AddElement(G4Element* element, G4double fraction)
{
  // initialization
  if (fNumberOfComponents == 0) {
    fMassFractionVector = new G4double[fArrayLength];
    fAtomsVector        = new G4int   [fArrayLength];
  }

  // filling ...
  if (G4int(fNumberOfComponents) < maxNbComponents) {
      size_t el = 0;
      while ((el<fNumberOfElements)&&(element!=(*theElementVector)[el])) el++;
      if (el<fNumberOfElements) fMassFractionVector[el] += fraction;
      else {
	theElementVector->push_back(element); 
        fMassFractionVector[el] = fraction;
        fNumberOfElements++;
	element->increaseCountUse();
      }
      fNumberOfComponents++;  
  } else {
/*    G4cerr << "G4Material::AddElement ERROR for " << fName << " nElement= " 
	   <<  fNumberOfElements << G4endl;
    G4Exception
    ("ERROR!!! - Attempt to add more than the declared number of components.");
*/  }    

  // filled.
  if (G4int(fNumberOfComponents) == maxNbComponents) {

     size_t i=0;
     G4double Zmol(0.), Amol(0.);
     // check sum of weights -- OK?
     G4double wtSum(0.0);
     for (i=0;i<fNumberOfElements;i++) {
       wtSum += fMassFractionVector[i];
       Zmol +=  fMassFractionVector[i]*(*theElementVector)[i]->GetZ();
       Amol +=  fMassFractionVector[i]*(*theElementVector)[i]->GetA();
     }
     if (std::abs(1.-wtSum) > perThousand) {
/*       G4cerr << "WARNING !! for " << fName << " sum of fractional masses "
              <<  wtSum << " is not 1 - results may be wrong" 
              << G4endl;
*/     }
     for (i=0;i<fNumberOfElements;i++) {
       fAtomsVector[i] = 
	 G4int(fMassFractionVector[i]*Amol/(*theElementVector)[i]->GetA()+0.5);
     }
     
     ComputeDerivedQuantities();
  }
}

//....oooOO0OOooo........oooOO0OOooo........oooOO0OOooo........oooOO0OOooo......

// AddMaterial -- composition by fraction of mass

void G4Material::AddMaterial(G4Material* material, G4double fraction)
{
  // initialization
  if (fNumberOfComponents == 0) {
    fMassFractionVector = new G4double[fArrayLength];
    fAtomsVector        = new G4int   [fArrayLength];
  }

  size_t nelm = material->GetNumberOfElements();

  // arrays should be extended
  if(nelm > 1) {
    G4int nold    = fArrayLength;
    fArrayLength += nelm - 1;
    G4double* v1 = new G4double[fArrayLength];
    G4int* i1    = new G4int[fArrayLength];
    for(G4int i=0; i<nold; i++) {
      v1[i] = fMassFractionVector[i];
      i1[i] = fAtomsVector[i];
    }
    delete [] fAtomsVector;
    delete [] fMassFractionVector;
    fMassFractionVector = v1;
    fAtomsVector = i1;
  }

  // filling ...
  if (G4int(fNumberOfComponents) < maxNbComponents) {
     for (size_t elm=0; elm<nelm; elm++)
       {
        G4Element* element = (*(material->GetElementVector()))[elm];
        size_t el = 0;
        while ((el<fNumberOfElements)&&(element!=(*theElementVector)[el])) el++;
        if (el < fNumberOfElements) fMassFractionVector[el] += fraction
                                          *(material->GetFractionVector())[elm];
        else {
	  theElementVector->push_back(element); 
          fMassFractionVector[el] = fraction
	                                  *(material->GetFractionVector())[elm];
          fNumberOfElements++;
	  element->increaseCountUse();
        }
       } 
      fNumberOfComponents++;  
  } else {
/*    G4cerr << "G4Material::AddElement ERROR for " << fName << " nElement= " 
	   <<  fNumberOfElements << G4endl;
    G4Exception
      ("ERROR!!! - Attempt to add more than the declared number of components.");
 */ }    
  // filled.
  
  if (G4int(fNumberOfComponents) == maxNbComponents) {
     size_t i=0;
     G4double Zmol(0.), Amol(0.);
     // check sum of weights -- OK?
     G4double wtSum(0.0);
     for (i=0;i<fNumberOfElements;i++) {
       wtSum += fMassFractionVector[i];
       Zmol +=  fMassFractionVector[i]*(*theElementVector)[i]->GetZ();
       Amol +=  fMassFractionVector[i]*(*theElementVector)[i]->GetA();
     }
     if (std::abs(1.-wtSum) > perThousand) {
  /*     G4cerr << "WARNING !! for " << fName << " sum of fractional masses "
              <<  wtSum << " is not 1 - results may be wrong" 
              << G4endl;
  */   }
     for (i=0;i<fNumberOfElements;i++) {
       fAtomsVector[i] = 
	 G4int(fMassFractionVector[i]*Amol/(*theElementVector)[i]->GetA()+0.5);
     }
     
     ComputeDerivedQuantities();
  }
}

//....oooOO0OOooo........oooOO0OOooo........oooOO0OOooo........oooOO0OOooo......

void G4Material::ComputeDerivedQuantities()
{
  // Header routine to compute various properties of material.
  // 

  // Number of atoms per volume (per element), total nb of electrons per volume
  G4double Zi, Ai;
  TotNbOfAtomsPerVolume = 0.;
  if (VecNbOfAtomsPerVolume) delete [] VecNbOfAtomsPerVolume;
  VecNbOfAtomsPerVolume = new G4double[fNumberOfElements];
  TotNbOfElectPerVolume = 0.;
  
  for (size_t i=0;i<fNumberOfElements;i++) {
     Zi = (*theElementVector)[i]->GetZ();
     Ai = (*theElementVector)[i]->GetA();
     VecNbOfAtomsPerVolume[i] = Avogadro*fDensity*fMassFractionVector[i]/Ai;
     TotNbOfAtomsPerVolume += VecNbOfAtomsPerVolume[i];
     TotNbOfElectPerVolume += VecNbOfAtomsPerVolume[i]*Zi;
  }
        
  ComputeRadiationLength();
  ComputeNuclearInterLength();

///  if (fIonisation) delete fIonisation;
///  fIonisation  = new G4IonisParamMat(this);
///  if (fSandiaTable) delete fSandiaTable;
///  fSandiaTable = new G4SandiaTable(this);
}

//....oooOO0OOooo........oooOO0OOooo........oooOO0OOooo........oooOO0OOooo......

void G4Material::ComputeRadiationLength()
{
  G4double radinv = 0.0 ;
  for (size_t i=0;i<fNumberOfElements;i++) {
     radinv += VecNbOfAtomsPerVolume[i]*((*theElementVector)[i]->GetfRadTsai());
   }
  fRadlen = (radinv <= 0.0 ? DBL_MAX : 1./radinv);
}

//....oooOO0OOooo........oooOO0OOooo........oooOO0OOooo........oooOO0OOooo......

void G4Material::ComputeNuclearInterLength()
{
  const G4double lambda0 = 35*g/cm2;
  G4double NILinv = 0.0;
  for (size_t i=0;i<fNumberOfElements;i++) {
     NILinv +=
     VecNbOfAtomsPerVolume[i]*std::pow(((*theElementVector)[i]->GetN()),0.6666667); 
   }
  NILinv *= amu/lambda0; 
  fNuclInterLen = (NILinv <= 0.0 ? DBL_MAX : 1./NILinv);
}

//....oooOO0OOooo........oooOO0OOooo........oooOO0OOooo........oooOO0OOooo......

void G4Material::InitializePointers()
{
  theElementVector         = 0;
  fMassFractionVector      = 0;
  fAtomsVector             = 0;
  fMaterialPropertiesTable = 0;
    
  VecNbOfAtomsPerVolume    = 0;
  //fIonisation              = 0;
  //fSandiaTable             = 0;

  // Store in the static Table of Materials
  theMaterialTable.push_back(this);
  fIndexInTable = theMaterialTable.size() - 1;
}

//....oooOO0OOooo........oooOO0OOooo........oooOO0OOooo........oooOO0OOooo......

const G4MaterialTable* G4Material::GetMaterialTable()
{
  return &theMaterialTable;
}

//....oooOO0OOooo........oooOO0OOooo........oooOO0OOooo........oooOO0OOooo......

size_t G4Material::GetNumberOfMaterials()
{
  return theMaterialTable.size();
}

//....oooOO0OOooo........oooOO0OOooo........oooOO0OOooo........oooOO0OOooo......

G4Material* G4Material::GetMaterial(G4String materialName, G4bool warning)
{  
  // search the material by its name 
  for (size_t J=0 ; J<theMaterialTable.size() ; J++)
   {
    if (theMaterialTable[J]->GetName() == materialName)
       return theMaterialTable[J];
   }
  
  // the material does not exist in the table
/*  if (warning) {
  G4cout << "\n---> warning from G4Material::GetMaterial(). The material: "
         << materialName << " does not exist in the table. Return NULL pointer."
	 << G4endl;
  }	 */
  return 0;          
}

//....oooOO0OOooo........oooOO0OOooo........oooOO0OOooo........oooOO0OOooo......

G4Material::G4Material(const G4Material& right)
{
  InitializePointers();
  *this = right;
}

//....oooOO0OOooo........oooOO0OOooo........oooOO0OOooo........oooOO0OOooo......

G4Material::~G4Material()
{
  //  G4cout << "### Destruction of material " << fName << " started" <<G4endl;
  if (theElementVector)       delete    theElementVector;
  if (fMassFractionVector)    delete [] fMassFractionVector;
  if (fAtomsVector)           delete [] fAtomsVector;
  if (VecNbOfAtomsPerVolume)  delete [] VecNbOfAtomsPerVolume;
///  if (fIonisation)            delete    fIonisation;
///  if (fSandiaTable)           delete    fSandiaTable;
  
  // Remove this material from theMaterialTable.
  //
  theMaterialTable[fIndexInTable] = 0;
}

//....oooOO0OOooo........oooOO0OOooo........oooOO0OOooo........oooOO0OOooo......

const G4Material& G4Material::operator=(const G4Material& right)
{
  if (this != &right)
    {
      fName                    = right.fName;
      fChemicalFormula         = right.fChemicalFormula;
      fDensity                 = right.fDensity;
      fState                   = right.fState;
      fTemp                    = right.fTemp;
      fPressure                = right.fPressure;
           
      if (fImplicitElement)    delete    ((*theElementVector)[0]);      
      if (theElementVector)    delete    theElementVector;
      if (fMassFractionVector) delete [] fMassFractionVector;
      if (fAtomsVector)        delete [] fAtomsVector;
      
      maxNbComponents          = right.maxNbComponents;
      fNumberOfComponents      = right.fNumberOfComponents;
      fNumberOfElements        = right.fNumberOfElements;     
      fImplicitElement         = right.fImplicitElement;
      
      if (fImplicitElement) {
        G4double z = (*right.theElementVector)[0]->GetZ();
	G4double a = (*right.theElementVector)[0]->GetA();
        theElementVector          = new G4ElementVector(1,(G4Element*)0);
	(*theElementVector)[0]    = new G4Element(fName," ",z,a);
	fMassFractionVector       = new G4double[1];
	fMassFractionVector[0]    = 1.;
      } else {		
        theElementVector       = new G4ElementVector(fNumberOfElements,0);
        fMassFractionVector    = new G4double[fNumberOfElements];     
        for (size_t i=0; i<fNumberOfElements; i++) {
           (*theElementVector)[i]= (*right.theElementVector)[i];
           fMassFractionVector[i]= right.fMassFractionVector[i];
        }
      }
      
      if (right.fAtomsVector) { 
        fAtomsVector       = new G4int[fNumberOfElements];
        for (size_t i=0; i<fNumberOfElements; i++)              
           fAtomsVector[i] = right.fAtomsVector[i];
      }
      	   
      fMaterialPropertiesTable = right.fMaterialPropertiesTable;
      
      ComputeDerivedQuantities();      
    } 
  return *this;
}

//....oooOO0OOooo........oooOO0OOooo........oooOO0OOooo........oooOO0OOooo......

G4int G4Material::operator==(const G4Material& right) const
{
  return (this == (G4Material *) &right);
}

//....oooOO0OOooo........oooOO0OOooo........oooOO0OOooo........oooOO0OOooo......

G4int G4Material::operator!=(const G4Material& right) const
{
  return (this != (G4Material *) &right);
}

//....oooOO0OOooo........oooOO0OOooo........oooOO0OOooo........oooOO0OOooo......


std::ostream& operator<<(std::ostream& flux, G4Material* material)
{
  std::ios::fmtflags mode = flux.flags();
  flux.setf(std::ios::fixed,std::ios::floatfield);
  G4long prec = flux.precision(3);
  
  flux
    << " Material: "         << std::setw(8) <<  material->fName
    << " " << material->fChemicalFormula << " ";
//    << "  density: "         << std::setw(6) << std::setprecision(3)  
//    << G4BestUnit(material->fDensity,"Volumic Mass") 
//    << "  RadL: "            << std::setw(7)  << std::setprecision(3)  
//    << G4BestUnit(material->fRadlen,"Length")
//    << "  Nucl.Int.Length: " << std::setw(7)  << std::setprecision(3)  
//    << G4BestUnit(material->fNuclInterLen,"Length")    
//    << "  Imean: "           << std::setw(7)  << std::setprecision(3)  
//	<< G4BestUnit(material->GetIonisation()->GetMeanExcitationEnergy(),"Energy");
    
  if(material->fState == kStateGas)
    flux
      << "  temperature: " << std::setw(6) << std::setprecision(2)  
      << (material->fTemp)/kelvin << " K"
      << "  pressure: "    << std::setw(6) << std::setprecision(2)   
      << (material->fPressure)/atmosphere << " atm";

  for (size_t i=0; i<material->fNumberOfElements; i++)
    flux 
      << "\n   ---> " << (*(material->theElementVector))[i] 
      << "  ElmMassFraction: " << std::setw(6)<< std::setprecision(2) 
      << (material->fMassFractionVector[i])/perCent << " %" 
      << "  ElmAbundance "     << std::setw(6)<< std::setprecision(2) 
      << 100*(material->VecNbOfAtomsPerVolume[i])/(material->TotNbOfAtomsPerVolume)
      << " %";

  flux.precision(prec);    
  flux.setf(mode,std::ios::floatfield);
            
  return flux;
}

//....oooOO0OOooo........oooOO0OOooo........oooOO0OOooo........oooOO0OOooo......

 std::ostream& operator<<(std::ostream& flux, G4Material& material)
{
  flux << &material;        
  return flux;
}

//....oooOO0OOooo........oooOO0OOooo........oooOO0OOooo........oooOO0OOooo......
     
std::ostream& operator<<(std::ostream& flux, G4MaterialTable MaterialTable)
{
 //Dump info for all known materials
   flux << "\n***** Table : Nb of materials = " << MaterialTable.size() 
        << " *****\n" << G4endl;
        
   for (size_t i=0; i<MaterialTable.size(); i++) flux << MaterialTable[i] 
                                                       << G4endl << G4endl;

   return flux;
}      

//....oooOO0OOooo........oooOO0OOooo........oooOO0OOooo........oooOO0OOooo......
