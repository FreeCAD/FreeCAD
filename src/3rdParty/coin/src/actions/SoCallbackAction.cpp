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
  \class SoCallbackAction SoCallbackAction.h Inventor/actions/SoCallbackAction.h
  \brief The SoCallbackAction class invokes callbacks at specific nodes.

  \ingroup coin_actions

  This action has mechanisms for tracking traversal position and
  traversal state.  In combination with the ability to pass geometry
  primitives to callback actions set by the user, this does for
  instance make it rather straightforward to extract the geometry of a
  scene graph.

  You should be able to use this action for most of your "simple"
  traversal needs, instead of cooking up your own code, as the
  SoCallbackAction is rather flexible.

  A common use of this action is to extract geometry of non-primitive
  shapes as triangles.  A full-fledged example that demonstrates this
  on a scene graph with two spheres follows:

  \code
   #include <Inventor/SoDB.h>
   #include <Inventor/SoPrimitiveVertex.h>
   #include <Inventor/actions/SoCallbackAction.h>
   #include <Inventor/nodes/SoCoordinate3.h>
   #include <Inventor/nodes/SoIndexedFaceSet.h>
   #include <Inventor/nodes/SoSeparator.h>
   #include <Inventor/nodes/SoShape.h>
   #include <Inventor/nodes/SoSphere.h>
   #include <Inventor/nodes/SoTranslation.h>


   static SoCoordinate3 * coord3 = NULL;
   static SoIndexedFaceSet * ifs = NULL;

   static int coord3idx = 0;


   static void
   triangle_cb(void * userdata, SoCallbackAction * action,
               const SoPrimitiveVertex * v1,
               const SoPrimitiveVertex * v2,
               const SoPrimitiveVertex * v3)
   {
     const SbVec3f vtx[] = { v1->getPoint(), v2->getPoint(), v3->getPoint() };
     const SbMatrix mm = action->getModelMatrix();

     SbVec3f vx[3];
     for (int j=0; j < 3; j++) { mm.multVecMatrix(vtx[j], vx[j]); }

     // (This is sub-optimal -- should scan for the same vertex
     // coordinates already being present in the SoCoordinate3
     // node. We'll get lots of duplicate coordinates from this.)
     coord3->point.setNum(coord3->point.getNum() + 3);
     coord3->point.setValues(coord3idx, 3, vx);

     int32_t indices[] = { coord3idx, coord3idx + 1, coord3idx + 2, -1 };
     coord3idx += 3;

     int oldsize = ifs->coordIndex.getNum();
     ifs->coordIndex.setNum(oldsize + 4);
     ifs->coordIndex.setValues(oldsize, 4, indices);

     // (Note that it would likely be desirable to grab normal vectors,
     // materials and / or texture coordinates in a real-world
     // application. How to do this is not shown by the above code,
     // but it is not much different from the extraction of vertex
     // coordinates.)
   }


   int
   main(void)
   {
     SoDB::init();

     SoSeparator * root = new SoSeparator;
     root->addChild(new SoSphere);
     SoTranslation * trans = new SoTranslation;
     trans->translation.setValue(10, 0, 0);
     root->addChild(trans);
     SoSphere * ss = new SoSphere;
     ss->radius = 3;
     root->addChild(ss);

     root->ref();

     coord3 = new SoCoordinate3;
     coord3->point.setNum(0);
     ifs = new SoIndexedFaceSet;
     ifs->coordIndex.setNum(0);

     SoCallbackAction ca;
     ca.addTriangleCallback(SoShape::getClassTypeId(), triangle_cb, NULL);
     ca.apply(root);

     root->unref();

     // [the generated SoCoordinate3 and SoIndexedFaceSet nodes would now
     // typically be used in a scene graph in a viewer, or written to disk
     // or something]

     return 0;
   }
  \endcode
*/

/*!
  \typedef void SoTriangleCB(void *userdata, SoCallbackAction *action, const SoPrimitiveVertex *v1, const SoPrimitiveVertex *v2, const SoPrimitiveVertex *v3)
  
  \param userdata is a void pointer to any data the application need to
  know of in the callback function (like for instance a \e this
  pointer).
  \param action the action which invoked the callback
  \param v1 first vertex of the triangle
  \param v2 second vertex of the triangle
  \param v3 third vertex of the triangle

  \sa SoLineSegmentCB, SoPointCB
*/

/*!
  \typedef void SoLineSegmentCB(void *userdata, SoCallbackAction *action, const SoPrimitiveVertex *v1, const SoPrimitiveVertex *v2)
  
  \param userdata is a void pointer to any data the application need to
  know of in the callback function (like for instance a \e this
  pointer).
  \param action the action which invoked the callback
  \param v1 first vertex of the line
  \param v2 second vertex of the line

  \sa setPassCallback() SoPointCB
*/

/*!
  \typedef void SoPointCB(void *userdata, SoCallbackAction *action, const SoPrimitiveVertex *v)
  
  \param userdata is a void pointer to any data the application need to
  know of in the callback function (like for instance a \e this
  pointer).
  \param action the action which invoked the callback
  \param v the vertex of the point

  \sa setPassCallback()
*/


