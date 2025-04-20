# SPDX-License-Identifier: LGPL-2.1-or-later

# ***************************************************************************
# *                                                                         *
# *   Copyright (c) 2013 Yorik van Havre <yorik@uncreated.net>              *
# *   Copyright (c) 2020 Travis Apple <travisapple@gmail.com>               *
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

# REFS:
# https://github.com/mrdoob/three.js/blob/master/examples/webgl_interactive_buffergeometry.html
# https://threejs.org/examples/#webgl_buffergeometry_lines
# https://forum.freecad.org/viewtopic.php?t=51245
# https://forum.freecad.org/viewtopic.php?t=29487
# https://threejs.org/examples/#webgl_raycast_sprite
#
# Params for export()
#    'colors' is of the form: {'Body': [1,0,0], 'Body001': [1,1,0], 'Body002': [1,0,1] }
#    'camera' is of the form: "PerspectiveCamera {\n  viewportMapping ADJUST_CAMERA\n  position 30.242626 -51.772324 85.63475\n  orientation -0.4146691 0.088459305 -0.90566254  4.7065201\nnearDistance 53.126431\n  farDistance 123.09125\n  aspectRatio 1\n  focalDistance 104.53851\n  heightAngle 0.78539819\n\n}"
#    The 'camera' string for the active document may be generated from: import OfflineRenderingUtils; OfflineRenderingUtils.getCamera(FreeCAD.ActiveDocument.FileName);
#
# Development reload oneliner:
# def re(): from importlib import reload;import importWebGL;reload(importWebGL);o=FreeCAD.getDocument("YourDocName");importWebGL.export([o.getObject("YourBodyName")],u"C:/path/to/your/file.htm");

## @package importWebGL
#  \ingroup ARCH
#  \brief FreeCAD WebGL Exporter
#
#  This module provides tools to export HTML files containing the
#  exported objects in WebGL format and a simple three.js-based viewer.

"""FreeCAD WebGL Exporter"""

import json
import textwrap
from builtins import open as pyopen
from typing import NotRequired, TypedDict

import numpy as np

import FreeCAD
import Draft
import Mesh
import OfflineRenderingUtils
import Part

if FreeCAD.GuiUp:
    import FreeCADGui
    from draftutils.translate import translate
else:
    FreeCADGui = None

    def translate(ctxt, txt):
        return txt


disableCompression = False  # Compress object data before sending to JS
base = "abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ1234567890!#$%&()*+-:;/=>?@[]^_,.{|}~`"  # safe str chars for js in all cases
baseFloat = ",.-0123456789"
threejs_version = "0.172.0"


