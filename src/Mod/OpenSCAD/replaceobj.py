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


