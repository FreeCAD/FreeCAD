import PathScripts.PathOp as PathOp
import Path
import FreeCAD
import FreeCADGui
from FreeCAD import Console
import time
import json
import math
import area
from pivy import coin

__doc__ = "Class and implementation of the Adaptive path operation."

def discretize(edge, flipDirection=False):
    pts=edge.discretize(Deflection=0.01)
    if flipDirection: pts.reverse()
    return pts

def IsEqualInXYPlane(e1, e2):
    return math.sqrt((e2.x-e1.x)*(e2.x-e1.x) +
              (e2.y - e1.y) * (e2.y - e1.y))<0.01

def connectEdges(edges):
    ''' Makes the list of connected discretized paths '''
    # find edge
    lastPoint=None
    remaining = []
    pathArray = []
    combined = []
    for edge in edges:
        p1 = edge.valueAt(edge.FirstParameter)
        p2 = edge.valueAt(edge.LastParameter)
        m1 =  edge.valueAt((edge.LastParameter+edge.LastParameter)/2)
        duplicate = False
        for ex in remaining:
            exp1 = ex.valueAt(ex.FirstParameter)
            exp2 = ex.valueAt(ex.LastParameter)
            exm1 = ex.valueAt((ex.FirstParameter + ex.LastParameter)/2)
            if IsEqualInXYPlane(exp1, p1) and IsEqualInXYPlane(exp2, p2) and IsEqualInXYPlane(exm1, m1):
                duplicate = True
            if IsEqualInXYPlane(exp1, p2) and IsEqualInXYPlane(exp2, p1) and IsEqualInXYPlane(exm1, m1):
                duplicate = True
        if not duplicate:
            remaining.append(edge)

    newPath=True
    while len(remaining)>0:
        if newPath:
            edge=remaining[0]
            p1 = edge.valueAt(edge.FirstParameter)
            p2 = edge.valueAt(edge.LastParameter)
            if len(combined)>0: pathArray.append(combined)
            combined = []
            combined.append(discretize(edge))
            remaining.remove(edge)
            lastPoint=p2
            newPath=False

        anyMatch=False
        for e in remaining:
            p1 = e.valueAt(e.FirstParameter)
            p2 = e.valueAt(e.LastParameter)
            if IsEqualInXYPlane(lastPoint,p1):
                combined.append(discretize(e))
                remaining.remove(e)
                lastPoint=p2
                anyMatch=True
                break
            elif IsEqualInXYPlane(lastPoint,p2):
                combined.append(discretize(e,True))
                remaining.remove(e)
                lastPoint=p1
                anyMatch=True
                break
        if not anyMatch:
            newPath=True


    #make sure last path  is appended
    if len(combined)>0: pathArray.append(combined)
    combined = []
    return pathArray

def convertTo2d(pathArray):
    output = []
    for path in pathArray:
        pth2 = []
        for edge in path:
            for pt in edge:
                pth2.append([pt[0],pt[1]])
        output.append(pth2)
    return output


sceneGraph = None
scenePathNodes = [] #for scene cleanup aftewards
topZ = 10

def sceneDrawPath(path, color=(0, 0, 1)):
    global sceneGraph
    global scenePathNodes
    coPoint = coin.SoCoordinate3()
    pts = []
    for pt in path:
        pts.append([pt[0], pt[1], topZ])

    coPoint.point.setValues(0, len(pts), pts)
    ma = coin.SoBaseColor()
    ma.rgb = color
    li = coin.SoLineSet()
    li.numVertices.setValue(len(pts))
    pathNode = coin.SoSeparator()
    pathNode.addChild(coPoint)
    pathNode.addChild(ma)
    pathNode.addChild(li)
    sceneGraph.addChild(pathNode)
    scenePathNodes.append(pathNode) #for scene cleanup afterwards

def sceneClean():
    global scenePathNodes
    for n in scenePathNodes:
        sceneGraph.removeChild(n)
    del scenePathNodes[:]

