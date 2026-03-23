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
  \page elements The Element Classes

  Elements are mostly internal to Coin, unless you create new extension
  nodes over Coin.  Then you will probably need to know about them. \n

  Elements are part of the design for scene graph traversal in Coin. \n

  It works like this: any traversal action instantiates and keeps a
  single SoState instance during traversal.  The SoState instance uses
  SoElement objects as "memory units" to keep track of the current
  state for any feature of the scene graph nodes. \n

  As an example, consider the SoPointSize node: when the SoPointSize
  node is traversed by for instance a SoGLRenderAction, it will itself
  push a SoPointSizeElement onto the SoGLRenderAction's SoState stack.
  Later, when a SoPointSet node occurs in the scene graph, it will
  request the current point size value from the SoState by reading off
  the value of its SoPointSizeElement. \n

  SoSeparator nodes will push and pop elements on and off the state
  stack, so anything that changes state below a SoSeparator node will
  not influence anything \e above the SoSeparator. \n


  For more information on the theoretical underpinnings of this
  traversal design, you should consider reading available literature
  on the so-called "Visitor pattern".  We recommend "Design Patterns",
  by Gamma, Helm, Johnson, Vlissides (aka the "Gang Of Four").  This
  book actually uses the Inventor API traversal mechanism as the case
  study for explaining the Visitor pattern. \n

  \TOOLMAKER_REF \n

  The following is a complete example on how to extend Coin with your
  own traversal elements.  First, the class declaration of the new
  element (i.e. the header include file):

  \code
  // [texturefilenameelement.h]
  #ifndef TEXTUREFILENAMEELEMENT_H
  #define TEXTUREFILENAMEELEMENT_H

  #include <Inventor/elements/SoReplacedElement.h>
  #include <Inventor/SbString.h>

  class TextureFilenameElement : public SoReplacedElement {
    typedef SoReplacedElement inherited;

    SO_ELEMENT_HEADER(TextureFilenameElement);
  public:
    static void initClass(void);

    virtual void init(SoState * state);
    static void set(SoState * const state, SoNode * const node,
                    const SbString & filename);
    static const SbString & get(SoState * const state);
    static const TextureFilenameElement * getInstance(SoState * state);

  protected:
    virtual ~TextureFilenameElement();
    virtual void setElt(const SbString & filename);

  private:
    SbString filename;
  };

  #endif // !TEXTUREFILENAMEELEMENT_H
  \endcode

  The implementation of the element:

  \code
  // [texturefilenameelement.cpp]
  //
  // The purpose of the code in this file is to demonstrate how you can
  // make your own elements for scene graph traversals.
  //
  // Code by Peder Blekken <pederb@sim.no>. Copyright (C)
  // Kongsberg Oil & Gas Technologies.

  #include "texturefilenameelement.h"


  SO_ELEMENT_SOURCE(TextureFilenameElement);


  void
  TextureFilenameElement::initClass(void)
  {
    SO_ELEMENT_INIT_CLASS(TextureFilenameElement, inherited);
  }

  void
  TextureFilenameElement::init(SoState * state)
  {
    this->filename = "<none>";
  }

  TextureFilenameElement::~TextureFilenameElement()
  {
  }

  void
  TextureFilenameElement::set(SoState * const state, SoNode * const node,
                              const SbString & filename)
  {
    TextureFilenameElement * elem = (TextureFilenameElement *)
      SoReplacedElement::getElement(state, classStackIndex, node);
    elem->setElt(filename);
  }

  const SbString &
  TextureFilenameElement::get(SoState * const state)
  {
    return TextureFilenameElement::getInstance(state)->filename;
  }

  void
  TextureFilenameElement::setElt(const SbString & filename)
  {
    this->filename = filename;
  }

  const TextureFilenameElement *
  TextureFilenameElement::getInstance(SoState * state)
  {
    return (const TextureFilenameElement *)
      SoElement::getConstElement(state, classStackIndex);
  }
  \endcode

  And a small, standalone test application putting the new element to
  use:

  \code
  // [lstextures.cpp]
  //
  // The purpose of this file is to make a small wrapper "tool" around
  // the TextureFilenameElement extension element, just for showing
  // example code on how to make use of a user-defined custom element.
  //
  // The code goes like this:
  //
  // We initialize the element, enable it for the SoCallbackAction, read
  // a scene graph file, set callbacks on SoTexture2 and all shape nodes
  // and applies the SoCallbackAction. The callbacks will then print out
  // the texture filename information from the TextureFilenameElement
  // each time an interesting node is hit.
  //
  //
  // Code by Peder Blekken <pederb@sim.no>. Cleaned up, integrated in
  // Coin distribution and commented by Morten Eriksen
  // <mortene@sim.no>. Copyright (C) Kongsberg Oil & Gas Technologies.

  #include <Inventor/SoDB.h>
  #include <Inventor/SoInput.h>
  #include <Inventor/actions/SoCallbackAction.h>
  #include <Inventor/nodes/SoSeparator.h>
  #include <Inventor/nodes/SoTexture2.h>
  #include <Inventor/nodes/SoShape.h>
  #include <Inventor/misc/SoState.h>
  #include <cstdio>

  #include "texturefilenameelement.h"


  SoCallbackAction::Response
  pre_tex2_cb(void * data, SoCallbackAction * action, const SoNode * node)
  {
    const SbString & filename = ((SoTexture2 *)node)->filename.getValue();
    TextureFilenameElement::set(action->getState(), (SoNode *)node, filename);

    (void)fprintf(stdout, "=> New texture: %s\n",
                  filename.getLength() == 0 ?
                  "<inlined>" : filename.getString());

    return SoCallbackAction::CONTINUE;
  }

  SoCallbackAction::Response
  pre_shape_cb(void * data, SoCallbackAction * action, const SoNode * node)
  {
    const SbString & filename =
      TextureFilenameElement::get(action->getState());

    (void)fprintf(stdout, "   Texturemap on %s: %s\n",
                  node->getTypeId().getName().getString(),
                  filename.getLength() == 0 ?
                  "<inlined>" : filename.getString());

    return SoCallbackAction::CONTINUE;
  }

  void
  usage(const char * appname)
  {
    (void)fprintf(stderr, "\n\tUsage: %s <modelfile.iv>\n\n", appname);
    (void)fprintf(stderr,
                  "\tLists all texture filenames in the model file,\n"
                  "\tand on which shape nodes they are used.\n\n"
                  "\tThe purpose of this example utility is simply to\n"
                  "\tshow how to create and use an extension element for\n"
                  "\tscene graph traversal.\n\n");
  }

  int
  main(int argc, char ** argv)
  {
    if (argc != 2) {
      usage(argv[0]);
      exit(1);
    }

    SoDB::init();

    TextureFilenameElement::initClass();
    SO_ENABLE(SoCallbackAction, TextureFilenameElement);

    SoInput input;
    if (!input.openFile(argv[1])) {
      (void)fprintf(stderr, "ERROR: couldn't open file \"%s\".\n", argv[1]);
      exit(1);
    }

    SoSeparator * root = SoDB::readAll(&input);
    if (root) {
      root->ref();
      SoCallbackAction cbaction;
      cbaction.addPreCallback(SoTexture2::getClassTypeId(), pre_tex2_cb, NULL);
      cbaction.addPreCallback(SoShape::getClassTypeId(), pre_shape_cb, NULL);
      cbaction.apply(root);
      root->unref();
      return 0;
    }
    return 1;
  }
  \endcode

  \ingroup coin_elements
