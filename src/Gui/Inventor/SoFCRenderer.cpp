/****************************************************************************
 *   Copyright (c) 2020 Zheng, Lei (realthunder) <realthunder.dev@gmail.com>*
 *                                                                          *
 *   This file is part of the FreeCAD CAx development system.               *
 *                                                                          *
 *   This library is free software; you can redistribute it and/or          *
 *   modify it under the terms of the GNU Library General Public            *
 *   License as published by the Free Software Foundation; either           *
 *   version 2 of the License, or (at your option) any later version.       *
 *                                                                          *
 *   This library  is distributed in the hope that it will be useful,       *
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of         *
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the          *
 *   GNU Library General Public License for more details.                   *
 *                                                                          *
 *   You should have received a copy of the GNU Library General Public      *
 *   License along with this library; see the file COPYING.LIB. If not,     *
 *   write to the Free Software Foundation, Inc., 59 Temple Place,          *
 *   Suite 330, Boston, MA  02111-1307, USA                                 *
 *                                                                          *
 ****************************************************************************/

#include "PreCompiled.h"

#include <algorithm>
#include <unordered_map>

#include <Inventor/actions/SoGLRenderAction.h>
#include <Inventor/elements/SoTextureEnabledElement.h>
#include <Inventor/elements/SoOverrideElement.h>
#include <Inventor/elements/SoLazyElement.h>
#include <Inventor/elements/SoLinePatternElement.h>
#include <Inventor/elements/SoLineWidthElement.h>
#include <Inventor/elements/SoPointSizeElement.h>
#include <Inventor/elements/SoDrawStyleElement.h>
#include <Inventor/elements/SoMaterialBindingElement.h>
#include <Inventor/elements/SoCacheElement.h>
#include <Inventor/elements/SoPolygonOffsetElement.h>
#include <Inventor/elements/SoViewVolumeElement.h>
#include <Inventor/elements/SoModelMatrixElement.h>
#include <Inventor/elements/SoTextureUnitElement.h>
#include <Inventor/elements/SoMultiTextureEnabledElement.h>
#include <Inventor/elements/SoMultiTextureImageElement.h>
#include <Inventor/elements/SoMultiTextureMatrixElement.h>
#include <Inventor/elements/SoShapeHintsElement.h>
#include <Inventor/elements/SoLightModelElement.h>
#include <Inventor/elements/SoDepthBufferElement.h>
#include <Inventor/annex/FXViz/elements/SoShadowStyleElement.h>
#include <Inventor/nodes/SoGroup.h>
#include <Inventor/nodes/SoMaterial.h>
#include <Inventor/nodes/SoDepthBuffer.h>
#include <Inventor/annex/FXViz/nodes/SoShadowDirectionalLight.h>
#include <Inventor/annex/FXViz/nodes/SoShadowSpotLight.h>
#include <Inventor/actions/SoCallbackAction.h>
#include <Inventor/SbPlane.h>
#include <Inventor/SbBox3f.h>

#include "SoFCRenderer.h"
#include "SoFCRenderCache.h"
#include "SoFCVertexCache.h"
#include "../ViewParams.h"

using namespace Gui;

typedef SoFCRenderCache::Material Material;
typedef SoFCRenderCache::VertexCacheEntry VertexCacheEntry;
typedef SoFCRenderCache::VertexCacheMap VertexCacheMap;
typedef SoFCRenderCache::CacheKey CacheKey;
typedef SoFCRenderCache::CacheKeyPtr CacheKeyPtr;
typedef Gui::CoinPtr<SoFCRenderCache> RenderCachePtr;
typedef Gui::CoinPtr<SoFCVertexCache> VertexCachePtr;

#define PRIVATE(obj) ((obj)->pimpl)

struct CacheKeyCompare {
  bool operator()(const CacheKeyPtr &a, const CacheKeyPtr &b) const {
    if (a == b) return false;
    if (!a) return true;
    if (!b) return false;
    if (a->size() < b->size()) return true;
    if (a->size() > b->size()) return false;
    return (*a) < (*b);
  }
};

typedef std::set<CacheKeyPtr, CacheKeyCompare> CacheKeySet;

struct DrawEntry {
  const Material * material;
  const VertexCacheEntry * ventry;
  SbBox3f bbox;
  int skip;

  DrawEntry(const Material * m, const VertexCacheEntry * v)
    :material(m), ventry(v), skip(0)
  {
    v->cache->getBoundingBox(v->identity ? nullptr : &v->matrix, this->bbox);
  }
};

struct DrawEntryIndex {
  std::size_t idx;
  float distance;
  DrawEntryIndex(std::size_t i)
    : idx(i)
  {}
};

enum RenderPass {
  RenderPassNormal            = 0,
  RenderPassLineSolid         = 1,
  RenderPassLinePattern       = 2,
  RenderPassLineMask          = 3,
  RenderPassHighlight         = 4,
};

class SoFCRendererP {
public:
  SoFCRendererP()
  {
    this->updateselection = false;
  }

  ~SoFCRendererP()
  {
  }

  bool applyMaterial(SoGLRenderAction * action,
                     const Material & next,
                     bool transp,
                     int pass = RenderPassNormal);

  void setupMatrix(SoState * state, const VertexCacheEntry * ventry);

  void updateSelection();

  static std::size_t pushDrawEntry(std::vector<DrawEntry> & draw_entries,
                                   const Material & material,
                                   const VertexCacheEntry & ventry);

