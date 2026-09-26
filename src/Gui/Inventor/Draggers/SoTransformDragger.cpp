// SPDX-License-Identifier: LGPL-2.1-or-later
// SPDX-FileCopyrightText: 2015 Thomas Anderson <blobfish[at]gmx.com>
// SPDX-FileCopyrightText: 2026 Joao Matos
// SPDX-FileNotice: Part of the FreeCAD project.

/******************************************************************************
 *                                                                            *
 *   FreeCAD is free software: you can redistribute it and/or modify          *
 *   it under the terms of the GNU Lesser General Public License as           *
 *   published by the Free Software Foundation, either version 2.1 of the     *
 *   License, or (at your option) any later version.                          *
 *                                                                            *
 *   FreeCAD is distributed in the hope that it will be useful, but           *
 *   WITHOUT ANY WARRANTY; without even the implied warranty of               *
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the            *
 *   GNU Lesser General Public License for more details.                      *
 *                                                                            *
 *   You should have received a copy of the GNU Lesser General Public         *
 *   License along with FreeCAD.  If not, see                                *
 *   <https://www.gnu.org/licenses/>.                                         *
 *                                                                            *
 ******************************************************************************/

#include <array>
#include <cassert>
#include <numbers>

#include <Inventor/SbRotation.h>
#include <Inventor/actions/SoGLRenderAction.h>
#include <Inventor/draggers/SoScale1Dragger.h>
#include <Inventor/draggers/SoScale2UniformDragger.h>
#include <Inventor/engines/SoComposeVec3f.h>
#include <Inventor/nodes/SoLightModel.h>
#include <Inventor/nodes/SoDrawStyle.h>
#include <Inventor/nodes/SoBaseColor.h>
#include <Inventor/nodes/SoCone.h>
#include <Inventor/nodes/SoCoordinate3.h>
#include <Inventor/nodes/SoCylinder.h>
#include <Inventor/nodes/SoCube.h>
#include <Inventor/nodes/SoGroup.h>
#include <Inventor/nodes/SoLineSet.h>
#include <Inventor/nodes/SoOrthographicCamera.h>
#include <Inventor/nodes/SoPerspectiveCamera.h>
#include <Inventor/nodes/SoPickStyle.h>
#include <Inventor/nodes/SoRotation.h>
#include <Inventor/nodes/SoSeparator.h>
#include <Inventor/nodes/SoSphere.h>
#include <Inventor/nodes/SoSwitch.h>
#include <Inventor/nodes/SoTranslation.h>
#include <Inventor/nodes/SoText2.h>
#include <Inventor/nodes/SoAnnotation.h>
#include <Inventor/nodes/SoFontStyle.h>

#include <Base/Quantity.h>
#include <Base/Converter.h>

#include "SoTransformDragger.h"
#include "SoLinearDragger.h"
#include "SoPlanarDragger.h"
#include "SoRotationDragger.h"
#include "Utilities.h"

#include <Gui/SoLabelNodes.h>
#include <Gui/Inventor/SoToggleSwitch.h>


/*
   GENERAL NOTE ON COIN3D CUSTOM DRAGGERS
   * You basically have two choices for creating custom dragger geometry.
   * 1) create an .iv file and set environment variable to the file path. This
   *     comes with install headaches.
   * 2) create an .iv file and run through a mock compiler that generates a header
   *     file to include in the project. I would have gone this way but after installing
   *     inventor-demo(ubuntu), the mock compiler tool was there only in source and make
   *     didn't do anything. Didn't want to put any time into something I didn't like anyway.
   *
   * static SbList <SoNode*> * defaultdraggerparts = NULL; is a global definition
   * in SoInteractionKit that contains the geometry. There doesn't appear to be anyway
   * to add to this other than readDefaultParts, that takes a file. So maybe a temp file?
   *
   * naming appears to be central to the core. It looks like as long as an object
   * is alive SoNode::getByName() will find it. So maybe just create my own little
   * container of objects to keep the default geometry alive....This appears to be
   * working and I like this solution.
   *
   * SoInteractionKit warns about these
   * names all being the same scope and do NOT have to be unique. Need to make names
   * descriptive to avoid collisions.

   * this is point of the SoGroup accessed from SoFCDB::getStorage().
*/

using namespace Gui;

namespace
{
// The planar square spans 1.15..3.15 in each local axis.  Keep the scale
// controls as three distinct sides of one offset outline: two straight bars
// followed by the L-shaped corner.  4.075 halves the previous 1.85 clearance
// between the square edge and the outer control coordinate.
constexpr float planarScaleOffset = 4.075F;
constexpr float planarScaleBarCenter = 1.665F;
constexpr float planarScaleBarLength = 1.8F;
constexpr float planarScaleCornerLength = 1.0F;
constexpr float planarScaleLineThickness = 0.24F;
constexpr float planarScaleLineDepth = 0.18F;

SoSeparator* buildPlanarScaleBar(float alongBarCenter, bool active)
{
    auto root = new SoSeparator;

    auto lightModel = new SoLightModel;
    lightModel->model = SoLightModel::BASE_COLOR;
    root->addChild(lightModel);

    if (active) {
        auto color = new SoBaseColor;
        color->rgb = SbColor(1.0F, 1.0F, 0.0F);
        root->addChild(color);
    }

    auto translation = new SoTranslation;
    translation->translation = SbVec3f(planarScaleOffset, alongBarCenter, 0.0F);
    root->addChild(translation);

    auto handle = new SoCube;
    handle->width = planarScaleLineThickness;
    handle->height = planarScaleBarLength;
    handle->depth = planarScaleLineDepth;
    root->addChild(handle);
    return root;
}

SoSeparator* buildPlanarScaleCorner(bool active)
{
    auto root = new SoSeparator;

    auto lightModel = new SoLightModel;
    lightModel->model = SoLightModel::BASE_COLOR;
    root->addChild(lightModel);

    if (active) {
        auto color = new SoBaseColor;
        color->rgb = SbColor(1.0F, 1.0F, 0.0F);
        root->addChild(color);
    }

    // Join the two legs at their endpoints. Separate separators prevent one
    // leg's translation from accumulating into the other leg.
    constexpr float cornerCenter = planarScaleOffset - planarScaleCornerLength / 2.0F;
    auto horizontalRoot = new SoSeparator;
    auto horizontalPosition = new SoTranslation;
    horizontalPosition->translation = SbVec3f(cornerCenter, planarScaleOffset, 0.0F);
    horizontalRoot->addChild(horizontalPosition);
    auto horizontal = new SoCube;
    horizontal->width = planarScaleCornerLength;
    horizontal->height = planarScaleLineThickness;
    horizontal->depth = planarScaleLineDepth;
    horizontalRoot->addChild(horizontal);
    root->addChild(horizontalRoot);

    auto verticalRoot = new SoSeparator;
    auto verticalPosition = new SoTranslation;
    verticalPosition->translation = SbVec3f(planarScaleOffset, cornerCenter, 0.0F);
    verticalRoot->addChild(verticalPosition);
    auto vertical = new SoCube;
    vertical->width = planarScaleLineThickness;
    vertical->height = planarScaleCornerLength;
    vertical->depth = planarScaleLineDepth;
    verticalRoot->addChild(vertical);
    root->addChild(verticalRoot);
    return root;
}

void setupPlanarScaleBar(SoScale1Dragger* dragger, float alongBarCenter)
{
    dragger->setPart("scaler", buildPlanarScaleBar(alongBarCenter, false));
    dragger->setPart("scalerActive", buildPlanarScaleBar(alongBarCenter, true));
    dragger->setPart("feedback", new SoSeparator);
    dragger->setPart("feedbackActive", new SoSeparator);
}

void setupPlanarScaleCorner(SoScale2UniformDragger* dragger)
{
    dragger->setPart("scaler", buildPlanarScaleCorner(false));
    dragger->setPart("scalerActive", buildPlanarScaleCorner(true));
    dragger->setPart("feedback", new SoSeparator);
    dragger->setPart("feedbackActive", new SoSeparator);
}
}  // namespace

