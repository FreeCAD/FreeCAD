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

#include "PathSim.h"


using namespace Base;
using namespace PathSimulator;

TYPESYSTEM_SOURCE(PathSimulator::PathSim , Base::BaseClass);

PathSim::PathSim()
{
}

PathSim::~PathSim()
{
}

void PathSim::BeginSimulation(Part::TopoShape * stock, float resolution)
{
	Base::BoundBox3d bbox = stock->getBoundBox();
	m_stock = std::make_unique<cStock>(bbox.MinX, bbox.MinY, bbox.MinZ, bbox.LengthX(), bbox.LengthY(), bbox.LengthZ(), resolution);
}

void PathSim::SetToolShape(const TopoDS_Shape& toolShape, float resolution)
{
	m_tool = std::make_unique<cSimTool>(toolShape, resolution);
}

Base::Placement * PathSim::ApplyCommand(Base::Placement * pos, Command * cmd)
{
	Point3D fromPos(*pos);
	Point3D toPos(*pos);
	toPos.UpdateCmd(*cmd);
	if (m_tool)
	{
		if (cmd->Name == "G0" || cmd->Name == "G1")
		{
			m_stock->ApplyLinearTool(fromPos, toPos, *m_tool);
		}
		else if (cmd->Name == "G2")
		{
			Vector3d vcent = cmd->getCenter();
			Point3D cent(vcent);
			m_stock->ApplyCircularTool(fromPos, toPos, cent, *m_tool, false);
		}
		else if (cmd->Name == "G3")
		{
			Vector3d vcent = cmd->getCenter();
			Point3D cent(vcent);
			m_stock->ApplyCircularTool(fromPos, toPos, cent, *m_tool, true);
		}
	}

	Base::Placement *plc = new Base::Placement();
	Vector3d vec(toPos.x, toPos.y, toPos.z);
	plc->setPosition(vec);
	return plc;
}






