// Copyright (C) 1998-2005 by Systems in Motion. All rights reserved.

#ifndef COIN_SHAPESCALE_H
#define COIN_SHAPESCALE_H
#include <Inventor/nodekits/SoSubKit.h>
#include <Inventor/nodekits/SoBaseKit.h>
#include <Inventor/fields/SoSFFloat.h>

class SbViewport;
class SoState;
class SbColor;
class SbVec2s;

class ShapeScale : public SoBaseKit {
  typedef SoBaseKit inherited;

  SO_KIT_HEADER(ShapeScale);
  
  SO_KIT_CATALOG_ENTRY_HEADER(topSeparator);
  SO_KIT_CATALOG_ENTRY_HEADER(scale);
  SO_KIT_CATALOG_ENTRY_HEADER(shape);
  
 public:
  ShapeScale(void);
  static void initClass(void);

  SoSFFloat active;
  SoSFFloat projectedSize;

 protected:
  virtual void GLRender(SoGLRenderAction * action);
  virtual ~ShapeScale();
};

#endif // ! SHAPESCALE_H
