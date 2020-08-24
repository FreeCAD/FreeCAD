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
#include <Inventor/nodes/SoLight.h>
#include <Inventor/actions/SoCallbackAction.h>
#include <Inventor/details/SoFaceDetail.h>
#include <Inventor/details/SoLineDetail.h>
#include <Inventor/details/SoPointDetail.h>

#include "../InventorBase.h"
#include "SoFCRenderCache.h"
#include "SoFCVertexCache.h"
#include "SoFCDetail.h"
#include "SoFCDiffuseElement.h"
#include "SoFCDisplayModeElement.h"

using namespace Gui;

typedef CoinPtr<SoFCVertexCache> VertexCachePtr;
typedef CoinPtr<SoFCRenderCache> RenderCachePtr;
typedef SoFCRenderCache::Material Material;
typedef SoFCRenderCache::VertexCacheEntry VertexCacheEntry;

struct CacheEntry {
  RenderCachePtr cache;
  VertexCachePtr vcache;
  Material material;
  SbMatrix matrix;
  bool resetmatrix;
  bool identity;

  CacheEntry(const SbMatrix & m,
             bool iden, bool reset,
             SoFCRenderCache * c,
             SoFCVertexCache *vc)
    :cache(c), vcache(vc), resetmatrix(reset), identity(iden)
  {
    if (!identity) this->matrix = m;
  }
};

class SoFCRenderCacheP {
public:
  SoFCRenderCacheP()
  {
  }

  ~SoFCRenderCacheP()
  {
  }

  void captureMaterial(SoState * state);

  Material mergeMaterial(const SbMatrix &matrix,
                         bool identity,
                         const Material &parent,
                         const Material &child);

  void finalizeMaterial(Material & material);

  void applyMaterial(SoGLRenderAction * action,
                     Material & material,
                     const Material & next,
                     bool first, bool transp=false);

  void addChildCache(SoState *state,
                     SoFCRenderCache * cache,
                     SoFCVertexCache *vcache,
                     bool opencache);

  const SoElement * shapehintselement;
  const SoElement * shadowstyleelement;
  const SoElement * linepatternelement;
  const SoElement * linewidthelement;
  const SoElement * pointsizeelement;
  const SoElement * polygonoffsetelement;
  const SoElement * drawstyleelement;
  const SoElement * materialbindingelement;

  SoFCRenderCache::VertexCacheMap vcachemap;

  std::vector<CacheEntry> caches;
  SbFCUniqueId nodeid;
  intptr_t nodeptr;

  Material material;
  uint32_t facecolor;
  uint32_t nonfacecolor;
  bool resetmatrix;
};

template<class T>
static inline const T * constElement(SoState * state)
{
  // calling SoState::getConstElement() instead of SoElement::getConstElement()
  // to avoid cache dependency
  return static_cast<const T *>(state->getConstElement(T::getClassStackIndex()));
}

#define PRIVATE(obj) ((obj)->pimpl)

SoFCRenderCache::SoFCRenderCache(SoState *state, const SoNode * node)
  : SoCache(state), pimpl(new SoFCRenderCacheP)
{
  // Do not try to access node through nodeptr, it is not ref'd
  PRIVATE(this)->nodeptr = reinterpret_cast<intptr_t>(node);
  PRIVATE(this)->nodeid = node->getNodeId();
}

SoFCRenderCache::~SoFCRenderCache()
{
  delete pimpl;
}

static const SbMatrix matrixidentity(SbMatrix::identity());

void SoFCRenderCache::initClass()
{
  SO_ENABLE(SoCallbackAction, SoShadowStyleElement);
  SoFCDiffuseElement::initClass();
}

void SoFCRenderCache::cleanup()
{
  SoFCDiffuseElement::cleanup();
}

bool
SoFCRenderCache::Material::operator<(const Material &other) const
{
#define MAT_COMPARE(_v) do {\
    if (this->_v < other._v) return true;\
    if (this->_v > other._v) return false;\
  }while(0)

  MAT_COMPARE(order);
  MAT_COMPARE(type);
  MAT_COMPARE(depthtest);
  MAT_COMPARE(depthfunc);
  MAT_COMPARE(depthwrite);
  if (this->type == Triangle) {
    MAT_COMPARE(lights);
    MAT_COMPARE(textures);
    MAT_COMPARE(shadowstyle);
    MAT_COMPARE(diffuse);
    MAT_COMPARE(pervertexcolor);
    MAT_COMPARE(ambient);
    MAT_COMPARE(emissive);
    MAT_COMPARE(specular);
    MAT_COMPARE(shininess);
    MAT_COMPARE(lightmodel);
    MAT_COMPARE(vertexordering);
    MAT_COMPARE(culling);
    MAT_COMPARE(twoside);
    MAT_COMPARE(drawstyle);
    MAT_COMPARE(polygonoffsetstyle);
    MAT_COMPARE(polygonoffsetfactor);
    MAT_COMPARE(polygonoffsetunits);

    // no need to differentiate texture matrices. Its only used for merging to
    // upper hierarchy
    // MAT_COMPARE(texturematrices);
  } else {
    MAT_COMPARE(diffuse);
    MAT_COMPARE(pervertexcolor);
    if (this->type == Line)
      MAT_COMPARE(linewidth);
    else
      MAT_COMPARE(pointsize);
  }
  return false;
}

