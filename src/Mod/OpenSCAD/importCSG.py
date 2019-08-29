# -*- coding: utf8 -*-

#***************************************************************************
#*                                                                         *
#*   Copyright (c) 2012 Keith Sloan <keith@sloan-home.co.uk>               *
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
#*   Acknowledgements :                                                    *
#*                                                                         *
#*     Thanks to shoogen on the FreeCAD forum and Peter Li                 *
#*     for programming advice and some code.                               *
#*                                                                         *
#*                                                                         *
#***************************************************************************
__title__="FreeCAD OpenSCAD Workbench - CSG importer"
__author__ = "Keith Sloan <keith@sloan-home.co.uk>"
__url__ = ["http://www.sloan-home.co.uk/ImportCSG"]

printverbose = False

import FreeCAD, io, os, sys
if FreeCAD.GuiUp:
    import FreeCADGui
    gui = True
else:
    if printverbose: print("FreeCAD Gui not present.")
    gui = False


import ply.lex as lex
import ply.yacc as yacc
import Part

from OpenSCADFeatures import *
from OpenSCADUtils import *

params = FreeCAD.ParamGet("User parameter:BaseApp/Preferences/Mod/OpenSCAD")
printverbose = params.GetBool('printVerbose',False)

# Get the token map from the lexer.  This is required.
import tokrules
from tokrules import tokens

try:
    _encoding = QtGui.QApplication.UnicodeUTF8
    def translate(context, text):
        "convenience function for Qt translator"
        from PySide import QtGui
        return QtGui.QApplication.translate(context, text, None, _encoding)
except AttributeError:
    def translate(context, text):
        "convenience function for Qt translator"
        from PySide import QtGui
        return QtGui.QApplication.translate(context, text, None)

def open(filename):
    "called when freecad opens a file."
    global doc
    global pathName
    docname = os.path.splitext(os.path.basename(filename))[0]
    doc = FreeCAD.newDocument(docname)
    if filename.lower().endswith('.scad'):
        tmpfile=callopenscad(filename)
        if workaroundforissue128needed():
            pathName = '' #https://github.com/openscad/openscad/issues/128
            #pathName = os.getcwd() #https://github.com/openscad/openscad/issues/128
        else:
            pathName = os.path.dirname(os.path.normpath(filename))
        processcsg(tmpfile)
        try:
            os.unlink(tmpfile)
        except OSError:
            pass
    else:
        pathName = os.path.dirname(os.path.normpath(filename))
        processcsg(filename)
    return doc

def insert(filename,docname):
    "called when freecad imports a file"
    global doc
    global pathName
    groupname = os.path.splitext(os.path.basename(filename))[0]
    try:
        doc=FreeCAD.getDocument(docname)
    except NameError:
        doc=FreeCAD.newDocument(docname)
    #importgroup = doc.addObject("App::DocumentObjectGroup",groupname)
    if filename.lower().endswith('.scad'):
        tmpfile=callopenscad(filename)
        if workaroundforissue128needed():
            pathName = '' #https://github.com/openscad/openscad/issues/128
            #pathName = os.getcwd() #https://github.com/openscad/openscad/issues/128
        else:
            pathName = os.path.dirname(os.path.normpath(filename))
        processcsg(tmpfile)
        try:
            os.unlink(tmpfile)
        except OSError:
            pass
    else:
        pathName = os.path.dirname(os.path.normpath(filename))
        processcsg(filename)

def processcsg(filename):
    global doc

    if printverbose: print ('ImportCSG Version 0.6a')
    # Build the lexer
    if printverbose: print('Start Lex')
    lex.lex(module=tokrules)
    if printverbose: print('End Lex')

    # Build the parser   
    if printverbose: print('Load Parser')
    # No debug out otherwise Linux has protection exception
    parser = yacc.yacc(debug=0)
    if printverbose: print('Parser Loaded')
    # Give the lexer some input
    #f=open('test.scad', 'r')
    f = io.open(filename, 'r', encoding="utf8")
    #lexer.input(f.read())

    if printverbose: print('Start Parser')
    # Swap statements to enable Parser debugging
    #result = parser.parse(f.read(),debug=1)
    result = parser.parse(f.read())
    f.close()
    if printverbose:
        print('End Parser')
        print(result)
    FreeCAD.Console.PrintMessage('End processing CSG file\n')
    doc.recompute()

def p_block_list_(p):
    '''
    block_list : statement
               | block_list statement
               | statementwithmod
               | block_list statementwithmod
    '''
    #if printverbose: print("Block List")
    #if printverbose: print(p[1])
    if(len(p) > 2) :
        if printverbose: print(p[2])
        p[0] = p[1] + p[2]
    else :
        p[0] = p[1]
    #if printverbose: print("End Block List")

def p_render_action(p):
    'render_action : render LPAREN keywordargument_list RPAREN OBRACE block_list EBRACE'
    if printverbose: print("Render (ignored)")
    p[0] = p[6]

def p_group_action1(p):
    'group_action1 : group LPAREN RPAREN OBRACE block_list EBRACE'
    if printverbose: print("Group")
# Test if need for implicit fuse
    if (len(p[5]) > 1) :
        p[0] = [fuse(p[5],"Group")]
    else :
        p[0] = p[5]

def p_group_action2(p) :
    'group_action2 : group LPAREN RPAREN SEMICOL'
    if printverbose: print("Group2")
    p[0] = []
   
def p_boolean(p) :
    '''
    boolean : true
            | false
    '''
    p[0] = p[1]

#def p_string(p):
#    'string : QUOTE ID QUOTE'
#    p[0] = p[2]

def p_stripped_string(p):
    'stripped_string : STRING'
    p[0] = p[1].strip('"')

