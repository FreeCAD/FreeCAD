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

#include <ForeignFiles/SoSTLFileKit.h>
#include "coindefs.h"

#include <Inventor/SbBasic.h>
#include <Inventor/lists/SbList.h>
#include <Inventor/errors/SoDebugError.h>
#include <Inventor/actions/SoCallbackAction.h>
#include <Inventor/actions/SoReorganizeAction.h>
#include <Inventor/nodes/SoSeparator.h>
#include <Inventor/nodes/SoShapeHints.h>
#include <Inventor/nodes/SoTexture2.h>
#include <Inventor/nodes/SoNormal.h>
#include <Inventor/nodes/SoNormalBinding.h>
#include <Inventor/nodes/SoMaterial.h>
#include <Inventor/nodes/SoMaterialBinding.h>
#include <Inventor/nodes/SoCoordinate3.h>
#include <Inventor/nodes/SoIndexedFaceSet.h>
#include <Inventor/nodes/SoInfo.h>
#include <Inventor/SbBSPTree.h>
#include <Inventor/SoPrimitiveVertex.h>

#include "steel.h"
#include "nodekits/SoSubKitP.h"


#if 0
  SoCallback         "callbackList"         SoBaseKit
  SoSeparator        "topSeparator"         SoForeignFileKit
  SoShapeHints         "shapehints"           SoSTLFileKit
  SoTexture2           "texture"              SoSTLFileKit
  SoNormalBinding      "normalbinding"        SoSTLFileKit
  SoNormal             "normals"              SoSTLFileKit
  SoMaterialBinding    "materialbinding"      SoSTLFileKit
  SoMaterial           "material"             SoSTLFileKit
  SoCoordinate3        "coordinates"          SoSTLFileKit
  SoIndexedFaceSet     "facets"               SoSTLFileKit
#endif // 0

class SoSTLFileKitP {
public:
  SoSTLFileKitP(SoSTLFileKit * pub)
  : api(pub) {
    this->data = new SbList<uint16_t>;
    this->points = new SbBSPTree;
    this->normals = new SbBSPTree;
  }
  ~SoSTLFileKitP(void) {
    delete this->data;
    delete this->points;
    delete this->normals;
  }

public:
  SoSTLFileKit * const api;

  SbList<uint16_t> * data;
  SbBSPTree * points;
  SbBSPTree * normals;

  int numfacets;
  int numvertices;
  int numnormals;
  int numsharedvertices;
  int numsharednormals;
  int numredundantfacets;
}; // SoSTLFileKitP

// *************************************************************************

/*!
  \class SoSTLFileKit SoSTLFileKit.h ForeignFiles/SoSTLFileKit.h
  \brief SoSTLFileKit is a class for using STL files with Coin.

  Class for using STL files with Coin.  You can use it to read and
  write STL files, and convert back and forth between Open Inventor
  scene graphs and SoSTLFileKits.

  STL files are 3D models intended for 3D printers, and is a format
  supported by a wide variety of computer-aided design programs.  STL
  models are, because of their intended purpose, always
  representations of solid objects.  STL is short for
  stereolithography, the process used for 3D printing.

  Ordinary STL models do not contain color information.  There are,
  however, two extensions to the binary file format for specifying
  color.  Currently neither extension is supported.  This is caused by
  lack of sample models using the extensions and will be added as soon
  as such models are found.  We have the specs on the extensions, and
  it should be pretty straight-forwards to implement, but we want to
  get it right at once since we have write support (we don't want to
  inadvertently create a third color extension ;).

  When writing STL files, certain STL model criteria are not enforced
  by SoSTLFileKit.  These are:

  - STL models should represent complete solids - it is the user's
    responsibility to give models of solid data to readScene(), and
    not readScene()'s responsibility to check the incoming data.

  - STL models should have all triangles in counterclockwise order.
    This is not enforced either.

  - STL models should reside in the positive octant of the coordinate
    space.  This is also the user's responsibility to ensure, although
    adding functionality for translating the model should be easy, so
    it might get implemented.

  Since the color extensions are not supported yet, color information
  is not collected either when converting Open Inventor scene graphs to
  SoSTLFileKits.

  \relates foreignfileformats
  \COIN_CLASS_EXTENSION
  \since Coin 3.0
*/

