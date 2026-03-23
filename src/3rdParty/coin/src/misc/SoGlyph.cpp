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
  \class SoGlyph SoGlyph.h Inventor/misc/SoGlyph.h
  \brief The SoGlyph class is used to generate and reuse font glyph bitmaps and outlines.

  <b>This class is now obsolete, and will be removed from a later
  version of Coin.</b>
  
  SoGlyph is the public interface all text nodes (both built-in and
  extensions) should use to generate bitmaps and outlines for font
  glyphs. It maintains an internal cache of previously requested
  glyphs to avoid needless calls into the font library.
  
  Primer: a \e glyph is the graphical representation of a given
  character of a given font at a given size and orientation. It can be
  either a \e bitmap (pixel aligned with the viewport) or an \e
  outline (polygonal representation) that can be transformed or
  extruded like any other 3D geometry. Bitmaps are used by SoText2,
  while the other text nodes uses outlines.
  
  \COIN_CLASS_EXTENSION

  \since Coin 2.0

  \sa SoText2, SoText3, SoAsciiText
*/

// SoGlyph uses the Coin-internal font lib wrapper functions
// (cc_flw_*()) to provide bitmaps and outlines.
//
// FIXME: font support for outline glyphs. 200303?? preng.
  
#include <Inventor/misc/SoGlyph.h>
#include "coindefs.h"

#include <cstdlib>
#include <cstring>

#include <Inventor/errors/SoDebugError.h>
#include <Inventor/C/tidbits.h>
#include <Inventor/SbName.h>
#include <Inventor/SbString.h>
#include <Inventor/SbVec2f.h>
#include <Inventor/SbVec2s.h>
#include <Inventor/lists/SbList.h>
#include <Inventor/elements/SoFontNameElement.h>
#include <Inventor/elements/SoFontSizeElement.h>

#include "tidbitsp.h"
#include "threads/threadsutilp.h"
#include "fonts/common.h"
#include "fonts/fontlib_wrapper.h"
#include "fonts/defaultfonts.h"

class SoGlyphP {
public:
  SoGlyphP(SoGlyph * master) : master(master) { }
  SoGlyph * master;


  const SbVec2f * coords;
  SbBox2f bbox;
  const int * faceidx;
  const int * edgeidx;
  int refcount;
  float ymin, ymax;
  
  int fontidx;
  int glyphidx;
  float angle;
  SbVec2s size;
  unsigned int character;
  SbBool fonttypeis3d;
  SbBool coordsinstalled;

  int bitmapwidth;
  int bitmapheight;
  cc_font_bitmap * bitmap;

  struct {
    unsigned int didcalcbbox : 1;
  } flags;

  void setup3DFontData();

  static SoGlyph * createSystemGlyph(const char character, const SbName & font);
  static SoGlyph * createSystemGlyph(const unsigned int COIN_UNUSED_ARG(character), SoState * COIN_UNUSED_ARG(state)) {return NULL;};  static SoGlyph * createSystemGlyph(const char character, int fontid);
};

#define PRIVATE(p) ((p)->pimpl)
#define PUBLIC(p) ((p)->master)


/*!
  Constructor.
*/
SoGlyph::SoGlyph(void)
{
  PRIVATE(this) = new SoGlyphP(this);
  PRIVATE(this)->refcount = 0;
  PRIVATE(this)->flags.didcalcbbox = 0;
  PRIVATE(this)->coords = NULL;
  PRIVATE(this)->faceidx = NULL;
  PRIVATE(this)->edgeidx = NULL;
  PRIVATE(this)->ymin = 0.0f;
  PRIVATE(this)->ymax = 0.0f;

  /* Setting fonttype==3D as default since the 2D text node (SoText2)
     is no longer depending on the SoGlyph node (20030908 handegar) */
  PRIVATE(this)->fonttypeis3d = TRUE;

  PRIVATE(this)->fontidx = 0;
  PRIVATE(this)->coordsinstalled = FALSE;

  PRIVATE(this)->bitmapwidth = 0;
  PRIVATE(this)->bitmapheight = 0;
  PRIVATE(this)->bitmap = NULL;
}

