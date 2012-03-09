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
# FreeCAD modules
import FreeCAD,FreeCADGui
from FreeCAD import Part, Base
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
        Output.write(" # 1: Ship displacement [ton]\n")
        Output.write(" # 2: Draft [m]\n")
        Output.write(" # 3: Wetted surface [m2]\n")
        Output.write(" # 4: 1cm triming ship moment [ton m]\n")
        Output.write(" # 5: Bouyance center x coordinate\n")
        Output.write(" # 6: Floating area\n")
        Output.write(" # 7: KBt\n")
        Output.write(" # 8: BMt\n")
        Output.write(" #\n")
        Output.write(" #################################################################\n")
        # Print data
        for i in range(0,len(drafts)):
            draft = drafts[i]
            point = Tools.Point(ship,draft,trim)
            string = "%f %f %f %f %f %f %f %f\n" % (point.disp, point.draft, point.wet, point.mom, point.xcb, point.farea, point.KBt, point.BMt)
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
        # Convert volume
        comm = "gs -r300 -dEPSCrop -dTextAlphaBits=4 -sDEVICE=png16m -sOutputFile=%s.png -dBATCH -dNOPAUSE %s.eps" % (filename,filename)
        if os.system(comm):
            msg = Translator.translate("Can't execute ghostscript. Maybe is not installed?\n")
            FreeCAD.Console.PrintError(msg)
            msg = Translator.translate("Generated image will not converted to png\n")
            FreeCAD.Console.PrintError(msg)
            return True
        # Convert stability
        filename = self.path + 'stability'
        comm = "gs -r300 -dEPSCrop -dTextAlphaBits=4 -sDEVICE=png16m -sOutputFile=%s.png -dBATCH -dNOPAUSE %s.eps" % (filename,filename)
        if os.system(comm):
            msg = Translator.translate("Can't execute ghostscript. Maybe is not installed?\n")
            FreeCAD.Console.PrintError(msg)
            msg = Translator.translate("Generated image will not converted to png\n")
            FreeCAD.Console.PrintError(msg)
            return True
        return False
