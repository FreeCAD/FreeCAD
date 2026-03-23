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

//
// Private class for SoVectorizeAction. Does most of the work.
//

#include "VectorizeActionP.h"
#include "coindefs.h"
#include <Inventor/elements/SoViewingMatrixElement.h>
#include <Inventor/elements/SoProjectionMatrixElement.h>
#include <Inventor/elements/SoModelMatrixElement.h>
#include <Inventor/SoPrimitiveVertex.h>
#include <Inventor/elements/SoLazyElement.h>
#include <Inventor/elements/SoEnvironmentElement.h>
#include <Inventor/details/SoPointDetail.h>
#include <Inventor/details/SoFaceDetail.h>
#include <Inventor/nodes/SoSpotLight.h>
#include <Inventor/nodes/SoPointLight.h>
#include <Inventor/elements/SoLightElement.h>
#include <Inventor/elements/SoViewVolumeElement.h>
#include <Inventor/elements/SoLightModelElement.h>
#include <Inventor/elements/SoNormalElement.h>
#include <Inventor/elements/SoFontNameElement.h>
#include <Inventor/elements/SoFontSizeElement.h>
#include <Inventor/elements/SoViewportRegionElement.h>
#include <Inventor/elements/SoViewVolumeElement.h>
#include <Inventor/elements/SoCullElement.h>
#include <Inventor/elements/SoCoordinateElement.h>
#include <Inventor/nodes/SoDirectionalLight.h>
#include <Inventor/nodes/SoText2.h>
#include <Inventor/nodes/SoImage.h>
#include <Inventor/nodes/SoCamera.h>
#include <Inventor/nodes/SoAnnotation.h>
#include <Inventor/caches/SoBoundingBoxCache.h>
#include <Inventor/elements/SoClipPlaneElement.h>
#include <Inventor/SbClip.h>

#include <cstdlib>

#define PUBLIC(obj) ((obj)->publ)

//
// constructor.
//
SoVectorizeActionP::SoVectorizeActionP(SoVectorizeAction * p)
{
  PUBLIC(this) = p;

  this->clipper = new SbClip(clip_cb, this);
  this->shapeprojmatrix = SbMatrix::identity();
  this->output = NULL;

  PUBLIC(this)->addTriangleCallback(SoShape::getClassTypeId(),
                                    SoVectorizeActionP::triangle_cb,
                                    this);

  PUBLIC(this)->addLineSegmentCallback(SoShape::getClassTypeId(),
                                       SoVectorizeActionP::line_segment_cb,
                                       this);
  PUBLIC(this)->addPointCallback(SoShape::getClassTypeId(),
                                 SoVectorizeActionP::point_cb,
                                 this);

  PUBLIC(this)->addPreCallback(SoShape::getClassTypeId(),
                               SoVectorizeActionP::pre_shape_cb,
                               this);
  PUBLIC(this)->addPostCallback(SoShape::getClassTypeId(),
                                SoVectorizeActionP::post_shape_cb,
                                this);

  PUBLIC(this)->addPreCallback(SoText2::getClassTypeId(),
                               SoVectorizeActionP::pre_text2_cb, this);

  PUBLIC(this)->addPreCallback(SoImage::getClassTypeId(),
                               SoVectorizeActionP::pre_image_cb, this);

  PUBLIC(this)->addPreCallback(SoAnnotation::getClassTypeId(),
                               SoVectorizeActionP::pre_anno_cb, this);

  PUBLIC(this)->addPostCallback(SoAnnotation::getClassTypeId(),
                                SoVectorizeActionP::post_anno_cb, this);
  // for view-frustum culling
  PUBLIC(this)->addPostCallback(SoCamera::getClassTypeId(),
                                camera_cb, NULL);

  this->page.startpos = SbVec2f(10.0f, 10.0f);
  this->page.size = SbVec2f(190.0f, 277.0f);

  this->viewport.startpos = SbVec2f(10.0f, 10.0f);
  this->viewport.size = SbVec2f(190.0f, 277.0f);
  
  this->orientation = SoVectorizeAction::PORTRAIT;

  this->background.color = SbColor(1.0f, 1.0f, 1.0f);
  this->background.on = FALSE;

  this->nominalwidth = 0.35f;
  this->pixelimagesize = 0.35f;
  this->pointstyle = SoVectorizeAction::CIRCLE;
  this->annotationidx = 0;
}

//
// destructor.
//
SoVectorizeActionP::~SoVectorizeActionP()
{
  this->reset();
  delete this->clipper;
  delete this->output;
}

//
// reset this class. Making it ready for a new batch of geometry.
//
void
SoVectorizeActionP::reset(void)
{
  int i;
  for (i = 0; i < this->vertexdatalist.getLength(); i++) {
    delete this->vertexdatalist[i];
  }
  this->vertexdatalist.truncate(0);

  for (i = 0; i < this->itemlist.getLength(); i++) {
    delete this->itemlist[i];
  }
  this->itemlist.truncate(0);

  for (i = 0; i < this->annotationlist.getLength(); i++) {
    delete this->annotationlist[i];
  }
  this->annotationlist.truncate(0);
  this->bsp.clear();
}