  void renderOpaque(SoGLRenderAction * action,
                    std::vector<DrawEntry> & draw_entries,
                    std::vector<std::size_t> & indices,
                    int pass = RenderPassNormal);

  void renderTransparency(SoGLRenderAction * action,
                          std::vector<DrawEntry> & draw_entries,
                          std::vector<DrawEntryIndex> & indices,
                          bool sort=true);

  void applyKeys(const CacheKeySet & keys, int skip=1);
  void applyKey(const CacheKeyPtr & key, int skip=1);

  std::vector<DrawEntry> drawentries;
  std::vector<DrawEntry> slentries;
  std::vector<DrawEntry> hlentries; 

  std::vector<std::size_t> opaquevcache;
  std::vector<std::size_t> opaqueontop;
  std::vector<std::size_t> opaqueselections;
  std::vector<std::size_t> opaquehighlight;
  std::vector<std::size_t> linesontop; // has both lines and points
  std::vector<std::size_t> trianglesontop;

  SbPlane prevplane;
  std::vector<DrawEntryIndex> transpvcache;
  std::vector<DrawEntryIndex> transpontop;
  std::vector<DrawEntryIndex> transpselections;
  std::vector<DrawEntryIndex> transphighlight;

  std::map<int, const VertexCacheMap *> selections;
  std::map<int, const VertexCacheMap *> selectionsontop;
  std::vector<DrawEntryIndex> transpselectionsontop;
  std::vector<std::size_t> selstriangleontop;
  std::vector<std::size_t> selsontop; // include only non-explicitly selected lines and points
  std::vector<std::size_t> selslineontop; // include only explicitly selected lines
  std::vector<std::size_t> selspointontop; // include only explictly selected points
  bool updateselection;

  std::map<CacheKeyPtr, std::vector<std::size_t>, CacheKeyCompare> cachetable;

  VertexCacheMap highlightcaches;
  CacheKeySet highlightkeys;
  CacheKeySet selectionkeys;
  CacheKeyPtr selkey;

  std::vector<RenderCachePtr> scene;
  RenderCachePtr highlight;

  SbBox3f scenebbox;
  SbBox3f highlightbbox;
  SbBox3f selectionbbox;

  Material material;
  const Material * prevmaterial;
  bool recheckmaterial;
  int prevpass;

  SbMatrix matrix;
  bool identity;

  uint32_t highlightcolor;
  bool notexture;
  bool depthwriteonly;
};

SoFCRenderer::SoFCRenderer()
  : pimpl(new SoFCRendererP)
{
}

SoFCRenderer::~SoFCRenderer()
{
  delete pimpl;
}

static inline void
setGLColor(int name, uint32_t col)
{
  GLfloat c[4];
  c[0] = ((col >> 24)&0xff)/255.0f;
  c[1] = ((col >> 16)&0xff)/255.0f;
  c[2] = ((col >> 8)&0xff)/255.0f;
  c[3] = 1.0f;
  glMaterialfv(GL_FRONT_AND_BACK, name, c);
}

static inline void
setGLFeature(int name, int current, int next, int mask)
{
  if ((current & mask) && !(next & mask))
    glDisable(name);
  else if (!(current & mask) && (next & mask))
    glEnable(name);
}

static const SbMatrix matrixidentity(SbMatrix::identity());

