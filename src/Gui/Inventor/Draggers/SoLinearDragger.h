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

#ifndef GUI_LINEAR_DRAGGER_H
#define GUI_LINEAR_DRAGGER_H

#include <Inventor/draggers/SoDragger.h>
#include <Inventor/fields/SoSFColor.h>
#include <Inventor/fields/SoSFDouble.h>
#include <Inventor/fields/SoSFFloat.h>
#include <Inventor/fields/SoSFInt32.h>
#include <Inventor/fields/SoSFRotation.h>
#include <Inventor/fields/SoSFString.h>
#include <Inventor/fields/SoSFVec3f.h>
#include <Inventor/projectors/SbLineProjector.h>
#include <Inventor/projectors/SbPlaneProjector.h>
#include <Base/Vector3D.h>

class SoCamera;
class SoSwitch;
class SoBaseColor;
class SoTransform;
class SoCalculator;

namespace Gui
{
class SoLinearGeometryKit: public SoBaseKit
{
    SO_KIT_HEADER(SoLinearGeometryKit);

public:
    static void initClass();

    SoSFVec3f tipPosition;

protected:
    SoLinearGeometryKit();
    ~SoLinearGeometryKit() override = default;

private:
    using inherited = SoBaseKit;
};

/*!
 * @brief Arrow geometry
 * 
 * A class to contain the geometry for SoLinearDragger
 */
class SoArrowGeometry: public SoLinearGeometryKit
{
    SO_KIT_HEADER(SoArrowGeometry);
    SO_KIT_CATALOG_ENTRY_HEADER(lightModel);
    SO_KIT_CATALOG_ENTRY_HEADER(arrowBody);
    SO_KIT_CATALOG_ENTRY_HEADER(arrowTip);

    SO_KIT_CATALOG_ENTRY_HEADER(_arrowBodyTranslation);
    SO_KIT_CATALOG_ENTRY_HEADER(_arrowTipTranslation);

public:
    static void initClass();
    SoArrowGeometry();

    SoSFFloat coneBottomRadius;
    SoSFFloat coneHeight;
    SoSFFloat cylinderHeight;
    SoSFFloat cylinderRadius;

protected:
    ~SoArrowGeometry() override = default;

    void notify(SoNotList* notList) override;

private:
    using inherited = SoLinearGeometryKit;
};

/*! @brief Translation Dragger.
 *
 * used for translating along axis. Set the
 * translationIncrement to desired step. Use
 * 'translationIncrementCount' multiplied with
 * 'translationIncrement' for a full double
 * precision vector scalar.
 */
class SoLinearDragger : public SoDragger
{
    SO_KIT_HEADER(SoLinearDragger);
    SO_KIT_CATALOG_ENTRY_HEADER(activeSwitch);
    SO_KIT_CATALOG_ENTRY_HEADER(secondaryColor);
    SO_KIT_CATALOG_ENTRY_HEADER(labelSwitch);
    SO_KIT_CATALOG_ENTRY_HEADER(labelSeparator);
    SO_KIT_CATALOG_ENTRY_HEADER(scale);
    SO_KIT_CATALOG_ENTRY_HEADER(arrow);

public:
    static void initClass();
    SoLinearDragger();

    SoSFString label; //!< set from outside and used to label
    SoSFVec3f translation; //!< set from outside and used from outside for single precision.
    SoSFDouble translationIncrement; //!< set from outside and used for rounding.
    SoSFInt32 translationIncrementCount; //!< number of steps. used from outside.
    SoSFFloat autoScaleResult; //!< set from parent dragger.
    SoSFColor activeColor; //!< colour of the dragger while being dragged
    SoSFBool labelVisible; //!< controls the visibility of the dragger label
    SoSFVec3f geometryScale; //!< the scale of the dragger geometry

protected:
    ~SoLinearDragger() override;
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
    SbVec3f roundTranslation(const SbVec3f &vecIn, float incrementIn);

    SoSeparator* buildLabelGeometry();
    SoBaseColor* buildActiveColor();

    using inherited = SoDragger;
};

class SoLinearDraggerContainer: public SoInteractionKit
{
    SO_KIT_HEADER(SoLinearDraggerContainer);
    SO_KIT_CATALOG_ENTRY_HEADER(draggerSwitch);
    SO_KIT_CATALOG_ENTRY_HEADER(baseColor);
    SO_KIT_CATALOG_ENTRY_HEADER(transform);
    SO_KIT_CATALOG_ENTRY_HEADER(dragger);

public:
    static void initClass();
    SoLinearDraggerContainer();

    SoSFRotation rotation;
    SoSFColor color;
    SoSFVec3f translation;
    SoSFBool visible;

    void setPointerDirection(const Base::Vector3d& dir);

    SoLinearDragger* getDragger();

private:
    SoBaseColor* buildColor();
    SoTransform* buildTransform();

    using inherited = SoInteractionKit;
};

}

#endif /* GUI_LINEAR_DRAGGER_H */
