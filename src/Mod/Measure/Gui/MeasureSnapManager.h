// SPDX-License-Identifier: LGPL-2.1-or-later

/***************************************************************************
 *   Copyright (c) 2026 Krrish777 <777krrish[at]gmail.com>                 *
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


#pragma once

#include <memory>


namespace Gui
{
class SelectionChanges;
}

namespace MeasureGui
{

class MeasureSnapIndicator;

// Owns the snap preview and reacts to hover events routed from the measure task dialog.
class MeasureSnapManager
{
public:
    MeasureSnapManager();
    ~MeasureSnapManager();

    // Called once per element entry/leave. Must not throw into the selection machinery.
    void onPreselect(const Gui::SelectionChanges& msg);

private:
    std::unique_ptr<MeasureSnapIndicator> mIndicator;
};

}  // namespace MeasureGui