def p_statement(p):
    '''statement : part
                 | operation
                 | multmatrix_action
                 | group_action1
                 | group_action2
                 | color_action
                 | render_action
                 | not_supported
    '''
    p[0] = p[1]

def p_anymodifier(p):
    '''anymodifier : MODIFIERBACK
                   | MODIFIERDEBUG
                   | MODIFIERROOT
                   | MODIFIERDISABLE
    '''
    #just return the plain modifier for now
    #has to be changed when the modifiers are implemented
    #please note that disabled objects usually are stripped of the CSG output during compilation
    p[0] = p[1]

def p_statementwithmod(p):
    '''statementwithmod : anymodifier statement'''
    #ignore the modifiers but add them to the label
    modifier = p[1]
    obj = p[2]
    if hasattr(obj,'Label'):
        obj.Label = modifier + obj.Label
    p[0] = obj

def p_part(p):
    '''
    part : sphere_action
         | cylinder_action
         | cube_action
         | circle_action
         | square_action
         | text_action
         | polygon_action_nopath
         | polygon_action_plus_path
         | polyhedron_action
         '''
    p[0] = p[1]

def p_2d_point(p):
    '2d_point : OSQUARE NUMBER COMMA NUMBER ESQUARE'
    global points_list
    if printverbose: print("2d Point")
    p[0] = [float(p[2]),float(p[4])]

def p_points_list_2d(p):
    '''
    points_list_2d : 2d_point COMMA
                   | points_list_2d 2d_point COMMA
                   | points_list_2d 2d_point
                   '''
    if p[2] == ',' :
        #if printverbose:
        #    print("Start List")
        #    print(p[1])
        p[0] = [p[1]]
    else :
        if printverbose:
            print(p[1])
            print(p[2])
        p[1].append(p[2])
        p[0] = p[1]
    #if printverbose: print(p[0])

def p_3d_point(p):
    '3d_point : OSQUARE NUMBER COMMA NUMBER COMMA NUMBER ESQUARE'
    global points_list
    if printverbose: print("3d point")
    p[0] = [p[2],p[4],p[6]]
   
def p_points_list_3d(p):
    '''
    points_list_3d : 3d_point COMMA
               | points_list_3d 3d_point COMMA
               | points_list_3d 3d_point
               '''
    if p[2] == ',' :
        if printverbose: print("Start List")
        if printverbose: print(p[1])
        p[0] = [p[1]]
    else :
        if printverbose: print(p[1])
        if printverbose: print(p[2])
        p[1].append(p[2])
        p[0] = p[1]
    if printverbose: print(p[0])

def p_path_points(p):
    '''
    path_points : NUMBER COMMA
                | path_points NUMBER COMMA
                | path_points NUMBER
                '''
    #if printverbose: print("Path point")
    if p[2] == ',' :
        #if printverbose: print('Start list')
        #if printverbose: print(p[1])
        p[0] = [int(p[1])]
    else :
        #if printverbose: print(p[1])
        #if printverbose: print(len(p[1]))
        #if printverbose: print(p[2])
        p[1].append(int(p[2]))
        p[0] = p[1]
    #if printverbose: print(p[0])


def p_path_list(p):
    'path_list : OSQUARE path_points ESQUARE'
    #if printverbose: print('Path List ')
    #if printverbose: print(p[2])
    p[0] = p[2]

def p_path_set(p) :
    '''
    path_set : path_list
             | path_set COMMA path_list
             '''
    #if printverbose: print('Path Set')
    #if printverbose: print(len(p))
    if len(p) == 2 :
        p[0] = [p[1]]
    else :
        p[1].append(p[3])
        p[0] = p[1]
    #if printverbose: print(p[0])

def p_operation(p):
    '''
    operation : difference_action
              | intersection_action
              | union_action
              | rotate_extrude_action
              | linear_extrude_with_twist
              | rotate_extrude_file
              | import_file1
              | surface_action
              | projection_action
              | hull_action
              | minkowski_action
              | offset_action
              '''
    p[0] = p[1]

def placeholder(name,children,arguments):
    from OpenSCADFeatures import OpenSCADPlaceholder
    newobj=doc.addObject("Part::FeaturePython",name)
    OpenSCADPlaceholder(newobj,children,str(arguments))
    if gui:
        if FreeCAD.ParamGet("User parameter:BaseApp/Preferences/Mod/OpenSCAD").\
            GetBool('useViewProviderTree'):
            from OpenSCADFeatures import ViewProviderTree
            ViewProviderTree(newobj.ViewObject)
        else:
            newobj.ViewObject.Proxy = 0
    #don't hide the children
    return newobj

def CGALFeatureObj(name,children,arguments=[]):
    myobj=doc.addObject("Part::FeaturePython",name)
    CGALFeature(myobj,name,children,str(arguments))
    if gui:
        for subobj in children:
            subobj.ViewObject.hide()
        if FreeCAD.ParamGet("User parameter:BaseApp/Preferences/Mod/OpenSCAD").\
            GetBool('useViewProviderTree'):
            from OpenSCADFeatures import ViewProviderTree
            ViewProviderTree(myobj.ViewObject)
        else:
            myobj.ViewObject.Proxy = 0
    return myobj

