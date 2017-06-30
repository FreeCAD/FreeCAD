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
// $Id: G4GDMLRead.cc,v 1.47 2009/05/12 15:46:43 gcosmo Exp $
// GEANT4 tag $Name: geant4-09-03-patch-01 $
//
// class G4GDMLRead Implementation
//
// History:
// - Created.                                  Zoltan Torzsok, November 2007
// -------------------------------------------------------------------------


#include "../../PreCompiled.h"
#include <Base/Console.h>




///#include "globals.hh"
#include "../global/globals.hh"

#include "G4GDMLRead.hh"
/*
#include "G4UnitsTable.hh"
#include "G4Element.hh"
#include "G4Material.hh"
*/
///#include "G4SolidStore.hh"
///#include "G4LogicalVolumeStore.hh"
///#include "G4PhysicalVolumeStore.hh"
#include "../geometry/management/G4SolidStore.hh"
#include "../geometry/management/G4LogicalVolumeStore.hh"
#include "../geometry/management/G4PhysicalVolumeStore.hh"



G4GDMLRead::G4GDMLRead()
  : validate(true), check(false), inLoop(0), loopCount(0)
{
///   G4UnitDefinition::BuildUnitsTable();
}

G4GDMLRead::~G4GDMLRead()
{
}

G4String G4GDMLRead::Transcode(const XMLCh* const toTranscode)
{
   char* char_str = xercesc::XMLString::transcode(toTranscode);
   G4String my_str(char_str);
   xercesc::XMLString::release(&char_str);
   return my_str;
}

void G4GDMLRead::OverlapCheck(G4bool flag)
{
   check = flag;
}

G4String G4GDMLRead::GenerateName(const G4String& nameIn, G4bool strip)
{
   G4String nameOut(nameIn);

   if (inLoop>0)
   {
///     nameOut = eval.SolveBrackets(nameOut);
//     std::stringstream stream;
//     stream << "0x" << loopCount;
//     nameOut = nameOut + stream.str();
   }
   if (strip) { StripName(nameOut); }

   return nameOut;
}

void G4GDMLRead::GeneratePhysvolName(const G4String& nameIn,
                                     G4VPhysicalVolume* physvol)
{/**/
   G4String nameOut(nameIn);

   if (nameIn.empty())
   {
     std::stringstream stream;
     stream << physvol->GetLogicalVolume()->GetName() << "_PV";
     nameOut = stream.str();
   }
   nameOut = eval.SolveBrackets(nameOut);

   physvol->SetName(nameOut);/**/
}

G4String G4GDMLRead::Strip(const G4String& name) const
{
  G4String sname(name);
  return sname.remove(sname.find("0x"));
}

void G4GDMLRead::StripName(G4String& name) const
{
  name.remove(name.find("0x"));
}

void G4GDMLRead::StripNames() const
{/*
  // Strips off names of volumes, solids elements and materials from possible
  // reference pointers or IDs attached to their original identifiers.

  G4PhysicalVolumeStore* pvols = G4PhysicalVolumeStore::GetInstance();
  G4LogicalVolumeStore* lvols = G4LogicalVolumeStore::GetInstance();
  G4SolidStore* solids = G4SolidStore::GetInstance();
  const G4ElementTable* elements = G4Element::GetElementTable();
  const G4MaterialTable* materials = G4Material::GetMaterialTable();

  G4cout << "Stripping off GDML names of materials, solids and volumes ..."
         << G4endl;

  G4String sname;
  register size_t i;

  // Solids...
  //
  for (i=0; i<solids->size(); i++)
  {
    G4VSolid* psol = (*solids)[i];
    sname = psol->GetName();
    StripName(sname);
    psol->SetName(sname);
  }

  // Logical volumes...
  //
  for (i=0; i<lvols->size(); i++)
  {
    G4LogicalVolume* lvol = (*lvols)[i];
    sname = lvol->GetName();
    StripName(sname);
    lvol->SetName(sname);
  }

  // Physical volumes...
  //
  for (i=0; i<pvols->size(); i++)
  {
    G4VPhysicalVolume* pvol = (*pvols)[i];
    sname = pvol->GetName();
    StripName(sname);
    pvol->SetName(sname);
  }

  // Materials...
  //
  for (i=0; i<materials->size(); i++)
  {
    G4Material* pmat = (*materials)[i];
    sname = pmat->GetName();
    StripName(sname);
    pmat->SetName(sname);
  }

  // Elements...
  //
  for (i=0; i<elements->size(); i++)
  {
    G4Element* pelm = (*elements)[i];
    sname = pelm->GetName();
    StripName(sname);
    pelm->SetName(sname);
  }*/
}

