# SPDX-License-Identifier: LGPL-2.1-or-later
# SPDX-FileNotice: Part of the FreeCAD project.

# ***************************************************************************
# *   Copyright (c) 2024 Ondsel <development@ondsel.com>                    *
# *   Copyright (c) 2025 Billy Huddleston <billy@ivdc.com>                  *
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

from PySide import QtGui, QtCore
import FreeCAD
import FreeCADGui
import Path.Log
import os
import tempfile


if False:
    Path.Log.setLevel(Path.Log.Level.DEBUG, Path.Log.thisModule())
    Path.Log.trackModule(Path.Log.thisModule())
else:
    Path.Log.setLevel(Path.Log.Level.INFO, Path.Log.thisModule())


class ImageBuilder:
    def __init__(self, file_path):
        self.file_path = file_path

    def build_image(self, obj, image_name, as_bytes=False, view="default"):
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

    def build_image(self, obj, imageName, as_bytes=False, view="default"):
        if as_bytes:
            return b""
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

    def prepare_view(self, obj, view="default"):
        # Create a new view
        Path.Log.debug("CAM - Preparing view\n")

        mw = FreeCADGui.getMainWindow()
        num_windows = len(mw.getWindows())

        # Create and configure the view
        view_obj = FreeCADGui.ActiveDocument.createView("Gui::View3DInventor")
        view_obj.setAnimationEnabled(False)
        view_obj.setCameraType("Orthographic")
        if view == "headon":
            view_obj.viewFront()
        else:
            view_obj.viewIsometric()

        # Resize the window
        mdi = mw.findChild(QtGui.QMdiArea)
        view_window = mdi.activeSubWindow()
        view_window.resize(500, 500)
        view_window.showMaximized()

        # First make everything invisible
        self.record_visibility()

        # Then make only our target object visible and select it
        obj.Visibility = True
        FreeCADGui.Selection.clearSelection()
        FreeCADGui.Selection.addSelection(obj)
        FreeCADGui.Selection.clearSelection()  # Clear so the selection highlight does not appear in the image

        # Get the active view and fit to selection
        a_view = FreeCADGui.activeDocument().activeView()
        try:
            a_view.fitAll()  # First fit all to ensure the object is in view
            FreeCADGui.updateGui()
            a_view.fitSelection()  # Then try to fit to the selection
        except Exception:
            # If fitSelection fails, we already called fitAll
            pass

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

    def build_image(self, obj, image_name, as_bytes=False, view="default"):
        Path.Log.debug("CAM - Building image\n")
        """
        Makes an image of the target object. Returns either the image as bytes or a filename.
        """

        idx = self.prepare_view(obj, view=view)

        if as_bytes:
            # Capture directly to memory without writing to disk
            img_bytes = self.capture_image_to_bytes()
            self.destroy_view(idx)
            return img_bytes
        else:
            # Write to disk as before
            file_path = os.path.join(self.file_path, image_name)
            self.capture_image(file_path)
            self.destroy_view(idx)
            result = f"{file_path}_t.png"
            Path.Log.debug(f"Image saved to: {result}")
            return result

    def capture_image(self, file_path):
        FreeCADGui.updateGui()
        Path.Log.debug("CAM - capture image to file\n")
        a_view = FreeCADGui.activeDocument().activeView()
        # Generate higher resolution images - 800x800 pixels for better quality on high-DPI displays
        a_view.saveImage(file_path + ".png", 800, 800, "Current")
        a_view.saveImage(file_path + "_t.png", 800, 800, "Transparent")
        a_view.setAnimationEnabled(True)

    def capture_image_to_bytes(self):
        """Capture the current view directly to bytes without writing to disk"""
        FreeCADGui.updateGui()
        Path.Log.debug("CAM - capture image to bytes\n")
        a_view = FreeCADGui.activeDocument().activeView()

        try:
            # Use FreeCAD's built-in method for getting a QImage directly
            # This approach is based on the same method the viewport uses internally
            qimg = a_view.grabFramebuffer()

            # Convert QImage to bytes using QBuffer (purely in memory)
            buffer = QtCore.QBuffer()
            buffer.open(QtCore.QIODevice.WriteOnly)
            qimg.save(buffer, "PNG")
            img_bytes = buffer.data().data()
            buffer.close()

            a_view.setAnimationEnabled(True)
            return img_bytes

        except Exception as e:
            # Fallback to temporary file approach if the direct method fails
            Path.Log.debug(f"Direct image capture failed: {e}, using fallback method")

            with tempfile.NamedTemporaryFile(suffix=".png", delete=False) as temp:
                temp_path = temp.name

            # saveImage doesn't write to memory directly, so we need to use a temporary file
            # Generate higher resolution images - 800x800 pixels for better quality on high-DPI displays
            a_view.saveImage(temp_path, 800, 800, "Transparent")

            # Read the temporary file into memory
            with open(temp_path, "rb") as f:
                img_bytes = f.read()

            # Clean up the temporary file
            try:
                os.unlink(temp_path)
            except Exception:
                # Ignore errors during temporary file cleanup, as failure to delete is non-critical
                pass

            a_view.setAnimationEnabled(True)
            return img_bytes


class NonGuiImageBuilder(ImageBuilder):
    def __init__(self, file_path):
        super().__init__(file_path)
        Path.Log.debug("nonguiimagebuilder")

    def build_image(self, obj, image_name, as_bytes=False, view="default"):
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
            if as_bytes:
                qimg = off.getQImage()
                buffer = QtCore.QBuffer()
                buffer.open(QtCore.QIODevice.WriteOnly)
                qimg.save(buffer, "PNG")
                return buffer.data().data()
            else:
                if off.isWriteSupported("PNG"):
                    file_path = f"{self.file_path}{os.path.sep}{image_name}.png"
                    off.writeToFile(file_path, "PNG")
                    return file_path
                else:
                    Path.Log.debug("PNG format is not supported.")
                    return False

        except Exception as e:
            print(f"An error occurred: {e}")
            return False
