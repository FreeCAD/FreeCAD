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

"""Fuzzy command search popup ("press S").

The popup is shared: sketch edit mode feeds it every sketch tool
(Sketcher_CommandSearch); everywhere else Std_CommandSearch feeds it the tools
on the toolbars currently shown, including the grouped toolbar (ribbon).
"""

import functools
import re

import FreeCAD
import FreeCADGui
from PySide import QtCore, QtGui, QtWidgets

translate = FreeCAD.Qt.translate

MAX_RECENT = 8
MAX_CLOSEST = 8  # shown when nothing matches, so a typo never empties the list

# Extra words people search for that are not in a command's own text.
KEYWORDS = {
    "Sketcher_ConstrainLock": "fix fixed anchor ground",
    "Sketcher_ConstrainBlock": "fix fixed freeze",
    "Sketcher_ConstrainCoincidentUnified": "point on object midpoint join connect",
    "Sketcher_ConstrainTangent": "collinear",
    "Sketcher_ConstrainEqual": "same",
    "Sketcher_Dimension": "smart measure",
    "Sketcher_CreateSpiral": "helix coil swirl archimedean logarithmic fermat involute",
    "Sketcher_Trimming": "cut delete segment",
    "Sketcher_Translate": "move copy array pattern rectangular",
    "Sketcher_Rotate": "circular pattern polar array",
    "Sketcher_Symmetry": "mirror flip",
    "Sketcher_Projection": "project include reference",
    "Sketcher_ToggleConstruction": "reference helper",
    "Sketcher_CreateOblong": "rounded",
    "Sketcher_CreatePoint": "vertex pt",
    "Sketcher_CreatePolyline": "lines chain",
    "PartDesign_Pad": "pad pocket cut join",
    "PartDesign_Revolution": "revolution groove lathe",
    "PartDesign_AdditivePipe": "pipe sweep path",
    "PartDesign_AdditiveHelix": "helix spring thread",
    "PartDesign_Thickness": "thickness hollow",
    "PartDesign_Boolean": "boolean union subtract",
    "PartDesign_NewSketch": "sketch",
    "PartDesign_Plane": "datum offset plane",
    "PartDesign_Line": "datum line",
    "PartDesign_Point": "datum",
    "PartDesign_Mirrored": "mirror",
    "Std_Part": "part container",
    "Std_Measure": "distance length angle",
}


class Entry:
    """One searchable tool."""

    def __init__(self, key, title, category, icon, run, enabled=True, keywords=""):
        self.key = key  # remembered for "recent"
        self.title = title.replace("&", "").rstrip(".…").strip()
        self.category = category
        self.icon = icon or QtGui.QIcon()
        self.run = run
        self.enabled = enabled
        self.title_text = _words(self.title)
        self.title_words = self.title_text.split()
        self.extra_words = " ".join(
            (_words(key.split("_", 1)[-1]), KEYWORDS.get(key, ""), keywords, category.lower())
        ).split()

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
                    loose = _typo_match(token, self.title_words + self.extra_words)
                if loose is None:
                    return None
                total += loose
                strong = False
        return strong, total

    def distance(self, tokens):
        """How far tokens are from this entry's words; used to list the closest tools."""
        words = self.title_words + self.extra_words
        if not words:
            return float("inf")
        return sum(min(_typo_distance(token, word) for word in words) for token in tokens)


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


@functools.lru_cache(maxsize=20000)
def _typo_distance(token, word):
    """Edit distance from token to word or to a start of word ("extrdue" -> "extrude")."""
    n = len(token)
    return min(_edit_distance(token, word[:k]) for k in {n - 1, n, n + 1, len(word)} if k > 0)


