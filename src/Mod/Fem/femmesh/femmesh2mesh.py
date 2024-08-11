# ***************************************************************************
# *   Copyright (c) 2023 Peter McB                                          *
# *   added the function, mesh_2_femmesh, to convert the MESH               *
# *                     into a triangular FEMMESH                           *
# *                                                                         *
# *   Copyright (c) 2016 Frantisek Loeffelmann <LoffF@email.cz>             *
# *   extension to the work of:                                             *
#         Frantisek Loeffelmann, Ulrich Brammer, Bernd Hahnebach            *
# *                                                                         *
# *   This program is free software; you can redistribute it and/or modify  *
# *   it under the terms of the GNU Lesser General Public License (LGPL)    *
# *   as published by the Free Software Foundation; either version 2 of     *
# *   the License, or (at your option) any later version.                   *
# *   for detail see the LICENCE text file.                                 *
# *                                                                         *
# *   This program is distributed in the hope that it will be useful,       *
# *   but WITHOUT ANY WARRANTY; without even the implied warranty of        *
# *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the         *
# *   GNU Library General Public License for more details.                  *
# *                                                                         *
# *   You should have received a copy of the GNU Library General Public     *
# *   License along with this program; if not, write to the Free Software   *
# *   Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  *
# *   USA                                                                   *
# *                                                                         *
# ***************************************************************************

__title__ = "FemMesh to Mesh converter"
__author__ = "Frantisek Loeffelmann, Ulrich Brammer, Bernd Hahnebach, Peter McB"
__url__ = "https://www.freecad.org"

## @package FwmMesh2Mesh
#  \ingroup FEM

import time, os

import FreeCAD
import FreeCADGui
import Fem

# import Mesh

"""
from os.path import join
the_file = join(FreeCAD.getResourceDir(), "examples", "FemCalculixCantilever3D.FCStd")
fc_file = FreeCAD.openDocument(the_file)
fem_mesh = fc_file.getObject("Box_Mesh").FemMesh  # do not remove the _
result = fc_file.getObject("CCX_Results")
scale = 1  # displacement scale factor
from femmesh import femmesh2mesh
out_mesh = femmesh2mesh.femmesh_2_mesh(fem_mesh, result, scale)
import Mesh
Mesh.show(Mesh.Mesh(out_mesh))

"""
# These dictionaries list the nodes, that define faces of an element.
# The key is the face number, used internally by FreeCAD.
# The list contains the nodes in the element for each face.
tetFaces = {1: [0, 1, 2], 2: [0, 3, 1], 3: [1, 3, 2], 4: [2, 3, 0]}

pentaFaces = {1: [0, 1, 2], 2: [3, 5, 4], 3: [0, 3, 4, 1], 4: [1, 4, 5, 2], 5: [0, 2, 5, 3]}

hexaFaces = {  # hexa8 or hexa20 (ignoring mid-nodes)
    1: [0, 1, 2, 3],
    2: [4, 7, 6, 5],
    3: [0, 4, 5, 1],
    4: [1, 5, 6, 2],
    5: [2, 6, 7, 3],
    6: [3, 7, 4, 0],
}

pyraFaces = {  # pyra5 or pyra13 (ignoring mid-nodes)
    1: [0, 1, 2, 3],
    2: [0, 4, 1],
    3: [1, 4, 2],
    4: [2, 4, 3],
    5: [3, 4, 0],
}

face_dicts = {
    4: tetFaces,
    5: pyraFaces,
    6: pentaFaces,
    8: hexaFaces,
    10: tetFaces,
    13: pyraFaces,
    15: pentaFaces,
    20: hexaFaces,
}


def femmeshtofacets(fm):
    nodes = fm.Nodes
    facets = []
    for i in fm.FacesOnly:
        t = fm.getElementNodes(i)
        facet = [fm.getNodeById(t[0]), fm.getNodeById(t[1]), fm.getNodeById(t[2])]
        facets.append(facet)
    return facets


def femmeshtofacetsControlPoints(fm):
    facets = []
    for i in fm.FacesOnly:
        t = fm.getElementNodes(i)
        cfacets = [
            [fm.getNodeById(t[0]), fm.getNodeById(t[3]), fm.getNodeById(t[5])],
            [fm.getNodeById(t[5]), fm.getNodeById(t[3]), fm.getNodeById(t[4])],
            [fm.getNodeById(t[3]), fm.getNodeById(t[1]), fm.getNodeById(t[4])],
            [fm.getNodeById(t[5]), fm.getNodeById(t[4]), fm.getNodeById(t[2])],
        ]
        facets.extend(cfacets)
    return facets


