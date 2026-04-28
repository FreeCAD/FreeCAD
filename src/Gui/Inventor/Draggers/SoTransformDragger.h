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
#include <Inventor/sensors/SoIdleSensor.h>
#include <Inventor/nodes/SoBaseColor.h>
#include <FCGlobal.h>

#include <string>

class SoCamera;

namespace Gui
{

class SoLinearDraggerContainer;

/*! @brief Coordinate System Dragger
 *
 * used to transform objects in 3d space. Set initial:
 * translation, rotation, translationIncrement and
 * rotationIncrement. Use *IncrementCount* multiplied
 * with *Increment for full double precision output.
 *
 * Dragger can be displayed in 2 modes: static scale and auto scale.
 * For static you can set the field scale and you are done.
 * For autoscale you set the field scale & call setupAutoScale with
 * the viewer camera. @see setUpAutoScale @see scale.
 */
class GuiExport SoTransformDragger: public SoDragger
{
    SO_KIT_HEADER(SoTransformDragger);
    SO_KIT_CATALOG_ENTRY_HEADER(annotation);
    SO_KIT_CATALOG_ENTRY_HEADER(scaleNode);
    SO_KIT_CATALOG_ENTRY_HEADER(pickStyle);
    // Translator
    SO_KIT_CATALOG_ENTRY_HEADER(xTranslatorDragger);
    SO_KIT_CATALOG_ENTRY_HEADER(yTranslatorDragger);
    SO_KIT_CATALOG_ENTRY_HEADER(zTranslatorDragger);
    // Planar Translator
    SO_KIT_CATALOG_ENTRY_HEADER(xyPlanarTranslatorSwitch);
    SO_KIT_CATALOG_ENTRY_HEADER(yzPlanarTranslatorSwitch);
    SO_KIT_CATALOG_ENTRY_HEADER(zxPlanarTranslatorSwitch);
    SO_KIT_CATALOG_ENTRY_HEADER(xyPlanarTranslatorSeparator);
    SO_KIT_CATALOG_ENTRY_HEADER(yzPlanarTranslatorSeparator);
    SO_KIT_CATALOG_ENTRY_HEADER(zxPlanarTranslatorSeparator);
    SO_KIT_CATALOG_ENTRY_HEADER(xyPlanarTranslatorColor);
    SO_KIT_CATALOG_ENTRY_HEADER(yzPlanarTranslatorColor);
    SO_KIT_CATALOG_ENTRY_HEADER(zxPlanarTranslatorColor);
    SO_KIT_CATALOG_ENTRY_HEADER(xyPlanarTranslatorRotation);
    SO_KIT_CATALOG_ENTRY_HEADER(yzPlanarTranslatorRotation);
    SO_KIT_CATALOG_ENTRY_HEADER(zxPlanarTranslatorRotation);
    SO_KIT_CATALOG_ENTRY_HEADER(xyPlanarTranslatorDragger);
    SO_KIT_CATALOG_ENTRY_HEADER(yzPlanarTranslatorDragger);
    SO_KIT_CATALOG_ENTRY_HEADER(zxPlanarTranslatorDragger);
    // Rotator
    SO_KIT_CATALOG_ENTRY_HEADER(xRotatorDragger);
    SO_KIT_CATALOG_ENTRY_HEADER(yRotatorDragger);
    SO_KIT_CATALOG_ENTRY_HEADER(zRotatorDragger);

public:
    static void initClass();
    SoTransformDragger();
    ~SoTransformDragger() override;

    SoSFVec3f translation;  //!< initial translation and reflects single precision movement.
    SoSFDouble translationIncrement;       //!< set from outside used for rounding.
    SoSFInt32 translationIncrementCountX;  //!< used from outside for translation x steps.
    SoSFInt32 translationIncrementCountY;  //!< used from outside for translation y steps.
    SoSFInt32 translationIncrementCountZ;  //!< used from outside for translation z steps.

    SoSFRotation rotation;         //!< initial rotation and reflects single precision movement.
    SoSFDouble rotationIncrement;  //!< radians set from outside and used for rounding.
    SoSFInt32 rotationIncrementCountX;  //!< used from outside for rotation x steps.
    SoSFInt32 rotationIncrementCountY;  //!< used from outside for rotation y steps.
    SoSFInt32 rotationIncrementCountZ;  //!< used from outside for rotation z steps.

    SoSFString xAxisLabel;  //!< label for X axis
    SoSFString yAxisLabel;  //!< label for Y axis
    SoSFString zAxisLabel;  //!< label for Z axis

