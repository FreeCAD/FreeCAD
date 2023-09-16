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

""" A collection of functions to handle installing a macro icon to the toolbar. """

import os

import FreeCAD
import FreeCADGui
from PySide import QtCore, QtWidgets
import Addon

translate = FreeCAD.Qt.translate


def ask_to_install_toolbar_button(repo: Addon) -> None:
    """Presents a dialog to the user asking if they want to install a toolbar button for
    a particular macro, and walks through that process if they agree to do so."""
    pref = FreeCAD.ParamGet("User parameter:BaseApp/Preferences/Addons")
    do_not_show_dialog = pref.GetBool("dontShowAddMacroButtonDialog", False)
    button_exists = check_for_button(repo)
    if not do_not_show_dialog and not button_exists:
        add_toolbar_button_dialog = FreeCADGui.PySideUic.loadUi(
            os.path.join(os.path.dirname(__file__), "add_toolbar_button_dialog.ui")
        )
        add_toolbar_button_dialog.setWindowFlag(QtCore.Qt.WindowStaysOnTopHint, True)
        add_toolbar_button_dialog.buttonYes.clicked.connect(lambda: install_toolbar_button(repo))
        add_toolbar_button_dialog.buttonNever.clicked.connect(
            lambda: pref.SetBool("dontShowAddMacroButtonDialog", True)
        )
        add_toolbar_button_dialog.exec()


def check_for_button(repo: Addon) -> bool:
    """Returns True if a button already exists for this macro, or False if not."""
    command = FreeCADGui.Command.findCustomCommand(repo.macro.filename)
    if not command:
        return False
    custom_toolbars = FreeCAD.ParamGet("User parameter:BaseApp/Workbench/Global/Toolbar")
    toolbar_groups = custom_toolbars.GetGroups()
    for group in toolbar_groups:
        toolbar = custom_toolbars.GetGroup(group)
        if toolbar.GetString(command, "*") != "*":
            return True
    return False


def ask_for_toolbar(repo: Addon, custom_toolbars) -> object:
    """Determine what toolbar to add the icon to. The first time it is called it prompts the
    user to select or create a toolbar. After that, the prompt is optional and can be configured
    via a preference. Returns the pref group for the new toolbar."""
    pref = FreeCAD.ParamGet("User parameter:BaseApp/Preferences/Addons")

    # In this one spot, default True: if this is the first time we got to
    # this chunk of code, we are always going to ask.
    ask = pref.GetBool("alwaysAskForToolbar", True)

    if ask:
        select_toolbar_dialog = FreeCADGui.PySideUic.loadUi(
            os.path.join(os.path.dirname(__file__), "select_toolbar_dialog.ui")
        )
        select_toolbar_dialog.setWindowFlag(QtCore.Qt.WindowStaysOnTopHint, True)

        select_toolbar_dialog.comboBox.clear()

        for group in custom_toolbars:
            ref = FreeCAD.ParamGet("User parameter:BaseApp/Workbench/Global/Toolbar/" + group)
            name = ref.GetString("Name", "")
            if name:
                select_toolbar_dialog.comboBox.addItem(name)
            else:
                FreeCAD.Console.PrintWarning(
                    f"Custom toolbar {group} does not have a Name element\n"
                )
        new_menubar_option_text = translate("AddonsInstaller", "Create new toolbar")
        select_toolbar_dialog.comboBox.addItem(new_menubar_option_text)

        result = select_toolbar_dialog.exec()
        if result == QtWidgets.QDialog.Accepted:
            selection = select_toolbar_dialog.comboBox.currentText()
            if select_toolbar_dialog.checkBox.checkState() == QtCore.Qt.Unchecked:
                pref.SetBool("alwaysAskForToolbar", False)
            else:
                pref.SetBool("alwaysAskForToolbar", True)
            if selection == new_menubar_option_text:
                return create_new_custom_toolbar()
            return get_toolbar_with_name(selection)
        return None

    # If none of the above code returned...
    custom_toolbar_name = pref.GetString("CustomToolbarName", "Auto-Created Macro Toolbar")
    toolbar = get_toolbar_with_name(custom_toolbar_name)
    if not toolbar:
        # They told us not to ask, but then the toolbar got deleted... ask anyway!
        ask = pref.RemBool("alwaysAskForToolbar")
        return ask_for_toolbar(repo, custom_toolbars)
    return toolbar


def get_toolbar_with_name(name: str) -> object:
    """Try to find a toolbar with a given name. Returns the preference group for the toolbar
    if found, or None if it does not exist."""
    top_group = FreeCAD.ParamGet("User parameter:BaseApp/Workbench/Global/Toolbar")
    custom_toolbars = top_group.GetGroups()
    for toolbar in custom_toolbars:
        group = FreeCAD.ParamGet("User parameter:BaseApp/Workbench/Global/Toolbar/" + toolbar)
        group_name = group.GetString("Name", "")
        if group_name == name:
            return group
    return None


