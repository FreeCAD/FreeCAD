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

#include "SoSceneManagerP.h"
#include "coindefs.h"

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif // HAVE_CONFIG_H

#include <Inventor/SoDB.h>
#include <Inventor/system/gl.h>
#include <Inventor/nodes/SoCamera.h>
#include <Inventor/actions/SoGLRenderAction.h>
#include <Inventor/actions/SoSearchAction.h>
#include <Inventor/sensors/SoNodeSensor.h>
#ifdef HAVE_NODEKITS
#include <Inventor/nodekits/SoBaseKit.h>
#endif // HAVE_NODEKITS


#define PRIVATE(p) (p->pimpl)
#define PUBLIC(p) (p->publ)

SoSceneManagerP::SoSceneManagerP(SoSceneManager * publ) 
{
  PUBLIC(this) = publ;
  this->searchaction = new SoSearchAction;
}

SoSceneManagerP::~SoSceneManagerP()
{
  delete this->searchaction;
}

// defcamera is the default camera returned if none is found in root

SoCamera * 
SoSceneManagerP::searchForCamera(SoNode * root,
                                 SoCamera * defcamera)
{
  this->searchaction->setType(SoCamera::getClassTypeId());
  this->searchaction->setInterest(SoSearchAction::FIRST);
#ifdef HAVE_NODEKITS
  SbBool old = SoBaseKit::isSearchingChildren();
  SoBaseKit::setSearchingChildren(TRUE);
#endif // HAVE_NODEKITS
  this->searchaction->apply(root);
#ifdef HAVE_NODEKITS
  SoBaseKit::setSearchingChildren(old);
#endif // HAVE_NODEKITS
  SoFullPath * path = (SoFullPath*) this->searchaction->getPath();
  if (path) {
    SoNode * tail = path->getTail();
    this->searchaction->reset();
    return (SoCamera*) tail;
  }
  return defcamera;
}

void 
SoSceneManagerP::renderCB(void * userdata, class SoRenderManager * COIN_UNUSED_ARG(mgr))
{
  SoSceneManagerP * thisp = (SoSceneManagerP *) userdata;
  assert(thisp);
  if (thisp->rendercb) {
    thisp->rendercb(thisp->rendercbdata, PUBLIC(thisp));
  }
}

#undef PRIVATE
#undef PUBLIC
