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
  \class SmSwitchboard SmSwitchboard.h SmallChange/nodes/SmSwitchboard.h
  \brief The SmSwitchboard class is a group node that can toggle children
  on and off arbitrarily.

  FIXME: write doc

  \ingroup nodes
*/

// FIXME: implement proper searching / SearchAction handling  2002-02-07 larsa
// FIXME: implement proper writing / WriteAction handling  2002-02-07 larsa

#include "PreCompiled.h"
#include "SmSwitchboard.h"
#include <Inventor/nodes/SoSubNode.h>
#include <Inventor/misc/SoChildList.h>

#include <Inventor/actions/SoGetBoundingBoxAction.h>
#include <Inventor/actions/SoSearchAction.h>
#include <Inventor/actions/SoGetMatrixAction.h>
#include <Inventor/actions/SoGLRenderAction.h>
#include <Inventor/actions/SoPickAction.h>
#include <Inventor/actions/SoHandleEventAction.h>
#include <Inventor/actions/SoCallbackAction.h>
#include <Inventor/actions/SoGetPrimitiveCountAction.h>
#include <Inventor/actions/SoWriteAction.h>
#include <Inventor/elements/SoCacheElement.h>
#include <Inventor/SoOutput.h>

#include <Inventor/errors/SoDebugError.h>

/*!
  \var SoMFBool SmSwitchboard::enable

  Selects which child to traverse during rendering (and some other)
  actions.

  When the length of this multifield is larger than the number of children
  this group has, the enable list is modulated over the children.  This lets
  you have full control over the number of times and order each child is
  traversed.

  Default enabled value is \c FALSE.
*/

// *************************************************************************

// doc in super
void
SmSwitchboard::initClass(void)
{
  SO_NODE_INIT_CLASS(SmSwitchboard, SoGroup, SoGroup);
}

SO_NODE_SOURCE(SmSwitchboard);

/*!
  Default constructor.
*/
SmSwitchboard::SmSwitchboard(void)
{
  SO_NODE_CONSTRUCTOR(SmSwitchboard);

  SO_NODE_ADD_FIELD(enable, (FALSE));
}

/*!
  Constructor.

  The argument should be the approximate number of children which is
  expected to be inserted below this node. The number need not be
  exact, as it is only used as a hint for better memory resource
  allocation.
*/
SmSwitchboard::SmSwitchboard(int numchildren)
  : inherited(numchildren)
{
  SO_NODE_CONSTRUCTOR(SmSwitchboard);

  SO_NODE_ADD_FIELD(enable, (FALSE));
}

/*!
  Destructor.
*/
SmSwitchboard::~SmSwitchboard(void) // virtual, protected
{
}

// Documented in superclass.
void
SmSwitchboard::doAction(SoAction * action)
{
    int numindices;
    const int * indices;
    if (action->getPathCode(numindices, indices) == SoAction::IN_PATH) {
        // FIXME: We ignore the IN_PATH optimization
        for ( int i = 0; i < this->getChildren()->getLength(); i++ ) {
            if ( this->enable[i % this->enable.getNum()] )
                this->getChildren()->traverse(action,i);
        }

        //this->getChildren()->traverse(action); // traverse all children
        //this->getChildren()->traverseInPath(action, newnumindices, newindices);
    }
    else {

        for ( int i = 0; i < this->getChildren()->getLength(); i++ ) {
            if ( this->enable[i % this->enable.getNum()] )
                this->getChildren()->traverse(action,i);
        }

        //this->getChildren()->traverse(action); // traverse all children
    }
}

