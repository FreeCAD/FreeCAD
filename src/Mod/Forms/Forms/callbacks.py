# SPDX-License-Identifier: LGPL-2.1-or-later
"""Explicit ownership of deferred editor work and weak native callbacks."""

import weakref

from PySide import QtCore


def weak_callback(method):
    """Do not let a native sensor's Python trampoline retain its owner."""
    reference = weakref.WeakMethod(method)

    def invoke(*args):
        callback = reference()
        if callback is not None:
            return callback(*args)
        return None

    return invoke


class DeferredCallbacks:
    """Cancel queued work before its widgets, view, or session are destroyed."""

    def __init__(self):
        self.pending = set()
        self.closed = False

    def later(self, delay, callback):
        if self.closed:
            return
        timer = QtCore.QTimer()
        timer.setSingleShot(True)

        def invoke():
            self.pending.discard(timer)
            timer.timeout.disconnect()
            timer.deleteLater()
            if not self.closed:
                callback()

        self.pending.add(timer)
        timer.timeout.connect(invoke)
        timer.start(delay)

    def close(self):
        self.closed = True
        pending, self.pending = self.pending, set()
        for timer in pending:
            timer.stop()
            timer.timeout.disconnect()
            timer.deleteLater()
