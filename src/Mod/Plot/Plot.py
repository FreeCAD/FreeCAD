# ***************************************************************************
# *   Copyright (c) 2011, 2012 Jose Luis Cercos Pita <jlcercos@gmail.com>   *
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

import FreeCAD

import PySide
from PySide import QtCore, QtGui
from distutils.version import LooseVersion as V
import sys

try:
    import matplotlib

    matplotlib.use("Qt5Agg")

    # Force matplotlib to use PySide backend by temporarily unloading PyQt
    if "PyQt5.QtCore" in sys.modules:
        del sys.modules["PyQt5.QtCore"]
        import matplotlib.pyplot as plt
        import PyQt5.QtCore
    else:
        import matplotlib.pyplot as plt

    from matplotlib.backends.backend_qt5agg import FigureCanvasQTAgg as FigureCanvas
    from matplotlib.backends.backend_qt5agg import NavigationToolbar2QT as NavigationToolbar

    from matplotlib.figure import Figure
except ImportError:
    FreeCAD.Console.PrintWarning(
        "The 'matplotlib' Python package was not found. Plot module cannot be loaded\n"
    )
    raise ImportError("matplotlib not installed")


def getMainWindow():
    """Return the FreeCAD main window."""
    toplevel = PySide.QtGui.QApplication.topLevelWidgets()
    for i in toplevel:
        if i.metaObject().className() == "Gui::MainWindow":
            return i
    return None


def getMdiArea():
    """Return FreeCAD MdiArea."""
    mw = getMainWindow()
    if not mw:
        return None
    childs = mw.children()
    for c in childs:
        if isinstance(c, PySide.QtGui.QMdiArea):
            return c
    return None


def getPlot():
    """Return the selected Plot document if exist."""
    # Get active tab
    mdi = getMdiArea()
    if not mdi:
        return None
    sub = mdi.activeSubWindow()
    if not sub:
        return None
    # Explore childrens looking for Plot class
    for i in sub.children():
        if i.metaObject().className() == "Plot":
            return i
    return None


def closePlot():
    """closePlot(): Close the active plot window."""
    # Get active tab
    mdi = getMdiArea()
    if not mdi:
        return None
    sub = mdi.activeSubWindow()
    if not sub:
        return None
    # Explore childrens looking for Plot class
    for i in sub.children():
        if i.metaObject().className() == "Plot":
            sub.close()


def figure(winTitle="plot"):
    """Create a new plot subwindow/tab.

    Keyword arguments:
    winTitle -- Plot tab title.
    """
    mdi = getMdiArea()
    if not mdi:
        return None
    win = Plot(winTitle)
    sub = mdi.addSubWindow(win)
    sub.show()
    return win


def plot(x, y, name=None):
    """Plots a new serie (as line plot)

    Keyword arguments:
    x -- X values
    y -- Y values
    name -- Data serie name (for legend).
    """
    # Get active plot, or create another one if don't exist
    plt = getPlot()
    if not plt:
        plt = figure()
    # Call to plot
    return plt.plot(x, y, name)


def series():
    """Return all the lines from a selected plot."""
    plt = getPlot()
    if not plt:
        return []
    return plt.series


def removeSerie(index):
    """Remove a data serie from the active plot.

    Keyword arguments:
    index -- Index of the serie to remove.
    """
    # Get active series
    plt = getPlot()
    if not plt:
        return
    plots = plt.series
    if not plots:
        return
    # Remove line from plot
    axes = plots[index].axes
    axes.lines.pop(plots[index].lid)
    # Remove serie from list
    del plt.series[index]
    # Update GUI
    plt.update()


