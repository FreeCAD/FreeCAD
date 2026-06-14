// SPDX-License-Identifier: LGPL-2.1-or-later
// SPDX-FileCopyrightText: 2010 Werner Mayer <wmayer[at]users.sourceforge.net>
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

#include <FCConfig.h>

#include <sstream>

#include <Inventor/actions/SoGetBoundingBoxAction.h>
#include <Inventor/actions/SoGLRenderAction.h>
#include <Inventor/elements/SoLazyElement.h>
#include <Inventor/elements/SoModelMatrixElement.h>
#include <Inventor/elements/SoViewportRegionElement.h>
#include <Inventor/elements/SoViewVolumeElement.h>
#include <Inventor/nodekits/SoShapeKit.h>
#include <Inventor/nodes/SoBaseColor.h>
#include <Inventor/nodes/SoCone.h>
#include <Inventor/nodes/SoCoordinate3.h>
#include <Inventor/nodes/SoCube.h>
#include <Inventor/nodes/SoDrawStyle.h>
#include <Inventor/nodes/SoFontStyle.h>
#include <Inventor/nodes/SoLineSet.h>
#include <Inventor/nodes/SoPickStyle.h>
#include <Inventor/nodes/SoPointSet.h>
#include <Inventor/nodes/SoScale.h>
#include <Inventor/nodes/SoSeparator.h>
#include <Inventor/nodes/SoText2.h>
#include <Inventor/nodes/SoTranslation.h>

#include <Base/Color.h>
#include <Gui/ViewParams.h>

#include "SoAxisCrossKit.h"
#include "SoDevicePixelRatioElement.h"

using namespace Gui;

SO_KIT_SOURCE(SoShapeScale)

//  Constructor.
SoShapeScale::SoShapeScale()
{
    SO_KIT_CONSTRUCTOR(SoShapeScale);

    SO_KIT_ADD_FIELD(active, (true));
    SO_KIT_ADD_FIELD(scaleFactor, (1.0f));

    SO_KIT_ADD_CATALOG_ENTRY(topSeparator, SoSeparator, false, this, "", false);
    SO_KIT_ADD_CATALOG_ABSTRACT_ENTRY(shape, SoNode, SoCube, true, topSeparator, "", true);
    SO_KIT_ADD_CATALOG_ENTRY(scale, SoScale, false, topSeparator, shape, false);

    SO_KIT_INIT_INSTANCE();
}

// Destructor.
SoShapeScale::~SoShapeScale() = default;

void SoShapeScale::initClass()
{
    SO_KIT_INIT_CLASS(SoShapeScale, SoBaseKit, "BaseKit");
}

void SoShapeScale::GLRender(SoGLRenderAction* action)
{
    auto* scale = static_cast<SoScale*>(this->getAnyPart(SbName("scale"), true));
    if (!this->active.getValue()) {
        SbVec3f v(1.0f, 1.0f, 1.0f);
        if (scale->scaleFactor.getValue() != v) {
            scale->scaleFactor = v;
        }
    }
    else {
        SoState* state = action->getState();
        const SbViewportRegion& vp = SoViewportRegionElement::get(state);
        const SbViewVolume& vv = SoViewVolumeElement::get(state);

        SbVec3f center(0.0f, 0.0f, 0.0f);
        float nsize = this->scaleFactor.getValue() / float(vp.getViewportSizePixels()[0]);
        SoModelMatrixElement::get(state).multVecMatrix(center, center);  // world coords
        float sf = vv.getWorldToScreenScale(center, nsize);

        sf *= SoDevicePixelRatioElement::get(state);

        SbVec3f v(sf, sf, sf);
        if (scale->scaleFactor.getValue() != v) {
            scale->scaleFactor = v;
        }
    }

    inherited::GLRender(action);
}

// --------------------------------------------------------------

SO_KIT_SOURCE(SoAxisCrossKit)

void SoAxisCrossKit::initClass()
{
    SO_KIT_INIT_CLASS(SoAxisCrossKit, SoBaseKit, "BaseKit");
}

