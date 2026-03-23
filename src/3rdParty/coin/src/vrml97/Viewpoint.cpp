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
  \class SoVRMLViewpoint SoVRMLViewpoint.h Inventor/VRMLnodes/SoVRMLViewpoint.h
  \brief The SoVRMLViewpoint class is a perspective camera class.

  \ingroup coin_VRMLnodes

  \WEB3DCOPYRIGHT

  \verbatim
  Viewpoint {
    eventIn      SFBool     set_bind
    exposedField SFFloat    fieldOfView    0.785398  # (0,inf)
    exposedField SFBool     jump           TRUE
    exposedField SFRotation orientation    0 0 1 0   # [-1,1],(-inf,inf)
    exposedField SFVec3f    position       0 0 10    # (-inf,inf)
    field        SFString   description    ""
    eventOut     SFTime     bindTime
    eventOut     SFBool     isBound
  }
  \endverbatim

  The Viewpoint node defines a specific location in the local
  coordinate system from which the user may view the scene. Viewpoint
  nodes are bindable children nodes (see 4.6.10, Bindable children
  nodes:
  http://www.web3d.org/documents/specifications/14772/V2.0/part1/concepts.html#4.6.10)
  and thus there exists a Viewpoint node stack in the browser in which
  the top-most Viewpoint node on the stack is the currently active
  Viewpoint node.  If a TRUE value is sent to the set_bind eventIn of
  a Viewpoint node, it is moved to the top of the Viewpoint node stack
  and activated. When a Viewpoint node is at the top of the stack, the
  user's view is conceptually re-parented as a child of the Viewpoint
  node. All subsequent changes to the Viewpoint node's coordinate
  system change the user's view (e.g., changes to any ancestor
  transformation nodes or to the Viewpoint node's position or
  orientation fields). Sending a set_bind FALSE event removes the
  Viewpoint node from the stack and produces isBound FALSE and
  bindTime events. If the popped Viewpoint node is at the top of the
  viewpoint stack, the user's view is re-parented to the next entry in
  the stack. More details on binding stacks can be found in 4.6.10,
  Bindable children nodes
  (<http://www.web3d.org/documents/specifications/14772/V2.0/part1/concepts.html#4.6.10>).
  When a Viewpoint node is moved to the top
  of the stack, the existing top of stack Viewpoint node sends an
  isBound FALSE event and is pushed down the stack.

  An author can automatically move the user's view through the world
  by binding the user to a Viewpoint node and then animating either
  the Viewpoint node or the transformations above it. Browsers shall
  allow the user view to be navigated relative to the coordinate
  system defined by the Viewpoint node (and the transformations above
  it) even if the Viewpoint node or its ancestors' transformations are
  being animated.  

  The \e bindTime eventOut sends the time at which the Viewpoint node is
  bound or unbound. This can happen:

  - during loading;
  - when a set_bind event is sent to the Viewpoint node;
  - when the browser binds to the Viewpoint node through its user interface
    described below.

  The position and orientation fields of the Viewpoint node specify
  relative locations in the local coordinate system. Position is
  relative to the coordinate system's origin (0,0,0), while
  orientation specifies a rotation relative to the default
  orientation. In the default position and orientation, the viewer is
  on the Z-axis looking down the -Z-axis toward the origin with +X to
  the right and +Y straight up. Viewpoint nodes are affected by the
  transformation hierarchy.

  Navigation types (see SoVRMLNavigationInfo) that require a
  definition of a down vector (e.g., terrain following) shall use the
  negative Y-axis of the coordinate system of the currently bound
  Viewpoint node. Likewise, navigation types that require a definition
  of an up vector shall use the positive Y-axis of the coordinate
  system of the currently bound Viewpoint node. The orientation field
  of the Viewpoint node does not affect the definition of the down or
  up vectors.  This allows the author to separate the viewing
  direction from the gravity direction.

  The jump field specifies whether the user's view "jumps" to the
  position and orientation of a bound Viewpoint node or remains
  unchanged. This jump is instantaneous and discontinuous in that no
  collisions are performed and no ProximitySensor nodes are checked in
  between the starting and ending jump points. If the user's position
  before the jump is inside a ProximitySensor the exitTime of that
  sensor shall send the same timestamp as the bind eventIn. Similarly,
  if the user's position after the jump is inside a ProximitySensor
  the enterTime of that sensor shall send the same timestamp as the
  bind eventIn. Regardless of the value of jump at bind time, the
  relative viewing transformation between the user's view and the
  current Viewpoint node shall be stored with the current Viewpoint
  node for later use when un-jumping (i.e., popping the Viewpoint node
  binding stack from a Viewpoint node with jump TRUE). The following
  summarizes the bind stack rules (see 4.6.10, Bindable children
  nodes:
  http://www.web3d.org/documents/specifications/14772/V2.0/part1/concepts.html#4.6.10)
  with additional rules regarding Viewpoint nodes (displayed in
  boldface type):

  - During read, the first encountered Viewpoint node is bound by
    pushing it to the top of the Viewpoint node stack. If a Viewpoint
    node name is specified in the URL that is being read, this named
    Viewpoint node is considered to be the first encountered Viewpoint
    node.  Nodes contained within SoVRMLInline nodes, within the
    strings passed to the Browser.createVrmlFromString() method, or
    within files passed to the Browser.createVrmlFromURL() method (see
    4.12.10, Browser script interface:
    http://www.web3d.org/documents/specifications/14772/V2.0/part1/concepts.html#4.12.10)
    are not candidates for the first encountered Viewpoint node. The
    first node within a prototype instance is a valid candidate for
    the first encountered Viewpoint node. The first encountered
    Viewpoint node sends an isBound TRUE event.
 
  - When a set_bind TRUE event is received by a Viewpoint node,

    - If it is not on the top of the stack: The relative
      transformation from the current top of stack Viewpoint node to
      the user's view is stored with the current top of stack
      Viewpoint node. The current top of stack node sends an isBound
      FALSE event. The new node is moved to the top of the stack and
      becomes the currently bound Viewpoint node.  The new Viewpoint
      node (top of stack) sends an isBound TRUE event.  If jump is
      TRUE for the new Viewpoint node, the user's view is
      instantaneously "jumped" to match the values in the position and
      orientation fields of the new Viewpoint node.

    - If the node is already at the top of the stack, this event has no affect.

  - When a set_bind FALSE event is received by a Viewpoint node in the
    stack, it is removed from the stack. If it was on the top of the
    stack,

    - it sends an isBound FALSE event, 
    
    - the next node in the stack becomes the currently bound Viewpoint
      node (i.e., pop) and issues an isBound TRUE event,
   
    - if its jump field value is TRUE, the user's view is
      instantaneously "jumped" to the position and orientation of the
      next Viewpoint node in the stack with the stored relative
      transformation of this next Viewpoint node applied.  

  - If a set_bind FALSE event is received by a node not in the stack,
    the event is ignored and isBound events are not sent.
 
  - When a node replaces another node at the top of the stack, the
    isBound TRUE and FALSE events from the two nodes are sent
    simultaneously (i.e., with identical timestamps).
 
  - If a bound node is deleted, it behaves as if it received a
    set_bind FALSE event.

  The jump field may change after a Viewpoint node is bound. The rules
  described above still apply. If jump was TRUE when the Viewpoint
  node is bound, but changed to FALSE before the set_bind FALSE is
  sent, the Viewpoint node does not un-jump during unbind. If jump was
  FALSE when the Viewpoint node is bound, but changed to TRUE before
  the set_bind FALSE is sent, the Viewpoint node does perform the
  un-jump during unbind.

  Note that there are two other mechanisms that result in the binding
  of a new Viewpoint:

  - An Anchor node's url field specifies a "#ViewpointName".
 
  - A script invokes the loadURL() method and the URL argument
    specifies a "#ViewpointName".

  Both of these mechanisms override the jump field value of the
  specified Viewpoint node ("#ViewpointName") and assume that jump is
  TRUE when binding to the new Viewpoint. The behaviour of the viewer
  transition to the newly bound Viewpoint depends on the currently
  bound NavigationInfo node's type field value (see SoVRMLNavigationInfo).

  The \e fieldOfView field specifies a preferred minimum viewing angle
  from this viewpoint in radians. A small field of view roughly
  corresponds to a telephoto lens; a large field of view roughly
  corresponds to a wide-angle lens. The field of view shall be greater
  than zero and smaller than . The value of fieldOfView represents the
  minimum viewing angle in any direction axis perpendicular to the
  view. For example, a browser with a rectangular viewing projection
  shall have the following relationship:

  \verbatim
      display width    tan(FOVhorizontal/2)
      -------------- = -----------------
      display height   tan(FOVvertical/2)
  \endverbatim

  where the smaller of display width or display height determines
  which angle equals the fieldOfView (the larger angle is computed
  using the relationship described above). The larger angle shall not
  exceed and may force the smaller angle to be less than fieldOfView
  in order to sustain the aspect ratio.

  The description field specifies a textual description of the
  Viewpoint node. This may be used by browser-specific user
  interfaces. If a Viewpoint's description field is empty it is
  recommended that the browser not present this Viewpoint in its
  browser-specific user interface.

  The URL syntax ".../scene.wrl#ViewpointName" specifies the user's
  initial view when loading "scene.wrl" to be the first Viewpoint node
  in the VRML file that appears as DEF ViewpointName Viewpoint {...}.
  This overrides the first Viewpoint node in the VRML file as the
  initial user view, and a set_bind TRUE message is sent to the
  Viewpoint node named "ViewpointName". If the Viewpoint node named
  "ViewpointName" is not found, the browser shall use the first
  Viewpoint node in the VRML file (i.e. the normal default
  behaviour). The URL syntax "#ViewpointName" (i.e. no file name)
  specifies a viewpoint within the existing VRML file. If this URL is
  loaded (e.g. Anchor node's url field or loadURL() method is invoked
  by a Script node), the Viewpoint node named "ViewpointName" is bound
  (a set_bind TRUE event is sent to this Viewpoint node).

  The results are undefined if a Viewpoint node is bound and is the
  child of an LOD, Switch, or any node or prototype that disables its
  children.  If a Viewpoint node is bound that results in collision
  with geometry, the browser shall perform its self-defined navigation
  adjustments as if the user navigated to this point (see SoVRMLCollision).

*/

/*!
  \var SoSFVec3f SoVRMLViewpoint::position
  The viewpoint position. Default value is (0, 0, 0).
*/

/*!
  \var SoSFRotation SoVRMLViewpoint::orientation
  The camera orientation. By default the camera is aligned along the negative z-axis.
*/

/*!
  \var SoSFFloat SoVRMLViewpoint::fieldOfView
  Field of view. Default value is PI/4.
*/

/*!
  \var SoSFString SoVRMLViewpoint::description
  A textual viewpoint description. Is empty by default.
*/

/*!
  \var SoSFBool SoVRMLViewpoint::jump
  Jump TRUE/FALSE.
*/

/*!
  \var SoSFBool SoVRMLViewpoint::set_bind
  An eventIn that is used to bind the viewpoint.
*/

/*!
  \var SoSFTime SoVRMLViewpoint::bindTime
  An eventOut that is sent when the viewpoint is bound.
*/

/*!
  \var SoSFBool SoVRMLViewpoint::isBound
  An eventOut that is sent when the viewpoint is bound/unbound.
*/

#include <Inventor/VRMLnodes/SoVRMLViewpoint.h>
#include "coindefs.h"

#include <cmath>

#include <Inventor/VRMLnodes/SoVRMLMacros.h>

#include "nodes/SoSubNodeP.h"

SO_NODE_SOURCE(SoVRMLViewpoint);

/*!
  \copydetails SoNode::initClass(void)
*/
void
SoVRMLViewpoint::initClass(void)
{
  SO_NODE_INTERNAL_INIT_CLASS(SoVRMLViewpoint, SO_VRML97_NODE_TYPE);
}

/*!
  Constructor.
*/
SoVRMLViewpoint::SoVRMLViewpoint(void)
{
  SO_VRMLNODE_INTERNAL_CONSTRUCTOR(SoVRMLViewpoint);

  SO_VRMLNODE_ADD_EXPOSED_FIELD(position, (0.0f, 0.0f, 0.0f));
  SO_VRMLNODE_ADD_EXPOSED_FIELD(orientation, (SbRotation::identity()));
  SO_VRMLNODE_ADD_EXPOSED_FIELD(fieldOfView, (float(M_PI)/4.0f));
  SO_VRMLNODE_ADD_EXPOSED_FIELD(jump, (TRUE));

  SO_VRMLNODE_ADD_FIELD(description, (""));

  SO_VRMLNODE_ADD_EVENT_IN(set_bind);
  SO_VRMLNODE_ADD_EVENT_OUT(bindTime);
  SO_VRMLNODE_ADD_EVENT_OUT(isBound);
}

/*!
  Destructor.
*/
SoVRMLViewpoint::~SoVRMLViewpoint()
{
}

// Doc in parent
void
SoVRMLViewpoint::GLRender(SoGLRenderAction * COIN_UNUSED_ARG(action))
{
}

#endif // HAVE_VRML97
