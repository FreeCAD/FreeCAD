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

#ifndef Grabber3d_h_
#define Grabber3d_h_

#include <Mod/TechDraw/TechDrawGlobal.h>

class SoSeparator;
class SoCamera;
class SoNode;
class SbVec2s;
class SbVec2f;
class SbVec2d;
class SoVectorizeAction;

namespace App {
class Document;
class DocumentObject;
class NavigationStyle;
}
namespace Gui {
class Document;
class View3DInventorViewer;
}

#include "MDIViewPage.h"

namespace TechDraw {
}

namespace TechDrawGui
{

/// Utility functions for obtaining 3d window image
class TechDrawGuiExport Grabber3d {
    public:
    static void quickView(App::Document* appDoc,
                          int outWidth, int outHeight,
                          const QColor bgColor,
                          QImage &image);
    static double copyActiveViewToSvgFile(App::Document* appDoc, 
                                        std::string fileSpec,
                                        double outWidth = 138.5,    //TODO: change to A4 for release
                                        double outHeight = 95.0,    //ISO A5 defaults
                                        bool paintBackground = true,
                                        const QColor& bgcolor = QColor(Qt::white),
                                        double lineWidth = 1.0,     //1 mm
                                        double border = 0.0,        //no border
                                        int mode = 0);              //SoRenderManager::RenderMode(0) - AS_IS

    static SoSeparator* copySceneGraph(SoNode* sgIn);

    static void execVectorizeAction(Gui::View3DInventorViewer* viewer,
                                   SoVectorizeAction* va,
                                   double width, double height,
                                   bool paintBackground, const QColor& bgcolor,
                                   double lineWidth, double border);

    static double getViewerScale(Gui::View3DInventorViewer* viewer);
    static double getPaperScale(Gui::View3DInventorViewer* viewer,
                                double pWidth, double pHeight);

    static void postProcessSvg(std::string fileSpec);
};

} //end namespace TechDrawGui
#endif
