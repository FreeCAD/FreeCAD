#ifndef COIN_SOSTLFILEKIT_H
#define COIN_SOSTLFILEKIT_H

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

#include <Inventor/SbBasic.h>

#include <Inventor/fields/SoSFString.h>
#include <Inventor/fields/SoSFBool.h>
#include <Inventor/fields/SoSFEnum.h>

#include <Inventor/annex/ForeignFiles/SoForeignFileKit.h>

class SbVec3f;
class SoCallbackAction;
class SoPrimitiveVertex;
class SoSTLFileKitP;

class COIN_DLL_API SoSTLFileKit : public SoForeignFileKit {
  typedef SoForeignFileKit inherited;

  SO_KIT_HEADER(SoSTLFileKit);
  SO_KIT_CATALOG_ENTRY_HEADER(shapehints);
  SO_KIT_CATALOG_ENTRY_HEADER(texture);
  SO_KIT_CATALOG_ENTRY_HEADER(normalbinding);
  SO_KIT_CATALOG_ENTRY_HEADER(normals);
  SO_KIT_CATALOG_ENTRY_HEADER(materialbinding);
  SO_KIT_CATALOG_ENTRY_HEADER(material);
  SO_KIT_CATALOG_ENTRY_HEADER(coordinates);
  SO_KIT_CATALOG_ENTRY_HEADER(facets);

public:
  static void initClass(void);
  SoSTLFileKit(void);

  enum Colorization { GREY, MATERIALISE, TNO_VISICAM };

  SoSFString info;
  SoSFBool binary;
  SoSFEnum colorization;

  static SbBool identify(const char * filename);
  virtual SbBool canReadFile(const char * filename = NULL) const;
  virtual SbBool readFile(const char * filename);

  virtual SbBool canWriteFile(const char * filename = NULL) const;
  virtual SbBool writeFile(const char * filename);


  SbBool canReadScene(void) const;
  SbBool readScene(SoNode * scene);
  virtual SoSeparator *convert();

protected:
  virtual ~SoSTLFileKit(void);

  void reset(void);
  SbBool addFacet(const SbVec3f & v1, const SbVec3f & v2, const SbVec3f & v3,
                  const SbVec3f & normal);
  void organizeModel(void);

private:
  SoSTLFileKitP * pimpl;

  static void add_facet_cb(void * closure, SoCallbackAction * action, const SoPrimitiveVertex * v1, const SoPrimitiveVertex * v2, const SoPrimitiveVertex * v3);
  static void put_facet_cb(void * closure, SoCallbackAction * action, const SoPrimitiveVertex * v1, const SoPrimitiveVertex * v2, const SoPrimitiveVertex * v3);

}; // SoSTLFileKit

#endif // !COIN_SOSTLFILEKIT_H
