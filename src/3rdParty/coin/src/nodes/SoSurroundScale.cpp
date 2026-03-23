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
  \class SoSurroundScale SoSurroundScale.h Inventor/nodes/SoSurroundScale.h
  \brief The SoSurroundScale class is used to automatically scale geometry to surround other geometry.

  \ingroup coin_nodes

  This node calculates a transformation (a translation and a scale)
  which will, when the node is traversed, be appended to the current
  model matrix, making a default cube placed directly to the right of
  this node in the graph surround geometry to the right of the
  container branch this node is on. The container is specified by the
  field SoSurroundScale::numNodesUpToContainer.

  When calculating the bounding box to be surrounded, the bounding box
  action will be applied to the container node, and the bounding box
  calculations will be reset after traversing the node specified by
  the field SoSurroundScale::numNodesUpToReset.

  A way of thinking of the container node is that there exists a path
  from the root-node to the SoSurroundScale node.  This path has a
  number of nodes in it.  numNodesUpToContainer is the number of nodes
  in the path from the SoSurroundScale node towards the
  root node. When having counted numNodesUpToContainer from the
  SoSurroundScale node, you will have reached the container node. The
  container node is the seed-node that the bounding box is calculated
  from. All nodes lower than the seed-node will be a part of the
  bounding box unless they are closer to the SoSurroundScale node than
  numNodesUpToReset.

  To make sure the node(s) that are to be scaled by the
  SoSurroundScale node aren't a part of the bounding box, the
  numNodesUpToReset is used. It is also the number of nodes up from
  the SoSurroundScale node towards the root node.  The meaning of
  numNodesUpToReset is that nodes that are closer to the
  SoSurroundNode than the node at position numNodesUpToReset are not
  to be used for bounding box calculations.

  Here's a complete, standalone example which sets up an
  SoTrackballDragger (connected to a cube), and uses an
  SoSurroundScale node to make it auto-scale to the size of the high
  cube and translates it an offset to one side of the cube:

  \code
  #include <Inventor/Qt/SoQt.h>
  #include <Inventor/Qt/viewers/SoQtExaminerViewer.h>
  #include <Inventor/draggers/SoTrackballDragger.h>
  #include <Inventor/nodes/SoAntiSquish.h>
  #include <Inventor/nodes/SoSeparator.h>
  #include <Inventor/nodes/SoCube.h>
  #include <Inventor/nodes/SoSurroundScale.h>
  #include <Inventor/nodes/SoRotation.h>
  #include <Inventor/nodes/SoTranslation.h>


  int
  main(int argc, char **argv)
  {
    QWidget * window = SoQt::init(argv[0]);
    if (window == NULL) exit(1);

    SoSeparator * root = new SoSeparator;
    root->ref();

    SoSeparator * surroundsep = new SoSeparator;
    root->addChild(surroundsep);

    SoTranslation * translation = new SoTranslation;
    translation->translation = SbVec3f(12, 0, 0);
    surroundsep->addChild(translation);

    SoSurroundScale * ss = new SoSurroundScale;
    ss->numNodesUpToReset = 1;
    ss->numNodesUpToContainer = 2;
    surroundsep->addChild(ss);

    SoAntiSquish * antisquish = new SoAntiSquish;
    antisquish->sizing = SoAntiSquish::BIGGEST_DIMENSION;
    surroundsep->addChild(antisquish);

    SoTrackballDragger * dragger = new SoTrackballDragger;
    surroundsep->addChild(dragger);

    SoRotation * rotation = new SoRotation;
    rotation->rotation.connectFrom(& dragger->rotation);
    root->addChild(rotation);

    SoCube * cube = new SoCube;
    cube->height = 10.0f;
    root->addChild(cube);

    SoQtExaminerViewer * viewer = new SoQtExaminerViewer(window);
    viewer->setSceneGraph(root);
    viewer->setViewing(FALSE);
    viewer->setDecoration(FALSE);
    viewer->show();

    SoQt::show(window);
    SoQt::mainLoop();

    delete viewer;
    root->unref();

    return 0;
  }
  \endcode

  It might be easier to see how the SoSurroundScale node works in the
  above example by looking at the actual scene graph:

  \code
  #Inventor V2.1 ascii

  Separator {
     Separator {
        Translation {
           translation 12 0 0
        }
        SurroundScale {
           numNodesUpToContainer 2
           numNodesUpToReset 1
        }
        AntiSquish {
           sizing BIGGEST_DIMENSION
        }
        DEF mydragger TrackballDragger {
        }
     }
     Rotation {
        rotation 0 0 1  0 =
        USE mydragger . rotation
     }
     Cube {
        height 10
     }
  }
  \endcode

  This node is internally used by manipulators to make it possible for
  them to scale their dragger's geometry to match the scene graph
  geometry it is modifying, as is demonstrated above (but outside of
  the context of a manipulator).

  It is also generally useful for application programmers who want any
  particular piece of geometry surround other geometry of unknown or
  changing extent.

  SoSurroundScale nodes in the scene graph are often paired up with
  SoAntiSquish nodes to get uniform scaling along all three principal
  axes, as has also been done in the above example.

  <b>FILE FORMAT/DEFAULTS:</b>
  \code
    SurroundScale {
        numNodesUpToContainer 0
        numNodesUpToReset 0
    }
  \endcode

  \sa SoAntiSquish