#define PRIVATE(obj) ((obj)->pimpl)

SO_KIT_SOURCE(SoSTLFileKit)

/*!
  \copydetails SoNode::initClass(void)
*/

void
SoSTLFileKit::initClass(void)
{
  SO_KIT_INIT_CLASS(SoSTLFileKit, SoForeignFileKit, SoForeignFileKit);

  SoType type = SoSTLFileKit::getClassTypeId();
  SoForeignFileKit::registerFileExtension(type, "stl", SoSTLFileKit::identify);
}

/*!
  Returns whether or not \a filename is identified as an STL file.
*/

SbBool
SoSTLFileKit::identify(const char * filename)
{
  assert(filename);
  stl_reader * reader = stl_reader_create(filename);
  if ( !reader ) {
    return FALSE;
  }
  stl_reader_destroy(reader);
  return TRUE;
}

/*!
  Constructor.
*/

SoSTLFileKit::SoSTLFileKit(void)
{
  PRIVATE(this) = new SoSTLFileKitP(this);

  SO_KIT_INTERNAL_CONSTRUCTOR(SoSTLFileKit);

  SO_KIT_ADD_FIELD(info, (""));
  SO_KIT_ADD_FIELD(binary, (FALSE));
  SO_KIT_ADD_FIELD(colorization, (SoSTLFileKit::GREY));

  SO_KIT_DEFINE_ENUM_VALUE(Colorization, GREY);
  SO_KIT_DEFINE_ENUM_VALUE(Colorization, MATERIALISE);
  SO_KIT_DEFINE_ENUM_VALUE(Colorization, TNO_VISICAM);

  SO_KIT_SET_SF_ENUM_TYPE(colorization, Colorization);

  SO_KIT_ADD_CATALOG_ENTRY(facets, SoIndexedFaceSet,
                           FALSE, topSeparator, \x0, FALSE);
  SO_KIT_ADD_CATALOG_ENTRY(coordinates, SoCoordinate3,
                           FALSE, topSeparator, facets, FALSE);
  SO_KIT_ADD_CATALOG_ENTRY(material, SoMaterial,
                           FALSE, topSeparator, coordinates, FALSE);
  SO_KIT_ADD_CATALOG_ENTRY(materialbinding, SoMaterialBinding,
                           FALSE, topSeparator, material, FALSE);
  SO_KIT_ADD_CATALOG_ENTRY(normals, SoNormal,
                           FALSE, topSeparator, materialbinding, FALSE);
  SO_KIT_ADD_CATALOG_ENTRY(normalbinding, SoNormalBinding,
                           FALSE, topSeparator, normals, FALSE);
  SO_KIT_ADD_CATALOG_ENTRY(texture, SoTexture2,
                           FALSE, topSeparator, normalbinding, FALSE);
  SO_KIT_ADD_CATALOG_ENTRY(shapehints, SoShapeHints,
                           FALSE, topSeparator, texture, FALSE);

  SO_KIT_INIT_INSTANCE();
}

/*!
  Destructor.
*/

SoSTLFileKit::~SoSTLFileKit(void)
{
  delete PRIVATE(this);
  PRIVATE(this) = NULL;
}

// doc in inherited class
SbBool
SoSTLFileKit::canReadFile(const char * filename) const
{
  if ( !filename ) return TRUE; // we can read STL files, in general
  return SoSTLFileKit::identify(filename);
}

/*!
  Reads in an STL file.  Both ASCII and binary files are supported.
  For binary files, the color extensions are not implemented yet.

  Returns FALSE if \a filename could not be opened or parsed
  correctly.

  \sa canReadFile
*/