#include <Inventor/actions/SoCallbackAction.h>

#include <Inventor/SoPath.h>
#include <Inventor/elements/SoComplexityElement.h>
#include <Inventor/elements/SoCoordinateElement.h>
#include <Inventor/elements/SoCreaseAngleElement.h>
#include <Inventor/elements/SoDecimationPercentageElement.h>
#include <Inventor/elements/SoFocalDistanceElement.h>
#include <Inventor/elements/SoFontNameElement.h>
#include <Inventor/elements/SoFontSizeElement.h>
#include <Inventor/elements/SoLazyElement.h>
#include <Inventor/elements/SoLightAttenuationElement.h>
#include <Inventor/elements/SoLinePatternElement.h>
#include <Inventor/elements/SoLineWidthElement.h>
#include <Inventor/elements/SoMaterialBindingElement.h>
#include <Inventor/elements/SoModelMatrixElement.h>
#include <Inventor/elements/SoNormalBindingElement.h>
#include <Inventor/elements/SoNormalElement.h>
#include <Inventor/elements/SoOverrideElement.h>
#include <Inventor/elements/SoPickStyleElement.h>
#include <Inventor/elements/SoPointSizeElement.h>
#include <Inventor/elements/SoProfileCoordinateElement.h>
#include <Inventor/elements/SoProfileElement.h>
#include <Inventor/elements/SoProjectionMatrixElement.h>
#include <Inventor/elements/SoShapeHintsElement.h>
#include <Inventor/elements/SoSwitchElement.h>
#include <Inventor/elements/SoMultiTextureCoordinateElement.h>
#include <Inventor/elements/SoMultiTextureMatrixElement.h>
#include <Inventor/elements/SoTextureOverrideElement.h>
#include <Inventor/elements/SoUnitsElement.h>
#include <Inventor/elements/SoViewVolumeElement.h>
#include <Inventor/elements/SoViewingMatrixElement.h>
#include <Inventor/elements/SoViewportRegionElement.h>
#include <Inventor/elements/SoCacheElement.h>
#include <Inventor/elements/SoViewportRegionElement.h>
#include <Inventor/elements/SoCullElement.h>
#include <Inventor/lists/SoEnabledElementsList.h>
#include <Inventor/misc/SoState.h>
#include <Inventor/nodes/SoShape.h>
#include <Inventor/SbViewportRegion.h>

#include "actions/SoSubActionP.h"
#include "SbBasicP.h"

#ifndef DOXYGEN_SKIP_THIS

class SoCallbackData { //internal class
public:
  SoCallbackData(void * cbfunc = NULL, void * userdata = NULL)
    : func(cbfunc), data(userdata), next(NULL) {}


  void append(SoCallbackData * newdata) {
    SoCallbackData * cbdata = this;
    while (cbdata->next != NULL) cbdata = cbdata->next;
    cbdata->next = newdata;
  }

  void deleteAll(void) {
    SoCallbackData * cbdata = this;
    SoCallbackData * nextptr;
    while (cbdata) {
      nextptr = cbdata->next;
      delete cbdata;
      cbdata = nextptr;
    }
  }

  SoCallbackAction::Response doNodeCallbacks(SoCallbackAction * action,
                                             const SoNode * node);
  void doTriangleCallbacks(SoCallbackAction * action,
                           const SoPrimitiveVertex * const v1,
                           const SoPrimitiveVertex * const v2,
                           const SoPrimitiveVertex * const v3);

  void doLineSegmentCallbacks(SoCallbackAction * action,
                              const SoPrimitiveVertex * const v1,
                              const SoPrimitiveVertex * const v2);
  void doPointCallbacks(SoCallbackAction * action,
                        const SoPrimitiveVertex * v);

public:
  void * func;
  void * data;
  SoCallbackData * next;
};


SoCallbackAction::Response
SoCallbackData::doNodeCallbacks(SoCallbackAction * action,
                                const SoNode * node)
{
  SoCallbackData * cbdata = this;
  SoCallbackAction::Response response = SoCallbackAction::CONTINUE;
  while (cbdata) {
    assert(cbdata->func != NULL);
    SoCallbackAction::SoCallbackActionCB * cbfunc =
      object_to_function_cast<SoCallbackAction::SoCallbackActionCB *>( cbdata->func);
    SoCallbackAction::Response ret = cbfunc(cbdata->data, action, node);
    if (ret == SoCallbackAction::ABORT) return SoCallbackAction::ABORT;
    if (ret == SoCallbackAction::PRUNE) response = ret;
    cbdata = cbdata->next;
  }
  return response;
}

void
SoCallbackData::doTriangleCallbacks(SoCallbackAction * action,
                                    const SoPrimitiveVertex * const v1,
                                    const SoPrimitiveVertex * const v2,
                                    const SoPrimitiveVertex * const v3)
{
  SoCallbackData * cbdata = this;
  while (cbdata) {
    assert(cbdata->func != NULL);
    SoTriangleCB * tricb = object_to_function_cast<SoTriangleCB *> (cbdata->func);
    tricb(cbdata->data, action, v1, v2, v3);
    cbdata = cbdata->next;
  }
}

