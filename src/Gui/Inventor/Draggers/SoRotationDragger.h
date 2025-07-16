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

#ifndef GUI_ROTATION_DRAGGER_H
#define GUI_ROTATION_DRAGGER_H

#include <Inventor/draggers/SoDragger.h>
#include <Inventor/fields/SoSFColor.h>
#include <Inventor/fields/SoSFDouble.h>
#include <Inventor/fields/SoSFFloat.h>
#include <Inventor/fields/SoSFInt32.h>
#include <Inventor/fields/SoSFRotation.h>
#include <Inventor/fields/SoSFString.h>
#include <Inventor/projectors/SbLineProjector.h>
#include <Inventor/projectors/SbPlaneProjector.h>
#include <Inventor/nodes/SoBaseColor.h>
#include <Base/Vector3D.h>

class SoTransform;
class SoCalculator;
class SoCoordinate3;

namespace Gui
{
class SoRotatorGeometryKit: public SoBaseKit
{
    SO_KIT_HEADER(SoLinearGeometryKit);

public:
    static void initClass();

    SoSFVec3f pivotPosition;

protected:
    SoRotatorGeometryKit();
    ~SoRotatorGeometryKit() override = default;

private:
    using inherited = SoBaseKit;
};

/*!
 * @brief Rotator geometry
 * 
 * A class to contain the geometry for SoRotationDragger
 */
class SoRotatorGeometry: public SoRotatorGeometryKit
{
    SO_KIT_HEADER(SoArrowGeometry);
    SO_KIT_CATALOG_ENTRY_HEADER(lightModel);
    SO_KIT_CATALOG_ENTRY_HEADER(drawStyle);
    SO_KIT_CATALOG_ENTRY_HEADER(arcCoords);
    SO_KIT_CATALOG_ENTRY_HEADER(arc);
    SO_KIT_CATALOG_ENTRY_HEADER(rotorPivot);

    SO_KIT_CATALOG_ENTRY_HEADER(_rotorPivotTranslation);

public:
    static void initClass();
    SoRotatorGeometry();

    SoSFFloat arcAngle; //!< in radians
    SoSFFloat arcRadius;
    SoSFFloat sphereRadius;
    SoSFFloat arcThickness;

protected:
    ~SoRotatorGeometry() override = default;

    void notify(SoNotList* notList) override;

private:
    constexpr static int segments = 10; //!< segments of the arc per arcAngle

    using inherited = SoRotatorGeometryKit;
};

/*! @brief Rotation Dragger.
 *
 * used for rotating around an axis. Set the rotation
 * increment to desired step. Use rotationIncrementCount
 * multiplied with rotationIncrement for full double
 * precision vector scalar.
 */
class SoRotationDragger : public SoDragger
{
    SO_KIT_HEADER(SoRotationDragger);
    SO_KIT_CATALOG_ENTRY_HEADER(activeSwitch);
    SO_KIT_CATALOG_ENTRY_HEADER(secondaryColor);
    SO_KIT_CATALOG_ENTRY_HEADER(scale);
    SO_KIT_CATALOG_ENTRY_HEADER(rotator);

public:
    static void initClass();
    SoRotationDragger();

    SoSFRotation rotation; //!< set from outside and used from outside for single precision.
    SoSFDouble rotationIncrement; //!< set from outside and used for rounding.
    SoSFInt32 rotationIncrementCount; //!< number of steps. used from outside.
    SoSFColor activeColor; //!< colour of the dragger while being dragged.
    SoSFVec3f geometryScale; //!< the scale of the dragger geometry

protected:
    ~SoRotationDragger() override;
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
    int roundIncrement(const float &radiansIn);
    SoBaseColor* buildActiveColor();

    using inherited = SoDragger;
};

class SoRotationDraggerContainer: public SoInteractionKit
{
    SO_KIT_HEADER(SoRotationDraggerContainer);
    SO_KIT_CATALOG_ENTRY_HEADER(draggerSwitch);
    SO_KIT_CATALOG_ENTRY_HEADER(baseColor);
    SO_KIT_CATALOG_ENTRY_HEADER(transform);
    SO_KIT_CATALOG_ENTRY_HEADER(dragger);

public:
    static void initClass();
    SoRotationDraggerContainer();

    SoSFRotation rotation;
    SoSFColor color;
    SoSFVec3f translation;
    SoSFBool visible;

    void setPointerDirection(const Base::Vector3d& dir);

    SoRotationDragger* getDragger();

private:
    SoBaseColor* buildColor();
    SoTransform* buildTransform();

    using inherited = SoInteractionKit;
};

}

#endif /* GUI_ROTATION_DRAGGER_H */