def getHTMLTemplate():
    return textwrap.dedent("""\
    <!DOCTYPE html>
    <html lang="en">
        <head>
            <title>$pagetitle</title>
            <meta charset="utf-8">
            <meta name="viewport" content="width=device-width, user-scalable=no, minimum-scale=1.0, maximum-scale=1.0">
            <meta name="generator" content="FreeCAD $version">
            <style>
                * {
                    margin: 0;
                    padding: 0;
                }
                body {
                    background: #ffffff; /* Old browsers */
                    background: -moz-linear-gradient(top, #e3e9fc 0%, #ffffff 70%, #e2dab3 100%); /* FF3.6-15 */
                    background: -webkit-linear-gradient(top, #e3e9fc 0%,#ffffff 70%,#e2dab3 100%); /* Chrome10-25, Safari5.1-6 */
                    background: linear-gradient(to bottom, #e3e9fc 0%,#ffffff 70%,#e2dab3 100%); /* W3C, IE10+, FF16+, Chrome26+, Opera12+, Safari7+ */
                    width: 100vw;
                    height: 100vh;
                }
                canvas { display: block; }
                #mainCanvas {
                    width: 100%;
                    height: 100%;
                }
                #arrowCanvas  {
                    position: absolute;
                    left: 0px;
                    bottom: 0px;
                    width: 150px;
                    height: 150px;
                    z-index: 100;
                }
                select { width: 170px; }
            </style>
        </head>
        <script type="importmap">
            {
                "imports": {
                    "three": "https://cdn.jsdelivr.net/npm/three@$threejs_version/build/three.module.js",
                    "three/addons/": "https://cdn.jsdelivr.net/npm/three@$threejs_version/examples/jsm/"
                }
            }
        </script>
        <body>
            <canvas id="mainCanvas"></canvas>
            <canvas id="arrowCanvas"></canvas>
            <script type="module">
                // Direct from mrdoob: https://www.jsdelivr.com/package/npm/three
                import * as THREE from 'three'
                import { OrbitControls } from     'three/addons/controls/OrbitControls.js';
                import { GUI } from               'three/addons/libs/lil-gui.module.min.js';
                import { Line2 } from             'three/addons/lines/Line2.js';
                import { LineMaterial } from      'three/addons/lines/LineMaterial.js';
                import { LineGeometry } from      'three/addons/lines/LineGeometry.js';
                import { EdgeSplitModifier } from 'three/addons/modifiers/EdgeSplitModifier.js';

                const data = $data;

                // Z is up for FreeCAD
                THREE.Object3D.DEFAULT_UP = new THREE.Vector3(0, 0, 1);

                const defaultWireColor = new THREE.Color('rgb(0,0,0)');
                const defaultWireLineWidth = 2; // in pixels

                const raycasterObj = []; // list of obj that can mouseover highlight

                const canvas = document.querySelector('#mainCanvas');

                const scene = new THREE.Scene();

                const renderer = new THREE.WebGLRenderer({
                    alpha: true,
                    antialias: true,
                    canvas: canvas
                }); // Clear bg so we can set it with css
                renderer.setClearColor(0x000000, 0);

                let renderRequested = false;

                // HemisphereLight gives different colors of light from the top
                // and bottom simulating reflected light from the 'ground' and
                // 'sky'
                scene.add(new THREE.HemisphereLight(0xC7E8FF, 0xFFE3B3, 0.4));

                const dLight1 = new THREE.DirectionalLight(0xffffff, 0.4);
                dLight1.position.set(5, -2, 3);
                scene.add(dLight1);
                const dLight2 = new THREE.DirectionalLight(0xffffff, 0.4);
                dLight2.position.set(-5, 2, 3);
                scene.add(dLight2);

                if (data.compressed) {
                    const base = data.base;
                    const baseFloat = data.baseFloat;

                    function baseDecode(input) {
                        const baseCt = base.length;
                        const output = [];
                        const len = parseInt(input[0]); // num chars of each element
                        for (let i = 1; i < input.length; i += len) {
                            const str = input.substring(i, i + len).trim();
                            let val = 0;
                            for (let s = 0; s < str.length; s++) {
                                const ind = base.indexOf(str[s]);
                                val += ind * Math.pow(baseCt, s);
                            }
                            output.push(val);
                        }
                        return output;
                    }

                    function floatDecode(input) {
                        const baseCt = base.length;
                        const baseFloatCt = baseFloat.length;
                        let numString = '';
                        for (let i = 0; i < input.length; i += 4) {
                            const b90chunk = input.substring(i, i + 4).trim();
                            let quotient = 0;
                            for (let s = 0; s < b90chunk.length; s++) {
                                const ind = base.indexOf(b90chunk[s]);
                                quotient += ind * Math.pow(baseCt, s);
                            }
                            let buffer = '';
                            for (let s = 0; s < 7; s++) {
                                buffer = baseFloat[quotient % baseFloatCt] + buffer;
                                quotient = parseInt(quotient / baseFloatCt);
                            }
                            numString += buffer;
                        }
                        let trailingCommas = 0;
                        for (let s = 1; s < 7; s++) {
                            if (numString[numString.length - s] == baseFloat[0]) {
                                trailingCommas++;
                            }
                        }
                        numString = numString.substring(0, numString.length - trailingCommas);
                        return numString;
                    }

                    // Decode from base90 and distribute the floats
                    for (const obj of data.objects) {
                        obj.floats = JSON.parse('[' + floatDecode(obj.floats) + ']');
                        obj.verts = baseDecode(obj.verts).map(x => obj.floats[x]);
                        obj.facets = baseDecode(obj.facets);
                        obj.wires = obj.wires.map(w => baseDecode(w).map(x => obj.floats[x]));
                        obj.facesToFacets = obj.facesToFacets.map(x => baseDecode(x));
                    }
                }
                else {
                    for (const obj of data.objects) {
                        obj.verts = obj.verts.map(x => parseFloat(x));
                        obj.wires = obj.wires.map(w => w.map(x => parseFloat(x)))
                        obj.facesToFacets = obj.facesToFacets.map(w => w.map(x => parseFloat(x)));
                    }
                }
                // Get bounds for global clipping
                const globalMaxMin = [{min: null, max: null},
                                      {min: null, max: null},
                                      {min: null, max: null}];
                for (const obj of data.objects) {
                    for (let v = 0; v < obj.verts.length; v++) {
                        if (globalMaxMin[v % 3] === null
                            || obj.verts[v] < globalMaxMin[v % 3].min) {
                            globalMaxMin[v % 3].min = obj.verts[v];
                        }
                        if (globalMaxMin[v % 3] === null
                            || obj.verts[v] > globalMaxMin[v % 3].max) {
                            globalMaxMin[v % 3].max = obj.verts[v];
                        }
                    }
                }
                let bigrange = 0;
                // add a little extra
                for (const i of globalMaxMin) {
                    const range = i.max - i.min;
                    if (range > bigrange) {
                        bigrange = range;
                    }
                    i.min -= range * 0.01;
                    i.max += range * 0.01;
                }

                const camCenter = new THREE.Vector3(
                    0.5 * (globalMaxMin[0].max - globalMaxMin[0].min) + globalMaxMin[0].min,
                    0.5 * (globalMaxMin[1].max - globalMaxMin[1].min) + globalMaxMin[1].min,
                    0.5 * (globalMaxMin[2].max - globalMaxMin[2].min) + globalMaxMin[2].min );
                const viewSize = 1.5 * bigrange; // make the view area a little bigger than the object
                const aspectRatio = canvas.clientWidth / canvas.clientHeight;
                const originalAspect = aspectRatio;

                function initCam(camera) {
                    // XXX this needs to treat the perspective and orthographic
                    // cameras differently
                    camera.position.set(
                        data.camera.position_x,
                        data.camera.position_y,
                        data.camera.position_z);
                    camera.lookAt(camCenter);
                    camera.updateMatrixWorld();
                }

                let cameraType = data.camera.type;
                const persCamera = new THREE.PerspectiveCamera(
                    50, aspectRatio, 1, 100000);
                initCam(persCamera);
                const orthCamera = new THREE.OrthographicCamera(
                    -aspectRatio * viewSize / 2, aspectRatio * viewSize / 2,
                    viewSize / 2, -viewSize / 2, -100000, 100000);
                initCam(orthCamera);

                function assignMesh(positions, color, opacity, faces) {
                    const baseGeometry = new THREE.BufferGeometry();
                    baseGeometry.setAttribute('position', new THREE.BufferAttribute(
                        positions, 3));

                    // EdgeSplitModifier is used to combine verts so that smoothing normals can be generated WITHOUT removing the hard edges of the design
                    // REF: https://threejs.org/examples/?q=edge#webgl_modifier_edgesplit - https://github.com/mrdoob/three.js/pull/20535
                    const edgeSplit = new EdgeSplitModifier();
                    const cutOffAngle = 20;
                    const geometry = edgeSplit.modify(
                        baseGeometry, cutOffAngle * Math.PI / 180);
                    geometry.computeVertexNormals();
                    geometry.computeBoundingSphere();

                    const material = new THREE.MeshLambertMaterial({
                        color: color,
                        side: THREE.DoubleSide,
                        vertexColors: false,
                        opacity: opacity,
                        transparent: opacity != 1.0,
                        fog: false
                    });

                    const meshobj = new THREE.Mesh(geometry, material);
                    meshobj.name = meshobj.uuid;
                    faces.push(meshobj.uuid);
                    scene.add(meshobj);
                    raycasterObj.push(meshobj);
                }

                const objects = [];
                for (const obj of data.objects) {
                    // Each face gets its own material because they each can
                    // have different colors
                    const faces = [];
                    if (obj.facesToFacets.length > 0) {
                        for (let f=0; f < obj.facesToFacets.length; f++) {
                            const facecolor = obj.faceColors.length > 0 ? obj.faceColors[f] : obj.color;
                            const positions = new Float32Array(obj.facesToFacets[f].length * 9);
                            for (let a=0; a < obj.facesToFacets[f].length; a++) {
                                for (let b=0; b < 3; b++) {
                                    for (let c=0; c < 3; c++) {
                                        positions[9 * a + 3 * b + c] = obj.verts[3 * obj.facets[3 * obj.facesToFacets[f][a] + b ] + c ];
                                    }
                                }
                            }
                            assignMesh(positions, facecolor, obj.opacity, faces);
                        }
                    } else {
                        // No facesToFacets means that there was a tessellate()
                        // mismatch inside FreeCAD. Use all facets in object to
                        // create this mesh
                        const positions = new Float32Array(obj.facets.length * 3);
                        for (let a=0; a < obj.facets.length; a++) {
                            for (let b=0; b < 3; b++) {
                                positions[3 * a + b] = obj.verts[3 * obj.facets[a] + b];
                            }
                        }
                        assignMesh(positions, obj.color, obj.opacity, faces);
                    }

                    // Wires
                    // cannot have lines in WebGL that are wider than 1px due to browser limitations so Line2 workaround lib is used
                    // REF: https://threejs.org/examples/?q=fat#webgl_lines_fat - https://jsfiddle.net/brLk6aud/1/
                    // This material is shared by all wires in this object
                    const wirematerial = new LineMaterial( {
                        color: defaultWireColor,
                        linewidth: defaultWireLineWidth,
                        dashed: false, dashSize: 1, gapSize: 1, dashScale: 3
                    } );
                    wirematerial.resolution.set(
                        canvas.clientWidth * window.devicePixelRatio,
                        canvas.clientHeight * window.devicePixelRatio);

                    const wires = [];
                    for (const w of obj.wires) {
                        const wiregeometry = new LineGeometry();
                        wiregeometry.setPositions(w);
                        const wire = new Line2(wiregeometry, wirematerial);
                        wire.computeLineDistances();
                        wire.scale.set(1, 1, 1);
                        wire.name = wire.uuid;
                        scene.add(wire);
                        wires.push(wire.name);
                    }
                    objects.push({
                        data: obj,
                        faces: faces,
                        wires: wires,
                        wirematerial: wirematerial,
                        gui_link: null
                    });
                }

                // ---- GUI Init ----
                const gui = new GUI({ width: 300, closeFolders: true });
                const addFolder = GUI.prototype.addFolder;
                GUI.prototype.addFolder = function(...args) {
                    return addFolder.call(this, ...args).close();
                }
                const guiparams = {
                    wiretype: 'Normal',
                    wirewidth: defaultWireLineWidth,
                    wirecolor: '#' + defaultWireColor.getHexString(),
                    clippingx: 100,
                    clippingy: 100,
                    clippingz: 100,
                    cameraType: cameraType,
                    navright: function() { navChange([1,  0, 0]); },
                    navtop:   function() { navChange([0,  0, 1]); },
                    navfront: function() { navChange([0, -1, 0]); }
                };

                // ---- Wires ----
                const wiretypes = { Normal: 'Normal', Dashed: 'Dashed', None: 'None' };

                const wireFolder = gui.addFolder('Wire');
                wireFolder.add(guiparams, 'wiretype', wiretypes).name('Wire Display').onChange(wireChange);
                wireFolder.add(guiparams, 'wirewidth').min(1).max(5).step(1).name('Wire Width').onChange(wireChange);
                wireFolder.addColor(guiparams, 'wirecolor').name('Wire Color').onChange(wireChange);

                function wireChange() {
                    for (const obj of objects) {
                        const m = obj.wirematerial;
                        if (m.dashed) {
                            if (guiparams.wiretype != 'Dashed') {
                                m.dashed = false;
                                delete m.defines.USE_DASH;
                            }
                        } else {
                            if (guiparams.wiretype == 'Dashed') {
                                m.dashed = true;
                                // Dashed lines require this as of r122. delete if not dashed
                                m.defines.USE_DASH = ""; // https://discourse.threejs.org/t/dashed-line2-material/10825
                            }
                        }
                        if (guiparams.wiretype == 'None') {
                            m.visible = false;
                        } else {
                            if ((obj.faces.length == 0) | scene.getObjectByName(obj.faces[0]).material.visible){
                                m.visible = true;
                            }
                        }
                        m.linewidth = guiparams.wirewidth;
                        m.color = new THREE.Color(guiparams.wirecolor);
                        m.needsUpdate = true;
                    }
                    requestRender();
                }
                wireChange();

                // ---- Clipping ----
                const clippingFolder = gui.addFolder('Clipping');
                clippingFolder.add(guiparams, 'clippingx').min(0).max(100).step(1).name('X-Axis Clipping').onChange(clippingChange);
                clippingFolder.add(guiparams, 'clippingy').min(0).max(100).step(1).name('Y-Axis Clipping').onChange(clippingChange);
                clippingFolder.add(guiparams, 'clippingz').min(0).max(100).step(1).name('Z-Axis Clipping').onChange(clippingChange);

                const clipPlaneX = new THREE.Plane(new THREE.Vector3( -1, 0, 0 ), 0);
                const clipPlaneY = new THREE.Plane(new THREE.Vector3( 0, -1, 0 ), 0);
                const clipPlaneZ = new THREE.Plane(new THREE.Vector3( 0, 0, -1 ), 0);

                function clippingChange() {
                    if (guiparams.clippingx < 100 || guiparams.clippingy < 100 || guiparams.clippingz < 100) {
                        if (renderer.clippingPlanes.length == 0) {
                            renderer.clippingPlanes.push(clipPlaneX, clipPlaneY, clipPlaneZ);
                        }
                    }
                    clipPlaneX.constant = (globalMaxMin[0].max - globalMaxMin[0].min) * guiparams.clippingx / 100.0 + globalMaxMin[0].min;
                    clipPlaneY.constant = (globalMaxMin[1].max - globalMaxMin[1].min) * guiparams.clippingy / 100.0 + globalMaxMin[1].min;
                    clipPlaneZ.constant = (globalMaxMin[2].max - globalMaxMin[2].min) * guiparams.clippingz / 100.0 + globalMaxMin[2].min;
                    requestRender();
                }

                // ---- Camera & Navigation ----
                const camFolder = gui.addFolder('Camera');
                const cameraTypes = { Perspective: 'Perspective', Orthographic: 'Orthographic' };
                camFolder.add(guiparams, 'cameraType', cameraTypes).name('Camera type').onChange(cameraChange);
                camFolder.add(guiparams, 'navright').name('View Right');
                camFolder.add(guiparams, 'navtop').name('View Top');
                camFolder.add(guiparams, 'navfront').name('View Front');

                function navChange(v) {
                    const t = new THREE.Vector3();
                    new THREE.Box3().setFromObject(scene).getSize(t);
                    persControls.object.position.set(
                        v[0] * t.x * 2 + camCenter.x,
                        v[1] * t.y * 2 + camCenter.y,
                        v[2] * t.z * 2 + camCenter.z);
                    persControls.target = camCenter;
                    persControls.update();
                    orthControls.object.position.set(
                        v[0] * t.x + camCenter.x,
                        v[1] * t.y + camCenter.y,
                        v[2] * t.z + camCenter.z);
                    orthControls.target = camCenter;
                    orthControls.update();
                    // controls.update() implicitly calls requestRender()
                }

                function cameraChange(v) {
                    cameraType = v;
                    requestRender();
                }

                const guiObjects = gui.addFolder('Objects');
                for (const obj of objects) {
                    // Ignore objects with no vertices
                    if (obj.data.verts.length > 0) {
                        const guiObjData = {
                            obj: obj, color: obj.data.color, opacity: obj.data.opacity, show: true };
                        const guiObject = guiObjects.addFolder(obj.data.name);
                        guiObject.addColor(guiObjData, 'color').name('Color').onChange(GUIObjectChange);
                        guiObject.add(guiObjData, 'opacity').min(0.0).max(1.0).step(0.05).name('Opacity').onChange(GUIObjectChange);
                        guiObject.add(guiObjData, 'show').onChange(GUIObjectChange).listen();
                        obj.gui_link = guiObjData
                    }
                }

                function GUIObjectChange(v) {
                    for (const f of this.object.obj.faces) {
                        const m = scene.getObjectByName(f).material;
                        if (this.property == 'color') {
                            m.color.setStyle(v);
                        }
                        if (this.property == 'opacity') {
                            m.opacity = v;
                            m.transparent = (v != 1.0);
                        }
                        if (this.property == 'show') {
                            m.visible = v
                        }
                    }
                    if (this.property == 'opacity') {
                        const m = this.object.obj.wirematerial;
                        m.opacity = v;
                        m.transparent = (v != 1.0);
                    }
                    if (this.property == 'show') {
                        const m = this.object.obj.wirematerial;
                        m.visible = v
                    }
                    requestRender();
                }


                // Make simple orientation arrows and box - REF: http://jsfiddle.net/b97zd1a3/16/
                const arrowCanvas = document.querySelector('#arrowCanvas');
                const arrowRenderer = new THREE.WebGLRenderer({
                    alpha: true,
                    canvas: arrowCanvas
                }); // clear
                arrowRenderer.setClearColor(0x000000, 0);
                arrowRenderer.setSize(arrowCanvas.clientWidth * window.devicePixelRatio,
                                      arrowCanvas.clientHeight * window.devicePixelRatio,
                                      false);

                const arrowScene = new THREE.Scene();

                const arrowCamera = new THREE.PerspectiveCamera(
                    50, arrowCanvas.clientWidth / arrowCanvas.clientHeight, 1, 500 );
                arrowCamera.up = persCamera.up; // important!

                const arrowPos = new THREE.Vector3(0, 0, 0);
                arrowScene.add(new THREE.ArrowHelper(
                    new THREE.Vector3(1, 0, 0), arrowPos, 60, 0x7F2020, 20, 10));
                arrowScene.add(new THREE.ArrowHelper(
                    new THREE.Vector3(0, 1, 0), arrowPos, 60, 0x207F20, 20, 10));
                arrowScene.add(new THREE.ArrowHelper(
                    new THREE.Vector3(0, 0, 1), arrowPos, 60, 0x20207F, 20, 10));
                arrowScene.add(new THREE.Mesh(
                    new THREE.BoxGeometry(40, 40, 40),
                    new THREE.MeshLambertMaterial({ color: 0xaaaaaa })
                ));
                arrowScene.add(new THREE.HemisphereLight(0xC7E8FF, 0xFFE3B3, 1.2));

                // Controls
                const persControls = new OrbitControls(persCamera, renderer.domElement);
                persControls.target = camCenter; // rotate around center of parts
                // persControls.enablePan = false;
                // persControls.enableDamping = true;
                persControls.update();
                const orthControls = new OrbitControls(orthCamera, renderer.domElement);
                orthControls.target = camCenter; // rotate around center of parts
                // orthControls.enablePan = false;
                // orthControls.enableDamping = true;
                orthControls.update();

                function render() {
                    renderRequested = false;
                    persControls.update();
                    if (cameraType == 'Perspective') {
                        arrowCamera.position.copy(persCamera.position);
                        arrowCamera.position.sub(persControls.target);
                    }
                    orthControls.update();
                    if (cameraType == 'Orthographic') {
                        arrowCamera.position.copy(orthCamera.position);
                        arrowCamera.position.sub(orthControls.target);
                    }
                    arrowCamera.lookAt(arrowScene.position);
                    arrowCamera.position.setLength(200);

                    if (cameraType == 'Perspective') {
                        renderer.render(scene, persCamera);
                    }
                    if (cameraType == 'Orthographic') {
                        renderer.render(scene, orthCamera);
                    }
                    arrowRenderer.render(arrowScene, arrowCamera);
                };

                function requestRender() {
                    if (!renderRequested) {
                        renderRequested = true;
                        requestAnimationFrame(render);
                    }
                }

                persControls.addEventListener('change', requestRender);
                orthControls.addEventListener('change', requestRender);
                renderer.domElement.addEventListener('mousemove', onMouseMove);
                renderer.domElement.addEventListener('dblclick', onMouseDblClick);
                window.addEventListener('resize', onMainCanvasResize, false);

                onMainCanvasResize();
                requestRender();

                function onMainCanvasResize() {
                    const pixelRatio = window.devicePixelRatio;
                    const width = canvas.clientWidth * pixelRatio | 0;
                    const height = canvas.clientHeight * pixelRatio | 0;
                    const needResize = canvas.width !== width || canvas.height !== height;
                    const aspect = canvas.clientWidth / canvas.clientHeight;
                    if (needResize) {
                        renderer.setSize(width, height, false);

                        // See https://stackoverflow.com/questions/39373113/three-js-resize-window-not-scaling-properly
                        const change = originalAspect / aspect;
                        const newSize = viewSize * change;
                        orthCamera.left = -aspect * newSize / 2;
                        orthCamera.right = aspect * newSize  / 2;
                        orthCamera.top = newSize / 2;
                        orthCamera.bottom = -newSize / 2;
                        orthCamera.updateProjectionMatrix();

                        persCamera.aspect = canvas.clientWidth / canvas.clientHeight;
                        persCamera.updateProjectionMatrix();
                    }

                    for (const obj of objects) {
                        obj.wirematerial.resolution.set(width, height);
                    }
                    requestRender();
                }

                // Use mouse double click to toggle the gui for the selected object
                function onMouseDblClick(e){
                    let c = false;
                    if (cameraType == 'Orthographic') {
                        c = orthCamera;
                    }
                    if (cameraType == 'Perspective') {
                        c = persCamera;
                    }
                    if (!c) {
                        return;
                    }

                    const raycaster = new THREE.Raycaster();
                    raycaster.setFromCamera(new THREE.Vector2(
                        (e.clientX / canvas.clientWidth) * 2 - 1,
                        -(e.clientY / canvas.clientHeight) * 2 + 1),
                                            c);
                    const intersects = raycaster.intersectObjects(raycasterObj);

                    for (const i of intersects) {
                        const m = i.object;
                        if (!m.material.visible){continue};
                        for (const obj of objects) {
                            for (const face_uuid of obj.faces) {
                                if (face_uuid == m.uuid) {
                                    obj.gui_link.show = false
                                    obj.wirematerial.visible = false
                                    for (const face of obj.faces) {
                                        scene.getObjectByName(face).material.visible = false
                                    }
                                    requestRender();
                                    return
                                }
                            }
                        }
                    }
                }


                function onMouseMove(e)  {
                    let c = false;
                    if (cameraType == 'Orthographic') {
                        c = orthCamera;
                    }
                    if (cameraType == 'Perspective') {
                        c = persCamera;
                    }
                    if (!c) {
                        return;
                    }

                    const raycaster = new THREE.Raycaster();
                    raycaster.setFromCamera(new THREE.Vector2(
                        (e.clientX / canvas.clientWidth) * 2 - 1,
                        -(e.clientY / canvas.clientHeight) * 2 + 1),
                                            c);
                    const intersects = raycaster.intersectObjects(raycasterObj);

                    let chosen = '';
                    for (const i of intersects) {
                        const m = i.object.material;
                        if ((m.opacity > 0) & m.visible) {
                            if (m.emissive.getHex() == 0x000000) {
                                m.emissive.setHex( 0x777777 );
                                m.needsUpdate = true;
                                requestRender();
                            }
                            chosen = i.object.name;
                            break;
                        }
                    }
                    for (const r of raycasterObj) {
                        if (r.name == chosen) {
                            continue;
                        }
                        if (r.material.emissive.getHex() != 0x000000) {
                            r.material.emissive.setHex(0x000000);
                            r.material.needsUpdate = true;
                            requestRender();
                        }
                    }
                }
            </script>
        </body>
    </html>
    """)


