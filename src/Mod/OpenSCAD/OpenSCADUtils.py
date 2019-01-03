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

__title__="FreeCAD OpenSCAD Workbench - Utility Functions"
__author__ = "Sebastian Hoogen"
__url__ = ["http://www.freecadweb.org"]

'''
This Script includes various python helper functions that are shared across
the module
'''

try:
    from PySide import QtGui
    _encoding = QtGui.QApplication.UnicodeUTF8
    def translate(context, text):
        "convenience function for Qt translator"
        return QtGui.QApplication.translate(context, text, None, _encoding)
except AttributeError:
    def translate(context, text):
        "convenience function for Qt translator"
        from PySide import QtGui
        return QtGui.QApplication.translate(context, text, None)

import io

try:
    import FreeCAD
    BaseError = FreeCAD.Base.FreeCADError
except (ImportError, AttributeError):
    BaseError = RuntimeError

class OpenSCADError(BaseError):
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
    elif sys.platform == 'darwin':
        ascript = (b'tell application "Finder"\n'
        b'POSIX path of (application file id "org.openscad.OpenSCAD"'
        b'as alias)\n'
        b'end tell')
        p1=subprocess.Popen(['osascript','-'],stdin=subprocess.PIPE,\
                stdout=subprocess.PIPE,stderr=subprocess.PIPE)
        stdout,stderr = p1.communicate(ascript)
        if p1.returncode == 0:
            opathl=stdout.split('\n')
            if len(opathl) >=1:
                return opathl[0]+'Contents/MacOS/OpenSCAD'
        #test the default path
        testpath="/Applications/OpenSCAD.app/Contents/MacOS/OpenSCAD"
        if os.path.isfile(testpath):
            return testpath
    else: #unix
        p1=subprocess.Popen(['which','openscad'],stdout=subprocess.PIPE)
        if p1.wait() == 0:
            output = p1.stdout.read()
            if sys.version_info.major >= 3:
                output = output.decode("utf-8")
            opath = output.split('\n')[0]
            return opath

def workaroundforissue128needed():
    '''sets the import path depending on the OpenSCAD Version
    for versions <= 2012.06.23 to the current working dir
    for versions above to the inputfile dir
    see https://github.com/openscad/openscad/issues/128'''
    vdate=getopenscadversion().split('-')[0]
    vdate=vdate.split(' ')[2].split('.')
    if len(vdate) == 1: # probably YYYYMMDD format (i.e. git version)
        vdate = vdate[0]
        year, mon = int("".join(vdate[0:4])), int("".join(vdate[4:6]))
    else: # YYYY.MM(.DD?) (latest release)
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
            stdout=subprocess.PIPE,stderr=subprocess.PIPE,universal_newlines=True)
        p.wait()
        stdout=p.stdout.read().strip()
        stderr=p.stderr.read().strip()
        return (stdout or stderr)

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
        kwargs.update({'stdout':subprocess.PIPE,'stderr':subprocess.PIPE})
        p=subprocess.Popen(*args,**kwargs)
        stdoutd,stderrd = p.communicate()
        stdoutd = stdoutd.decode("utf8")
        stderrd = stderrd.decode("utf8")
        if p.returncode != 0:
            raise OpenSCADError('%s %s\n' % (stdoutd.strip(),stderrd.strip()))
            #raise Exception,'stdout %s\n stderr%s' %(stdoutd,stderrd)
        if stderrd.strip():
            FreeCAD.Console.PrintWarning(stderrd+u'\n')
        if stdoutd.strip():
            FreeCAD.Console.PrintMessage(stdoutd+u'\n')
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
                    (next(tempfilenamegen),outputext))
        check_output2([osfilename,'-o',outputfilename, inputfilename])
        return outputfilename
    else:
        raise OpenSCADError('OpenSCAD executable unavailable')

def callopenscadstring(scadstr,outputext='csg'):
    '''create a tempfile and call the open scad binary
    returns the filename of the result (or None),
    please delete the file afterwards'''
    import os,tempfile,time
    dir1=tempfile.gettempdir()
    inputfilename=os.path.join(dir1,'%s.scad' % next(tempfilenamegen))
    inputfile = io.open(inputfilename,'w', encoding="utf8")
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
    for key,value in FreeCAD.getImportType().items():
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
    rt=list(rt)
    mat=[]
    for y in range(len(rt)):
        mline=[]
        for x in range(len(l)):
            mline.append(sum([le*re for le,re in zip(l[y],rt[x])]))
        mat.append(mline)
    return mat

