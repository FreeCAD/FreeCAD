
#***************************************************************************
#*                                                                         *
#*   Copyright (c) 2009 Yorik van Havre <yorik@uncreated.net>              * 
#*                                                                         *
#*   This program is free software; you can redistribute it and/or modify  *
#*   it under the terms of the GNU Lesser General Public License (LGPL)    *
#*   as published by the Free Software Foundation; either version 2 of     *
#*   the License, or (at your option) any later version.                   *
#*   for detail see the LICENCE text file.                                 *
#*                                                                         *
#*   This program is distributed in the hope that it will be useful,       *
#*   but WITHOUT ANY WARRANTY; without even the implied warranty of        *
#*   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the         *
#*   GNU Library General Public License for more details.                  *
#*                                                                         *
#*   You should have received a copy of the GNU Library General Public     *
#*   License along with this program; if not, write to the Free Software   *
#*   Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  *
#*   USA                                                                   *
#*                                                                         *
#***************************************************************************

__title__="FreeCAD Draft Workbench - SVG importer/exporter"
__author__ = "Yorik van Havre, Sebastian Hoogen"
__url__ = ["http://www.freecadweb.org"]

## @package importSVG
#  \ingroup DRAFT
#  \brief SVG file importer & exporter
#
#  This module provides support for importing and exporting SVG files. It
#  enables importing/exporting objects directly to/from the 3D document, but
#  doesn't handle the SVG output from the Drawng and TechDraw modules.

'''
This script imports SVG files in FreeCAD. Currently only reads the following entities:
paths, lines, circular arcs ,rects, circles, ellipses, polygons, polylines.
currently unsupported: use, image
'''
#ToDo:
# ignoring CDATA
# handle image element (external references and inline base64)
# debug Problem with 'Sans' font from Inkscape
# debug Problem with fill color
# implement inherting fill style from group
# handle relative units

import xml.sax, string, FreeCAD, os, math, re, Draft, DraftVecUtils
from FreeCAD import Vector

if FreeCAD.GuiUp:
    from DraftTools import translate
    from PySide import QtCore, QtGui
else:
    def translate(ctxt,txt):
        return txt

try: import FreeCADGui
except ImportError: gui = False
else: gui = True

try: draftui = FreeCADGui.draftToolBar
except AttributeError: draftui = None

if open.__module__ in ['__builtin__','io']:
  pythonopen = open

svgcolors = {
          'Pink': (255, 192, 203),
          'Blue': (0, 0, 255),
          'Honeydew': (240, 255, 240),
          'Purple': (128, 0, 128),
          'Fuchsia': (255, 0, 255),
          'LawnGreen': (124, 252, 0),
          'Amethyst': (153, 102, 204),
          'Crimson': (220, 20, 60),
          'White': (255, 255, 255),
          'NavajoWhite': (255, 222, 173),
          'Cornsilk': (255, 248, 220),
          'Bisque': (255, 228, 196),
          'PaleGreen': (152, 251, 152),
          'Brown': (165, 42, 42),
          'DarkTurquoise': (0, 206, 209),
          'DarkGreen': (0, 100, 0),
          'MediumOrchid': (186, 85, 211),
          'Chocolate': (210, 105, 30),
          'PapayaWhip': (255, 239, 213),
          'Olive': (128, 128, 0),
          'Silver': (192, 192, 192),
          'PeachPuff': (255, 218, 185),
          'Plum': (221, 160, 221),
          'DarkGoldenrod': (184, 134, 11),
          'SlateGrey': (112, 128, 144),
          'MintCream': (245, 255, 250),
          'CornflowerBlue': (100, 149, 237),
          'Gold': (255, 215, 0),
          'HotPink': (255, 105, 180),
          'DarkBlue': (0, 0, 139),
          'LimeGreen': (50, 205, 50),
          'DeepSkyBlue': (0, 191, 255),
          'DarkKhaki': (189, 183, 107),
          'LightGrey': (211, 211, 211),
          'Yellow': (255, 255, 0),
          'Gainsboro': (220, 220, 220),
          'MistyRose': (255, 228, 225),
          'SandyBrown': (244, 164, 96),
          'DeepPink': (255, 20, 147),
          'Magenta': (255, 0, 255),
          'AliceBlue': (240, 248, 255),
          'DarkCyan': (0, 139, 139),
          'DarkSlateGrey': (47, 79, 79),
          'GreenYellow': (173, 255, 47),
          'DarkOrchid': (153, 50, 204),
          'OliveDrab': (107, 142, 35),
          'Chartreuse': (127, 255, 0),
          'Peru': (205, 133, 63),
          'Orange': (255, 165, 0),
          'Red': (255, 0, 0),
          'Wheat': (245, 222, 179),
          'LightCyan': (224, 255, 255),
          'LightSeaGreen': (32, 178, 170),
          'BlueViolet': (138, 43, 226),
          'LightSlateGrey': (119, 136, 153),
          'Cyan': (0, 255, 255),
          'MediumPurple': (147, 112, 219),
          'MidnightBlue': (25, 25, 112),
          'FireBrick': (178, 34, 34),
          'PaleTurquoise': (175, 238, 238),
          'PaleGoldenrod': (238, 232, 170),
          'Gray': (128, 128, 128),
          'MediumSeaGreen': (60, 179, 113),
          'Moccasin': (255, 228, 181),
          'Ivory': (255, 255, 240),
          'DarkSlateBlue': (72, 61, 139),
          'Beige': (245, 245, 220),
          'Green': (0, 128, 0),
          'SlateBlue': (106, 90, 205),
          'Teal': (0, 128, 128),
          'Azure': (240, 255, 255),
          'LightSteelBlue': (176, 196, 222),
          'DimGrey': (105, 105, 105),
          'Tan': (210, 180, 140),
          'AntiqueWhite': (250, 235, 215),
          'SkyBlue': (135, 206, 235),
          'GhostWhite': (248, 248, 255),
          'MediumTurquoise': (72, 209, 204),
          'FloralWhite': (255, 250, 240),
          'LavenderBlush': (255, 240, 245),
          'SeaGreen': (46, 139, 87),
          'Lavender': (230, 230, 250),
          'BlanchedAlmond': (255, 235, 205),
          'DarkOliveGreen': (85, 107, 47),
          'DarkSeaGreen': (143, 188, 143),
          'SpringGreen': (0, 255, 127),
          'Navy': (0, 0, 128),
          'Orchid': (218, 112, 214),
          'SaddleBrown': (139, 69, 19),
          'IndianRed': (205, 92, 92),
          'Snow': (255, 250, 250),
          'SteelBlue': (70, 130, 180),
          'MediumSlateBlue': (123, 104, 238),
          'Black': (0, 0, 0),
          'LightBlue': (173, 216, 230),
          'Turquoise': (64, 224, 208),
          'MediumVioletRed': (199, 21, 133),
          'DarkViolet': (148, 0, 211),
          'DarkGray': (169, 169, 169),
          'Salmon': (250, 128, 114),
          'DarkMagenta': (139, 0, 139),
          'Tomato': (255, 99, 71),
          'WhiteSmoke': (245, 245, 245),
          'Goldenrod': (218, 165, 32),
          'MediumSpringGreen': (0, 250, 154),
          'DodgerBlue': (30, 144, 255),
          'Aqua': (0, 255, 255),
          'ForestGreen': (34, 139, 34),
          'LemonChiffon': (255, 250, 205),
          'LightSlateGray': (119, 136, 153),
          'SlateGray': (112, 128, 144),
          'LightGray': (211, 211, 211),
          'Indigo': (75, 0, 130),
          'CadetBlue': (95, 158, 160),
          'LightYellow': (255, 255, 224),
          'DarkOrange': (255, 140, 0),
          'PowderBlue': (176, 224, 230),
          'RoyalBlue': (65, 105, 225),
          'Sienna': (160, 82, 45),
          'Thistle': (216, 191, 216),
          'Lime': (0, 255, 0),
          'Seashell': (255, 245, 238),
          'DarkRed': (139, 0, 0),
          'LightSkyBlue': (135, 206, 250),
          'YellowGreen': (154, 205, 50),
          'Aquamarine': (127, 255, 212),
          'LightCoral': (240, 128, 128),
          'DarkSlateGray': (47, 79, 79),
          'Khaki': (240, 230, 140),
          'DarkGrey': (169, 169, 169),
          'BurlyWood': (222, 184, 135),
          'LightGoldenrodYellow': (250, 250, 210),
          'MediumBlue': (0, 0, 205),
          'DarkSalmon': (233, 150, 122),
          'RosyBrown': (188, 143, 143),
          'LightSalmon': (255, 160, 122),
          'PaleVioletRed': (219, 112, 147),
          'Coral': (255, 127, 80),
          'Violet': (238, 130, 238),
          'Grey': (128, 128, 128),
          'LightGreen': (144, 238, 144),
          'Linen': (250, 240, 230),
          'OrangeRed': (255, 69, 0),
          'DimGray': (105, 105, 105),
          'Maroon': (128, 0, 0),
          'LightPink': (255, 182, 193),
          'MediumAquamarine': (102, 205, 170),
          'OldLace': (253, 245, 230)
          }
