#***************************************************************************
#*   Copyright (c) 2012 Sebastian Hoogen <github@sebastianhoogen.de>       *
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

__title__ = "FreeCAD OpenSCAD Workbench - replace object function"
__author__ = "Sebastian Hoogen"
__url__ = ["https://www.freecad.org"]

'''
This functions allows to replace an object in the feature hierarchy
'''


def replaceobj(parent, oldchild, newchild):
    for propname in parent.PropertiesList:
        propvalue = parent.getPropertyByName(propname)
        if type(propvalue) == list:
            bModified = False
            for dontcare in range(propvalue.count(oldchild)):
                propvalue[propvalue.index(oldchild)] = newchild
                bModified = True
            if bModified:
                if propname == "ExpressionEngine":
                    # fixme: proper handling?
                    FreeCAD.Console.PrintWarning("Expressions in "+parent.Name+" need to be modified, but they were not. Please do that manually.")
                    continue
                setattr(parent,propname,propvalue)
        else:
            if propvalue == oldchild:
                setattr(parent,propname,newchild)
                print(propname, parent.getPropertyByName(propname))
            #else: print(propname,propvalue)
    parent.touch()

def replaceobjfromselection(objs):
    # The Parent can be omitted as long as one object is orphaned
    if len(objs) == 2:
        InListLength = tuple((len(obj.InList)) for obj in objs)
        if InListLength == (0,1):
            newchild,oldchild  = objs
            parent = oldchild.InList[0]
        elif InListLength == (1,0):
            oldchild,newchild  = objs
            parent = oldchild.InList[0]
        else:
            raise ValueError("Selection ambiguous. Please select oldchild,\
            newchild and parent")
    elif len(objs) == 3:
        if objs[2] in objs[0].InList: oldchild, newchild, parent = objs
        elif objs[0] in objs[1].InList: parent, oldchild, newchild = objs
        elif objs[0] in objs[2].InList: parent, newchild, oldchild = objs
        elif objs[1] in objs[0].InList: oldchild, parent, newchild = objs
        elif objs[1] in objs[2].InList: newchild, parent, oldchild = objs
        elif objs[2] in objs[1].InList: newchild, oldchild, parent = objs
        else:
            raise ValueError("Cannot determine current parent-child relationship")
    else:
        raise ValueError("Wrong number of selected objects")
    replaceobj(parent,oldchild,newchild)
    parent.Document.recompute()


if __name__ == '__main__':
    import FreeCAD
    import FreeCADGui
    objs = [selobj.Object for selobj in FreeCADGui.Selection.getSelectionEx()]
    replaceobjfromselection(objs)
