# SPDX-License-Identifier: LGPL-2.1-or-later
# SPDX-FileCopyrightText: 2026 Max Wilfinger
# SPDX-FileNotice: Part of the FreeCAD project.

################################################################################
#                                                                              #
#   FreeCAD is free software: you can redistribute it and/or modify            #
#   it under the terms of the GNU Lesser General Public License as             #
#   published by the Free Software Foundation, either version 2.1              #
#   of the License, or (at your option) any later version.                     #
#                                                                              #
#   FreeCAD is distributed in the hope that it will be useful,                 #
#   but WITHOUT ANY WARRANTY; without even the implied warranty                #
#   of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.                    #
#   See the GNU Lesser General Public License for more details.                #
#                                                                              #
#   You should have received a copy of the GNU Lesser General Public           #
#   License along with FreeCAD. If not, see https://www.gnu.org/licenses       #
#                                                                              #
################################################################################

"""Manual-input locking for Draft task-panel input fields."""

## @package gui_field_locks
#  \ingroup draftguitools
#  \brief Manual-input locking for Draft task-panel coordinate fields.

from PySide import QtCore
from PySide import QtGui
from PySide import QtWidgets

from draftutils.translate import translate

_NUMERIC_PREFIX = "0123456789.,-+"


class _FieldLockFilter(QtCore.QObject):
    """Handle lock shortcuts without consuming field events."""

    def __init__(self, group, key):
        super().__init__()
        self._group = group
        self._key = key

    def eventFilter(self, obj, event):
        if event.type() == QtCore.QEvent.MouseButtonDblClick:
            self._group.unlock(self._key)
        elif event.type() == QtCore.QEvent.KeyPress and event.key() in (
            QtCore.Qt.Key_Enter,
            QtCore.Qt.Key_Return,
        ):
            self._group.lock_valid_input(self._key)
        elif event.type() == QtCore.QEvent.KeyPress and event.key() in (
            QtCore.Qt.Key_Up,
            QtCore.Qt.Key_Down,
        ):
            self._group.queue_locked_value_refresh(self._key)
        return False


class InputFieldLockGroup:
    """Track manual-input locks for a group of ``Gui::InputField`` widgets.

    Parameters
    ----------
    on_change: callable, optional
        Called as ``on_change(key, locked)`` after a field's lock state
        changes, for owners that need to react (e.g. update a snapper).
    """

    def __init__(self, on_change=None):
        self._on_change = on_change
        self._active = True
        self._locked = {}
        self._fields = {}
        self._actions = {}
        self._filters = []
        self._edit_revisions = {}

    def add_field(self, key, field):
        """Attach the lock icon and the lock/unlock triggers to a field."""
        icon = QtGui.QIcon(":/icons/Draft_Snap_Lock.svg")
        action = field.addAction(icon, QtWidgets.QLineEdit.TrailingPosition)
        action.setVisible(False)
        action.setToolTip(
            translate(
                "draft",
                "Keeps the value fixed during 3D input. The lock icon or a double-click unlocks the field.",
            )
        )
        action.triggered.connect(lambda checked=False, k=key: self.unlock(k))
        field.textEdited.connect(lambda text, k=key: self._queue_edit(k, text))
        flt = _FieldLockFilter(self, key)
        field.installEventFilter(flt)
        self._filters.append(flt)
        self._fields[key] = field
        self._actions[key] = action
        self._locked[key] = False

    def _queue_edit(self, key, text):
        if not self._active:
            return
        revision = self._edit_revisions.get(key, 0) + 1
        self._edit_revisions[key] = revision
        QtCore.QTimer.singleShot(
            0,
            lambda k=key, t=text, r=revision: self._finish_edit(k, t, r),
        )

    def _finish_edit(self, key, text, revision):
        if not self._active or self._edit_revisions.get(key) != revision:
            return
        locked = self._has_lockable_input(key, text)
        self.set_locked(key, locked)

    def queue_locked_value_refresh(self, key):
        if not self._active:
            return
        QtCore.QTimer.singleShot(0, lambda k=key: self._refresh_locked_value(k))

    def _refresh_locked_value(self, key):
        if self._active and self.is_locked(key):
            self._notify_change(key, True)

    def _has_lockable_input(self, key, text=None):
        if not self._active:
            return False
        field = self._fields.get(key)
        if field is None:
            return False
        text = field.text() if text is None else text
        text = text.lstrip()
        return bool(text) and text[0] in _NUMERIC_PREFIX and field.hasAcceptableInput()

    def lock_valid_input(self, key):
        if self._has_lockable_input(key):
            if self.is_locked(key):
                self._notify_change(key, True)
            else:
                self.set_locked(key, True)

    def is_locked(self, key):
        return self._locked.get(key, False)

    def locked_value(self, key):
        if not self.is_locked(key):
            return None
        field = self._fields.get(key)
        if field is None:
            return None
        value = field.property("rawValue")
        return None if value is None else float(value)

    def any_locked(self):
        return any(self._locked.values())

    def set_locked(self, key, locked):
        if not self._active:
            return
        if self._locked.get(key) == locked:
            return
        self._locked[key] = locked
        action = self._actions.get(key)
        if action is not None:
            action.setVisible(locked)
        self._notify_change(key, locked)

    def _notify_change(self, key, locked):
        if self._on_change is not None:
            self._on_change(key, locked)

    def unlock(self, key):
        self._edit_revisions[key] = self._edit_revisions.get(key, 0) + 1
        self.set_locked(key, False)

    def unlock_keys(self, keys):
        for key in keys:
            self.unlock(key)

    def unlock_all(self):
        self.unlock_keys(list(self._locked))

    def dispose(self):
        """Release all locks and ignore callbacks from discarded fields."""
        if not self._active:
            return
        self._active = False
        for key, locked in self._locked.items():
            self._edit_revisions[key] = self._edit_revisions.get(key, 0) + 1
            self._locked[key] = False
            if locked:
                self._notify_change(key, False)