void
SoCallbackData::doLineSegmentCallbacks(SoCallbackAction * action,
                                       const SoPrimitiveVertex * const v1,
                                       const SoPrimitiveVertex * const v2)
{
  SoCallbackData * cbdata = this;
  while (cbdata) {
    assert(cbdata->func != NULL);
    SoLineSegmentCB * linecb = object_to_function_cast<SoLineSegmentCB *>( cbdata->func);
    linecb(cbdata->data, action, v1, v2);
    cbdata = cbdata->next;
  }
}

void
SoCallbackData::doPointCallbacks(SoCallbackAction * action,
                                 const SoPrimitiveVertex * v)
{
  SoCallbackData * cbdata = this;
  while (cbdata) {
    assert(cbdata->func != NULL);
    SoPointCB * ptcb = object_to_function_cast<SoPointCB *>( cbdata->func);
    ptcb(cbdata->data, action, v);
    cbdata = cbdata->next;
  }
}

// class to hold private, hidden data
class SoCallbackActionP {
public:
  SbBool viewportset;
  SbViewportRegion viewport;
  SoCallbackAction::Response response;
  SoNode * currentnode;

  SbList <SoCallbackData *> precallback;
  SbList <SoCallbackData *> postcallback;

  SoCallbackData * pretailcallback;
  SoCallbackData * posttailcallback;

  SbList <SoCallbackData *> trianglecallback;
  SbList <SoCallbackData *> linecallback;
  SbList <SoCallbackData *> pointcallback;

  SbBool callbackall;
};

#endif // !DOXYGEN_SKIP_THIS


// ***********************************************************************

/*!
  \typedef Response SoCallbackAction::SoCallbackActionCB(void * userdata, SoCallbackAction * action, const SoNode * node)

  Callback functions need to be of this type. \a node is at the
  current traversal point in the scene graph.
 */

/*!
  \enum SoCallbackAction::Response
  Response values for callback function.
 */
/*!
  \var SoCallbackAction::Response SoCallbackAction::CONTINUE
  Continue traversal as usual.
 */
/*!
  \var SoCallbackAction::Response SoCallbackAction::ABORT
  Abort traversal immediately.  No other callbacks are called after
  this has been returned.
 */
/*!
  \var SoCallbackAction::Response SoCallbackAction::PRUNE
  Don't do traversal of neither the current node (if returning from a
  pre-traversal callback) nor its children.

  If returned from a pre-callback, the post-callbacks will still be
  called.  If returned from a post-callback, the behaviour will be the
  same as for returning CONTINUE.
*/

// ***********************************************************************


SO_ACTION_SOURCE(SoCallbackAction);


/*!
  \copydetails SoAction::initClass(void)
*/
void
SoCallbackAction::initClass(void)
{
  SO_ACTION_INTERNAL_INIT_CLASS(SoCallbackAction, SoAction);

  SO_ENABLE(SoCallbackAction, SoViewportRegionElement);
  SO_ENABLE(SoCallbackAction, SoDecimationTypeElement);
  SO_ENABLE(SoCallbackAction, SoDecimationPercentageElement);
  SO_ENABLE(SoCallbackAction, SoOverrideElement);
  SO_ENABLE(SoCallbackAction, SoTextureOverrideElement);
  SO_ENABLE(SoCallbackAction, SoLazyElement);
  SO_ENABLE(SoCallbackAction, SoCacheElement);

  // view frustum culling is normally not used for this action, but
  // the application programmer can manually add any number of culling
  // planes to optimize callback action traversal. This is used by the
  // SoExtSelection node.
  SO_ENABLE(SoCallbackAction, SoCullElement);
}

#define PRIVATE(obj) ((obj)->pimpl)

/*!
  Default constructor. Will set the viewport to a standard
  viewport with size 640x512.
*/
SoCallbackAction::SoCallbackAction(void)
{
  this->commonConstructor();
}

/*!
  Constructor which lets you specify the viewport.

  This constructor is an extension versus the Open Inventor API.
*/
SoCallbackAction::SoCallbackAction(const SbViewportRegion & vp)
{
  this->commonConstructor();
  PRIVATE(this)->viewport = vp;
  PRIVATE(this)->viewportset = TRUE;
}

void
SoCallbackAction::commonConstructor(void)
{
  SO_ACTION_CONSTRUCTOR(SoCallbackAction);

  PRIVATE(this)->pretailcallback = NULL;
  PRIVATE(this)->posttailcallback = NULL;
  PRIVATE(this)->viewportset = FALSE;
  PRIVATE(this)->callbackall = FALSE;
}

/*!
  Sets the viewport region for this action. When set, the viewport
  element is initialized right before a traversal starts, making it
  the current viewport.

  This method is an extension versus the Open Inventor API.
*/
void
SoCallbackAction::setViewportRegion(const SbViewportRegion & vp)
{
  PRIVATE(this)->viewport = vp;
  PRIVATE(this)->viewportset = TRUE;
}