//
// clip and add (if inside clipping planes) a point.
//
void 
SoVectorizeActionP::add_point(vertexdata * vd, SoState * state)
{
  SbBool dophong = this->phong;
  // if there are no normals on the state we fall back to BASE_COLOR
  // lighting model
  if (dophong) dophong = SoNormalElement::getInstance(state)->getNum() > 0;
  
  if (!this->completelyinside || this->clipplanes.getLength()) {
    int i;
    if (!this->completelyinside) {
      for (i = 0; i < 6; i++) {
        if (!this->vvplane[i].isInHalfSpace(vd->point)) return;
      }
    }
    for (i = 0; i < this->clipplanes.getLength(); i++) {
      if (!this->clipplanes[i].isInHalfSpace(vd->point)) return;
    }
  }
  
  SbVec3f v;
  this->shapeprojmatrix.multVecMatrix(vd->point, v);
  v[2] = 0.0f;

  SbVec3f wv;
  SoVectorizePoint * point = new SoVectorizePoint;

  SbColor4f c;
  c.setPackedValue(vd->diffuse);
  this->shapetoworldmatrix.multVecMatrix(vd->point, wv);
  point->vidx = this->bsp.addPoint(v);
  if (dophong) {
    point->col = this->shade_vertex(state, vd->point,
                                    c,
                                    vd->normal).getPackedValue();
  }
  else {
    point->col = c.getPackedValue();
  }
  point->depth = this->cameraplane.getDistance(wv);
  this->addPoint(point);
}

//
// clip and add (if inside clipping planes)  a line 
//
void 
SoVectorizeActionP::add_line(vertexdata * vd0, vertexdata * vd1, SoState * state)
{
  int i;

  vertexdata * vd[2];
  vd[0] = vd0;
  vd[1] = vd1;
  SbVec3f v[2];

  SbBool dophong = this->phong;
  // if there are no normals on the state we fall back to BASE_COLOR
  // lighting model
  if (dophong) dophong = SoNormalElement::getInstance(state)->getNum() > 0;
  
  if (!this->completelyinside || this->clipplanes.getLength()) {
    if (!this->completelyinside) {
      for (i = 0; i < 6; i++) {
        if (!this->clip_line(vd[0], vd[1], this->vvplane[i])) return;
      }
    }
    for (i = 0; i < this->clipplanes.getLength(); i++) {
      if (!this->clip_line(vd[0], vd[1], this->clipplanes[i])) return;
    }
  }

  for (i = 0; i < 2; i++) {
    this->shapeprojmatrix.multVecMatrix(vd[i]->point, v[i]);
    v[i][2] = 0.0f;
  }

  SbVec3f wv[2];
  SoVectorizeLine * line = new SoVectorizeLine;

  float accdist = 0.0f;
  SbColor4f c;

  for (i = 0; i < 2; i++) {
    c.setPackedValue(vd[i]->diffuse);
    this->shapetoworldmatrix.multVecMatrix(vd[i]->point, wv[i]);
    line->vidx[i] = this->bsp.addPoint(v[i]);
    if (dophong) {
      line->col[i] = this->shade_vertex(state, vd[i]->point,
                                         c,
                                         vd[i]->normal).getPackedValue();
    }
    else {
      line->col[i] = c.getPackedValue();
    }
    accdist += this->cameraplane.getDistance(wv[i]);
  }
  line->depth = accdist / 2.0f;
  this->addLine(line);

}

//
// SoCallbackAction line segment callback
//
void
SoVectorizeActionP::line_segment_cb(void * userdata, SoCallbackAction * action,
                                    const SoPrimitiveVertex * v1,
                                    const SoPrimitiveVertex * v2)
{
  SoVectorizeActionP * thisp = (SoVectorizeActionP*) userdata;
  // needed for vertexdata memory handling
  thisp->curr_vertexdata_index = 0;

  if (thisp->drawstyle == SoDrawStyleElement::POINTS) {
    point_cb(userdata, action, v1);
    point_cb(userdata, action, v2);
    return;
  }

  SoState * state = action->getState();
  thisp->add_line(thisp->create_vertexdata(v1, state),
                  thisp->create_vertexdata(v2, state),
                  state);
}

//
// SoCallbackAction point callback.
//
void
SoVectorizeActionP::point_cb(void * userdata, SoCallbackAction * action,
                             const SoPrimitiveVertex * v)
{
  SoVectorizeActionP * thisp = (SoVectorizeActionP*) userdata;
  // needed for vertexdata memory handling
  thisp->curr_vertexdata_index = 0;

  SoState * state = action->getState();
  thisp->add_point(thisp->create_vertexdata(v, state), state);
}