bool
SoFCRendererP::applyMaterial(SoGLRenderAction * action,
                             const Material & next,
                             bool transp,
                             int pass)
{
  bool first = this->prevmaterial == nullptr;
  SoState * state = action->getState();

  // depth buffer write without color
  if (this->depthwriteonly) {
    // disable any texture
    if (this->material.textures.getNum()) {
      this->material.textures.clear();
      state->pop();
      state->push();
    }
    // disable lighting
    if (this->material.lightmodel != SoLazyElement::BASE_COLOR) {
      this->material.lightmodel = SoLazyElement::BASE_COLOR;
      glDisable(GL_LIGHTING);
    }
    // disable per vertex color
    this->material.pervertexcolor = false;
    // enable depth write
    if (!this->material.depthwrite) {
      this->material.depthwrite = true;
      glDepthMask(GL_TRUE);
    }
    // force GL_LESS depth function
    if (this->material.depthfunc != SoDepthBuffer::LESS) {
      this->material.depthfunc = SoDepthBuffer::LESS;
      glDepthFunc(GL_LESS);
    }
    // enable depth test
    if (!this->material.depthtest) {
      this->material.depthtest = true;
      glEnable(GL_DEPTH_TEST);
    }
    return true;
  }

  this->material.pervertexcolor = next.pervertexcolor;

  if (next.type == Material::Triangle) {
    bool texturechanged = first || this->material.textures != next.textures;
    bool lightchanged = first || this->material.lights != next.lights;

    if (!first && (texturechanged || lightchanged)) {
      state->pop();
      state->push();
    }

    if (!this->notexture && texturechanged) {
      if (next.textures.getNum()) {
        for (auto & texentry : next.textures.getData()) {
          auto t = this->material.textures.get(texentry.first);
          if (t && *t == texentry.second)
            continue;
          SoMultiTextureMatrixElement::set(state, NULL, texentry.first,
              texentry.second.identity ? matrixidentity : texentry.second.matrix);
          SoTextureUnitElement::set(state, NULL, texentry.first);
          texentry.second.texture->GLRender(action);
        }
      }
      this->material.textures = next.textures;
    }

    if (lightchanged) {
      if (next.lights.getNum()) {
        for(auto & info : next.lights.getData()) {
          if (!info.identity)
            SoModelMatrixElement::set(state, NULL, info.matrix);
          info.light->GLRender(action);
          if (!info.identity)
            SoModelMatrixElement::makeIdentity(state, NULL);
        }
      }
      this->material.lights = next.lights;
    }
  }

  bool depthtest = next.isOnTop() ? false : next.depthtest;
  bool depthwrite = transp ? false : next.depthwrite;
  int8_t depthfunc = next.depthfunc;
  uint32_t linepattern = next.linepattern;
  uint32_t col = next.diffuse;
  auto overrideflags = next.overrideflags;
  float linewidth = next.linewidth;
  float pointsize = next.pointsize;

  if ((pass & RenderPassLineMask) == RenderPassLinePattern) {
    if (pass == RenderPassLinePattern) {
      transp = true;
      uint32_t alpha = (uint32_t)(ViewParams::getTransparencyOnTop() * 255);
      if (alpha < (col & 0xff))
        col = (col & 0xffffff00) | alpha;
      overrideflags.set(Material::FLAG_TRANSPARENCY);
    }
    depthtest = false;
    uint32_t sellinepattern = ViewParams::getSelectionLinePattern();
    if (sellinepattern && ViewParams::getSelectionLinePatternScale() > 1)
      sellinepattern |= ViewParams::getSelectionLinePatternScale() << 16;

    if (sellinepattern && !next.hasLinePattern())
      linepattern  = sellinepattern;
  }
  else if ((pass & RenderPassLineMask) == RenderPassLineSolid) {
    depthtest = true;
    depthfunc = SoDepthBuffer::LEQUAL;
    depthwrite = false;

    float scale = ViewParams::getSelectionLineThicken();
    pointsize = std::max(pointsize, pointsize * scale);
    linewidth = std::max(linewidth, linewidth * scale);
  }

  if (pass & RenderPassHighlight) {
    float scale = ViewParams::getSelectionLineThicken();
    pointsize = std::max(std::max(pointsize, pointsize * scale), 3.0f);
    linewidth = std::max(std::max(linewidth, linewidth * scale), 2.0f);
  }

  if (first || this->material.depthtest != depthtest) {
    if (depthtest)
      glEnable(GL_DEPTH_TEST);
    else
      glDisable(GL_DEPTH_TEST);
    this->material.depthtest = depthtest;
  }

  if (first || this->material.depthwrite != depthwrite) {
    glDepthMask(depthwrite ? GL_TRUE : GL_FALSE);
    this->material.depthwrite = depthwrite;
  }

  if (first || this->material.depthfunc != depthfunc) {
    switch (depthfunc) {
    case SoDepthBuffer::NEVER:     glDepthFunc(GL_NEVER);     break;
    case SoDepthBuffer::ALWAYS:    glDepthFunc(GL_ALWAYS);    break;
    case SoDepthBuffer::LESS:      glDepthFunc(GL_LESS);      break;
    case SoDepthBuffer::LEQUAL:    glDepthFunc(GL_LEQUAL);    break;
    case SoDepthBuffer::EQUAL:     glDepthFunc(GL_EQUAL);     break;
    case SoDepthBuffer::GEQUAL:    glDepthFunc(GL_GEQUAL);    break;
    case SoDepthBuffer::GREATER:   glDepthFunc(GL_GREATER);   break;
    case SoDepthBuffer::NOTEQUAL:  glDepthFunc(GL_NOTEQUAL);  break;
    }
    this->material.depthfunc = depthfunc;
  }

  if (first || this->material.lightmodel != next.lightmodel) {
    if (next.lightmodel == SoLazyElement::PHONG)
      glEnable(GL_LIGHTING);
    else
      glDisable(GL_LIGHTING);
    this->material.lightmodel = next.lightmodel;
  }

  // Always set color because the current color may be changed by opengl draw call
  glColor4ub((unsigned char)((col>>24)&0xff),
              (unsigned char)((col>>16)&0xff),
              (unsigned char)((col>>8)&0xff),
              (unsigned char)(col&0xff));

  if (overrideflags != this->material.overrideflags
      || (overrideflags.test(Material::FLAG_TRANSPARENCY)
          && (col&0xff) != (this->material.diffuse&0xff)))
  {
    if (overrideflags.test(Material::FLAG_TRANSPARENCY)) {
      glBlendFunc(GL_CONSTANT_ALPHA, GL_ONE_MINUS_CONSTANT_ALPHA);
      glBlendColor(0.f, 0.f, 0.f,  (col & 0xff)/255.f);
    }
    else 
      glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
  }

  this->material.overrideflags = overrideflags;
  this->material.diffuse = col;

  if (next.type == Material::Line) {
    if (first || this->material.linewidth != linewidth) {
      glLineWidth(linewidth);
      this->material.linewidth = linewidth;
    }

    if (first || this->material.linepattern != linepattern) {
      if ((linepattern & 0xffff) == 0xffff)
        glDisable(GL_LINE_STIPPLE);
      else {
        glEnable(GL_LINE_STIPPLE);
        glLineStipple((GLint) (linepattern >> 16), (GLshort) (linepattern & 0xffff));
      }
      this->material.linepattern = linepattern;
    }
    if (!first)
      return true;
  }

  if (next.type == Material::Point) {
    if (first || this->material.pointsize != pointsize) {
      glPointSize(pointsize);
      this->material.pointsize = pointsize;
    }
    if (!first)
      return true;
  }

  if (first || this->material.ambient != next.ambient) {
    setGLColor(GL_AMBIENT, next.ambient);
    this->material.ambient = next.ambient;
  }

  if (first || this->material.emissive != next.emissive) {
    setGLColor(GL_EMISSION, next.emissive);
    this->material.emissive = next.emissive;
  }

  if (first || this->material.specular != next.specular) {
    setGLColor(GL_SPECULAR, next.specular);
    this->material.specular = next.specular;
  }

  if (first || this->material.shininess != next.shininess) {
    glMaterialf(GL_FRONT_AND_BACK, GL_SHININESS, next.shininess*128.0f);
    this->material.shininess = next.shininess;
  }

  if (first || this->material.vertexordering != next.vertexordering) {
    glFrontFace(next.vertexordering == SoLazyElement::CW ? GL_CW : GL_CCW);
    this->material.vertexordering = next.vertexordering;
  }

  int8_t twoside = next.twoside;
  if (transp)
    twoside = 1;
  if (first || this->material.twoside != twoside) {
    glLightModeli(GL_LIGHT_MODEL_TWO_SIDE, twoside ? GL_TRUE : GL_FALSE);
    this->material.twoside = twoside;
  }

  int8_t culling = next.culling;
  if (transp)
    culling = 0;
  if (first || this->material.culling != culling) {
    if (culling) glEnable(GL_CULL_FACE);
    else glDisable(GL_CULL_FACE);
    this->material.culling = culling;
  }

  if (first || this->material.drawstyle != next.drawstyle) {
    switch ((SoDrawStyleElement::Style)next.drawstyle) {
    case SoDrawStyleElement::LINES:
      glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
      break;
    case SoDrawStyleElement::POINTS:
      glPolygonMode(GL_FRONT_AND_BACK, GL_POINT);
      break;
    default:
      glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
    }
    this->material.drawstyle = next.drawstyle;
  }

  if (first || this->material.polygonoffsetstyle != next.polygonoffsetstyle) {
    setGLFeature(GL_POLYGON_OFFSET_FILL,
                 this->material.polygonoffsetstyle,
                 next.polygonoffsetstyle,
                 SoPolygonOffsetElement::FILLED);
    setGLFeature(GL_POLYGON_OFFSET_LINE,
                 this->material.polygonoffsetstyle,
                 next.polygonoffsetstyle,
                 SoPolygonOffsetElement::LINES);
    setGLFeature(GL_POLYGON_OFFSET_POINT,
                 this->material.polygonoffsetstyle,
                 next.polygonoffsetstyle,
                 SoPolygonOffsetElement::POINTS);
    this->material.polygonoffsetstyle = next.polygonoffsetstyle;
  }

  if (first || this->material.polygonoffsetfactor != next.polygonoffsetfactor
            || this->material.polygonoffsetunits != next.polygonoffsetunits) {
    glPolygonOffset(next.polygonoffsetfactor, next.polygonoffsetunits);
    this->material.polygonoffsetfactor = next.polygonoffsetfactor;
    this->material.polygonoffsetunits = next.polygonoffsetunits;
  }
  return true;
}

