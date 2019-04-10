
import FreeCAD
import Part
import sys
import os
import urllib as urlreader
import tempfile

class ImportTypes:
    STEP = "STEP"

class UNITS:
    MM = "mm"
    IN = "in"


def importShape(importType, fileName):
    """
    Imports a file based on the type (STEP, STL, etc)
    :param importType: The type of file that we're importing
    :param fileName: THe name of the file that we're importing
    """

    #Check to see what type of file we're working with
    if importType == ImportTypes.STEP:
        return importStep(fileName)


#Loads a STEP file into a CQ.Workplane object
def importStep(fileName):
    """
        Accepts a file name and loads the STEP file into a cadquery shape
        :param fileName: The path and name of the STEP file to be imported
    """
    from .shapes import Shape
    #Now read and return the shape
    try:
        rshape = Part.read(fileName)

        # Extract all solids and surfaces
        geometry = []
        for solid in rshape.Solids:
            geometry.append(Shape.cast(solid))

        for shell in rshape.Shells:
            geometry.append(Shape.cast(shell))

        from ..cq import Workplane
        return Workplane("XY").newObject(geometry)

    except:
        raise ValueError("STEP File Could not be loaded")

#Loads a STEP file from an URL into a CQ.Workplane object
def importStepFromURL(url):
    #Now read and return the shape
    try:
        # Account for differences in urllib between Python 2 and 3
        if hasattr(urlreader, "urlopen"):
            webFile = urlreader.urlopen(url)
        else:
            import urllib.request
            webFile = urllib.request.urlopen(url)

        tempFile = tempfile.NamedTemporaryFile(suffix='.step', delete=False)
        tempFile.write(webFile.read())
        webFile.close()
        tempFile.close()

        # Read saved file and return CQ Workplane object
        return importStep(tempFile.name)
    except:
        raise ValueError("STEP File from the URL: " + url + " Could not be loaded")
