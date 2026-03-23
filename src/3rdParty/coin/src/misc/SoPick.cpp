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

#include "misc/SoPick.h"

#include <cmath>

#include <Inventor/actions/SoRayPickAction.h>
#include <Inventor/SoPickedPoint.h>
#include <Inventor/details/SoConeDetail.h>
#include <Inventor/details/SoCylinderDetail.h>
#include <Inventor/details/SoCubeDetail.h>
#include <Inventor/SbLine.h>
#include <Inventor/nodes/SoCone.h>
#include <Inventor/nodes/SoCylinder.h>
#include <Inventor/SbPlane.h>
#include <Inventor/SbCylinder.h>
#include <Inventor/SbSphere.h>

//
// this was actually much easier than I first though since the Cone
// is aligned with the y-axis.
//
// A point on an SoCone can be expressed by:
//
// x^2 + z^2 = r^2, where r = ((h/2)-y)*br/h
//
// Substituting x, y and z with the parametric line equations, and we
// can find zero, one or two solutions for t. We have to check the y-value
// afterwards to see if it is between +/- (h/2)
//

static int
intersect_cone_line(const float br,
                    const float h,
                    const SbLine & line,
                    SbVec3f & enter,
                    SbVec3f & exit)
{
  float h2 = h * 0.5f;
  SbVec3f d = line.getDirection();
  SbVec3f p = line.getPosition();

  float tmp = (br * br)/(h * h);

  float a = d[0]*d[0] + d[2]*d[2] - d[1]*d[1]*tmp;
  float b = 2.0f*d[0]*p[0] + 2.0f*d[2]*p[2] + (2.0f*h2*d[1] - 2.0f*p[1]*d[1]) * tmp;
  float c = p[0]*p[0] + p[2]*p[2] + (2.0f*p[1]*h2 - h2*h2 - p[1]*p[1])*tmp;

  float root = b*b - 4.0f*a*c;

  if (root < 0) return 0;

  root = (float) sqrt(root);

  float t0 = (-b - root) / (2.0f*a);
  float t1 = (-b + root) / (2.0f*a);

  if (t1 < t0) SbSwap(t0, t1);

  enter = p + t0*d;
  exit = p + t1*d;

  int numisect = 0;
  if (fabs(enter[1]) <= h2) numisect++;
  if (fabs(exit[1]) <= h2 && t0 != t1) {
    numisect++;
    if (numisect == 1) enter = exit;
  }
  return numisect;
}

void
sopick_pick_cone(const float bottomRadius,
                 const float h,
                 const unsigned int flags,
                 SoShape * const shape,
                 SoRayPickAction * const action)
{
  action->setObjectSpace();
  const SbLine & line = action->getLine();

  int numisect = 0;
  SbVec3f isect[2];

  if (flags & SOPICK_SIDES) {
    numisect = intersect_cone_line(bottomRadius,
                                   h,
                                   line,
                                   isect[0],
                                   isect[1]);

    for (int i = 0; i < numisect; i++) {
      if (action->isBetweenPlanes(isect[i])) {
        SoPickedPoint * pp = action->addIntersection(isect[i]);
        if (pp) {
          // normalize the cone so that the apex is at (0,0,0)
          SbVec3f npoint(isect[i][0], isect[i][1] - h*0.5f, isect[i][2]);
          SbVec3f ptonaxis(0.0f, npoint[1], 0.0f);

          // calculate some vectors to help find the normal
          SbVec3f v0 = npoint-ptonaxis;
          SbVec3f v1 = v0.cross(SbVec3f(0.0f, -1.0f, 0.0f));
          (void) v1.normalize();
          SbVec3f n = npoint.cross(v1);
          (void) n.normalize();
          pp->setObjectNormal(n);
          pp->setObjectTextureCoords(SbVec4f((float) (atan2(npoint[0], npoint[2]) *
                                                      (1.0 / (2.0 * M_PI)) + 0.5),
                                             -npoint[1] / h, 0.0f, 1.0f));
          SoConeDetail * detail = new SoConeDetail;
          detail->setPart((int)SoCone::SIDES);
          pp->setDetail(detail, shape);
        }
      }
    }
  }

  if ((numisect < 2) && (flags & SOPICK_BOTTOM)) {
    SbPlane bottom(SbVec3f(0, 1, 0), -h * 0.5f);
    SbVec3f bpt;
    float r = bottomRadius;
    float r2 = r * r;
    if (bottom.intersect(line, bpt)) {
      if (((bpt[0] * bpt[0] + bpt[2] * bpt[2]) <= r2) &&
          (action->isBetweenPlanes(bpt))) {
        SoPickedPoint * pp = action->addIntersection(bpt);
        if (pp) {
          pp->setObjectNormal(SbVec3f(0.0f, -1.0f, 0.0f));
          pp->setObjectTextureCoords(SbVec4f(0.5f + bpt[0] / (2.0f * r),
                                             0.5f + bpt[2] / (2.0f * r),
                                             0.0f, 1.0f));

          SoConeDetail * detail = new SoConeDetail();
          detail->setPart((int)SoCone::BOTTOM);
          pp->setDetail(detail, shape);
        }
      }
    }
  }
}

