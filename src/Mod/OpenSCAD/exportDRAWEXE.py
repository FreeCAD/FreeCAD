
#***************************************************************************
#*                                                                         *
#*   Copyright (c) 2014 Sebastian Hoogen <github@sebastianhoogen.de>       *
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
__title__="FreeCAD OpenSCAD Workbench - DRAWEXE exporter"
__author__ = "Sebastian Hoogen <github@sebastianhoogen.de>"

import FreeCAD, Part

if open.__module__ == '__builtin__':
        pythonopen = open

def f2s(n):
    '''convert to numerical value to string'''
    #return str(float(n))
    return ('%0.18f' % n).rstrip('0')

def placement2draw(placement,name='object'):
    """converts a FreeCAD Placement to trotate and ttranslate commands"""
    drawcommand=''
    if not placement.Rotation.isNull():
        import math
        dx,dy,dz=placement.Rotation.Axis
        an=math.degrees(placement.Rotation.Angle)
        drawcommand += "trotate %s 0 0 0 %s %s %s %s\n" % \
            (name,f2s(dx),f2s(dy),f2s(dz),f2s(an))
    if placement.Base.Length > 1e-8:
        x,y,z=placement.Base
        drawcommand += "ttranslate %s %s %s %s\n" % \
            (name,f2s(x),f2s(y),f2s(z))
    return drawcommand

def saveShape(csg,filename,shape,name,hasplacement = True):
    import os
    spath,sname = os.path.split(filename)
    sname=sname.replace('.','-')
    uname='%s-%s' %(sname,name)
    breppath=os.path.join(spath,'%s.brep'%uname)
    csg.write("restore %s.brep\n"%uname)
    csg.write("renamevar %s %s\n"%(uname,name))
    if hasplacement is None:  # saved with placement
        hasplacement = False # saved with placement
        shape.exportBrep(breppath)
    elif not hasplacement: #doesn't matter
        shape.exportBrep(breppath)
    else: #remove placement
        sh=shape.copy()
        sh.Placement=FreeCAD.Placement()
        sh.exportBrep(breppath)
    return hasplacement

def saveSweep(csg,ob,filename):
    import Part
    spine,subshapelst=ob.Spine
    #process_object(csg,spine,filename)
    try:
        path=Part.Wire([spine.Shape.getElement(subshapename) for \
                subshapename in spine,subshapelst])
    except: # BRep_API: command not done
        if spine.Shape.ShapeType == 'Edge':
            path = spine.Shape
        elif spine.Shape.ShapeType == 'Wire':
            path = Part.Wire(spine.Shape)
        else:
            raise ValueError('Unsuitabel Shape Type')
    spinename = '%s-0-spine' % ob.Name
    hasplacement = saveShape(csg,filename, path,spinename,None) # placement with shape
    #safePlacement(ob.Placement,ob.Name)
    csg.write('mksweep %s\n' % spinename)
    #setsweep
    setoptions=[]
    buildoptions=[]
    if ob.Frenet:
        setoptions.append('-FR')
    else:
        setoptions.append('-CF')
    if ob.Transition == 'Transformed':
        buildoptions.append('-M')
    elif ob.Transition == 'Right corner':
        buildoptions.append('-C')
    elif ob.Transition == 'Round corner':
        buildoptions.append('-R')
    if ob.Solid:
        buildoptions.append('-S')
    csg.write('setsweep %s\n' % (" ".join(setoptions)))
    #addsweep
    sections=ob.Sections
    sectionnames = []
    for i,subobj in enumerate(ob.Sections):
        #process_object(csg,subobj,filename)
        #sectionsnames.append(subobj.Name)
        #d1['basename']=subobj.Name
        sectionname = '%s-0-section-%02d-%s' % (ob.Name,i,subobj.Name)
        addoptions=[]
        sh = subobj.Shape
        if sh.ShapeType == 'Vertex':
            pass
        elif sh.ShapeType == 'Wire' or sh.ShapeType == 'Edge':
            sh = Part.Wire(sh)
        elif sh.ShapeType == 'Face':
            sh = sh.OuterWire
        else:
            raise ValueError
        hasplacement = saveShape(csg,filename,sh,sectionname,None) # placement with shape
        csg.write('addsweep %s %s\n' % (sectionname," ".join(addoptions)))
    csg.write('buildsweep %s %s\n' % (ob.Name," ".join(buildoptions)))


