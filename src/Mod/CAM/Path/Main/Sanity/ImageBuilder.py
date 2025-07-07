# ***************************************************************************
# *   Copyright (c) 2024 Ondsel <development@ondsel.com>                    *
# *                                                                         *
# *   This file is part of the FreeCAD CAx development system.              *
# *                                                                         *
# *   This program is free software; you can redistribute it and/or modify  *
# *   it under the terms of the GNU Lesser General Public License (LGPL)    *
# *   as published by the Free Software Foundation; either version 2 of     *
# *   the License, or (at your option) any later version.                   *
# *   for detail see the LICENCE text file.                                 *
# *                                                                         *
# *   FreeCAD is distributed in the hope that it will be useful,            *
# *   but WITHOUT ANY WARRANTY; without even the implied warranty of        *
# *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the         *
# *   GNU Lesser General Public License for more details.                   *
# *                                                                         *
# *   You should have received a copy of the GNU Library General Public     *
# *   License along with FreeCAD; if not, write to the Free Software        *
# *   Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  *
# *   USA                                                                   *
# *                                                                         *
# ***************************************************************************

from PySide import QtGui
import FreeCAD
import FreeCADGui
import Path.Log
import os
import time

if False:
    Path.Log.setLevel(Path.Log.Level.DEBUG, Path.Log.thisModule())
    Path.Log.trackModule(Path.Log.thisModule())
else:
    Path.Log.setLevel(Path.Log.Level.INFO, Path.Log.thisModule())


class ImageBuilder:
    def __init__(self, file_path):
        self.file_path = file_path

    def build_image(self, obj, image_name):
        raise NotImplementedError("Subclass must implement abstract method")

    def save_image(self, image):
        raise NotImplementedError("Subclass must implement abstract method")


class ImageBuilderFactory:
    @staticmethod
    def get_image_builder(file_path, **kwargs):

        # return DummyImageBuilder(file_path, **kwargs)
        if FreeCAD.GuiUp:
            return GuiImageBuilder(file_path, **kwargs)
        else:
            return DummyImageBuilder(file_path, **kwargs)
        # return NonGuiImageBuilder(file_path, **kwargs)


class DummyImageBuilder(ImageBuilder):
    def __init__(self, file_path):
        Path.Log.debug("Initializing dummyimagebuilder")
        super().__init__(file_path)

    def build_image(self, obj, imageName):
        return self.file_path


class GuiImageBuilder(ImageBuilder):
    """
    A class for generating images of 3D objects in a FreeCAD GUI environment.
    """

    def __init__(self, file_path):
        super().__init__(file_path)

        Path.Log.debug("Initializing GuiImageBuilder")
        self.file_path = file_path
        self.currentCamera = FreeCADGui.ActiveDocument.ActiveView.getCameraType()

        self.doc = FreeCADGui.ActiveDocument

    def __del__(self):
        Path.Log.debug("Destroying GuiImageBuilder")
        self.restore_visibility()

    def prepare_view(self, obj):
        # Create a new view
        Path.Log.debug("CAM - Preparing view\n")

        mw = FreeCADGui.getMainWindow()
        num_windows = len(mw.getWindows())

        # Create and configure the view
        view = FreeCADGui.ActiveDocument.createView("Gui::View3DInventor")
        view.setAnimationEnabled(False)
        view.viewIsometric()
        view.setCameraType("Perspective")

        # Resize the window
        mdi = mw.findChild(QtGui.QMdiArea)
        view_window = mdi.activeSubWindow()
        view_window.resize(500, 500)
        view_window.showMaximized()

        FreeCADGui.Selection.clearSelection()

        self.record_visibility()
        obj.Visibility = True

        # Return the index of the new window (= old number of windows)
        return num_windows

    def record_visibility(self):
        self.visible = [o for o in self.doc.Document.Objects if o.Visibility]
        for o in self.doc.Document.Objects:
            o.Visibility = False

    def destroy_view(self, idx):
        Path.Log.debug("CAM - destroying view\n")
        mw = FreeCADGui.getMainWindow()
        windows = mw.getWindows()
        mw.removeWindow(windows[idx])

    def restore_visibility(self):
        Path.Log.debug("CAM - Restoring visibility\n")
        for o in self.visible:
            o.Visibility = True

    def build_image(self, obj, image_name):
        Path.Log.debug("CAM - Building image\n")
        """
        Makes an image of the target object.  Returns filename.
        """

        file_path = os.path.join(self.file_path, image_name)

        idx = self.prepare_view(obj)

        self.capture_image(file_path)
        self.destroy_view(idx)

        result = f"{file_path}_t.png"

        Path.Log.debug(f"Saving image to: {file_path}")
        Path.Log.debug(f"Image saved to: {result}")
        return result

    def capture_image(self, file_path):

        FreeCADGui.updateGui()
        Path.Log.debug("CAM - capture image\n")
        a_view = FreeCADGui.activeDocument().activeView()
        a_view.saveImage(file_path + ".png", 500, 500, "Current")
        a_view.saveImage(file_path + "_t.png", 500, 500, "Transparent")
        a_view.setAnimationEnabled(True)


class NonGuiImageBuilder(ImageBuilder):
    def __init__(self, file_path):
        super().__init__(file_path)
        Path.Log.debug("nonguiimagebuilder")

    def build_image(self, obj, image_name):
        """
        Generates a headless picture of a 3D object and saves it as a PNG and optionally a PostScript file.

        Args:
        - obj: The 3D object to generate an image for.
        - image_name: Base name for the output image file without extension.

        Returns:
        - A boolean indicating the success of the operation.
        """
        # Ensure the 'Part' and 'coin' modules are available, along with necessary attributes/methods.
        if not hasattr(obj, "Shape") or not hasattr(obj.Shape, "writeInventor"):
            Path.Log.debug("Object does not have the required attributes.")
            return False

        try:
            # Generate Inventor data from the object's shape
            iv = obj.Shape.writeInventor()

            # Prepare Inventor data for rendering
            inp = coin.SoInput()
            inp.setBuffer(iv)
            data = coin.SoDB.readAll(inp)

            if data is None:
                Path.Log.debug("Failed to read Inventor data.")
                return False

            # Setup the scene
            base = coin.SoBaseColor()
            base.rgb.setValue(0.6, 0.7, 1.0)
            data.insertChild(base, 0)

            root = coin.SoSeparator()
            light = coin.SoDirectionalLight()
            cam = coin.SoOrthographicCamera()
            root.addChild(cam)
            root.addChild(light)
            root.addChild(data)

            # Camera and rendering setup
            axo = coin.SbRotation(-0.353553, -0.146447, -0.353553, -0.853553)
            viewport = coin.SbViewportRegion(400, 400)
            cam.orientation.setValue(axo)
            cam.viewAll(root, viewport)
            off = coin.SoOffscreenRenderer(viewport)
            root.ref()
            ret = off.render(root)
            root.unref()

            # Saving the rendered image
            if off.isWriteSupported("PNG"):
                file_path = f"{self.file_path}{os.path.sep}{imageName}.png"
                off.writeToFile(file_path, "PNG")
            else:
                Path.Log.debug("PNG format is not supported.")
                # return False

            # Optionally save as PostScript if supported
            file_path = f"{self.file_path}{os.path.sep}{imageName}.ps"
            off.writeToPostScript(ps_file_path)

            return file_path

        except Exception as e:
            print(f"An error occurred: {e}")
            return False