void
SoFCRenderer::clear()
{
  PRIVATE(this)->prevplane = SbPlane();
  PRIVATE(this)->opaquevcache.clear();
  PRIVATE(this)->transpvcache.clear();
  PRIVATE(this)->opaqueontop.clear();
  PRIVATE(this)->transpontop.clear();

  PRIVATE(this)->linesontop.clear();
  PRIVATE(this)->trianglesontop.clear();

  PRIVATE(this)->opaqueselections.clear();
  PRIVATE(this)->transpselections.clear();
  PRIVATE(this)->selections.clear();
  PRIVATE(this)->selectionsontop.clear();
  PRIVATE(this)->transpselectionsontop.clear();
  PRIVATE(this)->selstriangleontop.clear();
  PRIVATE(this)->selslineontop.clear();
  PRIVATE(this)->selspointontop.clear();
  PRIVATE(this)->selsontop.clear();
  PRIVATE(this)->selectionkeys.clear();

  PRIVATE(this)->highlightcaches.clear();
  PRIVATE(this)->opaquehighlight.clear();
  PRIVATE(this)->transphighlight.clear();
  PRIVATE(this)->highlightkeys.clear();

  PRIVATE(this)->cachetable.clear();
}

inline void
SoFCRendererP::applyKey(const CacheKeyPtr & key, int skip)
{
  auto it = this->cachetable.find(key);
  if (it != this->cachetable.end()) {
    for (std::size_t idx : it->second)
      this->drawentries[idx].skip += skip;
  }
}

inline void
SoFCRendererP::applyKeys(const CacheKeySet & keys, int skip)
{
  for (auto & key : keys)
    applyKey(key, skip);
}

void
SoFCRenderer::clearHighlight()
{
  PRIVATE(this)->highlightcaches.clear();
  PRIVATE(this)->opaquehighlight.clear();
  PRIVATE(this)->transphighlight.clear();
  PRIVATE(this)->hlentries.clear();
  PRIVATE(this)->applyKeys(PRIVATE(this)->highlightkeys, -1);
  PRIVATE(this)->highlightkeys.clear();
  PRIVATE(this)->highlightbbox = SbBox3f();
}

inline std::size_t
SoFCRendererP::pushDrawEntry(std::vector<DrawEntry> & draw_entries,
                             const Material & material, 
                             const VertexCacheEntry & ventry)
{
  draw_entries.emplace_back(&material, &ventry);
  if (draw_entries.back().bbox.isEmpty()) {
    draw_entries.pop_back();
    return 0;
  }
  return draw_entries.size();
}

