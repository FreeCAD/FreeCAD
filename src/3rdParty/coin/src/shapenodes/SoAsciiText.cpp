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
  \class SoAsciiText SoAsciiText.h Inventor/nodes/SoAsciiText.h
  \brief The SoAsciiText class renders flat 3D text.

  \ingroup coin_nodes

  The text is rendered using 3D polygon geometry.

  The size of the textual geometry representation is decided from the
  SoFont::size field of a preceding SoFont-node in the scene graph,
  which specifies the size in unit coordinates. This value sets the
  approximate vertical size of the letters.  The default value if no
  SoFont-nodes are used, is 10.

  The complexity of the glyphs is controlled by a preceding
  SoComplexity node with \e Type set to OBJECT_SPACE. Please note
  that the default built-in 3D font will not be affected by the
  SoComplexity node.

  This node is different from the SoText2 node in that it rotates,
  scales, translates etc just like other geometry in the scene. It is
  different from the SoText3 node in that it renders the text "flat",
  i.e. does not extrude the fonts to have depth.

  To get an intuitive feeling for how SoAsciiText works, take a look
  at this sample Inventor file in examiner viewer:

  \verbatim
  #Inventor V2.1 ascii

  Separator {
    Font {
      size 10
      name "Arial:Bold Italic"
    }

    BaseColor {
      rgb 1 0 0 #red
    }
    AsciiText {
      width [ 0, 1, 50 ]
      justification LEFT #Standard alignment
      string [ "LEFT", "LEFT", "LEFT", "LEFT", "LEFT LEFT" ]
    }
    BaseColor { 
      rgb 1 1 0
    }
    Sphere { radius 1.5 }

    Translation {
      translation 0 -50 0
    }
    BaseColor {
      rgb 0 1 0 #green
    }
    AsciiText {
      width [ 0, 1, 50 ]
      justification RIGHT
      string [ "RIGHT", "RIGHT", "RIGHT", "RIGHT", "RIGHT RIGHT" ]
    }
    BaseColor { 
      rgb 0 1 1
    }
    Sphere { radius 1.5 }

    Translation {
      translation 0 -50 0
    }
    BaseColor {
      rgb 0 0 1 #blue
    }
    AsciiText {
      width [ 0, 1, 50 ]
      justification CENTER
      string [ "CENTER", "CENTER", "CENTER", "CENTER", "CENTER CENTER" ]
    }
    BaseColor { 
      rgb 1 0 1
    }
    Sphere { radius 1.5 }
  }
  \endverbatim

  In examinerviewer the Inventor file looks something like this:

  <center>
  \image html asciitext.png "Rendering of Example Scenegraph"
  </center>

  <b>FILE FORMAT/DEFAULTS:</b>
  \code
    AsciiText {
        string ""
        spacing 1
        justification LEFT
        width 0
    }
  \endcode

  \sa SoFont, SoFontStyle, SoText2, SoText3
  \since SGI Inventor 2.1
*/

// FIXME: Write doc about how text is textured. jornskaa 20040716

// *************************************************************************

#include <Inventor/nodes/SoAsciiText.h>
#include "coindefs.h"

#include <cstring>
#include <cfloat> // FLT_MIN

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
#include <Inventor/elements/SoGLMultiTextureEnabledElement.h>
#include <Inventor/elements/SoGLCacheContextElement.h>
#include <Inventor/elements/SoComplexityTypeElement.h>
#include <Inventor/elements/SoComplexityElement.h>
#include <Inventor/elements/SoCacheElement.h>
#include <Inventor/elements/SoTextOutlineEnabledElement.h>
#include <Inventor/errors/SoDebugError.h>
#include <Inventor/misc/SoState.h>
#include <Inventor/system/gl.h>
#include <Inventor/threads/SbMutex.h>

#include "caches/SoGlyphCache.h"
#include "fonts/glyph3d.h"
#include "nodes/SoSubNodeP.h"

// *************************************************************************

/*!
  \enum SoAsciiText::Justification
  Used to specify horizontal string alignment.
*/
/*!
  \var SoAsciiText::Justification SoAsciiText::LEFT
  Left edges of strings are aligned.
*/
/*!
  \var SoAsciiText::Justification SoAsciiText::RIGHT
  Right edges of strings are aligned.
*/
/*!
  \var SoAsciiText::Justification SoAsciiText::CENTER
  Centers of strings are aligned.
*/