static void
delete_list_elements(SbList<SoCallbackData *> & cl)
{
  int n = cl.getLength();
  for (int i = 0; i < n; i++) {
    if (cl[i] != NULL) {
      cl[i]->deleteAll();
    }
  }
}

/*!
  Destructor.
*/
SoCallbackAction::~SoCallbackAction()
{
  delete_list_elements(PRIVATE(this)->precallback);
  delete_list_elements(PRIVATE(this)->postcallback);
  delete_list_elements(PRIVATE(this)->trianglecallback);
  delete_list_elements(PRIVATE(this)->linecallback);
  delete_list_elements(PRIVATE(this)->pointcallback);

  if (PRIVATE(this)->pretailcallback) {
    PRIVATE(this)->pretailcallback->deleteAll();
  }
  if (PRIVATE(this)->posttailcallback) {
    PRIVATE(this)->posttailcallback->deleteAll();
  }
}

//
// for setting node callbacks. makes sure NULLs are filled in where not set
//
static void
set_callback_data_idx(SbList<SoCallbackData *> & list, const int idx,
                      void * func, void * data)
{
  int n = list.getLength();
  while (n <= idx) {
    list.append(NULL);
    n++;
  }
  if (list[idx] == NULL) list[idx] = new SoCallbackData(func, data);
  else list[idx]->append(new SoCallbackData(func, data));
}

static void
set_callback_data(SbList<SoCallbackData *> & list, const SoType type,
                  void * func, void * data)
{
  SoTypeList derivedtypes;
  int n = SoType::getAllDerivedFrom(type, derivedtypes);
  for (int i = 0; i < n; i++) {
    set_callback_data_idx(list, static_cast<int>(derivedtypes[i].getData()),
                          func, data);
  }
}

/*!
  Set a function \a cb to call before every node of \a type is
  traversed. \a cb will be called with \a userdata.
 */
void
SoCallbackAction::addPreCallback(const SoType type, SoCallbackActionCB * cb,
                                 void * userdata)
{
  set_callback_data(PRIVATE(this)->precallback, type, function_to_object_cast<void *>(cb), userdata);
}

/*!
  Set a function \a cb to call after every node of \a type has been
  traversed. \a cb will be called with \a userdata.
 */
void
SoCallbackAction::addPostCallback(const SoType type, SoCallbackActionCB * cb,
                                  void * userdata)
{
  set_callback_data(PRIVATE(this)->postcallback, type, function_to_object_cast<void *>(cb), userdata);
}

/*!
  Set a function \a cb to call before the tail of a path is
  traversed. \a cb will be called with \a userdata.
 */
void
SoCallbackAction::addPreTailCallback(SoCallbackActionCB * cb, void * userdata)
{
  if (PRIVATE(this)->pretailcallback == NULL)
    PRIVATE(this)->pretailcallback = new SoCallbackData(function_to_object_cast<void *>(cb), userdata);
  else
    PRIVATE(this)->pretailcallback->append(new SoCallbackData(function_to_object_cast<void *>(cb), userdata));
}

/*!
  Set a function \a cb to call after the tail of a path has been
  traversed. \a cb will be called with \a userdata.
 */
void
SoCallbackAction::addPostTailCallback(SoCallbackActionCB * cb, void * userdata)
{
  if (PRIVATE(this)->posttailcallback == NULL)
    PRIVATE(this)->posttailcallback = new SoCallbackData(function_to_object_cast<void *>(cb), userdata);
  else
    PRIVATE(this)->posttailcallback->append(new SoCallbackData(function_to_object_cast<void *>(cb), userdata));
}

/*!
  Set a function \a cb to call when traversing a node of \a type which
  generates triangle primitives for rendering. \a cb will be called
  with \a userdata.
 */
void
SoCallbackAction::addTriangleCallback(const SoType type, SoTriangleCB * cb,
                                      void * userdata)
{
  set_callback_data(PRIVATE(this)->trianglecallback, type, function_to_object_cast<void *>(cb), userdata);
}

/*!
  Set a function \a cb to call when traversing a node of \a type which
  generates line primitives for rendering. \a cb will be called with
  \a userdata.
 */
void
SoCallbackAction::addLineSegmentCallback(const SoType type, SoLineSegmentCB * cb,
                                         void * userdata)
{
  set_callback_data(PRIVATE(this)->linecallback, type, function_to_object_cast<void *>(cb), userdata);
}

/*!
  Set a function \a cb to call when traversing a node of \a type which
  generates single point primitives for rendering. \a cb will be
  called with \a userdata.
 */
void
SoCallbackAction::addPointCallback(const SoType type, SoPointCB * cb,
                                   void * userdata)
{
  set_callback_data(PRIVATE(this)->pointcallback, type, function_to_object_cast<void *>(cb), userdata);
}

/************************************************************************************/

/*!
  Returns current decimation type setting.
 */
SoDecimationTypeElement::Type
SoCallbackAction::getDecimationType(void) const
{
  return SoDecimationTypeElement::get(this->state);
}

