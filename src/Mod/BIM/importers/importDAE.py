# SPDX-License-Identifier: LGPL-2.1-or-later

# ***************************************************************************
# *                                                                         *
# *   Copyright (c) 2011 Yorik van Havre <yorik@uncreated.net>              *
# *                                                                         *
# *   This file is part of FreeCAD.                                         *
# *                                                                         *
# *   FreeCAD is free software: you can redistribute it and/or modify it    *
# *   under the terms of the GNU Lesser General Public License as           *
# *   published by the Free Software Foundation, either version 2.1 of the  *
# *   License, or (at your option) any later version.                       *
# *                                                                         *
# *   FreeCAD is distributed in the hope that it will be useful, but        *
# *   WITHOUT ANY WARRANTY; without even the implied warranty of            *
# *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU      *
# *   Lesser General Public License for more details.                       *
# *                                                                         *
# *   You should have received a copy of the GNU Lesser General Public      *
# *   License along with FreeCAD. If not, see                               *
# *   <https://www.gnu.org/licenses/>.                                      *
# *                                                                         *
# ***************************************************************************

__title__  = "FreeCAD Collada importer"
__author__ = "Yorik van Havre"
__url__    = "https://www.freecad.org"

## @package importDAE
#  \ingroup ARCH
#  \brief DAE (Collada) file format importer and exporter
#
#  This module provides tools to import and export Collada (.dae) files.

import os
from typing import Optional
from xml.sax.saxutils import escape as sax_escape

import numpy as np

import FreeCAD
import Arch
import Draft
import Mesh
import MeshPart

from draftutils import params

if FreeCAD.GuiUp:
    from draftutils.translate import translate
else:
    # \cond
    def translate(context, text):
        return text
    # \endcond


def xml_escape(text: str, entities: dict[str, str] = None) -> str:
    """Escape text for XML.

    This is a wrapper around xml.sax.saxutils.escape that replaces also
    `"` with `&quot;` by default.
    """
    if entities is None:
        entities = {'"': "&quot;"}
    return sax_escape(text, entities=entities)


def import_collada() -> bool:
    """Return True if the `collada` module is available.

    Also imports the module.

    """
    global collada
    try:
        import collada
    except ImportError:
        FreeCAD.Console.PrintError(translate("BIM", "pycollada not found, collada support is disabled.") + "\n")
        return False
    return True


def triangulate(shape):
    """Triangulate the given shape."""
    mesher = params.get_param_arch("ColladaMesher")
    tessellation = params.get_param_arch("ColladaTessellation")
    grading = params.get_param_arch("ColladaGrading")
    segs_per_edge = params.get_param_arch("ColladaSegsPerEdge")
    segs_per_radius = params.get_param_arch("ColladaSegsPerRadius")
    second_order = params.get_param_arch("ColladaSecondOrder")
    optimize = params.get_param_arch("ColladaOptimize")
    allow_quads = params.get_param_arch("ColladaAllowQuads")
    if mesher == 0:
        return shape.tessellate(tessellation)
    elif mesher == 1:
        return MeshPart.meshFromShape(Shape=shape, MaxLength=tessellation).Topology
    else:
        return MeshPart.meshFromShape(
                Shape=shape,
                GrowthRate=grading,
                SegPerEdge=segs_per_edge,
                SegPerRadius=segs_per_radius,
                SecondOrder=second_order,
                Optimize=optimize,
                AllowQuad=allow_quads,
        ).Topology


def open(filename):
    """Called when FreeCAD wants to open a file."""
    if not import_collada():
        return
    docname = os.path.splitext(os.path.basename(filename))[0]
    doc = FreeCAD.newDocument(docname)
    doc.Label = docname
    FreeCAD.ActiveDocument = doc
    read(filename)
    return doc