//
// internal method used to set picked point attributes
// when picking on the side of the cylinder
//
static void
set_side_pp_data(SoPickedPoint * pp, const SbVec3f & isect,
                 const float halfh)
{
  // the normal vector for a cylinder side is the intersection point,
  // without the y-component, of course.
  SbVec3f normal(isect[0], 0.0f, isect[2]);
  (void) normal.normalize();
  pp->setObjectNormal(normal);

  // just reverse the way texture coordinates are generated to find
  // the picked point texture coordinate
  SbVec4f texcoord;
  texcoord.setValue((float) atan2(isect[0], isect[2]) *
                    (1.0f / (2.0f * (float) M_PI)) + 0.5f,
                    (isect[1] + halfh) / (2.0f * halfh),
                    0.0f, 1.0f);
  pp->setObjectTextureCoords(texcoord);
}


void
sopick_pick_cylinder(const float r,
                     const float height,
                     const unsigned int flags,
                     SoShape * const shape,
                     SoRayPickAction * const action)
{
  action->setObjectSpace();
  const SbLine & line = action->getLine();
  float halfh = height * 0.5f;

  // FIXME: should be possible to simplify cylinder test, since this
  // cylinder is aligned with the y-axis. 19991110 pederb.

  int numPicked = 0; // will never be > 2
  SbVec3f enter, exit;

  if (flags & SOPICK_SIDES) {
#if 0
    // The following line of code doesn't compile with GCC 2.95, as
    // reported by Petter Reinholdtsen (pere@hungry.com) on
    // coin-discuss.
    //
    // Update: it doesn't work with GCC 2.95.2 either, which is now
    // the current official release of GCC. And I can't find any
    // mention of a bug like this being fixed from the CVS ChangeLog,
    // neither in the gcc/egcs head branch nor the release-2.95
    // branch.  20000103 mortene.
    //
    // FIXME: should a) make sure this is known to the GCC
    // maintainers, b) have an autoconf check to test for this exact
    // bug. 19991230 mortene.
    SbCylinder cyl(SbLine(SbVec3f(0.0f, 0.0f, 0.0f), SbVec3f(0.0f, 1.0f, 0.0f)), r);
#else // GCC 2.95 work-around.
    SbVec3f v0(0.0f, 0.0f, 0.0f);
    SbVec3f v1(0.0f, 1.0f, 0.0f);
    SbLine l(v0, v1);
    SbCylinder cyl(l, r);
#endif // GCC 2.95 work-around.

    if (cyl.intersect(line, enter, exit)) {
      if ((fabs(enter[1]) <= halfh) && action->isBetweenPlanes(enter)) {
        SoPickedPoint * pp = action->addIntersection(enter);
        if (pp) {
          set_side_pp_data(pp, enter, halfh);
          SoCylinderDetail * detail = new SoCylinderDetail();
          detail->setPart((int)SoCylinder::SIDES);
          pp->setDetail(detail, shape);
          numPicked++;
        }
      }
      if ((fabs(exit[1]) <= halfh) && (enter != exit) && action->isBetweenPlanes(exit)) {
        SoPickedPoint * pp = action->addIntersection(exit);
        if (pp) {
          set_side_pp_data(pp, exit, halfh);
          SoCylinderDetail * detail = new SoCylinderDetail();
          detail->setPart((int)SoCylinder::SIDES);
          pp->setDetail(detail, shape);
          numPicked++;
        }
      }
    }
  }

  float r2 = r * r;

  SbBool matperpart = flags & SOPICK_MATERIAL_PER_PART;

  if ((numPicked < 2) && (flags & SOPICK_TOP)) {
    SbPlane top(SbVec3f(0.0f, 1.0f, 0.0f), halfh);
    if (top.intersect(line, enter)) {
      if (((enter[0] * enter[0] + enter[2] * enter[2]) <= r2) &&
          (action->isBetweenPlanes(enter))) {
        SoPickedPoint * pp = action->addIntersection(enter);
        if (pp) {
          if (matperpart) pp->setMaterialIndex(1);
          pp->setObjectNormal(SbVec3f(0.0f, 1.0f, 0.0f));
          pp->setObjectTextureCoords(SbVec4f(0.5f + enter[0] / (2.0f * r),
                                             0.5f - enter[2] / (2.0f * r),
                                             0.0f, 1.0f));
          SoCylinderDetail * detail = new SoCylinderDetail();
          detail->setPart((int)SoCylinder::TOP);
          pp->setDetail(detail, shape);
          numPicked++;
        }
      }
    }
  }

  if ((numPicked < 2) && (flags & SOPICK_BOTTOM)) {
    SbPlane bottom(SbVec3f(0, 1, 0), -halfh);
    if (bottom.intersect(line, enter)) {
      if (((enter[0] * enter[0] + enter[2] * enter[2]) <= r2) &&
          (action->isBetweenPlanes(enter))) {
        SoPickedPoint * pp = action->addIntersection(enter);
        if (pp) {
          if (matperpart) pp->setMaterialIndex(2);
          pp->setObjectNormal(SbVec3f(0.0f, -1.0f, 0.0f));
          pp->setObjectTextureCoords(SbVec4f(0.5f + enter[0] / (2.0f * r),
                                             0.5f + enter[2] / (2.0f * r),
                                             0.0f, 1.0f));
          SoCylinderDetail * detail = new SoCylinderDetail();
          detail->setPart((int)SoCylinder::BOTTOM);
          pp->setDetail(detail, shape);
        }
      }
    }
  }
}

