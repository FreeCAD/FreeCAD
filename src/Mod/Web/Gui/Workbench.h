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


#ifndef WEBGUI_WORKBENCH_H
#define WEBGUI_WORKBENCH_H

#include <Gui/Workbench.h>

namespace WebGui
{

/**
 * @author Werner Mayer
 */
class Workbench: public Gui::StdWorkbench
{
    TYPESYSTEM_HEADER_WITH_OVERRIDE();

public:
    Workbench();
    ~Workbench() override;

    /** Defines the standard context menu. */
    void setupContextMenu(const char* recipient, Gui::MenuItem*) const override;

protected:
    /** Defines the standard menus. */
    Gui::MenuItem* setupMenuBar() const override;
    /** Defines the standard toolbars. */
    Gui::ToolBarItem* setupToolBars() const override;
    /** Defines the standard command bars. */
    Gui::ToolBarItem* setupCommandBars() const override;
    /** Returns a DockWindowItems structure of dock windows this workbench. */
    Gui::DockWindowItems* setupDockWindows() const override;


};  // namespace WebGui

}  // namespace WebGui
#endif  // WEB_WORKBENCH_H