SO_KIT_SOURCE(SoTransformDragger)

void SoTransformDragger::initClass()
{
    SoLinearDraggerContainer::initClass();
    SoPlanarDragger::initClass();
    SoRotationDraggerContainer::initClass();
    SO_KIT_INIT_CLASS(SoTransformDragger, SoDragger, "Dragger");
}

SoTransformDragger::SoTransformDragger()
    : axisScale(1.0f, 1.0f, 1.0f)
{
    SO_KIT_CONSTRUCTOR(SoTransformDragger);

#if defined(Q_OS_MACOS) || defined(Q_OS_FREEBSD) || defined(Q_OS_OPENBSD)
    this->ref();
#endif

    SO_KIT_ADD_CATALOG_ENTRY(annotation, So3DAnnotation, TRUE, geomSeparator, "", TRUE);
    SO_KIT_ADD_CATALOG_ENTRY(scaleNode, SoScale, TRUE, annotation, "", TRUE);
    SO_KIT_ADD_CATALOG_ENTRY(pickStyle, SoPickStyle, TRUE, annotation, "", TRUE);

    // Translator
    SO_KIT_ADD_CATALOG_ENTRY(xTranslatorDragger, SoLinearDraggerContainer, TRUE, annotation, "", TRUE);
    SO_KIT_ADD_CATALOG_ENTRY(yTranslatorDragger, SoLinearDraggerContainer, TRUE, annotation, "", TRUE);
    SO_KIT_ADD_CATALOG_ENTRY(zTranslatorDragger, SoLinearDraggerContainer, TRUE, annotation, "", TRUE);

    // Planar Translator

    SO_KIT_ADD_CATALOG_ENTRY(xyPlanarTranslatorSwitch, SoSwitch, TRUE, annotation, "", TRUE);
    SO_KIT_ADD_CATALOG_ENTRY(yzPlanarTranslatorSwitch, SoSwitch, TRUE, annotation, "", TRUE);
    SO_KIT_ADD_CATALOG_ENTRY(zxPlanarTranslatorSwitch, SoSwitch, TRUE, annotation, "", TRUE);

    SO_KIT_ADD_CATALOG_ENTRY(
        xyPlanarTranslatorSeparator,
        SoSeparator,
        TRUE,
        xyPlanarTranslatorSwitch,
        "",
        TRUE
    );
    SO_KIT_ADD_CATALOG_ENTRY(
        yzPlanarTranslatorSeparator,
        SoSeparator,
        TRUE,
        yzPlanarTranslatorSwitch,
        "",
        TRUE
    );
    SO_KIT_ADD_CATALOG_ENTRY(
        zxPlanarTranslatorSeparator,
        SoSeparator,
        TRUE,
        zxPlanarTranslatorSwitch,
        "",
        TRUE
    );

    SO_KIT_ADD_CATALOG_ENTRY(
        xyPlanarTranslatorColor,
        SoBaseColor,
        TRUE,
        xyPlanarTranslatorSeparator,
        "",
        TRUE
    );
    SO_KIT_ADD_CATALOG_ENTRY(
        yzPlanarTranslatorColor,
        SoBaseColor,
        TRUE,
        yzPlanarTranslatorSeparator,
        "",
        TRUE
    );
    SO_KIT_ADD_CATALOG_ENTRY(
        zxPlanarTranslatorColor,
        SoBaseColor,
        TRUE,
        zxPlanarTranslatorSeparator,
        "",
        TRUE
    );

    SO_KIT_ADD_CATALOG_ENTRY(
        xyPlanarTranslatorRotation,
        SoRotation,
        TRUE,
        xyPlanarTranslatorSeparator,
        "",
        TRUE
    );
    SO_KIT_ADD_CATALOG_ENTRY(
        yzPlanarTranslatorRotation,
        SoRotation,
        TRUE,
        yzPlanarTranslatorSeparator,
        "",
        TRUE
    );
    SO_KIT_ADD_CATALOG_ENTRY(
        zxPlanarTranslatorRotation,
        SoRotation,
        TRUE,
        zxPlanarTranslatorSeparator,
        "",
        TRUE
    );

    SO_KIT_ADD_CATALOG_ENTRY(
        xyPlanarTranslatorDragger,
        SoPlanarDragger,
        TRUE,
        xyPlanarTranslatorSeparator,
        "",
        TRUE
    );
    SO_KIT_ADD_CATALOG_ENTRY(
        yzPlanarTranslatorDragger,
        SoPlanarDragger,
        TRUE,
        yzPlanarTranslatorSeparator,
        "",
        TRUE
    );
    SO_KIT_ADD_CATALOG_ENTRY(
        zxPlanarTranslatorDragger,
        SoPlanarDragger,
        TRUE,
        zxPlanarTranslatorSeparator,
        "",
        TRUE
    );

    // Planar scale
    SO_KIT_ADD_CATALOG_ENTRY(planarScaleSwitch, SoToggleSwitch, TRUE, annotation, "", TRUE);
    SO_KIT_ADD_CATALOG_ENTRY(xyPlanarScaleSeparator, SoSeparator, TRUE, planarScaleSwitch, "", TRUE);
    SO_KIT_ADD_CATALOG_ENTRY(yzPlanarScaleSeparator, SoSeparator, TRUE, planarScaleSwitch, "", TRUE);
    SO_KIT_ADD_CATALOG_ENTRY(zxPlanarScaleSeparator, SoSeparator, TRUE, planarScaleSwitch, "", TRUE);
    SO_KIT_ADD_CATALOG_ENTRY(xyPlanarScaleColor, SoBaseColor, TRUE, xyPlanarScaleSeparator, "", TRUE);
    SO_KIT_ADD_CATALOG_ENTRY(yzPlanarScaleColor, SoBaseColor, TRUE, yzPlanarScaleSeparator, "", TRUE);
    SO_KIT_ADD_CATALOG_ENTRY(zxPlanarScaleColor, SoBaseColor, TRUE, zxPlanarScaleSeparator, "", TRUE);
    SO_KIT_ADD_CATALOG_ENTRY(xyPlanarScaleRotation, SoRotation, TRUE, xyPlanarScaleSeparator, "", TRUE);
    SO_KIT_ADD_CATALOG_ENTRY(yzPlanarScaleRotation, SoRotation, TRUE, yzPlanarScaleSeparator, "", TRUE);
    SO_KIT_ADD_CATALOG_ENTRY(zxPlanarScaleRotation, SoRotation, TRUE, zxPlanarScaleSeparator, "", TRUE);
    SO_KIT_ADD_CATALOG_ENTRY(xyPlanarScaleXSeparator, SoSeparator, TRUE, xyPlanarScaleSeparator, "", TRUE);
    SO_KIT_ADD_CATALOG_ENTRY(yzPlanarScaleXSeparator, SoSeparator, TRUE, yzPlanarScaleSeparator, "", TRUE);
    SO_KIT_ADD_CATALOG_ENTRY(zxPlanarScaleXSeparator, SoSeparator, TRUE, zxPlanarScaleSeparator, "", TRUE);
    SO_KIT_ADD_CATALOG_ENTRY(xyPlanarScaleXRotation, SoRotation, TRUE, xyPlanarScaleXSeparator, "", TRUE);
    SO_KIT_ADD_CATALOG_ENTRY(yzPlanarScaleXRotation, SoRotation, TRUE, yzPlanarScaleXSeparator, "", TRUE);
    SO_KIT_ADD_CATALOG_ENTRY(zxPlanarScaleXRotation, SoRotation, TRUE, zxPlanarScaleXSeparator, "", TRUE);
    SO_KIT_ADD_CATALOG_ENTRY(xyPlanarScaleXDragger, SoScale1Dragger, TRUE, xyPlanarScaleXSeparator, "", TRUE);
    SO_KIT_ADD_CATALOG_ENTRY(yzPlanarScaleXDragger, SoScale1Dragger, TRUE, yzPlanarScaleXSeparator, "", TRUE);
    SO_KIT_ADD_CATALOG_ENTRY(zxPlanarScaleXDragger, SoScale1Dragger, TRUE, zxPlanarScaleXSeparator, "", TRUE);
    SO_KIT_ADD_CATALOG_ENTRY(xyPlanarScaleYSeparator, SoSeparator, TRUE, xyPlanarScaleSeparator, "", TRUE);
    SO_KIT_ADD_CATALOG_ENTRY(yzPlanarScaleYSeparator, SoSeparator, TRUE, yzPlanarScaleSeparator, "", TRUE);
    SO_KIT_ADD_CATALOG_ENTRY(zxPlanarScaleYSeparator, SoSeparator, TRUE, zxPlanarScaleSeparator, "", TRUE);
    SO_KIT_ADD_CATALOG_ENTRY(xyPlanarScaleYRotation, SoRotation, TRUE, xyPlanarScaleYSeparator, "", TRUE);
    SO_KIT_ADD_CATALOG_ENTRY(yzPlanarScaleYRotation, SoRotation, TRUE, yzPlanarScaleYSeparator, "", TRUE);
    SO_KIT_ADD_CATALOG_ENTRY(zxPlanarScaleYRotation, SoRotation, TRUE, zxPlanarScaleYSeparator, "", TRUE);
    SO_KIT_ADD_CATALOG_ENTRY(xyPlanarScaleYDragger, SoScale1Dragger, TRUE, xyPlanarScaleYSeparator, "", TRUE);
    SO_KIT_ADD_CATALOG_ENTRY(yzPlanarScaleYDragger, SoScale1Dragger, TRUE, yzPlanarScaleYSeparator, "", TRUE);
    SO_KIT_ADD_CATALOG_ENTRY(zxPlanarScaleYDragger, SoScale1Dragger, TRUE, zxPlanarScaleYSeparator, "", TRUE);
    SO_KIT_ADD_CATALOG_ENTRY(
        xyPlanarScaleUniformDragger,
        SoScale2UniformDragger,
        TRUE,
        xyPlanarScaleSeparator,
        "",
        TRUE
    );
    SO_KIT_ADD_CATALOG_ENTRY(
        yzPlanarScaleUniformDragger,
        SoScale2UniformDragger,
        TRUE,
        yzPlanarScaleSeparator,
        "",
        TRUE
    );
    SO_KIT_ADD_CATALOG_ENTRY(
        zxPlanarScaleUniformDragger,
        SoScale2UniformDragger,
        TRUE,
        zxPlanarScaleSeparator,
        "",
        TRUE
    );

    // Rotator
    SO_KIT_ADD_CATALOG_ENTRY(xRotatorDragger, SoRotationDraggerContainer, TRUE, annotation, "", TRUE);
    SO_KIT_ADD_CATALOG_ENTRY(yRotatorDragger, SoRotationDraggerContainer, TRUE, annotation, "", TRUE);
    SO_KIT_ADD_CATALOG_ENTRY(zRotatorDragger, SoRotationDraggerContainer, TRUE, annotation, "", TRUE);

    // Other
    SO_KIT_ADD_FIELD(translation, (0.0, 0.0, 0.0));
    SO_KIT_ADD_FIELD(translationIncrement, (1.0));
    SO_KIT_ADD_FIELD(translationIncrementCountX, (0));
    SO_KIT_ADD_FIELD(translationIncrementCountY, (0));
    SO_KIT_ADD_FIELD(translationIncrementCountZ, (0));

    SO_KIT_ADD_FIELD(rotation, (SbVec3f(0.0, 0.0, 1.0), 0.0));
    SO_KIT_ADD_FIELD(rotationIncrement, (std::numbers::pi / 8.0));
    SO_KIT_ADD_FIELD(rotationIncrementCountX, (0));
    SO_KIT_ADD_FIELD(rotationIncrementCountY, (0));
    SO_KIT_ADD_FIELD(rotationIncrementCountZ, (0));

    SO_KIT_ADD_FIELD(planarScaleFactor, (1.0, 1.0, 1.0));
    SO_KIT_ADD_FIELD(planarScaleVisible, (FALSE));

    SO_KIT_ADD_FIELD(draggerSize, (1.0));
    SO_KIT_ADD_FIELD(autoScaleResult, (1.0));

    SO_KIT_ADD_FIELD(xAxisLabel, ("X"));
    SO_KIT_ADD_FIELD(yAxisLabel, ("Y"));
    SO_KIT_ADD_FIELD(zAxisLabel, ("Z"));

    SO_KIT_INIT_INSTANCE();

    // Colors
    setAxisColors(
        SbColor(1.0, 0, 0).getPackedValue(0.0f),
        SbColor(0, 1.0, 0).getPackedValue(0.0f),
        SbColor(0, 0, 1.0).getPackedValue(0.0f)
    );

    // Translator
    setupTranslationDraggers();

    // Planar Translator
    SoPlanarDragger* tPlanarDragger;
    tPlanarDragger = SO_GET_ANY_PART(this, "xyPlanarTranslatorDragger", SoPlanarDragger);
    tPlanarDragger->translationIncrement.connectFrom(&this->translationIncrement);
    tPlanarDragger->autoScaleResult.connectFrom(&this->autoScaleResult);
    translationIncrementCountX.appendConnection(&tPlanarDragger->translationIncrementXCount);
    translationIncrementCountY.appendConnection(&tPlanarDragger->translationIncrementYCount);
    tPlanarDragger = SO_GET_ANY_PART(this, "yzPlanarTranslatorDragger", SoPlanarDragger);
    tPlanarDragger->translationIncrement.connectFrom(&this->translationIncrement);
    tPlanarDragger->autoScaleResult.connectFrom(&this->autoScaleResult);
    translationIncrementCountZ.appendConnection(&tPlanarDragger->translationIncrementXCount);
    translationIncrementCountY.appendConnection(&tPlanarDragger->translationIncrementYCount);
    tPlanarDragger = SO_GET_ANY_PART(this, "zxPlanarTranslatorDragger", SoPlanarDragger);
    tPlanarDragger->translationIncrement.connectFrom(&this->translationIncrement);
    tPlanarDragger->autoScaleResult.connectFrom(&this->autoScaleResult);
    translationIncrementCountX.appendConnection(&tPlanarDragger->translationIncrementXCount);
    translationIncrementCountZ.appendConnection(&tPlanarDragger->translationIncrementYCount);

    auto planarScaleSwitch = SO_GET_ANY_PART(this, "planarScaleSwitch", SoToggleSwitch);
    planarScaleSwitch->on.connectFrom(&planarScaleVisible);

    setupPlanarScaleBar(
        SO_GET_ANY_PART(this, "xyPlanarScaleXDragger", SoScale1Dragger),
        -planarScaleBarCenter
    );
    setupPlanarScaleBar(
        SO_GET_ANY_PART(this, "yzPlanarScaleXDragger", SoScale1Dragger),
        -planarScaleBarCenter
    );
    setupPlanarScaleBar(
        SO_GET_ANY_PART(this, "zxPlanarScaleXDragger", SoScale1Dragger),
        -planarScaleBarCenter
    );
    setupPlanarScaleBar(
        SO_GET_ANY_PART(this, "xyPlanarScaleYDragger", SoScale1Dragger),
        planarScaleBarCenter
    );
    setupPlanarScaleBar(
        SO_GET_ANY_PART(this, "yzPlanarScaleYDragger", SoScale1Dragger),
        planarScaleBarCenter
    );
    setupPlanarScaleBar(
        SO_GET_ANY_PART(this, "zxPlanarScaleYDragger", SoScale1Dragger),
        planarScaleBarCenter
    );
    setupPlanarScaleCorner(
        SO_GET_ANY_PART(this, "xyPlanarScaleUniformDragger", SoScale2UniformDragger)
    );
    setupPlanarScaleCorner(
        SO_GET_ANY_PART(this, "yzPlanarScaleUniformDragger", SoScale2UniformDragger)
    );
    setupPlanarScaleCorner(
        SO_GET_ANY_PART(this, "zxPlanarScaleUniformDragger", SoScale2UniformDragger)
    );

    // Rotator
    setupRotationDraggers();

    // Switches
    SoSwitch* sw;
    // Planar Translator
    sw = SO_GET_ANY_PART(this, "xyPlanarTranslatorSwitch", SoSwitch);
    SoInteractionKit::setSwitchValue(sw, SO_SWITCH_ALL);
    sw = SO_GET_ANY_PART(this, "yzPlanarTranslatorSwitch", SoSwitch);
    SoInteractionKit::setSwitchValue(sw, SO_SWITCH_ALL);
    sw = SO_GET_ANY_PART(this, "zxPlanarTranslatorSwitch", SoSwitch);
    SoInteractionKit::setSwitchValue(sw, SO_SWITCH_ALL);

    // Rotations

    SoRotation* localRotation;
    SbRotation tempRotation;
    auto angle = static_cast<float>(std::numbers::pi / 2.0);
    // Planar Translator
    localRotation = SO_GET_ANY_PART(this, "xyPlanarTranslatorRotation", SoRotation);
    localRotation->rotation.setValue(SbRotation::identity());
    localRotation = SO_GET_ANY_PART(this, "yzPlanarTranslatorRotation", SoRotation);
    localRotation->rotation.setValue(SbVec3f(0.0, -1.0, 0.0), angle);
    localRotation = SO_GET_ANY_PART(this, "zxPlanarTranslatorRotation", SoRotation);
    localRotation->rotation.setValue(SbVec3f(1.0, 0.0, 0.0), angle);
    // Planar scale
    localRotation = SO_GET_ANY_PART(this, "xyPlanarScaleRotation", SoRotation);
    localRotation->rotation.setValue(SbRotation::identity());
    localRotation = SO_GET_ANY_PART(this, "yzPlanarScaleRotation", SoRotation);
    localRotation->rotation.setValue(SbVec3f(0.0, -1.0, 0.0), angle);
    localRotation = SO_GET_ANY_PART(this, "zxPlanarScaleRotation", SoRotation);
    localRotation->rotation.setValue(SbVec3f(1.0, 0.0, 0.0), angle);
    // SoScale1Dragger always projects pointer motion onto its local X axis.
    // Rotate the local-Y controls while keeping their authored geometry in
    // place so each bar reacts to motion normal to the bar.
    localRotation = SO_GET_ANY_PART(this, "xyPlanarScaleXRotation", SoRotation);
    localRotation->rotation.setValue(SbVec3f(0.0, 0.0, 1.0), angle);
    localRotation = SO_GET_ANY_PART(this, "yzPlanarScaleXRotation", SoRotation);
    localRotation->rotation.setValue(SbVec3f(0.0, 0.0, 1.0), angle);
    localRotation = SO_GET_ANY_PART(this, "zxPlanarScaleXRotation", SoRotation);
    localRotation->rotation.setValue(SbVec3f(0.0, 0.0, 1.0), angle);
    localRotation = SO_GET_ANY_PART(this, "xyPlanarScaleYRotation", SoRotation);
    localRotation->rotation.setValue(SbRotation::identity());
    localRotation = SO_GET_ANY_PART(this, "yzPlanarScaleYRotation", SoRotation);
    localRotation->rotation.setValue(SbRotation::identity());
    localRotation = SO_GET_ANY_PART(this, "zxPlanarScaleYRotation", SoRotation);
    localRotation->rotation.setValue(SbRotation::identity());
    // Rotator
    SoRotationDraggerContainer* rDragger;
    rDragger = SO_GET_ANY_PART(this, "xRotatorDragger", SoRotationDraggerContainer);
    tempRotation = SbRotation(SbVec3f(1.0, 0.0, 0.0), angle);
    tempRotation *= SbRotation(SbVec3f(0.0, 0.0, 1.0), angle);
    rDragger->rotation.setValue(tempRotation);
    rDragger = SO_GET_ANY_PART(this, "yRotatorDragger", SoRotationDraggerContainer);
    tempRotation = SbRotation(SbVec3f(0.0, -1.0, 0.0), angle);
    tempRotation *= SbRotation(SbVec3f(0.0, 0.0, -1.0), angle);
    rDragger->rotation.setValue(tempRotation);
    rDragger = SO_GET_ANY_PART(this, "zRotatorDragger", SoRotationDraggerContainer);
    rDragger->rotation.setValue(SbRotation::identity());

    // this is for non-autoscale mode. this will be disconnected for autoscale
    // and won't be used. see setUpAutoScale.
    auto scaleEngine = new SoComposeVec3f();  // uses coin ref scheme.
    scaleEngine->x.connectFrom(&draggerSize);
    scaleEngine->y.connectFrom(&draggerSize);
    scaleEngine->z.connectFrom(&draggerSize);
    SoScale* localScaleNode = SO_GET_ANY_PART(this, "scaleNode", SoScale);
    localScaleNode->scaleFactor.connectFrom(&scaleEngine->vector);
    autoScaleResult.connectFrom(&draggerSize);

    SoPickStyle* localPickStyle = SO_GET_ANY_PART(this, "pickStyle", SoPickStyle);
    localPickStyle->style = SoPickStyle::SHAPE_ON_TOP;

    addValueChangedCallback(&SoTransformDragger::valueChangedCB);

    translationSensor.setFunction(&SoTransformDragger::translationSensorCB);
    translationSensor.setData(this);
    translationSensor.setPriority(0);

    rotationSensor.setFunction(&SoTransformDragger::rotationSensorCB);
    rotationSensor.setData(this);
    rotationSensor.setPriority(0);

    cameraSensor.setFunction(&SoTransformDragger::cameraCB);
    cameraSensor.setData(this);

    idleSensor.setFunction(&SoTransformDragger::idleCB);
    idleSensor.setData(this);

    this->addFinishCallback(&SoTransformDragger::finishDragCB, this);

    this->setUpConnections(TRUE, TRUE);
}

