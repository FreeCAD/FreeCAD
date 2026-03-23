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
  \class SoGetPrimitiveCountAction SoGetPrimitiveCountAction.h Inventor/actions/SoGetPrimitiveCountAction.h
  \brief The SoGetPrimitiveCountAction class counts the primitives in a scene.

  \ingroup coin_actions

  Apply this action to a scene if you need to know the number of
  primitives present in a scene graph, or parts of a scene graph.


  One common mistake to make when using this action is to think that
  it traverses just the parts currently in view, like SoGLRenderAction
  does. (SoGLRenderAction culls away the scene graph parts outside the
  camera view volume and does not traverse those.) Like most other
  action classes, SoGetPrimitiveCountAction actually traverses the
  complete scene graph, not just the parts currently in view.

  \since Coin 1.0
  \since TGS Inventor 2.5
*/

#include <Inventor/actions/SoGetPrimitiveCountAction.h>

#include <Inventor/SbName.h>
#include <Inventor/lists/SoEnabledElementsList.h>
#include <Inventor/nodes/SoNode.h>
#include <Inventor/elements/SoDecimationPercentageElement.h>
#include <Inventor/elements/SoDecimationTypeElement.h>
#include <Inventor/elements/SoViewportRegionElement.h>

#include "actions/SoSubActionP.h"

class SoGetPrimitiveCountActionP {
public:
  SbViewportRegion viewport;
};

SO_ACTION_SOURCE(SoGetPrimitiveCountAction);


/*!
  \copydetails SoAction::initClass(void)
*/
void
SoGetPrimitiveCountAction::initClass(void)
{
  SO_ACTION_INTERNAL_INIT_CLASS(SoGetPrimitiveCountAction, SoAction);

  SO_ENABLE(SoGetPrimitiveCountAction, SoDecimationPercentageElement);
  SO_ENABLE(SoGetPrimitiveCountAction, SoDecimationTypeElement);
  SO_ENABLE(SoGetPrimitiveCountAction, SoViewportRegionElement);
}

/*!
  Constructor.
*/
SoGetPrimitiveCountAction::SoGetPrimitiveCountAction()
{
  this->commonConstructor(SbViewportRegion(640, 512));
}

/*!
  Constructor. Supply the current viewport region to make SCREEN_SPACE counting correct.
*/
SoGetPrimitiveCountAction::SoGetPrimitiveCountAction(const SbViewportRegion & vp)
{
  this->commonConstructor(vp);
}

void
SoGetPrimitiveCountAction::commonConstructor(const SbViewportRegion & vp) 
{
  SO_ACTION_CONSTRUCTOR(SoGetPrimitiveCountAction);

  this->textastris = TRUE;
  this->approx = FALSE;
  this->nonvertexastris = TRUE;
  this->pimpl->viewport = vp;
}

/*!
  The destructor.
*/
SoGetPrimitiveCountAction::~SoGetPrimitiveCountAction()
{
}

/*!
  Returns number of triangles in graph.
*/
int
SoGetPrimitiveCountAction::getTriangleCount(void) const
{
  return this->numtris;
}

/*!
  Returns number of lines in graph.
*/
int
SoGetPrimitiveCountAction::getLineCount(void) const
{
  return this->numlines;
}

/*!
  Returns number of points in graph.

  Note that by "point", it is meant an actual point primitive (for
  rendering), such as in the SoPointSet shape node, not a polygon
  vertex. For counting the total number of polygon vertices in a
  scene (sub) graph, use instead the SoCallbackAction with the
  appropriate callback.
*/
int
SoGetPrimitiveCountAction::getPointCount(void) const
{
  return this->numpoints;
}

/*!
  Returns number of texts in the graph.
*/
int
SoGetPrimitiveCountAction::getTextCount(void) const
{
  return this->numtexts;
}

/*!
  Returns the number of images in the graph.
*/
int
SoGetPrimitiveCountAction::getImageCount(void) const
{
  return this->numimages;
}

/*!
  Returns whether there are any primitives in graph or not.
*/
SbBool
SoGetPrimitiveCountAction::containsNoPrimitives(void)
{
  return
    this->numtris == 0 &&
    this->numlines == 0 &&
    this->numpoints == 0 &&
    this->numtexts == 0 &&
    this->numimages == 0;
}

/*!
  Returns whether there are non-triangular primitives in graph.
*/
SbBool
SoGetPrimitiveCountAction::containsNonTriangleShapes(void)
{
  return
    this->numlines != 0 ||
    this->numpoints != 0 ||
    this->numtexts != 0 ||
    this->numimages != 0;
}

