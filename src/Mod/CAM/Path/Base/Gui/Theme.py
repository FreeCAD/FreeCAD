# SPDX-License-Identifier: LGPL-2.1-or-later
# ***************************************************************************
# *   Copyright (c) 2026 Billy Huddleston <connor9220@gmail.com>            *
# *                                                                         *
# *   This program is free software; you can redistribute it and/or modify  *
# *   it under the terms of the GNU Lesser General Public License (LGPL)    *
# *   as published by the Free Software Foundation; either version 2 of     *
# *   the License, or (at your option) any later version.                   *
# *   for detail see the LICENCE text file.                                 *
# *                                                                         *
# *   This program is distributed in the hope that it will be useful,       *
# *   but WITHOUT ANY WARRANTY; without even the implied warranty of        *
# *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the         *
# *   GNU Library General Public License for more details.                  *
# *                                                                         *
# *   You should have received a copy of the GNU Library General Public     *
# *   License along with this program; if not, write to the Free Software   *
# *   Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  *
# *   USA                                                                   *
# *                                                                         *
# ***************************************************************************

import FreeCAD

__title__ = "CAM UI theme helpers"
__url__ = "https://www.freecad.org"
__doc__ = "Helpers to adapt CAM artwork to the light/dark theme in use."


def get_theme_name() -> str:
    """Name of the theme currently selected in the preferences ("" if unset)."""
    return FreeCAD.ParamGet("User parameter:BaseApp/Preferences/MainWindow").GetString("Theme", "")


def is_dark_theme() -> bool:
    """True when a dark theme is active."""
    theme = get_theme_name()
    return "dark" in theme.lower() if theme else False
