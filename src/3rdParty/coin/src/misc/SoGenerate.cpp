/**************************************************************************\
 * Copyright (c) Kongsberg Oil & Gas Technologies AS
 * All rights reserved.
 * 
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are
 * met:
 * 
 * Redistributions of source code must retain the above copyright notice,
 * this list of conditions and the following disclaimer.
 * 
 * Redistributions in binary form must reproduce the above copyright
 * notice, this list of conditions and the following disclaimer in the
 * documentation and/or other materials provided with the distribution.
 * 
 * Neither the name of the copyright holder nor the names of its
 * contributors may be used to endorse or promote products derived from
 * this software without specific prior written permission.
 * 
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 * "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 * LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
 * A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
 * HOLDER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
 * SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
 * LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
\**************************************************************************/

#include "misc/SoGenerate.h"

#include <Inventor/SoPrimitiveVertex.h>
#include <Inventor/details/SoCubeDetail.h>
#include <Inventor/details/SoCylinderDetail.h>
#include <Inventor/details/SoConeDetail.h>

#include <Inventor/nodes/SoCone.h>
#include <Inventor/nodes/SoCylinder.h>

// generate a 3d circle in the x-z plane
static void
sogenerate_generate_3d_circle(SbVec3f *coords, const int num, const float radius, const float y)
{
  // FIXME: this function completely duplicates
  // sogl_generate_3d_circle() in SoGL.cpp, AFAICS.  Should avoid
  // unnecessary duplication. 20010909 mortene.

  float delta = (float) ((2.0*M_PI)/(double)num);
  float angle = 0.0f;
  for (int i = 0; i < num; i++) {
    coords[i][0] = (float) (-sin(angle) * radius);
    coords[i][1] = y;
    coords[i][2] = (float) (-cos(angle) * radius);
    angle += delta;
  }
}

// generate a 2d circle
static void
sogenerate_generate_2d_circle(SbVec2f *coords, const int num, const float radius)
{
  // FIXME: this function completely duplicates
  // sogl_generate_3d_circle() in SoGL.cpp, AFAICS.  Should avoid
  // unnecessary duplication. 20010909 mortene.

  float delta = (float) (2.0*M_PI/(double)num);
  float angle = 0.0f;
  for (int i = 0; i < num; i++) {
    coords[i][0] = (float) (-sin(angle) * radius);
    coords[i][1] = (float) (-cos(angle) * radius);
    angle += delta;
  }
}

//
// the 12 triangles in the cube
//
static int sogenerate_cube_vindices[] =
{
  0, 1, 3, 2,
  5, 4, 6, 7,
  1, 5, 7, 3,
  4, 0, 2, 6,
  4, 5, 1, 0,
  2, 3, 7, 6
};

static const SbVec2f sogenerate_cube_texcoords[] =
{
  SbVec2f(1.0f, 1.0f),
  SbVec2f(0.0f, 1.0f),
  SbVec2f(0.0f, 0.0f),
  SbVec2f(1.0f, 0.0f)
};

//
// a cube needs 6 normals
//
static const SbVec3f sogenerate_cube_normals[] =
{
  SbVec3f(0.0f, 0.0f, 1.0f),
  SbVec3f(0.0f, 0.0f, -1.0f),
  SbVec3f(-1.0f, 0.0f, 0.0f),
  SbVec3f(1.0f, 0.0f, 0.0f),
  SbVec3f(0.0f, 1.0f, 0.0f),
  SbVec3f(0.0f, -1.0f, 0.0f)
};

static void
sogenerate_generate_cube_vertices(SbVec3f *varray,
                       const float w,
                       const float h,
                       const float d)
{
  for (int i = 0; i < 8; i++) {
    varray[i].setValue((i&1) ? -w : w,
                       (i&2) ? -h : h,
                       (i&4) ? -d : d);
  }
}

