/***************************************************************************
 *   Copyright (c) 2005 Jürgen Riegel <juergen.riegel@web.de>              *
 *                                                                         *
 *   This file is part of the FreeCAD CAx development system.              *
 *                                                                         *
 *   This library is free software; you can redistribute it and/or         *
 *   modify it under the terms of the GNU Library General Public           *
 *   License as published by the Free Software Foundation; either          *
 *   version 2 of the License, or (at your option) any later version.      *
 *                                                                         *
 *   This library  is distributed in the hope that it will be useful,      *
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of        *
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the         *
 *   GNU Library General Public License for more details.                  *
 *                                                                         *
 *   You should have received a copy of the GNU Library General Public     *
 *   License along with this library; see the file COPYING.LIB. If not,    *
 *   write to the Free Software Foundation, Inc., 59 Temple Place,         *
 *   Suite 330, Boston, MA  02111-1307, USA                                *
 *                                                                         *
 ***************************************************************************/

#ifndef SONAVIGATIONDRAGGER_H
#define SONAVIGATIONDRAGGER_H

//  Geometry resources and part names for this dragger:

//  Resource Names:                     Part Names:
// rotTransTranslatorTranslator      translator.translator
// rotTransTranslatorTranslatorActive
//                                   translator.translatorActive
// rotTransTranslatorFeedback        translator.feedback
// rotTransTranslatorFeedbackActive  translator.feedbackActive

// rotTransRotatorRotator             XRotator.rotator
// rotTransRotatorRotatorActive       XRotator.rotatorActive
// rotTransRotatorFeedback            XRotator.feedback
// rotTransRotatorFeedbackActive      XRotator.feedbackActive
// (and similarly for parts of the YRotator and ZRotator)

#include <Inventor/draggers/SoDragger.h>
#include <Inventor/fields/SoSFRotation.h>
#include <Inventor/fields/SoSFVec3f.h>
#include <Inventor/sensors/SoFieldSensor.h>


//class TranslateRadialDragger;
class SoRotateCylindricalDragger;

class RotTransDragger : public SoDragger
{
   SO_KIT_HEADER(RotTransDragger);

   // Makes the dragger surround other objects
   SO_KIT_CATALOG_ENTRY_HEADER(surroundScale);
// Keeps the dragger evenly sized in all 3 dimensions
   SO_KIT_CATALOG_ENTRY_HEADER(antiSquish);

   // The translating dragger...
   SO_KIT_CATALOG_ENTRY_HEADER(translator);

   // The X and Z rotators need to be turned so as to orient
   // correctly. So create a separator part and put an
   // SoRotation node and the dragger underneath.
   SO_KIT_CATALOG_ENTRY_HEADER(XRotatorSep);
   SO_KIT_CATALOG_ENTRY_HEADER(XRotatorRot);
   SO_KIT_CATALOG_ENTRY_HEADER(XRotator);

   SO_KIT_CATALOG_ENTRY_HEADER(YRotator);

   SO_KIT_CATALOG_ENTRY_HEADER(ZRotatorSep);
   SO_KIT_CATALOG_ENTRY_HEADER(ZRotatorRot);
   SO_KIT_CATALOG_ENTRY_HEADER(ZRotator);

  public:

   // Constructor
   RotTransDragger();

   // These fields reflect state of the dragger at all times.
   SoSFRotation rotation;
   SoSFVec3f   translation;

   // This should be called once after SoInteraction::init().
   static void initClass();

  protected:

   // These sensors ensure that the motionMatrix is updated
   // when the fields are changed from outside.
   SoFieldSensor *rotFieldSensor;
   SoFieldSensor *translFieldSensor;
   static void fieldSensorCB(void *, SoSensor *);

   // This function is invoked by the child draggers when they
   // change their value.
   static void valueChangedCB(void *, SoDragger *);

   // Called at the beginning and end of each dragging motion.
   // Tells the "surroundScale" part to recalculate.
   static void invalidateSurroundScaleCB(void *, SoDragger *);

   // This will detach/attach the fieldSensor.
   // It is called at the end of the constructor (to attach).
   // and at the start/end of SoBaseKit::readInstance()
   // and on the new copy at the start/end of SoBaseKit::copy()
   // Returns the state of the node when this was called.
   SbBool setUpConnections( SbBool onOff,
                        SbBool doItAlways = false) override;

   // This allows us to specify that certain parts do not
   // write out. We'll use this on the antiSquish and
   // surroundScale parts.
   void setDefaultOnNonWritingFields() override;

  private:

   static const char NavigationDraggerLayout[];

   // Destructor.
   ~RotTransDragger() override;
};

#endif //SONAVIGATIONDRAGGER_H

