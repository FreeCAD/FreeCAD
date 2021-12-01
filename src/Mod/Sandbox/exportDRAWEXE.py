
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

# unsupported primitives
# Part:: Wedge, Helix, Spiral, Elipsoid
# Draft: Rectangle, BSpline, BezCurve

def quaternionToString(rot):
    def shorthexfloat(f):
        s=f.hex()
        mantisse, exponent = f.hex().split('p',1)
        return '%sp%s' % (mantisse.rstrip('0'),exponent)
    x,y,z,w=rot.Q
    return 'q=%s+%s*i+%s*j+%s*k' % (shorthexfloat(w),shorthexfloat(x),
            shorthexfloat(y),shorthexfloat(z))

def f2s(n,angle=False,axis=False):
    '''convert to numerical value to string
    try to remove no significant digits, by guessing a former rounding
    '''
    if abs(n) < 1e-14: return '0'
    if angle and len(('%0.6e' % n).split('e')[0].rstrip('0') ) < 3:
        return ('%0.5f' % n).rstrip('0').rstrip('.')
    elif axis and len(('%0.13e' % n).split('e')[0].rstrip('0') ) < 6:
        return ('%0.10f' % n).rstrip('0').rstrip('.')
    else:
        for i in range(20):
            s = ('%%1.%df'% i) % n
            if float(s) == n:
                return s
        for i in range(20):
            s = ('%%0.%de'% i) % n
            if float(s) == n:
                return s

def ax2_xdir(normal):
    #adapted from gp_Ax2.ccc (c) OpenCascade SAS LGPL 2.1+

    xa=abs(normal.x)
    ya=abs(normal.y)
    za=abs(normal.z)
    if ya <= xa and ya <= za:
        if xa > za:
            return FreeCAD.Vector(-normal.z,0, normal.x)
        else:
            return FreeCAD.Vector( normal.z,0,-normal.x)
    elif xa <= ya and xa <= za:
        if ya > za:
            return FreeCAD.Vector(0,-normal.z, normal.y)
        else:
            return FreeCAD.Vector(0, normal.z,-normal.y)
    else:
        if xa > ya:
            return FreeCAD.Vector(-normal.y, normal.x,0)
        else:
            return FreeCAD.Vector( normal.y,-normal.x,0)

def occversiontuple():
    import FreeCAD,Part
    occmajs,occmins,occfixs = FreeCAD.ConfigGet('OCC_VERSION').split('.')[:3]
    return (int(occmajs),int(occmins),int(occfixs))

def polygonstr(r,pcount):
    import math
    v=FreeCAD.Vector(r,0,0)
    m=FreeCAD.Matrix()
    m.rotateZ(2*math.pi/pcount)
    points=[]
    for i in range(pcount):
        points.append(v)
        v=m.multiply(v)
    points.append(v)
    return ' '.join('%s %s %s'%(f2s(v.x),f2s(v.y),f2s(v.z)) \
            for v in points)

def formatobjtype(ob):
    objtype=ob.TypeId
    if (ob.isDerivedFrom('Part::FeaturePython') or \
            ob.isDerivedFrom('Part::Part2DObjectPython') or\
            ob.isDerivedFrom('App::FeaturePython')) and \
            hasattr(ob.Proxy,'__module__'):
        return '%s::%s.%s' % (ob.TypeId,ob.Proxy.__module__,\
                    ob.Proxy.__class__.__name__)
    else:
        return ob.TypeId

def placement2draw(placement,name='object'):
    """converts a FreeCAD Placement to trotate and ttranslate commands"""
    drawcommand=''
    if not placement.Rotation.isNull():
        import math
        #dx,dy,dz=placement.Rotation.Axis
        ax=placement.Rotation.Axis
        import itertools
        # denormalize rotation axis
        for t in itertools.product((0,1,-1),repeat=3):
            if t != (0,0,0):
                if (ax-FreeCAD.Vector(*t).normalize()).Length < 1e-15:
                    dx,dy,dz = t
                    break
        else:
            dx,dy,dz=placement.Rotation.Axis
            #drawcommand += "# %s\n" %quaternionToString(placement.Rotation)
        an=math.degrees(placement.Rotation.Angle)
        drawcommand += "trotate %s 0 0 0 %s %s %s %s\n" % (name,\
            f2s(dx),f2s(dy),f2s(dz),\
#            f2s(dx,axis=True),f2s(dy,axis=True),f2s(dz,axis=True),\
            f2s(an,angle=True))
    if placement.Base.Length > 1e-8:
        x,y,z=placement.Base
        drawcommand += "ttranslate %s %s %s %s\n" % \
            (name,f2s(x),f2s(y),f2s(z))
    return drawcommand

