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
  \class SoAppearanceKit SoAppearanceKit.h Inventor/nodekits/SoAppearanceKit.h
  \brief The SoAppearanceKit class is a node kit catalog that collects miscellaneous appearance node types.

  \ingroup coin_nodekits

  \NODEKIT_PRE_DIAGRAM

  \verbatim
  CLASS SoAppearanceKit
  -->"this"
        "callbackList"
  -->   "lightModel"
  -->   "environment"
  -->   "drawStyle"
  -->   "material"
  -->   "complexity"
  -->   "texture2"
  -->   "font"
  \endverbatim

  \NODEKIT_POST_DIAGRAM


  \NODEKIT_PRE_TABLE

  \verbatim
  CLASS SoAppearanceKit
  PVT   "this",  SoAppearanceKit  --- 
        "callbackList",  SoNodeKitListPart [ SoCallback, SoEventCallback ] 
        "lightModel",  SoLightModel  --- 
        "environment",  SoEnvironment  --- 
        "drawStyle",  SoDrawStyle  --- 
        "material",  SoMaterial  --- 
        "complexity",  SoComplexity  --- 
        "texture2",  SoTexture2  --- 
        "font",  SoFont  --- 
  \endverbatim

  \NODEKIT_POST_TABLE
*/


#include <Inventor/nodekits/SoAppearanceKit.h>

#include <Inventor/nodes/SoLightModel.h>
#include <Inventor/nodes/SoEnvironment.h>
#include <Inventor/nodes/SoDrawStyle.h>
#include <Inventor/nodes/SoMaterial.h>
#include <Inventor/nodes/SoComplexity.h>
#include <Inventor/nodes/SoTexture2.h>
#include <Inventor/nodes/SoFont.h>

#include "nodekits/SoSubKitP.h"

SO_KIT_SOURCE(SoAppearanceKit);


/*!
  Constructor.
*/
SoAppearanceKit::SoAppearanceKit(void)
{
  SO_KIT_INTERNAL_CONSTRUCTOR(SoAppearanceKit);

  // Note: we must use "" instead of , , to humour MS VisualC++ 6.

  SO_KIT_ADD_CATALOG_ENTRY(lightModel, SoLightModel, TRUE, this, environment, TRUE);
  SO_KIT_ADD_CATALOG_ENTRY(environment, SoEnvironment, TRUE, this, drawStyle, TRUE);
  SO_KIT_ADD_CATALOG_ENTRY(drawStyle, SoDrawStyle, TRUE, this, material, TRUE);
  SO_KIT_ADD_CATALOG_ENTRY(material, SoMaterial, TRUE, this, complexity, TRUE);
  SO_KIT_ADD_CATALOG_ENTRY(complexity, SoComplexity, TRUE, this, texture2, TRUE);
  SO_KIT_ADD_CATALOG_ENTRY(texture2, SoTexture2, TRUE, this, font, TRUE);
  SO_KIT_ADD_CATALOG_ENTRY(font, SoFont, TRUE, this, "", TRUE);

  SO_KIT_INIT_INSTANCE();
}

/*!
  Destructor.
*/
SoAppearanceKit::~SoAppearanceKit()
{
}

/*!
  \copydetails SoNode::initClass(void)
*/
void
SoAppearanceKit::initClass(void)
{
  SO_KIT_INTERNAL_INIT_CLASS(SoAppearanceKit, SO_FROM_INVENTOR_1);
}

#endif // HAVE_NODEKITS
