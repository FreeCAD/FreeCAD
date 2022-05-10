/***************************************************************************
 *   Copyright (c) 2008 Werner Mayer <wmayer[at]users.sourceforge.net>     *
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


#ifndef SKETCHER_WORKBENCH_H
#define SKETCHER_WORKBENCH_H

#include <Gui/Workbench.h>
#include <Gui/MenuManager.h>
#include <Gui/ToolBarManager.h>
#include <Mod/Sketcher/SketcherGlobal.h>

namespace SketcherGui {

/**
 * @author Werner Mayer
 */
class SketcherGuiExport Workbench : public Gui::StdWorkbench
{
    TYPESYSTEM_HEADER();

public:
    Workbench();
    virtual ~Workbench();

protected:
    Gui::MenuItem* setupMenuBar() const;
    Gui::ToolBarItem* setupToolBars() const;
    Gui::ToolBarItem* setupCommandBars() const;
};

SketcherGuiExport void addSketcherWorkbenchSketchActions(Gui::MenuItem& sketch);
SketcherGuiExport void addSketcherWorkbenchGeometries(Gui::MenuItem& geom);
SketcherGuiExport void addSketcherWorkbenchConstraints(Gui::MenuItem& cons);
SketcherGuiExport void addSketcherWorkbenchTools(Gui::MenuItem& consaccel);
SketcherGuiExport void addSketcherWorkbenchBSplines(Gui::MenuItem& bspline);
SketcherGuiExport void addSketcherWorkbenchVirtualSpace(Gui::MenuItem& virtualspace);

SketcherGuiExport void addSketcherWorkbenchSketchActions(Gui::ToolBarItem& sketch);
SketcherGuiExport void addSketcherWorkbenchGeometries(Gui::ToolBarItem& geom);
SketcherGuiExport void addSketcherWorkbenchConstraints(Gui::ToolBarItem& cons);
SketcherGuiExport void addSketcherWorkbenchTools(Gui::ToolBarItem& consaccel);
SketcherGuiExport void addSketcherWorkbenchBSplines(Gui::ToolBarItem& bspline);
SketcherGuiExport void addSketcherWorkbenchVirtualSpace(Gui::ToolBarItem& virtualspace);

} // namespace SketcherGui

#endif // SKETCHER_WORKBENCH_H
