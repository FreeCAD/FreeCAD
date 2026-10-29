# SPDX-License-Identifier: LGPL-2.1-or-later

# ***************************************************************************
# *   Copyright (c) 2026 Jesse Trutna <jesse@trutna.com>                    *
# *                                                                         *
# *   This file is part of the FreeCAD CAx development system.              *
# *                                                                         *
# *   This program is free software; you can redistribute it and/or modify  *
# *   it under the terms of the GNU Lesser General Public License (LGPL)    *
# *   as published by the Free Software Foundation; either version 2 of     *
# *   the License, or (at your option) any later version.                   *
# *   for detail see the LICENCE text file.                                 *
# *                                                                         *
# *   FreeCAD is distributed in the hope that it will be useful,            *
# *   but WITHOUT ANY WARRANTY; without even the implied warranty of        *
# *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the         *
# *   GNU Lesser General Public License for more details.                   *
# *                                                                         *
# *   You should have received a copy of the GNU Lesser General Public     *
# *   License along with FreeCAD; if not, write to the Free Software        *
# *   Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  *
# *   USA                                                                   *
# *                                                                         *
# ***************************************************************************

"""Fuzzy command search popup for sketch edit mode (Sketcher_CommandSearch)."""

import re

import FreeCAD
import FreeCADGui
from PySide import QtCore, QtGui, QtWidgets

translate = FreeCAD.Qt.translate

PARAM_PATH = "User parameter:BaseApp/Preferences/Mod/Sketcher/CommandSearch"
MAX_RECENT = 8

# Commands that make no sense from the search box in edit mode.
EXCLUDED = {
    "Sketcher_CommandSearch",
    "Sketcher_CopyClipboard",
    "Sketcher_Cut",
    "Sketcher_Paste",
    "Sketcher_CreatePolylineLegacy",
    "Sketcher_EditSketch",
    "Sketcher_LeaveGroup",
    "Sketcher_MapSketch",
    "Sketcher_MergeSketches",
    "Sketcher_MirrorSketch",
    "Sketcher_NewSketch",
    "Sketcher_ProfilesHexagon1",
    "Sketcher_ReorientSketch",
    "Sketcher_ValidateSketch",
}

# Extra words people search for that are not in the command's own text.
KEYWORDS = {
    "Sketcher_ConstrainLock": "fix fixed anchor ground",
    "Sketcher_ConstrainBlock": "fix fixed freeze",
    "Sketcher_ConstrainCoincidentUnified": "point on object midpoint join connect",
    "Sketcher_ConstrainTangent": "collinear",
    "Sketcher_ConstrainEqual": "same",
    "Sketcher_Dimension": "smart measure",
    "Sketcher_Trimming": "cut delete segment",
    "Sketcher_Translate": "move copy array pattern rectangular",
    "Sketcher_Rotate": "circular pattern polar array",
    "Sketcher_Symmetry": "mirror flip",
    "Sketcher_Projection": "project include reference",
    "Sketcher_ToggleConstruction": "reference helper",
    "Sketcher_CreateOblong": "rounded",
    "Sketcher_CreatePoint": "vertex pt",
    "Sketcher_CreatePolyline": "lines chain",
}


def _category(name, menu_text):
    n = name[len("Sketcher_") :]
    if "Dimension" in menu_text:
        return translate("Sketcher_CommandSearch", "Dimension")
    if n.startswith("Constrain"):
        return translate("Sketcher_CommandSearch", "Constraint")
    if n.startswith("Create"):
        return translate("Sketcher_CommandSearch", "Create")
    if n.startswith("Select"):
        return translate("Sketcher_CommandSearch", "Select")
    if n.startswith("BSpline"):
        return translate("Sketcher_CommandSearch", "B-Spline")
    if n.startswith(("View", "ArcOverlay", "SwitchVirtualSpace")):
        return translate("Sketcher_CommandSearch", "View")
    if n in ("LeaveSketch", "CancelSketch", "StopOperation"):
        return translate("Sketcher_CommandSearch", "Sketch")
    return translate("Sketcher_CommandSearch", "Modify")


def _words(text):
    """Lowercase text with CamelCase and punctuation split into words."""
    text = re.sub(r"([a-z])([A-Z])", r"\1 \2", text)
    return " ".join(re.findall(r"[a-z0-9]+", text.lower()))


def _prefix_match(token, words):
    """Score token as the start of one of words, or None."""
    for i, word in enumerate(words):
        if word.startswith(token):
            # Earlier words and whole-word matches rank higher.
            return 100 - 10 * min(i, 5) + (20 if word == token else 0)
    return None


