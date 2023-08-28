# Script to create a Menger sponge
# (c) 2012 Werner Mayer LGPL

# The script is based on the work of daxmick at
# http://forum.freecad.org/viewtopic.php?f=3&t=2307

import threading
import Mesh

# Create a global mesh and make copies of them
# This makes the algorithm faster by ~60%.
box = Mesh.createBox(1,1,1)

# Create a Box and Place it a coords (x,y,z)
def PlaceBox(x,y,z):
   global box
   mbox=box.copy()
   mbox.translate(x,y,z)
   return mbox

def Sierpinski(level,x0,y0,z0):
   #print(threading.current_thread().name)
   boxnums = pow(3,level)
   thirds = boxnums / 3
   twothirds = thirds * 2
   if(level == 0):
      rangerx = [x0]
      rangery = [y0]
      rangerz = [z0]
   else:
      rangerx = [ x0, x0 + thirds, x0 + twothirds ]
      rangery = [ y0, y0 + thirds, y0 + twothirds ]
      rangerz = [ z0, z0 + thirds, z0 + twothirds ]
   block = 1
   skip=[5,11,13,14,15,17,23]
   mesh=Mesh.Mesh()
   for i in rangerx:
      for j in rangery:
         for k in rangerz:
            if block not in skip:
               if(level > 0):
                  mesh.addMesh(Sierpinski(level-1,i,j,k))
               else:
                  mesh.addMesh(PlaceBox(i,j,k))
            block+=1
   return mesh

### Multi-threaded ###

class MengerThread(threading.Thread):
    def __init__(self,args):
        self.args=args
        self.mesh=Mesh.Mesh()
        threading.Thread.__init__(self)
    def run(self):
        for i in self.args:
            self.mesh.addMesh(Sierpinski(*i))

def makeMengerSponge_mt(level=3,x0=0,y0=0,z0=0):
    """
    Is much slower than makeMengerSponge!!! :(
    """
    if level == 0:
        mesh=Sierpinski(level,x0,y0,z0)
        Mesh.show(mesh)
        return

    boxnums = pow(3,level)
    thirds = boxnums / 3
    twothirds = thirds * 2

    rangerx = [ x0, x0 + thirds, x0 + twothirds ]
    rangery = [ y0, y0 + thirds, y0 + twothirds ]
    rangerz = [ z0, z0 + thirds, z0 + twothirds ]
    block = 1
    skip=[5,11,13,14,15,17,23]

    # collect the arguments for the algorithm in a list
    args=[]
    for i in rangerx:
        for j in rangery:
            for k in rangerz:
                if block not in skip:
                    args.append((level-1,i,j,k))
                block+=1

    numJobs = 4
    threads=[]
    while numJobs > 0:
        size = len(args)
        count = size / numJobs
        numJobs-=1
        thr=MengerThread(args[:count])
        threads.append(thr)
        args=args[count:]

    print("Number of threads: %i" % (len(threads)))
    for thr in threads:
        thr.start()
    for thr in threads:
        thr.join()

    mesh=Mesh.Mesh()
    for thr in threads:
        mesh.addMesh(thr.mesh)
        del thr.mesh

    print(mesh)
    mesh.removeDuplicatedPoints()
    mesh.removeFacets(mesh.getInternalFacets())
    mesh.rebuildNeighbourHood()
    print("Mesh is solid: %s" % (mesh.isSolid()))
    Mesh.show(mesh)


### Single-threaded ###

def makeMengerSponge(level=3,x0=0,y0=0,z0=0):
    mesh=Sierpinski(level,x0,y0,z0)
    mesh.removeDuplicatedPoints()
    mesh.removeFacets(mesh.getInternalFacets())
    mesh.rebuildNeighbourHood()
    print("Mesh is solid: %s" % (mesh.isSolid()))
    Mesh.show(mesh)