/*!
  Destructor.
*/
SoGlyph::~SoGlyph()
{
  delete PRIVATE(this);
}

/*!
  Should be called when a node no longer will use a glyph. Will
  free memory used by this glyph when it is no longer used by any node.
*/
void
SoGlyph::unref(void) const
{
  SoGlyph::unrefGlyph((SoGlyph*)this);
}

/*!  
  Used to indicate how the glyph should be treated. This is needed if
  correct bounding box shall be calculated etc. As default, glyphs are
  treated as a part of a 2D font.
*/

void 
SoGlyph::setFontType(Fonttype type) const
{
  if (type == SoGlyph::FONT3D) {
    PRIVATE(this)->fonttypeis3d = TRUE;
  }
  else {
    PRIVATE(this)->fonttypeis3d = FALSE;
  }  
}


/*!
  Returns coordinates for this glyph.
*/
const SbVec2f *
SoGlyph::getCoords(void) const
{
  if (!PRIVATE(this)->coordsinstalled) {
    PRIVATE(this)->setup3DFontData();
    PRIVATE(this)->coordsinstalled = TRUE;
  }
  return PRIVATE(this)->coords;
}

/*!
  Returns face indices for this glyph.
*/
const int *
SoGlyph::getFaceIndices(void) const
{
  if (!PRIVATE(this)->coordsinstalled) {
    PRIVATE(this)->setup3DFontData();
    PRIVATE(this)->coordsinstalled = TRUE;
  }

  return PRIVATE(this)->faceidx;
}

/*!
  Returns edge indices for this glyph.
*/
const int *
SoGlyph::getEdgeIndices(void) const
{
  if (!PRIVATE(this)->coordsinstalled) {
    PRIVATE(this)->setup3DFontData();
    PRIVATE(this)->coordsinstalled = TRUE;
  }
  
  return PRIVATE(this)->edgeidx;
}

/*!
  Returns a pointer to the next clockwise edge. Returns NULL if
  none could be found.
*/
const int *
SoGlyph::getNextCWEdge(const int edgeidx) const
{
  int idx = edgeidx * 2;
  // test for common case
  if (edgeidx > 0) {
    if (PRIVATE(this)->edgeidx[idx] == PRIVATE(this)->edgeidx[idx-1])
      return &PRIVATE(this)->edgeidx[idx-2];
  }
  // do a linear search
  int findidx = PRIVATE(this)->edgeidx[idx];
  const int * ptr = PRIVATE(this)->edgeidx;
  while (*ptr >= 0) {
    if (ptr[1] == findidx) return ptr;
    ptr += 2;
  }
  return NULL;
}

/*!
  Returns a pointer to the next counterclockwise edge. Returns NULL if
  none could be found.
*/
const int *
SoGlyph::getNextCCWEdge(const int edgeidx) const
{
  int idx = edgeidx * 2;
  // test for common case
  if (PRIVATE(this)->edgeidx[idx+1] == PRIVATE(this)->edgeidx[idx+2])
    return &PRIVATE(this)->edgeidx[idx+2];
  // do a linear search
  int findidx = PRIVATE(this)->edgeidx[idx+1];
  const int * ptr = PRIVATE(this)->edgeidx;
  while (*ptr >= 0) {
    if (*ptr == findidx) return ptr;
    ptr += 2;
  }
  return NULL;
}

/*!
  Convenience method which returns the exact width of the glyph.
*/
float
SoGlyph::getWidth(void) const
{

  if (!PRIVATE(this)->fonttypeis3d)
    return (float) PRIVATE(this)->bitmapwidth;

  const SbBox2f & box = this->getBoundingBox();
  return box.getMax()[0] - box.getMin()[0];
}

