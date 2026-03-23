#ifndef COIN_SOINTERACTIONKIT_H
#define COIN_SOINTERACTIONKIT_H

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

#include <Inventor/nodekits/SoSubKit.h>
#include <Inventor/nodekits/SoBaseKit.h>
#include <Inventor/fields/SoSFEnum.h>

#ifdef COIN_INTERNAL
class SoFieldSensor;
#else // !COIN_INTERNAL
// Include this header file for better Open Inventor compatibility.
#include <Inventor/sensors/SoFieldSensor.h>
#endif // !COIN_INTERNAL

class SoSensor;
class SoSeparator;

class COIN_DLL_API SoInteractionKit : public SoBaseKit {
  typedef SoBaseKit inherited;

  SO_KIT_HEADER(SoInteractionKit);

  SO_KIT_CATALOG_ENTRY_HEADER(geomSeparator);
  SO_KIT_CATALOG_ENTRY_HEADER(topSeparator);

public:
  SoSFEnum renderCaching;
  SoSFEnum boundingBoxCaching;
  SoSFEnum renderCulling;
  SoSFEnum pickCulling;

public:
  SoInteractionKit(void);
  static void initClass(void);

  enum CacheEnabled { OFF, ON, AUTO };

  virtual SbBool setPartAsPath(const SbName &partname,
                               SoPath *path);
  virtual SbBool setPartAsDefault(const SbName &partname,
                                  SoNode *node,
                                  SbBool onlyifdefault = TRUE);
  virtual SbBool setPartAsDefault(const SbName &partname,
                                  const SbName &nodename,
                                  SbBool onlyifdefault = TRUE);
  SbBool isPathSurrogateInMySubgraph(const SoPath *path,
                                     SoPath *&pathToOwner,
                                     SbName  &surrogatename,
                                     SoPath *&surrogatepath,
                                     SbBool fillargs = TRUE);
  SbBool isPathSurrogateInMySubgraph(const SoPath *path);
  static void setSwitchValue(SoNode *node, const int newVal);
  virtual SbBool setPart(const SbName & partname, SoNode * from);

protected:
  virtual ~SoInteractionKit();
  virtual void copyContents(const SoFieldContainer *fromFC,
                            SbBool copyConnections);

  virtual SbBool setPart(const int partNum, SoNode *node);
  virtual SbBool readInstance(SoInput *in, unsigned short flags);
  static void readDefaultParts(const char *fileName,
                               const char defaultBuffer[],
                               int defBufSize);
  virtual SbBool setAnyPartAsDefault(const SbName &partname,
                                     SoNode *node,
                                     SbBool anypart = TRUE,
                                     SbBool onlyifdefault = TRUE);
  virtual SbBool setAnyPartAsDefault(const SbName &partname,
                                     const SbName &nodename,
                                     SbBool anypart = TRUE,
                                     SbBool onlyifdefault = TRUE);
  SbBool setAnySurrogatePath(const SbName &name,
                             SoPath *path,
                             SbBool leafcheck = FALSE,
                             SbBool publiccheck = FALSE);
  virtual SbBool setUpConnections(SbBool onoff, SbBool doitalways = FALSE);
  virtual void setDefaultOnNonWritingFields();

  SoFieldSensor *fieldSensor;
  static void fieldSensorCB(void *, SoSensor *);
  SoSeparator *oldTopSep;

  void connectSeparatorFields( SoSeparator *dest, SbBool onOff );

private:
  class SoInteractionKitP * pimpl;
  friend class SoInteractionKitP;
};

#endif // !COIN_SOINTERACTIONKIT_H
