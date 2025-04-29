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

#ifndef GUI_TDRAGGER_H
#define GUI_TDRAGGER_H

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
    SO_KIT_CATALOG_ENTRY_HEADER(activeColor);
    SO_KIT_CATALOG_ENTRY_HEADER(translator);
    SO_KIT_CATALOG_ENTRY_HEADER(cylinderSeparator);
    SO_KIT_CATALOG_ENTRY_HEADER(coneSeparator);
    SO_KIT_CATALOG_ENTRY_HEADER(labelSeparator);

    static constexpr float coneBottomRadius { 0.8f };
    static constexpr float coneHeight { 2.5f };

    static constexpr float cylinderHeight { 10.0f };
    static constexpr float cylinderRadius { 0.1f };
public:
    static void initClass();
    SoLinearDragger();

    SoSFString label; //!< set from outside and used to label
    SoSFVec3f translation; //!< set from outside and used from outside for single precision.
    SoSFDouble translationIncrement; //!< set from outside and used for rounding.
    SoSFInt32 translationIncrementCount; //!< number of steps. used from outside.
    SoSFFloat autoScaleResult; //!< set from parent dragger.

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
    void buildFirstInstance();
    SbVec3f roundTranslation(const SbVec3f &vecIn, float incrementIn);
    SoGroup* buildGeometry();

    SoSeparator* buildCylinderGeometry() const;
    SoSeparator* buildConeGeometry() const;
    SoSeparator* buildLabelGeometry();
    SoBaseColor* buildActiveColor();

    using inherited = SoDragger;
};

}

#endif /* TDRAGGER_H */
