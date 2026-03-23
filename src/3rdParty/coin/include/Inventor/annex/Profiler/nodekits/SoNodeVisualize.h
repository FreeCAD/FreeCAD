#ifndef COIN_SONODEVISUALIZE_H
#define COIN_SONODEVISUALIZE_H

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

#include <Inventor/nodekits/SoBaseKit.h>
#include <Inventor/nodes/SoSeparator.h>
#include <Inventor/lists/SoNodeList.h>
#include <Inventor/tools/SbPimplPtr.h>
#include <Inventor/SbVec2s.h>
#include <Inventor/SbVec3f.h>

// *************************************************************************

// FIXME: i don't like the name of this class. "SoScenegraphStructure"
// or some such would be better.  -mortene.

class SoProfilerStats;

class COIN_DLL_API SoNodeVisualize : public SoBaseKit
{
  typedef SoBaseKit inherited;

  SO_KIT_HEADER(SoNodeVisualize);
  SO_KIT_CATALOG_ENTRY_HEADER(topSeparator);
  SO_KIT_CATALOG_ENTRY_HEADER(childrenVisible);
  SO_KIT_CATALOG_ENTRY_HEADER(color);
  SO_KIT_CATALOG_ENTRY_HEADER(texture);
  SO_KIT_CATALOG_ENTRY_HEADER(textureTransform);
  SO_KIT_CATALOG_ENTRY_HEADER(shape);
  SO_KIT_CATALOG_ENTRY_HEADER(rotSwitch);
  SO_KIT_CATALOG_ENTRY_HEADER(rotation);
  SO_KIT_CATALOG_ENTRY_HEADER(childGeometry);
  SO_KIT_CATALOG_ENTRY_HEADER(translation);
  SO_KIT_CATALOG_ENTRY_HEADER(lines);
  SO_KIT_CATALOG_ENTRY_HEADER(lineSep);

public:
  static void initClass(void);
  static void cleanClass(void);

  SoNodeVisualize * visualize(SoNode *);
  static SoNodeVisualize* visualizeTree(SoNode * node,int depth=-1);
  bool clicked();
  void setAlternate(bool alternating=true);
  bool nodeHasChildren();
  unsigned int nodeNumChildren();
  bool isAlternating() const;

  virtual void handleEvent(SoHandleEventAction * action);

  void traverse(SoProfilerStats * stats);

protected:
  SoNodeVisualize(void);
  virtual ~SoNodeVisualize();
  SbVec2s getWidth() ;

  void setupChildCatalog(SoNode * node, int depth);
  void visualizeSubTree(SoNode * node,int depth=-1);
  void recalculate();
  SbVec2s recalculateWidth();
  SoNodeVisualize* getSoNodeVisualizeRoot();
  void internalAlternating(bool alternating, int direction);
  void reset();
  SoNodeList * getChildGeometry();

 private:
  SbPimplPtr<class SoNodeVisualizeP> pimpl;

  // NOT IMPLEMENTED
  SoNodeVisualize(const SoNodeVisualize &);
  SoNodeVisualize & operator = (const SoNodeVisualize &);

  bool dirty;
  SbVec2s width;
  SoNodeVisualize *parent;

  SoNode *node;
};

#endif // !COIN_SONODEVISUALIZE_H
