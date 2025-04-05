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

//! ViewMakers is a collection of methods for creating views on a page


#ifndef VIEWMAKERS_H
#define VIEWMAKERS_H

// #include <string>
#include <vector>
#include <QString>

#include <Mod/TechDraw/TechDrawGlobal.h>
#include <Base/Vector3D.h>

namespace App {
class DocumentObject;
}

namespace Gui {
class Command;
}

namespace TechDraw {
class DrawPage;
class DrawView;
class DrawViewPart;
class DrawViewClip;
class DrawBrokenView;
class DrawViewSection;
class DrawComplexSection;
class DrawViewSpreadsheet;
class DrawViewDraft;
class DrawViewImage;
class DrawViewArch;
class DrawViewSymbol;
class DrawProjGroup;

namespace ViewMakers {

    // TODO: the shape object vectors can be const references??
    DrawViewPart* makeShapeViewSelection(DrawPage* page);
    DrawViewPart* makeShapeView(DrawPage* page,
                            const std::vector<App::DocumentObject*>& shapeObjs,
                            const std::vector<App::DocumentObject*>& xShapeObjs);

    DrawBrokenView* makeBrokenViewSelection(DrawPage* page);
    DrawBrokenView* makeBrokenView(DrawPage* page,
                                   const std::vector<App::DocumentObject*>& shapes,
                                   const std::vector<App::DocumentObject*>& xShapes,
                                   const std::vector<App::DocumentObject*>& breakObjects);

    DrawComplexSection* makeComplexSectionSelection(DrawPage* page);
    DrawComplexSection* makeComplexSection(DrawPage* page,
                                           const std::vector<App::DocumentObject*>& shapes,
                                           const std::vector<App::DocumentObject*>& xShapes,
                                           const App::DocumentObject* profileObject,
                                           const std::vector<std::string>& profileSubs);

    DrawProjGroup* makeProjectionGroupSelection(DrawPage* page);
    DrawProjGroup* makeProjectionGroup(DrawPage* page,
                                 const std::vector<App::DocumentObject*>& shapes,
                                 const std::vector<App::DocumentObject*>& xShapes,
                                 const std::pair<Base::Vector3d, Base::Vector3d>& directions);


    DrawViewSpreadsheet*  makeSpreadsheetView(TechDraw::DrawPage* page);
    DrawViewDraft*  makeDraftView(TechDraw::DrawPage* page);
    DrawViewArch*  makeBIMView(TechDraw::DrawPage* page);
    DrawViewSymbol*  makeSymbolView(TechDraw::DrawPage* page, const QString& filename);
    DrawViewImage*  makeImageView(TechDraw::DrawPage* page, const QString& filename);

    DrawViewClip*  makeClipGroup(TechDraw::DrawPage* page);
    void  addViewToClipGroup(TechDraw::DrawViewClip* clip, TechDraw::DrawView* view);
    void  removeViewFromClipGroup(TechDraw::DrawViewClip* clip, TechDraw::DrawView* view);


}   // end namespace ViewMakers
}   // end namespace TechDraw

#endif