    void clearIncrementCounts();  //!< used to reset after drag update.

    /*! @brief Overall scale of dragger node.
     *
     * When using autoscale mode, this represents normalized device coordinates (0.0 to 1.0). A value
     * of 0.05 is a good place to start. When NOT using autoscale mode, scale represents
     * a traditional scale and a value of 1.0 is a good place to start.
     */
    SoSFFloat draggerSize;
    SoSFFloat autoScaleResult;  //!< result of autoscale calculation and used by childdraggers.
                                //!< Don't use.

    SoIdleSensor idleSensor;  //!< might be overkill, but want to make sure of performance.
    void setUpAutoScale(SoCamera* cameraIn);  //!< used to setup the auto scaling of dragger.

    void setAxisColors(unsigned long x, unsigned long y, unsigned long z);  //!< set the axis colors.

    //! @name Visibility Functions
    //@{
    void showTranslationX();     //!< show the x translation dragger.
    void showTranslationY();     //!< show the y translation dragger.
    void showTranslationZ();     //!< show the z translation dragger.
    void hideTranslationX();     //!< hide the x translation dragger.
    void hideTranslationY();     //!< hide the y translation dragger.
    void hideTranslationZ();     //!< hide the z translation dragger.
    bool isShownTranslationX();  //!< is x translation dragger shown.
    bool isShownTranslationY();  //!< is y translation dragger shown.
    bool isShownTranslationZ();  //!< is z translation dragger shown.

    void showPlanarTranslationXY();      //!< show the xy planar translation dragger.
    void showPlanarTranslationYZ();      //!< show the yz planar translation dragger.
    void showPlanarTranslationZX();      //!< show the zx planar translation dragger.
    void hidePlanarTranslationXY();      //!< hide the xy planar translation dragger.
    void hidePlanarTranslationYZ();      //!< hide the yz planar translation dragger.
    void hidePlanarTranslationZX();      //!< hide the zx planar translation dragger.
    bool isShownPlanarTranslationXY();   //!< is xy planar translation dragger shown.
    bool isShownPlanarTranslationYZ();   //!< is yz planar translation dragger shown.
    bool isShownPlanarTranslationZX();   //!< is zx planar translation dragger shown.
    bool isHiddenPlanarTranslationXY();  //!< is xy planar translation dragger hidden.
    bool isHiddenPlanarTranslationYZ();  //!< is yz planar translation dragger hidden.
    bool isHiddenPlanarTranslationZX();  //!< is zx planar translation dragger hidden.

    void showRotationX();     //!< show the x rotation dragger.
    void showRotationY();     //!< show the y rotation dragger.
    void showRotationZ();     //!< show the z rotation dragger.
    void hideRotationX();     //!< hide the x rotation dragger.
    void hideRotationY();     //!< hide the y rotation dragger.
    void hideRotationZ();     //!< hide the z rotation dragger.
    bool isShownRotationX();  //!< is x rotation dragger shown.
    bool isShownRotationY();  //!< is x rotation dragger shown.
    bool isShownRotationZ();  //!< is x rotation dragger shown.
    //@}

    void GLRender(SoGLRenderAction* action) override;

protected:
    SbBool setUpConnections(SbBool onoff, SbBool doitalways = FALSE) override;
    void handleEvent(SoHandleEventAction* action) override;

    static void translationSensorCB(void* f, SoSensor*);
    static void rotationSensorCB(void* f, SoSensor*);
    static void valueChangedCB(void*, SoDragger* d);
    static void cameraCB(void* data, SoSensor*);
    static void idleCB(void* data, SoSensor*);  //!< scheduled from cameraCB to auto scale dragger.
    static void finishDragCB(void* data, SoDragger*);


    SoFieldSensor translationSensor;
    SoFieldSensor rotationSensor;
    SoFieldSensor cameraSensor;

private:
    // Used to compensate for axis scale in world transformation when doing
    // auto scale.
    SbVec3f axisScale;

    bool scaleInited {false};

    void updateAxisScale();

    void setupTranslationDraggers();
    void setupTranslationDragger(
        const std::string& name,
        SoSFString* label,
        SoSFInt32& incrementCount,
        const SbVec3f& rotDir
    );
    void setupRotationDraggers();
    void setupRotationDragger(const std::string& name, SoSFInt32& incrementCount);

    using inherited = SoDragger;
};

}  // namespace Gui