*/

/*!
  \class SoElement Inventor/elements/SoElement.h
  \brief SoElement is the abstract base class for all elements.

  This is the base class for all the element classes in Coin.

  \ingroup coin_elements
*/

// *************************************************************************

#include <Inventor/elements/SoElement.h>

#include <cstdlib>
#include <cassert>

#include <Inventor/elements/SoElements.h>
#include <Inventor/elements/SoTextureUnitElement.h>
#include <Inventor/elements/SoGLMultiTextureCoordinateElement.h>
#include <Inventor/elements/SoGLMultiTextureImageElement.h>
#include <Inventor/elements/SoGLMultiTextureEnabledElement.h>
#include <Inventor/elements/SoGLMultiTextureMatrixElement.h>
#include <Inventor/elements/SoBumpMapElement.h>
#include <Inventor/elements/SoBumpMapCoordinateElement.h>
#include <Inventor/elements/SoBumpMapMatrixElement.h>
#include <Inventor/elements/SoTextureCombineElement.h>
#include <Inventor/elements/SoCacheHintElement.h>

#include <Inventor/elements/SoCullElement.h> // internal element
#include <Inventor/elements/SoGLLazyElement.h> // internal element
#include <Inventor/misc/SoState.h>
#include <Inventor/lists/SoTypeList.h>

