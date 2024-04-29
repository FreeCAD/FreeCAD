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


using namespace Base;
using namespace CAMSimulator;

TYPESYSTEM_SOURCE(CAMSimulator::CAMSim , Base::BaseClass);

CAMSim::CAMSim()
{
}

CAMSim::~CAMSim()
{
}

void CAMSim::BeginSimulation(Part::TopoShape * stock, float resolution)
{
	Base::BoundBox3d bbox = stock->getBoundBox();
	m_stock = std::make_unique<cStock>(bbox.MinX, bbox.MinY, bbox.MinZ, bbox.LengthX(), bbox.LengthY(), bbox.LengthZ(), resolution);
    CAMSimulator::ShowWindow();
}

void CAMSim::SetToolShape(const TopoDS_Shape& toolShape, float resolution)
{
}

Base::Placement * CAMSim::ApplyCommand(Base::Placement * pos, Command * cmd)
{
	Base::Placement *plc = new Base::Placement();
    Vector3d vec(pos->getPosition().x, pos->getPosition().y, pos->getPosition().z);
	plc->setPosition(vec);
	return plc;
}