//
// SoCallbackAction triangle callback.
//
void
SoVectorizeActionP::triangle_cb(void * userdata,
                                SoCallbackAction * action,
                                const SoPrimitiveVertex * v1,
                                const SoPrimitiveVertex * v2,
                                const SoPrimitiveVertex * v3)
{
  SoVectorizeActionP * thisp = (SoVectorizeActionP*) userdata;
  // needed for vertexdata memory handling
  thisp->curr_vertexdata_index = 0;

  int i;

  SoState * state = action->getState();

  // need to do some extra work when in line mode, since we don't want
  // to tessellate a polygon into triangles, but draw the polygon as
  // one line loop.
  if (thisp->drawstyle == SoDrawStyleElement::LINES) {
    const SoDetail * detail = v1->getDetail();
    // it's not required to have a detail instance per vertex, so
    // check if we actually have one before testing the type
    if (detail && (detail->getTypeId() == SoFaceDetail::getClassTypeId())) {
      
      const SoFaceDetail * face = (const SoFaceDetail*) detail;
      int idx = face->getFaceIndex();
      if (idx != thisp->prevfaceindex) { // a new face has arrived
        thisp->prevfaceindex = idx;
        int numv = face->getNumPoints();
        if (numv) {
          vertexdata * v0 = thisp->create_vertexdata(face->getPoint(0), state);
          vertexdata * prev = v0;
          for (i = 1; i < numv; i++) {
            vertexdata * v = thisp->create_vertexdata(face->getPoint(i), state);
            thisp->add_line(prev, v, state);
            prev = v;
          }
          thisp->add_line(prev, v0, state);
        }
      }
    }
    else {
      // fall back to just sending the three triangle edges
      line_segment_cb(userdata, action, v1, v2);
      line_segment_cb(userdata, action, v2, v3);
      line_segment_cb(userdata, action, v3, v1);
      thisp->prevfaceindex = -1;
    }
    return;
  }
  if (thisp->drawstyle == SoDrawStyleElement::POINTS) {
    point_cb(userdata, action, v1);
    point_cb(userdata, action, v2);
    point_cb(userdata, action, v3);
    return;
  }

  // FIXME: use growable arrays. This assumes a maximum of 8 clipping
  // planes (which is usually the maximum number in OpenGL...)
  vertexdata * vd[9+8];
  SbVec3f v[9+8];
  SbVec3f wv[9+8];
  vd[0] = thisp->create_vertexdata(v1, state);
  vd[1] = thisp->create_vertexdata(v2, state);
  vd[2] = thisp->create_vertexdata(v3, state);
  int n = 3;

  if (!thisp->completelyinside || thisp->clipplanes.getLength()) {
    thisp->clipper->reset();
    thisp->clipper->addVertex(vd[0]->point, vd[0]);
    thisp->clipper->addVertex(vd[1]->point, vd[1]);
    thisp->clipper->addVertex(vd[2]->point, vd[2]);
    
    if (!thisp->completelyinside) {
      for (i = 0; i < 6; i++) {
        thisp->clipper->clip(thisp->vvplane[i]);
      }
    }
    for (i = 0; i < thisp->clipplanes.getLength(); i++) {
      thisp->clipper->clip(thisp->clipplanes[i]);
    }
    n = thisp->clipper->getNumVertices();
    if (n < 3) return;
    for (i = 0; i < n; i++) {
      vd[i] =(vertexdata*) thisp->clipper->getVertexData(i);
    }
  }

  SbColor4f c;
  for (i = 0; i < n; i++) {
    c.setPackedValue(vd[i]->diffuse);
    thisp->shapetoworldmatrix.multVecMatrix(vd[i]->point, wv[i]);
    thisp->shapeprojmatrix.multVecMatrix(vd[i]->point, v[i]);
    v[i][2] = 0.0f;

    if (thisp->phong) {
      vd[i]->diffuse = thisp->shade_vertex(state, vd[i]->point,
                                           c,
                                           vd[i]->normal).getPackedValue();
    }
    else {
      vd[i]->diffuse = c.getPackedValue(); 
    }
  }

  // clipping might create a convex polygon, so tessellate it using
  // the triangle fan technique.
  for (i = 0; i < n-2; i++) {
    if (thisp->docull) {
      SbVec3f d0, d1;
      d0 = v[i+1] - v[0];
      d1 = v[i+2] - v[0];
      float z = d0[0] * d1[1] - d0[1] * d1[0];
      if ((z < 0.0f && thisp->ccw) || (z > 0.0f && !thisp->ccw)) {
        continue; // try next triangle
      }
    }
    SoVectorizeTriangle * tri = new SoVectorizeTriangle;
    float accdist = 0.0f;
    tri->vidx[0] = thisp->bsp.addPoint(v[0]);
    tri->col[0] = vd[0]->diffuse;
    accdist += thisp->cameraplane.getDistance(wv[0]);
    
    for (int j = 1; j < 3; j++) {
      tri->vidx[j] = thisp->bsp.addPoint(v[i+j]);
      tri->col[j] = vd[i+j]->diffuse;
      accdist += thisp->cameraplane.getDistance(wv[i+j]);
    }
    tri->depth = accdist / 3.0f;
    thisp->addTriangle(tri);
  }
}

