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

/*!
  \class SoTextureCombine SoTextureCombine.h Inventor/nodes/SoTextureCombine.h
  \brief The SoTextureCombine class is a node for setting texture combine functions.

  \ingroup coin_nodes

  This node is more or less an exact mapping of the OpenGL
  ARB_texture_env_combine extension (included in OpenGL in OpenGL
  v1.3). See
  http://oss.sgi.com/projects/ogl-sample/registry/ARB/texture_env_combine.txt
  for more information about this extension.

  Basically this node lets you specify up to three sources, and a
  function to combine those sources. In addition to the function, it is
  possible to apply simple operands on the sources. In the
  documentation below, the sources will be called Arg0, Arg1, and
  Arg2, just like in the ARB_texture_env_mode specification.

  It's possible to specify both a color and an alpha operation.

  This node has many fields, but usually it is sufficient to set only
  one or very few fields. The selected operation decides which
  values you need to set. One common example is to add a light map
  to textured geometry. A light map can look like this:
  
  <center>
  \image html lightmap.jpg "Rendering of an Example LightMap"
  </center>
  
  The example below just shows how to apply the light map to a cube,
  with one light source on each side of the cube. Usually the texture
  coordinates are calculated so that a spot light or a point light is
  simulated.

  \verbatim

  Texture2 { filename "wood.jpg" }
  
  Switch {
    whichChild -3   # use to toggle light map on/off
    TextureUnit {
      unit 1
    }
    TextureCombine {
      rgbOperation ADD_SIGNED
      rgbSource [PREVIOUS, TEXTURE]
      rgbOperand [SRC_COLOR, SRC_COLOR ]
      alphaOperation REPLACE
      alphaSource [TEXTURE]
      alphaOperand [SRC_ALPHA]
    }
    Texture2 { filename "lightmap.jpg" }
    TextureUnit { unit 0 }
  }
  Cube { }

  \endverbatim

  The scene above in a viewer:

  <center>
  \image html lightmap_screenshot.png "Rendering of Example Scenegraph"
  </center>
  
  In addition to the functions you can set in rgbOperation (or
  alphaOperation), it's possible to create more complex texture
  functions by combining two textures that have already been
  combined. You can use the SoSceneTexture2 node to create those
  textures. Below is an example that shows how to implement Arg0*Arg1
  + Arg2*Arg0, where Arg0 = texture1 RGB, Arg1 = texture2 RGB, Arg2 =
  texture 2 alpha:

  \verbatim 

  ShapeHints { vertexOrdering COUNTERCLOCKWISE shapeType SOLID }

  Separator {
    SceneTexture2 {
      size 256 256
      transparencyFunction NONE
      scene Separator {
        OrthographicCamera {
          height 2
          aspectRatio 1
          position 0 0 1
          viewportMapping LEAVE_ALONE
        }
        LightModel { model BASE_COLOR }
        Coordinate3 {
          point [ -1 -1 0, 1 -1 0, 1 1 0, -1 1 0 ] 
        }
        DEF texture1 Texture2 { filename "texture1.png" }
        TextureUnit { unit 1 }
        TextureCombine {
          rgbOperation MODULATE
          rgbSource [ PREVIOUS, TEXTURE ]
          rgbOperand [ SRC_COLOR, SRC_COLOR ]
          alphaOperation REPLACE
          alphaSource [TEXTURE]
          alphaOperand [ SRC_ALPHA ]
        }
        DEF texture2 Texture2 { filename "texture2_with_alpha.png" }
        TextureCoordinate2 {
          point [0 0, 1 0, 1 1, 0 1]
        }
        FaceSet { numVertices 4 }
      }
    }
    TextureUnit { unit 1 }
    TextureCombine {
      rgbOperation ADD
      rgbSource [ PREVIOUS, TEXTURE ]
      rgbOperand [ SRC_COLOR, SRC_COLOR ]
      alphaOperation REPLACE
      alphaSource [TEXTURE]
      alphaOperand [ SRC_ALPHA ]
    }
    SceneTexture2 {
      size 256 256
      transparencyFunction NONE
      scene Separator {
        OrthographicCamera {
          height 2
          aspectRatio 1
          position 0 0 1
          viewportMapping LEAVE_ALONE
        }
        LightModel { model BASE_COLOR }
        Coordinate3 {
          point [ -1 -1 0, 1 -1 0, 1 1 0, -1 1 0 ] 
        }
        USE texture1
        TextureUnit { unit 1 }
        TextureCombine {
          rgbOperation MODULATE
          rgbSource [ PREVIOUS, TEXTURE ]
          rgbOperand [ SRC_COLOR, SRC_ALPHA ]
          alphaOperation REPLACE
          alphaSource [TEXTURE]
          alphaOperand [ SRC_ALPHA ]
        }
        USE texture2
        TextureCoordinate2 {
          point [0 0, 1 0, 1 1, 0 1]
        }
        FaceSet { numVertices 4 }
      }
    }
    # map resulting texture onto a Cube
    Cube { }
  }
  \endverbatim

  It should be possible to create almost any kind of texture function
  using this scheme, at the cost of extra texture memory usage (the
  intermediate textures), of course.

  <b>FILE FORMAT/DEFAULTS:</b>
  \code
    TextureCombine {
        rgbSource [  ]
        alphaSource [  ]
        rgbOperand [  ]
        alphaOperand [  ]
        rgbOperation MODULATE
        alphaOperation MODULATE
        rgbScale 1
        alphaScale 1
        constantColor 1 1 1 1
    }
  \endcode

  \since Coin 2.3
*/

