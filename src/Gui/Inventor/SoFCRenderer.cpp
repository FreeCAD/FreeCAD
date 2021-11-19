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

#ifndef FC_OS_WIN32
# ifndef GL_GLEXT_PROTOTYPES
# define GL_GLEXT_PROTOTYPES 1
# endif
#endif

# ifdef FC_OS_WIN32
#  include <windows.h>
#  include <GL/gl.h>
#  include <GL/glext.h>
# else
#  ifdef FC_OS_MACOSX
#   include <OpenGL/gl.h>
#   include <OpenGL/glext.h>
#  else
#   include <GL/gl.h>
#   include <GL/glext.h>
#  endif //FC_OS_MACOSX
# endif //FC_OS_WIN32
// Should come after glext.h to avoid warnings
# include <Inventor/C/glue/gl.h>

#include <algorithm>
#include <unordered_map>

#include <Inventor/actions/SoGLRenderAction.h>
#include <Inventor/elements/SoTextureEnabledElement.h>
#include <Inventor/elements/SoShapeStyleElement.h>
#include <Inventor/elements/SoOverrideElement.h>
#include <Inventor/elements/SoLazyElement.h>
#include <Inventor/elements/SoGLLazyElement.h>
#include <Inventor/elements/SoLinePatternElement.h>
#include <Inventor/elements/SoLineWidthElement.h>
#include <Inventor/elements/SoPointSizeElement.h>
#include <Inventor/elements/SoDrawStyleElement.h>
#include <Inventor/elements/SoMaterialBindingElement.h>
#include <Inventor/elements/SoCacheElement.h>
#include <Inventor/elements/SoPolygonOffsetElement.h>
#include <Inventor/elements/SoViewVolumeElement.h>
#include <Inventor/elements/SoViewportRegionElement.h>
#include <Inventor/elements/SoModelMatrixElement.h>
#include <Inventor/elements/SoTextureUnitElement.h>
#include <Inventor/elements/SoMultiTextureEnabledElement.h>
#include <Inventor/elements/SoMultiTextureImageElement.h>
#include <Inventor/elements/SoMultiTextureMatrixElement.h>
#include <Inventor/elements/SoShapeHintsElement.h>
#include <Inventor/elements/SoLightModelElement.h>
#include <Inventor/elements/SoDepthBufferElement.h>
#include <Inventor/elements/SoClipPlaneElement.h>
#include <Inventor/elements/SoCullElement.h>
#include <Inventor/elements/SoGLShaderProgramElement.h>
#include <Inventor/sensors/SoFieldSensor.h>
#include <Inventor/annex/FXViz/elements/SoShadowStyleElement.h>
#include <Inventor/nodes/SoGroup.h>
#include <Inventor/nodes/SoMaterial.h>
#include <Inventor/nodes/SoDepthBuffer.h>
#include <Inventor/nodes/SoClipPlane.h>
#include <Inventor/annex/FXViz/nodes/SoShadowDirectionalLight.h>
#include <Inventor/annex/FXViz/nodes/SoShadowSpotLight.h>
#include <Inventor/actions/SoCallbackAction.h>
#include <Inventor/SbPlane.h>
#include <Inventor/SbBox3f.h>
#include <Inventor/SbSphere.h>
#include <Inventor/SbRotation.h>

#include <Base/Console.h>
#include "SoFCRenderer.h"
#include "SoFCRenderCache.h"
#include "SoFCVertexCache.h"
#include "SoFCDisplayModeElement.h"
#include "../ViewParams.h"

FC_LOG_LEVEL_INIT("Renderer", true, true)

using namespace Gui;

typedef SoFCRenderCache::Material Material;
typedef SoFCRenderCache::VertexCacheEntry VertexCacheEntry;
typedef SoFCRenderCache::VertexCacheMap VertexCacheMap;
typedef SoFCRenderCache::CacheKey CacheKey;
typedef SoFCRenderCache::CacheKeyPtr CacheKeyPtr;
typedef Gui::CoinPtr<SoFCRenderCache> RenderCachePtr;
typedef Gui::CoinPtr<SoFCVertexCache> VertexCachePtr;

#define PRIVATE(obj) ((obj)->pimpl)

#define FC_GLERROR_CHECK _check_glerror(__LINE__)
  
static inline void
_check_glerror(int line) {
  if (FC_LOG_INSTANCE.isEnabled(FC_LOGLEVEL_LOG)) {
    GLenum err = glGetError();
    if (err != GL_NO_ERROR)
      _FC_ERR(__FILE__, line, "GL error: " << err);
  }
}

typedef SoFCRenderCache::CacheKeySet CacheKeySet;
typedef SoFCRenderCache::CacheKeyHasher CacheKeyHasher;

struct DrawEntry {
  const Material * material;
  const VertexCacheEntry * ventry;
  SbBox3f bbox;
  float radius;
  int skip;