/*!
  Returns the bounding box of this glyph. This value is cached for performance.
*/
const SbBox2f &
SoGlyph::getBoundingBox(void) const
{
  // this method needs to be const, so cast away constness
  SoGlyph * thisp = (SoGlyph*) this;
  if (!PRIVATE(this)->flags.didcalcbbox) {

    if (!PRIVATE(this)->coordsinstalled) {
      PRIVATE(this)->setup3DFontData();
      PRIVATE(this)->coordsinstalled = TRUE;
    }

    PRIVATE(thisp)->bbox.makeEmpty();
    const int *ptr = PRIVATE(this)->edgeidx;
    int idx = *ptr++;

    while (idx >= 0) {
      PRIVATE(thisp)->bbox.extendBy(PRIVATE(this)->coords[idx]);
      idx = *ptr++;
    }

    // FIXME: the 'get_advance' call has been separated into one
    // 'get_vector_advance' and one 'get_bitmap_advance' call. As we
    // cannot separate whether we are dealing with bitmaps or vector
    // glyphs in this class, we choose 'get_vector_advance' as
    // default. (20030926 handegar)    
    float advancex, advancey;
    cc_flw_get_vector_advance(PRIVATE(this)->fontidx, PRIVATE(this)->glyphidx, &advancex, &advancey);
    SbVec2f max = PRIVATE(this)->bbox.getMax();
    
    PRIVATE(this)->bbox.extendBy(SbVec2f(advancex, advancey));
   
    PRIVATE(thisp)->flags.didcalcbbox = 1;
  }


  return PRIVATE(this)->bbox;
}

/*!
  Sets the coordinates for this glyph.
*/
void
SoGlyph::setCoords(const SbVec2f *coords, int numcoords)
{
  // It used to be valid to call this function with a negative value
  // (which signified that data should not be copied). All invoking
  // code just passed in -1, so we have simplified this function
  // (SoGlyph is being obsoleted anyway).
  //
  // Note that since we are just copying the data pointer, we assume
  // that all 3D glyphs use permanent storage for their publicly
  // exposed data.
  assert(numcoords == -1);

  PRIVATE(this)->coords = coords;
}

/*!
  Sets the face indices for this glyph.
*/
void
SoGlyph::setFaceIndices(const int *indices, int numindices)
{
  // It used to be valid to call this function with a negative value
  // (which signified that data should not be copied). All invoking
  // code just passed in -1, so we have simplified this function
  // (SoGlyph is being obsoleted anyway).
  //
  // Note that since we are just copying the data pointer, we assume
  // that all 3D glyphs use permanent storage for their publicly
  // exposed data.
  assert(numindices == -1);

  PRIVATE(this)->faceidx = indices;
}

/*!
  Sets the edge indices for this glyph.
*/
void
SoGlyph::setEdgeIndices(const int *indices, int numindices)
{
  // It used to be valid to call this function with a negative value
  // (which signified that data should not be copied). All invoking
  // code just passed in -1, so we have simplified this function
  // (SoGlyph is being obsoleted anyway).
  //
  // Note that since we are just copying the data pointer, we assume
  // that all 3D glyphs use permanent storage for their publicly
  // exposed data.
  assert(numindices == -1);

  PRIVATE(this)->edgeidx = indices;
}



//
// static methods to handle glyph reusage.
//
// FIXME: use SbHash to look up glyphs a bit faster. pederb, 20000323
//

class coin_glyph_info {
public:
  coin_glyph_info() {
    this->character = 0;
    this->size = 0.0;
    this->glyph = NULL;
    this->angle = 0.0;
  }
  coin_glyph_info(const unsigned int characterarg, const float sizearg, const SbName &fontarg, SoGlyph *glypharg, const float anglearg)
  {
    this->character = characterarg;
    this->size = sizearg;
    this->font = fontarg;
    this->angle = anglearg;
    this->glyph = glypharg;
  }
  
  // Note: bitmap glyphs have valid size, polygonal glyphs have size=-1.0
  SbBool matches(const unsigned int characterarg, const float sizearg, 
                 const SbName & fontarg, const float anglearg) {
    return (this->character == characterarg) && (this->size == sizearg) && (this->font == fontarg) && (this->angle == anglearg);
  }
  
  // AIX native compiler xlC needs equality and inequality operators
  // to compile templates where these operators are referenced (even
  // if they are actually never used).
  
