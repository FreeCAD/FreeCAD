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

#ifndef _PreComp_

#endif

#include <App/Document.h>
#include <Base/Console.h>
#include <Gui/Application.h>
#include <Gui/Document.h>
#include <Gui/MDIView.h>
#include <Gui/View3DInventor.h>
#include <Gui/View3DInventorViewer.h>

#include "Grabber3d.h"

using namespace TechDrawGui;
using namespace Gui;

void Grabber3d::quickView(App::Document* appDoc,
                          const QColor bgColor,
                          QImage &image)
{
//    Base::Console().Message("G3d::quickView() - %d x %d\n", outWidth, outHeight);
    //get the active view
    Gui::Document* guiDoc = Gui::Application::Instance->getDocument(appDoc);
    Gui::MDIView* mdiView = guiDoc->getActiveView();
    if (mdiView == nullptr) {
        Base::Console().Warning("G3d::quickView - no ActiveView - returning\n");
        return;
    }

    View3DInventor* view3d = qobject_cast<View3DInventor*>(mdiView);
    if (view3d == nullptr) {
        Base::Console().Warning("G3d::quickView - no viewer for ActiveView - returning\n");
        return;
    }

    View3DInventorViewer* viewer = view3d->getViewer();
    if (viewer == nullptr) {
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