def saveShape(csg,filename,shape,name,hasplacement = True,cleanshape=False):
    import os
    spath,sname = os.path.split(filename)
    sname=sname.replace('.','-')
    uname='%s-%s' %(sname,name)
    breppath=os.path.join(spath,'%s.brep'%uname)
    csg.write("restore %s.brep %s\n" % (uname,name))
    if cleanshape:
        import Part
        try:
            shape = shape.cleaned()
        except Part.OCCError:
            shape = shape.copy()
    if hasplacement is None:  # saved with placement
        hasplacement = False # saved with placement
        shape.exportBrep(breppath)
    elif not hasplacement: #doesn't matter
        shape.exportBrep(breppath)
    else: #remove placement
        sh=shape.copy()
        sh.Placement=FreeCAD.Placement()
        # it not yet tested if changing the placement recreated the
        # tessellation. But for now we simply do the cleaning once again
        # to stay on the safe side
        if cleanshape:
            shape = shape.cleaned()
        sh.exportBrep(breppath)
    return hasplacement



def isDraftFeature(ob):
    if (ob.isDerivedFrom('Part::FeaturePython') or \
            ob.isDerivedFrom('Part::Part2DObjectPython')) and \
            hasattr(ob.Proxy,'__module__') and \
            ob.Proxy.__module__ == 'Draft':
        return True

def isDraftClone(ob):
    if (ob.isDerivedFrom('Part::FeaturePython') or \
            ob.isDerivedFrom('Part::Part2DObjectPython')) and \
            hasattr(ob.Proxy,'__module__') and \
            ob.Proxy.__module__ == 'Draft':
        import Draft
        return isinstance(ob.Proxy,Draft._Clone)

def isDraftCircle(ob):
    if isDraftFeature(ob):
        import Draft
        return isinstance(ob.Proxy,Draft._Circle)

def isDraftEllipse(ob):
    if isDraftFeature(ob):
        import Draft
        return isinstance(ob.Proxy,Draft._Ellipse)

def isDraftPolygon(ob):
    if isDraftFeature(ob):
        import Draft
        return isinstance(ob.Proxy,Draft._Polygon)

def isDraftPoint(ob):
    if isDraftFeature(ob):
        import Draft
        return isinstance(ob.Proxy,Draft._Point)

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

def isDraftShape2DView(ob):
    if isDraftFeature(ob):
        import Draft
        return isinstance(ob.Proxy,Draft._Shape2DView)

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