/*!
  Returns current decimation percentage setting.
 */
float
SoCallbackAction::getDecimationPercentage(void) const
{
  return SoDecimationPercentageElement::get(this->state);
}

/*!
  Returns current complexity setting.
 */
float
SoCallbackAction::getComplexity(void) const
{
  return SoComplexityElement::get(this->state);
}

/*!
  Returns current complexity type setting.
*/
SoComplexity::Type
SoCallbackAction::getComplexityType(void) const
{
  return static_cast<SoComplexity::Type>(SoComplexityTypeElement::get(this->state));
}

/*!
  Returns current number of coordinates in the state.
*/
int32_t
SoCallbackAction::getNumCoordinates(void) const
{
  return SoCoordinateElement::getInstance(this->state)->getNum();
}

/*!
  Returns a coordinate triplet from the current state pool of
  coordinates.
*/
const SbVec3f &
SoCallbackAction::getCoordinate3(const int index) const
{
  return SoCoordinateElement::getInstance(this->state)->get3(index);
}

/*!
  Returns a coordinate quadruplet from the current state pool of
  coordinates.
*/
const SbVec4f &
SoCallbackAction::getCoordinate4(const int index) const
{
  return SoCoordinateElement::getInstance(this->state)->get4(index);
}

/*!
  Returns current draw style setting.
*/
SoDrawStyle::Style
SoCallbackAction::getDrawStyle(void) const
{
  return static_cast<SoDrawStyle::Style>(SoDrawStyleElement::get(this->state));
}

/*!
  Returns current line pattern setting.
*/
unsigned short
SoCallbackAction::getLinePattern(void) const
{
  return SoLinePatternElement::get(this->state);
}

/*!
  Returns current line width setting.
*/
float
SoCallbackAction::getLineWidth(void) const
{
  return SoLineWidthElement::get(this->state);
}

/*!
  Returns current point size setting.
*/
float
SoCallbackAction::getPointSize(void) const
{
  return SoPointSizeElement::get(this->state);
}

/*!
  Returns current font name setting.
*/
const SbName &
SoCallbackAction::getFontName(void) const
{
  return SoFontNameElement::get(this->state);
}

/*!
  Returns current font size setting.
*/
float
SoCallbackAction::getFontSize(void) const
{
  return SoFontSizeElement::get(this->state);
}

/*!
  Returns current light model setting.
*/
SoLightModel::Model
SoCallbackAction::getLightModel(void) const
{
  return static_cast<SoLightModel::Model>(SoLazyElement::getLightModel(this->state));
}

/*!
  Returns current light attenuation setting.
*/
const SbVec3f &
SoCallbackAction::getLightAttenuation(void) const
{
  return SoLightAttenuationElement::get(this->state);
}


/*!
  Returns current material settings.
*/
void
SoCallbackAction::getMaterial(SbColor & ambient, SbColor & diffuse,
                              SbColor & specular, SbColor & emission,
                              float & shininess, float & transparency,
                              const int index) const
{
  ambient = SoLazyElement::getAmbient(this->state);
  diffuse = SoLazyElement::getDiffuse(this->state, index);
  emission = SoLazyElement::getEmissive(this->state);
  specular = SoLazyElement::getSpecular(this->state);
  shininess = SoLazyElement::getShininess(this->state);
  transparency = SoLazyElement::getTransparency(this->state, index);
}

/*!
  Returns current material binding setting.
*/
SoMaterialBinding::Binding
SoCallbackAction::getMaterialBinding(void) const
{
  return static_cast<SoMaterialBinding::Binding>(
    SoMaterialBindingElement::get(this->state)
    );
}

/*!
  Returns current number of normals in the state.
*/
uint32_t
SoCallbackAction::getNumNormals(void) const
{
  return SoNormalElement::getInstance(this->state)->getNum();
}

/*!
  Returns the normal vectors at \a index from the current state.
*/
const SbVec3f &
SoCallbackAction::getNormal(const int index) const
{
  return SoNormalElement::getInstance(this->state)->get(index);
}

/*!
  Returns current normal binding setting.
*/
SoNormalBinding::Binding
SoCallbackAction::getNormalBinding(void) const
{
  return static_cast<SoNormalBinding::Binding>(
    SoNormalBindingElement::get(this->state)
    );
}

/*!
  Returns current number of profile coordinates in the state.
*/
int32_t
SoCallbackAction::getNumProfileCoordinates(void) const
{
  return SoProfileCoordinateElement::getInstance(this->state)->getNum();
}

/*!
  Returns current number of SbVec2f profile coordinates in the state.
*/
const SbVec2f &
SoCallbackAction::getProfileCoordinate2(const int index) const
{
  return SoProfileCoordinateElement::getInstance(this->state)->get2(index);
}

/*!
  Returns current number of SbVec3f profile coordinates in the state.
*/
const SbVec3f &
SoCallbackAction::getProfileCoordinate3(const int index) const
{
  return SoProfileCoordinateElement::getInstance(this->state)->get3(index);
}