*/

// *************************************************************************

#include <Inventor/nodes/SoSurroundScale.h>

#include <Inventor/actions/SoGetMatrixAction.h>
#include <Inventor/actions/SoGetBoundingBoxAction.h>
#include <Inventor/elements/SoModelMatrixElement.h>
#include <Inventor/elements/SoViewportRegionElement.h>
#include <Inventor/errors/SoDebugError.h>

#include "nodes/SoSubNodeP.h"

// *************************************************************************

/*!
  \var SoSFInt32 SoSurroundScale::numNodesUpToContainer

  Number of nodes in the path counting from this and "upwards" to the
  container node.
*/
/*!
  \var SoSFInt32 SoSurroundScale::numNodesUpToReset

  Number of nodes in the path counting from this and "upwards" to the
  node where we will reset the bounding box value.
*/

/*!
  \var SoSurroundScale::cachedScale
  \COININTERNAL
*/
/*!
  \var SoSurroundScale::cachedInvScale
  \COININTERNAL
*/
/*!
  \var SoSurroundScale::cachedTranslation
  \COININTERNAL
*/
/*!
  \var SoSurroundScale::cacheOK
  \COININTERNAL
*/
/*!
  \var SoSurroundScale::doTranslations
  \COININTERNAL
*/


// *************************************************************************

SO_NODE_SOURCE(SoSurroundScale);

/*!
  Constructor.
*/
SoSurroundScale::SoSurroundScale(void)
{
  SO_NODE_INTERNAL_CONSTRUCTOR(SoSurroundScale);

  SO_NODE_ADD_FIELD(numNodesUpToContainer, (0));
  SO_NODE_ADD_FIELD(numNodesUpToReset, (0));

  this->cacheOK = FALSE;
  this->ignoreinbbox = FALSE;
  this->doTranslations = TRUE;
}

/*!
  Destructor.
*/
SoSurroundScale::~SoSurroundScale()
{
}

// Doc in superclass.
/*!
  \copybrief SoBase::initClass(void)
*/
void
SoSurroundScale::initClass(void)
{
  SO_NODE_INTERNAL_INIT_CLASS(SoSurroundScale, SO_FROM_INVENTOR_1);
}


/*!
  Invalidates the cached transformation, forcing a recalculation to be
  done the next time this node is traversed.
*/
void
SoSurroundScale::invalidate(void)
{
  this->cacheOK = FALSE;
}

