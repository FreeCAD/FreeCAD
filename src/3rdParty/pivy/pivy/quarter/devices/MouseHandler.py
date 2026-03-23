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

from pivy.qt import QtCore, QtGui

from pivy import coin

from .DeviceHandler import DeviceHandler

class MouseHandler(DeviceHandler):
    def __init__(self):
        """The MouseHandler class provides translation of mouse events
        on the QuarterWidget. It is registered with the DeviceManager by
        default."""

        self.location2 = coin.SoLocation2Event()
        self.mousebutton = coin.SoMouseButtonEvent()
        self.windowsize = coin.SbVec2s(-1, -1)

    def translateEvent(self, qevent):
        """Translates from QMouseEvents to SoLocation2Events and SoMouseButtonEvents"""

        if qevent.type() == QtCore.QEvent.MouseMove:
            return self.mouseMoveEvent(qevent)

        if qevent.type() in (QtCore.QEvent.MouseButtonPress, QtCore.QEvent.MouseButtonRelease):
            return self.mouseButtonEvent(qevent)

        if qevent.type() == QtCore.QEvent.Wheel:
            return self.mouseWheelEvent(qevent)

        if qevent.type() == QtCore.QEvent.Resize:
            self.resizeEvent(qevent)

        return None


    def resizeEvent(self, qevent):
        self.windowsize = coin.SbVec2s(qevent.size().width(),
                                       qevent.size().height())

    def mouseMoveEvent(self, qevent):
        self.setModifiers(self.location2, qevent)

        assert(self.windowsize[1] != -1)
        pos = coin.SbVec2s(qevent.pos().x(), self.windowsize[1] - qevent.pos().y() - 1)
        self.location2.setPosition(pos)
        return self.location2

    def mouseWheelEvent(self, qevent):

        # FIXME 20080509 jkg: zooming with mouse wheel seems to not work.
        # At least it does not work in the original Quarter examples either.

        self.setModifiers(self.mousebutton, qevent)
        self.mousebutton.setPosition(self.location2.getPosition())

        # QWheelEvent::delta() returns the distance that the wheel is
        # rotated, in eights of a degree. A positive value indicates that
        # the wheel was rotated forwards away from the user; a negative
        # value indicates that the wheel was rotated backwards toward the
        # user.

        if qevent.angleDelta().y() > 0:
            self.mousebutton.setButton(coin.SoMouseButtonEvent.BUTTON4)
        else:
            self.mousebutton.setButton(coin.SoMouseButtonEvent.BUTTON5)

        self.mousebutton.setState(coin.SoButtonEvent.DOWN)
        return self.mousebutton


    def mouseButtonEvent(self, qevent):
        self.setModifiers(self.mousebutton, qevent)
        self.mouseMoveEvent(qevent) # NOTE jkg: mouseMoveEvent not triggered when showing popup menu in PyQt
        self.mousebutton.setPosition(self.location2.getPosition())

        if qevent.type() == QtCore.QEvent.MouseButtonPress:
            self.mousebutton.setState(coin.SoButtonEvent.DOWN)
        else:
            self.mousebutton.setState(coin.SoButtonEvent.UP)

        if qevent.button() == QtCore.Qt.LeftButton:
            self.mousebutton.setButton(coin.SoMouseButtonEvent.BUTTON1)
        elif qevent.button() == QtCore.Qt.RightButton:
            self.mousebutton.setButton(coin.SoMouseButtonEvent.BUTTON2)
        elif qevent.button() == QtCore.Qt.MiddleButton:
            self.mousebutton.setButton(coin.SoMouseButtonEvent.BUTTON3)
        else:
            self.mousebutton.setButton(coin.SoMouseButtonEvent.ANY)
            coin.SoDebugError.postInfo("MouseHandler.mouseButtonEvent",
                                  "Unhandled ButtonState = %x", qevent.button())
        return self.mousebutton