SbBool
SoSTLFileKit::readFile(const char * filename)
{
  assert(filename);

  this->reset();

  stl_reader * reader = stl_reader_create(filename);
  if ( !reader ) {
    SoDebugError::postInfo("SoSTLFileKit::readFile",
                           "unable to create STL reader for '%s'.",
                           filename);
    return FALSE;
  }

  SbBool binary = (stl_reader_flags(reader) & STL_BINARY) ? TRUE : FALSE;

  SoShapeHints * hints =
    SO_GET_ANY_PART(this, "shapehints", SoShapeHints);
  hints->vertexOrdering.setValue(SoShapeHints::UNKNOWN_ORDERING);
  // what it should have been
  // hints->vertexOrdering.setValue(SoShapeHints::COUNTERCLOCKWISE);
  hints->shapeType.setValue(SoShapeHints::SOLID);
  hints->faceType.setValue(SoShapeHints::UNKNOWN_FACE_TYPE);

  SoNormalBinding * normalbinding =
    SO_GET_ANY_PART(this, "normalbinding", SoNormalBinding);
  normalbinding->value = SoNormalBinding::PER_FACE_INDEXED;

  stl_facet * facet = stl_facet_create();
  SbBool loop = TRUE, success = TRUE;
  while ( loop ) {
    const int peekval = stl_reader_peek(reader);
    if ( peekval == STL_BEGIN ) {
      // FIXME: implement proper action
    } else if ( peekval == STL_INIT_INFO ) {
      // FIXME: set info
    } else if ( peekval == STL_EXIT_INFO ) {
      // FIXME: implement proper action
    } else if ( peekval == STL_END ) {
      loop = FALSE;
    } else if ( peekval == STL_FACET ) {
      stl_real x, y, z;
      stl_reader_fill_facet(reader, facet);
      stl_facet_get_normal(facet, &x, &y, &z);
      SbVec3f normal((float) x, (float) y, (float) z);
      stl_facet_get_vertex1(facet, &x, &y, &z);
      SbVec3f vertex1((float) x, (float) y, (float) z);
      stl_facet_get_vertex2(facet, &x, &y, &z);
      SbVec3f vertex2((float) x, (float) y, (float) z);
      stl_facet_get_vertex3(facet, &x, &y, &z);
      SbVec3f vertex3((float) x, (float) y, (float) z);
      if ( normal.length() == 0.0f ) { // auto-calculate
        SbVec3f v1(vertex2-vertex1);
        SbVec3f v2(vertex3-vertex1);
        normal = v1.cross(v2);
        float len = normal.length();
        if ( len > 0 ) normal /= len;
      }
      unsigned int data = stl_facet_get_padding(facet);

      SbBool added = this->addFacet(vertex1, vertex2, vertex3, normal);

#if defined(COIN_EXTRA_DEBUG) || 1
      if ( added && binary ) {
        // binary contains padding, which might be colorization
        // colorization is not implemented yet, so therefore some debug
        // output comes here so colorized models can be detected.
        PRIVATE(this)->data->append((uint16_t) data);
        if ( data != 0 ) {
          fprintf(stderr, "facet %5d - data: %04x\n", PRIVATE(this)->numfacets - 1, data);
        }
      }
#endif // COIN_EXTRA_DEBUG
    } else if ( peekval == STL_ERROR ) {
      SoDebugError::post("SoSTLFileKit::readFile",
                         "error '%s' after %d facets, line %d.",
                         stl_reader_get_error(reader),
                         PRIVATE(this)->numfacets,
                         stl_reader_get_line_number(reader));
      loop = FALSE;
      success = FALSE;
      if (strcmp(stl_reader_get_error(reader), "premature end of file") == 0) {
        // this one we will accept though - models with missing
        // end-indicator have been found...
        success = TRUE;
      }
    }
  }

  // done - no need for the BSP trees to contain data any more
  PRIVATE(this)->points->clear();
  PRIVATE(this)->normals->clear();

  stl_facet_destroy(facet);
  stl_reader_destroy(reader);

  if ( !success ) {
    this->reset();
  } else {
    this->organizeModel();
  }
  return success;
}

// doc in inherited class
SbBool
SoSTLFileKit::canReadScene(void) const
{
  return TRUE;
}

