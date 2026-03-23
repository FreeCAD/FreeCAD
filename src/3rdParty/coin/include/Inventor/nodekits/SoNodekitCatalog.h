#ifndef COIN_SONODEKITCATALOG_H
#define COIN_SONODEKITCATALOG_H

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

#include <Inventor/SoType.h>
#include <Inventor/lists/SbList.h>

class SbName;


#define SO_CATALOG_NAME_NOT_FOUND -1


class COIN_DLL_API SoNodekitCatalog {
public:
  static void initClass(void);

  SoNodekitCatalog(void);
  ~SoNodekitCatalog();

  int getNumEntries(void) const;
  int getPartNumber(const SbName & name) const;
  const SbName & getName(int part) const;

  SoType getType(int part) const;
  SoType getType(const SbName & name) const;
  SoType getDefaultType(int part) const;
  SoType getDefaultType(const SbName & name) const;
  SbBool isNullByDefault(int part) const;
  SbBool isNullByDefault(const SbName & name) const;

  SbBool isLeaf(int part) const;
  SbBool isLeaf(const SbName & name) const;
  const SbName & getParentName(int part) const;
  const SbName & getParentName(const SbName & name) const;
  int getParentPartNumber(int part) const;
  int getParentPartNumber(const SbName & name) const;
  const SbName & getRightSiblingName(int part) const;
  const SbName & getRightSiblingName(const SbName & name) const;
  int getRightSiblingPartNumber(int part) const;
  int getRightSiblingPartNumber(const SbName & name) const;

  SbBool isList(int part) const;
  SbBool isList(const SbName & name) const;
  SoType getListContainerType(int part) const;
  SoType getListContainerType(const SbName & name) const;
  const SoTypeList & getListItemTypes(int part) const;
  const SoTypeList & getListItemTypes(const SbName & name) const;

  SbBool isPublic(int part) const;
  SbBool isPublic(const SbName & name) const;

  SoNodekitCatalog * clone(SoType type) const;

  SbBool addEntry(const SbName & name, SoType type, SoType defaulttype,
                  SbBool isdefaultnull, const SbName & parent,
                  const SbName & rightsibling, SbBool islist,
                  SoType listcontainertype, SoType listitemtype,
                  SbBool ispublic);
  void addListItemType(int part, SoType type);
  void addListItemType(const SbName & name, SoType type);

  void narrowTypes(const SbName & name, SoType newtype, SoType newdefaulttype);
  void setNullByDefault(const SbName & name, SbBool nullbydefault);

  SbBool recursiveSearch(int part, const SbName & name,
                         SoTypeList * checked) const;

  void printCheck(void) const;

private:
  SbBool hasEntry(const SbName & name) const;
  SbBool hasListItemType(const SbName & name, SoType type) const;
  SbBool reallyAddEntry(class CatalogItem * newitem);

  int getPartNumber(const SbList<class CatalogItem *> & l,
                    const SbName & name) const;
  void addListItemType(const SbList<class CatalogItem *> & l,
                       int part, SoType type);
  SbBool addListItemType(const SbList<class CatalogItem *> & l,
                         const SbName & name, SoType type);

  SbList<class CatalogItem *> items;
  SbList<class CatalogItem *> delayeditems;
};

#endif // !COIN_SONODEKITCATALOG_H
