###
# Copyright (c) 2002-2008 Kongsberg SIM
#
# Permission to use, copy, modify, and distribute this software for any
# purpose with or without fee is hereby granted, provided that the above
# copyright notice and this permission notice appear in all copies.
#
# THE SOFTWARE IS PROVIDED "AS IS" AND THE AUTHOR DISCLAIMS ALL WARRANTIES
# WITH REGARD TO THIS SOFTWARE INCLUDING ALL IMPLIED WARRANTIES OF
# MERCHANTABILITY AND FITNESS. IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR
# ANY SPECIAL, DIRECT, INDIRECT, OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES
# WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS, WHETHER IN AN
# ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT OF
# OR IN CONNECTION WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.
#

from pivy.qt.QtCore import QThread
from pivy.qt.QtCore import QWaitCondition
from pivy.qt.QtCore import QMutex
from pivy.qt.QtCore import SIGNAL


class SignalThread(QThread):
    def __init__(self, parent = None):
        QThread.__init__(self, parent)

        self.waitcond = QWaitCondition()
        self.mutex = QMutex()
        self.isstopped = False

    def trigger(self):
        """lock first to make sure the QThread is actually waiting for a signal"""
        self.mutex.lock()
        self.waitcond.wakeOne()
        self.mutex.unlock()

    def stopThread(self):
        self.mutex.lock()
        self.isstopped = True
        self.waitcond.wakeOne()
        self.mutex.unlock()

    def run(self):
        self.mutex.lock()
        while not self.isstopped:
            # just wait, and trigger every time we receive a signal
            self.waitcond.wait(self.mutex)
            if not self.isstopped:
                self.emit(SIGNAL("triggerSignal()"))
        self.mutex.unlock()