def _loose_match(token, text):
    """Tolerate typos and abbreviations ("cnstr"): token letters in order, close together."""
    i = 0
    first = None
    for ch in token:
        j = text.find(ch, i)
        if j < 0:
            return None
        first = j if first is None else first
        i = j + 1
    span = i - first
    if span > 2 * len(token):
        return None
    return 50 - span


class _Entry:
    def __init__(self, name):
        info = FreeCADGui.Command.get(name).getInfo()
        self.name = name
        self.title = info["menuText"].replace("&", "").rstrip(".").strip()
        self.category = _category(name, self.title)
        self.title_text = _words(self.title)
        self.title_words = self.title_text.split()
        self.extra_words = " ".join(
            (_words(name[len("Sketcher_") :]), KEYWORDS.get(name, ""), self.category.lower())
        ).split()
        actions = FreeCADGui.Command.get(name).getAction()
        self.icon = actions[0].icon() if actions else QtGui.QIcon()

    def score(self, tokens):
        """Return (strong, score) or None. Strong means every token starts a word."""
        total = 0
        strong = True
        for token in tokens:
            title = _prefix_match(token, self.title_words)
            extra = _prefix_match(token, self.extra_words)
            if title is not None:
                total += 2 * title
            elif extra is not None:
                total += extra
            else:
                loose = _loose_match(token, self.title_text)
                if loose is None:
                    return None
                total += loose
                strong = False
        return strong, total


def _entries():
    names = []
    for name in FreeCADGui.listCommands():
        if not name.startswith("Sketcher_") or name in EXCLUDED:
            continue
        if name.startswith("Sketcher_Comp"):
            continue  # toolbar drop-downs; their members are listed individually
        if not FreeCADGui.Command.get(name).getAction():
            continue  # not exposed anywhere in the UI
        names.append(name)
    entries = [_Entry(n) for n in names]
    # The same title can belong to more than one command; keep one of each.
    seen = set()
    unique = []
    for e in sorted(entries, key=lambda e: e.name):
        if e.title not in seen:
            seen.add(e.title)
            unique.append(e)
    return unique


def _recent():
    grp = FreeCAD.ParamGet(PARAM_PATH)
    return [n for n in grp.GetString("Recent", "").split(";") if n]


def _remember(name):
    recent = [name] + [n for n in _recent() if n != name]
    FreeCAD.ParamGet(PARAM_PATH).SetString("Recent", ";".join(recent[:MAX_RECENT]))


class _ItemDelegate(QtWidgets.QStyledItemDelegate):
    """Draws the category right-aligned and dimmed next to the command title."""

    def sizeHint(self, option, index):
        size = super().sizeHint(option, index)
        return QtCore.QSize(size.width(), max(size.height(), 26))

    def paint(self, painter, option, index):
        super().paint(painter, option, index)
        category = index.data(QtCore.Qt.UserRole + 1)
        if not category:
            return
        painter.save()
        selected = option.state & QtWidgets.QStyle.State_Selected
        role = QtGui.QPalette.HighlightedText if selected else QtGui.QPalette.PlaceholderText
        painter.setPen(option.palette.color(role))
        rect = option.rect.adjusted(0, 0, -10, 0)
        painter.drawText(rect, QtCore.Qt.AlignRight | QtCore.Qt.AlignVCenter, category)
        painter.restore()


