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

#ifndef CSYSDRAGGER_H
#define CSYSDRAGGER_H

#include <Inventor/draggers/SoDragger.h>
#include <Inventor/fields/SoSFColor.h>
#include <Inventor/fields/SoSFDouble.h>
#include <Inventor/fields/SoSFFloat.h>
#include <Inventor/fields/SoSFInt32.h>
#include <Inventor/fields/SoSFRotation.h>
#include <Inventor/projectors/SbLineProjector.h>
#include <Inventor/projectors/SbPlaneProjector.h>
#include <Inventor/sensors/SoFieldSensor.h>
#include <Inventor/sensors/SoIdleSensor.h>
#include <FCGlobal.h>


class SoCamera;

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
class TDragger : public SoDragger
{
    SO_KIT_HEADER(TDragger);
    SO_KIT_CATALOG_ENTRY_HEADER(translatorSwitch);
    SO_KIT_CATALOG_ENTRY_HEADER(translator);
    SO_KIT_CATALOG_ENTRY_HEADER(translatorActive);
public:
    static void initClass();
    TDragger();
    SoSFVec3f translation; //!< set from outside and used from outside for single precision.
    SoSFDouble translationIncrement; //!< set from outside and used for rounding.
    SoSFInt32 translationIncrementCount; //!< number of steps. used from outside.
    SoSFFloat autoScaleResult; //!< set from parent dragger.

protected:
    ~TDragger() override;
    SbBool setUpConnections(SbBool onoff, SbBool doitalways = FALSE) override;

    static void startCB(void *, SoDragger * d);
    static void motionCB(void *, SoDragger * d);
    static void finishCB(void *, SoDragger * d);
    static void fieldSensorCB(void *f, SoSensor *);
    static void valueChangedCB(void *, SoDragger *d);

    void dragStart();
    void drag();
    void dragFinish();

    SoFieldSensor fieldSensor;
    SbLineProjector projector;

private:
    void buildFirstInstance();
    SbVec3f roundTranslation(const SbVec3f &vecIn, float incrementIn);
    SoGroup* buildGeometry();
    using inherited = SoDragger;
};

/*! @brief Planar Translation Dragger.
 *
 * used for translating on a plane. Set the
 * translationIncrement to desired step. Use
 * 'translationIncrementXCount' or
 * 'translationIncrementYCount' multiplied with
 * 'translationIncrement' for a full double
 * precision vector scalar.
 *
 * @author qewer33
 */
class TPlanarDragger : public SoDragger
{
    SO_KIT_HEADER(TDragger);
    SO_KIT_CATALOG_ENTRY_HEADER(planarTranslatorSwitch);
    SO_KIT_CATALOG_ENTRY_HEADER(planarTranslator);
    SO_KIT_CATALOG_ENTRY_HEADER(planarTranslatorActive);
public:
    static void initClass();
    TPlanarDragger();
    SoSFVec3f translation; //!< set from outside and used from outside for single precision.
    SoSFDouble translationIncrement; //!< set from outside and used for rounding.
    SoSFInt32 translationIncrementXCount; //!< number of steps. used from outside.
    SoSFInt32 translationIncrementYCount; //!< number of steps. used from outside.
    SoSFFloat autoScaleResult; //!< set from parent dragger.

protected:
    ~TPlanarDragger() override;
    SbBool setUpConnections(SbBool onoff, SbBool doitalways = FALSE) override;

    static void startCB(void *, SoDragger * d);
    static void motionCB(void *, SoDragger * d);
    static void finishCB(void *, SoDragger * d);
    static void fieldSensorCB(void *f, SoSensor *);
    static void valueChangedCB(void *, SoDragger *d);

    void dragStart();
    void drag();
    void dragFinish();

    SoFieldSensor fieldSensor;
    SbPlaneProjector projector;

private:
    void buildFirstInstance();
    SbVec3f roundTranslation(const SbVec3f &vecIn, float incrementIn);
    SoGroup* buildGeometry();
    using inherited = SoDragger;
};

/*! @brief Rotation Dragger.
 *
 * used for rotating around an axis. Set the rotation
 * increment to desired step. Use rotationIncrementCount
 * multiplied with rotationIncrement for full double
 * precision vector scalar.
 */
