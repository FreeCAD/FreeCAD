# SPDX-License-Identifier: LGPL-2.1-or-later

# Ensure default values are set in defines if they are not already provided
defines.setdefault('containing_folder', '.')
defines.setdefault('app_name', 'FreeCAD.app')
defines.setdefault('icon_path', 'Contents/Resources/freecad.icns')


files = [f"{defines['containing_folder']}/{defines['app_name']}"]
symlinks = {"Applications": "/Applications"}
badge_icon = f"{defines['containing_folder']}/{defines['app_name']}/{defines['icon_path']}"
window_rect = ((200, 200), (600, 400))
icon_locations = {f"{defines['app_name']}": (180, 150), "Applications": (420, 150)}
size = "4g"