/*!  \var SoMFString SoAsciiText::string 

  The set of strings to render.  Each string in the multiple value
  field will be rendered on a separate line.

  The default value of the field is a single empty string.
*/
/*!
  \var SoSFFloat SoAsciiText::spacing

  Vertical spacing between the baselines of two consecutive horizontal lines.
  Default value is 1.0, which means that it is equal to the vertical size of
  the highest character in the bitmap alphabet.
*/
/*!
  \var SoSFEnum SoAsciiText::justification

  Determines horizontal alignment of text strings.

  If justification is set to SoAsciiText::LEFT, the left edge of the first string
  is at the origin and all strings are aligned with their left edges.
  If set to SoAsciiText::RIGHT, the right edge of the first string is
  at the origin and all strings are aligned with their right edges. Otherwise,
  if set to SoAsciiText::CENTER, the center of the first string is at the
  origin and all strings are aligned with their centers.
  The origin is always located at the baseline of the first line of text.

  Default value is SoAsciiText::LEFT.
*/

/*!  \var SoMFFloat SoAsciiText::width
  Defines the width of each line.  The text is scaled to be within the
  specified units.  The size of the characters will remain the same;
  only the X-positions are scaled.  When width <= 0, the width
  value is ignored and the text rendered as normal.  The exact width of
  the rendered text depends not only on the width field, but also on
  the maximum character width in the rendered string.  The string will
  be attempted to fit within the specified width, but if it is unable
  to do so, it uses the largest character in the string as the
  width.  If fewer widths are specified than the number of strings, the
  strings without matching widths are rendered with default width.
*/

// *************************************************************************

class SoAsciiTextP {
public:

  SoAsciiTextP(SoAsciiText * master) : master(master) { }
  SoAsciiText * master;

  void setUpGlyphs(SoState * state, SoAsciiText * textnode);
  void calculateStringStretch(const int i, const cc_font_specification * fontspec, 
                              float & stretchfactor, float & stretchlength);
  
  SbList <float> glyphwidths;
  SbList <float> stringwidths;
  SbBox3f maxglyphbbox;

  SoGlyphCache * cache;

#ifdef COIN_THREADSAFE
  void lock(void) { this->mutex.lock(); }
  void unlock(void) { this->mutex.unlock(); }
#else  // ! COIN_THREADSAFE
  void lock(void) { }
  void unlock(void) { }
#endif // ! COIN_THREADSAFE

private:
#ifdef COIN_THREADSAFE
  // FIXME: a mutex for every instance seems a bit excessive,
  // especially since Microsoft Windows might have rather strict limits on the
  // total amount of mutex resources a process (or even a user) can
  // allocate. so consider making this a class-wide instance instead.
  // -mortene.
  SbMutex mutex;
#endif // COIN_THREADSAFE
};

#define PRIVATE(p) ((p)->pimpl)

// *************************************************************************

SO_NODE_SOURCE(SoAsciiText);

// *************************************************************************

/*!
  Constructor.
*/
SoAsciiText::SoAsciiText(void)
{
  SO_NODE_INTERNAL_CONSTRUCTOR(SoAsciiText);

  SO_NODE_ADD_FIELD(string, (""));
  SO_NODE_ADD_FIELD(spacing, (1.0f));
  SO_NODE_ADD_FIELD(justification, (SoAsciiText::LEFT));
  SO_NODE_ADD_FIELD(width, (0.0f));

  SO_NODE_DEFINE_ENUM_VALUE(Justification, LEFT);
  SO_NODE_DEFINE_ENUM_VALUE(Justification, RIGHT);
  SO_NODE_DEFINE_ENUM_VALUE(Justification, CENTER);
  SO_NODE_SET_SF_ENUM_TYPE(justification, Justification);

  PRIVATE(this) = new SoAsciiTextP(this);
  PRIVATE(this)->cache = NULL;
}

/*!
  Destructor.
*/
SoAsciiText::~SoAsciiText()
{
  if (PRIVATE(this)->cache) PRIVATE(this)->cache->unref();
  delete PRIVATE(this);
}

/*!
  \copybrief SoBase::initClass(void)
*/
void
SoAsciiText::initClass(void)
{
  SO_NODE_INTERNAL_INIT_CLASS(SoAsciiText, SO_FROM_INVENTOR_2_1|SoNode::VRML1);
}

