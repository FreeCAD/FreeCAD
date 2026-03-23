from pivy import coin


def add_marker_from_svg(file_path, marker_name, pixel_x=10, pixel_y=None,
                     isLSBFirst=False, isUpToDown=False):
    """adds a new marker bitmap from a vector graphic (svg)"""

    # get an icon from the svg rendered with the given pixel
    from pivy.qt import QtCore, QtGui
    pixel_y = pixel_y or pixel_x
    icon = QtGui.QIcon(file_path)
    icon = QtGui.QBitmap(icon.pixmap(pixel_x, pixel_y))

    # create a XMP-icon
    buffer=QtCore.QBuffer()
    buffer.open(buffer.WriteOnly)
    icon.save(buffer,"XPM")
    buffer.close()

    # get a string from the XMP-icon
    ary = str(buffer.buffer(), "utf8")
    ary = ary.split("\n", 1)[1]
    ary = ary.replace('\n', "").replace('"', "").replace(";", "")
    ary = ary.replace("}", "").replace("#", "x").replace(".", " ")
    string = str.join("", ary.split(",")[3:])
    
    # add the new marker style
    setattr(coin.SoMarkerSet, marker_name, coin.SoMarkerSet.getNumDefinedMarkers())
    coin.SoMarkerSet.addMarker(getattr(coin.SoMarkerSet, marker_name), 
                               coin.SbVec2s([pixel_x, pixel_y]), string,
                               isLSBFirst, isUpToDown)


def get_point_on_screen(render_manager, screen_pos, normal="camera", point=None):
    """get coordinates from pixel position"""
    
    pCam = render_manager.getCamera()
    vol = pCam.getViewVolume()

    point = point or coin.SbVec3f(0, 0, 0)

    if normal == "camera":
        plane = vol.getPlane(10)
        normal = plane.getNormal()
    elif normal == "x":
        normal = coin.SbVec3f(1, 0, 0)
    elif normal == "y":
        normal = coin.SbVec3f(0, 1, 0)
    elif normal == "z":
        normal = coin.SbVec3f(0, 0, 1)
    normal.normalize()
    x, y = screen_pos
    vp = render_manager.getViewportRegion()
    size = vp.getViewportSize()
    dX, dY = size

    fRatio = vp.getViewportAspectRatio()
    pX = float(x) / float(vp.getViewportSizePixels()[0])
    pY = float(y) / float(vp.getViewportSizePixels()[1])

    if (fRatio > 1.0):
        pX = (pX - 0.5 * dX) * fRatio + 0.5 * dX
    elif (fRatio < 1.0):
        pY = (pY - 0.5 * dY) / fRatio + 0.5 * dY

    plane = coin.SbPlane(normal, point)
    line = coin.SbLine(*vol.projectPointToLine(coin.SbVec2f(pX,pY)))
    pt = plane.intersect(line)
    return pt