void
SoFCRenderer::setScene(const std::vector<RenderCachePtr> & caches)
{
  if (PRIVATE(this)->scene == caches)
    return;

  PRIVATE(this)->scenebbox = SbBox3f();
  PRIVATE(this)->prevplane = SbPlane();
  PRIVATE(this)->opaquevcache.clear();
  PRIVATE(this)->opaqueontop.clear();
  PRIVATE(this)->transpvcache.clear();
  PRIVATE(this)->transpontop.clear();
  PRIVATE(this)->cachetable.clear();
  PRIVATE(this)->drawentries.clear();
  PRIVATE(this)->linesontop.clear();
  PRIVATE(this)->trianglesontop.clear();

  PRIVATE(this)->scene = caches;
  PRIVATE(this)->scenebbox = SbBox3f();

  for (auto & cache : PRIVATE(this)->scene) {
    const auto & vcaches = cache->getVertexCaches(true);
    for (const auto & v : vcaches) {
      auto & material = v.first;
      auto & ventries = v.second;
      if (ventries.empty()) continue;
      if (material.drawstyle == SoDrawStyleElement::INVISIBLE) continue;

      bool fulltransp = material.transptexture;
      if (!fulltransp && !material.pervertexcolor)
        fulltransp = (material.diffuse & 0xff) == 0xff ? false : true;

      for (auto & ventry : ventries) {
        std::size_t idx = SoFCRendererP::pushDrawEntry(PRIVATE(this)->drawentries, material, ventry);
        if (!idx)
          continue;
        --idx;
        PRIVATE(this)->scenebbox.extendBy(PRIVATE(this)->drawentries.back().bbox);
        PRIVATE(this)->cachetable[ventry.key].push_back(idx);

        if (material.isOnTop() && material.type == Material::Triangle)
          PRIVATE(this)->trianglesontop.emplace_back(idx);

        if (!fulltransp && (!material.pervertexcolor
                            || ventry.cache->hasOpaqueParts())) {
          if (material.isOnTop()) {
            if (material.type != Material::Triangle)
              PRIVATE(this)->linesontop.emplace_back(idx);
            else 
              PRIVATE(this)->opaqueontop.emplace_back(idx);
          } else
            PRIVATE(this)->opaquevcache.emplace_back(idx);
        }

        if (fulltransp || (material.pervertexcolor
                            && ventry.cache->hasTransparency())) {
          if (material.isOnTop())
            PRIVATE(this)->transpontop.emplace_back(idx);
          else
            PRIVATE(this)->transpvcache.emplace_back(idx);
        }
      }
    }
  }
  PRIVATE(this)->applyKeys(PRIVATE(this)->highlightkeys);
  PRIVATE(this)->selectionkeys.clear();
  PRIVATE(this)->updateselection = true;
}

void
SoFCRenderer::setHighlight(VertexCacheMap && caches)
{
  clearHighlight();
  PRIVATE(this)->highlightcaches = std::move(caches);

  for (auto & v : PRIVATE(this)->highlightcaches) {
    auto & material = v.first;
    auto & ventries = v.second;
    if (ventries.empty()) continue;
    if (material.drawstyle == SoDrawStyleElement::INVISIBLE) continue;

    bool fulltransp = material.transptexture;
    if (!fulltransp && !material.pervertexcolor)
      fulltransp = (material.diffuse & 0xff) == 0xff ? false : true;

    for (auto & ventry : ventries) {
      std::size_t idx = SoFCRendererP::pushDrawEntry(PRIVATE(this)->hlentries, material, ventry);
      if (!idx)
        continue;
      --idx;

      if (material.isOnTop()
          && (material.partialhighlight 
              || (ventry.partidx < 0
                  && ventry.cache == ventry.cache->getWholeCache())))
      {
        // hide original object because we are doing full object highlight on top
        PRIVATE(this)->highlightkeys.insert(ventry.key);
        PRIVATE(this)->highlightbbox.extendBy(PRIVATE(this)->hlentries.back().bbox);
      }

      if (!fulltransp && (!material.pervertexcolor
                          || ventry.cache->hasOpaqueParts()))
        PRIVATE(this)->opaquehighlight.emplace_back(idx);

      if (fulltransp || (material.pervertexcolor
                          && ventry.cache->hasTransparency()))
        PRIVATE(this)->transphighlight.emplace_back(idx);
    }
  }
  PRIVATE(this)->applyKeys(PRIVATE(this)->highlightkeys);
}

void
SoFCRenderer::addSelection(int id, const VertexCacheMap & caches)
{
  if (id > 0)
    PRIVATE(this)->selectionsontop[id] = &caches;
  else 
    PRIVATE(this)->selections[id] = &caches;
  PRIVATE(this)->updateselection = true;
}

void
SoFCRenderer::removeSelection(int id)
{
  if (id > 0) {
    if (PRIVATE(this)->selectionsontop.erase(id))
      PRIVATE(this)->updateselection = true;
  }
  else if (PRIVATE(this)->selections.erase(id))
    PRIVATE(this)->updateselection = true;
}