def export(
    exportList, filename: str, colors: dict[str, str] | None = None, camera: str | None = None
):
    """Exports objects to an html file"""

    global disableCompression, base, baseFloat

    data = {"camera": {}, "file": {}, "objects": []}

    populate_camera(data["camera"], camera)

    # Take the objects out of groups
    objectslist = Draft.get_group_contents(exportList, walls=True, addgroups=False)
    # objectslist = Arch.pruneIncluded(objectslist)

    for obj in objectslist:
        # Pull all obj data before we dig down the links
        label = obj.Label
        color, opacity = get_view_properties(obj, label, colors)

        validObject = False
        if obj.isDerivedFrom("Mesh::Feature"):
            mesh = obj.Mesh
            validObject = True
        if obj.isDerivedFrom("Part::Feature"):
            objShape = obj.Shape
            validObject = True
        if obj.isDerivedFrom("App::Link"):
            linkPlacement = obj.LinkPlacement
            while True:  # drill down to get to the actual obj
                if obj.isDerivedFrom("App::Link"):
                    if obj.ViewObject.OverrideMaterial:
                        color = Draft.getrgb(
                            obj.ViewObject.ShapeMaterial.DiffuseColor, testbw=False
                        )
                    obj = obj.LinkedObject
                    if hasattr(obj, "__len__"):
                        FreeCAD.Console.PrintMessage(f"{label}: Sub-Links are Unsupported.\n")
                        break
                elif obj.isDerivedFrom("Part::Feature"):
                    objShape = obj.Shape.copy(False)
                    objShape.Placement = linkPlacement
                    validObject = True
                    break
                elif obj.isDerivedFrom("Mesh::Feature"):
                    mesh = obj.Mesh.copy()
                    mesh.Placement = linkPlacement
                    validObject = True
                    break

        if not validObject:
            continue

        objdata = {
            "name": label,
            "color": color,
            "opacity": opacity,
            "verts": "",
            "facets": "",
            "wires": [],
            "faceColors": [],
            "facesToFacets": [],
            "floats": [],
        }

        if obj.isDerivedFrom("Part::Feature"):
            deviation = 0.5
            if FreeCADGui and hasattr(obj.ViewObject, "Deviation"):
                deviation = obj.ViewObject.Deviation

                # obj.ViewObject.DiffuseColor is length=1 when all faces are the same color, length=len(faces) for when they're not
                if len(obj.ViewObject.DiffuseColor) == len(objShape.Faces):
                    for fc in obj.ViewObject.DiffuseColor:
                        objdata["faceColors"].append(Draft.getrgb(fc, testbw=False))

            # get verts and facets for ENTIRE object
            shapeData = objShape.tessellate(deviation)
            mesh = Mesh.Mesh(shapeData)

            if len(objShape.Faces) > 1:
                # Map each Facet created by tessellate() to a Face so that it can be colored correctly using faceColors
                # This is done by matching the results of a tessellate() on EACH FACE to the overall tessellate stored in shapeData
                # if there is any error in matching these two then we display the whole object as one face and forgo the face colors
                for f in objShape.Faces:
                    faceData = f.tessellate(deviation)
                    found = True
                    # face verts. List of type Vector()
                    for fv in range(len(faceData[0])):
                        found = False
                        for sv in range(len(shapeData[0])):  # shape verts
                            # do not use isEqual() here
                            if faceData[0][fv] == shapeData[0][sv]:
                                # replace with the index of shapeData[0]
                                faceData[0][fv] = sv
                                found = True
                                break
                        if not found:
                            break
                    if not found:
                        FreeCAD.Console.PrintMessage("Facet to Face Mismatch.\n")
                        objdata["facesToFacets"] = []
                        break

                    # map each of the face facets to the shape facets and make a list of shape facet indices that belong to this face
                    facetList = []
                    for ff in faceData[1]:  # face facets
                        found = False
                        for sf in range(len(shapeData[1])):  # shape facets
                            if (
                                faceData[0][ff[0]] in shapeData[1][sf]
                                and faceData[0][ff[1]] in shapeData[1][sf]
                                and faceData[0][ff[2]] in shapeData[1][sf]
                            ):
                                facetList.append(sf)
                                found = True
                                break
                        if not found:
                            break
                    if not found:
                        FreeCAD.Console.PrintMessage("Facet List Mismatch.\n")
                        objdata["facesToFacets"] = []
                        break

                    if not disableCompression:
                        facetList = baseEncode(facetList)

                    objdata["facesToFacets"].append(facetList)

            wires = []  # Add wires
            for f in objShape.Faces:
                for w in f.Wires:
                    wo = Part.Wire(Part.__sortEdges__(w.Edges))
                    # use strings to avoid 0.00001 written as 1e-05
                    wire = []
                    for v in wo.discretize(QuasiDeflection=0.005):
                        wire.extend([f"{v.x:.5f}", f"{v.y:.5f}", f"{v.z:.5f}"])
                    wires.append(wire)

            if not disableCompression:
                wires, objdata["floats"] = compress_wires(wires, objdata["floats"])
            objdata["wires"] = wires

        vIndex = {}
        verts = []
        for p in mesh.Points:
            vIndex[p.Index] = p.Index
            verts.extend([f"{p.Vector.x:.5f}", f"{p.Vector.y:.5f}", f"{p.Vector.z:.5f}"])

        facets = [vIndex[i] for f in mesh.Facets for i in f.PointIndices]

        if not disableCompression:
            verts, objdata["floats"] = compress_verts(verts, objdata["floats"])
            objdata["floats"] = compress_floats(objdata["floats"])
            facets = baseEncode(facets)
            verts = baseEncode(verts)

        objdata["facets"] = facets
        objdata["verts"] = verts

        data["objects"].append(objdata)

    html = getHTMLTemplate()
    html = html.replace("$pagetitle", FreeCAD.ActiveDocument.Label)
    version = FreeCAD.Version()
    html = html.replace("$version", f"{version[0]}.{version[1]}.{version[2]}")

    # Remove data compression in JS
    data["compressed"] = not disableCompression
    data["base"] = base
    data["baseFloat"] = baseFloat

    html = html.replace("$data", json.dumps(data, separators=(",", ":")))  # Shape Data
    html = html.replace("$threejs_version", threejs_version)

    with pyopen(filename, "w", encoding="utf-8") as outfile:
        outfile.write(html)
    FreeCAD.Console.PrintMessage(translate("Arch", "Successfully written") + f" {filename}\n")


