import sys
from pivy.qt.QtWidgets import QApplication
from pivy.qt.QtGui import QColor
from pivy import quarter, coin, interactive, utils

class ConnectionMarker(interactive.Marker):
    def __init__(self, points):
        super(ConnectionMarker, self).__init__(points, True)


class ConnectionPolygon(interactive.Polygon):
    std_col = "green"
    def __init__(self, markers):
        super(ConnectionPolygon, self).__init__(
            sum([m.points for m in markers], []), True)
        self.markers = markers

        for m in self.markers:
            m.on_drag.append(self.update_polygon)

    def update_polygon(self):
        self.points = sum([m.points for m in self.markers], [])

    @property
    def drag_objects(self):
        return self.markers

    def check_dependency(self):
        if any([m._delete for m in self.markers]):
            self.delete()


class ConnectionLine(interactive.Line):
    def __init__(self, markers):
        super(ConnectionLine, self).__init__(
            sum([m.points for m in markers], []), True)
        self.markers = markers
        for m in self.markers:
            m.on_drag.append(self.update_line)

    def update_line(self):
        self.points = sum([m.points for m in self.markers], [])

    @property
    def drag_objects(self):
        return self.markers

    def check_dependency(self):
        if any([m._delete for m in self.markers]):
            self.delete()


def main():
    app = QApplication(sys.argv)
    utils.addMarkerFromSvg("test.svg", "CUSTOM_MARKER",  40)
    viewer = quarter.QuarterWidget()
    root = interactive.InteractionSeparator(viewer.sorendermanager)
    root.pick_radius = 40

    m1 = ConnectionMarker([[-1, -1, -1]])
    m2 = ConnectionMarker([[-1,  1, -1]])
    m3 = ConnectionMarker([[ 1,  1, -1]])
    m4 = ConnectionMarker([[ 1, -1, -1]])

    m5 = ConnectionMarker([[-1, -1,  1]])
    m6 = ConnectionMarker([[-1,  1,  1]])
    m7 = ConnectionMarker([[ 1,  1,  1]])
    m8 = ConnectionMarker([[ 1, -1,  1]])

    points = [m1, m2, m3, m4, m5, m6, m7, m8]

    l01 = ConnectionLine([m1, m2])
    l02 = ConnectionLine([m2, m3])
    l03 = ConnectionLine([m3, m4])
    l04 = ConnectionLine([m4, m1])

    l05 = ConnectionLine([m5, m6])
    l06 = ConnectionLine([m6, m7])
    l07 = ConnectionLine([m7, m8])
    l08 = ConnectionLine([m8, m5])

    l09 = ConnectionLine([m1, m5])
    l10 = ConnectionLine([m2, m6])
    l11 = ConnectionLine([m3, m7])
    l12 = ConnectionLine([m4, m8])

    lines = [l01, l02, l03, l04, l05, l06, l07, l08, l09, l10, l11, l12]

    p1 = ConnectionPolygon([m1, m2, m3, m4])
    p2 = ConnectionPolygon([m8, m7, m6, m5])
    p3 = ConnectionPolygon([m5, m6, m2, m1])
    p4 = ConnectionPolygon([m6, m7, m3, m2])
    p5 = ConnectionPolygon([m7, m8, m4, m3])
    p6 = ConnectionPolygon([m8, m5, m1, m4])

    polygons = [p1, p2, p3, p4, p5, p6]
    root += points + lines + polygons
    root.register()

    viewer.setSceneGraph(root)
    viewer.setBackgroundColor(QColor(255, 255, 255))
    viewer.setWindowTitle("minimal")
    viewer.show()
    sys.exit(app.exec_())

if __name__ == '__main__':
    main()
