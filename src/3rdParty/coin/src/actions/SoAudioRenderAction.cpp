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
  \class SoAudioRenderAction SoAudioRenderAction.h Inventor/actions/SoAudioRenderAction.h
  \brief The SoAudioRenderAction class renders the aural parts of the scene graph.

  \ingroup coin_actions

  Applying this method at a root node for a scene graph, path or
  path list will render all sound-related nodes contained within that instance to
  the current SoAudioDevice.
 */


#include <Inventor/actions/SoAudioRenderAction.h>

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif // HAVE_CONFIG_H

#include <Inventor/elements/SoModelMatrixElement.h>
#include <Inventor/elements/SoCoordinateElement.h>
#include <Inventor/elements/SoSwitchElement.h>
#include <Inventor/elements/SoUnitsElement.h>
#include <Inventor/elements/SoFocalDistanceElement.h>
#include <Inventor/elements/SoProjectionMatrixElement.h>
#include <Inventor/elements/SoViewVolumeElement.h>
#include <Inventor/elements/SoViewingMatrixElement.h>
#include <Inventor/elements/SoSoundElement.h>
#include <Inventor/nodes/SoCoordinate3.h>
#include <Inventor/nodes/SoCoordinate4.h>
#include <Inventor/nodes/SoGroup.h>
#include <Inventor/nodes/SoTransform.h>
#include <Inventor/nodes/SoSeparator.h>
#include <Inventor/nodes/SoListener.h>

#ifdef HAVE_VRML97
#include <Inventor/VRMLnodes/SoVRMLSound.h>
#include <Inventor/VRMLnodes/SoVRMLAudioClip.h>
#endif // HAVE_VRML97

#include "SbBasicP.h"
#include "actions/SoSubActionP.h"

class SoAudioRenderActionP
{
public:
};

SO_ACTION_SOURCE(SoAudioRenderAction);

/*!
  \copybrief SoAction::initClass(void)
*/
void SoAudioRenderAction::initClass()
{
  SO_ACTION_INTERNAL_INIT_CLASS(SoAudioRenderAction, SoAction);

  SO_ENABLE(SoAudioRenderAction, SoModelMatrixElement);
  SO_ENABLE(SoAudioRenderAction, SoSwitchElement);
  SO_ENABLE(SoAudioRenderAction, SoUnitsElement);

  SO_ENABLE(SoAudioRenderAction, SoFocalDistanceElement);
  SO_ENABLE(SoAudioRenderAction, SoProjectionMatrixElement);
  SO_ENABLE(SoAudioRenderAction, SoViewVolumeElement);
  SO_ENABLE(SoAudioRenderAction, SoViewingMatrixElement);

  SO_ENABLE(SoAudioRenderAction, SoSoundElement);
}

SoAudioRenderAction::SoAudioRenderAction()
{
  SO_ACTION_CONSTRUCTOR(SoAudioRenderAction);
}

SoAudioRenderAction::~SoAudioRenderAction()
{
}

void SoAudioRenderAction::beginTraversal(SoNode *node)
{
  traverse(node);
}

void SoAudioRenderAction::callDoAction(SoAction *action, SoNode *node)
{
  node->doAction(action);
}

void SoAudioRenderAction::callAudioRender(SoAction *action, SoNode *node)
{
  SoAudioRenderAction * audioRenderAction = coin_assert_cast<SoAudioRenderAction *>(action);

  if (node->isOfType(SoListener::getClassTypeId())) {
    SoListener *listener;
    listener = coin_assert_cast<SoListener *>(node);
    listener->audioRender(audioRenderAction);
  }
#ifdef HAVE_VRML97
  else if (node->isOfType(SoVRMLSound::getClassTypeId())) {
    SoVRMLSound *sound;
    sound = coin_assert_cast<SoVRMLSound *>(node);
    sound->audioRender(audioRenderAction);
  }
  else if (node->isOfType(SoVRMLAudioClip::getClassTypeId())) {
    SoVRMLAudioClip *clip;
    clip = coin_assert_cast<SoVRMLAudioClip *>(node);
    clip->audioRender(audioRenderAction);
  }
#endif // HAVE_VRML97
}

/*
FIXME 20021101 thammer: remember to override invalidateState if we
keep an internal state at all. Called from 
SoSceneManager.setAudioRenderAction
*/