def _edit_distance(a, b):
    """Optimal string alignment distance: insertions, deletions, substitutions, swaps."""
    prev2 = None
    prev = list(range(len(b) + 1))
    for i in range(1, len(a) + 1):
        cur = [i] + [0] * len(b)
        for j in range(1, len(b) + 1):
            cost = a[i - 1] != b[j - 1]
            cur[j] = min(prev[j] + 1, cur[j - 1] + 1, prev[j - 1] + cost)
            if i > 1 and j > 1 and a[i - 1] == b[j - 2] and a[i - 2] == b[j - 1]:
                cur[j] = min(cur[j], prev2[j - 2] + 1)
        prev2, prev = prev, cur
    return prev[-1]


def _typo_match(token, words):
    """Score a misspelling: about one mistake per three letters ("cirlce", "rectangel")."""
    if len(token) < 3:
        return None
    best = min((_typo_distance(token, word) for word in words), default=None)
    if best is None or best > max(1, len(token) // 3):
        return None
    return 40 - 10 * best


def _recent(param_path):
    return [n for n in FreeCAD.ParamGet(param_path).GetString("Recent", "").split(";") if n]


def _remember(param_path, key):
    recent = [key] + [n for n in _recent(param_path) if n != key]
    FreeCAD.ParamGet(param_path).SetString("Recent", ";".join(recent[:MAX_RECENT]))


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
    def __init__(self, entries, param_path, placeholder, disabled_hint, parent=None):
        super().__init__(parent, QtCore.Qt.Popup)
        self.setAttribute(QtCore.Qt.WA_DeleteOnClose)
        self.setFrameShape(QtWidgets.QFrame.StyledPanel)
        self.setMinimumWidth(360)
        self.entries = entries
        self.param_path = param_path
        self.disabled_hint = disabled_hint

        self.edit = QtWidgets.QLineEdit(self)
        self.edit.setPlaceholderText(placeholder)
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

        self.edit.textChanged.connect(self.refresh)
        self.edit.installEventFilter(self)
        self.list.itemClicked.connect(self.run)
        self.refresh("")

    def refresh(self, text):
        tokens = _words(text).split()
        recent = _recent(self.param_path)
        matches = []
        for e in self.entries:
            match = e.score(tokens) if tokens else (True, 0)
            if match is not None:
                matches.append((match, e))
        # Loose (typo) matches are only a fallback for when nothing matches properly.
        if any(strong for (strong, _), _ in matches):
            matches = [m for m in matches if m[0][0]]
        elif not matches and tokens:
            # Still nothing: show the closest tools rather than an empty list.
            closest = sorted(self.entries, key=lambda e: e.distance(tokens))[:MAX_CLOSEST]
            matches = [((False, -rank), e) for rank, e in enumerate(closest)]

        ranked = []
        for index, ((_, score), e) in enumerate(matches):
            is_recent = e.key in recent
            rank = len(recent) - recent.index(e.key) if is_recent else 0
            if not tokens and not is_recent:
                score = -1000  # empty query: recent first, rest alphabetical
            ranked.append((not e.enabled, -score, -rank, e.title, index, e))

        self.list.clear()
        for row in sorted(ranked, key=lambda r: r[:5]):
            e = row[-1]
            item = QtWidgets.QListWidgetItem(e.icon, e.title)
            item.setData(QtCore.Qt.UserRole, row[4])
            item.setData(QtCore.Qt.UserRole + 1, e.category)
            if not e.enabled:
                item.setFlags(item.flags() & ~QtCore.Qt.ItemIsEnabled)
                item.setToolTip(self.disabled_hint)
            self.list.addItem(item)
        self._matches = [e for _, e in matches]
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
        entry = self._matches[item.data(QtCore.Qt.UserRole)]
        _remember(self.param_path, entry.key)
        self.close()
        # Let the popup release the keyboard before the tool starts.
        QtCore.QTimer.singleShot(0, entry.run)

    def popup_at(self, pos):
        self.adjustSize()
        screen = QtGui.QGuiApplication.screenAt(pos) or QtGui.QGuiApplication.primaryScreen()
        area = screen.availableGeometry()
        x = min(max(pos.x() - self.width() // 2, area.left()), area.right() - self.width())
        y = min(max(pos.y() - self.edit.height() // 2, area.top()), area.bottom() - self.height())
        self.move(x, y)
        self.show()
        self.edit.setFocus()


def show(entries, param_path, placeholder, disabled_hint=""):
    popup = CommandSearchPopup(
        entries, param_path, placeholder, disabled_hint, FreeCADGui.getMainWindow()
    )
    popup.popup_at(QtGui.QCursor.pos())
    return popup


# --- Search over the tools shown on the toolbars ------------------------------------------

PARAM_PATH = "User parameter:BaseApp/Preferences/General/CommandSearch"


def _action_entries(actions, category, seen):
    entries = []
    for action in actions:
        if action.isSeparator() or isinstance(action, QtWidgets.QWidgetAction):
            continue
        if action.menu():
            entries += _action_entries(action.menu().actions(), category, seen)
            continue
        if not action.isVisible() or not action.text():
            continue
        key = action.objectName() or action.text()
        if key in seen or key == "Std_CommandSearch":
            continue
        seen.add(key)
        entries.append(
            Entry(key, action.text(), category, action.icon(), action.trigger, action.isEnabled())
        )
    return entries


def toolbar_entries():
    """Every tool on the visible toolbars, labelled with its toolbar or ribbon group."""
    mw = FreeCADGui.getMainWindow()
    seen = set()
    entries = []
    for bar in mw.findChildren(QtWidgets.QToolBar):
        if not bar.isVisible() or bar.objectName() == "Workbench":
            continue
        # Ribbon groups: pinned buttons plus the group's menu, labelled with the group.
        groups = [
            w
            for w in bar.findChildren(QtWidgets.QWidget)
            if isinstance(getattr(w, "menu", None), QtWidgets.QMenu)
        ]
        for group in groups:
            category = group.label.text().replace("▾", "").strip().title()
            buttons = [
                b.defaultAction()
                for b in group.findChildren(QtWidgets.QToolButton)
                if b.defaultAction()
            ]
            entries += _action_entries(buttons + group.menu.actions(), category, seen)
        if groups:
            continue
        category = bar.windowTitle()
        actions = []
        for action in bar.actions():
            button = bar.widgetForAction(action)
            # Drop-down buttons keep their items in the button's own menu.
            if isinstance(button, QtWidgets.QToolButton) and button.menu():
                actions += button.menu().actions()
            else:
                actions.append(action)
        entries += _action_entries(actions, category, seen)
    return entries


def show_toolbar_search():
    return show(
        toolbar_entries(),
        PARAM_PATH,
        translate("Std_CommandSearch", "Search tools…"),
        translate("Std_CommandSearch", "Not available right now"),
    )


class _StdCommandSearch:
    def GetResources(self):
        return {
            "Pixmap": "edit-find",
            "MenuText": QtCore.QT_TRANSLATE_NOOP("Std_CommandSearch", "Search Tools…"),
            "ToolTip": QtCore.QT_TRANSLATE_NOOP(
                "Std_CommandSearch",
                "Opens a search box to find and start any tool on the toolbars",
            ),
            "Accel": "S",
        }

    def IsActive(self):
        # Sketch edit mode has its own search (Sketcher_CommandSearch) on the same key.
        doc = FreeCADGui.ActiveDocument
        vp = doc.getInEdit() if doc else None
        return not (vp and vp.Object.isDerivedFrom("Sketcher::SketchObject"))

    def Activated(self):
        # Look the module up at call time so a reloaded module takes effect.
        from freecad.gui import CommandSearch

        CommandSearch.show_toolbar_search()


if "Std_CommandSearch" not in FreeCADGui.listCommands():
    FreeCADGui.addCommand("Std_CommandSearch", _StdCommandSearch())
