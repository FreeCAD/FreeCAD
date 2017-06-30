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
// $Id: G4GDMLWriteDefine.hh,v 1.14 2009/04/24 15:34:20 gcosmo Exp $
// GEANT4 tag $Name: geant4-09-03-patch-01 $
//
//
// class G4GDMLWriteDefine
//
// Class description:
//
// GDML writer class for positionings and transformations.

// History:
// - Created.                                  Zoltan Torzsok, November 2007
// -------------------------------------------------------------------------

#ifndef _G4GDMLWRITEDEFINE_INCLUDED_
#define _G4GDMLWRITEDEFINE_INCLUDED_

//#include "G4Types.hh"
#include "../global/G4Types.hh"
//#include "G4ThreeVector.hh"
//#include "G4RotationMatrix.hh"
#include "../global/G4ThreeVector.hh"
#include "../geometry/management/G4RotationMatrix.hh"

#include "G4GDMLWrite.hh"

class G4GDMLWriteDefine : public G4GDMLWrite
{

  public:

    G4ThreeVector GetAngles(const G4RotationMatrix&);
    void ScaleWrite(xercesc::DOMElement* element,
                    const G4String& name, const G4ThreeVector& scl)
         { Scale_vectorWrite(element,"scale",name,scl); }
    void RotationWrite(xercesc::DOMElement* element,
                    const G4String& name, const G4ThreeVector& rot)
         { Rotation_vectorWrite(element,"rotation",name,rot); }
    void PositionWrite(xercesc::DOMElement* element,
                    const G4String& name, const G4ThreeVector& pos)
         { Position_vectorWrite(element,"position",name,pos); }
    void FirstrotationWrite(xercesc::DOMElement* element,
                    const G4String& name, const G4ThreeVector& rot)
         { Rotation_vectorWrite(element,"firstrotation",name,rot); }
    void FirstpositionWrite(xercesc::DOMElement* element,
                    const G4String& name, const G4ThreeVector& pos)
         { Position_vectorWrite(element,"firstposition",name,pos); }
    void AddPosition(const G4String& name, const G4ThreeVector& pos)
         { Position_vectorWrite(defineElement,"position",name,pos); }

    virtual void DefineWrite(xercesc::DOMElement*);

  protected:

    G4GDMLWriteDefine();
    virtual ~G4GDMLWriteDefine();

    void Scale_vectorWrite(xercesc::DOMElement*, const G4String&,
                              const G4String&, const G4ThreeVector&);
    void Rotation_vectorWrite(xercesc::DOMElement*, const G4String&,
                              const G4String&, const G4ThreeVector&);
    void Position_vectorWrite(xercesc::DOMElement*, const G4String&,
                              const G4String&, const G4ThreeVector&);

  protected:

    static const G4double kRelativePrecision;
    static const G4double kAngularPrecision;
    static const G4double kLinearPrecision;
  
    xercesc::DOMElement* defineElement;

};

#endif