  DrawEntry(const Material * m, const VertexCacheEntry * v)
    :material(m), ventry(v), skip(0)
  {
    v->cache->getBoundingBox(v->identity ? nullptr : &v->matrix, this->bbox);
    SbSphere sphere;
    sphere.circumscribe(this->bbox);
    this->radius = sphere.getRadius();
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

struct HatchTexture
{
  const void *key = nullptr;
  SbFCVector<unsigned char> data;
  GLuint texture = 0;
  FC_COIN_COUNTER(int) refcount = 0;
  int width = 100;
  int height = 100;
  int nc = 0;
};

class SoFCRendererP {
public:
  SoFCRendererP()
  {
    this->updateselection = false;
    memset(this->stats, 0, sizeof(this->stats));
  }

  ~SoFCRendererP()
  {
  }

  void deleteHatchTexture();

  bool applyMaterial(SoGLRenderAction * action,
                     const Material & next,
                     bool transp,
                     int pass = RenderPassNormal);

  void setupMatrix(SoGLRenderAction * action, const DrawEntry &draw_entry);

  void updateSelection();

  static std::size_t pushDrawEntry(SbFCVector<DrawEntry> & draw_entries,
                                   const Material & material,
                                   const VertexCacheEntry & ventry);

  bool renderSection(SoGLRenderAction *action, DrawEntry &draw_entry, int &pass, bool &pushed);

  void renderOutline(SoGLRenderAction *action, DrawEntry &draw_entry, bool highlight);

  void pauseShadowRender(SoState *state, bool paused);
  void renderLines(SoState *state, int array, DrawEntry &draw_entry);
  void renderPoints(SoGLRenderAction *action, int array, DrawEntry &draw_entry);

  void renderOpaque(SoGLRenderAction * action,
                    SbFCVector<DrawEntry> & draw_entries,
                    SbFCVector<std::size_t> & indices,
                    int pass = RenderPassNormal);

  void renderTransparency(SoGLRenderAction * action,
                          SbFCVector<DrawEntry> & draw_entries,
                          SbFCVector<DrawEntryIndex> & indices,
                          bool sort=true);

  void applyKeys(const CacheKeySet &keys, int skip=1);
  void changeKey(const CacheKeySet &keys, int idx, int skip);

  SbFCVector<DrawEntry> drawentries;
  SbFCVector<DrawEntry> slentries;
  SbFCVector<DrawEntry> hlentries; 

  SbFCVector<std::size_t> opaquevcache;
  SbFCVector<std::size_t> opaqueontop;
  SbFCVector<std::size_t> opaqueselections;
  SbFCVector<std::size_t> opaquehighlight;
  SbFCVector<std::size_t> opaquelineshighlight; // has both lines and points
  SbFCVector<std::size_t> linesontop; // has both lines and points
  SbFCVector<std::size_t> trianglesontop;

  SbPlane prevplane;
  SbFCVector<DrawEntryIndex> transpvcache;
  SbFCVector<DrawEntryIndex> transpontop;
  SbFCVector<DrawEntryIndex> transpselections;
  SbFCVector<DrawEntryIndex> transphighlight;

  SbFCMap<int, const VertexCacheMap *> selections;
  SbFCMap<int, const VertexCacheMap *> selectionsontop;
  SbFCVector<DrawEntryIndex> transpselectionsontop;
  SbFCVector<std::size_t> selstriangleontop;
  SbFCVector<std::size_t> selsontop; // include only non-explicitly selected lines and points
  SbFCVector<std::size_t> selslineontop; // include only explicitly selected lines
  SbFCVector<std::size_t> selspointontop; // include only explictly selected points
  bool updateselection;

  std::unordered_map<CacheKeyPtr,
                     SbFCVector<std::size_t>,
                     CacheKeyHasher,
                     CacheKeyHasher> cachetable;

  VertexCacheMap highlightcaches;
  CacheKeySet highlightkeys;
  CacheKeySet selectionkeys;
  CacheKeyPtr selkey;

  RenderCachePtr scene;

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
  bool hlwholeontop = false;

  bool shadowrenderpaused = false;
  bool shadowrendering = false;
  bool shadowmapping = false;
  bool transpshadowmapping = false;

  HatchTexture *hatchtexture = nullptr;

  char stats[512];
  int drawcallcount;
};

static std::map<const void *, HatchTexture> _HatchTextures;

SoFCRenderer::SoFCRenderer()
  : pimpl(new SoFCRendererP)
{
}

SoFCRenderer::~SoFCRenderer()
{
  PRIVATE(this)->deleteHatchTexture();
  delete pimpl;
}

void
SoFCRendererP::deleteHatchTexture()
{
  if (!this->hatchtexture || --this->hatchtexture->refcount)
    return;
  if (this->hatchtexture->texture)
    glDeleteTextures(1, &this->hatchtexture->texture);
  _HatchTextures.erase(this->hatchtexture->key);
  this->hatchtexture = nullptr;
}

void
SoFCRenderer::setHatchImage(const void *dataptr, int nc, int width, int height)
{
  if (!dataptr) {
    PRIVATE(this)->deleteHatchTexture();
    return;
  }

  auto &info = _HatchTextures[dataptr];
  if (&info == PRIVATE(this)->hatchtexture)
    return;

  PRIVATE(this)->deleteHatchTexture();
  if (++info.refcount == 1) {
    info.width = width;
    info.height = height;
    info.nc = nc;
    info.key = dataptr;
    info.data.resize(nc * width * height);
    memcpy(&info.data[0], dataptr, info.data.size());
  }
  PRIVATE(this)->hatchtexture = &info;
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
  FC_GLERROR_CHECK;
}

static inline void
setGLFeature(int name, int current, int next, int mask)
{
  if ((current & mask) && !(next & mask))
    glDisable(name);
  else if (!(current & mask) && (next & mask))
    glEnable(name);
  FC_GLERROR_CHECK;
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

  if (this->shadowmapping
      && (next.isOnTop() || !(next.shadowstyle & SoShadowStyleElement::CASTS_SHADOW)))
  {
    return false;
  }

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
      SoLazyElement::setLightModel(state, this->material.lightmodel);
      glDisable(GL_LIGHTING);
      FC_GLERROR_CHECK;
    }
    // disable per vertex color
    this->material.pervertexcolor = false;
    // enable depth write
    if (!this->material.depthwrite) {
      this->material.depthwrite = true;
      glDepthMask(GL_TRUE);
      FC_GLERROR_CHECK;
    }
    // force GL_LESS depth function
    if (this->material.depthfunc != SoDepthBuffer::LESS) {
      this->material.depthfunc = SoDepthBuffer::LESS;
      glDepthFunc(GL_LESS);
      FC_GLERROR_CHECK;
    }
    // enable depth test
    if (!this->material.depthtest) {
      this->material.depthtest = true;
      glEnable(GL_DEPTH_TEST);
      FC_GLERROR_CHECK;
    }
    return true;
  }

  this->material.pervertexcolor = next.pervertexcolor;

  auto clippers = next.clippers;
  if (this->shadowmapping
      || ((ViewParams::getNoSectionOnTop()
          || (ViewParams::getSectionConcave() && clippers.getNum() > 1))
          && next.isOnTop()))
    clippers.clear();

  bool clipperchanged = first || this->material.clippers != clippers;
  bool texturechanged = clipperchanged
            || this->material.textures != next.textures;
  bool lightchanged = texturechanged
            || this->material.lights != next.lights;

  if (clipperchanged || texturechanged || lightchanged) {
    state->pop();
    state->push();

    if (clippers.getNum()) {
      for(auto & info : clippers.getData()) {
        if (!info.identity)
          SoModelMatrixElement::set(state, NULL, info.matrix);
        state->setCacheOpen(false);
        info.node->GLRender(action);
        if (!info.identity)
          SoModelMatrixElement::makeIdentity(state, NULL);
      }
    }
    this->material.clippers = clippers;

    if (!this->notexture && texturechanged) {
      if (next.textures.getNum()) {
        for (auto & texentry : next.textures.getData()) {
          auto t = this->material.textures.get(texentry.first);
          if (t && *t == texentry.second)
            continue;
          SoMultiTextureMatrixElement::set(state, NULL, texentry.first,
              texentry.second.identity ? matrixidentity : texentry.second.matrix);
          SoTextureUnitElement::set(state, NULL, texentry.first);
          state->setCacheOpen(false);
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
          state->setCacheOpen(false);
          info.node->GLRender(action);
          if (!info.identity)
            SoModelMatrixElement::makeIdentity(state, NULL);
        }
      }
      this->material.lights = next.lights;
    }
  }