def isorthogonal(submatrix,precision=4):
    """checking if 3x3 Matrix is orthogonal (M*Transp(M)==I)"""
    prod=multiplymat(submatrix,list(zip(*submatrix)))
    return [[round(f,precision) for f in line] \
        for line in prod]==[[1,0,0],[0,1,0],[0,0,1]]

def detsubmatrix(s):
    """get the determinant of a 3x3 Matrix given as list of row vectors"""
    return s[0][0]*s[1][1]*s[2][2]+s[0][1]*s[1][2]*s[2][0]+\
           s[0][2]*s[1][0]*s[2][1]-s[2][0]*s[1][1]*s[0][2]-\
           s[2][1]*s[1][2]*s[0][0]-s[2][2]*s[1][0]*s[0][1]

def isspecialorthogonalpython(submat,precision=4):
    return isorthogonal(submat,precision) and round(detsubmatrix(submat),precision)==1

def isrotoinversionpython(submat,precision=4):
    return isorthogonal(submat,precision) and round(detsubmatrix(submat),precision)==-1

def isspecialorthogonal(mat,precision=4):
    return abs(mat.submatrix(3).isOrthogonal(10**(-precision))-1.0) < \
            10**(-precision) and \
            abs(mat.submatrix(3).determinant()-1.0) < 10**(-precision)

def decomposerotoinversion(m,precision=4):
    import FreeCAD
    rmat = [[round(f,precision) for f in line] for line in fcsubmatrix(m)]
    cmat = FreeCAD.Matrix()
    if rmat ==[[-1,0,0],[0,1,0],[0,0,1]]:
        cmat.scale(-1,1,1)
        return m*cmat,FreeCAD.Vector(1)
    elif rmat ==[[1,0,0],[0,-1,0],[0,0,1]]:
        cmat.scale(1,-1,1)
        return m*cmat, FreeCAD.Vector(0,1)
    elif rmat ==[[1,0,0],[0,1,0],[0,0,-1]]:
        cmat.scale(1,1,-1)
        return m*cmat, FreeCAD.Vector(0,0,1)
    else:
        cmat.scale(1,1,-1)
        return m*cmat, FreeCAD.Vector(0,0,1)

def mirror2mat(nv,bv):
    import FreeCAD
    """calculate the transformation matrix of a mirror feature"""
    mbef=FreeCAD.Matrix()
    mbef.move(bv * -1)
    maft=FreeCAD.Matrix()
    maft.move(bv)
    return maft*vec2householder(nv)*mbef

def vec2householder(nv):
    """calculated the householder matrix for a given normal vector"""
    import FreeCAD
    lnv=nv.dot(nv)
    l=2/lnv if lnv > 0 else 0
    hh=FreeCAD.Matrix(nv.x*nv.x*l,nv.x*nv.y*l,nv.x*nv.z*l,0,\
                      nv.y*nv.x*l,nv.y*nv.y*l,nv.y*nv.z*l,0,\
                      nv.z*nv.x*l,nv.z*nv.y*l,nv.z*nv.z*l,0,0,0,0,0)
    return FreeCAD.Matrix()-hh


def angneg(d):
    return d if (d <= 180.0) else (d-360)

def shorthexfloat(f):
    s=f.hex()
    mantisse, exponent = f.hex().split('p',1)
    return '%sp%s' % (mantisse.rstrip('0'),exponent)


def comparerotations(r1,r2):
    import FreeCAD
    '''compares two rotations
    a value of zero means that they are identical'''
    r2c=FreeCAD.Rotation(r2)
    r2c.invert()
    return r1.multiply(r2c).Angle

def findbestmatchingrotation(r1):
    import FreeCAD
    vangl = \
