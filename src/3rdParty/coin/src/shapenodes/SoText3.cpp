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
  \class SoText3 SoText3.h Inventor/nodes/SoText3.h
  \brief The SoText3 class renders extruded 3D text.

  \ingroup coin_nodes

  Render text as 3D geometry.

  The size of the textual geometry representation is decided from the
  SoFont::size field of a preceding SoFont-node in the scene graph,
  which specifies the size in unit coordinates. This value sets the
  approximate vertical size of the letters.  The default value if no
  SoFont-nodes are used, is 10.

  This node will create 3D geometry from a specified font defined by a
  preceding SoFont node. The complexity of the glyphs is controlled by
  a preceding SoComplexity node with \e Type set to OBJECT_SPACE.
  Please note that the default built-in 3D font will not be affected by
  the SoComplexity node.

  This is a simple example of an extruded SoText3 string:

  \verbatim
   #Inventor V2.1 ascii

   Separator {
     renderCaching ON
     Font {
        name "Arial"
        size 2
     }
     ProfileCoordinate2 {
       point [ 0 0,
               0.05 0.05,
               0.25 0.05,
               0.3 0 ]
     }
     LinearProfile {
       index [ 0, 1, 2, 3 ]
     }
     Complexity {
       type OBJECT_SPACE
       value 1
     }
     ShapeHints {
       creaseAngle 1.5
       shapeType SOLID
       vertexOrdering COUNTERCLOCKWISE
     }
     Material {
       diffuseColor 0.6 0.6 0.8
       specularColor 1 1 1
     }
     Text3 {
       string ["Coin3D"]
       parts ALL
     }
   }
  \endverbatim

  <center>
  \image html text3.png "Rendering of Example File"
  </center>


  if SoText3::Part is set to SIDES or ALL and no profile is provided, a
  flat, one unit long profile will be created.

  Separate colors can be assigned to the front, sides and back of the
  glyphs by adding a preceding SoMaterialBinding node.  Set the \e value
  field to PER_PART (default is OVERALL). The front, side and back of
  the glyphs will then be colored according to diffuse color 0, 1 and 2
  found on the stack.

  Beware that using a lot of SoText3 text characters in a scene will
  usually have severe impact on the rendering performance, as each and
  every character of the text increases the polygon count a lot. This
  makes SoText3 nodes most suitable in situations where you just need
  a few characters to be placed in your scene, rather than to
  visualize complete sentences.

  <b>FILE FORMAT/DEFAULTS:</b>
  \code
    Text3 {
        string ""
        spacing 1
        justification LEFT
        parts FRONT
    }
  \endcode

  \sa SoFont, SoFontStyle, SoText2, SoAsciiText, SoProfile
*/

// *************************************************************************

#include <Inventor/nodes/SoText3.h>

#include <cstring>
#include <cfloat> // FLT_MAX, FLT_MIN

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif // HAVE_CONFIG_H

#include <Inventor/SoPrimitiveVertex.h>
#include <Inventor/actions/SoGLRenderAction.h>
#include <Inventor/actions/SoGetPrimitiveCountAction.h>
#include <Inventor/bundles/SoMaterialBundle.h>
#include <Inventor/details/SoTextDetail.h>
#include <Inventor/elements/SoFontNameElement.h>
#include <Inventor/elements/SoFontSizeElement.h>
#include <Inventor/elements/SoGLShapeHintsElement.h>
#include <Inventor/elements/SoMaterialBindingElement.h>
#include <Inventor/elements/SoLazyElement.h>
#include <Inventor/elements/SoGLCacheContextElement.h>
#include <Inventor/elements/SoComplexityTypeElement.h>
#include <Inventor/elements/SoComplexityElement.h>
#include <Inventor/elements/SoCreaseAngleElement.h>
#include <Inventor/elements/SoGLMultiTextureEnabledElement.h>
#include <Inventor/elements/SoTextOutlineEnabledElement.h>
#include <Inventor/misc/SoState.h>
#include <Inventor/misc/SoNormalGenerator.h>
#include <Inventor/nodes/SoProfile.h>
#include <Inventor/nodes/SoNurbsProfile.h>
#include <Inventor/SbLine.h>
#include <Inventor/SbBox2f.h>
#include <Inventor/lists/SbList.h>
#include <Inventor/elements/SoCacheElement.h>
#include <Inventor/system/gl.h>

#ifdef COIN_THREADSAFE
#include <Inventor/threads/SbMutex.h>
#endif // COIN_THREADSAFE

#if COIN_DEBUG
#include <Inventor/errors/SoDebugError.h>
#endif // COIN_DEBUG

#include "coindefs.h" // COIN_OBSOLETED()
#include "nodes/SoSubNodeP.h"
#include "fonts/glyph3d.h"
#include "caches/SoGlyphCache.h"

// *************************************************************************

/*!
  \enum SoText3::Part
  Used to specify which parts should be rendered/generated.
*/
/*!
  \var SoText3::Part SoText3::FRONT
  Front of characters.
*/
/*!
  \var SoText3::Part SoText3::SIDES
  Sides of characters.
*/
/*!
  \var SoText3::Part SoText3::BACK
  Back of characters.
*/
/*!
  \var SoText3::Part SoText3::ALL
  All parts.
*/

/*!
  \enum SoText3::Justification
  Used to specify horizontal string alignment.
*/
/*!
  \var SoText3::Justification SoText3::LEFT
  Left edges of strings are aligned.
*/
/*!
  \var SoText3::Justification SoText3::RIGHT
  Right edges of strings are aligned.
*/
/*!
  \var SoText3::Justification SoText3::CENTER
  Centers of strings are aligned.
*/


/*!
  \var SoMFString SoText3::string

  The set of strings to render.  Each string in the multiple value
  field will be rendered on a separate line.

  The default value of the field is a single empty string.
*/
/*!
  \var SoSFFloat SoText3::spacing

  Vertical spacing between the baselines of two consecutive horizontal lines.
  Default value is 1.0, which means that it is equal to the vertical size of
  the highest character in the bitmap alphabet.
*/
/*!
  \var SoSFEnum SoText3::justification

  Determines horizontal alignment of text strings.

  If justification is set to SoText3::LEFT, the left edge of the first string
  is at the origin and all strings are aligned with their left edges.
  If set to SoText3::RIGHT, the right edge of the first string is
  at the origin and all strings are aligned with their right edges. Otherwise,
  if set to SoText3::CENTER, the center of the first string is at the
  origin and all strings are aligned with their centers.
  The origin is always located at the baseline of the first line of text.

  Default value is SoText3::LEFT.
*/
/*!
  \var SoSFBitMask SoText3::parts

  Character parts. Default is to show only the front facing part.
*/

// FIXME: missing features, pederb 20000224
//   - Texture coordinates are not generated yet.
//   - Normals for SIDES should be smoothed.

// *************************************************************************

