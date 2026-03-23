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
  \class SoGLDisplayList Inventor/elements/SoGLDisplayList.h
  \brief The SoGLDisplayList class stores and manages OpenGL display lists.

  \ingroup coin_elements

  The TEXTURE_OBJECT type is not directly supported in Coin. We handle
  textures differently in a more flexible class called SoGLImage,
  which also stores some information about the texture used when
  rendering. Old code which use this element should not stop
  working though. The texture object extension will just not be used,
  and the texture will be stored in a display list instead.
*/

// *************************************************************************

#include <Inventor/elements/SoGLDisplayList.h>

#include <cstring>
#include <cassert>

#include <Inventor/C/glue/gl.h>
#include <Inventor/caches/SoGLRenderCache.h>
#include <Inventor/elements/SoCacheElement.h>
#include <Inventor/elements/SoGLCacheContextElement.h>
#include <Inventor/errors/SoDebugError.h>
#include <Inventor/misc/SoState.h>
#include <Inventor/misc/SoGLDriverDatabase.h>

#include "glue/glp.h"
#include "rendering/SoGL.h"
#include "coindefs.h"

#ifndef COIN_WORKAROUND_NO_USING_STD_FUNCS
using std::strcmp;
#endif // !COIN_WORKAROUND_NO_USING_STD_FUNCS

class SoGLDisplayListP {
 public:
  SoGLDisplayList::Type type;
  int numalloc;
  unsigned int firstindex;
  int context;
  int refcount;
  int openindex;
  SbBool mipmap;
  GLenum texturetarget;
};

#define PRIVATE(obj) obj->pimpl

// *************************************************************************

/*!
  Constructor.
*/
SoGLDisplayList::SoGLDisplayList(SoState * state, Type type, int allocnum,
                                 SbBool mipmaptexobj)
{
  PRIVATE(this) = new SoGLDisplayListP;
  PRIVATE(this)->type = type;
  PRIVATE(this)->numalloc = allocnum;
  PRIVATE(this)->context = SoGLCacheContextElement::get(state);
  PRIVATE(this)->refcount = 0;
  PRIVATE(this)->mipmap = mipmaptexobj;
  PRIVATE(this)->texturetarget = 0;

#if COIN_DEBUG && 0 // debug
  SoDebugError::postInfo("SoGLDisplayList::SoGLDisplayList", "%p", this);
#endif // debug

  // Check for known buggy OpenGL driver.
  const cc_glglue * glw = cc_glglue_instance(PRIVATE(this)->context);
  assert(glw->versionstr && "glGetString() returned 0 -- no valid GL context?");
  if (glw->versionstr && strcmp(glw->versionstr, "1.3.1 NVIDIA 28.02") == 0) {
    // (From NVidia's changelog, it looks like the problem we've been
    // seeing with the 28.02 driver and displaylists *might* have been
    // fixed for the next version (28.80)).

    // Here's an Inventor file which can be used to reproduce the bug:
    // -----8<----- [snip] ----------8<----- [snip] ----------8<------
    // #Inventor V2.1 ascii
    //
    // # This dead simple scene graph causes the examiner viewer to go blank
    // # on a Linux box with GeForce2 card and version 1.3.1 28.02 of the
    // # NVidia OpenGL drivers. The problem is gone for version 1.3.1 29.60
    // # of the drivers, so this seems _very_ much like a driver bug with
    // # OpenGL display lists.
    // #
    // # The bug is also present for SGI Inventor.
    // #
    // # <mortene@sim.no>.
    //
    // Separator {
    //    renderCaching ON
    //    ShapeHints {
    //       vertexOrdering COUNTERCLOCKWISE
    //       faceType UNKNOWN_FACE_TYPE
    //    }
    //    Cube { }
    // }
    // -----8<----- [snip] ----------8<----- [snip] ----------8<------

    // FIXME: should be more robust, and rather just disable the use
    // of GL displaylists (but still issuing an
    // SoDebugError::postWarning()). This should be straightforward to
    // do when the FIXME below on better handling of the case where we
    // are not able to allocate displaylist indices is taken care
    // of. 20020911 mortene.

    static SbBool first = TRUE;
    if (first) {
      SoDebugError::post("SoGLDisplayList::SoGLDisplayList",
                         "This OpenGL driver ('%s') is known to contain serious "
                         "bugs in GL displaylist handling, and we strongly urge "
                         "you to upgrade! As long as you are using this driver, "
                         "GL rendering is likely to cause all sorts of nasty "
                         "problems.",
                         glw->versionstr);
      first = FALSE;
    }
  }

  // Reserve displaylist IDs.

  if (PRIVATE(this)->type == TEXTURE_OBJECT) {
    assert(allocnum == 1 && "it is only possible to create one texture object at a time");
    if (SoGLDriverDatabase::isSupported(glw, SO_GL_TEXTURE_OBJECT)) {
      // use temporary variable, in case GLuint is typedef'ed to
      // something other than unsigned int
      GLuint tmpindex;
      cc_glglue_glGenTextures(glw, 1, &tmpindex);
      PRIVATE(this)->firstindex = (unsigned int )tmpindex;
    }
    else { // Fall back to display list, allocation happens further down below.
      PRIVATE(this)->type = DISPLAY_LIST;
    }
  }

  if (PRIVATE(this)->type == DISPLAY_LIST) {
    PRIVATE(this)->firstindex = (unsigned int) glGenLists(allocnum);
    if (PRIVATE(this)->firstindex == 0) {
      SoDebugError::post("SoGLDisplayList::SoGLDisplayList",
                         "Could not reserve %d displaylist%s. "
                         "Expect flawed rendering.",
                         allocnum, allocnum==1 ? "" : "s");
      // FIXME: be more robust in handling this -- the rendering will
      // gradually go bonkers after we hit this problem. 20020619 mortene.
    }
#if COIN_DEBUG && 0 // debug
    SoDebugError::postInfo("SoGLDisplayList::SoGLDisplayList",
                           "firstindex==%d", PRIVATE(this)->firstindex);
#endif // debug
  }
}