//
// callback for the SoImage node.
//
SoCallbackAction::Response
SoVectorizeActionP::pre_image_cb(void * userdata,
                                 SoCallbackAction * action,
                                 const SoNode * node)
{
  SoVectorizeActionP * thisp = (SoVectorizeActionP*) userdata;
  if (thisp->drawstyle == SoDrawStyleElement::INVISIBLE) return SoCallbackAction::PRUNE;

  SoState * state = action->getState();
 
  SbVec3f nilpoint(0.0f, 0.0f, 0.0f);
  const SbMatrix & mat = SoModelMatrixElement::get(state);
  mat.multVecMatrix(nilpoint, nilpoint);
  const SbViewVolume & vv = SoViewVolumeElement::get(state);
  // this function will also modify the z-value of nilpoint
  // according to the view matrix
  vv.projectToScreen(nilpoint, nilpoint);

  // remove images that are in front of or behind clipping planes
  if (nilpoint[2] < 0.0f || nilpoint[2] > 1.0f) 
    return SoCallbackAction::CONTINUE;;

  const SoImage * image = (const SoImage*) node;
  SoVectorizeImage * item = new SoVectorizeImage;

  SbVec2s pixsize;
  int nc;

  item->image.data = image->image.getValue(pixsize, nc);
  item->image.size = pixsize;
  item->image.nc = nc;

  float xpos = nilpoint[0];
  float ypos = nilpoint[1];

  if (image->width.getValue() > 0) pixsize[0] = (short) image->width.getValue();
  if (image->height.getValue() > 0) pixsize[1] = (short) image->height.getValue();
  
  float xsize = PUBLIC(thisp)->pixelsToUnits((int) pixsize[0]);
  float ysize = PUBLIC(thisp)->pixelsToUnits((int) pixsize[1]);
  
  switch (image->vertAlignment.getValue()) {
  default:
    assert(0 && "unknown vertAlgnment");
  case SoImage::BOTTOM:
    break;
  case SoImage::HALF:
    ypos -= ysize * 0.5f;
    break;
  case SoImage::TOP:
    ypos -= ysize;
    break;
  }
  switch (image->horAlignment.getValue()) {
  default: 
    assert(0 && "unknown horAlgnment");
  case SoImage::LEFT:
    break;
  case SoImage::CENTER: 
    xpos -= xsize * 0.5f;
    break;
  case SoImage::RIGHT:
    xpos -= xsize;
    break;
  }

  // calculate z-distance for correct sorting
  SbVec3f wv;
  thisp->shapetoworldmatrix.multVecMatrix(SbVec3f(0.0f, 0.0f, 0.0f), wv);
  float zdist = thisp->cameraplane.getDistance(wv);

  item->pos = SbVec2f(xpos, ypos);  
  item->size = SbVec2f(xsize, ysize);  
  item->depth = zdist;
  thisp->addImage(item);

  return SoCallbackAction::CONTINUE;
}

//
// callback for the SoText2 node.
//
SoCallbackAction::Response
SoVectorizeActionP::pre_text2_cb(void * userdata,
                                 SoCallbackAction * action,
                                 const SoNode * node)
{
  SoVectorizeActionP * thisp = (SoVectorizeActionP*) userdata;
  if (thisp->drawstyle == SoDrawStyleElement::INVISIBLE) return SoCallbackAction::PRUNE;
 
  const SoText2 * text2 = (const SoText2*) node;
  SoState * state = action->getState();

  SbVec3f nilpoint(0.0f, 0.0f, 0.0f);
  const SbMatrix & mat = SoModelMatrixElement::get(state);
  mat.multVecMatrix(nilpoint, nilpoint);
  const SbViewVolume & vv = SoViewVolumeElement::get(state);
  // this function will also modify the z-value of nilpoint
  // according to the view matrix
  vv.projectToScreen(nilpoint, nilpoint);

  // remove texts that are in front of or behind clipping planes
  if (nilpoint[2] < 0.0f || nilpoint[2] > 1.0f) 
    return SoCallbackAction::CONTINUE;;

  const SbViewportRegion & vp = SoViewportRegionElement::get(state);
  SbVec2s vpsize = vp.getViewportSizePixels();

  SbName fontname = SoFontNameElement::get(state);
  float fontsize = SoFontSizeElement::get(state); // in pixels

  fontsize *= PUBLIC(thisp)->getPixelImageSize(); // convert to mm
  fontsize /= PUBLIC(thisp)->getPageSize()[1]; // normalized

  float yspacing = text2->spacing.getValue() * fontsize;

  float xpos = nilpoint[0];
  float ypos = nilpoint[1];

  SoVectorizeText::Justification j;
  switch (text2->justification.getValue()) {
  default:
  case SoText2::LEFT:
    j = SoVectorizeText::LEFT;
    break;
  case SoText2::CENTER:
    j = SoVectorizeText::CENTER;
    break;
  case SoText2::RIGHT:
    j = SoVectorizeText::RIGHT;
    break;
  }

  // calculate z-distance for correct sorting
  SbVec3f wv;
  thisp->shapetoworldmatrix.multVecMatrix(SbVec3f(0.0f, 0.0f, 0.0f), wv);
  float zdist = thisp->cameraplane.getDistance(wv);

  for (int i = 0; i < text2->string.getNum(); i++) {

    SbString str = text2->string[i];
    if (str.getLength()) {
      SoVectorizeText * item = new SoVectorizeText;
      item->string = str;
      item->fontname = fontname;
      item->fontsize = fontsize;
      item->pos = SbVec2f(xpos, ypos);
      float t = SoLazyElement::getTransparency(state, 0);
      item->col = SoLazyElement::getDiffuse(state, 0).getPackedValue(t);
      item->justification = j;
      item->depth = zdist;
      thisp->addText(item);
    }
    ypos -= yspacing;
  }

  return SoCallbackAction::CONTINUE;
}