// internal method used to add a sphere intersection to the ray pick
// action, and set the correct pp normal and texture coordinates
static void
try_add_intersection(SoRayPickAction * action, const SbVec3f & pt)
{
  if (action->isBetweenPlanes(pt)) {
    SoPickedPoint * pp = action->addIntersection(pt);
    if (pp) {
      SbVec3f normal = pt;
      (void) normal.normalize();
      pp->setObjectNormal(normal);
      SbVec4f tc((float) (atan2(pt[0], pt[2]) * (1.0 / (2.0*M_PI)) + 0.5),
                 (float) (atan2(pt[1], sqrt(pt[0]*pt[0] + pt[2]*pt[2])) * (1.0/M_PI) + 0.5),
                 0.0f, 1.0f);
      pp->setObjectTextureCoords(tc);
    }
  }
}

void
sopick_pick_sphere(const float radius,
                   SoRayPickAction * const action)
{
  action->setObjectSpace();
  const SbLine & line = action->getLine();
  SbSphere sphere(SbVec3f(0.0f, 0.0f, 0.0f), radius);
  SbVec3f enter, exit;
  if (sphere.intersect(line, enter, exit)) {
    try_add_intersection(action, enter);
    if (exit != enter) try_add_intersection(action, exit);
  }
}

void
sopick_pick_cube(const float width,
                 const float height,
                 const float depth,
                 const unsigned int flags,
                 SoShape * const shape,
                 SoRayPickAction * const action)
{
  static int translation[6] = {2, 3, 5, 4, 1, 0}; // translate into detail part-num
  static int textranslation[3][2] = {{2,1},{0,2},{0,1}}; // to get correct texcoords
  action->setObjectSpace();
  const SbLine & line = action->getLine();
  float size[3];
  size[0] = width * 0.5f;
  size[1] = height * 0.5f;
  size[2] = depth * 0.5f;

  int cnt = 0;
  // test intersection with all six planes
  for (int i = 0; i < 3; i++) {
    for (int j = -1; j <= 1; j += 2) {
      SbVec3f norm(0, 0, 0);
      norm[i] = (float)j;
      SbVec3f isect;

      SbPlane plane(norm, size[i]);
      if (plane.intersect(line, isect)) {
        int i1 = (i+1) % 3;
        int i2 = (i+2) % 3;

        if (isect[i1] >= -size[i1] && isect[i1] <= size[i1] &&
            isect[i2] >= -size[i2] && isect[i2] <= size[i2] &&
            action->isBetweenPlanes(isect)) {
          SoPickedPoint * pp = action->addIntersection(isect);
          if (pp) {
            SoCubeDetail * detail = new SoCubeDetail();
            detail->setPart(translation[cnt]);
            pp->setDetail(detail, shape);
            if (flags & SOPICK_MATERIAL_PER_PART)
              pp->setMaterialIndex(translation[cnt]);
            pp->setObjectNormal(norm);
            i1 = textranslation[i][0];
            i2 = textranslation[i][1];
            float s = isect[i1] + size[i1];
            float t = isect[i2] + size[i2];
            if (size[i1]) s /= (size[i1]*2.0f);
            if (size[i2]) t /= (size[i2]*2.0f);
            switch (i) {
            default: // just to avoid warnings
            case 0:
              if (j > 0) s = 1.0f - s;
              break;
            case 1:
              if (j > 0) t = 1.0f - t;
              break;
            case 2:
              if (j < 0) s = 1.0f - s;
              break;
            }
            pp->setObjectTextureCoords(SbVec4f(s, t, 0.0f, 1.0f));
          }
        }
      }
      cnt++;
    }
  }
}
