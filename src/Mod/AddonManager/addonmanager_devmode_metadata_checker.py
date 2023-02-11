# ***************************************************************************
# *                                                                         *
# *   Copyright (c) 2022 FreeCAD Project Association                        *
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

""" Metadata validation functions """

from typing import List

import FreeCAD

from Addon import Addon
import NetworkManager


class MetadataValidators:
    """A collection of tools for validating various pieces of metadata. Prints validation
    information to the console."""

    def validate_all(self, repos):
        """Developer tool: check all repos for validity and print report"""

        FreeCAD.Console.PrintMessage("\n\nADDON MANAGER DEVELOPER MODE CHECKS\n")
        FreeCAD.Console.PrintMessage("-----------------------------------\n")

        counter = 0
        for addon in repos:
            counter += 1
            if addon.metadata is not None:
                self.validate_package_xml(addon)
            elif addon.repo_type == Addon.Kind.MACRO:
                if addon.macro.parsed:
                    if len(addon.macro.icon) == 0 and len(addon.macro.xpm) == 0:
                        FreeCAD.Console.PrintMessage(
                            f"Macro '{addon.name}' does not have an icon\n"
                        )
            else:
                FreeCAD.Console.PrintMessage(
                    f"Addon '{addon.name}' does not have a package.xml file\n"
                )

        FreeCAD.Console.PrintMessage("-----------------------------------\n\n")

    def validate_package_xml(self, addon: Addon):
        """Check the package.xml file for the specified Addon"""
        if addon.metadata is None:
            return

        # The package.xml standard has some required elements that the basic XML reader is not
        # actually checking for. In developer mode, actually make sure that all of the rules are
        # being followed for each element.

        errors = []

        errors.extend(self.validate_top_level(addon))
        errors.extend(self.validate_content(addon))

        if len(errors) > 0:
            FreeCAD.Console.PrintError(
                f"Errors found in package.xml file for '{addon.name}'\n"
            )
            for error in errors:
                FreeCAD.Console.PrintError(f"   * {error}\n")

    def validate_content(self, addon: Addon) -> List[str]:
        """Validate the Content items for this Addon"""
        errors = []
        contents = addon.metadata.Content

        missing_icon = True
        if addon.metadata.Icon and len(addon.metadata.Icon) > 0:
            missing_icon = False
        else:
            if "workbench" in contents:
                wb = contents["workbench"][0]
                if wb.Icon:
                    missing_icon = False
        if missing_icon:
            errors.append("No <icon> element found, or <icon> element is invalid")

        if "workbench" in contents:
            for wb in contents["workbench"]:
                errors.extend(self.validate_workbench_metadata(wb))

        if "preferencepack" in contents:
            for wb in contents["preferencepack"]:
                errors.extend(self.validate_preference_pack_metadata(wb))

        return errors

    def validate_top_level(self, addon) -> List[str]:
        """Check for the presence of the required top-level elements"""
        errors = []
        if not addon.metadata.Name or len(addon.metadata.Name) == 0:
            errors.append(
                "No top-level <name> element found, or <name> element is empty"
            )
        if not addon.metadata.Version or addon.metadata.Version == "0.0.0":
            errors.append(
                "No top-level <version> element found, or <version> element is invalid"
            )
        # if not addon.metadata.Date or len(addon.metadata.Date) == 0:
        #    errors.append(f"No top-level <date> element found, or <date> element is invalid")
        if not addon.metadata.Description or len(addon.metadata.Description) == 0:
            errors.append(
                "No top-level <description> element found, or <description> element is invalid"
            )

        maintainers = addon.metadata.Maintainer
        if len(maintainers) == 0:
            errors.append("No top-level <maintainers> found, at least one is required")
        for maintainer in maintainers:
            if len(maintainer["email"]) == 0:
                errors.append(
                    f"No email address specified for maintainer '{maintainer['name']}'"
                )

        licenses = addon.metadata.License
        if len(licenses) == 0:
            errors.append("No top-level <license> found, at least one is required")

        urls = addon.metadata.Urls
        errors.extend(self.validate_urls(urls))
        return errors

    def validate_urls(self, urls) -> List[str]:
        """Check the URLs provided by the addon"""
        errors = []
        if len(urls) == 0:
            errors.append(
                "No <url> elements found, at least a repo url must be provided"
            )
        else:
            found_repo = False
            found_readme = False
            for url in urls:
                if url["type"] == "repository":
                    found_repo = True
                    if len(url["branch"]) == 0:
                        errors.append(
                            "<repository> element is missing the 'branch' attribute"
                        )
                elif url["type"] == "readme":
                    found_readme = True
                    location = url["location"]
                    p = NetworkManager.AM_NETWORK_MANAGER.blocking_get(location)
                    if not p:
                        errors.append(
                            f"Could not access specified readme at {location}"
                        )
                    else:
                        p = p.data().decode("utf8")
                        if "<html" in p or "<!DOCTYPE html>" in p:
                            pass
                        else:
                            errors.append(
                                f"Readme data found at {location}"
                                " does not appear to be rendered HTML"
                            )
            if not found_repo:
                errors.append("No repo url specified")
            if not found_readme:
                errors.append(
                    "No readme url specified (not required, but highly recommended)"
                )
        return errors

    def validate_workbench_metadata(self, workbench) -> List[str]:
        """Validate the required element(s) for a workbench"""
        errors = []
        if not workbench.Classname or len(workbench.Classname) == 0:
            errors.append("No <classname> specified for workbench")
        return errors

    def validate_preference_pack_metadata(self, pack) -> List[str]:
        """Validate the required element(s) for a preference pack"""
        errors = []
        if not pack.Name or len(pack.Name) == 0:
            errors.append("No <name> specified for preference pack")
        return errors