#include "elements/SoTextureScalePolicyElement.h" // internal element
#include "elements/SoTextureScaleQualityElement.h" // internal  element
#include "tidbitsp.h"
#include "coindefs.h"

// *************************************************************************

/*!
  \fn SoElement * SoElement::getElement(SoState * const state, const int stackIndex)

  This method returns the top instance (in the \a state stack) of the
  element class with stack index \a stackIndex.

  The returned instance is writable.  To make this instance, some lazy
  evaluation may have to be performed, so use getConstElement() instead
  if the instance shouldn't be modified.

  If no instance is available and cannot be made, \c NULL is
  returned.

  \sa const SoElement * SoElement::getConstElement(SoState * const state, const int stackIndex)
*/

/*!
  \var SoType SoElement::typeId
  The element's unique SoType type identification.
*/

/*!
  \var int SoElement::stackIndex
  The index in the state stack for this particular element instance.
*/

/*!
  \var int SoElement::depth
  The depth of the element instance in the state stack.
*/

/*!
  \fn SoType SoElement::getClassTypeId(void)
  This static method returns the SoType object associated with
  objects of this class.
*/
/*!
  \var SoElement::classStackIndex
  This is the static state stack index for the class.
*/
/*!
  \fn int SoElement::getClassStackIndex(void)
  This static method returns the state stack index for the class.
*/


// *************************************************************************

/*! Provides mapping from state stack indices to element types. */
SoTypeList * SoElement::stackToType;

int SoElement::classStackIndex;
SoType SoElement::classTypeId STATIC_SOTYPE_INIT;

SoType SoElement::getClassTypeId(void) { return SoElement::classTypeId; }
int SoElement::getClassStackIndex(void) { return SoElement::classStackIndex; }

// *************************************************************************

/*!
  This function initializes all the built-in Coin element classes.
*/
void
SoElement::initElements(void)
{
  SoAccumulatedElement::initClass();
  SoClipPlaneElement::initClass();
  SoGLClipPlaneElement::initClass();
  SoLightElement::initClass();
  SoModelMatrixElement::initClass();
  SoBBoxModelMatrixElement::initClass();
  SoGLModelMatrixElement::initClass();
  SoProfileElement::initClass();
  SoCacheElement::initClass();
  SoInt32Element::initClass();
  SoAnnoText3CharOrientElement::initClass();
  SoAnnoText3FontSizeHintElement::initClass();
  SoAnnoText3RenderPrintElement::initClass();
  SoComplexityTypeElement::initClass();
  SoDecimationTypeElement::initClass();
  SoDrawStyleElement::initClass();
  SoGLDrawStyleElement::initClass();
  SoGLLightIdElement::initClass();
  SoLinePatternElement::initClass();
  SoGLLinePatternElement::initClass();
  SoMaterialBindingElement::initClass();
  SoNormalBindingElement::initClass();
  SoPickStyleElement::initClass();
  SoSwitchElement::initClass();
  SoTextOutlineEnabledElement::initClass();
  SoTextureCoordinateBindingElement::initClass();
  SoUnitsElement::initClass();
  SoFloatElement::initClass();
  SoComplexityElement::initClass();
  SoCreaseAngleElement::initClass();
  SoDecimationPercentageElement::initClass();
  SoFocalDistanceElement::initClass();
  SoFontSizeElement::initClass();
  SoLineWidthElement::initClass();
  SoGLLineWidthElement::initClass();
  SoPointSizeElement::initClass();
  SoGLPointSizeElement::initClass();
  SoTextureQualityElement::initClass();
  SoGLRenderPassElement::initClass();
  SoGLUpdateAreaElement::initClass();
  SoLocalBBoxMatrixElement::initClass();
  SoOverrideElement::initClass();
  SoTextureOverrideElement::initClass();
  SoPickRayElement::initClass();
  SoReplacedElement::initClass();
  SoCoordinateElement::initClass();
  SoGLCoordinateElement::initClass();
  SoGLColorIndexElement::initClass();
  SoEnvironmentElement::initClass();
  SoGLEnvironmentElement::initClass();
  SoFontNameElement::initClass();
  SoLightAttenuationElement::initClass();
  SoNormalElement::initClass();
  SoGLNormalElement::initClass();
  SoPolygonOffsetElement::initClass();
  SoGLPolygonOffsetElement::initClass();
  SoProjectionMatrixElement::initClass();
  SoGLProjectionMatrixElement::initClass();
  SoProfileCoordinateElement::initClass();
  SoViewingMatrixElement::initClass();
  SoGLViewingMatrixElement::initClass();
  SoViewVolumeElement::initClass();
  SoShapeHintsElement::initClass();
  SoGLShapeHintsElement::initClass();
  SoShapeStyleElement::initClass();
  SoViewportRegionElement::initClass();
  SoGLViewportRegionElement::initClass();
  SoWindowElement::initClass();

  SoTransparencyElement::initClass();
  SoAmbientColorElement::initClass();
  SoDiffuseColorElement::initClass();
  SoEmissiveColorElement::initClass();
  SoLightModelElement::initClass();
  SoShininessElement::initClass();
  SoSpecularColorElement::initClass();

  SoLazyElement::initClass();
  SoGLLazyElement::initClass();
  SoCullElement::initClass();
  SoGLCacheContextElement::initClass();

  SoTextureScalePolicyElement::initClass();
  SoTextureScaleQualityElement::initClass();

  SoListenerPositionElement::initClass();
  SoListenerOrientationElement::initClass();
  SoListenerDopplerElement::initClass();
  SoListenerGainElement::initClass();

  SoSoundElement::initClass();

  SoTextureUnitElement::initClass();

  SoMultiTextureCoordinateElement::initClass();
  SoMultiTextureImageElement::initClass();
  SoMultiTextureEnabledElement::initClass();
  SoMultiTextureMatrixElement::initClass();
  SoGLMultiTextureCoordinateElement::initClass();
  SoGLMultiTextureImageElement::initClass();
  SoGLMultiTextureEnabledElement::initClass();
  SoGLMultiTextureMatrixElement::initClass();

  SoBumpMapElement::initClass();
  SoBumpMapCoordinateElement::initClass();
  SoBumpMapMatrixElement::initClass();

  SoTextureCombineElement::initClass();
  SoCacheHintElement::initClass();

  SoGLVBOElement::initClass();

  SoDepthBufferElement::initClass();
  SoGLDepthBufferElement::initClass();

  SoVertexAttributeElement::initClass();
  SoGLVertexAttributeElement::initClass();
  SoVertexAttributeBindingElement::initClass();
}