// private destructor. Use ref()/unref()
SoGLDisplayList::~SoGLDisplayList()
{
#if COIN_DEBUG && 0 // debug
  SoDebugError::postInfo("SoGLDisplayList::~SoGLDisplayList", "%p", this);
#endif // debug

  if (PRIVATE(this)->type == DISPLAY_LIST) {
    glDeleteLists((GLuint) PRIVATE(this)->firstindex, PRIVATE(this)->numalloc);
  }
  else {
    assert(PRIVATE(this)->type == TEXTURE_OBJECT);

    const cc_glglue * glw = cc_glglue_instance(PRIVATE(this)->context);
    assert(cc_glglue_has_texture_objects(glw));

    // Use temporary variable in case GLUint != unsigned int.
    GLuint tmpindex = (GLuint) PRIVATE(this)->firstindex;
    // It is only possible to create one texture object at a time, so
    // there's only one index to delete.
    cc_glglue_glDeleteTextures(glw, 1, &tmpindex);
  }
  delete PRIVATE(this);
}

/*!
  Increase reference count for this display list/texture object.
*/
void
SoGLDisplayList::ref(void)
{
  PRIVATE(this)->refcount++;
}

/*!
  Decrease reference count for this instance. When reference count
  reaches 0, the instance is deleted.
*/
void
SoGLDisplayList::unref(SoState * state)
{
  assert(PRIVATE(this)->refcount > 0);
  if (--PRIVATE(this)->refcount == 0) {
    // Let SoGLCacheContext delete this instance the next time context is current.
    SoGLCacheContextElement::scheduleDelete(state, this);
  }
}

/*!
  Open this display list/texture object.
*/
void
SoGLDisplayList::open(SoState * state, int index)
{
  if (PRIVATE(this)->type == DISPLAY_LIST) {
    PRIVATE(this)->openindex = index;
    // using GL_COMPILE here instead of GL_COMPILE_AND_EXECUTE will
    // lead to much higher performance on nVidia cards, and doesn't
    // hurt performance for other vendors.
    glNewList((GLuint) (PRIVATE(this)->firstindex+PRIVATE(this)->openindex), GL_COMPILE);
  }
  else {
    assert(PRIVATE(this)->type == TEXTURE_OBJECT);
    assert(index == 0);
    this->bindTexture(state);
  }
}

