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
  \class SoOverrideElement Inventor/elements/SoOverrideElement.h
  \brief The SoOverrideElement maintains a list of overridable elements and a list over which elements should be overridden.

  \ingroup coin_elements

  Only certain elements can be overridden.


  The remaining class documentation describes a single, special case:

  A common request for functionality is to override only the
  transparency of the full scene graph, or parts of the scene
  graph.

  In the original SGI Inventor, this is nearly impossible, as the API
  was designed to only make it possible to override all or none of the
  fields of a node. So calling SoNode::setOverride() on an SoMaterial
  node will cause all material settings of that node to override all
  material settings further on in the scene graph, and there is no way
  to override only the transparency settings.

  In Coin, we have added in a little hack to overcome this problem,
  since it is such a common request for functionality: to have
  separate transparency override settings, set the environment
  variable \c COIN_SEPARATE_DIFFUSE_TRANSPARENCY_OVERRIDE to "1" to
  enable this hack.

  (Do however note that this will not work when the SoPackedColor or
  SoVertexProperty node is used to specify diffuse color and
  transparency -- only with the SoMaterial node.)

  Here is a complete, standalone example which demonstrates how to
  accomplish this:

  \code
  #include <Inventor/Qt/SoQt.h>
  #include <Inventor/Qt/viewers/SoQtExaminerViewer.h>
  #include <Inventor/nodes/SoSeparator.h>
  #include <Inventor/nodes/SoMaterial.h>

  // *************************************************************************

  const char * scene = "#Inventor V2.1 asci\n"
  "\n"
  "Separator {"
  "   Cone { }"
  "   Translation { translation 1 0 5 }"
  ""
  "   DEF OVERRIDEMATERIAL Material { transparency 0.5 }"
  ""
  "   DEF OBJMATERIAL Material {"
  "      diffuseColor 0.5 0 0"
  "      transparency 0"
  "   }"
  "   Sphere { }"
  "}"
  ;

  int
  main(int argc, char ** argv)
  {
    QWidget * window = SoQt::init(argv[0]);

    (void)coin_setenv("COIN_SEPARATE_DIFFUSE_TRANSPARENCY_OVERRIDE", "1", TRUE);

    SoInput * in = new SoInput;
    in->setBuffer(scene, strlen(scene));
    SoSeparator * root = SoDB::readAll(in);
    assert(root);
    delete in;

    root->ref();

    SoMaterial * overridemat = (SoMaterial *)
      SoBase::getNamedBase("OVERRIDEMATERIAL", SoMaterial::getClassTypeId());
    assert(overridemat);

    overridemat->diffuseColor.setIgnored(TRUE);
    overridemat->setOverride(TRUE);

    SoQtExaminerViewer * viewer = new SoQtExaminerViewer(window);
    viewer->setSceneGraph(root);
    viewer->show();

    SoQt::show(window);
    SoQt::mainLoop();

    delete viewer;
    root->unref();

    return 0;
  }
  \endcode
*/

// *************************************************************************

#include "SbBasicP.h"

#include <Inventor/elements/SoOverrideElement.h>

#include <cassert>
#include <cstdlib>

#include <Inventor/C/tidbits.h>

// *************************************************************************

#define SO_GET_OVERRIDE(flag) \
const SoOverrideElement * const element = \
  coin_assert_cast<const SoOverrideElement *>(getConstElement(state, classStackIndex)); \
return (element->flags & flag)

#define SO_SET_OVERRIDE(flag) \
SoOverrideElement * const element = \
  const_cast<SoOverrideElement * const> \
  ( \
   coin_safe_cast<const SoOverrideElement *>(getElement(state, classStackIndex)) \
    ); \
if (element && override) \
  element->flags |= flag; \
else if (element) \
  element->flags &= ~flag

// *************************************************************************

static int COIN_SEPARATE_DIFFUSE_TRANSPARENCY_OVERRIDE = -1;

// *************************************************************************

static SbBool
use_separate_transp_diffuse(void)
{
  if (COIN_SEPARATE_DIFFUSE_TRANSPARENCY_OVERRIDE < 0) {
    COIN_SEPARATE_DIFFUSE_TRANSPARENCY_OVERRIDE = 0;

    const char * env =
      coin_getenv("COIN_SEPARATE_DIFFUSE_TRANSPARENCY_OVERRIDE");
    if (env) {
      COIN_SEPARATE_DIFFUSE_TRANSPARENCY_OVERRIDE = atoi(env);
    }
  }
  return COIN_SEPARATE_DIFFUSE_TRANSPARENCY_OVERRIDE ?
    TRUE : FALSE;
}

