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
#include "config.h"
#endif // HAVE_CONFIG_H

#ifdef HAVE_NODEKITS

/*!
  \class SoBaseKit SoBaseKit.h Inventor/nodekits/SoBaseKit.h
  \brief The SoBaseKit class is the top level superclass for nodekits.

  \ingroup coin_nodekits

  Node kits are collections of nodes and other node kits (from here on
  node kits which are part of some other node kit, will only be referred
  to as nodes or parts, see catalogs and parts), organized in a way
  that is convenient for its use. A node kit inherits SoNode and can
  thus be inserted into a scene graph as any other node.

  The organizing of the nodes and node kits of some node kit, is done
  through catalogs. A node kit's catalog describes the nodes that can
  be members of the node kit. These members are called parts. Thus a
  node kit has a catalog describing the parts that it offers to the
  user.

  Each part in the catalog has some values saying something about the
  part itself and about the role the part plays in the scene graph.
  Those values are:

  <dl>
  <dt> Name
  <dd> The name of the part.
  <dt> Type
  <dd> The part's node type.
  <dt> Default Type
  <dd> If the part's type is an abstract superclass, this value will hold
  the default subclass used by this part.
  <dt> Created by default?
  <dd> Holds \c TRUE if the part should be instantiated when the node kit
  is instantiated, otherwise the part is kept empty until it is set by some
  of the means applicable.
  <dt> Parent Name
  <dd> The name of the part that is this part's parent.
  <dt> Right Sibling
  <dd> The name of the part that is the part immediately to the right of
  this part in the node kit scene graph.
  <dt> Is it a list?
  <dd> Holds \c TRUE if the part is a list, otherwise it is \c FALSE. See
  SoNodeKitListPart for more info on node kit lists.
  <dt> List Container Type
  <dd> The type of group node used to hold the items if the part is a list.
  <dt> List Element Type
  <dd> The types of nodes that is allowed to be held by this part if the part
  is a list.
  <dt> Is it public?
  <dd> Holds \c TRUE if the part should be publicly available, otherwise
  it holds \c FALSE.
  </dl>

  Node kits use lazy instantiation when it creates its parts. This means
  that the nodes making up the parts of the nodekit only are created when
  they are needed. If the "Created by default?" holds TRUE, then the part
  is created when the node kit itself is instantiated. If not, parts are
  created when they are requested through SoBaseKit::getPart() or the
  SO_GET_PART() macro, or created with SoBaseKit::set(). Also, if a part is
  set with SoBaseKit::setPart() or the SO_SET_PART() macro, any previously
  uncreated parts above the set part in the hierarchy, are created
  automatically.

  The advantages of using node kits to represent a scene graph are many.
  \li Since a node kit collects nodes into a single unit, it becomes
      an extra abstraction layer for the application programmer. Such
      a layer can represent a model of a human being as one unit where
      subunits as arms, legs, textures, etc. are contained within. Thus
      we can instantiate a model of a human by creating an instance of
      the node kit, instead of having to create a possibly large
      amount of nodes needed for such a model.
  \li A part of the node kit doesn't have one specific setup. A shape part
      can e.g. be swapped with any other shape, since they are of the same
      type. If the node kit of a human has a part called "head" which is of
      type SoShape, it might default to a sphere. But if the programmer
      thinks that a cube might fit better, one can set the "head" part to
      a cube instead, or maybe a face set representing a complex model of
      a head.
  \li Node kits can have as simple or as complex catalogs as needed. The
      nodes included in the node kit can, if needed, represent the
      whole range of Inventor features. One part can as easily be of a
      node kit type, making it possible to create hierarchies of node kits.
      Having a node kit of a human, it might be feasible to have sub node
      kits describing the different body parts.
  \li Node kits are an efficient way of creating scene graphs. If some
      part of it isn't needed at the moment of node kit instantiation,
      they aren't created. Thus parts are only created when needed, either
      by the application or some other part.
  \li The application code becomes smaller and easier to read, as the node
      kits provides simple routines for creating and setting parts.
  \li New node kits can be created through subclassing to obtain simple
      setups of scene graphs best fitted to the application.

  The usage of a node kit is straightforward. Below follows a code
  example showing some simple SoShapeKit usage.

  \code

  #include <Inventor/Qt/SoQt.h>
  #include <Inventor/Qt/viewers/SoQtExaminerViewer.h>
  #include <Inventor/nodekits/SoShapeKit.h>
  #include <Inventor/nodes/SoSeparator.h>
  #include <Inventor/nodes/SoCube.h>

  int
  main(int argc, char ** argv)
  {
    QWidget * window = SoQt::init(argv[0]);

    SoQtExaminerViewer * viewer = new SoQtExaminerViewer(window);

    // Instantiating a shape kit, by default creating a simple sphere.
    SoShapeKit * shapekit = new SoShapeKit;
    // Swapping the sphere with a cube.
    shapekit->setPart("shape", new SoCube);
    // Setting the cube to be rendered in the color red. The shape kit
    // has a SoAppearanceKit as one of its parts. The "material" part
    // used to set the color of the shape, really belongs the
    // appearance kit. If the SoShapeKit::set() is used, it will
    // check if some of its sub kits have a part with the name given,
    // and delegate the setting to the correct kit.
    shapekit->set("material", "diffuseColor 1 0 0");

    SoSeparator * newroot = new SoSeparator;
    newroot->ref();

    newroot->addChild(shapekit);

    viewer->setSceneGraph(newroot);

    viewer->show();
    SoQt::show(window);

    SoQt::mainLoop();
    delete viewer;

    newroot->unref();
    return 0;
  }
  \endcode

  The above code snippet will produce a viewer with a side view to
  the scene shown below:

  <center>
  \image html basekitexample.png "Rendering of Example Scenegraph"
  </center>

  Notice that the code needed for creating this simple shape using
  a shape kit, amounts to this:

  \code
   SoShapeKit * shapekit = new SoShapeKit;

   shapekit->setPart("shape", new SoCube);
   shapekit->set("material", "diffuseColor 1 0 0");
  \endcode

  ..while doing it without shape kits amounts to this:

  \code
  SoSeparator * root = new SoSeparator;
  SoMaterial * material = new SoMaterial;
  material->diffuseColor.setValue(1,0,0);
  root->addChild(material);
  root->addChild(new SoCube);
  \endcode

  ..so even for this minuscule mock-up example, you save on code
  verbosity and complexity.


  \TOOLMAKER_REF


  Following is a complete example of a node kit extension. The node
  kit is a kit which automatically scales a shape so it will be the
  same size in screen pixels, no matter which distance it is from the
  camera. This is useful for marker graphics. The shape defaults to a
  cube, but can be set by the programmer to any shape or scene
  subgraph.

  The header file:

  \code
  // Copyright (C) Kongsberg Oil & Gas Technologies. All rights reserved.

  #ifndef COIN_SHAPESCALE_H
  #define COIN_SHAPESCALE_H
  #include <Inventor/nodekits/SoSubKit.h>
  #include <Inventor/nodekits/SoBaseKit.h>
  #include <Inventor/fields/SoSFFloat.h>

  class SbViewport;
  class SoState;
  class SbColor;
  class SbVec2s;

  class ShapeScale : public SoBaseKit {
    typedef SoBaseKit inherited;

    SO_KIT_HEADER(ShapeScale);

    SO_KIT_CATALOG_ENTRY_HEADER(topSeparator);
    SO_KIT_CATALOG_ENTRY_HEADER(scale);
    SO_KIT_CATALOG_ENTRY_HEADER(shape);

   public:
    ShapeScale(void);
    static void initClass(void);

    SoSFFloat active;
    SoSFFloat projectedSize;

   protected:
    virtual void GLRender(SoGLRenderAction * action);
    virtual ~ShapeScale();
  };

  #endif // ! SHAPESCALE_H
  \endcode

  The source code for the example:

  \code
  // Copyright (C) Kongsberg Oil & Gas Technologies. All rights reserved.

  //  The ShapeScale class is used for scaling a shape based on
  //  projected size.
  //
  //  This nodekit can be inserted in your scene graph to add for
  //  instance 3D markers that will be of a constant projected size.
  //
  //  The marker shape is stored in the "shape" part. Any kind of node
  //  can be used, even group nodes with several shapes, but the
  //  marker shape should be approximately of unit size, and with a
  //  center position in (0, 0, 0).


  //  SoSFFloat ShapeScale::active
  //  Turns the scaling on/off. Default value is TRUE.


  //  SoSFFloat ShapeScale::projectedSize
  //  The requested projected size of the shape. Default value is 5.0.

  #include "ShapeScale.h"

  #include <Inventor/actions/SoGLRenderAction.h>
  #include <Inventor/nodes/SoShape.h>
  #include <Inventor/nodes/SoScale.h>
  #include <Inventor/nodes/SoCube.h>
  #include <Inventor/nodes/SoSeparator.h>
  #include <Inventor/elements/SoViewVolumeElement.h>
  #include <Inventor/elements/SoViewportRegionElement.h>
  #include <Inventor/elements/SoModelMatrixElement.h>

  SO_KIT_SOURCE(ShapeScale);


  //  Constructor.
  ShapeScale::ShapeScale(void)
  {
    SO_KIT_CONSTRUCTOR(ShapeScale);

    SO_KIT_ADD_FIELD(active, (TRUE));
    SO_KIT_ADD_FIELD(projectedSize, (5.0f));

    SO_KIT_ADD_CATALOG_ENTRY(topSeparator, SoSeparator, FALSE, this, \x0, FALSE);
    SO_KIT_ADD_CATALOG_ABSTRACT_ENTRY(shape, SoNode, SoCube, TRUE, topSeparator, \x0, TRUE);
    SO_KIT_ADD_CATALOG_ENTRY(scale, SoScale, FALSE, topSeparator, shape, FALSE);

    SO_KIT_INIT_INSTANCE();
  }


  // Destructor.
  ShapeScale::~ShapeScale()
  {
  }

  // Initializes this class. Call before using it.

  void
  ShapeScale::initClass(void)
  {
    SO_KIT_INIT_CLASS(ShapeScale, SoBaseKit, "BaseKit");
  }

  static void
  update_scale(SoScale * scale, const SbVec3f & v)
  {
    // only write to field when scaling has changed.
    if (scale->scaleFactor.getValue() != v) {
      scale->scaleFactor = v;
    }
  }

  // Overridden to (re)initialize scaling before rendering marker.
  void
  ShapeScale::GLRender(SoGLRenderAction * action)
  {
    SoState * state = action->getState();

    SoScale * scale = (SoScale*) this->getAnyPart(SbName("scale"), TRUE);
    if (!this->active.getValue()) {
      update_scale(scale, SbVec3f(1.0f, 1.0f, 1.0f));
    }
    else {
      const SbViewportRegion & vp = SoViewportRegionElement::get(state);
      const SbViewVolume & vv = SoViewVolumeElement::get(state);
      SbVec3f center(0.0f, 0.0f, 0.0f);
      float nsize = this->projectedSize.getValue() / float(vp.getViewportSizePixels()[1]);
      SoModelMatrixElement::get(state).multVecMatrix(center, center); // transform to WCS
      float scalefactor = vv.getWorldToScreenScale(center, nsize);
      update_scale(scale, SbVec3f(scalefactor, scalefactor, scalefactor));
    }
    inherited::GLRender(action);
  }
  \endcode

  And a complete example showing how one can use this node kit:

  \code
  // Copyright (C) Kongsberg Oil & Gas Technologies. All rights reserved.

  #include <Inventor/Qt/SoQt.h>
  #include <Inventor/Qt/viewers/SoQtExaminerViewer.h>
  #include <Inventor/SoInput.h>
  #include <Inventor/SoOutput.h>
  #include <Inventor/SoPickedPoint.h>
  #include <Inventor/actions/SoRayPickAction.h>
  #include <Inventor/events/SoMouseButtonEvent.h>
  #include <Inventor/nodes/SoBaseColor.h>
  #include <Inventor/nodes/SoCube.h>
  #include <Inventor/nodes/SoEventCallback.h>
  #include <Inventor/nodes/SoSeparator.h>
  #include <Inventor/nodes/SoSwitch.h>
  #include <Inventor/nodes/SoTranslation.h>
  #include <cassert>
  #include <cstdlib>
  #include <ctime>

  #include "ShapeScale.h"

  // Returns random value between 0.0f and 1.0f.
  static float
  normalized_rand(void)
  {
    return float(rand())/float(RAND_MAX);
  }

  static SoSeparator *
  construct_new_marker(const SbVec3f & v)
  {
    SoSeparator * markerroot = new SoSeparator;

    SoTranslation * t = new SoTranslation;
    t->translation = v;
    markerroot->addChild(t);

    ShapeScale * kit = new ShapeScale;
    kit->active = TRUE;
    kit->projectedSize = 5.0f;

    // create the marker
    SoSeparator * markersep = new SoSeparator;

    SoBaseColor * mat = new SoBaseColor;
    mat->rgb.setValue(normalized_rand(), normalized_rand(), normalized_rand());
    markersep->addChild(mat);

    // marker shape should be unit size, with center in (0.0f, 0.0f, 0.0f)
    SoCube * cube = new SoCube;
    cube->width = 1.0f;
    cube->height = 1.0f;
    cube->depth = 1.0f;

    markersep->addChild(cube);
    kit->setPart("shape", markersep);
    markerroot->addChild(kit);

    return markerroot;
  }

  static void
  event_cb(void * ud, SoEventCallback * n)
  {
    const SoMouseButtonEvent * mbe = (SoMouseButtonEvent *)n->getEvent();

    if (mbe->getButton() == SoMouseButtonEvent::BUTTON1 &&
      mbe->getState() == SoButtonEvent::DOWN) {

      SoQtExaminerViewer * viewer = (SoQtExaminerViewer *)ud;

      SoRayPickAction rp(viewer->getViewportRegion());
      rp.setPoint(mbe->getPosition());
      rp.apply(viewer->getSceneManager()->getSceneGraph());

      SoPickedPoint * point = rp.getPickedPoint();
      if (point == NULL) {
        (void)fprintf(stderr, "\n** miss! **\n\n");
        return;
      }

      n->setHandled();

      const SoPath * p = rp.getCurPath();

      for (int i = 0; i < p->getLength(); i++) {
        SoNode * n = p->getNodeFromTail(i);
        if (n->isOfType(SoGroup::getClassTypeId())) {
          SoGroup * g = (SoGroup *)n;
          g->addChild(construct_new_marker(point->getPoint()));
          break;
        }
      }
    }
  }

  void
  show_instructions(void)
  {
    (void)fprintf(stdout,
      "\nThis example program demonstrates the use of the ShapeScale nodekit.\n"
      "\nQuick instructions:\n\n"
      "  * place the marker by clicking on a shape with the left mouse button\n"
      "  * hit ESC to toggle back and forth to view mode\n"
      "  * zoom back and forth to see how the markers stay the same size\n\n");
  }

  int
  main(int argc, char ** argv)
  {
    if (argc != 2) {
      (void) fprintf(stderr,"\nSpecify an Inventor file as argument.\n");
      return -1;
    }

    QWidget * window = SoQt::init(argv[0]);
    ShapeScale::initClass(); // init our extension nodekit

    SoQtExaminerViewer * ex1 = new SoQtExaminerViewer(window);

    SoInput input;
    SbBool ok = input.openFile(argv[1]);
    if (!ok) {
      (void) fprintf(stderr, "Unable to open file: %s\n", argv[1]);
      return -1;
    }

    SoSeparator * root = SoDB::readAll(&input);

    if (root == NULL) {
      (void) fprintf(stderr, "Unable to read file: %s\n", argv[1]);
      return -1;
    }

    show_instructions();

    SoSeparator * newroot = new SoSeparator;
    newroot->ref();

    newroot->addChild(root);

    // create event callback and marker nodes
    SoSeparator * sep = new SoSeparator;
    newroot->addChild(sep);

    SoEventCallback * ecb = new SoEventCallback;
    ecb->addEventCallback(SoMouseButtonEvent::getClassTypeId(), event_cb, ex1);
    sep->addChild(ecb);

    ex1->setSceneGraph(newroot);
    ex1->setTransparencyType(SoGLRenderAction::SORTED_OBJECT_BLEND);
    ex1->setViewing(FALSE);

    ex1->show();
    SoQt::show(window);

    SoQt::mainLoop();
    delete ex1;

    newroot->unref();
    return 0;
  }
  \endcode
*/

