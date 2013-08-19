/***************************************************************************
 *   Copyright (c) 2008 Werner Mayer <werner.wm.mayer@gmx.de>              *
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

#ifndef _PreComp_
# include <qobject.h>
#endif

#include "Workbench.h"
#include <Gui/ToolBarManager.h>
#include <Gui/MenuManager.h>


using namespace FemGui;

#if 0 // needed for Qt's lupdate utility
    qApp->translate("Workbench", "FEM");
    qApp->translate("Workbench", "&FEM");
#endif

/// @namespace FemGui @class Workbench
TYPESYSTEM_SOURCE(FemGui::Workbench, Gui::StdWorkbench)

Workbench::Workbench()
{
}

Workbench::~Workbench()
{
}

Gui::ToolBarItem* Workbench::setupToolBars() const
{
    Gui::ToolBarItem* root = StdWorkbench::setupToolBars();
    Gui::ToolBarItem* fem = new Gui::ToolBarItem(root);
    fem->setCommand("FEM");
     *fem << "Fem_CreateFromShape"
          << "Fem_NewMechanicalAnalysis"
          << "Fem_FemAddPart"
          << "Fem_CreateNodesSet"
          << "Fem_ConstraintFixed"
          << "Fem_ConstraintForce"
          << "Fem_ConstraintBearing"
          << "Fem_ConstraintGear"   
          << "Fem_ConstraintPulley";
    return root;
}

Gui::MenuItem* Workbench::setupMenuBar() const
{
    Gui::MenuItem* root = StdWorkbench::setupMenuBar();
    Gui::MenuItem* item = root->findItem("&Windows");
    Gui::MenuItem* fem = new Gui::MenuItem;
    root->insertItem(item, fem);
    fem->setCommand("&FEM");
    *fem << "Fem_CreateFromShape"
		 << "Fem_MechanicalMaterial"
		 << "Fem_NewMechanicalAnalysis"
		 << "Fem_MechanicalJobControl"
         << "Fem_CreateNodesSet"
	     << "Fem_ConstraintFixed"
         << "Fem_ConstraintForce"
         << "Fem_ConstraintBearing"
         << "Fem_ConstraintGear"   
         << "Fem_ConstraintPulley";

    return root;
}
