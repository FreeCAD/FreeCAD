# ***************************************************************************
# *   Copyright (c) 2024 Peter McB
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
#  \brief create labels


import FreeCADGui
from pivy import coin

"""
Place Label on the screen with an optionl image.
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
        self.ispicture = False
        if image != "":
            self.isimage = True
            self.myImage = coin.SoImage()
            self.textSep.addChild(self.myImage)
            self.myImage.filename.setValue(image)
        self.activeDoc = FreeCADGui.ActiveDocument
        self.view = self.activeDoc.ActiveView
        self.viewer = self.view.getViewer()
        self.render = self.viewer.getSoRenderManager()
        self.sup = self.render.addSuperimposition(self.textSep)

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
        self.textSep.addChild(self.myImage)
        self.myImage.filename.setValue(image)

# remove the image
    def remove_image(self):
        if self.isimage:
            # self.myImage = coin.SoImage()
            self.textSep.removeChild(self.myImage)
            self.isimage = False
            self.sup = self.render.addSuperimposition(self.textSep)

# remove the label
    def remove(self):
        # remove the Superimposition layer with :
        self.render.removeSuperimposition(self.sup)

if __name__ == "__main__":
    t1 = createLabel((-0.98, 0.90, 0), "this a new text")
"""
Examples:
import CreateLabels as CL
t1 = CL.createLabel( (+0.5, -0.90, 0), "this is a image", image = "mesh.jpg")
t2 = CL.createLabel( (-0.7, -0.90, 0), "this is text")

t1.set_colour((0,1,0))
t1.remove_image()
t1.add_image("opera.jpg")
t1.set_text("hello world")
t1.set_position((+0.7, 0.50, 0))
t1.remove()
"""