class SoText3P {
public:
  SoText3P(SoText3 * master) : master(master) { }

  void render(SoState * state, const cc_font_specification * fontspec, unsigned int part);
  void generate(SoAction * action, const cc_font_specification * fontspec, unsigned int part);

  SbList <float> widths;
  void setUpGlyphs(SoState * state, SoText3 * textnode);
  SbBox3f maxglyphbbox;
  SoNormalGenerator * normalgenerator;

  SoGlyphCache * cache;

  void lock(void) {
#ifdef COIN_THREADSAFE
    this->mutex.lock();
#endif // COIN_THREADSAFE
  }
  void unlock(void) {
#ifdef COIN_THREADSAFE
    this->mutex.unlock();
#endif // COIN_THREADSAFE
  }
private:
#ifdef COIN_THREADSAFE
  // FIXME: a mutex for every instance seems a bit excessive,
  // especially since Microsoft Windows might have rather strict limits on the
  // total amount of mutex resources a process (or even a user) can
  // allocate. so consider making this a class-wide instance instead.
  // -mortene.
  SbMutex mutex;
#endif // COIN_THREADSAFE
  SoText3 * master;
};

#define PRIVATE(p) ((p)->pimpl)
#define PUBLIC(p) ((p)->master)

// *************************************************************************

SO_NODE_SOURCE(SoText3);

// *************************************************************************

SoText3::SoText3(void)
{
  SO_NODE_INTERNAL_CONSTRUCTOR(SoText3);

  SO_NODE_ADD_FIELD(string, (""));
  SO_NODE_ADD_FIELD(spacing, (1.0f));
  SO_NODE_ADD_FIELD(justification, (SoText3::LEFT));
  SO_NODE_ADD_FIELD(parts, (SoText3::FRONT));

  SO_NODE_DEFINE_ENUM_VALUE(Justification, LEFT);
  SO_NODE_DEFINE_ENUM_VALUE(Justification, RIGHT);
  SO_NODE_DEFINE_ENUM_VALUE(Justification, CENTER);
  SO_NODE_SET_SF_ENUM_TYPE(justification, Justification);

  SO_NODE_DEFINE_ENUM_VALUE(Part, FRONT);
  SO_NODE_DEFINE_ENUM_VALUE(Part, SIDES);
  SO_NODE_DEFINE_ENUM_VALUE(Part, BACK);
  SO_NODE_DEFINE_ENUM_VALUE(Part, ALL);
  SO_NODE_SET_SF_ENUM_TYPE(parts, Part);

  PRIVATE(this) = new SoText3P(this);
  PRIVATE(this)->normalgenerator = new SoNormalGenerator(FALSE, 0xff);
  PRIVATE(this)->cache = NULL;
}

SoText3::~SoText3()
{
  if (PRIVATE(this)->cache) PRIVATE(this)->cache->unref();
  delete PRIVATE(this)->normalgenerator;
  delete PRIVATE(this);
}

/*!
  \copydetails SoNode::initClass(void)
*/
void
SoText3::initClass(void)
{
  SO_NODE_INTERNAL_INIT_CLASS(SoText3, SO_FROM_INVENTOR_2_1);
}

// doc in parent
void
SoText3::computeBBox(SoAction * action, SbBox3f & box, SbVec3f & center)
{
  SoState * state = action->getState();

  PRIVATE(this)->lock();
  PRIVATE(this)->setUpGlyphs(state, this);
  SoCacheElement::addCacheDependency(state, PRIVATE(this)->cache);

  const cc_font_specification * fontspec = PRIVATE(this)->cache->getCachedFontspec();

  int i, n = PRIVATE(this)->widths.getLength();
  if (n == 0) {
    PRIVATE(this)->unlock();
    return; // empty bbox
  }
  float maxw = FLT_MIN;
  for (i = 0; i < n; i++) {
    maxw = SbMax(maxw, PRIVATE(this)->widths[i]);
  }

  if (maxw == FLT_MIN) { // There is no text to bound. Returning.
    PRIVATE(this)->unlock();
    return;
  }

  SbBox2f maxbox;

  const float maxy = 0;
  float miny = -this->spacing.getValue() * fontspec->size * (n-1);

  float minx, maxx;
  switch (this->justification.getValue()) {
  case SoText3::LEFT:
    minx = 0.0f;
    maxx = maxw;
    break;
  case SoText3::RIGHT:
    minx = -maxw;
    maxx = 0.0f;
    break;
  case SoText3::CENTER:
    maxx = maxw * 0.5f;
    minx = -maxx;
    break;
  default:
    assert(0 && "unknown justification");
    minx = maxx = 0.0f;
    break;
  }

  // check profiles and extend bounding box if necessary
  float profsize = 0;
  float minz = -1.0f, maxz = 0.0f;

  const SoNodeList profilenodes = SoProfileElement::get(state);
  int numprofiles = profilenodes.getLength();
  if ( numprofiles > 0) {
    assert(profilenodes[0]->getTypeId().isDerivedFrom(SoProfile::getClassTypeId()));
    for (int i = numprofiles-1; i >= 0; i--) {
      SoProfile *pn = (SoProfile *)profilenodes[i];
      if (pn->isOfType(SoNurbsProfile::getClassTypeId())) {
        // Don't use SoProfile::getVertices() for SoNurbsProfile
        // nodes as this would cause a call to the GLU library, which
        // requires a valid GL context. Instead we approximate using
        // SoNurbsProfile::getTrimCurve(), and use the control points
        // to calculate the bounding box. This is an approximation,
        // but the same technique is used in So[Indexed]NurbsSurface
        // and So[Indexed]NurbsCurve. To avoid this approximation we
        // would need our own NURBS library.    pederb, 20000926
        SoNurbsProfile * np = (SoNurbsProfile*) pn;
        float * knots;
        int32_t numknots;
        int dim;
        int32_t numpts;
        float * points;
        np->getTrimCurve(state, numpts, points, dim,
                         numknots, knots);
        for (int j = 0; j < numpts; j++) {
          if (-points[j*dim] > maxz) maxz = -points[j*dim];
          if (-points[j*dim] < minz) minz = -points[j*dim];
          if (points[j*dim+1] > profsize) profsize = points[j*dim+1];
        }
      }
      else {
        int32_t num;
        SbVec2f *coords;
        pn->getVertices(state, num, coords);
        for (int j = 0; j < num; j++) {
          if (-coords[j][0] > maxz) maxz = -coords[j][0];
          if (-coords[j][0] < minz) minz = -coords[j][0];
          if (coords[j][1] > profsize) profsize = coords[j][1];
        }
      }
    }
  }
  else {
    // extrude
    if (this->parts.getValue() == SoText3::BACK) {
      maxz = -1.0f;
    }
    else if (this->parts.getValue() == SoText3::FRONT) {
      minz = 0.0f;
    }
  }

  box.setBounds(SbVec3f(minx, miny, minz), SbVec3f(maxx, maxy, maxz));

  // Expanding bbox so that glyphs like 'j's and 'q's are completely inside.
  box.extendBy(SbVec3f(0,PRIVATE(this)->maxglyphbbox.getMin()[1] - (n-1) * fontspec->size, 0));
  box.extendBy(PRIVATE(this)->maxglyphbbox);

  box.extendBy(SbVec3f(box.getMax()[0] + profsize, box.getMax()[1] + profsize, 0));
  box.extendBy(SbVec3f(box.getMin()[0] - profsize, box.getMin()[1] - profsize, 0));

  center = box.getCenter();
  PRIVATE(this)->unlock();
}