static inline uint32_t
getOverrideFlags(SoState * state)
{
  uint32_t res = 0;
  uint32_t flags = SoOverrideElement::getFlags(state);
  if (flags & SoOverrideElement::AMBIENT_COLOR) res |= Material::FLAG_AMBIENT;
  if (flags & SoOverrideElement::DIFFUSE_COLOR) res |= Material::FLAG_DIFFUSE;
  if (flags & SoOverrideElement::DRAW_STYLE) res |= Material::FLAG_DRAW_STYLE;
  if (flags & SoOverrideElement::EMISSIVE_COLOR) res |= Material::FLAG_EMISSIVE;
  if (flags & SoOverrideElement::LIGHT_MODEL) res |= Material::FLAG_LIGHT_MODEL;
  if (flags & SoOverrideElement::LINE_PATTERN) res |= Material::FLAG_LINE_PATTERN;
  if (flags & SoOverrideElement::LINE_WIDTH) res |= Material::FLAG_LINE_WIDTH;
  if (flags & SoOverrideElement::MATERIAL_BINDING) res |= Material::FLAG_MATERIAL_BINDING;
  if (flags & SoOverrideElement::POINT_SIZE) res |= Material::FLAG_POINT_SIZE;
  if (flags & SoOverrideElement::SHAPE_HINTS) res |= Material::FLAG_SHAPE_HINTS;
  if (flags & SoOverrideElement::SHININESS) res |= Material::FLAG_SHININESS;
  if (flags & SoOverrideElement::SPECULAR_COLOR) res |= Material::FLAG_SPECULAR;
  if (flags & SoOverrideElement::POLYGON_OFFSET) res |= Material::FLAG_POLYGON_OFFSET;
  if (flags & SoOverrideElement::TRANSPARENCY) res |= Material::FLAG_TRANSPARENCY;
  return res;
}

void
SoFCRenderCache::Material::init(SoState * state)
{
  this->depthtest = 1;
  this->depthfunc = SoDepthBuffer::LEQUAL;
  this->depthwrite = 1;
  this->order = 0;
  this->overrideflags = 0;
  this->maskflags = 0;
  this->diffuse = 0;
  this->ambient = 0;
  this->emissive = 0;
  this->specular = 0;
  this->linewidth = 1;
  this->pointsize = 1;
  this->shininess = 0.f;
  this->polygonoffsetstyle = 0;
  this->polygonoffsetunits = 0.f;
  this->polygonoffsetfactor = 0.f;
  this->linepattern = 0xffff;
  this->type = 0;
  this->materialbinding = 0;
  this->pervertexcolor = false;
  this->transptexture = false;
  this->lightmodel = SoLazyElement::PHONG;
  this->vertexordering = SoLazyElement::CW;
  this->culling = false;
  this->twoside = false;
  this->drawstyle = 0;
  this->shadowstyle = SoShadowStyleElement::CASTS_SHADOW_AND_SHADOWED; 
  this->texturematrices.clear();
  this->textures.clear();
  this->lights.clear();

  if (!state)
    return;

  this->overrideflags = getOverrideFlags(state);

  float t;
  if (this->overrideflags & (Material::FLAG_DIFFUSE | Material::FLAG_EMISSIVE)) {
    t = SoLazyElement::getTransparency(state, 0);
    this->diffuse = SoLazyElement::getDiffuse(state, 0).getPackedValue(t);
  }

  t = 0.0f;
  if (this->overrideflags & Material::FLAG_EMISSIVE)
    this->emissive = SoLazyElement::getEmissive(state).getPackedValue(t);

  if (this->overrideflags & Material::FLAG_AMBIENT)
    this->ambient = SoLazyElement::getAmbient(state).getPackedValue(t);

  if (this->overrideflags & Material::FLAG_SPECULAR)
    this->specular = SoLazyElement::getSpecular(state).getPackedValue(t);

  if (this->overrideflags & Material::FLAG_SHININESS)
    this->shininess = SoLazyElement::getShininess(state);

  if (this->overrideflags & Material::FLAG_LIGHT_MODEL)
    this->lightmodel = SoLazyElement::getLightModel(state);

  if (this->overrideflags & Material::FLAG_SHAPE_HINTS) {
    SoShapeHintsElement::VertexOrdering ordering;
    SoShapeHintsElement::ShapeType shapetype;
    SoShapeHintsElement::FaceType facetype;
    SoShapeHintsElement::get(state, ordering, shapetype, facetype);
    this->vertexordering = ordering == SoShapeHintsElement::CLOCKWISE ?
                                            SoLazyElement::CW : SoLazyElement::CCW;
    this->twoside = ordering != SoShapeHintsElement::UNKNOWN_ORDERING
                       && shapetype == SoShapeHintsElement::UNKNOWN_SHAPE_TYPE;
    this->culling = ordering != SoShapeHintsElement::UNKNOWN_ORDERING
                       && shapetype == SoShapeHintsElement::SOLID;
    this->twoside = SoLazyElement::getTwoSidedLighting(state);
  }

  if (this->overrideflags & Material::FLAG_MATERIAL_BINDING)
    this->materialbinding = SoMaterialBindingElement::get(state);

  if (this->overrideflags & Material::FLAG_LINE_PATTERN)
    this->linepattern = SoLinePatternElement::get(state);

  if (this->overrideflags & Material::FLAG_LINE_WIDTH)
    this->linewidth = SoLineWidthElement::get(state);

  if (this->overrideflags & Material::FLAG_POINT_SIZE)
    this->pointsize = SoPointSizeElement::get(state);

  if (this->overrideflags & Material::FLAG_POLYGON_OFFSET) {
    SbBool on;
    SoPolygonOffsetElement::Style style;
    SoPolygonOffsetElement::get(state,
                                this->polygonoffsetfactor,
                                this->polygonoffsetunits,
                                style,
                                on);
    if (!on)
      this->polygonoffsetstyle = 0;
    else
      this->polygonoffsetstyle = style;
  }

  if (this->overrideflags & Material::FLAG_DRAW_STYLE)
    this->drawstyle = SoDrawStyleElement::get(state);

  if (this->overrideflags & Material::FLAG_SHAPE_HINTS) {
    SoShapeHintsElement::VertexOrdering ordering;
    SoShapeHintsElement::ShapeType shapetype;
    SoShapeHintsElement::FaceType facetype;
    SoShapeHintsElement::get(state, ordering, shapetype, facetype);
    this->vertexordering = ordering == SoShapeHintsElement::CLOCKWISE ?
                                            SoLazyElement::CW : SoLazyElement::CCW;
    this->twoside = ordering != SoShapeHintsElement::UNKNOWN_ORDERING
                       && shapetype == SoShapeHintsElement::UNKNOWN_SHAPE_TYPE;
    this->culling = ordering != SoShapeHintsElement::UNKNOWN_ORDERING
                       && shapetype == SoShapeHintsElement::SOLID;
  }
}