#include <Inventor/nodekits/SoBaseKit.h>

#include <cstdlib>
#include <climits>
#include <cctype>
#include <cstring>

#include <Inventor/nodekits/SoNodeKitListPart.h>
#include <Inventor/nodes/SoSeparator.h>
#include <Inventor/nodes/SoCallback.h>
#include <Inventor/nodes/SoEventCallback.h>
#include <Inventor/misc/SoChildList.h>
#include <Inventor/actions/SoSearchAction.h>
#include <Inventor/actions/SoAudioRenderAction.h>
#include <Inventor/actions/SoGetMatrixAction.h>
#include <Inventor/actions/SoRayPickAction.h>
#include <Inventor/actions/SoGetBoundingBoxAction.h>
#include <Inventor/actions/SoWriteAction.h>
#include <Inventor/SoInput.h>
#include <Inventor/SoOutput.h>
#include <Inventor/details/SoNodeKitDetail.h>
#include <Inventor/SoPickedPoint.h>
#include <Inventor/lists/SoPickedPointList.h>
#include <Inventor/lists/SoNodeList.h>
#include <Inventor/errors/SoReadError.h>
#include <Inventor/C/tidbits.h> // coin_isspace()
#include <Inventor/errors/SoDebugError.h>

#include "coindefs.h" // COIN_OBSOLETED()
#include "io/SoWriterefCounter.h"
#include "nodekits/SoSubKitP.h"

class SoBaseKitP {
public:
  SoBaseKitP(SoBaseKit * kit) : kit(kit) { }

  SoBaseKit * kit;
  SoFieldData * writedata;
  SbBool didcount;

  // This array is a 1-1 mapping of the fields corresponding to the
  // catalog parts. Catalog indices will therefore also be used as
  // indices into this array.
  SbList<SoSFNode*> instancelist;

  void addKitDetail(SoFullPath * path, SoPickedPoint * pp);
  void createWriteData(void);
  void testParentWrite(void);

  void copyParts(const SoBaseKit * srckit, SbList <SoNode*> & partlist,
                 const SbBool copyconnections);

  void setParts(SbList <SoNode*> partlist, const SbBool leafparts);

  SbBool readUnknownFields(SoInput *in, SoFieldData *&unknownFieldData );
};

#define PRIVATE(p) ((p)->pimpl)
#define PUBLIC(p) ((p)->kit)

SbBool SoBaseKit::searchchildren = FALSE;

SO_KIT_SOURCE(SoBaseKit);


/*!
  \var SoChildList * SoBaseKit::children
  \COININTERNAL
*/
/*!
  \var SbBool SoBaseKit::connectionsSetUp
  \COININTERNAL
*/


/*!
  Constructor.

  This is the top-level superclass of all node kit and dragger
  classes. The catalog structure of SoBaseKit is as follows:

  \verbatim
  CLASS SoBaseKit
  -->"this"
  -->   "callbackList"
  \endverbatim

  \NODEKIT_POST_DIAGRAM

  \NODEKIT_PRE_TABLE

  \verbatim
  CLASS SoBaseKit
  PVT   "this",  SoBaseKit  ---
        "callbackList",  SoNodeKitListPart [ SoCallback, SoEventCallback ]
  \endverbatim

  \NODEKIT_POST_TABLE

  As can be seen from the catalog, all node kits can have a callback
  node in front of all other nodes in the kit. This is handy for
  catching events that should go to application processing.
*/
SoBaseKit::SoBaseKit(void)
{
  PRIVATE(this) = new SoBaseKitP(this);
  PRIVATE(this)->writedata = NULL;

  SO_KIT_INTERNAL_CONSTRUCTOR(SoBaseKit);

  // Can't use ADD_CATALOG_ENTRY macro for the toplevel "this" entry,
  // as we don't want to call SO_NODE_ADD_FIELD(). This is how the
  // invocation would have looked if we could use the macro:
  //
  // SO_KIT_ADD_CATALOG_ENTRY(this, SoBaseKit, TRUE, "", "", FALSE);

  SoBaseKit::classcatalog->addEntry("this",
                                    SoBaseKit::getClassTypeId(),
                                    SoBaseKit::getClassTypeId(),
                                    TRUE,
                                    "",
                                    "",
                                    FALSE,
                                    SoType::badType(),
                                    SoType::badType(),
                                    FALSE);

  SO_KIT_ADD_CATALOG_LIST_ENTRY(callbackList, SoSeparator, TRUE, this, "", SoCallback, TRUE);
  SO_KIT_ADD_LIST_ITEM_TYPE(callbackList, SoEventCallback);

  // this could be created on demand, but will make it more complicated
  this->children = new SoChildList(this);

  this->connectionsSetUp = FALSE;
  SO_KIT_INIT_INSTANCE();
}

/*!
  Destructor.
*/
SoBaseKit::~SoBaseKit()
{
  delete this->children;
  delete PRIVATE(this)->writedata;
  delete PRIVATE(this);
}

/*!
  \copydetails SoNode::initClass(void)
*/
void
SoBaseKit::initClass(void)
{
  SO_NODE_INTERNAL_INIT_CLASS(SoBaseKit, SO_FROM_INVENTOR_1);
  // set rayPick method
  SoType type = SoBaseKit::getClassTypeId();
  SoRayPickAction::addMethod(type, SoNode::rayPickS);
  SoAudioRenderAction::addMethod(type,
                                 SoAudioRenderAction::callDoAction);
  SoBaseKit::searchchildren = FALSE;
}

/*!
  Returns a pointer to the node part with \a partname.

  This method calls SoBaseKit::getAnyPart() with \a leafcheck and \a
  publiccheck both set to \c TRUE.

  See the documentation of SoBaseKit::getAnyPart() for information on
  how to use \a partname and \a makeifneeded, and what you can expect
  to get returned from this method.
*/
SoNode *
SoBaseKit::getPart(const SbName & partname, SbBool makeifneeded)
{
  return this->getAnyPart(partname, makeifneeded, TRUE, TRUE);
}

