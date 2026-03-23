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

"""The DeviceManager class is responsible for holding a list of
  devices which can translate events such as a MouseHandler and
  KeyboardHandler for translation of mouse and keyboard events,
  respectively.

  Custom device handlers can be registered with this class for more
  functionality
"""
from __future__ import print_function

from pivy.qt.QtCore import QEvent
from pivy.qt.QtGui import QMouseEvent
from pivy.coin import SoLocation2Event
from pivy.coin import SbVec2s

#from pivy.quarter import DeviceHandler
#from pivy.quarter.QuarterWidget import QuarterWidget


#class DeviceManagerP {
#public:
#  QList<DeviceHandler *> devices;
#  QuarterWidget * quarterwidget;
#  SbVec2s lastmousepos;
#  QPoint globalpos;
#};

class DeviceManager:
    def __init__(self, quarterwidget):
        assert(quarterwidget)

        # NOTE jkg: equalient to DeviceManagerP
        self.devices = []
        self.quarterwidget = quarterwidget
        self.lastmousepos = SbVec2s(0, 0)

    def translateEvent(self, qevent):
        """Runs through the list of registered devices to translate event"""
        if qevent.type() == QEvent.MouseMove:
            self.globalpos = qevent.globalPos()

        for device in self.devices:
            soevent = device.translateEvent(qevent)
            if soevent:
                # cache mouse position so other devices can access it
                if (soevent.getTypeId() == SoLocation2Event.getClassTypeId()):
                    self.lastmousepos = soevent.getPosition()

                return soevent

        return None

    def getWidget(self):
        """Returns the QuarterWidget this devicemanager belongs to"""
        return self.quarterwidget

    def getLastGlobalPosition(self):
        """Returns the last mouse position in global coordinates"""
        return self.globalpos

    def getLastMousePosition(self):
        """Returns the last mouse position"""
        return self.lastmousepos;

    def registerDevice(self, device):
        """ Register a device for event translation"""
        if not device in self.devices:
            device.setManager(self)
            self.devices.append(device)

    def unregisterDevice(self, device):
        """unregister a device"""
        print("FIXME jkg: unregisterdevice not completely tested/ported")
        if device in self.devices:
            self.devices.removeAt(self.devices.indexOf(device))
        else:
            # FIXME jkg: give warning (not in original quarter)
            pass