/*!
  Sets whether SoText3 nodes are counted as the triangles of the
  fonts in the text strings or the text itself. The default is to
  count as triangles.

  \sa is3DTextCountedAsTriangles()
*/
void
SoGetPrimitiveCountAction::setCount3DTextAsTriangles(const SbBool flag)
{
  this->textastris = flag;
}

/*!
  Returns whether SoText3 nodes is counted as triangles or text.

  \sa is3DTextCountedAsTriangles()
*/
SbBool
SoGetPrimitiveCountAction::is3DTextCountedAsTriangles(void)
{
  return this->textastris;
}

/*!
  Returns whether shapes can use an approximate value when counting
  primitives. This is faster than doing an accurate count.  The
  default is to not approximate.

  \sa setCanApproximate()
*/
SbBool
SoGetPrimitiveCountAction::canApproximateCount(void)
{
  return this->approx;
}

/*!
  Sets whether shapes can do an approximate count.
  \sa canApproximateCount()
*/
void
SoGetPrimitiveCountAction::setCanApproximate(const SbBool flag)
{
  this->approx = flag;
}

/*!
  Set up the decimation parameters for the traversal.

  On-the-fly decimation is supported in Coin yet, so this call will
  not have any effect until this feature has been implemented.
*/
void
SoGetPrimitiveCountAction::setDecimationValue(SoDecimationTypeElement::Type type,
                                              float percentage)
{
  this->decimationtype = type;
  this->decimationpercentage = percentage;
}

/*!
  Returns decimation type used during the traversal count.
 */
SoDecimationTypeElement::Type
SoGetPrimitiveCountAction::getDecimationType(void)
{
  return this->decimationtype;
}

/*!
  Returns decimation percentage used during the traversal count.
 */
float
SoGetPrimitiveCountAction::getDecimationPercentage(void)
{
  return this->decimationpercentage;
}

/*!
  Adds \a num triangles to total count. Used by node instances in the
  scene graph during traversal.
*/
void
SoGetPrimitiveCountAction::addNumTriangles(const int num)
{
  this->numtris += num;
}

/*!
  Adds \a num lines to total count. Used by node instances in the
  scene graph during traversal.
*/
void
SoGetPrimitiveCountAction::addNumLines(const int num)
{
  this->numlines += num;
}

/*!
  Adds \a num points to total count. Used by node instances in the
  scene graph during traversal.
*/
void
SoGetPrimitiveCountAction::addNumPoints(const int num)
{
  this->numpoints += num;
}

/*!
  Adds \a num texts to total count. Used by node instances in the
  scene graph during traversal.
*/
void
SoGetPrimitiveCountAction::addNumText(const int num)
{
  this->numtexts += num;
}

/*!
  Adds \a num texture image maps to total count. Used by node
  instances in the scene graph during traversal.
*/
void
SoGetPrimitiveCountAction::addNumImage(const int num)
{
  this->numimages += num;
}

/*!
  Adds a single triangle to the total count. Used by node instances in
  the scene graph during traversal.
*/
void
SoGetPrimitiveCountAction::incNumTriangles(void)
{
  this->numtris++;
}

/*!
  Adds a single line to the total count. Used by node instances in the
  scene graph during traversal.
*/
void
SoGetPrimitiveCountAction::incNumLines(void)
{
  this->numlines++;
}

/*!
  Adds a single point to the total count. Used by node instances in
  the scene graph during traversal.
*/
void
SoGetPrimitiveCountAction::incNumPoints(void)
{
  this->numpoints++;
}

/*!
  Adds a single text to the total count. Used by node instances in the
  scene graph during traversal.
*/
void
SoGetPrimitiveCountAction::incNumText(void)
{
  this->numtexts++;
}

/*!
  Adds a single texture image map to the total count. Used by node
  instances in the scene graph during traversal.
*/
void
SoGetPrimitiveCountAction::incNumImage(void)
{
  this->numimages++;
}


// Documented in superclass. Overridden to reset all counters to zero
// before traversal starts.
void
SoGetPrimitiveCountAction::beginTraversal(SoNode * node)
{
  this->numtris = 0;
  this->numlines = 0;
  this->numpoints = 0;
  this->numtexts = 0;
  this->numimages = 0;

  SoViewportRegionElement::set(state, this->pimpl->viewport);

//  SoDecimationTypeElement::set(this->getState(), this->decimationtype);
//  SoDecimationPercentageElement::set(this->getState(), this->decimationpercentage);

  this->traverse(node);
}