/*!
  Returns the full path name to a catalog part, given the part's
  current item pointer.
*/
SbString
SoBaseKit::getPartString(const SoBase * part)
{
  const SoNodekitCatalog * catalog = this->getNodekitCatalog();
  if (part->isOfType(SoNode::getClassTypeId())) {
    int idx = this->findNodeInThisKit((SoNode *)part);
    if (idx >= 0) {
      return SbString(catalog->getName(idx).getString());
    }
    return SbString();
  }
  else if (part->isOfType(SoPath::getClassTypeId())) {
    SoFullPath * path = (SoFullPath *)part;
    int pathidx = path->findNode(this);
    if (pathidx < 0) return SbString();
    SoBaseKit * kit = this;
    SbString partname;
    int parentnum = 0;
    SoNode * tail = path->getTail();
    SoNode * node = kit;
    while (node != tail) {
      node = path->getNode(++pathidx);
      int partnum = kit->findNodeInThisKit(node, parentnum);
      if (partnum < 0) {
#if COIN_DEBUG
        SoDebugError::postWarning("SoBaseKit::getPartString",
                                  "Illegal path");
#endif // COIN_DEBUG
        return SbString();
      }
      if (catalog->isLeaf(partnum)) {
        if (partname != "") partname += '.';
        partname += catalog->getName(partnum).getString();
      }
      if (node->isOfType(SoNodeKitListPart::getClassTypeId())) {
        // no sense in using SoNodeKitListPart as a non-leaf node, right?
        assert(catalog->isLeaf(partnum));
        SoNodeKitListPart * list = (SoNodeKitListPart *)node;
        pathidx += 2; // // skip container node
        if (pathidx >= path->getLength()) {
#if COIN_DEBUG
          SoDebugError::postWarning("SoBaseKit::getPartString",
                                    "Path too short");
#endif // COIN_DEBUG
          return SbString();
        }
        node = path->getNode(pathidx);
        int childidx = list->findChild(node);
        assert(childidx >= 0);
        partname += '[';
        partname.addIntString(childidx);
        partname += ']';
      }
      if (node->isOfType(SoBaseKit::getClassTypeId())) {
        kit = (SoBaseKit *) node;
        catalog = kit->getNodekitCatalog();
        parentnum = 0;
      }
      else {
        // search more in this kit
        parentnum = partnum;
      }
    }
    return partname;
  }
  return SbString();
}

/*!
  Calls SoBaseKit::createPathToAnyPart() with \a leafcheck \c TRUE,
  and \a publiccheck \c TRUE (and other arguments as given to this
  function).

  See SoBaseKit::createPathToAnyPart() for documentation.
*/
SoNodeKitPath *
SoBaseKit::createPathToPart(const SbName & partname, SbBool makeifneeded, const SoPath * pathtoextend)
{
  return this->createPathToAnyPart(partname, makeifneeded, TRUE, TRUE, pathtoextend);
}

/*!
  Sets the catalog part given by \a partname to the \a from node pointer.
*/
SbBool
SoBaseKit::setPart(const SbName & partname, SoNode * from)
{
  return this->setAnyPart(partname, from, FALSE);
}

static const char *
skip_spaces(const char * ptr)
{
  // ANSI C isspace() takes the current locale into account. Under
  // Microsoft Windows, this can lead to "interesting" artifacts, like a case
  // with RR tracked down and fixed by <thammer@sim.no> where a
  // character (was it ü?) with ASCII value > 127 made isspace()
  // return non-nil on a German system. So we're using our own
  // locale-independent isspace() implementation instead.
  while (coin_isspace(*ptr)) ptr++;
  return ptr;
}

static int
find_partname_length(const char * ptr)
{
  int cnt = 0;
  while (ptr[cnt] && !coin_isspace(ptr[cnt]) &&
         ptr[cnt] != '{' && ptr[cnt] != '}') {
    cnt++;
  }
  return cnt;
}

/*!
  Sets nodekit part field values. The input argument string is of the
  format:

  \code
  partname {
    fieldname fieldval
    fieldname fieldval
    [...]
  }
  partname {
    fieldname fieldval
    fieldname fieldval
    [...]
  }
  [...]
  \endcode

  (Whitespace layout is ignored, as always for Inventor format input
  strings.)

  Here's an example, changing several values of the camera part of an
  SoCameraKit instance:

  \code
  kit->set("camera { heightAngle 0.3927  nearDistance 1.1  farDistance 999.9 }");
  \endcode
*/
SbBool
SoBaseKit::set(const char * namevaluepairliststring)
{
  const size_t stringlen = strlen(namevaluepairliststring); // cache this value
  const char * currptr = skip_spaces(namevaluepairliststring);
  SoInput memInput;

  while (*currptr) {
    int partnamelen = find_partname_length(currptr);
    const char * start = skip_spaces(currptr + partnamelen);
    if (*start != '{') { // first non-space after partname should be a {
#if COIN_DEBUG
      SoDebugError::postWarning("SoBaseKit::set",
                                "parse error at byte %d in input string",
                                start-namevaluepairliststring);
#endif // COIN_DEBUG
      return FALSE;
    }
    start++; // skip {
    SbString partname(currptr, 0, partnamelen-1);
    SoBaseKit * kit = this;
    int partNum;
    SbBool isList;
    int listIdx;
    if (!SoBaseKit::findPart(partname, kit, partNum, isList, listIdx, TRUE, NULL, TRUE)) {
#if COIN_DEBUG
      SoDebugError::postWarning("SoBaseKit::set",
                                "part \"%s\" not found",
                                partname.getString());
#endif // COIN_DEBUG
      return FALSE;
    }

    SoNode * node = PRIVATE(kit)->instancelist[partNum]->getValue();
    PRIVATE(kit)->instancelist[partNum]->setDefault(FALSE);

    if (isList) {
      SoNodeKitListPart * list = (SoNodeKitListPart *)node;
      if (listIdx < 0 || listIdx > list->getNumChildren()) {
#if COIN_DEBUG
        SoDebugError::postWarning("SoBaseKit::set",
                                  "index %d out of bounds for part \"%s\"",
                                  listIdx, partname.getString());
#endif // COIN_DEBUG
        return FALSE;
      }
      else if (listIdx == list->getNumChildren()) {
        if (!list->canCreateDefaultChild()) {
#if COIN_DEBUG
          SoDebugError::postWarning("SoBaseKit::set",
                                    "Unable to create default child for list-part \"%s\"",
                                    partname.getString());
#endif // COIN_DEBUG
          return FALSE;
        }
        node = list->createAndAddDefaultChild();
      }
      else {
        node = list->getChild(listIdx);
      }
    }
    memInput.setBuffer(start, stringlen - (start-namevaluepairliststring));
    SbBool dummy;
    if (!node->getFieldData()->read(&memInput, node, TRUE, dummy)) {
#if COIN_DEBUG
      SoDebugError::postWarning("SoBaseKit::set",
                                "error while parsing data for part \"%s\"",
                                partname.getString());
#endif // COIN_DEBUG
      return FALSE;
    }
    currptr = start + (int) memInput.getNumBytesRead();
    if (*currptr == '}') currptr++;
    assert(currptr <= namevaluepairliststring + stringlen);
    currptr = skip_spaces(currptr);
  }
  return TRUE;
}

/*!
  This just overloads the other SoBaseKit::set() method, and provides
  a way to set a part value by using a separate input argument for the
  name of the part and the name of the field (i.e. parameter)
  settings.
*/
SbBool
SoBaseKit::set(const char * partnamestring, const char * parameterstring)
{
  SbString partname(partnamestring);
  int partNum;
  SbBool isList;
  int listIdx;
  SoBaseKit * kit = this;
  if (SoBaseKit::findPart(partname, kit, partNum, isList, listIdx, TRUE, NULL, TRUE)) {
    SoNode * node = PRIVATE(kit)->instancelist[partNum]->getValue();
    PRIVATE(kit)->instancelist[partNum]->setDefault(FALSE);
    assert(node != NULL); // makeifneeded was TRUE in findPart call
    if (isList) {
      assert(node->isOfType(SoNodeKitListPart::getClassTypeId()));
      SoNodeKitListPart * list = (SoNodeKitListPart *) node;
      if (listIdx < 0 || listIdx > list->getNumChildren()) {
#if COIN_DEBUG
        SoDebugError::postWarning("SoBaseKit::set",
                                  "index %d out of bounds for part \"%s\"",
                                  listIdx, partnamestring);
#endif // COIN_DEBUG
        return FALSE;
      }
      else if (listIdx == list->getNumChildren()) {
        if (!list->canCreateDefaultChild()) {
#if COIN_DEBUG
          SoDebugError::postWarning("SoBaseKit::set",
                                    "Unable to create default child for list-part \"%s\"",
                                    partname.getString());
#endif // COIN_DEBUG
          return FALSE;
        }
        node = list->createAndAddDefaultChild();
      }
      else {
        node = list->getChild(listIdx);
      }
    }
    if (node) {
      SoInput memInput;
      SbBool dummy;
      memInput.setBuffer(parameterstring, strlen(parameterstring));
      const SoFieldData * fielddata = node->getFieldData();
      return fielddata->read(&memInput, node, TRUE, dummy);
    }
  }
  return FALSE;
}

// Doc in superclass.
void
SoBaseKit::doAction(SoAction * action)
{
  int numindices;
  const int * indices;
  if (action->getPathCode(numindices, indices) == SoAction::IN_PATH) {
    this->children->traverseInPath(action, numindices, indices);
  }
  else {
    this->children->traverse(action);
  }
}

// Doc in superclass.
void
SoBaseKit::callback(SoCallbackAction * action)
{
  SoBaseKit::doAction((SoAction *)action);
}

// Doc in superclass.
void
SoBaseKit::GLRender(SoGLRenderAction * action)
{
  SoBaseKit::doAction((SoAction *)action);
}

// Doc in superclass. Overridden to calculate bounding box center.
void
SoBaseKit::getBoundingBox(SoGetBoundingBoxAction * action)
{
  int numindices;
  const int * indices;
  int last = action->getPathCode(numindices, indices) == SoAction::IN_PATH ?
    indices[numindices-1] : this->children->getLength() - 1;

  SbVec3f acccenter(0.0f, 0.0f, 0.0f);
  int numacc = 0;

  for (int i = 0; i <= last; i++) {
    this->children->traverse(action, i, i);
    if (action->isCenterSet()) {
      acccenter += action->getCenter();
      numacc++;
      action->resetCenter();
    }
  }
  if (numacc) action->setCenter(acccenter / float(numacc), FALSE);
}

// Doc in superclass.
void
SoBaseKit::getMatrix(SoGetMatrixAction * action)
{
  // SoBaseKit should be traversed like a normal SoGroup node, and the
  // children should only be traversed if we're IN_PATH or OFF_PATH
  // (SoGetMatrixAction is only applied on a path or on a single node,
  // and we must not calculate when BELOW_PATH or NO_PATH).
  int numindices;
  const int * indices;
  switch (action->getPathCode(numindices, indices)) {
  case SoAction::IN_PATH:
    this->children->traverseInPath(action, numindices, indices);
    break;
  case SoAction::OFF_PATH:
    this->children->traverse(action);
    break;
  default:
    break;
  }
}

