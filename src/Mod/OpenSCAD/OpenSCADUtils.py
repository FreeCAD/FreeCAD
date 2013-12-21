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

__title__="FreeCAD OpenSCAD Workbench - Utility Fuctions"
__author__ = "Sebastian Hoogen"
__url__ = ["http://www.freecadweb.org"]

'''
This Script includes various pyhton helper functions that are shared across
the module
'''

class OpenSCADError(Exception):
    def __init__(self,value):
        self.value= value
    #def __repr__(self):
    #    return self.msg
    def __str__(self):
        return repr(self.value)

def searchforopenscadexe():
    import os,sys,subprocess
    if sys.platform == 'win32':
        testpaths = [os.path.join(os.environ.get('Programfiles(x86)','C:'),\
            'OpenSCAD\\openscad.exe')]
        if 'ProgramW6432' in os.environ:
            testpaths.append(os.path.join(os.environ.get('ProgramW6432','C:')\
                ,'OpenSCAD\\openscad.exe'))
        for testpath in testpaths:
            if os.path.isfile(testpath):
                return testpath
    else: #unix
        p1=subprocess.Popen(['which','openscad'],stdout=subprocess.PIPE)
        if p1.wait() == 0:
            opath=p1.stdout.read().split('\n')[0]
            return opath

def workaroundforissue128needed():
    '''sets the import path depending on the OpenSCAD Verion
    for versions <= 2012.06.23 to the current working dir
    for versions above to the inputfile dir
    see https://github.com/openscad/openscad/issues/128'''
    vdate=getopenscadversion().split(' ')[2].split('.')
    year,mon=int(vdate[0]),int(vdate[1])
    return (year<2012 or (year==2012 and (mon <6 or (mon == 6 and \
        (len(vdate)<3 or int(vdate[2]) <=23)))))
    #ifdate=int(vdate[0])+(int(vdate[1])-1)/12.0
    #if len(vdate)>2:
    #    fdate+=int((vdate[2])-1)/12.0/31.0
    #return fdate < 2012.4759

def getopenscadversion(osfilename=None):
    import os,subprocess,time
    if not osfilename:
        import FreeCAD
        osfilename = FreeCAD.ParamGet(\
            "User parameter:BaseApp/Preferences/Mod/OpenSCAD").\
            GetString('openscadexecutable')
    if osfilename and os.path.isfile(osfilename):
        p=subprocess.Popen([osfilename,'-v'],\
            stdout=subprocess.PIPE,universal_newlines=True)
        p.wait()
        return p.stdout.read().strip()

def newtempfilename():
    import os,time
    formatstr='fc-%05d-%06d-%06d'
    count = 0
    while True:
        count+=1
        yield formatstr % (os.getpid(),int(time.time()*100) % 1000000,count)

tempfilenamegen=newtempfilename()

def callopenscad(inputfilename,outputfilename=None,outputext='csg',keepname=False):
    '''call the open scad binary
    returns the filename of the result (or None),
    please delete the file afterwards'''
    import FreeCAD,os,subprocess,tempfile,time
    def check_output2(*args,**kwargs):
        kwargs.update({'stdout':subprocess.PIPE})
        p=subprocess.Popen(*args,**kwargs)
        stdoutd,stderrd = p.communicate()
        if p.returncode != 0:
            raise OpenSCADError('%s\n' % stdoutd.strip())
            #raise Exception,'stdout %s\n stderr%s' %(stdoutd,stderrd)
        if stdoutd.strip():
            FreeCAD.Console.PrintWarning(stdoutd+u'\n')
            return stdoutd

    osfilename = FreeCAD.ParamGet(\
        "User parameter:BaseApp/Preferences/Mod/OpenSCAD").\
        GetString('openscadexecutable')
    if osfilename and os.path.isfile(osfilename):
        if not outputfilename:
            dir1=tempfile.gettempdir()
            if keepname:
                outputfilename=os.path.join(dir1,'%s.%s' % (os.path.split(\
                    inputfilename)[1].rsplit('.',1)[0],outputext))
            else:
                outputfilename=os.path.join(dir1,'%s.%s' % \
                    (tempfilenamegen.next(),outputext))
        check_output2([osfilename,'-o',outputfilename, inputfilename],\
            stderr=subprocess.STDOUT)
        return outputfilename

