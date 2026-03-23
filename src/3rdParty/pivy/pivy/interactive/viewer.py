"""3D viewer widget for interactive graphics visualization.

This module provides a Qt-based 3D viewer widget using Quarter (Coin3D/Qt integration)
for displaying and interacting with 3D graphics scenes.
"""

from __future__ import print_function
from pivy import quarter, coin
from pivy.qt import QtGui, QtCore
import tempfile

class Viewer(quarter.QuarterWidget):
    """A 3D graphics viewer widget based on Quarter.

    This widget provides a standalone 3D viewer window with an orthographic camera
    and white background. It can be used to display Coin3D scene graphs and optionally
    export rendered frames as images.

    Attributes:
        sg: The root scene graph separator node. Add child nodes to this to display
            objects in the viewer.
        app: The QApplication instance used by the viewer.

    Example:
        >>> viewer = Viewer()
        >>> viewer.sg += [coin.SoCube()]
        >>> viewer.show()
    """
    def __init__(self, *args, **kwrds):
        """Initialize the viewer widget.

        Sets up a Qt application (or reuses existing one), creates an orthographic
        camera, and configures the viewer with a white background and window flags
        to keep it on top.

        Args:
            *args: Positional arguments passed to QuarterWidget.
            **kwrds: Keyword arguments passed to QuarterWidget.
        """
        try:
            self.app = QtGui.QApplication([])
        except RuntimeError:
            self.app = QtGui.QApplication.instance()

        super(Viewer, self).__init__(*args, **kwrds)
        self.sg = coin.SoSeparator()
        self.sg += [coin.SoOrthographicCamera()]
        self.setSceneGraph(self.sg)
        self.setBackgroundColor(coin.SbColor(1,1,1))
        self.setWindowFlags(self.windowFlags() | QtCore.Qt.WindowStaysOnTopHint)

    def show(self, exec_widget=True):
        """Display the viewer window and start the event loop.

        Shows the viewer window, fits the view to show all objects, positions
        the window in the bottom-right corner of the screen, and optionally
        runs the Qt event loop.

        Args:
            exec_widget: If True, runs QApplication.exec_() to start the event loop.
                If False, uses a timer to close the widget after a short delay
                (useful for automated screenshots).

        Returns:
            IPython.display.Image or None: If IPython is available and exec_widget
                is False, returns an Image object of the rendered frame. Otherwise
                returns None.
        """
        super(Viewer, self).show()
        self.viewAll()
        rec = self.app.desktop().screenGeometry()
        self.move(rec.width() - self.size().width(), 
                  rec.height() - self.size().height())
        if not exec_widget:
            timer = QtCore.QTimer()
            # timer.timeout.connect(self.close)
            timer.singleShot(20, self.close)
        self.app.exec_()
        try:
            from IPython.display import Image
            return Image(self.name)
        except ImportError as e:
            print(e)



    def closeEvent(self, *args):
        """Handle the widget close event.

        Captures the current frame buffer as an image and saves it to a temporary
        PNG file when the viewer is closed. The filename is stored in self.name.

        Args:
            *args: Event arguments passed by Qt (unused).
        """
        image = self.grabFrameBuffer()
        _, name = tempfile.mkstemp(suffix=".png")
        image.save(name, "png")
        self.name = name