(1.0, 2.0, 3.0, 4.0, 5.0, 6.0, 7.0, 8.0, 9.0, 10.0, 11.0, 11.25, 12.0, 13.0,
14.0, 15.0, 16.0, (180.0/11.0), 17.0, 18.0, 19.0, 20.0, 21.0, 22.0, 22.5,
23.0, 24.0, 25.0, 26.0, 27.0, 28.0, 29.0, 30.0, 31.0, 32.0, (360.0/11.0),
33.0, 33.75, 34.0, 35.0, 36.0, 37.0, 38.0, 39.0, 40.0, 41.0, 42.0, 43.0,
44.0, 45.0, 46.0, 47.0, 48.0, 49.0,(540.0/11.0), 50.0, 51.0, (360.0/7.0),
52.0, 53.0, 54.0, 55.0, 56.0, 56.25, 57.0, 58.0, 59.0, 60.0, 61.0, 62.0,
63.0, 64.0, 65.0,(720.0/11.0), 66.0, 67.0, 67.5, 68.0, 69.0, 70.0, 71.0,
72.0, 73.0, 74.0, 75.0, 76.0, 77.0, 78.0, 78.75, 79.0, 80.0, 81.0,(900.0/11.0),
82.0, 83.0, 84.0, 85.0, 86.0, 87.0, 88.0, 89.0, 90.0, 91.0, 92.0, 93.0, 94.0,
95.0, 96.0, 97.0, 98.0,(1080.0/11.0), 99.0, 100.0, 101.0, 101.25, 102.0,
(720.0/7.0), 103.0, 104.0, 105.0, 106.0, 107.0, 108.0, 109.0, 110.0, 111.0,
112.0, 112.5, 113.0, 114.0, (1260.0/11), 115.0, 116.0, 117.0, 118.0, 119.0,
120.0, 121.0, 122.0, 123.0, 123.75, 124.0, 125.0, 126.0, 127.0, 128.0,
 129.0, 130.0,(1440.0/11.0), 131.0, 132.0, 133.0, 134.0, 135.0, 136.0,
137.0, 138.0, 139.0, 140.0, 141.0, 142.0, 143.0, 144.0, 145.0, 146.0, 146.25,
147.0, (1620.0/11.0), 148.0, 149.0, 150.0, 151.0, 152.0, 153.0, 154.0,
(1080.0/7.0), 155.0, 156.0, 157.0, 157.5, 158.0, 159.0, 160.0, 161.0, 162.0,
163.0, (1800.0/11.0), 164.0, 165.0, 166.0, 167.0, 168.0, 168.75, 169.0, 170.0,
171.0, 172.0, 173.0, 174.0, 175.0, 176.0, 177.0,178.0, 179.0,180.0,
-179.0, -178.0, -177.0, -176.0, -175.0, -174.0, -173.0, -172.0, -171.0, -170.0,
-169.0, -168.75, -168.0, -167.0, -166.0, -165.0, -164.0, (-1800.0/11.0),
-163.0, -162.0, -161.0, -160.0, -159.0, -158.0, -157.5, -157.0, -156.0,
-155.0, (-1080.0/7.0), -154.0, -153.0, -152.0, -151.0, -150.0, -149.0, -148.0,
(-1620.0/11.0), -147.0, -146.25, -146.0, -145.0, -144.0, -143.0, -142.0,
-141.0, -140.0, -139.0,-138.0, -137.0, -136.0, -135.0, -134.0, -133.0, -132.0,
 -131.0, (-1440/11.0), -130.0, -129.0, -128.0,-127.0, -126.0, -125.0, -124.0,
 -123.75, -123.0, -122.0, -121.0, -120.0, -119.0, -118.0, -117.0, -116.0,
-115.0,(-1260.0/11.0), -114.0, -113.0, -112.5, -112.0, -111.0, -110.0, -109.0,
-108.0, -107.0, -106.0, -105.0,-104.0, -103.0,(-720.0/7.0), -102.0, -101.25,
-101.0, -100.0, -99.0, (-1080.0/11.0), -98.0, -97.0, -96.0, -95.0, -94.0,
-93.0, -92.0, -91.0, -90.0, -89.0, -88.0, -87.0, -86.0, -85.0, -84.0, -83.0,
-82.0,(-900.0/11.0), -81.0, -80.0, -79.0, -78.75, -78.0, -77.0, -76.0, -75.0,
-74.0, -73.0, -72.0, -71.0, -70.0, -69.0, -68.0, -67.5, -67.0, -66.0,
(-720.0/11.0), -65.0, -64.0, -63.0, -62.0, -61.0, -60.0, -59.0, -58.0, -57.0,
-56.25, -56.0, -55.0, -54.0, -53.0, -52.0,(-360.0/7.0), -51.0, -50.0,
(-540.0/11.0), -49.0, -48.0, -47.0, -46.0, -45.0, -44.0, -43.0, -42.0, -41.0,
-40.0, -39.0, -38.0, -37.0, -36.0, -35.0, -34.0, -33.75, -33.0,(-360.0/11.0),
-32.0, -31.0, -30.0, -29.0, -28.0, -27.0, -26.0, -25.0, -24.0, -23.0, -22.5,
-22.0, -21.0, -20.0, -19.0, -18.0, -17.0,(-180.0/11.0), -16.0, -15.0, -14.0,
-13.0, -12.0, -11.25, -11.0, -10.0, -9.0, -8.0, -7.0, -6.0, -5.0, -4.0, -3.0,
-2.0, -1.0)
    def tup2nvect(tup):
        """convert a tuple to a normalized vector"""
        v=FreeCAD.Vector(*tup)
        v.normalize()
        return v

    def wkaxes():
        """well known axes for rotations"""
        vtupl=((1,0,0),(0,1,0),(0,0,1),
            (1,1,0),(1,0,1),(0,1,1),(-1,1,0),(-1,0,1),(0,1,-1),
            (1,1,1),(1,1,-1),(1,-1,1),(-1,1,1))
        return tuple(tup2nvect(tup) for tup in vtupl)

    bestrot=FreeCAD.Rotation()
    dangle = comparerotations(r1,bestrot)
    for axis in wkaxes():
        for angle in vangl:
            for axissign in (1.0,-1.0):
                r2=FreeCAD.Rotation(axis*axissign,angle)
                dangletest = comparerotations(r1,r2)
                if dangletest < dangle:
                    bestrot = r2
                    dangle = dangletest
    return (bestrot,dangle)