def get_view_properties(obj, label: str, colors: dict[str, str] | None) -> tuple[str, float]:
    """Get the color and opacity of the object"""
    color = "#cccccc"
    opacity = 1.0
    if FreeCADGui and hasattr(obj.ViewObject, "ShapeColor"):
        color = Draft.getrgb(obj.ViewObject.ShapeColor, testbw=False)
        opacity = int((100 - obj.ViewObject.Transparency) / 5) / 20  # 0>>1 with step of 0.05
    elif colors:
        if label in colors:
            color = Draft.getrgb(colors[label], testbw=False)
    return color, opacity


class CameraDict(TypedDict):
    """Dictionary for camera contents"""

    type: NotRequired[str]
    focalDistance: NotRequired[str]
    position_x: NotRequired[str]
    position_y: NotRequired[str]
    position_z: NotRequired[str]


def populate_camera(data: CameraDict, camera: str | None):
    if not FreeCADGui and not camera:
        camera = OfflineRenderingUtils.getCamera(FreeCAD.ActiveDocument.FileName)

    if camera:
        # REF: src/Mod/BIM/OfflineRenderingUtils.py
        camnode = OfflineRenderingUtils.getCoinCamera(camera)
        cameraPosition = camnode.position.getValue().getValue()
        data["type"] = "Orthographic"
        if "PerspectiveCamera" in camera:
            data["type"] = "Perspective"
        data["focalDistance"] = camnode.focalDistance.getValue()
        data["position_x"] = cameraPosition[0]
        data["position_y"] = cameraPosition[1]
        data["position_z"] = cameraPosition[2]
    else:
        v = FreeCADGui.ActiveDocument.ActiveView
        data["type"] = v.getCameraType()
        data["focalDistance"] = v.getCameraNode().focalDistance.getValue()
        data["position_x"] = v.viewPosition().Base.x
        data["position_y"] = v.viewPosition().Base.y
        data["position_z"] = v.viewPosition().Base.z