def GenerateGCode(op,obj,adaptiveResults, helixDiameter):
    if len(adaptiveResults)==0 or len(adaptiveResults[0]["AdaptivePaths"])==0:
      return

    minLiftDistance = op.tool.Diameter
    p1 =  adaptiveResults[0]["HelixCenterPoint"]
    p2 =  adaptiveResults[0]["StartPoint"]
    helixRadius =math.sqrt((p1[0]-p2[0]) * (p1[0]-p2[0]) +  (p1[1]-p2[1]) * (p1[1]-p2[1]))
    stepDown = obj.StepDown.Value
    passStartDepth=obj.StartDepth.Value
    if stepDown<0.1 : stepDown=0.1
    length = 2*math.pi * helixRadius
    if float(obj.HelixAngle)<1: obj.HelixAngle=1
    helixAngleRad = math.pi * float(obj.HelixAngle)/180.0
    depthPerOneCircle=length * math.tan(helixAngleRad)
    stepUp =  obj.LiftDistance.Value
    if stepUp<0:
        stepUp=0

    lx=adaptiveResults[0]["HelixCenterPoint"][0]
    ly=adaptiveResults[0]["HelixCenterPoint"][1]

    step=0
    while passStartDepth>obj.FinalDepth.Value and step<1000:
        step=step+1
        passEndDepth=passStartDepth-stepDown
        if passEndDepth<obj.FinalDepth.Value: passEndDepth=obj.FinalDepth.Value

        for region in adaptiveResults:
            startAngle = math.atan2(region["StartPoint"][1] - region["HelixCenterPoint"][1], region["StartPoint"][0] - region["HelixCenterPoint"][0])

            lx=region["HelixCenterPoint"][0]
            ly=region["HelixCenterPoint"][1]

            r = helixRadius - 0.01
            #helix ramp
            passDepth = (passStartDepth - passEndDepth)
            maxfi =  passDepth / depthPerOneCircle *  2 * math.pi
            fi = 0
            offsetFi =-maxfi + startAngle-math.pi/16

            helixStart = [region["HelixCenterPoint"][0] + r * math.cos(offsetFi), region["HelixCenterPoint"][1] + r * math.sin(offsetFi)]

            op.commandlist.append(Path.Command("(helix to depth: %f)"%passEndDepth))
            #if step == 1:
            #rapid move to start point
            op.commandlist.append(Path.Command(
                "G0", {"X": helixStart[0], "Y": helixStart[1], "Z": obj.ClearanceHeight.Value}))
            #rapid move to safe height
            op.commandlist.append(Path.Command(
                "G0", {"X": helixStart[0], "Y": helixStart[1], "Z": obj.SafeHeight.Value}))

            op.commandlist.append(Path.Command("G1", {
                                  "X": helixStart[0], "Y": helixStart[1], "Z": passStartDepth, "F": op.vertFeed}))

            while fi<maxfi:
                x = region["HelixCenterPoint"][0] + r * math.cos(fi+offsetFi)
                y = region["HelixCenterPoint"][1] + r * math.sin(fi+offsetFi)
                z = passStartDepth - fi / maxfi * (passStartDepth - passEndDepth)
                op.commandlist.append(Path.Command("G1", { "X": x, "Y":y, "Z":z, "F": op.vertFeed}))
                lx=x
                ly=y
                fi=fi+math.pi/16
            op.commandlist.append(Path.Command("(adaptive - depth: %f)"%passEndDepth))
            #add adaptive paths
            for pth in region["AdaptivePaths"]:
                motionType = pth[0]  #[0] contains motion type
                for pt in pth[1]: #[1] contains list of points
                    x=pt[0]
                    y =pt[1]
                    dist=math.sqrt((x-lx)*(x-lx) + (y-ly)*(y-ly))
                    if motionType == area.AdaptiveMotionType.Cutting:
                        op.commandlist.append(Path.Command("G1", { "X": x, "Y":y, "Z":passEndDepth, "F": op.horizFeed}))
                    elif motionType == area.AdaptiveMotionType.LinkClear:
                         if dist > minLiftDistance:
                            if lx!=x or ly!=y:
                                op.commandlist.append(Path.Command("G0", { "X": lx, "Y":ly, "Z":passEndDepth+stepUp}))
                            op.commandlist.append(Path.Command("G0", { "X": x, "Y":y, "Z":passEndDepth+stepUp}))
                    elif motionType == area.AdaptiveMotionType.LinkNotClear:
                        if lx!=x or ly!=y:
                            op.commandlist.append(Path.Command("G0", { "X": lx, "Y":ly, "Z":obj.ClearanceHeight.Value}))
                        op.commandlist.append(Path.Command("G0", { "X": x, "Y":y, "Z":obj.ClearanceHeight.Value}))
                    elif motionType == area.AdaptiveMotionType.LinkClearAtPrevPass:
                        if lx!=x or ly!=y:
                            op.commandlist.append(Path.Command("G0", { "X": lx, "Y":ly, "Z":passStartDepth+stepUp}))
                        op.commandlist.append(Path.Command("G0", { "X": x, "Y":y, "Z":passStartDepth+stepUp}))
                    lx=x
                    ly=y
            #return to safe height in this Z pass
            op.commandlist.append(Path.Command("G0", { "X": lx, "Y":ly, "Z":obj.ClearanceHeight.Value}))
        passStartDepth=passEndDepth
        #return to safe height in this Z pass
        op.commandlist.append(Path.Command("G0", { "X": lx, "Y":ly, "Z":obj.ClearanceHeight.Value}))

    op.commandlist.append(Path.Command("G0", { "X": lx, "Y":ly, "Z":obj.ClearanceHeight.Value}))