// Doc in parent.
void
SoAsciiText::GLRender(SoGLRenderAction * action)
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
      SoDebugError::postWarning("SoAsciiText::GLRender",
                                "Support for rendering SoAsciiText nodes in outline "
                                "(i.e. heeding the SoTextOutlineEnabledElement) "
                                "not yet implemented.");
      warned = TRUE;
    }
  }


  PRIVATE(this)->setUpGlyphs(state, this);
  SoCacheElement::addCacheDependency(state, PRIVATE(this)->cache);

  const cc_font_specification * fontspec = PRIVATE(this)->cache->getCachedFontspec(); 

  SbBool do2Dtextures = FALSE;
  SbBool do3Dtextures = FALSE;
  if (SoGLMultiTextureEnabledElement::get(state, 0)) {
    do2Dtextures = TRUE;
    if (SoGLMultiTextureEnabledElement::getMode(state, 0) == SoGLMultiTextureEnabledElement::TEXTURE3D) {
      do3Dtextures = TRUE;
    }
  }

  // FIXME: implement proper support for 3D-texturing, and get rid of
  // this. 20020120 mortene.
  if (do3Dtextures) {
    static SbBool first = TRUE;
    if (first) {
      first = FALSE;
      SoDebugError::postWarning("SoAsciiText::GLRender",
                                "3D-textures not properly supported for this node type yet.");
    }
  }

  SoMaterialBundle mb(action);
  mb.sendFirst();
  
  glBegin(GL_TRIANGLES);
  glNormal3f(0.0f, 0.0f, 1.0f);

  float ypos = 0.0f;
  int i, n = this->string.getNum();
  for (i = 0; i < n; i++) {
    float stretchfactor, stretchlength;
    PRIVATE(this)->calculateStringStretch(i, fontspec, stretchfactor, stretchlength);

    float xpos = 0.0f;
    const float currwidth = stretchlength;
    switch (this->justification.getValue()) {
    case SoAsciiText::RIGHT:
      xpos = -currwidth;
      break;
    case SoAsciiText::CENTER:
      xpos = -currwidth * 0.5f;
      break;
    default:
      break;
    }

    SbString str = this->string[i];
    cc_glyph3d * prevglyph = NULL;
    const char * p = str.getString();
    size_t length = cc_string_utf8_validate_length(p);
    // No assertion as zero length is handled correctly (results in a new line)

    for (unsigned int strcharidx = 0; strcharidx < length; strcharidx++) {
      uint32_t glyphidx = 0;

      glyphidx = cc_string_utf8_get_char(p);
      p = cc_string_utf8_next_char(p);

      cc_glyph3d * glyph = cc_glyph3d_ref(glyphidx, fontspec);

      // Get kerning
      if (strcharidx > 0) {
        float kerningx, kerningy;
        cc_glyph3d_getkerning(prevglyph, glyph, &kerningx, &kerningy);
        xpos += kerningx * stretchfactor * fontspec->size;
      }

      if (prevglyph) {
        cc_glyph3d_unref(prevglyph);
      }
      prevglyph = glyph;

      const SbVec2f * coords = (SbVec2f *) cc_glyph3d_getcoords(glyph);
      const int * ptr = cc_glyph3d_getfaceindices(glyph);

      while (*ptr >= 0) {
        SbVec2f v0, v1, v2;
        v2 = coords[*ptr++];
        v1 = coords[*ptr++];
        v0 = coords[*ptr++];

        // FIXME: Is the text textured correctly when stretching is
        // applied (when width values have been given that are
        // not the same as the length of the string)? jornskaa 20040716
        if (do2Dtextures) {
          glTexCoord2f(v0[0] + xpos/fontspec->size, v0[1] + ypos/fontspec->size);
        }
        glVertex3f(v0[0] * fontspec->size + xpos, v0[1] * fontspec->size + ypos, 0.0f);
        
        if (do2Dtextures) {
          glTexCoord2f(v1[0] + xpos/fontspec->size, v1[1] + ypos/fontspec->size);
        }
        glVertex3f(v1[0] * fontspec->size + xpos, v1[1] * fontspec->size + ypos, 0.0f);
        
        if (do2Dtextures) {
          glTexCoord2f(v2[0] + xpos/fontspec->size, v2[1] + ypos/fontspec->size);
        }
        glVertex3f(v2[0] * fontspec->size + xpos, v2[1] * fontspec->size + ypos, 0.0f);

      }

      float advancex, advancey;
      cc_glyph3d_getadvance(glyph, &advancex, &advancey);
      xpos += (advancex * stretchfactor * fontspec->size);
    }
    if (prevglyph) {
      cc_glyph3d_unref(prevglyph);
      prevglyph = NULL;
    }

    ypos -= fontspec->size * this->spacing.getValue();
  }
  glEnd();

  PRIVATE(this)->unlock();

  if (SoComplexityTypeElement::get(state) == SoComplexityTypeElement::OBJECT_SPACE) {
    SoGLCacheContextElement::shouldAutoCache(state, SoGLCacheContextElement::DO_AUTO_CACHE);
    SoGLCacheContextElement::incNumShapes(state);
  }
}

