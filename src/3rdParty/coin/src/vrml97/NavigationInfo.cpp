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
  \class SoVRMLNavigationInfo SoVRMLNavigationInfo.h Inventor/VRMLnodes/SoVRMLNavigationInfo.h
  \brief The SoVRMLNavigationInfo class is used to specify avatar and viewer settings.

  \ingroup coin_VRMLnodes
  
  \WEB3DCOPYRIGHT

  \verbatim
  NavigationInfo {
    eventIn      SFBool   set_bind
    exposedField MFFloat  avatarSize      [0.25, 1.6, 0.75] # [0, inf)
    exposedField SFBool   headlight       TRUE
    exposedField SFFloat  speed           1.0               # [0, inf)
    exposedField MFString type            ["WALK", "ANY"]
    exposedField SFFloat  visibilityLimit 0.0               # [0, inf)
    eventOut     SFBool   isBound
  }
  \endverbatim
 
  The NavigationInfo node contains information describing the physical
  characteristics of the viewer's avatar and viewing
  model. NavigationInfo node is a bindable node (see 4.6.10, Bindable
  children nodes:
  http://www.web3d.org/documents/specifications/14772/V2.0/part1/concepts.html#4.6.10).
  Thus, there exists a NavigationInfo node stack in which the top-most
  NavigationInfo node on the stack is the currently bound
  NavigationInfo node. The current NavigationInfo node is considered
  to be a child of the current Viewpoint node regardless of where it
  is initially located in the VRML file. Whenever the current
  Viewpoint nodes changes, the current NavigationInfo node shall be
  re-parented to it by the browser. Whenever the current
  NavigationInfo node changes, the new NavigationInfo node shall be
  re-parented to the current Viewpoint node by the browser.  If a TRUE
  value is sent to the set_bind eventIn of a NavigationInfo node, the
  node is pushed onto the top of the NavigationInfo node stack. When a
  NavigationInfo node is bound, the browser uses the fields of the
  NavigationInfo node to set the navigation controls of its user
  interface and the NavigationInfo node is conceptually re-parented
  under the currently bound Viewpoint node. All subsequent scaling
  changes to the current Viewpoint node's coordinate system
  automatically change aspects (see below) of the NavigationInfo node
  values used in the browser (e.g., scale changes to any ancestors'
  transformations). A FALSE value sent to set_bind pops the
  NavigationInfo node from the stack, results in an isBound FALSE
  event, and pops to the next entry in the stack which shall be
  re-parented to the current Viewpoint node. 4.6.10, Bindable children
  nodes, has more details on binding stacks
  (<http://www.web3d.org/documents/specifications/14772/V2.0/part1/concepts.html#4.6.10>).

  The type field specifies an ordered list of navigation paradigms
  that specify a combination of navigation types and the initial
  navigation type. The navigation type of the currently bound
  NavigationInfo node determines the user interface capabilities of
  the browser. For example, if the currently bound NavigationInfo
  node's type is "WALK", the browser shall present a WALK navigation
  user interface paradigm (see below for description of
  WALK). Browsers shall recognize and support at least the following
  navigation types: "ANY", "WALK", "EXAMINE", "FLY", and "NONE".

  If "ANY" does not appear in the type field list of the
  currently bound NavigationInfo, the browser's navigation user
  interface shall be restricted to the recognized navigation types
  specified in the list. In this case, browsers shall not present a
  user interface that allows the navigation type to be changed to a
  type not specified in the list. However, if any one of the values in
  the type field are "ANY", the browser may provide any type of
  navigation interface, and allow the user to change the navigation
  type dynamically. Furthermore, the first recognized type in the list
  shall be the initial navigation type presented by the browser's user
  interface.  

  ANY navigation specifies that the browser may choose the
  navigation paradigm that best suits the content and provide a user
  interface to allow the user to change the navigation paradigm
  dynamically. The results are undefined if the currently bound
  NavigationInfo's type value is "ANY" and Viewpoint transitions (see
  SoVRMLViewpoint) are triggered by the Anchor node (see SoVRMLAnchor)
  or the loadURL()scripting method (see 4.12.10, Browser script
  interface: 
  (<http://www.web3d.org/documents/specifications/14772/V2.0/part1/concepts.html#4.6.10>).

  WALK navigation is used for exploring a virtual world
  on foot or in a vehicle that rests on or hovers above the ground. It
  is strongly recommended that WALK navigation define the up vector in
  the +Y direction and provide some form of terrain following and
  gravity in order to produce a walking or driving experience. If the
  bound NavigationInfo's type is "WALK", the browser shall strictly
  support collision detection (see SoVRMLCollision).  

  FLY navigation is similar to WALK except that terrain following and
  gravity may be disabled or ignored. There shall still be some notion
  of "up" however. If the bound NavigationInfo's type is "FLY", the
  browser shall strictly support collision detection (see 6.8,
  Collision).  

  EXAMINE navigation is used for viewing individual objects and often
  includes (but does not require) the ability to spin around the
  object and move the viewer closer or further away.

  NONE navigation disables and removes all browser-specific navigation
  user interface forcing the user to navigate using only mechanisms
  provided in the scene, such as Anchor nodes or scripts that include
  loadURL().

  If the NavigationInfo type is "WALK", "FLY", "EXAMINE", or "NONE" or
  a combination of these types (i.e., "ANY" is not in the list),
  Viewpoint transitions (see SoVRMLViewpoint) triggered by the Anchor
  node (see SoVRMLAnchor) or the loadURL() scripting method shall be
  implemented as a jump cut from the old Viewpoint to the new
  Viewpoint with transition effects that shall not trigger events
  besides the exit and enter events caused by the jump.  

  Browsers may create browser-specific navigation type extensions. It
  is recommended that extended type names include a unique suffix
  (e.g., HELICOPTER_mydomain.com) to prevent conflicts.

  Viewpoint transitions (see SoVRMLViewpoint) triggered by the Anchor
  node (see SoVRMLAnchor) or the loadURL() scripting method are
  undefined for extended navigation types. If none of the types are
  recognized by the browser, the default "ANY" is used. These strings
  values are case sensitive ("any" is not equal to "ANY").

  The speed field specifies the rate at which the viewer
  travels through a scene in metres per second. Since browsers may
  provide mechanisms to travel faster or slower, this field specifies
  the default, average speed of the viewer when the NavigationInfo
  node is bound. If the NavigationInfo type is EXAMINE, speed shall not affect the
  viewer's rotational speed. Scaling in the transformation hierarchy
  of the currently bound Viewpoint node (see above) scales the speed;
  parent translation and rotation transformations have no effect on
  speed. Speed shall be non-negative. Zero speed indicates that the
  avatar's position is stationary, but its orientation and field of
  view may still change. If the navigation type is "NONE", the speed
  field has no effect.  

  The avatarSize field specifies the user's physical dimensions in the
  world for the purpose of collision detection and terrain following.
  It is a multi-value field allowing several dimensions to be
  specified. The first value shall be the allowable distance between
  the user's position and any collision geometry (as specified by a
  Collision node ) before a collision is detected. The second shall be
  the height above the terrain at which the browser shall maintain the
  viewer. The third shall be the height of the tallest object over
  which the viewer can move. This allows staircases to be built with
  dimensions that can be ascended by viewers in all browsers. The
  transformation hierarchy of the currently bound Viewpoint node
  scales the avatarSize. Translations and rotations have no effect on
  avatarSize.

  For purposes of terrain following, the browser maintains a notion of
  the down direction (down vector), since gravity is applied in the
  direction of the down vector. This down vector shall be along the
  negative Y-axis in the local coordinate system of the currently
  bound Viewpoint node (i.e., the accumulation of the Viewpoint node's
  ancestors' transformations, not including the Viewpoint node's
  orientation field).

  Geometry beyond the visibilityLimit may not be rendered. A value of
  0.0 indicates an infinite visibility limit. The visibilityLimit
  field is restricted to be greater than or equal to zero.

  The speed, avatarSize and visibilityLimit values are all scaled by
  the transformation being applied to the currently bound Viewpoint
  node. If there is no currently bound Viewpoint node, the values are
  interpreted in the world coordinate system.  This allows these
  values to be automatically adjusted when binding to a Viewpoint node
  that has a scaling transformation applied to it without requiring a
  new NavigationInfo node to be bound as well. The results are
  undefined if the scale applied to the Viewpoint node is non-uniform.

  The headlight field specifies whether a browser shall turn on a
  headlight. A headlight is a directional light that always points in
  the direction the user is looking. Setting this field to TRUE allows
  the browser to provide a headlight, possibly with user interface
  controls to turn it on and off. Scenes that enlist precomputed
  lighting (e.g., radiosity solutions) can turn the headlight off. The
  headlight shall have intensity = 1, color = (1 1 1),
  ambientIntensity = 0.0, and direction = (0 0 -1).  

  It is recommended that the near clipping plane be set to one-half of
  the collision radius as specified in the avatarSize field (setting
  the near plane to this value prevents excessive clipping of objects
  just above the collision volume, and also provides a region inside
  the collision volume for content authors to include geometry
  intended to remain fixed relative to the viewer). Such geometry
  shall not be occluded by geometry outside of the collision volume.

*/

/*!
  \var SoMFString SoVRMLNavigationInfo::type
  Types of viewer. Possible values are "WALK", "ANY", "EXAMINE", "FLY" and "NONE".
  Is set to "WALK" and "ANY" by default.
*/

/*!
  \var SoSFFloat SoVRMLNavigationInfo::speed
  Navigation speed. Default value is 1.0.
*/

/*!
  \var SoMFFloat SoVRMLNavigationInfo::avatarSize
  Size of avatar. Default value is  (0.25, 1.6, 0.75).
*/

/*!
  \var SoSFFloat SoVRMLNavigationInfo::visibilityLimit
  Visibility limit. Default value is 0.0.
*/

/*!
  \var SoSFBool SoVRMLNavigationInfo::headlight
  Specifies whether headlight should be enabled. Default value is TRUE.
*/


#include <Inventor/VRMLnodes/SoVRMLNavigationInfo.h>
#include "coindefs.h"

#include <Inventor/VRMLnodes/SoVRMLMacros.h>

#include "nodes/SoSubNodeP.h"

SO_NODE_SOURCE(SoVRMLNavigationInfo);

/*!
  \copydetails SoNode::initClass(void)
*/
void
SoVRMLNavigationInfo::initClass(void) // static
{
  SO_NODE_INTERNAL_INIT_CLASS(SoVRMLNavigationInfo, SO_VRML97_NODE_TYPE);
}

/*!
  Constructor.
*/
SoVRMLNavigationInfo::SoVRMLNavigationInfo(void)
{
  SO_VRMLNODE_INTERNAL_CONSTRUCTOR(SoVRMLNavigationInfo);

  SO_VRMLNODE_ADD_EXPOSED_FIELD(type, ("WALK"));
  this->type.setNum(2);
  this->type.set1Value(1, "ANY");
  this->type.setDefault(TRUE);

  SO_VRMLNODE_ADD_EXPOSED_FIELD(speed, (1.0f));
  SO_VRMLNODE_ADD_EXPOSED_FIELD(avatarSize, (0.25f));
  this->avatarSize.setNum(3);
  this->avatarSize.set1Value(1, 1.6f);
  this->avatarSize.set1Value(2, 0.75f);
  this->avatarSize.setDefault(TRUE);

  SO_VRMLNODE_ADD_EXPOSED_FIELD(visibilityLimit, (0.0f));
  SO_VRMLNODE_ADD_EXPOSED_FIELD(headlight, (TRUE));

  SO_VRMLNODE_ADD_EVENT_IN(set_bind);
  SO_VRMLNODE_ADD_EVENT_OUT(isBound);
}

/*!
  Destructor.
*/
SoVRMLNavigationInfo::~SoVRMLNavigationInfo() // virtual, protected
{
}

// Doc in parent
void
SoVRMLNavigationInfo::GLRender(SoGLRenderAction * COIN_UNUSED_ARG(action))
{
}

#endif // HAVE_VRML97