def roundrotation(rot,maxangulardistance=1e-5):
    '''guess the rotation axis and angle for a rotation
    recreated from rounded floating point values
    (from a quaterion or transformation matrix)'''
    def teststandardrot(r1,maxangulardistance=1e-5):
        '''test a few common rotations beforehand'''
        import FreeCAD,itertools
        eulers = []
        for angle in (90,-90,180,45,-45,135,-135):
            for euler in itertools.permutations((0,0,angle)):
                eulers.append(euler)
        for euler in itertools.product((0,45,90,135,180,-45,-90,-135),repeat=3):
                eulers.append(euler)
        for euler in eulers:
            r2 = FreeCAD.Rotation(*euler)
            if comparerotations(r1,r2) < maxangulardistance:
                return r2

    if rot.isNull():
        return rot
    firstguess = teststandardrot(rot,maxangulardistance)
    if firstguess is not None:
        return firstguess
    #brute force
    bestguess,angulardistance = findbestmatchingrotation(rot)
    if angulardistance < maxangulardistance: #use guess
        return bestguess
    else: #use original
        return rot

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
        outputfilename=os.path.join(dir1,'%s.stl' % next(tempfilenamegen))
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

def process2D_ObjectsViaOpenSCADShape(ObjList,Operation,doc):
    import FreeCAD,importDXF
    import os,tempfile
    # Mantis 3419
    params = FreeCAD.ParamGet("User parameter:BaseApp/Preferences/Mod/OpenSCAD")
    fn  = params.GetInt('fnForImport',32)
    fnStr = ",$fn=" + str(fn)
    #
    dir1=tempfile.gettempdir()
    filenames = []
    for item in ObjList :
        outputfilename=os.path.join(dir1,'%s.dxf' % next(tempfilenamegen))
        importDXF.export([item],outputfilename,True,True)
        filenames.append(outputfilename)
    # Mantis 3419
    dxfimports = ' '.join("import(file = \"%s\" %s);" % \
        #filename \
        (os.path.split(filename)[1], fnStr) for filename in filenames)
    #
    tmpfilename = callopenscadstring('%s(){%s}' % (Operation,dxfimports),'dxf')
    from OpenSCAD2Dgeom import importDXFface
    # TBD: assure the given doc is active
    face = importDXFface(tmpfilename,None,None)
    #clean up
    filenames.append(tmpfilename) #delete the output file as well
    try:
        os.unlink(tmpfilename)
    except OSError:
        pass
    return face

