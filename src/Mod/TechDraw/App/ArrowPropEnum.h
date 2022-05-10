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

#ifndef _ARROWENUMS_H_
#define _ARROWENUMS_H_

#include <vector>
#include <string>
#include <QCoreApplication>


namespace TechDraw
{

//common definitions for line ends / arrows
enum ArrowType { FILLED_ARROW = 0,
                 OPEN_ARROW,
                 TICK,
                 DOT,
                 OPEN_CIRCLE,
                 FORK,
                 FILLED_TRIANGLE,
                 NONE};

class TechDrawExport ArrowPropEnum {
    Q_DECLARE_TR_FUNCTIONS(TechDraw::ArrowPropEnum)

    public:
        static const char* ArrowTypeEnums[];
        static const int   ArrowCount;
        static const std::vector<std::string> ArrowTypeIcons;

private:

};

} //end namespace TechDraw
#endif
