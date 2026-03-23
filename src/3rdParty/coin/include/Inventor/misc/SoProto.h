#ifndef COIN_SOPROTO_H
#define COIN_SOPROTO_H

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

#include <Inventor/nodes/SoNode.h>

class SoProto;
class SoInput;
class SoProtoInstance;
class SoProtoP;

typedef SoProto * SoFetchExternProtoCB(SoInput * in,
                                       const SbString * urls,
                                       const int numurls,
                                       void * closure);

// We need to inherit SoNode to be able to insert the PROTO definition
// into the scene graph.
class COIN_DLL_API SoProto : public SoNode {
public:
  SoProto(const SbBool externproto = FALSE);

  static void setFetchExternProtoCallback(SoFetchExternProtoCB * cb,
                                          void * closure);

  virtual SoType getTypeId(void) const;
  static SoType getClassTypeId(void);

  static SoProto * findProto(const SbName & name);
  static void initClass(void);

  SoProtoInstance * createProtoInstance(void);
  void addISReference(SoNode * container,
                      const SbName & fieldname,
                      const SbName & interfacename);
  SbName findISReference(const SoFieldContainer * container,
                         const SbName & fieldname);

  void addReference(const SbName & name, SoBase * base);
  void removeReference(const SbName & name);
  SoBase * findReference(const SbName & name) const;

  void addRoute(const SbName & fromnode, const SbName & fromfield,
                const SbName & tonode, const SbName & tofield);

  SbName getProtoName(void) const;

  virtual SbBool readInstance(SoInput * input, unsigned short flags);
  virtual void write(SoWriteAction * action);

protected:

  virtual ~SoProto();
  virtual void destroy(void);

private:
  SbBool writeInterface(SoOutput * out);
  SbBool writeDefinition(SoWriteAction * action);

  SbBool readInterface(SoInput * in);
  SbBool readDefinition(SoInput * in);

  SbBool writeURLs(SoOutput * out);
  SoProtoP * pimpl;
  friend class SoProtoP;

  SbBool setupExtern(SoInput * in, SoProto * externproto);

  SoNode * createInstanceRoot(SoProtoInstance * inst) const;
  void connectISRefs(SoProtoInstance * inst, SoNode * src, SoNode * dst) const;
};

#endif // !COIN_SOPROTO_H