/*!
  Converts a scene graph into an SoSTLFileKit.  Useful for creating
  STL files.

  \sa canReadScene, canWriteFile, writeFile
*/

SbBool
SoSTLFileKit::readScene(SoNode * scene)
{
  this->reset();

  scene->ref();
  SoCallbackAction cba;
  cba.addTriangleCallback(SoType::fromName("SoNode"), add_facet_cb, this);
  cba.apply(scene);
  scene->unrefNoDelete();

  // no need for the BSP trees to contain data any more
  PRIVATE(this)->points->clear();
  PRIVATE(this)->normals->clear();

  this->organizeModel();

  return TRUE;
}

SoSeparator *
SoSTLFileKit::convert()
{
  SoSeparator * sceneroot = new SoSeparator;
  sceneroot->ref();

  SoInfo * info = new SoInfo;
  info->string = "STL model data, created by Coin " COIN_VERSION ".";
  sceneroot->addChild(info);
    
  SoShapeHints * shapehints_orig =
    SO_GET_ANY_PART(this, "shapehints", SoShapeHints);
  SoShapeHints * shapehints_copy = new SoShapeHints;
  shapehints_copy->copyContents(shapehints_orig, FALSE);
  sceneroot->addChild(shapehints_copy);

  SoTexture2 * texture_orig = SO_GET_ANY_PART(this, "texture", SoTexture2);
  SoTexture2 * texture_copy = new SoTexture2;
  texture_copy->copyContents(texture_orig, FALSE);
  sceneroot->addChild(texture_copy);

  SoNormalBinding * normalbinding_orig =
    SO_GET_ANY_PART(this, "normalbinding", SoNormalBinding);
  SoNormalBinding * normalbinding_copy = new SoNormalBinding;
  normalbinding_copy->copyContents(normalbinding_orig, FALSE);
  sceneroot->addChild(normalbinding_copy);

  SoNormal * normals_orig = SO_GET_ANY_PART(this, "normals", SoNormal);
  SoNormal * normals_copy = new SoNormal;
  normals_copy->copyContents(normals_orig, FALSE);
  sceneroot->addChild(normals_copy);

  SoMaterialBinding * materialbinding_orig =
    SO_GET_ANY_PART(this, "materialbinding", SoMaterialBinding);
  SoMaterialBinding * materialbinding_copy = new SoMaterialBinding;
  materialbinding_copy->copyContents(materialbinding_orig, FALSE);
  sceneroot->addChild(materialbinding_copy);

  SoMaterial * material_orig = SO_GET_ANY_PART(this, "material", SoMaterial);
  SoMaterial * material_copy = new SoMaterial;
  material_copy->copyContents(material_orig, FALSE);
  sceneroot->addChild(material_copy);

  SoCoordinate3 * coordinates_orig =
    SO_GET_ANY_PART(this, "coordinates", SoCoordinate3);
  SoCoordinate3 * coordinates_copy = new SoCoordinate3;
  coordinates_copy->copyContents(coordinates_orig, FALSE);
  sceneroot->addChild(coordinates_copy);

  SoIndexedFaceSet * facets_orig =
    SO_GET_ANY_PART(this, "facets", SoIndexedFaceSet);
  SoIndexedFaceSet * facets_copy = new SoIndexedFaceSet;
  facets_copy->copyContents(facets_orig, FALSE);
  sceneroot->addChild(facets_copy);

  // optimize/reorganize mesh
  SoReorganizeAction ra;
  ra.apply(sceneroot);

  // FIXME: remove redundant scene graph nodes after scene reorganization

  sceneroot->unrefNoDelete();
  return sceneroot;
}

// doc in inherited class
SbBool
SoSTLFileKit::canWriteFile(const char * filename) const
{
  return inherited::canWriteFile(filename);
}

/*!
  Writes the STL model to an STL file.

  \sa binary, info, canWriteFile, canReadScene
*/

