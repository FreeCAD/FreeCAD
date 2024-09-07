/***************************************************************************
 *   Copyright (c) 2024 Benjamin Bræstrup Sayoc <benj5378@outlook.com>     *
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
#include <vector>
#endif


#include <Gui/Command.h>
#include <Gui/Selection.h>
#include <Mod/TechDraw/App/DrawViewPart.h>

#include "CommandUtil.h"


using namespace TechDrawGui;



TechDraw::DrawViewPart* CommandUtil::getDrawViewPart(Gui::Command* cmd)
{
    std::vector<TechDraw::DrawViewPart*> dvps = getDrawViewParts(cmd);
    if(dvps.empty()) {
        return nullptr;
    }
    return dvps.front();
}

// Consider making this a template
std::vector<TechDraw::DrawViewPart*> CommandUtil::getDrawViewParts(Gui::Command* cmd)
{
    std::vector<App::DocumentObject*> baseObjs =
        cmd->getSelection().getObjectsOfType(TechDraw::DrawViewPart::getClassTypeId());
    if (baseObjs.empty()) {
        return std::vector<TechDraw::DrawViewPart*>();
    }

    std::vector<TechDraw::DrawViewPart*> dvps;
    for(App::DocumentObject* &obj : baseObjs) {
        dvps.push_back(static_cast<TechDraw::DrawViewPart*>(obj));
    }
    return dvps;
}
