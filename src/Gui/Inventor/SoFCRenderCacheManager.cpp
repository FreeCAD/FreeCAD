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

#include <Inventor/actions/SoGLRenderAction.h>
#include <Inventor/elements/SoGLCacheContextElement.h>
#include <Inventor/elements/SoTextureEnabledElement.h>
#include <Inventor/elements/SoCacheElement.h>
#include <Inventor/nodes/SoGroup.h>
#include <Inventor/nodes/SoShape.h>
#include <Inventor/nodes/SoResetTransform.h>
#include <Inventor/nodes/SoTexture2Transform.h>
#include <Inventor/nodes/SoTexture3Transform.h>
#include <Inventor/nodes/SoTextureMatrixTransform.h>
#include <Inventor/nodes/SoMaterial.h>
#include <Inventor/nodes/SoDepthBuffer.h>
#include <Inventor/nodes/SoLightModel.h>
#include <Inventor/nodes/SoLight.h>
#include <Inventor/nodes/SoAnnotation.h>
#include <Inventor/sensors/SoDataSensor.h>
#include <Inventor/sensors/SoPathSensor.h>
#include <Inventor/actions/SoCallbackAction.h>
#include <Inventor/SoPrimitiveVertex.h>
#include <Inventor/details/SoFaceDetail.h>
#include <Inventor/details/SoLineDetail.h>
#include <Inventor/details/SoPointDetail.h>
#include <Inventor/misc/SoTempPath.h>

#include <unordered_map>
#include <map>

#include "../ViewParams.h"
#include "../InventorBase.h"
#include "../SoFCUnifiedSelection.h"

#include "SoFCVertexCache.h"
#include "SoFCRenderCache.h"
#include "SoFCRenderer.h"
#include "SoFCRenderCacheManager.h"

using namespace Gui;

// ---------------------------------------------------------------

typedef CoinPtr<SoPath> PathPtr;
typedef CoinPtr<SoFCVertexCache> VertexCachePtr;
typedef CoinPtr<SoFCRenderCache> RenderCachePtr;
typedef SoFCRenderCache::VertexCacheMap VertexCacheMap;

// copied from boost::hash_combine. 
template <class T>
static inline void hash_combine(std::size_t& seed, const T& v)
{
  std::hash<T> hasher;
  seed ^= hasher(v) + 0x9e3779b9 + (seed<<6) + (seed>>2);
}

struct PathHasher {
  std::size_t operator()(const PathPtr & path) const {
    if (!path) return 0;
    std::size_t seed = 0;
    for (int i=0, n=path->getLength(); i<n; ++i) {
      hash_combine(seed, path->getNode(i));
    }
    return seed;
  }
};

struct ElementEntry {
  ElementEntry()
    : id(0), color(0)
  {}

  std::unique_ptr<SoDetail> detail;
  int id;
  uint32_t color;
  VertexCacheMap vcachemap;
};

class SelectionSensor : public SoPathSensor {
public:
  SelectionSensor() {
    setTriggerFilter(SoPathSensor::NODES);
    ontop = false;
  }

  void clear(SoFCRenderer * renderer) {
    if (this->cache)
      return;
    this->cache.reset();
    for (auto & v : this->elements) {
      renderer->removeSelection(v.second.id);
      v.second.vcachemap.clear();
    }
  }

  std::unordered_map<std::string, ElementEntry> elements;
  RenderCachePtr cache;
  bool ontop;
};

typedef std::unordered_map<PathPtr, SelectionSensor, PathHasher> SelectionPathMap;

class SoFCRenderCacheManagerP {
public:
  SoFCRenderCacheManagerP();
  ~SoFCRenderCacheManagerP();

  void addSelection(const char *key,
                    const SoDetail * detail,
                    uint32_t color,
                    bool ontop,
                    bool alt);