SoAxisCrossKit::SoAxisCrossKit()
{
    SO_KIT_CONSTRUCTOR(SoAxisCrossKit);

    // Add the parts to the catalog...
    SO_KIT_ADD_CATALOG_ENTRY(xAxis, SoShapeKit, true, this, "", true);
    SO_KIT_ADD_CATALOG_ENTRY(xHead, SoShapeKit, true, this, "", true);
    SO_KIT_ADD_CATALOG_ENTRY(yAxis, SoShapeKit, true, this, "", true);
    SO_KIT_ADD_CATALOG_ENTRY(yHead, SoShapeKit, true, this, "", true);
    SO_KIT_ADD_CATALOG_ENTRY(zAxis, SoShapeKit, true, this, "", true);
    SO_KIT_ADD_CATALOG_ENTRY(zHead, SoShapeKit, true, this, "", true);

    SO_KIT_INIT_INSTANCE();

    createAxes();
}

SoAxisCrossKit::~SoAxisCrossKit() = default;

// This kit is made up entirely of SoShapeKits.
// Since SoShapeKits do not affect state, neither does this.
SbBool SoAxisCrossKit::affectsState() const
{
    return false;
}

void SoAxisCrossKit::addWriteReference(SoOutput* /*out*/, SbBool /*isfromfield*/)
{
    // this node should not be written out to a file
}

void SoAxisCrossKit::getBoundingBox(SoGetBoundingBoxAction* action)
{
    inherited::getBoundingBox(action);
    action->resetCenter();
    action->setCenter(SbVec3f(0, 0, 0), false);
}

// Set up parts for default configuration of the jumping jack
void SoAxisCrossKit::createAxes()
{
    // Create the heads.
    auto head = new SoCone;
    head->bottomRadius.setValue(5);
    head->height.setValue(10);
    setPart("xHead.shape", head);
    setPart("yHead.shape", head);
    setPart("zHead.shape", head);

    // Create the axes.
    auto coords = new SoCoordinate3;
    coords->point.set1Value(0, SbVec3f(0, 0, 0));
    coords->point.set1Value(1, SbVec3f(90, 0, 0));
    setPart("xAxis.coordinate3", coords);
    setPart("yAxis.coordinate3", coords);
    setPart("zAxis.coordinate3", coords);

    auto shape = new SoLineSet;
    setPart("xAxis.shape", shape);
    setPart("yAxis.shape", shape);
    setPart("zAxis.shape", shape);

    // Place the axes and heads
    set("yAxis.transform", "rotation 0 0 1 1.5707999");
    set("zAxis.transform", "rotation 0 1 0 -1.5707999");

    set("xHead.transform", "translation 95 0 0");
    set("xHead.transform", "scaleFactor 0.5 1.5 0.5");
    set("xHead.transform", "rotation 0 0 -1  1.5707999");

    set("yHead.transform", "translation 0 95 0");
    set("yHead.transform", "scaleFactor 0.5 1.5 0.5");
    set("yHead.transform", "rotation 0 0 1 0");

    set("zHead.transform", "translation 0 0 95");
    set("zHead.transform", "scaleFactor 0.5 1.5 0.5");
    set("zHead.transform", "rotation 1 0 0  1.5707999");

    // Set colors & styles
    set("xAxis.appearance.lightModel", "model BASE_COLOR");
    set("xHead.appearance.lightModel", "model BASE_COLOR");
    set("yAxis.appearance.lightModel", "model BASE_COLOR");
    set("yHead.appearance.lightModel", "model BASE_COLOR");
    set("zAxis.appearance.lightModel", "model BASE_COLOR");
    set("zHead.appearance.lightModel", "model BASE_COLOR");
    set("xAxis.appearance.drawStyle", "lineWidth 1");
    set("yAxis.appearance.drawStyle", "lineWidth 1");
    set("zAxis.appearance.drawStyle", "lineWidth 1");

    unsigned long colorLong;
    Base::Color color;
    std::stringstream parameterstring;

    colorLong = Gui::ViewParams::instance()->getAxisXColor();
    color = Base::Color(static_cast<uint32_t>(colorLong));
    parameterstring << "diffuseColor " << color.r << " " << color.g << " " << color.b;
    set("xAxis.appearance.material", parameterstring.str().c_str());
    set("xHead.appearance.material", parameterstring.str().c_str());

    colorLong = Gui::ViewParams::instance()->getAxisYColor();
    color = Base::Color(static_cast<uint32_t>(colorLong));
    parameterstring << "diffuseColor " << color.r << " " << color.g << " " << color.b;
    set("yAxis.appearance.material", parameterstring.str().c_str());
    set("yHead.appearance.material", parameterstring.str().c_str());

    colorLong = Gui::ViewParams::instance()->getAxisZColor();
    color = Base::Color(static_cast<uint32_t>(colorLong));
    parameterstring << "diffuseColor " << color.r << " " << color.g << " " << color.b;
    set("zAxis.appearance.material", parameterstring.str().c_str());
    set("zHead.appearance.material", parameterstring.str().c_str());

    // Make unpickable
    set("xAxis.pickStyle", "style UNPICKABLE");
    set("xHead.pickStyle", "style UNPICKABLE");
    set("yAxis.pickStyle", "style UNPICKABLE");
    set("yHead.pickStyle", "style UNPICKABLE");
    set("zAxis.pickStyle", "style UNPICKABLE");
    set("zHead.pickStyle", "style UNPICKABLE");
}

