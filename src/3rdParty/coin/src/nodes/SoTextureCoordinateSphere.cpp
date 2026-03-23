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

/*!
  \class SoTextureCoordinateSphere include/Inventor/nodes/SoTextureCoordinateSphere.h
  \brief The SoTextureCoordinateSphere class generates sphere mapped texture coordinates for shapes.

  \ingroup coin_nodes

  <b>FILE FORMAT/DEFAULTS:</b>
  \code
    TextureCoordinateSphere {
    }
  \endcode

  \since Coin 2.3
*/
// FIXME: Add a better class description (20040123 handegar)

// *************************************************************************

#include <Inventor/nodes/SoTextureCoordinateSphere.h>
#include "coindefs.h"

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif // HAVE_CONFIG

#include <Inventor/C/glue/gl.h>
#include <Inventor/SbBox3f.h>
#include <Inventor/SoFullPath.h>
#include <Inventor/actions/SoCallbackAction.h>
#include <Inventor/actions/SoGLRenderAction.h>
#include <Inventor/actions/SoPickAction.h>
#include <Inventor/caches/SoBoundingBoxCache.h>
#include <Inventor/elements/SoGLCacheContextElement.h>
#include <Inventor/elements/SoGLMultiTextureCoordinateElement.h>
#include <Inventor/elements/SoTextureUnitElement.h>
#include <Inventor/misc/SoState.h>
#include <Inventor/nodes/SoNode.h>
#include <Inventor/nodes/SoShape.h>
#include <Inventor/threads/SbStorage.h>

#include "nodes/SoSubNodeP.h"

// *************************************************************************

typedef struct {
  SbVec3f origo;
  SbBox3f boundingbox;
  SoNode * currentshape;
  SoState * currentstate;
  SbVec4f texcoordreturn;
} so_texcoordsphere_data;

static void
so_texcoordsphere_construct_data(void * closure)
{
  so_texcoordsphere_data * data = (so_texcoordsphere_data *) closure;
  data->currentshape = NULL;
  data->currentstate = NULL;
  data->origo = SbVec3f(0,0,0);
}

static void
so_texcoordsphere_destruct_data(void * COIN_UNUSED_ARG(closure))
{
}

SO_NODE_SOURCE(SoTextureCoordinateSphere);

class SoTextureCoordinateSphereP {

public:
  SoTextureCoordinateSphereP(SoTextureCoordinateSphere * texturenode)
    : master(texturenode) { }

  SbVec4f calculateTextureCoordinate(const SbVec3f & point, const SbVec3f & n);

  so_texcoordsphere_data * so_texcoord_get_data() {
    so_texcoordsphere_data * data = NULL;
    data = (so_texcoordsphere_data *) this->so_texcoord_storage->get();
    assert(data && "Error retrieving thread data.");
    return data;
  }

  SbStorage * so_texcoord_storage;

private:
  SoTextureCoordinateSphere * master;
};


static const SbVec4f & textureCoordinateSphereCallback(void * userdata, const SbVec3f & point, const SbVec3f & normal);

#define PRIVATE(p) (p->pimpl)
#define PUBLIC(p) (p->master)


/*!
  Constructor.
*/
SoTextureCoordinateSphere::SoTextureCoordinateSphere(void)
{

  PRIVATE(this) = new SoTextureCoordinateSphereP(this);

  SO_NODE_INTERNAL_CONSTRUCTOR(SoTextureCoordinateSphere);

  pimpl->so_texcoord_storage = new SbStorage(sizeof(so_texcoordsphere_data),
                                             so_texcoordsphere_construct_data,
                                             so_texcoordsphere_destruct_data);
}

/*!
  Destructor.
*/
SoTextureCoordinateSphere::~SoTextureCoordinateSphere()
{
  delete pimpl->so_texcoord_storage;
  delete PRIVATE(this);
}

// Documented in superclass.
/*!
  \copybrief SoBase::initClass(void)
*/
void
SoTextureCoordinateSphere::initClass(void)
{
  SO_NODE_INTERNAL_INIT_CLASS(SoTextureCoordinateSphere, SO_FROM_COIN_2_3);

  SO_ENABLE(SoGLRenderAction, SoGLMultiTextureCoordinateElement);
  SO_ENABLE(SoCallbackAction, SoMultiTextureCoordinateElement);
  SO_ENABLE(SoPickAction, SoMultiTextureCoordinateElement);

}

