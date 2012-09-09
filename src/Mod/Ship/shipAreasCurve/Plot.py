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
from FreeCAD import Base
import Image, ImageGui
# FreeCADShip modules
from shipUtils import Paths, Translator

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
    def __init__(self, x, y, disp, xcb, ship):
        """ Constructor. performs plot and show it (Using pyxplot).
        @param x X coordinates.
        @param y Transversal areas.
        @param disp Ship displacement.
        @param xcb Bouyancy center length.
        @param ship Active ship instance.
        """
        if self.createDirectory():
            return
        if self.saveData(x,y,ship):
            return
        if self.saveLayout(x,y,disp,xcb,ship):
            return
        if self.execute():
            return
        ImageGui.open(self.path + 'areas.png')

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

    def saveData(self,x,y,ship):
        """ Write data file.
        @param x X coordinates.
        @param y Transversal areas.
        @param ship Active ship instance.
        @return True if error happens.
        """
        # Open the file
        filename = self.path + 'areas.dat'
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
        Output.write(" # 1: X coordiante [m]\n")
        Output.write(" # 2: Transversal area [m2]\n")
        Output.write(" # 3: X FP coordinate [m]\n")
        Output.write(" # 4: Y FP coordinate (bounds in order to draw it)\n")
        Output.write(" # 3: X AP coordinate [m]\n")
        Output.write(" # 4: Y AP coordinate (bounds in order to draw it)\n")
        Output.write(" #\n")
        Output.write(" #################################################################\n")
        # Get perpendiculars data
        Lpp = ship.Length
        FPx =  0.5*Lpp
        APx = -0.5*Lpp
        maxArea = max(y)
        # Print data
        if len(x) < 2:
            msg = Translator.translate("Not enough data to plot.\n")
            FreeCAD.Console.PrintError(msg)
        string = "%f %f %f %f %f %f\n" % (x[0], y[0], FPx, 0.0, APx, 0.0)
        Output.write(string)
        for i in range(1, len(x)):
            string = "%f %f %f %f %f %f\n" % (x[i], y[i], FPx, maxArea, APx, maxArea)
            Output.write(string)
        # Close file
        Output.close()
        self.dataFile = filename
        msg = Translator.translate("Data saved at '" + self.dataFile + "'.\n")
        FreeCAD.Console.PrintMessage(msg)
        return False

    def saveLayout(self, x, y, disp, xcb, ship):
        """ Prints the data output.
        @param x X coordinates.
        @param y Transversal areas.
        @param disp Ship displacement.
        @param xcb Bouyancy center length.
        @param ship Active ship instance.
        @return True if error happens.
        """
        filename = self.path + 'areas.pyxplot'
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
        Output.write("set output '%s'\n" % (self.path + 'areas.eps'))
        Output.write("set nokey\n")
        Output.write("set grid\n")
        Output.write("# X axis\n")
        Output.write("set xlabel 'x / $m$'\n")
        Output.write("set xtic\n")
        Output.write("# Y axis\n")
        Output.write("set ylabel 'Area / $m^2$'\n")
        Output.write("set ytic\n")
        Output.write("# Line styles\n")
        Output.write("set style 1 line linetype 1 linewidth 1 colour rgb (0):(0):(0)\n")
        Output.write("set style 2 line linetype 1 linewidth 2 colour rgb (0):(0):(0)\n")        
        # Get perpendiculars data
        Lpp = ship.Length
        FPx =  0.5*Lpp
        APx = -0.5*Lpp
        maxArea = max(y)
        # Perpendicular labels
        Output.write("# Perpendiculars labels\n")
        Output.write("set label (1) AP %f,%f\n" % (APx+0.01*Lpp, 0.01*maxArea))
        Output.write("set label (2) AP %f,%f\n" % (APx+0.01*Lpp, 0.95*maxArea))
        Output.write("set label (3) FP %f,%f\n" % (FPx+0.01*Lpp, 0.01*maxArea))
        Output.write("set label (4) FP %f,%f\n" % (FPx+0.01*Lpp, 0.95*maxArea))
        # Additional data
        Output.write("# Additional data\n")
        Output.write("set label (5) 'XCB = %g m' %f,%f\n" % (xcb, -0.25*Lpp, 0.25*maxArea))
        Output.write("set label (6) 'Maximum area = %g m2' %f,%f\n" % (maxArea, -0.25*Lpp, 0.15*maxArea))
        Output.write("set label (7) 'Displacement = %g tons' %f,%f\n" % (disp, -0.25*Lpp, 0.05*maxArea))
        # Write plot call
        Output.write("# Plot\n")        
        Output.write("plot '%s' using 1:2 title 'Transversal areas' axes x1y1 with lines style 1, \\\n" % (self.dataFile))        
        Output.write("     '%s' using 3:4 title 'FP' axes x1y1 with lines style 2, \\\n" % (self.dataFile))        
        Output.write("     '%s' using 5:6 title 'AP' axes x1y1 with lines style 2\n" % (self.dataFile))        
        # Close file
        self.layoutFile = filename
        Output.close()
        return False

    def execute(self):
        """ Calls pyxplot in order to plot an save an image.
        @return True if error happens.
        """
        filename = self.path + 'areas'
        comm = "pyxplot %s" % (self.layoutFile)
        if os.system(comm):
            msg = Translator.translate("Can't execute pyxplot. Maybe is not installed?\n")
            FreeCAD.Console.PrintError(msg)
            msg = Translator.translate("Plot will not generated\n")
            FreeCAD.Console.PrintError(msg)
            return True
        comm = "gs -r300 -dEPSCrop -dTextAlphaBits=4 -sDEVICE=png16m -sOutputFile=%s.png -dBATCH -dNOPAUSE %s.eps" % (filename,filename)
        if os.system(comm):
            msg = Translator.translate("Can't execute ghostscript. Maybe is not installed?\n")
            FreeCAD.Console.PrintError(msg)
            msg = Translator.translate("Generated image will not converted to png\n")
            FreeCAD.Console.PrintError(msg)
            return True
        return False