void
SoFCRenderCacheP::captureMaterial(SoState * state)
{
  Material & m = this->material;
  m.overrideflags = getOverrideFlags(state);

  if (this->materialbindingelement != constElement<SoMaterialBindingElement>(state)) {
    m.maskflags |= Material::FLAG_MATERIAL_BINDING;
    m.materialbinding = SoMaterialBindingElement::get(state);
  }

  if (this->linepatternelement != constElement<SoLinePatternElement>(state)) {
    m.maskflags |= Material::FLAG_LINE_PATTERN;
    m.linepattern = SoLinePatternElement::get(state);
  }
  if (this->linewidthelement != constElement<SoLineWidthElement>(state)) {
    m.maskflags |= Material::FLAG_LINE_WIDTH;
    m.linewidth = SoLineWidthElement::get(state);
  }
  if (this->pointsizeelement != constElement<SoPointSizeElement>(state)) {
    m.maskflags |= Material::FLAG_POINT_SIZE;
    m.pointsize = SoPointSizeElement::get(state);
  }
  if (this->polygonoffsetelement != constElement<SoPolygonOffsetElement>(state)) {
    m.maskflags |= Material::FLAG_POLYGON_OFFSET;
    SbBool on;
    SoPolygonOffsetElement::Style style;
    SoPolygonOffsetElement::get(state,
                                m.polygonoffsetfactor,
                                m.polygonoffsetunits,
                                style,
                                on);
    if (!on)
      m.polygonoffsetstyle = 0;
    else
      m.polygonoffsetstyle = style;
  }
  if (this->drawstyleelement != constElement<SoDrawStyleElement>(state)) {
    m.maskflags |= Material::FLAG_DRAW_STYLE;
    m.drawstyle = SoDrawStyleElement::get(state);
  }
  if (this->shadowstyleelement != constElement<SoShadowStyleElement>(state)) {
    m.maskflags |= Material::FLAG_SHADOW_STYLE;
    m.shadowstyle = SoShadowStyleElement::get(state);
  }
  if (this->shapehintselement != constElement<SoShapeHintsElement>(state)) {
    m.maskflags |= Material::FLAG_SHAPE_HINTS
                    | Material::FLAG_CULLING
                    | Material::FLAG_VERTEXORDERING
                    | Material::FLAG_TWOSIDE;
    SoShapeHintsElement::VertexOrdering ordering;
    SoShapeHintsElement::ShapeType shapetype;
    SoShapeHintsElement::FaceType facetype;
    SoShapeHintsElement::get(state, ordering, shapetype, facetype);
    m.vertexordering = ordering == SoShapeHintsElement::CLOCKWISE ?
                                            SoLazyElement::CW : SoLazyElement::CCW;
    m.twoside = ordering != SoShapeHintsElement::UNKNOWN_ORDERING
                       && shapetype == SoShapeHintsElement::UNKNOWN_SHAPE_TYPE;
    m.culling = ordering != SoShapeHintsElement::UNKNOWN_ORDERING
                       && shapetype == SoShapeHintsElement::SOLID;
  }
}

void
SoFCRenderCache::setLightModel(SoState * state, const SoLightModel * lightmode)
{
  if (PRIVATE(this)->material.overrideflags & Material::FLAG_LIGHT_MODEL)
    return;

  if (lightmode->isOverride())
    PRIVATE(this)->material.overrideflags |= Material::FLAG_LIGHT_MODEL;
  PRIVATE(this)->material.maskflags |= Material::FLAG_LIGHT_MODEL;
  PRIVATE(this)->material.lightmodel = SoLightModelElement::get(state);
}

template<class N, class T>
static inline bool
testMaterial(SoFCRenderCache::Material &m, N node, T field, int flag, int mask)
{
  if (!(node->*field).isIgnored() && (node->*field).getNum() && !(m.overrideflags & flag)) {
    if (node->isOverride())
      m.overrideflags |= flag;
    m.maskflags |= mask;
    return true;
  }
  return false;
}