const SbVec4f &
textureCoordinateSphereCallback(void * userdata,
                          const SbVec3f & point,
                          const SbVec3f & normal)
{

  SoTextureCoordinateSphereP * pimpl = (SoTextureCoordinateSphereP *) userdata;
  so_texcoordsphere_data * data = pimpl->so_texcoord_get_data();

  SoState * state = data->currentstate;
  SoFullPath * path = (SoFullPath *) state->getAction()->getCurPath();
  SoNode * node = path->getTail();


  if (!node->isOfType(SoShape::getClassTypeId())) {
    // FIXME: A better way to handle this? (20040122 handegar)
    assert(FALSE && "TextureCoordinateSphere callback called for a non-SoShape node.");
  }

  // Cast the node into a shape
  SoShape * shape = (SoShape *) node;

  if (shape != data->currentshape) {
    data->boundingbox.makeEmpty();
    const SoBoundingBoxCache * bboxcache = shape->getBoundingBoxCache();
    if (bboxcache && bboxcache->isValid(state)) {
      data->boundingbox = bboxcache->getProjectedBox();
      data->origo = data->boundingbox.getCenter();
    }
    else {
      shape->computeBBox(state->getAction(), data->boundingbox, data->origo);
      data->origo = data->boundingbox.getCenter();
    }
    data->currentshape = shape;
  }

  const SbVec4f & ret = pimpl->calculateTextureCoordinate(point, normal);

  data->texcoordreturn = ret;
  return data->texcoordreturn;

}

SbVec4f
SoTextureCoordinateSphereP::calculateTextureCoordinate(const SbVec3f & point, const SbVec3f & COIN_UNUSED_ARG(n))
{

  // FIXME: This way of mapping will always lead to artifacts in the
  // change between 360 and 0 degrees around the Y-axis. This is
  // unavoidable as the callback cannot predict when the last vertex
  // will be received, and therefore be able to patch up the
  // transition. (20040127 handegar)

  SbVec4f tc((float) (atan2(point[0], point[2]) * (1.0/(2.0*M_PI)) + 0.5),
             (float) (atan2(point[1], sqrt(point[0]*point[0] + point[2]*point[2])) * (1.0/M_PI) + 0.5),
             0.0f, 1.0f);

  return tc;

}


// Documented in superclass.
void
SoTextureCoordinateSphere::doAction(SoAction * action)
{
  so_texcoordsphere_data * data = PRIVATE(this)->so_texcoord_get_data();
  
  data->currentstate = action->getState();
  data->currentshape = NULL;
  
  int unit = SoTextureUnitElement::get(data->currentstate);
  SoMultiTextureCoordinateElement::setFunction(data->currentstate, this,
                                               unit, textureCoordinateSphereCallback,
                                               PRIVATE(this));
}

// Documented in superclass.
void
SoTextureCoordinateSphere::GLRender(SoGLRenderAction * action)
{
  so_texcoordsphere_data * data = PRIVATE(this)->so_texcoord_get_data();

  data->currentstate = action->getState();
  data->currentshape = NULL;

  int unit = SoTextureUnitElement::get(data->currentstate);
  const cc_glglue * glue = cc_glglue_instance(SoGLCacheContextElement::get(action->getState()));
  int maxunits = cc_glglue_max_texture_units(glue);
  if (unit < maxunits) {        
    SoMultiTextureCoordinateElement::setFunction(data->currentstate, this,
                                                 unit, textureCoordinateSphereCallback,
                                                 PRIVATE(this));
  }
}

// Documented in superclass.
void
SoTextureCoordinateSphere::callback(SoCallbackAction * action)
{
  SoTextureCoordinateSphere::doAction((SoAction *)action);
}

// Documented in superclass.
void
SoTextureCoordinateSphere::pick(SoPickAction * action)
{
  SoTextureCoordinateSphere::doAction((SoAction *)action);
}

#undef PRIVATE
#undef PUBLIC