void
SoFCRendererP::updateSelection()
{
  if (!this->updateselection)
    return;

  this->updateselection = false;
  this->opaqueselections.clear();
  this->transpselections.clear();
  this->transpselectionsontop.clear();
  this->selstriangleontop.clear();
  this->selslineontop.clear();
  this->selspointontop.clear();
  this->selsontop.clear();
  this->slentries.clear();
  this->selectionbbox = SbBox3f();
 
  CacheKeySet renderkeys;
  CacheKeyPtr lastkey;

  applyKeys(this->selectionkeys, -1);
  this->selectionkeys.clear();

  // checkKey() serves two purposes. In case of whole object selection, 1) make
  // sure normal object rendering is skipped, 2) make sure no duplicate
  // rendering of the same object selection.
  auto checkKey = [&](const Material & material, const VertexCacheEntry & ventry) -> std::size_t {
    std::size_t idx = pushDrawEntry(this->slentries, material, ventry);
    if (!idx)
      return 0;
    if (ventry.partidx >= 0 || ventry.cache != ventry.cache->getWholeCache())
      return idx;
    if (lastkey != ventry.key) {
      lastkey = ventry.key;
      if (!this->selkey)
        this->selkey.reset(new CacheKey);
      *this->selkey = *ventry.key;
    }
    this->selkey->push_back(ventry.cache->getNodeId());
    this->selkey->push_back(material.type);
    if (this->selectionkeys.insert(ventry.key).second) {
      applyKey(ventry.key);
      renderkeys.insert(this->selkey);
      this->selkey.reset();
      lastkey.reset();
    }
    else if (renderkeys.insert(this->selkey).second) {
      this->selkey.reset();
      lastkey.reset();
    }
    else {
      this->selkey->pop_back();
      this->selkey->pop_back();
      this->slentries.pop_back();
      return 0;
    }
    this->selectionbbox.extendBy(this->slentries.back().bbox);
    return idx;
  };

  for (auto & sel : this->selectionsontop) {
    for (auto & v : *sel.second) {
      auto & material = v.first;
      auto & ventries = v.second;
      if (ventries.empty()) continue;
      if (material.drawstyle == SoDrawStyleElement::INVISIBLE) continue;

      for (auto & ventry : ventries) {
        std::size_t idx = checkKey(material, ventry);
        if (!idx)
          continue;
        --idx;
        switch (material.type) {
          case Material::Triangle:
            this->transpselectionsontop.emplace_back(idx);
            if (!(sel.first & SoFCRenderer::SelIdSelected) || material.partialhighlight)
              this->selstriangleontop.emplace_back(idx);
            break;
          case Material::Line:
            if (sel.first & SoFCRenderer::SelIdPartial)
              this->selslineontop.emplace_back(idx);
            else if (!(sel.first & SoFCRenderer::SelIdFull) || material.partialhighlight)
              this->selsontop.emplace_back(idx);
            else
              this->transpselectionsontop.emplace_back(idx);
            break;
          case Material::Point:
            if (sel.first & SoFCRenderer::SelIdPartial)
              this->selspointontop.emplace_back(idx);
            else if (!(sel.first & SoFCRenderer::SelIdFull) || material.partialhighlight)
              this->selsontop.emplace_back(idx);
            else
              this->transpselectionsontop.emplace_back(idx);
            break;
        }
      }
    }
  }

  for (auto & sel : this->selections) {
    for (auto & v : *sel.second) {
      auto & material = v.first;
      auto & ventries = v.second;
      if (ventries.empty()) continue;
      if (material.drawstyle == SoDrawStyleElement::INVISIBLE) continue;

      bool fulltransp = material.transptexture;
      if (!fulltransp && !material.pervertexcolor)
        fulltransp = (material.diffuse & 0xff) == 0xff ? false : true;

      for (auto & ventry : ventries) {
        std::size_t idx = checkKey(material, ventry);
        if (!idx)
            continue;
        --idx;
        if (!fulltransp && (!material.pervertexcolor
                            || ventry.cache->hasOpaqueParts()))
          this->opaqueselections.emplace_back(idx);

        if (fulltransp || (material.pervertexcolor
                            && ventry.cache->hasTransparency()))
          this->transpselections.emplace_back(idx);
      }
    }
  }
}

void
SoFCRenderer::getBoundingBox(SbBox3f & bbox) const
{
  PRIVATE(this)->updateSelection();
  if (!PRIVATE(this)->scenebbox.isEmpty())
    bbox.extendBy(PRIVATE(this)->scenebbox);
  if (!PRIVATE(this)->highlightbbox.isEmpty())
    bbox.extendBy(PRIVATE(this)->highlightbbox);
  if (!PRIVATE(this)->selectionbbox.isEmpty())
    bbox.extendBy(PRIVATE(this)->selectionbbox);
}

void inline 
SoFCRendererP::setupMatrix(SoState * state, const VertexCacheEntry * ventry)
{
  if (this->identity) {
    if (ventry->identity)
      SoModelMatrixElement::makeIdentity(state, NULL);
    else
      SoModelMatrixElement::set(state, NULL, ventry->matrix);
  } else if (ventry->identity)
    SoModelMatrixElement::set(state, NULL, this->matrix);
  else {
    SbMatrix mat = this->matrix;
    SoModelMatrixElement::set(state, NULL, mat.multLeft(ventry->matrix));
  }
}

