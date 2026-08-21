// SPDX-License-Identifier: LGPL-2.1-or-later

#include "ViewProviderAxisTriad.h"

#include <Inventor/nodes/SoAnnotation.h>
#include <Inventor/nodes/SoCoordinate3.h>
#include <Inventor/nodes/SoDrawStyle.h>
#include <Inventor/nodes/SoLineSet.h>
#include <Inventor/nodes/SoMaterial.h>
#include <Inventor/nodes/SoPickStyle.h>
#include <Inventor/nodes/SoSeparator.h>
#include <Inventor/nodes/SoSphere.h>
#include <Inventor/nodes/SoSwitch.h>

namespace
{

SoSeparator* createAxis(float x,
                        float y,
                        float z,
                        float red,
                        float green,
                        float blue,
                        float lineWidth)
{
    auto* axis = new SoAnnotation;

    auto* style = new SoDrawStyle;
    style->style = SoDrawStyle::LINES;
    style->lineWidth = lineWidth;
    axis->addChild(style);

    auto* material = new SoMaterial;
    material->diffuseColor.setValue(red, green, blue);
    material->emissiveColor.setValue(red, green, blue);
    axis->addChild(material);

    auto* coords = new SoCoordinate3;
    coords->point.set1Value(0, 0.0F, 0.0F, 0.0F);
    coords->point.set1Value(1, x, y, z);
    axis->addChild(coords);

    auto* line = new SoLineSet;
    line->numVertices.set1Value(0, 2);
    axis->addChild(line);

    return axis;
}

SoSeparator* createOriginHub(float radius,
                             float red,
                             float green,
                             float blue,
                             float transparency)
{
    auto* hub = new SoSeparator;

    auto* material = new SoMaterial;
    material->diffuseColor.setValue(red, green, blue);
    material->emissiveColor.setValue(red, green, blue);
    material->transparency.setValue(transparency);
    hub->addChild(material);

    auto* sphere = new SoSphere;
    sphere->radius = radius;
    hub->addChild(sphere);

    return hub;
}

SoSeparator* createAxisTriad(float axisLength,
                             float lineWidth,
                             float hubRadius,
                             float hubRed,
                             float hubGreen,
                             float hubBlue,
                             float hubTransparency)
{
    auto* triad = new SoAnnotation;

    auto* pickStyle = new SoPickStyle;
    pickStyle->style = SoPickStyle::SHAPE_ON_TOP;
    triad->addChild(pickStyle);

    triad->addChild(createAxis(axisLength, 0.0F, 0.0F, 1.0F, 0.0F, 0.0F, lineWidth));
    triad->addChild(createAxis(0.0F, axisLength, 0.0F, 0.0F, 0.75F, 0.0F, lineWidth));
    triad->addChild(createAxis(0.0F, 0.0F, axisLength, 0.0F, 0.25F, 1.0F, lineWidth));
    triad->addChild(createOriginHub(hubRadius, hubRed, hubGreen, hubBlue, hubTransparency));
    return triad;
}

}  // namespace

SoSeparator* MbDFEMGui::createAxisTriad()
{
    return ::createAxisTriad(10.0F, 3.0F, 1.5F, 0.8F, 0.8F, 0.8F, 0.35F);
}

SoSeparator* MbDFEMGui::createMassMarkerAxisTriad()
{
    return ::createAxisTriad(14.0F, 5.0F, 2.8F, 1.0F, 0.75F, 0.05F, 0.1F);
}

void MbDFEMGui::updateAxisTriadSwitch(SoSwitch* axisTriadSwitch, bool visible)
{
    if (axisTriadSwitch) {
        axisTriadSwitch->whichChild = visible ? SO_SWITCH_ALL : SO_SWITCH_NONE;
    }
}
