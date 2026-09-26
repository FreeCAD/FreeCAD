// SPDX-License-Identifier: LGPL-2.1-or-later
/****************************************************************************
 *                                                                          *
 *   Copyright (c) 2026 Chris Jones <chris.r.jones.1983@gmail.com>          *
 *                                                                          *
 *   This file is part of FreeCAD.                                          *
 *                                                                          *
 *   FreeCAD is free software: you can redistribute it and/or modify it     *
 *   under the terms of the GNU Lesser General Public License as            *
 *   published by the Free Software Foundation, either version 2.1 of the   *
 *   License, or (at your option) any later version.                        *
 *                                                                          *
 *   FreeCAD is distributed in the hope that it will be useful, but         *
 *   WITHOUT ANY WARRANTY; without even the implied warranty of             *
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU       *
 *   Lesser General Public License for more details.                        *
 *                                                                          *
 *   You should have received a copy of the GNU Lesser General Public       *
 *   License along with FreeCAD. If not, see                                *
 *   <https://www.gnu.org/licenses/>.                                       *
 *                                                                          *
 ***************************************************************************/

#pragma once

#include <string>
#include <vector>

#include <FCGlobal.h>

namespace Gui::ThemeDefaults
{
/// Copy the listed unsigned (color) keys from the active theme's preference pack
/// into the user's config. groupPath is relative to the root,
/// e.g. "BaseApp/Preferences/View". Keys the theme doesn't define are left alone.
GuiExport void applyColors(const std::string& groupPath, const std::vector<std::string>& keys);

/// Remove the listed unsigned keys from the user's config.
GuiExport void removeColors(const std::string& groupPath, const std::vector<std::string>& keys);

}  // namespace Gui::ThemeDefaults