// --------------------------------------------------------------

SO_NODE_SOURCE(SoRegPoint)

void SoRegPoint::initClass()
{
    SO_NODE_INIT_CLASS(SoRegPoint, SoShape, "Shape");
}

SoRegPoint::SoRegPoint()
{
    SO_NODE_CONSTRUCTOR(SoRegPoint);

    SO_NODE_ADD_FIELD(base, (SbVec3f(0, 0, 0)));
    SO_NODE_ADD_FIELD(normal, (SbVec3f(1, 1, 1)));
    SO_NODE_ADD_FIELD(length, (3.0));
    SO_NODE_ADD_FIELD(color, (1.0f, 0.447059f, 0.337255f));
    SO_NODE_ADD_FIELD(text, (""));

    root = new SoSeparator();
    root->ref();

    auto* pickStyle = new SoPickStyle();
    pickStyle->style = SoPickStyle::UNPICKABLE;
    root->addChild(pickStyle);

    const SbVec3f p1 = base.getValue();
    const SbVec3f p2 = p1 + normal.getValue() * length.getValue();

    geometryRoot = new SoSeparator();
    root->addChild(geometryRoot);

    geometryColor = new SoBaseColor();
    geometryColor->rgb.setValue(this->color.getValue());
    geometryRoot->addChild(geometryColor);

    auto* lineStyle = new SoDrawStyle();
    lineStyle->lineWidth = 1.0f;

    lineCoordinates = new SoCoordinate3();
    lineCoordinates->point.set1Value(0, p1);
    lineCoordinates->point.set1Value(1, p2);

    lineSet = new SoLineSet();
    lineSet->numVertices.set1Value(0, 2);

    auto* lineSep = new SoSeparator();
    lineSep->addChild(lineStyle);
    lineSep->addChild(lineCoordinates);
    lineSep->addChild(lineSet);
    geometryRoot->addChild(lineSep);

    auto* basePointStyle = new SoDrawStyle();
    basePointStyle->pointSize = 5.0f;

    basePointCoordinates = new SoCoordinate3();
    basePointCoordinates->point.set1Value(0, p1);

    basePointSet = new SoPointSet();
    basePointSet->numPoints = 1;

    auto* basePointSep = new SoSeparator();
    basePointSep->addChild(basePointStyle);
    basePointSep->addChild(basePointCoordinates);
    basePointSep->addChild(basePointSet);
    geometryRoot->addChild(basePointSep);

    auto* tipPointStyle = new SoDrawStyle();
    tipPointStyle->pointSize = 2.0f;

    tipPointCoordinates = new SoCoordinate3();
    tipPointCoordinates->point.set1Value(0, p2);

    tipPointSet = new SoPointSet();
    tipPointSet->numPoints = 1;

    auto* tipPointSep = new SoSeparator();
    tipPointSep->addChild(tipPointStyle);
    tipPointSep->addChild(tipPointCoordinates);
    tipPointSep->addChild(tipPointSet);
    geometryRoot->addChild(tipPointSep);

    textRoot = new SoSeparator();
    root->addChild(textRoot);

    move = new SoTranslation();
    move->translation.setValue(p2);
    textRoot->addChild(move);

    textColor = new SoBaseColor();
    textColor->rgb.setValue(this->color.getValue());

    auto* font = new SoFontStyle;
    font->size = 14;

    label = new SoText2();
    label->string = this->text.getValue();

    auto* sub = new SoSeparator();
    sub->addChild(textColor);
    sub->addChild(font);
    sub->addChild(label);
    textRoot->addChild(sub);
}