/*!
  Returns current list of profile nodes.
*/
const SoNodeList &
SoCallbackAction::getProfile(void) const
{
  return SoProfileElement::get(this->state);
}

/*!
  Returns current vertex ordering shape hint setting.

  Please note that this is the vertex ordering set by the SoShapeHints
  node. If you want to find the vertex ordering for VRML nodes you'll
  need to read this directly from the \a ccw field in those
  nodes.
*/
SoShapeHints::VertexOrdering
SoCallbackAction::getVertexOrdering(void) const
{
  return static_cast<SoShapeHints::VertexOrdering>(
    SoShapeHintsElement::getVertexOrdering(this->state)
    );
}

/*!
  Returns current shape type hint setting.

  Please note that this is the shape type set by the SoShapeHints
  node. If you want to find the shape type for VRML nodes you'll
  need to read this directly from the \a solid field in those
  nodes.
*/
SoShapeHints::ShapeType
SoCallbackAction::getShapeType(void) const
{
  return static_cast<SoShapeHints::ShapeType>(
    SoShapeHintsElement::getShapeType(this->state)
    );
}

/*!
  Returns current face type hint setting.

  Please note that this is the face type set by the SoShapeHints
  node. If you want to find the face type for VRML nodes you'll
  need to read this directly from the \a convex field in those
  nodes.

*/
SoShapeHints::FaceType
SoCallbackAction::getFaceType(void) const
{
  return static_cast<SoShapeHints::FaceType>(
    SoShapeHintsElement::getFaceType(this->state)
    );
}

/*!
  Returns current crease angle setting. Please note that this is the
  crease angle value set by the SoShapeHints node. If you want to find
  the crease angle for VRML nodes you'll need to read this directly
  from the creaseAngle field in those nodes.
*/
float
SoCallbackAction::getCreaseAngle(void) const
{
  return SoCreaseAngleElement::get(this->state);
}

/*!
  Returns current number of texture coordinates in the traversal
  state.
*/
int32_t
SoCallbackAction::getNumTextureCoordinates(void) const
{
  return SoMultiTextureCoordinateElement::getInstance(this->state)->getNum(0);
}

/*!
  Returns SbVec2f texture coordinate at \a index from the texture
  coordinate pool of the traversal state.
*/
const SbVec2f &
SoCallbackAction::getTextureCoordinate2(const int index) const
{
  return SoMultiTextureCoordinateElement::getInstance(this->state)->get2(0, index);
}

/*!
  Returns SbVec3f texture coordinate at \a index from the texture
  coordinate pool of the traversal state.

  \COIN_FUNCTION_EXTENSION

  \since Coin 2.0
*/
const SbVec3f &
SoCallbackAction::getTextureCoordinate3(const int index) const
{
  return SoMultiTextureCoordinateElement::getInstance(this->state)->get3(0, index);
}

/*!
  Returns SbVec4f texture coordinate at \a index from the texture
  coordinate pool of the traversal state.
*/
const SbVec4f &
SoCallbackAction::getTextureCoordinate4(const int index) const
{
  return SoMultiTextureCoordinateElement::getInstance(this->state)->get4(0, index);
}

/*!
  Returns current texture coordinate binding setting.
*/
SoTextureCoordinateBinding::Binding
SoCallbackAction::getTextureCoordinateBinding(void) const
{
  return static_cast<SoTextureCoordinateBinding::Binding>(
    SoTextureCoordinateBindingElement::get(this->state)
    );
}

/*!
  Returns current texture blend color setting.
*/
const SbColor &
SoCallbackAction::getTextureBlendColor(void) const
{
  return SoMultiTextureImageElement::getBlendColor(this->state, 0);
}

/*!
  Returns current texture image settings.
*/
const unsigned char *
SoCallbackAction::getTextureImage(SbVec2s & size, int & numcomps) const
{
  return SoMultiTextureImageElement::getImage(state, 0, size, numcomps);
}

/*!
  Returns current 3D texture image settings.

  \COIN_FUNCTION_EXTENSION

  \since Coin 2.0
*/
const unsigned char *
SoCallbackAction::getTextureImage(SbVec3s & size, int & numcomps) const
{
  return SoMultiTextureImageElement::getImage(state, 0, size, numcomps);
}

/*!
  Returns current texture transformation matrix setting.
*/
const SbMatrix &
SoCallbackAction::getTextureMatrix(void) const
{
  return SoMultiTextureMatrixElement::get(this->state, 0);
}

/*!
  Returns current texture mapping model setting.
*/
SoTexture2::Model
SoCallbackAction::getTextureModel(void) const
{
  return static_cast<SoTexture2::Model>( SoMultiTextureImageElement::getModel(this->state, 0));
}

/*!
  Returns current texture wrapping setting for the \c S coordinate.
*/
SoTexture2::Wrap
SoCallbackAction::getTextureWrapS(void) const
{
  return static_cast<SoTexture2::Wrap>( SoMultiTextureImageElement::getWrapS(this->state,0));
}