def insert(filename, docname):
    """Called when FreeCAD wants to import a file."""
    if not import_collada():
        return
    try:
        doc = FreeCAD.getDocument(docname)
    except NameError:
        doc = FreeCAD.newDocument(docname)
    FreeCAD.ActiveDocument = doc
    read(filename)
    return doc


def read(filename):
    """Read a DAE file."""
    doc = FreeCAD.activeDocument()
    if not doc:
        return
    col = collada.Collada(filename, ignore=[collada.common.DaeUnsupportedError])
    # Read the unitmeter info from DAE file and compute unit to convert to mm.
    unit_meter = col.assetInfo.unitmeter or 1.0
    unit = unit_meter / 0.001
    bound_geom: collada.geometry.BoundGeometry
    # Implementation note: there's also `col.geometries` but when using them,
    # the materials are string and Gaël didn't find a way to get the material
    # node from this string.
    for bound_geom in col.scene.objects('geometry'):
        prim: collada.primitive.BoundPrimitive
        for prim in bound_geom.primitives():
            if not isinstance(prim, collada.triangleset.BoundTriangleSet):
                # e.g. a BoundLineSet, which is not supported yet.
                continue
            # Get the materials and associated vertices.
            meshes: dict[collada.scene.MaterialNode, list] = {}
            tri: collada.triangleset.Triangle
            for tri in prim:
                material_node: collada.material.Material = tri.material
                if material_node not in meshes:
                    # Not yet in the dict, create a new entry.
                    meshes[material_node] = []
                if len(tri.vertices) != 3:
                    msg = (
                            f"Warning: triangle with {len(tri.vertices)} vertices found"
                            f" in {bound_geom.original.name}, expected 3. Skipping this triangle."
                    )
                    FreeCAD.Console.PrintWarning(msg + "\n")
                    continue
                # tri.vertices is a numpy array.
                meshes[material_node].append((tri.vertices * unit).tolist())
            # Create a mesh for each material node.
            for material_node, vertices in meshes.items():
                mesh = Mesh.Mesh(vertices)
                name = bound_geom.original.name
                if not name:
                    name = bound_geom.original.id
                obj = doc.addObject("Mesh::Feature", name)
                obj.Label = name
                obj.Mesh = mesh
                if not material_node:
                    continue
                if FreeCAD.GuiUp:
                    fc_mat = FreeCAD.Material()
                    # We do not import transparency because it is often set
                    # wrongly (transparency mistaken for opacity).
                    # TODO: Ask whether to import transparency.
                    field_map = {
                            "ambient": "AmbientColor",
                            "diffuse": "DiffuseColor",
                            "emission": "EmissiveColor",
                            "specular": "SpecularColor",
                            "shininess": "Shininess",
                            # "transparency": "Transparency",
                    }
                    for col_field, fc_field in field_map.items():
                        try:
                            # Implementation note: using floats, so values must
                            # be within [0, 1]. OK.
                            setattr(fc_mat, fc_field, getattr(material_node.effect, col_field))
                        except ValueError:
                            # The collada value is not compatible with FreeCAD.
                            pass
                        except TypeError:
                            # color is not a tuple but a texture.
                            pass
                    obj.ViewObject.ShapeAppearance = (fc_mat,)

    # Print the errors that occurred during reading.
    if col.errors:
        FreeCAD.Console.PrintWarning(translate("BIM", "File was read but some errors occurred:") + "\n")
    for e in col.errors:
        FreeCAD.Console.PrintWarning(str(e) + "\n")
    if FreeCAD.GuiUp:
        FreeCAD.Gui.SendMsgToActiveView("ViewFit")


