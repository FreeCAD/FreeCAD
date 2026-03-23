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

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif // HAVE_CONFIG_H

#ifdef HAVE_VRML97

/*!
  \class SoVRMLText SoVRMLText.h Inventor/VRMLnodes/SoVRMLText.h
  \brief The SoVRMLText class is used to represent text in a scene.

  \ingroup coin_VRMLnodes

  \WEB3DCOPYRIGHT

  Important note: currently, the implementation of this node is not
  complete, and some of the features mentioned in the documentation
  below may not be working yet.

  \WEB3DCOPYRIGHT
  
  \verbatim
  Text { 
    exposedField  MFString string    []
    exposedField  SFNode   fontStyle NULL
    exposedField  MFFloat  length    []      # [0,)
    exposedField  SFFloat  maxExtent 0.0     # [0,)
  }
  \endverbatim

  The Text node specifies a two-sided, flat text string object
  positioned in the Z=0 plane of the local coordinate system based on
  values defined in the fontStyle field (see SoVRMLFontStyle).
  Text nodes may contain multiple text strings specified using the
  UTF-8 encoding as specified by ISO 10646-1:1993 (see
  <http://www.web3d.org/documents/specifications/14772/V2.0/part1/references.html#[UTF8]>).
  The text strings are stored in the order in which the text mode
  characters are to be produced as defined by the parameters in the
  FontStyle node.

  The text strings are contained in the string field. The fontStyle
  field contains one FontStyle node that specifies the font size, font
  family and style, direction of the text strings, and any specific
  language rendering techniques used for the text.

  The maxExtent field limits and compresses all of the text strings if
  the length of the maximum string is longer than the maximum extent,
  as measured in the local coordinate system. If the text string with
  the maximum length is shorter than the maxExtent, then there is no
  compressing. The maximum extent is measured horizontally for
  horizontal text (FontStyle node: horizontal=TRUE) and vertically for
  vertical text (FontStyle node: horizontal=FALSE). The maxExtent
  field shall be greater than or equal to zero.

  The length field contains an MFFloat value that specifies the length
  of each text string in the local coordinate system. If the string is
  too short, it is stretched (either by scaling the text or by adding
  space between the characters). If the string is too long, it is
  compressed (either by scaling the text or by subtracting space
  between the characters). If a length value is missing (for example,
  if there are four strings but only three length values), the missing
  values are considered to be 0. The length field shall be greater
  than or equal to zero.

  Specifying a value of 0 for both the maxExtent and length fields
  indicates that the string may be any length.

  \sa SoVRMLFontStyle
*/

/*!
  \var SoMFString SoVRMLText::string
  The strings. Empty by default.
*/

/*!
  \var SoMFFloat SoVRMLText::length
  Length of each string in the local coordinate system.
*/

/*!
  \var SoSFNode SoVRMLText::fontStyle
  Can contain an SoVRMLFontStyle node.
*/

/*!
  \var SoSFFloat SoVRMLText::maxExtent
  Maximum object space extent of longest string.
*/

#include <Inventor/VRMLnodes/SoVRMLText.h>
#include "coindefs.h"

#include <cfloat> // FLT_MIN
#include <cstddef>
#include <cstring>

#include <Inventor/SoPrimitiveVertex.h>
#include <Inventor/VRMLnodes/SoVRMLFontStyle.h>
#include <Inventor/VRMLnodes/SoVRMLMacros.h>
#include <Inventor/actions/SoGLRenderAction.h>
#include <Inventor/actions/SoGetPrimitiveCountAction.h>
#include <Inventor/bundles/SoMaterialBundle.h>
#include <Inventor/details/SoTextDetail.h>
#include <Inventor/elements/SoCacheElement.h>
#include <Inventor/elements/SoComplexityTypeElement.h>
#include <Inventor/elements/SoFontNameElement.h>
#include <Inventor/elements/SoFontSizeElement.h>
#include <Inventor/elements/SoGLCacheContextElement.h>
#include <Inventor/elements/SoGLMultiTextureEnabledElement.h>
#include <Inventor/errors/SoDebugError.h>
#include <Inventor/lists/SbList.h>
#include <Inventor/nodes/SoAsciiText.h>
#include <Inventor/sensors/SoFieldSensor.h>
#include <Inventor/system/gl.h>
#ifdef HAVE_THREADS
#include <Inventor/threads/SbMutex.h>
#endif // HAVE_THREADS

#include "nodes/SoSubNodeP.h"
#include "caches/SoGlyphCache.h"
#include "fonts/glyph3d.h"


// *************************************************************************

class SoVRMLTextP {
public:
  SoVRMLTextP(SoVRMLText * master) : master(master) { }
  SoVRMLText * master;

  float getWidth(const int idx, const float fontsize);
  SbList <float> glyphwidths;
  void setUpGlyphs(SoState * state, SoVRMLText * textnode);
  SoGlyphCache * cache;

  void updateFontStyle(void);

  SoFieldSensor * fontstylesensor;

