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

#include <FCGlobal.h>

class SoTransform;
class SoCalculator;
class SoCoordinate3;

namespace Gui
{
class GuiExport SoRotatorGeometryKit: public SoBaseKit
{
    SO_KIT_HEADER(SoLinearGeometryKit);

public:
    static void initClass();

    SoSFVec3f pivotPosition;
    SoSFVec3f geometryScale;

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
class GuiExport SoRotatorGeometry: public SoRotatorGeometryKit
{
    SO_KIT_HEADER(SoRotatorGeometry);
    SO_KIT_CATALOG_ENTRY_HEADER(separator);
    SO_KIT_CATALOG_ENTRY_HEADER(lightModel);
    SO_KIT_CATALOG_ENTRY_HEADER(pickStyle);
    SO_KIT_CATALOG_ENTRY_HEADER(drawStyle);
    SO_KIT_CATALOG_ENTRY_HEADER(arcCoords);
    SO_KIT_CATALOG_ENTRY_HEADER(arc);
    SO_KIT_CATALOG_ENTRY_HEADER(pivotSeparator);
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

class GuiExport SoRotatorGeometry2: public SoRotatorGeometry
{
    SO_KIT_HEADER(SoRotatorGeometry2);
    SO_KIT_CATALOG_ENTRY_HEADER(leftArrowSwitch);
    SO_KIT_CATALOG_ENTRY_HEADER(leftArrowSeparator);
    SO_KIT_CATALOG_ENTRY_HEADER(leftArrowTransform);
    SO_KIT_CATALOG_ENTRY_HEADER(leftArrow);
    SO_KIT_CATALOG_ENTRY_HEADER(rightArrowSwitch);
    SO_KIT_CATALOG_ENTRY_HEADER(rightArrowSeparator);
    SO_KIT_CATALOG_ENTRY_HEADER(rightArrowTransform);
    SO_KIT_CATALOG_ENTRY_HEADER(rightArrow);

public:
    static void initClass();
    SoRotatorGeometry2();

    SoSFFloat coneBottomRadius;
    SoSFFloat coneHeight;
    SoSFBool leftArrowVisible;
    SoSFBool rightArrowVisible;

    void toggleArrowVisibility();

protected:
    ~SoRotatorGeometry2() override = default;

    void notify(SoNotList* notList) override;

private:
    constexpr static int segments = 10; //!< segments of the arc per arcAngle

    using inherited = SoRotatorGeometry;
};

class GuiExport SoRotatorArrow: public SoRotatorGeometryKit
{
    SO_KIT_HEADER(SoRotatorArrow);
    SO_KIT_CATALOG_ENTRY_HEADER(separator);
    SO_KIT_CATALOG_ENTRY_HEADER(lightModel);
    SO_KIT_CATALOG_ENTRY_HEADER(pickStyle);
    SO_KIT_CATALOG_ENTRY_HEADER(arrowBody);
    SO_KIT_CATALOG_ENTRY_HEADER(arrowTip);

    SO_KIT_CATALOG_ENTRY_HEADER(_arrowTransform);
    SO_KIT_CATALOG_ENTRY_HEADER(_arrowBodyTranslation);
    SO_KIT_CATALOG_ENTRY_HEADER(_arrowTipTranslation);

public:
    static void initClass();
    SoRotatorArrow();

    SoSFFloat coneBottomRadius;
    SoSFFloat coneHeight;
    SoSFFloat cylinderHeight;
    SoSFFloat cylinderRadius;
    SoSFFloat radius;
    SoSFFloat minRadius;

    void flipArrow();

protected:
    ~SoRotatorArrow() override = default;

    void notify(SoNotList* notList) override;

private:
    constexpr static int segments = 10; //!< segments of the arc per arcAngle

    using inherited = SoRotatorGeometryKit;
};

class GuiExport SoRotatorGeometryBaseKit: public SoBaseKit
{
    SO_KIT_HEADER(SoRotatorGeometryBaseKit);

public:
    static void initClass();

    SoSFRotation rotation; //!< set from the parent dragger
    SoSFVec3f geometryScale; //!< set from the parent dragger
    SoSFBool active; //!< set from the parent dragger

protected:
    SoRotatorGeometryBaseKit();
    ~SoRotatorGeometryBaseKit() override = default;

private:
    using inherited = SoBaseKit;
};

class GuiExport SoRotatorBase: public SoRotatorGeometryBaseKit
{
    SO_KIT_HEADER(SoArrowBase);
    SO_KIT_CATALOG_ENTRY_HEADER(separator);
    SO_KIT_CATALOG_ENTRY_HEADER(lightModel);
    SO_KIT_CATALOG_ENTRY_HEADER(pickStyle);
    SO_KIT_CATALOG_ENTRY_HEADER(baseColor);
    SO_KIT_CATALOG_ENTRY_HEADER(drawStyle);
    SO_KIT_CATALOG_ENTRY_HEADER(arcCoords);
    SO_KIT_CATALOG_ENTRY_HEADER(arc);

public:
    static void initClass();
    SoRotatorBase();

    SoSFFloat arcRadius;
    SoSFFloat minArcRadius;
    SoSFFloat arcThickness;
    SoSFColor color;

protected:
    ~SoRotatorBase() override = default;

    void notify(SoNotList* notList) override;

private:
    constexpr static int segments = 50; //!< segments of the arc per arcAngle

    using inherited = SoRotatorGeometryBaseKit;
};

/*! @brief Rotation Dragger.
 *
 * used for rotating around an axis. Set the rotation
 * increment to desired step. Use rotationIncrementCount
 * multiplied with rotationIncrement for full double
 * precision vector scalar.
 */
class GuiExport SoRotationDragger : public SoDragger
{
    SO_KIT_HEADER(SoRotationDragger);
    SO_KIT_CATALOG_ENTRY_HEADER(baseGeomSwitch);
    SO_KIT_CATALOG_ENTRY_HEADER(baseGeom);
    SO_KIT_CATALOG_ENTRY_HEADER(baseColor);
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
    SoSFColor color; //!< colour of the dragger
    SoSFColor activeColor; //!< colour of the dragger while being dragged.
    SoSFVec3f geometryScale; //!< the scale of the dragger geometry
    SoSFBool active; //!< set when the dragger is being dragged
    SoSFBool baseGeomVisible; //!< toggles if the dragger has a base geometry or not

    void instantiateBaseGeometry();

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
    SoBaseColor* buildColor();

    using inherited = SoDragger;
};

class GuiExport SoRotationDraggerContainer: public SoInteractionKit
{
    SO_KIT_HEADER(SoRotationDraggerContainer);
    SO_KIT_CATALOG_ENTRY_HEADER(draggerSwitch);
    SO_KIT_CATALOG_ENTRY_HEADER(transform);
    SO_KIT_CATALOG_ENTRY_HEADER(dragger);

public:
    static void initClass();
    SoRotationDraggerContainer();

    SoSFRotation rotation;
    SoSFColor color;
    SoSFVec3f translation;
    SoSFBool visible;

    SbVec3f getPointerDirection();
    void setPointerDirection(const SbVec3f& dir);
    void setArcNormalDirection(const SbVec3f& dir);

    SoRotationDragger* getDragger();

private:
    SoTransform* buildTransform();

    using inherited = SoInteractionKit;
};

}

#endif /* GUI_ROTATION_DRAGGER_H */