def isDraftFeature(ob):
    if (ob.isDerivedFrom('Part::FeaturePython') or \
            ob.isDerivedFrom('Part::Part2DObjectPython')) and \
            hasattr(ob.Proxy,'__module__') and \
            ob.Proxy.__module__ == 'Draft':
        return True

def isDraftClone(ob):
    if ob.isDerivedFrom('Part::FeaturePython') and \
            hasattr(ob.Proxy,'__module__') and \
            ob.Proxy.__module__ == 'Draft':
        import Draft
        return isinstance(ob.Proxy,Draft._Clone)

def isDraftCircle(ob):
    if isDraftFeature(ob):
        import Draft
        return isinstance(ob.Proxy,Draft._Circle)

def isDraftWire(ob):
    if isDraftFeature(ob):
        import Draft
        if isinstance(ob.Proxy,Draft._Wire):
            #only return true if we support all options
            #"Closed" append last point at the end
            #"MakeFace" 
            #"Points" data we need
            # the usage of 'start' and 'end' is not clear
            if ob.Base is None and ob.Tool is None and \
                    ob.FilletRadius.Value == 0.0 and \
                    ob.ChamferSize.Value == 0.0:
                return True


def isOpenSCADFeature(ob):
    if ob.isDerivedFrom('Part::FeaturePython') and \
            hasattr(ob.Proxy,'__module__') and \
            ob.Proxy.__module__ == 'OpenSCADFeatures':
        return True

def isOpenSCADMultMatrixFeature(ob):
    if ob.isDerivedFrom('Part::FeaturePython') and \
            hasattr(ob.Proxy,'__module__') and \
            ob.Proxy.__module__ == 'OpenSCADFeatures':
        import OpenSCADFeatures
        return isinstance(ob.Proxy,OpenSCADFeatures.MatrixTransform)

def isDeform(ob):
    """tests whether the object is a Matrix transformation
    that does a non-uniform scaling"""
    # the [ is important to exclude cases with additional
    # rotation or mirroring.
    # TBD decompose complex matrix operations
    return isOpenSCADMultMatrixFeature(ob) and \
            ob.Matrix.analyze().startswith('Scale [')



