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

#include "PreCompiled.h"  // NOLINT
#ifndef _PreComp_
#include <string>
#include <vector>
#endif

#include "CAMSim.h"
#include "DlgCAMSimulator.h"
#include <Mod/Part/App/BRepMesh.h>


using namespace Base;
using namespace CAMSimulator;

TYPESYSTEM_SOURCE(CAMSimulator::CAMSim, Base::BaseClass);

void CAMSim::BeginSimulation(const Part::TopoShape& stock, float quality)
{
    DlgCAMSimulator::GetInstance()->startSimulation(stock, quality);
}

void CAMSimulator::CAMSim::resetSimulation()
{
    DlgCAMSimulator::GetInstance()->resetSimulation();
}

void CAMSim::addTool(const std::vector<float>& toolProfilePoints,
                     int toolNumber,
                     float diameter,
                     float resolution)
{
    DlgCAMSimulator::GetInstance()->addTool(toolProfilePoints, toolNumber, diameter, resolution);
}

void CAMSimulator::CAMSim::SetBaseShape(const Part::TopoShape& baseShape, float resolution)
{
    if (baseShape.isNull()) {
        return;
    }

    DlgCAMSimulator::GetInstance()->SetBaseShape(baseShape, resolution);
}

void CAMSim::AddCommand(Command* cmd)
{
    std::string gline = cmd->toGCode();
    DlgCAMSimulator::GetInstance()->addGcodeCommand(gline.c_str());
}
