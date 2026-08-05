// SPDX-License-Identifier: LGPL-2.1-or-later

#include "ViewProviderAxisTriad.h"

#include <QAction>
#include <QMenu>

#include <Gui/ActionFunction.h>

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

SoSeparator* createAxis(float x, float y, float z, float red, float green, float blue)
{
    auto* axis = new SoAnnotation;

    auto* style = new SoDrawStyle;
    style->style = SoDrawStyle::LINES;
    style->lineWidth = 3.0F;
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

SoSeparator* createOriginHub()
{
    auto* hub = new SoSeparator;

    auto* material = new SoMaterial;
    material->diffuseColor.setValue(0.8F, 0.8F, 0.8F);
    material->emissiveColor.setValue(0.8F, 0.8F, 0.8F);
    material->transparency.setValue(0.35F);
    hub->addChild(material);

    auto* sphere = new SoSphere;
    sphere->radius = 1.5F;
    hub->addChild(sphere);

    return hub;
}

}  // namespace

SoSeparator* MbDFEMGui::createAxisTriad()
{
    constexpr float axisLength = 10.0F;

    auto* triad = new SoAnnotation;

    auto* pickStyle = new SoPickStyle;
    pickStyle->style = SoPickStyle::SHAPE_ON_TOP;
    triad->addChild(pickStyle);

    triad->addChild(createAxis(axisLength, 0.0F, 0.0F, 1.0F, 0.0F, 0.0F));
    triad->addChild(createAxis(0.0F, axisLength, 0.0F, 0.0F, 0.75F, 0.0F));
    triad->addChild(createAxis(0.0F, 0.0F, axisLength, 0.0F, 0.25F, 1.0F));
    triad->addChild(createOriginHub());
    return triad;
}

SoSwitch* MbDFEMGui::createAxisTriadSwitch(bool visible)
{
    auto* axisTriadSwitch = new SoSwitch;
    axisTriadSwitch->whichChild = visible ? SO_SWITCH_ALL : SO_SWITCH_NONE;
    axisTriadSwitch->addChild(createAxisTriad());
    return axisTriadSwitch;
}

void MbDFEMGui::updateAxisTriadSwitch(SoSwitch* axisTriadSwitch, bool visible)
{
    if (axisTriadSwitch) {
        axisTriadSwitch->whichChild = visible ? SO_SWITCH_ALL : SO_SWITCH_NONE;
    }
}

QAction* MbDFEMGui::addAxisTriadContextMenuAction(QMenu* menu,
                                                  Gui::ActionFunction* actionFunction,
                                                  bool checked,
                                                  std::function<void(bool)> setVisible)
{
    if (!menu || !actionFunction) {
        return nullptr;
    }

    QAction* action = menu->addAction(QObject::tr("Axis Triad"));
    action->setCheckable(true);
    action->setChecked(checked);
    actionFunction->toggle(action, std::move(setVisible));
    return action;
}