  SbBool operator==(const coin_glyph_info & gi) {
    return this->matches(gi.character, gi.size, gi.font, gi.angle) && this->glyph == gi.glyph;
  }
  SbBool operator!=(const coin_glyph_info & gi) {
    return !(*this == gi);
  }

  unsigned int character;
  float size;
  SbName font;
  SoGlyph * glyph;
  float angle;
};

static SbList <coin_glyph_info> * activeGlyphs = NULL;
static void * SoGlyph_mutex = NULL;

static void
SoGlyph_cleanup(void)
{
  delete activeGlyphs;
  activeGlyphs = NULL;
  CC_MUTEX_DESTRUCT(SoGlyph_mutex);
}

/*!
  Returns a character of the specified font, suitable for polygonal
  rendering.
*/
const SoGlyph *
SoGlyph::getGlyph(const char character, const SbName & font)
{

  // FIXME: the API and implementation of this class isn't consistent
  // with regard to the parameters and variables that are glyph codes
  // -- some places they are "char", other places "int", some places
  // signed, other places unsigned. Should audit and fix as much as
  // possible without breaking API and ABI compatibility. *sigh*
  // 20030611 mortene.
  
  // Similar code in start of getGlyph(..., state) - keep in sync.
  if (SoGlyph_mutex == NULL) {
    CC_MUTEX_CONSTRUCT(SoGlyph_mutex);
  }

  // FIXME: it would probably be a good idea to have a small LRU-type
  // glyph cache to avoid freeing glyphs too early. If for instance
  // the user creates a single SoText3 node which is used several
  // times in a graph with different fonts, glyphs will be freed and
  // recreated all the time. pederb, 20000324

  CC_MUTEX_LOCK(SoGlyph_mutex);

  if (activeGlyphs == NULL) {
    activeGlyphs = new SbList <coin_glyph_info>;
    coin_atexit((coin_atexit_f *)SoGlyph_cleanup, CC_ATEXIT_NORMAL);
  }

  int i, n = activeGlyphs->getLength();
  for (i = 0; i < n; i++) {
    // Search for fontsize -1 to avoid getting a bitmap glyph.
    if ((*activeGlyphs)[i].matches(character, -1.0, font, 0.0)) break;
  }
  if (i < n) {
    SoGlyph *glyph = (*activeGlyphs)[i].glyph;
    PRIVATE(glyph)->refcount++;
    CC_MUTEX_UNLOCK(SoGlyph_mutex);
    return glyph;
  }

  SoGlyph * glyph = SoGlyphP::createSystemGlyph(character, font);
  
  // FIXME: don't think this is necessary, we should _always_ get a
  // glyph. If none exist in the font, we should eventually fall back
  // on making a square in the code we're calling into. Move the code
  // below to handle this deeper down into the call-stack. 20030527 mortene.
  if (glyph == NULL) { // no system font could be loaded
    glyph = new SoGlyph;
    if (character <= 32 || character >= 127) {
      // treat all these characters as spaces
      static int spaceidx[] = { -1 };
      glyph->setCoords(NULL);
      glyph->setFaceIndices(spaceidx);
      glyph->setEdgeIndices(spaceidx);
      PRIVATE(glyph)->bbox.setBounds(SbVec2f(0.0f, 0.0f), SbVec2f(0.2f, 0.0f));
      PRIVATE(glyph)->flags.didcalcbbox = 1;
    }
    else {
      const int idx = character-33;
      glyph->setCoords((const SbVec2f*)coin_default3dfont_get_coords()[idx]);
      glyph->setFaceIndices(coin_default3dfont_get_faceidx()[idx]);
      glyph->setEdgeIndices(coin_default3dfont_get_edgeidx()[idx]);
    }
  }
  // Use impossible font size to avoid mixing polygonal & bitmap glyphs.
  coin_glyph_info info(character, -1.0, font, glyph, 0.0);
  PRIVATE(glyph)->refcount++;
  activeGlyphs->append(info);
  CC_MUTEX_UNLOCK(SoGlyph_mutex);
  return glyph;

}