class RDragger : public SoDragger
{
    SO_KIT_HEADER(RDragger);
    SO_KIT_CATALOG_ENTRY_HEADER(rotatorSwitch);
    SO_KIT_CATALOG_ENTRY_HEADER(rotator);
    SO_KIT_CATALOG_ENTRY_HEADER(rotatorActive);
public:
    static void initClass();
    RDragger();
    SoSFRotation rotation; //!< set from outside and used from outside for single precision.
    SoSFDouble rotationIncrement; //!< set from outside and used for rounding.
    SoSFInt32 rotationIncrementCount; //!< number of steps. used from outside.
    SoSFColor color; //!< set from outside. non-active color.

protected:
    ~RDragger() override;
    SbBool setUpConnections(SbBool onoff, SbBool doitalways = FALSE) override;

    static void startCB(void *, SoDragger * d);
    static void motionCB(void *, SoDragger * d);
    static void finishCB(void *, SoDragger * d);
    static void fieldSensorCB(void *f, SoSensor *);
    static void valueChangedCB(void *, SoDragger *d);

    void dragStart();
    void drag();
    void dragFinish();

    SoFieldSensor fieldSensor;
    SbPlaneProjector projector;
    float arcRadius;

private:
    void buildFirstInstance();
    int roundIncrement(const float &radiansIn);
    SoGroup* buildGeometry();
    using inherited = SoDragger;
};

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
class GuiExport SoFCCSysDragger : public SoDragger
{
    SO_KIT_HEADER(SoFCCSysDragger);
    SO_KIT_CATALOG_ENTRY_HEADER(annotation);
    SO_KIT_CATALOG_ENTRY_HEADER(scaleNode);
    // Translator
    SO_KIT_CATALOG_ENTRY_HEADER(xTranslatorSwitch);
    SO_KIT_CATALOG_ENTRY_HEADER(yTranslatorSwitch);
    SO_KIT_CATALOG_ENTRY_HEADER(zTranslatorSwitch);
    SO_KIT_CATALOG_ENTRY_HEADER(xTranslatorSeparator);
    SO_KIT_CATALOG_ENTRY_HEADER(yTranslatorSeparator);
    SO_KIT_CATALOG_ENTRY_HEADER(zTranslatorSeparator);
    SO_KIT_CATALOG_ENTRY_HEADER(xTranslatorColor);
    SO_KIT_CATALOG_ENTRY_HEADER(yTranslatorColor);
    SO_KIT_CATALOG_ENTRY_HEADER(zTranslatorColor);
    SO_KIT_CATALOG_ENTRY_HEADER(xTranslatorRotation);
    SO_KIT_CATALOG_ENTRY_HEADER(yTranslatorRotation);
    SO_KIT_CATALOG_ENTRY_HEADER(zTranslatorRotation);
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
    SO_KIT_CATALOG_ENTRY_HEADER(xRotatorSwitch);
    SO_KIT_CATALOG_ENTRY_HEADER(yRotatorSwitch);
    SO_KIT_CATALOG_ENTRY_HEADER(zRotatorSwitch);
    SO_KIT_CATALOG_ENTRY_HEADER(xRotatorSeparator);
    SO_KIT_CATALOG_ENTRY_HEADER(yRotatorSeparator);
    SO_KIT_CATALOG_ENTRY_HEADER(zRotatorSeparator);
    SO_KIT_CATALOG_ENTRY_HEADER(xRotatorColor);
    SO_KIT_CATALOG_ENTRY_HEADER(yRotatorColor);
    SO_KIT_CATALOG_ENTRY_HEADER(zRotatorColor);
    SO_KIT_CATALOG_ENTRY_HEADER(xRotatorRotation);
    SO_KIT_CATALOG_ENTRY_HEADER(yRotatorRotation);
    SO_KIT_CATALOG_ENTRY_HEADER(zRotatorRotation);
    SO_KIT_CATALOG_ENTRY_HEADER(xRotatorDragger);
    SO_KIT_CATALOG_ENTRY_HEADER(yRotatorDragger);
    SO_KIT_CATALOG_ENTRY_HEADER(zRotatorDragger);
public:
    static void initClass();
    SoFCCSysDragger();
    ~SoFCCSysDragger() override;

    SoSFVec3f translation; //!< initial translation and reflects single precision movement.
    SoSFDouble translationIncrement; //!< set from outside used for rounding.
    SoSFInt32 translationIncrementCountX; //!< used from outside for translation x steps.
    SoSFInt32 translationIncrementCountY; //!< used from outside for translation y steps.
    SoSFInt32 translationIncrementCountZ; //!< used from outside for translation z steps.

    SoSFRotation rotation; //!< initial rotation and reflects single precision movement.
    SoSFDouble rotationIncrement; //!< radians set from outside and used for rounding.
    SoSFInt32 rotationIncrementCountX; //!< used from outside for rotation x steps.
    SoSFInt32 rotationIncrementCountY; //!< used from outside for rotation y steps.
    SoSFInt32 rotationIncrementCountZ; //!< used from outside for rotation z steps.