  static SoCallbackAction::Response preSeparator(void *, SoCallbackAction *action, const SoNode * node);
  static SoCallbackAction::Response postSeparator(void *, SoCallbackAction *action, const SoNode * node);
  static SoCallbackAction::Response preAnnotation(void *, SoCallbackAction *action, const SoNode * node);
  static SoCallbackAction::Response postAnnotation(void *, SoCallbackAction *action, const SoNode * node);
  static SoCallbackAction::Response preShape(void *, SoCallbackAction *action, const SoNode * node);
  static SoCallbackAction::Response postShape(void *, SoCallbackAction *action, const SoNode * node);
  static SoCallbackAction::Response postLightModel(void *, SoCallbackAction *action, const SoNode * node);
  static SoCallbackAction::Response postLight(void *, SoCallbackAction *action, const SoNode * node);
  static SoCallbackAction::Response postMaterial(void *, SoCallbackAction *action, const SoNode * node);
  static SoCallbackAction::Response postDepthBuffer(void *, SoCallbackAction *action, const SoNode * node);
  static SoCallbackAction::Response postResetTransform(void *, SoCallbackAction *action, const SoNode * node);
  static SoCallbackAction::Response postTextureTransform(void *, SoCallbackAction *action, const SoNode * node);
  static SoCallbackAction::Response postTexture(void *, SoCallbackAction *action, const SoNode * node);
  static void addTriangle(void *,
                          SoCallbackAction * action,
                          const SoPrimitiveVertex * v0,
                          const SoPrimitiveVertex * v1,
                          const SoPrimitiveVertex * v2);
  static void addLine(void *,
                      SoCallbackAction * action,
                      const SoPrimitiveVertex * v0,
                      const SoPrimitiveVertex * v1);
  static void addPoint(void *, SoCallbackAction * action, const SoPrimitiveVertex * v);

  static void updateSelection(void *, SoSensor *sensor);

  class NodeSensor : public SoDataSensor
  {
  public:
    NodeSensor()
      : node(NULL), master(NULL)
    {}

    ~NodeSensor() { detach(); }

    void attach(SoFCRenderCacheManagerP * master, const SoNode *node) {
      if (this->node == node) return;
      assert(!this->node);
      this->master = master;
      this->node = const_cast<SoNode *>(node);
      this->node->addAuditor(this, SoNotRec::SENSOR);
    }

    void detach() {
      if (!this->node) return;
      this->node->removeAuditor(this, SoNotRec::SENSOR);
      this->node = NULL;
    }

    SoNode * node;
    SoFCRenderCacheManagerP * master;
  };

  class CacheSensor : public NodeSensor
  {
  public:
    virtual void dyingReference(void) {
      SoNode * node = this->node;
      this->detach();
      master->cachetable.erase(node);
    }

    std::vector<RenderCachePtr> caches;
  };

  class VCacheSensor : public NodeSensor
  {
  public:
    virtual void dyingReference(void) {
      SoNode * node = this->node;
      this->detach();
      master->vcachetable.erase(node);
    }

    std::vector<VertexCachePtr> caches;
  };

  FC_COIN_THREAD_LOCAL std::unordered_map<const SoNode *, CacheSensor> cachetable;
  FC_COIN_THREAD_LOCAL std::unordered_map<const SoNode *, VCacheSensor> vcachetable;

  SoCallbackAction action;
  VertexCachePtr vcache;

  std::unordered_map<std::string, SelectionPathMap> selcaches;
  std::map<int, SoPath*> selpaths;

  int selid;
  
  std::vector<RenderCachePtr> stack;
  std::vector<RenderCachePtr> caches;
  SbFCUniqueId sceneid;
  std::unordered_set<const SoNode *> nodeset;
  const SoNode * prunenode;
  int traversedepth;
  SoFCRenderer *renderer;
  int annotation;
  bool initmaterial;
};

#define PRIVATE(obj) ((obj)->pimpl)

SoFCRenderCacheManagerP::SoFCRenderCacheManagerP()
{
  this->initmaterial = false;
  this->prunenode = nullptr;
  this->selid = 0;
  this->sceneid = 0;
  this->annotation = 0;
  this->action.addPreCallback(SoFCSelectionRoot::getClassTypeId(), &preSeparator, this);
  this->action.addPostCallback(SoFCSelectionRoot::getClassTypeId(), &postSeparator, this);
  this->action.addPreCallback(SoAnnotation::getClassTypeId(), &preAnnotation, this);
  this->action.addPostCallback(SoAnnotation::getClassTypeId(), &postAnnotation, this);
  this->action.addPreCallback(SoShape::getClassTypeId(), &preShape, this);
  this->action.addPostCallback(SoShape::getClassTypeId(), &postShape, this);
  this->action.addPostCallback(SoTexture::getClassTypeId(), &postTexture, this);
  this->action.addPostCallback(SoResetTransform::getClassTypeId(), &postResetTransform, this);
  this->action.addPostCallback(SoTextureMatrixTransform::getClassTypeId(), &postTextureTransform, this);
  this->action.addPostCallback(SoTexture2Transform::getClassTypeId(), &postTextureTransform, this);
  this->action.addPostCallback(SoTexture3Transform::getClassTypeId(), &postTextureTransform, this);
  this->action.addPostCallback(SoLightModel::getClassTypeId(), &postLightModel, this);
  this->action.addPostCallback(SoMaterial::getClassTypeId(), &postMaterial, this);
  this->action.addPostCallback(SoDepthBuffer::getClassTypeId(), &postDepthBuffer, this);
  this->action.addPostCallback(SoLight::getClassTypeId(), &postLight, this);

  this->action.addTriangleCallback(SoShape::getClassTypeId(), &addTriangle, this);
  this->action.addLineSegmentCallback(SoShape::getClassTypeId(), &addLine, this);
  this->action.addPointCallback(SoShape::getClassTypeId(), &addPoint, this);

  this->renderer = new SoFCRenderer;
}

