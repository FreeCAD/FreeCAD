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

import FreeCAD as App
import FreeCADGui as Gui
import Spreadsheet


def load():
    """Directly create the load condition"""
    # Check that a ship has been selected
    ship = None
    selObjs = Gui.Selection.getSelection()
    if not selObjs:
        msg = QtGui.QApplication.translate(
            "ship_console",
            "A ship instance must be selected before using this tool (no"
            " objects selected)",
            None,
            QtGui.QApplication.UnicodeUTF8)
        App.Console.PrintError(msg + '\n')
        return
    for i in range(len(selObjs)):
        obj = selObjs[i]
        props = obj.PropertiesList
        try:
            props.index("IsShip")
        except ValueError:
            continue
        if obj.IsShip:
            if ship:
                msg = QtGui.QApplication.translate(
                    "ship_console",
                    "More than one ship have been selected (the extra"
                    " ships will be ignored)",
                    None,
                    QtGui.QApplication.UnicodeUTF8)
                App.Console.PrintWarning(msg + '\n')
                break
            ship = obj

    if not ship:
        msg = QtGui.QApplication.translate(
            "ship_console",
            "A ship instance must be selected before using this tool (no"
            " valid ship found at the selected objects)",
            None,
            QtGui.QApplication.UnicodeUTF8)
        App.Console.PrintError(msg + '\n')
        return

    # Create the spreadsheet
    s = App.activeDocument().addObject('Spreadsheet::Sheet',
                                       'LoadCondition')

    # Add a reference to the owner ship
    s.mergeCells('A1:D1')
    s.setAlignment('A1:A1', 'center', 'keep')
    s.setStyle('A1:A1', 'bold', 'add')
    s.setStyle('A1:A1', 'underline', 'add')
    s.set("A1", "SHIP DATA")
    s.set("A2", "ship")
    s.set("A3", ship.Label)
    s.set("B2", "internal ref")
    s.set("B3", ship.Name)
    s.setForeground('A1:B3', (0.5,0.5,0.5))

    # Add the weights data
    s.mergeCells('A4:D4')
    s.setAlignment('A4:A4', 'center', 'keep')
    s.setStyle('A4:A4', 'bold', 'add')
    s.setStyle('A4:A4', 'underline', 'add')
    s.set("A4", "WEIGHTS DATA")
    s.set("A5", "weight")
    s.set("B5", "internal ref")
    for i in range(len(ship.Weights)):
        weight = App.activeDocument().getObject(ship.Weights[i])
        s.set("A{}".format(i + 6), weight.Label)
        s.set("B{}".format(i + 6), weight.Name)
    s.setForeground('A4:B{}'.format(5 + len(ship.Weights)), (0.5,0.5,0.5))

    # Add the tanks data
    s.mergeCells('A{0}:D{0}'.format(6 + len(ship.Weights)))
    s.setAlignment('A{0}:A{0}'.format(6 + len(ship.Weights)), 'center', 'keep')
    s.setStyle('A{0}:A{0}'.format(6 + len(ship.Weights)), 'bold', 'add')
    s.setStyle('A{0}:A{0}'.format(6 + len(ship.Weights)), 'underline', 'add')
    s.set("A{}".format(6 + len(ship.Weights)), "TANKS DATA")
    s.set("A{}".format(7 + len(ship.Weights)), "tank")
    s.set("B{}".format(7 + len(ship.Weights)), "internal ref")
    s.set("C{}".format(7 + len(ship.Weights)), "Fluid density [kg/m^3]")
    s.set("D{}".format(7 + len(ship.Weights)), "Filling ratio (interval [0.0,1.0])")
    for i in range(len(ship.Tanks)):
        tank = App.activeDocument().getObject(ship.Tanks[i])
        s.set("A{}".format(i + 8 + len(ship.Weights)), tank.Label)
        s.set("B{}".format(i + 8 + len(ship.Weights)), tank.Name)
        s.set("C{}".format(i + 8 + len(ship.Weights)), "998.0")
        s.set("D{}".format(i + 8 + len(ship.Weights)), "0.0")
    s.setForeground('A{0}:A{0}'.format(6 + len(ship.Weights)), (0.5,0.5,0.5))
    s.setForeground('A{0}:D{0}'.format(7 + len(ship.Weights)), (0.5,0.5,0.5))
    s.setForeground('A{}:B{}'.format(8 + len(ship.Weights),
                                     8 + len(ship.Weights) + len(ship.Tanks)),
                    (0.5,0.5,0.5))

    # Add the spreadsheet to the list of loading conditions of the ship
    lcs = ship.LoadConditions[:]
    lcs.append(s.Name)
    ship.LoadConditions = lcs

    # Recompute to take the changes
    App.activeDocument().recompute()