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

from pivy.qt.QtCore import QTimer
from pivy.qt.QtCore import QObject
from pivy.qt.QtCore import QThread
from pivy.qt.QtCore import SIGNAL

from pivy.coin import SoDB
from pivy.coin import SbTime
from pivy.coin import SoRenderManager

from .SignalThread import SignalThread


class SensorManager(QObject):

    def __init__(self):
        QObject.__init__(self, None)
        self._mainthread = QThread.currentThread()

        self._signalthread = SignalThread()
        QObject.connect(self._signalthread, SIGNAL("triggerSignal()"),
                        self.sensorQueueChanged)

        self._idletimer = QTimer()
        self._delaytimer = QTimer()
        self._timerqueuetimer = QTimer()

        self._idletimer.setSingleShot(True)
        self._delaytimer.setSingleShot(True)
        self._timerqueuetimer.setSingleShot(True)

        self.connect(self._idletimer, SIGNAL("timeout()"), self.idleTimeout)
        self.connect(self._delaytimer, SIGNAL("timeout()"), self.delayTimeout)
        self.connect(self._timerqueuetimer, SIGNAL("timeout()"), self.timerQueueTimeout)

        SoDB.getSensorManager().setChangedCallback(self.sensorQueueChangedCB, self)
        SoDB.setRealTimeInterval(1.0 / 25.0)
        SoRenderManager.enableRealTimeUpdate(False)

    def __del__(self):
        SoDB.getSensorManager().setChangedCallback(None, None)

        if self._signalthread.isRunning():
            self._signalthread.stopThread()
            self._signalthread.wait()

    def sensorQueueChangedCB(self, closure):
        # if we get a callback from another thread, route the callback
        # through SignalThread so that we receive the callback in the
        # QApplication thread (needed since QTimer isn't thread safe)
        if QThread.currentThread() != closure._mainthread:
            if not closure._signalthread.isRunning():
                closure._signalthread.start()
            self._signalthread.trigger()
        else:
            self.sensorQueueChanged()

    def sensorQueueChanged(self):
        sensormanager = SoDB.getSensorManager()
        assert(sensormanager)

        interval = sensormanager.isTimerSensorPending()

        if interval:
            interval -= SbTime.getTimeOfDay()

            if interval.getValue() <= 0.0:
                interval.setValue(1.0/5000.0)

            if not self._timerqueuetimer.isActive():
                # FIXME jkg: frodo changed this to time.getMsecValue() in C++ Quarter. test and apply.
                self._timerqueuetimer.start(interval.getMsecValue())
            else:
                self._timerqueuetimer.setInterval(interval.getMsecValue())
        elif self._timerqueuetimer.isActive():
            self._timerqueuetimer.stop()


        if sensormanager.isDelaySensorPending():
            self._idletimer.start(0)

            if not self._delaytimer.isActive():
                time = SoDB.getDelaySensorTimeout()

                if time != SbTime.zero():
                    self._delaytimer.start(interval.getMsecValue())
            else:
                if self._idletimer.isActive():
                    self._idletimer.stop()

                if self._delaytimer.isActive():
                    self._delaytimer.stop()

    def idleTimeout(self):
        SoDB.getSensorManager().processTimerQueue()
        SoDB.getSensorManager().processDelayQueue(True)
        self.sensorQueueChanged()

    def timerQueueTimeout(self):
        SoDB.getSensorManager().processTimerQueue()
        self.sensorQueueChanged()

    def delayTimeout(self):
        SoDB.getSensorManager().processTimerQueue()
        SoDB.getSensorManager().processDelayQueue(False)
        self.sensorQueueChanged()