// *************************************************************************

/*!
  \enum SoTextureCombine::Source
  For enumerating combiner sources.
*/

/*!
  \enum SoTextureCombine::Operand
  For enumerating source operands.
*/

/*!
  \enum SoTextureCombine::Operation
  For enumerating combiner operations/functions.
*/


/*!
  \var SoTextureCombine::Source SoTextureCombine::PRIMARY_COLOR

  Choose primary color as source.
*/

/*!
  \var SoTextureCombine::Source SoTextureCombine::TEXTURE

  Choose texture as source.
*/

/*!
  \var SoTextureCombine::Source SoTextureCombine::CONSTANT

  Choose the constantColor field as source.
*/

/*!
  \var SoTextureCombine::Source SoTextureCombine::PREVIOUS

  Choose the previous unit's texture as source.
*/

/*!
  \var SoTextureCombine::Operand SoTextureCombine::SRC_COLOR

  Use the source color as operand.
*/

/*!
  \var SoTextureCombine::Operand SoTextureCombine::ONE_MINUS_SRC_COLOR

  Use one minus source color as operand.
*/

/*!
  \var SoTextureCombine::Operand SoTextureCombine::SRC_ALPHA

  Use the source alpha as operand.
*/

/*!
  \var SoTextureCombine::Operand SoTextureCombine::ONE_MINUS_SRC_ALPHA

  Use one minus source alpha as operand.
*/

/*!
  \var SoTextureCombine::Operation SoTextureCombine::REPLACE

  dst = Arg0
*/

/*!
  \var SoTextureCombine::Operation SoTextureCombine::MODULATE

  dst = Arg0 * Arg1
*/

/*!
  \var SoTextureCombine::Operation SoTextureCombine::ADD

  dst = Arg0 + Arg1
*/

/*!
  \var SoTextureCombine::Operation SoTextureCombine::ADD_SIGNED

  dst = Arg0 + Arg1 - 0.5
*/

/*!
  \var SoTextureCombine::Operation SoTextureCombine::SUBTRACT

  dst = Arg0 - Arg1
*/

/*!
  \var SoTextureCombine::Operation SoTextureCombine::INTERPOLATE

  dst = Arg0 * (Arg2) + Arg1 * (1-Arg2)
*/

/*!
  \var SoTextureCombine::Operation SoTextureCombine::DOT3_RGB

  Dot product of Arg0 and Arg1
*/

/*!
  \var SoTextureCombine::Operation SoTextureCombine::DOT3_RGBA

  Dot product (including alpha) of Arg0 and Arg1
*/

/*!
  SoTextureCombine::rgbSource

  The color sources. This field is empty by default, but you can
  insert up to three values [Arg0, Arg1, Arg2]. When the field
  contains less than three values, the default [TEXTURE, PREVIOUS,
  CONSTANT], will be used for missing values.

  For texture unit 0, PREVIOUS maps to PRIMARY_COLOR,
*/

/*!
  SoTextureCombine::alphaSource

  The alpha sources. This field is empty by default, but you can
  insert up to three values [Arg0, Arg1, Arg2]. When the field
  contains less than three values, the default [TEXTURE, PREVIOUS,
  CONSTANT], will be used for missing values.

  For texture unit 0, PREVIOUS maps to PRIMARY_COLOR,
*/

/*!
  SoTextureCombine::rgbOperand

  The color operands. This field is empty by default, but you can
  insert up to three values. When the field contains less than three
  values, the default [SRC_COLOR, SRC_COLOR, SRC_COLOR] will be used
  for missing values.
*/

/*!
  SoTextureCombine::alphaOperand

  The alpha operands. This field is empty by default, but you can
  insert up to three values. When the field contains less than three
  values, the default [SRC_ALPHA, SRC_ALPHA, SRC_ALPHA] will be used
  for missing values. Please not that only SRC_ALPHA and
  ONE_MINUS_SRC_ALPHA are valid operands for alpha operations.
*/

/*!
  SoTextureCombine::rgbOperation

  The color operation. Default value is MODULATE.
*/