class so_generate_prim_private {
public:
  static void generate_cone(const float radius,
                            const float height,
                            const int numslices,
                            const unsigned int flags,
                            SoShape * const shape,
                            SoAction * const action) {
    int i;
    int slices = numslices;
    if (slices > 128) slices = 128;
    if (slices < 4) slices = 4;

    float h2 = height * 0.5f;

    // put coordinates on the stack
    SbVec3f coords[129];
    SbVec3f normals[130];
    SbVec2f texcoords[129];

    sogenerate_generate_3d_circle(coords, slices, radius, -h2);
    coords[slices] = coords[0];

    double a = atan(height/radius);
    sogenerate_generate_3d_circle(normals, slices, (float) sin(a), (float) cos(a));
    normals[slices] = normals[0];
    normals[slices+1] = normals[1];

    int matnr = 0;

    SoPrimitiveVertex vertex;
    SoConeDetail sideDetail;
    SoConeDetail bottomDetail;
    sideDetail.setPart(SoCone::SIDES);
    bottomDetail.setPart(SoCone::BOTTOM);

    // FIXME: the texture coordinate generation for cone sides is of
    // sub-par quality. The textures comes out looking "skewed" and
    // "compressed". 20010926 mortene.

    if (flags & SOGEN_GENERATE_SIDE) {
      vertex.setDetail(&sideDetail);
      vertex.setMaterialIndex(matnr);

      shape->beginShape(action, SoShape::TRIANGLES);
      i = 0;

      float t = 1.0;
      float delta = 1.0f / slices;

      while (i < slices) {
        vertex.setTextureCoords(SbVec2f(t - delta*0.5f, 1.0f));
        vertex.setNormal((normals[i] + normals[i+1])*0.5f);
        vertex.setPoint(SbVec3f(0.0f, h2, 0.0f));
        shape->shapeVertex(&vertex);

        vertex.setTextureCoords(SbVec2f(t, 0.0f));
        vertex.setNormal(normals[i]);
        vertex.setPoint(coords[i]);
        shape->shapeVertex(&vertex);

        vertex.setTextureCoords(SbVec2f(t-delta, 0.0f));
        vertex.setNormal(normals[i+1]);
        vertex.setPoint(coords[i+1]);
        shape->shapeVertex(&vertex);

        i++;
        t -= delta;
      }
      if (flags & SOGEN_MATERIAL_PER_PART) matnr++;
      shape->endShape();
    }

    if (flags & SOGEN_GENERATE_BOTTOM) {
      vertex.setDetail(&bottomDetail);
      vertex.setMaterialIndex(matnr);

      sogenerate_generate_2d_circle(texcoords, slices, 0.5f);
      texcoords[slices] = texcoords[0];

      shape->beginShape(action, SoShape::TRIANGLE_FAN);
      vertex.setNormal(SbVec3f(0.0f, -1.0f, 0.0f));
      for (i = slices-1; i >= 0; i--) {
        vertex.setTextureCoords(texcoords[i]+SbVec2f(0.5f, 0.5f));
        vertex.setPoint(coords[i]);
        shape->shapeVertex(&vertex);
      }
      shape->endShape();
    }
  }