svgcolorslower = dict((key.lower(),value) for key,value in \
    list(svgcolors.items()))

def getcolor(color):
    "checks if the given string is a RGB value, or if it is a named color. returns 1-based RGBA tuple."
    if (color[0] == "#"):
        if len(color) == 7:
            r = float(int(color[1:3],16)/255.0)
            g = float(int(color[3:5],16)/255.0)
            b = float(int(color[5:],16)/255.0)
        elif len(color) == 4: #expand the hex digits
            r = float(int(color[1],16)*17/255.0)
            g = float(int(color[2],16)*17/255.0)
            b = float(int(color[3],16)*17/255.0)
        return (r,g,b,0.0)
    elif color.lower().startswith('rgb('):
        cvalues=color[3:].lstrip('(').rstrip(')').replace('%','').split(',')
        if '%' in color:
            r,g,b = [int(float(cv))/100.0 for cv in cvalues]
        else:
            r,g,b = [int(float(cv))/255.0 for cv in cvalues]
        return (r,g,b,0.0)
    else:
        v=svgcolorslower.get(color.lower())
        if v:
            r,g,b = [float(vf)/255.0 for vf in v]
            return (r,g,b,0.0)
        #for k,v in svgcolors.items():
        #    if (k.lower() == color.lower()): pass

def transformCopyShape(shape,m):
    """apply transformation matrix m on given shape
since OCCT 6.8.0 transformShape can be used to apply certain non-orthogonal
transformations on shapes. This way a conversion to BSplines in
transformGeometry can be avoided."""
    if abs(m.A11**2+m.A12**2 -m.A21**2-m.A22**2) < 1e-8 and \
            abs(m.A11*m.A21+m.A12*m.A22) < 1e-8: #no shear
        try:
            newshape=shape.copy()
            newshape.transformShape(m)
            return newshape
        except Part.OCCError: # older versions of OCCT will refuse to work on
            pass              # non-orthogonal matrices
    return shape.transformGeometry(m)

def getsize(length,mode='discard',base=1):
        """parses length values containing number and unit
        with mode 'discard': extracts a number from the given string (removes unit suffixes)
        with mode 'tuple': return number and unit as a tuple
        with mode 'css90.0': convert the unit to px assuming 90dpi
        with mode 'css96.0': convert the unit to px assuming 96dpi
        with mode 'mm90.0': convert the unit to millimeter assuming 90dpi
        with mode 'mm96.0': convert the unit to millimeter assuming 96dpi"""
        if mode == 'mm90.0':
            tomm={
                    '' : 25.4/90, #default
                    'px' : 25.4/90,
                    'pt' : 1.25*25.4/90,
                    'pc' : 15*25.4/90,
                    'mm' : 1.0,
                    'cm' : 10.0,
                    'in' : 25.4,
                    'em':  15*2.54/90, #arbitrarily chosen; has to depend on font size
                    'ex': 10*2.54/90, #arbitrarily chosen; has to depend on font size

                    '%': 100 #arbitrarily chosen; has to depend on viewport size or (for filling patterns) on bounding box
                    }
        if mode == 'mm96.0':
            tomm={
                    '' : 25.4/96, #default
                    'px' : 25.4/96,
                    'pt' : 1.25*25.4/96,
                    'pc' : 15*25.4/96,
                    'mm' : 1.0,
                    'cm' : 10.0,
                    'in' : 25.4,
                    'em':  15*2.54/96, #arbitrarily chosen; has to depend on font size
                    'ex': 10*2.54/96, #arbitrarily chosen; has to depend on font size

                    '%': 100 #arbitrarily chosen; has to depend on viewport size or (for filling patterns) on bounding box
                    }
        if mode == 'css90.0':
            topx={
                    '' : 1.0, #default
                    'px' : 1.0,
                    'pt' : 1.25,
                    'pc' : 15,
                    'mm' : 90.0/25.4,
                    'cm' : 90.0/254.0,
                    'in' : 90,
                    'em':  15, #arbitrarily chosen; has to depend on font size
                    'ex':  10, #arbitrarily chosen; has to depend on font size

                    '%': 100 #arbitrarily chosen; has to depend on viewport size or (for filling patterns) on bounding box
                    }
        if mode == 'css96.0':
            topx={
                    '' : 1.0, #default
                    'px' : 1.0,
                    'pt' : 1.25,
                    'pc' : 15,
                    'mm' : 96.0/25.4,
                    'cm' : 96.0/254.0,
                    'in' : 96,
                    'em':  15, #arbitrarily chosen; has to depend on font size
                    'ex':  10, #arbitrarily chosen; has to depend on font size

                    '%': 100 #arbitrarily chosen; has to depend on viewport size or (for filling patterns) on bounding box
                    }
        number, exponent, unit=re.findall('([-+]?[0-9]*\.?[0-9]+([eE][-+]?[0-9]+)?)(px|pt|pc|mm|cm|in|em|ex|%)?',length)[0]
        if mode =='discard':
                return float(number)
        elif mode == 'tuple':
                return float(number),unit
        elif mode == 'isabsolute':
                return unit in ('mm','cm','in','px','pt')
        elif mode == 'mm96.0' or mode == 'mm90.0':
                return float(number)*tomm[unit]
        elif mode == 'css96.0' or mode == 'css90.0':
                if unit != '%':
                        return float(number)*topx[unit]
                else:
                        return float(number)*base