void G4GDMLRead::LoopRead(const xercesc::DOMElement* const element,
     void(G4GDMLRead::*func)(const xercesc::DOMElement* const))
{/*
   G4String var;
   G4String from;
   G4String to;
   G4String step;

   const xercesc::DOMNamedNodeMap* const attributes = element->getAttributes();
   XMLSize_t attributeCount = attributes->getLength();

   for (XMLSize_t attribute_index=0;
        attribute_index<attributeCount;attribute_index++)
   {
      xercesc::DOMNode* attribute_node = attributes->item(attribute_index);

      if (attribute_node->getNodeType() != xercesc::DOMNode::ATTRIBUTE_NODE)
      { continue; }

      const xercesc::DOMAttr* const attribute
            = dynamic_cast<xercesc::DOMAttr*>(attribute_node);   
      const G4String attribute_name = Transcode(attribute->getName());
      const G4String attribute_value = Transcode(attribute->getValue());

      if (attribute_name=="for")  { var = attribute_value; }  else
      if (attribute_name=="from") { from = attribute_value; } else
      if (attribute_name=="to")   { to = attribute_value; }   else
      if (attribute_name=="step") { step = attribute_value; }
   }

   if (var.empty())
   {
     G4Exception("G4GDMLRead::loopRead()", "InvalidRead",
                 FatalException, "No variable is determined for loop!");
   }

   if (!eval.IsVariable(var))
   {
     G4Exception("G4GDMLRead::loopRead()", "InvalidRead",
                 FatalException, "Variable is not defined in loop!");
   }

   G4int _var = eval.EvaluateInteger(var);
   G4int _from = eval.EvaluateInteger(from);
   G4int _to = eval.EvaluateInteger(to);
   G4int _step = eval.EvaluateInteger(step);
   
   if (!from.empty()) { _var = _from; }

   if (_from == _to)
   {
     G4Exception("G4GDMLRead::loopRead()", "InvalidRead",
                 FatalException, "Empty loop!");
   }
   if ((_from < _to) && (_step <= 0))
   {
     G4Exception("G4GDMLRead::loopRead()", "InvalidRead",
                 FatalException, "Infinite loop!");
   }
   if ((_from > _to) && (_step >= 0))
   {
     G4Exception("G4GDMLRead::loopRead()", "InvalidRead",
                 FatalException, "Infinite loop!");
   }

   inLoop++;

   while (_var <= _to)
   {
      eval.SetVariable(var,_var);
      (this->*func)(element);
      _var += _step;
      loopCount++;
   }

   inLoop--;
   if (!inLoop) { loopCount = 0; }*/
}

void G4GDMLRead::ExtensionRead(const xercesc::DOMElement* const)
{/*
   G4String error_msg = "No handle to user-code for parsing extensions!";
   G4Exception("G4GDMLRead::ExtensionRead()",
               "NotImplemented", JustWarning, error_msg);*/
}

void G4GDMLRead::Read(const G4String& fileName,
                            G4bool validation,
                            G4bool isModule)
{
	
/*
   if (isModule)
   {
      G4cout << "G4GDML: Reading module '" << fileName << "'..." << G4endl;
   }
   else
   {
      G4cout << "G4GDML: Reading '" << fileName << "'..." << G4endl;
   }
*/
   inLoop = 0;
   validate = validation;

///   xercesc::ErrorHandler* handler = new G4GDMLErrorHandler(!validate);
   xercesc::XercesDOMParser* parser = new xercesc::XercesDOMParser;

   parser->setValidationScheme(xercesc::XercesDOMParser::Val_Always);
   parser->setValidationSchemaFullChecking(true);
   parser->setCreateEntityReferenceNodes(false); 
     // Entities will be automatically resolved by Xerces

   parser->setDoNamespaces(true);
   parser->setDoSchema(true);
///   parser->setErrorHandler(handler);
	//	 develString=G4String("OK");

   try { parser->parse(fileName.c_str()); }
   catch (const xercesc::XMLException &e)
     { 
		// G4cout << "G4GDML: " << Transcode(e.getMessage()) << G4endl;
		 Base::Console().Message(Transcode(e.getMessage()).c_str());
   }
   catch (const xercesc::DOMException &e)
     { 
		 //G4cout << "G4GDML: " << Transcode(e.getMessage()) << G4endl; 
		 Base::Console().Message(Transcode(e.getMessage()).c_str());
   }

   xercesc::DOMDocument* doc = parser->getDocument();

   if (!doc)
   {
     G4String error_msg = "Unable to open document: " + fileName;
	 Base::Console().Message((G4String("Unable to open document: ")+error_msg).c_str());
   ///  G4Exception("G4GDMLRead::Read()", "InvalidRead",
      ///           FatalException, error_msg);
   }
   xercesc::DOMElement* element = doc->getDocumentElement();
/**/
   if (!element)
   {
	 Base::Console().Message("Empty document!");
    /// G4Exception("G4GDMLRead::Read()", "InvalidRead",
       ///          FatalException, "Empty document!");
   }

   for (xercesc::DOMNode* iter = element->getFirstChild();
        iter != 0; iter = iter->getNextSibling())
   {
 

	   if (iter->getNodeType() != xercesc::DOMNode::ELEMENT_NODE)  { continue; }

      const xercesc::DOMElement* const child
           = dynamic_cast<xercesc::DOMElement*>(iter);
 //
	  const G4String tag = Transcode(child->getTagName());

      if (tag=="define")    { DefineRead(child);    } else
      ///if (tag=="materials") { MaterialsRead(child); } else
	  if (tag=="solids")    { 	SolidsRead(child);   } else
      if (tag=="setup")     { SetupRead(child);     } else
      if (tag=="structure") { StructureRead(child); } ///else
      ///if (tag=="extension") { ExtensionRead(child); }
      ///else
      ///{
        ///G4String error_msg = "Unknown tag in gdml: " + tag;
        ///G4Exception("G4GDMLRead::Read()", "InvalidRead",
           ///         FatalException, error_msg);
      ///}
   }

   if (parser)  { delete parser;  }
/*
///   if (handler) { delete handler; }

   if (isModule)
   {
     /// G4cout << "G4GDML: Reading module '" << fileName << "' done!" << G4endl;
   }
   else
   {
    ///  G4cout << "G4GDML: Reading '" << fileName << "' done!" << G4endl;
     /// StripNames();
   }*/
}
