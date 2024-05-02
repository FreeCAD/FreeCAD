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

#include "BalloonPropEnum.h"


namespace TechDraw {

const int   BalloonPropEnum::BalloonCount = 8;
const char* BalloonPropEnum::BalloonTypeEnums[]= {
    QT_TRANSLATE_NOOP("BalloonPropEnum", "Circular"),
    QT_TRANSLATE_NOOP("BalloonPropEnum", "None"),
    QT_TRANSLATE_NOOP("BalloonPropEnum", "Triangle"),
    QT_TRANSLATE_NOOP("BalloonPropEnum", "Inspection"),
    QT_TRANSLATE_NOOP("BalloonPropEnum", "Hexagon"),
    QT_TRANSLATE_NOOP("BalloonPropEnum", "Square"),
    QT_TRANSLATE_NOOP("BalloonPropEnum", "Rectangle"),
    QT_TRANSLATE_NOOP("BalloonPropEnum", "Line"),
    nullptr};

const std::vector<std::string> BalloonPropEnum::BalloonTypeIcons = { ":icons/circular.svg",
                                            ":icons/none.svg",
                                            ":icons/triangle.svg",
                                            ":icons/inspection.svg",
                                            ":icons/hexagon.svg",
                                            ":icons/square.svg",
                                            ":icons/rectangle.svg",
                                            ":icons/bottomline.svg"};

}
