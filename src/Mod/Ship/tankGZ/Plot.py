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
import Part, Image, ImageGui
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
    def __init__(self, x, y, disp, draft, trim):
        """ Constructor. performs plot and show it (Using pyxplot).
        @param x Roll angles [deg].
        @param y GZ value [m].
        @param disp Ship displacement [tons].
        @param draft Ship draft [m].
        @param trim Ship trim angle [deg].
        """
        if self.createDirectory():
            return
        if self.saveData(x,y):
            return
        if self.saveLayout(x,y, disp, draft, trim):
            return
        if self.execute():
            return
        ImageGui.open(self.path + 'gz.png')

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

    def saveData(self,x,y):
        """ Write data file.
        @param x Roll angles.
        @param y GZ value.
        @return True if error happens.
        """
        # Open the file
        filename = self.path + 'gz.dat'
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
        Output.write(" # This file contains transversal GZ stability parameter, filled with following columns:\n")
        Output.write(" # 1: Roll angles [deg]\n")
        Output.write(" # 2: GZ [m]\n")
        Output.write(" #\n")
        Output.write(" #################################################################\n")
        # Print data
        if len(x) < 2:
            msg = Translator.translate("Not enough data to plot.\n")
            FreeCAD.Console.PrintError(msg)
            return True
        for i in range(0, len(x)):
            string = "%f %f\n" % (x[i], y[i])
            Output.write(string)
        # Close file
        Output.close()
        self.dataFile = filename
        msg = Translator.translate("Data saved at '" + self.dataFile + "'.\n")
        FreeCAD.Console.PrintMessage(msg)
        return False

    def saveLayout(self, x, y, disp, draft, trim):
        """ Prints the data output.
        @param x Roll angles.
        @param y GZ value.
        @param disp Ship displacement.
        @param draft Ship draft.
        @param trim Ship trim angle.
        @return True if error happens.
        """
        filename = self.path + 'gz.pyxplot'
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
        Output.write(" # This file contains a script to plot transversal GZ stability parameter.\n")
        Output.write(" # To use it execute:\n")
        Output.write(" #\n")
        Output.write(" # pyxplot %s\n" % (filename))
        Output.write(" #\n")
        Output.write(" #################################################################\n")
        # Write general options for hydrostatics
        Output.write("set numeric display latex\n")
        Output.write("set output '%s'\n" % (self.path + 'gz.eps'))
        Output.write("set nokey\n")
        Output.write("set grid\n")
        Output.write("# X axis\n")
        Output.write("set xlabel '$roll$ / degrees'\n")
        Output.write("set xtic\n")
        Output.write("# Y axis\n")
        Output.write("set ylabel '$GZ$ / m'\n")
        Output.write("set ytic\n")
        Output.write("# Line styles\n")
        Output.write("set style 1 line linetype 1 linewidth 2 colour rgb (0):(0):(0)\n")        
        # Additional data
        Output.write("# Additional data\n")
        Output.write("set label (1) '$\\Delta = %g \\mathrm{tons}$' %f,%f\n" % (disp, x[0] + 0.65*(x[-1] - x[0]), min(y) + 0.95*(max(y)-min(y))))
        Output.write("set label (2) '$T = %g \\mathrm{m}$' %f,%f\n" % (draft, x[0] + 0.65*(x[-1] - x[0]), min(y) + 0.85*(max(y)-min(y))))
        Output.write("set label (3) '$Trim = %g^\\circ$' %f,%f\n" % (trim, x[0] + 0.65*(x[-1] - x[0]), min(y) + 0.75*(max(y)-min(y))))
        # Write plot call
        Output.write("# Plot\n")        
        Output.write("plot '%s' using 1:2 title 'GZ' axes x1y1 with lines style 1\n" % (self.dataFile))
        # Close file
        self.layoutFile = filename
        Output.close()
        return False

    def execute(self):
        """ Calls pyxplot in order to plot an save an image.
        @return True if error happens.
        """
        filename = self.path + 'gz'
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
