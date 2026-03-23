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
  \class SoWrapperKit SoWrapperKit.h Inventor/nodekits/SoWrapperKit.h
  \brief The SoWrapperKit class is a simple kit for wrapping a transform and a subgraph.

  \ingroup coin_nodekits

  
  \NODEKIT_PRE_DIAGRAM

  \verbatim
  CLASS SoWrapperKit
  -->"this"
        "callbackList"
        "topSeparator"
           "pickStyle"
           "appearance"
           "units"
           "transform"
           "texture2Transform"
           "childList"
  -->      "localTransform"
  -->      "contents"
  \endverbatim

  \NODEKIT_POST_DIAGRAM


  \NODEKIT_PRE_TABLE

  \verbatim
  CLASS SoWrapperKit
  PVT   "this",  SoWrapperKit  --- 
        "callbackList",  SoNodeKitListPart [ SoCallback, SoEventCallback ] 
  PVT   "topSeparator",  SoSeparator  --- 
        "pickStyle",  SoPickStyle  --- 
        "appearance",  SoAppearanceKit  --- 
        "units",  SoUnits  --- 
        "transform",  SoTransform  --- 
        "texture2Transform",  SoTexture2Transform  --- 
        "childList",  SoNodeKitListPart [ SoShapeKit, SoSeparatorKit ] 
        "localTransform",  SoTransform  --- 
        "contents",  SoSeparator  --- 
  \endverbatim

  \NODEKIT_POST_TABLE
*/

#include <Inventor/nodekits/SoWrapperKit.h>

#include <Inventor/nodes/SoSeparator.h>
#include <Inventor/nodes/SoTransform.h>

#include "nodekits/SoSubKitP.h"

SO_KIT_SOURCE(SoWrapperKit);


/*!
  Constructor.
*/
SoWrapperKit::SoWrapperKit(void)
{
  SO_KIT_INTERNAL_CONSTRUCTOR(SoWrapperKit);

  // Note: we must use "" instead of , , to humour MS VisualC++ 6.

  SO_KIT_ADD_CATALOG_ENTRY(localTransform, SoTransform, TRUE, topSeparator, contents, TRUE);
  SO_KIT_ADD_CATALOG_ENTRY(contents, SoSeparator, TRUE, topSeparator, "", TRUE);

  SO_KIT_INIT_INSTANCE();
}

/*!
  Destructor.
*/
SoWrapperKit::~SoWrapperKit()
{
}

/*!
  \copydetails SoNode::initClass(void)
*/
void
SoWrapperKit::initClass(void)
{
  SO_KIT_INTERNAL_INIT_CLASS(SoWrapperKit, SO_FROM_INVENTOR_1);
}

#endif // HAVE_NODEKITS
