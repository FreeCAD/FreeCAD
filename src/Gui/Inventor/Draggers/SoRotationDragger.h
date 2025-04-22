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

#ifndef GUI_RDRAGGER_H
#define GUI_RDRAGGER_H

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

namespace Gui
{
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
    SO_KIT_CATALOG_ENTRY_HEADER(rotatorSwitch);
    SO_KIT_CATALOG_ENTRY_HEADER(rotator);
    SO_KIT_CATALOG_ENTRY_HEADER(rotatorActive);
public:
    static void initClass();
    SoRotationDragger();
    SoSFRotation rotation; //!< set from outside and used from outside for single precision.
    SoSFDouble rotationIncrement; //!< set from outside and used for rounding.
    SoSFInt32 rotationIncrementCount; //!< number of steps. used from outside.
    SoSFColor color; //!< set from outside. non-active color.

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
    float arcRadius;

private:
    void buildFirstInstance();
    int roundIncrement(const float &radiansIn);
    SoGroup* buildGeometry();
    using inherited = SoDragger;
};

}

#endif /* GUI_RDRAGGER_H */