def convertmidpointstocontrolpoints(tria6):
    c3 = tria6[3] * 2 - tria6[0] * 0.5 - tria6[1] * 0.5
    c4 = tria6[4] * 2 - tria6[1] * 0.5 - tria6[2] * 0.5
    c5 = tria6[5] * 2 - tria6[2] * 0.5 - tria6[0] * 0.5
    return (tria6[0], tria6[1], tria6[2], c3, c4, c5)


def femmeshtofacetsSubdiv(fm, nsubdivs):
    nrows = nsubdivs + 1
    facets = []
    for i in fm.FacesOnly:
        tria6 = [fm.getNodeById(j) for j in fm.getElementNodes(i)]
        if not bflateval:
            tria6 = convertmidpointstocontrolpoints(tria6)
        prevrow = []
        tfacets = []
        for r in range(nrows + 1):
            s = 1 - r / nrows
            row = []
            for m in range(r + 1):
                t = (m / r if r != 0 else 0) * (1 - s)
                u = 1 - s - t
                row.append(evaltria6(tria6, s, t, u))
            for a in range(len(prevrow)):
                tfacets.append([row[a], prevrow[a], row[a + 1]])
                if a + 1 < len(prevrow):
                    tfacets.append([prevrow[a], prevrow[a + 1], row[a + 1]])
            prevrow = row
        facets.extend(tfacets)
    return facets


bflateval = False


def evaltria6(tria6, s, t, u):
    if bflateval:
        return tria6[0] * s + tria6[1] * t + tria6[2] * u

    return (
        tria6[0] * (s * s)
        + tria6[1] * (t * t)
        + tria6[2] * (u * u)
        + tria6[4] * (2 * t * u)
        + tria6[5] * (2 * u * s)
        + tria6[3] * (2 * s * t)
    )