def compress_floats(floats: list[str]) -> str:
    """Compress floats to base 90

    Use ratio of 7x base13 to 4x base90 because 13^7 ~ 90^4
    """
    fullstr = json.dumps(floats, separators=(",", ":"))
    fullstr = fullstr.replace("[", "").replace("]", "").replace('"', "")
    floatStr = ""
    baseFloatCt = len(baseFloat)
    baseCt = len(base)
    for fs in range(0, len(fullstr), 7):  # chunks of 7 chars, skip the first one
        str7 = fullstr[fs : (fs + 7)]
        quotient = 0
        for s in range(len(str7)):
            quotient += baseFloat.find(str7[s]) * pow(baseFloatCt, (6 - s))
        for v in range(4):
            floatStr += base[quotient % baseCt]
            quotient = int(quotient / baseCt)
    return floatStr


def compress_wires(wires: list[list[str]], floats: list[str]) -> tuple[list[list[str]], list[str]]:
    """
    Create floats list to compress wires being written into the JS
    """
    lengths = []
    for w in wires:
        lengths.append(len(w))
        floats.extend(w)

    float_arr, all_wires = np.unique(floats, return_inverse=True)
    wire_arrays = np.array_split(all_wires, np.cumsum(lengths[:-1]))
    return [baseEncode(w.tolist()) for w in wire_arrays], float_arr.tolist()