void
SmSwitchboard::GLRender(SoGLRenderAction * action)
{
    int numindices;
    const int * indices;
    SoAction::PathCode pathcode = action->getPathCode(numindices, indices);
    
    SoNode ** childarray = (SoNode**) this->getChildren()->getArrayPtr();
    SoState * state = action->getState();
    
    if (pathcode == SoAction::IN_PATH) {
        int lastchild = indices[numindices - 1];
        for (int i = 0; i <= lastchild && !action->hasTerminated(); i++) {
            SoNode * child = childarray[i];
            
            action->pushCurPath(i, child);
            if (action->getCurPathCode() != SoAction::OFF_PATH ||
                child->affectsState()) {
                if (!action->abortNow()) {
                    if ( this->enable[i % this->enable.getNum()] ) {
                        child->GLRender(action);
                    }
                }
                else {
                    SoCacheElement::invalidate(state);
                }
                }
                action->popCurPath(pathcode);
        }
    }
    else {
        action->pushCurPath();
        int n = this->getChildren()->getLength();
        for (int i = 0; i < n && !action->hasTerminated(); i++) {
            action->popPushCurPath(i, childarray[i]);
            
            if (pathcode == SoAction::OFF_PATH && !childarray[i]->affectsState()) {
                continue;
            }
            
            if (action->abortNow()) {
                // only cache if we do a full traversal
                SoCacheElement::invalidate(state);
                break;
            }

            if ( this->enable[i % this->enable.getNum()] )
                childarray[i]->GLRender(action);
            
            #if COIN_DEBUG
            // The GL error test is default disabled for this optimized
            // path.  If you get a GL error reporting an error in the
            // Separator node, enable this code by setting the environment
            // variable COIN_GLERROR_DEBUGGING to "1" to see exactly which
            // node caused the error.
            static SbBool chkglerr = sogl_glerror_debugging();
            if (chkglerr) {
                cc_string str;
                cc_string_construct(&str);
                const unsigned int errs = coin_catch_gl_errors(&str);
                if (errs > 0) {
                    SoDebugError::post("SoGroup::GLRender",
                                       "glGetError()s => '%s', nodetype: '%s'",
                                       cc_string_get_text(&str),
                                       (*this->getChildren())[i]->getTypeId().getName().getString());
                }
                cc_string_clean(&str);
            }
            #endif // COIN_DEBUG
            
        }
        action->popCurPath();
    }
}

void
SmSwitchboard::getBoundingBox(SoGetBoundingBoxAction * action)
{
    // Sanity check. This has caught bugs.
    assert(this->getNumChildren() == this->getChildren()->getLength());
    
    int numindices;
    const int * indices;
    int lastchildindex;
    
    if (action->getPathCode(numindices, indices) == SoAction::IN_PATH)
        lastchildindex = indices[numindices-1];
    else
        lastchildindex = this->getNumChildren() - 1;
    
    assert(lastchildindex < this->getNumChildren());
    
    // Initialize accumulation variables.
    SbVec3f acccenter(0.0f, 0.0f, 0.0f);
    int numcenters = 0;
    
    for (int i = 0; i <= lastchildindex; i++) {
        
        if ( this->enable[i % this->enable.getNum()] )
            this->getChildren()->traverse(action, i);
        
        // If center point is set, accumulate.
        if (action->isCenterSet()) {
            acccenter += action->getCenter();
            numcenters++;
            action->resetCenter();
        }
    }
    
    if (numcenters != 0)
        action->setCenter(acccenter / float(numcenters), FALSE);
}

void
SmSwitchboard::getMatrix(SoGetMatrixAction * action)
{
    switch (action->getCurPathCode()) {
        case SoAction::NO_PATH:
        case SoAction::BELOW_PATH:
            break;
        case SoAction::OFF_PATH:
        case SoAction::IN_PATH:
            SmSwitchboard::doAction((SoAction *)action);
            break;
    }
}

void
SmSwitchboard::callback(SoCallbackAction *action)
{
    SmSwitchboard::doAction(action);
}

// Documented in superclass.
void
SmSwitchboard::pick(SoPickAction *action)
{
    SmSwitchboard::doAction(action);
}

// Documented in superclass.
void
SmSwitchboard::handleEvent(SoHandleEventAction *action)
{
    SmSwitchboard::doAction(action);
}

void
SmSwitchboard::search(SoSearchAction * action)
{
    // Include this node in the search.
    inherited::search(action);
    if (action->isFound()) return;
    
    // If we're not the one being sought after, try child subgraphs.
    SmSwitchboard::doAction((SoAction *)action);
}