void
SoFCRenderCache::setMaterial(SoState * state, const SoMaterial * material)
{
  (void)state;
  Material & m = PRIVATE(this)->material;
  float t = 0.f;
  if (testMaterial(m, material, &SoMaterial::diffuseColor, Material::FLAG_DIFFUSE, Material::FLAG_DIFFUSE)) {
    m.diffuse &= 0xff;
    m.diffuse |= material->diffuseColor[0].getPackedValue(t) & 0xffffff00;
    SbFCUniqueId id = material->diffuseColor.getNum() > 1 ? material->getNodeId() : 0;
    SoFCDiffuseElement::set(state, &id, NULL);
  }
  if (testMaterial(m, material, &SoMaterial::transparency, Material::FLAG_TRANSPARENCY, Material::FLAG_TRANSPARENCY)) {
    m.diffuse &= 0xffffff00;
    float alpha = SbClamp(1.0f-material->transparency[0], 0.f, 1.f);
    m.diffuse |= (uint8_t)(alpha * 255);
    SbFCUniqueId id = material->transparency.getNum() > 1 ? material->getNodeId() : 0;
    SoFCDiffuseElement::set(state, NULL, &id);
  }
  if (testMaterial(m, material, &SoMaterial::ambientColor, Material::FLAG_AMBIENT, Material::FLAG_AMBIENT))
    m.ambient = material->ambientColor[0].getPackedValue(t);
  if (testMaterial(m, material, &SoMaterial::emissiveColor, Material::FLAG_EMISSIVE, Material::FLAG_EMISSIVE))
    m.emissive = material->emissiveColor[0].getPackedValue(t);
  if (testMaterial(m, material, &SoMaterial::specularColor, Material::FLAG_SPECULAR, Material::FLAG_SPECULAR))
    m.specular = material->specularColor[0].getPackedValue(t);
  if (testMaterial(m, material, &SoMaterial::shininess, Material::FLAG_SHININESS, Material::FLAG_SHININESS))
    m.shininess = material->shininess[0];
}

void
SoFCRenderCache::setDepthBuffer(SoState * state, const SoDepthBuffer * node)
{
  (void)state;
  Material & m = PRIVATE(this)->material;
  if (!node->test.isIgnored()) {
    m.maskflags |= Material::FLAG_DEPTH_TEST;
    m.depthtest = node->test.getValue() ? 1 : 0;
  }
  if (!node->write.isIgnored()) {
    m.maskflags |= Material::FLAG_DEPTH_WRITE;
    m.depthwrite = node->write.getValue() ? 1 : 0;
  }
  if (!node->function.isIgnored()) {
    m.maskflags |= Material::FLAG_DEPTH_FUNC;
    m.depthfunc = node->function.getValue() ? 1 : 0;
  }
}

static inline bool
canSetMaterial(SoFCRenderCache::Material & res,
               const SoFCRenderCache::Material & parent,
               uint32_t flag, uint32_t mask)
{
  return ((parent.overrideflags & flag)
      || (!(res.maskflags & mask) && (parent.maskflags & mask)));
}

template<class T>
static inline void
copyMaterial(SoFCRenderCache::Material &res,
             const SoFCRenderCache::Material &parent,
             T member,
             uint32_t flag, uint32_t mask)
{
  if (canSetMaterial(res, parent, flag, mask)) {
    res.*member = parent.*member;
    res.maskflags |= mask;
  }
}

void
SoFCRenderCache::increaseRenderingOrder()
{
  ++PRIVATE(this)->material.order;
}

void
SoFCRenderCache::decreaseRenderingOrder()
{
  --PRIVATE(this)->material.order;
}