//
// Callback that is called before traversing an SoShape (or derivates)
// instance. Stores lots of state information to optimize primitive
// extraction.
//
SoCallbackAction::Response
SoVectorizeActionP::pre_shape_cb(void * userdata,
                                 SoCallbackAction * action,
                                 const SoNode * node)
{ 
  SoVectorizeActionP * thisp = (SoVectorizeActionP*) userdata;
  SoState * state = action->getState();
  state->push();

  thisp->drawstyle = SoDrawStyleElement::get(state);
  if (thisp->drawstyle == SoDrawStyleElement::INVISIBLE) {
    // don't pop. It will be done in post_shape_cb
    return SoCallbackAction::PRUNE;
  }

  thisp->shapetoworldmatrix = SoModelMatrixElement::get(state);
  thisp->shapetovrc = (SoModelMatrixElement::get(state) *
                       SoViewingMatrixElement::get(state));

  thisp->shapeprojmatrix = (SoModelMatrixElement::get(state) *
                            SoViewingMatrixElement::get(state) *
                            SoProjectionMatrixElement::get(state));

  // convert to a more convenient coordinate system [0,0,0]-[1,1,1]
  SbMatrix m;
  m.setTranslate(SbVec3f(1.0f, 1.0f, 1.0f));
  thisp->shapeprojmatrix.multRight(m);
  m.setScale(0.5f);
  thisp->shapeprojmatrix.multRight(m);

  thisp->shapematerial.emissive = SoLazyElement::getEmissive(state);
  thisp->shapematerial.specular = SoLazyElement::getSpecular(state);
  thisp->shapematerial.ambient = SoLazyElement::getAmbient(state);
  thisp->shapematerial.shininess = SoLazyElement::getShininess(state);

  thisp->cameraplane = SoViewVolumeElement::get(state).getPlane(0.0f);

  SoEnvironmentElement::get(state,
                            thisp->environment.ambientintensity,
                            thisp->environment.ambientcolor,
                            thisp->environment.attenuation,
                            thisp->environment.fogtype,
                            thisp->environment.fogcolor,
                            thisp->environment.fogvisibility,
                            thisp->environment.fogstart);

  SoShapeHintsElement::VertexOrdering vo;
  SoShapeHintsElement::ShapeType st;
  SoShapeHintsElement::FaceType ft;

  SoShapeHintsElement::get(state, vo, st, ft);

  thisp->ccw = TRUE;
  thisp->docull = FALSE;
  thisp->twoside = FALSE;

  if (vo == SoShapeHintsElement::CLOCKWISE) thisp->ccw = FALSE;

  if (vo != SoShapeHintsElement::UNKNOWN_ORDERING &&
      st == SoShapeHintsElement::SOLID) thisp->docull = TRUE;

  if (vo != SoShapeHintsElement::UNKNOWN_ORDERING &&
      st != SoShapeHintsElement::SOLID) thisp->twoside = TRUE;

  thisp->phong = SoLightModelElement::get(state) == SoLightModelElement::PHONG;

  SbBox3f bbox;

  SoShape * shape = (SoShape *) node;
  const SoBoundingBoxCache * bboxcache = shape->getBoundingBoxCache();
  if (bboxcache && bboxcache->isValid(state)) {
    bbox = bboxcache->getProjectedBox();
  }
  else {
    SbVec3f center;
    shape->computeBBox(action, bbox, center);
  }
  if (SoCullElement::cullBox(state, bbox, TRUE)) {
    return SoCallbackAction::PRUNE;
  }
  thisp->completelyinside = SoCullElement::completelyInside(state);
  if (!thisp->completelyinside) {
    const SbViewVolume & vv = SoViewVolumeElement::get(state);
    vv.getViewVolumePlanes(thisp->vvplane);
    SbMatrix toobj = SoModelMatrixElement::get(state).inverse();
    for (int i =0; i < 6; i++) {
      thisp->vvplane[i].transform(toobj);
    }
  }
  const SoClipPlaneElement * celem = SoClipPlaneElement::getInstance(state);
  thisp->clipplanes.truncate(0);
  
  if (celem->getNum()) {
    SbMatrix toobj = SoModelMatrixElement::get(state).inverse();
  
    for (int i = 0; i < celem->getNum(); i++) {
      SbPlane plane = celem->get(i, TRUE);
      plane.transform(toobj);
      thisp->clipplanes.append(plane);
    }
  }
  // used to detect when a new polygon arrives
  thisp->prevfaceindex = -1;

  // cache some attributes
  thisp->linepattern = PUBLIC(thisp)->getLinePattern();
  thisp->linewidth = PUBLIC(thisp)->getLineWidth();
  thisp->pointsize = PUBLIC(thisp)->getPointSize();

  return SoCallbackAction::CONTINUE;
}

