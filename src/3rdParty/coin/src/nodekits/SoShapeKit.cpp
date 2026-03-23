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
  \class SoShapeKit SoShapeKit.h Inventor/nodekits/SoShapeKit.h
  \brief The SoShapeKit class provides templates to insert what is usually needed for shape nodes.

  \ingroup coin_nodekits

  \NODEKIT_PRE_DIAGRAM

  \verbatim
  CLASS SoShapeKit
  -->"this"
        "callbackList"
        "topSeparator"
           "pickStyle"
           "appearance"
           "units"
           "transform"
           "texture2Transform"
  -->      "materialBinding"
  -->      "normalBinding"
  -->      "textureCoordinateBinding"
  -->      "shapeHints"
  -->      "coordinate3"
  -->      "coordinate4"
  -->      "normal"
  -->      "textureCoordinate2"
  -->      "profileCoordinate2"
  -->      "profileCoordinate3"
  -->      "profileList"
           "childList"
  -->      "textureCoordinateFunction"
  -->      "localTransform"
  -->      "shapeSeparator"
  -->         "shape"
  \endverbatim

  \NODEKIT_POST_DIAGRAM


  \NODEKIT_PRE_TABLE

  \verbatim
  CLASS SoShapeKit
  PVT   "this",  SoShapeKit  --- 
        "callbackList",  SoNodeKitListPart [ SoCallback, SoEventCallback ] 
  PVT   "topSeparator",  SoSeparator  --- 
        "pickStyle",  SoPickStyle  --- 
        "appearance",  SoAppearanceKit  --- 
        "units",  SoUnits  --- 
        "transform",  SoTransform  --- 
        "texture2Transform",  SoTexture2Transform  --- 
        "materialBinding",  SoMaterialBinding  --- 
        "normalBinding",  SoNormalBinding  --- 
        "textureCoordinateBinding",  SoTextureCoordinateBinding  --- 
        "shapeHints",  SoShapeHints  --- 
        "coordinate3",  SoCoordinate3  --- 
        "coordinate4",  SoCoordinate4  --- 
        "normal",  SoNormal  --- 
        "textureCoordinate2",  SoTextureCoordinate2  --- 
        "profileCoordinate2",  SoProfileCoordinate2  --- 
        "profileCoordinate3",  SoProfileCoordinate3  --- 
        "profileList",  SoNodeKitListPart [ SoProfile ] 
        "childList",  SoNodeKitListPart [ SoShapeKit, SoSeparatorKit ] 
        "textureCoordinateFunction",  SoTextureCoordinateFunction  --- , (default type = SoTextureCoordinateDefault)
        "localTransform",  SoTransform  --- 
  PVT   "shapeSeparator",  SoSeparator  --- 
        "shape",  SoShape  --- , (default type = SoCube)
  \endverbatim

  \NODEKIT_POST_TABLE
*/

#include <Inventor/nodekits/SoShapeKit.h>

#include <Inventor/nodekits/SoNodeKitListPart.h>
#include <Inventor/nodes/SoMaterialBinding.h>
#include <Inventor/nodes/SoNormalBinding.h>
#include <Inventor/nodes/SoTextureCoordinateBinding.h>
#include <Inventor/nodes/SoShapeHints.h>
#include <Inventor/nodes/SoCoordinate3.h>
#include <Inventor/nodes/SoCoordinate4.h>
#include <Inventor/nodes/SoNormal.h>
#include <Inventor/nodes/SoTextureCoordinate2.h>
#include <Inventor/nodes/SoProfileCoordinate2.h>
#include <Inventor/nodes/SoProfileCoordinate3.h>
#include <Inventor/nodes/SoTextureCoordinateDefault.h>
#include <Inventor/nodes/SoTransform.h>
#include <Inventor/nodes/SoSeparator.h>
#include <Inventor/nodes/SoCube.h>
#include <Inventor/nodes/SoProfile.h>

#include "nodekits/SoSubKitP.h"


SO_KIT_SOURCE(SoShapeKit);


/*!
  Constructor.
*/
SoShapeKit::SoShapeKit(void)
{
  SO_KIT_INTERNAL_CONSTRUCTOR(SoShapeKit);

  // Note: we must use "" instead of , , to humour MS VisualC++ 6.

  SO_KIT_ADD_CATALOG_ENTRY(materialBinding, SoMaterialBinding, TRUE, topSeparator, normalBinding, TRUE);
  SO_KIT_ADD_CATALOG_ENTRY(normalBinding, SoNormalBinding, TRUE, topSeparator, textureCoordinateBinding, TRUE);
  SO_KIT_ADD_CATALOG_ENTRY(textureCoordinateBinding, SoTextureCoordinateBinding, TRUE, topSeparator, shapeHints, TRUE);
  SO_KIT_ADD_CATALOG_ENTRY(shapeHints, SoShapeHints, TRUE, topSeparator, coordinate3, TRUE);
  SO_KIT_ADD_CATALOG_ENTRY(coordinate3, SoCoordinate3, TRUE, topSeparator, coordinate4, TRUE);
  SO_KIT_ADD_CATALOG_ENTRY(coordinate4, SoCoordinate4, TRUE, topSeparator, normal, TRUE);
  SO_KIT_ADD_CATALOG_ENTRY(normal, SoNormal, TRUE, topSeparator, textureCoordinate2, TRUE);
  SO_KIT_ADD_CATALOG_ENTRY(textureCoordinate2, SoTextureCoordinate2, TRUE, topSeparator, profileCoordinate2, TRUE);
  SO_KIT_ADD_CATALOG_ABSTRACT_ENTRY(textureCoordinateFunction, SoTextureCoordinateFunction, SoTextureCoordinateDefault, TRUE, topSeparator, localTransform, TRUE);
  SO_KIT_ADD_CATALOG_ENTRY(profileCoordinate2, SoProfileCoordinate2, TRUE, topSeparator, profileCoordinate3, TRUE);
  SO_KIT_ADD_CATALOG_ENTRY(profileCoordinate3, SoProfileCoordinate3, TRUE, topSeparator, profileList, TRUE);
  SO_KIT_ADD_CATALOG_LIST_ENTRY(profileList, SoGroup, TRUE, topSeparator, childList, SoProfile, TRUE);
  SO_KIT_ADD_CATALOG_ENTRY(localTransform, SoTransform, TRUE, topSeparator, shapeSeparator, TRUE);
  SO_KIT_ADD_CATALOG_ENTRY(shapeSeparator, SoSeparator, TRUE, topSeparator, "", FALSE);
  SO_KIT_ADD_CATALOG_ABSTRACT_ENTRY(shape, SoShape, SoCube, FALSE, shapeSeparator, "", TRUE);

  SO_KIT_INIT_INSTANCE();
}

/*!
  Destructor.
*/
SoShapeKit::~SoShapeKit()
{
}

/*!
  \copydetails SoNode::initClass(void)
*/
void
SoShapeKit::initClass(void)
{
  SO_KIT_INTERNAL_INIT_CLASS(SoShapeKit, SO_FROM_INVENTOR_1);
}

// Documented in superclass.
void
SoShapeKit::setDefaultOnNonWritingFields(void)
{
  this->shapeSeparator.setDefault(TRUE);
  inherited::setDefaultOnNonWritingFields();
}

#endif // HAVE_NODEKITS
