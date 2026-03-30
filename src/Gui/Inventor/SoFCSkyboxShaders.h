/***************************************************************************
 *   Copyright (c) 2024 FreeCAD Project Association                        *
 *                                                                         *
 *   This file is part of the FreeCAD CAx development system.              *
 *                                                                         *
 *   This library is free software; you can redistribute it and/or         *
 *   modify it under the terms of the GNU Library General Public           *
 *   License as published by the Free Software Foundation; either          *
 *   version 2 of the License, or (at your option) any later version.      *
 *                                                                         *
 *   This library  is distributed in the hope that it will be useful,      *
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of        *
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the         *
 *   GNU Library General Public License for more details.                  *
 *                                                                         *
 *   You should have received a copy of the GNU Library General Public     *
 *   License along with this library; see the file COPYING.LIB. If not,    *
 *   write to the Free Software Foundation, Inc., 59 Temple Place,         *
 *   Suite 330, Boston, MA  02111-1307, USA                                *
 *                                                                         *
 ***************************************************************************/

#pragma once

// GLSL shaders for the HDRI skybox.
// Vertex shader passes the object-space vertex position as the cubemap
// sampling direction; fragment shader samples the cube map.

// The skybox is drawn as a fullscreen NDC quad (vertices at ±1 in x/y, z=0).
// The vertex shader passes the NDC xy through; the fragment shader
// reconstructs the world-space view ray analytically so the skybox fills
// the screen correctly for both perspective and orthographic projections
// with no near/far clipping artifacts.

static const char* skyboxVert = R"(
varying vec3 vWorldDir;
void main() {
    // Build the inverse view rotation (view->world) in the vertex shader
    // where gl_ModelViewMatrix is unambiguously the camera matrix.
    // Inverse of a pure rotation = its transpose.
    // Use explicit column extraction for GLSL 1.10 compatibility.
    vec3 c0 = gl_ModelViewMatrix[0].xyz;
    vec3 c1 = gl_ModelViewMatrix[1].xyz;
    vec3 c2 = gl_ModelViewMatrix[2].xyz;
    mat3 invViewRot = mat3(c0.x, c1.x, c2.x,
                           c0.y, c1.y, c2.y,
                           c0.z, c1.z, c2.z);

    // Reconstruct the view-space ray for this screen corner.
    // P[0][0] = cot(fovX/2) for perspective, 1/halfWidth for ortho.
    // For ortho we clamp to a 90-degree-equivalent spread so the sky
    // fills the screen without extreme fisheye distortion.
    float scaleX = gl_ProjectionMatrix[0][0];
    float scaleY = gl_ProjectionMatrix[1][1];
    bool  isOrtho = (gl_ProjectionMatrix[3][3] != 0.0);
    float invSX = isOrtho ? 1.0 : (1.0 / scaleX);
    float invSY = isOrtho ? (scaleX / (scaleY * scaleX)) : (1.0 / scaleY);
    // In ortho keep the aspect ratio but treat it like a 90-fov camera.
    if (isOrtho) {
        invSX = 1.0;
        invSY = scaleX / scaleY;
    }

    // Scale > 1 widens the sampled arc of the panorama, making the
    // environment features appear smaller and more distant.
    const float skyScale = 1.5;
    vec3 viewRay = normalize(vec3(gl_Vertex.x * invSX * skyScale,
                                  gl_Vertex.y * invSY * skyScale,
                                  -1.0));
    // FreeCAD world space is Z-up, but the cubemap was baked with Y as the
    // elevation axis (standard equirectangular convention).  Remap so that
    // world +Z (up) maps to cubemap +Y (sky face) and the front-view
    // direction (+Y world) maps to a horizon face rather than the sky.
    vec3 d = invViewRot * viewRay;
    vWorldDir = vec3(d.x, d.z, -d.y);
    gl_Position = vec4(gl_Vertex.xy, 0.9999, 1.0);
}
)";

static const char* skyboxFrag = R"(
uniform samplerCube cubemap;
varying vec3 vWorldDir;
void main() {
    gl_FragColor = textureCube(cubemap, vWorldDir);
}
)";
