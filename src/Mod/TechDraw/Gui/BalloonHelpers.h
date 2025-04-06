// SPDX-License-Identifier: LGPL-2.0-or-later

/***************************************************************************
 *   Copyright (c) 2025 WandererFan <wandererfan@gmail.com>                *
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

//! BalloonHelpers is a collection of support methods for balloon command


#ifndef BALLOONHELPERS_H
#define BALLOONHELPERS_H

#include <string>
#include <vector>

#include <QPointF>

#include <Mod/TechDraw/TechDrawGlobal.h>
#include <Base/Vector3D.h>

namespace App {
class DocumentObject;
}

namespace Gui {
class Command;
}

namespace TechDrawGui {
class QGIView;
}

namespace TechDraw {
class DrawView;
class DrawViewPart;

namespace BalloonHelpers {

    bool checkSelectionBalloon(Gui::Command* cmd, unsigned maxObjs);
    bool checkDrawViewPartBalloon(Gui::Command* cmd);
    bool checkDirectPlacement(const TechDrawGui::QGIView* view, const std::vector<std::string>& subNames,
                           QPointF& placement);

}   // end namespace BalloonHelpers
}   // end namespace TechDraw

#endif