  int justificationmajor;
  int justificationminor;
  SbBool lefttorighttext;
  SbBool toptobottomtext;
  SbBool horizontaltext;
  float textspacing;
  float textsize;
  float maxglyphheight;
  float maxglyphwidth;
  SbBox3f maxglyphbbox;
  int fontfamily;
  int fontstyle;

#ifdef COIN_THREADSAFE
  void lock(void) { this->mutex.lock(); }
  void unlock(void) { this->mutex.unlock(); }
private:
  SbMutex mutex;
#else // !COIN_THREADSAFE
  void lock(void) { }
  void unlock(void) { }
#endif // !COIN_THREADSAFE
};

#define PRIVATE(obj) (obj->pimpl)
#define PUBLIC(obj) (obj->master)

// *************************************************************************

SO_NODE_SOURCE(SoVRMLText);

// *************************************************************************

/*!
  \copydetails SoNode::initClass(void)
*/
void
SoVRMLText::initClass(void)
{
  SO_NODE_INTERNAL_INIT_CLASS(SoVRMLText, SO_VRML97_NODE_TYPE);
}

static void 
fontstylechangeCB(void * data, SoSensor * COIN_UNUSED_ARG(sensor))
{
  SoVRMLTextP * pimpl = (SoVRMLTextP *) data;
  pimpl->lock();
  if (pimpl->cache) pimpl->cache->invalidate();
  pimpl->unlock();
}

/*!
  Constructor.
*/
SoVRMLText::SoVRMLText(void)
{
  PRIVATE(this) = new SoVRMLTextP(this);

  SO_VRMLNODE_INTERNAL_CONSTRUCTOR(SoVRMLText);

  SO_VRMLNODE_ADD_EMPTY_EXPOSED_MFIELD(string);

  SO_VRMLNODE_ADD_EXPOSED_FIELD(fontStyle, (NULL));
  SO_VRMLNODE_ADD_EXPOSED_FIELD(maxExtent, (0.0f));
  SO_VRMLNODE_ADD_EMPTY_EXPOSED_MFIELD(length);

  // Default text setup
  PRIVATE(this)->textsize = 1.0f;
  PRIVATE(this)->textspacing = 1.0f;
  PRIVATE(this)->maxglyphheight = 1.0f;
  PRIVATE(this)->maxglyphwidth = 1.0f;
  PRIVATE(this)->lefttorighttext = TRUE;
  PRIVATE(this)->toptobottomtext = TRUE;
  PRIVATE(this)->horizontaltext = TRUE;
  PRIVATE(this)->justificationmajor = SoAsciiText::LEFT;
  PRIVATE(this)->justificationminor = SoAsciiText::LEFT;
  
  PRIVATE(this)->fontstylesensor = new SoFieldSensor(fontstylechangeCB, PRIVATE(this));
  PRIVATE(this)->fontstylesensor->attach(&fontStyle);
  PRIVATE(this)->fontstylesensor->setPriority(0);
  
  PRIVATE(this)->cache = NULL;
}

float
SoVRMLTextP::getWidth(const int idx, const float COIN_UNUSED_ARG(fontsize))
{
  float w = this->glyphwidths[idx];
  float maxe = PUBLIC(this)->maxExtent.getValue();
  if (maxe > 0.0f && w > maxe) w = maxe;
  return w;
}


/*!
  Destructor.
*/
SoVRMLText::~SoVRMLText()
{
  if (PRIVATE(this)->cache) PRIVATE(this)->cache->unref();

  delete PRIVATE(this)->fontstylesensor;
  delete PRIVATE(this);
}