def p_offset_action(p):
    'offset_action : offset LPAREN keywordargument_list RPAREN OBRACE block_list EBRACE'
    if len(p[6]) == 0:
        newobj = placeholder('group',[],'{}')
    elif (len(p[6]) == 1 ): #single object
        subobj = p[6]
    else:
        subobj = fuse(p[6],"Offset Union")
    if 'r' in p[3] :
        offset = float(p[3]['r'])
    if 'delta' in p[3] : 
        offset = float(p[3]['delta'])
    if subobj[0].Shape.Volume == 0 :
       newobj=doc.addObject("Part::Offset2D",'Offset2D')
       newobj.Source = subobj[0] 
       newobj.Value = offset
       if 'r' in p[3] :
           newobj.Join = 0 
       else :
           newobj.Join = 2 
    else :
       newobj=doc.addObject("Part::Offset",'offset')
       newobj.Shape = subobj[0].Shape.makeOffset(offset)
    newobj.Document.recompute()
    if gui:
        subobj[0].ViewObject.hide()
#        if FreeCAD.ParamGet("User parameter:BaseApp/Preferences/Mod/OpenSCAD").\
#            GetBool('useViewProviderTree'):
#            from OpenSCADFeatures import ViewProviderTree
#            ViewProviderTree(newobj.ViewObject)
#        else:
#            newobj.ViewObject.Proxy = 0
    p[0] = [newobj]

def p_hull_action(p):
    'hull_action : hull LPAREN RPAREN OBRACE block_list EBRACE'
    p[0] = [ CGALFeatureObj(p[1],p[5]) ]

def p_minkowski_action(p):
    '''
    minkowski_action : minkowski LPAREN keywordargument_list RPAREN OBRACE block_list EBRACE'''
    p[0] = [ CGALFeatureObj(p[1],p[6],p[3]) ]

def p_not_supported(p):
    '''
    not_supported : glide LPAREN keywordargument_list RPAREN OBRACE block_list EBRACE
                  | resize LPAREN keywordargument_list RPAREN OBRACE block_list EBRACE
                  | subdiv LPAREN keywordargument_list RPAREN OBRACE block_list EBRACE
                  '''
    if gui and not FreeCAD.ParamGet("User parameter:BaseApp/Preferences/Mod/OpenSCAD").\
            GetBool('usePlaceholderForUnsupported'):
        from PySide import QtGui
        QtGui.QMessageBox.critical(None, translate('OpenSCAD',"Unsupported Function")+" : "+p[1],translate('OpenSCAD',"Press OK"))
    else:
        p[0] = [placeholder(p[1],p[6],p[3])]

def p_size_vector(p):
    'size_vector : OSQUARE NUMBER COMMA NUMBER COMMA NUMBER ESQUARE'
    if printverbose: print("size vector")
    p[0] = [p[2],p[4],p[6]]

def p_keywordargument(p):
    '''keywordargument : ID EQ boolean
    | ID EQ NUMBER
    | ID EQ size_vector
    | ID EQ vector
    | ID EQ 2d_point
    | text EQ stripped_string
    | ID EQ stripped_string
     '''
    p[0] = (p[1],p[3])
    if printverbose: print(p[0])

def p_keywordargument_list(p):
    '''
    keywordargument_list : keywordargument
               | keywordargument_list COMMA keywordargument
    '''
    if len(p) == 2:
        p[0] = {p[1][0] : p[1][1]}
    else:
        p[1][p[3][0]] = p[3][1]
        p[0]=p[1]

def p_color_action(p):
    'color_action : color LPAREN vector RPAREN OBRACE block_list EBRACE'
    import math
    if printverbose: print("Color")
    color = tuple([float(f) for f in p[3][:3]]) #RGB
    transp = 100 - int(math.floor(100*float(p[3][3]))) #Alpha
    if gui:
        for obj in p[6]:
            obj.ViewObject.ShapeColor =color
            obj.ViewObject.Transparency = transp
    p[0] = p[6]

# Error rule for syntax errors
def p_error(p):
    if printverbose: print("Syntax error in input!")
    if printverbose: print(p)

def fuse(lst,name):
    global doc
    if printverbose: print("Fuse")
    if printverbose: print(lst)
    if len(lst) == 0:
        myfuse = placeholder('group',[],'{}')
    elif len(lst) == 1:
       return lst[0]
    # Is this Multi Fuse
    elif len(lst) > 2:
       if printverbose: print("Multi Fuse")
       myfuse = doc.addObject('Part::MultiFuse',name)
       myfuse.Shapes = lst
       if gui:
           for subobj in myfuse.Shapes:
               subobj.ViewObject.hide()
    else:
       if printverbose: print("Single Fuse")
       myfuse = doc.addObject('Part::Fuse',name)
       myfuse.Base = lst[0]
       myfuse.Tool = lst[1]
       if gui:
           myfuse.Base.ViewObject.hide()
           myfuse.Tool.ViewObject.hide()
    return(myfuse)

def p_union_action(p):
    'union_action : union LPAREN RPAREN OBRACE block_list EBRACE'
    if printverbose: print("union")
    newpart = fuse(p[5],p[1])
    if printverbose: print("Push Union Result")
    p[0] = [newpart]
    if printverbose: print("End Union")
    
def p_difference_action(p):  
    'difference_action : difference LPAREN RPAREN OBRACE block_list EBRACE'

    if printverbose: print("difference")
    if printverbose: print(len(p[5]))
    if printverbose: print(p[5])
    if (len(p[5]) == 0 ): #nochild
        mycut = placeholder('group',[],'{}')
    elif (len(p[5]) == 1 ): #single object
        p[0] = p[5]
    else:
# Cut using Fuse    
        mycut = doc.addObject('Part::Cut',p[1])
        mycut.Base = p[5][0]
#       Can only Cut two objects do we need to fuse extras
        if (len(p[5]) > 2 ):
           if printverbose: print("Need to Fuse Extra First")
           mycut.Tool = fuse(p[5][1:],'union')
        else :
           mycut.Tool = p[5][1]
        if gui:
            mycut.Base.ViewObject.hide()
            mycut.Tool.ViewObject.hide()
        if printverbose: print("Push Resulting Cut")
        p[0] = [mycut]
    if printverbose: print("End Cut")