// Doc in superclass.
void
SoBaseKit::handleEvent(SoHandleEventAction * action)
{
  SoBaseKit::doAction((SoAction *)action);
}

// Doc in superclass.
void
SoBaseKit::rayPick(SoRayPickAction * action)
{
  SoBaseKit::doAction((SoAction *)action);

  const SoPickedPointList & pplist = action->getPickedPointList();
  const int n = pplist.getLength();
  for (int i = 0; i < n; i++) {
    SoPickedPoint * pp = pplist[i];
    SoFullPath * path = (SoFullPath*) pp->getPath();
    if (path->containsNode(this) && pp->getDetail(this) == NULL) {
      PRIVATE(this)->addKitDetail(path, pp);
    }
  }
}

// Doc in superclass.
void
SoBaseKit::search(SoSearchAction * action)
{
  inherited::search(action);
  if (action->isFound() || !SoBaseKit::searchchildren) return;
  SoBaseKit::doAction((SoAction *)action);
}

// Test if node has all fields set to default and if the fields
// contains the default values. If so, we don't need to write it.
static SbBool
is_default_node(SoNode * node)
{
  SoNode * definstance = NULL;
  const SoFieldData * fielddata = node->getFieldData();
  int i, n = fielddata->getNumFields();
  for (i = 0; i < n; i++) {
    SoField * field = fielddata->getField(node, i);
    if (field->isConnectionEnabled() && field->isConnected()) break;
    if (definstance == NULL) {
      definstance = (SoNode *)node->getTypeId().createInstance();
      definstance->ref();
    }
    if (!field->isDefault() &&
        !field->isSame(*fielddata->getField(definstance, i))) break;
  }
  if (definstance) definstance->unref();
  // if all fields were tested, it is a default node.
  return i == n;
}

// Doc in superclass.
void
SoBaseKit::write(SoWriteAction * action)
{
  // debugging code start **************************************************

  // If the below envvar is set, we'll write nodekit's current scene
  // graph instead of writing as a nodekit.
  //
  // Note that if the nodekit is a dragger, the resulting scene graph
  // export may still not look exactly the same as when the exported
  // subgraph is contained within the dragger, as
  // SoDragger::GLRender() sets a number of elements in the traversal
  // state to non-intrusive "default" values before rendering the
  // dragger geometry. These settings will not be part of the exported
  // iv-file.
  static int dump = -1;
  if (dump == -1) {
    const char * env = coin_getenv("COIN_DEBUG_FLATTEN_NODEKITS_ON_WRITE");
    dump = env && (atoi(env) > 0);
  }
  if (dump) {
    this->children->traverse(action);
    return;
  }

  // debugging code end ****************************************************


  SoOutput * out = action->getOutput();
  if (out->getStage() == SoOutput::COUNT_REFS) {
    this->addWriteReference(out, FALSE);
  }
  else if (out->getStage() == SoOutput::WRITE) {
    if (this->writeHeader(out, FALSE, FALSE)) return; // no more to write
    // FIXME: shouldn't this if() rather be an assert? 20030523 mortene.
    if (PRIVATE(this)->writedata) {
      PRIVATE(this)->writedata->write(out, this);
      // we don't need it any more
      delete PRIVATE(this)->writedata;
      PRIVATE(this)->writedata = NULL;
    }
    this->writeFooter(out);
  }
  else assert(0 && "unknown stage");
}

// documented in superclass
void
SoBaseKit::addWriteReference(SoOutput * out, SbBool isfromfield)
{
  // don't call inherited::addWriteReference(), as we will handle
  // the fields ourselves, using a new fielddata. This is needed to
  // write fields in the correct order.
  SoBase::addWriteReference(out, isfromfield);

  // If first invocation during the reference counting pass, check
  // nodes in our catalog.
  if (!isfromfield && !SoWriterefCounter::instance(out)->hasMultipleWriteRefs(this)) {
    this->countMyFields(out);
  }
}

/*!
  Reference count the write connections to nodes in the catalog.
*/
void
SoBaseKit::countMyFields(SoOutput * out)
{
  assert(out->getStage() == SoOutput::COUNT_REFS);

  // already created?
  //
  // FIXME: could this ever be TRUE without that being an error
  // situation? I have a feeling this should rather be an
  // assert(). Investigate. 20030523 mortene.
  if (PRIVATE(this)->writedata) return;

  // Initialize isDefault() flag on fields that should not be
  // written. This is a virtual method.
  this->setDefaultOnNonWritingFields();

  const SoNodekitCatalog * catalog = this->getNodekitCatalog();

  // PRIVATE(this)->writedata contains a sorted list of fields.
  //
  // FIXME: the pimpl->writedata scheme doesn't look multithread-safe
  // wrt multiple SoWriteAction instances working in parallel over the
  // same scene. 20030521 mortene.
  PRIVATE(this)->createWriteData();

  // test if parent of parts is writing. Then we must write part anyway.
  PRIVATE(this)->testParentWrite();

  // we might count fields that won't be written here, but it
  // doesn't matter, since we're operating on a copy of the fields.

  int i, n = PRIVATE(this)->writedata->getNumFields();
  for (i = 0; i < n; i++) {
    const SbName name = PRIVATE(this)->writedata->getFieldName(i);
    SoField * field = PRIVATE(this)->writedata->getField(this, i);
    int partnum = catalog->getPartNumber(name);
    if (partnum < 0) {
      // field is not a part. Do normal field write.
      if (field->shouldWrite()) {
        field->write(out, name);
      }
    }
    else {
      if (!field->isDefault()) field->write(out, name);
      else {
        SoNode * node = (SoNode*) ((SoSFNode*)field)->getValue();
        if (node) {
          if (node->isOfType(SoBaseKit::getClassTypeId())) {
            SoBaseKit * kit = (SoBaseKit*) node;
            kit->countMyFields(out);
            if (kit->forceChildDrivenWriteRefs(out)) {
              field->setDefault(FALSE);
              // add a write reference on the kit node only. We supply
              // isfromfield TRUE to achieve this
              kit->addWriteReference(out, TRUE);
            }
          }
        }
      }
    }
  }
}

// Note: the following documentation for
// setDefaultOnNonWritingFields() will also be used for nodekit and
// dragger subclasses, so keep it general.
/*!
  (Be aware that this method is unlikely to be of interest to the
  application programmer who does not want to extend the library with
  new custom nodekits or draggers.  If you indeed \e are writing
  extensions, see the information in the SoBaseKit class
  documentation.)

  This is a virtual method, and the code in it should call
  SoField::setDefault() with argument \c TRUE on part fields that
  should not be written upon scene graph export operations.

  This is typically done when:

  <OL>

  <LI> field value is \c NULL and part is \c NULL by default </LI>

  <LI> it is a leaf SoGroup or SoSeparator node with no children </LI>

  <LI> it is a leaf listpart with no children and an SoGroup or
  SoSeparator container </LI>

  <LI> it is a non-leaf part and it is of SoGroup type and all fields
  are at their default values </LI>

  </OL>

  Subclasses should usually override this to do additional settings
  for new member fields.  From the subclass, do remember to call
  "upwards" to your superclass' setDefaultOnNonWritingFields() method.
*/
void
SoBaseKit::setDefaultOnNonWritingFields(void)
{
  const SoNodekitCatalog * catalog = this->getNodekitCatalog();
  int n = PRIVATE(this)->instancelist.getLength();
  for (int i = 1; i < n; i++) {
    SoSFNode * field = PRIVATE(this)->instancelist[i];
    if (field->isDefault()) { continue; }

    SoNode * node = field->getValue();

    if (node == NULL) {
      // first test listed in API doc above
      if (catalog->isNullByDefault(i)) { field->setDefault(TRUE); }
      continue;
    }

    const SbBool leaf = catalog->isLeaf(i);
    const SoType type = node->getTypeId();

    if (leaf) {
      // second test
      if ((type == SoGroup::getClassTypeId() ||
           type == SoSeparator::getClassTypeId()) &&
          ((SoGroup*)node)->getNumChildren() == 0) {
        field->setDefault(TRUE);
      }
      // third test
      else if (type == SoNodeKitListPart::getClassTypeId()) {
        SoNodeKitListPart * list = (SoNodeKitListPart*) node;
        const SoNode * container = list->getContainerNode();
        if (list->getNumChildren() == 0 && container &&
            (container->getTypeId() == SoSeparator::getClassTypeId() ||
             container->getTypeId() == SoGroup::getClassTypeId())) {
          field->setDefault(TRUE);
        }
      }
    }
    else { // not leaf
      // fourth test
      if (node->isOfType(SoGroup::getClassTypeId()) && is_default_node(node)) {
        field->setDefault(TRUE);
      }
    }
  }
}

/*!
  Returns \c TRUE if kit should write. This happens if shouldWrite()
  returns \c TRUE, or if any of the children (recursively) should
  write.
*/
SbBool
SoBaseKit::forceChildDrivenWriteRefs(SoOutput * out)
{
  if (SoWriterefCounter::instance(out)->shouldWrite(this)) return TRUE;

  // if NULL we already did this test, found that we shouldn't write,
  // deleted writedata and set writedata to NULL.
  if (!PRIVATE(this)->writedata) return FALSE;

  const SoNodekitCatalog * catalog = this->getNodekitCatalog();
  int i, n = PRIVATE(this)->writedata->getNumFields();

  // loop through fields and break as soon as we find a reason
  // to write
  for (i = 0; i < n; i++) {
    SoField * field = PRIVATE(this)->writedata->getField(this, i);
    int partnum = catalog->getPartNumber(PRIVATE(this)->writedata->getFieldName(i));
    if (!field->isDefault()) break;
    else if (partnum < 0 && field->isIgnored()) break;
    else if (partnum > 0) {
      SoSFNode * part = (SoSFNode*) field;
      SoNode * node = part->getValue();
      if (node) {
        if (SoWriterefCounter::instance(out)->shouldWrite(node)) break;
        else if (node->isOfType(SoBaseKit::getClassTypeId())) {
          SoBaseKit * kit = (SoBaseKit*) node;
          // recurse
          if (kit->forceChildDrivenWriteRefs(out)) break;
        }
      }
    }
  }

  if (i < n) { // did we find a reason to write?
    SoBase::addWriteReference(out);
    return TRUE;
  }
  else {
    delete PRIVATE(this)->writedata;
    PRIVATE(this)->writedata = NULL;
    return FALSE;
  }
}


// Documented in superclass.
void
SoBaseKit::getPrimitiveCount(SoGetPrimitiveCountAction * action)
{
  SoBaseKit::doAction((SoAction *)action);
}

// Documented in superclass.
SoChildList *
SoBaseKit::getChildren(void) const
{
  return this->children;
}

/*!
  Print out the full nodekit catalog structure.  Just invokes
  SoBaseKit::printSubDiagram() on the catalog root. Useful for
  debugging.

  Example output:

  \verbatim
  CLASS SoWrapperKit
  -->"this"
        "callbackList"
        "topSeparator"
           "pickStyle"
           "appearance"
           "units"
           "transform"
           "texture2Transform"
           "childList"
  -->      "localTransform"
  -->      "contents"
  \endverbatim

  The arrows denote new entries in the catalog for the particular
  class versus its superclass. (Apart from the root entry, of
  course.)

  For a more detailed catalog dump, see SoBaseKit::printTable().
*/
void
SoBaseKit::printDiagram(void)
{
  fprintf(stdout, "CLASS So%s\n", this->getTypeId().getName().getString());
  this->printSubDiagram("this", 0);
}