// Doc in parent.
void
SoAsciiText::getPrimitiveCount(SoGetPrimitiveCountAction * action)
{
  // don't create an new cache in getPrimitiveCount(). SoCacheElement is not enabled
  if (action->is3DTextCountedAsTriangles() && PRIVATE(this)->cache) {        
    PRIVATE(this)->lock();
    //SoState * state = action->getState();
    const cc_font_specification * fontspec = PRIVATE(this)->cache->getCachedFontspec();
    
    const int lines = this->string.getNum();
    int numtris = 0;      
    for (int i = 0;i < lines; ++i) {

      SbString str = this->string[i];
      const char * p = str.getString();
      size_t length = cc_string_utf8_validate_length(p);
      // No assertion as zero length is handled correctly (results in a new line)

      for (unsigned int strcharidx = 0; strcharidx < length; strcharidx++) {
	uint32_t glyphidx = 0;

	glyphidx = cc_string_utf8_get_char(p);
	p = cc_string_utf8_next_char(p);

        cc_glyph3d * glyph = cc_glyph3d_ref(glyphidx, fontspec);

        int cnt = 0;
        const int * ptr = cc_glyph3d_getfaceindices(glyph);
        while (*ptr++ >= 0) 
          cnt++;

        numtris += cnt / 3;
        
        cc_glyph3d_unref(glyph);
      }
    }
    action->addNumTriangles(numtris);
    PRIVATE(this)->unlock();
  }
  else {
    action->addNumText(this->string.getNum());
  }
}

// This method calculates the stretchfactor needed to make the string
// occupy the specified amount of units. If no units are specified, an
// identity stretchfactor of 1.0 is returned. The length of the
// stretched string is also returned. It approximates to find one of the
// most probable last characters of the rendered string, so small errors
// might occur.
void SoAsciiTextP::calculateStringStretch(const int i, const cc_font_specification * fontspec, 
                                          float & stretchfactor, float & stretchedlength) 
{
  assert(i < master->string.getNum());

  // Some sanitychecking before starting the calculations
  if (i >= master->width.getNum() || master->width[i] <= 0 || 
      master->string[i].getLength() == 0) {

    stretchfactor = 1.0f;
    stretchedlength = this->stringwidths[i];
    return;
  }

  // Approximate the stretchfactor
  stretchfactor = master->width[i] / this->stringwidths[i];

  cc_glyph3d * prevglyph = NULL;
  float originalmaxx = 0.0f;
  float originalmaxxpos = 0.0f;
  float originalxpos = 0.0f;
  float maxglyphwidth = 0.0f;
  float maxx = 0.0f;
  unsigned int strcharidx;

  // Find last character in the stretched text
  SbString str = master->string[i];
  const char * p = str.getString();
  size_t length = cc_string_utf8_validate_length(p);
  // No assertion as zero length is handled correctly (results in a new line)

  for (strcharidx = 0; strcharidx < length; strcharidx++) {
    uint32_t glyphidx = 0;

    glyphidx = cc_string_utf8_get_char(p);
    p = cc_string_utf8_next_char(p);

    cc_glyph3d * glyph = cc_glyph3d_ref(glyphidx, fontspec);
    float glyphwidth = cc_glyph3d_getwidth(glyph) * fontspec->size;

    // Adjust the distance between neighbouring characters
    if (prevglyph) {
      float kerningx, kerningy;
      cc_glyph3d_getkerning(prevglyph, glyph, &kerningx, &kerningy);
      originalxpos += kerningx * fontspec->size;
    }

    // Find the maximum endposition in the x-direction
    float endx = originalxpos * stretchfactor + glyphwidth;
    if (endx > maxx) {
      originalmaxxpos = originalxpos;
      originalmaxx = originalxpos + glyphwidth;

      maxx = endx;
      maxglyphwidth = glyphwidth;
    }

    // Advance to the next character in the x-direction
    float advancex, advancey;
    cc_glyph3d_getadvance(glyph, &advancex, &advancey);
    originalxpos += advancex * fontspec->size;

    // Remove the previous glyph from memory
    if (prevglyph) {
      cc_glyph3d_unref(prevglyph);
    }

    // Make ready for next run
    prevglyph = glyph;
  }

  // Unreference the last glyph
  cc_glyph3d_unref(prevglyph);
  prevglyph = NULL;
  
  // Calculate the accurate stretchfactor and the width of the
  // string. This should be close to the specified width unless the
  // specified width is less than the longest character in the string.
  const float oldendxpos = this->stringwidths[i] - maxglyphwidth;
  const float newendxpos = master->width[i] - maxglyphwidth;
  if (oldendxpos <= 0 || newendxpos <= 0) {
    // oldendxpos should be > 0 anyways, but just in case.  if
    // newendxpos < 0, then the specified width is less than that
    // possible with the current font.
    stretchfactor = 0.0f;
    stretchedlength = maxglyphwidth;
  }
  else {
    // The width of the stretched string is longer than the longest
    // character in the string.
    stretchfactor = newendxpos / oldendxpos;
    stretchedlength = stretchfactor * originalmaxxpos + maxglyphwidth;
  }
}

