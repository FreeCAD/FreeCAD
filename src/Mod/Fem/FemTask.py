# ***************************************************************************
# *                                                                         *
# *   Copyright (c) 2017 - Markus Hovorka <m.hovorka@live.de>               *
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


__title__ = "FemTask"
__author__ = "Markus Hovorka"
__url__ = "http://www.freecadweb.org"


import threading
import time

import FemReport
import FemSignal


class Task(object):

    def __init__(self):
        self.report = None
        self.signalStarting = set()
        self.signalStarted = set()
        self.signalStoping = set()
        self.signalStoped = set()
        self.signalAbort = set()
        self.signalStatus = set()
        self.signalStatusCleared = set()
        self.startTime = None
        self.stopTime = None
        self.running = False
        self._aborted = False
        self._failed = False
        self._status = []

        def stoping():
            self.stopTime = time.time()
            self.running = False
        self.signalStoping.add(stoping)

    @property
    def time(self):
        if self.startTime is not None:
            endTime = (
                self.stopTime
                if self.stopTime is not None
                else time.time())
            return endTime - self.startTime
        return None

    @property
    def failed(self):
        return self._failed

    @property
    def aborted(self):
        return self._aborted

    @property
    def status(self):
        return "".join(self._status)

    def start(self):
        self.report = FemReport.Report()
        self.clearStatus()
        self._aborted = False
        self._failed = False
        self.stopTime = None
        self.startTime = time.time()
        self.running = True
        FemSignal.notify(self.signalStarting)
        FemSignal.notify(self.signalStarted)

    def run(self):
        raise NotImplementedError()

    def join(self):
        raise NotImplementedError()

    def abort(self):
        self._aborted = True
        FemSignal.notify(self.signalAbort)

    def fail(self):
        self._failed = True

    def pushStatus(self, line):
        self._status.append(line)
        FemSignal.notify(self.signalStatus, line)

    def clearStatus(self):
        self._status = []
        FemSignal.notify(self.signalStatusCleared)

    def protector(self):
        try:
            self.run()
        except:
            self.fail()
            raise


class Thread(Task):

    def __init__(self):
        super(Thread, self).__init__()
        self._thread = None

    def start(self):
        super(Thread, self).start()
        self._thread = threading.Thread(
            target=self.protector)
        self._thread.daemon = True
        self._thread.start()
        self._attachObserver()

    def join(self):
        if self._thread is not None:
            self._thread.join()

    def _attachObserver(self):
        def waitForStop():
            self._thread.join()
            FemSignal.notify(self.signalStoping)
            FemSignal.notify(self.signalStoped)
        thread = threading.Thread(target=waitForStop)
        thread.daemon = True
        thread.start()
