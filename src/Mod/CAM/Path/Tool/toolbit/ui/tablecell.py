# -*- coding: utf-8 -*-
# ***************************************************************************
# *   Copyright (c) 2025 Samuel Abels <knipknap@gmail.com>                  *
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
import re
from PySide import QtGui, QtCore
import FreeCAD
from ...shape import ToolBitShape


def isub(text, old, repl_pattern):
    pattern = "|".join(re.escape(o) for o in old)
    return re.sub("(" + pattern + ")", repl_pattern, text, flags=re.I)


def interpolate_colors(start_color, end_color, ratio):
    r = 1.0 - ratio
    red = start_color.red() * r + end_color.red() * ratio
    green = start_color.green() * r + end_color.green() * ratio
    blue = start_color.blue() * r + end_color.blue() * ratio
    return QtGui.QColor(int(red), int(green), int(blue))


class TwoLineTableCell(QtGui.QWidget):
    def __init__(self, parent=None):
        super(TwoLineTableCell, self).__init__(parent)
        self.tool_no = ""
        self.pocket = ""
        self.upper_text = ""
        self.lower_text = ""
        self.search_highlight = ""

        palette = self.palette()
        bg_role = self.backgroundRole()
        bg_color = palette.color(bg_role)
        fg_role = self.foregroundRole()
        fg_color = palette.color(fg_role)

        self.vbox = QtGui.QVBoxLayout()
        self.label_upper = QtGui.QLabel()
        self.label_upper.setStyleSheet("margin-top: 8px")

        color = interpolate_colors(bg_color, fg_color, 0.8)
        style = "margin-bottom: 8px; color: {};".format(color.name())
        self.label_lower = QtGui.QLabel()
        self.label_lower.setStyleSheet(style)
        self.vbox.addWidget(self.label_upper)
        self.vbox.addWidget(self.label_lower)

        style = "color: {}".format(fg_color.name())
        self.label_left = QtGui.QLabel()
        self.label_left.setMinimumWidth(40)
        self.label_left.setTextFormat(QtCore.Qt.RichText)
        self.label_left.setAlignment(QtCore.Qt.AlignCenter | QtCore.Qt.AlignVCenter)
        self.label_left.setStyleSheet(style)

        ratio = self.devicePixelRatioF()
        self.icon_size = QtCore.QSize(50 * ratio, 60 * ratio)
        self.icon_widget = QtGui.QLabel()

        style = "color: {}".format(fg_color.name())
        self.label_right = QtGui.QLabel()
        self.label_right.setMinimumWidth(40)
        self.label_right.setTextFormat(QtCore.Qt.RichText)
        self.label_right.setAlignment(QtCore.Qt.AlignCenter)
        self.label_right.setStyleSheet(style)

        self.hbox = QtGui.QHBoxLayout()
        self.hbox.addWidget(self.label_left, 0)
        self.hbox.addWidget(self.icon_widget, 0)
        self.hbox.addLayout(self.vbox, 1)
        self.hbox.addWidget(self.label_right, 0)

        self.setLayout(self.hbox)

    def _highlight(self, text):
        if not self.search_highlight:
            return text
        highlight_fmt = r'<font style="background: yellow; color: black">\1</font>'
        return isub(text, self.search_highlight.split(" "), highlight_fmt)

    def _update(self):
        # Handle tool number display
        if self.tool_no is not None and self.tool_no != "":
            text = self._highlight(str(self.tool_no))
            self.label_left.setText(f"<b>{text}</b>")
            self.label_left.setVisible(True)
        else:
            self.label_left.setVisible(False)

        text = self._highlight(self.pocket)
        lbl = FreeCAD.Qt.translate("CAM_Toolbit", "Pocket")
        text = f"{lbl}\n<h3>{text}</h3>" if text else ""
        self.label_right.setText(text)

        text = self._highlight(self.upper_text)
        self.label_upper.setText(f"<big><b>{text}</b></big>")

        text = self._highlight(self.lower_text)
        self.label_lower.setText(text)
        self.label_lower.setText(f"{text}")

    def set_tool_no(self, no):
        self.tool_no = no
        self._update()

    def set_pocket(self, pocket):
        self.pocket = str(pocket) if pocket else ""
        self._update()

    def set_upper_text(self, text):
        self.upper_text = text
        self._update()

    def set_lower_text(self, text):
        self.lower_text = text
        self._update()

    def set_icon(self, pixmap):
        self.hbox.removeWidget(self.icon_widget)
        self.icon_widget = QtGui.QLabel()
        self.icon_widget.setPixmap(pixmap)
        self.hbox.insertWidget(1, self.icon_widget, 0)

    def set_icon_from_shape(self, shape: ToolBitShape):
        icon = shape.get_icon()
        if not icon:
            return
        pixmap = icon.get_qpixmap(self.icon_size)
        if pixmap:
            self.set_icon(pixmap)

    def contains_text(self, text):
        for term in text.lower().split(" "):
            tool_no_str = str(self.tool_no) if self.tool_no is not None else ""
            # Check against the raw text content, not the HTML-formatted text
            if (
                term not in tool_no_str.lower()
                and term not in self.upper_text.lower()
                and term not in self.lower_text.lower()
            ):
                return False
        return True

    def highlight(self, text):
        self.search_highlight = text
        self._update()


class CompactTwoLineTableCell(TwoLineTableCell):
    def __init__(self, parent=None):
        super(CompactTwoLineTableCell, self).__init__(parent)

        # Reduce icon size
        ratio = self.devicePixelRatioF()
        self.icon_size = QtCore.QSize(32 * ratio, 32 * ratio)

        # Reduce margins
        self.label_upper.setStyleSheet("margin: 2px 0px 0px 0px; font-size: .8em;")
        self.label_lower.setStyleSheet("margin: 0px 0px 2px 0px; font-size: .8em;")
        self.vbox.setSpacing(0)
        self.hbox.setContentsMargins(0, 0, 0, 0)
