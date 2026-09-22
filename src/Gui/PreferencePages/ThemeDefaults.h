// SPDX-License-Identifier: LGPL-2.1-or-later

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