def makewire(path,checkclosed=False,donttry=False):
        '''try to make a wire out of the list of edges. If the 'Wire' functions fails or the wire is not
        closed if required the 'connectEdgesToWires' function is used'''
        if not donttry:
                try:
                        import Part
                        sh = Part.Wire(Part.__sortEdges__(path))
                        #sh = Part.Wire(path)
                        isok = (not checkclosed) or sh.isClosed()
                        if len(sh.Edges) != len(path):
                            isok = False
                except Part.OCCError:# BRep_API:command not done
                        isok = False
        if donttry or not isok:
                        #Code from wmayer forum p15549 to fix the tolerance problem
                        #original tolerance = 0.00001
                        comp=Part.Compound(path)
                        sh = comp.connectEdgesToWires(False,10**(-1*(Draft.precision()-2))).Wires[0]
                        if len(sh.Edges) != len(path):
                            FreeCAD.Console.PrintWarning("Unable to form a wire\n")
                            sh = comp
        return sh

def arccenter2end(center,rx,ry,angle1,angledelta,xrotation=0.0):
        '''calculate start and end vector and flags of an arc given in center parametrization
        see http://www.w3.org/TR/SVG/implnote.html#ArcImplementationNotes
        returns (v1,v2,largerc,sweep)'''
        vr1=Vector(rx*math.cos(angle1),ry*math.sin(angle1),0)
        vr2=Vector(rx*math.cos(angle1+angledelta),ry*math.sin(angle1+angledelta),0)
        mxrot=FreeCAD.Matrix()
        mxrot.rotateZ(xrotation)
        v1 = mxrot.multiply(vr1).add(center)
        v2 = mxrot.multiply(vr2).add(center)
        fa = ((abs(angledelta) / math.pi) % 2) > 1 # <180deg
        fs = angledelta < 0
        return v1,v2,fa,fs

def arcend2center(lastvec,currentvec,rx,ry,xrotation=0.0,correction=False):
        '''calculate (positive and negative) possible centers for an arc in endpoint parameterization
        see http://www.w3.org/TR/SVG/implnote.html#ArcImplementationNotes
        rotation or x-axis has to be specified in radians (CCW)
        the sweepflag is interpreted as: sweepflag <==>  arc is travelled clockwise 
        returns [(vcenter+,angle1+,angledelta+),(...-)]'''
        #scalefacsign = 1 if (largeflag != sweepflag) else -1
        rx = float(rx)
        ry = float(ry)
        v0 = lastvec.sub(currentvec)
        v0.multiply(0.5)
        m1=FreeCAD.Matrix()
        m1.rotateZ(-xrotation) #Formular 6.5.1
        v1=m1.multiply(v0)
        if correction:
                eparam = v1.x**2 / rx**2 + v1.y**2 / ry**2
                if eparam > 1:
                        eproot = math.sqrt(eparam)
                        rx = eproot * rx
                        ry = eproot * ry
        denom = rx**2 * v1.y**2+ ry**2 * v1.x**2
        numer = rx**2 * ry**2 -denom
        results=[]
        if abs(numer/denom) < 10**(-1*(Draft.precision())):
                scalefacpos = 0
        else:
                try:
                        scalefacpos = math.sqrt(numer/denom)
                except ValueError:
                        FreeCAD.Console.PrintMessage('sqrt(%f/%f)\n' % (numer,denom))
                        scalefacpos = 0
        for scalefacsign in (1,-1):
            scalefac = scalefacpos * scalefacsign
            vcx1 = Vector(v1.y*rx/ry,-v1.x*ry/rx,0).multiply(scalefac)  # Step2 F.6.5.2
            m2=FreeCAD.Matrix()
            m2.rotateZ(xrotation)
            centeroff = currentvec.add(lastvec)
            centeroff.multiply(.5)
            vcenter = m2.multiply(vcx1).add(centeroff) # Step3 F.6.5.3
            #angle1 = Vector(1,0,0).getAngle(Vector((v1.x-vcx1.x)/rx,(v1.y-vcx1.y)/ry,0)) # F.6.5.5
            #angledelta = Vector((v1.x-vcx1.x)/rx,(v1.y-vcx1.y)/ry,0).getAngle(Vector((-v1.x-vcx1.x)/rx,(-v1.y-vcx1.y)/ry,0)) # F.6.5.6
            #we need the right sign for the angle 
            angle1 = DraftVecUtils.angle(Vector(1,0,0),Vector((v1.x-vcx1.x)/rx,(v1.y-vcx1.y)/ry,0)) # F.6.5.5
            angledelta = DraftVecUtils.angle(Vector((v1.x-vcx1.x)/rx,(v1.y-vcx1.y)/ry,0),Vector((-v1.x-vcx1.x)/rx,(-v1.y-vcx1.y)/ry,0)) # F.6.5.6
            results.append((vcenter,angle1,angledelta))
        return results,(rx,ry)


def getrgb(color):
        "returns a rgb value #000000 from a freecad color"
        r = str(hex(int(color[0]*255)))[2:].zfill(2)
        g = str(hex(int(color[1]*255)))[2:].zfill(2)
        b = str(hex(int(color[2]*255)))[2:].zfill(2)
        return "#"+r+g+b

