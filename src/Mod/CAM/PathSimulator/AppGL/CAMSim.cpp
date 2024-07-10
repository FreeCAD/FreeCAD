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

#include "PreCompiled.h"
#include "CAMSim.h"
#include "DlgCAMSimulator.h"
#include <stdio.h>
#include <BRepMesh_IncrementalMesh.hxx>
#include <BRepTools.hxx>
#include <Mod/Part/App/BRepMesh.h>
#include <vector>



using namespace Base;
using namespace CAMSimulator;

TYPESYSTEM_SOURCE(CAMSimulator::CAMSim, Base::BaseClass);

#define MAX_GCODE_LINE_LEN 120

CAMSim::CAMSim()
{}

CAMSim::~CAMSim()
{}

void CAMSim::BeginSimulation(const Part::TopoShape& stock, float quality)
{
    DlgCAMSimulator::GetInstance()->startSimulation(stock, quality);
}

void CAMSimulator::CAMSim::resetSimulation()
{
    DlgCAMSimulator::GetInstance()->resetSimulation();
}

void CAMSim::addTool(const std::vector<float> toolProfilePoints,
                     int toolNumber,
                     float diameter,
                     float resolution)
{
    DlgCAMSimulator::GetInstance()->addTool(toolProfilePoints, toolNumber, diameter, resolution);
}

void CAMSimulator::CAMSim::SetBaseShape(const Part::TopoShape& bshape, float resolution)
{
    if (bshape.isNull()) {
        return;
    }

    DlgCAMSimulator::GetInstance()->SetBaseShape(bshape, resolution);

    //BRepMesh_IncrementalMesh aMesh(bshape, resolution);

    //std::vector<Part::TopoShape::Domain> domains;
    //Part::TopoShape(bshape).getDomains(domains);

    //std::vector<Base::Vector3d> points;
    //std::vector<Part::TopoShape::Facet> facets;
    //Part::BRepMesh mesh;
    //mesh.getFacesFromDomains(domains, points, facets);
    //Part::TopoShape::Facet& facet = facets[0];
    
}

void CAMSim::AddCommand(Command* cmd)
{
    std::string gline = cmd->toGCode();
    DlgCAMSimulator::GetInstance()->addGcodeCommand(gline.c_str());
}