// private method that removed glyph from active list when deleted
void
SoGlyph::unrefGlyph(SoGlyph *glyph)
{
  CC_MUTEX_LOCK(SoGlyph_mutex);
  assert(activeGlyphs);
  assert(PRIVATE(glyph)->refcount > 0);
  PRIVATE(glyph)->refcount--;
  if (PRIVATE(glyph)->refcount == 0) {
    int i, n = activeGlyphs->getLength();
    for (i = 0; i < n; i++) {
      if ((*activeGlyphs)[i].glyph == glyph) break;
    }
    assert(i < n);
    activeGlyphs->removeFast(i);
    cc_flw_unref_font(PRIVATE(glyph)->fontidx);
    delete glyph;
  }
  CC_MUTEX_UNLOCK(SoGlyph_mutex);
}

/*!
  Returns a character of the specified font, suitable for bitmap
  rendering.  The size parameter overrides state's SoFontSizeElement
  (if != SbVec2s(0,0))
*/
const SoGlyph *
SoGlyph::getGlyph(SoState * state,
                  const unsigned int character,
                  const SbVec2s & size,
                  const float angle)
{
  assert(state);
  
  SbName state_name = SoFontNameElement::get(state);
  float state_size = SoFontSizeElement::get(state);
  
  if (state_name == SbName::empty()) {
    state_name = SbName("defaultFont");
    state_size = 10.0;
  } 
     
  SbVec2s fontsize((short)state_size, (short)state_size);
  if (size != SbVec2s(0,0)) { fontsize = size; }
  
  // Similar code in start of getGlyph(..., fontname) - keep in sync.
  if (SoGlyph_mutex == NULL) {
    CC_MUTEX_CONSTRUCT(SoGlyph_mutex);
  }

  CC_MUTEX_LOCK(SoGlyph_mutex);
  
  if (activeGlyphs == NULL) {
    activeGlyphs = new SbList <coin_glyph_info>;
    coin_atexit((coin_atexit_f *)SoGlyph_cleanup, CC_ATEXIT_NORMAL);
  }

  int i, n = activeGlyphs->getLength();
  for (i = 0; i < n; i++) {
    if ((*activeGlyphs)[i].matches(character, fontsize[0], state_name, angle)) {
      SoGlyph *glyph = (*activeGlyphs)[i].glyph;
      PRIVATE(glyph)->refcount++;
      CC_MUTEX_UNLOCK(SoGlyph_mutex);
      return glyph;
    }
  }
  
  // FIXME: use font style in addition to font name. preng 2003-03-03
  SbString fontname = state_name.getString();

  const int font = cc_flw_get_font_id(fontname.getString(), fontsize[1], angle, 0.5f);
  // Should _always_ be able to get hold of a font.
  assert(font >= 0);
  cc_flw_ref_font(font);

  const int glyphidx = cc_flw_get_glyph(font, character);

  // Should _always_ be able to get hold of a glyph -- if no glyph is
  // available for a specific character, a default empty rectangle
  // should be used.  -mortene.
  assert(glyphidx >= 0);

  SoGlyph * g = new SoGlyph();
  PRIVATE(g)->fontidx = font;
  PRIVATE(g)->glyphidx = glyphidx;
  PRIVATE(g)->size = fontsize;
  PRIVATE(g)->angle = angle;
  PRIVATE(g)->character = character;
  coin_glyph_info info(character, state_size, state_name, g, angle);
  PRIVATE(g)->refcount++;
  activeGlyphs->append(info);

  CC_MUTEX_UNLOCK(SoGlyph_mutex);
  return g;
}

// Pixel advance for this glyph.
SbVec2s
SoGlyph::getAdvance(void) const
{
  assert(PRIVATE(this)->fontidx >= 0 && PRIVATE(this)->glyphidx >= 0);

  int x, y;
  // FIXME: the 'get_advance' call has been separated into one
  // 'get_vector_advance' and one 'get_bitmap_advance' call. As we
  // cannot separate whether we are dealing with bitmaps or vector
  // glyphs in this class, we choose 'get_bitmap_advance' as
  // default. (20030926 handegar)    
  cc_flw_get_bitmap_advance(PRIVATE(this)->fontidx, PRIVATE(this)->glyphidx, &x, &y);
  return SbVec2s((short)x, (short)y);
}

