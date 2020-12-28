#***************************************************************************
#*   Copyright (c) 2011 Yorik van Havre <yorik@uncreated.net>              *
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

import FreeCAD, Mesh, os, numpy, MeshPart, Arch, Draft, Part, types
from collections import defaultdict
if FreeCAD.GuiUp:
    import FreeCADGui
    from DraftTools import translate
else:
    # \cond
    def translate(context,text):
        return text
    # \endcond

## @package importDAE
#  \ingroup ARCH
#  \brief DAE (Collada) file format importer and exporter
#
#  This module provides tools to import and export Collada (.dae) files.

__title__  = "FreeCAD Collada importer"
__author__ = "Yorik van Havre"
__url__    = "https://www.freecadweb.org"

DEBUG = True

try:
    # Python 2 forward compatibility
    range = xrange
except NameError:
    pass

try:
    import collada as _collada
    collada = _collada
except Exception:
    collada = None

def _collada_node_objects(self, tipo, matrix=None):
    if not matrix is None: M = numpy.dot( matrix, self.matrix )
    else: M = self.matrix
    for node in self.children:
        for obj in node.objects(tipo, M):
            yield obj

_collada_node_init_orig = None

def _collada_node_init(self, *args, **kargs):
    _collada_node_init_orig(self, *args, **kargs)
    self.objects = types.MethodType(_collada_node_objects,self)

def checkCollada():

    "checks if collada if available"

    global collada
    if not collada:
        try:
            import collada as _collada
            collada = _collada
        except ImportError:
            FreeCAD.Console.PrintError(translate("Arch","pycollada not found, collada support is disabled.")+"\n")
            return False

    # monkey patch collada.scene.Node.objects() because of a bug in older version
    global _collada_node_init_orig
    if collada.__version__ == '0.4' and not _collada_node_init_orig:
        _collada_node_init_orig = collada.scene.Node.__init__
        collada.scene.Node.__init__ = _collada_node_init
    return True


def triangulate(shape, dosegment):
    "triangulates the given face"

    param = FreeCAD.ParamGet("User parameter:BaseApp/Preferences/Mod/Arch")
    mesher = param.GetInt("ColladaMesher",0)
    tessellation = param.GetFloat("ColladaTessellation",1.0)
    grading = param.GetFloat("ColladaGrading",0.3)
    segsperedge = param.GetInt("ColladaSegsPerEdge",1)
    segsperradius = param.GetInt("ColladaSegsPerRadius",2)
    secondorder = param.GetBool("ColladaSecondOrder",False)
    optimize = param.GetBool("ColladaOptimize",True)
    allowquads = param.GetBool("ColladaAllowQuads",False)

    if mesher == 0:
        angulardeflect = min(0.1, tessellation * 0.5 + 0.005)
        return MeshPart.meshFromShape(Shape=shape,
                                      LinearDeflection=tessellation,
                                      AngularDeflection=angulardeflect,
                                      Relative=True,
                                      Segments=dosegment)

    def _build_mesh(s):
        if mesher == 1:
            return MeshPart.meshFromShape(Shape=s,MaxLength=tessellation)
        else:
            return MeshPart.meshFromShape(Shape=s,GrowthRate=grading,SegPerEdge=segsperedge,
                SegPerRadius=segsperradius,SecondOrder=secondorder,Optimize=optimize,
                AllowQuad=allowquads)

    if not dosegment:
        return _build_mesh(shape)
    segments = []
    faces = []
    for face in shape.Faces:
        mesh = _build_mesh(face)
        start = len(faces)
        faces.append(mesh.Topology)
        segments.append((start, len(faces)))

    mesh = Mesh.Mesh(faces)
    if len(segments) > 1:
        for s in segments:
            mesh.addSegment(range(s[0], s[1]))

    return mesh

def open(filename):

    "called when freecad wants to open a file"

    if not checkCollada():
        return
    docname = (os.path.splitext(os.path.basename(filename))[0]).encode("utf8")
    doc = FreeCAD.newDocument(docname)
    doc.Label = decode(docname)
    FreeCAD.ActiveDocument = doc
    read(filename)
    return doc


def insert(filename,docname):

    "called when freecad wants to import a file"

    if not checkCollada():
        return
    try:
        doc = FreeCAD.getDocument(docname)
    except NameError:
        doc = FreeCAD.newDocument(docname)
    FreeCAD.ActiveDocument = doc
    read(filename)
    return doc