SoTransformDragger::~SoTransformDragger()
{
    this->setUpConnections(FALSE, TRUE);
    translationSensor.setData(nullptr);
    translationSensor.detach();
    rotationSensor.setData(nullptr);
    rotationSensor.detach();
    cameraSensor.setData(nullptr);
    cameraSensor.detach();
    idleSensor.setData(nullptr);
    idleSensor.unschedule();

    removeValueChangedCallback(&SoTransformDragger::valueChangedCB);
    removeFinishCallback(&SoTransformDragger::finishDragCB, this);
}


SbBool SoTransformDragger::setUpConnections(SbBool onoff, SbBool doitalways)
{
    if (!doitalways && (connectionsSetUp == onoff)) {
        return onoff;
    }

    auto tDraggerX = SO_GET_ANY_PART(this, "xTranslatorDragger", SoLinearDraggerContainer);
    auto tDraggerY = SO_GET_ANY_PART(this, "yTranslatorDragger", SoLinearDraggerContainer);
    auto tDraggerZ = SO_GET_ANY_PART(this, "zTranslatorDragger", SoLinearDraggerContainer);
    SoPlanarDragger* tPlanarDraggerXZ
        = SO_GET_ANY_PART(this, "xyPlanarTranslatorDragger", SoPlanarDragger);
    SoPlanarDragger* tPlanarDraggerYZ
        = SO_GET_ANY_PART(this, "yzPlanarTranslatorDragger", SoPlanarDragger);
    SoPlanarDragger* tPlanarDraggerZX
        = SO_GET_ANY_PART(this, "zxPlanarTranslatorDragger", SoPlanarDragger);
    std::array<SoDragger*, 9> planarScaleDraggers {
        SO_GET_ANY_PART(this, "xyPlanarScaleXDragger", SoScale1Dragger),
        SO_GET_ANY_PART(this, "xyPlanarScaleYDragger", SoScale1Dragger),
        SO_GET_ANY_PART(this, "xyPlanarScaleUniformDragger", SoScale2UniformDragger),
        SO_GET_ANY_PART(this, "yzPlanarScaleXDragger", SoScale1Dragger),
        SO_GET_ANY_PART(this, "yzPlanarScaleYDragger", SoScale1Dragger),
        SO_GET_ANY_PART(this, "yzPlanarScaleUniformDragger", SoScale2UniformDragger),
        SO_GET_ANY_PART(this, "zxPlanarScaleXDragger", SoScale1Dragger),
        SO_GET_ANY_PART(this, "zxPlanarScaleYDragger", SoScale1Dragger),
        SO_GET_ANY_PART(this, "zxPlanarScaleUniformDragger", SoScale2UniformDragger),
    };
    auto rDraggerX = SO_GET_ANY_PART(this, "xRotatorDragger", SoRotationDraggerContainer);
    auto rDraggerY = SO_GET_ANY_PART(this, "yRotatorDragger", SoRotationDraggerContainer);
    auto rDraggerZ = SO_GET_ANY_PART(this, "zRotatorDragger", SoRotationDraggerContainer);

    if (onoff) {
        inherited::setUpConnections(onoff, doitalways);

        registerChildDragger(tDraggerX->getDragger());
        registerChildDragger(tDraggerY->getDragger());
        registerChildDragger(tDraggerZ->getDragger());
        registerChildDragger(tPlanarDraggerXZ);
        registerChildDragger(tPlanarDraggerYZ);
        registerChildDragger(tPlanarDraggerZX);
        for (auto dragger : planarScaleDraggers) {
            dragger->addValueChangedCallback(&SoTransformDragger::planarScaleValueChangedCB, this);
            registerChildDraggerMovingIndependently(dragger);
            dragger->addFinishCallback(&SoTransformDragger::planarScaleFinishCB, this);
        }
        registerChildDragger(rDraggerX->getDragger());
        registerChildDragger(rDraggerY->getDragger());
        registerChildDragger(rDraggerZ->getDragger());

        translationSensorCB(this, nullptr);
        if (this->translationSensor.getAttachedField() != &this->translation) {
            this->translationSensor.attach(&this->translation);
        }
        rotationSensorCB(this, nullptr);
        if (this->rotationSensor.getAttachedField() != &this->rotation) {
            this->rotationSensor.attach(&this->rotation);
        }
    }
    else {
        unregisterChildDragger(tDraggerX->getDragger());
        unregisterChildDragger(tDraggerY->getDragger());
        unregisterChildDragger(tDraggerZ->getDragger());
        unregisterChildDragger(tPlanarDraggerXZ);
        unregisterChildDragger(tPlanarDraggerYZ);
        unregisterChildDragger(tPlanarDraggerZX);
        for (auto dragger : planarScaleDraggers) {
            dragger->removeValueChangedCallback(&SoTransformDragger::planarScaleValueChangedCB, this);
            dragger->removeFinishCallback(&SoTransformDragger::planarScaleFinishCB, this);
            unregisterChildDraggerMovingIndependently(dragger);
        }
        unregisterChildDragger(rDraggerX->getDragger());
        unregisterChildDragger(rDraggerY->getDragger());
        unregisterChildDragger(rDraggerZ->getDragger());

        inherited::setUpConnections(onoff, doitalways);

        if (this->translationSensor.getAttachedField()) {
            this->translationSensor.detach();
        }

        if (this->rotationSensor.getAttachedField()) {
            this->rotationSensor.detach();
        }
    }
    return !(this->connectionsSetUp = onoff);
}