SbBool
SoSTLFileKit::writeFile(const char * filename)
{
  unsigned int flags = 0;
  if ( this->binary.getValue() ) {
    flags |= STL_BINARY;
    // set up flags for colorization if wanted
  }

  stl_writer * writer = stl_writer_create(filename, flags);
  if ( !writer ) {
    return FALSE;
  }

  stl_facet * facet = stl_facet_create();
  assert(facet);
  stl_writer_set_facet(writer, facet);

  SbString infostring = this->info.getValue();
  if ( infostring.getLength() > 0 ) {
    if ( stl_writer_set_info(writer, infostring.getString()) != STL_OK ) {
      SoDebugError::post("SoSTLFileKit::writeFile",
                         "error: '%s'",
                         stl_writer_get_error(writer));
      return FALSE;
    }
  }

  this->ref();
  SoCallbackAction cba;
  cba.addTriangleCallback(SoNode::getClassTypeId(), put_facet_cb, writer);
  cba.apply(this);
  this->unrefNoDelete();

  stl_writer_destroy(writer);

  return TRUE;
}

// *************************************************************************

/*!
  Resets the STL model so it contains nothing.
*/

void
SoSTLFileKit::reset(void)
{
  PRIVATE(this)->numvertices = 0;
  PRIVATE(this)->numfacets = 0;
  PRIVATE(this)->numnormals = 0;
  PRIVATE(this)->numsharedvertices = 0;
  PRIVATE(this)->numsharednormals = 0;
  PRIVATE(this)->numredundantfacets = 0;

  PRIVATE(this)->data->truncate(0);
  PRIVATE(this)->points->clear();
  PRIVATE(this)->normals->clear();
    
  this->setAnyPart("shapehints", new SoShapeHints);
  this->setAnyPart("texture", new SoTexture2);
  this->setAnyPart("normalbinding", new SoNormalBinding);
  this->setAnyPart("normals", new SoNormal);
  this->setAnyPart("materialbinding", new SoMaterialBinding);
  this->setAnyPart("material", new SoMaterial);
  this->setAnyPart("coordinates", new SoCoordinate3);
  this->setAnyPart("facets", new SoIndexedFaceSet);

  SoNormalBinding * normalbinding =
    SO_GET_ANY_PART(this, "normalbinding", SoNormalBinding);
  normalbinding->value = SoNormalBinding::PER_FACE_INDEXED;

  SoShapeHints * shapehints =
    SO_GET_ANY_PART(this, "shapehints", SoShapeHints);
  shapehints->vertexOrdering = SoShapeHints::UNKNOWN_ORDERING;
  // proper model is
  //   shapehints->vertexOrdering = SoShapeHints::COUNTERCLOCKWISE;
  // but many models are not proper
  shapehints->shapeType = SoShapeHints::SOLID;
}

/*!
  Adds one triangle to the STL model.

  \sa reset, organizeModel
*/