/*!
  Not implemented. Should probably have been private in Open Inventor API. Let us
  know if you need this method for anything, and we'll implement it.
*/
SbBox3f
SoText3::getCharacterBounds(SoState * COIN_UNUSED_ARG(state), int COIN_UNUSED_ARG(stringindex), int COIN_UNUSED_ARG(charindex))
{
  COIN_OBSOLETED();
  return SbBox3f();
}

// doc in parent
void
SoText3::GLRender(SoGLRenderAction * action)
{
  if (!this->shouldGLRender(action))
    return;

  PRIVATE(this)->lock();

  SoState * state = action->getState();

  // FIXME: implement this feature. 20040820 mortene.
  static SbBool warned = FALSE;
  if (!warned) {
    const int stackidx = SoTextOutlineEnabledElement::getClassStackIndex();
    const SbBool outlinepresence = state->isElementEnabled(stackidx);

    if (outlinepresence && SoTextOutlineEnabledElement::get(state)) {
#if COIN_DEBUG
      SoDebugError::postWarning("SoText3::GLRender",
                                "Support for rendering SoText3 nodes in outline "
                                "(i.e. heeding the SoTextOutlineEnabledElement) "
                                "not yet implemented.");
#endif // COIN_DEBUG
      warned = TRUE;
    }
  }


  PRIVATE(this)->setUpGlyphs(state, this);
  SoCacheElement::addCacheDependency(state, PRIVATE(this)->cache);

  const cc_font_specification * fontspec = PRIVATE(this)->cache->getCachedFontspec();

  SoMaterialBindingElement::Binding binding = SoMaterialBindingElement::get(state);
  SoMaterialBundle mb(action);
  mb.sendFirst();

  const unsigned int prts = this->parts.getValue();
  SoLazyElement * lazyelement = SoLazyElement::getInstance(state);
  const int numdiffuse = lazyelement->getNumDiffuse();

  SbBool matperpart = (binding != SoMaterialBindingElement::OVERALL);

  if (prts & SoText3::FRONT) {
    PRIVATE(this)->render(state, fontspec, SoText3::FRONT);
  }
  if (prts & SoText3::SIDES) {
    if (matperpart && (numdiffuse > 1))
        mb.send(1, FALSE);
    PRIVATE(this)->render(state, fontspec, SoText3::SIDES);
  }
  if (prts & SoText3::BACK) {
    if (matperpart && (numdiffuse > 2))
      mb.send(2, FALSE);
    PRIVATE(this)->render(state, fontspec, SoText3::BACK);
  }

  if (SoComplexityTypeElement::get(state) == SoComplexityTypeElement::OBJECT_SPACE) {
    SoGLCacheContextElement::shouldAutoCache(state, SoGLCacheContextElement::DO_AUTO_CACHE);
    SoGLCacheContextElement::incNumShapes(state);
  }
  PRIVATE(this)->unlock();
}

// doc in parent
void
SoText3::getPrimitiveCount(SoGetPrimitiveCountAction * action)
{
  if (action->is3DTextCountedAsTriangles()) {
    // will cause a call to generatePrimitives()
    // slow, but we can't be bothered to implement a new loop to count triangles
    inherited::getPrimitiveCount(action);
  }
  else {
    action->addNumText(this->string.getNum());
  }
}

// doc in parent
void
SoText3::generatePrimitives(SoAction * action)
{
  SoState * state = action->getState();

  PRIVATE(this)->lock();
  PRIVATE(this)->setUpGlyphs(state, this);
  
  if (PRIVATE(this)->cache) {
    const cc_font_specification * fontspec = PRIVATE(this)->cache->getCachedFontspec();
    unsigned int prts = this->parts.getValue();
    
    if (prts & SoText3::FRONT) {
      PRIVATE(this)->generate(action, fontspec, SoText3::FRONT);
    }
    if (prts & SoText3::SIDES) {
      PRIVATE(this)->generate(action, fontspec, SoText3::SIDES);
    }
    if (prts & SoText3::BACK) {
      PRIVATE(this)->generate(action, fontspec, SoText3::BACK);
    }
  }
  PRIVATE(this)->unlock();
}

// doc in parent
SoDetail *
SoText3::createTriangleDetail(SoRayPickAction * COIN_UNUSED_ARG(action),
                              const SoPrimitiveVertex * v1,
                              const SoPrimitiveVertex * COIN_UNUSED_ARG(v2),
                              const SoPrimitiveVertex * COIN_UNUSED_ARG(v3),
                              SoPickedPoint * COIN_UNUSED_ARG(pp))
{
  // generatePrimitives() places text details inside each primitive vertex
  assert(v1->getDetail());
  return v1->getDetail()->copy();
}