void SoTransformDragger::translationSensorCB(void* f, SoSensor*)
{
    auto sudoThis = static_cast<SoTransformDragger*>(f);
    if (!f) {
        return;
    }

    SbMatrix matrix = sudoThis->getMotionMatrix();  // clazy:exclude=rule-of-two-soft
    sudoThis->workFieldsIntoTransform(matrix);
    sudoThis->setMotionMatrix(matrix);
}

void SoTransformDragger::rotationSensorCB(void* f, SoSensor*)
{
    auto sudoThis = static_cast<SoTransformDragger*>(f);
    if (!f) {
        return;
    }

    SbMatrix matrix = sudoThis->getMotionMatrix();  // clazy:exclude=rule-of-two-soft
    sudoThis->workFieldsIntoTransform(matrix);
    sudoThis->setMotionMatrix(matrix);
}

void SoTransformDragger::planarScaleValueChangedCB(void* f, SoDragger* d)
{
    auto sudoThis = static_cast<SoTransformDragger*>(f);
    if (!sudoThis || !d || sudoThis->resettingPlanarScale) {
        return;
    }

    SbVec3f childScale;
    if (auto scale1 = dynamic_cast<SoScale1Dragger*>(d)) {
        childScale = scale1->scaleFactor.getValue();
    }
    else if (auto scale2 = dynamic_cast<SoScale2UniformDragger*>(d)) {
        childScale = scale2->scaleFactor.getValue();
    }
    else {
        return;
    }

    auto isPart = [sudoThis, d](const char* name) {
        return sudoThis->getAnyPart(SbName(name), FALSE, FALSE, FALSE) == d;
    };

    SbVec3f localScale(1.0F, 1.0F, 1.0F);
    const bool isY = isPart("xyPlanarScaleYDragger") || isPart("yzPlanarScaleYDragger")
        || isPart("zxPlanarScaleYDragger");
    const bool isUniform = isPart("xyPlanarScaleUniformDragger")
        || isPart("yzPlanarScaleUniformDragger") || isPart("zxPlanarScaleUniformDragger");
    if (isUniform) {
        localScale.setValue(childScale[0], childScale[1], 1.0F);
    }
    else if (isY) {
        localScale[0] = childScale[0];
    }
    else {
        localScale[1] = childScale[0];
    }

    SbVec3f scale;
    const bool isXY = isPart("xyPlanarScaleXDragger") || isPart("xyPlanarScaleYDragger")
        || isPart("xyPlanarScaleUniformDragger");
    const bool isYZ = isPart("yzPlanarScaleXDragger") || isPart("yzPlanarScaleYDragger")
        || isPart("yzPlanarScaleUniformDragger");
    if (isXY) {
        scale = localScale;
    }
    else if (isYZ) {
        scale.setValue(1.0F, localScale[1], localScale[0]);
    }
    else {
        scale.setValue(localScale[0], 1.0F, localScale[1]);
    }
    sudoThis->planarScaleFactor = scale;

    // The scale is an editing result, not the transform of the manipulator.
    // Reset the child immediately so all nine controls remain screen-sized.
    sudoThis->resettingPlanarScale = true;
    d->setMotionMatrix(SbMatrix::identity());
    sudoThis->resettingPlanarScale = false;
}