def decode(name):

    "decodes encoded strings"

    try:
        decodedName = (name.decode("utf8"))
    except UnicodeDecodeError:
        try:
            decodedName = (name.decode("latin1"))
        except UnicodeDecodeError:
            FreeCAD.Console.PrintError(translate("Arch","Error: Couldn't determine character encoding"))
            decodedName = name
    return decodedName

def _get_matrix(matrix):
    if matrix is None:
        return FreeCAD.Matrix()
    (a11,a12,a13,a14),(a21,a22,a23,a24),(a31,a32,a33,a34),(a41,a42,a43,a44) = matrix
    return FreeCAD.Matrix(a11,a12,a13,a14,
                          a21,a22,a23,a24,
                          a31,a32,a33,a34,
                          a41,a42,a43,a44)

def _read_material(node):
    try:
        effect = node.material.effect
        return FreeCAD.Material(
                AmbientColor = effect.ambient[:3],
                SpecularColor = effect.specular[:3],
                EmissiveColor = effect.emission[:3],
                DiffuseColor = effect.diffuse[:3],
                Shininess = effect.shininess,
                Transparency = effect.transparency)
    except Exception:
        return None

def _same_material(mat1, mat2):
    return mat1.AmbientColor == mat2.AmbientColor \
            and mat1.SpecularColor == mat2.SpecularColor \
            and mat1.EmissiveColor == mat2.EmissiveColor \
            and mat1.DiffuseColor == mat2.DiffuseColor \
            and mat1.Shininess == mat2.Shininess \
            and mat1.Transparency == mat2.Transparency

def read(filename):

    "reads a DAE file"

    param = FreeCAD.ParamGet("User parameter:BaseApp/Preferences/Mod/Arch")
    instancing = param.GetBool("ColladaImportInstances",False)

    col = collada.Collada(filename, ignore=[collada.DaeUnsupportedError])
    # Read the unitmeter info from dae file and compute unit to convert to mm
    unitmeter = col.assetInfo.unitmeter or 1
    unit = unitmeter / 0.001
    #for geom in col.geometries:
    #for geom in col.scene.objects('geometry'):

    if instancing:
        geommap = defaultdict(list)
    else:
        geommap = None

    for node in col.scene.nodes:
        for geomnode in node.objects('geometry'):
            mesh = None
            primitives = bound_primtives = []
            mat = None
            if geommap is not None:
                obj = geommap.get(geomnode.original, None)
                if not obj:
                    primitives = list(geomnode.original.primitives)
                    if not primitives:
                        continue
                    bound_primtives = list(geomnode.primitives())
                    obj = FreeCAD.ActiveDocument.addObject("Mesh::Feature","Mesh")
                    obj.Label = geomnode.original.xmlnode.get('name', obj.Name)
                    mesh = Mesh.Mesh()
                elif FreeCAD.GuiUp:
                    # link to mesh does not support per face color yet, so just
                    # check the material of the first bound primitive
                    this_mat = obj.ViewObject.ShapeMaterial
                    for bprim in geomnode.primitives():
                        mat = _read_material(bprim)
                        if mat:
                            if _same_material(mat, this_mat):
                                mat = None
                            break
            else:
                bound_primtives = primitives = list(geomnode.primitives())
                if not primitives:
                    continue
                obj = FreeCAD.ActiveDocument.addObject("Mesh::Feature","Mesh")
                obj.Label = geomnode.original.xmlnode.get('name', obj.Name)
                mesh = Mesh.Mesh()

            segments = []
            skip = 0
            unsupported = defaultdict(list)
            for bprim, prim in zip(bound_primtives, primitives):
                if not isinstance(prim, collada.triangleset.TriangleSet):
                    cnt = unsupported[prim.__class__.__name__.split('.')[-1]]
                    if not cnt:
                        cnt.append(1)
                    else:
                        cnt[0] += 1
                start = mesh.CountFacets
                facets = []
                for tri in prim:
                    for v in tri.vertices:
                        v = [x * unit for x in v]
                        facets.append([v[0],v[1],v[2]])
                if not facets:
                    skip += 1
                    continue
                mesh.addFacets(facets)
                if mesh.CountFacets > start:
                    face_mat = _read_material(bprim)
                    face_color = None
                    if face_mat:
                        face_color = face_mat.DiffuseColor[:3]
                        if not mat:
                            mat = face_mat
                    segments.append((range(start, mesh.CountFacets), face_color))

            if skip:
                FreeCAD.Console.PrintWarning(
                    translate("Arch","Skip %d empty primitives in %s.\n" % (skip, obj.Label)))
            for name,cnt in unsupported.items():
                FreeCAD.Console.PrintWarning(
                    translate("Arch","Skip %d unsupported primitive %s in %s.\n" \
                            % (cnt, name, obj.Label)))

            if mesh:
                highlight = False
                if mat and FreeCAD.GuiUp:
                    obj.ViewObject.ShapeMaterial = mat
                    if len(segments) > 1 or len(segments[0][0]) != mesh.CountFacets:
                        for segment in segments:
                            mesh.addSegment(segment[0], segment[1])
                            if segment[1]:
                                highlight = True

                obj.Mesh = mesh
                if highlight:
                    obj.ViewObject.highlightSegments()
                obj.recompute(True)

            if geommap is None:
                continue

            matrix = _get_matrix(geomnode.matrix)
            t,r,s,_ = matrix.getTransform()
            if mesh and FreeCAD.Vector(s).isEqual(FreeCAD.Vector(1.0,1.0,1.0), 1e-7):
                obj.Placement = FreeCAD.Placement(t,r)
                if geommap is not None:
                    geommap[geomnode.original] = obj
                obj.recompute(True)
                continue

            link = FreeCAD.ActiveDocument.addObject("App::Link","Link")
            if mesh:
                obj.Visibility = False
                if geommap is not None:
                    geommap[geomnode.original] = link
                mat = None
            link.LinkedObject = obj
            link.Placement = FreeCAD.Placement(t,r)
            link.ScaleVector = s
            link.Label = geomnode.original.xmlnode.get('name', obj.Label)
            if mat:
                link.ViewObject.ShapeMaterial = mat
                link.ViewObject.OverrideMaterial = True
            link.recompute(True)