void
SoFCRendererP::renderOpaque(SoGLRenderAction * action,
                            std::vector<DrawEntry> & draw_entries,
                            std::vector<std::size_t> & indices,
                            int pass)
{
  SoState * state = action->getState();
  for (std::size_t idx : indices) {
    auto & draw_entry = draw_entries[idx];
    if (draw_entry.skip)
      continue;
    if (this->recheckmaterial 
        || this->prevpass != pass
        || this->prevmaterial != draw_entry.material) {
      if (!applyMaterial(action, *draw_entry.material, false, pass))
        continue;
      this->prevpass = pass;
      this->recheckmaterial = false;
      this->prevmaterial = draw_entry.material;
    }
    setupMatrix(state, draw_entry.ventry);

    int array = SoFCVertexCache::ALL;
    if (!this->material.pervertexcolor)
      array ^= SoFCVertexCache::COLOR;
    if (this->notexture)
      array ^= SoFCVertexCache::TEXCOORD;

    bool overridelightmodel = false;
    if (this->material.lightmodel == SoLazyElement::BASE_COLOR)
      array ^= SoFCVertexCache::NORMAL;
    else if (!draw_entry.ventry->cache->getNormalArray()) {
      array ^= SoFCVertexCache::NORMAL;
      overridelightmodel = true;
      glDisable(GL_LIGHTING);
    }

    switch (draw_entry.material->type) {
    case Material::Triangle:
      if (!draw_entry.ventry->cache->hasTransparency())
        draw_entry.ventry->cache->renderTriangles(state, array, draw_entry.ventry->partidx);
      else if (!this->material.pervertexcolor) {
        // this means override transparency (i.e. force opaque)
        draw_entry.ventry->cache->renderTriangles(state, SoFCVertexCache::NON_SORTED, draw_entry.ventry->partidx);
      }
      else if (!this->material.twoside) {
        glLightModeli(GL_LIGHT_MODEL_TWO_SIDE, GL_TRUE);
        draw_entry.ventry->cache->renderTriangles(state, array, draw_entry.ventry->partidx);
        glLightModeli(GL_LIGHT_MODEL_TWO_SIDE, GL_FALSE);
      }
      break;
    case Material::Line:
      draw_entry.ventry->cache->renderLines(state, array, draw_entry.ventry->partidx);
      break;
    case Material::Point:
      draw_entry.ventry->cache->renderPoints(state, array, draw_entry.ventry->partidx);
      break;
    }
    if (overridelightmodel)
      glEnable(GL_LIGHTING);
  }
}

void
SoFCRendererP::renderTransparency(SoGLRenderAction * action,
                                  std::vector<DrawEntry> & draw_entries,
                                  std::vector<DrawEntryIndex> & indices,
                                  bool sort)
{
  if (indices.empty())
    return;

  SoState * state = action->getState();

  if (sort) {
    SbPlane plane = SoViewVolumeElement::get(state).getPlane(0.0);
    if (plane != this->prevplane) {
      this->prevplane = plane;
      if (!this->identity)
        plane.transform(this->matrix.inverse());
      for (auto & v : indices)
        v.distance = plane.getDistance(draw_entries[v.idx].bbox.getCenter());

      std::sort(indices.begin(), indices.end(),
        [](const DrawEntryIndex &a, const DrawEntryIndex &b) {
          return a.distance < b.distance;
        });
    }
  }

  glEnable(GL_BLEND);
  glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

  for (auto & v : indices) {
    auto & draw_entry = draw_entries[v.idx];
    if (draw_entry.skip)
      continue;
    if (this->recheckmaterial || this->prevmaterial != draw_entry.material) {
      if (!applyMaterial(action, *draw_entry.material, true))
        continue;
      this->recheckmaterial = false;
      this->prevmaterial = draw_entry.material;
    }
    setupMatrix(state, draw_entry.ventry);

    int array = SoFCVertexCache::ALL;
    if (!this->material.pervertexcolor)
      array ^= SoFCVertexCache::COLOR;
    if (this->notexture)
      array ^= SoFCVertexCache::TEXCOORD;

    bool overridelightmodel = false;
    if (this->material.lightmodel == SoLazyElement::BASE_COLOR)
      array ^= SoFCVertexCache::NORMAL;
    else if (!draw_entry.ventry->cache->getNormalArray()) {
      array ^= SoFCVertexCache::NORMAL;
      overridelightmodel = true;
      glDisable(GL_LIGHTING);
    }

    switch (draw_entry.material->type) {
    case Material::Line:
      draw_entry.ventry->cache->renderLines(state, array, draw_entry.ventry->partidx);
      break;
    case Material::Point:
      draw_entry.ventry->cache->renderPoints(state, array, draw_entry.ventry->partidx);
      break;
    case Material::Triangle:
      {
        if (!draw_entry.ventry->cache->hasTransparency())
          array |= SoFCVertexCache::FULL_SORTED_ARRAY;
        else
          array |= SoFCVertexCache::SORTED_ARRAY;
        draw_entry.ventry->cache->renderTriangles(state, array, draw_entry.ventry->partidx);
      }
      break;
    }

    if (overridelightmodel)
      glEnable(GL_LIGHTING);
  }

  glDisable(GL_BLEND);
}