def export(
        exports: list[FreeCAD.DocumentObject],
        filename: str,
        tessellation: int = 1,
        colors: Optional[dict[str, tuple]] = None,
):
    """Export FreeCAD contents to a DAE file.

    Parameters
    ----------
    - tessellation is used when breaking curved surfaces into triangles.
    - colors is an optional dictionary of {objName: shapeColorTuple} or
             {objName: diffuseColorList} elements to be used in non-GUI
             mode if you want to be able to export colors.

    """
    if not import_collada():
        return
    if colors is None:
        colors = {}
    scale = params.get_param_arch("ColladaScalingFactor")
    scale = scale * 0.001  # from millimeters (FreeCAD) to meters (Collada)
    default_color = Draft.get_rgba_tuple(params.get_param_view("DefaultShapeColor"))[:3]
    col_mesh = collada.Collada()
    col_mesh.assetInfo.upaxis = collada.asset.UP_AXIS.Z_UP
    # Authoring info.
    col_contributor = collada.asset.Contributor()
    try:
        author = FreeCAD.ActiveDocument.CreatedBy
    except UnicodeEncodeError:
        author = FreeCAD.ActiveDocument.CreatedBy.encode("utf8")
    author = xml_escape(author)
    col_contributor.author = author
    ver = FreeCAD.Version()
    appli = f"FreeCAD v{ver[0]}.{ver[1]} build {ver[2]}"
    col_contributor.authoring_tool = appli
    # Bug in collada from 0.4 to 0.9, contributors are not written to file.
    # Set it anyway for future versions.
    col_mesh.assetInfo.contributors.append(col_contributor)
    col_mesh.assetInfo.unitname = "meter"
    col_mesh.assetInfo.unitmeter = 1.0
    default_mat = None
    obj_ind = 0
    scene_nodes = []
    objects = Draft.get_group_contents(
            exports,
            walls=True,
            addgroups=True,
    )
    objects = Arch.pruneIncluded(objects, strict=True)
    for obj in objects:
        findex = np.array([])
        m: Optional[Mesh.Mesh] = None
        if obj.isDerivedFrom("Part::Feature"):
            FreeCAD.Console.PrintMessage(f"Exporting shape of object {obj.Name} (\"{obj.Label}\")" + "\n")
            new_shape = obj.Shape.copy()
            new_shape.Placement = obj.getGlobalPlacement()
            m = Mesh.Mesh(triangulate(new_shape))
        elif obj.isDerivedFrom("Mesh::Feature"):
            FreeCAD.Console.PrintMessage(f"Exporting mesh of object {obj.Name} (\"{obj.Label}\")" + "\n")
            m = obj.Mesh
        elif obj.isDerivedFrom("App::Part"):
            for child in obj.OutList:
                objects.append(child)
            continue
        else:
            continue
        if m:
            topology = m.Topology
            facets = m.Facets

            # Vertex indices.
            vindex = np.empty(len(topology[0]) * 3)
            for i in range(len(topology[0])):
                v = topology[0][i]
                vindex[list(range(i*3, i*3+3))] = (v.x*scale, v.y*scale, v.z*scale)

            # Normals.
            nindex = np.empty(len(facets) * 3)
            for i in range(len(facets)):
                n = facets[i].Normal
                nindex[list(range(i*3, i*3+3))] = (n.x,n.y,n.z)

            # Face indices.
            findex = np.empty(len(topology[1]) * 6, np.int64)
            for i in range(len(topology[1])):
                f = topology[1][i]
                findex[list(range(i*6, i*6+6))] = (f[0], i, f[1], i, f[2], i)

        vert_src = collada.source.FloatSource(f"cubeverts-array{obj_ind}", vindex, ("X", "Y", "Z"))
        normal_src = collada.source.FloatSource(f"cubenormals-array{obj_ind}", nindex, ("X", "Y", "Z"))
        geom = collada.geometry.Geometry(
                collada=col_mesh,
                id=f"geometry{obj_ind}",
                name=xml_escape(obj.Label),
                sourcebyid=[vert_src, normal_src],
        )
        input_list = collada.source.InputList()
        input_list.addInput(0, "VERTEX", f"#cubeverts-array{obj_ind}")
        input_list.addInput(1, "NORMAL", f"#cubenormals-array{obj_ind}")
        mat_node: Optional[collada.scene.MaterialNode] = None
        mat_ref = "materialref"
        if (
                hasattr(obj, "Material")
            and obj.Material
            and hasattr(obj.Material, "Material")
            and ("DiffuseColor" in obj.Material.Material)
        ):
            kd = tuple([float(k) for k in obj.Material.Material["DiffuseColor"].strip("()").split(",")])
            effect = collada.material.Effect(
                    id=f"effect_{obj.Material.Name}",
                    params=[],
                    shadingtype="phong",
                    diffuse=kd,
                    specular=(1, 1, 1),
            )
            mat = collada.material.Material(
                    id=f"mat_{obj.Material.Name}",
                    name=obj.Material.Name,
                    effect=effect,
            )
            col_mesh.effects.append(effect)
            col_mesh.materials.append(mat)
            mat_ref = f"ref_{obj.Material.Name}"
            mat_node = collada.scene.MaterialNode(
                    symbol=mat_ref,
                    target=mat,
                    inputs=[],
            )
        if not mat_node:
            if obj.Name in colors:
                color = colors[obj.Name]
                if color:
                    if isinstance(color[0], tuple):
                        # This is a diffusecolor. For now, use the first color.
                        # TODO: Support per-face colors
                        color = color[0]
                    kd = color[:3]
                    effect = collada.material.Effect(
                            id=f"effect_{obj.Name}",
                            params=[],
                            shadingtype="phong",
                            diffuse=kd,
                            specular=(1, 1, 1),
                    )
                    mat = collada.material.Material(
                            id=f"mat_{obj.Name}",
                            name=obj.Name,
                            effect=effect,
                    )
                    col_mesh.effects.append(effect)
                    col_mesh.materials.append(mat)
                    mat_ref = f"ref_{obj.Name}"
                    mat_node = collada.scene.MaterialNode(
                            symbol=mat_ref,
                            target=mat,
                            inputs=[],
                    )
            elif FreeCAD.GuiUp:
                if hasattr(obj.ViewObject, "ShapeColor"):
                    kd = obj.ViewObject.ShapeColor[:3]
                    effect = collada.material.Effect(
                            id=f"effect_{obj.Name}",
                            params=[],
                            shadingtype="phong",
                            diffuse=kd,
                            specular=(1, 1, 1),
                    )
                    mat = collada.material.Material(
                            id=f"mat_{obj.Name}",
                            name=obj.Name,
                            effect=effect,
                    )
                    col_mesh.effects.append(effect)
                    col_mesh.materials.append(mat)
                    mat_ref = f"ref_{obj.Name}"
                    mat_node = collada.scene.MaterialNode(
                            symbol=mat_ref,
                            target=mat,
                            inputs=[],
                    )
        if not mat_node:
            if not default_mat:
                effect = collada.material.Effect(
                        id="effect_default",
                        params=[],
                        shadingtype="phong",
                        diffuse=default_color,
                        specular=(1, 1, 1),
                )
                default_mat = collada.material.Material(
                        id="mat_default",
                        name="default_material",
                        effect=effect,
                )
                col_mesh.effects.append(effect)
                col_mesh.materials.append(default_mat)
            mat_node = collada.scene.MaterialNode(
                    symbol=mat_ref,
                    target=default_mat,
                    inputs=[],
            )
        triset = geom.createTriangleSet(indices=findex, inputlist=input_list, materialid=mat_ref)
        geom.primitives.append(triset)
        col_mesh.geometries.append(geom)
        geom_node = collada.scene.GeometryNode(geom, [mat_node])
        node = collada.scene.Node(id=f"node{obj_ind}", children=[geom_node])
        scene_nodes.append(node)
        obj_ind += 1
    scene = collada.scene.Scene("scene", scene_nodes)
    col_mesh.scenes.append(scene)
    col_mesh.scene = scene
    col_mesh.write(filename)
    FreeCAD.Console.PrintMessage(translate("BIM", f'file "{filename}" successfully created.' + "\n"))