def exportSelection(filename, colors=None):
    """exportSelection(filename,colors=None):

    New style exporter function called by freecad to export current selection.
    It is added to allow the function to extract object hierarchy from the
    current selection to derived to correct global placement.
    """

    objset = set()
    for sel in FreeCADGui.Selection.getSelectionEx('*', 0):
        subs = sel.SubElementNames
        if not subs:
            objset.add((sel.Object, ''))
            continue
        for sub in subs:
            objset.add((sel.Object, Part.splitSubname(sub)[0]))
    _export(objset, filename, colors)

def export(exportList,filename,tessellation=1,colors=None):
    """export(exportList,filename,tessellation=1,colors=None) -- exports FreeCAD contents to a DAE file.
    colors is an optional dictionary of objName:shapeColorTuple or objName:diffuseColorList elements
    to be used in non-GUI mode if you want to be able to export colors. Tessellation is used when breaking
    curved surfaces into triangles."""

    _ = tessellation # not used?
    colorMap = {}
    if colors and exportList:
        doc = exportList[0].Document
        for name, c in colors.items():
            try:
                if '#' not in name:
                    colorMap[doc.getObject(name).FullName] = c
            except Exception:
                pass

    _export({(o,'') for o in exportList}, filename, colorMap, tessellation)

def _get_material(colmesh, mat, color, effects):
    if isinstance(color[0], tuple):
        return [_get_material(colmesh, mat, c, effects) for c in color]

    if not mat:
        key = Draft.getrgb(color,testbw=False)[1:]
        matname = 'color_' + key
    else:
        key = [mat.AmbientColor[:3],
               mat.EmissiveColor[:3],
               mat.SpecularColor[:3],
               mat.Shininess,
               mat.Transparency,
               color[:3]]

    try:
        return effects[key]
    except Exception:
        if mat:
            matname = 'mat_%d' % len(effects)

    if mat:
        effect = collada.material.Effect(
            'effect_' + matname,
            [],
            'phong',
            diffuse=color[:3],
            specular=mat.SpecularColor[:3],
            ambient=mat.AmbientColor[:3],
            emission=mat.EmissiveColor[:3],
            shininess=mat.Shininess,
            transparency=mat.Transparency)
    else:
        effect = collada.material.Effect(
            'effect_' + matname, [], 'phong', diffuse=color[:3])

    colmesh.effects.append(effect)
    mat = collada.material.Material(matname, matname, effect)
    colmesh.materials.append(mat)
    effects[matname] = mat
    return mat

