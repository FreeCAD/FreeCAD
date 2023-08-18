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

#ifndef PATHSIMULATOR_PathSim_H
#define PATHSIMULATOR_PathSim_H

#include <memory>
#include <TopoDS_Shape.hxx>

#include <Mod/Path/App/Command.h>
#include <Mod/Part/App/TopoShape.h>
#include <Mod/Path/PathGlobal.h>

#include "VolSim.h"


using namespace Path;

namespace PathSimulator
{

    /** The representation of a CNC Toolpath Simulator */

	class PathSimulatorExport PathSim : public Base::BaseClass
    {
        TYPESYSTEM_HEADER();

        public:
			PathSim();
			~PathSim();

			void BeginSimulation(Part::TopoShape * stock, float resolution);
			void SetToolShape(const TopoDS_Shape& toolShape, float resolution);
			Base::Placement * ApplyCommand(Base::Placement * pos, Command * cmd);

		public:
			std::unique_ptr<cStock> m_stock;
			std::unique_ptr<cSimTool> m_tool;
	};

} //namespace Path


#endif // PATHSIMULATOR_PathSim_H