SoFCRenderCache::Material
SoFCRenderCacheP::mergeMaterial(const SbMatrix &matrix,
                                bool identity,
                                const Material &parent,
                                const Material &child)
{
  // merge material from bottom up

  Material res = child;

  if (parent.order > child.order)
    res.order = parent.order;
  
  copyMaterial(res, parent, &Material::depthtest, 0, Material::FLAG_DEPTH_TEST);
  copyMaterial(res, parent, &Material::depthfunc, 0, Material::FLAG_DEPTH_FUNC);
  copyMaterial(res, parent, &Material::depthwrite, 0, Material::FLAG_DEPTH_WRITE);

  if (canSetMaterial(res, parent, Material::FLAG_DIFFUSE, Material::FLAG_DIFFUSE)) {
    res.diffuse &= 0xff;
    res.diffuse |= parent.diffuse & 0xffffff00;
    res.maskflags |= Material::FLAG_DIFFUSE;
  }

  if (canSetMaterial(res, parent, Material::FLAG_TRANSPARENCY, Material::FLAG_TRANSPARENCY)) {
    res.diffuse &= 0xffffff00;
    res.diffuse |= parent.diffuse & 0xff;
    res.maskflags |= Material::FLAG_TRANSPARENCY;
  }

  if (res.type == Material::Line) {
    copyMaterial(res, parent, &Material::linewidth, Material::FLAG_LINE_WIDTH, Material::FLAG_LINE_WIDTH);
    copyMaterial(res, parent, &Material::linepattern, Material::FLAG_LINE_PATTERN, Material::FLAG_LINE_PATTERN);
    return res;
  }

  if (res.type == Material::Point) {
    copyMaterial(res, parent, &Material::pointsize, Material::FLAG_POINT_SIZE, Material::FLAG_POINT_SIZE);
    return res;
  }

  copyMaterial(res, parent, &Material::materialbinding, Material::FLAG_MATERIAL_BINDING, Material::FLAG_MATERIAL_BINDING);

  copyMaterial(res, parent, &Material::ambient, Material::FLAG_AMBIENT, Material::FLAG_AMBIENT);
  copyMaterial(res, parent, &Material::emissive, Material::FLAG_EMISSIVE, Material::FLAG_EMISSIVE);
  copyMaterial(res, parent, &Material::specular, Material::FLAG_SPECULAR, Material::FLAG_SPECULAR);
  copyMaterial(res, parent, &Material::shininess, Material::FLAG_SHININESS, Material::FLAG_SHININESS);
  copyMaterial(res, parent, &Material::drawstyle, Material::FLAG_DRAW_STYLE, Material::FLAG_DRAW_STYLE);
  copyMaterial(res, parent, &Material::lightmodel, Material::FLAG_LIGHT_MODEL, Material::FLAG_LIGHT_MODEL);
  copyMaterial(res, parent, &Material::shadowstyle, 0, Material::FLAG_SHADOW_STYLE);

  if (canSetMaterial(res, parent, Material::FLAG_POLYGON_OFFSET, Material::FLAG_POLYGON_OFFSET)) {
    res.polygonoffsetstyle = parent.polygonoffsetstyle;
    res.polygonoffsetfactor = parent.polygonoffsetfactor;
    res.polygonoffsetunits = parent.polygonoffsetunits;
    res.maskflags |= Material::FLAG_POLYGON_OFFSET;
  }

  if (canSetMaterial(res, parent, Material::FLAG_SHAPE_HINTS, Material::FLAG_SHAPE_HINTS)) {
    res.culling = parent.culling;
    res.vertexordering = parent.vertexordering;
    res.twoside = parent.twoside;
    res.maskflags |= Material::FLAG_SHAPE_HINTS
                    | Material::FLAG_CULLING
                    | Material::FLAG_VERTEXORDERING
                    | Material::FLAG_TWOSIDE;
  }
  else {
    copyMaterial(res, parent, &Material::culling, 0, Material::FLAG_CULLING);
    copyMaterial(res, parent, &Material::vertexordering, 0, Material::FLAG_VERTEXORDERING);
    copyMaterial(res, parent, &Material::twoside, 0, Material::FLAG_TWOSIDE);
  }

  res.texturematrices.combine(parent.texturematrices);

  if (parent.texturematrices.getNum()) {
    for (auto & v : parent.texturematrices.getData()) {
      const SoFCRenderCache::TextureInfo * pinfo = res.textures.get(v.first);
      if (!pinfo) continue;
      SoFCRenderCache::TextureInfo info = * pinfo;
      if (info.identity)
        info.matrix = v.second.matrix;
      else
        info.matrix.multLeft(v.second.matrix);
      info.identity = false;
      res.textures.set(v.first, info);
    }
  }
  res.textures.add(parent.textures, false);

  res.lights = parent.lights;
  if (identity)
    res.lights.append(child.lights);
  else if (child.lights.getNum()) {
    for (const auto & info : child.lights.getData()) {
      if (info.resetmatrix)
        res.lights.append(info);
      else {
        SoFCRenderCache::LightInfo copy = info;
        if (copy.identity)
          copy.matrix = matrix;
        else
          copy.matrix.multLeft(matrix);
        copy.identity = false;
        res.lights.append(copy);
      }
    }
  }

  if (!res.transptexture && res.textures.getNum()) {
    for (auto & info : res.textures.getData()) {
      if (info.second.transparent) {
        res.transptexture = true;
        break;
      }
    }
  }

  return res;
}

SbFCUniqueId
SoFCRenderCache::getNodeId() const
{
  return PRIVATE(this)->nodeid;
}

SbBool
SoFCRenderCache::isValid(const SoState * state) const
{
  return inherited::isValid(state);
}