//
// Callback which is called after a shape has been traversed.  We just
// use it to pop the state (we push in the pre callback).
//
SoCallbackAction::Response
SoVectorizeActionP::post_shape_cb(void * COIN_UNUSED_ARG(userdata),
                                  SoCallbackAction * action,
                                  const SoNode * COIN_UNUSED_ARG(node))
{
  SoState * state = action->getState();
  state->pop();
  return SoCallbackAction::CONTINUE;
}

//
// Callback which is called before traversing the SoAnnotation node.
// Needed so that annotated geometry is drawn on top of other
// geometry.
//
SoCallbackAction::Response
SoVectorizeActionP::pre_anno_cb(void * userdata,
                                SoCallbackAction * COIN_UNUSED_ARG(action),
                                const SoNode * COIN_UNUSED_ARG(node))
{
  SoVectorizeActionP * thisp = (SoVectorizeActionP*) userdata;
  thisp->annotationidx++;
  return SoCallbackAction::CONTINUE;
}

//
// SoAnnotation post callback.
//
SoCallbackAction::Response
SoVectorizeActionP::post_anno_cb(void * userdata,
                                 SoCallbackAction * COIN_UNUSED_ARG(action),
                                 const SoNode * COIN_UNUSED_ARG(node))
{
  SoVectorizeActionP * thisp = (SoVectorizeActionP*) userdata;
  thisp->annotationidx--;
  return SoCallbackAction::CONTINUE;
}

//
// Callback form qsort(). Will sort on depth.
//
static int
qsort_compare(const void * q0, const void * q1)
{
  SoVectorizeItem ** p0 = (SoVectorizeItem**) q0;
  SoVectorizeItem ** p1 = (SoVectorizeItem**) q1;

  SoVectorizeItem * i0 = *p0;
  SoVectorizeItem * i1 = *p1;

  if (i0->depth < i1->depth) return -1;
  return 1;
}

//
// Will sort and output items (painter's algorithm). FIXME: implement
// a better algorithm for hidden surface handling.
//

extern "C" {
typedef int qsort_cmp(const void *, const void *);
}

void
SoVectorizeActionP::outputItems(void)
{
  int i, n = this->itemlist.getLength();
  if (n) {
    SoVectorizeItem ** ptr = (SoVectorizeItem**) this->itemlist.getArrayPtr();
    qsort(ptr, n, sizeof(void*), (qsort_cmp *) qsort_compare);
    
    for (i = 0; i < n; i++) {
      PUBLIC(this)->printItem(ptr[i]);
    }
  }
  n = this->annotationlist.getLength();
  if (n) {
    SoVectorizeItem ** ptr = (SoVectorizeItem**) this->annotationlist.getArrayPtr();
    for (i = 0; i < n; i++) {
      PUBLIC(this)->printItem(ptr[i]);
    }
  }
}