def process2D_ObjectsViaOpenSCAD(ObjList,Operation,doc=None):
    import FreeCAD
    doc = doc or FreeCAD.activeDocument()
    face=process2D_ObjectsViaOpenSCADShape(ObjList,Operation,doc)
    obj=doc.addObject('Part::Feature',Operation)
    obj.Shape=face
    # Hide Children
    if FreeCAD.GuiUp:
       for index in ObjList :
          index.ViewObject.hide()
    return(obj)

def process3D_ObjectsViaOpenSCADShape(ObjList,Operation,maxmeshpoints=None):
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
            (maxmeshpoints or params.GetInt('tempmeshmaxpoints',5000)):
        stlmesh = meshoptempfile(Operation,meshes)
        sh=Part.Shape()
        sh.makeShapeFromMesh(stlmesh.Topology,0.1)
        solid = Part.Solid(sh)
        solid=solid.removeSplitter()
        if solid.Volume < 0:
           solid.complement()
        return solid

def process3D_ObjectsViaOpenSCAD(doc,ObjList,Operation):
    solid = process3D_ObjectsViaOpenSCADShape(ObjList,Operation)
    if solid is not None:
        obj=doc.addObject('Part::Feature',Operation) #non parametric objec
        obj.Shape=solid#.removeSplitter()
        if FreeCAD.GuiUp:
          for index in ObjList :
              index.ViewObject.hide()
        return(obj)

def process_ObjectsViaOpenSCADShape(doc,children,name,maxmeshpoints=None):
    if all((not obj.Shape.isNull() and obj.Shape.Volume == 0) \
            for obj in children):
        return process2D_ObjectsViaOpenSCADShape(children,name,doc)
    elif all((not obj.Shape.isNull() and obj.Shape.Volume > 0) \
            for obj in children):
        return process3D_ObjectsViaOpenSCADShape(children,name,maxmeshpoints)
    else:
        import FreeCAD
        FreeCAD.Console.PrintError( translate('OpenSCAD',\
            "Error all shapes must be either 2D or both must be 3D")+u'\n')

def process_ObjectsViaOpenSCAD(doc,children,name):
    if all((not obj.Shape.isNull() and obj.Shape.Volume == 0) \
            for obj in children):
        return process2D_ObjectsViaOpenSCAD(children,name,doc)
    elif all((not obj.Shape.isNull() and obj.Shape.Volume > 0) \
            for obj in children):
        return process3D_ObjectsViaOpenSCAD(doc,children,name)
    else:
        import FreeCAD
        FreeCAD.Console.PrintError( translate('OpenSCAD',\
            "Error all shapes must be either 2D or both must be 3D")+u'\n')

def removesubtree(objs):
    def addsubobjs(obj,toremoveset):
        toremove.add(obj)
        for subobj in obj.OutList:
            addsubobjs(subobj,toremoveset)

    import FreeCAD
    toremove=set()
    for obj in objs:
        addsubobjs(obj,toremove)
    checkinlistcomplete =False
    while not checkinlistcomplete:
        for obj in toremove:
            if (obj not in objs) and (frozenset(obj.InList) - toremove):
                toremove.remove(obj)
                break
        else:
            checkinlistcomplete = True
    for obj in toremove:
        obj.Document.removeObject(obj.Name)

def applyPlacement(shape):
    if shape.Placement.isNull():
        return shape
    else:
        import Part
        if shape.ShapeType == 'Solid':
            return Part.Solid(shape.childShapes()[0])
        elif shape.ShapeType == 'Face':
            return Part.Face(shape.childShapes())
        elif shape.ShapeType == 'Compound':
            return Part.Compound(shape.childShapes())
        elif shape.ShapeType == 'Wire':
            return Part.Wire(shape.childShapes())
        elif shape.ShapeType == 'Shell':
            return Part.Shell(shape.childShapes())
        else:
            return Part.Compound([shape])
