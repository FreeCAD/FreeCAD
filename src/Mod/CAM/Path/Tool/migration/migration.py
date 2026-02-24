# SPDX-License-Identifier: LGPL-2.1-or-later
# ***************************************************************************
# *                                                                         *
# *   Copyright (c) 2025 sliptonic <shopinthewoods@gmail.com>               *
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
"""
CAM Asset Migration Module

Handles migration of CAM assets during FreeCAD version upgrades.
"""

import FreeCAD
import Path
import Path.Preferences
import pathlib
import os
import glob
from ..assets.ui import AssetOpenDialog
from ..camassets import cam_assets
from ..library.serializers import all_serializers as library_serializers
from ..library.models import Library

# Logging setup - same pattern as Job.py
if False:
    Path.Log.setLevel(Path.Log.Level.DEBUG, Path.Log.thisModule())
    Path.Log.trackModule(Path.Log.thisModule())
else:
    Path.Log.setLevel(Path.Log.Level.ERROR, Path.Log.thisModule())

if FreeCAD.GuiUp:
    import FreeCADGui
    from PySide.QtWidgets import QApplication, QMessageBox
    from PySide.QtCore import Qt


class CAMAssetMigrator:
    """
    Handles migration of CAM assets during FreeCAD version upgrades.

    This class provides functionality to:
    - Check if migration is needed for custom CAM asset locations
    - Offer migration to users through a dialog
    - Perform the actual asset migration with versioned directories
    """

    def __init__(self):
        self.pref_group_path = "User parameter:BaseApp/Preferences/Mod/CAM/Migration"

    def check_migration_needed(self):
        self.check_asset_location()
        self.check_tool_library_workdir()

    def check_asset_location(self):
        """
        Check if CAM asset migration is needed for version upgrade.

        This method determines if the current CAM assets are stored in a custom
        location outside the default user data directory and if migration has
        not been offered for the current FreeCAD version.
        """
        Path.Log.debug("Starting CAM asset migration check")

        try:
            # Get current directories
            user_app_data_dir = FreeCAD.getUserAppDataDir()
            user_app_data_path = pathlib.Path(user_app_data_dir)
            Path.Log.debug(f"User app data directory: {user_app_data_dir}")

            # Get the current CAM asset path (may be naked or versioned)
            current_asset_path = Path.Preferences.getAssetPath()
            current_asset_pathlib = pathlib.Path(current_asset_path)
            Path.Log.debug(f"Current CAM asset path: {current_asset_path}")

            # Only migrate if CamAssets is outside the standard user data directory
            if current_asset_pathlib.is_relative_to(user_app_data_path):
                Path.Log.debug("CamAssets is in default location, no custom migration needed")
                return

            # Check if migration has already been offered for this version
            if self.has_migration_been_offered():
                Path.Log.debug("Migration has already been offered for this version, skipping")
                return

            # Determine the base path (naked path without version)
            if FreeCAD.ApplicationDirectories.isVersionedPath(str(current_asset_path)):
                # Check if we're already using the current version
                if FreeCAD.ApplicationDirectories.usingCurrentVersionConfig(
                    str(current_asset_path)
                ):
                    Path.Log.debug("Already using current version, no migration needed")
                    return

            Path.Log.info("Asset relocation is needed and should be offered")
            if self._offer_asset_relocation():
                self._migrate_assets(str(current_asset_path))
            return

        except Exception as e:
            Path.Log.error(f"Error checking CAM asset migration: {e}")
            import traceback

            Path.Log.info(f"Full traceback: {traceback.format_exc()}")
            return

    def check_tool_library_workdir(self):
        workdir_str = "LastPathToolLibrary"
        migrated_str = "Migrated" + workdir_str
        workdir = Path.Preferences.preferences().GetString(workdir_str)
        migrated_dir = Path.Preferences.preferences().GetString(migrated_str)
        Path.Log.debug(f"workdir: {workdir}, migrated: {migrated_dir}")
        if workdir and not migrated_dir:
            # Look for tool libraries to import
            if os.path.isdir(workdir):
                libraries = [f for f in glob.glob(workdir + os.path.sep + "*.fctl")]
                libraries.sort()
                if len(libraries):
                    # Migrate libraries, automatically and silently
                    Path.Log.info("Migrating tool libraries into CAM assets")
                    for library in libraries:
                        Path.Log.info("Migrating " + library)
                        import_dialog = AssetOpenDialog(
                            cam_assets,
                            asset_class=Library,
                            serializers=library_serializers,
                            parent=None,
                        )
                        asset = import_dialog.deserialize_file(pathlib.Path(library), quiet=True)
                        if asset:
                            cam_assets.add(asset)

                    # Mark directory as migrated
                    Path.Preferences.preferences().SetString(migrated_str, workdir)

    def _offer_asset_relocation(self):
        """
        Present asset relocation dialog to user.

        Returns:
            bool: True if user accepted relocation, False otherwise
        """
        # Get current version info
        major = int(FreeCAD.ConfigGet("BuildVersionMajor"))
        minor = int(FreeCAD.ConfigGet("BuildVersionMinor"))
        current_version = FreeCAD.ApplicationDirectories.versionStringForPath(major, minor)

        # Get current asset path for display
        current_asset_path = Path.Preferences.getAssetPath()

        Path.Log.debug(f"Offering asset relocation to user for version {current_version}")

        if not FreeCAD.GuiUp:
            Path.Log.debug("GUI not available, skipping migration offer")
            return False

        msg = (
            f"FreeCAD has been upgraded to version {current_version}.\n\n"
            f"Your CAM assets are stored in a custom location:\n{current_asset_path}\n\n"
            "Would you like to migrate your CAM assets to a versioned directory "
            "to preserve them during future upgrades?\n\n"
            "This will copy your assets to a new directory."
        )

        Path.Log.debug("Showing asset relocation dialog to user")

        reply = QMessageBox.question(
            None, "CAM Asset Migration", msg, QMessageBox.Yes | QMessageBox.No, QMessageBox.Yes
        )

        # Record that we offered migration for this version
        pref_group = FreeCAD.ParamGet(self.pref_group_path)
        offered_versions = pref_group.GetString("OfferedToMigrateCAMAssets", "")
        known_versions = set(offered_versions.split(",")) if offered_versions else set()
        known_versions.add(current_version)
        pref_group.SetString("OfferedToMigrateCAMAssets", ",".join(known_versions))
        Path.Log.debug(f"Updated offered versions: {known_versions}")

        if reply == QMessageBox.Yes:
            Path.Log.info("User accepted migration, starting asset migration")
            return True
        else:
            Path.Log.info("User declined migration")
            return False

    def _migrate_assets(self, source_path):
        """
        Perform actual directory copying and preference updates.

        Args:
            source_path: Current CAM asset directory path
        """
        Path.Log.info(f"Starting asset migration from {source_path}")

        try:
            FreeCAD.ApplicationDirectories.migrateAllPaths([source_path])
            Path.Log.info(
                "Migration complete - preferences will be handled automatically by the system"
            )

            if FreeCAD.GuiUp:
                QMessageBox.information(
                    None,
                    "Migration Complete",
                    f"CAM assets have been migrated from:\n{source_path}\n\n"
                    "The system will automatically handle preference updates.",
                )

        except Exception as e:
            error_msg = f"Failed to migrate CAM assets: {e}"
            Path.Log.error(error_msg)
            import traceback

            Path.Log.debug(f"Migration error traceback: {traceback.format_exc()}")
            if FreeCAD.GuiUp:
                QMessageBox.critical(None, "Migration Failed", error_msg)

    def has_migration_been_offered(self):
        """
        Check if migration has been offered for current version.

        Returns:
            bool: True if migration was offered for this version
        """

        pref_group = FreeCAD.ParamGet(self.pref_group_path)
        major = int(FreeCAD.ConfigGet("BuildVersionMajor"))
        minor = int(FreeCAD.ConfigGet("BuildVersionMinor"))

        current_version_string = FreeCAD.ApplicationDirectories.versionStringForPath(major, minor)
        offered_versions = pref_group.GetString("OfferedToMigrateCAMAssets", "")
        known_versions = set(offered_versions.split(",")) if offered_versions else set()
        return current_version_string in known_versions