void
SoFCRenderCache::open(SoState *state, bool initmaterial)
{
  SoCacheElement::set(state, this);

  PRIVATE(this)->facecolor = 0;
  PRIVATE(this)->nonfacecolor = 0;
  PRIVATE(this)->material.init(initmaterial ? state : nullptr);

  if (initmaterial && SoFCDisplayModeElement::showHiddenLines(state)) {
    float t = SoFCDisplayModeElement::getTransparency(state);
    uint8_t alpha = static_cast<uint8_t>(std::min(std::max(1.f-t, 1.f), 0.f) * 255.f);
    PRIVATE(this)->material.diffuse = (PRIVATE(this)->material.diffuse & 0xffffff00) | alpha;
    PRIVATE(this)->material.overrideflags |= Material::FLAG_TRANSPARENCY;

    const SbColor * color = SoFCDisplayModeElement::getFaceColor(state);
    if (color)
      PRIVATE(this)->facecolor = color->getPackedValue(t);
    color = SoFCDisplayModeElement::getLineColor(state);
    if (color) {
      t = 1.0f;
      PRIVATE(this)->nonfacecolor = color->getPackedValue(t);
    }
  }

  // Call SoState::getElement() here to force create SoOverrideElement at the
  // current stack level to prevent it being captured in cache, because we want
  // to decouple override settings from child caches.
  state->getElement(SoOverrideElement::getClassStackIndex());
  // Remember the current override flags. If it weren't for the above call to
  // getElement(), this element will be added to our cache dependency.
  uint32_t flags = SoOverrideElement::getFlags(state);
  PRIVATE(this)->material.overrideflags = flags;

  // convert override flags to our own mask, and then reset the flags
  if (flags & Material::FLAG_AMBIENT)
    SoOverrideElement::setAmbientColorOverride(state, NULL, FALSE);
  if (flags & Material::FLAG_DIFFUSE)
    SoOverrideElement::setDiffuseColorOverride(state, NULL, FALSE);
  if (flags & Material::FLAG_SPECULAR)
    SoOverrideElement::setSpecularColorOverride(state, NULL, FALSE);
  if (flags & Material::FLAG_EMISSIVE)
    SoOverrideElement::setEmissiveColorOverride(state, NULL, FALSE);
  if (flags & Material::FLAG_SHININESS)
    SoOverrideElement::setShininessOverride(state, NULL, FALSE);
  if (flags & Material::FLAG_TRANSPARENCY)
    SoOverrideElement::setTransparencyOverride(state, NULL, FALSE);
  if (flags & Material::FLAG_DRAW_STYLE)
    SoOverrideElement::setDrawStyleOverride(state, NULL, FALSE);
  if (flags & Material::FLAG_LINE_PATTERN)
    SoOverrideElement::setLinePatternOverride(state, NULL, FALSE);
  if (flags & Material::FLAG_LINE_WIDTH)
    SoOverrideElement::setLineWidthOverride(state, NULL, FALSE);
  if (flags & Material::FLAG_POINT_SIZE)
    SoOverrideElement::setPointSizeOverride(state, NULL, FALSE);
  if (flags & Material::FLAG_MATERIAL_BINDING)
    SoOverrideElement::setMaterialBindingOverride(state, NULL, FALSE);
  if (flags & Material::FLAG_POLYGON_OFFSET)
    SoOverrideElement::setPolygonOffsetOverride(state, NULL, FALSE);
  if (flags & Material::FLAG_SHAPE_HINTS)
    SoOverrideElement::setShapeHintsOverride(state, NULL, FALSE);
  if (flags & Material::FLAG_LIGHT_MODEL)
    SoOverrideElement::setLightModelOverride(state, NULL, FALSE);

  // Capture current relavant elements to detect change happen inside the
  // currnet caching group node. When capturing materials, we only capture
  // elements set within the current node.
  PRIVATE(this)->linepatternelement = constElement<SoLinePatternElement>(state);
  PRIVATE(this)->linewidthelement = constElement<SoLineWidthElement>(state);
  PRIVATE(this)->pointsizeelement = constElement<SoPointSizeElement>(state);
  PRIVATE(this)->polygonoffsetelement = constElement<SoPolygonOffsetElement>(state);
  PRIVATE(this)->drawstyleelement = constElement<SoDrawStyleElement>(state);
  PRIVATE(this)->materialbindingelement = constElement<SoMaterialBindingElement>(state);
  PRIVATE(this)->shadowstyleelement = constElement<SoShadowStyleElement>(state);
  PRIVATE(this)->shapehintselement = constElement<SoShapeHintsElement>(state);

  PRIVATE(this)->resetmatrix = false;
}

void
SoFCRenderCache::close(SoState *state)
{
  (void)state;
  PRIVATE(this)->material.init();
}

class MyMultiTextureMatrixElement : public SoMultiTextureMatrixElement
{
public:
  int getNumUnits() const {
    return SoMultiTextureMatrixElement::getNumUnits();
  }

  const UnitData & getUnitData(const int unit) const {
    return SoMultiTextureMatrixElement::getUnitData(unit);
  }
};


void
SoFCRenderCacheP::addChildCache(SoState * state,
                                SoFCRenderCache * cache,
                                SoFCVertexCache * vcache,
                                bool opencache)
{
  auto elem = constElement<SoModelMatrixElement>(state);
  SbMatrix matrix = elem->getModelMatrix();
  bool identity = (matrix == matrixidentity);

  if (opencache) {
    if (!identity) {
      // reset to identity matrix to decouple model transformation from child cache
      static_cast<SoModelMatrixElement*>(
          state->getElement(SoModelMatrixElement::getClassStackIndex()))->init(state);
    }

    // reset textrue matrix to identity
    auto elem = constElement<MyMultiTextureMatrixElement>(state);
    for (int i=0, n=elem->getNumUnits(); i<n; ++i) {
      if (elem->getUnitData(i).textureMatrix != matrixidentity)
        SoMultiTextureMatrixElement::set(state, NULL, i, matrixidentity);
    }
  }

  this->caches.emplace_back(matrix,
                            identity,
                            this->resetmatrix,
                            cache,
                            vcache);
  captureMaterial(state);
  this->caches.back().material = this->material;
}

void
SoFCRenderCache::addChildCache(SoState *state, SoFCRenderCache * cache)
{
  PRIVATE(this)->addChildCache(state, cache, NULL, false);
  this->addCacheDependency(state, cache);
}

void
SoFCRenderCache::addChildCache(SoState *state, SoFCVertexCache * cache)
{
  PRIVATE(this)->addChildCache(state, NULL, cache, false);
  this->addCacheDependency(state, cache);
}

void
SoFCRenderCache::beginChildCaching(SoState *state, SoFCRenderCache * cache)
{
  PRIVATE(this)->addChildCache(state, cache, NULL, true);
}

void
SoFCRenderCache::beginChildCaching(SoState *state, SoFCVertexCache * cache)
{
  PRIVATE(this)->addChildCache(state, NULL, cache, true);
}

void
SoFCRenderCache::endChildCaching(SoState * state, SoCache * cache)
{
  this->addCacheDependency(state, cache);
}