// Note: the following documentation for initClass() will also be
// visible for subclasses, so keep it general.
/*!
  Initialize relevant common data for all instances, like the type
  system.
 */
void
SoElement::initClass(void)
{
  SoElement::stackToType = new SoTypeList;

  // Make sure we only initialize once.
  assert(SoElement::classTypeId == SoType::badType());
  SoElement::classTypeId =
    SoType::createType(SoType::badType(), "Element", NULL);

  SoElement::classStackIndex = -1;
  SoElement::initElements();

  coin_atexit(reinterpret_cast<coin_atexit_f *>(SoElement::cleanup), CC_ATEXIT_NORMAL);
}

// atexit callback
void
SoElement::cleanup(void)
{
  delete SoElement::stackToType;
  SoElement::classTypeId STATIC_SOTYPE_INIT;
}

/*!
  The constructor.  To create element instances, use SoType::createInstance()
  for the elements type identifier.
*/
SoElement::SoElement(void)
  : nextup(NULL),
    nextdown(NULL)
{
}

/*!
  The destructor.
*/
SoElement::~SoElement()
{
}

/*!
  This function initializes the element type in the given SoState.  It
  is called for the first element of each enabled element type in
  SoState objects.
*/
void
SoElement::init(SoState * COIN_UNUSED_ARG(state))
{
  // virtual method
}

/*!
  This method is called every time a new element is required in one of
  the stacks. This happens when a writable element is requested, using
  SoState::getElement() or indirectly SoElement::getElement(), and the
  depth of the current element is less than the state depth.

  Override this method if your element needs to copy data from the
  previous top of stack. The push() method is called on the new
  element, and the previous element can be found using
  SoElement::getNextInStack().
*/
void
SoElement::push(SoState * COIN_UNUSED_ARG(state))
{
  // virtual method
}