void
SoFCRenderer::render(SoGLRenderAction * action)
{
  SoState * state = action->getState();

  PRIVATE(this)->updateSelection();

  PRIVATE(this)->depthwriteonly = false;
  PRIVATE(this)->notexture = false;
  PRIVATE(this)->prevmaterial = nullptr;
  PRIVATE(this)->recheckmaterial = false;
  PRIVATE(this)->material.init();

  glPushAttrib(GL_LIGHTING_BIT|
               GL_DEPTH_BUFFER_BIT|
               GL_COLOR_BUFFER_BIT|
               GL_TEXTURE_BIT|
               GL_LINE_BIT|
               GL_POINT_BIT|
               GL_ENABLE_BIT|
               GL_POLYGON_BIT);
  state->push();

  PRIVATE(this)->matrix = SoModelMatrixElement::get(state);
  PRIVATE(this)->identity = (PRIVATE(this)->matrix == SbMatrix::identity());

  PRIVATE(this)->renderOpaque(action,
                              PRIVATE(this)->drawentries,
                              PRIVATE(this)->opaquevcache);

  PRIVATE(this)->recheckmaterial = true;
  PRIVATE(this)->notexture = true;

  PRIVATE(this)->renderOpaque(action,
                              PRIVATE(this)->slentries,
                              PRIVATE(this)->opaqueselections);

  PRIVATE(this)->recheckmaterial = true;
  PRIVATE(this)->notexture = false;

  PRIVATE(this)->renderTransparency(action,
                                    PRIVATE(this)->drawentries,
                                    PRIVATE(this)->transpvcache);

  PRIVATE(this)->recheckmaterial = true;
  PRIVATE(this)->notexture = true;

  PRIVATE(this)->renderTransparency(action,
                                    PRIVATE(this)->slentries,
                                    PRIVATE(this)->transpselections);

  PRIVATE(this)->recheckmaterial = true;
  PRIVATE(this)->notexture = false;

  PRIVATE(this)->renderOpaque(action,
                              PRIVATE(this)->drawentries,
                              PRIVATE(this)->opaqueontop);

  PRIVATE(this)->renderTransparency(action,
                                    PRIVATE(this)->drawentries,
                                    PRIVATE(this)->transpontop,
                                    false);

  PRIVATE(this)->recheckmaterial = true;
  PRIVATE(this)->notexture = true;

  PRIVATE(this)->renderTransparency(action,
                                    PRIVATE(this)->slentries,
                                    PRIVATE(this)->transpselectionsontop,
                                    false);

  bool hassel = PRIVATE(this)->selstriangleontop.size()
                    && (PRIVATE(this)->selsontop.size()
                        || PRIVATE(this)->selslineontop.size());
  bool hasontop = PRIVATE(this)->trianglesontop.size()
                      && PRIVATE(this)->linesontop.size();
  int pass = RenderPassNormal;

  if (hassel || hasontop) {
    // If there is lines/points on top perform a depth write only rendering
    // pass for all the triangles on top, so that we can distinguish line style
    // for hidden (by depth test) and non-hidden lines/points.

    PRIVATE(this)->recheckmaterial = true;
    PRIVATE(this)->depthwriteonly = true;
    glColorMask(GL_FALSE, GL_FALSE, GL_FALSE, GL_FALSE);
    if (hasontop)
      PRIVATE(this)->renderOpaque(action,
                                  PRIVATE(this)->drawentries,
                                  PRIVATE(this)->trianglesontop);
    if (hassel)
      PRIVATE(this)->renderOpaque(action,
                                  PRIVATE(this)->slentries,
                                  PRIVATE(this)->selstriangleontop);
    PRIVATE(this)->depthwriteonly = false;
    glColorMask(GL_TRUE, GL_TRUE, GL_TRUE, GL_TRUE);

    pass = RenderPassLinePattern;
  }

  // Even if we are calling renderOpaque() below (because of lines and points
  // render), we shall still respect the transparency setting, e.g. we'll use
  // transparency to dim the hidden lines. So we enable blending here.
  glEnable(GL_BLEND);

  // Rendering lines/points on top (i.e. without depth test), with user
  // configurable line pattern.
  PRIVATE(this)->renderOpaque(action,
                              PRIVATE(this)->drawentries,
                              PRIVATE(this)->linesontop,
                              pass);

  PRIVATE(this)->renderOpaque(action,
                              PRIVATE(this)->slentries,
                              PRIVATE(this)->selsontop,
                              pass);

  PRIVATE(this)->renderOpaque(action,
                              PRIVATE(this)->slentries,
                              PRIVATE(this)->selslineontop,
                              pass | RenderPassHighlight);

  if (hassel || hasontop) {
    // Second pass for rendering non-hidden lines/points. The depth test will
    // be enabled by applyMaterial() up on seeing this RenderPassLineSolid
    pass = RenderPassLineSolid;

    if (hasontop)
      PRIVATE(this)->renderOpaque(action,
                                  PRIVATE(this)->drawentries,
                                  PRIVATE(this)->linesontop,
                                  pass);
    if (hassel) {
      PRIVATE(this)->renderOpaque(action,
                                  PRIVATE(this)->slentries,
                                  PRIVATE(this)->selsontop,
                                  pass);

      PRIVATE(this)->renderOpaque(action,
                                  PRIVATE(this)->slentries,
                                  PRIVATE(this)->selslineontop,
                                  pass | RenderPassHighlight);
    }
  }

  glDisable(GL_BLEND);

  PRIVATE(this)->renderOpaque(action,
                              PRIVATE(this)->slentries,
                              PRIVATE(this)->selspointontop,
                              RenderPassHighlight);

  PRIVATE(this)->renderOpaque(action,
                              PRIVATE(this)->hlentries,
                              PRIVATE(this)->opaquehighlight,
                              RenderPassHighlight);

  PRIVATE(this)->renderTransparency(action,
                                    PRIVATE(this)->hlentries,
                                    PRIVATE(this)->transphighlight,
                                    false);

  state->pop();
  glPopAttrib();
}

// vim: noai:ts=2:sw=2
