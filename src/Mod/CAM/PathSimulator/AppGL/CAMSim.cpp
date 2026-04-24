// SPDX-License-Identifier: LGPL-2.1-or-later

/***************************************************************************
 *   Copyright (c) 2017 Shai Seger <shaise at gmail>                       *
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


#include "CAMSim.h"

#include "DlgCAMSimulator.h"
#include <string>
#include <vector>

using namespace Base;

TYPESYSTEM_SOURCE(CAMSimulator::CAMSim, Base::BaseClass);

namespace CAMSimulator
{

void CAMSim::BeginSimulation(const Part::TopoShape& stock, float quality)
{
    DlgCAMSimulator::instance()->startSimulation(stock, quality);
}

void CAMSim::resetSimulation(Gui::Document* doc)
{
    DlgCAMSimulator::instance()->resetSimulation(doc);
}

void CAMSim::addTool(
    const std::vector<float>& toolProfilePoints,
    int toolNumber,
    float diameter,
    float resolution
)
{
    DlgCAMSimulator::instance()->addTool(toolProfilePoints, toolNumber, diameter, resolution);
}

void CAMSim::SetBaseShape(const Part::TopoShape& baseShape, float resolution)
{
    if (baseShape.isNull()) {
        return;
    }

    DlgCAMSimulator::instance()->setBaseShape(baseShape, resolution);
}

void CAMSim::AddCommand(Command* cmd)
{
    std::string gline = cmd->toGCode();
    DlgCAMSimulator::instance()->addGcodeCommand(gline.c_str());
}

}  // namespace CAMSimulator