def create_new_custom_toolbar() -> object:
    """Create a new custom toolbar and returns its preference group."""

    # We need two names: the name of the auto-created toolbar, as it will be displayed to the
    # user in various menus, and the underlying name of the toolbar group. Both must be
    # unique.

    # First, the displayed name
    custom_toolbar_name = "Auto-Created Macro Toolbar"
    top_group = FreeCAD.ParamGet("User parameter:BaseApp/Workbench/Global/Toolbar")
    custom_toolbars = top_group.GetGroups()
    name_taken = check_for_toolbar(custom_toolbar_name)
    if name_taken:
        i = 2  # Don't use (1), start at (2)
        while True:
            test_name = custom_toolbar_name + f" ({i})"
            if not check_for_toolbar(test_name):
                custom_toolbar_name = test_name
            i = i + 1

    # Second, the toolbar preference group name
    i = 1
    while True:
        new_group_name = "Custom_" + str(i)
        if new_group_name not in custom_toolbars:
            break
        i = i + 1

    custom_toolbar = FreeCAD.ParamGet(
        "User parameter:BaseApp/Workbench/Global/Toolbar/" + new_group_name
    )
    custom_toolbar.SetString("Name", custom_toolbar_name)
    custom_toolbar.SetBool("Active", True)
    return custom_toolbar


def check_for_toolbar(toolbar_name: str) -> bool:
    """Returns True if the toolbar exists, otherwise False"""
    return get_toolbar_with_name(toolbar_name) is not None


def install_toolbar_button(repo: Addon) -> None:
    """If the user has requested a toolbar button be installed, this function is called
    to continue the process and request any additional required information."""
    pref = FreeCAD.ParamGet("User parameter:BaseApp/Preferences/Addons")
    custom_toolbar_name = pref.GetString("CustomToolbarName", "Auto-Created Macro Toolbar")

    # Default to false here: if the variable hasn't been set, we don't assume
    # that we have to ask, because the simplest is to just create a new toolbar
    # and never ask at all.
    ask = pref.GetBool("alwaysAskForToolbar", False)

    # See if there is already a custom toolbar for macros:
    top_group = FreeCAD.ParamGet("User parameter:BaseApp/Workbench/Global/Toolbar")
    custom_toolbars = top_group.GetGroups()
    if custom_toolbars:
        # If there are already custom toolbars, see if one of them is the one we used last time
        found_toolbar = False
        for toolbar_name in custom_toolbars:
            test_toolbar = FreeCAD.ParamGet(
                "User parameter:BaseApp/Workbench/Global/Toolbar/" + toolbar_name
            )
            name = test_toolbar.GetString("Name", "")
            if name == custom_toolbar_name:
                custom_toolbar = test_toolbar
                found_toolbar = True
                break
        if ask or not found_toolbar:
            # We have to ask the user what to do...
            custom_toolbar = ask_for_toolbar(repo, custom_toolbars)
            if custom_toolbar:
                custom_toolbar_name = custom_toolbar.GetString("Name")
                pref.SetString("CustomToolbarName", custom_toolbar_name)
    else:
        # Create a custom toolbar
        custom_toolbar = FreeCAD.ParamGet(
            "User parameter:BaseApp/Workbench/Global/Toolbar/Custom_1"
        )
        custom_toolbar.SetString("Name", custom_toolbar_name)
        custom_toolbar.SetBool("Active", True)

    if custom_toolbar:
        install_macro_to_toolbar(repo, custom_toolbar)
    else:
        FreeCAD.Console.PrintMessage("In the end, no custom toolbar was set, bailing out\n")


def install_macro_to_toolbar(repo: Addon, toolbar: object) -> None:
    """Adds an icon for the given macro to the given toolbar."""
    menuText = repo.display_name
    tooltipText = f"<b>{repo.display_name}</b>"
    if repo.macro.comment:
        tooltipText += f"<br/><p>{repo.macro.comment}</p>"
        whatsThisText = repo.macro.comment
    else:
        whatsThisText = translate(
            "AddonsInstaller", "A macro installed with the FreeCAD Addon Manager"
        )
    statustipText = (
        translate("AddonsInstaller", "Run", "Indicates a macro that can be 'run'")
        + " "
        + repo.display_name
    )
    if repo.macro.icon:
        if os.path.isabs(repo.macro.icon):
            pixmapText = os.path.normpath(repo.macro.icon)
        else:
            macro_repo_dir = FreeCAD.getUserMacroDir(True)
            pixmapText = os.path.normpath(os.path.join(macro_repo_dir, repo.macro.icon))
    elif repo.macro.xpm:
        macro_repo_dir = FreeCAD.getUserMacroDir(True)
        icon_file = os.path.normpath(os.path.join(macro_repo_dir, repo.macro.name + "_icon.xpm"))
        with open(icon_file, "w", encoding="utf-8") as f:
            f.write(repo.macro.xpm)
        pixmapText = icon_file
    else:
        pixmapText = None

    # Add this command to that toolbar
    command_name = FreeCADGui.Command.createCustomCommand(
        repo.macro.filename,
        menuText,
        tooltipText,
        whatsThisText,
        statustipText,
        pixmapText,
    )
    toolbar.SetString(command_name, "FreeCAD")

    # Force the toolbars to be recreated
    wb = FreeCADGui.activeWorkbench()
    wb.reloadActive()


def remove_custom_toolbar_button(repo: Addon) -> None:
    """If this repo contains a macro, look through the custom commands and
    see if one is set up for this macro. If so, remove it, including any
    toolbar entries."""

    command = FreeCADGui.Command.findCustomCommand(repo.macro.filename)
    if not command:
        return
    custom_toolbars = FreeCAD.ParamGet("User parameter:BaseApp/Workbench/Global/Toolbar")
    toolbar_groups = custom_toolbars.GetGroups()
    for group in toolbar_groups:
        toolbar = custom_toolbars.GetGroup(group)
        if toolbar.GetString(command, "*") != "*":
            toolbar.RemString(command)

    FreeCADGui.Command.removeCustomCommand(command)

    # Force the toolbars to be recreated
    wb = FreeCADGui.activeWorkbench()
    wb.reloadActive()