  bool depthtest = next.isOnTop() ? false : next.depthtest;
  bool depthwrite = !next.isOnTop() && transp ? false : next.depthwrite;
  int8_t depthfunc = next.depthfunc;
  uint32_t linepattern = next.linepattern;
  uint32_t col = next.diffuse;
  uint32_t emissive = next.emissive;
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
  }

  if (pass & RenderPassHighlight) {
    float scale = ViewParams::getSelectionLineThicken();
    if (scale < 1.0)
      scale = 1.0;
    float w = linewidth * scale;
    if (ViewParams::getSelectionLineMaxWidth() > 1.0)
      w = std::min<float>(w, std::max<float>(linewidth, ViewParams::getSelectionLineMaxWidth()));
    linewidth = w;

    float pscale = ViewParams::getSelectionPointScale();
    if (pscale < 1.0)
      pscale = scale;
    w = pointsize * pscale;
    if (ViewParams::getSelectionPointMaxSize() > 1.0)
      w = std::min<float>(w, std::max<float>(pointsize, ViewParams::getSelectionPointMaxSize()));
    pointsize = w;
  }

  if (first || this->material.depthtest != depthtest) {
    if (depthtest)
      glEnable(GL_DEPTH_TEST);
    else
      glDisable(GL_DEPTH_TEST);
    FC_GLERROR_CHECK;
    this->material.depthtest = depthtest;
  }

  if (first || this->material.depthclamp != next.depthclamp) {
    if (next.depthclamp)
      glEnable(GL_DEPTH_CLAMP);
    else
      glDisable(GL_DEPTH_CLAMP);
    FC_GLERROR_CHECK;
    this->material.depthclamp = next.depthclamp;
  }

  if (first || this->material.depthwrite != depthwrite) {
    glDepthMask(depthwrite ? GL_TRUE : GL_FALSE);
    FC_GLERROR_CHECK;
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
    FC_GLERROR_CHECK;
    this->material.depthfunc = depthfunc;
  }

  auto lightmodel = next.lightmodel;
  if (next.type != Material::Triangle)
    lightmodel = SoLazyElement::BASE_COLOR;

  if (first || this->material.lightmodel != lightmodel) {
    SoLazyElement::setLightModel(state, lightmodel);
    if (lightmodel == SoLazyElement::PHONG)
      glEnable(GL_LIGHTING);
    else
      glDisable(GL_LIGHTING);
    FC_GLERROR_CHECK;
    this->material.lightmodel = lightmodel;
  }

  // Always set color because the current color may be changed by opengl draw call
  glColor4ub((unsigned char)((col>>24)&0xff),
              (unsigned char)((col>>16)&0xff),
              (unsigned char)((col>>8)&0xff),
              (unsigned char)(col&0xff));
  FC_GLERROR_CHECK;

  if (overrideflags != this->material.overrideflags
      || (overrideflags.test(Material::FLAG_TRANSPARENCY)
          && (col&0xff) != (this->material.diffuse&0xff)))
  {
    static bool hasBlendColor = true;
    GLenum sfactor = GL_SRC_ALPHA, dfactor = GL_ONE_MINUS_SRC_ALPHA;
    if (hasBlendColor && overrideflags.test(Material::FLAG_TRANSPARENCY)) {
#ifdef FC_OS_WIN32
      static PFNGLBLENDCOLORPROC glBlendColor;
      if (hasBlendColor && !glBlendColor) {
        const cc_glglue * glue = cc_glglue_instance(action->getCacheContext());
        glBlendColor = (PFNGLBLENDCOLORPROC)cc_glglue_getprocaddress(glue, "glBlendColor");
        hasBlendColor = (glBlendColor != nullptr);
      }
#endif
      if (hasBlendColor) {
        glBlendColor(0.f, 0.f, 0.f,  (col & 0xff)/255.f);
        sfactor = GL_CONSTANT_ALPHA_EXT;
        dfactor = GL_ONE_MINUS_CONSTANT_ALPHA_EXT;
        FC_GLERROR_CHECK;
      }
    }
    glBlendFunc(sfactor, dfactor);
    FC_GLERROR_CHECK;
  }

  this->material.overrideflags = overrideflags;
  this->material.diffuse = col;

  // Must clear emission color for lines and points if they are to be rendered
  // with lighting as BASE_COLOR. For some reason, if shadow is enabled
  // (possibly due to extra light source), emission color is taking effect
  // even if lighting is BASE_COLOR.
  if (this->material.lightmodel == SoLazyElement::BASE_COLOR)
    emissive = 0;

  if (first || this->material.emissive != emissive) {
    setGLColor(GL_EMISSION, emissive);
    this->material.emissive = emissive;
  }

  if (next.type == Material::Line) {
    if (first || this->material.linewidth != linewidth) {
      glLineWidth(linewidth);
      FC_GLERROR_CHECK;
      this->material.linewidth = linewidth;
    }

    if (first || this->material.linepattern != linepattern) {
      if ((linepattern & 0xffff) == 0xffff)
        glDisable(GL_LINE_STIPPLE);
      else {
        glEnable(GL_LINE_STIPPLE);
        glLineStipple((GLint) (linepattern >> 16), (GLshort) (linepattern & 0xffff));
      }
      FC_GLERROR_CHECK;
      this->material.linepattern = linepattern;
    }
    if (!first)
      return true;
  }

  if (next.type == Material::Point) {
    if (first || this->material.pointsize != pointsize) {
      glPointSize(pointsize);
      this->material.pointsize = pointsize;
      FC_GLERROR_CHECK;
    }
    if (!first)
      return true;
  }

  if (first || this->material.ambient != next.ambient) {
    setGLColor(GL_AMBIENT, next.ambient);
    this->material.ambient = next.ambient;
  }

  if (first || this->material.specular != next.specular) {
    setGLColor(GL_SPECULAR, next.specular);
    this->material.specular = next.specular;
  }

  if (first || this->material.shininess != next.shininess) {
    glMaterialf(GL_FRONT_AND_BACK, GL_SHININESS, next.shininess*128.0f);
    FC_GLERROR_CHECK;
    this->material.shininess = next.shininess;
  }

  if (first || this->material.vertexordering != next.vertexordering) {
    glFrontFace(next.vertexordering == SoLazyElement::CW ? GL_CW : GL_CCW);
    FC_GLERROR_CHECK;
    this->material.vertexordering = next.vertexordering;
  }

  int8_t twoside = next.twoside;
  if (transp || next.isOnTop())
    twoside = 1;
  if (first || this->material.twoside != twoside) {
    SoLazyElement::setTwosideLighting(state, twoside);
    glLightModeli(GL_LIGHT_MODEL_TWO_SIDE, twoside ? GL_TRUE : GL_FALSE);
    FC_GLERROR_CHECK;
    this->material.twoside = twoside;
  }

  int8_t culling = next.culling;
  if (transp)
    culling = 0;
  if (first || this->material.culling != culling) {
    if (culling) glEnable(GL_CULL_FACE);
    else glDisable(GL_CULL_FACE);
    FC_GLERROR_CHECK;
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
    FC_GLERROR_CHECK;
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
    FC_GLERROR_CHECK;
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
  PRIVATE(this)->opaquelineshighlight.clear();
  PRIVATE(this)->transphighlight.clear();
  PRIVATE(this)->highlightkeys.clear();

  PRIVATE(this)->cachetable.clear();
}

void
SoFCRendererP::changeKey(const CacheKeySet & keys, int idx, int skip)
{
  auto & draw_entry = this->drawentries[idx];
  int prev = draw_entry.skip;
  draw_entry.skip += skip;
  if (!draw_entry.ventry->mergecount)
    return;
  if (!prev && draw_entry.skip) {
    for (int i=1; i<=draw_entry.ventry->mergecount; ++i) {
      changeKey(keys, idx+i, -1);
      i += this->drawentries[idx+i].ventry->mergecount;
    }
  }
  else if (prev && !draw_entry.skip) {
    for (int i=1; i<=draw_entry.ventry->mergecount; ++i) {
      changeKey(keys, idx+i, 1);
      i += this->drawentries[idx+i].ventry->mergecount;
    }
  }
}

