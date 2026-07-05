# SPDX-License-Identifier: LicenseRef-Parashell-Proprietary

# ********************************************************************************
# *   Copyright (c) 2026 Odin Glynn-Martin <odin.glynn[at]parashell.cloud>  	    *
# *                                                                         	    *
# *   This file is part of the Parashell Mod system.                        	    *
# *                                                                         	    *
# *   PROPRIETARY // NON-CONFIDENTIAL                                           *
# *                                                                         	    *
# *   This file contains proprietary resources used by compiled       		    *
# *   Cython modules within the Parashell Mod system. It is provided in     	    *
# *   uncompiled form solely to support the runtime environment and              *
# *   must not be treated as open-source software.                          	    *
# *                                                                         	    *
# *   Unauthorised copying, distribution, modification, or use of this      	    *
# *   file, in whole or in part, is strictly prohibited without the         	    *
# *   express written permission of the copyright holder.                   	    *
# *                                                                         	    *
# *   All rights reserved.                                                  	    *
# *                                                                         	    *
# ********************************************************************************

"""Initialise the Agent module's GUI counterpart."""

from __future__ import annotations

import atexit
import os
import sys
from typing import TYPE_CHECKING

import FreeCAD
import FreeCADGui
from PySide6 import QtCore, QtWidgets

import agent_config
import ags
import hostcontrol

if TYPE_CHECKING:
    from collections.abc import Callable
    from types import ModuleType

_STARTUP_DELAY_MS = 1000
_WINDOW_POLL_INTERVAL_MS = 500
_RETRY_INTERVAL_MS = 5000
_STATUS_MARGIN_PX = 16

_singleton = None
_auth_core_cache = None
_auth_warning_printed = False


def _module_dir() -> str:
    return os.path.dirname(os.path.abspath(__file__))


def _module_root() -> str:
    return os.path.dirname(_module_dir())


def _sibling_dir(*names: str) -> str | None:
    root = _module_root()
    for name in names:
        path = os.path.join(root, name)
        if os.path.isdir(path):
            return path
    try:
        entries = os.listdir(root)
    except OSError:
        return None
    wanted = {name.lower() for name in names}
    for entry in entries:
        if entry.lower() in wanted:
            path = os.path.join(root, entry)
            if os.path.isdir(path):
                return path
    return None


def _has_module(directory: str | None, module_name: str) -> bool:
    if not directory or not os.path.isdir(directory):
        return False
    try:
        entries = os.listdir(directory)
    except OSError:
        return False
    for entry in entries:
        root, ext = os.path.splitext(entry)
        if ext in (".py", ".pyd", ".so") and root.split(".")[0] == module_name:
            return True
    return False


def _resolve_bridge_dir() -> str | None:
    sibling = _sibling_dir("Bridge", "bridge")
    if _has_module(sibling, "bridge"):
        return sibling

    for getter in ("getUserAppDataDir", "getResourceDir", "getHomePath"):
        try:
            base = getattr(FreeCAD, getter)()
        except Exception:
            continue
        for name in ("Bridge", "bridge"):
            candidate = os.path.join(base, "Mod", name)
            if _has_module(candidate, "bridge"):
                return candidate
    return None


def _resolve_auth_core() -> ModuleType | None:
    global _auth_core_cache
    if _auth_core_cache is not None:
        return _auth_core_cache

    sibling = _sibling_dir("Auth", "auth")
    if sibling and os.path.isdir(sibling) and sibling not in sys.path:
        sys.path.insert(0, sibling)

    try:
        import auth_core

        _auth_core_cache = auth_core
        return auth_core
    except Exception:
        return None


def _build_auth_provider() -> Callable[[], dict[str, object] | None]:
    def provider() -> dict[str, object] | None:
        global _auth_warning_printed
        try:
            auth_core = _resolve_auth_core()
            if auth_core is None:
                if not _auth_warning_printed:
                    FreeCAD.Console.PrintWarning(
                        "Parashell Agent: Auth module not found yet; waiting "
                        "for a signed-in session.\n"
                    )
                    _auth_warning_printed = True
                return None
            if not auth_core.is_signed_in():
                return None
            token = auth_core.valid_access_token()
            if not token:
                return None
            session = auth_core.current_session() or {}
            user = session.get("user") or {}
            return {
                "accessToken": token,
                "user": {
                    "sub": user.get("sub", ""),
                    "email": user.get("email", ""),
                    "name": user.get("name", ""),
                    "profilePictureUrl": user.get("profile_picture_url", ""),
                },
            }
        except Exception:
            return None

    return provider