def callopenscadstring(scadstr,outputext='csg'):
    '''create a tempfile and call the open scad binary
    returns the filename of the result (or None),
    please delete the file afterwards'''
    import os,tempfile,time
    dir1=tempfile.gettempdir()
    inputfilename=os.path.join(dir1,'%s.scad' % tempfilenamegen.next())
    inputfile = open(inputfilename,'w')
    inputfile.write(scadstr)
    inputfile.close()
    outputfilename = callopenscad(inputfilename,outputext=outputext,\
        keepname=True)
    os.unlink(inputfilename)
    return outputfilename

def reverseimporttypes():
    '''allows to search for supported filetypes by module'''

    def getsetfromdict(dict1,index):
        if index in dict1:
            return dict1[index]
        else:
            set1=set()
            dict1[index]=set1
            return set1

    importtypes={}
    import FreeCAD
    for key,value in FreeCAD.getImportType().iteritems():
        if type(value) is str:
            getsetfromdict(importtypes,value).add(key)
        else:
            for vitem in value:
                getsetfromdict(importtypes,vitem).add(key)
    return importtypes


def fcsubmatrix(m):
    """Extracts the 3x3 Submatrix from a freecad Matrix Object
    as a list of row vectors"""
    return [[m.A11,m.A12,m.A13],[m.A21,m.A22,m.A23],[m.A31,m.A32,m.A33]]

def multiplymat(l,r):
    """multiply matrices given as lists of row vectors"""
    rt=zip(*r) #transpose r
    mat=[]
    for y in range(len(rt)):
        mline=[]
        for x in range(len(l)):
            mline.append(sum([le*re for le,re in zip(l[y],rt[x])]))
        mat.append(mline)
    return mat

def isorthogonal(submatrix,precision=4):
    """checking if 3x3 Matrix is ortogonal (M*Transp(M)==I)"""
    prod=multiplymat(submatrix,zip(*submatrix))
    return [[round(f,precision) for f in line] for line in prod]==[[1,0,0],[0,1,0],[0,0,1]]

def detsubmatrix(s):
    """get the determinant of a 3x3 Matrix given as list of row vectors"""
    return s[0][0]*s[1][1]*s[2][2]+s[0][1]*s[1][2]*s[2][0]+s[0][2]*s[1][0]*s[2][1]\
          -s[2][0]*s[1][1]*s[0][2]-s[2][1]*s[1][2]*s[0][0]-s[2][2]*s[1][0]*s[0][1]

def isspecialorthogonalpython(submat,precision=4):
    return isorthogonal(submat,precision) and round(detsubmatrix(submat),precision)==1

def isspecialorthogonal(mat,precision=4):
    return abs(mat.submatrix(3).isOrthogonal(10**(-precision))-1.0) < 10**(-precision) and \
            abs(mat.submatrix(3).determinant()-1.0) < 10**(-precision)

def callopenscadmeshstring(scadstr):
    """Call OpenSCAD and return the result as a Mesh"""
    import Mesh,os
    tmpfilename=callopenscadstring(scadstr,'stl')
    newmesh=Mesh.Mesh()
    newmesh.read(tmpfilename)
    try:
        os.unlink(tmpfilename)
    except OSError:
        pass
    return newmesh

def meshopinline(opname,iterable1):
    """uses OpenSCAD to combine meshes
    takes the name of the CGAL operation and an iterable (tuple,list) of 
    FreeCAD Mesh objects
    includes all the mesh data in the SCAD file
    """
    from exportCSG import mesh2polyhedron
    return callopenscadmeshstring('%s(){%s}' % (opname,' '.join(\
        (mesh2polyhedron(meshobj) for meshobj in iterable1))))

def meshoptempfile(opname,iterable1):
    """uses OpenSCAD to combine meshes
    takes the name of the CGAL operation and an iterable (tuple,list) of 
    FreeCAD Mesh objects
    uses stl files to supply the mesh data
    """
    import os,tempfile
    dir1=tempfile.gettempdir()
    filenames = []
    for mesh in iterable1:
        outputfilename=os.path.join(dir1,'%s.stl' % tempfilenamegen.next())
        mesh.write(outputfilename)
        filenames.append(outputfilename)
    #absolute path causes error. We rely that the scad file will be in the dame tmpdir
    meshimports = ' '.join("import(file = \"%s\");" % \
        #filename \
        os.path.split(filename)[1] for filename in filenames)
    result = callopenscadmeshstring('%s(){%s}' % (opname,meshimports))
    for filename in filenames:
        try:
            os.unlink(filename)
        except OSError:
            pass
    return result