/*!
  Print out the nodekit catalog structure from \a rootname and
  downwards in the catalog tree, with indentation starting at \a
  level.

  \sa printDiagram()
*/
void
SoBaseKit::printSubDiagram(const SbName & rootname, int level)
{
  const SoNodekitCatalog * parentcatalog = NULL;
  if (this->getTypeId() != SoBaseKit::getClassTypeId()) {
    SoType parenttype = this->getTypeId().getParent();
    SoBaseKit * parentobj = (SoBaseKit *)parenttype.createInstance();
    parentcatalog = parentobj->getNodekitCatalog();
    parentobj->ref();
    parentobj->unref();
  }

  const SoNodekitCatalog * thiscat = this->getNodekitCatalog();

  int i = 0;
  if (!parentcatalog ||
      parentcatalog->getPartNumber(rootname) == SO_CATALOG_NAME_NOT_FOUND ||
      parentcatalog->getType(rootname) != thiscat->getType(rootname)) {
    fprintf(stdout, "-->");
    i++;
  }
  for (; i < level+1; i++) fprintf(stdout, "   ");

  fprintf(stdout, "\"%s\"\n", rootname.getString());

  for (int j=0; j < thiscat->getNumEntries(); j++) {
    if (thiscat->getParentName(j) == rootname)
      this->printSubDiagram(thiscat->getName(j), level + 1);
  }
}

/*!
  Write the complete nodekit catalog in table form.

  Example output:

  \verbatim
  CLASS SoWrapperKit
  PVT   "this",  SoWrapperKit  ---
        "callbackList",  SoNodeKitListPart [ SoCallback, SoEventCallback ]
  PVT   "topSeparator",  SoSeparator  ---
        "pickStyle",  SoPickStyle  ---
        "appearance",  SoAppearanceKit  ---
        "units",  SoUnits  ---
        "transform",  SoTransform  ---
        "texture2Transform",  SoTexture2Transform  ---
        "childList",  SoNodeKitListPart [ SoShapeKit, SoSeparatorKit ]
        "localTransform",  SoTransform  ---
        "contents",  SoSeparator  ---
  \endverbatim

  \c PVT denotes that it is a private entry in the catalog, then
  follows the part name and the part type. If the part is a list, the
  allowed node types for the list is given in square brackets, and if
  not there's a triple hyphen. If the part type is abstract, the
  default part type will be listed last (not shown in the example
  output above).
*/
void
SoBaseKit::printTable(void)
{
  fprintf(stdout, "CLASS So%s\n", this->getTypeId().getName().getString());

  const SoNodekitCatalog * thiscat = this->getNodekitCatalog();
  for (int i=0; i < thiscat->getNumEntries(); i++) {
    const SoType t = thiscat->getType(i);
    fprintf(stdout, "%s   \"%s\",  So%s ",
            thiscat->isPublic(i) ? "   " : "PVT",
            thiscat->getName(i).getString(),
            t.getName().getString());
    if (thiscat->isList(i)) {
      SoTypeList tlist = thiscat->getListItemTypes(i);
      fprintf(stdout, "[ ");
      for (int j=0; j < tlist.getLength(); j++) {
        if (j) fprintf(stdout, ", ");
        fprintf(stdout, "So%s", tlist[j].getName().getString());
      }
      fprintf(stdout, " ] ");
    }
    else {
      fprintf(stdout, " --- ");
    }

    if (t != thiscat->getDefaultType(i)) {
      fprintf(stdout, ", (default type = So%s)",
              thiscat->getDefaultType(i).getName().getString());
    }
    fprintf(stdout, "\n");
  }
}

/*!
  Returns the value of the flag indicating whether or not the kit
  parts are searched during SoSearchAction traversal.

  \sa SoBaseKit::setSearchingChildren()
*/
SbBool
SoBaseKit::isSearchingChildren(void)
{
  return SoBaseKit::searchchildren;
}

/*!
  Set whether or not the kit parts should be searched during
  SoSearchAction traversal. The default value is \c FALSE.
*/
void
SoBaseKit::setSearchingChildren(const SbBool newval)
{
  SoBaseKit::searchchildren = newval;
}

// Documented in superclass. Overridden to also recurse on non-null
// part nodes.
SoNode *
SoBaseKit::addToCopyDict(void) const
{
  SoNode * cp = (SoNode*) SoFieldContainer::checkCopy(this);
  if (cp == NULL) { // not copied?
    cp = (SoNode*) this->getTypeId().createInstance();
    assert(cp);
    cp->ref();
    SoFieldContainer::addCopy(this, cp);
    cp->unrefNoDelete();

    int n = PRIVATE(this)->instancelist.getLength();
    for (int i = 1; i < n; i++) {
      SoNode * node = PRIVATE(this)->instancelist[i]->getValue();
      if (node != NULL) node->addToCopyDict();
    }
  }
  return cp;
}

// (Doc in superclass.) Overridden to copy parts correctly.
void
SoBaseKit::copyContents(const SoFieldContainer * fromfc,
                        SbBool copyconnections)
{
  int i;

  // disable connections while copying
  SbBool oldsetup = this->setUpConnections(FALSE);

  // do normal node copy
  inherited::copyContents(fromfc, copyconnections);

  const SoBaseKit * srckit = (const SoBaseKit*) fromfc;

  // convenient reference
  /*const SbList <SoSFNode*> & srcfields = srckit->getCatalogInstances();*/

  const int n = PRIVATE(this)->instancelist.getLength();

  // use temporary lists to store part node pointers and field
  // default flag, as we will modify the originals.
  SbList <SoNode *> partlist;
  SbList <SbBool> flaglist;

  // part 0 is this
  partlist.append(NULL);
  flaglist.append(FALSE);

  // initialize temporary lists
  for (i = 1; i < n; i++) {
    partlist.append(NULL);
    flaglist.append(PRIVATE(this)->instancelist[i]->isDefault());
  }

  // copy parts, taking care of scene graph
  PRIVATE(this)->copyParts(srckit, partlist, copyconnections);

  // remove all old children before copying parts
  this->getChildren()->truncate(0);

  // reset part fields
  for (i = 1; i < n; i++) {
    PRIVATE(this)->instancelist[i]->setValue(NULL);
    PRIVATE(this)->instancelist[i]->setDefault(TRUE);
  }

  // set non-leaf nodes first
  PRIVATE(this)->setParts(partlist, FALSE);

  // then leaf nodes
  PRIVATE(this)->setParts(partlist, TRUE);

  // do final pass
  for (i = 1; i < n; i++) {
    // restore default flag for fields
    PRIVATE(this)->instancelist[i]->setDefault(flaglist[i]);

    // unref nodes in temporary list as they were ref'ed
    // when inserted
    if (partlist[i]) partlist[i]->unref();
  }

  // enable connections
  if (oldsetup) this->setUpConnections(TRUE);
}

/*!
  Returns a pointer to the group node above an SoNodeKitListPart in
  the catalog given by \a listname.

  If the list part (and its container) was not yet constructed, they
  will be so if \a makeifneeded is \c TRUE (otherwise, \c NULL will be
  returned).
*/
SoGroup *
SoBaseKit::getContainerNode(const SbName & listname, SbBool makeifneeded)
{
  SoBaseKit * kit = this;
  int partNum;
  SbBool isList;
  int listIdx;
  if (SoBaseKit::findPart(SbString(listname.getString()), kit, partNum,
                          isList, listIdx, makeifneeded, NULL, TRUE)) {
    SoNode * node = PRIVATE(kit)->instancelist[partNum]->getValue();
    if (node == NULL) return NULL;
    assert(node->isOfType(SoNodeKitListPart::getClassTypeId()));
    SoNodeKitListPart * list = (SoNodeKitListPart *)node;
    return list->getContainerNode();
  }
  return NULL;
}

/*!
  Returns catalog part of the given \a partname.

  If the \a partname part is not in the nodekit's catalog, return \c
  NULL.

  If the part is specified in the catalog, but has not yet been made,
  the function will either construct the part (if \a makeifneeded is
  \c TRUE) or just return \c NULL (if \a makeifneeded is \c FALSE).

  If \a leafcheck is \c TRUE, a pointer to the part will only be
  returned if it is a leaf in the catalog (otherwise \c NULL is
  returned).

  If \a publiccheck is \c TRUE, a pointer to the part will only be
  returned if it is a public catalog part (otherwise \c NULL is
  returned).


  The \a partname input argument should be given as a \e "path" of
  catalog part names down to the wanted leaf part. The syntax for
  specifying \a partname "paths" is as follows (given in Backus-Naur
  Form (BNF)):

  \verbatim
  BNF:

  partname = singlename | compoundname
  compoundname = singlename | compoundname.singlename
  singlename = singlepartname | singlelistelementname
  singlelistelementname = singlelistname[idx]

  singlepartname is name of a part ("ordinary", nodekit or list)
  singlelistname is name of a part which is a list
  idx is an integer value
  \endverbatim
*/
SoNode *
SoBaseKit::getAnyPart(const SbName & partname, SbBool makeifneeded,
                      SbBool leafcheck, SbBool publiccheck)
{

  SoBaseKit * kit = this;
  int partNum;
  SbBool isList;
  int listIdx;

  SbString partstring(partname.getString());

  if (SoBaseKit::findPart(partstring, kit, partNum, isList, listIdx,
                          makeifneeded, NULL, TRUE)) {

    if (publiccheck && !kit->getNodekitCatalog()->isPublic(partNum)) {
      SoDebugError::postWarning("SoBaseKit::getAnyPart",
                                "Part \"%s\" found in %s, but access is private.",
                                partname.getString(),
                                this->getTypeId().getName().getString());
      return NULL;
    }

    if (!leafcheck || kit->getNodekitCatalog()->isLeaf(partNum)) {
      if (isList) {
        SoNode * partnode = PRIVATE(kit)->instancelist[partNum]->getValue();
        if (partnode == NULL) return NULL;
        assert(partnode->isOfType(SoNodeKitListPart::getClassTypeId()));
        SoNodeKitListPart * list = (SoNodeKitListPart *) partnode;
        if (listIdx >= 0 && listIdx < list->getNumChildren()) {
          return list->getChild(listIdx);
        }
        else if (makeifneeded && (listIdx == list->getNumChildren())) {
          if (!list->canCreateDefaultChild()) {
#if COIN_DEBUG
            SoDebugError::postWarning("SoBaseKit::getAnyPart",
                                      "Unable to create default child for list-part \"%s\"",
                                      partname.getString());
#endif // COIN_DEBUG
          }
          return list->createAndAddDefaultChild();
        }
        else {
#if COIN_DEBUG
          SoDebugError::postWarning("SoBaseKit::getAnyPart",
                                    "index %d out of bounds for part \"%s\"",
                                    listIdx, partname.getString());
#endif // COIN_DEBUG
        }
      }
      else {
        return PRIVATE(kit)->instancelist[partNum]->getValue();
      }
    }
  }

  // FIXME:
  // run cleanup?, in case some node has been temporarily created while
  // searching for the part?? pederb, 2000-01-05

#if COIN_DEBUG
  if (makeifneeded) { // user probably expected part to be found, post a warning
    SoDebugError::postWarning("SoBaseKit::getAnyPart",
                              "part \"%s\" not found in %s",
                              partname.getString(),
                              this->getTypeId().getName().getString());
  }
#endif // COIN_DEBUG
  return NULL;
}