void
SoFCRendererP::applyKeys(const CacheKeySet & keys, int skip)
{
  for (auto & key : keys) {
    auto it = this->cachetable.find(key);
    if (it != this->cachetable.end()) {
      for (std::size_t idx : it->second)
        changeKey(keys, idx, skip);
    }
  }
}

void
SoFCRenderer::clearHighlight()
{
  PRIVATE(this)->hlwholeontop = false;
  PRIVATE(this)->highlightcaches.clear();
  PRIVATE(this)->opaquehighlight.clear();
  PRIVATE(this)->opaquelineshighlight.clear();
  PRIVATE(this)->transphighlight.clear();
  PRIVATE(this)->hlentries.clear();
  PRIVATE(this)->applyKeys(PRIVATE(this)->highlightkeys, -1);
  PRIVATE(this)->highlightkeys.clear();
  PRIVATE(this)->highlightbbox = SbBox3f();
}

inline std::size_t
SoFCRendererP::pushDrawEntry(SbFCVector<DrawEntry> & draw_entries,
                             const Material & material, 
                             const VertexCacheEntry & ventry)
{
  draw_entries.emplace_back(&material, &ventry);
  // if (draw_entries.back().bbox.isEmpty()) {
  //   draw_entries.pop_back();
  //   return 0;
  // }
  return draw_entries.size();
}