SoRegPoint::~SoRegPoint()
{
    root->unref();
    root = nullptr;
    geometryRoot = nullptr;
    geometryColor = nullptr;
    lineCoordinates = nullptr;
    lineSet = nullptr;
    basePointCoordinates = nullptr;
    basePointSet = nullptr;
    tipPointCoordinates = nullptr;
    tipPointSet = nullptr;
    textRoot = nullptr;
    move = nullptr;
    textColor = nullptr;
    label = nullptr;
}

/**
 * Renders the probe with text label and a bullet at the base point.
 */
void SoRegPoint::GLRender(SoGLRenderAction* action)
{
    if (!shouldGLRender(action) || !action) {
        return;
    }

    SoState* state = action->getState();
    if (!state || !root) {
        return;
    }

    state->push();
    SoLazyElement::setLightModel(state, SoLazyElement::BASE_COLOR);
    root->GLRender(action);
    state->pop();
}

void SoRegPoint::generatePrimitives(SoAction* action)
{
    // This node is implemented as a small internal scenegraph; delegate to it.
    // This is primarily useful for non-GL actions and keeps behavior consistent.
    if (root && action) {
        root->doAction(action);
    }
}

/**
 * Sets the bounding box of the probe to \a box and its center to \a center.
 */
void SoRegPoint::computeBBox(SoAction* action, SbBox3f& box, SbVec3f& center)
{
    root->doAction(action);
    if (action->getTypeId().isDerivedFrom(SoGetBoundingBoxAction::getClassTypeId())) {
        static_cast<SoGetBoundingBoxAction*>(action)->resetCenter();
    }

    SbVec3f p1 = base.getValue();
    SbVec3f p2 = p1 + normal.getValue() * length.getValue();

    box.extendBy(p1);
    box.extendBy(p2);

    center = box.getCenter();
}

void SoRegPoint::notify(SoNotList* node)
{
    SoField* f = node->getLastField();
    if (f == &this->base || f == &this->normal || f == &this->length) {
        const SbVec3f p1 = base.getValue();
        const SbVec3f p2 = p1 + normal.getValue() * length.getValue();

        if (move) {
            move->translation.setValue(p2);
        }
        if (lineCoordinates) {
            lineCoordinates->point.set1Value(0, p1);
            lineCoordinates->point.set1Value(1, p2);
        }
        if (basePointCoordinates) {
            basePointCoordinates->point.set1Value(0, p1);
        }
        if (tipPointCoordinates) {
            tipPointCoordinates->point.set1Value(0, p2);
        }
    }
    else if (f == &this->color) {
        if (geometryColor) {
            geometryColor->rgb = this->color.getValue();
        }
        if (textColor) {
            textColor->rgb = this->color.getValue();
        }
    }
    else if (f == &this->text) {
        if (label) {
            label->string = this->text.getValue();
        }
    }

    SoShape::notify(node);
}