class CommandSearchPopup(QtWidgets.QFrame):
    def __init__(self, parent=None):
        super().__init__(parent, QtCore.Qt.Popup)
        self.setAttribute(QtCore.Qt.WA_DeleteOnClose)
        self.setFrameShape(QtWidgets.QFrame.StyledPanel)
        self.setMinimumWidth(360)

        self.edit = QtWidgets.QLineEdit(self)
        self.edit.setPlaceholderText(
            translate("Sketcher_CommandSearch", "Search sketch tools and constraints…")
        )
        self.edit.setClearButtonEnabled(True)
        self.list = QtWidgets.QListWidget(self)
        self.list.setItemDelegate(_ItemDelegate(self.list))
        self.list.setIconSize(QtCore.QSize(20, 20))
        self.list.setUniformItemSizes(True)
        self.list.setFocusPolicy(QtCore.Qt.NoFocus)
        self.list.setHorizontalScrollBarPolicy(QtCore.Qt.ScrollBarAlwaysOff)

        layout = QtWidgets.QVBoxLayout(self)
        layout.setContentsMargins(6, 6, 6, 6)
        layout.setSpacing(4)
        layout.addWidget(self.edit)
        layout.addWidget(self.list)

        self.entries = _entries()
        self.active = {e.name for e in self.entries if FreeCADGui.Command.get(e.name).isActive()}

        self.edit.textChanged.connect(self.refresh)
        self.edit.installEventFilter(self)
        self.list.itemClicked.connect(self.run)
        self.refresh("")

    def refresh(self, text):
        tokens = _words(text).split()
        recent = _recent()
        matches = []
        for e in self.entries:
            match = e.score(tokens) if tokens else (True, 0)
            if match is not None:
                matches.append((match, e))
        # Loose (typo) matches are only a fallback for when nothing matches properly.
        if any(strong for (strong, _), _ in matches):
            matches = [m for m in matches if m[0][0]]

        ranked = []
        for (_, score), e in matches:
            is_recent = e.name in recent
            rank = len(recent) - recent.index(e.name) if is_recent else 0
            if not tokens and not is_recent:
                score = -1000  # empty query: recent first, rest alphabetical
            ranked.append((e.name not in self.active, -score, -rank, e.title, e))

        self.list.clear()
        for disabled, _, _, _, e in sorted(ranked, key=lambda r: r[:4]):
            item = QtWidgets.QListWidgetItem(e.icon, e.title)
            item.setData(QtCore.Qt.UserRole, e.name)
            item.setData(QtCore.Qt.UserRole + 1, e.category)
            if disabled:
                item.setFlags(item.flags() & ~QtCore.Qt.ItemIsEnabled)
                item.setToolTip(
                    translate("Sketcher_CommandSearch", "Select suitable geometry first")
                )
            self.list.addItem(item)
        self._select(0, 1)

        rows = min(max(self.list.count(), 1), 12)
        self.list.setFixedHeight(rows * self.list.sizeHintForRow(0) + 2 * self.list.frameWidth())
        self.adjustSize()

    def _select(self, start, step):
        """Select the first enabled row at or after start, moving by step."""
        row = start
        while 0 <= row < self.list.count():
            if self.list.item(row).flags() & QtCore.Qt.ItemIsEnabled:
                self.list.setCurrentRow(row)
                return
            row += step

    def eventFilter(self, obj, event):
        if obj is self.edit and event.type() == QtCore.QEvent.KeyPress:
            key = event.key()
            if key in (QtCore.Qt.Key_Down, QtCore.Qt.Key_Up):
                step = 1 if key == QtCore.Qt.Key_Down else -1
                self._select(self.list.currentRow() + step, step)
                return True
            if key in (QtCore.Qt.Key_Return, QtCore.Qt.Key_Enter):
                self.run(self.list.currentItem())
                return True
            if key == QtCore.Qt.Key_Escape:
                self.close()
                return True
        return super().eventFilter(obj, event)

    def run(self, item):
        if item is None or not (item.flags() & QtCore.Qt.ItemIsEnabled):
            return
        name = item.data(QtCore.Qt.UserRole)
        _remember(name)
        self.close()
        # Let the popup release the keyboard before the tool starts.
        QtCore.QTimer.singleShot(0, lambda: FreeCADGui.runCommand(name))

    def popup_at(self, pos):
        self.adjustSize()
        screen = QtGui.QGuiApplication.screenAt(pos) or QtGui.QGuiApplication.primaryScreen()
        area = screen.availableGeometry()
        x = min(max(pos.x() - self.width() // 2, area.left()), area.right() - self.width())
        y = min(max(pos.y() - self.edit.height() // 2, area.top()), area.bottom() - self.height())
        self.move(x, y)
        self.show()
        self.edit.setFocus()


def show():
    popup = CommandSearchPopup(FreeCADGui.getMainWindow())
    popup.popup_at(QtGui.QCursor.pos())
    return popup


class _CommandSearch:
    def GetResources(self):
        return {
            "Pixmap": "edit-find",
            "MenuText": QtCore.QT_TRANSLATE_NOOP("Sketcher_CommandSearch", "Search Commands…"),
            "ToolTip": QtCore.QT_TRANSLATE_NOOP(
                "Sketcher_CommandSearch",
                "Opens a search box to find and start any sketch tool or constraint",
            ),
            "Accel": "S",
            "CmdType": "ForEdit",
        }

    def IsActive(self):
        doc = FreeCADGui.ActiveDocument
        vp = doc.getInEdit() if doc else None
        return bool(vp) and vp.Object.isDerivedFrom("Sketcher::SketchObject")

    def Activated(self):
        # Look the module up at call time so a reloaded module takes effect.
        import SketcherCommandSearch

        SketcherCommandSearch.show()


if "Sketcher_CommandSearch" not in FreeCADGui.listCommands():
    FreeCADGui.addCommand("Sketcher_CommandSearch", _CommandSearch())