class Drawexporter(object):
    def __init__(self, filename):
        self.objectcache=set()
        self.csg = pythonopen(filename,'w')
        #self.csg=csg
        self.filename=filename
        #settings
        self.alwaysexplode = True
        self.cleanshape = False

    def __enter__(self):
        return self

    def write_header(self):
        import FreeCAD
        #self.csg.write('#!/usr/bin/env DRAWEXE\n')
        self.csg.write('#generated by FreeCAD %s\n' % \
                '.'.join(FreeCAD.Version()[0:3]))
        self.csg.write('pload MODELING\n')

    def write_displayonly(self,objlst):
        self.csg.write('donly %s\n'%' '.join([obj.Name for obj in objlst]))

    def saveSweep(self,ob):
        import Part
        spine,subshapelst=ob.Spine
        #process_object(csg,spine,filename)
        explodeshape = self.alwaysexplode or self.process_object(spine,True)
        if explodeshape:
            self.process_object(spine)
            if len(subshapelst) and spine.Shape.ShapeType != 'Edge':
                #raise NotImplementedError # hit the fallback
                # currently all subshapes are edges
                self.csg.write('explode %s E\n' % spine.Name )
                edgelst = ' '.join(('%s_%s' % (spine.Name,ss[4:]) for ss \
                        in subshapelst))
                spinename = '%s-0-spine' % ob.Name
                self.csg.write('wire %s %s\n' %(spinename,edgelst))
            elif spine.Shape.ShapeType == 'Wire':
                spinename = spine.Name
            elif spine.Shape.ShapeType == 'Edge':
                spinename = '%s-0-spine' % ob.Name
                self.csg.write('wire %s %s\n' %(spinename,spine.Name))
        else: # extract only the used subshape
            if len(subshapelst):
                path=Part.Wire([spine.Shape.getElement(subshapename) for \
                        subshapename in subshapelst])
            elif spine.Shape.ShapeType == 'Edge':
                path = spine.Shape
            elif spine.Shape.ShapeType == 'Wire':
                path = Part.Wire(spine.Shape)
            else:
                raise ValueError('Unsuitabel Shape Type')
            spinename = '%s-0-spine' % ob.Name
            saveShape(self.csg,self.filename, path,spinename,None,\
                    self.cleanshape) # placement with shape
        #safePlacement(ob.Placement,ob.Name)
        self.csg.write('mksweep %s\n' % spinename)
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
        self.csg.write('setsweep %s\n' % (" ".join(setoptions)))
        #addsweep
        sections=ob.Sections
        sectionnames = []
        for i,subobj in enumerate(ob.Sections):
            #process_object(csg,subobj,filename)
            #sectionsnames.append(subobj.Name)
            #d1['basename']=subobj.Name
            sectionname = '%s-0-section-%02d-%s' % (ob.Name,i,subobj.Name)
            addoptions=[]
            explodeshape = self.alwaysexplode or \
                    self.process_object(subobj,True)
            if explodeshape:
                sh = subobj.Shape
                if sh.ShapeType == 'Vertex' or sh.ShapeType == 'Wire' or \
                        sh.ShapeType == 'Edge' or \
                        sh.ShapeType == 'Face' and len(sh.Wires) == 1:
                    self.process_object(subobj)
                    if sh.ShapeType == 'Wire' or sh.ShapeType == 'Vertex':
                        #csg.write('tcopy %s %s\n' %(subobj.Name,sectionname))
                        sectionname = subobj.Name
                    if sh.ShapeType == 'Edge':
                        self.csg.write('explode %s E\n' % subobj.Name )
                        self.csg.write('wire %s %s_1\n' %(sectionname,subobj.Name))
                    if sh.ShapeType == 'Face':
                        #we should use outer wire when it becomes available
                        self.csg.write('explode %s W\n' % subobj.Name )
                        #csg.write('tcopy %s_1 %s\n' %(subobj.Name,sectionname))
                        sectionname ='%s_1' % subobj.Name
                else:
                    explodeshape = False
            if not explodeshape: # extract only the used subshape
                sh = subobj.Shape
                if sh.ShapeType == 'Vertex':
                    pass
                elif sh.ShapeType == 'Wire' or sh.ShapeType == 'Edge':
                    sh = Part.Wire(sh)
                elif sh.ShapeType == 'Face':
                    sh = sh.OuterWire
                else:
                    raise ValueError('Unrecognized Shape Type')
                saveShape(self.csg,self.filename,sh,sectionname,None,\
                        self.cleanshape) # placement with shape
            self.csg.write('addsweep %s %s\n' % \
                    (sectionname," ".join(addoptions)))
        self.csg.write('buildsweep %s %s\n' % (ob.Name," ".join(buildoptions)))

    def process_object(self,ob,checksupported=False,toplevel=False):
        if not checksupported and ob.Name in self.objectcache:
            return # object in present
        if not checksupported:
            self.objectcache.add(ob.Name)
        d1 = {'name':ob.Name}
        if hasattr(ob,'Placement'):
            hasplacement = not ob.Placement.isNull()
        else:
            hasplacement = False
        if ob.TypeId in ["Part::Cut","Part::Fuse","Part::Common",\
                "Part::Section"]:
            if checksupported: return True # The object is supported
            d1.update({'part':ob.Base.Name,'tool':ob.Tool.Name,\
                'command':'b%s' % ob.TypeId[6:].lower()})
            self.process_object(ob.Base)
            self.process_object(ob.Tool)
            self.csg.write("%(command)s %(name)s %(part)s %(tool)s\n"%d1)
        elif ob.TypeId == "Part::Plane" :
            if checksupported: return True # The object is supported
            d1.update({'uname':'%s-untrimmed' % d1['name'],\
                    'length': f2s(ob.Length),'width': f2s(ob.Width)})
            self.csg.write("plane %s 0 0 0\n"%d1['uname'])
            self.csg.write(\
                    "mkface %(name)s %(uname)s 0 %(length)s 0 %(width)s\n"%d1)
        elif ob.TypeId == "Part::Ellipse" :
            if checksupported: return True # The object is supported
            d1.update({'uname':'%s-untrimmed'%d1['name'], 'maj':\
                    f2s(ob.MajorRadius), 'min': f2s(ob.MinorRadius),\
                    'pf':f2s(ob.Angle0.getValueAs('rad').Value), \
                    'pl':f2s(ob.Angle1.getValueAs('rad').Value)})
            self.csg.write("ellipse %(uname)s 0 0 0 %(maj)s %(min)s\n"%d1)
            self.csg.write('mkedge %(name)s %(uname)s %(pf)s %(pl)s\n' % d1)
        elif ob.TypeId == "Part::Sphere" :
            if checksupported: return True # The object is supported
            d1.update({'radius':f2s(ob.Radius),'angle1':f2s(ob.Angle1),\
                'angle2':f2s(ob.Angle2),'angle3':f2s(ob.Angle3)})
            if ob.Angle1.Value == -90 and ob.Angle2.Value == 90 and \
                    ob.Angle3.Value == 360:
                self.csg.write('psphere %(name)s %(radius)s\n'%d1)
            else:
                self.csg.write('psphere %(name)s %(radius)s %(angle1)s '
                        '%(angle2)s %(angle3)s\n'%d1)
        elif ob.TypeId == "Part::Box" :
            if checksupported: return True # The object is supported
            d1.update({'dx':f2s(ob.Length),'dy':f2s(ob.Width),'dz':f2s(ob.Height)})
            self.csg.write('box %(name)s %(dx)s %(dy)s %(dz)s\n'%d1)
        elif ob.TypeId == "Part::Cylinder" :
            if checksupported: return True # The object is supported
            d1.update({'radius':f2s(ob.Radius),'height':f2s(ob.Height),\
                'angle':f2s(ob.Angle)})
            if ob.Angle.Value == 360:
                self.csg.write('pcylinder %(name)s %(radius)s %(height)s\n'%d1)
            else:
                self.csg.write('pcylinder %(name)s %(radius)s %(height)s '\
                        '%(angle)s\n'%d1)
        elif ob.TypeId == "Part::Cone" :
            if checksupported: return True # The object is supported
            d1.update({'radius1':f2s(ob.Radius1),'radius2':f2s(ob.Radius2),\
                    'height':f2s(ob.Height),'angle':f2s(ob.Angle)})
            if ob.Angle.Value == 360:
                self.csg.write('pcone %(name)s %(radius1)s %(radius2)s '\
                        '%(height)s\n'%d1)
            else:
                self.csg.write('pcone %(name)s %(radius1)s %(radius2)s '\
                        '%(height)s %(angle)s\n'%d1)
        elif ob.TypeId == "Part::Torus" :
            if checksupported: return True # The object is supported
            d1.update({'radius1':f2s(ob.Radius1),'radius2':f2s(ob.Radius2),\
                'angle1': f2s(ob.Angle1),'angle2':f2s(ob.Angle2),\
                'angle3': f2s(ob.Angle3)})
            if ob.Angle1.Value == -180 and ob.Angle2.Value == 180 and \
                    ob.Angle3.Value == 360:
                self.csg.write('ptorus %(name)s %(radius1)s %(radius2)s\n'%d1)
            else:
                self.csg.write('ptorus %(name)s %(radius1)s %(radius2)s '\
                        '%(angle1)s %(angle2)s %(angle3)s\n' % d1)
        elif ob.TypeId == "Part::Mirroring" :
            if checksupported: return True # The object is supported
            self.process_object(ob.Source)
            self.csg.write('tcopy %s %s\n'%(ob.Source.Name,d1['name']))
            b=ob.Base
            d1['x']=f2s(ob.Base.x)
            d1['y']=f2s(ob.Base.y)
            d1['z']=f2s(ob.Base.z)
            d1['dx']=f2s(ob.Normal.x)
            d1['dy']=f2s(ob.Normal.y)
            d1['dz']=f2s(ob.Normal.z)
            self.csg.write('tmirror %(name)s %(x)s %(y)s %(z)s %(dx)s %(dy)s %(dz)s\n' \
                    % d1)
        elif ob.TypeId == 'Part::Compound':
            if len(ob.Links) == 0:
                pass
            elif len(ob.Links) == 1:
                if checksupported:
                    return self.process_object(ob.Links[0],True)
                self.process_object(ob.Links[0])
                self.csg.write('tcopy %s %s\n'%(ob.Links[0].Name,d1['name']))
            else:
                if checksupported: return True # The object is supported
                basenames=[]
                for i,subobj in enumerate(ob.Links):
                    self.process_object(subobj)
                    basenames.append(subobj.Name)
                self.csg.write('compound %s %s\n' % (' '.join(basenames),ob.Name))
        elif ob.TypeId in ["Part::MultiCommon", "Part::MultiFuse"]:
            if len(ob.Shapes) == 0:
                pass
            elif len(ob.Shapes) == 1:
                if checksupported:
                    return self.process_object(ob.Shapes[0],True)
                self.process_object(ob.Shapes[0],)
                self.csg.write('tcopy %s %s\n'%(ob.Shapes[0].Name,d1['name']))
            elif ob.TypeId == "Part::MultiFuse" and \
                    occversiontuple() >= (6,8,1):
                if checksupported: return True # The object is supported
                for subobj in ob.Shapes:
                    self.process_object(subobj)
                self.csg.write("bclearobjects\nbcleartools\n")
                self.csg.write("baddobjects %s\n" % ob.Shapes[0].Name)
                self.csg.write("baddtools %s\n" % " ".join(subobj.Name for \
                        subobj in ob.Shapes[1:]))
                self.csg.write("bfillds\n")
                self.csg.write("bbop %s 1\n" % ob.Name) #BOPAlgo_FUSE == 1
            else:
                if checksupported: return True # The object is supported
                topname = ob.Name
                command = 'b%s' % ob.TypeId[11:].lower()
                lst1=ob.Shapes[:]
                current=lst1.pop(0)
                curname=current.Name
                self.process_object(current)
                i=1
                while lst1:
                    if len(lst1) >= 2:
                        nxtname='to-%s-%03d-t'%(topname,i)
                    else:
                        nxtname=topname
                    nxt=lst1.pop(0)
                    self.process_object(nxt)
                    self.csg.write("%s %s %s %s\n"%(command,nxtname,curname,nxt.Name))
                    curname=nxtname
                    i+=1
        elif (isDraftPolygon(ob) and ob.ChamferSize.Value == 0 and\
                ob.FilletRadius.Value == 0 and ob.Support is None) or\
                ob.TypeId == "Part::Prism" or \
                ob.TypeId == "Part::RegularPolygon":
            if checksupported: return True # The object is supported
            draftpolygon = isDraftPolygon(ob)
            if draftpolygon:
                pcount = ob.FacesNumber
                if ob.DrawMode =='inscribed':
                    r=ob.Radius.Value
                elif ob.DrawMode =='circumscribed':
                    import math
                    r =  ob.Radius.Value/math.cos(math.pi/pcount)
                else:
                    raise ValueError
            else:
                pcount = ob.Polygon
                r=ob.Circumradius.Value
            justwire = ob.TypeId == "Part::RegularPolygon" or \
                    (draftpolygon and ob.MakeFace == False)
            polyname = '%s-polyline' % d1['name']
            if justwire:
                wirename = d1['name']
            else:
                wirename = '%s-polywire' % d1['name']
                if ob.TypeId == "Part::Prism":
                    facename = '%s-polyface' % d1['name']
                else:
                    facename = d1['name']
            self.csg.write('polyline %s %s\n' % (polyname,polygonstr(r,pcount)))
            self.csg.write('wire %s %s\n' %(wirename,polyname))
            if not justwire:
                self.csg.write('mkplane %s %s\n' % (facename,polyname))
                if ob.TypeId == "Part::Prism":
                    self.csg.write('prism %s %s 0 0 %s\n' % \
                            (d1['name'],facename, f2s(ob.Height.Value)))
        elif ob.TypeId == "Part::Extrusion" and ob.TaperAngle.Value == 0:
            if checksupported: return True # The object is supported
            self.process_object(ob.Base)
            #Warning does not fully resemble the functionality of
            #Part::Extrusion
            #csg.write('tcopy %s %s\n'%(ob.Base.Name,d1['name']))
            facename=ob.Base.Name
            self.csg.write('prism %s %s %s %s %s\n' % (d1['name'],facename,\
                f2s(ob.Dir.x),f2s(ob.Dir.y),f2s(ob.Dir.z)))
        elif ob.TypeId == "Part::Fillet" and True: #disabled
            if checksupported: return True # The object is supported
            self.process_object(ob.Base)
            self.csg.write('explode %s E\n' % ob.Base.Name )
            self.csg.write('blend %s %s %s\n' % (d1['name'],ob.Base.Name,\
                ' '.join(('%s %s'%(f2s(e[1]),'%s_%d' % (ob.Base.Name,e[0])) \
                for e in ob.Edges))))
        elif ob.TypeId == "Part::Thickness" and not ob.SelfIntersection and \
                ob.Mode == 'Skin':
            if checksupported: return True # The object is supported
            jointype = {'Arc':'a','Intersection':'i','Tangent':'t'} #Join
            inter = {False: 'p', True: 'c'} #Intersection
            baseobj, facelist = ob.Faces
            self.process_object(baseobj)
            faces = ' '.join([('%s_%s' %(baseobj.Name,f[4:])) \
                for f in facelist])
            value = f2s(ob.Value)
            self.csg.write('explode %s F\n' % baseobj.Name )
            self.csg.write('offsetparameter 1e-7 %s %s\n' % \
                    (inter[ob.Intersection],jointype[ob.Join]))
            self.csg.write('offsetload %s %s %s\n'%(baseobj.Name,value,faces))
            self.csg.write('offsetperform %s\n' % d1['name'] )

        elif ob.TypeId == "Part::Sweep" and True:
            if checksupported: return True # The object is supported
            self.saveSweep(ob)
        elif ob.TypeId == "Part::Loft":
            if checksupported: return True # The object is supported
            sectionnames=[]
            for i,subobj in enumerate(ob.Sections):
                explodeshape = self.alwaysexplode or \
                        self.process_object(suboobj,True)
                if explodeshape and False: #disabled TBD
                    try:
                        raise NotImplementedError
                        sectionname = '%s-%02d-section' % (ob.Name,i)
                        sh = subobj.Shape
                        if sh.isNull():
                            raise ValueError # hit the fallback
                        tempname=spine.Name
                        if sh.ShapeType == 'Compound':
                            sh = sh.childShapes()[0]
                            self.csg.write('explode %s\n' % tempname )
                            tempname = '%s_1' % tempname
                        if sh.ShapeType == 'Face':
                            #sh = sh.OuterWire #not available
                            if len(sh.Wires) == 1:
                                sh=sh.Wires[0]
                                self.csg.write('explode %s\n W' % tempname )
                                tempname = '%s_1' % tempname
                            else:
                                raise NotImplementedError
                        elif sh.ShapeType == 'Edge':
                            self.csg.write('wire %s %s\n' %(sectionname,tempname))
                            tempname = sectionname
                        sectionname = tempname
                    except NotImplementedError:
                        explodeshape = False # fallback
                else:
                    explodeshape = False # fallback if we hit the False before
                if not explodeshape: # extract only the used subshape
                    sh = subobj.Shape
                    if not sh.isNull():
                        if sh.ShapeType == 'Compound':
                            sh = sh.childShapes()[0]
                        if sh.ShapeType == 'Face':
                            sh = sh.OuterWire
                        elif sh.ShapeType == 'Edge':
                            import Part
                            sh = Part.Wire([sh])
                        elif sh.ShapeType == 'Wire':
                            import Part
                            sh = Part.Wire(sh)
                        elif sh.ShapeType == 'Vertex':
                            pass
                        else:
                            raise ValueError('Unsuitabel Shape Type')
                        sectionname = '%s-%02d-section' % (ob.Name,i)
                        saveShape(self.csg,self.filename, sh,sectionname,None,\
                                self.cleanshape) # placement with shape
                    sectionnames.append(sectionname)
            if ob.Closed:
                sectionnames.append(sectionnames[0])
            self.csg.write('thrusections %s %d %d %s\n' % \
                    (ob.Name,int(ob.Solid),\
                    int(ob.Ruled), ' '.join(sectionnames)))
        elif isDeform(ob): #non-uniform scaling
            if checksupported: return True # The object is supported
            m=ob.Matrix
            self.process_object(ob.Base)
            #csg.write('tcopy %s %s\n'%(ob.Base.Name,d1['name']))
            d1['basename']=ob.Base.Name
            d1['cx']=f2s(m.A11)
            d1['cy']=f2s(m.A22)
            d1['cz']=f2s(m.A33)
            self.csg.write('deform %(name)s %(basename)s %(cx)s %(cy)s %(cz)s\n' % d1)
            if m.A14 > 1e-8 or m.A24 > 1e-8 or m.A34 > 1e-8:
                self.csg.write("ttranslate %s %s %s %s\n" % \
                    (ob.Name,f2s(m.A14),f2s(m.A24),f2s(m.A34)))
        elif isDraftPoint(ob) or ob.TypeId == "Part::Vertex":
            if checksupported: return True # The object is supported
            d1['x']=f2s(ob.X)
            d1['y']=f2s(ob.Y)
            d1['z']=f2s(ob.Z)
            self.csg.write('vertex %(name)s %(x)s %(y)s %(z)s\n' % d1)
        elif isDraftCircle(ob) or ob.TypeId == "Part::Circle" or \
                isDraftEllipse(ob):
            if checksupported: return True # The object is supported
            isdraftcircle=isDraftCircle(ob)
            isdraftellipse=isDraftCircle(ob)
            "circle name x y [z [dx dy dz]] [ux uy [uz]] radius"
            curvename = '%s-curve' % d1['name']
            if ob.TypeId == "Part::Circle":
                radius=f2s(float(ob.Radius))
                pfirst=f2s(ob.Angle0.getValueAs('rad').Value)
                plast=f2s(ob.Angle1.getValueAs('rad').Value)
                self.csg.write('circle %s 0 0 0 %s\n' % (curvename,radius))
                self.csg.write('mkedge %s %s %s %s\n' % \
                    (d1['name'],curvename,pfirst,plast))
            else: #draft
                makeface = ob.MakeFace and \
                    (ob.Shape.isNull() or ob.Shape.ShapeType == 'Face')
                #FreeCAD ignores a failed mkplane but it may
                #break the model in DRAWEXE
                edgename  = '%s-edge' % d1['name']

                if isdraftcircle:
                    pfirst=f2s(ob.FirstAngle.getValueAs('rad').Value)
                    plast=f2s(ob.LastAngle.getValueAs('rad').Value)
                    radius=f2s(ob.Radius.Value)
                    self.csg.write('circle %s 0 0 0 %s\n' % (curvename,radius))
                else: #draft ellipse
                    import math
                    majr=f2s(float(ob.MajorRadius))
                    minr=f2s(float(ob.MinorRadius))
                    pfirst=f2s(math.radians(ob.FirstAngle))
                    plast =f2s(math.radians(ob.LastAngle))
                    self.csg.write('ellipse %s 0 0 0 %s %s\n' % \
                            (curvename,majr,minr))
                self.csg.write('mkedge %s %s %s %s\n' % \
                        (edgename,curvename,pfirst,plast))
                if makeface:
                    wirename = '%s-wire' % d1['name']
                    self.csg.write('wire %s %s\n' %(wirename,edgename))
                    self.csg.write('mkplane %s %s\n' % (d1['name'],wirename))
                else:
                    self.csg.write('wire %s %s\n' %(d1['name'],edgename))
        elif ob.TypeId == "Part::Line":
            if checksupported: return True # The object is supported
            self.csg.write('polyline %s %s %s %s %s %s %s\n' % \
                    (d1['name'],f2s(ob.X1),f2s(ob.Y1),f2s(ob.Z1),\
                    f2s(ob.X2),f2s(ob.Y2),f2s(ob.Z2)))
        elif isDraftWire(ob):
            if checksupported: return True # The object is supported
            points=ob.Points
            if ob.Closed:
                points.append(points[0])
            polyname = '%s-dwireline' % d1['name']
            pointstr=' '.join('%s %s %s'%(f2s(v.x),f2s(v.y),f2s(v.z)) \
                    for v in points)
            self.csg.write('polyline %s %s\n' % (polyname,pointstr))
            if ob.MakeFace:
                wirename = '%s-dwirewire' % d1['name']
                self.csg.write('wire %s %s\n' %(wirename,polyname))
                facename =  d1['name']
                self.csg.write('mkplane %s %s\n' % (facename,polyname))
            else:
                wirename =  d1['name']
                self.csg.write('wire %s %s\n' %(wirename,polyname))
        elif isDraftClone(ob):
            if checksupported: return True # The object is supported
            x,y,z=ob.Scale
            if x == y == z: #uniform scaling
                d1['scale']=f2s(x)
            else:
                d1['cx']=f2s(x)
                d1['cy']=f2s(y)
                d1['cz']=f2s(z)
            if len(ob.Objects) == 1:
                d1['basename']=ob.Objects[0].Name
                self.process_object(ob.Objects[0])
                if x == y == z: #uniform scaling
                    self.csg.write('tcopy %(basename)s %(name)s\n' % d1)
                    self.csg.write('pscale %(name)s 0 0 0 %(scale)s\n' % d1)
                else:
                    self.csg.write('deform %(name)s %(basename)s'\
                            ' %(cx)s %(cy)s %(cz)s\n' % d1)
            else: #compound
                newnames=[]
                for i,subobj in enumerate(ob.Objects):
                    self.process_object(subobj)
                    d1['basename']=subobj.Name
                    newname='%s-%2d' % (ob.Name,i)
                    d1['newname']=newname
                    newnames.append(newname)
                    if x == y == z: #uniform scaling
                        self.csg.write('tcopy %(basename)s %(newname)s\n' % d1)
                        self.csg.write('pscale %(newname)s 0 0 0 %(scale)s\n' % d1)
                    else:
                        self.csg.write('deform %(newname)s %(basename)s'\
                                ' %(cx)s %(cy)s %(cz)s\n' % d1)
                self.csg.write('compound %s %s\n' % (' '.join(newnames),ob.Name))
        elif isDraftShape2DView(ob) and not ob.Tessellation and \
                ob.ProjectionMode == "Solid" and ob.Base is not None and \
                hasattr(ob.Base,'Shape'):
        # not supported are groups, Arch/Sections and individual faces mode
            if checksupported: return True # The object is supported
            self.process_object(ob.Base)
            v=ob.Projection
            x=ax2_xdir(v)
            self.csg.write('hprj %s_proj 0 0 0 %s %s %s %s %s %s\n' % \
                    ( ob.Name,f2s(v.x),f2s(v.y),f2s(v.z)\
                    , f2s(x.x),f2s(x.y),f2s(x.z)))
            self.csg.write('houtl %s_outl %s\n' % (ob.Name, ob.Base.Name))
            self.csg.write('hfill %s_outl %s_proj 0\n' %(ob.Name,ob.Name)) #0?
            self.csg.write('hload %s_outl\n' % (ob.Name))
            self.csg.write('hsetprj %s_proj\n' % (ob.Name))
            self.csg.write('hupdate\n')
            self.csg.write('hhide\n')
            self.csg.write('unset -nocomplain vl v1l vnl vol vil hl h1l hnl hol hil\n')
            self.csg.write('hres2d\n')
            if ob.HiddenLines:
                self.csg.write('compound vl v1l vnl vol vil hl h1l hnl hol hil %s\n' % ob.Name)
            else:
                self.csg.write('compound vl v1l vnl vol vil %s\n' % ob.Name)
        #elif ob.isDerivedFrom('Part::FeaturePython') and \
        #    hasattr(ob.Proxy,'__module__'):
        #    pass
        elif ob.isDerivedFrom('Part::Feature') :
            if ob.Shape.isNull(): #would crash in exportBrep otherwise
                raise ValueError('Shape of %s is Null' % ob.Name)
            if checksupported: return False # The object is not supported
            self.csg.write('#saved shape of unsupported %s Object\n' % \
                    formatobjtype(ob))
            hasplacement = saveShape(self.csg,self.filename,ob.Shape,ob.Name,\
                    hasplacement,self.cleanshape)
        elif ob.isDerivedFrom('App::Annotation') :
            return False # ignored here
            #anntotations needs to be drawn after erase/donly
        else: # not derived from Part::Feature
            if not toplevel:
                raise ValueError('Can not export child object')
            else:
                if ob.Name != ob.Label:
                    labelstr = 'Label %s' % ob.Label.encode('unicode-escape')
                else:
                    labelstr = ''
                self.csg.write('#omitted unsupported %s Object %s%s\n' %\
                        (formatobjtype(ob),ob.Name,labelstr))
                self.csg.write('#Properties: %s\n' % \
                        ','.join(ob.PropertiesList))
                return False
                #The object is not present and can not be referenced
        if hasplacement:
            self.csg.write(placement2draw(ob.Placement,ob.Name))
        if ob.Name != ob.Label:
            self.csg.write('#Object Label: %s\n' % ob.Label.encode('unicode-escape'))
        return ob.Name #The object is present and can be referenced

    def export_annotations(self,objlst):
        for ob in objlst:
            if ob.isDerivedFrom('App::Annotation') :
                if ob.Name != ob.Label:
                    self.csg.write('#Annotation Name %s Label %s"\n' % \
                            (ob.Name,ob.Label.encode('unicode-escape')))
                else:
                    self.csg.write('#Annotation %s\n' % (ob.Name))
                v=ob.Position
                self.csg.write('dtext %s %s %s "%s"\n' % \
                        (f2s(v.x),f2s(v.y),f2s(v.z), '\\n'.join(\
                        ob.LabelText).encode(\
                        'ascii', errors='xmlcharrefreplace')))

    def export_objects(self,objlst,toplevel=True):
        self.write_header()
        toplevelobjs = [self.process_object(ob, toplevel=toplevel)\
                for ob in objlst]
        names = [name for name in toplevelobjs if name is not False]
        self.csg.write('donly %s\n'%(' '.join(names)))
        self.export_annotations(objlst)
        #for ob in objlst:
        #    self.process_object(ob,toplevel=toplevel)
        #self.write_displayonly(objlst)

    def __exit__(self,exc_type, exc_val, exc_tb ):
        self.csg.close()

def export(exportList,filename):
    "called when freecad exports a file"
    with Drawexporter(filename) as exporter:
        exporter.export_objects(exportList)

if 'tcl' not in FreeCAD.getExportType():
    FreeCAD.addExportType("DRAWEXE script (*.tcl)","exportDRAWEXE")