def _build_materials(colmesh, mat, color, obj, matref, effects, segcount, defaultcolor):
    if not color and hasattr(obj,"Material"):
        try:
            color = tuple([float(k) \
                    for k in obj.Material.Material["DiffuseColor"].strip("()").split(",")])
        except Exception:
            pass

    if not color and FreeCAD.GuiUp:
        mat = getattr(obj.ViewObject, 'ShapeMaterial', None)
        if segcount and hasattr(obj.ViewObject,"DiffuseColor"):
            colors = obj.ViewObject.DiffuseColor
            if segcount == len(colors):
                color = colors
        if not color and hasattr(obj.ViewObject, 'ShapeColor'):
            color = obj.ViewObject.ShapeColor[:3]

    if not color:
        color = defaultcolor

    if isinstance(color[0], tuple) and (not segcount or segcount != len(color)):
        color = color[0]

    mat = _get_material(colmesh, mat, color, effects)

    if not segcount:
        return [collada.scene.MaterialNode(matref, mat, inputs=[])]

    if not isinstance(mat, list):
        mat = [mat] * segcount
    nodes = []
    for i,m in enumerate(mat):
        node = collada.scene.MaterialNode(
                matref + '_face' + str(i), m, inputs=[])
        nodes.append(node)
    return nodes

def _build_geom(colmesh, mesh, scale, obj, ind, dosegment):
    name = obj.Name
    label = obj.Label
    matref = 'mat_%s_%d' % (name,ind)

    findex = numpy.array([])

    Topology = mesh.Topology
    Facets = mesh.Facets

    vertices = numpy.empty(len(Topology[0]) * 3)
    for i,v in enumerate(Topology[0]):
        vertices[[i*3, i*3+1, i*3+2]] = (v.x*scale,v.y*scale,v.z*scale)

    normals = numpy.empty(len(Facets) * 3)
    for i,facet in enumerate(Facets):
        n = facet.Normal
        normals[[i*3, i*3+1, i*3+2]] = (n.x,n.y,n.z)

    vert_src = collada.source.FloatSource(
            "vertices_%s_%d"%(name,ind), vertices, ('X', 'Y', 'Z'))
    normal_src = collada.source.FloatSource(
            "normals_%s_%d"%(name,ind), normals, ('X', 'Y', 'Z'))
    geom = collada.geometry.Geometry(
            colmesh, "geometry_%s_%d"%(name,ind), label, [vert_src, normal_src])

    def _add_facets(facets, ref):
        # face indices
        findex = numpy.empty(len(facets) * 6, numpy.int64)
        for i,f in enumerate(facets):
            j = i*6
            findex[[j, j+1, j+2, j+3, j+4, j+5]] = (f[0],i,f[1],i,f[2],i)

        input_list = collada.source.InputList()
        input_list.addInput(0, 'VERTEX', "#" + vert_src.id)
        input_list.addInput(1, 'NORMAL', "#" + normal_src.id)

        triset = geom.createTriangleSet(findex, input_list, ref)
        geom.primitives.append(triset)

    if mesh.countSegments() and dosegment:
        segments = mesh.countSegments()
        for i in range(0, segments):
            facets = []
            for idx in mesh.getSegment(i):
                facets.append(Topology[1][idx])
            _add_facets(facets, matref + '_face' + str(i))
    else:
        segments = 0
        _add_facets(Topology[1], matref)

    colmesh.geometries.append(geom)
    return geom, segments, matref

