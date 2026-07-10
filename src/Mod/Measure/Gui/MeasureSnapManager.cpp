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


#include <Base/Console.h>
#include <Base/Exception.h>
#include <Gui/Selection/Selection.h>

#include "MeasureSnapManager.h"
#include "MeasureSnapIndicator.h"

using namespace MeasureGui;

MeasureSnapManager::MeasureSnapManager()
    : mIndicator(std::make_unique<MeasureSnapIndicator>())
{}

MeasureSnapManager::~MeasureSnapManager() = default;

void MeasureSnapManager::onPreselect(const Gui::SelectionChanges& msg)
{
    try {
        (void)msg;
    }
    catch (const Base::Exception& e) {
        Base::Console().log("MeasureSnapManager: %s\n", e.what());
    }
    catch (...) {
        Base::Console().log("MeasureSnapManager: unexpected error on preselect\n");
    }
}