class MyMultiTextureImageElement : public SoMultiTextureImageElement
{
public:
  SbBool hasTransparency(const int unit) const {
    return SoMultiTextureImageElement::hasTransparency(unit);
  }
};

void
SoFCRenderCache::addTexture(SoState * state, const SoTexture * texture)
{
  int unit = SoTextureUnitElement::get(state);

  TextureInfo info;
  info.texture = const_cast<SoTexture*>(texture);

  auto elem = constElement<MyMultiTextureImageElement>(state);
  info.transparent = elem->hasTransparency(unit);

  info.identity = true;
  auto melem = constElement<MyMultiTextureMatrixElement>(state);
  if (melem->getNumUnits() > unit) {
    const auto & data = melem->getUnitData(unit);
    if (data.textureMatrix != matrixidentity) {
      info.identity = false;
      info.matrix = data.textureMatrix;
    }
  }

  PRIVATE(this)->material.textures.set(unit, info);  
}

void
SoFCRenderCache::addTextureTransform(SoState * state, const SoNode * node)
{
  (void)node;
  int unit = SoTextureUnitElement::get(state);

  MatrixInfo info;
  bool identity = true;
  auto melem = constElement<MyMultiTextureMatrixElement>(state);
  if (melem->getNumUnits() > unit) {
    const auto & data = melem->getUnitData(unit);
    if (data.textureMatrix != matrixidentity) {
      identity = false;
      info.matrix = data.textureMatrix;
    }
  }
  if (identity)
    PRIVATE(this)->material.texturematrices.erase(unit);
  else
    PRIVATE(this)->material.texturematrices.set(unit, info);
}

void
SoFCRenderCache::addLight(SoState * state, const SoLight * light)
{
  (void)state;
  if (!light->on.getValue()) return;

  LightInfo info;
  info.light = const_cast<SoLight*>(light);
  info.resetmatrix = PRIVATE(this)->resetmatrix;

  auto elem = constElement<SoModelMatrixElement>(state);
  if (elem->getModelMatrix() == matrixidentity)
    info.identity = true;
  else {
    info.identity = false;
    info.matrix = elem->getModelMatrix();
  }

  PRIVATE(this)->material.lights.append(info);
}

void
SoFCRenderCache::resetMatrix()
{
  PRIVATE(this)->resetmatrix = true;
}

inline void
SoFCRenderCacheP::finalizeMaterial(Material & material)
{
  if (material.materialbinding == SoMaterialBindingElement::OVERALL)
    material.pervertexcolor = false;

  if (material.type == Material::Triangle) {
    if (this->facecolor) {
      material.pervertexcolor = false;
      material.diffuse = this->facecolor;
    }
  }
  else if (this->nonfacecolor) {
    material.pervertexcolor = false;
    material.diffuse = this->nonfacecolor;
  }

  if (material.pervertexcolor && (material.maskflags & Material::FLAG_TRANSPARENCY))
    material.overrideflags |= Material::FLAG_TRANSPARENCY;
}

const SoFCRenderCache::VertexCacheMap &
SoFCRenderCache::getVertexCaches(bool finalize)
{
  auto & vcachemap = PRIVATE(this)->vcachemap;
  if (!vcachemap.empty())
    return vcachemap;

  std::unordered_map<void *, CacheKeyPtr> keymap;
  CacheKeyPtr selfkey;

  for (auto & entry : PRIVATE(this)->caches) {
    if (entry.vcache) {
      if (!selfkey)
        selfkey.reset(new CacheKey(1, PRIVATE(this)->nodeptr));

      entry.material.pervertexcolor = entry.vcache->colorPerVertex();

      if (entry.vcache->getNumTriangleIndices()) {
        Material material = entry.material;
        material.type = Material::Triangle;
        if (finalize)
          PRIVATE(this)->finalizeMaterial(material);
        vcachemap[material].emplace_back(entry.vcache,
                                         entry.matrix,
                                         entry.identity,
                                         entry.resetmatrix,
                                         selfkey);
      }
      if (entry.vcache->getNumLineIndices()) {
        Material material = entry.material;
        material.type = Material::Line;
        if (finalize)
          PRIVATE(this)->finalizeMaterial(material);
        vcachemap[material].emplace_back(entry.vcache,
                                         entry.matrix,
                                         entry.identity,
                                         entry.resetmatrix,
                                         selfkey);
      }
      if (entry.vcache->getNumPointIndices()) {
        Material material = entry.material;
        material.type = Material::Point;
        if (finalize)
          PRIVATE(this)->finalizeMaterial(material);
        vcachemap[material].emplace_back(entry.vcache,
                                         entry.matrix,
                                         entry.identity,
                                         entry.resetmatrix,
                                         selfkey);
      }
      continue;
    }
    auto it = vcachemap.end();
    const auto & childvcaches = entry.cache->getVertexCaches(); 
    for (const auto & child : childvcaches) {
      Material material = PRIVATE(this)->mergeMaterial(
            entry.matrix, entry.identity, entry.material, child.first);

      if (finalize)
        PRIVATE(this)->finalizeMaterial(material);

      VertexCacheMap::value_type value(material, {});
      it = vcachemap.insert(it, value);
      for (const VertexCacheEntry & childentry : child.second) {
        CacheKeyPtr & key = keymap[childentry.key.get()];
        if (!key) {
          key.reset(new CacheKey);
          key->reserve(childentry.key->size()+1);
          key->push_back(PRIVATE(this)->nodeptr);
          key->insert(key->end(), childentry.key->begin(), childentry.key->end());
        }
        if (entry.identity || childentry.resetmatrix)
          it->second.emplace_back(childentry.cache,
                                  childentry.matrix,
                                  childentry.identity,
                                  childentry.resetmatrix,
                                  key);
        else if (childentry.identity)
          it->second.emplace_back(childentry.cache,
                                  entry.matrix,
                                  entry.identity,
                                  false,
                                  key);
        else {
          it->second.emplace_back(childentry.cache,
                                  entry.matrix,
                                  false,
                                  false,
                                  key);
          it->second.back().matrix.multLeft(childentry.matrix);
        }
      }
      ++it;
    }
  }

  return vcachemap;
}

