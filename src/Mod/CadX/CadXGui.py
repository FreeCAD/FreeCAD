# SPDX-License-Identifier: LGPL-2.1-or-later

"""Module-level GUI wiring for the cad-x assistant.

Owns the singleton session and the dock registration.  ``InitGui`` calls
:func:`ensure_commands_registered`; the panel itself is created lazily the
first time it is shown.  Reopening after close works through
``View > Panels > cad-x Assistant`` and through the ``CadX_Assistant``
command (``Gui.runCommand("CadX_Assistant")``).
"""

from __future__ import annotations

import FreeCADGui as Gui

from CadXChatClient import OllamaClient
from CadXToolProtocol import make_graph_registry


DOCK_NAME = "CadXAssistantPanel"
COMMAND_NAME = "CadX_Assistant"
PANEL_TITLE = "cad-x Assistant"

_session = None
_registered = False


def get_session():
    global _session
    if _session is None:
        from CadXSession import ChatSession

        _session = ChatSession(OllamaClient(tool_registry=make_graph_registry()))
    return _session


def find_panel_dock():
    try:
        from PySide import QtWidgets
    except ImportError:
        return None
    main_window = Gui.getMainWindow()
    if main_window is None:
        return None
    return main_window.findChild(QtWidgets.QDockWidget, DOCK_NAME)


def show_panel() -> None:
    """Create (if needed) and raise the assistant dock.

    Uses the same Qt-native docking pattern as the Help and BIM modules:
    this FreeCAD version does not expose DockWindowManager to Python.
    """

    from PySide import QtCore, QtWidgets

    from CadXPanel import AssistantPanel

    main_window = Gui.getMainWindow()
    if main_window is None:
        raise RuntimeError("The cad-x assistant requires the FreeCAD main window.")

    dock = find_panel_dock()
    widget = dock.widget() if dock is not None else None
    if not isinstance(widget, AssistantPanel):
        if dock is not None:  # stale dock from a previous session layout
            main_window.removeDockWidget(dock)
            dock.deleteLater()
        widget = AssistantPanel(get_session(), get_session_client())
        widget.setMinimumWidth(320)
        dock = QtWidgets.QDockWidget(PANEL_TITLE, main_window)
        dock.setObjectName(DOCK_NAME)
        dock.setWidget(widget)
        main_window.addDockWidget(QtCore.Qt.RightDockWidgetArea, dock)
        _add_view_menu_entry(main_window, dock)
    dock.show()
    dock.raise_()


def get_session_client():
    """Return the client behind the singleton session (for panel status)."""

    return get_session().client


def _add_view_menu_entry(main_window, dock) -> None:
    """Also list the toggle next to View, so reopening stays discoverable.

    The dock already appears under View > Panels; this adds a top-level
    ``cad-x Assistant`` entry as well.  Menu lookup is best-effort and
    skipped when the View menu cannot be identified (localized UIs).
    """

    menu_bar = main_window.menuBar()
    if menu_bar is None:
        return
    for menu_action in menu_bar.actions():
        if menu_action.text().replace("&", "").strip().lower() == "view":
            menu = menu_action.menu()
            if menu is not None and dock.toggleViewAction() not in menu.actions():
                menu.addAction(dock.toggleViewAction())
            return


def ensure_commands_registered() -> None:
    """Register the toggle command once per session and open the panel."""

    global _registered
    if not _registered:
        Gui.addCommand(COMMAND_NAME, _AssistantCommand())
        _registered = True
    from PySide import QtCore

    QtCore.QTimer.singleShot(0, show_panel)


class _AssistantCommand:
    """Toggles the assistant dock; reachable via Gui.runCommand."""

    def GetResources(self):  # noqa: N802 - FreeCAD command API
        return {
            "MenuText": PANEL_TITLE,
            "ToolTip": "Show or hide the cad-x chat assistant",
        }

    def IsActive(self) -> bool:  # noqa: N802 - FreeCAD command API
        return True

    def Activated(self) -> None:
        dock = find_panel_dock()
        if dock is None:
            show_panel()
            return
        dock.setVisible(not dock.isVisible())
