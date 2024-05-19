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

#ifndef CAMSimulator_CAMSim_H
#define CAMSimulator_CAMSim_H

#include <memory>
#include <TopoDS_Shape.hxx>

#include <Mod/CAM/App/Command.h>
#include <Mod/Part/App/TopoShape.h>
#include <Mod/CAM/PathGlobal.h>
#include <Mod/Mesh/App/Mesh.h>
#include <Mod/CAM/App/Command.h>

#include "DlgCAMSimulator.h"

using namespace Path;

namespace CAMSimulator
{

/** The representation of a CNC Toolpath Simulator */

class CAMSimulatorExport CAMSim: public Base::BaseClass
{
    // TYPESYSTEM_HEADER();

public:
    static Base::Type getClassTypeId(void);
    virtual Base::Type getTypeId(void) const;
    static void init(void);
    static void* create(void);

private:
    static Base::Type classTypeId;


public:
    CAMSim();
    ~CAMSim();

    void BeginSimulation(Part::TopoShape* stock, float resolution);
    void resetSimulation();
    void addTool(const std::vector<float> toolProfilePoints,
                 int toolNumber,
                 float diameter,
                 float resolution);
    void AddCommand(Command* cmd);

public:
    std::unique_ptr<SimStock> m_stock;
};

}  // namespace CAMSimulator


#endif  // CAMSimulator_CAMSim_H