// *************************************************************************

/*!
\enum SoOverrideElement::FlagBits

FIXME: write doc.
*/

SO_ELEMENT_SOURCE(SoOverrideElement);

/*!
  \copydetails SoElement::initClass(void)
*/

void
SoOverrideElement::initClass(void)
{
  SO_ELEMENT_INIT_CLASS(SoOverrideElement, inherited);
}

/*!
  Destructor.
*/

SoOverrideElement::~SoOverrideElement(void)
{
}

/*!
  Initializes the element to its default value. The default
  value for flags is 0.
*/

void
SoOverrideElement::init(SoState * state)
{
  inherited::init(state);
  this->flags = 0;
}

//! FIXME: write doc.

void
SoOverrideElement::push(SoState * state)
{
  inherited::push(state);
  const SoOverrideElement * prev = coin_assert_cast<SoOverrideElement *>
    (
     this->getNextInStack()
     );
  this->flags = prev->flags;
}

//! FIXME: write doc.

SbBool
SoOverrideElement::matches(const SoElement *element) const
{
  return (coin_assert_cast<const SoOverrideElement *>(element))->flags == this->flags;
}

//! FIXME: write doc.

SoElement *
SoOverrideElement::copyMatchInfo(void) const
{
  SoOverrideElement * elem = static_cast<SoOverrideElement *>
    (
     this->getTypeId().createInstance()
     );
  elem->flags = this->flags;
  return elem;
}

//! FIXME: write doc.

void
SoOverrideElement::print(FILE * /* file */) const
{
}

/*!
  FIXME: write doc.
*/

SbBool
SoOverrideElement::getAmbientColorOverride(SoState * const state)
{
  SO_GET_OVERRIDE(AMBIENT_COLOR);
}

/*!
  FIXME: write doc.
*/

SbBool
SoOverrideElement::getColorIndexOverride(SoState * const state)
{
  SO_GET_OVERRIDE(COLOR_INDEX);
}

/*!
  FIXME: write doc.
*/

SbBool
SoOverrideElement::getComplexityOverride(SoState * const state)
{
  SO_GET_OVERRIDE(COMPLEXITY);
}

/*!
  FIXME: write doc.
*/

SbBool
SoOverrideElement::getComplexityTypeOverride(SoState * const state)
{
  SO_GET_OVERRIDE(COMPLEXITY_TYPE);
}

/*!
  FIXME: write doc.
*/

SbBool
SoOverrideElement::getCreaseAngleOverride(SoState * const state)
{
  SO_GET_OVERRIDE(CREASE_ANGLE);
}

/*!
  FIXME: write doc.
*/

SbBool
SoOverrideElement::getDiffuseColorOverride(SoState * const state)
{
  SO_GET_OVERRIDE(DIFFUSE_COLOR);
}

/*!
  FIXME: write doc.
*/

SbBool
SoOverrideElement::getDrawStyleOverride(SoState * const state)
{
  SO_GET_OVERRIDE(DRAW_STYLE);
}

/*!
  FIXME: write doc.
*/

SbBool
SoOverrideElement::getEmissiveColorOverride(SoState * const state)
{
  SO_GET_OVERRIDE(EMISSIVE_COLOR);
}

/*!
  FIXME: write doc.
*/

SbBool
SoOverrideElement::getFontNameOverride(SoState * const state)
{
  SO_GET_OVERRIDE(FONT_NAME);
}

/*!
  FIXME: write doc.
*/

SbBool
SoOverrideElement::getFontSizeOverride(SoState * const state)
{
  SO_GET_OVERRIDE(FONT_SIZE);
}

/*!
  FIXME: write doc.
*/

SbBool
SoOverrideElement::getLightModelOverride(SoState * const state)
{
  SO_GET_OVERRIDE(LIGHT_MODEL);
}

/*!
  FIXME: write doc.
*/

SbBool
SoOverrideElement::getLinePatternOverride(SoState * const state)
{
  SO_GET_OVERRIDE(LINE_PATTERN);
}

/*!
  FIXME: write doc.
*/

SbBool
SoOverrideElement::getLineWidthOverride(SoState * const state)
{
  SO_GET_OVERRIDE(LINE_WIDTH);
}

/*!
  FIXME: write doc.
*/

SbBool
SoOverrideElement::getMaterialBindingOverride(SoState * const state)
{
  SO_GET_OVERRIDE(MATERIAL_BINDING);
}

/*!
  FIXME: write doc.
*/

SbBool
SoOverrideElement::getPointSizeOverride(SoState * const state)
{
  SO_GET_OVERRIDE(POINT_SIZE);
}