SbBool
SoSTLFileKit::addFacet(const SbVec3f & v1, const SbVec3f & v2, const SbVec3f & v3, const SbVec3f & n)
{
  SoNormal * normals =
    SO_GET_ANY_PART(this, "normals", SoNormal);
  SoCoordinate3 * coordinates =
    SO_GET_ANY_PART(this, "coordinates", SoCoordinate3);
  SoIndexedFaceSet * facets =
    SO_GET_ANY_PART(this, "facets", SoIndexedFaceSet);

  // find existing indexes if any
  long v1idx = PRIVATE(this)->points->findPoint(v1), v1new = (v1idx == -1);
  long v2idx = PRIVATE(this)->points->findPoint(v2), v2new = (v2idx == -1);
  long v3idx = PRIVATE(this)->points->findPoint(v3), v3new = (v3idx == -1);
  if (!v1new) { v1idx = (long) reinterpret_cast<intptr_t>(PRIVATE(this)->points->getUserData(v1idx)); }
  if (!v2new) { v2idx = (long) reinterpret_cast<intptr_t>(PRIVATE(this)->points->getUserData(v2idx)); }
  if (!v3new) { v3idx = (long) reinterpret_cast<intptr_t>(PRIVATE(this)->points->getUserData(v3idx)); }
  long nidx = PRIVATE(this)->normals->findPoint(n);
  if (nidx != -1) { nidx = (long) reinterpret_cast<intptr_t>(PRIVATE(this)->normals->getUserData(nidx)); }

  // toss out invalid facets - facets where two or more points are in
  // the same location.  what are these - are they lines and points or
  // something?  selection?  borders?  creases?
  if ((!v1new && !v2new && (v1idx == v2idx)) ||
      (!v1new && !v3new && (v1idx == v3idx)) ||
      (!v2new && !v3new && (v2idx == v3idx)) ||
      (v1new && v2new && (v1 == v2)) ||
      (v1new && v3new && (v1 == v3)) ||
      (v2new && v3new && (v2 == v3))) {
    // the above test is optimized for using vertex indexes if
    // possible and avoid vec3f-comparisons when index-comparisons
    // should have sufficed.
    PRIVATE(this)->numredundantfacets += 1;
    return FALSE;
  }

#if 0 // disabled (O(n^2))
  // toss out redundant facets, if any...
  if (!v1new && !v2new && !v3new) {
    int count = facets->coordIndex.getNum();
    const int32_t * points = facets->coordIndex.getValues(0);
    int i;
    for (i = 0; i < count; i++) {
      if (points[i] == v1idx) {
        int beg = i - (i % 4);
        if ( ((points[beg] == v1idx) && (points[beg+1] == v2idx) &&
              (points[beg+2] == v3idx)) ||
             ((points[beg] == v2idx) && (points[beg+1] == v3idx) &&
              (points[beg+2] == v1idx)) ||
             ((points[beg] == v3idx) && (points[beg+1] == v1idx) &&
              (points[beg+2] == v2idx)) ) {
          // same vertices, same vertex ordering (we drop comparing normal)
          PRIVATE(this)->numredundantfacets += 1;
          return FALSE;
        }
      }
    }
  }
#endif

  // add facet (triangle) to faceset
  if (v1new) {
    v1idx = PRIVATE(this)->numvertices;
    coordinates->point.set1Value(v1idx, v1);
    PRIVATE(this)->points->addPoint(v1, (void*) ((uintptr_t) v1idx));
    PRIVATE(this)->numvertices++;
  } else {
    PRIVATE(this)->numsharedvertices++;
  }
  facets->coordIndex.set1Value(PRIVATE(this)->numfacets*4, v1idx);
  
  if (v2new) {
    v2idx = PRIVATE(this)->numvertices;
    coordinates->point.set1Value(v2idx, v2);
    PRIVATE(this)->points->addPoint(v2, (void*) ((uintptr_t) v2idx));
    PRIVATE(this)->numvertices++;
  } else {
    PRIVATE(this)->numsharedvertices++;
  }
  facets->coordIndex.set1Value(PRIVATE(this)->numfacets*4+1, v2idx);

  if (v3new) {
    v3idx = PRIVATE(this)->numvertices;
    coordinates->point.set1Value(v3idx, v3);
    PRIVATE(this)->points->addPoint(v3, (void*) ((uintptr_t) v3idx));
    PRIVATE(this)->numvertices++;
  } else {
    PRIVATE(this)->numsharedvertices++;
  }
  facets->coordIndex.set1Value(PRIVATE(this)->numfacets*4+2, v3idx);
  facets->coordIndex.set1Value(PRIVATE(this)->numfacets*4+3, -1);

  if (nidx == -1) {
    nidx = PRIVATE(this)->numnormals;
    normals->vector.set1Value(nidx, n);
    PRIVATE(this)->normals->addPoint(n, (void*) ((uintptr_t) nidx));
    PRIVATE(this)->numnormals++;
  } else {
    PRIVATE(this)->numsharednormals++;
  }
  facets->normalIndex.set1Value(PRIVATE(this)->numfacets, nidx);

  PRIVATE(this)->numfacets++;
  return TRUE;
}

/*!
  Should be called after the STL model is completely set up in the
  SoSTLFileKit through import from a file or from a scene graph.  The
  model will then be optimized for fast rendering.

  \sa addFacet, reset
*/