/*!
  Returns current texture wrapping setting for the \c T coordinate.
*/
SoTexture2::Wrap
SoCallbackAction::getTextureWrapT(void) const
{
  return static_cast<SoTexture2::Wrap>(SoMultiTextureImageElement::getWrapT(this->state, 0));
}

/*!
  Returns current texture wrapping setting for the \c R coordinate.

  \COIN_FUNCTION_EXTENSION

  \since Coin 2.0
*/
SoTexture2::Wrap
SoCallbackAction::getTextureWrapR(void) const
{
  return static_cast<SoTexture2::Wrap>(SoMultiTextureImageElement::getWrapR(this->state, 0));
}

/*!
  Returns current model matrix.
*/
const SbMatrix &
SoCallbackAction::getModelMatrix(void) const
{
  return SoModelMatrixElement::get(this->state);
}

/*!
  Returns current units setting.
*/
SoUnits::Units
SoCallbackAction::getUnits(void) const
{
  return static_cast<SoUnits::Units>(SoUnitsElement::get(this->state));
}

/*!
  Returns current camera focal distance setting.
*/
float
SoCallbackAction::getFocalDistance(void) const
{
  return SoFocalDistanceElement::get(this->state);
}

/*!
  Returns current projection matrix.
*/
const SbMatrix &
SoCallbackAction::getProjectionMatrix(void) const
{
  return SoProjectionMatrixElement::get(this->state);
}

/*!
  Returns current viewing matrix.
*/
const SbMatrix &
SoCallbackAction::getViewingMatrix(void) const
{
  return SoViewingMatrixElement::get(this->state);
}

/*!
  Returns current view volume setting.
*/
const SbViewVolume &
SoCallbackAction::getViewVolume(void) const
{
  return SoViewVolumeElement::get(this->state);
}

/*!
  Returns current viewport region setting.

  This method is an extension versus the Open Inventor API.
*/
const SbViewportRegion &
SoCallbackAction::getViewportRegion(void) const
{
  return SoViewportRegionElement::get(this->getState());
}

/*!
  Returns current pick style setting.
*/
SoPickStyle::Style
SoCallbackAction::getPickStyle(void) const
{
  return static_cast<SoPickStyle::Style>(SoPickStyleElement::get(this->state));
}

/*!
  Returns last SoSwitch::whichChild setting during the traversal.
*/
int32_t
SoCallbackAction::getSwitch(void) const
{
  return SoSwitchElement::get(this->state);
}

/************************************************************************************/

/*!
  \COININTERNAL
 */
SoCallbackAction::Response
SoCallbackAction::getCurrentResponse(void) const
{
  return PRIVATE(this)->response;
}

/*!
  \COININTERNAL

  Invoke all "pre traversal" callbacks.
 */
void
SoCallbackAction::invokePreCallbacks(const SoNode * const node)
{
  // reset response if previous node was pruned
  if (PRIVATE(this)->response == PRUNE) PRIVATE(this)->response = CONTINUE;

  int idx = static_cast<int>(node->getTypeId().getData());

  if (idx < PRIVATE(this)->precallback.getLength() && PRIVATE(this)->precallback[idx] != NULL) {
    PRIVATE(this)->response = PRIVATE(this)->precallback[idx]->doNodeCallbacks(this, node);
    if (PRIVATE(this)->response == SoCallbackAction::ABORT) {
      this->setTerminated(TRUE);
      return;
    }
  }

  if (this->getWhatAppliedTo() == SoAction::PATH &&
      this->getPathAppliedTo()->getTail() == node && PRIVATE(this)->pretailcallback != NULL) {
    PRIVATE(this)->response = PRIVATE(this)->pretailcallback->doNodeCallbacks(this, node);
    if (PRIVATE(this)->response == SoCallbackAction::ABORT) {
      this->setTerminated(TRUE);
      return;
    }
  }
  // FIXME: add code to handle pathlist traversal callbacks
  // pederb, 19991209
}

/*!
  \COININTERNAL

  Invoke all "post traversal" callbacks.
 */
void
SoCallbackAction::invokePostCallbacks(const SoNode * const node)
{
  // reset response if previous node was pruned
  if (PRIVATE(this)->response == PRUNE) PRIVATE(this)->response = CONTINUE;

  int idx = static_cast<int>(node->getTypeId().getData());
  if (idx < PRIVATE(this)->postcallback.getLength() && PRIVATE(this)->postcallback[idx] != NULL) {
    PRIVATE(this)->response = static_cast<Response>(PRIVATE(this)->postcallback[idx]->doNodeCallbacks(this, node));
    if (PRIVATE(this)->response == SoCallbackAction::ABORT) {
      this->setTerminated(TRUE);
      return;
    }
  }

  if (this->getWhatAppliedTo() == SoAction::PATH &&
      this->getPathAppliedTo()->getTail() == node && PRIVATE(this)->posttailcallback) {
    PRIVATE(this)->response = PRIVATE(this)->posttailcallback->doNodeCallbacks(this, node);
    if (PRIVATE(this)->response == SoCallbackAction::ABORT) {
      this->setTerminated(TRUE);
      return;
    }
  }
  // FIXME: add code to handle pathlist traversal callbacks
  // pederb, 19991209
}

