// SPDX-License-Identifier: LGPL-2.1-or-later

/***************************************************************************
 *   Copyright (c) 2019 Zheng Lei <realthunder.dev@gmail.com>              *
 *                                                                         *
 *   This file is part of FreeCAD.                                         *
 *                                                                         *
 *   FreeCAD is free software: you can redistribute it and/or modify it    *
 *   under the terms of the GNU Lesser General Public License as           *
 *   published by the Free Software Foundation, either version 2.1 of the  *
 *   License, or (at your option) any later version.                       *
 *                                                                         *
 *   FreeCAD is distributed in the hope that it will be useful, but        *
 *   WITHOUT ANY WARRANTY; without even the implied warranty of            *
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU      *
 *   Lesser General Public License for more details.                       *
 *                                                                         *
 *   You should have received a copy of the GNU Lesser General Public      *
 *   License along with FreeCAD. If not, see                               *
 *   <https://www.gnu.org/licenses/>.                                      *
 *                                                                         *
 **************************************************************************/


#ifndef GUI_SELECTION_BOX_SELECTION_H
#define GUI_SELECTION_BOX_SELECTION_H

#include <memory>
#include <QCursor>
#include <Base/Tools2D.h>
#include <Base/ViewProj.h>
#include <Gui/View3DInventorViewer.h>


namespace Gui
{

class GuiExport BoxSelection
{
    using SelectionMode = enum
    {
        CENTER,
        INTERSECT
    };

private:
    App::Document* doc;
    Gui::View3DInventorViewer* viewer;
    struct Select
    {
        ViewProviderDocumentObject* vp;
        SelectionMode mode;
        bool selectElement;
        // Base::ViewProjMethod proj;
        Base::Polygon2d polygon;
        Base::Matrix4D mat;
        bool transform = true;
        int depth = 0;
    };
    std::vector<std::string> getBoxSelection(
        ViewProviderDocumentObject* vp,
        SelectionMode mode,
        bool selectElement,
        const Base::ViewProjMethod& proj,
        const Base::Polygon2d& polygon,
        const Base::Matrix4D& mat,
        bool transform = true,
        int depth = 0
    );

public:
    BoxSelection(App::Document* doc, Gui::View3DInventorViewer* viewer);
    void perform(bool selectElement);
};
}  // namespace Gui

#endif  // GUI_SELECTION_BOX_SELECTION_H