class svgHandler(xml.sax.ContentHandler):
        "this handler parses the svg files and creates freecad objects"

        def __init__(self):
                "retrieving Draft parameters"
                params = FreeCAD.ParamGet("User parameter:BaseApp/Preferences/Mod/Draft")
                self.style = params.GetInt("svgstyle")
                self.disableUnitScaling = params.GetBool("svgDisableUnitScaling",False)
                self.count = 0
                self.transform = None
                self.grouptransform = []
                self.lastdim = None
                self.viewbox = None
                self.symbols = {}
                self.currentsymbol = None
                self.svgdpi = 1.0

                global Part
                import Part
        
                if gui and draftui:
                        r = float(draftui.color.red()/255.0)
                        g = float(draftui.color.green()/255.0)
                        b = float(draftui.color.blue()/255.0)
                        self.lw = float(draftui.linewidth)
                else:
                        self.lw = float(params.GetInt("linewidth"))
                        c = params.GetUnsigned("color")
                        r = float(((c>>24)&0xFF)/255)
                        g = float(((c>>16)&0xFF)/255)
                        b = float(((c>>8)&0xFF)/255)
                self.col = (r,g,b,0.0)

        def format(self,obj):
                "applies styles to passed object"
                if gui:
                        v = obj.ViewObject
                        if self.color: v.LineColor = self.color
                        if self.width: v.LineWidth = self.width
                        if self.fill: v.ShapeColor = self.fill
        
        def startElement(self, name, attrs):

                # reorganizing data into a nice clean dictionary

                self.count += 1

                FreeCAD.Console.PrintMessage('processing element %d: %s\n'%(self.count,name))
                FreeCAD.Console.PrintMessage('existing group transform: %s\n'%(str(self.grouptransform)))
                
                data = {}
                for (keyword,content) in list(attrs.items()):
                        #print keyword,content
                        if keyword != "style":
                            content = content.replace(',',' ')
                            content = content.split()
                        #print keyword,content
                        data[keyword]=content

                if self.count == 1 and name == 'svg':
                    if 'inkscape:version' in data:
                        InksDocName = attrs.getValue('sodipodi:docname')
                        InksFullver = attrs.getValue('inkscape:version')[:4]
                        InksFullverlst = InksFullver.split('.')
                        if (
                            int(InksFullverlst[0]) == 0 and
                            int(InksFullverlst[1]) > 91
                            ):
                            self.svgdpi = 96.0
                        if (
                            int(InksFullverlst[0]) == 0 and
                            int(InksFullverlst[1]) < 92
                            ):
                            self.svgdpi = 90.0
                        if (
                            int(InksFullverlst[0]) > 0
                            ):
                            self.svgdpi = 96.0
                    if not 'inkscape:version' in data:
                        msgBox = QtGui.QMessageBox()
                        msgBox.setText(translate("ImportSVG","This SVG file does not appear to have been produced by Inkscape. If it does not contain absolute units then a DPI setting will be used."))
                        msgBox.setInformativeText(translate("ImportSVG","Do you wish to use 96dpi? Choosing 'No' will revert to the older standard 90dpi"))
                        msgBox.setStandardButtons(QtGui.QMessageBox.Yes | QtGui.QMessageBox.No)
                        msgBox.setDefaultButton(QtGui.QMessageBox.No)
                        ret = msgBox.exec_()
                        if ret == QtGui.QMessageBox.Yes:
                            self.svgdpi = 96.0
                        else:
                            self.svgdpi = 90.0
                        if ret:
                            FreeCAD.Console.PrintMessage("****** User specified "+str(self.svgdpi)+"dpi ******\n")
                    if self.svgdpi == 1.0:
                        FreeCAD.Console.PrintWarning("This SVG file ("+InksDocName+") has an unrecognised format which means the dpi could not be determined; therefore importing with 96dpi\n")
                        self.svgdpi = 96.0
                if 'style' in data:
                        if not data['style']:
                                pass#empty style attribute stops inhertig from parent
                        else:
                                content = data['style'].replace(' ','')
                                content = content.split(';')
                                for i in content:
                                        pair = i.split(':')
                                        if len(pair)>1: data[pair[0]]=pair[1]

                for k in ['x','y','x1','y1','x2','y2','r','rx','ry','cx','cy','width','height']:
                        if k in data:
                                data[k] = getsize(data[k][0],'css'+str(self.svgdpi))

                for k in ['fill','stroke','stroke-width','font-size']:
                        if k in data:
                                if isinstance(data[k],list):
                                    if data[k][0].lower().startswith("rgb("):
                                        data[k] = ",".join(data[k])
                                    else:
                                        data[k]=data[k][0]

                # extracting style info

                self.fill = None
                self.color = None
                self.width = None
                self.text = None

                if name == 'svg':
                        m=FreeCAD.Matrix()
                        if not self.disableUnitScaling:
                            if 'width' in data and 'height' in data and \
                                'viewBox' in data:
                                    vbw=float(data['viewBox'][2])
                                    vbh=float(data['viewBox'][3])
                                    w=attrs.getValue('width')
                                    h=attrs.getValue('height')
                                    self.viewbox=(vbw,vbh)
                                    if len(self.grouptransform)==0:
                                        unitmode='mm'+str(self.svgdpi)
                                    else: #nested svg element
                                        unitmode='css'+str(self.svgdpi)
                                    abw = getsize(w,unitmode)
                                    abh = getsize(h,unitmode)
                                    sx=abw/vbw
                                    sy=abh/vbh
                                    preservearstr=' '.join(data.get('preserveAspectRatio',[])).lower()
                                    uniformscaling = round(sx/sy,5) == 1
                                    if uniformscaling:
                                        m.scale(Vector(sx,sy,1))
                                    else:
                                        FreeCAD.Console.PrintWarning('Scaling Factors do not match!!!\n')
                                        if preservearstr.startswith('none'):
                                            m.scale(Vector(sx,sy,1))
                                        else: #preserve the aspect ratio
                                            if preservearstr.endswith('slice'):
                                                sxy=max(sx,sy)
                                            else:
                                                sxy=min(sx,sy)
                                            m.scale(Vector(sxy,sxy,1))
                            elif len(self.grouptransform)==0:
                                #fallback to current dpi
                                m.scale(Vector(25.4/self.svgdpi,25.4/self.svgdpi,1))
                        self.grouptransform.append(m) 
                if 'fill' in data:
                        if data['fill'][0] != 'none':
                                self.fill = getcolor(data['fill'])
                if 'stroke' in data:
                        if data['stroke'][0] != 'none':
                                self.color = getcolor(data['stroke'])
                if 'stroke-width' in data:
                        if data['stroke-width'] != 'none':
                                self.width = getsize(data['stroke-width'],'css'+str(self.svgdpi))
                if 'transform' in data:
                        m = self.getMatrix(attrs.getValue('transform'))
                        if name == "g":
                                self.grouptransform.append(m)
                        else:
                                self.transform = m
                else:
                        if name == "g":
                                self.grouptransform.append(FreeCAD.Matrix())

                if (self.style == 1):
                        self.color = self.col
                        self.width = self.lw

                pathname = None
                if 'id' in data:
                        pathname = data['id'][0]
                        FreeCAD.Console.PrintMessage('name: %s\n'%pathname)
                        
                # processing paths
                        
                if name == "path":
                        FreeCAD.Console.PrintMessage('data: %s\n'%str(data))
                        
                        if not pathname: pathname = 'Path'

                        path = []
                        point = []
                        lastvec = Vector(0,0,0)
                        lastpole = None
                        command = None
                        relative = False
                        firstvec = None

                        if "freecad:basepoint1" in data:
                                p1 = data["freecad:basepoint1"]
                                p1 = Vector(float(p1[0]),-float(p1[1]),0)
                                p2 = data["freecad:basepoint2"]
                                p2 = Vector(float(p2[0]),-float(p2[1]),0)
                                p3 = data["freecad:dimpoint"]
                                p3 = Vector(float(p3[0]),-float(p3[1]),0)
                                obj = Draft.makeDimension(p1,p2,p3)
                                self.applyTrans(obj)
                                self.format(obj)
                                self.lastdim = obj
                                data['d']=[]
                        pathcommandsre=re.compile('\s*?([mMlLhHvVaAcCqQsStTzZ])\s*?([^mMlLhHvVaAcCqQsStTzZ]*)\s*?',re.DOTALL)
                        pointsre=re.compile('([-+]?[0-9]*\.?[0-9]+([eE][-+]?[0-9]+)?)',re.DOTALL)
                        for d,pointsstr in pathcommandsre.findall(' '.join(data['d'])):
                                relative = d.islower()
                                pointlist = [float(number) for number,exponent in pointsre.findall(pointsstr.replace(',',' '))]

                                if (d == "M" or d == "m"):
                                        x = pointlist.pop(0)
                                        y = pointlist.pop(0)
                                        if path:
                                                #sh = Part.Wire(path)
                                                sh = makewire(path)
                                                if self.fill and sh.isClosed():
                                                    sh = Part.Face(sh)
                                                sh = self.applyTrans(sh)
                                                obj = self.doc.addObject("Part::Feature",pathname)
                                                obj.Shape = sh
                                                self.format(obj)
                                                if self.currentsymbol:
                                                    self.symbols[self.currentsymbol].append(obj)
                                                path = []
                                                #if firstvec:
                                                #        lastvec = firstvec #Move relative to last move command not last draw command
                                        if relative:
                                                lastvec = lastvec.add(Vector(x,-y,0))
                                        else:
                                                lastvec = Vector(x,-y,0)
                                        firstvec = lastvec
                                        FreeCAD.Console.PrintMessage('move %s\n'%str(lastvec))
                                        lastpole = None
                                if (d == "L" or d == "l") or \
                                        ((d == 'm' or d == 'M') and pointlist) :
                                        for x,y in zip(pointlist[0::2],pointlist[1::2]):
                                                if relative:
                                                        currentvec = lastvec.add(Vector(x,-y,0))
                                                else:
                                                        currentvec = Vector(x,-y,0)
                                                if not DraftVecUtils.equals(lastvec,currentvec):
                                                        seg = Part.LineSegment(lastvec,currentvec).toShape()
                                                        FreeCAD.Console.PrintMessage("line %s %s\n" %(lastvec,currentvec))
                                                        lastvec = currentvec
                                                        path.append(seg)
                                                lastpole = None
                                elif (d == "H" or d == "h"):
                                        for x in pointlist:
                                                if relative:
                                                        currentvec = lastvec.add(Vector(x,0,0))
                                                else:
                                                        currentvec = Vector(x,lastvec.y,0)
                                                seg = Part.LineSegment(lastvec,currentvec).toShape()
                                                lastvec = currentvec
                                                lastpole = None
                                                path.append(seg)
                                elif (d == "V" or d == "v"):
                                        for y in pointlist:
                                                if relative:
                                                        currentvec = lastvec.add(Vector(0,-y,0))
                                                else:
                                                        currentvec = Vector(lastvec.x,-y,0)
                                                if lastvec!=currentvec:
                                                    seg = Part.LineSegment(lastvec,currentvec).toShape()
                                                    lastvec = currentvec
                                                    lastpole = None
                                                    path.append(seg)
                                elif (d == "A" or d == "a"):
                                        for rx,ry,xrotation, largeflag, sweepflag,x,y in \
                                                zip(pointlist[0::7],pointlist[1::7],pointlist[2::7],pointlist[3::7],pointlist[4::7],pointlist[5::7],pointlist[6::7]):

                                                #support for large-arc and x-rotation are missing
                                                if relative:
                                                        currentvec = lastvec.add(Vector(x,-y,0))
                                                else:
                                                        currentvec = Vector(x,-y,0)
                                                chord = currentvec.sub(lastvec)
                                                if (not largeflag) and abs(rx-ry) < 10**(-1*Draft.precision()): # small circular arc
                                                        # perp = chord.cross(Vector(0,0,-1))
                                                        # here is a better way to find the perpendicular
                                                        if sweepflag == 1:
                                                                # clockwise
                                                                perp = DraftVecUtils.rotate2D(chord,-math.pi/2)
                                                        else:
                                                                # anticlockwise
                                                                perp = DraftVecUtils.rotate2D(chord,math.pi/2)
                                                        chord.multiply(.5)
                                                        if chord.Length > rx: a = 0
                                                        else: a = math.sqrt(rx**2-chord.Length**2)
                                                        s = rx - a
                                                        perp.multiply(s/perp.Length)
                                                        midpoint = lastvec.add(chord.add(perp))
                                                        seg = Part.Arc(lastvec,midpoint,currentvec).toShape()
                                                else:# big arc or elliptical arc
                                                        solution,(rx,ry) = arcend2center(lastvec,currentvec,rx,ry,math.radians(-xrotation),True) 
                                                        negsol = (largeflag != sweepflag)
                                                        vcenter,angle1,angledelta = solution[negsol]
                                                        #print angle1
                                                        #print angledelta
                                                        if ry > rx:
                                                                rx,ry=ry,rx
                                                                swapaxis = True
                                                        else:
                                                                swapaxis = False
                                                         #print 'Elliptical arc %s rx=%f ry=%f' % (vcenter,rx,ry)
                                                        e1 = Part.Ellipse(vcenter,rx,ry)
                                                        if sweepflag:
                                                                #angledelta=-(-angledelta % (math.pi *2)) # Step4
                                                                #angledelta=(-angledelta % (math.pi *2)) # Step4
                                                                angle1  = angle1+angledelta
                                                                angledelta = -angledelta
                                                                #angle1 = math.pi - angle1 

                                                        e1a = Part.Arc(e1,angle1-swapaxis*math.radians(90),\
                                                                angle1+angledelta-swapaxis*math.radians(90))
                                                        #e1a = Part.Arc(e1,angle1-0*swapaxis*math.radians(90),angle1+angledelta-0*swapaxis*math.radians(90))
                                                        if swapaxis or xrotation >  10**(-1*Draft.precision()):
                                                                m3=FreeCAD.Matrix()
                                                                m3.move(vcenter)
                                                                rot90=FreeCAD.Matrix(0,-1,0,0,1,0) #90
                                                                #swapaxism=FreeCAD.Matrix(0,1,0,0,1,0) 
                                                                if swapaxis:
                                                                        m3=m3.multiply(rot90)
                                                                m3.rotateZ(math.radians(-xrotation))
                                                                m3.move(vcenter.multiply(-1))
                                                                e1a.transform(m3)
                                                        seg = e1a.toShape()
                                                        if sweepflag:
                                                                seg.reverse()
                                                                #obj = self.doc.addObject("Part::Feature",'DEBUG %s'%pathname) #DEBUG
                                                                #obj.Shape = seg #DEBUG
                                                                #seg = Part.LineSegment(lastvec,currentvec).toShape() #DEBUG
                                                lastvec = currentvec
                                                lastpole = None
                                                path.append(seg)
                                elif (d == "C" or d == "c") or\
                                        (d =="S" or d == "s"):
                                        smooth = (d == 'S'  or d == 's')
                                        if smooth:
                                            piter = list(zip(pointlist[2::4],pointlist[3::4],pointlist[0::4],pointlist[1::4],pointlist[2::4],pointlist[3::4]))
                                        else:
                                            piter = list(zip(pointlist[0::6],pointlist[1::6],pointlist[2::6],pointlist[3::6],pointlist[4::6],pointlist[5::6]))
                                        for p1x,p1y,p2x,p2y,x,y in piter:
                                                if smooth:
                                                        if lastpole is not None and lastpole[0]=='cubic':
                                                                pole1 = lastvec.sub(lastpole[1]).add(lastvec)
                                                        else:
                                                                pole1 = lastvec
                                                else:
                                                        if relative:
                                                                pole1 = lastvec.add(Vector(p1x,-p1y,0))
                                                        else:
                                                                pole1 = Vector(p1x,-p1y,0)
                                                if relative:
                                                        currentvec = lastvec.add(Vector(x,-y,0))
                                                        pole2 = lastvec.add(Vector(p2x,-p2y,0))
                                                else:
                                                        currentvec = Vector(x,-y,0)
                                                        pole2 = Vector(p2x,-p2y,0)

                                                if not DraftVecUtils.equals(currentvec,lastvec):
                                                        mainv = currentvec.sub(lastvec)
                                                        pole1v = lastvec.add(pole1)
                                                        pole2v = currentvec.add(pole2)
                                                        #print "cubic curve data:",mainv.normalize(),pole1v.normalize(),pole2v.normalize()
                                                        if True and \
                                                        pole1.distanceToLine(lastvec,currentvec) < 10**(-1*(2+Draft.precision())) and \
                                                        pole2.distanceToLine(lastvec,currentvec) < 10**(-1*(2+Draft.precision())):
                                                                #print "straight segment"
                                                                seg = Part.LineSegment(lastvec,currentvec).toShape()
                                                        else:
                                                                #print "cubic bezier segment"
                                                                b = Part.BezierCurve()
                                                                b.setPoles([lastvec,pole1,pole2,currentvec])
                                                                seg = b.toShape()
                                                        #print "connect ",lastvec,currentvec
                                                        lastvec = currentvec
                                                        lastpole = ('cubic',pole2)
                                                        path.append(seg)
                                elif (d == "Q" or d == "q") or\
                                        (d =="T" or d == "t"):
                                        smooth = (d == 'T'  or d == 't')
                                        if smooth:
                                            piter = list(zip(pointlist[1::2],pointlist[1::2],pointlist[0::2],pointlist[1::2]))
                                        else:
                                            piter = list(zip(pointlist[0::4],pointlist[1::4],pointlist[2::4],pointlist[3::4]))
                                        for px,py,x,y in piter:
                                                if smooth:
                                                        if lastpole is not None and lastpole[0]=='quadratic':
                                                                pole = lastvec.sub(lastpole[1]).add(lastvec)
                                                        else:
                                                                pole = lastvec
                                                else:
                                                        if relative:
                                                                pole = lastvec.add(Vector(px,-py,0))
                                                        else:
                                                                pole = Vector(px,-py,0)
                                                if relative:
                                                        currentvec = lastvec.add(Vector(x,-y,0))
                                                else:
                                                        currentvec = Vector(x,-y,0)

                                                if not DraftVecUtils.equals(currentvec,lastvec):
                                                        if True and \
                                                        pole.distanceToLine(lastvec,currentvec) < 20**(-1*(2+Draft.precision())):
                                                                #print "straight segment"
                                                                seg = Part.LineSegment(lastvec,currentvec).toShape()
                                                        else:
                                                                #print "quadratic bezier segment"
                                                                b = Part.BezierCurve()
                                                                b.setPoles([lastvec,pole,currentvec])
                                                                seg = b.toShape()
                                                        #print "connect ",lastvec,currentvec
                                                        lastvec = currentvec
                                                        lastpole = ('quadratic',pole)
                                                        path.append(seg)
                                elif (d == "Z") or (d == "z"):
                                        if not DraftVecUtils.equals(lastvec,firstvec):
                                            try:
                                                seg = Part.LineSegment(lastvec,firstvec).toShape()
                                            except Part.OCCError:
                                                pass
                                            else:
                                                path.append(seg)
                                        if path: #the path should be closed by now
                                                #sh=makewire(path,True)
                                                sh=makewire(path,donttry=False)
                                                if self.fill and (len(sh.Wires) == 1) and sh.Wires[0].isClosed():
                                                    sh = Part.Face(sh)
                                                sh = self.applyTrans(sh)
                                                obj = self.doc.addObject("Part::Feature",pathname)
                                                obj.Shape = sh
                                                self.format(obj)
                                                path = []
                                                if firstvec:
                                                        lastvec = firstvec #Move relative to recent draw command
                                                point = []
                                                command = None
                                                if self.currentsymbol:
                                                    self.symbols[self.currentsymbol].append(obj)
                        if path:
                                sh=makewire(path,checkclosed=False)
                                #sh = Part.Wire(path)
                                if self.fill and sh.isClosed():
                                    sh = Part.Face(sh)
                                sh = self.applyTrans(sh)
                                obj = self.doc.addObject("Part::Feature",pathname)
                                obj.Shape = sh
                                self.format(obj)
                                if self.currentsymbol:
                                    self.symbols[self.currentsymbol].append(obj)


                # processing rects

                if name == "rect":
                        if not pathname: pathname = 'Rectangle'
                        edges = []
                        if not "x" in data:
                            data["x"] = 0
                        if not "y" in data:
                            data["y"] = 0
                        if ('rx' not in data or data['rx'] < 10**(-1*Draft.precision())) and \
                           ('ry' not in data or data['ry'] < 10**(-1*Draft.precision())): #negative values are invalid