def p_intersection_action(p):
    'intersection_action : intersection LPAREN RPAREN OBRACE block_list EBRACE'

    if printverbose: print("intersection")
    # Is this Multi Common
    if (len(p[5]) > 2):
       if printverbose: print("Multi Common")
       mycommon = doc.addObject('Part::MultiCommon',p[1])
       mycommon.Shapes = p[5]
       if gui:
           for subobj in mycommon.Shapes:
               subobj.ViewObject.hide()
    elif (len(p[5]) == 2):
       if printverbose: print("Single Common")
       mycommon = doc.addObject('Part::Common',p[1])
       mycommon.Base = p[5][0]
       mycommon.Tool = p[5][1]
       if gui:
           mycommon.Base.ViewObject.hide()
           mycommon.Tool.ViewObject.hide()
    elif (len(p[5]) == 1):
        mycommon = p[5][0]
    else : # 1 child
        mycommon = placeholder('group',[],'{}')
    p[0] = [mycommon]
    if printverbose: print("End Intersection")

def process_rotate_extrude(obj):
    newobj=doc.addObject("Part::FeaturePython",'RefineRotateExtrude')
    RefineShape(newobj,obj)
    if gui:
        if FreeCAD.ParamGet("User parameter:BaseApp/Preferences/Mod/OpenSCAD").\
            GetBool('useViewProviderTree'):
            from OpenSCADFeatures import ViewProviderTree
            ViewProviderTree(newobj.ViewObject)
        else:
            newobj.ViewObject.Proxy = 0
        obj.ViewObject.hide()
    myrev = doc.addObject("Part::Revolution","RotateExtrude")
    myrev.Source = newobj
    myrev.Axis = (0.00,1.00,0.00)
    myrev.Base = (0.00,0.00,0.00)
    myrev.Angle = 360.00
    myrev.Placement=FreeCAD.Placement(FreeCAD.Vector(),FreeCAD.Rotation(0,0,90))
    if gui:
        newobj.ViewObject.hide()
    return(myrev)

def p_rotate_extrude_action(p): 
    'rotate_extrude_action : rotate_extrude LPAREN keywordargument_list RPAREN OBRACE block_list EBRACE'
    if printverbose: print("Rotate Extrude")
    if (len(p[6]) > 1) :
        part = fuse(p[6],"Rotate Extrude Union")
    else :
        part = p[6][0]
    p[0] = [process_rotate_extrude(part)]
    if printverbose: print("End Rotate Extrude")

def p_rotate_extrude_file(p):
    'rotate_extrude_file : rotate_extrude LPAREN keywordargument_list RPAREN SEMICOL'
    if printverbose: print("Rotate Extrude File")
    filen,ext =p[3]['file'] .rsplit('.',1)
    obj = process_import_file(filen,ext,p[3]['layer'])
    p[0] = [process_rotate_extrude(obj)]
    if printverbose: print("End Rotate Extrude File")

def process_linear_extrude(obj,h) :
    #if gui:
    newobj=doc.addObject("Part::FeaturePython",'RefineLinearExtrude')
    RefineShape(newobj,obj)#mylinear)
    if gui:
        if FreeCAD.ParamGet("User parameter:BaseApp/Preferences/Mod/OpenSCAD").\
            GetBool('useViewProviderTree'):
            from OpenSCADFeatures import ViewProviderTree
            ViewProviderTree(newobj.ViewObject)
        else:
            newobj.ViewObject.Proxy = 0
        obj.ViewObject.hide()
        #mylinear.ViewObject.hide()
    mylinear = doc.addObject("Part::Extrusion","LinearExtrude")
    mylinear.Base = newobj #obj
    mylinear.Dir = (0,0,h)
    mylinear.Placement=FreeCAD.Placement()
    # V17 change to False mylinear.Solid = True
    mylinear.Solid = False
    if gui:
        newobj.ViewObject.hide()
    return(mylinear)

def process_linear_extrude_with_twist(base,height,twist) :   
    newobj=doc.addObject("Part::FeaturePython",'twist_extrude')
    Twist(newobj,base,height,-twist) #base is an FreeCAD Object, height and twist are floats
    if gui:
        if FreeCAD.ParamGet("User parameter:BaseApp/Preferences/Mod/OpenSCAD").\
            GetBool('useViewProviderTree'):
            from OpenSCADFeatures import ViewProviderTree
            ViewProviderTree(newobj.ViewObject)
        else:
            newobj.ViewObject.Proxy = 0
    #import ViewProviderTree from OpenSCADFeatures
    #ViewProviderTree(obj.ViewObject)
    return(newobj)

def p_linear_extrude_with_twist(p):
    'linear_extrude_with_twist : linear_extrude LPAREN keywordargument_list RPAREN OBRACE block_list EBRACE'
    if printverbose: print("Linear Extrude With Twist")
    h = float(p[3]['height'])
    if printverbose: print("Twist : ",p[3])
    if 'twist' in p[3]:
        t = float(p[3]['twist'])
    else:
        t = 0
    # Test if null object like from null text
    if (len(p[6]) == 0) :
        p[0] = []
        return
    if (len(p[6]) > 1) :
        obj = fuse(p[6],"Linear Extrude Union")
    else :
        obj = p[6][0]
    if t:
        newobj = process_linear_extrude_with_twist(obj,h,t)
    else:
        newobj = process_linear_extrude(obj,h)
    if p[3]['center']=='true' :
       center(newobj,0,0,h)
    p[0] = [newobj]
    if printverbose: print("End Linear Extrude with twist")