class AgentSidebar:
    def __init__(self) -> None:
        self._dock = None
        self._stack = None
        self._status_label = None
        self._host_widget = None
        self._host_layout = None
        self._widget = None
        self._toolbar = None
        self._retry_timer = None
        self._shutting_down = False
        self._logging = False
        self._auth_provider = None
        self._bridge_log_forwarder = None
        self._retry_forwarder = None
        self._quit_forwarder = None

    def start(self) -> None:
        if hostcontrol.is_killed():
            return
        hostcontrol.register_kill(self._kill)
        self._auth_provider = _build_auth_provider()
        self._build_ui()
        self._launch()
        self._register_shutdown_hooks()

    def _main_window(self) -> QtWidgets.QMainWindow | None:
        return FreeCADGui.getMainWindow()

    def _build_ui(self) -> None:
        main_window = self._main_window()
        if main_window is None:
            return

        existing = main_window.findChild(
            QtWidgets.QDockWidget, agent_config.DOCK_OBJECT_NAME
        )
        if existing is not None:
            self._dock = existing
            return

        dock = QtWidgets.QDockWidget("Agent", main_window)
        dock.setObjectName(agent_config.DOCK_OBJECT_NAME)
        dock.setAllowedAreas(QtCore.Qt.DockWidgetArea.RightDockWidgetArea)
        dock.setFeatures(
            QtWidgets.QDockWidget.DockWidgetFeature.DockWidgetClosable
            | QtWidgets.QDockWidget.DockWidgetFeature.DockWidgetMovable
        )
        dock.setMinimumWidth(agent_config.MIN_SIDEBAR_WIDTH)

        content = QtWidgets.QWidget(dock)
        content.setMinimumWidth(agent_config.MIN_SIDEBAR_WIDTH)
        outer = QtWidgets.QVBoxLayout(content)
        outer.setContentsMargins(0, 0, 0, 0)
        outer.setSpacing(0)

        stack = QtWidgets.QStackedLayout()
        outer.addLayout(stack)

        status = QtWidgets.QLabel("Connecting to the Parashell Agent...")
        status.setAlignment(QtCore.Qt.AlignmentFlag.AlignCenter)
        status.setWordWrap(True)
        status.setContentsMargins(
            _STATUS_MARGIN_PX,
            _STATUS_MARGIN_PX,
            _STATUS_MARGIN_PX,
            _STATUS_MARGIN_PX,
        )
        stack.addWidget(status)

        host = QtWidgets.QWidget(content)
        host_layout = QtWidgets.QVBoxLayout(host)
        host_layout.setContentsMargins(0, 0, 0, 0)
        host_layout.setSpacing(0)
        stack.addWidget(host)
        stack.setCurrentWidget(status)

        dock.setWidget(content)
        main_window.addDockWidget(QtCore.Qt.DockWidgetArea.RightDockWidgetArea, dock)
        dock.show()

        self._dock = dock
        self._stack = stack
        self._status_label = status
        self._host_widget = host
        self._host_layout = host_layout

        try:
            main_window.resizeDocks(
                [dock],
                [agent_config.DEFAULT_SIDEBAR_WIDTH],
                QtCore.Qt.Orientation.Horizontal,
            )
        except Exception:
            dock.resize(agent_config.DEFAULT_SIDEBAR_WIDTH, dock.height())

        self._build_toolbar()

    def _build_toolbar(self) -> None:
        main_window = self._main_window()
        if main_window is None or self._dock is None:
            return

        toolbar = main_window.findChild(
            QtWidgets.QToolBar, agent_config.TOOLBAR_OBJECT_NAME
        )
        if toolbar is None:
            toolbar = QtWidgets.QToolBar("Agent", main_window)
            toolbar.setObjectName(agent_config.TOOLBAR_OBJECT_NAME)
            main_window.addToolBar(QtCore.Qt.ToolBarArea.TopToolBarArea, toolbar)

        action = self._dock.toggleViewAction()
        action.setText("Agent")
        action.setToolTip("Show or hide the Parashell Agent sidebar")
        action.setIcon(
            main_window.style().standardIcon(
                QtWidgets.QStyle.StandardPixmap.SP_MessageBoxInformation
            )
        )
        toolbar.addAction(action)
        self._toolbar = toolbar

    def _set_status(self, message: str) -> None:
        if self._status_label is not None:
            self._status_label.setText(message)
        if self._stack is not None and self._status_label is not None:
            self._stack.setCurrentWidget(self._status_label)

    def _launch(self) -> None:
        if self._shutting_down or self._host_widget is None:
            return

        try:
            url = agent_config.agent_url()
        except hostcontrol.HostControlError:
            return
        except Exception as exc:
            self._set_status(
                "Could not resolve the Parashell Agent endpoint. Retrying..."
            )
            FreeCAD.Console.PrintError(
                f"Parashell Agent: failed to resolve agent URL: {exc}\n"
            )
            self._schedule_retry()
            return

        local = agent_config.is_loopback_url(url)

        bridge_dir = _resolve_bridge_dir()
        if bridge_dir is None and not local:
            self._set_status(
                "The Parashell Agent could not locate the Bridge module. "
                "Ensure the Bridge plugin is installed alongside the Agent."
            )
            FreeCAD.Console.PrintError(
                "Parashell Agent: could not locate the Bridge module directory.\n"
            )
            self._schedule_retry()
            return

        try:
            import agent_core
        except Exception as exc:
            self._set_status(
                "The Parashell Agent could not load QtWebEngine. Ensure the "
                "pixi 'default' environment with qt6-webengine is active."
            )
            FreeCAD.Console.PrintError(
                f"Parashell Agent: failed to import agent_core: {exc}\n"
            )
            return

        try:
            widget = agent_core.AgentWidget(
                url,
                agent_config.VERSION,
                bridge_dir,
                self._host_widget,
                auth_provider=self._auth_provider,
            )
        except Exception as exc:
            self._set_status("Failed to create the Parashell Agent view.")
            FreeCAD.Console.PrintError(
                f"Parashell Agent: failed to create agent widget: {exc}\n"
            )
            self._schedule_retry()
            return

        bridge = widget.bridge()
        if bridge is not None:
            self._bridge_log_forwarder = ags.connect(
                bridge.stderr, self._on_bridge_log, arg="str"
            )
        self._host_layout.addWidget(widget)
        self._widget = widget
        if self._stack is not None:
            self._stack.setCurrentWidget(self._host_widget)
        QtCore.QTimer.singleShot(0, widget.load)
        FreeCAD.Console.PrintMessage("Parashell Agent: sidebar active.\n")

    def _kill(self) -> None:
        self._shutting_down = True
        if self._retry_timer is not None:
            self._retry_timer.stop()
            self._retry_timer = None
        if self._widget is not None:
            try:
                self._widget.shutdown()
            except Exception:
                pass
            self._widget = None
        main_window = self._main_window()
        if main_window is not None:
            toolbar = main_window.findChild(
                QtWidgets.QToolBar, agent_config.TOOLBAR_OBJECT_NAME
            )
            if toolbar is not None:
                main_window.removeToolBar(toolbar)
                toolbar.deleteLater()
            dock = main_window.findChild(
                QtWidgets.QDockWidget, agent_config.DOCK_OBJECT_NAME
            )
            if dock is not None:
                main_window.removeDockWidget(dock)
                dock.deleteLater()
        self._dock = None
        self._toolbar = None
        self._stack = None
        self._status_label = None
        self._host_widget = None
        self._host_layout = None

    def _on_bridge_log(self, line: str) -> None:
        if self._logging:
            return
        self._logging = True
        try:
            FreeCAD.Console.PrintMessage(f"Parashell Agent: {line}\n")
        finally:
            self._logging = False

    def _schedule_retry(self) -> None:
        if self._shutting_down:
            return
        if self._retry_timer is None:
            self._retry_timer = QtCore.QTimer()
            self._retry_timer.setSingleShot(True)
            self._retry_forwarder = ags.connect(self._retry_timer.timeout, self._launch)
        self._retry_timer.start(_RETRY_INTERVAL_MS)

    def _register_shutdown_hooks(self) -> None:
        atexit.register(self.shutdown)
        app = QtWidgets.QApplication.instance()
        if app is not None:
            self._quit_forwarder = ags.connect(app.aboutToQuit, self.shutdown)

    def shutdown(self) -> None:
        if self._shutting_down:
            return
        self._shutting_down = True
        if self._retry_timer is not None:
            self._retry_timer.stop()
        if self._widget is not None:
            self._widget.shutdown()


def _start() -> None:
    global _singleton
    if _singleton is not None:
        return
    if FreeCADGui.getMainWindow() is None:
        QtCore.QTimer.singleShot(_WINDOW_POLL_INTERVAL_MS, _start)
        return
    _singleton = AgentSidebar()
    _singleton.start()


QtCore.QTimer.singleShot(_STARTUP_DELAY_MS, _start)