def _export(exportSet, filename, colors):
    if not checkCollada(): return
    p = FreeCAD.ParamGet("User parameter:BaseApp/Preferences/Mod/Arch")
    instancing = p.GetBool("ColladaExportInstances",False)
    scale = p.GetFloat("ColladaScalingFactor",1.0)
    scale = scale * 0.001 # from millimeters (FreeCAD) to meters (Collada)
    p = FreeCAD.ParamGet("User parameter:BaseApp/Preferences/View")
    c = p.GetUnsigned("DefaultShapeColor",4294967295)
    defaultcolor = (float((c>>24)&0xFF)/255.0,float((c>>16)&0xFF)/255.0,float((c>>8)&0xFF)/255.0)
    colmesh = collada.Collada()
    colmesh.assetInfo.upaxis = collada.asset.UP_AXIS.Z_UP
    # authoring info
    cont = collada.asset.Contributor()
    try:
        author = FreeCAD.ActiveDocument.CreatedBy
    except UnicodeEncodeError:
        author = FreeCAD.ActiveDocument.CreatedBy.encode("utf8")
    author = author.replace("<","")
    author = author.replace(">","")
    cont.author = author
    ver = FreeCAD.Version()
    appli = "FreeCAD v" + ver[0] + "." + ver[1] + " build" + ver[2] + "\n"
    cont.authoring_tool = appli
    #print(author, appli)
    colmesh.assetInfo.contributors.append(cont)
    colmesh.assetInfo.unitname = "meter"
    colmesh.assetInfo.unitmeter = 1.0
    defaultmat = None
    objind = 0
    scenenodes = []
    exportList = list(exportSet)
    geomcache = defaultdict(list)
    effects = {}

    objectslist = Draft.get_group_contents(exportList, walls=True,
                                           addgroups=True)
    objectslist = Arch.pruneIncluded(objectslist)

    if colors is None:
        colors = {}

    matrix0 = FreeCAD.Matrix()
    instances = []

    param = FreeCAD.ParamGet("User parameter:BaseApp/Preferences/Mod/Arch")
    dosegment = param.GetBool("ColladaExportSegments", False)

    for parentobj, sub in objectslist:
        path = parentobj.Name + '.' + sub
        sobj, matrix = parentobj.getSubObject(sub, retType=1, matrix=matrix0)

        if not sobj:
            FreeCAD.Console.PrintWarning(translate("Arch","Cannot find sub object %s.%s\n" \
                        % (parentobj.FullName, sub)))
            continue

        obj, matrix = sobj.getLinkedObject(recursive=True, matrix=matrix, transform=False)

        item = geomcache[obj]
        if item:
            instances.append((path, obj, sobj, matrix))
            continue

        # DO NOT use getattr(obj, 'Mesh') because incomplete support of Mesh in
        # Link, especially link array!
        if obj.isDerivedFrom('Mesh::Feature'):
            mesh = Mesh.Mesh(obj.Mesh)
            mesh.Placement = FreeCAD.Placement()
            item.append(mesh)
            instances.append((path, obj, sobj, matrix))
        else:
            shape = Part.getShape(obj, transform=False)
            if shape.isNull():
                FreeCAD.Console.PrintError("Unable to export object %s (%s.%s), Skipping.\n" \
                                            % (sobj.Label, sobj.FullName, sub))
                continue

            item.append(triangulate(shape, dosegment))
            instances.append((path, obj, sobj, matrix))

    if instancing:
        for obj, item in geomcache.items():
            geom,segcount,matref = _build_geom(
                    colmesh, item[0], scale, obj, objind, dosegment)
            item.append(geom)
            item.append(matref)
            item.append(segcount)
            objind += 1

    nodecache = {}
    for path, obj, sobj, matrix in instances:
        item = geomcache[obj]
        if instancing:
            geom = item[1]
            matref = item[2]
            segcount = item[3]
            transforms = [collada.scene.MatrixTransform(numpy.array(matrix.A))]
        else:
            mesh = Mesh.Mesh(item[0])
            mesh.transform(matrix)
            transforms = []
            geom,segcount,matref = _build_geom(colmesh, mesh, scale, obj, objind, dosegment)

        color = colors.get(sobj.FullName, None)
        mat = None
        if not color and FreeCAD.GuiUp:
            if getattr(sobj.ViewObject, 'OverrideMaterial', False):
                mat = getattr(sobj.ViewObject, 'ShapeMaterial', None)
                if mat:
                    color = mat.DiffuseColor

        if not instancing or color:
            # this means either color override or no instancing, so build a geom
            # node for material binding
            matnodes = _build_materials(
                    colmesh, mat, color, obj, matref, effects, segcount, defaultcolor)
            geomnode = collada.scene.GeometryNode(geom, matnodes)
            node = collada.scene.Node("gnode_%s_%d" % (path, objind),
                                      children=[geomnode],
                                      transforms=transforms)
        else:
            # normal coloring with instancing, query cache first
            try:
                node = nodecache[geom]
            except Exception:
                color = colors.get(obj.FullName, None)
                matnodes = _build_materials(
                        colmesh, mat, color, sobj, matref, effects, segcount, defaultcolor)
                # create a GeometryNode for material binding
                geomnode = collada.scene.GeometryNode(geom, matnodes)
                # wrap the GeometryNode with a Node without transformation so
                # that we can use NodeNode to reference this geometry without
                # new material binding
                node = collada.scene.Node("node_%s_%d" % (obj.Name, objind), children=[geomnode])
                colmesh.nodes.append(node)
                nodecache[geom] = node
            node = collada.scene.Node("ref_%s_%d" % (path,objind),
                                      children=[collada.scene.NodeNode(node)],
                                      transforms=transforms)
        scenenodes.append(node)
        objind += 1

    myscene = collada.scene.Scene("myscene", scenenodes)
    colmesh.scenes.append(myscene)
    colmesh.scene = myscene
    colmesh.write(filename)
    FreeCAD.Console.PrintMessage(
            translate("Arch","file %s successfully created.\n") % filename)
