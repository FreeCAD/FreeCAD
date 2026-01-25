// SPDX-License-Identifier: LGPL-2.0-or-later

/***************************************************************************
 *   Copyright (c) 2024 WandererFan <wandererfan@gmail.com>                *
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



#pragma once

#include <string>
#include <vector>

#include <Mod/TechDraw/TechDrawGlobal.h>
#include <Base/Vector3D.h>

namespace App {
class DocumentObject;
}

namespace Gui {
class Command;
}

namespace TechDraw {
class DrawView;
class DrawViewPart;

/**
 * CommandHelpers is a collection of methods for common actions in commands.
 */
namespace CommandHelpers {
TechDraw::DrawView* firstViewInSelection(Gui::Command* cmd);
TechDraw::DrawView* firstNonSpreadsheetInSelection(Gui::Command* cmd);

std::vector<std::string> getSelectedSubElements(Gui::Command* cmd,
                                                TechDraw::DrawViewPart* &dvp,
                                                std::string subType = "Edge");


std::pair<App::DocumentObject*, std::string> faceFromSelection();


}   // end namespace CommandHelpers
}   // end namespace TechDraw