  static void generate_cylinder(const float radius,
                                const float height,
                                const int numslices,
                                const unsigned int flags,
                                SoShape * const shape,
                                SoAction * const action) {
    int i;
    int slices = numslices;
    if (slices > 128) slices = 128;
    if (slices < 4) slices = 4;

    float h2 = height * 0.5f;

    SbVec3f coords[129];
    SbVec3f normals[130];
    SbVec2f texcoords[129];

    sogenerate_generate_3d_circle(coords, slices, radius, -h2);
    coords[slices] = coords[0];

    sogenerate_generate_3d_circle(normals, slices, 1.0f, 0.0f);
    normals[slices] = normals[0];
    normals[slices+1] = normals[1];

    int matnr = 0;

    SoPrimitiveVertex vertex;
    SoCylinderDetail sideDetail;
    SoCylinderDetail bottomDetail;
    SoCylinderDetail topDetail;
    sideDetail.setPart(SoCylinder::SIDES);
    bottomDetail.setPart(SoCylinder::BOTTOM);
    topDetail.setPart(SoCylinder::TOP);

    if (flags & SOGEN_GENERATE_SIDE) {
      shape->beginShape(action, SoShape::QUAD_STRIP);
      vertex.setDetail(&sideDetail);
      vertex.setMaterialIndex(matnr);
      i = 0;

      float t = 0.0;
      float inc = 1.0f / slices;

      while (i <= slices) {
        vertex.setTextureCoords(SbVec2f(t, 1.0f));
        vertex.setNormal(normals[i]);
        SbVec3f c = coords[i];
        vertex.setPoint(SbVec3f(c[0], h2, c[2]));
        shape->shapeVertex(&vertex);

        vertex.setTextureCoords(SbVec2f(t, 0.0f));
        vertex.setPoint(c);
        shape->shapeVertex(&vertex);
        i++;
        t += inc;
      }
      if (flags & SOGEN_MATERIAL_PER_PART) matnr++;
      shape->endShape();
    }

    if (flags & (SOGEN_GENERATE_BOTTOM | SOGEN_GENERATE_TOP)) {
      sogenerate_generate_2d_circle(texcoords, slices, 0.5f);
      texcoords[slices] = texcoords[0];
    }

    if (flags & SOGEN_GENERATE_TOP) {
      vertex.setMaterialIndex(matnr);
      vertex.setDetail(&topDetail);
      vertex.setNormal(SbVec3f(0.0f, 1.0f, 0.0f));
      shape->beginShape(action, SoShape::TRIANGLE_FAN);

      for (i = 0; i < slices; i++) {
        vertex.setTextureCoords(SbVec2f(texcoords[i][0] + 0.5f, 1.0f - texcoords[i][1] - 0.5f));
        const SbVec3f &c = coords[i];
        vertex.setPoint(SbVec3f(c[0], h2, c[2]));
        shape->shapeVertex(&vertex);
      }
      shape->endShape();
      if (flags & SOGEN_MATERIAL_PER_PART) matnr++;
    }
    if (flags & SOGEN_GENERATE_BOTTOM) {
      vertex.setMaterialIndex(matnr);
      vertex.setDetail(&bottomDetail);
      shape->beginShape(action, SoShape::TRIANGLE_FAN);
      vertex.setNormal(SbVec3f(0.0f, -1.0f, 0.0f));

      for (i = slices-1; i >= 0; i--) {
        vertex.setTextureCoords(texcoords[i] + SbVec2f(0.5f, 0.5f));
        vertex.setPoint(coords[i]);
        shape->shapeVertex(&vertex);
      }
      shape->endShape();
    }
  }

  static void generate_cube(const float width,
                            const float height,
                            const float depth,
                            const unsigned int flags,
                            SoShape * const shape,
                            SoAction * const action) {
    SbVec3f varray[8];
    sogenerate_generate_cube_vertices(varray,
                           width * 0.5f,
                           height * 0.5f,
                           depth * 0.5f);


    SoPrimitiveVertex vertex;
    SoCubeDetail cubeDetail;
    vertex.setDetail(&cubeDetail);
    vertex.setMaterialIndex(0);

    shape->beginShape(action, SoShape::QUADS);
    int *iptr = sogenerate_cube_vindices;
    const SbVec3f *nptr = sogenerate_cube_normals;
    const SbVec2f *tptr = sogenerate_cube_texcoords;

    for (int i = 0; i < 6; i++) { // 6 quads
      vertex.setNormal(nptr[i]);
      if (flags & SOGEN_MATERIAL_PER_PART) vertex.setMaterialIndex(i);
      for (int j = 0; j < 4; j++) {
        vertex.setTextureCoords(tptr[j]);
        vertex.setPoint(varray[*iptr++]);
        shape->shapeVertex(&vertex);
      }
    }
    shape->endShape();
  }

