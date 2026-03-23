#ifndef COIN_SOVECTORIZEACTIONP_H
#define COIN_SOVECTORIZEACTIONP_H

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

#include <Inventor/actions/SoCallbackAction.h>
#include <Inventor/nodes/SoShape.h>
#include <Inventor/lists/SbList.h>
#include <Inventor/SbBSPTree.h>
#include <Inventor/SoPrimitiveVertex.h>
#include <Inventor/SbColor4f.h>
#include <Inventor/SbPlane.h>
#include <Inventor/annex/HardCopy/SoVectorizeAction.h>
#include <Inventor/annex/HardCopy/SoVectorOutput.h>
#include <Inventor/elements/SoDrawStyleElement.h>
#include <Inventor/SbVec2s.h>
#include <Inventor/SbImage.h>
#include "VectorizeItems.h"

class SbClip;
class SoPointDetail;

class SoVectorizeActionP {
public:

  SoVectorizeActionP(SoVectorizeAction * p);
  ~SoVectorizeActionP();

public:
  SbBSPTree bsp;
  SbList <SoVectorizeItem*> itemlist;
  SbList <SoVectorizeItem*> annotationlist;
  SoVectorOutput * output;

  int annotationidx;

  struct Page {
    SbVec2f startpos;
    SbVec2f size;
  } page;

  struct Viewport {
    SbVec2f startpos;
    SbVec2f size;
  } viewport;

  SoVectorizeAction::Orientation orientation;

  struct Background {
    SbBool on;
    SbColor color;
  } background;

  float nominalwidth;
  float pixelimagesize;
  SoVectorizeAction::PointStyle pointstyle;

  SbBool testInside(SoState * state,
                    const SbVec3f & p0, 
                    const SbVec3f & p1,
                    const SbVec3f & p2) const;


  void addTriangle(SoVectorizeTriangle * tri);
  void addLine(SoVectorizeLine * line);
  void addPoint(SoVectorizePoint * point);
  void addText(SoVectorizeText * text);
  void addImage(SoVectorizeImage * image);
  
  void outputItems(void);
  void reset(void);

private:
  
  typedef struct {
    SbVec3f point;
    SbVec3f normal;
    uint32_t diffuse;
  } vertexdata;

  SbList <vertexdata*> vertexdatalist;
  int curr_vertexdata_index;
  vertexdata * alloc_vertexdata(void);
  vertexdata * create_vertexdata(const SoPrimitiveVertex * pv, SoState * state);
  vertexdata * create_vertexdata(const SoPointDetail * pd, SoState * state);
  void add_line(vertexdata * vd0, vertexdata * vd1, SoState * state);
  void add_point(vertexdata * vd, SoState * state);
  
  SbBool clip_line(vertexdata * v0, vertexdata * v1, const SbPlane & plane);

  struct ShapeMaterial {
    SbColor ambient_light_model;
    SbColor ambient;
    SbColor emissive;
    SbColor specular;
    float shininess;
  } shapematerial;

  struct Environment {
    float ambientintensity;
    SbColor ambientcolor;
    SbVec3f attenuation;
    int32_t fogtype;
    SbColor fogcolor;
    float fogvisibility;
    float fogstart;
  } environment;

  SbColor4f shade_vertex(SoState * state,
                         const SbVec3f & v,
                         const SbColor4f & vcolor,
                         const SbVec3f & normal);

  SoVectorizeAction * publ;
  SbMatrix shapeprojmatrix;
  SbMatrix shapetoworldmatrix;
  SbMatrix shapetovrc;
  SbPlane cameraplane;
  SbBool docull;
  SbBool twoside;
  SbBool ccw;
  SbBool phong;
  int faceidx;
  int lineidx;
  SoDrawStyleElement::Style drawstyle;
  SbClip * clipper;
  SbBool completelyinside;
  SbPlane vvplane[6];
  SbList <SbPlane> clipplanes;
  int prevfaceindex;
  float linewidth;
  uint16_t linepattern;
  float pointsize;

  static SoCallbackAction::Response pre_shape_cb(void * userdata,
                                                 SoCallbackAction * action,
                                                 const SoNode * node);
  static SoCallbackAction::Response post_shape_cb(void * userdata,
                                                  SoCallbackAction * action,
                                                  const SoNode * node);
  
  static SoCallbackAction::Response pre_text2_cb(void * userdata,
                                                 SoCallbackAction * action,
                                                 const SoNode * node);
  static SoCallbackAction::Response pre_image_cb(void * userdata,
                                                 SoCallbackAction * action,
                                                 const SoNode * node);
  static SoCallbackAction::Response pre_anno_cb(void * userdata,
                                                SoCallbackAction * action,
                                                const SoNode * node);
  static SoCallbackAction::Response post_anno_cb(void * userdata,
                                                 SoCallbackAction * action,
                                                 const SoNode * node);


  static void triangle_cb(void * userdata,
                          SoCallbackAction * action,
                          const SoPrimitiveVertex * v1,
                          const SoPrimitiveVertex * v2,
                          const SoPrimitiveVertex * v3);
  
  static void line_segment_cb(void * userdata, SoCallbackAction * action,
                              const SoPrimitiveVertex * v1,
                              const SoPrimitiveVertex * v2);

  static void point_cb(void * userdata, SoCallbackAction * action,
                       const SoPrimitiveVertex * v);

  static void calc_new_vertexdata(vertexdata * v,
                                  const SbVec3f & pos,
                                  const vertexdata * v0,
                                  const vertexdata * v1);
  
  static void * clip_cb(const SbVec3f & v0, void * vdata0, 
                        const SbVec3f & v1, void * vdata1,
                        const SbVec3f & newvertex,
                        void * userdata);
  static SoCallbackAction::Response camera_cb(void * data, 
                                              SoCallbackAction * action, 
                                              const SoNode * node);

};

// some convenience functions
inline SbVec2f
to_mm(const SbVec2f & val, const SoVectorizeAction::DimensionUnit unit)
{
  switch (unit) {
  default:
  case SoVectorizeAction::MM:
    return val;
  case SoVectorizeAction::INCH:
    return val * 25.4f;
  case SoVectorizeAction::METER:
    return val * 1000.0f;
  }
}

inline SbVec2f
from_mm(const SbVec2f & val, const SoVectorizeAction::DimensionUnit unit)
{
  switch (unit) {
  default:
  case SoVectorizeAction::MM:
    return val;
  case SoVectorizeAction::INCH:
    return val / 25.4f;
  case SoVectorizeAction::METER:
    return val / 1000.0f;
  }
}

inline float
to_mm(const float val, const SoVectorizeAction::DimensionUnit unit)
{
  switch (unit) {
  default:
  case SoVectorizeAction::MM:
    return val;
  case SoVectorizeAction::INCH:
    return val * 25.4f;
  case SoVectorizeAction::METER:
    return val * 1000.0f;
  }
}


inline float
from_mm(const float val, const SoVectorizeAction::DimensionUnit unit)
{
  switch (unit) {
  default:
  case SoVectorizeAction::MM:
    return val;
  case SoVectorizeAction::INCH:
    return val / 25.4f;
  case SoVectorizeAction::METER:
    return val / 1000.0f;
  }
}

#endif // COIN_SOVECTORIZEACTIONP_H