// Doc in parent
void
SoVRMLText::GLRender(SoGLRenderAction * action)
{
  if (!this->shouldGLRender(action)) return;

  PRIVATE(this)->lock();
  SoState * state = action->getState();
  PRIVATE(this)->setUpGlyphs(state, this);
  SoCacheElement::addCacheDependency(state, PRIVATE(this)->cache);

  const cc_font_specification * fontspec = PRIVATE(this)->cache->getCachedFontspec();

  SoMaterialBundle mb(action);
  mb.sendFirst();

  SbBool do2Dtextures = FALSE;
  SbBool do3Dtextures = FALSE;
  if (SoGLMultiTextureEnabledElement::get(state)) {
    do2Dtextures = TRUE;
    if (SoGLMultiTextureEnabledElement::getMode(state) ==
        SoGLMultiTextureEnabledElement::TEXTURE3D) {
      do3Dtextures = TRUE;
    }
  }
  // FIXME: implement proper support for 3D-texturing, and get rid of
  // this. 20020120 mortene.
  if (do3Dtextures) {
    static SbBool first = TRUE;
    if (first) {
      first = FALSE;
      SoDebugError::postWarning("SoVRMLText::GLRender",
                                "3D-textures not properly supported for this node type yet.");
    }
  }

  int i;
  const int n = this->string.getNum();

  glBegin(GL_TRIANGLES);
  glNormal3f(0.0f, 0.0f, 1.0f);

  const float spacing = PRIVATE(this)->textspacing * PRIVATE(this)->textsize;
  int maxstringchars = 0;
  float ypos = 0.0f;

  for (i = 0; i < n; ++i) 
    maxstringchars = SbMax(maxstringchars, this->string[i].getLength());
  
  for (i = 0; i < n; i++) {

    float xpos = 0.0f;   
    float stretchlength = 0.0f;
    if (i < this->length.getNum()) 
      stretchlength = this->length[i];
    float stretchfactor = (stretchlength * PRIVATE(this)->textsize) / strlen(this->string[i].getString());
    
    float compressfactor = 1;
    if (this->maxExtent.getValue() > 0) {
      if (PRIVATE(this)->glyphwidths[i] > this->maxExtent.getValue()) {
        assert(PRIVATE(this)->glyphwidths[i] != 0 && "String length == 0! Cannot divide");
        compressfactor = (this->maxExtent.getValue() * PRIVATE(this)->textsize) / PRIVATE(this)->glyphwidths[i];
      }
    }
    
    
    if (PRIVATE(this)->horizontaltext) { // -- Horizontal text ----------------------
      
      switch (PRIVATE(this)->justificationmajor) {
      case SoAsciiText::LEFT:
        xpos = 0.0f;
        break;
      case SoAsciiText::CENTER:
        if (PRIVATE(this)->lefttorighttext)
          xpos = - PRIVATE(this)->glyphwidths[i] * PRIVATE(this)->textsize * 0.5f;
        else
          xpos = PRIVATE(this)->glyphwidths[i] * PRIVATE(this)->textsize * 0.5f;
        break;
      case SoAsciiText::RIGHT:
        if (PRIVATE(this)->lefttorighttext)
          xpos = -PRIVATE(this)->glyphwidths[i] * PRIVATE(this)->textsize;
        else
          xpos = PRIVATE(this)->glyphwidths[i] * PRIVATE(this)->textsize;
        break;
      default:
        break;
      }

      switch (PRIVATE(this)->justificationminor) {
        // FIXME: shouldn't the calculations below take the
        // FontStyle::topToBottom field into account? 20051207 mortene.

      case SoAsciiText::LEFT:
        break;
      case SoAsciiText::CENTER:
        {
          const float VERTICALSIZE = spacing * n;
          const float FIRSTLINEPOS = -(VERTICALSIZE / 2);
          ypos = FIRSTLINEPOS + (i * spacing);
        }
        break;
      case SoAsciiText::RIGHT:
        ypos = (i * spacing) + ((n-1) * spacing);
        break;        
      default:
        break;
      }
    
    }
    else { // -- Vertical text -----------------------------------------

      if (PRIVATE(this)->lefttorighttext)
        xpos = i * spacing;
      else
        xpos = -i * spacing;
        
      switch (PRIVATE(this)->justificationmajor) {
      case SoAsciiText::LEFT:        
        ypos = -PRIVATE(this)->maxglyphheight;
        break;
      case SoAsciiText::RIGHT:
        if (PRIVATE(this)->toptobottomtext)
          ypos = this->string[i].getLength() * spacing;
        else
          ypos = -this->string[i].getLength() * spacing;
        break;
      case SoAsciiText::CENTER:
        if (PRIVATE(this)->toptobottomtext)
          ypos = this->string[i].getLength() * PRIVATE(this)->textsize * 0.5f;
        else
          ypos = -this->string[i].getLength() * PRIVATE(this)->textsize * 0.5f;
        break;
      default:
        break;
      }

      switch (PRIVATE(this)->justificationminor) {
      case SoAsciiText::LEFT:
        break;
      case SoAsciiText::CENTER:
        xpos -= ((n-1) * spacing) * PRIVATE(this)->textsize * 0.5f;
        break;
      case SoAsciiText::RIGHT:
        xpos -= ((n-1) * spacing) * PRIVATE(this)->textsize;
        break;        
      default:
        break;
      }

    }
     
    cc_glyph3d * prevglyph = NULL;

    SbString str = this->string[i];
    const char * p = str.getString();
    size_t len = cc_string_utf8_validate_length(p);
    assert(len);

    for (unsigned int strcharidx = 0; strcharidx < len; strcharidx++) {
      uint32_t glyphidx = 0;

      glyphidx = cc_string_utf8_get_char(p);
      p = cc_string_utf8_next_char(p);

      cc_glyph3d * glyph = cc_glyph3d_ref(glyphidx, fontspec);

      float advancex, advancey;
      cc_glyph3d_getadvance(glyph, &advancex, &advancey);

      const SbVec2f * coords = (SbVec2f *) cc_glyph3d_getcoords(glyph);
      const int * ptr = cc_glyph3d_getfaceindices(glyph);

      if (PRIVATE(this)->horizontaltext) {
        if (!PRIVATE(this)->lefttorighttext) {// Right to left text.
          xpos -= (advancex + stretchfactor) * compressfactor * PRIVATE(this)->textsize;
        }
      }             

      if (strcharidx > 0) {
        float kerningx = 0.0f;
        float kerningy = 0.0f;
        cc_glyph3d_getkerning(prevglyph, glyph, &kerningx, &kerningy);
        xpos += kerningx * PRIVATE(this)->textsize;
      }
      if (prevglyph) {
        cc_glyph3d_unref(prevglyph);
      }
      prevglyph = glyph;

      while (*ptr >= 0) {
        SbVec2f v0, v1, v2;
        v2 = coords[*ptr++];
        v1 = coords[*ptr++];
        v0 = coords[*ptr++];
        
     
        if (do2Dtextures) {
          glTexCoord2f(v0[0] + xpos/PRIVATE(this)->textsize, v0[1] + ypos/PRIVATE(this)->textsize);
        }
        glVertex3f((v0[0]*PRIVATE(this)->textsize) + xpos, (v0[1]*PRIVATE(this)->textsize) + ypos, 0.0f);

        if (do2Dtextures) {
          glTexCoord2f(v1[0] + xpos/PRIVATE(this)->textsize, v1[1] + ypos/PRIVATE(this)->textsize);
        }
        glVertex3f(v1[0] * PRIVATE(this)->textsize + xpos, v1[1] * PRIVATE(this)->textsize + ypos, 0.0f);

        if (do2Dtextures) {
          glTexCoord2f(v2[0] + xpos/PRIVATE(this)->textsize, v2[1] + ypos/PRIVATE(this)->textsize);
        }
        glVertex3f(v2[0] * PRIVATE(this)->textsize + xpos, v2[1] * PRIVATE(this)->textsize + ypos, 0.0f);

      }
      
      if (!PRIVATE(this)->horizontaltext) {
        
        if (PRIVATE(this)->toptobottomtext)
          ypos -= PRIVATE(this)->textsize;
        else 
          ypos += PRIVATE(this)->textsize;
        
      } else if (PRIVATE(this)->lefttorighttext) {
        xpos += (advancex + stretchfactor) * compressfactor * PRIVATE(this)->textsize;
      }
    }

    if (PRIVATE(this)->horizontaltext) {
      if (PRIVATE(this)->toptobottomtext)
        ypos -= spacing * PRIVATE(this)->maxglyphheight;
      else
        ypos += spacing * PRIVATE(this)->maxglyphheight;
    }

    if (prevglyph) {
      cc_glyph3d_unref(prevglyph);
      prevglyph = NULL;
    }
  }
  glEnd();
  PRIVATE(this)->unlock();

  if (SoComplexityTypeElement::get(state) == SoComplexityTypeElement::OBJECT_SPACE) {
    SoGLCacheContextElement::shouldAutoCache(state, SoGLCacheContextElement::DO_AUTO_CACHE);
    SoGLCacheContextElement::incNumShapes(state);
  }
}