/*!
  Close this display list/texture object.
*/
void
SoGLDisplayList::close(SoState * COIN_UNUSED_ARG(state))
{
  if (PRIVATE(this)->type == DISPLAY_LIST) {
    glEndList();
    GLenum err = sogl_glerror_debugging() ? glGetError() : GL_NO_ERROR;
    if (err == GL_OUT_OF_MEMORY) {
      SoDebugError::post("SoGLDisplayList::close",
                         "Not enough memory resources available on system "
                         "to store full display list. Expect flaws in "
                         "rendering.");
    }
    glCallList((GLuint) (PRIVATE(this)->firstindex + PRIVATE(this)->openindex));
  }
  else {
    const cc_glglue * glw = cc_glglue_instance(PRIVATE(this)->context);
    assert(cc_glglue_has_texture_objects(glw));
    GLenum target = PRIVATE(this)->texturetarget;
    if (target == 0) {
      // target is not set. Assume normal 2D texture.
      target = GL_TEXTURE_2D;
    }
    // unbind current texture object
    cc_glglue_glBindTexture(glw, target, (GLuint) 0);
  }
}

/*!
  Execute this display list/texture object.
*/
void
SoGLDisplayList::call(SoState * state, int index)
{
  if (PRIVATE(this)->type == DISPLAY_LIST) {
    glCallList((GLuint) (PRIVATE(this)->firstindex + index));
  }
  else {
    assert(PRIVATE(this)->type == TEXTURE_OBJECT);
    assert(index == 0);
    this->bindTexture(state);
  }
  this->addDependency(state);
}

/*!
  Create a dependency on the display list.
*/
void
SoGLDisplayList::addDependency(SoState * state)
{
  if (state->isCacheOpen()) {
    SoGLRenderCache * cache = (SoGLRenderCache*)
      SoCacheElement::getCurrentCache(state);
    if (cache) cache->addNestedCache(this);
  }
}

/*!
  Returns whether the texture object stored in this instance
  was created with mipmap data. This method is an extension
  versus the Open Inventor API.
*/
SbBool
SoGLDisplayList::isMipMapTextureObject(void) const
{
  return PRIVATE(this)->mipmap;
}

/*!
  Return type. Display list or texture object.
*/
SoGLDisplayList::Type
SoGLDisplayList::getType(void) const
{
  return PRIVATE(this)->type;
}

/*!
  Return number of display lists/texture objects allocated.
*/
int
SoGLDisplayList::getNumAllocated(void) const
{
  return PRIVATE(this)->numalloc;
}

/*!
  Return first GL index for this display list.
*/
unsigned int
SoGLDisplayList::getFirstIndex(void) const
{
  return PRIVATE(this)->firstindex;
}

/*!
  Return an id for the current context.
*/
int
SoGLDisplayList::getContext(void) const
{
  return PRIVATE(this)->context;
}

/*!
  Sets the texture object target
  \since Coin 2.5
*/
void
SoGLDisplayList::setTextureTarget(int target)
{
  PRIVATE(this)->texturetarget = (GLenum) target;
}

/*!
  Returns the texture target
  \since Coin 2.5
*/
int
SoGLDisplayList::getTextureTarget(void) const
{
  if (PRIVATE(this)->texturetarget)
    return (int) PRIVATE(this)->texturetarget;
  return GL_TEXTURE_2D;
}

/*!
  \COININTERNAL

  \COIN_FUNCTION_EXTENSION

  \since Coin 2.0
*/
void
SoGLDisplayList::bindTexture(SoState * COIN_UNUSED_ARG(state))
{
  const cc_glglue * glw = cc_glglue_instance(PRIVATE(this)->context);
  assert(cc_glglue_has_texture_objects(glw));

  GLenum target = PRIVATE(this)->texturetarget;
  if (target == 0) {
    // target is not set. Assume normal 2D texture.
    target = GL_TEXTURE_2D;
  }
  cc_glglue_glBindTexture(glw, target, (GLuint)PRIVATE(this)->firstindex);
}

#undef PRIVATE