SoFCRenderCacheManagerP::~SoFCRenderCacheManagerP()
{
  delete this->renderer;
}

const std::map<int, SoPath*> &
SoFCRenderCacheManager::getSelectionPaths() const
{
  return PRIVATE(this)->selpaths;
}

SoFCRenderCacheManager::SoFCRenderCacheManager()
  :pimpl(new SoFCRenderCacheManagerP)
{
}

SoFCRenderCacheManager::~SoFCRenderCacheManager()
{
  delete pimpl;
}

void
SoFCRenderCacheManager::clear()
{
  PRIVATE(this)->stack.clear();
  PRIVATE(this)->caches.clear();
  PRIVATE(this)->nodeset.clear();
  PRIVATE(this)->cachetable.clear();
  PRIVATE(this)->vcachetable.clear();
  PRIVATE(this)->selcaches.clear();
  PRIVATE(this)->selpaths.clear();
  PRIVATE(this)->renderer->clear();
}

bool
SoFCRenderCacheManager::isOnTop(const std::string & key)
{
  auto it = PRIVATE(this)->selcaches.find(key);
  if (it == PRIVATE(this)->selcaches.end())
    return false;
  for (auto & v : it->second) {
    if (v.second.ontop)
      return true;
  }
  return false;
}

void
SoFCRenderCacheManager::setHighlight(SoPath * path,
                                     const SoDetail * detail,
                                     uint32_t color,
                                     bool ontop)
{
  PRIVATE(this)->caches.clear();
  SoState * state = PRIVATE(this)->action.getState();
  if (ontop)
    SoFCSwitch::setOverrideSwitch(state, true);
  PRIVATE(this)->action.apply(path);
  if (ontop)
    SoFCSwitch::setOverrideSwitch(state, false);
  if (PRIVATE(this)->caches.size()) {
    int order = ontop ? 1 : 0;
    PRIVATE(this)->renderer->setHighlight(
          PRIVATE(this)->caches[0]->buildHighlightCache(
            order, detail, color, true, false));
    PRIVATE(this)->caches.clear();
  }
}

void
SoFCRenderCacheManager::clearHighlight()
{
  PRIVATE(this)->renderer->clearHighlight();
}

void
SoFCRenderCacheManagerP::updateSelection(void * userdata, SoSensor * _sensor)
{
  SoFCRenderCacheManagerP * self = reinterpret_cast<SoFCRenderCacheManagerP*>(userdata);
  SelectionSensor * sensor = static_cast<SelectionSensor*>(_sensor);

  SoPath * path = sensor->getAttachedPath();
  sensor->clear(self->renderer);
  if (!path->getLength())
    return;

  self->caches.clear();
  SoState * state = self->action.getState();
  if (sensor->ontop)
    SoFCSwitch::setOverrideSwitch(state, true);
  self->action.apply(path);
  if (sensor->ontop)
    SoFCSwitch::setOverrideSwitch(state, false);

  if (self->caches.empty())
    return;

  sensor->cache = self->caches[0];
  self->caches.clear();

  for (auto & v : sensor->elements) {
    auto & elentry = v.second;
    elentry.vcachemap = sensor->cache->buildHighlightCache(
        elentry.id, elentry.detail.get(), elentry.color,
        ViewParams::highlightIndicesOnFullSelect());
    self->renderer->addSelection(elentry.id, elentry.vcachemap);
  }
}