def p_import_file1(p):
    'import_file1 : import LPAREN keywordargument_list RPAREN SEMICOL'
    if printverbose: print("Import File")
    filen,ext =p[3]['file'].rsplit('.',1)
    p[0] = [process_import_file(filen,ext,p[3]['layer'])]
    if printverbose: print("End Import File")

def p_surface_action(p):
    'surface_action : surface LPAREN keywordargument_list RPAREN SEMICOL'
    if printverbose: print("Surface")
    obj = doc.addObject("Part::Feature",'surface')
    obj.Shape,xoff,yoff=makeSurfaceVolume(p[3]['file'])
    if p[3]['center']=='true' :
        center(obj,xoff,yoff,0.0)
    p[0] = [obj]
    if printverbose: print("End surface")

def process_import_file(fname,ext,layer):
    if printverbose: print("Importing : "+fname+"."+ext+" Layer : "+layer)
    if ext.lower() in reverseimporttypes()['Mesh']:
        obj=process_mesh_file(fname,ext)
    elif ext.lower() == 'dxf' :
        obj=processDXF(fname,layer)
    else:
        raise ValueError("Unsupported file extension %s" % ext)
    return(obj)

def process_mesh_file(fname,ext):
    import Mesh,Part
    fullname = fname+'.'+ext
    filename = os.path.join(pathName,fullname)
    objname = os.path.split(fname)[1]
    mesh1 = doc.getObject(objname) #reuse imported object
    if not mesh1:
        Mesh.insert(filename)
        mesh1=doc.getObject(objname)
    if mesh1 is not None:
        if gui:
            mesh1.ViewObject.hide()
        sh=Part.Shape()
        sh.makeShapeFromMesh(mesh1.Mesh.Topology,0.1)
        solid = Part.Solid(sh)
        obj=doc.addObject('Part::Feature',"Mesh")
        #ImportObject(obj,mesh1) #This object is not mutable from the GUI
        #ViewProviderTree(obj.ViewObject)
        solid=solid.removeSplitter()
        if solid.Volume < 0:
            #sh.reverse()
            #sh = sh.copy()
            solid.complement()
        obj.Shape=solid#.removeSplitter()
    else: #mesh1 is None
        FreeCAD.Console.PrintError('Mesh not imported %s.%s %s\n' % \
                (objname,ext,filename))
        import Part
        obj=doc.addObject('Part::Feature',"FailedMeshImport")
        obj.Shape=Part.Compound([])
    return(obj)


def processTextCmd(t):
    import os
    from OpenSCADUtils import callopenscadstring
    tmpfilename = callopenscadstring(t,'dxf')
    from OpenSCAD2Dgeom import importDXFface 
    face = importDXFface(tmpfilename,None,None)
    obj=doc.addObject('Part::Feature','text')
    obj.Shape=face
    try:
        os.unlink(tmpfilename)
    except OSError:
        pass
    return(obj)

def processDXF(fname,layer):
    global doc
    global pathName
    from OpenSCAD2Dgeom import importDXFface
    if printverbose: print("Process DXF file")
    if printverbose: print("File Name : "+fname)
    if printverbose: print("Layer : "+layer)
    if printverbose: print("PathName : "+pathName)
    dxfname = fname+'.dxf'
    filename = os.path.join(pathName,dxfname)
    shortname = os.path.split(fname)[1]
    if printverbose: print("DXF Full path : "+filename)
    face = importDXFface(filename,layer,doc)
    obj=doc.addObject('Part::Feature','dxf_%s_%s' % (shortname,layer or "all"))
    obj.Shape=face
    if printverbose: print("DXF Diagnostics")
    if printverbose: print(obj.Shape.ShapeType)
    if printverbose: print("Closed : "+str(obj.Shape.isClosed()))
    if printverbose: print(obj.Shape.check())
    if printverbose: print([w.isClosed() for w in obj.Shape.Wires])
    return(obj)

def processSTL(fname):
    if printverbose: print("Process STL file")

def p_multmatrix_action(p):
    'multmatrix_action : multmatrix LPAREN matrix RPAREN OBRACE block_list EBRACE'
    if printverbose: print("MultMatrix")
    transform_matrix = FreeCAD.Matrix()
    if printverbose: print("Multmatrix")
    if printverbose: print(p[3])
    m1l=sum(p[3],[])
    if any('x' in me for me in m1l): #hexfloats
        m1l=[float.fromhex(me) for me in m1l]
        matrixisrounded=False
    elif max((len(me) for me in m1l)) >= 14: #might have double precision
        m1l=[float(me) for me in m1l] # assume precise output
        m1l=[(0 if (abs(me) < 1e-15) else me) for me in m1l]
        matrixisrounded=False
    else: #trucanted numbers
        m1l=[round(float(me),12) for me in m1l] #round
        matrixisrounded=True
    transform_matrix = FreeCAD.Matrix(*tuple(m1l))
    if printverbose: print(transform_matrix)
    if printverbose: print("Apply Multmatrix")