SoFCRenderCache::VertexCacheMap
SoFCRenderCache::buildHighlightCache(int order, const SoDetail * detail, uint32_t color)
{
  VertexCacheMap res;
  uint32_t alpha = color & 0xff;

  const SoPointDetail * pd = nullptr;
  const SoLineDetail * ld = nullptr;
  const SoFaceDetail * fd = nullptr;
  const SoFCDetail * d = nullptr;
  if (detail) {
    if (detail->isOfType(SoPointDetail::getClassTypeId()))
      pd = static_cast<const SoPointDetail*>(detail);
    else if (detail->isOfType(SoLineDetail::getClassTypeId()))
      ld = static_cast<const SoLineDetail*>(detail);
    else if (detail->isOfType(SoFaceDetail::getClassTypeId()))
      fd = static_cast<const SoFaceDetail*>(detail);
    else if (detail->isOfType(SoFCDetail::getClassTypeId()))
      d = static_cast<const SoFCDetail*>(detail);
  }

  for (auto & child : getVertexCaches(true)) {
    Material material = child.first;
    if (color)
      material.pervertexcolor = false;

    for (auto & ventry : child.second) {
      material.order = order;
      material.depthfunc = SoDepthBuffer::LEQUAL;
      if (detail) {
        switch(material.type) {
        case Material::Point:
          if (!pd && (!d || d->getIndices(SoFCDetail::Vertex).empty()))
            continue;
          break;
        case Material::Line:
          if (!ld && (!d || d->getIndices(SoFCDetail::Edge).empty()))
            continue;
          break;
        default:
          if (!fd && (!d || d->getIndices(SoFCDetail::Face).empty()))
            continue;
        }
      }

      if (color) {
        if (material.type != Material::Triangle) {
          material.lightmodel = SoLazyElement::BASE_COLOR;
          material.diffuse = color | 0xff;
        } else { 
          uint32_t a = (child.first.diffuse & 0xff);
          if (child.first.pervertexcolor && ventry.cache->hasTransparency()) {
            if (a == 0xff)
              a = 0x80;
          }
          if (a > alpha)
            a = alpha;
          if (alpha != 0xff)
            material.overrideflags |= Material::FLAG_TRANSPARENCY;
          material.diffuse = (material.diffuse & 0xffffff00) | a;
          material.emissive = color | 0xff;
        }
      }

      VertexCacheEntry newentry = ventry;

      switch(material.type) {
      case Material::Point:
        if (!material.order)
          material.order = 1;
        if (material.pointsize < 4.0f)
          material.pointsize = 4.0f;
        if (pd) {
          if (pd->getCoordinateIndex() >= 0)
            newentry.partidx = pd->getCoordinateIndex();
        }
        else if (d) {
          const auto & indices = d->getIndices(SoFCDetail::Vertex);
          if (indices.size() == 1 && *indices.begin() >= 0)
            newentry.partidx = *indices.begin();
          else if (indices.size() > 1) {
            newentry.cache = new SoFCVertexCache(*newentry.cache);
            newentry.cache->addPoints(indices);
          }
        }
        break;
      case Material::Line:
        if (material.linewidth < 2.0f)
          material.linewidth = 2.0f;
        if (ld) {
          if (ld->getLineIndex() >= 0)
            newentry.partidx = ld->getLineIndex();
        }
        else if (d) {
          const auto & indices = d->getIndices(SoFCDetail::Edge);
          if (indices.size() == 1 && *indices.begin() >= 0)
            newentry.partidx = *indices.begin();
          else if (indices.size() > 1) {
            newentry.cache = new SoFCVertexCache(*newentry.cache);
            newentry.cache->addLines(indices);
          }
        }
        break;
      default:
        if (fd) {
          if (fd->getPartIndex() >= 0)
            newentry.partidx = fd->getPartIndex();
        }
        else if (d) {
          const auto & indices = d->getIndices(SoFCDetail::Face);
          if (indices.size() == 1 && *indices.begin() >= 0)
            newentry.partidx = *indices.begin();
          else if (indices.size() > 1) {
            newentry.cache = new SoFCVertexCache(*newentry.cache);
            newentry.cache->addTriangles(indices);
          }
        }
        if (color && newentry.partidx >= 0) {
          uint32_t col = newentry.cache->getFaceColor(newentry.partidx);
          if ((col & 0xff) == 0xff && alpha == 0xff) {
            if (material.lightmodel == SoLazyElement::BASE_COLOR)
              material.diffuse = color;
            else
              material.diffuse = col;
          }
        }
        break;
      }
      res[material].push_back(newentry);
    }
  }
  return res;
}

// vim: noai:ts=2:sw=2