/*!
  Return path with nested SoNodeKit instances down in the catalog
  hierarchy given by \a partname.

  If the trailing part has not been made and \a makeifneeded is \c
  TRUE, make an instance of the part type and insert into the catalog,
  as done in setAnyPart().

  If \a leafcheck is \c TRUE, ignore non-leaf catalog node entries. If
  \a publiccheck is \c TRUE, ignore private catalog entries.

  \a pathtoextend is a path through the nodekit instance catalog
  hierarchy, where we should pick up and continue to create the path
  from where \a pathtoextend terminates. If \a pathtoextend is \c
  NULL, we simply start at the "this" top level node.

  Returns \c NULL on failure, for any of the possible reasons
  described above (part ends in non-leaf or private catalog entry,
  part is not syntactically valid or refers to non-existing catalog
  entries).
*/
SoNodeKitPath *
SoBaseKit::createPathToAnyPart(const SbName & partname, SbBool makeifneeded,
                               SbBool leafcheck, SbBool publiccheck,
                               const SoPath * pathtoextend)
{
  SoFullPath * path;
  if (pathtoextend) {
    path = (SoFullPath *)pathtoextend->copy();
    path->ref();
    // pop off nodes beyond this kit node
    if (path->containsNode(this)) while (path->getTail() != this && path->getLength()) path->pop();
    else if (path->getLength()) {
      SoNode * node = path->getTail();
      if (!node->getChildren() || node->getChildren()->find(this) < 0) {
#if COIN_DEBUG
        SoDebugError::postWarning("SoBaseKit::createPathToAnyPart",
                                  "pathtoextend is illegal");
#endif // COIN_DEBUG
        path->unref();
        return NULL;
      }
      path->append(this); // this should be safe now
    }
  }
  else {
    path = (SoFullPath *)new SoPath(this);
    path->ref();
  }

  SoBaseKit * kit = this;
  int partNum;
  SbBool isList;
  int listIdx;

  if (SoBaseKit::findPart(SbString(partname.getString()), kit, partNum,
                          isList, listIdx, makeifneeded, path)) {
    const SoNodekitCatalog * catalog = kit->getNodekitCatalog();
    if ((leafcheck && ! catalog->isLeaf(partNum)) ||
        (publiccheck && ! catalog->isPublic(partNum))) {
      path->unref();
      return NULL;
    }

    SoNode * node = PRIVATE(kit)->instancelist[partNum]->getValue();
    if (node) {
      path->append(node);
      if (isList) {
        SoNodeKitListPart * list = (SoNodeKitListPart *)node;
        int numlistchildren = list->getNumChildren();
        if (listIdx < 0 || listIdx > numlistchildren || (!makeifneeded && listIdx == numlistchildren)) {
#if COIN_DEBUG
          SoDebugError::postWarning("SoBaseKit::createPathToAnyPart",
                                    "index %d out of bounds for part \"%s\"",
                                    listIdx, partname.getString());
#endif // COIN_DEBUG
          path->unref();
          return NULL;
        }
        else if (listIdx == numlistchildren) {
          if (!list->canCreateDefaultChild()) {
#if COIN_DEBUG
            SoDebugError::postWarning("SoBaseKit::createPathToAnyPart",
                                      "Unable to create default child for list-part \"%s\"",
                                      partname.getString());
#endif //COIN_DEBUG

          }
          else {
            SoNode * newnode = list->createAndAddDefaultChild();
            path->append(list->getContainerNode());
            path->append(newnode);
          }
        }
        else {
          path->append(list->getContainerNode());
          path->append(list->getChild(listIdx));
        }
      }
      path->unrefNoDelete();
      return (SoNodeKitPath *)path;
    }
  }
  path->unref();
  return NULL;
}

/*!
  \COININTERNAL
*/
SbBool
SoBaseKit::setAnyPart(const SbName & partname, SoNode * from, SbBool anypart)
{
  SoBaseKit * kit = this;
  int partNum;
  SbBool isList;
  int listIdx;

  SbString partstring(partname.getString());

  // FIXME: findPart() really needs another parameter, since we need
  // to create intermediate parts, but not the leaf part. For now we
  // just supply makeifneeded = TRUE, and then immediately overwrite
  // the part here. pederb, 2004-06-07
  if (SoBaseKit::findPart(partstring, kit, partNum, isList, listIdx, TRUE, NULL, TRUE)) {
    if (anypart || kit->getNodekitCatalog()->isPublic(partNum)) {
      if (isList) {
        SoNode * partnode = PRIVATE(kit)->instancelist[partNum]->getValue();
        if (partnode) {
          assert(partnode->isOfType(SoNodeKitListPart::getClassTypeId()));
          SoNodeKitListPart * list = (SoNodeKitListPart *) partnode;
          if (listIdx >= 0 && listIdx <= list->getNumChildren()) {
            if (listIdx == list->getNumChildren())
              list->addChild(from);
            else
              list->replaceChild(listIdx, from);
            return TRUE;
          }
          else {
#if COIN_DEBUG
            SoDebugError::postWarning("SoBaseKit::setAnyPart",
                                      "index %d out of bounds for part \"%s\"",
                                      listIdx, partname.getString());
#endif // COIN_DEBUG
          }
        }
      }
      else {
        return kit->setPart(partNum, from);
      }
    }
    else {
#if COIN_DEBUG
      SoDebugError::postWarning("SoBaseKit::setAnyPart",
                                "attempted to set non-public part \"%s\"",
                                partname.getString());
#endif // COIN_DEBUG
    }
  }
#if COIN_DEBUG
  else {
    SoDebugError::postWarning("SoBaseKit::setAnyPart",
                              "part '%s' not found in %s",
                              partname.getString(),
                              this->getTypeId().getName().getString());
  }
#endif // COIN_DEBUG

  // FIXME:
  // run cleanup, in case some node has been temporarily created while
  // searching for the part?? pederb, 2000-01-05
  return FALSE;
}

/*!
  Not part of the Coin API.

  It is supposed to create the SoNodekitParts class instance. Since
  this class can only be used by SoBaseKit (all members are private,
  with SoBaseKit as friend), we decided to not support this class, and
  solve the problem of recording which parts are created in an
  alternative way.
*/
void
SoBaseKit::createNodekitPartsList(void)
{
  assert(0 &&
         "SoBaseKit::createNodekitPartsList() should not be used with Coin");
}

/*!
  Replaces the createNodekitPartsList() method.

  Sets up the list of SoSFNode fields with node pointers to the
  instances in our catalog.
*/
void
SoBaseKit::createFieldList(void)
{
  // FIXME:
  // is there any way to make sure this code is only run once, and in
  // the top level constructor. pederb, 2000-01-06
  //
  const SoNodekitCatalog * catalog = this->getNodekitCatalog();
  // only do this if the catalog has been created
  if (catalog) {
    PRIVATE(this)->instancelist.truncate(0);
    PRIVATE(this)->instancelist.append(NULL); // first catalog entry is "this"
    for (int i = 1; i < catalog->getNumEntries(); i++) {
      PRIVATE(this)->instancelist.append((SoSFNode *)this->getField(catalog->getName(i)));
      assert(PRIVATE(this)->instancelist[i] != NULL);
    }
  }
}

/*!
  \COININTERNAL
*/
void
SoBaseKit::createDefaultParts(void)
{
  // FIXME:
  // is there any way to make sure this code is only run once, and in
  // the top level constructor. pederb, 2000-01-06
  //
  const SoNodekitCatalog * catalog = this->getNodekitCatalog();
  // only do this if the catalog has been created
  if (catalog) {
    for (int i = 1; i < PRIVATE(this)->instancelist.getLength(); i++) {
      if (!catalog->isNullByDefault(i)) {
        SoNode * old = PRIVATE(this)->instancelist[i]->getValue();
        if ((old == NULL || ! old->isOfType(catalog->getDefaultType(i)) )) {
          this->makePart(i);
          PRIVATE(this)->instancelist[i]->setDefault(TRUE);
        }
      }
    }
  }
}

/*!
  In Open Inventor, this method returns a pointer to a private class.
  It will always return \c NULL in Coin.

  \sa createNodekitPartsList()
*/
const SoNodekitParts *
SoBaseKit::getNodekitPartsList(void) const
{
  assert(0 &&
         "SoBaseKit::getNodekitPartsList() obsoleted in Coin");
  return NULL;
}

/*!
  \COININTERNAL
*/
const SbList<SoSFNode*> &
SoBaseKit::getCatalogInstances(void) const
{
//    return this->fieldList;
  return PRIVATE(this)->instancelist;
}

/*!
  Obsoleted from the API in Coin.
*/
void
SoBaseKit::catalogError(void)
{
  COIN_OBSOLETED();
}

// Note: the following documentation for setUpConnections() will also
// be visible for subclass nodekits and draggers, so keep it general.
/*!
  Sets up all internal connections for instances of this class.

  (This method will usually not be of interest to the application
  programmer, unless you want to extend the library with new custom
  nodekits or dragger classes.  If so, see the SoBaseKit class
  documentation.)
*/
SbBool
SoBaseKit::setUpConnections(SbBool COIN_UNUSED_ARG(onoff), SbBool COIN_UNUSED_ARG(doitalways))
{
  return this->connectionsSetUp;
}

// doc in super
SbBool
SoBaseKit::readInstance(SoInput * in, unsigned short COIN_UNUSED_ARG(flags))
{
  int i;

  SbBool oldnotify = this->enableNotify(FALSE);
  SbBool oldsetup = this->setUpConnections(FALSE);

  // store old part values to find which parts are read
  SoNodeList nodelist;
  SbList <SbBool> defaultlist;

  const SoNodekitCatalog * cat = this->getNodekitCatalog();

  // Dummy first element to get indices to match instancelist (where
  // the dummy "this" catalog entry is first).
  nodelist.append(NULL);
  defaultlist.append(FALSE);

  // copy all parts into nodelist, and then set all parts to NULL
  // and default before reading
  for (i = 1; i < PRIVATE(this)->instancelist.getLength(); i++) {
    nodelist.append(PRIVATE(this)->instancelist[i]->getValue());
    defaultlist.append(PRIVATE(this)->instancelist[i]->isDefault());
    PRIVATE(this)->instancelist[i]->setValue(NULL);
    PRIVATE(this)->instancelist[i]->setDefault(TRUE);
  }

  // reset the node kit by removing all children. We will restore it
  // by setting the parts again later
  this->getChildren()->truncate(0);

  // actually read the nodekit.
  // Use readUnknownFields instead to read fields not part of catalog
  // SbBool ret = inherited::readInstance(in, flags);

  // Fields that's not part of catalog is read as a SoSFNode, and stored
  // in unknownfielddata. Later they'll be put in nodekit using setAnyPart.
  SbBool ret = TRUE;
  SoFieldData * unknownfielddata = new SoFieldData;
  if (!PRIVATE(this)->readUnknownFields(in, unknownfielddata))
    ret = FALSE;

  if (ret) {
    // loop through fields and copy the read parts into nodelist
    for (i = 1; i < PRIVATE(this)->instancelist.getLength(); i++) {
      if (!PRIVATE(this)->instancelist[i]->isDefault()) { // we've read a part
        nodelist.set(i, PRIVATE(this)->instancelist[i]->getValue());
        defaultlist[i] = FALSE;
        // set to NULL again so that setPart() will not get confused
        PRIVATE(this)->instancelist[i]->setValue(NULL);
      }
    }

    // restore the nodekit with all old and read parts
    for (i = 1; i < PRIVATE(this)->instancelist.getLength(); i++) {
      if (!cat->isLeaf(i) && nodelist[i]) {
        // if not leaf, remove all children. They will be re-added
        // later when the children parts are set.
        assert(nodelist[i]->isOfType(SoGroup::getClassTypeId()));
        SoGroup * g = (SoGroup*) nodelist[i];
        g->removeAllChildren();
      }
      this->setPart(i, nodelist[i]);
      PRIVATE(this)->instancelist[i]->setDefault(defaultlist[i]);
    }

    // put the unknown fields into nodekit using setAnyPart
    SbName partname;
    SoNode * pnode;
    SoSFNode * pfield;
    for (i = 0; i < unknownfielddata->getNumFields(); i++) {
      partname = unknownfielddata->getFieldName(i);
      pfield = (SoSFNode *) unknownfielddata->getField(this, i);
      pnode = pfield->getValue();
      this->setAnyPart(partname, pnode);
    }
  }

  delete unknownfielddata;

  (void) this->setUpConnections(oldsetup);
  (void) this->enableNotify(oldnotify);

  return ret;
}

