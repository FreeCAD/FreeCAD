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
// $Id: G4VisAttributes.hh,v 1.19 2007/01/05 14:12:13 allison Exp $
// GEANT4 tag $Name: geant4-09-03-patch-01 $
//
// 
// John Allison  23rd October 1996

// Class Description:
// Visualization attributes are a set of information associated with the 
// visualizable objects. This information is necessary only for 
// visualization, and is not included in geometrical information such 
// as shapes, position, and orientation. 
// A typical example of a visualization attribute is "colour". 
// For example, in visualizing a box, the Visualization Manager must know 
// its colour. If an object to be visualized has not been assigned a set of 
// visualization attributes, then a proper default set is used 
// automatically. A set of visualization attributes is held by an 
// instance of class G4VisAttributes defined in the graphics_reps 
// category. The followings are commonly-used attributes:
//   - visibility
//   - visibility of daughters
//   - force wireframe style, force solid style
//   - force auxiliary edge visibility, force line segments pe circle
//   - colour 
// Class Description - End:


#ifndef __G4VISATTRIBUTES_HH__
#define __G4VISATTRIBUTES_HH__

///#include "globals.hh"
#include "../global/globals.hh"
#include <vector>
#include <map>

///#include "G4Colour.hh"
///#include "G4Color.hh"

class G4AttValue;
class G4AttDef;

class G4VisAttributes {

  friend std::ostream& operator << (std::ostream& os, const G4VisAttributes& a);

public: // With description

  enum LineStyle {unbroken, dashed, dotted};
  enum ForcedDrawingStyle {wireframe, solid};

  G4VisAttributes ();
  G4VisAttributes (G4bool visibility);
///  G4VisAttributes (const G4Colour& colour);
///  G4VisAttributes (G4bool visibility, const G4Colour& colour);

  // Compiler defaults for copy constructor and assigment operator are OK.
/*
  static const G4VisAttributes Invisible;

  static const G4VisAttributes& GetInvisible();

  G4bool operator != (const G4VisAttributes& a) const;
  G4bool operator == (const G4VisAttributes& a) const;

  void SetVisibility          (G4bool);
  void SetDaughtersInvisible  (G4bool);

  void SetColour              (const G4Colour&);
  void SetColor               (const G4Color&);
  void SetColour              (G4double red, G4double green, G4double blue,
			       G4double alpha = 1.);
  void SetColor               (G4double red, G4double green, G4double blue,
			       G4double alpha = 1.);
  void SetLineStyle           (LineStyle);
  void SetLineWidth           (G4double);
  void SetForceWireframe      (G4bool);
  void SetForceSolid          (G4bool);
  void SetForceAuxEdgeVisible (G4bool);
  void SetForceLineSegmentsPerCircle (G4int nSegments);
  // Allows choice of circle approximation.  A circle of 360 degrees
  // will be composed of nSegments line segments.  If your solid has
  // curves of D degrees that you need to divide into N segments,
  // specify nSegments = N * 360 / D.
  void SetStartTime           (G4double);
  void SetEndTime             (G4double);
  void SetAttValues           (const std::vector<G4AttValue>*);
  void SetAttDefs             (const std::map<G4String,G4AttDef>*);

  G4bool          IsVisible                      () const;
  G4bool          IsDaughtersInvisible           () const;
  const G4Colour& GetColour                      () const;
  const G4Color&  GetColor                       () const;
  LineStyle       GetLineStyle                   () const;
  G4double        GetLineWidth                   () const;
  G4bool          IsForceDrawingStyle            () const;
  ForcedDrawingStyle GetForcedDrawingStyle       () const;
  G4bool          IsForceAuxEdgeVisible          () const;
  G4bool          IsForceLineSegmentsPerCircle   () const;
  G4int           GetForcedLineSegmentsPerCircle () const;
  G4double        GetStartTime                   () const;
  G4double        GetEndTime                     () const;
  // Returns an expendable copy of the G4AttValues...
  const std::vector<G4AttValue>* CreateAttValues () const;
  // Returns the orginal long life G4AttDefs...
  const std::map<G4String,G4AttDef>* GetAttDefs  () const;

private:

  G4bool      fVisible;            // Visibility flag
  G4bool      fDaughtersInvisible; // Make daughters invsibile.
  G4Colour    fColour;
  LineStyle   fLineStyle;
  G4double    fLineWidth;          // Units of "normal" device linewidth, e.g.,
                                   // pixels for screen, 0.1 mm for paper.
  G4bool      fForceDrawingStyle;  // To switch on forced drawing style.
  ForcedDrawingStyle fForcedStyle; // Value of forced drawing style.
  G4bool    fForceAuxEdgeVisible;  // Force drawing of auxilary edges. 
  G4int fForcedLineSegmentsPerCircle;  // Forced lines segments per
				       // circle.  <=0 means not forced.
  G4double fStartTime, fEndTime;   // Time range.
  const std::vector<G4AttValue>*     fAttValues;  // For picking, etc.
  const std::map<G4String,G4AttDef>* fAttDefs;    // Corresponding definitions.
  */
};

#include "G4VisAttributes.icc"

#endif
