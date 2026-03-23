#ifndef COIN_SOSHAPEKIT_H
#define COIN_SOSHAPEKIT_H

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

#include <Inventor/nodekits/SoSeparatorKit.h>

class COIN_DLL_API SoShapeKit : public SoSeparatorKit {
  typedef SoSeparatorKit inherited;

  SO_KIT_HEADER(SoShapeKit);

  SO_KIT_CATALOG_ENTRY_HEADER(coordinate3);
  SO_KIT_CATALOG_ENTRY_HEADER(coordinate4);
  SO_KIT_CATALOG_ENTRY_HEADER(localTransform);
  SO_KIT_CATALOG_ENTRY_HEADER(materialBinding);
  SO_KIT_CATALOG_ENTRY_HEADER(normal);
  SO_KIT_CATALOG_ENTRY_HEADER(normalBinding);
  SO_KIT_CATALOG_ENTRY_HEADER(profileCoordinate2);
  SO_KIT_CATALOG_ENTRY_HEADER(profileCoordinate3);
  SO_KIT_CATALOG_ENTRY_HEADER(profileList);
  SO_KIT_CATALOG_ENTRY_HEADER(shape);
  SO_KIT_CATALOG_ENTRY_HEADER(shapeHints);
  SO_KIT_CATALOG_ENTRY_HEADER(shapeSeparator);
  SO_KIT_CATALOG_ENTRY_HEADER(textureCoordinate2);
  SO_KIT_CATALOG_ENTRY_HEADER(textureCoordinateBinding);
  SO_KIT_CATALOG_ENTRY_HEADER(textureCoordinateFunction);

public:
  SoShapeKit(void);

  static void initClass(void);

protected:
  virtual ~SoShapeKit();

  virtual void setDefaultOnNonWritingFields(void);
};

#endif // !COIN_SOSHAPEKIT_H
