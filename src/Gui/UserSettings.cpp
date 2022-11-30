/***************************************************************************
 *   Copyright (c) 2022 Werner Mayer <wmayer[at]users.sourceforge.net>     *
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
#endif


#include "UserSettings.h"
#include <App/Application.h>


using namespace Gui;

namespace {

ParameterGrp::handle getWSParameter()
{
    return App::GetApplication().GetParameterGroupByPath("User parameter:BaseApp/Preferences/MainWindow");
}

}

std::string WorkbenchSwitcher::getValue()
{
    return getWSParameter()->GetASCII("WSPosition", "WSToolbar");
}

bool WorkbenchSwitcher::isLeftCorner(const std::string& value)
{
    return (value == "WSLeftCorner");
}

bool WorkbenchSwitcher::isRightCorner(const std::string& value)
{
    return (value == "WSRightCorner");
}

bool WorkbenchSwitcher::isToolbar(const std::string& value)
{
    return (value == "WSToolbar");
}

QVector<std::string> WorkbenchSwitcher::values()
{
    QVector<std::string> wsPositions;
    wsPositions << "WSToolbar" << "WSLeftCorner" << "WSRightCorner";
    return wsPositions;
}

int WorkbenchSwitcher::getIndex()
{
    auto hGrp = getWSParameter();
    std::string pos = hGrp->GetASCII("WSPosition", "WSToolbar");
    auto wsPositions = values();
    int index = std::max(0, static_cast<int>(wsPositions.indexOf(pos)));
    return index;
}

void WorkbenchSwitcher::setIndex(int index)
{
    auto wsPositions = values();
    auto hGrp = getWSParameter();
    if (index >= 0 && index < wsPositions.size()) {
        hGrp->SetASCII("WSPosition", wsPositions[index].c_str());
    }
}
