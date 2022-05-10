/***************************************************************************
 *   Copyright (c) 2020 WandererFan <wandererfan@gmail.com>                *
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

#include "ArrowPropEnum.h"

namespace TechDraw {

const int   ArrowPropEnum::ArrowCount = 8;
const char* ArrowPropEnum::ArrowTypeEnums[]= { "Filled Arrow",
                               "Open Arrow",
                               "Tick",
                               "Dot",
                               "Open Circle",
                               "Fork",
                               "Filled Triangle",
                               "None",
                               nullptr};

const std::vector<std::string> ArrowPropEnum::ArrowTypeIcons = { ":icons/arrowfilled.svg",
                                            ":icons/arrowopen.svg",
                                            ":icons/arrowtick.svg",
                                            ":icons/arrowdot.svg",
                                            ":icons/arrowopendot.svg",
                                            ":icons/arrowfork.svg",
                                            ":icons/arrowpyramid.svg",
                                            ":icons/arrownone.svg"};

}