void SoTransformDragger::planarScaleFinishCB(void* f, SoDragger*)
{
    auto sudoThis = static_cast<SoTransformDragger*>(f);
    if (sudoThis) {
        sudoThis->planarScaleFactor = SbVec3f(1.0F, 1.0F, 1.0F);
    }
}

void SoTransformDragger::valueChangedCB(void*, SoDragger* d)
{
    auto sudoThis = dynamic_cast<SoTransformDragger*>(d);
    assert(sudoThis);
    SbMatrix matrix = sudoThis->getMotionMatrix();  // clazy:exclude=rule-of-two-soft

    // all this just to get the translation?
    SbVec3f localTranslation, scaleDummy;
    SbRotation localRotation, scaleOrientationDummy;
    matrix.getTransform(localTranslation, localRotation, scaleDummy, scaleOrientationDummy);

    sudoThis->translationSensor.detach();
    if (sudoThis->translation.getValue() != localTranslation) {
        sudoThis->translation = localTranslation;
    }
    sudoThis->translationSensor.attach(&sudoThis->translation);

    sudoThis->rotationSensor.detach();
    if (sudoThis->rotation.getValue() != localRotation) {
        sudoThis->rotation = localRotation;
    }
    sudoThis->rotationSensor.attach(&sudoThis->rotation);
}