// Doc in parent
void
SoVRMLText::getPrimitiveCount(SoGetPrimitiveCountAction * action)
{
  if (action->is3DTextCountedAsTriangles()) {        
    PRIVATE(this)->lock();
    // can't regenerate the cache in this action traversal since SoCacheElement isn't enabled
    if (PRIVATE(this)->cache) {
      const cc_font_specification * fontspec = PRIVATE(this)->cache->getCachedFontspec();
      const int lines = this->string.getNum();
      int numtris = 0;      
      
      for (int i = 0;i < lines; ++i) {
	SbString str = this->string[i];
	const char * p = str.getString();
	size_t len = cc_string_utf8_validate_length(p);
	assert(len);

        for (unsigned int strcharidx = 0; strcharidx < len; strcharidx++) {
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
    }
    PRIVATE(this)->unlock();
  }
  else {
    action->addNumText(this->string.getNum());
  }

}


// Doc in parent
void
SoVRMLText::notify(SoNotList * list)
{
  PRIVATE(this)->lock();
  if (PRIVATE(this)->cache) {
    SoField * f = list->getLastField();
    if (f == &this->string) PRIVATE(this)->cache->invalidate();
  }
  PRIVATE(this)->unlock();
  inherited::notify(list);
}

// Doc in parent
SoChildList *
SoVRMLText::getChildren(void) const
{
  return NULL;
}


// Doc in parent
void
SoVRMLText::computeBBox(SoAction * action,
                        SbBox3f & box,
                        SbVec3f & center)
{
  PRIVATE(this)->lock();

  PRIVATE(this)->setUpGlyphs(action->getState(), this);
  SoCacheElement::addCacheDependency(action->getState(), PRIVATE(this)->cache);
  
  int i;
  const int n = this->string.getNum();
  const float linespacing = PRIVATE(this)->textspacing * PRIVATE(this)->maxglyphheight;

  float maxw = FLT_MIN;
  int maxstringchars = 0;
  for (i = 0; i < n; i++) {
    maxw = SbMax(maxw, PRIVATE(this)->glyphwidths[i]); 
    maxstringchars = SbMax(maxstringchars, this->string[i].getLength());
  }

  if (maxw == FLT_MIN) { // There is no text to bound. Returning.
    PRIVATE(this)->unlock();
    return; 
  }
  
  float maxlength = 0.0f;

  for (i = 0; i < this->length.getNum();++i) 
    maxlength = SbMax(maxlength, this->length[i]);
  float stretchfactor = (maxlength * PRIVATE(this)->textsize);

  float maxy, miny;
  float minx, maxx;
  
  if (PRIVATE(this)->horizontaltext) {  // -- Horizontal text -----------------
    
    if (PRIVATE(this)->toptobottomtext) {
      miny = -PRIVATE(this)->textsize * PRIVATE(this)->textspacing * (n-1);
      maxy = PRIVATE(this)->textsize;
    } 
    else {
      miny = 0.0f;
      maxy = PRIVATE(this)->textsize * PRIVATE(this)->textspacing * (n-1);
    }
    
    minx = 0.0f;  
    maxx = (maxw + stretchfactor) * PRIVATE(this)->textsize;
 
    switch (PRIVATE(this)->justificationmajor) {
    case SoAsciiText::LEFT:
      if (!PRIVATE(this)->lefttorighttext) {
        maxx -= (maxw + stretchfactor) * PRIVATE(this)->textsize;
        minx -= (maxw + stretchfactor) * PRIVATE(this)->textsize;
      }
      break;
    case SoAsciiText::RIGHT:
      if (PRIVATE(this)->lefttorighttext) {
        maxx = 0.0f;
        minx = -maxw * PRIVATE(this)->textsize; 
      }
      else {
        minx = 0.0f;
        maxx = maxw * PRIVATE(this)->textsize;      
      }
      break;
    case SoAsciiText::CENTER:
      maxx -= maxw * PRIVATE(this)->textsize * 0.5f;
      minx -= maxw * PRIVATE(this)->textsize * 0.5f;
      break;
    default:
      assert(0 && "Unknown justification");
      minx = maxx = 0.0f;
      break;
    }
    
    switch (PRIVATE(this)->justificationminor) {
    case SoAsciiText::LEFT:
      break;
    case SoAsciiText::RIGHT:
      miny += ((n-1) * PRIVATE(this)->textsize);
      maxy += ((n-1) * PRIVATE(this)->textsize);
      break;
    case SoAsciiText::CENTER:
      miny += ((n-1) * PRIVATE(this)->textsize) * 0.5f;
      maxy += ((n-1) * PRIVATE(this)->textsize) * 0.5f;
      break;
    default:
      break;
    }

  }
  else { // -- Vertical text ----------------------------------------


    if (PRIVATE(this)->lefttorighttext) {
      minx = 0.0f;
      maxx = ((n-1) * linespacing) + PRIVATE(this)->textsize;
    }
    else {
      // FIXME: This is probably not the right way of doing this. The
      // box tends to be abit larger on the right side than
      // needed. (14Aug2003 handegar)
      maxx = ((n-1) * linespacing * 0.5f) - PRIVATE(this)->textsize;
      minx = -(n+2) * linespacing * 0.5f;
    }
    

    // FIXME: The 'stretchfactor' addon for vertical text not tested
    // yet... Might be wrong but it works for horizontal
    // text. (27Aug2003 handegar)
    if (PRIVATE(this)->toptobottomtext) {
      maxy = 0.0f;
      miny = -maxstringchars*PRIVATE(this)->textsize - stretchfactor;
    }
    else {
      miny = 0.0f;
      maxy = maxstringchars + stretchfactor;
    }

    switch (PRIVATE(this)->justificationmajor) {
    case SoAsciiText::LEFT:
      break;
    case SoAsciiText::RIGHT:
      maxy += maxstringchars;
      miny += maxstringchars;
      break;
    case SoAsciiText::CENTER:
      maxy += maxstringchars * 0.5f;
      miny += maxstringchars * 0.5f;
      break;
    default:
      assert(0 && "unknown justification");
      minx = maxx = 0.0f;
      break;
    }

    switch (PRIVATE(this)->justificationminor) {
    case SoAsciiText::LEFT:
      break;
    case SoAsciiText::CENTER:      
      maxx -= ((n-1) * PRIVATE(this)->maxglyphheight) * 0.5f;
      minx -= ((n-1) * PRIVATE(this)->maxglyphheight) * 0.5f;
      break;
    case SoAsciiText::RIGHT:
      maxx -= ((n-1) * PRIVATE(this)->maxglyphheight);
      minx -= ((n-1) * PRIVATE(this)->maxglyphheight);
      break;        
    default:
      break;
    }    
 
  }
  
  box.setBounds(SbVec3f(minx, miny, 0.0f), SbVec3f(maxx, maxy, 0.0f));

  // Expanding bbox so that glyphs like 'j's and 'q's are completely inside.
  if (PRIVATE(this)->toptobottomtext) 
    box.extendBy(SbVec3f(0,PRIVATE(this)->maxglyphbbox.getMin()[1] - (n-1)*PRIVATE(this)->textsize*PRIVATE(this)->textspacing, 0));

  center = box.getCenter();
  PRIVATE(this)->unlock();
}

// Doc in parent
void
SoVRMLText::generatePrimitives(SoAction * action)
{
  PRIVATE(this)->lock();

  PRIVATE(this)->setUpGlyphs(action->getState(), this);
  const cc_font_specification * fontspec = PRIVATE(this)->cache->getCachedFontspec();

  int i, n = this->string.getNum();
  const float spacing = PRIVATE(this)->textspacing * PRIVATE(this)->textsize;

  SbBool do2Dtextures = FALSE;
  SbBool do3Dtextures = FALSE;
  if (SoMultiTextureEnabledElement::get(action->getState())) do2Dtextures = TRUE;
  
  // FIXME: implement proper support for 3D-texturing, and get rid of
  // this. 20020120 mortene.
  if (do3Dtextures) {
    static SbBool first = TRUE;
    if (first) {
      first = FALSE;
      SoDebugError::postWarning("SoVRMLText::generatePrimitives",
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
  int maxstringchars = 0;
  for (i = 0; i < n; ++i) 
    maxstringchars = SbMax(maxstringchars, this->string[i].getLength());

  for (i = 0; i < n; i++) {
    detail.setStringIndex(i);
    float xpos = 0.0f;
    
    float stretchlength = 0.0f;
    if (i < this->length.getNum()) 
      stretchlength = this->length[i];
    float stretchfactor = (stretchlength * PRIVATE(this)->textsize) / strlen(this->string[i].getString());
    
    float compressfactor = 1;
    if (this->maxExtent.getValue() > 0) {
      if (PRIVATE(this)->glyphwidths[i] > this->maxExtent.getValue()) {
        assert(PRIVATE(this)->glyphwidths[i] != 0 && "String length == 0! Cannot divide");
        compressfactor = (this->maxExtent.getValue() * PRIVATE(this)->textsize) / PRIVATE(this)->glyphwidths[i];
      }
    }
        
    if (PRIVATE(this)->horizontaltext) { // -- Horizontal text ----------------------
      
      switch (PRIVATE(this)->justificationmajor) {
      case SoAsciiText::LEFT:
        xpos = 0.0f;
        break;
      case SoAsciiText::CENTER:
        if (PRIVATE(this)->lefttorighttext)
          xpos = - PRIVATE(this)->glyphwidths[i] * PRIVATE(this)->textsize * 0.5f;
        else
          xpos = PRIVATE(this)->glyphwidths[i] * PRIVATE(this)->textsize * 0.5f;
        break;
      case SoAsciiText::RIGHT:
        if (PRIVATE(this)->lefttorighttext) 
          xpos = -PRIVATE(this)->glyphwidths[i] * PRIVATE(this)->textsize;
        else
          xpos = PRIVATE(this)->glyphwidths[i] * PRIVATE(this)->textsize;
        break;
      default:
        break;
      }

      switch (PRIVATE(this)->justificationminor) {
      case SoAsciiText::LEFT:
        break;
      case SoAsciiText::CENTER:
        ypos = (i * spacing) + ((n-1) * spacing) * 0.5f;
        break;
      case SoAsciiText::RIGHT:
        ypos = (i * spacing) + ((n-1) * spacing);
        break;        
      default:
        break;
      }
    
    }
    else { // -- Vertical text -----------------------------------------

      if (PRIVATE(this)->lefttorighttext)
        xpos = i * spacing;
      else
        xpos = -i * spacing;
        
      switch (PRIVATE(this)->justificationmajor) {
      case SoAsciiText::LEFT:        
        ypos = -PRIVATE(this)->maxglyphheight;
        break;
      case SoAsciiText::RIGHT:
        if (PRIVATE(this)->toptobottomtext)
          ypos = this->string[i].getLength() * spacing;
        else
          ypos = -this->string[i].getLength() * spacing;
        break;
      case SoAsciiText::CENTER:
        if (PRIVATE(this)->toptobottomtext)
          ypos = this->string[i].getLength() * PRIVATE(this)->textsize * 0.5f;
        else
          ypos = -this->string[i].getLength() * PRIVATE(this)->textsize * 0.5f;
        break;
      default:
        break;
      }

      switch (PRIVATE(this)->justificationminor) {
      case SoAsciiText::LEFT:
        break;
      case SoAsciiText::CENTER:
        xpos -= ((n-1) * spacing) * PRIVATE(this)->textsize * 0.5f;
        break;
      case SoAsciiText::RIGHT:
        xpos -= ((n-1) * spacing) * PRIVATE(this)->textsize;
        break;        
      default:
        break;
      }

    }
    
    SbString str = this->string[i];
    cc_glyph3d * prevglyph = NULL;
    const char * p = str.getString();
    size_t len = cc_string_utf8_validate_length(p);
    assert(len);

    for (unsigned int strcharidx = 0; strcharidx < len; strcharidx++) {
      uint32_t glyphidx = 0;

      glyphidx = cc_string_utf8_get_char(p);
      p = cc_string_utf8_next_char(p);

      cc_glyph3d * glyph = cc_glyph3d_ref(glyphidx, fontspec);

      float advancex, advancey;
      cc_glyph3d_getadvance(glyph, &advancex, &advancey);

      const SbVec2f * coords = (SbVec2f *) cc_glyph3d_getcoords(glyph);
      const int * ptr = cc_glyph3d_getfaceindices(glyph);

      detail.setCharacterIndex(strcharidx);

      if (PRIVATE(this)->horizontaltext) {
        if (!PRIVATE(this)->lefttorighttext)
          xpos -= (advancex + stretchfactor) * PRIVATE(this)->textsize * compressfactor;
      }             
      
      if (strcharidx > 0) {       
        float kerningx = 0.0f;
        float kerningy = 0.0f;
        cc_glyph3d_getkerning(prevglyph, glyph, &kerningx, &kerningy);
        xpos += kerningx * PRIVATE(this)->textsize;
      }
      if (prevglyph) {
        cc_glyph3d_unref(prevglyph);
      }
      prevglyph = glyph;

      while (*ptr >= 0) {
        SbVec2f v0, v1, v2;
        v2 = coords[*ptr++];
        v1 = coords[*ptr++];
        v0 = coords[*ptr++];

        if (do2Dtextures) {
          vertex.setTextureCoords(SbVec2f(v0[0] + xpos/PRIVATE(this)->textsize, 
                                          v0[1] + ypos/PRIVATE(this)->textsize));
        }
        vertex.setPoint(SbVec3f(v0[0] * PRIVATE(this)->textsize + xpos, v0[1] * PRIVATE(this)->textsize + ypos, 0.0f));
        this->shapeVertex(&vertex);

        if (do2Dtextures) {
          vertex.setTextureCoords(SbVec2f(v1[0] + xpos/PRIVATE(this)->textsize, 
                                          v1[1] + ypos/PRIVATE(this)->textsize));
        }
        vertex.setPoint(SbVec3f(v1[0] * PRIVATE(this)->textsize + xpos, v1[1] * PRIVATE(this)->textsize + ypos, 0.0f));
        this->shapeVertex(&vertex);

        if (do2Dtextures) {
          vertex.setTextureCoords(SbVec2f(v2[0] + xpos/PRIVATE(this)->textsize, 
                                          v2[1] + ypos/PRIVATE(this)->textsize));
        }
        vertex.setPoint(SbVec3f(v2[0] * PRIVATE(this)->textsize + xpos, v2[1] * PRIVATE(this)->textsize + ypos, 0.0f));
        this->shapeVertex(&vertex);
      }


      if (!PRIVATE(this)->horizontaltext) {       
        if (PRIVATE(this)->toptobottomtext)
          ypos -= PRIVATE(this)->textsize;
        else 
          ypos += PRIVATE(this)->textsize;       
      } else if (PRIVATE(this)->lefttorighttext)
        xpos += (advancex + stretchfactor) * PRIVATE(this)->textsize * compressfactor; 
    }
    
    if (PRIVATE(this)->horizontaltext) {
      if (PRIVATE(this)->toptobottomtext)
        ypos -= spacing * PRIVATE(this)->maxglyphheight;
      else
        ypos += spacing * PRIVATE(this)->maxglyphheight;
    }
    if (prevglyph) {
      cc_glyph3d_unref(prevglyph);
      prevglyph = NULL;
    }
  }

  this->endShape();
  PRIVATE(this)->unlock();
}

void
SoVRMLTextP::updateFontStyle(void)
{
  // the defaults
  this->textsize = 1.0f;
  this->textspacing = 1.0f;
  this->lefttorighttext = TRUE;
  this->toptobottomtext = TRUE;
  this->horizontaltext = TRUE;
  this->justificationmajor = SoAsciiText::LEFT;
  this->justificationminor = SoAsciiText::LEFT;
  this->fontfamily = SoVRMLFontStyle::SERIF;
  this->fontstyle = SoVRMLFontStyle::PLAIN;

  SoVRMLFontStyle * fs = (SoVRMLFontStyle *)PUBLIC(this)->fontStyle.getValue();
  if (!fs) { return; }

  if (fs->justify.getNum() > 0) {
    // Major mode
    SbString s = fs->justify[0];
    if ((s == "BEGIN") || (s == "FIRST") || (s == "")) {
      this->justificationmajor = SoAsciiText::LEFT;  
    } 
    else if (s == "MIDDLE") {
      this->justificationmajor = SoAsciiText::CENTER;
    } 
    else if (s == "END") {
      this->justificationmajor = SoAsciiText::RIGHT;
    }
    // FIXME: else... Improve robustness & error reporting. 20051207 mortene.
  }
    
  if (fs->justify.getNum() > 1) {
    // Minor mode
    SbString s = fs->justify[1];
    if ((s == "BEGIN") || (s == "FIRST") || (s == ""))
      this->justificationminor = SoAsciiText::LEFT;  
    else if (s == "MIDDLE")
      this->justificationminor = SoAsciiText::CENTER;
    else if (s == "END")
      this->justificationminor = SoAsciiText::RIGHT;
    // FIXME: else... Improve robustness & error reporting. 20051207 mortene.
  }
  
  this->lefttorighttext = fs->leftToRight.getValue();
  this->toptobottomtext = fs->topToBottom.getValue();
  this->horizontaltext = fs->horizontal.getValue();
  this->textsize = fs->size.getValue();
  this->textspacing = fs->spacing.getValue();

  this->fontfamily = SoVRMLFontStyle::SERIF;
  this->fontstyle = SoVRMLFontStyle::PLAIN;

  const char * family = fs->family[0].getString();
  if (family && family[0] != '\0') {
    if (!strcmp(family, "SERIF"))
      this->fontfamily = SoVRMLFontStyle::SERIF;
    else if (!strcmp(family, "SANS"))
      this->fontfamily = SoVRMLFontStyle::SANS;
    else if (!strcmp(family, "TYPEWRITER"))
      this->fontfamily = SoVRMLFontStyle::TYPEWRITER;
  }
      
  const char * style = fs->style[0].getString();
  if (style && style[0] != '\0') {
    if (!strcmp(style, "PLAIN"))
      this->fontstyle = SoVRMLFontStyle::PLAIN;
    else if (!strcmp(style, "BOLD"))
      this->fontstyle = SoVRMLFontStyle::BOLD;
    else if (!strcmp(style, "ITALIC"))
      this->fontstyle = SoVRMLFontStyle::ITALIC;
    else if (!strcmp(style, "BOLDITALIC"))
      this->fontstyle = SoVRMLFontStyle::BOLDITALIC;
  }
}

// recalculate glyphs
void
SoVRMLTextP::setUpGlyphs(SoState * state, SoVRMLText * textnode)
{
  if (this->cache && this->cache->isValid(state)) return;
  
  this->updateFontStyle();

  SoGlyphCache * oldcache = this->cache;

  state->push();
  SbBool storedinvalid = SoCacheElement::setInvalid(FALSE);
  this->cache = new SoGlyphCache(state); 
  this->cache->ref();
  SoCacheElement::set(state, this->cache);
  this->cache->readFontspec(state);
  const cc_font_specification * fontspec = this->cache->getCachedFontspec(); 

  SbString fontstr;
  switch (this->fontfamily) {
  case SoVRMLFontStyle::PLAIN: fontstr = "Times New Roman"; break;
  case SoVRMLFontStyle::SANS: fontstr = "Arial"; break;
  case SoVRMLFontStyle::TYPEWRITER: fontstr = "Courier New"; break;
  default: /* FIXME: warn on faulty input data. 20030921 mortene. */ fontstr = "defaultFont"; break;
  }

  switch (this->fontstyle) {
  case SoVRMLFontStyle::BOLD: fontstr += ":Bold"; break;
  case SoVRMLFontStyle::ITALIC: fontstr += ":Italic"; break;
  case  SoVRMLFontStyle::BOLDITALIC: fontstr += ":Bold Italic"; break;
  default: /* FIXME: check for and warn on faulty input data. 20030921 mortene. */ break;
  }

  this->glyphwidths.truncate(0);

  for (int i = 0; i < textnode->string.getNum(); i++) {
    SbString str = textnode->string[i];
    const char * p = str.getString();
    size_t len = cc_string_utf8_validate_length(p);
    assert(len);

    float stringwidth = 0.0f;
    const float * maxbbox;
    float kerningx = 0;
    float kerningy = 0;
    float advancex = 0;
    float advancey = 0;
    cc_glyph3d * prevglyph = NULL;
    this->maxglyphbbox.makeEmpty();

    for (unsigned int strcharidx = 0; strcharidx < len; strcharidx++) {
      uint32_t glyphidx = 0;

      glyphidx = cc_string_utf8_get_char(p);
      p = cc_string_utf8_next_char(p);

      cc_glyph3d * glyph = cc_glyph3d_ref(glyphidx, fontspec);
      assert(glyph);
      this->cache->addGlyph(glyph);

      maxbbox = cc_glyph3d_getboundingbox(glyph); // Get max height
      this->maxglyphbbox.extendBy(SbVec3f(0, maxbbox[0] * fontspec->size, 0));
      this->maxglyphbbox.extendBy(SbVec3f(0, maxbbox[1] * fontspec->size, 0));

      if (strcharidx > 0) 
        cc_glyph3d_getkerning(prevglyph, glyph, &kerningx, &kerningy);          
      cc_glyph3d_getadvance(glyph, &advancex, &advancey);
  
      stringwidth += (advancex + kerningx);
      prevglyph = glyph;
          
    }

    if (prevglyph != NULL) {
      // Italic font might cause last letter to be outside bbox. Add width if needed.
      if (advancex < cc_glyph3d_getwidth(prevglyph)) 
        stringwidth += (cc_glyph3d_getwidth(prevglyph) - advancex);
    }      

    this->glyphwidths.append(stringwidth);
  }
  state->pop();
  SoCacheElement::setInvalid(storedinvalid);

  // unref old cache after creating the new one to avoid recreating glyphs
  if (oldcache) oldcache->unref();
}

#undef PRIVATE
#undef PUBLIC

#endif // HAVE_VRML97