void
SoSTLFileKit::organizeModel(void)
{
#if defined(COIN_EXTRA_DEBUG)
  SoDebugError::postInfo("SoSTLFileKit::organizeModel",
                         "model data imported successfully. "
                         "%d unique vertices, %d reuses. "
                         "%d unique normals, %d reuses. "
                         "%d facets, %d redundant facets.",
                         PRIVATE(this)->numvertices,
                         PRIVATE(this)->numsharedvertices,
                         PRIVATE(this)->numnormals,
                         PRIVATE(this)->numsharednormals,
                         PRIVATE(this)->numfacets,
                         PRIVATE(this)->numredundantfacets);
#endif // COIN_EXTRA_DEBUG

  SoIndexedFaceSet * facets =
    SO_GET_ANY_PART(this, "facets", SoIndexedFaceSet);

  assert(facets->coordIndex.getNum() == PRIVATE(this)->numfacets*4);
  assert(facets->normalIndex.getNum() == (PRIVATE(this)->numfacets));

  if ( PRIVATE(this)->numfacets > 300 ) {
    // FIXME: at some number of facets, reorganization for faster
    // rendering should really be performed.
  }
}

/*!
  Helper callback for readScene(), calling addFacet() for each
  triangle in the provided scene graph.

  \sa readScene
*/

void
SoSTLFileKit::add_facet_cb(void * closure,
                           SoCallbackAction * action,
                           const SoPrimitiveVertex * v1,
                           const SoPrimitiveVertex * v2,
                           const SoPrimitiveVertex * v3)
{
  assert(closure); assert(v1); assert(v2); assert(v3);
  SoSTLFileKit * filekit = (SoSTLFileKit *) closure;
  
  const SbMatrix & mm = action->getModelMatrix();

  // move the points into world space
  SbVec3f vertex1, vertex2, vertex3;
  mm.multVecMatrix(v1->getPoint(), vertex1);
  mm.multVecMatrix(v2->getPoint(), vertex2);
  mm.multVecMatrix(v3->getPoint(), vertex3);

  // flip ordering if the current shape is CW
  if (action->getVertexOrdering() == SoShapeHints::CLOCKWISE) {
    SbVec3f tmp = vertex2;
    vertex2 = vertex3;
    vertex3 = tmp;
  }
  SbVec3f vec1(vertex2-vertex1);
  SbVec3f vec2(vertex3-vertex1);
  SbVec3f normal(vec1.cross(vec2));
  (void) normal.normalize();

  filekit->addFacet(vertex1, vertex2, vertex3, normal);
}

/*!
  Helper callback for writeFile(), writing each triangle in the STL
  model to the STL file.

  \sa writeFile
*/

void
SoSTLFileKit::put_facet_cb(void * closure,
                           SoCallbackAction * COIN_UNUSED_ARG(action),
                           const SoPrimitiveVertex * v1,
                           const SoPrimitiveVertex * v2,
                           const SoPrimitiveVertex * v3)
{
  assert(closure); assert(v1); assert(v2); assert(v3);
  stl_writer * writer = (stl_writer *) closure;

  SbVec3f vertex1(v1->getPoint());
  SbVec3f vertex2(v2->getPoint());
  SbVec3f vertex3(v3->getPoint());

  SbVec3f vec1(vertex2-vertex1);
  SbVec3f vec2(vertex3-vertex1);
  SbVec3f normal(vec1.cross(vec2));
  (void) normal.normalize();

  stl_facet * facet = stl_writer_get_facet(writer);
  assert(facet);
  stl_facet_set_vertex1(facet, vertex1[0], vertex1[1], vertex1[2]);
  stl_facet_set_vertex2(facet, vertex2[0], vertex2[1], vertex2[2]);
  stl_facet_set_vertex3(facet, vertex3[0], vertex3[1], vertex3[2]);
  stl_facet_set_normal(facet, normal[0], normal[1], normal[2]);
  stl_facet_set_padding(facet, 0);

  stl_writer_put_facet(writer, facet);
}

#undef PRIVATE
#endif // HAVE_NODEKITS