// Doc in superclass.
void
SoSurroundScale::doAction(SoAction * action)
{
  SoState * state = action->getState();
  if (!this->cacheOK) {
    SbMatrix dummy;
    this->updateMySurroundParams(action, dummy);
  }
  if (this->doTranslations &&
      this->cachedTranslation != SbVec3f(0.0f, 0.0f, 0.0f)) {
    SoModelMatrixElement::translateBy(state, this, this->cachedTranslation);
  }
  if (this->cachedScale != SbVec3f(1.0f, 1.0f, 1.0f))
    SoModelMatrixElement::scaleBy(state, this, this->cachedScale);
}

/*!
  Sets whether the translation part of the transformation should be
  ignored or not. Default behavior is to translate.
*/
void
SoSurroundScale::setDoingTranslations(const SbBool val)
{
  this->doTranslations = val;
}

/*!
  Returns whether the translation part of the transformation should be
  ignored or not.

  \sa setDoingTranslations()
*/
SbBool
SoSurroundScale::isDoingTranslations(void)
{
  return this->doTranslations;
}

// Doc in superclass.
void
SoSurroundScale::callback(SoCallbackAction * action)
{
  SoSurroundScale::doAction((SoAction *)action);
}

// Doc in superclass.
void
SoSurroundScale::GLRender(SoGLRenderAction * action)
{
  SoSurroundScale::doAction((SoAction *)action);
}

// Doc in superclass.
void
SoSurroundScale::getBoundingBox(SoGetBoundingBoxAction * action)
{
  if (!this->isIgnoreInBbox())
    SoSurroundScale::doAction((SoAction *)action);
}

// Doc in superclass.
void
SoSurroundScale::getMatrix(SoGetMatrixAction * action)
{
  if (!this->cacheOK) {
    this->updateMySurroundParams(action, action->getInverse());
  }

  if (this->doTranslations &&
      this->cachedTranslation != SbVec3f(0.0f, 0.0f, 0.0f)) {
    SbMatrix m;
    m.setTranslate(this->cachedTranslation);
    action->getMatrix().multLeft(m);
    m.setTranslate(- this->cachedTranslation);
    action->getInverse().multRight(m);
  }

  if (this->cachedScale != SbVec3f(1.0f, 1.0f, 1.0f)) {
    SbMatrix m;
    m.setScale(this->cachedScale);
    action->getMatrix().multLeft(m);
    m.setScale(SbVec3f(1.0f / this->cachedScale[0],
                       1.0f / this->cachedScale[1],
                       1.0f / this->cachedScale[2]));
    action->getInverse().multRight(m);
  }
}

// Doc in superclass.
void
SoSurroundScale::pick(SoPickAction * action)
{
  SoSurroundScale::doAction((SoAction *)action);
}