void
SoFCRenderCacheManager::addSelection(const std::string & key,
                                     const std::string & element,
                                     SoPath * path,
                                     const SoDetail * detail,
                                     uint32_t color,
                                     bool ontop,
                                     bool alt)
{
  if (!path || !path->getLength())
    return;

  bool update = false;
  PathPtr selpath;
  auto & paths = PRIVATE(this)->selcaches[key];
  SelectionSensor * sensor;
  auto it = paths.find(path);
  if (it != paths.end()) {
    sensor = &it->second;
    selpath = it->first;
  }
  else {
    selpath = path->copy();
    sensor = &paths[selpath];
    sensor->attach(selpath);
    sensor->setFunction(&SoFCRenderCacheManagerP::updateSelection);
    sensor->setData(PRIVATE(this));
    update = true;
  }

  if (sensor->ontop)
    ontop = true;

  int id = 0;
  if (alt) {
    id |= SoFCRenderer::SelIdAlt;
    ontop = true;
  }
  id += ++PRIVATE(this)->selid;
  if (!ontop)
    id = -id;

  if (id > 0 && !sensor->ontop) {
    sensor->ontop = true;
    update = true;
    if (element.size()) {
      // When any element is selected on top, we shall bring the whole object on
      // top. So make sure a nil element entry exist.
      auto & elentry = sensor->elements[std::string()];
      if (!elentry.id) {
        elentry.id = id++;
        ++PRIVATE(this)->selid;
      }
    }
    for (auto & v : sensor->elements) {
      auto & elentry = v.second;
      if (elentry.id < 0) {
        PRIVATE(this)->renderer->removeSelection(elentry.id);
        elentry.id = id++;
        ++PRIVATE(this)->selid;
        if (element.empty())
          elentry.id |= SoFCRenderer::SelIdFull;
        else
          elentry.id |= SoFCRenderer::SelIdPartial;
      }

      elentry.color &= 0xffffff00;
      elentry.color |= color & 0xff;

      if (v.first.empty())
        PRIVATE(this)->selpaths[elentry.id] = selpath;

      if (sensor->cache) {
        elentry.vcachemap = sensor->cache->buildHighlightCache(
            elentry.id, elentry.detail.get(), elentry.color,
            ViewParams::highlightIndicesOnFullSelect());
        PRIVATE(this)->renderer->addSelection(elentry.id, elentry.vcachemap);
      }
    }
  }

  auto & elentry = sensor->elements[element];
  if (elentry.id && elentry.id != id) {
    if (elentry.id > 0 && element.empty())
      PRIVATE(this)->selpaths.erase(elentry.id);
    PRIVATE(this)->renderer->removeSelection(elentry.id);
  }
  if (id > 0 && (color & 0xffffff00)) {
    if (element.empty())
      id |= SoFCRenderer::SelIdFull;
    else
      id |= SoFCRenderer::SelIdPartial;
  }
  if (id > 0 && element.empty())
    PRIVATE(this)->selpaths[id] = selpath;
  elentry.id = id;
  elentry.color = color;
  elentry.detail.reset(detail ? detail->copy() : nullptr);
  if (sensor->cache) {
    elentry.vcachemap = sensor->cache->buildHighlightCache(
        elentry.id, elentry.detail.get(), elentry.color,
        ViewParams::highlightIndicesOnFullSelect());
    PRIVATE(this)->renderer->addSelection(elentry.id, elentry.vcachemap);
  }

  if (update)
    SoFCRenderCacheManagerP::updateSelection(PRIVATE(this), sensor);
}

void
SoFCRenderCacheManager::addSelection(const std::string & key,
                                     const std::string & element,
                                     SoNode * node,
                                     uint32_t color,
                                     bool ontop,
                                     bool alt)
{
  static FC_COIN_THREAD_LOCAL SoTempPath path(1);
  path.ref();
  path.truncate(0);
  path.append(node);
  addSelection(key, element, &path, nullptr, color, ontop, alt);
  path.truncate(0);
  path.unrefNoDelete();
}