#                        if True: 
                                p1 = Vector(data['x'],-data['y'],0)
                                p2 = Vector(data['x']+data['width'],-data['y'],0)
                                p3 = Vector(data['x']+data['width'],-data['y']-data['height'],0)
                                p4 = Vector(data['x'],-data['y']-data['height'],0)
                                edges.append(Part.LineSegment(p1,p2).toShape())
                                edges.append(Part.LineSegment(p2,p3).toShape())
                                edges.append(Part.LineSegment(p3,p4).toShape())
                                edges.append(Part.LineSegment(p4,p1).toShape())
                        else: #rounded edges
                                rx = data.get('rx')
                                ry = data.get('ry') or rx
                                rx = rx or ry 
                                if rx > 2 * data['width']:
                                        rx = data['width'] / 2.0
                                if ry > 2 * data['height']:
                                       ry = data['height'] / 2.0

                                p1=Vector(data['x']+rx,-data['y']-data['height']+ry,0)
                                p2=Vector(data['x']+data['width']-rx,-data['y']-data['height']+ry,0)
                                p3=Vector(data['x']+data['width']-rx,-data['y']-ry,0)
                                p4=Vector(data['x']+rx,-data['y']-ry,0)

                                if rx >= ry:
                                        e=Part.Ellipse(Vector(),rx,ry)
                                        e1a=Part.Arc(e,math.radians(180),math.radians(270))
                                        e2a=Part.Arc(e,math.radians(270),math.radians(360))
                                        e3a=Part.Arc(e,math.radians(0),math.radians(90))
                                        e4a=Part.Arc(e,math.radians(90),math.radians(180))
                                        m=FreeCAD.Matrix()
                                else:
                                        e=Part.Ellipse(Vector(),ry,rx)
                                        e1a=Part.Arc(e,math.radians(90),math.radians(180))
                                        e2a=Part.Arc(e,math.radians(180),math.radians(270))
                                        e3a=Part.Arc(e,math.radians(270),math.radians(360))
                                        e4a=Part.Arc(e,math.radians(0),math.radians(90))
                                        m=FreeCAD.Matrix(0,-1,0,0,1,0) # rotate +90 degree
                                esh=[]
                                for arc,point in ((e1a,p1),(e2a,p2),(e3a,p3),(e4a,p4)):
                                        m1=FreeCAD.Matrix(m)
                                        m1.move(point)
                                        arc.transform(m1)
                                        esh.append(arc.toShape())
                                for esh1,esh2 in zip(esh[-1:]+esh[:-1],esh):
                                        p1,p2 = esh1.Vertexes[-1].Point,esh2.Vertexes[0].Point
                                        if not DraftVecUtils.equals(p1,p2):
                                            edges.append(Part.LineSegment(esh1.Vertexes[-1].Point,esh2.Vertexes[0].Point).toShape()) #straight segments
                                        edges.append(esh2) # elliptical segments
                        sh = Part.Wire(edges)
                        if self.fill: sh = Part.Face(sh)
                        sh = self.applyTrans(sh)
                        obj = self.doc.addObject("Part::Feature",pathname)
                        obj.Shape = sh
                        self.format(obj)
                        if self.currentsymbol:
                            self.symbols[self.currentsymbol].append(obj)
                        
                # processing lines

                if name == "line":
                        if not pathname: pathname = 'Line'
                        p1 = Vector(data['x1'],-data['y1'],0)
                        p2 = Vector(data['x2'],-data['y2'],0)
                        sh = Part.LineSegment(p1,p2).toShape()
                        sh = self.applyTrans(sh)
                        obj = self.doc.addObject("Part::Feature",pathname)
                        obj.Shape = sh
                        self.format(obj)
                        if self.currentsymbol:
                            self.symbols[self.currentsymbol].append(obj)

                # processing polylines and polygons

                if name == "polyline" or name == "polygon":
                        '''a simpler implementation would be sh = Part.makePolygon([Vector(svgx,-svgy,0) for svgx,svgy in zip(points[0::2],points[1::2])])
                        but there would be more difficlult to search for duplicate points beforehand.'''
                        if not pathname: pathname = 'Polyline'
                        points=[float(d) for d in data['points']]
                        FreeCAD.Console.PrintMessage('points %s\n'%str(points))
                        lenpoints=len(points)
                        if lenpoints>=4 and lenpoints % 2 == 0:
                                lastvec = Vector(points[0],-points[1],0)
                                path=[]
                                if name == 'polygon':
                                        points=points+points[:2] # emulate closepath
                                for svgx,svgy in zip(points[2::2],points[3::2]):
                                        currentvec = Vector(svgx,-svgy,0)
                                        if not DraftVecUtils.equals(lastvec,currentvec):
                                                seg = Part.LineSegment(lastvec,currentvec).toShape()
                                                #print "polyline seg ",lastvec,currentvec
                                                lastvec = currentvec
                                                path.append(seg)
                                if path:
                                        sh = Part.Wire(path)
                                        if self.fill and sh.isClosed():
                                            sh = Part.Face(sh)
                                        sh = self.applyTrans(sh)
                                        obj = self.doc.addObject("Part::Feature",pathname)
                                        obj.Shape = sh
                                        if self.currentsymbol:
                                            self.symbols[self.currentsymbol].append(obj)

                # processing ellipses

                if (name == "ellipse") :
                        if not pathname: pathname = 'Ellipse'
                        c = Vector(data.get('cx',0),-data.get('cy',0),0)
                        rx = data['rx']
                        ry = data['ry']
                        if rx > ry:
                                sh = Part.Ellipse(c,rx,ry).toShape()
                        else:
                                sh = Part.Ellipse(c,ry,rx).toShape()
                                m3=FreeCAD.Matrix()
                                m3.move(c)
                                rot90=FreeCAD.Matrix(0,-1,0,0,1,0) #90
                                m3=m3.multiply(rot90)
                                m3.move(c.multiply(-1))
                                sh.transformShape(m3)
                                #sh = sh.transformGeometry(m3)
                        if self.fill:
                                sh = Part.Wire([sh])
                                sh = Part.Face(sh)
                        sh = self.applyTrans(sh)
                        obj = self.doc.addObject("Part::Feature",pathname)
                        obj.Shape = sh
                        self.format(obj)
                        if self.currentsymbol:
                            self.symbols[self.currentsymbol].append(obj)


                # processing circles

                if (name == "circle") and (not ("freecad:skip" in data)) :
                        if not pathname: pathname = 'Circle'
                        c = Vector(data.get('cx',0),-data.get('cy',0),0)
                        r = data['r']
                        sh = Part.makeCircle(r)
                        if self.fill:
                                sh = Part.Wire([sh])
                                sh = Part.Face(sh)
                        sh.translate(c)
                        sh = self.applyTrans(sh)
                        obj = self.doc.addObject("Part::Feature",pathname)
                        obj.Shape = sh
                        self.format(obj)
                        if self.currentsymbol:
                            self.symbols[self.currentsymbol].append(obj)

                # processing texts

                if name in ["text","tspan"]:
                        if not("freecad:skip" in data):
                                FreeCAD.Console.PrintMessage("processing a text\n")
                                if 'x' in data:
                                        self.x = data['x']
                                else:
                                        self.x = 0
                                if 'y' in data:
                                        self.y = data['y']
                                else:
                                        self.y = 0
                                if 'font-size' in data:
                                        if data['font-size'] != 'none':
                                                self.text = getsize(data['font-size'],'css'+str(self.svgdpi))
                                else:
                                        self.text = 1
                        else:
                                if self.lastdim:
                                        self.lastdim.ViewObject.FontSize = int(getsize(data['font-size']))
                                        
                # processing symbols
                
                if name == "symbol":
                    self.symbols[pathname] = []
                    self.currentsymbol = pathname
                    
                if name == "use":
                    if "xlink:href" in data:
                        symbol = data["xlink:href"][0][1:]
                        if symbol in self.symbols:
                            FreeCAD.Console.PrintMessage("using symbol "+symbol+"\n")
                            shapes = []
                            for o in self.symbols[symbol]:
                                if o.isDerivedFrom("Part::Feature"):
                                    shapes.append(o.Shape)
                            if shapes:
                                sh = Part.makeCompound(shapes)
                                v = FreeCAD.Vector(float(data['x']),-float(data['y']),0)
                                sh.translate(v)
                                sh = self.applyTrans(sh)
                                obj = self.doc.addObject("Part::Feature",symbol)
                                obj.Shape = sh
                                self.format(obj)
                        else:
                            FreeCAD.Console.PrintMessage("no symbol data\n")

                FreeCAD.Console.PrintMessage("done processing element %d\n"%self.count)
                
        def characters(self,content):
                if self.text:
                        FreeCAD.Console.PrintMessage("reading characters %s\n" % content)
                        obj=self.doc.addObject("App::Annotation",'Text')
                        obj.LabelText = content.encode('latin1')
                        if self.currentsymbol:
                            self.symbols[self.currentsymbol].append(obj)
                        vec = Vector(self.x,-self.y,0)
                        if self.transform:
                                vec = self.translateVec(vec,self.transform)
                                #print "own transform: ",self.transform, vec
                        for transform in self.grouptransform[::-1]:
                                #vec = self.translateVec(vec,transform)
                                vec = transform.multiply(vec)
                        #print "applying vector: ",vec
                        obj.Position = vec
                        if gui:
                                obj.ViewObject.FontSize = int(self.text)
                                if self.fill: obj.ViewObject.TextColor = self.fill
                                else: obj.ViewObject.TextColor = (0.0,0.0,0.0,0.0)

        def endElement(self, name):
            if not name in ["tspan"]:
                self.transform = None
                self.text = None
            if name == "g" or name == "svg":
                FreeCAD.Console.PrintMessage("closing group\n")
                self.grouptransform.pop()
            if name == "symbol":
                if self.doc.getObject("svgsymbols"):
                    group = self.doc.getObject("svgsymbols")
                else:
                    group = self.doc.addObject("App::DocumentObjectGroup","svgsymbols")
                for o in self.symbols[self.currentsymbol]:
                    group.addObject(o)
                self.currentsymbol = None
                    

        def applyTrans(self,sh):
                if isinstance(sh,Part.Shape):
                        if self.transform:
                                FreeCAD.Console.PrintMessage("applying object transform: %s\n" % self.transform)
                                #sh = transformCopyShape(sh,self.transform)
                                # see issue #2062
                                sh = sh.transformGeometry(self.transform)
                        for transform in self.grouptransform[::-1]:
                                FreeCAD.Console.PrintMessage("applying group transform: %s\n" % transform)
                                #sh = transformCopyShape(sh,transform)
                                # see issue 2062
                                sh = sh.transformGeometry(transform)
                        return sh
                elif Draft.getType(sh) == "Dimension":
                        pts = []
                        for p in [sh.Start,sh.End,sh.Dimline]:
                                cp = Vector(p)
                                if self.transform:
                                        FreeCAD.Console.PrintMessage("applying object transform: %s\n" % self.transform)
                                        cp = self.transform.multiply(cp)
                                for transform in self.grouptransform[::-1]:
                                        FreeCAD.Console.PrintMessage("applying group transform: %s\n" % transform)
                                        cp = transform.multiply(cp)
                                pts.append(cp)
                        sh.Start = pts[0]
                        sh.End = pts[1]
                        sh.Dimline = pts[2]

        def translateVec(self,vec,mat):
                v = Vector(mat.A14,mat.A24,mat.A34)
                return vec.add(v)

        def getMatrix(self,tr):
                "returns a FreeCAD matrix from a svg transform attribute"
                transformre=re.compile('(matrix|translate|scale|rotate|skewX|skewY)\s*?\((.*?)\)',re.DOTALL)
                m = FreeCAD.Matrix()
                for transformation, arguments in transformre.findall(tr):
                        argsplit=[float(arg) for arg in arguments.replace(',',' ').split()]
                        #m.multiply(FreeCAD.Matrix (1,0,0,0,0,-1))
                        #print '%s:%s %s %d' % (transformation, arguments,argsplit,len(argsplit))
                        if transformation == 'translate':
                                tx = argsplit[0]
                                ty = argsplit[1] if len(argsplit) > 1 else 0.0
                                m.move(Vector(tx,-ty,0))
                        elif transformation == 'scale':
                                sx = argsplit[0]
                                sy = argsplit[1] if len(argsplit) > 1 else sx
                                m.scale(Vector(sx,sy,1))
                        elif transformation == 'rotate':
                                cx = 0
                                cy = 0
                                angle = argsplit[0]
                                if len(argsplit) >= 3:
                                        cx = argsplit[1]
                                        cy = argsplit[2]
                                        m.move(Vector(cx,-cy,0))
                                m.rotateZ(math.radians(-angle)) #mirroring one axis equals changing the direction of rotation
                                if len(argsplit) >= 3:
                                        m.move(Vector(-cx,cy,0))
                        elif transformation == 'skewX':
                                m=m.multiply(FreeCAD.Matrix(1,-math.tan(math.radians(argsplit[0]))))
                        elif transformation == 'skewY':
                                m=m.multiply(FreeCAD.Matrix(1,0,0,0,-math.tan(math.radians(argsplit[0]))))
                        elif transformation == 'matrix':
