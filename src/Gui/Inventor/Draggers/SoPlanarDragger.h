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
#include <Inventor/projectors/SbLineProjector.h>
#include <Inventor/projectors/SbPlaneProjector.h>
#include <Inventor/nodes/SoBaseColor.h>

namespace Gui
{
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
class SoPlanarDragger: public SoDragger
{
    SO_KIT_HEADER(SoLinearDragger);
    SO_KIT_CATALOG_ENTRY_HEADER(planarTranslatorSwitch);
    SO_KIT_CATALOG_ENTRY_HEADER(planarTranslator);
    SO_KIT_CATALOG_ENTRY_HEADER(planarTranslatorActive);

public:
    static void initClass();
    SoPlanarDragger();
    SoSFVec3f translation;  //!< set from outside and used from outside for single precision.
    SoSFDouble translationIncrement;       //!< set from outside and used for rounding.
    SoSFInt32 translationIncrementXCount;  //!< number of steps. used from outside.
    SoSFInt32 translationIncrementYCount;  //!< number of steps. used from outside.
    SoSFFloat autoScaleResult;             //!< set from parent dragger.

protected:
    ~SoPlanarDragger() override;
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
    SbPlaneProjector projector;

private:
    void buildFirstInstance();
    SbVec3f roundTranslation(const SbVec3f& vecIn, float incrementIn);
    SoGroup* buildGeometry();
    using inherited = SoDragger;
};

}  // namespace Gui