def Execute(op,obj):
    global sceneGraph
    global topZ

    sceneGraph = FreeCADGui.ActiveDocument.ActiveView.getSceneGraph()

    Console.PrintMessage("*** Adaptive toolpath processing started...\n")

    #hide old toolpaths during recalculation
    obj.Path = Path.Path("(calculating...)")

    #store old visibility state
    job = op.getJob(obj)
    oldObjVisibility = obj.ViewObject.Visibility
    oldJobVisibility = job.ViewObject.Visibility

    obj.ViewObject.Visibility = False
    job.ViewObject.Visibility = False

    FreeCADGui.updateGui()
    try:
        Console.PrintMessage("Tool diam: %f \n"%op.tool.Diameter)
        helixDiameter = min(op.tool.Diameter,1000.0 if obj.HelixDiameterLimit.Value==0.0 else obj.HelixDiameterLimit.Value )
        nestingLimit=0
        topZ=op.stock.Shape.BoundBox.ZMax

        opType = area.AdaptiveOperationType.Clearing
        obj.Stopped = False
        obj.StopProcessing = False
        if obj.Tolerance<0.001: obj.Tolerance=0.001

        edges=[]
        for base, subs in obj.Base:
            for sub in subs:
                shape=base.Shape.getElement(sub)
                for edge in shape.Edges:
                    edges.append(edge)

        pathArray=connectEdges(edges)

        if obj.OperationType == "Clearing":
            if obj.Side == "Outside":
                if op.stock.StockType == "CreateCylinder":
                     pathArray.append([discretize(op.stock.Shape.Edges[0])])
                else:
                    stockBB = op.stock.Shape.BoundBox
                    v=[]
                    v.append(FreeCAD.Vector(stockBB.XMin,stockBB.YMin,0))
                    v.append(FreeCAD.Vector(stockBB.XMax,stockBB.YMin,0))
                    v.append(FreeCAD.Vector(stockBB.XMax,stockBB.YMax,0))
                    v.append(FreeCAD.Vector(stockBB.XMin,stockBB.YMax,0))
                    v.append(FreeCAD.Vector(stockBB.XMin,stockBB.YMin,0))
                    pathArray.append([v])
                if not obj.ProcessHoles: nestingLimit = 2
            elif not obj.ProcessHoles: nestingLimit = 1
            opType = area.AdaptiveOperationType.Clearing
        else: # profiling
            if obj.Side == "Outside":
                opType = area.AdaptiveOperationType.ProfilingOutside
            else:
                opType = area.AdaptiveOperationType.ProfilingInside
            if not obj.ProcessHoles: nestingLimit = 1

        path2d = convertTo2d(pathArray)
        # put here all properties that influence calculation of adaptive base paths,
        inputStateObject = {
            "tool": float(op.tool.Diameter),
            "tolerance": float(obj.Tolerance),
            "geometry" : path2d,
            "stepover" : float(obj.StepOver),
            "effectiveHelixDiameter": float(helixDiameter),
            "operationType": obj.OperationType,
            "side": obj.Side,
            "processHoles": obj.ProcessHoles,
            "stockToLeave": float(obj.StockToLeave)
        }

        inputStateChanged=False
        adaptiveResults=None

        if obj.AdaptiveOutputState !=None and obj.AdaptiveOutputState != "":
             adaptiveResults = obj.AdaptiveOutputState

        if json.dumps(obj.AdaptiveInputState) != json.dumps(inputStateObject):
             inputStateChanged=True
             adaptiveResults=None

        # progress callback fn, if return true it will stop processing
        def progressFn(tpaths):
            for path in tpaths: #path[0] contains the MotionType,#path[1] contains list of points
                sceneDrawPath(path[1])
            FreeCADGui.updateGui()
            return  obj.StopProcessing

        start=time.time()

        if inputStateChanged or adaptiveResults==None:
            a2d = area.Adaptive2d()
            a2d.stepOverFactor = 0.01*obj.StepOver
            a2d.toolDiameter = float(op.tool.Diameter)
            a2d.helixRampDiameter =  helixDiameter
            a2d.stockToLeave =float(obj.StockToLeave)
            a2d.tolerance = float(obj.Tolerance)
            a2d.opType = opType
            a2d.polyTreeNestingLimit = nestingLimit
            #EXECUTE
            results = a2d.Execute(path2d,progressFn)

            #need to convert results to python object to be JSON serializable
            adaptiveResults = []
            for result in results:
                adaptiveResults.append({
                    "HelixCenterPoint": result.HelixCenterPoint,
                    "StartPoint": result.StartPoint,
                    "AdaptivePaths": result.AdaptivePaths,
                    "ReturnMotionType": result.ReturnMotionType })



        GenerateGCode(op,obj,adaptiveResults,helixDiameter)

        if not obj.StopProcessing:
            Console.PrintMessage("*** Done. Elapsed: %f sec\n\n" %(time.time()-start))
            obj.AdaptiveOutputState = adaptiveResults
            obj.AdaptiveInputState=inputStateObject
        else:
            Console.PrintMessage("*** Processing cancelled (after: %f sec).\n\n" %(time.time()-start))
    finally:
        obj.ViewObject.Visibility = oldObjVisibility
        job.ViewObject.Visibility = oldJobVisibility
        sceneClean()



