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
// $Id: G4GDMLReadSetup.cc,v 1.10 2009/03/24 15:47:33 gcosmo Exp $
// GEANT4 tag $Name: geant4-09-03-patch-01 $
//
// class G4GDMLReadSetup Implementation
//
// Original author: Zoltan Torzsok, November 2007
//
// --------------------------------------------------------------------

#include "G4GDMLReadSetup.hh"

G4GDMLReadSetup::G4GDMLReadSetup() : G4GDMLReadSolids()
{
}

G4GDMLReadSetup::~G4GDMLReadSetup()
{
}

G4String G4GDMLReadSetup::GetSetup(const G4String& ref)
{
   if (setupMap.size() == 1)     // If there is only one setup defined,
   {                             // no matter how it is named
     return setupMap.begin()->second;
   }

   if (setupMap.find(ref) == setupMap.end())
   {
/*     G4String error_msg = "Referenced setup '" + ref + "' was not found!";
     G4Exception("G4GDMLReadSetup::getSetup()", "ReadError",
                 FatalException, error_msg);*/
   }

   return setupMap[ref];
}

void G4GDMLReadSetup::SetupRead(const xercesc::DOMElement* const element)
{
///   G4cout << "G4GDML: Reading setup..." << G4endl;

   G4String name;

   const xercesc::DOMNamedNodeMap* const attributes = element->getAttributes();
   XMLSize_t attributeCount = attributes->getLength();

   for (XMLSize_t attribute_index=0;
        attribute_index<attributeCount; attribute_index++)
   {
      xercesc::DOMNode* attribute_node = attributes->item(attribute_index);

      if (attribute_node->getNodeType() != xercesc::DOMNode::ATTRIBUTE_NODE)
        { continue; }

      const xercesc::DOMAttr* const attribute
            = dynamic_cast<xercesc::DOMAttr*>(attribute_node);   
      const G4String attName  = Transcode(attribute->getName());
      const G4String attValue = Transcode(attribute->getValue());

      if (attName=="name")  { name = attValue; }
   }

   for (xercesc::DOMNode* iter = element->getFirstChild();
        iter != 0; iter = iter->getNextSibling())
   {
      if (iter->getNodeType() != xercesc::DOMNode::ELEMENT_NODE)  { continue; }

      const xercesc::DOMElement* const child
            = dynamic_cast<xercesc::DOMElement*>(iter);
      const G4String tag = Transcode(child->getTagName());

      if (tag == "world") { setupMap[name] = GenerateName(RefRead(child)); }
   }
}
