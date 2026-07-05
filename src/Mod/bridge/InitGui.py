# SPDX-License-Identifier: LicenseRef-Parashell-Proprietary

#********************************************************************************
#*   Copyright (c) 2026 Odin Glynn-Martin <odin.glynn[at]parashell.cloud>  	    *
#*                                                                         	    *
#*   This file is part of the Parashell Mod system.                        	    *
#*                                                                         	    *
#*   PROPRIETARY // NON-CONFIDENTIAL                                           *
#*                                                                         	    *
#*   This file contains proprietary resources used by compiled       		    *
#*   Cython modules within the Parashell Mod system. It is provided in     	    *
#*   uncompiled form solely to support the runtime environment and              *
#*   must not be treated as open-source software.                          	    *
#*                                                                         	    *
#*   Unauthorised copying, distribution, modification, or use of this      	    *
#*   file, in whole or in part, is strictly prohibited without the         	    *
#*   express written permission of the copyright holder.                   	    *
#*                                                                         	    *
#*   All rights reserved.                                                  	    *
#*                                                                         	    *
#********************************************************************************

"""
Initialise the Bridge module's gui counterpart
"""

import FreeCAD
import FreeCADGui

try:
    from PySide6 import QtCore, QtGui, QtWidgets
except Exception:
    try:
        from PySide import QtCore, QtGui

        QtWidgets = QtGui
    except Exception:
        FreeCAD.Console.PrintError("Parashell MCP: Cannot import PySide6 or PySide\n")


def _init_mcp_bridge():
    from PySide6 import QtCore, QtGui, QtWidgets

    class _MCPStatusIndicator:
        STATE_STARTING = "starting"
        STATE_RUNNING = "running"
        STATE_RETRYING = "retrying"

        def __init__(self):
            self._label = None
            self._timer = None
            self._spin_angle = 0
            self._state = self.STATE_STARTING

        def _ensure_widget(self):
            if self._label is not None:
                return
            main_window = FreeCADGui.getMainWindow()
            status_bar = main_window.statusBar()
            self._label = QtWidgets.QLabel()
            self._label.setFixedSize(20, 20)
            self._label.setToolTip("AI Bridge: starting...")
            status_bar.addPermanentWidget(self._label)
            self._timer = QtCore.QTimer()
            self._timer.timeout.connect(self._animate)
            self._timer.start(100)

        def _animate(self):
            self._spin_angle = (self._spin_angle + 30) % 360
            size = 16
            pixmap = QtGui.QPixmap(size, size)
            pixmap.fill(QtCore.Qt.GlobalColor.transparent)
            painter = QtGui.QPainter(pixmap)
            painter.setRenderHint(QtGui.QPainter.RenderHint.Antialiasing)
            if self._state == self.STATE_RUNNING:
                painter.setBrush(QtGui.QColor(80, 200, 80))
                painter.setPen(QtCore.Qt.PenStyle.NoPen)
                painter.drawEllipse(2, 2, size - 4, size - 4)
            else:
                color = (
                    QtGui.QColor(80, 200, 80)
                    if self._state == self.STATE_STARTING
                    else QtGui.QColor(220, 180, 40)
                )
                pen = QtGui.QPen(color, 2.5)
                pen.setCapStyle(QtCore.Qt.PenCapStyle.RoundCap)
                painter.setPen(pen)
                painter.setBrush(QtCore.Qt.BrushStyle.NoBrush)
                rect = QtCore.QRectF(2, 2, size - 4, size - 4)
                painter.drawArc(rect, self._spin_angle * 16, 270 * 16)
            painter.end()
            self._label.setPixmap(pixmap)

        def set_starting(self):
            self._ensure_widget()
            self._state = self.STATE_STARTING
            self._label.setToolTip("AI Bridge: starting...")
            if not self._timer.isActive():
                self._timer.start(100)

        def set_running(self):
            self._ensure_widget()
            self._state = self.STATE_RUNNING
            self._label.setToolTip("AI Bridge: engaged")
            self._animate()
            self._timer.stop()

        def set_retrying(self):
            self._ensure_widget()
            self._state = self.STATE_RETRYING
            self._label.setToolTip("AI Bridge: failed, retrying in 5s...")
            if not self._timer.isActive():
                self._timer.start(100)

    indicator = None
    retry_timer = None

    def schedule_retry():
        nonlocal retry_timer
        if retry_timer is None:
            retry_timer = QtCore.QTimer()
            retry_timer.setSingleShot(True)
            retry_timer.timeout.connect(retry_boot)
        retry_timer.start(5000)

    def retry_boot():
        nonlocal indicator
        try:
            if indicator is None:
                indicator = _MCPStatusIndicator()
            indicator.set_starting()
        except Exception:
            pass
        try:
            import rpc

            rpc.set_qtcore(QtCore)
            if rpc.rpc_server_instance is not None:
                if indicator:
                    indicator.set_running()
                return
            result = rpc.start_rpc_server()
            if result:
                if indicator:
                    indicator.set_running()
                address = rpc.rpc_server_instance.server_address
                FreeCAD.Console.PrintMessage(
                    f"Parashell MCP: RPC server running on {address[0]}:{address[1]} (retry succeeded)\n"
                )
                QtCore.QTimer.singleShot(1500, boot_mcp_bridge)
                return
        except Exception as e:
            FreeCAD.Console.PrintWarning(f"Parashell MCP: Retry failed: {e}\n")
        if indicator:
            indicator.set_retrying()
        schedule_retry()

    def boot_mcp_bridge():
        try:
            import server

            if not server.is_running():
                server.start()
        except Exception as e:
            FreeCAD.Console.PrintWarning(
                f"Parashell MCP: MCP bridge start failed: {e}\n"
            )

    def boot_rpc_server():
        nonlocal indicator
        try:
            if indicator is None:
                indicator = _MCPStatusIndicator()
            indicator.set_starting()
        except Exception:
            pass
        try:
            import rpc

            rpc.set_qtcore(QtCore)
            result = rpc.start_rpc_server()
            if result:
                if indicator:
                    indicator.set_running()
                address = rpc.rpc_server_instance.server_address
                FreeCAD.Console.PrintMessage(
                    f"Parashell MCP: RPC server running on {address[0]}:{address[1]}\n"
                )
                QtCore.QTimer.singleShot(1500, boot_mcp_bridge)
                return
        except Exception as e:
            import traceback

            FreeCAD.Console.PrintWarning(f"Parashell MCP: RPC start failed: {e}\n")
            FreeCAD.Console.PrintWarning(traceback.format_exc() + "\n")
        if indicator:
            indicator.set_retrying()
        schedule_retry()

    QtCore.QTimer.singleShot(1000, boot_rpc_server)


_init_mcp_bridge()