void
SoFCRenderCacheManager::removeSelection(const std::string & key,
                                        const std::string & element,
                                        bool alt)
{
  auto it = PRIVATE(this)->selcaches.find(key);
  if (it == PRIVATE(this)->selcaches.end())
    return;

  auto & paths = it->second;

  for (auto itpath = paths.begin(); itpath!=paths.end(); ) {
    auto & sensor = itpath->second;
    auto iter = sensor.elements.find(element);
    if (iter == sensor.elements.end()) {
      ++itpath;
      continue;
    }
    auto & elentry = iter->second;
    if (alt) {
      if (!(elentry.id & SoFCRenderer::SelIdAlt)) {
        ++itpath;
        continue;
      }
      if (element.empty())
        PRIVATE(this)->selpaths.erase(elentry.id);
      PRIVATE(this)->renderer->removeSelection(elentry.id);
      if (!(elentry.id & SoFCRenderer::SelIdSelected)) {
        sensor.elements.erase(iter);
        if (sensor.elements.empty())
          itpath = paths.erase(itpath);
        else
          ++itpath;
        continue;
      }
      elentry.id ^= SoFCRenderer::SelIdAlt;
      PRIVATE(this)->renderer->addSelection(elentry.id, elentry.vcachemap);
      if (element.empty())
        PRIVATE(this)->selpaths[elentry.id] = itpath->first;
      ++itpath;
      continue;
    }
    if (element.empty())
      PRIVATE(this)->selpaths.erase(elentry.id);
    if (elentry.id & SoFCRenderer::SelIdAlt) {
      if (elentry.id & SoFCRenderer::SelIdSelected) {
        PRIVATE(this)->renderer->removeSelection(elentry.id);
        elentry.id &= ~(SoFCRenderer::SelIdSelected);
        elentry.color &= 0xff;
        elentry.vcachemap = sensor.cache->buildHighlightCache(
            elentry.id, elentry.detail.get(), elentry.color, false);
        PRIVATE(this)->renderer->addSelection(elentry.id, elentry.vcachemap);
      }
      ++itpath;
      continue;
    }
    PRIVATE(this)->renderer->removeSelection(elentry.id);
    sensor.elements.erase(iter);
    if (sensor.ontop && element.size()) {
      iter = sensor.elements.find(std::string());
      if (iter != sensor.elements.end()) {
        auto & elentry = iter->second;
        if (!(elentry.id & SoFCRenderer::SelIdAlt)) {
          PRIVATE(this)->selpaths.erase(elentry.id);
          PRIVATE(this)->renderer->removeSelection(elentry.id);
          sensor.elements.erase(iter);
        }
      }
    }
    if (sensor.elements.empty())
      itpath = paths.erase(itpath);
    else
      ++itpath;
    continue;
  }

  if (paths.empty())
    PRIVATE(this)->selcaches.erase(it);
}

void
SoFCRenderCacheManager::clearSelection(bool alt)
{
  auto itsel = PRIVATE(this)->selcaches.begin();
  while (itsel!=PRIVATE(this)->selcaches.end()) {
    auto & paths = itsel->second;
    for (auto itpath=paths.begin(); itpath!=paths.end(); ) {
      auto & sensor = itpath->second;
      for (auto iter=sensor.elements.begin(); iter!=sensor.elements.end(); ) {
        auto & elentry = iter->second;
        if (alt) {
          if (!(elentry.id & SoFCRenderer::SelIdAlt)) {
            ++iter;
            continue;
          }
          if (iter->first.empty())
            PRIVATE(this)->selpaths.erase(elentry.id);
          PRIVATE(this)->renderer->removeSelection(elentry.id);
          if (elentry.color) {
            elentry.id = ++PRIVATE(this)->selid;
            if (iter->first.empty())
              PRIVATE(this)->selpaths[elentry.id] = itpath->first;
            PRIVATE(this)->renderer->addSelection(elentry.id, elentry.vcachemap);
            ++iter;
            continue;
          }
          iter = sensor.elements.erase(iter);
          continue;
        }
        else if (elentry.id & SoFCRenderer::SelIdAlt) {
          if (elentry.id & SoFCRenderer::SelIdSelected) {
            PRIVATE(this)->renderer->removeSelection(elentry.id);
            elentry.id &= ~(SoFCRenderer::SelIdSelected);
            elentry.color &= 0xff;
            elentry.vcachemap = sensor.cache->buildHighlightCache(
                elentry.id, elentry.detail.get(), elentry.color, false);
            PRIVATE(this)->renderer->addSelection(elentry.id, elentry.vcachemap);
          }
          ++iter;
          continue;
        }
        else {
          if (iter->first.empty())
            PRIVATE(this)->selpaths.erase(elentry.id);
          PRIVATE(this)->renderer->removeSelection(elentry.id);
          iter = sensor.elements.erase(iter);
        }
      }
      if (sensor.elements.empty())
        itpath = paths.erase(itpath);
      else
        ++itpath;
    }
    if (paths.empty())
      itsel = PRIVATE(this)->selcaches.erase(itsel);
    else
      ++itsel;
  }
  if (PRIVATE(this)->selcaches.empty())
    PRIVATE(this)->selid = 0;
}

