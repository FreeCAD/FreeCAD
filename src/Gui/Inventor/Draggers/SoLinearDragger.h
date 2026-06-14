// SPDX-License-Identifier: LGPL-2.1-or-later

/***************************************************************************
 *   Copyright (c) 2015 Thomas Anderson <blobfish[at]gmx.com>              *
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

#pragma once

#include <Inventor/draggers/SoDragger.h>
#include <Inventor/fields/SoSFColor.h>
#include <Inventor/fields/SoSFDouble.h>
#include <Inventor/fields/SoSFFloat.h>
#include <Inventor/fields/SoSFInt32.h>
#include <Inventor/fields/SoSFRotation.h>
#include <Inventor/fields/SoSFString.h>
#include <Inventor/fields/SoSFVec3f.h>
#include <Inventor/projectors/SbLineProjector.h>

#include <FCGlobal.h>

class SoBaseColor;
class SoTransform;

namespace Gui
{
/*! @brief Translation Dragger.
 *
 * used for translating along axis. Set the
 * translationIncrement to desired step. Use
 * 'translationIncrementCount' multiplied with
 * 'translationIncrement' for a full double
 * precision vector scalar.
 */
class GuiExport SoLinearDragger: public SoDragger
{
    SO_KIT_HEADER(SoLinearDragger);
    SO_KIT_CATALOG_ENTRY_HEADER(baseGeomSwitch);
    SO_KIT_CATALOG_ENTRY_HEADER(baseGeom);
    SO_KIT_CATALOG_ENTRY_HEADER(baseColor);
    SO_KIT_CATALOG_ENTRY_HEADER(activeSwitch);
    SO_KIT_CATALOG_ENTRY_HEADER(secondaryColor);
    SO_KIT_CATALOG_ENTRY_HEADER(labelSwitch);
    SO_KIT_CATALOG_ENTRY_HEADER(labelSeparator);
    SO_KIT_CATALOG_ENTRY_HEADER(scale);
    SO_KIT_CATALOG_ENTRY_HEADER(arrow);

public:
    static void initClass();
    SoLinearDragger();

    SoSFString label;       //!< set from outside and used to label
    SoSFVec3f translation;  //!< set from outside and used from outside for single precision.
    SoSFDouble translationIncrement;      //!< set from outside and used for rounding.
    SoSFInt32 translationIncrementCount;  //!< number of steps. used from outside.
    SoSFFloat autoScaleResult;            //!< set from parent dragger.
    SoSFColor color;                      //!< colour of the dragger
    SoSFColor activeColor;                //!< colour of the dragger while being dragged
    SoSFBool labelVisible;                //!< controls the visibility of the dragger label
    SoSFVec3f geometryScale;              //!< the scale of the dragger geometry
    SoSFBool active;                      //!< set when the dragger is being dragged
    SoSFBool baseGeomVisible;             //!< toggles if the dragger has a base geometry or not

    void instantiateBaseGeometry();

protected:
    ~SoLinearDragger() override;
    SbBool setUpConnections(SbBool onoff, SbBool doitalways = FALSE) override;

    static void startCB(void*, SoDragger* d);
    static void motionCB(void*, SoDragger* d);
    static void finishCB(void*, SoDragger* d);
    static void fieldSensorCB(void* f, SoSensor*);
    static void valueChangedCB(void*, SoDragger* d);

    void dragStart();
    void drag();
    void dragFinish();

    SoFieldSensor fieldSensor;
    SbLineProjector projector;

private:
    SbVec3f roundTranslation(const SbVec3f& vecIn, float incrementIn);

    SoSeparator* buildLabelGeometry();
    SoBaseColor* buildActiveColor();
    SoBaseColor* buildColor();

    using inherited = SoDragger;
};

class GuiExport SoLinearDraggerContainer: public SoInteractionKit
{
    SO_KIT_HEADER(SoLinearDraggerContainer);
    SO_KIT_CATALOG_ENTRY_HEADER(draggerSwitch);
    SO_KIT_CATALOG_ENTRY_HEADER(transform);
    SO_KIT_CATALOG_ENTRY_HEADER(dragger);

public:
    static void initClass();
    SoLinearDraggerContainer();

    SoSFRotation rotation;
    SoSFColor color;
    SoSFVec3f translation;
    SoSFBool visible;

    SbVec3f getPointerDirection();
    void setPointerDirection(const SbVec3f& dir);

    SoLinearDragger* getDragger();

private:
    SoTransform* buildTransform();

    using inherited = SoInteractionKit;
};

}  // namespace Gui
