// SPDX-License-Identifier: LGPL-2.1-or-later
// SPDX-FileNotice: Part of the FreeCAD project.

/******************************************************************************
 *                                                                            *
 *   © 2019 Jean-Marie Verdun <jmverdun3@gmail.com>                           *
 *                                                                            *
 *   FreeCAD is free software: you can redistribute it and/or modify          *
 *   it under the terms of the GNU Lesser General Public License as           *
 *   published by the Free Software Foundation, either version 2.1            *
 *   of the License, or (at your option) any later version.                   *
 *                                                                            *
 *   FreeCAD is distributed in the hope that it will be useful,               *
 *   but WITHOUT ANY WARRANTY; without even the implied warranty              *
 *   of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.                  *
 *   See the GNU Lesser General Public License for more details.              *
 *                                                                            *
 *   You should have received a copy of the GNU Lesser General Public         *
 *   License along with FreeCAD. If not, see https://www.gnu.org/licenses     *
 *                                                                            *
 ******************************************************************************/


#ifndef Cloud_WORKBENCH_H
#define Cloud_WORKBENCH_H

#include <Gui/Workbench.h>

namespace CloudGui
{

class Workbench: public Gui::StdWorkbench
{
    TYPESYSTEM_HEADER();

public:
    Workbench();
    virtual ~Workbench();

protected:
    //    Gui::MenuItem* setupMenuBar() const;
    //    Gui::ToolBarItem* setupToolBars() const;
};

}  // namespace CloudGui


#endif  // Cloud_WORKBENCH_H