#   If more than one object on the stack for multmatrix fuse first
    if (len(p[6]) == 0) :
        part = placeholder('group',[],'{}')
    elif (len(p[6]) > 1) :
        part = fuse(p[6],"Matrix Union")
    else :
        part = p[6][0]
    if (isspecialorthogonalpython(fcsubmatrix(transform_matrix))) :
        if printverbose: print("special orthogonal")
        if matrixisrounded:
            if printverbose: print("rotation rounded")
            plm=FreeCAD.Placement(transform_matrix)
            plm=FreeCAD.Placement(plm.Base,roundrotation(plm.Rotation))
            part.Placement=plm.multiply(part.Placement)
        else:
            part.Placement=FreeCAD.Placement(transform_matrix).multiply(\
                    part.Placement)
        new_part = part
    elif isrotoinversionpython(fcsubmatrix(transform_matrix)):
        if printverbose: print("orthogonal and inversion")
        cmat,axisvec = decomposerotoinversion(transform_matrix)
        new_part=doc.addObject("Part::Mirroring",'mirr_%s'%part.Name)
        new_part.Source=part
        new_part.Normal=axisvec
        if matrixisrounded:
            if printverbose: print("rotation rounded")
            plm=FreeCAD.Placement(cmat)
            new_part.Placement=FreeCAD.Placement(plm.Base,roundrotation(plm.Rotation))
        else:
            new_part.Placement=FreeCAD.Placement(cmat)
        new_part.Label="mirrored %s" % part.Label
        if gui:
            part.ViewObject.hide()
    elif FreeCAD.ParamGet("User parameter:BaseApp/Preferences/Mod/OpenSCAD").\
        GetBool('useMultmatrixFeature'):
        from OpenSCADFeatures import MatrixTransform
        new_part=doc.addObject("Part::FeaturePython",'Matrix Deformation')
        MatrixTransform(new_part,transform_matrix,part)
        if gui:
            if FreeCAD.ParamGet("User parameter:BaseApp/Preferences/Mod/OpenSCAD").\
                GetBool('useViewProviderTree'):
                from OpenSCADFeatures import ViewProviderTree
                ViewProviderTree(new_part.ViewObject)
            else:
                new_part.ViewObject.Proxy = 0
            part.ViewObject.hide()
    else :
        if printverbose: print("Transform Geometry")
#       Need to recompute to stop transformGeometry causing a crash        
        doc.recompute()
        new_part = doc.addObject("Part::Feature","Matrix Deformation")
      #  new_part.Shape = part.Base.Shape.transformGeometry(transform_matrix)
        new_part.Shape = part.Shape.transformGeometry(transform_matrix) 
        if gui:
            part.ViewObject.hide()
    if False :  
#   Does not fix problemfile or beltTighener although later is closer       
        newobj=doc.addObject("Part::FeaturePython",'RefineMultMatrix')
        RefineShape(newobj,new_part)
        if gui:
            newobj.ViewObject.Proxy = 0
            new_part.ViewObject.hide()   
        p[0] = [newobj]
    else :
        p[0] = [new_part]
    if printverbose: print("Multmatrix applied")
    
def p_matrix(p):
    'matrix : OSQUARE vector COMMA vector COMMA vector COMMA vector ESQUARE'
    if printverbose: print("Matrix")
    p[0] = [p[2],p[4],p[6],p[8]]

def p_vector(p):
    'vector : OSQUARE NUMBER COMMA NUMBER COMMA NUMBER COMMA NUMBER ESQUARE'
    if printverbose: print("Vector")
    p[0] = [p[2],p[4],p[6],p[8]]

def center(obj,x,y,z):
    obj.Placement = FreeCAD.Placement(\
        FreeCAD.Vector(-x/2.0,-y/2.0,-z/2.0),\
        FreeCAD.Rotation(0,0,0,1))
    
def p_sphere_action(p):
    'sphere_action : sphere LPAREN keywordargument_list RPAREN SEMICOL'
    if printverbose: print("Sphere : ",p[3])
    r = float(p[3]['r'])
    mysphere = doc.addObject("Part::Sphere",p[1])
    mysphere.Radius = r
    if printverbose: print("Push Sphere")
    p[0] = [mysphere]
    if printverbose: print("End Sphere")

def myPolygon(n,r1):
    # Adapted from Draft::_Polygon
    import math
    if printverbose: print("My Polygon")
    angle = math.pi*2/n
    nodes = [FreeCAD.Vector(r1,0,0)]
    for i in range(n-1) :
        th = (i+1) * angle
        nodes.append(FreeCAD.Vector(r1*math.cos(th),r1*math.sin(th),0))
    nodes.append(nodes[0])
    polygonwire = Part.makePolygon(nodes)

    polygon = doc.addObject("Part::Feature","Polygon")
    polygon.Shape = Part.Face(polygonwire)
    return(polygon)

def p_cylinder_action(p):
    'cylinder_action : cylinder LPAREN keywordargument_list RPAREN SEMICOL'
    if printverbose: print("Cylinder")
    tocenter = p[3]['center']
    h = float(p[3]['h'])
    r1 = float(p[3]['r1'])
    r2 = float(p[3]['r2'])
    #n = int(p[3]['$fn'])
    n = int(round(float(p[3]['$fn'])))
    fnmax = FreeCAD.ParamGet(\
        "User parameter:BaseApp/Preferences/Mod/OpenSCAD").\
        GetInt('useMaxFN')
    if printverbose: print(p[3])
    if h > 0:
        if ( r1 == r2 and r1 > 0):
            if printverbose: print("Make Cylinder")
            if n < 3 or fnmax != 0 and n > fnmax:
                mycyl=doc.addObject("Part::Cylinder",p[1])
                mycyl.Height = h
                mycyl.Radius = r1
            else :
                if printverbose: print("Make Prism")
                if False: #user Draft Polygon
                    mycyl=doc.addObject("Part::Extrusion","prism")
                    mycyl.Dir = (0,0,h)
                    try :
                        import Draft
                        mycyl.Base = Draft.makePolygon(n,r1,face=True)
                    except :
                        # If Draft can't import (probably due to lack of Pivy on Mac and
                        # Linux builds of FreeCAD), this is a fallback.
                        # or old level of FreeCAD
                        if printverbose: print("Draft makePolygon Failed, falling back on manual polygon")
                        mycyl.Base = myPolygon(n,r1)
                        # mycyl.Solid = True

                    else :
                        pass
                    if gui:
                        mycyl.Base.ViewObject.hide()
                else: #Use Part::Prism primitive
                    mycyl=doc.addObject("Part::Prism","prism")
                    mycyl.Polygon = n
                    mycyl.Circumradius  = r1
                    mycyl.Height  = h

        elif (r1 != r2):
            if n < 3 or fnmax != 0 and n > fnmax:
                if printverbose: print("Make Cone")
                mycyl=doc.addObject("Part::Cone",p[1])
                mycyl.Height = h
                mycyl.Radius1 = r1
                mycyl.Radius2 = r2
            else:
                if printverbose: print("Make Frustum")
                mycyl=doc.addObject("Part::FeaturePython",'frustum')
                Frustum(mycyl,r1,r2,n,h)
                if gui:
                    if FreeCAD.ParamGet("User parameter:BaseApp/Preferences/Mod/OpenSCAD").\
                        GetBool('useViewProviderTree'):
                        from OpenSCADFeatures import ViewProviderTree
                        ViewProviderTree(mycyl.ViewObject)
                    else:
                        mycyl.ViewObject.Proxy = 0
        else: # r1 == r2 == 0
            FreeCAD.Console.PrintWarning('cylinder with radius zero\n')
            mycyl=doc.addObject("Part::Feature","emptycyl")
            mycyl.Shape = Part.Compound([])
    else: # h == 0
        FreeCAD.Console.PrintWarning('cylinder with height <= zero\n')
        mycyl=doc.addObject("Part::Feature","emptycyl")
        mycyl.Shape = Part.Compound([])
    if printverbose: print("Center = ",tocenter)
    if tocenter=='true' :
       center(mycyl,0,0,h)
    if False :  