void
SoText3P::render(SoState * state, const cc_font_specification * fontspec,
                 unsigned int part)
{
  int i, n = this->widths.getLength();

  int firstprofile = -1;
  int32_t profnum;
  SbVec2f *profcoords;
  float nearz =  FLT_MAX;
  float farz  = -FLT_MAX;

  float creaseangle = SoCreaseAngleElement::get(state);

  SbBool do2Dtextures = FALSE;
  SbBool do3Dtextures = FALSE;
  if (SoGLMultiTextureEnabledElement::get(state, 0)) {
    do2Dtextures = TRUE;
    if (SoGLMultiTextureEnabledElement::getMode(state, 0) ==
        SoMultiTextureEnabledElement::TEXTURE3D) {
      do3Dtextures = TRUE;
    }
  }
  // FIXME: implement proper support for 3D-texturing, and get rid of
  // this. (20031010 handegar)
  if (do3Dtextures) {
    static SbBool first = TRUE;
    if (first) {
      first = FALSE;
#if COIN_DEBUG
      SoDebugError::postWarning("SoText3::GLRender",
                                "3D-textures not supported for this node type yet.");
#endif // COIN_DEBUG
    }
  }


  const SoNodeList & profilenodes = SoProfileElement::get(state);
  int numprofiles = profilenodes.getLength();
  // Indicates if a valid profile has been specified. If it has not,
  // text will be rendered extruded.
  SbBool validprofile = FALSE;

  if (numprofiles > 0) {
    assert(profilenodes[0]->getTypeId().isDerivedFrom(SoProfile::getClassTypeId()));
    // Find near/far z (for modifying position of front/back)

    for (int l=numprofiles-1; l>=0; l--) {
      SoProfile * pn = (SoProfile *) profilenodes[l];
      pn->getVertices(state, profnum, profcoords);

      if (profnum > 0) {
        if (profcoords[profnum-1][0] > farz) farz = profcoords[profnum-1][0];
        if (profcoords[0][0] < nearz) nearz = profcoords[0][0];
        if (pn->linkage.getValue() == SoProfile::START_FIRST) {
          if (firstprofile == -1) {
            firstprofile = l;
            validprofile = TRUE;
          }
          break;
        }
      }
    }
    nearz = -nearz;
    farz = -farz;
  }

  // If no profiles have been specified, or if no valid coordinates have
  // been given, set near and far / front and back values to default.
  if (!validprofile) {
    nearz = 0.0;
    farz = -1.0;
  }

  float ypos = 0.0f;
  for (i = 0; i < n; i++) {

    float xpos = 0.0f;
    switch (PUBLIC(this)->justification.getValue()) {
    case SoText3::RIGHT:
      xpos = -this->widths[i];
      break;
    case SoText3::CENTER:
      xpos = - this->widths[i] * 0.5f;
      break;
    }

    SbString str = PUBLIC(this)->string[i];
    cc_glyph3d * prevglyph = NULL;
    const char * p = str.getString();
    size_t length = cc_string_utf8_validate_length(p);
    // No assertion as zero length is handled correctly (results in a new line)

    for (unsigned int strcharidx = 0; strcharidx < length; strcharidx++) {
      uint32_t glyphidx = 0;

      glyphidx = cc_string_utf8_get_char(p);
      p = cc_string_utf8_next_char(p);

      cc_glyph3d * glyph = cc_glyph3d_ref(glyphidx, fontspec);
      const SbVec2f * coords = (SbVec2f *) cc_glyph3d_getcoords(glyph);

      // Get kerning
      if (strcharidx > 0) {
        float kerningx, kerningy;
        cc_glyph3d_getkerning(prevglyph, glyph, &kerningx, &kerningy);
        xpos += kerningx * fontspec->size;
      }
      if (prevglyph) {
        cc_glyph3d_unref(prevglyph);
      }
      prevglyph = glyph;

      if (part != SoText3::SIDES) {  // FRONT & BACK
        const int * ptr = cc_glyph3d_getfaceindices(glyph);
        glBegin(GL_TRIANGLES);

        while (*ptr >= 0) {
          SbVec2f v0, v1, v2;
          float zval;
          if (part == SoText3::FRONT) {
            glNormal3f(0.0f, 0.0f, 1.0f);
            v2 = coords[*ptr++];
            v1 = coords[*ptr++];
            v0 = coords[*ptr++];
            zval = nearz;
          }
          else {  // BACK
            glNormal3f(0.0f, 0.0f, -1.0f);
            v0 = coords[*ptr++];
            v1 = coords[*ptr++];
            v2 = coords[*ptr++];
            zval = farz;
          }
          if(do2Dtextures)
            glTexCoord2f(v0[0] + xpos/fontspec->size,
                         v0[1] + ypos/fontspec->size);
          glVertex3f(v0[0] * fontspec->size + xpos, v0[1] * fontspec->size + ypos, zval);
          if(do2Dtextures)
            glTexCoord2f(v1[0] + xpos/fontspec->size,
                         v1[1] + ypos/fontspec->size);
          glVertex3f(v1[0] * fontspec->size + xpos, v1[1] * fontspec->size + ypos, zval);
          if(do2Dtextures)
            glTexCoord2f(v2[0] + xpos/fontspec->size,
                         v2[1] + ypos/fontspec->size);
          glVertex3f(v2[0] * fontspec->size + xpos, v2[1] * fontspec->size + ypos, zval);

        }
        glEnd();
      }
      else { // SIDES

        if (!validprofile) {  // no profile - extrude
          const int * ptr = cc_glyph3d_getedgeindices(glyph);
          SbVec2f v0, v1;
          int counter = 0;

          glBegin(GL_QUADS);

          while (*ptr >= 0) {
            v1 = coords[*ptr++];
            v0 = coords[*ptr++];
            const int * ccw = (int *) cc_glyph3d_getnextccwedge(glyph, counter);
            const int * cw  = (int *) cc_glyph3d_getnextcwedge(glyph, counter);
            SbVec3f vleft(coords[*(ccw+1)][0], coords[*(ccw+1)][1], 0);
            SbVec3f vright(coords[*cw][0], coords[*cw][1], 0);
            counter++;

            // create two 'normal' vectors pointing out from the edges
            SbVec3f normala(vright[0] - v0[0], vright[1] - v0[1], 0.0f);
            normala = normala.cross(SbVec3f(0.0f, 0.0f,  1.0f));
            if (normala.length() > 0)
              normala.normalize();

            SbVec3f normalb(v1[0] - vleft[0], v1[1] - vleft[1], 0.0f);
            normalb = normalb.cross(SbVec3f(0.0f, 0.0f,  1.0f));
            if (normalb.length() > 0)
              normalb.normalize();

            SbBool flatshading = FALSE;
            float dot = normala.dot(normalb);

            if(acos(dot) > creaseangle) {
              normala = SbVec3f(v1[0] - v0[0], v1[1] - v0[1], 0.0f);
              normala = normala.cross(SbVec3f(0.0f, 0.0f, 1.0f));
              if (normala.length() > 0)
                normala.normalize();

              flatshading = TRUE;
            }

            if (!flatshading) {
              if(do2Dtextures)
                 glTexCoord2f(v1[0] + xpos/fontspec->size,
                              v1[1] + ypos/fontspec->size);
              glNormal3fv(normala.getValue());
              glVertex3f(v1[0]*fontspec->size + xpos, v1[1]*fontspec->size + ypos, 0.0f);

              if(do2Dtextures)
                glTexCoord2f(v0[0] + xpos/fontspec->size,
                             v0[1] + ypos/fontspec->size);
              glNormal3fv(normalb.getValue());
              glVertex3f(v0[0]*fontspec->size + xpos, v0[1]*fontspec->size + ypos, 0.0f);

              if(do2Dtextures)
                glTexCoord2f(v0[0] + xpos/fontspec->size,
                             v0[1] + ypos/fontspec->size);
              glNormal3fv(normalb.getValue());
              glVertex3f(v0[0]*fontspec->size + xpos, v0[1]*fontspec->size + ypos, -1.0f);

              if(do2Dtextures)
                glTexCoord2f(v1[0] + xpos/fontspec->size,
                             v1[1] + ypos/fontspec->size);
              glNormal3fv(normala.getValue());
              glVertex3f(v1[0]*fontspec->size + xpos, v1[1]*fontspec->size + ypos, -1.0f);

            }
            else {
              glNormal3fv(normala.getValue());
              if(do2Dtextures)
                glTexCoord2f(v1[0] + xpos/fontspec->size,
                             v1[1] + ypos/fontspec->size);
              glVertex3f(v1[0]*fontspec->size + xpos, v1[1]*fontspec->size + ypos, 0.0f);

              if(do2Dtextures)
                glTexCoord2f(v0[0] + xpos/fontspec->size,
                             v0[1] + ypos/fontspec->size);
              glVertex3f(v0[0]*fontspec->size + xpos, v0[1]*fontspec->size + ypos, 0.0f);

              if(do2Dtextures)
                glTexCoord2f(v0[0] + xpos/fontspec->size,
                             v0[1] + ypos/fontspec->size);
              glVertex3f(v0[0]*fontspec->size + xpos, v0[1]*fontspec->size + ypos, -1.0f);

              if(do2Dtextures)
                glTexCoord2f(v1[0] + xpos/fontspec->size,
                             v1[1] + ypos/fontspec->size);
              glVertex3f(v1[0]*fontspec->size + xpos, v1[1]*fontspec->size + ypos, -1.0f);
            }
          }
          glEnd();

        }
        else {  // profile
          assert(validprofile && firstprofile >= 0);

          const int * indices = cc_glyph3d_getedgeindices(glyph);
          int ind = 0;
          SbVec3f normala, normalb;

          SbList <SbVec3f> vertexlist;
          this->normalgenerator->reset(FALSE);

          while (*indices >= 0) {

            int i0 = *indices++;
            int i1 = *indices++;
            SbVec3f va(coords[i0][0], coords[i0][1], nearz);
            SbVec3f vb(coords[i1][0], coords[i1][1], nearz);
            const int * ccw = (int *) cc_glyph3d_getnextccwedge(glyph, ind);
            const int * cw  = (int *) cc_glyph3d_getnextcwedge(glyph, ind);
            SbVec3f vleft(coords[*(ccw+1)][0], coords[*(ccw+1)][1], nearz);
            SbVec3f vright(coords[*cw][0], coords[*cw][1], nearz);
            ind++;

            va[0] = va[0] * fontspec->size;
            va[1] = va[1] * fontspec->size;
            vb[0] = vb[0] * fontspec->size;
            vb[1] = vb[1] * fontspec->size;
            vleft[0] = vleft[0] * fontspec->size;
            vleft[1] = vleft[1] * fontspec->size;
            vright[0] = vright[0] * fontspec->size;
            vright[1] = vright[1] * fontspec->size;

            // create two 'normal' vectors pointing out from the edges
            SbVec3f normala(vleft[0] - va[0], vleft[1] - va[1], 0.0f);
            normala = normala.cross(SbVec3f(0.0f, 0.0f,  -1.0f));
            if (normala.length() > 0)
              normala.normalize();

            SbVec3f normalb(vb[0] - vright[0], vb[1] - vright[1], 0.0f);
            normalb = normalb.cross(SbVec3f(0.0f, 0.0f,  -1.0f));
            if (normalb.length() > 0)
              normalb.normalize();

            SoProfile * pn = (SoProfile *) profilenodes[firstprofile];
            pn->getVertices(state, profnum, profcoords);

            SbVec3f vc,vd;
            SbVec2f starta(va[0], va[1]);
            SbVec2f startb(vb[0], vb[1]);

            for (int j=firstprofile; j<numprofiles; j++) {
              SoProfile * pn = (SoProfile *) profilenodes[j];
              pn->getVertices(state, profnum, profcoords);

              for (int k=1; k<profnum; k++) {

                // Calc points for two next faces
                vd[0] = starta[0] + (profcoords[k][1] * normalb[0]);
                vd[1] = starta[1] + (profcoords[k][1] * normalb[1]);
                vd[2] = -profcoords[k][0];
                vc[0] = startb[0] + (profcoords[k][1] * normala[0]);
                vc[1] = startb[1] + (profcoords[k][1] * normala[1]);
                vc[2] = -profcoords[k][0];

                // The windows tessellation sometimes return
                // illegal/empty tris. A test must be done to
                // prevent stdout from being flooded with
                // normalize() warnings from inside the normal
                // generator.

                if ((va != vd) && (va != vb) && (vd != vb)) {
                  vertexlist.append(va);
                  vertexlist.append(vd);
                  vertexlist.append(vb);
                  normalgenerator->triangle(va,vd,vb);
                }

                if ((vb != vd) && (vb != vc) && (vd != vc)) {
                  vertexlist.append(vb);
                  vertexlist.append(vd);
                  vertexlist.append(vc);
                  normalgenerator->triangle(vb,vd,vc);
                }

                va = vd;
                vb = vc;

              }
            }

          }

          normalgenerator->generate(creaseangle);
          const SbVec3f * normals = normalgenerator->getNormals();
          const int size = vertexlist.getLength();

          // NOTE: We add the xpos and ypos to each vertex at this
          // point because Linux systems seems to accumulate an error
          // when calculating the normals (i.e. two 'o's in a row
          // doesn't get the same normals due to the xpos
          // difference). This doesn't happen on Windows so it is
          // probably a floating point precision issue linked to the
          // compilator. (Tested on MSVC 6 and GCC 2.95.4) (20031010
          // handegar).

          glBegin(GL_TRIANGLES);

          for (int z = 0;z < size;z += 3) {

            // FIXME: Add proper texturing for profile
            // coords. (20031010 handegar)

            glNormal3fv(normals[z+2].getValue());
            glVertex3fv(SbVec3f(vertexlist[z+2][0] + xpos,
                                vertexlist[z+2][1] + ypos,
                                vertexlist[z+2][2]).getValue());

            glNormal3fv(normals[z+1].getValue());
            glVertex3fv(SbVec3f(vertexlist[z+1][0] + xpos,
                                vertexlist[z+1][1] + ypos,
                                vertexlist[z+1][2]).getValue());

            glNormal3fv(normals[z].getValue());
            glVertex3fv(SbVec3f(vertexlist[z][0] + xpos,
                                vertexlist[z][1] + ypos,
                                vertexlist[z][2]).getValue());
          }
          glEnd();

          vertexlist.truncate(0);

        }

      }

      float advancex, advancey;
      cc_glyph3d_getadvance(glyph, &advancex, &advancey);
      xpos += advancex * fontspec->size;

    }
    if (prevglyph) {
      cc_glyph3d_unref(prevglyph);
      prevglyph = NULL;
    }
    ypos -= fontspec->size * PUBLIC(this)->spacing.getValue();
  }
}