void SoTransformDragger::setUpAutoScale(SoCamera* cameraIn)
{
    // note: sofieldsensor checks if the current sensor is already attached
    // and takes appropriate action. So it is safe to attach to a field without
    // checking current attachment state.
    if (cameraIn->getTypeId() == SoOrthographicCamera::getClassTypeId()) {
        auto localCamera = dynamic_cast<SoOrthographicCamera*>(cameraIn);
        assert(localCamera);
        cameraSensor.attach(&localCamera->height);
        SoScale* localScaleNode = SO_GET_ANY_PART(this, "scaleNode", SoScale);
        localScaleNode->scaleFactor.disconnect();
        // This check shouldn't be needed but since CAM has its own
        // ViewProvider classes that implement setEdit but doesn't inherit from
        // ViewProviderDragger, we need to call Std_TransformManip twice.
        // This causes setEditViewer to be called twice and Coin throws an error
        // for trying to disconnect twice.
        if (autoScaleResult.isConnectedFromField()) {
            autoScaleResult.disconnect(&draggerSize);
        }
        cameraCB(this, nullptr);
    }
    else if (cameraIn->getTypeId() == SoPerspectiveCamera::getClassTypeId()) {
        auto localCamera = dynamic_cast<SoPerspectiveCamera*>(cameraIn);
        assert(localCamera);
        cameraSensor.attach(&localCamera->position);
        SoScale* localScaleNode = SO_GET_ANY_PART(this, "scaleNode", SoScale);
        localScaleNode->scaleFactor.disconnect();
        if (autoScaleResult.isConnectedFromField()) {
            autoScaleResult.disconnect(&draggerSize);
        }
        cameraCB(this, nullptr);
    }
}

void SoTransformDragger::cameraCB(void* data, SoSensor*)
{
    auto sudoThis = static_cast<SoTransformDragger*>(data);
    if (!sudoThis) {
        return;
    }
    if (!sudoThis->idleSensor.isScheduled()) {
        sudoThis->idleSensor.schedule();
    }
}

void SoTransformDragger::GLRender(SoGLRenderAction* action)
{
    if (!scaleInited) {
        scaleInited = true;
        updateDraggerCache(action->getCurPath());
        updateAxisScale();
    }

    inherited::GLRender(action);
}

void SoTransformDragger::updateAxisScale()
{
    SbMatrix localToWorld = getLocalToWorldMatrix();
    SbVec3f origin;
    localToWorld.multVecMatrix(SbVec3f(0.0, 0.0, 0.0), origin);
    SbVec3f vx, vy, vz;
    localToWorld.multVecMatrix(SbVec3f(1.0f, 0.0f, 0.0f), vx);
    localToWorld.multVecMatrix(SbVec3f(0.0f, 1.0f, 0.0f), vy);
    localToWorld.multVecMatrix(SbVec3f(0.0f, 0.0f, 1.0f), vz);
    float x = std::max((vx - origin).length(), 1e-7f);
    float y = std::max((vy - origin).length(), 1e-7f);
    float z = std::max((vz - origin).length(), 1e-7f);
    if (!axisScale.equals(SbVec3f(x, y, z), 1e-7f)) {
        axisScale.setValue(x, y, z);
        idleCB(this, &idleSensor);
    }
}

void SoTransformDragger::handleEvent(SoHandleEventAction* action)
{
    this->ref();

    inherited::handleEvent(action);
    updateAxisScale();

    this->unref();
}

