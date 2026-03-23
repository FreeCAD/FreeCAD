// Copyright (C) 1998-2005 by Systems in Motion. All rights reserved.

//  The ShapeScale class is used for scaling a shape based on
//  projected size.
//
//  This nodekit can be inserted in your scene graph to add for
//  instance 3D markers that will be of a constant projected size.
//
//  The marker shape is stored in the "shape" part. Any kind of node
//  can be used, even group nodes with several shapes, but the
//  marker shape should be approximately of unit size, and with a
//  center position in (0, 0, 0).


//  SoSFFloat ShapeScale::active
//  Turns the scaling on/off. Default value is TRUE.


//  SoSFFloat ShapeScale::projectedSize
//  The requested projected size of the shape. Default value is 5.0.

#include "ShapeScale.h"

#include <Inventor/actions/SoGLRenderAction.h>
#include <Inventor/nodes/SoShape.h>
#include <Inventor/nodes/SoScale.h>
#include <Inventor/nodes/SoCube.h>
#include <Inventor/nodes/SoSeparator.h>
#include <Inventor/elements/SoViewVolumeElement.h>
#include <Inventor/elements/SoViewportRegionElement.h>
#include <Inventor/elements/SoModelMatrixElement.h>

SO_KIT_SOURCE(ShapeScale);


//  Constructor.
ShapeScale::ShapeScale(void) 
{
  SO_KIT_CONSTRUCTOR(ShapeScale);

  SO_KIT_ADD_FIELD(active, (TRUE));
  SO_KIT_ADD_FIELD(projectedSize, (5.0f));  

  SO_KIT_ADD_CATALOG_ENTRY(topSeparator, SoSeparator, FALSE, this, \x0, FALSE);
  SO_KIT_ADD_CATALOG_ABSTRACT_ENTRY(shape, SoNode, SoCube, TRUE, topSeparator, \x0, TRUE);
  SO_KIT_ADD_CATALOG_ENTRY(scale, SoScale, FALSE, topSeparator, shape, FALSE);
  
  SO_KIT_INIT_INSTANCE();
}
  
  
// Destructor.
ShapeScale::~ShapeScale()
{
}
  
// Initializes this class. Call before using it.

void
ShapeScale::initClass(void)
{
  SO_KIT_INIT_CLASS(ShapeScale, SoBaseKit, "BaseKit");
}

static void 
update_scale(SoScale * scale, const SbVec3f & v)
{
  // only write to field when scaling has changed.
  if (scale->scaleFactor.getValue() != v) {
    scale->scaleFactor = v;
  }
}

// Overridden to (re)initialize scaling before rendering marker.
void 
ShapeScale::GLRender(SoGLRenderAction * action)
{
  SoState * state = action->getState();

  SoScale * scale = (SoScale*) this->getAnyPart(SbName("scale"), TRUE);
  if (!this->active.getValue()) {
    update_scale(scale, SbVec3f(1.0f, 1.0f, 1.0f));
  }
  else {
    const SbViewportRegion & vp = SoViewportRegionElement::get(state);
    const SbViewVolume & vv = SoViewVolumeElement::get(state);
    SbVec3f center(0.0f, 0.0f, 0.0f);
    float nsize = this->projectedSize.getValue() / float(vp.getViewportSizePixels()[1]);
    SoModelMatrixElement::get(state).multVecMatrix(center, center); // transform to WCS
    float scalefactor = vv.getWorldToScreenScale(center, nsize);
    update_scale(scale, SbVec3f(scalefactor, scalefactor, scalefactor));
  }
  inherited::GLRender(action);
}