// Doc in parent.
void
SoAsciiText::computeBBox(SoAction * action, SbBox3f & box, SbVec3f & center)
{
  PRIVATE(this)->lock();
  SoState * state = action->getState();

  PRIVATE(this)->setUpGlyphs(state, this);
  SoCacheElement::addCacheDependency(state, PRIVATE(this)->cache);

  const cc_font_specification * fontspec = PRIVATE(this)->cache->getCachedFontspec();

  int i;
  float maxw = FLT_MIN;
  for (i = 0; i < this->string.getNum(); i++) {
    float stretchfactor, stretchlength;
    PRIVATE(this)->calculateStringStretch(i, fontspec, stretchfactor, stretchlength);
    maxw = SbMax(maxw, stretchlength);
  }

  if (maxw == FLT_MIN) { // There is no text to bound. Returning.
    PRIVATE(this)->unlock();
    return; 
  }

  float maxy, miny;
  float minx, maxx;
  
  minx = 0;
  maxx = maxw;

  miny = -fontspec->size * this->spacing.getValue() * (this->string.getNum() - 1);
  maxy = fontspec->size;

  switch (this->justification.getValue()) {
  case SoAsciiText::LEFT:
    break;
  case SoAsciiText::RIGHT:
    maxx -= maxw;
    minx -= maxw;
    break;
  case SoAsciiText::CENTER:
    maxx -= maxw * 0.5f;
    minx -= maxw * 0.5f;
    break;
  default:
    assert(0 && "Unknown justification");
    minx = maxx = 0.0f;
    break;
  }

  box.setBounds(SbVec3f(minx, miny, 0.0f), SbVec3f(maxx, maxy, 0.0f));

  // maxglyphbbox should never be empty here.
  assert(!PRIVATE(this)->maxglyphbbox.isEmpty());

  // Expanding bbox so that glyphs like 'j's and 'q's are completely inside.
  box.extendBy(SbVec3f(0, PRIVATE(this)->maxglyphbbox.getMin()[1] - (this->string.getNum() - 1) * fontspec->size, 0));  
  box.extendBy(PRIVATE(this)->maxglyphbbox);
  center = box.getCenter();

  PRIVATE(this)->unlock();
}

