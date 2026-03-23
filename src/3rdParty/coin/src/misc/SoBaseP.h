#ifndef COIN_SOBASEP_H
#define COIN_SOBASEP_H

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

#include "misc/SbHash.h"

class SoBase;
class SoNode;
class SoAuditorList;
class SbPList;
class SoInput;

#ifndef COIN_INTERNAL
#error this is a private header file
#endif /* !COIN_INTERNAL */

// FIXME: should implement and use a proper set-abstraction
// datatype. 20050524 mortene.
typedef SbHash<const SoBase *, void *> SoBaseSet;

class SoBase::PImpl {
public:
  static const char OPEN_BRACE;
  static const char CLOSE_BRACE;

  static const char END_OF_LINE[];
  static const char DEF_KEYWORD[];
  static const char USE_KEYWORD[];
  static const char NULL_KEYWORD[];
  static const char ROUTE_KEYWORD[];
  static const char PROTO_KEYWORD[];
  static const char EXTERNPROTO_KEYWORD[];

  static void * mutex;
  static void * name2obj_mutex;
  static void * obj2name_mutex;
  static void * auditor_mutex;
  static void * global_mutex;

  static SbHash<const SoBase *, SoAuditorList *> * auditordict;
  static SbHash<const char *, SbPList *> * name2obj;
  static SbHash<const SoBase *, const char *> * obj2name;

  static SbBool trackbaseobjects;
  static void * allbaseobj_mutex;
  static SoBaseSet * allbaseobj; // maps from SoBase * to NULL

  static SbString * refwriteprefix;
  static SbBool tracerefs;
  static uint32_t writecounter;

  static void cleanup_auditordict(void);

  static void removeName2Obj(SoBase * const base, const char * const name);
  static void removeObj2Name(SoBase * const base, const char * const name);

  static void check_for_leaks(void);

  static SbBool readReference(SoInput * in, SoBase *& base);
  static SbBool readBase(SoInput * in, SbName & classname, SoBase *& base);
  static SbBool readBaseInstance(SoInput * in, const SbName & classname,
                                 const SbName & refname, SoBase *& base);

  static SoBase * createInstance(SoInput * in, const SbName & classname);
  static void flushInput(SoInput * in);

  static void rbptree_notify_cb(void * auditor, void * type, void * closure);

  static SoNode * readNode(SoInput * in);

  // only needed for the callback from cc_rbptree_traverse
  struct NotifyData {
    int cnt;
    int total;
    SoNotList * list;
    SoBase * thisp;
  };

}; // SoBase::PImpl

#endif // !COIN_SOBASEP_H