/*!
  \COININTERNAL

  Invoke all "triangle generation" callbacks.
 */
void
SoCallbackAction::invokeTriangleCallbacks(const SoShape * const shape,
                                          const SoPrimitiveVertex * const v1,
                                          const SoPrimitiveVertex * const v2,
                                          const SoPrimitiveVertex * const v3)
{
  int idx = static_cast<int>(shape->getTypeId().getData());
  if (idx < PRIVATE(this)->trianglecallback.getLength() && PRIVATE(this)->trianglecallback[idx] != NULL)
    PRIVATE(this)->trianglecallback[idx]->doTriangleCallbacks(this, v1, v2, v3);
}

/*!
  \COININTERNAL

  Invoke all "line segment generation" callbacks.
 */
void
SoCallbackAction::invokeLineSegmentCallbacks(const SoShape * const shape,
                                             const SoPrimitiveVertex * const v1,
                                             const SoPrimitiveVertex * const v2)
{
  int idx = static_cast<int>(shape->getTypeId().getData());
  if (idx < PRIVATE(this)->linecallback.getLength() && PRIVATE(this)->linecallback[idx] != NULL)
    PRIVATE(this)->linecallback[idx]->doLineSegmentCallbacks(this, v1, v2);
}

/*!
  \COININTERNAL

  Invoke all "point" callbacks.
 */
void
SoCallbackAction::invokePointCallbacks(const SoShape * const shape,
                                       const SoPrimitiveVertex * const v)
{
  int idx = static_cast<int>(shape->getTypeId().getData());
  if (idx < PRIVATE(this)->pointcallback.getLength() && PRIVATE(this)->pointcallback[idx] != NULL)
    PRIVATE(this)->pointcallback[idx]->doPointCallbacks(this, v);
}

/*!
  \COININTERNAL

  Check from the shape nodes whether or not to generate primitives
  from the complex shapes. If there are no callbacks attached to the
  node types, making the primitives would only be a waste of CPU.
 */
SbBool
SoCallbackAction::shouldGeneratePrimitives(const SoShape * shape) const
{
  int idx = static_cast<int>(shape->getTypeId().getData());
  if (idx < PRIVATE(this)->trianglecallback.getLength() && PRIVATE(this)->trianglecallback[idx])
    return TRUE;
  if (idx < PRIVATE(this)->linecallback.getLength() && PRIVATE(this)->linecallback[idx])
    return TRUE;
  if (idx < PRIVATE(this)->pointcallback.getLength() && PRIVATE(this)->pointcallback[idx])
    return TRUE;
  return FALSE;
}

/*!
  Returns the current tail of the traversal path for the callback
  action.
 */
SoNode *
SoCallbackAction::getCurPathTail(void)
{
  return PRIVATE(this)->currentnode;
}

/*!
  Used from nodes during traversal to keep a current node pointer in
  the action.
 */
void
SoCallbackAction::setCurrentNode(SoNode * const node)
{
  PRIVATE(this)->currentnode = node;
}

// Documented in superclass. Overridden from parent class to
// initialize variables which need to be reset for each traversal.
void
SoCallbackAction::beginTraversal(SoNode * node)
{
  PRIVATE(this)->response = SoCallbackAction::CONTINUE;
  // we set the viewport region element here. This element is not enabled
  // for SoCallbackAction in Inventor, bu we think it should be.
  // It makes it possible to calculate screen space stuff in
  // the callback action callbacks.
  if (PRIVATE(this)->viewportset) {
    SoViewportRegionElement::set(this->getState(), PRIVATE(this)->viewport);
  }
  this->traverse(node);
}

void SoCallbackAction::setCallbackAll(SbBool callbackall)
{
  PRIVATE(this)->callbackall = callbackall;
}

SbBool SoCallbackAction::isCallbackAll(void) const
{
  return PRIVATE(this)->callbackall;
}

#undef PRIVATE

#ifdef COIN_TEST_SUITE

#include <Inventor/nodes/SoSwitch.h>
#include <Inventor/nodes/SoCube.h>

static SoCallbackAction::Response
preCB(void * userdata, SoCallbackAction *, const SoNode * node)
{
  SbString *str = (SbString *)userdata;
  (*str) += node->getName();
  return SoCallbackAction::CONTINUE;
}

BOOST_AUTO_TEST_CASE(callbackall)
{
  SbString str;
  SoSwitch * sw = new SoSwitch;
  sw->setName("switch");
  SoCube * cube = new SoCube;
  cube->setName("cube"); 
  sw->addChild(cube);
  sw->ref();

  SoCallbackAction cba;
  cba.addPreCallback(SoNode::getClassTypeId(), preCB, &str);
  cba.apply(sw);
  BOOST_CHECK_MESSAGE(str == "switch", "Should not traverse under switch node");

  str = "";
  cba.setCallbackAll(true);
  cba.apply(sw);
  BOOST_CHECK_MESSAGE(str == "switchcube", "Should traverse under switch node");

  sw->unref();
}

#endif // COIN_TEST_SUITE