void SoTransformDragger::idleCB(void* data, SoSensor*)
{
    auto sudoThis = static_cast<SoTransformDragger*>(data);
    if (!data) {
        return;
    }
    SoField* field = sudoThis->cameraSensor.getAttachedField();
    if (field) {
        auto camera = static_cast<SoCamera*>(field->getContainer());
        SbMatrix localToWorld = sudoThis->getLocalToWorldMatrix();
        SbVec3f origin;
        localToWorld.multVecMatrix(SbVec3f(0.0, 0.0, 0.0), origin);

        SbViewVolume viewVolume = camera->getViewVolume();
        float radius = sudoThis->draggerSize.getValue() / 2.0;
        float localScale = viewVolume.getWorldToScreenScale(origin, radius);
        float sx, sy, sz;
        sudoThis->axisScale.getValue(sx, sy, sz);
        SbVec3f scaleVector(localScale / sx, localScale / sy, localScale / sz);
        SoScale* localScaleNode = SO_GET_ANY_PART(sudoThis, "scaleNode", SoScale);
        localScaleNode->scaleFactor.setValue(scaleVector);
        sudoThis->autoScaleResult.setValue(localScale);
    }
}

void SoTransformDragger::finishDragCB(void* data, SoDragger*)
{
    auto sudoThis = static_cast<SoTransformDragger*>(data);
    assert(sudoThis);

    // note: when creating a second view of the document and then closing
    // the first viewer it deletes the camera. However, the attached field
    // of the cameraSensor will be detached automatically.
    SoField* field = sudoThis->cameraSensor.getAttachedField();
    if (field) {
        auto camera = static_cast<SoCamera*>(field->getContainer());
        if (camera->getTypeId() == SoPerspectiveCamera::getClassTypeId()) {
            cameraCB(sudoThis, nullptr);
        }
    }
}

void SoTransformDragger::clearIncrementCounts()
{
    translationIncrementCountX.setValue(0);
    translationIncrementCountY.setValue(0);
    translationIncrementCountZ.setValue(0);
    rotationIncrementCountX.setValue(0);
    rotationIncrementCountY.setValue(0);
    rotationIncrementCountZ.setValue(0);
}

void SoTransformDragger::setAxisColors(unsigned long x, unsigned long y, unsigned long z)
{
    SbColor colorX;
    SbColor colorY;
    SbColor colorZ;

    float t = 0.0f;
    colorX.setPackedValue(x, t);
    colorY.setPackedValue(y, t);
    colorZ.setPackedValue(z, t);

    SoBaseColor* color;

    // Translator
    SoLinearDraggerContainer* tDragger;
    tDragger = SO_GET_ANY_PART(this, "xTranslatorDragger", SoLinearDraggerContainer);
    tDragger->color.setValue(colorX[0], colorX[1], colorX[2]);
    tDragger = SO_GET_ANY_PART(this, "yTranslatorDragger", SoLinearDraggerContainer);
    tDragger->color.setValue(colorY[0], colorY[1], colorY[2]);
    tDragger = SO_GET_ANY_PART(this, "zTranslatorDragger", SoLinearDraggerContainer);
    tDragger->color.setValue(colorZ[0], colorZ[1], colorZ[2]);
    // Planar Translator
    color = SO_GET_ANY_PART(this, "xyPlanarTranslatorColor", SoBaseColor);
    color->rgb.setValue(colorZ[0], colorZ[1], colorZ[2]);
    color = SO_GET_ANY_PART(this, "yzPlanarTranslatorColor", SoBaseColor);
    color->rgb.setValue(colorX[0], colorX[1], colorX[2]);
    color = SO_GET_ANY_PART(this, "zxPlanarTranslatorColor", SoBaseColor);
    color->rgb.setValue(colorY[0], colorY[1], colorY[2]);
    // Planar scale
    color = SO_GET_ANY_PART(this, "xyPlanarScaleColor", SoBaseColor);
    color->rgb.setValue(colorZ[0], colorZ[1], colorZ[2]);
    color = SO_GET_ANY_PART(this, "yzPlanarScaleColor", SoBaseColor);
    color->rgb.setValue(colorX[0], colorX[1], colorX[2]);
    color = SO_GET_ANY_PART(this, "zxPlanarScaleColor", SoBaseColor);
    color->rgb.setValue(colorY[0], colorY[1], colorY[2]);
    // Rotator
    SoRotationDraggerContainer* rDragger;
    rDragger = SO_GET_ANY_PART(this, "xRotatorDragger", SoRotationDraggerContainer);
    rDragger->color.setValue(colorX[0], colorX[1], colorX[2]);
    rDragger = SO_GET_ANY_PART(this, "yRotatorDragger", SoRotationDraggerContainer);
    rDragger->color.setValue(colorY[0], colorY[1], colorY[2]);
    rDragger = SO_GET_ANY_PART(this, "zRotatorDragger", SoRotationDraggerContainer);
    rDragger->color.setValue(colorZ[0], colorZ[1], colorZ[2]);
}

// Visibility API Functions

// Translator
void SoTransformDragger::showTranslationX()
{
    auto tDragger = SO_GET_ANY_PART(this, "xTranslatorDragger", SoLinearDraggerContainer);
    tDragger->visible = true;
}
void SoTransformDragger::showTranslationY()
{
    auto tDragger = SO_GET_ANY_PART(this, "yTranslatorDragger", SoLinearDraggerContainer);
    tDragger->visible = true;
}
void SoTransformDragger::showTranslationZ()
{
    auto tDragger = SO_GET_ANY_PART(this, "zTranslatorDragger", SoLinearDraggerContainer);
    tDragger->visible = true;
}

void SoTransformDragger::hideTranslationX()
{
    auto tDragger = SO_GET_ANY_PART(this, "xTranslatorDragger", SoLinearDraggerContainer);
    tDragger->visible = false;
}
void SoTransformDragger::hideTranslationY()
{
    auto tDragger = SO_GET_ANY_PART(this, "yTranslatorDragger", SoLinearDraggerContainer);
    tDragger->visible = false;
}
void SoTransformDragger::hideTranslationZ()
{
    auto tDragger = SO_GET_ANY_PART(this, "zTranslatorDragger", SoLinearDraggerContainer);
    tDragger->visible = false;
}

bool SoTransformDragger::isShownTranslationX()
{
    auto tDragger = SO_GET_ANY_PART(this, "xTranslatorDragger", SoLinearDraggerContainer);
    return tDragger->visible.getValue();
}
bool SoTransformDragger::isShownTranslationY()
{
    auto tDragger = SO_GET_ANY_PART(this, "yTranslatorDragger", SoLinearDraggerContainer);
    return tDragger->visible.getValue();
}
bool SoTransformDragger::isShownTranslationZ()
{
    auto tDragger = SO_GET_ANY_PART(this, "zTranslatorDragger", SoLinearDraggerContainer);
    return tDragger->visible.getValue();
}

