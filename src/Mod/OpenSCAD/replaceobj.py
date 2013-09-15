#***************************************************************************
#*                                                                         *
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

__title__="FreeCAD OpenSCAD Workbench - replace object fuction"
__author__ = "Sebastian Hoogen"
__url__ = ["http://www.freecadweb.org"]

'''
This fucntions allows to replace an object in the feature hierarchy
'''

def replaceobj(parent,oldchild,newchild):
    for propname in parent.PropertiesList:
        propvalue=parent.getPropertyByName(propname)
        if type(propvalue) == list:
            for dontcare in range(propvalue.count(oldchild)):
                propvalue[propvalue.index(oldchild)] = newchild
            setattr(parent,propname,propvalue)
            #print propname, parent.getPropertyByName(propname)
        else:
            if propvalue == oldchild:
                setattr(parent,propname,newchild)
                print propname, parent.getPropertyByName(propname)
            #else: print propname,propvalue
    parent.touch()

def replaceobjfromselection(objs):
    assert(len(objs)==3)
    if objs[2] in objs[0].InList: oldchild, newchild, parent = objs
    elif objs[0] in objs[1].InList: parent, oldchild, newchild = objs
    elif objs[0] in objs[2].InList: parent, newchild, oldchild = objs
    elif objs[1] in objs[0].InList: oldchild, parent, newchild = objs
    elif objs[1] in objs[2].InList: newchild, parent, oldchild = objs
    elif objs[2] in objs[1].InList: newchild, oldchild, parent = objs
    else: assert(False)
    replaceobj(parent,oldchild,newchild)

if __name__ == '__main__':
    import FreeCAD,FreeCADGui
    objs=[selobj.Object for selobj in FreeCADGui.Selection.getSelectionEx()]
    replaceobjfromselection(objs)