/*!
  This method is called when the state is popped, and the depth of
  the element is bigger than the current state depth. pop() is called
  on the new top of stack, and a pointer to the previous top of stack
  is passed in \a prevTopElement.

  Override this method if you need to copy some state information from
  the previous top of stack.
*/
void
SoElement::pop(SoState * COIN_UNUSED_ARG(state), const SoElement * COIN_UNUSED_ARG(prevTopElement))
{
  // virtual method
}

/*!
  This function is for printing element information, and is used
  mostly for debugging purposes.
*/
void
SoElement::print(FILE * file) const
{
  (void)fprintf(file, "%s[%p]\n",
                this->getTypeId().getName().getString(), this);
}

/*!
  This function returns \c TRUE is the element matches another element
  (of the same class), with respect to cache validity.

  If the application programmer's extension element has a matches()
  function, it should also have a copyMatchInfo() function.
*/
SbBool
SoElement::matches(const SoElement * COIN_UNUSED_ARG(element)) const
{
  return FALSE;
}

/*!
  \fn virtual SoElement * SoElement::copyMatchInfo(void) const = 0

  This function creates a copy of the element that contains enough
  information to enable the matches() function to work.

  Used to help with scene graph traversal caching operations.
*/

/*!
  Returns the number of allocated element stack index slots.
*/
int
SoElement::getNumStackIndices(void)
{
  return SoElement::stackToType->getLength();
}

/*!
  Returns the SoType identifier for the element class with element
  state stack index \a stackIndex.
*/
SoType
SoElement::getIdFromStackIndex(const int stackIndex)
{
  assert(SoElement::stackToType->getLength() > stackIndex);
  return (*SoElement::stackToType)[stackIndex];
}

/*!
  Sets the depth value of the element instance in the state stack.
*/
void
SoElement::setDepth(const int depth)
{
  this->depth = depth;
}

/*!
  Returns the state stack depth value of the element instance.
*/
int
SoElement::getDepth() const
{
  return this->depth;
}

/*!
  \fn void SoElement::capture(SoState * const state) const;

  This function does whatever is necessary in the state for caching
  purposes.  If should be called by subclasses of SoElement whenever
  any value in the element is accessed.
*/

/*!
  \fn void const SoElement * SoElement::getConstElement(SoState * const state, const int stackIndex);

  This method returns a reference to the top element of the class with
  stack index \a stackIndex. The returned element is non-mutable.

  (Don't try to be clever and cast away the constness -- if the
  returned instance is modified, strange, hard to find and generally
  wonderful bugs will most likely start to happen.)

  If no instance can be returned, \c NULL is returned.

  \sa SoElement * SoElement::getElement(SoState * const state, const int stackIndex)
*/

/*!
  Adds the element to the cache.
*/
void
SoElement::captureThis(SoState * state) const
{
  SoCacheElement::addElement(state, this);
}

/*!
  Sets the type identifier of an instance.

  Note that this is fundamentally different from the SoNode runtime
  type system.
*/
void
SoElement::setTypeId(const SoType typeId)
{
  this->typeId = typeId;
}

/*!
  Returns the type identification of an object derived from a class
  inheriting SoElement.  This is used for runtime type checking and
  "downward" casting.

  For a more thorough explanation of the runtime type identification
  functionality, see the documentation of SoBase::getTypeId().
*/
const SoType
SoElement::getTypeId(void) const
{
  return this->typeId;
}

/*!
  Returns the stack index for an element instance.
*/
int
SoElement::getStackIndex(void) const
{
  return this->stackIndex;
}

/*!
  Sets the stack index in an instance.  Used in constructors of
  derived elements.
*/
void
SoElement::setStackIndex(const int stackIndex)
{
  this->stackIndex = stackIndex;
}

/*!
  Returns the value of a new available stack index.
*/
int
SoElement::createStackIndex(const SoType typeId)
{
  if (typeId.canCreateInstance()) {
    SoElement::stackToType->append(typeId);
    return SoElement::stackToType->getLength() - 1;
  }
  return -1;
}

/*!
  Returns the next element down in the stack. Should be used in push()
  to get the previous element.

  This method has a slightly misleading name, but we didn't change it
  to stay compatible with the original SGI Inventor API.
*/
SoElement *
SoElement::getNextInStack(void) const
{
  return this->nextdown;
}

/*!
  Returns the next free element, i.e. the next element up in the stack.
*/
SoElement *
SoElement::getNextFree(void) const
{
  return this->nextup;
}
