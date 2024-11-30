# SPDX-License-Identifier: LGPL-2.1-or-later
# ***************************************************************************
# *                                                                         *
# *   Copyright (c) 2022-2023 FreeCAD Project Association                   *
# *                                                                         *
# *   This file is part of FreeCAD.                                         *
# *                                                                         *
# *   FreeCAD is free software: you can redistribute it and/or modify it    *
# *   under the terms of the GNU Lesser General Public License as           *
# *   published by the Free Software Foundation, either version 2.1 of the  *
# *   License, or (at your option) any later version.                       *
# *                                                                         *
# *   FreeCAD is distributed in the hope that it will be useful, but        *
# *   WITHOUT ANY WARRANTY; without even the implied warranty of            *
# *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU      *
# *   Lesser General Public License for more details.                       *
# *                                                                         *
# *   You should have received a copy of the GNU Lesser General Public      *
# *   License along with FreeCAD. If not, see                               *
# *   <https://www.gnu.org/licenses/>.                                      *
# *                                                                         *
# ***************************************************************************

from datetime import date, timedelta
import hashlib
import os

import addonmanager_freecad_interface as fci
import addonmanager_utilities as utils

translate = fci.translate


def local_cache_needs_update() -> bool:
    """Determine whether we need to update the cache, based on user preference, and previous
    cache update status. Returns either True or False."""

    if not _cache_exists():
        return True

    if _last_update_was_interrupted(reset_status=True):
        return True

    if _custom_repo_list_changed():
        return True

    # Figure out our cache update frequency: there is a combo box in the preferences dialog
    # with three options: never, daily, and weekly.
    days_between_updates = _days_between_updates()
    pref = fci.ParamGet("User parameter:BaseApp/Preferences/Addons")
    last_cache_update_string = pref.GetString("LastCacheUpdate", "never")

    if last_cache_update_string == "never":
        return True
    elif days_between_updates > 0:
        last_cache_update = date.fromisoformat(last_cache_update_string)
        delta_update = timedelta(days=days_between_updates)
        if date.today() >= last_cache_update + delta_update:
            return True
    elif days_between_updates == 0:
        return True

    return False


def _days_between_updates() -> int:
    pref = fci.ParamGet("User parameter:BaseApp/Preferences/Addons")
    update_frequency = pref.GetInt("UpdateFrequencyComboEntry", 0)
    if update_frequency == 0:
        return -1
    elif update_frequency == 1:
        return 1
    elif update_frequency == 2:
        return 7
    else:
        return 0


def _cache_exists() -> bool:
    cache_path = fci.getUserCachePath()
    am_path = os.path.join(cache_path, "AddonManager")
    return os.path.exists(am_path)


def _last_update_was_interrupted(reset_status: bool) -> bool:
    flag_file = utils.get_cache_file_name("CACHE_UPDATE_INTERRUPTED")
    if os.path.exists(flag_file):
        if reset_status:
            os.remove(flag_file)
        fci.Console.PrintMessage(
            translate(
                "AddonsInstaller",
                "Previous cache process was interrupted, restarting...\n",
            )
        )
        return True


def _custom_repo_list_changed() -> bool:
    pref = fci.ParamGet("User parameter:BaseApp/Preferences/Addons")
    stored_hash = pref.GetString("CustomRepoHash", "")
    custom_repos = pref.GetString("CustomRepositories", "")
    if custom_repos:
        hasher = hashlib.sha1()
        hasher.update(custom_repos.encode("utf-8"))
        new_hash = hasher.hexdigest()
    else:
        new_hash = ""
    if new_hash != stored_hash:
        pref.SetString("CustomRepoHash", new_hash)
        fci.Console.PrintMessage(
            translate(
                "AddonsInstaller",
                "Custom repo list changed, forcing recache...\n",
            )
        )
        return True
    return False
