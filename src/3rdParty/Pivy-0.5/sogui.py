#!/usr/bin/env python

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

"""This module provides a proxy class for the different Pivy SoGui bindings.

The following special variables can be set before this module is imported:

* SOGUI_DEBUG: if 'True' will print debug output

* SOGUI_BINDING: allows to specify an SoGui binding (e.g. 'SoQt') explicitly
"""

import sys

class SoGui_Proxy:
    """Probes for existing SoGui bindings and proxies method
    invocations to their SoGui counterparts."""

    def __init__(self, gui, debug):
        self.debug = debug

        # if no binding has been specified check for availability of a known
        # one in a defined order Quarter -> SoQt -> SoWin -> SoXt -> SoGtk
        if not gui:
            try:
                sogui = __import__('pivy.gui.soqt').gui.soqt
                gui = 'SoQt'
            except ImportError:
                try:
                    sogui = __import__('pivy.gui.sowin').gui.sowin
                    gui = 'SoWin'
                except ImportError:
                    try:
                        sogui = __import__('pivy.gui.soxt').gui.soxt
                        gui = 'SoXt'
                    except ImportError:
                        try:
                            sogui = __import__('pivy.gui.sogtk').gui.sogtk
                            gui = 'SoGtk'
                        except ImportError:
                            print 'SoGui proxy error: None of the known Gui bindings were found! Please specify one!'
                            sys.exit(1)

        # check if object is a user provided string possibly a new unknown SoGui binding.
        # try to bind it.
        elif type(gui) == type(''):
            try:
                sogui = getattr(__import__('pivy.gui.' + gui.lower()).gui, gui.lower())
            except ImportError:
                print 'SoGui proxy error: The specified Gui binding could not be bound!'
                sys.exit(1)

        # get a handle to our global module dictionary
        d = sys.modules[self.__module__].__dict__

        # add references to the possible classes in the corresponding
        # SoGui binding to our global (module) namespace
        for suffix in ['Cursor', 'Component', 'GLWidget', 'RenderArea',
                       'Viewer', 'FullViewer', 'FlyViewer', 'PlaneViewer',
                       'Device', 'Keyboard', 'Mouse',
                       'ExaminerViewer', 'ConstrainedViewer']:
            d['SoGui' + suffix] = eval('sogui.%s%s' % (gui, suffix))

        self.__gui__ = eval('sogui.' + gui)

    def __getattr__(self, name):
        if self.debug:
            print 'SoGui_Proxy: __getattr__() called for %s().' % name
        return getattr(self.__gui__, name)

    def __repr__(self):
        return 'SoGui proxy for ' + `self.__gui__`

    def __hash__(self):
        return 0

    __str__ = __repr__

# look for user overrides in the main dictionary of the interpreter
gui, debug = None, False

if sys.modules.has_key('__main__'):
    try:
        debug = sys.modules['__main__'].SOGUI_DEBUG
    except AttributeError:
        pass

    try:
        gui = sys.modules['__main__'].SOGUI_BINDING
    except AttributeError:
        pass

# if no GUI has been specified try using Quarter
if not gui:
    try:
        __import__('PyQt4')
        quarter = __import__('pivy.quarter').quarter
        gui = 'Quarter'

        import PyQt4
        import pivy
        
        class SoGui(object):
            @staticmethod
            def init(*args):
                SoGui_Quarter_Wrapper.qApp = PyQt4.QtGui.QApplication(sys.argv)
                return pivy.quarter.QuarterWidget()

            @staticmethod
            def mainLoop():
                SoGui_Quarter_Wrapper.qApp.exec_()

            @staticmethod
            def show(mainwindow):
                pass

        class SoGuiViewer(object):
            BROWSER, EDITOR = range(2)

            VIEW_AS_IS, VIEW_HIDDEN_LINE, VIEW_NO_TEXTURE, \
            VIEW_LOW_COMPLEXITY, VIEW_LINE, VIEW_POINT, VIEW_BBOX, \
            VIEW_LOW_RES_LINE, VIEW_LOW_RES_POINT, VIEW_SAME_AS_STILL, \
            VIEW_WIREFRAME_OVERLAY = range(11)

            STILL, INTERACTIVE = range(2)
            BUFFER_SINGLE, BUFFER_DOUBLE, BUFFER_INTERACTIVE = range(3)
            VARIABLE_NEAR_PLANE, CONSTANT_NEAR_PLANE = range(2)
            
        class SoGui_Quarter_Wrapper(object):
            """A SoGui QuarterWidget wrapper"""

            def __init__(self, mainwindow):
                self._root = None
                self.quarterwidget = mainwindow

            def getCamera(self):
                rendermgr = self.quarterwidget.getSoRenderManager()
                return rendermgr.getCamera()
                
            def getSize(self):
                size = self.quarterwidget.size()
                return pivy.coin.SbVec2s((size.width, size.height))

            def getViewportRegion(self):
                return pivy.coin.SbViewportRegion()

            def redrawOnSelectionChange(self, selection):
                # FIXME: investigate. 20080509 tamer.
                pass

            def setBackgroundColor(self, color):
                self.quarterwidget.setBackgroundColor(color)

            def setDrawStyle(self, type, style):
                # FIXME: we happily ignore the type. investigate. 20080509 tamer.
                rendermgr = self.quarterwidget.getSoRenderManager()
                rendermgr.setRenderMode(style)

            def setGLRenderAction(self, renderaction):
                rendermgr = self.quarterwidget.getSoRenderManager()
                rendermgr.setGLRenderAction(renderaction)
                
            def setHeadlight(self, onOff):
                self.quarterwidget.enableHeadlight(onOff)

            def setOverlaySceneGraph(self, overlay):
                # FIXME: investigate. 20080509 tamer.
                pass                
                
            def setSceneGraph(self, root):
                self.quarterwidget.setSceneGraph(root)
                self._root = root

            def setSize(self, size):
                self.quarterwidget.resize(size[0], size[1])

            def setTitle(self, title):
                self.quarterwidget.setWindowTitle(title)

            def show(self):
                self.quarterwidget.show()

            def viewAll(self):
                self.quarterwidget.viewAll()

        SoGuiExaminerViewer = SoGui_Quarter_Wrapper
        SoGuiRenderArea = SoGui_Quarter_Wrapper
    except ImportError:
        # instantiate the SoGui proxy
        SoGui = SoGui_Proxy(gui, debug)
else:
    SoGui = SoGui_Proxy(gui, debug)