//
// The OpenGL shading model
//
SbColor4f
SoVectorizeActionP::shade_vertex(SoState * state,
                                 const SbVec3f & vertex,
                                 const SbColor4f & vcolor,
                                 const SbVec3f & vnormal)
{
  float R,G,B,A;
  SbVec3f n, s, d;
  float dist, tmp, att, dot, dot_spot, dot_spec;
  float lR, lB, lG;

  SbBool dotwoside = this->twoside;

  if (vnormal == SbVec3f(0.0f, 0.0f, 0.0f)) return vcolor;
  this->shapetovrc.multDirMatrix(vnormal, n);
  
  // a null vector is ok here
  (void) n.normalize();

  const ShapeMaterial & m = this->shapematerial;
  const Environment & e = this->environment;

  R = m.emissive[0] + m.ambient[0] * e.ambientcolor[0] * e.ambientintensity;
  G = m.emissive[1] + m.ambient[1] * e.ambientcolor[1] * e.ambientintensity;
  B = m.emissive[2] + m.ambient[2] * e.ambientcolor[2] * e.ambientintensity;

  A = SbClamp(vcolor[3], 0.0f, 1.0f);

  const SoNodeList & lights = SoLightElement::getLights(state);

  // calculate contribution from all lights
  for (int i = 0; i < lights.getLength(); i++) {
    SoLight * l = (SoLight*) lights[i];
    if (l->on.getValue() == FALSE) continue;

    const SbMatrix & lighttoworld = SoLightElement::getMatrix(state, i);

    /* Light ambient is 0 in Inventor/Coin */
    if (l->isOfType(SoDirectionalLight::getClassTypeId())) {
      d = ((SoDirectionalLight*) l)->direction.getValue();
      d = -d;
      // move vector to the world coordinate system
      lighttoworld.multDirMatrix(d, d);
      (void) d.normalize(); // a null vector is ok here
      att = 1.0f;
    }
    else {
      SbVec3f vpos;
      this->shapetoworldmatrix.multVecMatrix(vertex, vpos);
      SbVec3f lpos;
      if (l->isOfType(SoPointLight::getClassTypeId())) {
        lpos = ((SoPointLight*)l)->location.getValue();
      }
      else {
        lpos = ((SoSpotLight*)l)->location.getValue();
      }
      lighttoworld.multVecMatrix(lpos, lpos);
      d = lpos - vpos;
      dist = d.length();
      (void) d.normalize(); // a null vector is ok
      att = 1.0f / (e.attenuation[0] + dist*(e.attenuation[1]+
                                             e.attenuation[2]));
    }
    dot = d.dot(n);

    if (dotwoside && dot < 0) {
      dot = -dot;
    }
    if (dot > 0.0f) {
      SbColor lcol = l->color.getValue() * l->intensity.getValue();

      /* diffuse light */
      lR = dot * lcol[0] * vcolor[0];
      lG = dot * lcol[1] * vcolor[1];
      lB = dot * lcol[2] * vcolor[2];

      if (l->isOfType(SoSpotLight::getClassTypeId())) {
        SoSpotLight * sl = (SoSpotLight*) l;
        SbVec3f spot_direction = sl->direction.getValue();
        lighttoworld.multDirMatrix(spot_direction, spot_direction);

        float spot_cutoff = (float)cos(sl->cutOffAngle.getValue());
        float spot_exp = sl->dropOffRate.getValue() * 128.0f;

        dot_spot = -d.dot(spot_direction);
        if (dotwoside && dot_spot < 0) dot_spot = -dot_spot;
        if (dot_spot < spot_cutoff) {
          // no contribution 
          continue;
        } else {
          // TODO: optimize
          if (spot_exp > 0.0f) {
            att = att * (float)pow(dot_spot, spot_exp);
          }
        }
      }

      // specular light. FIXME: consider supporting local viewer
      s[0] = d[0];
      s[1] = d[1];
      s[2] = d[2] + 1.0f;

      dot_spec = n.dot(s);

      if (dotwoside && dot_spec < 0) dot_spec = -dot_spec;
      if (dot_spec > 0) {
        tmp=(float)sqrt(s[0]*s[0]+s[1]*s[1]+s[2]*s[2]);
        if (tmp > 1E-3) {
          dot_spec=dot_spec / tmp;
        }
        dot_spec = (float) pow(dot_spec, m.shininess * 128.0f);
        lR += dot_spec * lcol[0] * m.specular[0];
        lG += dot_spec * lcol[1] * m.specular[1];
        lB += dot_spec * lcol[2] * m.specular[2];
      }
      R += att * lR;
      G += att * lG;
      B += att * lB;
    }
  }

  return SbColor4f(SbClamp(R, 0.0f, 1.0f),
                   SbClamp(G, 0.0f, 1.0f),
                   SbClamp(B, 0.0f, 1.0f));
}

//
// Adds a triangle item.
//
void 
SoVectorizeActionP::addTriangle(SoVectorizeTriangle * tri)
{
  if (this->annotationidx) {
    this->annotationlist.append(tri);
  }
  else {
    this->itemlist.append(tri);
  }
}

//
// Adds a line item.
//
void 
SoVectorizeActionP::addLine(SoVectorizeLine * line)
{
  if (this->annotationidx) {
    this->annotationlist.append(line);
  }
  else {
    this->itemlist.append(line);
  }
  line->width = this->linewidth;
  line->pattern = this->linepattern;
}

//
// Adds a point.
//
void 
SoVectorizeActionP::addPoint(SoVectorizePoint * point)
{
  if (this->annotationidx) {
    this->annotationlist.append(point);
  }
  else {
    this->itemlist.append(point);
  }
  point->size = this->pointsize;
}

//
// Adds text.
//
void 
SoVectorizeActionP::addText(SoVectorizeText * text)
{
  if (this->annotationidx) {
    this->annotationlist.append(text);
  }
  else {
    this->itemlist.append(text);
  }
}

//
// Adds image
//
void 
SoVectorizeActionP::addImage(SoVectorizeImage * image)
{
  if (this->annotationidx) {
    this->annotationlist.append(image);
  }
  else {
    this->itemlist.append(image);
  }
}


//
// Used by the clipper to calculate a new vertexdata based on two
// vertices and the clipping position.
//
void 
SoVectorizeActionP::calc_new_vertexdata(vertexdata * vd,
                                        const SbVec3f & newvertex,
                                        const vertexdata * vd0,
                                        const vertexdata * vd1)
{
  float len = (vd0->point - vd1->point).length();
  if (len == 0.0f) {
    *vd = *vd0;
  }
  else {
    SbColor4f c0, c1;
    c0.setPackedValue(vd0->diffuse);
    c1.setPackedValue(vd1->diffuse); 

    float t = (newvertex-vd0->point).length() / len;
    vd->normal = vd0->normal * (1.0f - t) + vd1->normal * t;
    (void) vd->normal.normalize(); // a null vector is ok
    
    SbColor4f res = c0 * (1.0f-t) + c1 * t;
    vd->diffuse = res.getPackedValue();
  }
  vd->point = newvertex;
}