/*!
  SoTextureCombine::alphaOperation

  The alpha operation. Default value is MODULATE.
*/

/*!
  SoTextureCombine::rgbScale

  Scale color result by this value. Supported values are 1, 2, and 4.
*/

/*!
  SoTextureCombine::alphaScale

  Scale alpha result by this value. Supported values are 1, 2, and 4.
*/

/*!
  SoTextureCombine::constantColor

  The constant color (when CONSTANT is used as source). Default value
  is (1,1,1,1).
*/


#include <Inventor/nodes/SoTextureCombine.h>

#include <Inventor/actions/SoGLRenderAction.h>
#include <Inventor/actions/SoPickAction.h>
#include <Inventor/actions/SoCallbackAction.h>
#include <Inventor/elements/SoTextureCombineElement.h>
#include <Inventor/elements/SoTextureUnitElement.h>
#include <Inventor/elements/SoGLCacheContextElement.h>
#include <Inventor/errors/SoDebugError.h>
#include <Inventor/C/glue/gl.h>
#include <Inventor/misc/SoGLDriverDatabase.h>

#include "nodes/SoSubNodeP.h"

// *************************************************************************

SO_NODE_SOURCE(SoTextureCombine);

/*!
  Constructor.
*/
SoTextureCombine::SoTextureCombine(void)
{
  SO_NODE_INTERNAL_CONSTRUCTOR(SoTextureCombine);

  SO_NODE_ADD_FIELD(rgbSource, (TEXTURE));
  SO_NODE_ADD_FIELD(alphaSource, (TEXTURE));
  SO_NODE_ADD_FIELD(rgbOperand, (SRC_COLOR));
  SO_NODE_ADD_FIELD(alphaOperand, (SRC_ALPHA));

  this->rgbSource.setNum(0);
  this->rgbSource.setDefault(TRUE);
  this->alphaSource.setNum(0);
  this->alphaSource.setDefault(TRUE);
  this->rgbOperand.setNum(0);
  this->rgbOperand.setDefault(TRUE);
  this->alphaOperand.setNum(0);
  this->alphaOperand.setDefault(TRUE);

  SO_NODE_ADD_FIELD(rgbOperation, (MODULATE));
  SO_NODE_ADD_FIELD(alphaOperation, (MODULATE));

  SO_NODE_ADD_FIELD(rgbScale, (1.0f));
  SO_NODE_ADD_FIELD(alphaScale, (1.0f));

  SO_NODE_ADD_FIELD(constantColor, (1.0f, 1.0f, 1.0f, 1.0f));

  SO_NODE_DEFINE_ENUM_VALUE(Source, PRIMARY_COLOR);
  SO_NODE_DEFINE_ENUM_VALUE(Source, TEXTURE);
  SO_NODE_DEFINE_ENUM_VALUE(Source, CONSTANT);
  SO_NODE_DEFINE_ENUM_VALUE(Source, PREVIOUS);

  SO_NODE_DEFINE_ENUM_VALUE(Operand, SRC_COLOR);
  SO_NODE_DEFINE_ENUM_VALUE(Operand, ONE_MINUS_SRC_COLOR);
  SO_NODE_DEFINE_ENUM_VALUE(Operand, SRC_ALPHA);
  SO_NODE_DEFINE_ENUM_VALUE(Operand, ONE_MINUS_SRC_ALPHA);

  SO_NODE_DEFINE_ENUM_VALUE(Operation, REPLACE);
  SO_NODE_DEFINE_ENUM_VALUE(Operation, MODULATE);
  SO_NODE_DEFINE_ENUM_VALUE(Operation, ADD);
  SO_NODE_DEFINE_ENUM_VALUE(Operation, ADD_SIGNED);
  SO_NODE_DEFINE_ENUM_VALUE(Operation, SUBTRACT);
  SO_NODE_DEFINE_ENUM_VALUE(Operation, INTERPOLATE);
  SO_NODE_DEFINE_ENUM_VALUE(Operation, DOT3_RGB);
  SO_NODE_DEFINE_ENUM_VALUE(Operation, DOT3_RGBA);

  SO_NODE_SET_MF_ENUM_TYPE(rgbSource, Source);
  SO_NODE_SET_MF_ENUM_TYPE(alphaSource, Source);
  SO_NODE_SET_MF_ENUM_TYPE(rgbOperand, Operand);
  SO_NODE_SET_MF_ENUM_TYPE(alphaOperand, Operand);

  SO_NODE_SET_SF_ENUM_TYPE(rgbOperation, Operation);
  SO_NODE_SET_SF_ENUM_TYPE(alphaOperation, Operation);
}


/*!
  Destructor.
*/
SoTextureCombine::~SoTextureCombine()
{
}