def legend(status=True, pos=None, fontsize=None):
    """Show/Hide the legend from the active plot.

    Keyword arguments:
    status -- True if legend must be shown, False otherwise.
    pos -- Legend position.
    fontsize -- Font size
    """
    plt = getPlot()
    if not plt:
        return
    plt.legend = status
    if fontsize:
        plt.legSiz = fontsize
    # Hide all legends
    for axes in plt.axesList:
        axes.legend_ = None
    # Legend must be activated on last axes
    axes = plt.axesList[-1]
    if status:
        # Setup legend handles and names
        lines = series()
        handles = []
        names = []
        for l in lines:
            if l.name is not None:
                handles.append(l.line)
                names.append(l.name)
        # Show the legend (at selected position or at best)
        if pos:
            l = axes.legend(handles, names, bbox_to_anchor=pos)
            plt.legPos = pos
        else:
            l = axes.legend(handles, names, loc="best")
            # Update canvas in order to compute legend data
            plt.canvas.draw()
            # Get resultant position
            try:
                fax = axes.get_frame().get_extents()
            except Exception:
                fax = axes.patch.get_extents()
            fl = l.get_frame()
            plt.legPos = (
                (fl._x + fl._width - fax.x0) / fax.width,
                (fl._y + fl._height - fax.y0) / fax.height,
            )
        # Set fontsize
        for t in l.get_texts():
            t.set_fontsize(plt.legSiz)
    plt.update()


def grid(status=True):
    """Show/Hide the grid from the active plot.

    Keyword arguments:
    status -- True if grid must be shown, False otherwise.
    """
    plt = getPlot()
    if not plt:
        return
    plt.grid = status
    axes = plt.axes
    axes.grid(status)
    plt.update()


def title(string):
    """Setup the plot title.

    Keyword arguments:
    string -- Plot title.
    """
    plt = getPlot()
    if not plt:
        return
    axes = plt.axes
    axes.set_title(string)
    plt.update()


def xlabel(string):
    """Setup the x label.

    Keyword arguments:
    string -- Title to set.
    """
    plt = getPlot()
    if not plt:
        return
    axes = plt.axes
    axes.set_xlabel(string)
    plt.update()


def ylabel(string):
    """Setup the y label.

    Keyword arguments:
    string -- Title to set.
    """
    plt = getPlot()
    if not plt:
        return
    axes = plt.axes
    axes.set_ylabel(string)
    plt.update()


def axesList():
    """Return the plot axes sets list."""
    plt = getPlot()
    if not plt:
        return []
    return plt.axesList


def axes():
    """Return the active plot axes."""
    plt = getPlot()
    if not plt:
        return None
    return plt.axes


def addNewAxes(rect=None, frameon=True, patchcolor="none"):
    """Add new axes to plot, setting it as the active one.

    Keyword arguments:
    rect -- Axes area, None to copy from the last axes data.
    frameon -- True to show frame, False otherwise.
    patchcolor -- Patch color, 'none' for transparent plot.
    """
    plt = getPlot()
    if not plt:
        return None
    fig = plt.fig
    if rect is None:
        rect = plt.axes.get_position()
    ax = fig.add_axes(rect, frameon=frameon)
    ax.xaxis.set_ticks_position("bottom")
    ax.spines["top"].set_color("none")
    ax.yaxis.set_ticks_position("left")
    ax.spines["right"].set_color("none")
    ax.patch.set_facecolor(patchcolor)
    plt.axesList.append(ax)
    plt.setActiveAxes(-1)
    plt.update()
    return ax


def save(path, figsize=None, dpi=None):
    """Save plot.

    Keyword arguments:
    path -- Destination file path.
    figsize -- w,h figure size tuple in inches.
    dpi -- Dots per inch.
    """
    plt = getPlot()
    if not plt:
        return
    # Backup figure options
    fig = plt.fig
    sizeBack = fig.get_size_inches()
    dpiBack = fig.get_dpi()
    # Save figure with new options
    if figsize:
        fig.set_size_inches(figsize[0], figsize[1])
    if dpi:
        fig.set_dpi(dpi)
    plt.canvas.print_figure(path)
    # Restore figure options
    fig.set_size_inches(sizeBack[0], sizeBack[1])
    fig.set_dpi(dpiBack)
    plt.update()