//
// recurse until not possible to split string any more, and return information
// about part and the kit the part is found in.
// Remember to set kit=this before calling this method, also remember that
// kit might change during this search.
//
// compoundname parts are created during this search, so it might be necessary
// to do a nodekit cleanup if part is not public, or if part is set to NULL.
//
//
// if path != NULL, kit-nodes will be appended to the path during the search
// The actual part is not added to the path. The head of the path should
// be set to the kit-node performing the search.
//
SbBool
SoBaseKit::findPart(const SbString & partname, SoBaseKit *& kit, int & partnum,
                    SbBool & islist, int & listidx, const SbBool makeifneeded,
                    SoPath * path, const SbBool recsearch)
{
  // BNF:
  //
  // partname = singlename | compoundname
  // compoundname = singlename | compoundname.singlename
  // singlename = singlepartname | singlelistelementname
  // singlelistelementname = singlelistname[idx]
  //
  // singlepartname is name of a part ("ordinary", nodekit or list)
  // singlelistname is name of a part which is a list
  // idx is an integer value

  if (partname == "this") {
    islist = FALSE;
    partnum = 0;
    return TRUE;
  }

  const char * stringptr = partname.getString();
  const char * periodptr = strchr(stringptr, '.'); // find first period
  const char * startbracket = strchr(stringptr, '[');

  if (periodptr && (startbracket > periodptr))
    startbracket = NULL; // will handle later

  islist = FALSE; // set to FALSE first
  SbString firstpartname;
  if (startbracket) { // get index
    long int listindex = strtol(startbracket+1, NULL, 10);
    if (listindex == LONG_MIN || listindex == LONG_MAX) {
#if COIN_DEBUG
      SoDebugError::postWarning("SoBaseKit::findPart",
                                "list index not properly specified");
#endif // COIN_DEBUG
      return FALSE;
    }
    const ptrdiff_t endidx = startbracket - stringptr - 1;
    firstpartname = partname.getSubString(0, (int)endidx);
    listidx = (int) listindex;
    islist = TRUE;
  }
  else if (periodptr) {
    const ptrdiff_t endidx = periodptr - stringptr - 1;
    firstpartname = partname.getSubString(0, (int)endidx);
  }
  else firstpartname = partname;

  partnum = kit->getNodekitCatalog()->getPartNumber(firstpartname);
  if (partnum == SO_CATALOG_NAME_NOT_FOUND) {
    if (recsearch) { // search leaf nodekits for this part?
      SoBaseKit * orgkit = kit;
      assert(path == NULL); // should not do recsearch when creating path
      const SoNodekitCatalog * catalog = orgkit->getNodekitCatalog();
      for (int i = 1; i < PRIVATE(orgkit)->instancelist.getLength(); i++) {
        if (catalog->isLeaf(i) &&
            catalog->getType(i).isDerivedFrom(SoBaseKit::getClassTypeId())) {
          kit = (SoBaseKit *)PRIVATE(orgkit)->instancelist[i]->getValue();
          SbBool didexist = kit != NULL;
          if (!didexist) {
            if (!makeifneeded) continue;
            orgkit->makePart(i);
            kit = (SoBaseKit *)PRIVATE(orgkit)->instancelist[i]->getValue();
          }
          if (SoBaseKit::findPart(partname, kit, partnum, islist, listidx,
                                  makeifneeded, path, recsearch)) {
            return TRUE;
          }
          else if (!didexist) {
            // we created this part, remove it
            orgkit->setPart(i, NULL);
          }
        }
      }
      kit = orgkit; // return with an error in this kit
    }
    // nope, not found
    return FALSE;
  }

  assert(partnum < PRIVATE(kit)->instancelist.getLength());
  SoSFNode * nodefield = PRIVATE(kit)->instancelist[partnum];
  assert(nodefield);

  if (makeifneeded && nodefield->getValue() == NULL) {
    kit->makePart(partnum);
  }

  if (path) {
    const SoNodekitCatalog * catalog = kit->getNodekitCatalog();
    SbList <SoNode*> nodestopart;
    int parent = catalog->getParentPartNumber(partnum);
    while (parent > 0) {
      SoNode * node = PRIVATE(kit)->instancelist[parent]->getValue();
      if (node == NULL) {
        assert(makeifneeded == FALSE);
        break;
      }
      nodestopart.push(node);
      parent = catalog->getParentPartNumber(parent);
    }
    assert(parent == 0 || !makeifneeded);
    while (nodestopart.getLength()) {
      SoNode * node = nodestopart.pop();
      path->append(node);
    }
  }

  if (periodptr == NULL) {
    // singlename or singlelistname found, do not recurse any more
    return TRUE; // all info has been found, just return TRUE
  }
  else { // recurse
    SoNode * node = nodefield->getValue();
    if (node == NULL) return FALSE;
    const ptrdiff_t startidx = periodptr - stringptr + 1;
    SbString newpartname = partname.getSubString((int)startidx);
    if (islist) {
      SoNodeKitListPart * list = (SoNodeKitListPart *) node;
      int numlistchildren = list->getNumChildren();
      if (listidx < 0 || listidx > numlistchildren || (!makeifneeded && listidx == numlistchildren)) {
#if COIN_DEBUG
        SoDebugError::postWarning("SoBaseKit::findPart",
                                  "index %d out of bounds for part \"%s\"",
                                  listidx,
                                  firstpartname.getString());
#endif // COIN_DEBUG
        return FALSE;
      }
      else if (listidx == numlistchildren) {
        (void) list->createAndAddDefaultChild();
      }
      SoNode * partnode = list->getChild(listidx);
      assert(partnode && partnode->isOfType(SoBaseKit::getClassTypeId()));
      kit = (SoBaseKit *)partnode;

      if (path) {
        path->append(list);
        path->append(list->getContainerNode());
      }
    }
    else {
      assert(node->isOfType(SoBaseKit::getClassTypeId()));
      kit = (SoBaseKit *)node;
    }
    if (path) path->append(kit);
    return SoBaseKit::findPart(newpartname, kit, partnum, islist,
                               listidx, makeifneeded, path, recsearch);
  }
}

//
// makes part, makes sure node is connected in the scene
//
SbBool
SoBaseKit::makePart(const int partnum)
{
  assert(partnum > 0 && partnum < PRIVATE(this)->instancelist.getLength());
  const SoNodekitCatalog * catalog = this->getNodekitCatalog();
  assert(catalog);

  SoNode * node = (SoNode *)catalog->getDefaultType(partnum).createInstance();
  if (catalog->isList(partnum)) {
    SoNodeKitListPart * list = (SoNodeKitListPart *) node;
    if (catalog->getListContainerType(partnum) != SoGroup::getClassTypeId()) {
      list->setContainerType(catalog->getListContainerType(partnum));
    }
    const SoTypeList & typelist = catalog->getListItemTypes(partnum);
    for (int i = 0; i < typelist.getLength(); i++) {
      list->addChildType(typelist[i]);
    }
    list->lockTypes();
  }
  return this->setPart(partnum, node);
}

/*!
  Sets parts, updates nodekit scene graph, and makes sure graph is
  valid with respect to right siblings and parent.  This method is
  virtual to enable subclasses to detect when a part changes value.

  This method is not part of the original SGI Open Inventor API, but
  is an extension specific to Coin.
*/
SbBool
SoBaseKit::setPart(const int partnum, SoNode * node)
{
  assert(partnum > 0 && partnum < PRIVATE(this)->instancelist.getLength());
  const SoNodekitCatalog * catalog = this->getNodekitCatalog();
  assert(catalog);

  if (node && !node->getTypeId().isDerivedFrom(catalog->getType(partnum))) {
#if COIN_DEBUG
    SoDebugError::postWarning("SoBaseKit::setPart",
                              "Attempted to set part \"%s\" "
                              "to wrong type. Expected \"%s\", got \"%s\"",
                              catalog->getName(partnum).getString(),
                              catalog->getType(partnum).getName().getString(),
                              node->getTypeId().getName().getString());
#endif // COIN_DEBUG
    return FALSE;
  }
  int parentIdx = catalog->getParentPartNumber(partnum);
  assert(parentIdx >= 0 && parentIdx < PRIVATE(this)->instancelist.getLength());
  SoNode * parent = NULL;
  if (parentIdx == 0) parent = this;
  else parent = PRIVATE(this)->instancelist[parentIdx]->getValue();
  if (parent == NULL) {
    this->makePart(parentIdx);
    parent = PRIVATE(this)->instancelist[parentIdx]->getValue();
  }
  assert(parent != NULL);
  SoChildList * childlist = parent->getChildren();
  assert(childlist != NULL);

  // if parent is a node derived from SoGroup, use the SoGroup access
  // functions to add/remove/insert children instead of SoChildList
  // directly. This is needed for VRML group nodes to work properly
  // inside node kits. pederb, 2004-06-23
  SoGroup * parentgroup = NULL;
  if (parent->isOfType(SoGroup::getClassTypeId())) {
    parentgroup = (SoGroup*) parent;
  }

  SoNode * oldnode = PRIVATE(this)->instancelist[partnum]->getValue();
  if (oldnode == node) return TRUE; // part is already inserted

  if (childlist->find(node) >= 0) {
    // FIXME: should really allow this, but since it's a bit complex
    // (we need to somehow keep better track of which SoGroup child
    // indices belong to which catalog parts), we just disallow it for
    // now. 20020808 mortene.
    SoDebugError::postWarning("SoBaseKit::setPart",
                              "Node pointer (%p, '%s', '%s') is already used under the same group node in the catalog "
                              "as a child of part '%s' -- this is not allowed",
                              node,
                              node->getName().getString(),
                              node->getTypeId().getName().getString(),
                              catalog->getName(parentIdx).getString());
    return FALSE;
  }

  if (oldnode != NULL) { // part exists, replace
    int oldIdx = childlist->find(oldnode);
    assert(oldIdx >= 0);

    if (parentgroup) {
      if (node) parentgroup->replaceChild(oldIdx, node);
      else parentgroup->removeChild(oldIdx);
    }
    else {
      if (node) childlist->set(oldIdx, node);
      else childlist->remove(oldIdx);
    }
  }
  else if (node) { // find where to insert in parent childlist
    int rightSibling = this->getRightSiblingIndex(partnum);
    if (rightSibling >= 0) { // part has right sibling, insert before
      int idx = childlist->find(PRIVATE(this)->instancelist[rightSibling]->getValue());
      assert(idx >= 0);
      if (parentgroup) {
        parentgroup->insertChild(node, idx);
      }
      else {
        childlist->insert(node, idx);
      }
    }
    else {
      if (parentgroup) {
        parentgroup->addChild(node);
      }
      else {
        childlist->append(node);
      }
    }
  }

  // set part field value
  PRIVATE(this)->instancelist[partnum]->setValue(node);
  return TRUE;
}

