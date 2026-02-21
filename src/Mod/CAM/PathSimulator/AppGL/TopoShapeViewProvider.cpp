/*
 * TopoShapeViewProvider.cpp
 *
 *  Created on: 15.07.2025
 *      Author: jffmichi
 */

#include "TopoShapeViewProvider.h"

#include <Inventor/nodes/SoSeparator.h>
#include <Inventor/nodes/SoSwitch.h>
#include <Mod/Part/App/TopoShape.h>

namespace CAMSimulator
{

TopoShapeViewProvider::TopoShapeViewProvider()
{
    pcSwitch = new SoSwitch;
    pcRoot->addChild(pcSwitch);

    setShapeVisible(true);
}

TopoShapeViewProvider& TopoShapeViewProvider::operator=(TopoShapeViewProvider&& vp)
{
    clear();

    pcShape = vp.pcShape;
    if (pcShape) {
        pcSwitch->addChild(pcShape);
    }

    vp.clear();

    return *this;
}

void TopoShapeViewProvider::clear()
{
    if (!pcShape) {
        return;
    }

    pcSwitch->removeChild(pcShape);
    pcShape = nullptr;
}

void TopoShapeViewProvider::setShape(const Part::TopoShape& shape)
{
    clear();

    // create SoNode from TopoShape

    std::stringstream s;
    shape.exportFaceSet(0.1f, 0.0f, {}, s);

    const auto str = s.str();
    SoInput in;
    in.setBuffer(str.data(), str.length());

    SoDB::read(&in, pcShape);
    pcSwitch->addChild(pcShape);
}

void TopoShapeViewProvider::setShapeVisible(bool b)
{
    pcSwitch->whichChild = b ? SO_SWITCH_ALL : SO_SWITCH_NONE;
}

} /* namespace CAMSimulator */