  static void generate_sphere(const float radius,
                              const int numstacks,
                              const int numslices,
                              SoShape * const shape,
                              SoAction * const action) {
    int stacks = numstacks;
    int slices = numslices;

    if (stacks < 3) stacks = 3;
    if (slices < 4) slices = 4;

    if (slices > 128) slices = 128;

    // used to cache last stack's data
    SbVec3f coords[129];
    SbVec3f normals[129];
    float S[129];

    int i, j;
    float rho;
    float drho;
    float theta;
    float dtheta;
    float tc, ts;
    SbVec3f tmp;

    drho = float(M_PI) / (float) (stacks-1);
    dtheta = 2.0f * float(M_PI) / (float) slices;

    float currs = 0.0f;
    float incs = 1.0f / (float)slices;
    rho = drho;
    theta = 0.0f;
    tc = (float) cos(rho);
    ts = - (float) sin(rho);
    tmp.setValue(0.0f,
                 tc,
                 ts);
    normals[0] = tmp;
    tmp *= radius;
    coords[0] = tmp;
    S[0] = currs;
    float dT = 1.0f / (float) (stacks-1);
    float T = 1.0f - dT;

    SoPrimitiveVertex vertex;
    shape->beginShape(action, SoShape::TRIANGLES);

    for (j = 1; j <= slices; j++) {
      vertex.setNormal(SbVec3f(0.0f, 1.0f, 0.0f));
      vertex.setTextureCoords(SbVec2f(currs + 0.5f * incs, 1.0f));
      vertex.setPoint(SbVec3f(0.0f, radius, 0.0f));
      shape->shapeVertex(&vertex);

      vertex.setNormal(normals[j-1]);
      vertex.setTextureCoords(SbVec2f(currs, T));
      vertex.setPoint(coords[j-1]);
      shape->shapeVertex(&vertex);

      currs += incs;
      theta += dtheta;
      S[j] = currs;
      tmp.setValue(float(sin(theta))*ts,
                   tc,
                   float(cos(theta))*ts);

      normals[j] = tmp;
      tmp *= radius;
      coords[j] = tmp;

      vertex.setNormal(normals[j]);
      vertex.setTextureCoords(SbVec2f(currs, T));
      vertex.setPoint(coords[j]);
      shape->shapeVertex(&vertex);

    }
    shape->endShape();

    rho += drho;

    for (i = 2; i < stacks-1; i++) {
      tc = (float)cos(rho);
      ts = - (float) sin(rho);
      shape->beginShape(action, SoShape::QUAD_STRIP);
      theta = 0.0f;
      for (j = 0; j <= slices; j++) {
        vertex.setTextureCoords(SbVec2f(S[j], T));
        vertex.setNormal(normals[j]);
        vertex.setPoint(coords[j]);
        shape->shapeVertex(&vertex);

        vertex.setTextureCoords(SbVec2f(S[j], T-dT));
        tmp.setValue(float(sin(theta))*ts,
                     tc,
                     float(cos(theta))*ts);
        normals[j] = tmp;
        vertex.setNormal(tmp);
        tmp *= radius;
        coords[j] = tmp;
        theta += dtheta;
        vertex.setPoint(tmp);
        shape->shapeVertex(&vertex);
      }
      shape->endShape();
      rho += drho;
      T -= dT;
    }

    shape->beginShape(action, SoShape::TRIANGLES);
    for (j = 0; j < slices; j++) {
      vertex.setTextureCoords(SbVec2f(S[j], T));
      vertex.setNormal(normals[j]);
      vertex.setPoint(coords[j]);
      shape->shapeVertex(&vertex);

      vertex.setTextureCoords(SbVec2f(S[j]+incs*0.5f, 0.0f));
      vertex.setNormal(SbVec3f(0.0f, -1.0f, 0.0f));
      vertex.setPoint(SbVec3f(0.0f, -radius, 0.0f));
      shape->shapeVertex(&vertex);

      vertex.setTextureCoords(SbVec2f(S[j+1], T));
      vertex.setNormal(normals[j+1]);
      vertex.setPoint(coords[j+1]);
      shape->shapeVertex(&vertex);
    }
    shape->endShape();
  }
};

void
sogen_generate_cone(const float bottomRadius,
                    const float height,
                    const int numslices,
                    const unsigned int flags,
                    SoShape * const shape,
                    SoAction * const action)
{
  so_generate_prim_private::generate_cone(bottomRadius,
                                          height,
                                          numslices,
                                          flags,
                                          shape,
                                          action);
}

void
sogen_generate_cylinder(const float radius,
                        const float height,
                        const int numslices,
                        const unsigned int flags,
                        SoShape * const shape,
                        SoAction * const action)
{
  so_generate_prim_private::generate_cylinder(radius,
                                              height,
                                              numslices,
                                              flags,
                                              shape,
                                              action);
}

void
sogen_generate_sphere(const float radius,
                      const int numstacks,
                      const int numslices,
                      SoShape * const shape,
                      SoAction * const action)
{
  so_generate_prim_private::generate_sphere(radius,
                                            numstacks,
                                            numslices,
                                            shape,
                                            action);
}

void
sogen_generate_cube(const float width,
                    const float height,
                    const float depth,
                    const unsigned int flags,
                    SoShape * const shape,
                    SoAction * const action)
{
  so_generate_prim_private::generate_cube(width,
                                          height,
                                          depth,
                                          flags,
                                          shape,
                                          action);
}
