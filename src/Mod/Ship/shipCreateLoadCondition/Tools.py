#***************************************************************************
#*                                                                         *
#*   Copyright (c) 2016                                                    *
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
#*   GNU Library General Public License for more detaillc.                  *
#*                                                                         *
#*   You should have received a copy of the GNU Library General Public     *
#*   License along with this program; if not, write to the Free Software   *
#*   Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  *
#*   USA                                                                   *
#*                                                                         *
#***************************************************************************


import FreeCAD as App
import Spreadsheet
import Units


READ_ONLY_FOREGROUND = (0.5, 0.5, 0.5)
READ_ONLY_BACKGROUND = (0.9, 0.9, 0.9)


def createLoadCondition(ship):
    """Create a new loading condition spreadsheet

    Position arguments:
    ship -- Ship object

    Returned value:
    lc -- The new loading condition spreadsheet object

    The new spreadsheet will initialize all the tanks as empty. To modify the
    fluid density and the filling level percentages the columns D and E should
    be edited (the first tank can be found at the row 6)

    For instance, the following code snippet can be used to set the first tank
    50% filled with water:
    lc.set("D6", "998.0")
    lc.set("E6", "0.5")

    The label of the tank can be extracted from the column C, for instance:
    lc.get("C6")
    Such information can be used to get the tank object (the followinbg code
    may fail if either the tank has been removed, or several objects with the
    same label already exist):
    tank = App.ActiveDocument.getObjectsByLabel(lc.get("C6"))[0]

    The tool will claim the new spreadsheet loading condition as a child of the
    ship object. Please do not remove the partner ship object before removing
    this new loading condition before.
    """
    # Create the spreadsheet
    lc = App.activeDocument().addObject('Spreadsheet::Sheet',
                                       'LoadCondition')

    # Add a description
    lc.setForeground('A1:B2', READ_ONLY_FOREGROUND)
    lc.setBackground('A1:B2', READ_ONLY_BACKGROUND)
    lc.setAlignment('B1:B2', 'center', 'keep')
    lc.setStyle('B1:B2', 'italic', 'add')
    lc.set("A1", "Ship:")
    lc.set("A2", "Load condition:")
    lc.set("B1", "=" + ship.Name + ".Label")
    lc.set("B2", "=Label")

    # Add the weights data
    lc.setAlignment('A4:A5', 'center', 'keep')
    lc.setStyle('A4:A5', 'bold', 'add')
    lc.setStyle('A4:A5', 'underline', 'add')
    lc.set("A4", "WEIGHTS DATA")
    lc.set("A5", "name")
    for i in range(len(ship.Weights)):
        weight = App.activeDocument().getObject(ship.Weights[i])
        lc.set("A{}".format(i + 6), "=" + weight.Name + ".Label")
    lc.setForeground('A4:A{}'.format(5 + len(ship.Weights)), READ_ONLY_FOREGROUND)
    lc.setBackground('A4:A{}'.format(5 + len(ship.Weights)), READ_ONLY_BACKGROUND)

    # Add the tanks data
    lc.mergeCells('C4:E4')
    lc.setForeground('C4:E5', READ_ONLY_FOREGROUND)
    lc.setBackground('C4:E5', READ_ONLY_BACKGROUND)
    lc.setAlignment('C4:E5', 'center', 'keep')
    lc.setStyle('C4:E5', 'bold', 'add')
    lc.setStyle('C4:E5', 'underline', 'add')
    lc.set("C4", "TANKS DATA")
    lc.set("C5", "name")
    lc.set("D5", "Fluid density [kg/m^3]")
    lc.set("E5", "Filling ratio (interval [0, 1])")
    if len(ship.Tanks):
        for i in range(len(ship.Tanks)):
            tank = App.activeDocument().getObject(ship.Tanks[i])
            lc.set("C{}".format(i + 6), "=" + tank.Name + ".Label")
            lc.set("D{}".format(i + 6), "998.0")
            lc.set("E{}".format(i + 6), "0.0")
        lc.setForeground('C6:C{}'.format(5 + len(ship.Tanks)), READ_ONLY_FOREGROUND)
        lc.setBackground('C6:C{}'.format(5 + len(ship.Tanks)), READ_ONLY_BACKGROUND)

    lc.setColumnWidth('A', 128)
    lc.setColumnWidth('B', 128)
    lc.setColumnWidth('C', 128)
    lc.setColumnWidth('D', 150)
    lc.setColumnWidth('E', 200)

    # Add the spreadsheet to the list of loading conditions of the ship
    lcs = ship.LoadConditions[:]
    lcs.append(lc.Name)
    ship.LoadConditions = lcs
    ship.Proxy.cleanLoadConditions(ship)

    # Recompute to take the changes
    App.activeDocument().recompute()

    return lc