def addNavigationToolbar():
    """Add the matplotlib QT navigation toolbar to the plot."""
    plt = getPlot()
    if not plt:
        return
    # Check that the navigation toolbar has not been already created
    if plt.mpl_toolbar is not None:
        return
    # Create the navigation toolbar and add it
    plt.mpl_toolbar = NavigationToolbar(plt.canvas, plt)
    vbox = plt.layout()
    vbox.addWidget(plt.mpl_toolbar)


def delNavigationToolbar():
    """Remove the matplotlib QT navigation toolbar from the plot."""
    plt = getPlot()
    if not plt:
        return
    # Check that the navigation toolbar already exist
    if plt.mpl_toolbar is None:
        return
    # Remove the widget from the layout
    vbox = plt.layout()
    vbox.removeWidget(plt.mpl_toolbar)
    # Destroy the navigation toolbar
    plt.mpl_toolbar.deleteLater()
    plt.mpl_toolbar = None


class Line:
    def __init__(self, axes, x, y, name):
        """Construct a new plot serie.

        Keyword arguments:
        axes -- Active axes
        x -- X values
        y -- Y values
        name -- Data serie name (for legend).
        """
        self.axes = axes
        self.x = x
        self.y = y
        self.name = name
        self.lid = len(axes.lines)
        (self.line,) = axes.plot(x, y)

    def setp(self, prop, value):
        """Change a line property value.

        Keyword arguments:
        prop -- Property name.
        value -- New property value.
        """
        plt.setp(self.line, prop, value)

    def getp(self, prop):
        """Get line property value.

        Keyword arguments:
        prop -- Property name.
        """
        return plt.getp(self.line, prop)


class Plot(PySide.QtGui.QWidget):
    def __init__(self, winTitle="plot", parent=None, flags=PySide.QtCore.Qt.WindowFlags(0)):
        """Construct a new plot widget.

        Keyword arguments:
        winTitle -- Tab title.
        parent -- Widget parent.
        flags -- QWidget flags
        """
        PySide.QtGui.QWidget.__init__(self, parent, flags)
        self.setWindowTitle(winTitle)
        # Create matplotlib canvas
        self.fig = Figure()
        self.canvas = FigureCanvas(self.fig)
        self.canvas.setParent(self)
        # Get axes
        self.axes = self.fig.add_subplot(111)
        self.axesList = [self.axes]
        self.axes.xaxis.set_ticks_position("bottom")
        self.axes.spines["top"].set_color("none")
        self.axes.yaxis.set_ticks_position("left")
        self.axes.spines["right"].set_color("none")
        # Add the navigation toolbar by default
        self.mpl_toolbar = NavigationToolbar(self.canvas, self)
        # Setup layout
        vbox = PySide.QtGui.QVBoxLayout()
        vbox.addWidget(self.canvas)
        vbox.addWidget(self.mpl_toolbar)
        self.setLayout(vbox)
        # Active series
        self.series = []
        # Indicators
        self.skip = False
        self.legend = False
        self.legPos = (1.0, 1.0)
        self.legSiz = 14
        self.grid = False

    def plot(self, x, y, name=None):
        """Plot a new line and return it.

        Keyword arguments:
        x -- X values
        y -- Y values
        name -- Serie name (for legend)."""
        l = Line(self.axes, x, y, name)
        self.series.append(l)
        # Update window
        self.update()
        return l

    def update(self):
        """Update the plot, redrawing the canvas."""
        if not self.skip:
            self.skip = True
            if self.legend:
                legend(self.legend, self.legPos, self.legSiz)
            self.canvas.draw()
            self.skip = False

    def isGrid(self):
        """Return True if Grid is active, False otherwise."""
        return bool(self.grid)

    def isLegend(self):
        """Return True if Legend is active, False otherwise."""
        return bool(self.legend)

    def setActiveAxes(self, index):
        """Change the current active axes.

        Keyword arguments:
        index -- Index of the new active axes set.
        """
        self.axes = self.axesList[index]
        self.fig.sca(self.axes)