    void clearIncrementCounts(); //!< used to reset after drag update.

    /*! @brief Overall scale of dragger node.
     *
     * When using autoscale mode, this represents normalized device coordinates (0.0 to 1.0). A value
     * of 0.05 is a good place to start. When NOT using autoscale mode, scale represents
     * a traditional scale and a value of 1.0 is a good place to start.
     */
    SoSFFloat draggerSize;
    SoSFFloat autoScaleResult; //!< result of autoscale calculation and used by childdraggers. Don't use.

    SoIdleSensor idleSensor; //!< might be overkill, but want to make sure of performance.
    void setUpAutoScale(SoCamera *cameraIn); //!< used to setup the auto scaling of dragger.

    void setAxisColors(unsigned long x, unsigned long y, unsigned long z); //!< set the axis colors.

    //! @name Visibility Functions
    //@{
    void showTranslationX(); //!< show the x translation dragger.
    void showTranslationY(); //!< show the y translation dragger.
    void showTranslationZ(); //!< show the z translation dragger.
    void hideTranslationX(); //!< hide the x translation dragger.
    void hideTranslationY(); //!< hide the y translation dragger.
    void hideTranslationZ(); //!< hide the z translation dragger.
    bool isShownTranslationX(); //!< is x translation dragger shown.
    bool isShownTranslationY(); //!< is y translation dragger shown.
    bool isShownTranslationZ(); //!< is z translation dragger shown.
    bool isHiddenTranslationX(); //!< is x translation dragger hidden.
    bool isHiddenTranslationY(); //!< is y translation dragger hidden.
    bool isHiddenTranslationZ(); //!< is z translation dragger hidden.

    void showPlanarTranslationXY(); //!< show the xy planar translation dragger.
    void showPlanarTranslationYZ(); //!< show the yz planar translation dragger.
    void showPlanarTranslationZX(); //!< show the zx planar translation dragger.
    void hidePlanarTranslationXY(); //!< hide the xy planar translation dragger.
    void hidePlanarTranslationYZ(); //!< hide the yz planar translation dragger.
    void hidePlanarTranslationZX(); //!< hide the zx planar translation dragger.
    bool isShownPlanarTranslationXY(); //!< is xy planar translation dragger shown.
    bool isShownPlanarTranslationYZ(); //!< is yz planar translation dragger shown.
    bool isShownPlanarTranslationZX(); //!< is zx planar translation dragger shown.
    bool isHiddenPlanarTranslationXY(); //!< is xy planar translation dragger hidden.
    bool isHiddenPlanarTranslationYZ(); //!< is yz planar translation dragger hidden.
    bool isHiddenPlanarTranslationZX(); //!< is zx planar translation dragger hidden.

    void showRotationX(); //!< show the x rotation dragger.
    void showRotationY(); //!< show the y rotation dragger.
    void showRotationZ(); //!< show the z rotation dragger.
    void hideRotationX(); //!< hide the x rotation dragger.
    void hideRotationY(); //!< hide the y rotation dragger.
    void hideRotationZ(); //!< hide the z rotation dragger.
    bool isShownRotationX(); //!< is x rotation dragger shown.
    bool isShownRotationY(); //!< is x rotation dragger shown.
    bool isShownRotationZ(); //!< is x rotation dragger shown.
    bool isHiddenRotationX(); //!< is x rotation dragger hidden.
    bool isHiddenRotationY(); //!< is x rotation dragger hidden.
    bool isHiddenRotationZ(); //!< is x rotation dragger hidden.
    //@}

    void GLRender(SoGLRenderAction * action) override;

protected:
    SbBool setUpConnections(SbBool onoff, SbBool doitalways = FALSE) override;
    void handleEvent(SoHandleEventAction * action) override;

    static void translationSensorCB(void *f, SoSensor *);
    static void rotationSensorCB(void *f, SoSensor *);
    static void valueChangedCB(void *, SoDragger *d);
    static void cameraCB(void *data, SoSensor *);
    static void idleCB(void *data, SoSensor *); //!< scheduled from cameraCB to auto scale dragger.
    static void finishDragCB(void *data, SoDragger *);


    SoFieldSensor translationSensor;
    SoFieldSensor rotationSensor;
    SoFieldSensor cameraSensor;

private:
    // Used to compensate for axis scale in world transformation when doing
    // auto scale.
    SbVec3f axisScale;

    bool scaleInited{false};

    void updateAxisScale();

    using inherited = SoDragger;
};

}

#endif // CSYSDRAGGER_H
