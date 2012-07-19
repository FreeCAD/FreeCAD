#***************************************************************************
#*                                                                         *
#*   Copyright (c) 2011, 2012                                              *  
#*   Jose Luis Cercos Pita <jlcercos@gmail.com>                            *  
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

import os
import math
# FreeCAD modules
import FreeCAD,FreeCADGui
from FreeCAD import Part, Base, Vector
from FreeCAD import Image, ImageGui
# FreeCADShip modules
from shipUtils import Paths, Translator
import Tools

header = """ #################################################################

 #####                 ####  ###   ####      ##### #   # ### ####
 #                    #      # #   #   #    #      #   #  #  #   #
 #     ##  #### ####  #     #   #  #   #     #     #   #  #  #   #
 ####  # # #  # #  #  #     #####  #   # ##   ##   #####  #  ####
 #     #   #### ####  #    #     # #   #        #  #   #  #  #
 #     #   #    #     #    #     # #   #         # #   #  #  #
 #     #   #### ####   ### #     # ####     #####  #   # ### #

 #################################################################
"""

class Plot(object):
    def __init__(self, ship, trim, drafts):
        """ Constructor. performs plot and show it (Using pyxplot).
        @param ship Selected ship instance
        @param trim Trim in degrees.
        @param drafts List of drafts to be performed.
        """
        if self.createDirectory():
            return
        if self.saveData(ship, trim, drafts):
            return
        if self.saveLayout(trim):
            return
        if self.execute():
            return
        ImageGui.open(self.path + 'volume.png')
        ImageGui.open(self.path + 'stability.png')
        ImageGui.open(self.path + 'coeffs.png')

    def createDirectory(self):
        """ Create needed folder to write data and scripts.
        @return True if error happens.
        """
        self.path = FreeCAD.ConfigGet("UserAppData") + "ShipOutput/"
        if not os.path.exists(self.path):
            os.makedirs(self.path)
        if not os.path.exists(self.path):
            msg = Translator.translate("Can't create '" + self.path + "' folder.\n")
            FreeCAD.Console.PrintError(msg)
        return False

    def saveData(self, ship, trim, drafts):
        """ Write data file.
        @param ship Selected ship instance
        @param trim Trim in degrees.
        @param drafts List of drafts to be performed.
        @return True if error happens.
        """
        # Open the file
        filename = self.path + 'hydrostatics.dat'
        try:
            Output = open(filename, "w")
        except IOError:
            msg = Translator.translate("Can't write '" + filename + "' file.\n")
            FreeCAD.Console.PrintError(msg)
            return True
        # Print header
        Output.write(header)
        Output.write(" #\n")
        Output.write(" # File automatically exported by FreeCAD-Ship\n")
        Output.write(" # This file contains transversal areas data, filled with following columns:\n")
        Output.write(" #  1: Ship displacement [ton]\n")
        Output.write(" #  2: Draft [m]\n")
        Output.write(" #  3: Wetted surface [m2]\n")
        Output.write(" #  4: 1cm triming ship moment [ton m]\n")
        Output.write(" #  5: Bouyance center x coordinate\n")
        Output.write(" #  6: Floating area\n")
        Output.write(" #  7: KBt\n")
        Output.write(" #  8: BMt\n")
        Output.write(" #  9: Cb (block coefficient)\n")
        Output.write(" # 10: Cf (Floating coefficient)\n")
        Output.write(" # 11: Cm (Main frame coefficient)\n")
        Output.write(" #\n")
        Output.write(" #################################################################\n")
        # Get external faces
        faces = self.externalFaces(ship.Shape)
        if len(faces) == 0:
            msg = Translator.translate("Can't detect external faces from ship object.\n")
            FreeCAD.Console.PrintError(msg)
        else:
            faces = Part.makeShell(faces)
        # Print data
        FreeCAD.Console.PrintMessage("Computing hydrostatics...\n")
        for i in range(0,len(drafts)):
            FreeCAD.Console.PrintMessage("\t%d / %d\n" % (i+1, len(drafts)))
            draft = drafts[i]
            point = Tools.Point(ship,faces,draft,trim)
            string = "%f %f %f %f %f %f %f %f %f %f %f\n" % (point.disp, point.draft, point.wet, point.mom, point.xcb, point.farea, point.KBt, point.BMt, point.Cb, point.Cf, point.Cm)
            Output.write(string)
        # Close file
        Output.close()
        self.dataFile = filename
        msg = Translator.translate("Data saved at '" + self.dataFile + "'.\n")
        FreeCAD.Console.PrintMessage(msg)
        return False

    def saveLayout(self, trim):
        """ Prints the pyxplot layout.
        @param trim Trim in degrees.
        @return True if error happens.
        """
        filename = self.path + 'volume.pyxplot'
        # Open the file
        try:
            Output = open(filename, "w")
        except IOError:
            msg = Translator.translate("Can't write '" + filename + "' file.\n")
            FreeCAD.Console.PrintError(msg)
            return True
        # Write header
        Output.write(header)
        Output.write(" #\n")
        Output.write(" # File automatically exported by FreeCAD-Ship\n")
        Output.write(" # This file contains a script to plot transversal areas curve.\n")
        Output.write(" # To use it execute:\n")
        Output.write(" #\n")
        Output.write(" # pyxplot %s\n" % (filename))
        Output.write(" #\n")
        Output.write(" #################################################################\n")
        # Write general options for hydrostatics
        Output.write("set numeric display latex\n")
        Output.write("set output '%s'\n" % (self.path + 'volume.eps'))
        Output.write("set title '$trim$ = %g [degrees]'\n" % (trim))
        Output.write("set key below\n")
        Output.write("set grid\n")
        # Configure axis
        Output.write("# Y axis\n")
        Output.write("set ylabel '$\\bigtriangleup$ / $\\mathrm{ton}$'\n")
        Output.write("set ytic\n")
        Output.write("# X axis\n")
        Output.write("set xlabel '$Draft$ / $\\mathrm{m}$'\n")
        Output.write("set xtic\n")
        Output.write("set x2label '\\textit{Wetted area} / $\\mathrm{m}^2$'\n")
        Output.write("set x2tic\n")
        Output.write("set x3label '\\textit{1cm trim moment} / $\\mathrm{ton} \\times \\mathrm{m}$'\n")
        Output.write("set x3tic\n")
        Output.write("set x4label '$XCB$ / $\\mathrm{m}$'\n")
        Output.write("set x4tic\n")
        Output.write("set axis x2 top\n")
        Output.write("set axis x4 top\n")
        Output.write("# Line styles\n")
        Output.write("set style 1 line linetype 1 linewidth 1 colour rgb (0):(0):(0)\n")
        Output.write("set style 2 line linetype 1 linewidth 1 colour rgb (1):(0):(0)\n")        
        Output.write("set style 3 line linetype 1 linewidth 1 colour rgb (0):(0):(1)\n")        
        Output.write("set style 4 line linetype 1 linewidth 1 colour rgb (0.1):(0.5):(0.1)\n")        
        # Write plot call
        Output.write("# Plot\n")        
        Output.write("plot '%s' using 2:1 title 'Draft' axes x1y1 with lines style 1, \\\n" % (self.dataFile))        
        Output.write("     '' using 3:1 title 'Wetted area' axes x2y1 with lines style 2, \\\n")        
        Output.write("     '' using 4:1 title '1cm trim moment' axes x3y1 with lines style 3, \\\n")        
        Output.write("     '' using 5:1 title 'XCB' axes x4y1 with lines style 4\n")        
        # Prepare second plot
        Output.write("set output '%s'\n" % (self.path + 'stability.eps'))
        Output.write("# X axis\n")
        Output.write("set x2label '\\textit{Floating area} / $\\mathrm{m}^2$'\n")
        Output.write("set x2tic\n")
        Output.write("set x3label '$KB_{T}$ / $\\mathrm{m}$'\n")
        Output.write("set x3tic\n")
        Output.write("set x4label '$BM_{T}$ / $\\mathrm{m}$'\n")
        Output.write("set x4tic\n")
        # Write plot call
        Output.write("# Plot\n")        
        Output.write("plot '%s' using 2:1 title 'Draft' axes x1y1 with lines style 1, \\\n" % (self.dataFile))        
        Output.write("     '' using 6:1 title 'Floating area' axes x2y1 with lines style 2, \\\n")        
        Output.write("     '' using 7:1 title '$KB_{T}$' axes x3y1 with lines style 3, \\\n")        
        Output.write("     '' using 8:1 title '$BM_{T}$' axes x4y1 with lines style 4\n")        
        # Prepare third plot
        Output.write("set output '%s'\n" % (self.path + 'coeffs.eps'))
        Output.write("# X axis\n")
        Output.write("set x2label '$C_{B}$'\n")
        Output.write("set x2tic\n")
        Output.write("set x3label '$C_{F}$'\n")
        Output.write("set x3tic\n")
        Output.write("set x4label '$C_{M}$'\n")
        Output.write("set x4tic\n")
        # Write plot call
        Output.write("# Plot\n")        
        Output.write("plot '%s' using 2:1 title 'Draft' axes x1y1 with lines style 1, \\\n" % (self.dataFile))        
        Output.write("     '' using 9:1 title '$C_{B}$' axes x2y1 with lines style 2, \\\n")        
        Output.write("     '' using 10:1 title '$C_{F}$' axes x3y1 with lines style 3, \\\n")        
        Output.write("     '' using 11:1 title '$C_{M}$' axes x4y1 with lines style 4\n")        
        # Close file
        self.layoutFile = filename
        Output.close()
        return False

    def execute(self):
        """ Calls pyxplot in order to plot an save an image.
        @return True if error happens.
        """
        # Plot
        filename = self.path + 'volume'
        comm = "pyxplot %s" % (self.layoutFile)
        if os.system(comm):
            msg = Translator.translate("Can't execute pyxplot. Maybe is not installed?\n")
            FreeCAD.Console.PrintError(msg)
            msg = Translator.translate("Plot will not generated\n")
            FreeCAD.Console.PrintError(msg)
            return True
        # Convert volume image
        comm = "gs -r300 -dEPSCrop -dTextAlphaBits=4 -sDEVICE=png16m -sOutputFile=%s.png -dBATCH -dNOPAUSE %s.eps" % (filename,filename)
        if os.system(comm):
            msg = Translator.translate("Can't execute ghostscript. Maybe is not installed?\n")
            FreeCAD.Console.PrintError(msg)
            msg = Translator.translate("Generated image will not converted to png\n")
            FreeCAD.Console.PrintError(msg)
            return True
        # Convert stability image
        filename = self.path + 'stability'
        comm = "gs -r300 -dEPSCrop -dTextAlphaBits=4 -sDEVICE=png16m -sOutputFile=%s.png -dBATCH -dNOPAUSE %s.eps" % (filename,filename)
        if os.system(comm):
            msg = Translator.translate("Can't execute ghostscript. Maybe is not installed?\n")
            FreeCAD.Console.PrintError(msg)
            msg = Translator.translate("Generated image will not converted to png\n")
            FreeCAD.Console.PrintError(msg)
            return True
        # Convert coefficients image
        filename = self.path + 'coeffs'
        comm = "gs -r300 -dEPSCrop -dTextAlphaBits=4 -sDEVICE=png16m -sOutputFile=%s.png -dBATCH -dNOPAUSE %s.eps" % (filename,filename)
        if os.system(comm):
            msg = Translator.translate("Can't execute ghostscript. Maybe is not installed?\n")
            FreeCAD.Console.PrintError(msg)
            msg = Translator.translate("Generated image will not converted to png\n")
            FreeCAD.Console.PrintError(msg)
            return True
        return False

    def lineFaceSection(self,line,surface):
        """ Returns the point of section of a line with a face
        @param line Line object, that can be a curve.
        @param surface Surface object (must be a Part::Shape)
        @return Section points array, [] if line don't cut surface
        """
        # Get initial data
        result = []
        vertexes = line.Vertexes
        nVertex = len(vertexes)
        # Perform the cut
        section = line.cut(surface)
        # Filter all old points
        points = section.Vertexes
        return points

    def externalFaces(self, shape):
        """ Returns detected external faces.
        @param shape Shape where external faces wanted.
        @return List of external faces detected.
        """
        result = []
        faces  = shape.Faces
        bbox   = shape.BoundBox
        L      = bbox.XMax - bbox.XMin
        B      = bbox.YMax - bbox.YMin
        T      = bbox.ZMax - bbox.ZMin
        dist   = math.sqrt(L*L + B*B + T*T)
        FreeCAD.Console.PrintMessage("Computing external faces...\n")
        # Valid/unvalid faces detection loop
        for i in range(0,len(faces)):
            FreeCAD.Console.PrintMessage("\t%d / %d\n" % (i+1, len(faces)))
            f = faces[i]
            # Create a line normal to surface at middle point
            u = 0.0
            v = 0.0
            try:
                surf    = f.Surface
                u       = 0.5*(surf.getUKnots()[0]+surf.getUKnots()[-1])
                v       = 0.5*(surf.getVKnots()[0]+surf.getVKnots()[-1])
            except:
                cog   = f.CenterOfMass
                [u,v] = f.Surface.parameter(cog)
            p0 = f.valueAt(u,v)
            try:
                n  = f.normalAt(u,v).normalize()
            except:
                continue
            p1 = p0 + n.multiply(1.5*dist)
            line = Part.makeLine(p0, p1)
            # Look for faces in front of this
            nPoints = 0
            for j in range(0,len(faces)):
                f2 = faces[j]
                section = self.lineFaceSection(line, f2)
                if len(section) <= 2:
                    continue
                # Add points discarding start and end
                nPoints = nPoints + len(section) - 2
            # In order to avoid special directions we can modify line
            # normal a little bit.
            angle = 5
            line.rotate(p0,Vector(1,0,0),angle)
            line.rotate(p0,Vector(0,1,0),angle)
            line.rotate(p0,Vector(0,0,1),angle)
            nPoints2 = 0
            for j in range(0,len(faces)):
                if i == j:
                    continue
                f2 = faces[j]
                section = self.lineFaceSection(line, f2)
                if len(section) <= 2:
                    continue
                # Add points discarding start and end
                nPoints2 = nPoints + len(section) - 2
            # If the number of intersection points is pair, is a
            # external face. So if we found an odd points intersection,
            # face must be discarded.
            if (nPoints % 2) or (nPoints2 % 2):
                continue
            result.append(f)
        return result