// render text geometry
void
SoText3::render(SoState * COIN_UNUSED_ARG(state), unsigned int COIN_UNUSED_ARG(part))
{
  assert(FALSE && "obsoleted");
}

void
SoText3::generate(SoAction * COIN_UNUSED_ARG(action), unsigned int COIN_UNUSED_ARG(part))
{
  assert(FALSE && "obsoleted");
}

// generate text geometry
void
SoText3P::generate(SoAction * action, const cc_font_specification * fontspec,
                   unsigned int part)
{
  SoState * state = action->getState();

  // SoCreaseAngleElement is not enabled for SoGetPrimitiveCountAction.
  float creaseangle = 0.5f;
  if (state->isElementEnabled(SoCreaseAngleElement::getClassStackIndex())) {
    creaseangle = SoCreaseAngleElement::get(state);
  }
  SoPrimitiveVertex vertex;
  SoTextDetail detail;
  detail.setPart(part);
  vertex.setDetail(&detail);

  // we might get here from getPrimitiveCount(). We therefore need to
  // check if lazy element is enabled
  if (SoMaterialBindingElement::get(state) !=
      SoMaterialBindingElement::OVERALL &&
      state->isElementEnabled(SoLazyElement::getClassStackIndex())) {

    SoLazyElement * lazyelement = SoLazyElement::getInstance(state);
    const int numdiffuse = lazyelement->getNumDiffuse();

    if (part == SoText3::SIDES && (numdiffuse > 1))
      vertex.setMaterialIndex(1);
    else if (part == SoText3::BACK && (numdiffuse > 2))
      vertex.setMaterialIndex(2);
  }

  SbBool do2Dtextures = FALSE;
  SbBool do3Dtextures = FALSE;

  // not all actions have these elements enabled
  // (for instance SoGetPrimitiveCountAction)
  if (state->isElementEnabled(SoMultiTextureEnabledElement::getClassStackIndex())) {
    if (SoMultiTextureEnabledElement::get(state)) do2Dtextures = TRUE;
  }
  // FIXME: implement proper support for 3D-texturing, and get rid of
  // this. (20031010 handegar)
  if (do3Dtextures) {
    static SbBool first = TRUE;
    if (first) {
      first = FALSE;
#if COIN_DEBUG
      SoDebugError::postWarning("SoText3::GLRender",
                                "3D-textures not supported for this node type yet.");
#endif // COIN_DEBUG
    }
  }


  int i, n = this->widths.getLength();

  int firstprofile = -1;
  int32_t profnum;
  SbVec2f *profcoords;
  float nearz =  FLT_MAX;
  float farz  = -FLT_MAX;

  const SoNodeList & profilenodes = SoProfileElement::get(state);
  int numprofiles = profilenodes.getLength();

  if (numprofiles > 0) {
    assert(profilenodes[0]->getTypeId().isDerivedFrom(SoProfile::getClassTypeId()));
    // Find near/far z (for modifying position of front/back)

    for (int l = numprofiles-1; l >= 0; l--) {
      SoProfile * pn = (SoProfile *)profilenodes[l];
      pn->getVertices(state, profnum, profcoords);

      if (profnum > 0) {
        if (profcoords[profnum-1][0] > farz) farz = profcoords[profnum-1][0];
        if (profcoords[0][0] < nearz) nearz = profcoords[0][0];
        if (pn->linkage.getValue() == SoProfile::START_FIRST) {
          if (firstprofile == -1) firstprofile = l;
          break;
        }
      }
    }

    nearz = -nearz;
    farz = -farz;
  }
  else {
    nearz = 0.0;
    farz = -1.0;
  }

  float ypos = 0.0f;
  for (i = 0; i < n; i++) {
    detail.setStringIndex(i);
    float xpos = 0.0f;
    switch (PUBLIC(this)->justification.getValue()) {
    case SoText3::RIGHT:
      xpos = -this->widths[i];
      break;
    case SoText3::CENTER:
      xpos = - this->widths[i] * 0.5f;
      break;
    }

    SbString str = PUBLIC(this)->string[i];
    cc_glyph3d * prevglyph = NULL;
    const char * p = str.getString();
    size_t length = cc_string_utf8_validate_length(p);
    // No assertion as zero length is handled correctly (results in a new line)

    for (unsigned int strcharidx = 0; strcharidx < length; strcharidx++) {
      uint32_t glyphidx = 0;

      glyphidx = cc_string_utf8_get_char(p);
      p = cc_string_utf8_next_char(p);

      cc_glyph3d * glyph = cc_glyph3d_ref(glyphidx, fontspec);
      const SbVec2f * coords = (SbVec2f *) cc_glyph3d_getcoords(glyph);

      detail.setCharacterIndex(strcharidx);

      // Get kerning
      if (strcharidx > 0) {
        float kerningx, kerningy;
        cc_glyph3d_getkerning(prevglyph, glyph, &kerningx, &kerningy);
        xpos += kerningx * fontspec->size;
      }
      if (prevglyph) {
        cc_glyph3d_unref(prevglyph);
      }
      prevglyph = glyph;

      if (part != SoText3::SIDES) {  // FRONT & BACK
        const int * ptr = cc_glyph3d_getfaceindices(glyph);
        PUBLIC(this)->beginShape(action, SoShape::TRIANGLES, NULL);

        while (*ptr >= 0) {
          SbVec2f v0, v1, v2;
          float zval;
          if (part == SoText3::FRONT) {
            vertex.setNormal(SbVec3f(0.0f, 0.0f, 1.0f));
            v2 = coords[*ptr++];
            v1 = coords[*ptr++];
            v0 = coords[*ptr++];
            zval = nearz;
          }
          else {  // BACK
            vertex.setNormal(SbVec3f(0.0f, 0.0f, -1.0f));
            v0 = coords[*ptr++];
            v1 = coords[*ptr++];
            v2 = coords[*ptr++];
            zval = farz;
          }

          if(do2Dtextures) {
            vertex.setTextureCoords(SbVec2f(v0[0] + xpos/fontspec->size, v0[1] + ypos/fontspec->size));
          }
          vertex.setPoint(SbVec3f(v0[0] * fontspec->size + xpos, v0[1] * fontspec->size + ypos, zval));
          PUBLIC(this)->shapeVertex(&vertex);

          if(do2Dtextures) {
            vertex.setTextureCoords(SbVec2f(v1[0] + xpos/fontspec->size, v1[1] + ypos/fontspec->size));
          }
          vertex.setPoint(SbVec3f(v1[0] * fontspec->size + xpos, v1[1] * fontspec->size + ypos, zval));
          PUBLIC(this)->shapeVertex(&vertex);

          if(do2Dtextures) {
            vertex.setTextureCoords(SbVec2f(v2[0] + xpos/fontspec->size, v2[1] + ypos/fontspec->size));
          }
          vertex.setPoint(SbVec3f(v2[0] * fontspec->size + xpos, v2[1] * fontspec->size + ypos, zval));
          PUBLIC(this)->shapeVertex(&vertex);
        }

        PUBLIC(this)->endShape();

      }
      else { // SIDES
        if (profilenodes.getLength() == 0) {  // no profile - extrude

          const int * ptr = cc_glyph3d_getedgeindices(glyph);
          SbVec2f v0, v1;
          int counter = 0;
          PUBLIC(this)->beginShape(action, SoShape::QUADS, NULL);

          while (*ptr >= 0) {
            v1 = coords[*ptr++];
            v0 = coords[*ptr++];
            const int * ccw = (int *) cc_glyph3d_getnextccwedge(glyph, counter);
            const int * cw  = (int *) cc_glyph3d_getnextcwedge(glyph, counter);
            SbVec3f vleft(coords[*(ccw+1)][0], coords[*(ccw+1)][1], 0);
            SbVec3f vright(coords[*cw][0], coords[*cw][1], 0);
            counter++;

            v0[0] = v0[0] * fontspec->size;
            v0[1] = v0[1] * fontspec->size;
            v1[0] = v1[0] * fontspec->size;
            v1[1] = v1[1] * fontspec->size;
            vleft[0] = vleft[0] * fontspec->size;
            vleft[1] = vleft[1] * fontspec->size;
            vright[0] = vright[0] * fontspec->size;
            vright[1] = vright[1] * fontspec->size;

            // create two 'normal' vectors pointing out from the edges
            SbVec3f normala(vright[0] - v0[0], vright[1] - v0[1], 0.0f);
            normala = normala.cross(SbVec3f(0.0f, 0.0f,  1.0f));
            if (normala.length() > 0)
              normala.normalize();

            SbVec3f normalb(v1[0] - vleft[0], v1[1] - vleft[1], 0.0f);
            normalb = normalb.cross(SbVec3f(0.0f, 0.0f,  1.0f));
            if (normalb.length() > 0)
              normalb.normalize();

            SbBool flatshading = FALSE;
            float dot = normala.dot(normalb);
            if(acos(dot) > creaseangle) {
              normala = SbVec3f(v1[0] - v0[0], v1[1] - v0[1], 0.0f);
              normala = normala.cross(SbVec3f(0.0f, 0.0f,  1.0f));
              if (normala.length() > 0)
                normala.normalize();
              flatshading = TRUE;
            }

            if (!flatshading) {
              if (do2Dtextures) {
                vertex.setTextureCoords(SbVec2f(v1[0] + xpos/fontspec->size,
                                                v1[1] + ypos/fontspec->size));
              }
              vertex.setNormal(normala);
              vertex.setPoint(SbVec3f(v1[0]*fontspec->size + xpos, v1[1]*fontspec->size + ypos, 0.0f));
              PUBLIC(this)->shapeVertex(&vertex);

              if (do2Dtextures) {
                vertex.setTextureCoords(SbVec2f(v0[0] + xpos/fontspec->size,
                                                v0[1] + ypos/fontspec->size));
              }
              vertex.setNormal(normalb);
              vertex.setPoint(SbVec3f(v0[0]*fontspec->size + xpos, v0[1]*fontspec->size + ypos, 0.0f));
              PUBLIC(this)->shapeVertex(&vertex);

              if (do2Dtextures) {
                vertex.setTextureCoords(SbVec2f(v0[0] + xpos/fontspec->size,
                                                v0[1] + ypos/fontspec->size));
              }
              vertex.setNormal(normalb);
              vertex.setPoint(SbVec3f(v0[0]*fontspec->size + xpos, v0[1]*fontspec->size + ypos, -1.0f));
              PUBLIC(this)->shapeVertex(&vertex);

              if (do2Dtextures) {
                vertex.setTextureCoords(SbVec2f(v1[0] + xpos/fontspec->size,
                                                v1[1] + ypos/fontspec->size));
              }
              vertex.setNormal(normala);
              vertex.setPoint(SbVec3f(v1[0]*fontspec->size + xpos, v1[1]*fontspec->size + ypos, -1.0f));
              PUBLIC(this)->shapeVertex(&vertex);
            }
            else {
              vertex.setNormal(normala);

              if (do2Dtextures) {
                vertex.setTextureCoords(SbVec2f(v1[0] + xpos/fontspec->size,
                                                v1[1] + ypos/fontspec->size));
              }
              vertex.setPoint(SbVec3f(v1[0]*fontspec->size + xpos, v1[1]*fontspec->size + ypos, 0.0f));
              PUBLIC(this)->shapeVertex(&vertex);

              if (do2Dtextures) {
                vertex.setTextureCoords(SbVec2f(v0[0] + xpos/fontspec->size,
                                                v0[1] + ypos/fontspec->size));
              }
              vertex.setPoint(SbVec3f(v0[0]*fontspec->size + xpos, v0[1]*fontspec->size + ypos, 0.0f));
              PUBLIC(this)->shapeVertex(&vertex);

              if (do2Dtextures) {
                vertex.setTextureCoords(SbVec2f(v0[0] + xpos/fontspec->size,
                                                v0[1] + ypos/fontspec->size));
              }
              vertex.setPoint(SbVec3f(v0[0]*fontspec->size + xpos, v0[1]*fontspec->size + ypos, -1.0f));
              PUBLIC(this)->shapeVertex(&vertex);

              if (do2Dtextures) {
                vertex.setTextureCoords(SbVec2f(v1[0] + xpos/fontspec->size,
                                                v1[1] + ypos/fontspec->size));
              }
              vertex.setPoint(SbVec3f(v1[0]*fontspec->size + xpos, v1[1]*fontspec->size + ypos, -1.0f));
              PUBLIC(this)->shapeVertex(&vertex);

            }
          }

          PUBLIC(this)->endShape();

        }
        else {  // profile

          const int *indices = cc_glyph3d_getedgeindices(glyph);
          int ind = 0;
          SbVec3f normala, normalb;

          SbList <SbVec3f> vertexlist;
          this->normalgenerator->reset(FALSE);

          while (*indices >= 0) {

            int i0 = *indices++;
            int i1 = *indices++;
            SbVec3f va(coords[i0][0], coords[i0][1], nearz);
            SbVec3f vb(coords[i1][0], coords[i1][1], nearz);
            const int *ccw = (int *) cc_glyph3d_getnextccwedge(glyph, ind);
            const int *cw  = (int *) cc_glyph3d_getnextcwedge(glyph, ind);
            SbVec3f vleft(coords[*(ccw+1)][0], coords[*(ccw+1)][1], nearz);
            SbVec3f vright(coords[*cw][0], coords[*cw][1], nearz);
            ind++;

            va[0] = va[0] * fontspec->size;
            va[1] = va[1] * fontspec->size;
            vb[0] = vb[0] * fontspec->size;
            vb[1] = vb[1] * fontspec->size;
            vleft[0] = vleft[0] * fontspec->size;
            vleft[1] = vleft[1] * fontspec->size;
            vright[0] = vright[0] * fontspec->size;
            vright[1] = vright[1] * fontspec->size;

            // create two 'normal' vectors pointing out from the edges
            SbVec3f normala(vleft[0] - va[0], vleft[1] - va[1], 0.0f);
            normala = normala.cross(SbVec3f(0.0f, 0.0f,  -1.0f));
            if (normala.length() > 0)
              normala.normalize();

            SbVec3f normalb(vb[0] - vright[0], vb[1] - vright[1], 0.0f);
            normalb = normalb.cross(SbVec3f(0.0f, 0.0f,  -1.0f));
            if (normalb.length() > 0)
              normalb.normalize();

            SoProfile *pn = (SoProfile *)profilenodes[firstprofile];
            pn->getVertices(state, profnum, profcoords);

            SbVec3f vc,vd;
            SbVec2f starta(va[0], va[1]);
            SbVec2f startb(vb[0], vb[1]);

            for (int j=firstprofile; j<numprofiles; j++) {
              SoProfile *pn = (SoProfile *)profilenodes[j];
              pn->getVertices(state, profnum, profcoords);

              for (int k=1; k<profnum; k++) {

                // Calc points for two next faces
                vd[0] = starta[0] + (profcoords[k][1] * normalb[0]);
                vd[1] = starta[1] + (profcoords[k][1] * normalb[1]);
                vd[2] = -profcoords[k][0];
                vc[0] = startb[0] + (profcoords[k][1] * normala[0]);
                vc[1] = startb[1] + (profcoords[k][1] * normala[1]);
                vc[2] = -profcoords[k][0];

                // The windows tessellation sometimes return
                // illegal/empty tris. A test must be done to prevent
                // stdout from being flooded with normalize() warnings
                // from inside the normal generator.

                if ((va != vd) && (va != vb) && (vd != vb)) {
                  vertexlist.append(va);
                  vertexlist.append(vd);
                  vertexlist.append(vb);
                  normalgenerator->triangle(va,vd,vb);
                }

                if ((vb != vd) && (vb != vc) && (vd != vc)) {
                  vertexlist.append(vb);
                  vertexlist.append(vd);
                  vertexlist.append(vc);
                  normalgenerator->triangle(vb,vd,vc);
                }

                va = vd;
                vb = vc;

              }

            }

          }


          normalgenerator->generate(creaseangle);
          const SbVec3f * normals = normalgenerator->getNormals();
          const int size = vertexlist.getLength();

          PUBLIC(this)->beginShape(action, SoShape::TRIANGLES, NULL);
          for (int z = 0;z < size;z += 3) {
            vertex.setNormal(normals[z+2].getValue());
            vertex.setPoint(SbVec3f(vertexlist[z+2][0] + xpos,
                                    vertexlist[z+2][1] + ypos,
                                    vertexlist[z+2][2]).getValue());
            PUBLIC(this)->shapeVertex(&vertex);

            vertex.setNormal(normals[z+1].getValue());
            vertex.setPoint(SbVec3f(vertexlist[z+1][0] + xpos,
                                    vertexlist[z+1][1] + ypos,
                                    vertexlist[z+1][2]).getValue());
            PUBLIC(this)->shapeVertex(&vertex);

            vertex.setNormal(normals[z].getValue());
            vertex.setPoint(SbVec3f(vertexlist[z][0] + xpos,
                                    vertexlist[z][1] + ypos,
                                    vertexlist[z][2]).getValue());
            PUBLIC(this)->shapeVertex(&vertex);

          }
          PUBLIC(this)->endShape();
          vertexlist.truncate(0);

        }
      }

      float advancex, advancey;
      cc_glyph3d_getadvance(glyph, &advancex, &advancey);
      xpos += advancex * fontspec->size;

    }
    if (prevglyph) {
      cc_glyph3d_unref(prevglyph);
      prevglyph = NULL;
    }
    ypos -= fontspec->size * PUBLIC(this)->spacing.getValue();
  }


}

