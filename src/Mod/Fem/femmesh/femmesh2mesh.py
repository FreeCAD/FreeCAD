# ***************************************************************************
# *                                                                         *
# *   Copyright (c) 2016 - Frantisek Loeffelmann <LoffF@email.cz>           *
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
__author__ = "Frantisek Loeffelmann, Ulrich Brammer, Bernd Hahnebach"
__url__ = "http://www.freecadweb.org"

## @package FwmMesh2Mesh
#  \ingroup FEM

import time
# import Mesh


'''
load FreeCADs 3D FEM example from Start Workbench

femmesh = App.ActiveDocument.getObject("Box_Mesh").FemMesh
result = App.ActiveDocument.getObject("CalculiX_static_results")
import femmesh.femmesh2mesh
out_mesh = femmesh.femmesh2mesh.femmesh_2_mesh(femmesh, result)
import Mesh
Mesh.show(Mesh.Mesh(out_mesh))

'''
# These dictionaries list the nodes, that define faces of an element.
# The key is the face number, used internally by FreeCAD.
# The list contains the nodes in the element for each face.
tetFaces = {
    1: [0, 1, 2],
    2: [0, 3, 1],
    3: [1, 3, 2],
    4: [2, 3, 0]}

pentaFaces = {
    1: [0, 1, 2],
    2: [3, 5, 4],
    3: [0, 3, 4, 1],
    4: [1, 4, 5, 2],
    5: [0, 2, 5, 3]}

hexaFaces = {        # hexa8 or hexa20 (ignoring mid-nodes)
    1: [0, 1, 2, 3],
    2: [4, 7, 6, 5],
    3: [0, 4, 5, 1],
    4: [1, 5, 6, 2],
    5: [2, 6, 7, 3],
    6: [3, 7, 4, 0]}

pyraFaces = {      # pyra5 or pyra13 (ignoring mid-nodes)
    1: [0, 1, 2, 3],
    2: [0, 4, 1],
    3: [1, 4, 2],
    4: [2, 4, 3],
    5: [3, 4, 0]}

face_dicts = {
    4: tetFaces,
    5: pyraFaces,
    6: pentaFaces,
    8: hexaFaces,
    10: tetFaces,
    13: pyraFaces,
    15: pentaFaces,
    20: hexaFaces}


def femmesh_2_mesh(myFemMesh, myResults=None):
    shiftBits = 20  # allows a million nodes, needs to be higher for more nodes in a FemMesh

    # This code generates a dict and a faceCode for each face of all elements
    # All faceCodes are than sorted.

    start_time = time.clock()
    faceCodeList = []
    faceCodeDict = {}

    if myFemMesh.VolumeCount > 0:
        for ele in myFemMesh.Volumes:
            element_nodes = myFemMesh.getElementNodes(ele)
            # print('element_node: ', element_nodes)
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
                    faceCode += (node << shifter)
                    # x << n: x shifted left by n bits = Multiplication
                    shifter += shiftBits
                # print('codeList: ', codeList)
                faceCodeDict[faceCode] = nodeList
                faceCodeList.append(faceCode)
    elif myFemMesh.FaceCount > 0:
        for ele in myFemMesh.Faces:
            element_nodes = myFemMesh.getElementNodes(ele)
            # print('element_node: ', element_nodes)
            faceDef = {1: [0, 1, 2]}

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
                    faceCode += (node << shifter)
                    # x << n: x shifted left by n bits = Multiplication
                    shifter += shiftBits
                # print('codeList: ', codeList)
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
                # print('found a single Face: ', faceCodeList[actFaceIdx])
                singleFaces.append(faceCodeList[actFaceIdx])
                actFaceIdx += 1
        else:
            print('found a last Face: ', faceCodeList[actFaceIdx])
            singleFaces.append(faceCodeList[actFaceIdx])
            actFaceIdx += 1

    output_mesh = []
    if myResults:
        print(myResults.Name)
        for myFace in singleFaces:
            face_nodes = faceCodeDict[myFace]
            dispVec0 = myResults.DisplacementVectors[myResults.NodeNumbers.index(face_nodes[0])]
            dispVec1 = myResults.DisplacementVectors[myResults.NodeNumbers.index(face_nodes[1])]
            dispVec2 = myResults.DisplacementVectors[myResults.NodeNumbers.index(face_nodes[2])]
            triangle = [myFemMesh.getNodeById(face_nodes[0]) + dispVec0,
                        myFemMesh.getNodeById(face_nodes[1]) + dispVec1,
                        myFemMesh.getNodeById(face_nodes[2]) + dispVec2]
            output_mesh.extend(triangle)
            # print('my triangle: ', triangle)
            if len(face_nodes) == 4:
                dispVec3 = myResults.DisplacementVectors[myResults.NodeNumbers.index(face_nodes[3])]
                triangle = [myFemMesh.getNodeById(face_nodes[2]) + dispVec2,
                            myFemMesh.getNodeById(face_nodes[3]) + dispVec3,
                            myFemMesh.getNodeById(face_nodes[0]) + dispVec0]
                output_mesh.extend(triangle)
                # print('my 2. triangle: ', triangle)

    else:
        for myFace in singleFaces:
            face_nodes = faceCodeDict[myFace]
            triangle = [myFemMesh.getNodeById(face_nodes[0]),
                        myFemMesh.getNodeById(face_nodes[1]),
                        myFemMesh.getNodeById(face_nodes[2])]
            output_mesh.extend(triangle)
            # print('my triangle: ', triangle)
            if len(face_nodes) == 4:
                triangle = [myFemMesh.getNodeById(face_nodes[2]),
                            myFemMesh.getNodeById(face_nodes[3]),
                            myFemMesh.getNodeById(face_nodes[0])]
                output_mesh.extend(triangle)
                # print('my 2. triangle: ', triangle)

    end_time = time.clock()
    print('Mesh by surface search method: ', end_time - start_time)
    return output_mesh
