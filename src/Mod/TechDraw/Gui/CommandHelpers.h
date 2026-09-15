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
#include <Mod/TechDraw/App/LineFormat.h>
#include <Mod/TechDraw/App/Cosmetic.h>
#include <Mod/TechDraw/App/CenterLine.h>

#include <Gui/Selection/SelectionObject.h>

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

namespace TechDrawGui
{
//TechDraw::LineFormat activeAttributes; // container holding global line attributes

//internal helper functions
TechDraw::LineFormat& _getActiveLineAttributes();
Base::Vector3d _circleCenter(Base::Vector3d p1, Base::Vector3d p2, Base::Vector3d p3);
void _createThreadCircle(const std::string Name, TechDraw::DrawViewPart* objFeat, double factor);
void _createThreadLines(const std::vector<std::string>& SubNames, TechDraw::DrawViewPart* objFeat,
                        double factor, bool endLine);
void _setLineAttributes(TechDraw::CosmeticEdge* cosEdge);
void _setLineAttributes(TechDraw::CenterLine* cosEdge);
void _setLineAttributes(TechDraw::CosmeticEdge* cosEdge, int style, float weight, Base::Color color);
void _setLineAttributes(TechDraw::CenterLine* cosEdge, int style, float weight, Base::Color color);
double _getAngle(Base::Vector3d center, Base::Vector3d point);
std::vector<Base::Vector3d> _getVertexPoints(const std::vector<std::string>& SubNames,
                                             TechDraw::DrawViewPart* objFeat);
bool _checkSel(Gui::Command* cmd, std::vector<Gui::SelectionObject>& selection,
               TechDraw::DrawViewPart*& objFeat, const std::string& message);
std::string _createBalloon(Gui::Command* cmd, TechDraw::DrawViewPart* objFeat);

} // namespace TechDrawGui