// Documented in superclass.
void
SoText3::notify(SoNotList * list)
{
  PRIVATE(this)->lock();
  if (PRIVATE(this)->cache) {
    SoField * f = list->getLastField();
    if (f == &this->string) PRIVATE(this)->cache->invalidate();
  }
  PRIVATE(this)->unlock();
  inherited::notify(list);
}

// recalculate glyphs
void
SoText3P::setUpGlyphs(SoState * state, SoText3 * textnode)
{
  // not all actions have SoCacheElement enabled
  if (!state->isElementEnabled(SoCacheElement::getClassStackIndex())) return;
  if (this->cache && this->cache->isValid(state)) return;
  SoGlyphCache * oldcache = this->cache;

  state->push();
  SbBool storedinvalid = SoCacheElement::setInvalid(FALSE);
  this->cache = new SoGlyphCache(state);
  this->cache->ref();
  SoCacheElement::set(state, this->cache);
  this->cache->readFontspec(state);
  const cc_font_specification * fontspec = this->cache->getCachedFontspec();

  this->widths.truncate(0);

  for (int i = 0; i < textnode->string.getNum(); i++) {

    float stringwidth = 0.0f;
    float kerningx = 0;
    float kerningy = 0;
    float advancex = 0;
    float advancey = 0;
    cc_glyph3d * prevglyph = NULL;

    const float * maxbbox;
    this->maxglyphbbox.makeEmpty();

    SbString str = textnode->string[i];
    const char * p = str.getString();
    size_t length = cc_string_utf8_validate_length(p);
    // No assertion as zero length is handled correctly (results in a new line)

    for (unsigned int strcharidx = 0; strcharidx < length; strcharidx++) {
      uint32_t glyphidx = 0;

      glyphidx = cc_string_utf8_get_char(p);
      p = cc_string_utf8_next_char(p);

      cc_glyph3d * glyph = cc_glyph3d_ref(glyphidx, fontspec);
      this->cache->addGlyph(glyph);
      assert(glyph);

      maxbbox = cc_glyph3d_getboundingbox(glyph); // Get max height

      this->maxglyphbbox.extendBy(SbVec3f(0, maxbbox[1] * fontspec->size, 0));
      this->maxglyphbbox.extendBy(SbVec3f(0, maxbbox[3] * fontspec->size, 0)); 

      if (strcharidx > 0)
        cc_glyph3d_getkerning(prevglyph, glyph, &kerningx, &kerningy);
      cc_glyph3d_getadvance(glyph, &advancex, &advancey);

      stringwidth += (advancex + kerningx) * fontspec->size;
      prevglyph = glyph;
    }

    if (prevglyph != NULL) {
      // Italic font might cause last letter to be outside bbox. Add width if needed.
      if (advancex < cc_glyph3d_getwidth(prevglyph))
        stringwidth += (cc_glyph3d_getwidth(prevglyph) - advancex) * fontspec->size;
    }

    this->widths.append(stringwidth);
  }

  state->pop();
  SoCacheElement::setInvalid(storedinvalid);

  // unref old cache after creating the new one to avoid recreating glyphs
  if (oldcache) oldcache->unref();

}

#undef PRIVATE
#undef PUBLIC