void
SoFCRenderCacheManager::render(SoGLRenderAction * action)
{
  SoState * state = action->getState();
  SoCacheElement::invalidate(state);
  SoGLCacheContextElement::shouldAutoCache(state,
                                           SoGLCacheContextElement::DONT_AUTO_CACHE);

  const SoPath * path = action->getCurPath();
  if (!PRIVATE(this)->sceneid || PRIVATE(this)->sceneid != path->getTail()->getNodeId()) {
    PRIVATE(this)->initmaterial = true;
    PRIVATE(this)->sceneid = path->getTail()->getNodeId();
    PRIVATE(this)->caches.clear();
    CoinPtr<SoPath> pathCopy(path->copy());
    PRIVATE(this)->action.apply(pathCopy);
    PRIVATE(this)->renderer->setScene(PRIVATE(this)->caches);
    PRIVATE(this)->caches.clear();
    PRIVATE(this)->initmaterial = false;
  }

  PRIVATE(this)->renderer->render(action);
}

SoCallbackAction::Response
SoFCRenderCacheManagerP::preSeparator(void *userdata,
                                      SoCallbackAction *action,
                                      const SoNode * node)
{
  SoFCRenderCacheManagerP *self = reinterpret_cast<SoFCRenderCacheManagerP*>(userdata);
  if (!self->nodeset.insert(node).second) {
      self->prunenode = node;
      SoDebugError::postWarning("SoFCRenderCacheManagerP::preSeparator",
                                "recursive scene graph detected, %p", node);
      return SoCallbackAction::PRUNE;
  }

  SoState * state = action->getState();
  SoFCRenderCache *currentcache = self->stack.empty() ? nullptr : self->stack.back();

  CacheSensor * sensor = nullptr;
  if (action->getCurPathCode() == SoAction::BELOW_PATH) {
    sensor = &self->cachetable[node];
    sensor->attach(self, node);
    for (auto it=sensor->caches.begin(); it!=sensor->caches.end();) {
      auto & cache = *it;
      if (cache->getNodeId() != node->getNodeId()) {
        it = sensor->caches.erase(it);
        continue;
      }
      if (cache->isValid(state)) {
        if (currentcache)
          currentcache->addChildCache(state, cache);
        else
          self->caches.push_back(cache);
        self->stack.push_back(cache);
        return SoCallbackAction::PRUNE;
      }
      ++it;
    }
  }

  state->push();

  intptr_t nodeptr = 0;
  bool selectable = true;
  if (node->isOfType(SoFCSelectionRoot::getClassTypeId())) {
    auto selroot = static_cast<const SoFCSelectionRoot*>(node);
    nodeptr = reinterpret_cast<intptr_t>(selroot);
    selectable = selroot->selectionStyle.getValue() != SoFCSelectionRoot::Unpickable;
  }

  RenderCachePtr cache(new SoFCRenderCache(state, nodeptr, node->getNodeId()));

  if (sensor)
    sensor->caches.push_back(cache);
  if (currentcache)
    currentcache->beginChildCaching(state, cache);
  else
    self->caches.push_back(cache);
  self->stack.push_back(cache);
  cache->open(state, selectable, self->initmaterial && self->stack.size()==1);
  return SoCallbackAction::CONTINUE;
}

SoCallbackAction::Response
SoFCRenderCacheManagerP::postSeparator(void *userdata,
                                       SoCallbackAction *action,
                                       const SoNode * node)
{
  (void)node;
  SoFCRenderCacheManagerP *self = reinterpret_cast<SoFCRenderCacheManagerP*>(userdata);
  if (self->stack.empty())
      return SoCallbackAction::CONTINUE;

  if (self->prunenode == node) {
    self->prunenode = nullptr;
    return SoCallbackAction::PRUNE;
  }

  self->nodeset.erase(node);
  SoState * state = action->getState();
  RenderCachePtr cache(self->stack.back());

  self->stack.pop_back();
  if (SoCacheElement::getCurrentCache(state) == cache) {
    state->pop();
    cache->close(state);
    if (self->stack.size())
      self->stack.back()->endChildCaching(state, cache);
  }
  return SoCallbackAction::CONTINUE;
}

SoCallbackAction::Response
SoFCRenderCacheManagerP::preAnnotation(void *userdata,
                                       SoCallbackAction *action,
                                       const SoNode * node)
{
  SoFCRenderCacheManagerP *self = reinterpret_cast<SoFCRenderCacheManagerP*>(userdata);
  if (self->stack.empty())
      return SoCallbackAction::CONTINUE;

  (void)node;
  (void)action;
  if (++self->annotation == 1)
    self->stack.back()->increaseRenderingOrder();
  return SoCallbackAction::CONTINUE;
}