//
// returns part number of existing right sibling or -1 if none exists
//
int
SoBaseKit::getRightSiblingIndex(const int partnum)
{
  assert(partnum > 0 && partnum < PRIVATE(this)->instancelist.getLength());
  const SoNodekitCatalog * catalog = this->getNodekitCatalog();

  int sibling = catalog->getRightSiblingPartNumber(partnum);

  // iterate until no more siblings or until we find an existing one
  while (sibling >= 0 && PRIVATE(this)->instancelist[sibling]->getValue() == NULL) {
    sibling = catalog->getRightSiblingPartNumber(sibling);
  }
  return sibling;
}

//
// Searches the field list to find of a node is in this kit.
// Returns catalog index, -1 if not found
//
// parentnum is checked if >= 0
//
int
SoBaseKit::findNodeInThisKit(SoNode * node, const int parentnum) const
{
  const SoNodekitCatalog * catalog = this->getNodekitCatalog();
  if (node == (SoNode *)this) return 0;
  int n = PRIVATE(this)->instancelist.getLength();
  for (int i = 1; i < n; i++) {
    if (PRIVATE(this)->instancelist[i]->getValue() == node &&
        (parentnum < 0 || catalog->getParentPartNumber(i) == parentnum))
      return i;
  }
  return -1;
}

// ******* methods in SoBaseKitP are below ******************************

//
// copy the fields in kit into a new fielddata. This is done to get
// the correct write order: non-part fields first, then leaf parts,
// then non-leaf parts.
//
void
SoBaseKitP::createWriteData(void)
{
  this->writedata = new SoFieldData;
  const SoNodekitCatalog * catalog = this->kit->getNodekitCatalog();
  const SoFieldData * fielddata = kit->getFieldData();

  int n = fielddata->getNumFields();
  for (int pass = 0; pass < 3; pass++) {
    for (int i = 0; i < n; i++) {
      int part = catalog->getPartNumber(fielddata->getFieldName(i));
      // NB: earlier (before 2003-03-26) we did not write private
      // parts.  However, several users have reported that SGI/TGS
      // Inventor do this so we have to write them too.
      // pederb, 2003-03-26
      if ((pass == 0 && part < 0) ||
          (pass == 1 && part > 0 && catalog->isLeaf(part)) ||
          (pass == 2 && part > 0 && !catalog->isLeaf(part))) {
        this->writedata->addField(this->kit,
                                  fielddata->getFieldName(i).getString(),
                                  fielddata->getField(this->kit, i));
      }
    }
  }
}

//
// test if parent part of a part is going to write, and if so
// write part even if isDefault()
//
void
SoBaseKitP::testParentWrite(void)
{
  const SoNodekitCatalog * catalog = this->kit->getNodekitCatalog();
  int n = this->instancelist.getLength();
  for (int i = 1; i < n; i++) {
    SoSFNode * field = this->instancelist[i];
    if (field->isDefault()) { // we might not write
      SoNode * node = field->getValue();
      // don't write if NULL, of course
      if (node) {
        int parent = catalog->getParentPartNumber(i);
        if (parent > 0) {
          assert(this->writedata);
          SbName dummy;
          SoNode * parentnode = this->instancelist[parent]->getValue();
          // we must write if parent is going to write
          if (parentnode &&
              !this->instancelist[parent]->isDefault()) {
            field->setDefault(FALSE);
          }
        }
      }
    }
  }
}

// Copy parts into 'partlist'. All parts have already been copied, but
// we need to update the parts that have a parent as a part, since the
// part node has already been copied by the parent, and we need to use
// that child node pointer, not the copied part.
void
SoBaseKitP::copyParts(const SoBaseKit * srckit, SbList <SoNode*> & partlist,
                      const SbBool COIN_UNUSED_ARG(copyconnections))
{
  int i;
  const int n = this->instancelist.getLength();
  const SoNodekitCatalog * catalog = this->kit->getNodekitCatalog();

  // convenient reference
  const SbList <SoSFNode*> & srcfields = srckit->getCatalogInstances();

  // copy parts that do not have a parent as a part
  for (i = 1; i < n; i++) {
    SoNode * dstnode = this->instancelist[i]->getValue();
    if (dstnode && catalog->getParentPartNumber(i) == 0) {
      SoNode * srcnode = srcfields[i]->getValue();
      assert(dstnode != srcnode);
      assert(srcnode != NULL);
      assert(srcnode->getTypeId() == dstnode->getTypeId());
      srcnode->assertAlive();
      dstnode->assertAlive();
      // the node has been copied since we called
      // SoNode::copyContents() . We just need to store the pointer
      dstnode->ref(); // ref before inserting into list
      if (partlist[i]) partlist[i]->unref();
      partlist[i] = dstnode;
    }
  }
  // copy parts where parent is a part. These parts will already
  // have been copied, but we need to figure out the parent part node,
  // and use the correct child node as the part node instead of the
  // already copied part node.
  for (i = 1; i < n; i++) {
    int parent = catalog->getParentPartNumber(i);
    if (parent > 0 && this->instancelist[i]->getValue()) {
      SoNode * srcgroup = srcfields[parent]->getValue();
      assert(srcgroup);
      SoNode * dstgroup = partlist[parent];
      assert(dstgroup);
      assert(dstgroup->getChildren());
      assert(srcgroup->getChildren());

      // find child index in src kit
      int childidx = srcgroup->getChildren()->find(srcfields[i]->getValue());
      assert(childidx >= 0);

      // use the already copied child as part node
      assert(childidx < dstgroup->getChildren()->getLength());
      SoNode * child = (*(dstgroup->getChildren()))[childidx];
      child->ref(); // ref before inserting
      if (partlist[i]) partlist[i]->unref(); // unref old node in list
      partlist[i] = child;
    }
  }
}

void
SoBaseKitP::setParts(SbList <SoNode*> partlist, const SbBool leafparts)
{
  const int n = this->instancelist.getLength();
  const SoNodekitCatalog * catalog = this->kit->getNodekitCatalog();

  for (int i = 1; i < n; i++) {
    SoNode * node = partlist[i];
    if (node) {
      SbBool leaftst = catalog->isLeaf(i);
      if (leaftst == leafparts) { // correct pass ?
        if (!leaftst) {
          // if it is not a leaf, remove children as the correct children
          // will be added  when children parts are set.
          assert(node->getChildren());
          node->getChildren()->truncate(0);
        }
        this->kit->setPart(i, node);
      }
    }
  }
}

//
// Adds a SoNodekitDetail to the picked point. path should
// contain this kit.
//
void
SoBaseKitP::addKitDetail(SoFullPath * path, SoPickedPoint * pp)
{
  const SoNodekitCatalog * catalog = this->kit->getNodekitCatalog();

  assert(path->findNode(this->kit) >= 0);

  const int n = path->getLength();
  for (int i = path->findNode(this->kit) + 1; i < n; i++) {
    SoNode * node = path->getNode(i);
    int idx = this->kit->findNodeInThisKit(node, -1);
    if (idx > 0 && catalog->isLeaf(idx)) {
      SoNodeKitDetail * detail = new SoNodeKitDetail;
      detail->setNodeKit(this->kit);
      detail->setPart(node);
      SbString partname(catalog->getName(idx));
      // check if node is a SoNodeKitListPart, and if the
      // path extends into the children. Supply index in partname
      // if this is the case.
      if (node->isOfType(SoNodeKitListPart::getClassTypeId()) &&
          path->getLength() >= i + 2) {
        SbString str;
        str.sprintf("%s[%d]",
                    partname.getString(),
                    path->getIndex(i+2));
        partname = SbName(str.getString());
      }
      detail->setPartName(partname);
      pp->setDetail(detail, this->kit);
      // finished
      break;
    }
  }
}

//  Reading in parts of nested nodekits does not allow certain shortcuts
//  that are specified by the Inventor Mentor. The Mentor specifies that
//  within nested nodekits intermediary kits can be left out and will be
//  created automatically. Reported by Gerhard Reitmayr.
SbBool 
SoBaseKitP::readUnknownFields(SoInput *in, SoFieldData *&unknownfielddata)
{
  const SoFieldData * fd = PUBLIC(this)->getFieldData();

  // Binary format
  if (in->isBinary()) {
    SbBool notbuiltin;
    return fd->read(in, PUBLIC(this), TRUE, notbuiltin);
  }

  SbBool firstfield = TRUE;
  SbName fielddescriptionsmarker("fields");

  // ASCII format
  // keep reading fields until we hit close bracket
  while (TRUE) {
    // read first character - if none, EOF
    char c;
    if (!in->read(c))
      return FALSE;
    in->putBack(c);

    if (c == '}')
      return TRUE;

    // read fieldname with no identifier, to be able to read names like
    // appearance.material
    SbName fieldname;
    if (!in->read(fieldname, FALSE))
      return TRUE;
    
    // if this is the first field we try to read, it might be the
    // field descriptions for extension node kits. Detect and read.
    if (firstfield) {
      firstfield = FALSE;
      if (fieldname == fielddescriptionsmarker) {
        if (!fd->readFieldDescriptions(in, PUBLIC(this), 0, FALSE)) {
          return FALSE;
        }
        continue; // read next field
      }
    }

    // try to read data into one of the fields in this nodekit first.
    // SoFieldData::read() will return TRUE and set foundname to FALSE
    // if the field isn't part of the node(kit)
    SbBool foundname;
    if (!fd->read(in, PUBLIC(this), fieldname, foundname))
      return FALSE;
    
    if (!foundname) {
      // add a node pointer field with this name to the unknownFieldData,
      // and read it
      unknownfielddata->addField(PUBLIC(this), fieldname.getString(),
                                 new SoSFNode);
      if (!unknownfielddata->read(in, PUBLIC(this), fieldname, foundname))
        return FALSE;
    }
  }
  // Will never be reached, but functions with a return value other than 
  // void must return *something* by default. At least gcc-4.0.0
  // (Apple snapshot 20041026, default in Mac OS 10.4) will warn.
  return TRUE;
}

#undef PRIVATE
#undef PUBLIC

#endif // HAVE_NODEKITS