#                            '''transformation matrix:
#                                   FreeCAD                 SVG
#                                (+A -C +0 +E)           (A C 0 E)
#                                (-B +D -0 -F)  = (-Y) * (B D 0 F) *(-Y)
#                                (+0 -0 +1 +0)           (0 0 1 0)
#                                (+0 -0 +0 +1)           (0 0 0 1)'''
                                m=m.multiply(FreeCAD.Matrix(argsplit[0],-argsplit[2],0,argsplit[4],-argsplit[1],argsplit[3],0,-argsplit[5]))
                        #else:
                                #print 'SKIPPED %s' % transformation
                        #print "m= ",m
                #print "generating transformation: ",m
                return m

def decodeName(name):
        "decodes encoded strings"
        try:
                decodedName = (name.decode("utf8"))
        except UnicodeDecodeError:
                try:
                        decodedName = (name.decode("latin1"))
                except UnicodeDecodeError:
                        FreeCAD.Console.PrintError("svg: error: couldn't determine character encoding\n")

                        decodedName = name
        return decodedName

def getContents(filename,tag,stringmode=False):
        "gets the contents of all the occurrences of the given tag in the given file"
        result = {}
        if stringmode:
                contents = filename
        else:
                f = pythonopen(filename)
                contents = f.read()
                f.close()
        contents = contents.replace('\n','_linebreak')
        searchpat = '<'+tag+'.*?</'+tag+'>'
        tags = re.findall(searchpat,contents)
        for t in tags:
                tagid = re.findall('id="(.*?)"',t)
                if tagid:
                        tagid = tagid[0]
                else:
                        tagid = 'none'
                res = t.replace('_linebreak','\n')
                result[tagid] = res
        return result