// Planar Translator
void SoTransformDragger::showPlanarTranslationXY()
{
    SoSwitch* sw = SO_GET_ANY_PART(this, "xyPlanarTranslatorSwitch", SoSwitch);
    SoInteractionKit::setSwitchValue(sw, SO_SWITCH_ALL);
}
void SoTransformDragger::showPlanarTranslationYZ()
{
    SoSwitch* sw = SO_GET_ANY_PART(this, "yzPlanarTranslatorSwitch", SoSwitch);
    SoInteractionKit::setSwitchValue(sw, SO_SWITCH_ALL);
}
void SoTransformDragger::showPlanarTranslationZX()
{
    SoSwitch* sw = SO_GET_ANY_PART(this, "zxPlanarTranslatorSwitch", SoSwitch);
    SoInteractionKit::setSwitchValue(sw, SO_SWITCH_ALL);
}

void SoTransformDragger::hidePlanarTranslationXY()
{
    SoSwitch* sw = SO_GET_ANY_PART(this, "xyPlanarTranslatorSwitch", SoSwitch);
    SoInteractionKit::setSwitchValue(sw, SO_SWITCH_NONE);
}
void SoTransformDragger::hidePlanarTranslationYZ()
{
    SoSwitch* sw = SO_GET_ANY_PART(this, "yzPlanarTranslatorSwitch", SoSwitch);
    SoInteractionKit::setSwitchValue(sw, SO_SWITCH_NONE);
}
void SoTransformDragger::hidePlanarTranslationZX()
{
    SoSwitch* sw = SO_GET_ANY_PART(this, "zxPlanarTranslatorSwitch", SoSwitch);
    SoInteractionKit::setSwitchValue(sw, SO_SWITCH_NONE);
}

bool SoTransformDragger::isShownPlanarTranslationXY()
{
    SoSwitch* sw = SO_GET_ANY_PART(this, "xyPlanarTranslatorSwitch", SoSwitch);
    return (sw->whichChild.getValue() == SO_SWITCH_ALL);
}
bool SoTransformDragger::isShownPlanarTranslationYZ()
{
    SoSwitch* sw = SO_GET_ANY_PART(this, "yzPlanarTranslatorSwitch", SoSwitch);
    return (sw->whichChild.getValue() == SO_SWITCH_ALL);
}
bool SoTransformDragger::isShownPlanarTranslationZX()
{
    SoSwitch* sw = SO_GET_ANY_PART(this, "zxPlanarTranslatorSwitch", SoSwitch);
    return (sw->whichChild.getValue() == SO_SWITCH_ALL);
}

bool SoTransformDragger::isHiddenPlanarTranslationXY()
{
    SoSwitch* sw = SO_GET_ANY_PART(this, "xyPlanarTranslatorSwitch", SoSwitch);
    return (sw->whichChild.getValue() == SO_SWITCH_NONE);
}
bool SoTransformDragger::isHiddenPlanarTranslationYZ()
{
    SoSwitch* sw = SO_GET_ANY_PART(this, "yzPlanarTranslatorSwitch", SoSwitch);
    return (sw->whichChild.getValue() == SO_SWITCH_NONE);
}
bool SoTransformDragger::isHiddenPlanarTranslationZX()
{
    SoSwitch* sw = SO_GET_ANY_PART(this, "zxPlanarTranslatorSwitch", SoSwitch);
    return (sw->whichChild.getValue() == SO_SWITCH_NONE);
}

// Rotator
void SoTransformDragger::showRotationX()
{
    SoRotationDraggerContainer* rDragger
        = SO_GET_ANY_PART(this, "xRotatorDragger", SoRotationDraggerContainer);
    rDragger->visible = true;
}
void SoTransformDragger::showRotationY()
{
    SoRotationDraggerContainer* rDragger
        = SO_GET_ANY_PART(this, "yRotatorDragger", SoRotationDraggerContainer);
    rDragger->visible = true;
}
void SoTransformDragger::showRotationZ()
{
    SoRotationDraggerContainer* rDragger
        = SO_GET_ANY_PART(this, "zRotatorDragger", SoRotationDraggerContainer);
    rDragger->visible = true;
}

void SoTransformDragger::hideRotationX()
{
    SoRotationDraggerContainer* rDragger
        = SO_GET_ANY_PART(this, "xRotatorDragger", SoRotationDraggerContainer);
    rDragger->visible = false;
}
void SoTransformDragger::hideRotationY()
{
    SoRotationDraggerContainer* rDragger
        = SO_GET_ANY_PART(this, "yRotatorDragger", SoRotationDraggerContainer);
    rDragger->visible = false;
}
void SoTransformDragger::hideRotationZ()
{
    SoRotationDraggerContainer* rDragger
        = SO_GET_ANY_PART(this, "zRotatorDragger", SoRotationDraggerContainer);
    rDragger->visible = false;
}

bool SoTransformDragger::isShownRotationX()
{
    SoRotationDraggerContainer* rDragger
        = SO_GET_ANY_PART(this, "xRotatorDragger", SoRotationDraggerContainer);
    return rDragger->visible.getValue();
}
bool SoTransformDragger::isShownRotationY()
{
    SoRotationDraggerContainer* rDragger
        = SO_GET_ANY_PART(this, "yRotatorDragger", SoRotationDraggerContainer);
    return rDragger->visible.getValue();
}
bool SoTransformDragger::isShownRotationZ()
{
    SoRotationDraggerContainer* rDragger
        = SO_GET_ANY_PART(this, "zRotatorDragger", SoRotationDraggerContainer);
    return rDragger->visible.getValue();
}

void SoTransformDragger::setupTranslationDraggers()
{
    setupTranslationDragger(
        "xTranslatorDragger",
        &xAxisLabel,
        translationIncrementCountX,
        SbVec3f(1.0, 0.0, 0.0)
    );
    setupTranslationDragger(
        "yTranslatorDragger",
        &yAxisLabel,
        translationIncrementCountY,
        SbVec3f(0.0, 1.0, 0.0)
    );
    setupTranslationDragger(
        "zTranslatorDragger",
        &zAxisLabel,
        translationIncrementCountZ,
        SbVec3f(0.0, 0.0, 1.0)
    );
}

void SoTransformDragger::setupTranslationDragger(
    const std::string& name,
    SoSFString* label,
    SoSFInt32& incrementCount,
    const SbVec3f& rotDir
)
{
    SoLinearDraggerContainer* draggerContainer
        = SO_GET_ANY_PART(this, name.c_str(), SoLinearDraggerContainer);
    SoLinearDragger* dragger = draggerContainer->getDragger();

    dragger->translationIncrement.connectFrom(&this->translationIncrement);
    dragger->autoScaleResult.connectFrom(&this->autoScaleResult);
    dragger->label.connectFrom(label);
    incrementCount.connectFrom(&dragger->translationIncrementCount);

    draggerContainer->setPointerDirection(rotDir);
}

void SoTransformDragger::setupRotationDraggers()
{
    setupRotationDragger("xRotatorDragger", rotationIncrementCountX);
    setupRotationDragger("yRotatorDragger", rotationIncrementCountY);
    setupRotationDragger("zRotatorDragger", rotationIncrementCountZ);
}

void SoTransformDragger::setupRotationDragger(const std::string& name, SoSFInt32& incrementCount)
{
    SoRotationDraggerContainer* draggerContainer
        = SO_GET_ANY_PART(this, name.c_str(), SoRotationDraggerContainer);
    SoRotationDragger* dragger = draggerContainer->getDragger();

    dragger->rotationIncrement.connectFrom(&this->rotationIncrement);
    incrementCount.connectFrom(&dragger->rotationIncrementCount);
}