#   Does not fix problemfile or beltTighener although later is closer       
        newobj=doc.addObject("Part::FeaturePython",'RefineCylinder')
        RefineShape(newobj,mycyl)
        if gui:
            if FreeCAD.ParamGet("User parameter:BaseApp/Preferences/Mod/OpenSCAD").\
                GetBool('useViewProviderTree'):
                from OpenSCADFeatures import ViewProviderTree
                ViewProviderTree(newobj.ViewObject)
            else:
                newobj.ViewObject.Proxy = 0
            mycyl.ViewObject.hide()
        p[0] = [newobj]
    else :
        p[0] = [mycyl]
    if printverbose: print("End Cylinder")

def p_cube_action(p):
    'cube_action : cube LPAREN keywordargument_list RPAREN SEMICOL'
    global doc
    l,w,h = [float(str1) for str1 in p[3]['size']]
    if (l > 0 and w > 0 and h >0):
        if printverbose: print("cube : ",p[3])
        mycube=doc.addObject('Part::Box',p[1])
        mycube.Length=l
        mycube.Width=w
        mycube.Height=h
    else:
        FreeCAD.Console.PrintWarning('cube with radius zero\n')
        mycube=doc.addObject("Part::Feature","emptycube")
        mycube.Shape = Part.Compound([])
    if p[3]['center']=='true' :
       center(mycube,l,w,h);
    p[0] = [mycube]
    if printverbose: print("End Cube")

def p_circle_action(p) :
    'circle_action : circle LPAREN keywordargument_list RPAREN SEMICOL'
    if printverbose: print("Circle : "+str(p[3]))
    r = float(p[3]['r'])
    # Avoid zero radius
    if r == 0 : r = 0.00001
    n = int(p[3]['$fn'])
    fnmax = FreeCAD.ParamGet(\
        "User parameter:BaseApp/Preferences/Mod/OpenSCAD").\
        GetInt('useMaxFN',50)
    # Alter Max polygon to control if polygons are circles or polygons
    # in the modules preferences
    import Draft
    if n == 0 or fnmax != 0 and n >= fnmax:
        mycircle = FreeCAD.ActiveDocument.addObject("Part::Part2DObjectPython",'circle')
        Draft._Circle(mycircle)
        mycircle.Radius = r
        mycircle.MakeFace = True
        #mycircle = Draft.makeCircle(r,face=True) # would call doc.recompute
        #mycircle = doc.addObject('Part::Circle',p[1]) #would not create a face
        #mycircle.Radius = r
    else :
        #mycircle = Draft.makePolygon(n,r) # would call doc.recompute
        mycircle = FreeCAD.ActiveDocument.addObject("Part::Part2DObjectPython",'polygon')
        Draft._Polygon(mycircle)
        mycircle.FacesNumber = n
        mycircle.Radius = r
        mycircle.DrawMode = "inscribed"
        mycircle.MakeFace = True
    if gui:
        Draft._ViewProviderDraft(mycircle.ViewObject)
    if printverbose: print("Push Circle")
    p[0] = [mycircle]

def p_square_action(p) :
    'square_action : square LPAREN keywordargument_list RPAREN SEMICOL'
    if printverbose: print("Square")
    size = p[3]['size']
    x = float(size[0])
    y = float(size[1])
    mysquare = doc.addObject('Part::Plane',p[1])
    mysquare.Length=x
    mysquare.Width=y
    if p[3]['center']=='true' :
       center(mysquare,x,y,0)
    p[0] = [mysquare]

def addString(t,s,p):
    return(t + ', ' +s+' = "'+p[3][s]+'"')

def addValue(t,v,p):
    return(t + ', ' +v+' = '+p[3][v])

def p_text_action(p) :
    'text_action : text LPAREN keywordargument_list RPAREN SEMICOL'
    # If text string is null ignore
    if p[3]['text'] == "" or p[3]['text'] == " " :
        p[0] = []
        return
    t = 'text ( text="'+p[3]['text']+'"'
    t = addValue(t,'size',p)
    t = addString(t,'spacing',p)
    t = addString(t,'font',p)
    t = addString(t,'direction',p)
    t = addString(t,'language',p)
    t = addString(t,'script',p)
    t = addString(t,'halign',p)
    t = addString(t,'valign',p)
    t = addValue(t,'$fn',p)
    t = addValue(t,'$fa',p)
    t = addValue(t,'$fs',p)
    t = t+');'

    FreeCAD.Console.PrintMessage("textmsg : "+t+"\n")
    p[0] = [processTextCmd(t)]

