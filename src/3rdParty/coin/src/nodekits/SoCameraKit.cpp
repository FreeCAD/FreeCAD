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
  \class SoCameraKit SoCameraKit.h Inventor/nodekits/SoCameraKit.h
  \brief The SoCameraKit class is a node kit with a transform and a camera.

  \ingroup coin_nodekits

  \NODEKIT_PRE_DIAGRAM

  \verbatim
  CLASS SoCameraKit
  -->"this"
        "callbackList"
  -->   "transformGroup"
  -->      "transform"
  -->      "camera"
  \endverbatim

  \NODEKIT_POST_DIAGRAM


  \NODEKIT_PRE_TABLE

  \verbatim
  CLASS SoCameraKit
  PVT   "this",  SoCameraKit  --- 
        "callbackList",  SoNodeKitListPart [ SoCallback, SoEventCallback ] 
  PVT   "transformGroup",  SoTransformSeparator  --- 
        "transform",  SoTransform  --- 
        "camera",  SoCamera  --- , (default type = SoPerspectiveCamera)
  \endverbatim

  \NODEKIT_POST_TABLE
*/

#include <Inventor/nodekits/SoCameraKit.h>

#include <Inventor/nodes/SoTransformSeparator.h>
#include <Inventor/nodes/SoTransform.h>
#include <Inventor/nodes/SoPerspectiveCamera.h>

#include "nodekits/SoSubKitP.h"

SO_KIT_SOURCE(SoCameraKit);


/*!
  Constructor.
*/
SoCameraKit::SoCameraKit(void)
{
  SO_KIT_INTERNAL_CONSTRUCTOR(SoCameraKit);

  // Note: we must use "" instead of , , to humour MS VisualC++ 6.

  SO_KIT_ADD_CATALOG_ENTRY(transformGroup, SoTransformSeparator, TRUE, this, "", FALSE);
  SO_KIT_ADD_CATALOG_ENTRY(transform, SoTransform, TRUE, transformGroup, camera, TRUE);
  SO_KIT_ADD_CATALOG_ABSTRACT_ENTRY(camera, SoCamera, SoPerspectiveCamera, FALSE, transformGroup, "", TRUE);

  SO_KIT_INIT_INSTANCE();
}

/*!
  Destructor.
*/
SoCameraKit::~SoCameraKit()
{
}

/*!
  \copydetails SoNode::initClass(void)
*/
void
SoCameraKit::initClass(void)
{
  SO_KIT_INTERNAL_INIT_CLASS(SoCameraKit, SO_FROM_INVENTOR_1);
}

#endif // HAVE_NODEKITS