/*!
  FIXME: write doc.
*/

SbBool
SoOverrideElement::getPickStyleOverride(SoState * const state)
{
  SO_GET_OVERRIDE(PICK_STYLE);
}

/*!
  FIXME: write doc.
*/

SbBool
SoOverrideElement::getShapeHintsOverride(SoState * const state)
{
  SO_GET_OVERRIDE(SHAPE_HINTS);
}

/*!
  FIXME: write doc.
*/

SbBool
SoOverrideElement::getShininessOverride(SoState * const state)
{
  SO_GET_OVERRIDE(SHININESS);
}

/*!
  FIXME: write doc.
*/

SbBool
SoOverrideElement::getSpecularColorOverride(SoState * const state)
{
  SO_GET_OVERRIDE(SPECULAR_COLOR);
}

/*!
  FIXME: write doc.
*/

SbBool
SoOverrideElement::getTransparencyOverride(SoState * const state)
{
  SO_GET_OVERRIDE(TRANSPARENCY);
}

/*!
  FIXME: write doc.
*/

SbBool
SoOverrideElement::getTransparencyTypeOverride(SoState * const state)
{
  SO_GET_OVERRIDE(TRANSPARENCY_TYPE);
}

/*!
  FIXME: write doc.
*/

SbBool
SoOverrideElement::getPolygonOffsetOverride(SoState * const state)
{
  SO_GET_OVERRIDE(POLYGON_OFFSET);
}

/*!
  Returns normal vector override value.

  \COIN_FUNCTION_EXTENSION

  \since Coin 2.0
*/
SbBool
SoOverrideElement::getNormalVectorOverride(SoState * const state)
{
  SO_GET_OVERRIDE(NORMAL_VECTOR);
}

/*!
  Returns normal binding override value.

  \COIN_FUNCTION_EXTENSION

  \since Coin 2.0
*/
SbBool
SoOverrideElement::getNormalBindingOverride(SoState * const state)
{
  SO_GET_OVERRIDE(NORMAL_BINDING);
}

/*!
  FIXME: write doc.
*/

void
SoOverrideElement::setAmbientColorOverride(SoState * const state,
                                           SoNode * const /* node */,
                                           const SbBool override)
{
  SO_SET_OVERRIDE(AMBIENT_COLOR);
}

/*!
  FIXME: write doc.
*/

void
SoOverrideElement::setColorIndexOverride(SoState * const state,
                                         SoNode * const /* node */,
                                         const SbBool override)
{
  SO_SET_OVERRIDE(COLOR_INDEX);
}

/*!
  FIXME: write doc.
*/

void
SoOverrideElement::setComplexityOverride(SoState * const state,
                                         SoNode * const /* node */,
                                         const SbBool override)
{
  SO_SET_OVERRIDE(COMPLEXITY);
}

/*!
  FIXME: write doc.
*/

void
SoOverrideElement::setComplexityTypeOverride(SoState * const state,
                                             SoNode * const /* node */,
                                             const SbBool override)
{
  SO_SET_OVERRIDE(COMPLEXITY_TYPE);
}

/*!
  FIXME: write doc.
*/

void
SoOverrideElement::setCreaseAngleOverride(SoState * const state,
                                          SoNode * const /* node */,
                                          const SbBool override)
{
  SO_SET_OVERRIDE(CREASE_ANGLE);
}

/*!
  Can be used to set diffuse color override. This will also set the
  transparency override. Since we feel this is a design flaw,
  it is possible to override this behaviour by setting an environment
  value called COIN_SEPARATE_DIFFUSE_TRANSPARENCY_OVERRIDE to 1.

  Please note that separate override will not work for the PackedColor
  or SoVertexProperty nodes.
*/
void
SoOverrideElement::setDiffuseColorOverride(SoState * const state,
                                           SoNode * const /* node */,
                                           const SbBool override)
{
  SO_SET_OVERRIDE(DIFFUSE_COLOR);
  if (!use_separate_transp_diffuse()) {
    SO_SET_OVERRIDE(TRANSPARENCY);
  }
}

/*!
  FIXME: write doc.
*/

void
SoOverrideElement::setDrawStyleOverride(SoState * const state,
                                        SoNode * const /* node */,
                                        const SbBool override)
{
  SO_SET_OVERRIDE(DRAW_STYLE);
}

/*!
  FIXME: write doc.
*/

void
SoOverrideElement::setEmissiveColorOverride(SoState * const state,
                                            SoNode * const /* node */,
                                            const SbBool override)
{
  SO_SET_OVERRIDE(EMISSIVE_COLOR);
}

/*!
  FIXME: write doc.
*/