/*!
  Calculates the translation and scale needed to make a default cube
  surround geometry to the right of the branch this node is on.
*/
void
SoSurroundScale::updateMySurroundParams(SoAction * action,
                                        const SbMatrix & /*inv*/)
{
  // I haven't found any use for the inv argument. The function
  // should be kept as is to make this node OIV compatible though.
  // pederb, 20000220
  int numtocontainer = this->numNodesUpToContainer.getValue();
  int numtoreset = this->numNodesUpToReset.getValue();
  const SoFullPath * curpath = (const SoFullPath *) action->getCurPath();
  const int curpathlen = curpath->getLength();

  if ((numtocontainer <= 0) || (numtocontainer >= curpathlen)) {
#if COIN_DEBUG
    SoDebugError::postWarning("SoSurroundScale::updateMySurroundParams",
                              "illegal field value, numNodesUpToContainer (==%d)"
                              "should always be > 0 and < path length\n",
                              numtocontainer);
#endif // debug
    this->cachedScale.setValue(1.0f, 1.0f, 1.0f);
    this->cachedInvScale.setValue(1.0f, 1.0f, 1.0f);
    this->cachedTranslation.setValue(0.0f, 0.0f, 0.0f);
    this->cacheOK = FALSE;
    return;
  }

  // make sure we don't get here when calculating the bbox
  SbBool storedignore = this->isIgnoreInBbox();
  this->setIgnoreInBbox(TRUE);

  SoPath * applypath = curpath->copy(0, curpathlen - numtocontainer);
  applypath->ref();

  SoPath * resetpath = NULL;
  // if numtoreset is out of range, just ignore it and don't use a
  // reset path
  if ((numtoreset >= 0) && (numtoreset < numtocontainer)) {
    resetpath = curpath->copy(0, curpathlen - numtoreset);
    resetpath->ref();
  }

  SbViewportRegion vp(100, 100);
  // need to test if SoViewportRegionElement is enabled since this
  // element is not enabled for SoAudioRenderAction.

  if (action->getState()->isElementEnabled(SoViewportRegionElement::getClassStackIndex())) {
    vp = SoViewportRegionElement::get(action->getState());
  }

  SoGetBoundingBoxAction bboxaction(vp);

  // reset bbox when returning from surroundscale branch,
  // meaning we'll calculate the bbox of only the geometry
  // to the right of this branch, getting the wanted result.
  if (resetpath) {
    bboxaction.setResetPath(resetpath, FALSE, SoGetBoundingBoxAction::ALL);
  }
  bboxaction.apply(applypath);
  applypath->unref();
  if (resetpath) {
    resetpath->unref();
  }

  SbBox3f box = bboxaction.getBoundingBox();
  if (box.isEmpty()) {
    this->cachedScale.setValue(1.0f, 1.0f, 1.0f);
    this->cachedInvScale.setValue(1.0f, 1.0f, 1.0f);
    this->cachedTranslation.setValue(0.0f, 0.0f, 0.0f);
  }
  else {
    box.getSize(this->cachedScale[0], this->cachedScale[1],
                this->cachedScale[2]);

    if (this->cachedScale[0] <= 0.0f ||
        this->cachedScale[1] <= 0.0f ||
        this->cachedScale[2] <= 0.0f) {

      // find the smallest scale not zero
      SbVec3f s = this->cachedScale;
      float min = SbMax(SbMax(s[0], s[1]), s[2]);
      if (s[0] > 0.0f && s[0] < min) min = s[0];
      if (s[1] > 0.0f && s[1] < min) min = s[1];
      if (s[2] > 0.0f && s[2] < min) min = s[2];

      min *= 0.05f; // set empty dimensions to some value
      if (min <= 0.0f) min = 1.0f;
      if (s[0] <= 0.0f) this->cachedScale[0] = min;
      if (s[1] <= 0.0f) this->cachedScale[1] = min;
      if (s[2] <= 0.0f) this->cachedScale[2] = min;
    }

    this->cachedScale *= 0.5f;
    this->cachedInvScale[0] = 1.0f / this->cachedScale[0];
    this->cachedInvScale[1] = 1.0f / this->cachedScale[1];
    this->cachedInvScale[2] = 1.0f / this->cachedScale[2];

    this->cachedTranslation = box.getCenter();
  }

  this->setIgnoreInBbox(storedignore);
  this->cacheOK = TRUE;
}

/*!
  Sets whether bounding box calculations in SoGetBoundingBoxAction
  should be affected by this node.

  Default is to ignore our bounding box calculations.
*/
void
SoSurroundScale::setIgnoreInBbox(const SbBool val)
{
  this->ignoreinbbox = val;
}

/*!
  Returns the value of the flag that decides whether bounding box
  calculations in SoGetBoundingBoxAction should be affected by this
  node.

  \sa setIgnoreInBbox()
*/
SbBool
SoSurroundScale::isIgnoreInBbox(void)
{
  return this->ignoreinbbox;
}