//
// Callback from SbClip. Will calculate and create a new vertexdata.
//
void * 
SoVectorizeActionP::clip_cb(const SbVec3f & COIN_UNUSED_ARG(v0), void * vdata0, 
                            const SbVec3f & COIN_UNUSED_ARG(v1), void * vdata1,
                            const SbVec3f & newvertex,
                            void * userdata)
{
  SoVectorizeActionP * thisp = (SoVectorizeActionP*) userdata;
  vertexdata * vd0 = (vertexdata*) vdata0;
  vertexdata * vd1 = (vertexdata*) vdata1;

  vertexdata * vd = thisp->alloc_vertexdata();
  
  calc_new_vertexdata(vd, newvertex, vd0, vd1);

  return vd;
}

//
// SoCamera pre callback. Needed to set up culling.
//
SoCallbackAction::Response 
SoVectorizeActionP::camera_cb(void * COIN_UNUSED_ARG(data), SoCallbackAction * action, const SoNode * node)
{
  assert(node->isOfType(SoCamera::getClassTypeId()));
  SoState * state = action->getState();
  SoCullElement::setViewVolume(state, SoViewVolumeElement::get(state));
  return SoCallbackAction::CONTINUE;
}

//
// Will allocate a new vertexdata, or reuse an old one.
//
SoVectorizeActionP::vertexdata * 
SoVectorizeActionP::alloc_vertexdata(void)
{
  if (this->curr_vertexdata_index >= this->vertexdatalist.getLength()) {
    this->vertexdatalist.append(new vertexdata);
  }
  assert(this->curr_vertexdata_index < this->vertexdatalist.getLength());
  return this->vertexdatalist[this->curr_vertexdata_index++];
}

//
// Create a vertexdata based on an SoPrimitiveVertex and the current state.
//
SoVectorizeActionP::vertexdata * 
SoVectorizeActionP::create_vertexdata(const SoPrimitiveVertex * pv, SoState * state)
{
  vertexdata * v = this->alloc_vertexdata();
  v->point = pv->getPoint();
  v->normal = pv->getNormal();

  int numt = SoLazyElement::getInstance(state)->getNumTransparencies();
  int midx = pv->getMaterialIndex();
  SbColor c = SoLazyElement::getDiffuse(state, midx);
  float t = SoLazyElement::getTransparency(state, SbClamp(midx, 0, numt-1));
  v->diffuse = c.getPackedValue(t);
  return v;
}

//
// Create a vertexdata based on an SoPointDetail and the current state.
//
SoVectorizeActionP::vertexdata * 
SoVectorizeActionP::create_vertexdata(const SoPointDetail * pd, SoState * state)
{
  vertexdata * v = this->alloc_vertexdata();
  
  const SoCoordinateElement * celem = SoCoordinateElement::getInstance(state);
  const SoNormalElement * nelem = SoNormalElement::getInstance(state);

  int cidx = pd->getCoordinateIndex();
  int nidx = pd->getNormalIndex();
  int midx = pd->getMaterialIndex();

  if (midx == -1) midx = 0; // workaround for Coin-bug (fixed 2003-06-03)

  v->point = cidx < celem->getNum() ? celem->get3(cidx) : SbVec3f(0.0f, 0.0f, 0.0f);
  v->normal = nidx < nelem->getNum() ? nelem->get(nidx) : SbVec3f(0.0f, 0.0f, 1.0f);

  int numt = SoLazyElement::getInstance(state)->getNumTransparencies();
  SbColor c = SoLazyElement::getDiffuse(state, midx);
  float t = SoLazyElement::getTransparency(state, SbClamp(midx, 0, numt-1));
  v->diffuse = c.getPackedValue(t);

  return v;
}

//
// Clip a line. Might modify v0 or v1. Returns TRUE if inside plane.
//
SbBool 
SoVectorizeActionP::clip_line(vertexdata * v0, vertexdata * v1, const SbPlane & plane)
{
  float d0 = plane.getDistance(v0->point);
  float d1 = plane.getDistance(v1->point);
  
  if (d0 < 0.0f && d1 <= 0.0f) {
    return FALSE; // outside plane
  }
  if (d0 >= 0.0f && d1 >= 0.0f) {
    return TRUE; // both inside
  }
  const SbVec3f & planeN = plane.getNormal();
  SbVec3f dir = v1->point - v0->point;

  // since we got here, we know that v0 != v1
  (void) dir.normalize(); 
  float dot = dir.dot(planeN);
  SbVec3f newvertex = v0->point - dir * (d0/dot);
  
  if (d0 < 0.0f) {
    calc_new_vertexdata(v0, newvertex, v0, v1);
  }
  else { // d1 < 0.0f
    calc_new_vertexdata(v1, newvertex, v0, v1);
  }
  return TRUE;
}

#undef PUBLIC