void
SoOverrideElement::setFontNameOverride(SoState * const state,
                                       SoNode * const /* node */,
                                       const SbBool override)
{
  SO_SET_OVERRIDE(FONT_NAME);
}

/*!
  FIXME: write doc.
*/

void
SoOverrideElement::setFontSizeOverride(SoState * const state,
                                       SoNode * const /* node */,
                                       const SbBool override)
{
  SO_SET_OVERRIDE(FONT_SIZE);
}

/*!
  FIXME: write doc.
*/

void
SoOverrideElement::setLightModelOverride(SoState * const state,
                                         SoNode * const /* node */,
                                         const SbBool override)
{
  SO_SET_OVERRIDE(LIGHT_MODEL);
}

/*!
  FIXME: write doc.
*/

void
SoOverrideElement::setLinePatternOverride(SoState * const state,
                                          SoNode * const /* node */,
                                          const SbBool override)
{
  SO_SET_OVERRIDE(LINE_PATTERN);
}

/*!
  FIXME: write doc.
*/

void
SoOverrideElement::setLineWidthOverride(SoState * const state,
                                        SoNode * const /* node */,
                                        const SbBool override)
{
  SO_SET_OVERRIDE(LINE_WIDTH);
}

//! FIXME: write doc.

void
SoOverrideElement::setMaterialBindingOverride(SoState * const state,
                                              SoNode * const /* node */,
                                              const SbBool override)
{
  SO_SET_OVERRIDE(MATERIAL_BINDING);
}

/*!
  FIXME: write doc.
*/

void
SoOverrideElement::setPickStyleOverride(SoState * const state,
                                        SoNode * const /* node */,
                                        const SbBool override)
{
  SO_SET_OVERRIDE(PICK_STYLE);
}

/*!
  FIXME: write doc.
*/

void
SoOverrideElement::setPointSizeOverride(SoState * const state,
                                        SoNode * const /* node */,
                                        const SbBool override)
{
  SO_SET_OVERRIDE(POINT_SIZE);
}

/*!
  FIXME: write doc.
*/

void
SoOverrideElement::setPolygonOffsetOverride(SoState * const state,
                                            SoNode * const /* node */,
                                            const SbBool override)
{
  SO_SET_OVERRIDE(POLYGON_OFFSET);
}

/*!
  FIXME: write doc.
*/

void
SoOverrideElement::setShapeHintsOverride(SoState * const state,
                                         SoNode * const /* node */,
                                         const SbBool override)
{
  SO_SET_OVERRIDE(SHAPE_HINTS);
}

/*!
  FIXME: write doc.
*/

void
SoOverrideElement::setShininessOverride(SoState * const state,
                                        SoNode * const /* node */,
                                        const SbBool override)
{
  SO_SET_OVERRIDE(SHININESS);
}

/*!
  FIXME: write doc.
*/

void
SoOverrideElement::setSpecularColorOverride(SoState * const state,
                                            SoNode * const /* node */,
                                            const SbBool override)
{
  SO_SET_OVERRIDE(SPECULAR_COLOR);
}


/*!
  Can be used to set the transparency override.

  \sa setDiffuseColorOverride().
*/
void
SoOverrideElement::setTransparencyOverride(SoState * const state,
                                           SoNode * const /* node */,
                                           const SbBool override)
{
  SO_SET_OVERRIDE(TRANSPARENCY);
  if (!use_separate_transp_diffuse()) {
    SO_SET_OVERRIDE(DIFFUSE_COLOR);
  }
}

/*!
  Can be used to set the transparency type override.

  \sa setDiffuseColorOverride().
*/
void
SoOverrideElement::setTransparencyTypeOverride(SoState * const state,
                                               SoNode * const /* node */,
                                               const SbBool override)
{
  SO_SET_OVERRIDE(TRANSPARENCY_TYPE);
}

/*!
  Can be used to set normal vector override.

  \COIN_FUNCTION_EXTENSION

  \since Coin 2.0
*/
void
SoOverrideElement::setNormalVectorOverride(SoState * const state,
                                           SoNode * const /* node */,
                                           const SbBool override)
{
  SO_SET_OVERRIDE(NORMAL_VECTOR);
}

/*!
  Can be used to set normal binding override.

  \COIN_FUNCTION_EXTENSION

  \since Coin 2.0
*/
void
SoOverrideElement::setNormalBindingOverride(SoState * const state,
                                            SoNode * const /* node */,
                                            const SbBool override)
{
  SO_SET_OVERRIDE(NORMAL_BINDING);
}

#undef SO_GET_OVERRIDE
#undef SO_SET_OVERRIDE