// Pixel kerning when rightglyph is placed to the right of this.
SbVec2s
SoGlyph::getKerning(const SoGlyph & rightglyph) const
{
  assert(PRIVATE(this)->fontidx >= 0 && PRIVATE(this)->glyphidx >= 0);
  assert(PRIVATE(&rightglyph)->fontidx >= 0 && PRIVATE(&rightglyph)->glyphidx >= 0);

  int x, y;
  // FIXME: Same issues as with 'getAdvance()'. See fixme. (20030926 handegar)
  cc_flw_get_bitmap_kerning(PRIVATE(this)->fontidx,
                            PRIVATE(this)->glyphidx, PRIVATE(&rightglyph)->glyphidx,
                            &x, &y);
  return SbVec2s((short)x, (short)y);
}

/*!
  Bitmap for glyph. \a size and \a pos are return parameters.
  Antialiased bitmap graphics are not yet supported.

  Note that this function may return \c NULL if the glyph has no
  visible pixels (as for e.g. the space character).

  The returned buffer should \e not be deallocated by the caller.
*/
unsigned char *
SoGlyph::getBitmap(SbVec2s & size, SbVec2s & pos, const SbBool COIN_UNUSED_ARG(antialiased)) const
{
  if (PRIVATE(this)->bitmap == NULL) {
    PRIVATE(this)->bitmap = cc_flw_get_bitmap(PRIVATE(this)->fontidx, PRIVATE(this)->glyphidx);
  }
  struct cc_font_bitmap * bm = PRIVATE(this)->bitmap;
  assert(bm);
  
  PRIVATE(this)->bitmapwidth = bm->width;
  PRIVATE(this)->bitmapheight = bm->rows;
  
  size[0] = bm->pitch * 8;
  size[1] = bm->rows;
  pos[0] = bm->bearingX;
  pos[1] = bm->bearingY; 

  return bm->buffer;
}


void
SoGlyphP::setup3DFontData(void)
{
  PUBLIC(this)->setFontType(SoGlyph::FONT3D);
  
  if (character <= 32 || character >= 127) {
    // treat all these characters as spaces
    static int spaceidx[] = { -1 };
    PUBLIC(this)->setCoords(NULL);
    PUBLIC(this)->setFaceIndices(spaceidx);
    PUBLIC(this)->setEdgeIndices(spaceidx);
    this->bbox.setBounds(SbVec2f(0.0f, 0.0f), SbVec2f(0.2f, 0.0f));
    this->flags.didcalcbbox = 1;
  }
  else {
    cc_font_vector_glyph * vector_glyph = cc_flw_get_vector_glyph(this->fontidx, this->character);
    
    if (vector_glyph == NULL) {
      // Default hardcoded 3d font. Size = 1.0
      const int idx = this->character-33;
      PUBLIC(this)->setCoords((const SbVec2f *)coin_default3dfont_get_coords()[idx]);
      PUBLIC(this)->setFaceIndices(coin_default3dfont_get_faceidx()[idx]);
      PUBLIC(this)->setEdgeIndices(coin_default3dfont_get_edgeidx()[idx]);
    } 
    else {
      // Install truetype font

      PUBLIC(this)->setCoords((const SbVec2f *)cc_flw_get_vector_glyph_coords(vector_glyph));
      PUBLIC(this)->setFaceIndices(cc_flw_get_vector_glyph_faceidx(vector_glyph));
      PUBLIC(this)->setEdgeIndices(cc_flw_get_vector_glyph_edgeidx(vector_glyph));
    }
  }
}


// should handle platform-specific font loading
SoGlyph *
SoGlyphP::createSystemGlyph(const char COIN_UNUSED_ARG(character), int COIN_UNUSED_ARG(fontid))
{ 
  return NULL;
}

SoGlyph *
SoGlyphP::createSystemGlyph(const char COIN_UNUSED_ARG(character), const SbName & COIN_UNUSED_ARG(font))
{
  return NULL;
}

#undef PRIVATE
#undef PUBLIC
