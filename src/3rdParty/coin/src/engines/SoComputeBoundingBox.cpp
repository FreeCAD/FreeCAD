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
  \class SoComputeBoundingBox SoComputeBoundingBox.h Inventor/engines/SoComputeBoundingBox.h
  \brief The SoComputeBoundingBox class is used to calculate a bounding box.

  \ingroup coin_engines

  This engine is simply a wrapper around the SoGetBoundingBoxAction,
  for a convenient way of having automatic updating of some data in
  the scene graph which is dependent on the bounding box of some other
  part of the scene.
*/

// FIXME: wouldn't it be better design to keep the SbViewportRegion of
// the SoGetBoundingBoxAction (and other data of the action) as
// inputs, so they are written and copied automatically? 20000922 mortene.

#include <Inventor/engines/SoComputeBoundingBox.h>
#include <Inventor/lists/SoEngineOutputList.h>
#include <Inventor/fields/SoSFVec3f.h>
#include <Inventor/actions/SoGetBoundingBoxAction.h>
#include <Inventor/SbViewportRegion.h>

#include "engines/SoSubEngineP.h"

SO_ENGINE_SOURCE(SoComputeBoundingBox);

/*!
  \var SoSFNode SoComputeBoundingBox::node

  If this input field is set and SoComputeBoundingBox::path is \c
  NULL, the resultant bounding box will be the box encompassing the
  scene graph rooted at this node pointer.
*/
/*!
  \var SoSFPath SoComputeBoundingBox::path

  If this input field is not \c NULL, the bounding box values of the
  given path will be set on the outputs.
*/
/*!
  \var SoEngineOutput SoComputeBoundingBox::min
  (SoSFVec3f) Corner coordinates of the bounding box.
*/
/*!
  \var SoEngineOutput SoComputeBoundingBox::max
  (SoSFVec3f) Corner coordinates of the bounding box.
*/
/*!
  \var SoEngineOutput SoComputeBoundingBox::boxCenter
  (SoSFVec3f) Geometric center point of the bounding box.
*/
/*!
  \var SoEngineOutput SoComputeBoundingBox::objectCenter

  (SoSFVec3f) Object center point for the bounding box. See
  SoGetBoundingBoxAction::getCenter() for an explanation for how this
  can differ from the geometric center point of the bounding box.
*/


/*!
  Default constructor. Sets up the internal SoGetBoundingBoxAction
  instance.
*/
SoComputeBoundingBox::SoComputeBoundingBox(void)
{
  SO_ENGINE_INTERNAL_CONSTRUCTOR(SoComputeBoundingBox);

  SO_ENGINE_ADD_INPUT(node,(NULL));
  SO_ENGINE_ADD_INPUT(path,(NULL));

  SO_ENGINE_ADD_OUTPUT(min, SoSFVec3f);
  SO_ENGINE_ADD_OUTPUT(max, SoSFVec3f);
  SO_ENGINE_ADD_OUTPUT(boxCenter, SoSFVec3f);
  SO_ENGINE_ADD_OUTPUT(objectCenter, SoSFVec3f);

  // Start with a default viewportregion.
  this->bboxaction = new SoGetBoundingBoxAction(SbViewportRegion());
}

/*!
  \copybrief SoBase::initClass(void)
*/
void
SoComputeBoundingBox::initClass(void)
{
  SO_ENGINE_INTERNAL_INIT_CLASS(SoComputeBoundingBox);
}

/*!
  Destructor. Deallocate the SoGetBoundingBoxAction instance.
*/
SoComputeBoundingBox::~SoComputeBoundingBox()
{
  delete this->bboxaction;
}

// doc in parent
void
SoComputeBoundingBox::evaluate(void)
{
  SoPath * bboxpath = this->path.getValue();
  SoNode * bboxnode = this->node.getValue();

  if (!bboxpath && !bboxnode) {
    // Nothing to see, move along.. (we "un-dirty" the fields to avoid
    // them being re-evaluated again and again while the engine inputs
    // are NULL).
    SO_ENGINE_OUTPUT(min, SoSFVec3f, setDirty(FALSE));
    SO_ENGINE_OUTPUT(max, SoSFVec3f, setDirty(FALSE));
    SO_ENGINE_OUTPUT(boxCenter, SoSFVec3f, setDirty(FALSE));
    SO_ENGINE_OUTPUT(objectCenter, SoSFVec3f, setDirty(FALSE));
    return;
  }

  if (bboxpath) this->bboxaction->apply(bboxpath);
  else this->bboxaction->apply(bboxnode);

  SbBox3f box = this->bboxaction->getBoundingBox();
  SO_ENGINE_OUTPUT(min, SoSFVec3f, setValue(box.getMin()));
  SO_ENGINE_OUTPUT(max, SoSFVec3f, setValue(box.getMax()));
  SO_ENGINE_OUTPUT(boxCenter, SoSFVec3f, setValue(box.getCenter()));

  const SbVec3f & center = this->bboxaction->getCenter();
  SO_ENGINE_OUTPUT(objectCenter, SoSFVec3f, setValue(center));
}

/*!
  Set viewport region for the SoGetBoundingBoxAction instance we're
  using for calculating bounding boxes.

  The default setting is to use an SbViewportRegion with only default
  values.
 */
void
SoComputeBoundingBox::setViewportRegion(const SbViewportRegion & vpr)
{
  this->bboxaction->setViewportRegion(vpr);
}

/*!
  Returns viewport region used by the internal SoGetBoundingBoxAction
  instance.
 */
const SbViewportRegion &
SoComputeBoundingBox::getViewportRegion(void) const
{
  return this->bboxaction->getViewportRegion();
}