def compress_verts(verts: list[str], floats: list[str]) -> tuple[list[int], list[str]]:
    """
    Create floats list to compress verts and wires being written into the JS
    """
    floats_v, ind, verts_v = np.unique(verts, return_index=True, return_inverse=True)

    # Reorder as np.unique orders the resulting array (needed for facet matching)
    floats_v = floats_v[ind.argsort()]
    reindex = dict(zip(ind.argsort(), np.arange(ind.size)))
    verts_v = np.vectorize(lambda entry: reindex[entry])(verts_v)

    # Get repeated indexes already existing from previous steps
    v_in_w = np.nonzero(np.isin(floats_v, floats))[0]
    w_in_v = np.nonzero(np.isin(floats, floats_v))[0]
    v_in_w2 = np.where(~np.isin(floats_v, floats))

    # Order values the same
    v_in_w = v_in_w[floats_v[v_in_w].argsort()]
    w_in_v = w_in_v[np.array(floats)[w_in_v].argsort()]

    # Replace repeated indexes that exist in floats
    new_index = len(floats)
    verts_v += new_index
    for vw, wv in zip(v_in_w + new_index, w_in_v):
        verts_v[verts_v == vw] = wv

    # Remove indexes of repeated entries in floats_v
    for vw in (v_in_w + new_index)[v_in_w.argsort()][::-1]:
        verts_v[verts_v > vw] -= 1

    return verts_v.tolist(), np.concatenate([floats, floats_v[v_in_w2]]).tolist()


def baseEncode(arr: list[int]) -> str:
    """Compresses an array of ints into a base90 string"""

    global base
    if len(arr) == 0:
        return ""

    longest = 0
    output = []
    baseCt = len(base)
    for v in range(len(arr)):
        buffer = ""
        quotient = arr[v]
        while True:
            buffer += base[quotient % baseCt]
            quotient = int(quotient / baseCt)
            if quotient == 0:
                break
        output.append(buffer)
        if len(buffer) > longest:
            longest = len(buffer)
    output = [("{:>" + str(longest) + "}").format(x) for x in output]  # pad each element
    return str(longest) + ("").join(output)
