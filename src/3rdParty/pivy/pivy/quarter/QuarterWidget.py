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

r"""
  Quarter is a light-weight glue library that provides seamless
  integration between Systems in Motions's \COIN high-level 3D
  visualization library and Trolltech's \QT 2D user interface
  library.

  \QT and \COIN is a perfect match since they are both open source,
  widely portable and easy to use. Quarter has evolved from Systems in
  Motion's own experiences using \COIN and \QT together in our
  applications.

  The functionality in Quarter revolves around QuarterWidget, a
  subclass of QGLWidget. This widget provides functionality for
  rendering of Coin scenegraphs and translation of QEvents into
  SoEvents. Using this widget is as easy as using any other QWidget.

  \subpage QuarterWidgetPlugin

  Quarter also comes with a plugin for Qt Designer, Trolltech's tool
  for designing and building GUIs. Once you install Quarter, the
  QuarterWidget becomes accessible in Qt Designer, and you can include
  it in the user interfaces you create. The plugin facility also
  provides you with the capability of dynamically loading ui files
  containing a QuarterWidget in your application.

  By using \COIN, \QT and Quarter to build your 3D graphics
  applications, you have the power to write software that is portable
  across the whole range of UNIX, Linux, Microsoft Windows and Mac OS
  X operating systems, from a 100% common codebase.

  For a small, completely stand-alone usage example on how to
  initialize the library and set up a viewer instance window, see the
  following code:

  \code
  #include <Inventor/nodes/SoBaseColor.h>
  #include <Inventor/nodes/SoCone.h>
  #include <Inventor/nodes/coin.SoSeparator.h>

  #include <Quarter/QuarterWidget.h>
  #include <Quarter/QuarterApplication.h>

  using namespace SIM::Coin3D::Quarter;

  int
  main(int argc, char ** argv)
  {
    // Initializes SoQt library (and implicitly also the Coin and Qt
    // libraries).
    QuarterApplication app(argc, argv);

    // Make a dead simple scene graph by using the Coin library, only
    // containing a single yellow cone under the scenegraph root.
    coin.SoSeparator * root = new coin.SoSeparator;
    root->ref();

    SoBaseColor * col = new SoBaseColor;
    col->rgb = SbColor(1, 1, 0);
    root->addChild(col);

    root->addChild(new SoCone);

    // Create a QuarterWidget for displaying a Coin scene graph
    QuarterWidget * viewer = new QuarterWidget;
    viewer->setSceneGraph(root);

    // Pop up the QuarterWidget
    viewer->show();
    // Loop until exit.
    app.exec();
    // Clean up resources.
    root->unref();
    delete viewer;

    return 0;
  }
  \endcode

  \subpage examples

  \page examples More Examples

  The examples code is included in Quarter and can be found in the
  src/examples subdirectory.

  \subpage directui

  \subpage dynamicui

  \subpage inheritui

  \subpage mdi

  \subpage examiner
"""

from pivy.qt import QtCore, QtGui, QtOpenGL
from pivy.qt.QtOpenGL import QOpenGLWidget


from pivy import coin

from .devices import DeviceManager
from .devices import MouseHandler
from .devices import KeyboardHandler

from .eventhandlers import EventManager
from .eventhandlers import DragDropHandler

from .SensorManager import SensorManager
from .ImageReader import ImageReader

from .ContextMenu import ContextMenu


# FIXME 20080508 jkg: change to private/static method?
def renderCB(closure, manager):
    assert(closure)
    thisp = closure
    thisp.update()

# FIXME jkg: figure out what foo is
def statechangeCB(userdata, statemachine, stateid, enter, foo):
    if enter:
        assert(userdata)
        thisp = userdata
        if thisp.contextmenuenabled and stateid == "contextmenurequest":
            thisp.getContextMenu().exec_(thisp.devicemanager.getLastGlobalPosition())
        if stateid in thisp.statecursormap.keys():
            cursor = thisp.statecursormap[stateid]
            thisp.setCursor(cursor)

def prerenderCB(userdata, manager):
    thisp = userdata
    evman = thisp.soeventmanager
    assert(thisp and evman)
    for c in range(evman.getNumSoScXMLStateMachines()):
        statemachine = evman.getSoScXMLStateMachine(c)
        statemachine.preGLRender()

def postrenderCB(userdata, manager):
    thisp = userdata
    evman = thisp.soeventmanager
    assert(evman)
    for c in range(evman.getNumSoScXMLStateMachines()):
        statemachine = evman.getSoScXMLStateMachine(c)
        statemachine.postGLRender()


