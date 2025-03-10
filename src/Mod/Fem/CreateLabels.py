# ***************************************************************************
# *   Copyright (c) 2024 Peter McB                                          *
# *                                                                         *
# *   This file is part of the FreeCAD CAx development system.              *
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

__title__ = "FreeCAD result mechanical task panel"
__author__ = "PMcB"
__url__ = "https://www.freecad.org"

## @package view_result_mechanical
#  \ingroup FEM
#  \brief Create and Place Labels on the screen with an optional image.

import FreeCADGui
from pivy import coin

"""
Place Label on the screen with an optional image.
positions on screen:
         (-.9, .9, 0)    top left
         (.9, .9, 0)     upper right
         (-.9, -.9, 0)   lower left
         (.9, -.9, 0)    lower right
"""


class createLabel:
    def __init__(self, trans, text, image="") -> None:
        self.textSep = coin.SoSeparator()
        self.cam = coin.SoOrthographicCamera()
        self.cam.aspectRatio = 1
        self.cam.viewportMapping = coin.SoCamera.LEAVE_ALONE

        self.trans = coin.SoTranslation()
        self.trans.translation = trans
        self.translation = trans
        self.color = coin.SoBaseColor()
        self.color.rgb = (0, 0, 0)

        self.myFont = coin.SoFont()
        self.myFont.name = "Arial,FreeSans,sans"
        #       self.myFont.name = "FreeMono,FreeSans,sans"
        self.size = 20  # 24
        self.myFont.size.setValue(self.size)

        self.SoText2 = coin.SoText2()
        self.SoText2.string = text

        self.textSep.addChild(self.cam)
        self.textSep.addChild(self.trans)
        self.textSep.addChild(self.color)
        self.textSep.addChild(self.myFont)
        self.textSep.addChild(self.SoText2)
        self.activeDoc = FreeCADGui.ActiveDocument
        self.view = self.activeDoc.ActiveView
        self.viewer = self.view.getViewer()
        self.render = self.viewer.getSoRenderManager()
        self.isimage = False
        if image != "":
            self.add_image(image)
            self.isimage = True
        self.sup = self.render.addSuperimposition(self.textSep)
        self.displayed = True

    # get position of label
    def get_position(self):
        return self.translation

    # set position of label
    def set_position(self, trans):
        self.trans.translation = trans
        self.translation = trans

    # set font
    def set_font(self, font):
        self.myFont.name = font

    # set font size
    def set_font_size(self, font_size):
        self.myFont.size.setValue(font_size)

    # set text of label
    def set_text(self, text):
        self.SoText2.string = text

    # set colour of label
    def set_colour(self, colour):
        self.color.rgb = colour
        self.textSep.addChild(self.color)

    # add or change the image with the label
    def add_image(self, image):
        self.remove_image()
        self.isimage = True
        self.myImage = coin.SoImage()
        self.myImage.filename.setValue(image)
        self.textSep.addChild(self.myImage)

    # remove the image
    def remove_image(self):
        if self.isimage:
            # self.myImage = coin.SoImage()
            self.textSep.removeChild(self.myImage)
            self.isimage = False

    # display the label
    def display(self):
        if not self.displayed:
            # display the Superimposition layer with :
            self.sup = self.render.addSuperimposition(self.textSep)
            self.displayed = True

    # hide the label
    def hide(self):
        # hide the Superimposition layer with :
        if self.displayed:
            self.render.removeSuperimposition(self.sup)
            self.displayed = False

    # help
    def help(self):
        print(
            """
Place Label on the screen with an optional image.
Create an instance:
import CreateLabels as CL
label =  CL.creatLabel(<position of label>, <title of label>, <optional image>)
e.g.:
label1 = CL.createLabel( (-0.7, -0.90, 0), "this is text")
label2 = CL.createLabel( (+0.5, -0.90, 0), "this is a image", image = "mesh.jpg")
Positions on screen:
        (-.9, .9, 0)    top left
        (.9, .9, 0)     upper right
        (-.9, -.9, 0)   lower left
        (.9, -.9, 0)    lower right

The following function are available:
        label1.set_position((0.7, 0.50, 0))
        pos= label1.get_position()
        label1.set_font("FreeMono,FreeSans,sans")
        label1.set_font_size(20)
        label1.set_text("hello world")
		label1.set_colour((0,1,0)) # colour of text
        label1.add_image("opera.jpg")
        label1.remove_image()
        label1.hide()  - hide the label
        label1.display()  - display the label, after hide
"""
        )


if __name__ == "__main__":
    label1 = createLabel((-0.98, 0.90, 0), "this a new text")
"""
Examples:
import CreateLabels as CL
label1 = CL.createLabel( (+0.5, -0.90, 0), "this is a image", image = "mesh.jpg")
label2 = CL.createLabel( (-0.7, -0.90, 0), "this is text")

label1.set_colour((0,1,0))
label1.remove_image()
label1.add_image("opera.jpg")
label1.set_text("hello world")
label1.set_position((+0.7, 0.50, 0))
label1.set_font("FreeMono,FreeSans,sans")
label1.hide()
"""
