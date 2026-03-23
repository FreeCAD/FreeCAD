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

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif // HAVE_CONFIG_H

#ifdef HAVE_NODEKITS

/*!
  \class SoSceneKit SoSceneKit.h Inventor/nodekits/SoSceneKit.h
  \brief The SoSceneKit class collects node kits needed to set up a scene: camera, light and shapes.

  \ingroup coin_nodekits

  \NODEKIT_PRE_DIAGRAM

  \verbatim
  CLASS SoSceneKit
  -->"this"
        "callbackList"
  -->   "topSeparator"
  -->      "cameraList"
  -->      "lightList"
  -->      "childList"
  \endverbatim

  \NODEKIT_POST_DIAGRAM


  \NODEKIT_PRE_TABLE

  \verbatim
  CLASS SoSceneKit
  PVT   "this",  SoSceneKit  --- 
        "callbackList",  SoNodeKitListPart [ SoCallback, SoEventCallback ] 
  PVT   "topSeparator",  SoSeparator  --- 
        "cameraList",  SoNodeKitListPart [ SoCameraKit ] 
        "lightList",  SoNodeKitListPart [ SoLightKit ] 
        "childList",  SoNodeKitListPart [ SoShapeKit, SoSeparatorKit ] 
  \endverbatim

  \NODEKIT_POST_TABLE
*/

#include <Inventor/nodekits/SoSceneKit.h>

#include <Inventor/nodekits/SoCameraKit.h>
#include <Inventor/nodekits/SoLightKit.h>
#include <Inventor/nodekits/SoShapeKit.h>
#include <Inventor/nodekits/SoNodeKitListPart.h>
#include <Inventor/nodes/SoSeparator.h>
#include <Inventor/nodes/SoSwitch.h>
#if COIN_DEBUG
#include <Inventor/errors/SoDebugError.h>
#endif // COIN_DEBUG

#include  "nodekits/SoSubKitP.h"

SO_KIT_SOURCE(SoSceneKit);


/*!
  Constructor.
*/
SoSceneKit::SoSceneKit(void)
{
  SO_KIT_INTERNAL_CONSTRUCTOR(SoSceneKit);

  // Note: we must use "" instead of , , to humour MS VisualC++ 6.

  SO_KIT_ADD_CATALOG_ENTRY(topSeparator, SoSeparator, TRUE, this, "", FALSE);
  SO_KIT_ADD_CATALOG_LIST_ENTRY(cameraList, SoSwitch, TRUE, topSeparator, lightList, SoCameraKit, TRUE);
  SO_KIT_ADD_CATALOG_LIST_ENTRY(lightList, SoGroup, TRUE, topSeparator, childList, SoLightKit, TRUE);
  SO_KIT_ADD_CATALOG_LIST_ENTRY(childList, SoGroup, TRUE, topSeparator, "", SoShapeKit, TRUE);
  SO_KIT_ADD_LIST_ITEM_TYPE(childList, SoSeparatorKit);

  SO_KIT_INIT_INSTANCE();
}

/*!
  Destructor.
*/
SoSceneKit::~SoSceneKit()
{
}

// doc from superclass
void
SoSceneKit::initClass(void)
{
  SO_KIT_INTERNAL_INIT_CLASS(SoSceneKit, SO_FROM_INVENTOR_1);
}

/*!
  Returns the index of the current active camera in cameraList.
*/
int
SoSceneKit::getCameraNumber(void)
{
  SoSwitch *sw = (SoSwitch*)this->getContainerNode(SbName("cameraList"));
  return sw->whichChild.getValue();
}

/*!
  Sets the current active camera in cameraList.
*/
void
SoSceneKit::setCameraNumber(int camnum)
{
  SoSwitch *sw = (SoSwitch*)this->getContainerNode(SbName("cameraList"));
#if COIN_DEBUG && 1 // debug
  if (camnum >= sw->getNumChildren()) {
    SoDebugError::postInfo("SoSceneKit::setCameraNumber",
                           "camera number %d is too large", camnum);
    return;
  }
#endif // debug
  sw->whichChild = camnum;
}

// Documented in superclass.
SbBool
SoSceneKit::affectsState(void) const
{
  return TRUE;
}

#endif // HAVE_NODEKITS