def open(filename):
        docname=os.path.split(filename)[1]
        doc=FreeCAD.newDocument(docname)
        doc.Label = docname[:-4]
        parser = xml.sax.make_parser()
        parser.setFeature(xml.sax.handler.feature_external_ges, False)
        parser.setContentHandler(svgHandler())
        parser._cont_handler.doc = doc
        f = pythonopen(filename)
        parser.parse(f)
        f.close()
        doc.recompute()
        return doc

def insert(filename,docname):
        try:
                doc=FreeCAD.getDocument(docname)
        except NameError:
                doc=FreeCAD.newDocument(docname)
        FreeCAD.ActiveDocument = doc
        parser = xml.sax.make_parser()
        parser.setFeature(xml.sax.handler.feature_external_ges, False)
        parser.setContentHandler(svgHandler())
        parser._cont_handler.doc = doc
        parser.parse(pythonopen(filename))
        doc.recompute()

def export(exportList,filename):
        "called when freecad exports a file"

        svg_export_style = FreeCAD.ParamGet("User parameter:BaseApp/Preferences/Mod/Draft").GetInt("svg_export_style")
        if svg_export_style != 0 and svg_export_style != 1:
            FreeCAD.Console.PrintMessage(translate("Unknown SVG export style, switching to Translated")+"\n")
            svg_export_style = 0

        # finding sheet size
        bb = None
        for ob in exportList:
                if ob.isDerivedFrom("Part::Feature"):
                    if bb:
                        bb.add(ob.Shape.BoundBox)
                    else:
                        bb = ob.Shape.BoundBox
        if bb:
            minx = bb.XMin
            maxx = bb.XMax
            miny = bb.YMin
            maxy = bb.YMax
        else:
            FreeCAD.Console.PrintError("The export list contains no shape\n")
            return
            
        if svg_export_style == 0:
            # translated-style exports get a bit of a margin
            margin = (maxx-minx)*.01
        else:
            # raw-style exports get no margin
            margin = 0

        minx -= margin 
        maxx += margin
        miny -= margin
        maxy += margin
        sizex = maxx-minx
        sizey = maxy-miny
        miny += margin

        # writing header
        # we specify the svg width and height in FreeCAD's physical units (mm),
        # and specify the viewBox so that user units maps one-to-one to mm
        svg = pythonopen(filename,'w') 
        svg.write('<?xml version="1.0"?>\n')
        svg.write('<!DOCTYPE svg PUBLIC "-//W3C//DTD SVG 1.1//EN"')
        svg.write(' "http://www.w3.org/Graphics/SVG/1.1/DTD/svg11.dtd">\n')
        svg.write('<svg')
        svg.write(' width="' + str(sizex) + 'mm" height="' + str(sizey) + 'mm"')
        if svg_export_style == 0:
            # translated-style exports have the viewbox starting at X=0, Y=0
            svg.write(' viewBox="0 0 ' + str(sizex) + ' ' + str(sizey) + '"')
        else:
            # raw-style exports have the viewbox starting at X=xmin, Y=-ymax
            # we need the funny Y here because SVG is upside down, and we
            # flip the sketch right-way up with a scale later
            svg.write(' viewBox="%f %f %f %f"' %(minx,-maxy,sizex,sizey))
        svg.write(' xmlns="http://www.w3.org/2000/svg" version="1.1"')
        svg.write('>\n')

        # writing paths
        for ob in exportList:
                if svg_export_style == 0:
                    # translated-style exports have the entire sketch translated to fit in the X>0, Y>0 quadrant
                    #svg.write('<g transform="translate('+str(-minx)+','+str(-miny+(2*margin))+') scale(1,-1)">\n')
                    svg.write('<g id="%s" transform="translate(%f,%f) '
                            'scale(1,-1)">\n'% (ob.Name,-minx,maxy))
                else:
                    # raw-style exports do not translate the sketch
                    svg.write('<g id="%s" transform="scale(1,-1)">\n' %\
                            ob.Name)
                svg.write(Draft.getSVG(ob))
                svg.write('<title>%s</title>\n' % str(ob.Label.encode('utf8'))\
                        .replace('<','&lt;').replace('>','&gt;'))
                        # replace('"',\ "&quot;")
                svg.write('</g>\n')
        # closing
        svg.write('</svg>')
        svg.close()