SoCallbackAction::Response
SoFCRenderCacheManagerP::postAnnotation(void *userdata,
                                        SoCallbackAction *action,
                                        const SoNode * node)
{
  SoFCRenderCacheManagerP *self = reinterpret_cast<SoFCRenderCacheManagerP*>(userdata);
  if (self->stack.empty())
      return SoCallbackAction::CONTINUE;

  (void)node;
  (void)action;
  if (self->annotation && --self->annotation == 0)
    self->stack.back()->decreaseRenderingOrder();
  return SoCallbackAction::CONTINUE;
}

SoCallbackAction::Response
SoFCRenderCacheManagerP::postTexture(void *userdata,
                                     SoCallbackAction *action,
                                     const SoNode * node)
{
  SoFCRenderCacheManagerP *self = reinterpret_cast<SoFCRenderCacheManagerP*>(userdata);
  if (self->stack.empty())
      return SoCallbackAction::CONTINUE;

  assert(node && node->isOfType(SoTexture::getClassTypeId()));
  self->stack.back()->addTexture(action->getState(), static_cast<const SoTexture*>(node));
  return SoCallbackAction::CONTINUE;
}

SoCallbackAction::Response
SoFCRenderCacheManagerP::postResetTransform(void *userdata,
                                           SoCallbackAction *action,
                                           const SoNode * node)
{
  (void)action;
  SoFCRenderCacheManagerP *self = reinterpret_cast<SoFCRenderCacheManagerP*>(userdata);
  if (self->stack.empty())
      return SoCallbackAction::CONTINUE;

  assert(node && node->isOfType(SoResetTransform::getClassTypeId()));
  const SoResetTransform *rs = static_cast<const SoResetTransform*>(node);
  if (!rs->whatToReset.isIgnored() &&
      (rs->whatToReset.getValue() & SoResetTransform::TRANSFORM)) {
    self->stack.back()->resetMatrix();
  }
  return SoCallbackAction::CONTINUE;
}

SoCallbackAction::Response
SoFCRenderCacheManagerP::postTextureTransform(void *userdata,
                                              SoCallbackAction *action,
                                              const SoNode * node)
{
  SoFCRenderCacheManagerP *self = reinterpret_cast<SoFCRenderCacheManagerP*>(userdata);
  if (self->stack.empty())
      return SoCallbackAction::CONTINUE;

  self->stack.back()->addTextureTransform(action->getState(), node);
  return SoCallbackAction::CONTINUE;
}

SoCallbackAction::Response
SoFCRenderCacheManagerP::postLightModel(void *userdata,
                                        SoCallbackAction *action,
                                        const SoNode * node)
{
  SoFCRenderCacheManagerP *self = reinterpret_cast<SoFCRenderCacheManagerP*>(userdata);
  if (self->stack.empty())
      return SoCallbackAction::CONTINUE;

  assert(node && node->isOfType(SoLightModel::getClassTypeId()));
  const SoLightModel *lightmodel = static_cast<const SoLightModel*>(node);
  self->stack.back()->setLightModel(action->getState(), lightmodel);
  return SoCallbackAction::CONTINUE;
}

SoCallbackAction::Response
SoFCRenderCacheManagerP::postMaterial(void *userdata,
                                      SoCallbackAction *action,
                                      const SoNode * node)
{
  SoFCRenderCacheManagerP *self = reinterpret_cast<SoFCRenderCacheManagerP*>(userdata);
  if (self->stack.empty())
      return SoCallbackAction::CONTINUE;

  assert(node && node->isOfType(SoMaterial::getClassTypeId()));
  const SoMaterial *material = static_cast<const SoMaterial*>(node);
  self->stack.back()->setMaterial(action->getState(), material);
  return SoCallbackAction::CONTINUE;
}

SoCallbackAction::Response
SoFCRenderCacheManagerP::postDepthBuffer(void *userdata,
                                         SoCallbackAction *action,
                                         const SoNode * node)
{
  SoFCRenderCacheManagerP *self = reinterpret_cast<SoFCRenderCacheManagerP*>(userdata);
  if (self->stack.empty())
      return SoCallbackAction::CONTINUE;

  assert(node && node->isOfType(SoDepthBuffer::getClassTypeId()));
  const SoDepthBuffer *dnode = static_cast<const SoDepthBuffer*>(node);
  self->stack.back()->setDepthBuffer(action->getState(), dnode);
  return SoCallbackAction::CONTINUE;
}

