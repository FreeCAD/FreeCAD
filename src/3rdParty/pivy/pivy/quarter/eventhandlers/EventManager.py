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

class EventManager:
    """ The EventManager class is responsible for holding a list of
      devices which can translate events such as a MouseHandler and
      KeyboardHandler for translation of mouse and keyboard events,
      respectively.

      Custom device handlers can be registered with this class for more
      functionality
    """
    def __init__(self, quarterwidget):
        assert(quarterwidget)
        self.quarterwidget = quarterwidget
        self.eventhandlers = []

    def handleEvent(self, qevent):
        """Runs through the list of registered devices to translate events"""
        for handler in self.eventhandlers:
            if handler.handleEvent(qevent):
                return True
        return False

    def getWidget(self):
        """Returns the QuarterWidget this devicemanager belongs to"""
        return self.quarterwidget

    def registerEventHandler(self, handler):
        """Register a device for event translation"""
        if (not handler in self.eventhandlers):
            handler.setManager(self)
            self.eventhandlers.append(handler)

    def unregisterEventHandler(self, handler):
        """unregister a device"""
        if handler in self.eventhandlers:
            self.eventhandlers.removeAt(self.eventhandlers.indexOf(handler))