// Doc in parent.
void
SoAsciiText::generatePrimitives(SoAction * action)
{
  PRIVATE(this)->lock();
  SoState * state = action->getState();
  PRIVATE(this)->setUpGlyphs(state, this);

  const cc_font_specification * fontspec = PRIVATE(this)->cache->getCachedFontspec();

  SbBool do2Dtextures = FALSE;
  SbBool do3Dtextures = FALSE;
  if (SoMultiTextureEnabledElement::get(state, 0)) {
    do2Dtextures = TRUE;
    if (SoMultiTextureEnabledElement::getMode(state,0) ==
        SoMultiTextureEnabledElement::TEXTURE3D) {
      do3Dtextures = TRUE;
    }
  }
  // FIXME: implement proper support for 3D-texturing, and get rid of
  // this. 20020120 mortene.
  if (do3Dtextures) {
    static SbBool first = TRUE;
    if (first) {
      first = FALSE;
      SoDebugError::postWarning("SoAsciiText::generatePrimitives",
                                "3D-textures not properly supported for this node type yet.");
    }
  }

  SoPrimitiveVertex vertex;
  SoTextDetail detail;
  detail.setPart(0);
  vertex.setDetail(&detail);
  vertex.setMaterialIndex(0);

  this->beginShape(action, SoShape::TRIANGLES, NULL);
  vertex.setNormal(SbVec3f(0.0f, 0.0f, 1.0f));

  float ypos = 0.0f;
  int i, n = this->string.getNum();
  for (i = 0; i < n; i++) {
    float stretchfactor, stretchlength;
    PRIVATE(this)->calculateStringStretch(i, fontspec, stretchfactor, stretchlength);

    detail.setStringIndex(i);
    float xpos = 0.0f;
    const float currwidth = stretchlength;
    switch (this->justification.getValue()) {
    case SoAsciiText::RIGHT:
      xpos = -currwidth;
      break;
    case SoAsciiText::CENTER:
      xpos = - currwidth * 0.5f;
      break;
    }
    
    SbString str = this->string[i];
    cc_glyph3d * prevglyph = NULL;
    const char * p = str.getString();
    size_t length = cc_string_utf8_validate_length(p);
    // No assertion as zero length is handled correctly (results in a new line)
      
    for (unsigned int strcharidx = 0; strcharidx < length; strcharidx++) {      
      uint32_t glyphidx = 0;

      glyphidx = cc_string_utf8_get_char(p);
      p = cc_string_utf8_next_char(p);

      cc_glyph3d * glyph = cc_glyph3d_ref(glyphidx, fontspec);
      
      // Get kerning
      if (strcharidx > 0) {
        float kerningx, kerningy;
        cc_glyph3d_getkerning(prevglyph, glyph, &kerningx, &kerningy);
        xpos += kerningx * stretchfactor * fontspec->size;
      }
      if (prevglyph) {
        cc_glyph3d_unref(prevglyph);
      }

      prevglyph = glyph;
      detail.setCharacterIndex(strcharidx);

      const SbVec2f * coords = (SbVec2f *) cc_glyph3d_getcoords(glyph);
      const int * ptr = cc_glyph3d_getfaceindices(glyph);

      while (*ptr >= 0) {
        SbVec2f v0, v1, v2;
        v2 = coords[*ptr++];
        v1 = coords[*ptr++];
        v0 = coords[*ptr++];

        // FIXME: Is the text textured correctly when stretching is
        // applied (when width values have been given that are
        // not the same as the length of the string)? jornskaa 20040716
        if(do2Dtextures) {
          vertex.setTextureCoords(SbVec2f(v0[0] + xpos/fontspec->size, v0[1] + ypos/fontspec->size));
        }
        vertex.setPoint(SbVec3f(v0[0] * fontspec->size + xpos, v0[1] * fontspec->size + ypos, 0.0f));
        this->shapeVertex(&vertex);

        if(do2Dtextures) {
          vertex.setTextureCoords(SbVec2f(v1[0] + xpos/fontspec->size, v1[1] + ypos/fontspec->size));
        }
        vertex.setPoint(SbVec3f(v1[0] * fontspec->size + xpos, v1[1] * fontspec->size + ypos, 0.0f));
        this->shapeVertex(&vertex);

        if(do2Dtextures) {
          vertex.setTextureCoords(SbVec2f(v2[0] + xpos/fontspec->size, v2[1] + ypos/fontspec->size));
        }
        vertex.setPoint(SbVec3f(v2[0] * fontspec->size + xpos, v2[1] * fontspec->size + ypos, 0.0f));
        this->shapeVertex(&vertex);
      }
      float advancex, advancey;
      cc_glyph3d_getadvance(glyph, &advancex, &advancey);
      xpos += (advancex * stretchfactor * fontspec->size);
    }
    ypos -= fontspec->size * this->spacing.getValue();
    if (prevglyph) {
      cc_glyph3d_unref(prevglyph);
      prevglyph = NULL;
    }
  }
  this->endShape();
  PRIVATE(this)->unlock();
}