def meshoponobjs(opname,inobjs):
    """
    takes a string (operation name) and a list of Feature Objects
    returns a mesh and a list of objects that were used
    Part Objects will be meshed
    """
    objs=[]
    meshes=[]
    for obj in inobjs:
        if obj.isDerivedFrom('Mesh::Feature'):
            objs.append(obj)
            meshes.append(obj.Mesh)
        elif obj.isDerivedFrom('Part::Feature'):
            #mesh the shape
            import FreeCAD
            params = FreeCAD.ParamGet("User parameter:BaseApp/Preferences/Mod/OpenSCAD")
            objs.append(obj)
            if False: # disabled due to issue 1292
                import MeshPart
                meshes.append(MeshPart.meshFromShape(obj.Shape,params.GetFloat(\
                'meshmaxlength',1.0), params.GetFloat('meshmaxarea',0.0),\
                 params.GetFloat('meshlocallen',0.0),\
                 params.GetFloat('meshdeflection',0.0)))
            else:
                import Mesh
                meshes.append(Mesh.Mesh(obj.Shape.tessellate(params.GetFloat(\
                            'meshmaxlength',1.0))))
        else:
            pass #neither a mesh nor a part
    if len(objs) > 0:
            return (meshoptempfile(opname,meshes),objs)
    else:
            return (None,[])

def process2D_ObjectsViaOpenSCAD(ObjList,Operation,doc=None):
    import FreeCAD,importDXF
    import os,tempfile
    #print "process2D"
    doc = doc or FreeCAD.activeDocument()
    dir1=tempfile.gettempdir()
    filenames = []
    #print "Export DXF"
    for item in ObjList :
        outputfilename=os.path.join(dir1,'%s.dxf' % tempfilenamegen.next())
        #print "Call Export : "+outputfilename
        importDXF.export([item],outputfilename,True,True)
        #print "File Exported"
        filenames.append(outputfilename)
    dxfimports = ' '.join("import(file = \"%s\");" % \
        #filename \
        os.path.split(filename)[1] for filename in filenames)
    #print "Call OpenSCAD : "+dxfimports
    tmpfilename = callopenscadstring('%s(){%s}' % (Operation,dxfimports),'dxf')
    #from importCSG import processDXF #import the result
    #obj = processDXF(tmpfilename,None)
    from OpenSCAD2Dgeom import importDXFface
    #print "Import DXF"
    face = importDXFface(tmpfilename,None,None)
    #print "Add Hull"
    obj=doc.addObject('Part::Feature',Operation)
    obj.Shape=face
    # Hide Children
    if FreeCAD.GuiUp:
       for index in ObjList :
          index.ViewObject.hide()
    #clean up
    filenames.append(tmpfilename) #delete the ouptut file as well
    try:
        os.unlink(tmpfilename)
    except OSError:
        pass
    return(obj)

def process3D_ObjectsViaOpenSCAD(doc,ObjList,Operation):
    import FreeCAD,Mesh,Part
    params = FreeCAD.ParamGet("User parameter:BaseApp/Preferences/Mod/OpenSCAD")
    if False: # disabled due to issue 1292
        import MeshPart
        meshes = [MeshPart.meshFromShape(obj.Shape,params.GetFloat(\
                'meshmaxlength',1.0), params.GetFloat('meshmaxarea',0.0),\
                 params.GetFloat('meshlocallen',0.0),\
                 params.GetFloat('meshdeflection',0.0)) for obj in ObjList]
    else:
        meshes = [Mesh.Mesh(obj.Shape.tessellate(params.GetFloat(\
                            'meshmaxlength',1.0))) for obj in ObjList]
    if max(mesh.CountPoints for mesh in meshes) < \
            params.GetInt('tempmeshmaxpoints',5000):
        stlmesh = meshoptempfile(Operation,meshes)
        sh=Part.Shape()
        sh.makeShapeFromMesh(stlmesh.Topology,0.1)
        solid = Part.Solid(sh)
        obj=doc.addObject('Part::Feature',Operation) #non parametric objec
        solid=solid.removeSplitter()
        if solid.Volume < 0:
           solid.complement()
        obj.Shape=solid#.removeSplitter()
        if FreeCAD.GuiUp:
          for index in ObjList :
              index.ViewObject.hide()
        return(obj)

def process_ObjectsViaOpenSCAD(doc,children,name):
    if all(obj.Shape.Volume == 0 for obj in children):
        return process2D_ObjectsViaOpenSCAD(children,name)
    elif all(obj.Shape.Volume > 0 for obj in children):
        return process3D_ObjectsViaOpenSCAD(doc,children,name)
    else:
        FreeCAD.Console.PrintError( unicode(translate('OpenSCAD',\
            "Error Both shapes must be either 2D or both must be 3D"))+u'\n')