void
SoFCRenderer::setScene(const RenderCachePtr &cache)
{
  if (!cache) {
    clear();
    return;
  }

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

  PRIVATE(this)->scene = cache;
  PRIVATE(this)->scenebbox = SbBox3f();

  int mergecount = 0;
  const auto & caches = cache->getVertexCaches(true);
  for (const auto & v : caches) {
    auto & material = v.first;
    auto & ventries = v.second;
    if (ventries.empty()) continue;
    if (material.drawstyle == SoDrawStyleElement::INVISIBLE) continue;

    bool fulltransp = material.transptexture;
    if (!fulltransp && !material.pervertexcolor)
      fulltransp = (material.diffuse & 0xff) == 0xff ? false : true;

    int vidx = -1;
    for (auto & ventry : ventries) {
      ++vidx;
      std::size_t idx = SoFCRendererP::pushDrawEntry(PRIVATE(this)->drawentries, material, ventry);
      if (!idx)
        continue;
      --idx;
      PRIVATE(this)->scenebbox.extendBy(PRIVATE(this)->drawentries.back().bbox);
      if (!ventry.skipcount)
        ++mergecount;
      else
        PRIVATE(this)->drawentries.back().skip = ventry.skipcount;
      PRIVATE(this)->cachetable[ventry.key].push_back(idx);
      for (int i=0; i<ventry.mergecount; ++i) {
        int j = i+vidx+1;
        if (j >= (int)ventries.size())
          break;
        auto &indices = PRIVATE(this)->cachetable[ventries[j].key];
        if (indices.empty() || indices.back() != idx)
          indices.push_back(idx);
        i += ventries[j].mergecount;
      }

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

  FC_TRACE("update scene " << caches.size() << " materials, "
        << PRIVATE(this)->drawentries.size() << " entries, "
        << mergecount << " after merge");

  PRIVATE(this)->applyKeys(PRIVATE(this)->highlightkeys);
  PRIVATE(this)->selectionkeys.clear();
  PRIVATE(this)->updateselection = true;
}

void
SoFCRenderer::setHighlight(VertexCacheMap && caches, bool wholeontop)
{
  clearHighlight();
  PRIVATE(this)->highlightcaches = std::move(caches);
  PRIVATE(this)->hlwholeontop = wholeontop;

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

      if (material.overrideflags.test(Material::FLAG_TRANSPARENCY)) {
        if ((material.diffuse & 0xff) != 0xff)
          PRIVATE(this)->transphighlight.emplace_back(idx);
        else if (material.type == Material::Triangle)
          PRIVATE(this)->opaquehighlight.emplace_back(idx);
        else
          PRIVATE(this)->opaquelineshighlight.emplace_back(idx);
      }
      else {
        if (!fulltransp && (!material.pervertexcolor
                            || ventry.cache->hasOpaqueParts()))
        {
          if (material.type == Material::Triangle)
            PRIVATE(this)->opaquehighlight.emplace_back(idx);
          else
            PRIVATE(this)->opaquelineshighlight.emplace_back(idx);
        }

        if (fulltransp || (material.pervertexcolor
                            && ventry.cache->hasTransparency()))
          PRIVATE(this)->transphighlight.emplace_back(idx);
      }
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

  applyKeys(this->selectionkeys, -1);
  this->selectionkeys.clear();

  // checkKey() serves two purposes. In case of whole object selection, 1) make
  // sure normal object rendering is skipped, 2) make sure no duplicate
  // rendering of the same object selection.
  auto checkKey = [&](const Material & material, const VertexCacheEntry & ventry) -> std::size_t {
    std::size_t idx = pushDrawEntry(this->slentries, material, ventry);
    if (!idx)
      return 0;
    if (!ventry.key || ventry.partidx >= 0 || ventry.cache != ventry.cache->getWholeCache())
      return idx;
    if (!this->selkey)
      this->selkey = std::allocate_shared<CacheKey>(SoFCAllocator<CacheKey>());
    this->selkey->forcePush(ventry.cache->getNodeId());
    this->selkey->forcePush(material.type);
    this->selkey->append(ventry.key);
    if (this->selectionkeys.insert(ventry.key).second) {
      renderkeys.insert(this->selkey);
      this->selkey.reset();
    }
    else if (renderkeys.insert(this->selkey).second)
      this->selkey.reset();
    else {
      this->selkey->clear();
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

  applyKeys(this->selectionkeys);
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
SoFCRendererP::setupMatrix(SoGLRenderAction * action, const DrawEntry &draw_entry)
{
  SoState *state = action->getState();
  const VertexCacheEntry *ventry = draw_entry.ventry;

  SoModelMatrixElement::makeIdentity(state, NULL);
  if (!this->identity)
    SoModelMatrixElement::mult(state, NULL, this->matrix);

  if (draw_entry.material->autozoom.getNum()) {
    for (auto &info : draw_entry.material->autozoom.getData()) {
      if (info.resetmatrix) {
        if (info.identity)
          SoModelMatrixElement::makeIdentity(state, NULL);
        else
          SoModelMatrixElement::set(state, NULL, info.matrix);
      } else if (!info.identity)
        SoModelMatrixElement::mult(state, NULL, info.matrix);
      info.node->GLRender(action);
    }
  }

  if (!ventry->identity)
    SoModelMatrixElement::mult(state, NULL, ventry->matrix);
}

void
SoFCRendererP::pauseShadowRender(SoState *state, bool paused)
{
  if (!this->shadowrendering || this->shadowrenderpaused == paused)
    return;
  this->shadowrenderpaused = paused;
  SoGLShaderProgramElement::enable(state, paused ? FALSE: TRUE);
}

void
SoFCRendererP::renderLines(SoState *state, int array, DrawEntry &draw_entry)
{
  if (this->depthwriteonly || this->shadowmapping)
    return;
  bool noseam = ViewParams::getHiddenLineHideSeam()
    && draw_entry.ventry->partidx < 0
    && draw_entry.material->outline;
  pauseShadowRender(state, true);
  draw_entry.ventry->cache->renderLines(state, array, draw_entry.ventry->partidx, noseam);
  ++this->drawcallcount;
}

void
SoFCRendererP::renderPoints(SoGLRenderAction *action, int array, DrawEntry &draw_entry)
{
  if (this->depthwriteonly || this->shadowmapping)
    return;
  if (!ViewParams::getHiddenLineHideVertex()
      || draw_entry.ventry->partidx >= 0
      || !draw_entry.material->outline) {
    pauseShadowRender(action->getState(), true);
    draw_entry.ventry->cache->renderPoints(action, array, draw_entry.ventry->partidx);
    ++this->drawcallcount;
  }
}

void
SoFCRendererP::renderOutline(SoGLRenderAction *action,
                             DrawEntry &draw_entry,
                             bool highlight)
{
  int drawidx = draw_entry.ventry->partidx;
  if (this->shadowmapping
      || this->depthwriteonly
      || draw_entry.material->type != Material::Triangle
      || (!draw_entry.material->outline
          && (!ViewParams::getShowPreSelectedFaceOutline()
              || !highlight
              || draw_entry.ventry->partidx < 0)))
    return;

  SoState *state = action->getState();

  int numparts = draw_entry.ventry->cache->getNumNonFlatParts();
  int dummyparts[1];
  const int *partindices = nullptr;
  if (this->material.clippers.getNum() && drawidx < 0) {
    numparts = draw_entry.ventry->cache->getNumFaceParts();
  } else if (ViewParams::HiddenLinePerFaceOutline() && numparts && drawidx < 0) {
    partindices = draw_entry.ventry->cache->getNonFlatParts();
  } else {
    numparts = 1;
    dummyparts[0] = drawidx;
    partindices = dummyparts;
  }

  bool pushed = false;
  for (int i=0; i<numparts; ++i) {
    int partidx;
    if (partindices) {
      if (drawidx >= 0 && drawidx != partindices[i])
        continue;
      partidx = partindices[i];
    } else
      partidx = i;

    if (!pushed) {
      pushed = true;
      glPushAttrib(GL_ENABLE_BIT
          | GL_DEPTH_BUFFER_BIT
          | GL_STENCIL_BUFFER_BIT
          | GL_CURRENT_BIT
          | GL_POLYGON_BIT);

      pauseShadowRender(state, true);

      glEnable(GL_STENCIL_TEST);
      glDisable(GL_LIGHTING);
      glDisable(GL_TEXTURE_2D);
      glDisable(GL_CULL_FACE);
      glDisable(GL_LINE_STIPPLE);
      // glDisable(GL_DEPTH_TEST);
      auto col = drawidx >= 0 ? this->material.emissive
                              : draw_entry.material->hiddenlinecolor;
      glColor3ub((unsigned char)((col>>24)&0xff),
          (unsigned char)((col>>16)&0xff),
          (unsigned char)((col>>8)&0xff));
      float linewidth = draw_entry.material->linewidth;

      if (highlight) {
        glDisable(GL_BLEND);
        float w = linewidth * std::max(1.0, ViewParams::getSelectionLineThicken());
        if (ViewParams::getSelectionLineMaxWidth() > 1.0)
          w = std::min<float>(w, std::max<float>(linewidth, ViewParams::getSelectionLineMaxWidth()));
        linewidth = w;
      }
      glLineWidth(linewidth*1.5f);
    }

    glClear(GL_STENCIL_BUFFER_BIT);
    glColorMask(GL_FALSE, GL_FALSE, GL_FALSE, GL_FALSE);

    glStencilFunc (GL_ALWAYS, 1, -1);
    glStencilOp (GL_KEEP, GL_REPLACE, GL_REPLACE);
    glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
    draw_entry.ventry->cache->renderTriangles(state,
                                              SoFCVertexCache::NON_SORTED_ARRAY,
                                              partidx);
    ++this->drawcallcount;
    glColorMask(GL_TRUE, GL_TRUE, GL_TRUE, GL_TRUE);
    glStencilFunc(GL_NOTEQUAL, 1, -1);
    glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
    draw_entry.ventry->cache->renderTriangles(state,
                                              SoFCVertexCache::NON_SORTED_ARRAY,
                                              partidx);
    ++this->drawcallcount;
  }
  if (pushed) {
    glPopAttrib();
    // For some reason, GL_CURRENT_BIT doesn't seem to restore the color?
    auto col = this->material.diffuse;
    unsigned char r = (col >> 24) & 0xff;
    unsigned char g = (col >> 16) & 0xff;
    unsigned char b = (col >> 8) & 0xff;
    unsigned char a = col & 0xff;
    glColor4ub(r, g, b, a);
    if (highlight)
      glLineWidth(this->material.linewidth);
  }
}

bool
SoFCRendererP::renderSection(SoGLRenderAction *action,
                             DrawEntry &draw_entry,
                             int &pass,
                             bool &pushed)
{
  int curpass = pass++;

  int numclip = this->material.clippers.getNum();
  bool concave = ViewParams::getSectionConcave() && numclip > 1;

  if (this->depthwriteonly
      || curpass >= numclip
      || draw_entry.ventry->partidx >= 0
      || (draw_entry.material->shapetype != SoShapeHintsElement::SOLID
            && !draw_entry.ventry->cache->hasSolid())
      || (!ViewParams::getSectionFill() && !concave))
    return curpass == 0;

  if (draw_entry.material->type != Material::Triangle) {
    if (!concave)
      return curpass == 0;
    if (!pushed) {
      pushed = true;
      glPushAttrib(GL_ENABLE_BIT);
    }
    if (curpass == 0) {
      for (int i=1; i<numclip; ++i)
        glDisable(GL_CLIP_PLANE0 + i);
    } else
      glDisable(GL_CLIP_PLANE0 + curpass - 1);
    glEnable(GL_CLIP_PLANE0 + curpass);
    return true;
  }

  if (!pushed) {
    pushed = true;
    glPushAttrib(GL_ENABLE_BIT
        | GL_DEPTH_BUFFER_BIT
        | GL_STENCIL_BUFFER_BIT);
  }

  if (curpass == 0 && concave) {
    if (this->material.depthfunc != SoDepthBuffer::LESS)
      glDepthFunc(GL_LESS);
    if (this->material.polygonoffsetstyle & SoPolygonOffsetElement::FILLED)
      glDisable(GL_POLYGON_OFFSET_FILL);
  }

  glEnable(GL_STENCIL_TEST);
  glClear(GL_STENCIL_BUFFER_BIT);

  for (int i=0; i<numclip; ++i) {
    if (i == curpass)
      glEnable(GL_CLIP_PLANE0 + i);
    else
      glDisable(GL_CLIP_PLANE0 + i);
    FC_GLERROR_CHECK;
  }

  glPushAttrib(GL_ENABLE_BIT);
  FC_GLERROR_CHECK;
  glDisable(GL_DEPTH_TEST);
  glColorMask(GL_FALSE, GL_FALSE, GL_FALSE, GL_FALSE);
  FC_GLERROR_CHECK;

  glStencilFunc (GL_ALWAYS, 1, 0x01);
  FC_GLERROR_CHECK;
  // Assuming OpenGL 2.0 support with two side stencil operation. So disable
  // face culling, and use GL_INVERT for stencil op.
  glDisable(GL_CULL_FACE);
  glDisable(GL_LIGHTING);
  glStencilOp (GL_KEEP, GL_KEEP, GL_INVERT);
  FC_GLERROR_CHECK;
  draw_entry.ventry->cache->renderSolids(action->getState());
  ++this->drawcallcount;

  glColorMask(GL_TRUE, GL_TRUE, GL_TRUE, GL_TRUE);
  FC_GLERROR_CHECK;

  glPopAttrib();

  if (!concave) {
    for (int i=0; i<numclip; ++i) {
      if (i != curpass)
        glEnable(GL_CLIP_PLANE0 + i);
      FC_GLERROR_CHECK;
    }
  }
  glDisable(GL_CLIP_PLANE0 + curpass);

  glStencilFunc (GL_EQUAL, 1, 0x01);
  glStencilOp (GL_KEEP, GL_KEEP, GL_KEEP);
  FC_GLERROR_CHECK;

  glPushAttrib(GL_ENABLE_BIT
      | GL_DEPTH_BUFFER_BIT
      | (this->hatchtexture ? 
          (GL_COLOR_BUFFER_BIT|GL_CURRENT_BIT|GL_TEXTURE_BIT): 0));
  FC_GLERROR_CHECK;

  glEnable(GL_DEPTH_TEST);
  glDepthMask(GL_TRUE);
  FC_GLERROR_CHECK;

  const auto &info = this->material.clippers.get(curpass);
  const SoClipPlane *clipper = info.cast<SoClipPlane>();

  SbPlane plane = clipper->plane.getValue();
  if (!info.identity)
    plane.transform(info.matrix);

  SbVec3f dir = plane.getNormal();
  SbRotation rotation(SbVec3f(0,0,1), dir);
  SbVec3f u,v;
  rotation.multVec(SbVec3f(1,0,0), u);
  u *= draw_entry.radius;
  rotation.multVec(SbVec3f(0,1,0), v);
  v *= draw_entry.radius;
  SbVec3f center = draw_entry.bbox.getCenter();
  dir *= -1;
  center += dir * plane.getDistance(center);
  SbVec3f v1,v2,v3,v4;
  v1 = v2 = center + v;
  v1 -= u;
  v2 += u;
  v3 = v4 = center - v;
  v3 += u;
  v4 -= u;
  auto matrix = SoModelMatrixElement::get(action->getState()).inverse();
  matrix.multVecMatrix(v1, v1);
  matrix.multVecMatrix(v2, v2);
  matrix.multVecMatrix(v3, v3);
  matrix.multVecMatrix(v4, v4);

  if (ViewParams::getSectionFillInvert()) {
    auto col = this->material.diffuse;
    unsigned char r = (col >> 24) & 0xff;
    unsigned char g = (col >> 16) & 0xff;
    unsigned char b = (col >> 8) & 0xff;
    unsigned char a = col & 0xff;
    if (r > 120 && r < 140) r = 180; else r = 255 - r;
    if (g > 120 && g < 140) g = 180; else g = 255 - g;
    if (b > 120 && b < 140) b = 180; else b = 255 - b;
    if (r+g+b < 10)
      r = g = b = 50;
    glColor4ub(r, g, b, a);
  }

  float hatchscale = std::max(1e-4f, 0.3f * ViewParams::getSectionHatchTextureScale());

  auto hatch = this->hatchtexture;
  if (!ViewParams::getSectionHatchTextureEnable())
    hatch = nullptr;
  if (hatch) {
    pauseShadowRender(action->getState(), true);
#ifdef FC_OS_WIN32
    static PFNGLACTIVETEXTUREPROC glActiveTexture;
    if (!glActiveTexture) {
      const cc_glglue * glue = cc_glglue_instance(action->getCacheContext());
      glActiveTexture = (PFNGLACTIVETEXTUREPROC)cc_glglue_getprocaddress(glue, "glActiveTexture");
    }
    if(glActiveTexture)
#endif
    glActiveTexture(GL_TEXTURE0);
    glEnable(GL_TEXTURE_2D);
    if (hatch->texture == 0) {
      glGenTextures(1, &hatch->texture);
      glBindTexture(GL_TEXTURE_2D, hatch->texture);
      glTexImage2D(GL_TEXTURE_2D, 0, hatch->nc,
          hatch->width, hatch->height, 0, GL_RGBA, GL_UNSIGNED_BYTE, &hatch->data[0]);
      glTexParameteri (GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
      glTexParameteri (GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
      glTexParameteri (GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
      glTexParameteri (GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    } else
      glBindTexture(GL_TEXTURE_2D, hatch->texture);

    SbViewVolume vv = SoViewVolumeElement::get(action->getState());
    // Using sight point behaves badly in perspective view
    SbVec3f center = vv.getSightPoint(vv.getNearDist() + vv.getDepth() * 0.5f);
    const SbViewportRegion & vp = SoViewportRegionElement::get(action->getState());
    SbVec2s vp_size = vp.getViewportSizePixels();
    float scale = hatchscale * vv.getWorldToScreenScale(center, 1.f);
    // This gives the pixel size of the current world unit size
    float pixelsize = vp_size[0] / scale;
    // This gives the pixel width of the current drawing section plane
    float width = draw_entry.radius * pixelsize;
    // And now we have the texture scale
    hatchscale = std::max(1e-3f, width / hatch->width);
  }

  glDisable(GL_LIGHTING);
  glBegin(GL_QUADS);
  glNormal3fv(dir.getValue());
  if(hatch)
    glTexCoord2f(0.f, hatchscale);
  glVertex3fv(v1.getValue());
  if(hatch)
    glTexCoord2f(0.f, 0.f);
  glVertex3fv(v2.getValue());
  if(hatch)
    glTexCoord2f(hatchscale, 0.f);
  glVertex3fv(v3.getValue());
  if(hatch)
    glTexCoord2f(hatchscale, hatchscale);
  glVertex3fv(v4.getValue());
  glEnd();
  FC_GLERROR_CHECK;

  glPopAttrib();

  if (ViewParams::getSectionFillInvert()) {
    auto col = this->material.diffuse;
    unsigned char r = (col >> 24) & 0xff;
    unsigned char g = (col >> 16) & 0xff;
    unsigned char b = (col >> 8) & 0xff;
    unsigned char a = col & 0xff;
    glColor4ub(r, g, b, a);
  }

  glDisable(GL_STENCIL_TEST);
  FC_GLERROR_CHECK;

  if (!concave) {
    renderSection(action, draw_entry, pass, pushed);
    if (curpass == 0) {
      for (int i=0; i<numclip; ++i) {
        glEnable(GL_CLIP_PLANE0 + i);
        FC_GLERROR_CHECK;
      }
    }
  } else {
    for (int i=0; i<numclip; ++i) {
      if (i == curpass)
        glEnable(GL_CLIP_PLANE0 + i);
      else
        glDisable(GL_CLIP_PLANE0 + i);
      FC_GLERROR_CHECK;
    }
  }
  return true;
}

void
SoFCRendererP::renderOpaque(SoGLRenderAction * action,
                            SbFCVector<DrawEntry> & draw_entries,
                            SbFCVector<std::size_t> & indices,
                            int pass)
{
  if (this->transpshadowmapping)
    return;

  bool pauseshadow = (&draw_entries == &this->slentries || &draw_entries == &this->hlentries);

  SoState * state = action->getState();
  for (std::size_t idx : indices) {
    auto & draw_entry = draw_entries[idx];
    if (draw_entry.skip > 0
        && !this->shadowmapping
        && ((!ViewParams::getSectionConcave() && !ViewParams::getNoSectionOnTop())
            || !draw_entry.material->clippers.getNum()))
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
    setupMatrix(action, draw_entry);

    int array = SoFCVertexCache::ALL;
    if (!this->material.pervertexcolor)
      array ^= SoFCVertexCache::COLOR;
    if (this->notexture)
      array ^= SoFCVertexCache::TEXCOORD;

    if (this->material.lightmodel == SoLazyElement::BASE_COLOR)
      array ^= SoFCVertexCache::NORMAL;
    else if (!draw_entry.ventry->cache->getNormalArray()) {
      array ^= SoFCVertexCache::NORMAL;
      this->material.lightmodel = SoLazyElement::BASE_COLOR;
      glDisable(GL_LIGHTING);
      FC_GLERROR_CHECK;
    }

    int n = 0;
    bool pushed = false;
    while (renderSection(action, draw_entry, n, pushed)) {
      if (!ViewParams::getSectionConcave()
          && this->material.clippers.getNum() > 0
          && SoCullElement::cullTest(state, draw_entry.bbox, FALSE))
      {
          continue;
      }
      switch (draw_entry.material->type) {
      case Material::Triangle:
        if (&draw_entries != &this->slentries
            && &draw_entries != &this->hlentries
            && draw_entry.material->outline
            && ViewParams::getHiddenLineHideFace())
          continue;

        pauseShadowRender(state, pauseshadow
            || !(draw_entry.material->shadowstyle & SoShadowStyleElement::SHADOWED));

        if (!draw_entry.ventry->cache->hasTransparency()) {
          draw_entry.ventry->cache->renderTriangles(state, array, draw_entry.ventry->partidx);
          ++this->drawcallcount;
        }
        else if (!this->material.pervertexcolor) {
          // this means override transparency (i.e. force opaque)
          draw_entry.ventry->cache->renderTriangles(state, SoFCVertexCache::NON_SORTED, draw_entry.ventry->partidx);
          ++this->drawcallcount;
        }
        else {
          if (!this->material.twoside) {
            SoLazyElement::setTwosideLighting(state, TRUE);
            glLightModeli(GL_LIGHT_MODEL_TWO_SIDE, GL_TRUE);
          }
          draw_entry.ventry->cache->renderTriangles(state, array, draw_entry.ventry->partidx);
          ++this->drawcallcount;
          if (!this->material.twoside) {
            SoLazyElement::setTwosideLighting(state, FALSE);
            glLightModeli(GL_LIGHT_MODEL_TWO_SIDE, GL_FALSE);
          }
          FC_GLERROR_CHECK;
        }
        break;
      case Material::Line:
        renderLines(state, array, draw_entry);
        break;
      case Material::Point:
        renderPoints(action, array, draw_entry);
        break;
      }
    }
    if (pushed)
      glPopAttrib();
    renderOutline(action, draw_entry, &draw_entries == &this->hlentries);
  }
}

void
SoFCRendererP::renderTransparency(SoGLRenderAction * action,
                                  SbFCVector<DrawEntry> & draw_entries,
                                  SbFCVector<DrawEntryIndex> & indices,
                                  bool sort)
{
  if (indices.empty())
    return;

  SoState * state = action->getState();

  if (this->shadowmapping) {
    if (!this->transpshadowmapping)
      return;
    sort = false;
  }

  bool pauseshadow = (&draw_entries == &this->slentries || &draw_entries == &this->hlentries);

  bool notriangle = false;
  if (&draw_entries != &this->slentries
      && &draw_entries != &this->hlentries
      && SoFCDisplayModeElement::showHiddenLines(state)
      && ViewParams::getHiddenLineHideFace())
  {
    notriangle = true;
  }

  if (!notriangle && sort) {
    SbPlane plane = SoViewVolumeElement::get(state).getPlane(0.0);
    if (plane.getNormal() != this->prevplane.getNormal()) {
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
  FC_GLERROR_CHECK;

  bool highlight = &draw_entries == &this->hlentries;

  for (auto & v : indices) {
    auto & draw_entry = draw_entries[v.idx];
    if (draw_entry.skip > 0 && !this->shadowmapping)
      continue;
    if (this->recheckmaterial || this->prevmaterial != draw_entry.material) {
      if (!applyMaterial(action, *draw_entry.material, true))
        continue;
      this->recheckmaterial = false;
      this->prevmaterial = draw_entry.material;
    }
    setupMatrix(action, draw_entry);

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
      FC_GLERROR_CHECK;
    }

    switch (draw_entry.material->type) {
    case Material::Line:
      renderLines(state, array, draw_entry);
      break;
    case Material::Point:
      renderPoints(action, array, draw_entry);
      break;
    case Material::Triangle:
      {
        bool pushed = false;
        int n = 0;
        while (renderSection(action, draw_entry, n, pushed)) {
          if (!ViewParams::getSectionConcave()
              && this->material.clippers.getNum() > 0
              && SoCullElement::cullTest(state, draw_entry.bbox, FALSE))
          {
            continue;
          }
          if (!notriangle) {
            if (this->shadowmapping)
                array |= SoFCVertexCache::NON_SORTED_ARRAY;
            else if (!draw_entry.ventry->cache->hasTransparency()
                || draw_entry.material->overrideflags.test(Material::FLAG_TRANSPARENCY))
              array |= SoFCVertexCache::FULL_SORTED_ARRAY;
            else
              array |= SoFCVertexCache::SORTED_ARRAY;

            pauseShadowRender(state, pauseshadow
                || !(draw_entry.material->shadowstyle & SoShadowStyleElement::SHADOWED));
            draw_entry.ventry->cache->renderTriangles(state,
                                                      array,
                                                      draw_entry.ventry->partidx,
                                                      sort ? &this->prevplane : nullptr);
            ++this->drawcallcount;
          }
          renderOutline(action, draw_entry, highlight);
        }
        if (pushed) {
          glPopAttrib();
          FC_GLERROR_CHECK;
        }
      }
      break;
    }

    if (overridelightmodel)
      glEnable(GL_LIGHTING);
    FC_GLERROR_CHECK;
  }

  glDisable(GL_BLEND);
  FC_GLERROR_CHECK;
}

void
SoFCRenderer::render(SoGLRenderAction * action)
{
  SoState * state = action->getState();

  const SoShapeStyleElement * shapestyle = SoShapeStyleElement::get(state);
  unsigned int shapestyleflags = shapestyle->getFlags();

  PRIVATE(this)->shadowrenderpaused = false;
  PRIVATE(this)->shadowrendering = (shapestyleflags & SoShapeStyleElement::SHADOWS) ? true : false;
  PRIVATE(this)->shadowmapping = (shapestyleflags & SoShapeStyleElement::SHADOWMAP) ? true : false;
  PRIVATE(this)->transpshadowmapping = PRIVATE(this)->shadowmapping && (shapestyleflags & 0x01000000);

  if (PRIVATE(this)->shadowmapping
      && !PRIVATE(this)->transpshadowmapping
      && PRIVATE(this)->transpvcache.size()
      && !action->isRenderingDelayedPaths()
      && !action->isRenderingTranspPaths())
  {
    // signal SoShadowGroup to render transparent shadow
    action->handleTransparency(TRUE);
  }

  PRIVATE(this)->updateSelection();

  PRIVATE(this)->depthwriteonly = false;
  PRIVATE(this)->notexture = false;
  PRIVATE(this)->prevmaterial = nullptr;
  PRIVATE(this)->recheckmaterial = false;
  PRIVATE(this)->material.init();

  glPushAttrib(GL_ALL_ATTRIB_BITS);
  state->push();

  SoLazyElement::setToDefault(state);
  glColorMaterial(GL_FRONT_AND_BACK, GL_DIFFUSE);
  glEnable(GL_COLOR_MATERIAL);

  PRIVATE(this)->matrix = SoModelMatrixElement::get(state);
  PRIVATE(this)->identity = (PRIVATE(this)->matrix == SbMatrix::identity());

  if (!action->isRenderingDelayedPaths()) {
    PRIVATE(this)->drawcallcount = 0;

    PRIVATE(this)->renderOpaque(action,
                                PRIVATE(this)->drawentries,
                                PRIVATE(this)->opaquevcache);

    PRIVATE(this)->recheckmaterial = true;
    // PRIVATE(this)->notexture = true;

    PRIVATE(this)->renderOpaque(action,
                                PRIVATE(this)->slentries,
                                PRIVATE(this)->opaqueselections,
                                RenderPassHighlight);

    PRIVATE(this)->recheckmaterial = true;
    PRIVATE(this)->notexture = false;

    if (!PRIVATE(this)->shadowmapping) {
      action->addDelayedPath(action->getCurPath()->copy());
      state->pop();
      glPopAttrib();
      FC_GLERROR_CHECK;
      SoGLLazyElement::getInstance(state)->reset(state,
                                                SoLazyElement::LIGHT_MODEL_MASK|
                                                SoLazyElement::TWOSIDE_MASK|
                                                SoLazyElement::SHADE_MODEL_MASK);
      return;
    }
  }

  PRIVATE(this)->renderTransparency(action,
                                    PRIVATE(this)->drawentries,
                                    PRIVATE(this)->transpvcache);

  PRIVATE(this)->recheckmaterial = true;

  PRIVATE(this)->renderTransparency(action,
                                    PRIVATE(this)->slentries,
                                    PRIVATE(this)->transpselections);

  PRIVATE(this)->renderOpaque(action,
                              PRIVATE(this)->drawentries,
                              PRIVATE(this)->opaqueontop);

  PRIVATE(this)->renderTransparency(action,
                                    PRIVATE(this)->drawentries,
                                    PRIVATE(this)->transpontop,
                                    false);

  if (PRIVATE(this)->shadowmapping) {
    state->pop();
    glPopAttrib();
    FC_GLERROR_CHECK;
    SoGLLazyElement::getInstance(state)->reset(state,
                                              SoLazyElement::LIGHT_MODEL_MASK|
                                              SoLazyElement::TWOSIDE_MASK|
                                              SoLazyElement::SHADE_MODEL_MASK);
    return;
  }
  
  PRIVATE(this)->recheckmaterial = true;
  // PRIVATE(this)->notexture = true;

  PRIVATE(this)->renderTransparency(action,
                                    PRIVATE(this)->slentries,
                                    PRIVATE(this)->transpselectionsontop,
                                    false);

  if (PRIVATE(this)->hlwholeontop) {
      PRIVATE(this)->renderOpaque(action,
                                  PRIVATE(this)->hlentries,
                                  PRIVATE(this)->opaquehighlight);
      PRIVATE(this)->renderTransparency(action,
                                        PRIVATE(this)->hlentries,
                                        PRIVATE(this)->transphighlight,
                                        false);
  }

  bool hassel = PRIVATE(this)->selsontop.size()
                        || PRIVATE(this)->selslineontop.size();
  bool hasontop = PRIVATE(this)->trianglesontop.size()
                      && PRIVATE(this)->linesontop.size();
  int pass = RenderPassNormal;

  if (hassel || hasontop || PRIVATE(this)->hlwholeontop) {
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

    if (PRIVATE(this)->hlwholeontop) {
        PRIVATE(this)->renderOpaque(action,
                                    PRIVATE(this)->hlentries,
                                    PRIVATE(this)->opaquehighlight,
                                    RenderPassHighlight);
        PRIVATE(this)->renderTransparency(action,
                                          PRIVATE(this)->hlentries,
                                          PRIVATE(this)->transphighlight,
                                          false);
    }

    PRIVATE(this)->depthwriteonly = false;
    glColorMask(GL_TRUE, GL_TRUE, GL_TRUE, GL_TRUE);

    pass = RenderPassLinePattern;
  }

  // Even if we are calling renderOpaque() below (because of lines and points
  // render), we shall still respect the transparency setting, e.g. we'll use
  // transparency to dim the hidden lines. So we enable blending here.
  glEnable(GL_BLEND);
  glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
  FC_GLERROR_CHECK;

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

  if (PRIVATE(this)->hlwholeontop)
    PRIVATE(this)->renderOpaque(action,
                                PRIVATE(this)->hlentries,
                                PRIVATE(this)->opaquelineshighlight,
                                pass);

  if (hassel || hasontop || PRIVATE(this)->hlwholeontop) {
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

    if (PRIVATE(this)->hlwholeontop) {
      PRIVATE(this)->renderOpaque(action,
                                  PRIVATE(this)->hlentries,
                                  PRIVATE(this)->opaquelineshighlight,
                                  pass | RenderPassHighlight);
    }
  }

  glDisable(GL_BLEND);
  FC_GLERROR_CHECK;

  if (!PRIVATE(this)->hlwholeontop) {
    PRIVATE(this)->renderOpaque(action,
                                PRIVATE(this)->hlentries,
                                PRIVATE(this)->opaquehighlight);

    PRIVATE(this)->renderTransparency(action,
                                      PRIVATE(this)->hlentries,
                                      PRIVATE(this)->transphighlight,
                                      false);

    PRIVATE(this)->renderOpaque(action,
                                PRIVATE(this)->hlentries,
                                PRIVATE(this)->opaquelineshighlight,
                                RenderPassHighlight);
  }

  PRIVATE(this)->renderOpaque(action,
                              PRIVATE(this)->slentries,
                              PRIVATE(this)->selspointontop,
                              RenderPassHighlight);

  state->pop();
  glPopAttrib();
  FC_GLERROR_CHECK;
  SoGLLazyElement::getInstance(state)->reset(state,
                                             SoLazyElement::LIGHT_MODEL_MASK|
                                             SoLazyElement::TWOSIDE_MASK|
                                             SoLazyElement::SHADE_MODEL_MASK);
}

const char *
SoFCRenderer::getStatistics() const
{
  snprintf(PRIVATE(this)->stats, sizeof(PRIVATE(this)->stats)-1,
      "draw calls: %d", PRIVATE(this)->drawcallcount);
  return PRIVATE(this)->stats;
}

// vim: noai:ts=2:sw=2