class QuarterWidget(QOpenGLWidget):

    _sensormanager = None
    _imagereader = None

    def __init__(self, *args, **kwargs):
        """
        Constructs a QuarterWidget.
        QuarterWidget(QWidget parent = None, QOpenGLWidget sharewidget = None, Qt.WindowFlags f = 0, scxml = "coin:scxml/navigation/examiner.xml")
        QuarterWidget(QSurfaceFormat format, QWidget parent = None, QOpenGLWidget sharewidget = None, Qt.WindowFlags f = 0, scxml = "coin:scxml/navigation/examiner.xml")
        """

        params = ["parent", "sharewidget"]
        values = {"parent": None, "sharewidget": None, "f": 0, "scxml": "coin:scxml/navigation/examiner.xml"}
        values.update(kwargs)
        
        if len(args) > 0 and isinstance(args[0], QtGui.QSurfaceFormat) or "format" in kwargs:
            params.insert(0, "format")

        if len(args) > len(params):
            values["f"] = args[len(params)]

        if len(args) > len(params) + 1:
            values["scxml"] = args[len(params) + 1]

        QOpenGLWidget.__init__(self, values["parent"])
        if "f" in values and values["f"]: self.setWindowFlags(values["f"])
        if "format" in values: self.setFormat(values["format"])

        # initialize Sensormanager and ImageReader instances only once
        if not QuarterWidget._sensormanager:
            QuarterWidget._sensormanager = SensorManager()

        if not QuarterWidget._imagereader:
            QuarterWidget._imagereader = ImageReader()

        self.cachecontext_list = []
        self.cachecontext = self.findCacheContext(self, values["sharewidget"])
        self.statecursormap = {}

        self.scene = None
        self.contextmenu = None
        self.contextmenuenabled = True

        self.sorendermanager = coin.SoRenderManager()
        self.soeventmanager = coin.SoEventManager()

        # Mind the order of initialization as the XML state machine uses
        # callbacks which depends on other state being initialized
        self.eventmanager = EventManager(self)
        self.devicemanager = DeviceManager(self)

        statemachine = coin.ScXML.readFile(values["scxml"])
        if statemachine and statemachine.isOfType(coin.SoScXMLStateMachine.getClassTypeId()):
            sostatemachine = coin.cast(statemachine, "SoScXMLStateMachine")
            statemachine.addStateChangeCallback(statechangeCB, self)
            self.soeventmanager.addSoScXMLStateMachine(sostatemachine)
            sostatemachine.initialize()
        else:
            raise "could not initialize statemachine, given file not found?"

        self.headlight = coin.SoDirectionalLight()

        self.sorendermanager.setAutoClipping(coin.SoRenderManager.VARIABLE_NEAR_PLANE)
        self.sorendermanager.setRenderCallback(renderCB, self)
        self.sorendermanager.setBackgroundColor(coin.SbColor4f(0, 0, 0, 0))
        self.sorendermanager.activate()
        self.sorendermanager.addPreRenderCallback(prerenderCB, self)
        self.sorendermanager.addPostRenderCallback(postrenderCB, self)

        self.soeventmanager.setNavigationState(coin.SoEventManager.MIXED_NAVIGATION)

        self.devicemanager.registerDevice(MouseHandler())
        self.devicemanager.registerDevice(KeyboardHandler())
        self.eventmanager.registerEventHandler(DragDropHandler())

        # set up a cache context for the default SoGLRenderAction
        self.sorendermanager.getGLRenderAction().setCacheContext(self.getCacheContextId())

        self.setStateCursor("interact", QtCore.Qt.ArrowCursor)
        self.setStateCursor("idle", QtCore.Qt.OpenHandCursor)
        self.setStateCursor("rotate", QtCore.Qt.ClosedHandCursor)
        self.setStateCursor("pan", QtCore.Qt.SizeAllCursor)
        self.setStateCursor("zoom", QtCore.Qt.SizeVerCursor)
        self.setStateCursor("seek", QtCore.Qt.CrossCursor)
        self.setStateCursor("spin", QtCore.Qt.OpenHandCursor)

        self.setMouseTracking(True)
        self.setFocusPolicy(QtCore.Qt.StrongFocus)

    @property
    def sceneGraph(self):
        return self.scene

    @sceneGraph.setter
    def sceneGraph(self, node, *args):
        self.setSceneGraph(node)

    def setSceneGraph(self, node):
        if node and self.scene==node:
            return

        camera = None
        superscene = None
        viewall = False

        if node:
            self.scene = node
            self.scene.ref()

            superscene = coin.SoSeparator()
            superscene.addChild(coin.SoDepthBuffer())
            superscene.addChild(self.headlight)

            camera = self.searchForCamera(node)
            if not camera:
                camera = coin.SoPerspectiveCamera()
                superscene.addChild(camera)
                viewall = True

            superscene.addChild(node)

        self.soeventmanager.setSceneGraph(superscene)
        self.sorendermanager.setSceneGraph(superscene)
        self.soeventmanager.setCamera(camera)
        self.sorendermanager.setCamera(camera)

        if viewall:
            self.viewAll()

        if superscene:
            superscene.touch()

    def viewAll(self):
        """ Reposition the current camera to display the entire scene"""
        viewallevent = coin.SbName("sim.coin3d.coin.navigation.ViewAll")
        for c in range(self.soeventmanager.getNumSoScXMLStateMachines()):
            sostatemachine = self.soeventmanager.getSoScXMLStateMachine(c)
            if (sostatemachine.isActive()):
                sostatemachine.queueEvent(viewallevent)
                sostatemachine.processEventQueue()

    def resizeGL(self, width, height):
        vp = coin.SbViewportRegion(width, height)
        self.sorendermanager.setViewportRegion(vp)
        self.soeventmanager.setViewportRegion(vp)

    def paintGL(self, *args):
        self.actualRedraw()

    def actualRedraw(self, *args):
        self.sorendermanager.render(True, True)

    def scheduleRedraw(self, *args):
        self.sorendermanager.scheduleRedraw()

    def event(self, qevent):
        """Translates Qt Events into Coin events and passes them on to the
          scenemanager for processing. If the event can not be translated or
          processed, it is forwarded to Qt and the method returns false. This
          method could be overridden in a subclass in order to catch events of
          particular interest to the application programmer."""

        if self.eventmanager.handleEvent(qevent):
            return True

        soevent = self.devicemanager.translateEvent(qevent)
        if (soevent and self.soeventmanager.processEvent(soevent)):
            return True

        # NOTE jkg: we must return True or False
        return QOpenGLWidget.event(self, qevent)

    def setStateCursor(self, state, cursor):
        self.statecursormap[state] = cursor

    def searchForCamera(self, root):
        sa = coin.SoSearchAction()
        sa.setInterest(coin.SoSearchAction.FIRST)
        sa.setType(coin.SoCamera.getClassTypeId())
        sa.apply(root)

        if sa.getPath():
            node = sa.getPath().getTail()
            if node and node.isOfType(coin.SoCamera.getClassTypeId()):
                return node
        return None

    def getCacheContextId(self):
        return self.cachecontext.id

    def findCacheContext(self, widget, sharewidget):

        class QuarterWidgetP_cachecontext:
            def __init__(self):
                self.widgetlist = []
                self.id = None

        for cachecontext in self.cachecontext_list:
            for widget in cachecontext.widgetlist:
                if (widget == sharewidget):
                    cachecontext.widgetlist.append(widget)
                    return cachecontext;
        cachecontext = QuarterWidgetP_cachecontext()
        cachecontext.id = coin.SoGLCacheContextElement.getUniqueCacheContext()
        cachecontext.widgetlist.append(widget)
        self.cachecontext_list.append(cachecontext)

        return cachecontext

    def getSoRenderManager(self):
        return self.sorendermanager

    def getSoEventManager(self):
        return self.soeventmanager

    def setBackgroundColor(self, color):
        """Set backgroundcolor to a given QColor
          Remember that QColors are given in integers between 0 and 255, as
          opposed to SbColor4f which is in [0 ,1]. The default alpha value for
          a QColor is 255, but you'll probably want to set it to zero before
          using it as an OpenGL clear color."""
        if isinstance(color, QtGui.QColor):
            bgcolor = coin.SbColor4f(max(0, min(1, color.red() / 255.0)),
                                     max(0, min(1, color.green() / 255.0)),
                                     max(0, min(1, color.blue() / 255.0)),
                                     max(0, min(1, color.alpha() / 255.0)))
            self.sorendermanager.setBackgroundColor(bgcolor)
        else:
            self.sorendermanager.setBackgroundColor(coin.SbColor4f(color))

    def getBackgroundColor(self):
        """  Returns color used for clearing the rendering area before
          rendering the scene."""

        bg = self.sorendermanager.getBackgroundColor()

        return QtGui.QColor(max(0, min(255, int(bg[0] * 255.0))),
                            max(0, min(255, int(bg[1] * 255.0))),
                            max(0, min(255, int(bg[2] * 255.0))),
                            max(0, min(255, int(bg[3] * 255.0))))

    def getContextMenu(self):
        """Returns the context menu used by the widget."""
        if not self.contextmenu:
            self.contextmenu = ContextMenu(self)
        # NOTE 20080508 jkg: seems like we can drop .getMenu() but I dont see why that works
        return self.contextmenu.getMenu()

    def contextMenuEnabled(self):
        return self.contextmenuenabled

    def enableContextMenu(self, yesno):
        self.contextmenuenabled = yesno

    def setTransparencyType(self, type):
        """This method sets the transparency type to be used for the scene."""
        assert(self.sorendermanager)
        self.sorendermanager.getGLRenderAction().setTransparencyType(type)
        self.sorendermanager.scheduleRedraw()

    def enableHeadlight(self, onoff):
        """  Enable/disable the headlight. This will toggle the SoDirectionalLigh::on
          field (returned from getHeadlight())."""
        self.headlight.on = onoff

    def getHeadlight(self):
        """Returns the light used for the headlight."""
        return self.headlight