SoCallbackAction::Response
SoFCRenderCacheManagerP::postLight(void *userdata,
                                  SoCallbackAction *action,
                                  const SoNode * node)
{
  SoFCRenderCacheManagerP *self = reinterpret_cast<SoFCRenderCacheManagerP*>(userdata);
  if (self->stack.empty())
      return SoCallbackAction::CONTINUE;

  assert(node && node->isOfType(SoLight::getClassTypeId()));
  const SoLight *light = static_cast<const SoLight*>(node);
  self->stack.back()->addLight(action->getState(), light);
  return SoCallbackAction::CONTINUE;
}

SoCallbackAction::Response
SoFCRenderCacheManagerP::preShape(void *userdata,
                                  SoCallbackAction *action,
                                  const SoNode * node)
{
  SoFCRenderCacheManagerP *self = reinterpret_cast<SoFCRenderCacheManagerP*>(userdata);
  if (self->stack.empty())
      return SoCallbackAction::PRUNE;

  SoState * state = action->getState();
  SoFCRenderCache *currentcache = self->stack.back();

  VCacheSensor & sensor = self->vcachetable[node];
  sensor.attach(self, node);
  CoinPtr<SoFCVertexCache> prev;
  for (auto it=sensor.caches.begin(); it!=sensor.caches.end();) {
    auto & cache = *it;
    if (!prev) prev = cache;
    if (cache->getNodeId() != node->getNodeId()) {
      it = sensor.caches.erase(it);
      continue;
    }
    if (cache->isValid(state)) {
      currentcache->addChildCache(state, cache);
      return SoCallbackAction::PRUNE;
    }
    ++it;
  }

  state->push();
  self->vcache.reset(new SoFCVertexCache(state, node, prev));
  sensor.caches.emplace_back(self->vcache.get());

  currentcache->beginChildCaching(state, self->vcache);
  self->vcache->open(state);
  return SoCallbackAction::CONTINUE;
}

SoCallbackAction::Response
SoFCRenderCacheManagerP::postShape(void *userdata,
                                   SoCallbackAction *action,
                                   const SoNode * node)
{
  (void)node;
  SoFCRenderCacheManagerP *self = reinterpret_cast<SoFCRenderCacheManagerP*>(userdata);
  if (!self->vcache)
    return SoCallbackAction::PRUNE;

  SoState *state = action->getState();
  state->pop();
  self->vcache->close(state);
  self->stack.back()->endChildCaching(state, self->vcache);
  self->vcache.reset();
  return SoCallbackAction::CONTINUE;
}

void
SoFCRenderCacheManagerP::addTriangle(void * userdata, SoCallbackAction *action,
                                     const SoPrimitiveVertex * v0,
                                     const SoPrimitiveVertex * v1,
                                     const SoPrimitiveVertex * v2)
{
  (void)action;
  SoFCRenderCacheManagerP *self = reinterpret_cast<SoFCRenderCacheManagerP*>(userdata);
  if (!self->vcache) return;

  assert(v0 < v1 && v0 < v2);
  assert(((const char*)v1- (const char*)v0) % sizeof(SoPrimitiveVertex) == 0);
  assert(((const char*)v2- (const char*)v0) % sizeof(SoPrimitiveVertex) == 0);

  int pdidx[3];
  pdidx[0] = 0;
  pdidx[1] = (int)(v1 - v0);
  pdidx[2] = (int)(v2 - v0);
  self->vcache->addTriangle(v0,v1,v2,pdidx);
}

void
SoFCRenderCacheManagerP::addLine(void * userdata, SoCallbackAction *action,
                                 const SoPrimitiveVertex * v0,
                                 const SoPrimitiveVertex * v1)
{
  (void)action;
  SoFCRenderCacheManagerP *self = reinterpret_cast<SoFCRenderCacheManagerP*>(userdata);
  if (!self->vcache) return;
  self->vcache->addLine(v0,v1);
}

void
SoFCRenderCacheManagerP::addPoint(void * userdata, SoCallbackAction *action,
                                  const SoPrimitiveVertex * v0)
{
  (void)action;
  SoFCRenderCacheManagerP *self = reinterpret_cast<SoFCRenderCacheManagerP*>(userdata);
  if (!self->vcache) return;
  self->vcache->addPoint(v0);
}

void
SoFCRenderCacheManager::getBoundingBox(SbBox3f & bbox) const
{
  PRIVATE(this)->renderer->getBoundingBox(bbox);
}

SbFCUniqueId
SoFCRenderCacheManager::getSceneNodeId() const
{
  return PRIVATE(this)->sceneid;
}

// vim: noai:ts=2:sw=2