// doc in parent
SoDetail *
SoAsciiText::createTriangleDetail(SoRayPickAction * COIN_UNUSED_ARG(action),
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
SoAsciiText::notify(SoNotList * list)
{
  PRIVATE(this)->lock();
  if (PRIVATE(this)->cache) {
    SoField * f = list->getLastField();
    if (f == &this->string) {
      PRIVATE(this)->cache->invalidate();
    }
  }
  PRIVATE(this)->unlock();
  inherited::notify(list);
}

// returns "normalized" width of specified string. When too few
// width values are supplied, the glyphwidths are used instead.
float
SoAsciiText::getWidth(const int idx, const float fontsize)
{
  if (idx < this->width.getNum() && this->width[idx] > 0.0f)
    return this->width[idx] / fontsize;
  return PRIVATE(this)->glyphwidths[idx];
}

// *************************************************************************
// SoAsciiTextP methods implemented below

// recalculate glyphs
void
SoAsciiTextP::setUpGlyphs(SoState * state, SoAsciiText * textnode)
{
  if (this->cache && this->cache->isValid(state)) return;
  SoGlyphCache * oldcache = this->cache;
  
  state->push();
  SbBool storedinvalid = SoCacheElement::setInvalid(FALSE);
  this->cache = new SoGlyphCache(state); 
  this->cache->ref();
  SoCacheElement::set(state, this->cache);
  this->cache->readFontspec(state);
  const cc_font_specification * fontspecptr = this->cache->getCachedFontspec(); 

  this->glyphwidths.truncate(0);
  this->stringwidths.truncate(0);
  this->maxglyphbbox.makeEmpty();

  float kerningx = 0;
  float kerningy = 0;
  float advancex = 0;
  float advancey = 0;
  cc_glyph3d * prevglyph = NULL;

  for (int i = 0; i < textnode->string.getNum(); i++) {
    float stringwidth = 0.0f;
    SbString str = textnode->string[i];
    const float * maxbbox;
    const char * p = str.getString();
    size_t length = cc_string_utf8_validate_length(p);
    // No assertion as zero length is handled correctly (results in a new line)

    for (unsigned int strcharidx = 0; strcharidx < length; strcharidx++) {
      uint32_t glyphidx = 0;

      glyphidx = cc_string_utf8_get_char(p);
      p = cc_string_utf8_next_char(p);

      cc_glyph3d * glyph = cc_glyph3d_ref(glyphidx, fontspecptr);
      this->cache->addGlyph(glyph);
      assert(glyph);

      maxbbox = cc_glyph3d_getboundingbox(glyph); // Get max height
      this->maxglyphbbox.extendBy(SbVec3f(0, maxbbox[0] * fontspecptr->size, 0));
      this->maxglyphbbox.extendBy(SbVec3f(0, maxbbox[1] * fontspecptr->size, 0));

      // FIXME: Shouldn't it be the 'advance' value be stored in this
      // list?  This data is only accessed via the public 'getWidth()'
      // method. (20031002 handegar)
      this->glyphwidths.append(cc_glyph3d_getwidth(glyph));
   
      if (strcharidx > 0) 
        cc_glyph3d_getkerning(prevglyph, glyph, &kerningx, &kerningy);          
      cc_glyph3d_getadvance(glyph, &advancex, &advancey);

      stringwidth += (advancex + kerningx) * fontspecptr->size;

      prevglyph = glyph;
    }

    if (prevglyph) {
      // Have to remove the appended advance and add the last character to the calculated with
      stringwidth += (cc_glyph3d_getwidth(prevglyph) - advancex) * fontspecptr->size;
      prevglyph = NULL; // To make sure the next line starts with blank sheets
    }

    this->stringwidths.append(stringwidth);
  }

  state->pop();
  SoCacheElement::setInvalid(storedinvalid);

  // unref old cache after creating the new one to avoid recreating glyphs
  if (oldcache) oldcache->unref();
}

// *************************************************************************

#undef PRIVATE
