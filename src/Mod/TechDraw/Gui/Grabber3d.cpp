/***************************************************************************
 *   Copyright (c) 2019 WandererFan <wandererfan@gmail.com>                *
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
#include <Gui/View3DInventor.h>
#include <Gui/View3DInventorViewer.h>
#include <Base/Console.h>
#include <Gui/MainWindow.h>
#include <Gui/MDIView.h>

#include "Grabber3d.h"

using namespace TechDrawGui;
using namespace Gui;

void Grabber3d::quickView(View3DInventor* view3d,
                          const QColor bgColor,
                          QImage &image)
{
//    Base::Console().Message("G3d::quickView());
    if (!Gui::getMainWindow()) {
        //this should already be checked in the caller
        Base::Console().Warning("G3d::quickView - no Main Window - returning\n");
        return;
    }

    if (!view3d) {
        //this should also already be checked in the caller
        Base::Console().Warning("G3d::quickView - no 3D view for ActiveView - returning\n");
        return;
    }

    View3DInventorViewer* viewer = view3d->getViewer();
    if (!viewer) {
        Base::Console().Warning("G3d::quickView - could not create viewer - returning\n");
        return;
    }
    //figure out the size of the active MdiView
    SbViewportRegion vport(viewer->getSoRenderManager()->getViewportRegion());
    SbVec2s vpSize = vport.getViewportSizePixels();
    short width;
    short height;
    vpSize.getValue(width, height);

    int samples = 8;  //magic number from Gui::View3DInventorViewer
    viewer->savePicture(width, height, samples, bgColor, image);
}