def femmesh_2_mesh(myFemMesh, myResults=None, myDispScale=1, subdivisions=0):
    if subdivisions != 0:
        return femmeshtofacetsSubdiv(myFemMesh, subdivisions)

    shiftBits = 20  # allows a million nodes, needs to be higher for more nodes in a FemMesh

    # This code generates a dict and a faceCode for each face of all elements
    # All faceCodes are than sorted.

    start_time = time.process_time()
    faceCodeList = []
    faceCodeDict = {}

    if myFemMesh.VolumeCount > 0:
        for ele in myFemMesh.Volumes:
            element_nodes = myFemMesh.getElementNodes(ele)
            # print("element_node: ", element_nodes)
            faceDef = face_dicts[len(element_nodes)]

            for key in faceDef:
                nodeList = []
                codeList = []
                faceCode = 0
                shifter = 0
                for nodeIdx in faceDef[key]:
                    nodeList.append(element_nodes[nodeIdx])
                    codeList.append(element_nodes[nodeIdx])
                codeList.sort()
                for node in codeList:
                    faceCode += node << shifter
                    # x << n: x shifted left by n bits = Multiplication
                    shifter += shiftBits
                # print("codeList: ", codeList)
                faceCodeDict[faceCode] = nodeList
                faceCodeList.append(faceCode)
    elif myFemMesh.FaceCount > 0:
        for ele in myFemMesh.Faces:
            element_nodes = myFemMesh.getElementNodes(ele)
            # print("element_node: ", element_nodes)
            if len(element_nodes) in [3, 6]:
                faceDef = {1: [0, 1, 2]}
            else:  # quad element
                faceDef = {1: [0, 1, 2, 3]}

            for key in faceDef:
                nodeList = []
                codeList = []
                faceCode = 0
                shifter = 0
                for nodeIdx in faceDef[key]:
                    nodeList.append(element_nodes[nodeIdx])
                    codeList.append(element_nodes[nodeIdx])
                codeList.sort()
                for node in codeList:
                    faceCode += node << shifter
                    # x << n: x shifted left by n bits = Multiplication
                    shifter += shiftBits
                # print("codeList: ", codeList)
                faceCodeDict[faceCode] = nodeList
                faceCodeList.append(faceCode)

    faceCodeList.sort()
    allFaces = len(faceCodeList)
    actFaceIdx = 0
    singleFaces = []
    # Here we search for faces, which do not have a counterpart.
    # These are the faces on the surface of the mesh.
    while actFaceIdx < allFaces:
        if actFaceIdx < (allFaces - 1):
            if faceCodeList[actFaceIdx] == faceCodeList[actFaceIdx + 1]:
                actFaceIdx += 2
            else:
                # print("found a single Face: ", faceCodeList[actFaceIdx])
                singleFaces.append(faceCodeList[actFaceIdx])
                actFaceIdx += 1
        else:
            FreeCAD.Console.PrintMessage(f"Found a last Face: {faceCodeList[actFaceIdx]}\n")
            singleFaces.append(faceCodeList[actFaceIdx])
            actFaceIdx += 1

    output_mesh = []
    if myResults:
        FreeCAD.Console.PrintMessage(f"{myResults.Name}\n")
        for myFace in singleFaces:
            face_nodes = faceCodeDict[myFace]
            dispVec0 = myResults.DisplacementVectors[myResults.NodeNumbers.index(face_nodes[0])]
            dispVec1 = myResults.DisplacementVectors[myResults.NodeNumbers.index(face_nodes[1])]
            dispVec2 = myResults.DisplacementVectors[myResults.NodeNumbers.index(face_nodes[2])]
            triangle = [
                myFemMesh.getNodeById(face_nodes[0]) + dispVec0 * myDispScale,
                myFemMesh.getNodeById(face_nodes[1]) + dispVec1 * myDispScale,
                myFemMesh.getNodeById(face_nodes[2]) + dispVec2 * myDispScale,
            ]
            output_mesh.extend(triangle)
            # print("my triangle: ", triangle)
            if len(face_nodes) == 4:
                dispVec3 = myResults.DisplacementVectors[myResults.NodeNumbers.index(face_nodes[3])]
                triangle = [
                    myFemMesh.getNodeById(face_nodes[2]) + dispVec2 * myDispScale,
                    myFemMesh.getNodeById(face_nodes[3]) + dispVec3 * myDispScale,
                    myFemMesh.getNodeById(face_nodes[0]) + dispVec0 * myDispScale,
                ]
                output_mesh.extend(triangle)
                # print("my 2. triangle: ", triangle)

    else:
        for myFace in singleFaces:
            face_nodes = faceCodeDict[myFace]
            triangle = [
                myFemMesh.getNodeById(face_nodes[0]),
                myFemMesh.getNodeById(face_nodes[1]),
                myFemMesh.getNodeById(face_nodes[2]),
            ]
            output_mesh.extend(triangle)
            # print("my triangle: ", triangle)
            if len(face_nodes) == 4:
                triangle = [
                    myFemMesh.getNodeById(face_nodes[2]),
                    myFemMesh.getNodeById(face_nodes[3]),
                    myFemMesh.getNodeById(face_nodes[0]),
                ]
                output_mesh.extend(triangle)
                # print("my 2. triangle: ", triangle)

    end_time = time.process_time()
    FreeCAD.Console.PrintMessage(f"Mesh by surface search method: {end_time - start_time}\n")
    # call to mesh_2_femmesh to convert mesh to femmesh before return statement
    mesh2femmesh = mesh_2_femmesh(myFemMesh, singleFaces, faceCodeDict)
    return output_mesh


# additional function to convert mesh to femmesh
def mesh_2_femmesh(myFemMesh, singleFaces, faceCodeDict):
    start_time = time.process_time()
    femmesh = Fem.FemMesh()
    myfemmesh = myFemMesh.Nodes
    # nodes contains the nodes that are used
    nodes = {}
    for myFace in singleFaces:
        face_nodes = faceCodeDict[myFace]
        for j in (0, 1, 2):
            try:
                nodes[face_nodes[j]] += 1
            except:
                nodes[face_nodes[j]] = 0
        if len(face_nodes) == 4:
            j = 3
            try:
                nodes[face_nodes[j]] += 1
            except:
                nodes[face_nodes[j]] = 0
    sfNode = femmesh.addNode
    sfFace = femmesh.addFace
    for key in myFemMesh.Nodes:
        mynode = myfemmesh[key]
        try:
            if nodes[key] >= 0:
                sfNode(mynode[0], mynode[1], mynode[2], key)
        except:
            pass

    output_mesh = []

    for myFace in singleFaces:
        face_nodes = faceCodeDict[myFace]
        sfFace(face_nodes[0], face_nodes[1], face_nodes[2])
        if len(face_nodes) == 4:
            sfFace(face_nodes[2], face_nodes[3], face_nodes[0])
    obj = FreeCAD.ActiveDocument.addObject("Fem::FemMeshObject", "Mesh2Fem")
    obj.FemMesh = femmesh
    end_time = time.process_time()
    FreeCAD.Console.PrintMessage(f"Convert to FemMesh: {end_time - start_time}\n")
    return obj


# end of mesh_2_femmesh