def process_object(csg,ob,filename):
    d1 = {'name':ob.Name}
    hasplacement = not ob.Placement.isNull()
    if ob.TypeId in ["Part::Cut","Part::Fuse","Part::Common","Part::Section"]:
        d1.update({'part':ob.Base.Name,'tool':ob.Tool.Name,\
            'command':'b%s' % ob.TypeId[6:].lower()})
        process_object(csg,ob.Base,filename)
        process_object(csg,ob.Tool,filename)
        csg.write("%(command)s %(name)s %(part)s %(tool)s\n"%d1)
    elif ob.TypeId == "Part::Sphere" :
        d1.update({'radius':f2s(ob.Radius),'angle1':f2s(ob.Angle1),\
            'angle2':f2s(ob.Angle2),'angle3':f2s(ob.Angle3)})
        csg.write('psphere %(name)s %(radius)s %(angle1)s %(angle2)s '\
            '%(angle3)s\n'%d1)
    elif ob.TypeId == "Part::Box" :
        d1.update({'dx':f2s(ob.Length),'dy':f2s(ob.Width),'dz':f2s(ob.Height)})
        csg.write('box %(name)s %(dx)s %(dy)s %(dz)s\n'%d1)
    elif ob.TypeId == "Part::Cylinder" :
        d1.update({'radius':f2s(ob.Radius),'height':f2s(ob.Height),\
            'angle':f2s(ob.Angle)})
        csg.write('pcylinder %(name)s %(radius)s %(height)s %(angle)s\n'%d1)
    elif ob.TypeId == "Part::Cone" :
        d1.update({'radius1':f2s(ob.Radius1),'radius2':f2s(ob.Radius2),\
            'height':f2s(ob.Height)})
        csg.write('pcone %(name)s %(radius1)s %(radius2)s %(height)s\n'%d1)
    elif ob.TypeId == "Part::Torus" :
        d1.update({'radius1':f2s(ob.Radius1),'radius2':f2s(ob.Radius2),\
            'angle1': f2s(ob.Angle1),'angle2':f2s(ob.Angle2),\
            'angle3': f2s(ob.Angle3)})
        csg.write('ptorus %(name)s %(radius1)s %(radius2)s %(angle1)s '\
                '%(angle2)s %(angle3)s\n' % d1)
    elif ob.TypeId == "Part::Mirroring" :
        process_object(csg,ob.Source,filename)
        csg.write('tcopy %s %s\n'%(ob.Source.Name,d1['name']))
        b=ob.Base
        d1['x']=f2s(ob.Base.x)
        d1['y']=f2s(ob.Base.y)
        d1['z']=f2s(ob.Base.z)
        d1['dx']=f2s(ob.Normal.x)
        d1['dy']=f2s(ob.Normal.y)
        d1['dz']=f2s(ob.Normal.z)
        csg.write('tmirror %(name)s %(x)s %(y)s %(z)s %(dx)s %(dy)s %(dz)s\n' \
                % d1)
    elif ob.TypeId == 'Part::Compound':
        if len(ob.Links) == 0:
            pass
        elif len(ob.Links) == 1:
            process_object(csg,ob.Links[0],filename)
            csg.write('tcopy %s %s\n'%(ob.Links[0].Name,d1['name']))
        else:
            basenames=[]
            for i,subobj in enumerate(ob.Links):
                process_object(csg,subobj,filename)
                basenames.append(subobj.Name)
            csg.write('compound %s %s\n' % (' '.join(basenames),ob.Name))
    elif ob.TypeId in ["Part::MultiCommon", "Part::MultiFuse"]:
        if len(ob.Shapes) == 0:
            pass
        elif len(ob.Shapes) == 1:
            process_object(csg,ob.Shapes[0],filename)
            csg.write('tcopy %s %s\n'%(ob.Shapes[0].Name,d1['name']))
        else:
            topname = ob.Name
            command = 'b%s' % ob.TypeId[11:].lower()
            lst1=ob.Shapes[:]
            current=lst1.pop(0)
            curname=current.Name
            process_object(csg,current,filename)
            i=1
            while lst1:
                if len(lst1) >= 2:
                    nxtname='to-%s-%03d-t'%(topname,i)
                else:
                    nxtname=topname
                nxt=lst1.pop(0)
                process_object(csg,nxt,filename)
                csg.write("%s %s %s %s\n"%(command,nxtname,curname,nxt.Name))
                curname=nxtname
                i+=1
    elif ob.TypeId == "Part::Prism" :
        import math
        polyname = '%s-polyline' % d1['name']
        wirename = '%s-polywire' % d1['name']
        facename = '%s-polyface' % d1['name']
        d1['base']= facename
        m=FreeCAD.Matrix()
        v=FreeCAD.Vector(ob.Circumradius.Value,0,0)
        m.rotateZ(2*math.pi/ob.Polygon)
        points=[]
        for i in range(ob.Polygon):
            points.append(v)
            v=m.multiply(v)
        points.append(v)
        pointstr=' '.join('%s %s %s'%(f2s(v.x),f2s(v.y),f2s(v.z)) \
                for v in points)
        csg.write('polyline %s %s\n' % (polyname,pointstr))
        csg.write('wire %s %s\n' %(wirename,polyname))
        csg.write('mkplane %s %s\n' % (facename,polyname))
        csg.write('prism %s %s 0 0 %s\n' % (d1['name'],facename,\
            f2s(ob.Height.Value)))
    elif ob.TypeId == "Part::Sweep" and True:
        saveSweep(csg,ob,filename)

    elif isDeform(ob): #non-uniform scaling
        m=ob.Matrix
        process_object(csg,ob.Base,filename)
        #csg.write('tcopy %s %s\n'%(ob.Base.Name,d1['name']))
        d1['basename']=ob.Base.Name
        d1['cx']=f2s(m.A11)
        d1['cy']=f2s(m.A22)
        d1['cz']=f2s(m.A33)
        csg.write('deform %(name)s %(basename)s %(cx)s %(cy)s %(cz)s\n' % d1)
        if m.A14 > 1e-8 or m.A24 > 1e-8 or m.A34 > 1e-8:
            csg.write("ttranslate %s %s %s %s\n" % \
                (ob.Name,f2s(m.A14),f2s(m.A24),f2s(m.A34)))
    elif isDraftCircle(ob):
        "circle name x y [z [dx dy dz]] [ux uy [uz]] radius"
        d1['radius']=ob.Radius.Value
        pfirst=f2s(ob.FirstAngle.getValueAs('rad').Value)
        plast=f2s(ob.LastAngle.getValueAs('rad').Value)

        #todo ofirst and p last as arguements to mkedge getValueAs('rad').Value
        curvename = '%s-curve' % d1['name']
        edgename = '%s-edge' % d1['name']
        wirename = '%s-dwirewire' % d1['name']
        csg.write('circle %s 0 0 0 %s\n' % (curvename,ob.Radius.Value))
        csg.write('mkedge %s %s %s %s\n' % (edgename,curvename,pfirst,plast))
        csg.write('wire %s %s\n' %(wirename,edgename))
        if ob.MakeFace:
            csg.write('mkplane %s %s\n' % (d1['name'],wirename))
        else:
            csg.write("renamevar %s %s\n"%(wirename,d1['name'])) #the wire is the final object
    elif isDraftWire(ob):
        points=ob.Points
        if ob.Closed:
            points.append(points[0])
        polyname = '%s-dwireline' % d1['name']
        pointstr=' '.join('%s %s %s'%(f2s(v.x),f2s(v.y),f2s(v.z)) \
                for v in points)
        csg.write('polyline %s %s\n' % (polyname,pointstr))
        if ob.MakeFace:
            wirename = '%s-dwirewire' % d1['name']
            csg.write('wire %s %s\n' %(wirename,polyname))
            facename =  d1['name']
            csg.write('mkplane %s %s\n' % (facename,polyname))
        else:
            wirename =  d1['name']
            csg.write('wire %s %s\n' %(wirename,polyname))
    elif isDraftClone(ob):
        x,y,z=ob.Scale.x
        if x == y == z: #uniform scaling
            d1['scale']=f2s(x)
        else:
            d1['cx']=f2s(x)
            d1['cy']=f2s(y)
            d1['cz']=f2s(z)
        if len(ob.Objects) == 1:
            d1['basename']=ob.Objects[0].Name
            process_object(csg,ob.Objects[0],filename)
            if x == y == z: #uniform scaling
                csg.write('tcopy %(basename)s %(name)s\n' % d1)
                csg.write('pscale %(name)s 0 0 0 %(scale)s\n' % d1)
            else:
                csg.write('deform %(name)s %(basename)s'\
                        ' %(cx)s %(cy)s %(cz)s\n' % d1)
        else: #compound
            newnames=[]
            for i,subobj in enumerate(ob.Objects):
                process_object(csg,subobj,filename)
                d1['basename']=subobj.Name
                newname='%s-%2d' % (ob.Name,i)
                d1['newname']=newname
                newnames.append(newname)
                if x == y == z: #uniform scaling
                    csg.write('tcopy %(basename)s %(newname)s\n' % d1)
                    csg.write('pscale %(newname)s 0 0 0 %(scale)s\n' % d1)
                else:
                    csg.write('deform %(newname)s %(basename)s'\
                            ' %(cx)s %(cy)s %(cz)s\n' % d1)
            csg.write('compound %s %s\n' % (' '.join(newnames),ob.Name))
        
    #elif ob.isDerivedFrom('Part::FeaturePython') and \
    #    hasattr(ob.Proxy,'__module__'):
    #    pass
    elif ob.isDerivedFrom('Part::Feature') :
        if ob.Shape.isNull(): #would crash in exportBrep otherwise
            raise ValueError
        hasplacement = saveShape(csg,filename,ob.Shape,ob.Name,hasplacement)
    if hasplacement:
        csg.write(placement2draw(ob.Placement,ob.Name))

def export(exportList,filename):
    "called when freecad exports a file"
    # process Objects
    csg = pythonopen(filename,'w')
    csg.write('#generated by FreeCAD\n')
    csg.write('pload ALL\n')
    for ob in exportList:
        process_object(csg,ob,filename)
    csg.write('donly %s\n'%' '.join([obj.Name for obj in exportList]))
    csg.close()