class PathAdaptive(PathOp.ObjectOp):
    def opFeatures(self, obj):
        '''opFeatures(obj) ... returns the OR'ed list of features used and supported by the operation.
        The default implementation returns "FeatureTool | FeatureDeptsh | FeatureHeights | FeatureStartPoint"
        Should be overwritten by subclasses.'''
        return PathOp.FeatureTool | PathOp.FeatureBaseEdges | PathOp.FeatureDepths | PathOp.FeatureStepDown | PathOp.FeatureHeights | PathOp.FeatureBaseGeometry

    def initOperation(self, obj):
        '''initOperation(obj) ... implement to create additional properties.
        Should be overwritten by subclasses.'''
        obj.addProperty("App::PropertyEnumeration", "Side", "Adaptive", "Side of selected faces that tool should cut")
        obj.Side = ['Outside', 'Inside']  # side of profile that cutter is on in relation to direction of profile

        obj.addProperty("App::PropertyEnumeration", "OperationType", "Adaptive", "Type of adaptive operation")
        obj.OperationType = ['Clearing', 'Profiling']  # side of profile that cutter is on in relation to direction of profile

        obj.addProperty("App::PropertyFloat", "Tolerance", "Adaptive",  "Influences accuracy and performance")
        obj.addProperty("App::PropertyPercent", "StepOver", "Adaptive", "Percent of cutter diameter to step over on each pass")
        obj.addProperty("App::PropertyDistance", "LiftDistance", "Adaptive", "Lift distance for rapid moves")
        obj.addProperty("App::PropertyDistance", "StockToLeave", "Adaptive", "How much stock to leave (i.e. for finishing operation)")
        obj.addProperty("App::PropertyBool", "ProcessHoles", "Adaptive","Process holes as well as the face outline")
        obj.addProperty("App::PropertyBool", "Stopped",
                        "Adaptive", "Stop processing")
        obj.setEditorMode('Stopped', 2) #hide this property

        obj.addProperty("App::PropertyBool", "StopProcessing",
                                  "Adaptive", "Stop processing")
        obj.setEditorMode('StopProcessing', 2)  # hide this property

        obj.addProperty("App::PropertyPythonObject", "AdaptiveInputState",
                        "Adaptive", "Internal input state")
        obj.addProperty("App::PropertyPythonObject", "AdaptiveOutputState",
                        "Adaptive", "Internal output state")
        obj.setEditorMode('AdaptiveInputState', 2) #hide this property
        obj.setEditorMode('AdaptiveOutputState', 2) #hide this property
        obj.addProperty("App::PropertyAngle", "HelixAngle", "Adaptive",  "Helix ramp entry angle (degrees)")
        obj.addProperty("App::PropertyLength", "HelixDiameterLimit", "Adaptive", "Limit helix entry diameter, if limit larger than tool diameter or 0, tool diameter is used")


    def opSetDefaultValues(self, obj):
        obj.Side="Inside"
        obj.OperationType = "Clearing"
        obj.Tolerance = 0.1
        obj.StepOver = 20
        obj.LiftDistance=1.0
        obj.ProcessHoles = True
        obj.Stopped = False
        obj.StopProcessing = False
        obj.HelixAngle = 5
        obj.HelixDiameterLimit = 0.0
        obj.AdaptiveInputState =""
        obj.AdaptiveOutputState = ""
        obj.StockToLeave= 0

    def opExecute(self, obj):
        '''opExecute(obj) ... called whenever the receiver needs to be recalculated.
        See documentation of execute() for a list of base functionality provided.
        Should be overwritten by subclasses.'''
        Execute(self,obj)



def Create(name):
    '''Create(name) ... Creates and returns a Pocket operation.'''
    obj = FreeCAD.ActiveDocument.addObject("Path::FeaturePython", name)
    proxy = PathAdaptive(obj)
    return obj
