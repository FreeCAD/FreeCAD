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
// $Id: G4GDMLParser.cc,v 1.14 2009/04/15 13:29:30 gcosmo Exp $
// GEANT4 tag $Name: geant4-09-03-patch-01 $
//
//
// class G4GDMLParser Implementation
//
// -------------------------------------------------------------------------

#include "G4GDMLParser.hh"

G4GDMLParser::G4GDMLParser()
  : urcode(false), uwcode(false)
{
  reader = new G4GDMLReadStructure;
  writer = new G4GDMLWriteStructure;
  xercesc::XMLPlatformUtils::Initialize();
}
/*
G4GDMLParser::G4GDMLParser(G4GDMLReadStructure* extr)
  : urcode(true), uwcode(false)
{
  reader = extr;
  writer = new G4GDMLWriteStructure;
  xercesc::XMLPlatformUtils::Initialize();
}
*/
/*
G4GDMLParser::G4GDMLParser(G4GDMLReadStructure* extr,
                           G4GDMLWriteStructure* extw)
  : urcode(true), uwcode(true)
{
  reader = extr;
  writer = extw;
  xercesc::XMLPlatformUtils::Initialize();
}
*/
G4GDMLParser::~G4GDMLParser()
{
  xercesc::XMLPlatformUtils::Terminate();
  if (!urcode) { delete reader; }
  if (!uwcode) { delete writer; }
}