/*!
  \copybrief SoBase::initClass(void)
*/
void
SoTextureCombine::initClass(void)
{
  SO_NODE_INTERNAL_INIT_CLASS(SoTextureCombine, SO_FROM_COIN_2_2);

  SO_ENABLE(SoGLRenderAction, SoTextureCombineElement);
}

// Doc from superclass.
void
SoTextureCombine::GLRender(SoGLRenderAction * action)
{
  const cc_glglue * glue = cc_glglue_instance(action->getCacheContext());

  SoTextureCombine::Operation rgbaop =
    (SoTextureCombine::Operation) this->rgbOperation.getValue();

  SoTextureCombine::Operation alphaop =
    (SoTextureCombine::Operation) this->alphaOperation.getValue();

  SbBool supported = cc_glglue_glversion_matches_at_least(glue, 1, 3, 0);

  if (!supported) {
    supported = SoGLDriverDatabase::isSupported(glue, "GL_ARB_texture_env_combine");
    if (supported && (alphaop == DOT3_RGB || alphaop == DOT3_RGBA ||
                      rgbaop == DOT3_RGB || rgbaop == DOT3_RGBA)) {
      supported =
        SoGLDriverDatabase::isSupported(glue, "GL_ARB_texture_env_dot3");
    }
  }
  
  if (supported) {
    SoTextureCombine::doAction((SoAction*)action);
  }
  else {
    static int didwarn = 0;
    if (!didwarn) {
      SoDebugError::postWarning("SoTextureCombine::GLRender",
                                "Your OpenGL driver does not support the "
                                "required extensions to do texture combine.");
      didwarn = 1;
    }

  }
}

// Doc from superclass.
void
SoTextureCombine::doAction(SoAction * action)
{
  SoState * state = action->getState();
  int unit = SoTextureUnitElement::get(state);

  SoTextureCombineElement::Source rgbsource[3] = {
    SoTextureCombineElement::TEXTURE,
    SoTextureCombineElement::PREVIOUS,
    SoTextureCombineElement::CONSTANT
  };
  SoTextureCombineElement::Operand rgboperand[3] = {
    SoTextureCombineElement::SRC_COLOR,
    SoTextureCombineElement::SRC_COLOR,
    SoTextureCombineElement::SRC_COLOR
  };

  SoTextureCombineElement::Source alphasource[3] = {
    SoTextureCombineElement::TEXTURE,
    SoTextureCombineElement::PREVIOUS,
    SoTextureCombineElement::CONSTANT
  };
  SoTextureCombineElement::Operand alphaoperand[3] = {
    SoTextureCombineElement::SRC_ALPHA,
    SoTextureCombineElement::SRC_ALPHA,
    SoTextureCombineElement::SRC_ALPHA
  };
  int i;
  for (i = 0; i < this->rgbSource.getNum() && i < 3; i++) {
    rgbsource[i] = (SoTextureCombineElement::Source) this->rgbSource.getValues(0)[i];
  }
  for (i = 0; i < this->alphaSource.getNum() && i < 3; i++) {
    alphasource[i] = (SoTextureCombineElement::Source) this->alphaSource.getValues(0)[i];
  }
  for (i = 0; i < this->rgbOperand.getNum() && i < 3; i++) {
    rgboperand[i] = (SoTextureCombineElement::Operand) this->rgbOperand.getValues(0)[i];
  }
  for (i = 0; i < this->alphaOperand.getNum() && i < 3; i++) {
    alphaoperand[i] = (SoTextureCombineElement::Operand) this->alphaOperand.getValues(0)[i];
  }

  SbColor4f col;
  SbVec4f tmp = this->constantColor.getValue();
  col[0] = tmp[0];
  col[1] = tmp[1];
  col[2] = tmp[2];
  col[3] = tmp[3];


  const cc_glglue * glue = 
    cc_glglue_instance(SoGLCacheContextElement::get(state));
  int maxunits = cc_glglue_max_texture_units(glue);

  if (unit < maxunits) {
    SoTextureCombineElement::set(state, this, unit,
                                 (SoTextureCombineElement::Operation) this->rgbOperation.getValue(),
                                 (SoTextureCombineElement::Operation) this->alphaOperation.getValue(),
                                 rgbsource, alphasource,
                                 rgboperand, alphaoperand,
                                 col,
                                 this->rgbScale.getValue(),
                                 this->alphaScale.getValue());
  }
}


// Doc from superclass.
void
SoTextureCombine::callback(SoCallbackAction * action)
{
  // So far only SoGLRenderAction supports SoTextureCombineElement.  We
  // may never support multiple texture units for SoCallbackAction,
  // but we reimplement the method just in case
  inherited::callback(action);
}

// Doc from superclass.
void
SoTextureCombine::pick(SoPickAction * action)
{
  // So far only SoGLRenderAction supports SoTextureCombineElement.  We
  // may never support multiple texture units for SoPickAction, but we
  // reimplement the method just in case
  inherited::pick(action);
}