def convert_points_list_to_vector(l):
    v = []
    for i in l :
        if printverbose: print(i)
        v.append(FreeCAD.Vector(i[0],i[1]))
    if printverbose: print(v)
    return(v)


def p_polygon_action_nopath(p) :
    'polygon_action_nopath : polygon LPAREN points EQ OSQUARE points_list_2d ESQUARE COMMA paths EQ undef COMMA keywordargument_list RPAREN SEMICOL'
    if printverbose: print("Polygon")
    if printverbose: print(p[6])
    v = convert_points_list_to_vector(p[6])
    mypolygon = doc.addObject('Part::Feature',p[1])
    if printverbose: print("Make Parts")
    # Close Polygon
    v.append(v[0])
    parts = Part.makePolygon(v)
    if printverbose: print("update object")
    mypolygon.Shape = Part.Face(parts)
    p[0] = [mypolygon]

def p_polygon_action_plus_path(p) :
    'polygon_action_plus_path : polygon LPAREN points EQ OSQUARE points_list_2d ESQUARE COMMA paths EQ OSQUARE path_set ESQUARE COMMA keywordargument_list RPAREN SEMICOL'
    if printverbose: print("Polygon with Path")
    if printverbose: print(p[6])
    v = convert_points_list_to_vector(p[6])
    if printverbose: print("Path Set List")
    if printverbose: print(p[12])
    for i in p[12] :
         if printverbose: print(i)
         mypolygon = doc.addObject('Part::Feature','wire')
         path_list = []
         for j in i :
             j = int(j)
             if printverbose: print(j)
             path_list.append(v[j])
#        Close path
         path_list.append(v[int(i[0])])
         if printverbose: print('Path List')
         if printverbose: print(path_list)
         wire = Part.makePolygon(path_list)
         mypolygon.Shape = Part.Face(wire)
         p[0] = [mypolygon]
#        This only pushes last polygon

def make_face(v1,v2,v3):
    wire = Part.makePolygon([v1,v2,v3,v1])
    face = Part.Face(wire)
    return face

def p_polyhedron_action(p) :
    '''polyhedron_action : polyhedron LPAREN points EQ OSQUARE points_list_3d ESQUARE COMMA faces EQ OSQUARE path_set ESQUARE COMMA keywordargument_list RPAREN SEMICOL
                      | polyhedron LPAREN points EQ OSQUARE points_list_3d ESQUARE COMMA triangles EQ OSQUARE points_list_3d ESQUARE COMMA keywordargument_list RPAREN SEMICOL'''
    if printverbose: print("Polyhedron Points")
    v = []
    for i in p[6] :
        if printverbose: print(i)
        v.append(FreeCAD.Vector(float(i[0]),float(i[1]),float(i[2])))
    if printverbose:
        print(v)
        print ("Polyhedron "+p[9])
        print (p[12])
    faces_list = []    
    mypolyhed = doc.addObject('Part::Feature',p[1])
    for i in p[12] :
        if printverbose: print(i)
        v2 = FreeCAD.Vector
        pp =[v2(v[k]) for k in i]
        # Add first point to end of list to close polygon
        pp.append(pp[0])
        print("pp")
        print(pp)
        w = Part.makePolygon(pp)
        print("w")
        print(w)
        try:
           f = Part.Face(w)
        except:
            secWireList = w.Edges[:]
            f = Part.makeFilledFace(Part.__sortEdges__(secWireList))
        #f = make_face(v[int(i[0])],v[int(i[1])],v[int(i[2])])
        faces_list.append(f)
    shell=Part.makeShell(faces_list)
    solid=Part.Solid(shell).removeSplitter()
    if solid.Volume < 0:
        solid.reverse()
    mypolyhed.Shape=solid
    p[0] = [mypolyhed]

def p_projection_action(p) :
    'projection_action : projection LPAREN keywordargument_list RPAREN OBRACE block_list EBRACE'
    if printverbose: print('Projection')
    if p[3]['cut']=='true' :
        planedim=1e9 # large but finite
        #infinite planes look bad in the GUI
        planename='xy_plane_used_for_project_cut'
        obj=doc.addObject('Part::MultiCommon','projection_cut')
        plane = doc.getObject(planename)
        if not plane:
            plane=doc.addObject("Part::Plane",planename)
            plane.Length=planedim*2
            plane.Width=planedim*2
            plane.Placement = FreeCAD.Placement(FreeCAD.Vector(\
                     -planedim,-planedim,0),FreeCAD.Rotation())
            if gui:
                plane.ViewObject.hide()
        if (len(p[6]) > 1):
            subobj = [fuse(p[6],"projection_cut_implicit_group")]
        else:
            subobj = p[6]
        obj.Shapes = [plane]+subobj
        if gui:
            subobj[0].ViewObject.hide()
        p[0] = [obj]
    else: # cut == 'false' => true projection
        if gui and not FreeCAD.ParamGet("User parameter:BaseApp/Preferences/Mod/OpenSCAD").\
                GetBool('usePlaceholderForUnsupported'):
            from PySide import QtGui
            QtGui.QMessageBox.critical(None, translate('OpenSCAD',"Unsupported Function")+" : "+p[1],translate('OpenSCAD',"Press OK"))
        else:
            p[0] = [placeholder(p[1],p[6],p[3])]
