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

//
//  3DS File Loader for Open Inventor
//
//  developed by PC John (peciva@fit.vutbr.cz)
//
//
//  Comments about 3ds files: Structure of the 3ds files is well known
//  (http://www.cyberloonies.com/3dsftk.html). However, it is often hard to
//  understand what is the information in 3ds file about and how to
//  interpret them. For example, texture coordinates are not always
//  represented by OpenGL texture coordinates, and I can't find out
//  what is their meaning.
//
//
//  All loaded models are centered around [0,0,0] and normalized to
//  size 10 by default.
//
//  If loading fails during reading 3ds file, file pointer position is
//  undefined.
//
//  By default only error messages are generated. COIN_DEBUG_3DS environment
//  variable can be used to specify amount of debug messages:
//  0 .. only error messages (default)
//  1 .. warnings that usually concerns data parsed from the 3ds file
//  2 .. print basic 3ds file structure info
//  3 .. print everything
//
//
//  TODO list:
//
//  - incomplete texture implementation - in 3ds files there is possible to
//    make materials with about 20 textures (diffuse color texture, specular
//    texture, bump-map texture,...) There is not a support for them in the
//    Inventor yet.
//
//  - Texture coordinates are not always loaded right, because 3ds uses
//    many strange mapping modes. Deeper understanding of 3ds will be needed
//    to fix this.
//
//  - Backface culling functionality is disabled because it does not work
//    right on some models. It looks like some models are CLOCKWISE and other
//    COUNTERCLOCKWISE.
//
//  - unimplemented lights
//
//  - per-vertex normals generation
//
//  - investigate the color of the default material objects
//
//  - ?environment? (ambient light, fog,...)
//
//  - ?emissiveColor? (maybe MAT_SELF_ILLUM and MAT_SELF_ILPCT chunks)
//
//  - some animations?
//
//  - public API to control the loading
//
//


#include "SoStream.h"

#include "coindefs.h"

#include <Inventor/SbPlane.h>
#include <Inventor/SbRotation.h>
#include <Inventor/nodes/SoCoordinate3.h>
#include <Inventor/nodes/SoIndexedTriangleStripSet.h>
#include <Inventor/nodes/SoMaterial.h>
#include <Inventor/nodes/SoMaterialBinding.h>
#include <Inventor/nodes/SoMatrixTransform.h>
#include <Inventor/nodes/SoNormal.h>
#include <Inventor/nodes/SoNormalBinding.h>
#include <Inventor/nodes/SoShapeHints.h>
#include <Inventor/nodes/SoSeparator.h>
#include <Inventor/nodes/SoTexture2.h>
#include <Inventor/nodes/SoTextureCoordinate2.h>
#include <Inventor/nodes/SoTextureCoordinateBinding.h>
#include <Inventor/nodes/SoTexture2Transform.h>
#include <Inventor/nodes/SoTriangleStripSet.h>
#include <Inventor/errors/SoDebugError.h>
#include <Inventor/SoInput.h>
#include <Inventor/C/tidbits.h>
#include <cstring>



#define DISABLE_BACKFACE_CULLING

#define MATNAME_LENGTH 17 // 16 + \0
#define OBJNAME_LENGTH 11 // 10 + \0


// File Header Chunks
#define M3DMAGIC    0x4D4D
#define M3D_VERSION 0x0002
#define MLIBMAGIC   0x3DAA
#define CMAGIC      0xC23D

// Major Section Chunks
#define MDATA  0x3D3D
#define KFDATA 0xB000

// Viewport Control Chunks
#define VIEWPORT_LAYOUT 0x7001
#define VIEWPORT_DATA   0x7011
#define VIEWPORT_DATA_3 0x7012
#define VIEWPORT_SIZE   0x7020


// MDATA Section Chunks
// Common Chunks
#define COLOR_F 0x0010
#define COLOR_24 0x0011
#define LIN_COLOR_24 0x0012
#define LIN_COLOR_F 0x0013
#define INT_PERCENTAGE 0x0030
#define FLOAT_PERCENTAGE 0x0031

// Section Settings Chunks
#define MESH_VERSION 0x3D3E
#define MASTER_SCALE 0x0100
#define LO_SHADOW_BIAS 0x1400
#define HI_SHADOW_BIAS 0x1410
#define SHADOW_MAP_SIZE 0x1420
#define SHADOW_SAMPLES 0x1430
#define SHADOW_RANGE 0x1440
#define SHADOW_FILTER 0x1450
#define RAY_BIAS 0x1460
#define O_CONSTS 0x1500
#define AMBIENT_LIGHT 0x2100

// Background Settings Chunks
#define BIT_MAP 0x1100
#define SOLID_BGND 0x1200
#define V_GRADIENT 0x1300
#define USE_BIT_MAP 0x1101
#define USE_SOLID_BGND 0x1201
#define USE_V_GRADIENT 0x1301

// Atmosphere Settings Chunks
#define FOG 0x2200
#define FOG_BGND 0x2210
#define LAYER_FOG 0x2302
#define DISTANCE_CUE 0x2300
#define DCUE_BGND 0x2310
#define USE_FOG 0x2201
#define USE_LAYER_FOG 0x2303
#define USE_DISTANCE_CUE 0x2301

// Viewport Chunks
#define DEFAULT_VIEW 0x3000
#define VIEW_TOP 0x3010
#define VIEW_BOTTOM 0x3020
#define VIEW_LEFT 0x3030
#define VIEW_RIGHT 0x3040
#define VIEW_FRONT 0x3050
#define VIEW_BACK 0x3060
#define VIEW_USER 0x3070
#define VIEW_CAMERA 0x3080

// Materials Chunks
#define MAT_ENTRY 0xAFFF
#define MAT_NAME 0xA000
#define MAT_AMBIENT 0xA010
#define MAT_DIFFUSE 0xA020
#define MAT_SPECULAR 0xA030
#define MAT_SHININESS 0xA040
#define MAT_SHIN2PCT 0xA041
#define MAT_TRANSPARENCY 0xA050
#define MAT_XPFALL 0xA052
#define MAT_USE_XPFALL 0xA240
#define MAT_REFBLUR 0xA053
#define MAT_SHADING 0xA100
#define MAT_USE_REFBLUR 0xA250
#define MAT_SELF_ILLUM 0xA080
#define MAT_TWO_SIDE 0xA081
#define MAT_DECAL 0xA082
#define MAT_ADDITIVE 0xA083
#define MAT_WIRE 0xA085
#define MAT_FACEMAP 0xA088
#define MAT_PHONGSOFT 0xA08C
#define MAT_WIREABS 0xA08E
#define MAT_WIRESIZE 0xA087
#define MAT_TEXMAP 0xA200
#define MAT_SXP_TEXT_DATA 0xA320
#define MAT_TEXMASK 0xA3EH
#define MAT_SXP_TEXTMASK_DATA 0xA32A
#define MAT_TEX2MAP 0xA33A
#define MAT_SXP_TEXT2_DATA 0xA321
#define MAT_TEX2MASK 0xA340H
#define MAT_SXP_TEXT2MASK_DATA 0xA32C
#define MAT_OPACMAP 0xA210
#define MAT_SXP_OPAC_DATA 0xA322
#define MAT_OPACMASK 0xA342
#define MAT_SXP_OPACMASK_DATA 0xA32E
#define MAT_BUMPMAP 0xA230
#define MAT_SXP_BUMP_DATA 0xA324
#define MAT_BUMPMASK 0xA344
#define MAT_SXP_BUMPMASK_DATA 0xA330
#define MAT_SPECMAP 0xA204
#define MAT_SXP_SPEC_DATA 0xA325
#define MAT_SPECMASK 0xA348
#define MAT_SXP_SPECMASK_DATA 0xA332
#define MAT_SHINMAP 0xA33C
#define MAT_SXP_SHIN_DATA 0xA326
#define MAT_SHINMASK 0xA346
#define MAT_SXP_SHINMASK_DATA 0xA334
#define MAT_SELFIMAP 0xA33D
#define MAT_SXP_SELFI_DATA 0xA328
#define MAT_SELFIMASK 0xA34A
#define MAT_SXP_SELFIMASK_DATA 0xA336
#define MAT_REFLMAP 0xA220
#define MAT_REFLMASK 0xA34C
#define MAT_SXP_REFLMASK_DATA 0xA338
#define MAT_ACUBIC 0xA310
#define MAT_MAPNAME 0xA300
#define MAT_MAP_TILING 0xA351
#define MAT_MAT_TEXBLUR 0xA353
#define MAT_MAP_USCALE 0xA354
#define MAT_MAP_VSCALE 0xA356
#define MAT_MAP_UOFFSET 0xA358
#define MAT_MAP_VOFFSET 0xA35A
#define MAT_MAP_ANG 0xA35C
#define MAT_MAP_COL1 0xA360
#define MAT_MAP_COL2 0xA362
#define MAT_MAP_RCOL 0xA364
#define MAT_MAP_GCOL 0xA366
#define MAT_MAP_BCOL 0xA368

// Object Chunks
#define NAMED_OBJECT 0x4000
#define N_TRI_OBJECT 0x4100
#define POINT_ARRAY 0x4110
#define POINT_FLAG_ARRAY 0x4111
#define FACE_ARRAY 0x4120
#define MSH_MAT_GROUP 0x4130
#define SMOOTH_GROUP 0x4150
#define MSH_BOXMAP 0x4190
#define TEX_VERTS 0x4140
#define MESH_MATRIX 0x4160
#define MESH_COLOR 0x4165
#define MESH_TEXTURE_INFO 0x4170
#define PROC_NAME 0x4181
#define PROC_DATA 0x4182
#define N_DIRECT_LIGHT 0x4600
#define DL_OFF 0x4620
#define DL_OUTER_RANGE 0x465A
#define DL_INNER_RANGE 0x4659
#define DL_MULTIPLIER 0x465B
#define DL_EXCLUDE 0x4654
#define DL_ATTENUATE 0x4625
#define DL_SPOTLIGHT 0x4610
#define DL_SPOT_ROLL 0x4656
#define DL_SHADOWED 0x4630
#define DL_LOCAL_SHADOW2 0x4641
#define DL_SEE_CONE 0x4650
#define DL_SPOT_RECTANGULAR 0x4651
#define DL_SPOT_ASPECT 0x4657
#define DL_SPOT_PROJECTOR 0x4653
#define DL_SPOT_OVERSHOOT 0x4652
#define DL_RAY_BIAS 0x4658
#define DL_RAYSHAD 0x4627
#define N_CAMERA 0x4700
#define CAM_SEE_CONE 0x4710
#define CAM_RANGES 0x4720
#define OBJ_HIDDEN 0x4010
#define OBJ_VIS_LOFTER 0x4011
#define OBJ_DOESNT_CAST 0x4012
#define OBJ_DONT_RCVSHADOW 0x4017
#define OBJ_MATTE 0x4013
#define OBJ_FAST 0x4014
#define OBJ_PROCEDURAL 0x4015
#define OBJ_FROZEN 0x4016




struct tagFace;
struct tagFaceGroup;
struct tagMaterial;
struct tagContext;


typedef struct tagVertex {
  SbVec3f point;
  SbVec2f texturePoint;
  SbList<tagFace*> faceList;

  SbVec3f getNormal(tagContext *con, uint16_t myIndex) const;
} Vertex;

typedef struct tagFace {
  uint16_t v1,v2,v3;
  uint16_t flags;
  tagFaceGroup *faceGroup;
  uint32_t e12,e23,e31;
  SbBool isDegenerated;

  SbVec3f getNormal(tagContext *con) const;
  float getAngle(tagContext *con, uint16_t vertexIndex) const;
  SbVec3f getWeightedNormal(tagContext *con, uint16_t vertexIndex) const;
  void init(tagContext *con, uint16_t a, uint16_t b, uint16_t c, uint16_t f);
} Face;

typedef struct tagFaceGroup {
  tagMaterial *mat;

  SbList<Face*> faceList;
  uint16_t numDegFaces;

  SbBool hasTexture2(tagContext *con);
  SoTexture2* getSoTexture2(tagContext *con);
  SbBool hasTexture2Transform(tagContext *con);
  SoTexture2Transform* getSoTexture2Transform(tagContext *con);
  SoMaterial* getSoMaterial(tagContext *con);
  SoNormal* createSoNormal(tagContext *con);

  SoCoordinate3* createSoCoordinate3_n(tagContext *con);
  SoTextureCoordinate2* createSoTextureCoordinate2_n(tagContext *con);
  SoTriangleStripSet* createSoTriStripSet_n(tagContext *con);

  SoIndexedTriangleStripSet* createSoIndexedTriStripSet_i(tagContext *con);
} FaceGroup;

// FIXME mortene: don't use "namespace"
namespace DefaultFaceGroup {
  static tagMaterial* getMaterial(tagContext *con);
  static SbBool isEmpty(tagContext *con);

  static SoMaterial* getSoMaterial(tagContext *con);
  static SoNormal* createSoNormal(tagContext *con);

  static SoCoordinate3* createSoCoordinate3_n(tagContext *con);
  static SoTriangleStripSet* createSoTriStripSet_n(tagContext *con);

  static SoIndexedTriangleStripSet* createSoIndexedTriStripSet_i(tagContext *con);
}

typedef struct tagEdge {
  SbList<Face*> faceList;
} Edge;

typedef struct tagMaterial {
  SbString name;
  SbColor ambient;
  SbColor diffuse;
  SbColor specular;
  float shininess;
  float transparency;
  SbString textureName;
  float uscale;
  float vscale;
  float uoffset;
  float voffset;
  SbBool twoSided;

  SoMaterial *matCache;
  SoTexture2 *texture2Cache;
  SoTexture2Transform *texture2TransformCache;

  void updateSoMaterial(int index, SoMaterial *m);
  SoMaterial* getSoMaterial(tagContext *con);
  SbBool hasTexture2(tagContext *con);
  SoTexture2* getSoTexture2(tagContext *con);
  SbBool hasTexture2Transform(tagContext *con);
  SoTexture2Transform* getSoTexture2Transform(tagContext *con);

  tagMaterial() : matCache(NULL), texture2Cache(NULL),
                  texture2TransformCache(NULL)  {}
  ~tagMaterial()  {
    if (matCache) matCache->unref();
    if (texture2Cache) texture2Cache->unref();
    if (texture2TransformCache) texture2TransformCache->unref();
  }
} Material;



typedef struct tagContext {
  // flags "What to load"
  int appendNormals;
  SbBool loadMaterials;
  SbBool loadTextures;
  SbBool loadObjNames;
  SbBool useIndexedTriSet;
  SbBool centerModel;

  // basic loading stuff
  SoStream &s;
  size_t stopPos;
  SoSeparator *root;
  SoSeparator *cObj;
  SbBool minMaxValid;
  float minX,maxX;
  float minY,maxY;
  float minZ,maxZ;
  char objectName[OBJNAME_LENGTH];
  int totalVertices;
  int totalFaces;

  // material stuff
  SbList<FaceGroup*> faceGroupList;
  SbList<Material*> matList;
  Material defaultMat;
  Material *cMat;
  SbColor cColor;
  float cColorFloat;
  SbBool textureCoordsFound;

  // geometry stuff
  Vertex *vertexList;
  uint16_t numVertices;
  Face *faceList;
  uint16_t numFaces;
  uint16_t numDefaultDegFaces;
  Edge *edgeList;
  uint32_t numEdges;

  // scene graph generator stuff
  SoTexture2 *genCurrentTexture;
  SoMaterial *genCurrentMaterial;
  SoTexture2Transform *genCurrentTexTransform;
  int genTwoSided;
  // multiple-time used nodes
  SoTexture2 *genEmptyTexture;
  SoTexture2Transform *genEmptyTexTransform;
  SoShapeHints *genOneSidedHints;
  SoShapeHints *genTwoSidedHints;


  SoCoordinate3* createSoCoordinate3_i(tagContext *con) const;
  SoTextureCoordinate2* createSoTextureCoordinate2_i(tagContext *con) const;

  SoTexture2* genGetEmptyTexture();
  SoTexture2Transform* genGetEmptyTexTransform();
  SoShapeHints* genGetOneSidedHints();
  SoShapeHints* genGetTwoSidedHints();

  tagContext(SoStream &stream) : s(stream), root(NULL), cObj(NULL),
      totalVertices(0), totalFaces(0),
      vertexList(NULL), faceList(NULL),
      genEmptyTexture(NULL), genEmptyTexTransform(NULL),
      genOneSidedHints(NULL), genTwoSidedHints(NULL)  {}
  ~tagContext() {
      for (int i=matList.getLength()-1; i>=1; i--)
        delete matList[i];

      if (genEmptyTexture)  genEmptyTexture->unref();
      if (genEmptyTexTransform)  genEmptyTexTransform->unref();
      if (genOneSidedHints)  genOneSidedHints->unref();
      if (genTwoSidedHints)  genTwoSidedHints->unref();

      assert(!root && !cObj && !vertexList && !faceList &&
          "You forgot to free some memory.");
  }
} Context;



#define CHUNK_DECL(_name_)  static void _name_(Context *con)

CHUNK_DECL(SkipChunk);
CHUNK_DECL(LoadNamedObject);
CHUNK_DECL(LoadNTriObject);
CHUNK_DECL(LoadPointArray);
CHUNK_DECL(LoadFaceArray);
CHUNK_DECL(LoadMshMatGroup);
CHUNK_DECL(LoadTexVerts);
CHUNK_DECL(LoadMatEntry);
CHUNK_DECL(LoadMatName);
CHUNK_DECL(LoadMatAmbient);
CHUNK_DECL(LoadMatDiffuse);
CHUNK_DECL(LoadMatSpecular);
CHUNK_DECL(LoadShininess);
CHUNK_DECL(LoadTransparency);
CHUNK_DECL(LoadMatTwoSide);
CHUNK_DECL(LoadColor24);
CHUNK_DECL(LoadLinColor24);
CHUNK_DECL(LoadIntPercentage);
CHUNK_DECL(LoadFloatPercentage);
CHUNK_DECL(LoadM3DMagic);
CHUNK_DECL(LoadM3DVersion);
CHUNK_DECL(LoadMData);
CHUNK_DECL(LoadMeshVersion);
CHUNK_DECL(LoadTexMap);
CHUNK_DECL(LoadMapName);
CHUNK_DECL(LoadMapUScale);
CHUNK_DECL(LoadMapVScale);
CHUNK_DECL(LoadMapUOffset);
CHUNK_DECL(LoadMapVOffset);
static int coin_debug_3ds();

static SbBool
read3dsFile(SoStream *in, SoSeparator *&root,
            int appendNormals, float creaseAngle,
            SbBool loadMaterials, SbBool loadTextures,
            SbBool loadObjNames, SbBool indexedTriSet,
            SbBool centerModel, float modelSize)
{
  // read the stream header
  uint16_t header;
  *in >> header;
  if (header != M3DMAGIC) {
    SoDebugError::post("read3dsFile",
                       "Bad 3ds stream: invalid header.");
    return FALSE;
  }

  // prepare Context structure
  Context con(*in);
  con.stopPos = 0;
  con.root = new SoSeparator;
  con.root->ref();
  con.minMaxValid = FALSE;

  // customize loader
  if (appendNormals >= 2)  appendNormals = 1; // per-vertex normals are
                                              // not currently supported
  con.appendNormals = appendNormals;
  con.loadMaterials = loadMaterials;
  con.loadTextures = loadTextures;
  con.loadObjNames = loadObjNames;
  con.useIndexedTriSet = indexedTriSet;
  con.centerModel = centerModel;

  // initialize materials and prepare default one
  con.matList.append(&con.defaultMat);
  // FIXME: the values for the default material are guessed one (maybe completely wrong)
  con.defaultMat.ambient = SbColor(0.6f, 0.6f, 0.6f);
  con.defaultMat.diffuse = SbColor(0.8f, 0.8f, 0.8f);
  con.defaultMat.specular = SbColor(0.f, 0.f, 0.f);
  con.defaultMat.shininess = 0.f;
  con.defaultMat.transparency = 0.f;
  con.defaultMat.twoSided = TRUE;  // FIXME: is default material double sided?
  con.defaultMat.matCache = new SoMaterial;
  con.defaultMat.matCache->ref();
  con.defaultMat.updateSoMaterial(0, con.defaultMat.matCache);
  con.cMat = 0;

  // build base scene graph

#ifdef DISABLE_BACKFACE_CULLING // put default shape hints into the scene;
          // Backface culling is set to off and two sided lighting is enabled.
  SoShapeHints *sh = new SoShapeHints;
  sh->vertexOrdering = SoShapeHints::COUNTERCLOCKWISE;
  sh->shapeType = SoShapeHints::UNKNOWN_SHAPE_TYPE;
  
  // configure creaseAngle on shape hints when normals are computed by shapes
  if (con.appendNormals == 0) {
    sh->creaseAngle = creaseAngle;
  }
  con.root->addChild(sh);
#endif

  // center model and scale it
  SoMatrixTransform *matrix = NULL;
  if (con.centerModel || modelSize != 0.f) {
    matrix = new SoMatrixTransform;
    con.root->addChild(matrix);
  }

  if (con.loadMaterials) {
    // material binding
    SoMaterialBinding *mb = new SoMaterialBinding;
    mb->value.setValue(SoMaterialBinding::OVERALL);
    con.root->addChild(mb);
  }

  if (con.appendNormals) {
    // normal binding
    SoNormalBinding *nb = new SoNormalBinding;
    if (con.appendNormals == 1)
      nb->value.setValue(SoNormalBinding::PER_PART); // normal for each strip of the set
    else
      nb->value.setValue(SoNormalBinding::PER_VERTEX);
    con.root->addChild(nb);
  }

  if (con.loadTextures) {
    SoTextureCoordinateBinding *tb = new SoTextureCoordinateBinding;
    if (con.useIndexedTriSet)
      tb->value.setValue(SoTextureCoordinateBinding::PER_VERTEX_INDEXED);
    else
      tb->value.setValue(SoTextureCoordinateBinding::PER_VERTEX);
    con.root->addChild(tb);
  }


  // read the stream
  LoadM3DMagic(&con);

  // handle errors
  if (con.s.isBad()) {
    con.root->unref();
    con.root = NULL;

    SoDebugError::post("read3dsFile",
                       "3ds loading failed.");
    return FALSE;
  }

  if (con.centerModel || modelSize != 0.f) {
    SbMatrix m;

    // center model
    if (con.centerModel)
      m.setTranslate(SbVec3f(-(con.maxX+con.minX)/2.f, -(con.maxY+con.minY)/2.f,
                             -(con.maxZ+con.minZ)/2.f));
    else
      m.makeIdentity();

    // set model size
    if (modelSize != 0.f) {
      SbMatrix m2;
      float max = SbMax(SbMax(con.maxX-con.minX, con.maxY-con.minY), con.maxZ-con.minZ);
      m2.setScale(modelSize/max);
      m.multRight(m2);
    }

    matrix->matrix.setValue(m);
  }

  // return root
  con.root->unrefNoDelete();
  root = con.root;
  con.root = NULL;

  // debug info
  if (coin_debug_3ds() >= 2)
    SoDebugError::postInfo("read3dsFile",
                           "File loading ok. Loaded %i vertices and %i faces.",
                           con.totalVertices, con.totalFaces);

  return TRUE;
}




#define CHUNK(_name_) CHUNK_DECL(_name_)

#define HEADER \
  if (con->s.isBad()) \
    return; \
 \
  uint32_t size; \
  con->s >> size;

#define FULLHEADER \
  HEADER \
  uint32_t cpos = static_cast<uint32_t>(con->s.getPos()); \
  uint32_t stopPos = cpos + size - 6;




#define READ_SUBCHUNKS(_subChunkSwitch_) \
while (con->s.getPos() < stopPos) { \
  uint16_t chid; \
  con->s >> chid; \
  if (con->s.isBad()) \
    break; \
 \
  switch (chid) { \
    _subChunkSwitch_; \
  default: \
    SkipChunk(con); \
  }; \
}





CHUNK(SkipChunk)
{
  FULLHEADER;

  // move on the position of the next chunk
  con->s.setPos(stopPos);
}



CHUNK(LoadM3DMagic)
{
  FULLHEADER;
  con->stopPos = con->s.getPos() + size - 6;

  if (coin_debug_3ds() >= 2)
    SoDebugError::postInfo("LoadM3DMagic",
                           "Loading 3ds stream (stream size: %i).", size);

  READ_SUBCHUNKS(
    case M3D_VERSION: LoadM3DVersion(con); break;
    case MDATA:       LoadMData(con); break;
    case KFDATA:      SkipChunk(con); break;
  )
}



CHUNK(LoadM3DVersion)
{
  HEADER;

  int32_t version;
  con->s >> version;

  if (version != 3 && coin_debug_3ds() >= 1)
    SoDebugError::postWarning("LoadM3DVersion",
                              "Non-standard 3ds format version: %i.", version);
}



CHUNK(LoadMData)
{
  FULLHEADER;

  READ_SUBCHUNKS(
    case MESH_VERSION: LoadMeshVersion(con); break;
    case MAT_ENTRY:    LoadMatEntry(con); break;
    case NAMED_OBJECT: LoadNamedObject(con); break;
  )
}



CHUNK(LoadMeshVersion)
{
  HEADER;

  int32_t version;
  con->s >> version;

  if (coin_debug_3ds() >= 3)
    SoDebugError::postInfo("LoadMeshVersion",
                           "The 3ds file version: %i.", version);
}



CHUNK(LoadNamedObject)
{
  FULLHEADER;

  assert(!con->cObj && "Forgot to free the current object.");

  // read object name
  con->s.readZString(con->objectName, OBJNAME_LENGTH);

  if (coin_debug_3ds() >= 3)
    SoDebugError::postInfo("LoadNamesObject",
                           "Name: %s.", con->objectName);

  READ_SUBCHUNKS(
    case N_TRI_OBJECT:  LoadNTriObject(con); break;
  )

  if (con->cObj) {
    // set object name
    if (con->loadObjNames && con->objectName[0] != '\0')
      con->cObj->setName(con->objectName);

    // add cObj to the main scene graph
    if (con->cObj->getNumChildren() > 0)
      con->root->addChild(con->cObj);
    con->cObj->unref();
    con->cObj = NULL;
  }
}



CHUNK(LoadNTriObject)
{
  FULLHEADER;

  assert(con->faceGroupList.getLength() == 0);
  con->numVertices = 0;
  con->numFaces = 0;
  con->numDefaultDegFaces = 0;
  con->textureCoordsFound = FALSE;

  if (coin_debug_3ds() >= 3)
    SoDebugError::postInfo("LoadNTriObject",
                           "About to load");

  READ_SUBCHUNKS(
    case POINT_ARRAY:       LoadPointArray(con); break;
    case POINT_FLAG_ARRAY:  SkipChunk(con); break;
    case FACE_ARRAY:        LoadFaceArray(con); break;
    case MSH_MAT_GROUP:     LoadMshMatGroup(con); break;
    case TEX_VERTS:         LoadTexVerts(con); break;
    case MESH_MATRIX:       SkipChunk(con); break;
    case MESH_COLOR:        SkipChunk(con); break;
    case MESH_TEXTURE_INFO: SkipChunk(con); break;
    case PROC_NAME:         SkipChunk(con); break;
    case PROC_DATA:         SkipChunk(con); break;
  )

  // debug info
  if (coin_debug_3ds() >= 3)
    SoDebugError::postInfo("LoadNTriObject",
                           "Object %s parsed - vertices: %i, faces %i.", &con->objectName, con->numVertices, con->numFaces);
  con->totalVertices += con->numVertices;
  con->totalFaces += con->numFaces;

  // create object separator
  con->cObj = new SoSeparator;
  con->cObj->ref();

  con->genCurrentTexture = NULL;
  con->genCurrentTexTransform = NULL;
  con->genCurrentMaterial = NULL;
  con->genTwoSided = -1;

  // create coordinates (in indexed mode)
  if (con->useIndexedTriSet) {
    // coordinates
    con->cObj->addChild(con->createSoCoordinate3_i(con));
    // texture coordinates
    if (con->textureCoordsFound && con->loadTextures)
      con->cObj->addChild(con->createSoTextureCoordinate2_i(con));
  }

  // create "default material" scene
  if (!DefaultFaceGroup::isEmpty(con)) {

    // default material has not a texture => switch it off
#if 0 // non-optimized version
    if (con->loadTextures)
      con->cObj->addChild(new SoTexture2);
#else
    if (con->loadTextures) {
      SoTexture2 *t = con->genGetEmptyTexture();
      if (t != con->genCurrentTexture) {
        con->genCurrentTexture = t;
        con->cObj->addChild(t);
      }
      SoTexture2Transform *tt = con->genGetEmptyTexTransform();
      if (tt != con->genCurrentTexTransform) {
        con->genCurrentTexTransform = tt;
        con->cObj->addChild(tt);
      }
    }
#endif

    // materials
#if 0 // non-optimized version
    if (con->loadMaterials)
      con->cObj->addChild(DefaultFaceGroup::getSoMaterial(con));
#else
    if (con->loadMaterials) {
      SoMaterial *m = DefaultFaceGroup::getSoMaterial(con);
      if (m != con->genCurrentMaterial) {
        con->genCurrentMaterial = m;
        con->cObj->addChild(con->genCurrentMaterial);
      }
    }
#endif

    // normals
    if (con->appendNormals)
      con->cObj->addChild(DefaultFaceGroup::createSoNormal(con));

#ifndef DISABLE_BACKFACE_CULLING
    // single x double faces
    SbBool matTwoSided = DefaultFaceGroup::getMaterial(con)->twoSided;
    if (con->genTwoSided == -1 || (matTwoSided && (con->genTwoSided == 0)) ||
       (!matTwoSided && (con->genTwoSided == 1))) {
      con->genTwoSided = (DefaultFaceGroup::getMaterial(con)->twoSided ? 1 : 0);
      con->cObj->addChild((con->genTwoSided == 1) ?
          con->genGetTwoSidedHints() : con->genGetOneSidedHints());
    }
#endif

    // load default material geometry
    if (con->useIndexedTriSet) {
      // indexed triStripSet
      con->cObj->addChild(DefaultFaceGroup::createSoIndexedTriStripSet_i(con));
    } else {
      // coordinates
      con->cObj->addChild(DefaultFaceGroup::createSoCoordinate3_n(con));
      // triStripSet
      con->cObj->addChild(DefaultFaceGroup::createSoTriStripSet_n(con));
    }
  }

  // create non-default materials scene
  for (int i=0; i<con->faceGroupList.getLength(); i++) {
    FaceGroup *fg = con->faceGroupList[i];

    // materials
#if 0 // non-optimized version
    if (con->loadMaterials)
      con->cObj->addChild(fg->getSoMaterial(con));
#else
    if (con->loadMaterials) {
      SoMaterial *m = fg->getSoMaterial(con);
      if (m != con->genCurrentMaterial) {
        con->genCurrentMaterial = m;
        con->cObj->addChild(m);
      }
    }
#endif

    // normals
    if (con->appendNormals)
      con->cObj->addChild(fg->createSoNormal(con));

    // textures - optimized code
    if (con->loadTextures) {
      if (fg->hasTexture2(con)) {
        SoTexture2 *t = fg->getSoTexture2(con);
        if (t != con->genCurrentTexture) {
          con->genCurrentTexture = t;
          con->cObj->addChild(t);
        }
      } else {
        SoTexture2 *t = con->genGetEmptyTexture();
        if (t != con->genCurrentTexture) {
          con->genCurrentTexture = t;
          con->cObj->addChild(t);
        }
      }
      // texture transform - optimized code
      if (fg->hasTexture2Transform(con)) {
        SoTexture2Transform *tt = fg->getSoTexture2Transform(con);
        if (tt != con->genCurrentTexTransform) {
          con->genCurrentTexTransform = tt;
          con->cObj->addChild(tt);
        }
      } else {
        SoTexture2Transform *tt = con->genGetEmptyTexTransform();
        if (tt != con->genCurrentTexTransform) {
          con->genCurrentTexTransform = tt;
          con->cObj->addChild(tt);
        }
      }
    }

#ifndef DISABLE_BACKFACE_CULLING
    // single x double faces
    if (con->genTwoSided == -1 || (fg->mat->twoSided && (con->genTwoSided == 0)) ||
       (!fg->mat->twoSided && (con->genTwoSided == 1))) {
      con->genTwoSided = (fg->mat->twoSided ? 1 : 0);
      con->cObj->addChild((con->genTwoSided == 1) ?
          con->genGetTwoSidedHints() : con->genGetOneSidedHints());
    }
#endif

    if (con->useIndexedTriSet) {
      // indexed triStripSet
      con->cObj->addChild(fg->createSoIndexedTriStripSet_i(con));
    } else {
      // coordinates
      con->cObj->addChild(fg->createSoCoordinate3_n(con));
      // textures
      if (con->loadTextures && fg->hasTexture2(con))
        con->cObj->addChild(fg->createSoTextureCoordinate2_n(con));
      // triStripSet
      con->cObj->addChild(fg->createSoTriStripSet_n(con));
    }
  }

  // clean up memory
  delete[] con->vertexList;
  con->vertexList = NULL;
  delete[] con->faceList;
  con->faceList = NULL;
  for (int j=con->faceGroupList.getLength()-1; j>=0; j--) {
    delete con->faceGroupList[j];
    con->faceGroupList.removeFast(j);
  }
}



CHUNK(LoadPointArray)
{
  HEADER;

  if (coin_debug_3ds() >= 3)
    SoDebugError::postInfo("LoadPointArray",
                           "Begin");


  // number of vertices
  uint16_t num;
  con->s >> num;

  // alloc memory for Vertices
  assert(con->vertexList == NULL && "Forgot to free memory.");
  con->vertexList = new Vertex[num];
  con->numVertices = num;

  // read points
  float x,y,z;
  for (int i=0; i<num; i++) {
    con->s >> x;
    con->s >> y;
    con->s >> z;
    con->vertexList[i].point = SbVec3f(x,z,-y); // 3ds has different
        //coordinate system. Z is up, and Y goes into the scene.
  }
}



CHUNK(LoadFaceArray)
{
  FULLHEADER;

  if (coin_debug_3ds() >= 3)
    SoDebugError::postInfo("LoadFaceArray",
                           "Begin");

  // number of faces
  uint16_t num;
  con->s >> num;

  // alloc memory for Faces
  assert(con->faceList == NULL && "Forgot to free memory.");
  con->faceList = new Face[num];
  con->numFaces = num;

  // alloc memory for edges
  if (con->appendNormals == 2) {
    con->numEdges = 0;
    con->edgeList = new Edge[num*3];
  }

  // make sure vertices are present yet
  if (num > 0) {
    if (con->vertexList == NULL) {
      assert(FALSE && "Vertex list not present.");
      con->s.setBadBit();
      return;
    }
  }

  // read Faces
  uint16_t a,b,c;
  uint16_t flags;
  for (int i=0; i<num; i++) {
    con->s >> a;
    con->s >> b;
    con->s >> c;
    con->s >> flags; // STUB: decode flags (edge visibility and texture
                     // wrapping, but first get idea what's the flags meaning)
    if (flags != 7 && coin_debug_3ds() >= 2)
      SoDebugError::postWarning("LoadFaceArray",
                                "Non-standard face flags: %x, investigate it.\n", flags);
    con->faceList[i].init(con, a,b,c,flags); // vertex ordering is counterclockwise.

    if (!con->minMaxValid) {
      con->minMaxValid = TRUE;
      con->minX = con->maxX = con->vertexList[a].point[0];
      con->minY = con->maxY = con->vertexList[a].point[1];
      con->minZ = con->maxZ = con->vertexList[a].point[2];
    }
    #define PROCESS_VERTEX(_abc_, _xyz_, _index_) \
      if (con->min##_xyz_ > con->vertexList[_abc_].point[_index_]) \
        con->min##_xyz_ = con->vertexList[_abc_].point[_index_]; \
      else \
        if (con->max##_xyz_ < con->vertexList[_abc_].point[_index_]) \
          con->max##_xyz_ = con->vertexList[_abc_].point[_index_]

    PROCESS_VERTEX(a, X, 0);
    PROCESS_VERTEX(a, Y, 1);
    PROCESS_VERTEX(a, Z, 2);
    PROCESS_VERTEX(b, X, 0);
    PROCESS_VERTEX(b, Y, 1);
    PROCESS_VERTEX(b, Z, 2);
    PROCESS_VERTEX(c, X, 0);
    PROCESS_VERTEX(c, Y, 1);
    PROCESS_VERTEX(c, Z, 2);
    #undef PROCESS_VERTEX
  }

  // report degenerated faces
  if (con->numDefaultDegFaces > 0 && coin_debug_3ds() >= 1)
    SoDebugError::postWarning("LoadFaceArray",
                              "There are %i degenerated faces in the object named \"%s\" - removing them.",
                              con->numDefaultDegFaces, &con->objectName);

  READ_SUBCHUNKS(
    case MSH_MAT_GROUP: LoadMshMatGroup(con); break;
    case SMOOTH_GROUP:  SkipChunk(con); break;
    case MSH_BOXMAP:    SkipChunk(con); break;
  )
}



CHUNK(LoadMshMatGroup)
{
  FULLHEADER;

  if (coin_debug_3ds() >= 4)
    SoDebugError::postInfo("LoadMatGroup",
                           "Begin");

  if (!con->loadMaterials && !con->loadTextures) {
    // move on the position of the next chunk
    con->s.setPos(stopPos);
    return;
  }

  // material name
  char materialName[MATNAME_LENGTH];
  con->s.readZString(materialName, MATNAME_LENGTH);
  int matIndex;
  for (matIndex=1; matIndex<con->matList.getLength(); matIndex++) {
    if (strcmp(con->matList[matIndex]->name.getString(), materialName) == 0)
      break;
  }
  if (matIndex == con->matList.getLength()) {
    assert(FALSE && "Wrong material name in the file.");
    con->s.setBadBit();
    return;
  }

  // create FaceGroup
  FaceGroup *mm = new FaceGroup;
  mm->mat = con->matList[matIndex];
  con->faceGroupList.append(mm);
  mm->numDegFaces = 0;

  // number of faces
  uint16_t num;
  con->s >> num;

  // make sure faces are present yet
  if (num > 0) {
    if (con->faceList == NULL) {
      assert(FALSE && "Face list not present.");
      con->s.setBadBit();
      return;
    }
  }

  // face indexes
  uint16_t faceMatIndex;
  for (int i=0; i<num; i++) {
    con->s >> faceMatIndex;
    if (faceMatIndex < con->numFaces) {
      assert(con->faceList[faceMatIndex].faceGroup == NULL &&
             "3ds file error: Two materials on one face.");
      con->faceList[faceMatIndex].faceGroup = mm;
      mm->faceList.append(&con->faceList[faceMatIndex]);
      if (con->faceList[faceMatIndex].isDegenerated) {
        mm->numDegFaces++;
        con->numDefaultDegFaces--;
      }
    } else {
      assert(FALSE && "Wrong face material index.");
      con->s.setBadBit();
      return;
    }
  }
}



CHUNK(LoadTexVerts)
{
  HEADER;

  if (coin_debug_3ds() >= 4)
    SoDebugError::postInfo("LoadTexVerts",
                           "Begin");

  con->textureCoordsFound = TRUE;

  // number of faces
  uint16_t num;
  con->s >> num;

  // make sure vertices are present yet
  if (num > 0) {
    if (con->vertexList == NULL) {
      assert(FALSE && "Vertex list not present.");
      con->s.setBadBit();
      return;
    }
  }

  // texture coordinates
  float u;
  float v;
  for (int i=0; i<num; i++) {
    con->s >> u;
    con->s >> v;
    con->vertexList[i].texturePoint = SbVec2f(u,v);
  }
}



CHUNK(LoadMatEntry)
{
  FULLHEADER;

  if (coin_debug_3ds() >= 4)
    SoDebugError::postInfo("LoadMatEntry",
                           "Begin");

  if (!con->loadMaterials && !con->loadTextures) {
    // move on the position of the next chunk
    con->s.setPos(stopPos);
    return;
  }

  assert(con->cMat == NULL);
  con->cMat = new Material;
  con->matList.append(con->cMat);

  // default values
  con->cMat->name = "";
  con->cMat->ambient = SbColor(0.f, 0.f, 0.f);
  con->cMat->diffuse = SbColor(0.f, 0.f, 0.f);
  con->cMat->specular = SbColor(0.f, 0.f, 0.f);
  con->cMat->shininess = 0.f;
  con->cMat->transparency = 0.f;
  con->cMat->twoSided = FALSE;

  READ_SUBCHUNKS(
    case MAT_NAME:     LoadMatName(con); break;
    case MAT_AMBIENT:  LoadMatAmbient(con); break;
    case MAT_DIFFUSE:  LoadMatDiffuse(con); break;
    case MAT_SPECULAR: LoadMatSpecular(con); break;
    case MAT_SHININESS: LoadShininess(con); break;
    case MAT_TRANSPARENCY: LoadTransparency(con); break;
    case MAT_TWO_SIDE: LoadMatTwoSide(con); break;
    case MAT_TEXMAP:   LoadTexMap(con); break;
  )

  // create new SoMaterial
  con->cMat->matCache = new SoMaterial;
  con->cMat->matCache->ref();
  con->cMat->updateSoMaterial(0, con->cMat->matCache);

  con->cMat = NULL;
}



CHUNK(LoadMatName)
{
  HEADER;

  if (coin_debug_3ds() >= 4)
    SoDebugError::postInfo("LoadMatName",
                           "Begin");

  char materialName[MATNAME_LENGTH];
  con->s.readZString(materialName, MATNAME_LENGTH);

  con->cMat->name = materialName;
}



CHUNK(LoadMatAmbient)
{
  FULLHEADER;

  if (coin_debug_3ds() >= 4)
    SoDebugError::postInfo("LoadMatAmbient",
                           "Begin");

  READ_SUBCHUNKS(
    case COLOR_24:     LoadColor24(con); break;
    case LIN_COLOR_24: LoadLinColor24(con); break;
  )
  con->cMat->ambient = con->cColor;
}



CHUNK(LoadMatDiffuse)
{
  FULLHEADER;

  if (coin_debug_3ds() >= 4)
    SoDebugError::postInfo("LoadMatDiffuse",
                           "Begin");

  READ_SUBCHUNKS(
    case COLOR_24:     LoadColor24(con); break;
    case LIN_COLOR_24: LoadLinColor24(con); break;
  )
  con->cMat->diffuse = con->cColor;
}



CHUNK(LoadMatSpecular)
{
  FULLHEADER;

  if (coin_debug_3ds() >= 4)
    SoDebugError::postInfo("LoadMatSpecular",
                           "Begin");

  READ_SUBCHUNKS(
    case COLOR_24:     LoadColor24(con); break;
    case LIN_COLOR_24: LoadLinColor24(con); break;
  )
  con->cMat->specular = con->cColor;
}



CHUNK(LoadShininess)
{
  FULLHEADER;

  if (coin_debug_3ds() >= 4)
    SoDebugError::postInfo("LoadMatShininess",
                           "Begin");

  con->cColorFloat = 0.f;

  READ_SUBCHUNKS(
    case INT_PERCENTAGE: LoadIntPercentage(con); break;
    case FLOAT_PERCENTAGE: LoadFloatPercentage(con); break;
  )
  con->cMat->shininess = con->cColorFloat;
}



CHUNK(LoadMatTwoSide)
{
  HEADER;

  if (coin_debug_3ds() >= 4)
    SoDebugError::postInfo("LoadMatTwoSide",
                           "Begin");

  con->cMat->twoSided = TRUE;
}



CHUNK(LoadTransparency)
{
  FULLHEADER;

  if (coin_debug_3ds() >= 4)
    SoDebugError::postInfo("LoadTransparency",
                           "Begin");

  con->cColorFloat = 0.f;

  READ_SUBCHUNKS(
    case INT_PERCENTAGE: LoadIntPercentage(con); break;
    case FLOAT_PERCENTAGE: LoadFloatPercentage(con); break;
  )
  con->cMat->transparency = con->cColorFloat;
}



CHUNK(LoadTexMap)
{
  FULLHEADER;

  if (coin_debug_3ds() >= 4)
    SoDebugError::postInfo("LoadTexMap",
                           "Begin");

  if (!con->loadTextures) {
    // move on the position of the next chunk
    con->s.setPos(stopPos);
    return;
  }

  con->cMat->uscale = con->cMat->vscale = 1.f;
  con->cMat->uoffset = con->cMat->voffset = 0.f;

  READ_SUBCHUNKS(
    case INT_PERCENTAGE: LoadIntPercentage(con); break;
    case FLOAT_PERCENTAGE: LoadFloatPercentage(con); break;
    case MAT_MAPNAME:    LoadMapName(con); break;
    case MAT_MAP_USCALE:  LoadMapUScale(con); break;
    case MAT_MAP_VSCALE:  LoadMapVScale(con); break;
    case MAT_MAP_UOFFSET: LoadMapUOffset(con); break;
    case MAT_MAP_VOFFSET: LoadMapVOffset(con); break;
  )

  if (con->cMat->textureName.getLength() > 0) {
    SoTexture2 *t = new SoTexture2;
    t->ref();
    t->filename.setValue(con->cMat->textureName);
    t->model.setValue(SoTexture2::DECAL);
    con->cMat->texture2Cache = t;
  }

  if (con->cMat->uscale  != 1.f || con->cMat->vscale  != 1.f ||
      con->cMat->uoffset != 0.f || con->cMat->voffset != 0.f) {
    SoTexture2Transform *tt = new SoTexture2Transform;
    tt->ref();
    tt->scaleFactor.setValue(SbVec2f(con->cMat->uscale, con->cMat->vscale));
    tt->translation.setValue(SbVec2f(con->cMat->uoffset, con->cMat->voffset));
    con->cMat->texture2TransformCache = tt;
  }
}



CHUNK(LoadMapName)
{
  HEADER;

  if (coin_debug_3ds() >= 4)
    SoDebugError::postInfo("LoadMapName",
                           "Begin");

  char textureName[13];
  con->s.readZString(textureName, 13);

  con->cMat->textureName = textureName;
}



CHUNK(LoadMapUScale)
{
  HEADER;

  if (coin_debug_3ds() >= 4)
    SoDebugError::postInfo("LoadMapUScale",
                           "Begin");

  con->s >> con->cMat->uscale;
}



CHUNK(LoadMapVScale)
{
  HEADER;

  if (coin_debug_3ds() >= 4)
    SoDebugError::postInfo("LoadMapVScale",
                           "Begin");

  con->s >> con->cMat->vscale;
}



CHUNK(LoadMapUOffset)
{
  HEADER;

  if (coin_debug_3ds() >= 4)
    SoDebugError::postInfo("LoadMapUOffset",
                           "Begin");

  con->s >> con->cMat->uoffset;
}



CHUNK(LoadMapVOffset)
{
  HEADER;
  if (coin_debug_3ds() >= 4)
    SoDebugError::postInfo("LoadMapVOffset",
                           "Begin");

  con->s >> con->cMat->voffset;
}



CHUNK(LoadColor24)
{
  HEADER;

  if (coin_debug_3ds() >= 4)
    SoDebugError::postInfo("LoadColor24",
                           "Begin");

  uint8_t r,g,b;
  con->s >> r;
  con->s >> g;
  con->s >> b;

  con->cColor = SbColor(float(r)/255.f, float(g)/255.f, float(b)/255.f);
}



CHUNK(LoadLinColor24)
{
  HEADER;

  if (coin_debug_3ds() >= 4)
    SoDebugError::postInfo("LoadLinColor24",
                           "Begin");

  uint8_t r,g,b;
  con->s >> r;
  con->s >> g;
  con->s >> b;

  con->cColor = SbColor(float(r)/255.f, float(g)/255.f, float(b)/255.f);
}



CHUNK(LoadIntPercentage)
{
  HEADER;

  if (coin_debug_3ds() >= 4)
    SoDebugError::postInfo("LoadIntPercentage",
                           "Begin");

  int16_t i;
  con->s >> i;

  con->cColorFloat = float(i)/100.f;
}



CHUNK(LoadFloatPercentage)
{
  HEADER;

  if (coin_debug_3ds() >= 4)
    SoDebugError::postInfo("LoadFloatPercentage",
                           "Begin");

  con->s >> (con->cColorFloat);
}





SbVec3f Vertex::getNormal(tagContext *con, uint16_t myIndex) const
{
  int c = this->faceList.getLength();
  SbVec3f normal(0.f,0.f,0.f);
  for (int i=0; i<c; i++)
    normal += this->faceList[i]->getWeightedNormal(con, myIndex);
  // ok to have a null vector here, it probably just means that we
  // have an empty triangle.
  (void) normal.normalize();
  return normal;
}



SbVec3f Face::getNormal(tagContext *con) const
{
  SbPlane plane(con->vertexList[v1].point, con->vertexList[v2].point,
      con->vertexList[v3].point);
  return plane.getNormal();
}



float Face::getAngle(tagContext *con, uint16_t vertexIndex) const
{
  int i1, i2;
  if (v1 == vertexIndex) {
    i1 = v2; i2 = v3;
  } else {
    i1 = v1;
    if (v2 == vertexIndex) i2 = v3;
    else {
      assert(v3 == vertexIndex);
      i2 = v2;
    }
  }

  SbVec3f vec1(con->vertexList[i1].point - con->vertexList[vertexIndex].point);
  SbVec3f vec2(con->vertexList[i2].point - con->vertexList[vertexIndex].point);

  SbRotation rot(vec1, vec2);
  float value;
  rot.getValue(vec1, value);
  return value;
}



SbVec3f Face::getWeightedNormal(tagContext *con, uint16_t vertexIndex) const
{
  return getNormal(con) * getAngle(con, vertexIndex);
}



void Face::init(tagContext *con, uint16_t a, uint16_t b, uint16_t c, uint16_t f)
{
  v1=a; v2=b; v3=c; flags=f;
  faceGroup = NULL;

  isDegenerated = (con->vertexList[v2].point-con->vertexList[v1].point).cross(
      con->vertexList[v3].point - con->vertexList[v1].point).sqrLength() == 0.f;
  if (isDegenerated)  con->numDefaultDegFaces++;

  int n,i;
  Face *face;
  if (con->appendNormals == 2) {
    e12 = e23 = e31 = 0xffffffff;

#define VERT_CODE(_nc_, _ns1_, _ns2_) \
    n = con->vertexList[v##_nc_].faceList.getLength(); \
    for (i=0; i<n; i++) { \
      face = con->vertexList[v##_nc_].faceList[i]; \
      if (v##_ns1_ == face->v##_ns1_) { \
        e##_nc_##_ns1_ = face->e##_nc_##_ns1_; \
        con->edgeList[face->e##_nc_##_ns1_].faceList.append(this); \
      } \
      if (v##_ns2_ == face->v##_ns1_) { \
        e##_ns2_##_nc_ = face->e##_nc_##_ns1_; \
        con->edgeList[face->e##_nc_##_ns1_].faceList.append(this); \
      } \
      if (v##_ns1_ == face->v##_ns2_) { \
        e##_nc_##_ns1_ = face->e##_ns2_##_nc_; \
        con->edgeList[face->e##_ns2_##_nc_].faceList.append(this); \
      } \
      if (v##_ns2_ == face->v##_ns2_) { \
        e##_ns2_##_nc_ = face->e##_ns2_##_nc_; \
        con->edgeList[face->e##_ns2_##_nc_].faceList.append(this); \
      } \
    } \
    con->vertexList[v##_nc_].faceList.append(this);

    VERT_CODE(1,2,3);
    VERT_CODE(2,3,1);
    VERT_CODE(3,1,2);

#undef VERT_CODE

    if (e12 == 0xffffffff)
      e12 = con->numEdges++;
    if (e23 == 0xffffffff)
      e23 = con->numEdges++;
    if (e31 == 0xffffffff)
      e31 = con->numEdges++;
  }
}



SbBool FaceGroup::hasTexture2(tagContext *con)
{ return mat->hasTexture2(con); }

SoTexture2* FaceGroup::getSoTexture2(tagContext *con)
{ return mat->getSoTexture2(con); }

SbBool FaceGroup::hasTexture2Transform(tagContext *con)
{ return mat->hasTexture2Transform(con); }

SoTexture2Transform* FaceGroup::getSoTexture2Transform(tagContext *con)
{ return mat->getSoTexture2Transform(con); }

SoMaterial* FaceGroup::getSoMaterial(tagContext *con)
{ return mat->getSoMaterial(con); }



SoNormal* FaceGroup::createSoNormal(tagContext *con)
{
  SoNormal *normals = new SoNormal;
  int num = faceList.getLength();
  if (con->appendNormals == 1) {
    normals->vector.setNum(num-numDegFaces);
    SbVec3f *v = normals->vector.startEditing();
    for (int i=0; i<num; i++) {
      Face *f = faceList[i];
      if (!f->isDegenerated)
        *(v++) = f->getNormal(con);
    }
    normals->vector.finishEditing();
  } else {
    normals->vector.setNum(0);
/*  FIXME: This is incomplete implementation of per-vertex normal generator.

    normals->vector.setNum(num*3);
    SbVec3f *v = normals->vector.startEditing();
    SbPlane p1, p2;
    for (int i=0; i<num; i++) {
      Face *f = faceList[i];
      Edge *e = &con->edgeList[f->e12];
      int fnum = e->faceList.getLength();
      int j;
      for (j=0; i<fnum; j++) {
        if (currface != facenum) { // check all but this face
      const SbVec3f &normal = facenormals[currface];
      if ((normal.dot(*facenormal)) > threshold) {
        // smooth towards this face
        vertnormal += normal;
      }
    }
  }
      *(v++) = con->vertexList[f->v1].getNormal(con, f->v1);
      *(v++) = con->vertexList[f->v2].getNormal(con, f->v2);
      *(v++) = con->vertexList[f->v3].getNormal(con, f->v3);
    }
    normals->vector.finishEditing();*/
  }
  return normals;
}



SoCoordinate3* FaceGroup::createSoCoordinate3_n(tagContext *con)
{
  assert(!con->useIndexedTriSet && "Improper use.");

  SoCoordinate3 *coords = new SoCoordinate3;
  int num = faceList.getLength();
  coords->point.setNum((num-numDegFaces)*3);
  SbVec3f *c = coords->point.startEditing();
  for (int i=0; i<num; i++) {
    Face *f = faceList[i];
    if (!f->isDegenerated) {
      *(c++) = con->vertexList[f->v1].point;
      *(c++) = con->vertexList[f->v2].point;
      *(c++) = con->vertexList[f->v3].point;
    }
  }
  coords->point.finishEditing();
  return coords;
}



SoTextureCoordinate2* FaceGroup::createSoTextureCoordinate2_n(tagContext *con)
{
  assert(!con->useIndexedTriSet && "Improper use.");

  SoTextureCoordinate2 *tCoords = new SoTextureCoordinate2;
  int num = faceList.getLength();
  tCoords->point.setNum((num-numDegFaces)*3);
  SbVec2f *c = tCoords->point.startEditing();
  for (int i=0; i<num; i++) {
    Face *f = faceList[i];
    if (!f->isDegenerated) {
      *(c++) = con->vertexList[f->v1].texturePoint;
      *(c++) = con->vertexList[f->v2].texturePoint;
      *(c++) = con->vertexList[f->v3].texturePoint;
    }
  }
  tCoords->point.finishEditing();
  return tCoords;
}



SoTriangleStripSet* FaceGroup::createSoTriStripSet_n(tagContext *con)
{
  assert(!con->useIndexedTriSet && "Improper use.");

  SoTriangleStripSet *triSet = new SoTriangleStripSet;
  int num = faceList.getLength() - numDegFaces;
  triSet->numVertices.setNum(num);
  int32_t *n = triSet->numVertices.startEditing();
  for (int i=0; i<num; i++) {
    *(n++) = 3;
  }
  triSet->numVertices.finishEditing();
  return triSet;
}



SoIndexedTriangleStripSet* FaceGroup::createSoIndexedTriStripSet_i(tagContext *con)
{
  assert(con->useIndexedTriSet && "Improper use.");

  SoIndexedTriangleStripSet *triSet = new SoIndexedTriangleStripSet;
  int num = faceList.getLength();
  int i;

  // coords
  triSet->coordIndex.setNum((num-numDegFaces)*4);
  int32_t *c = triSet->coordIndex.startEditing();
  for (i=0; i<num; i++) {
    Face *f = faceList[i];
    if (!f->isDegenerated) {
      *(c++) = f->v1;
      *(c++) = f->v2;
      *(c++) = f->v3;
      *(c++) = SO_END_STRIP_INDEX;
    }
  }
  triSet->coordIndex.finishEditing();

  // texture
  if (mat->hasTexture2(con) && con->loadTextures) {
    triSet->textureCoordIndex.setNum((num-numDegFaces)*4);
    int32_t *tc = triSet->textureCoordIndex.startEditing();
    for (i=0; i<num; i++) {
      Face *f = faceList[i];
      if (!f->isDegenerated) {
        *(tc++) = f->v1;
        *(tc++) = f->v2;
        *(tc++) = f->v3;
        *(tc++) = SO_END_STRIP_INDEX;
      }
    }
    triSet->textureCoordIndex.finishEditing();
  }

  return triSet;
}



Material* DefaultFaceGroup::getMaterial(tagContext *con)
{ return &con->defaultMat; }



SbBool DefaultFaceGroup::isEmpty(Context *con)
{
  int num = con->numFaces;
  for (int i=0; i<num; i++)
    if (con->faceList[i].faceGroup == NULL) return FALSE;
  return TRUE;
}



SoMaterial* DefaultFaceGroup::getSoMaterial(tagContext *con)
{
  return getMaterial(con)->getSoMaterial(con);
}



SoNormal* DefaultFaceGroup::createSoNormal(tagContext *con)
{
  SoNormal *normals = new SoNormal;
  int num = con->numFaces;
  int j = 0;
  if (con->appendNormals == 1) {
    normals->vector.setNum(num - con->numDefaultDegFaces);
    SbVec3f *v = normals->vector.startEditing();
    for (int i=0; i<num; i++) {
      Face *f = &con->faceList[i];
      if (f->faceGroup == NULL && !f->isDegenerated) {
        *(v++) = f->getNormal(con);
        j++;
      }
    }
    normals->vector.finishEditing();
    normals->vector.setNum(j);
  } else {
    assert(FALSE);
  }
  return normals;
}



SoCoordinate3* DefaultFaceGroup::createSoCoordinate3_n(tagContext *con)
{
  assert(!con->useIndexedTriSet && "Improper use.");

  SoCoordinate3 *coords = new SoCoordinate3;
  int num = con->numFaces;
  coords->point.setNum((num - con->numDefaultDegFaces) * 3);
  SbVec3f *c = coords->point.startEditing();
  int j = 0;
  for (int i=0; i<num; i++) {
    Face *f = &con->faceList[i];
    if (f->faceGroup == NULL && !f->isDegenerated) {
      *(c++) = con->vertexList[f->v1].point;
      *(c++) = con->vertexList[f->v2].point;
      *(c++) = con->vertexList[f->v3].point;
      j += 3;
    }
  }
  coords->point.finishEditing();
  coords->point.setNum(j);
  return coords;
}



SoTriangleStripSet* DefaultFaceGroup::createSoTriStripSet_n(tagContext *con)
{
  assert(!con->useIndexedTriSet && "Improper use.");

  SoTriangleStripSet *triSet = new SoTriangleStripSet;
  int num = con->numFaces - con->numDefaultDegFaces;
  int i;
  int j = 0;
  for (i=0; i<num; i++) {
    Face *f = &con->faceList[i];
    if (f->faceGroup == NULL)  j++;
  }
  j -= con->numDefaultDegFaces;

  triSet->numVertices.setNum(j);
  int32_t *n = triSet->numVertices.startEditing();
  for (i=0; i<j; i++) {
    *(n++) = 3;
  }
  triSet->numVertices.finishEditing();

  return triSet;
}



SoIndexedTriangleStripSet* DefaultFaceGroup::createSoIndexedTriStripSet_i(tagContext *con)
{
  assert(con->useIndexedTriSet && "Improper use.");

  SoIndexedTriangleStripSet *triSet = new SoIndexedTriangleStripSet;
  int num = con->numFaces;
  int i;
  int j = 0;
  for (i=0; i<num; i++) {
    Face *f = &con->faceList[i];
    if (f->faceGroup == NULL)  j++;
  }
  j -= con->numDefaultDegFaces;

  // coords
  triSet->coordIndex.setNum(j*4);
  int32_t *c = triSet->coordIndex.startEditing();
  for (i=0; i<num; i++) {
    Face *f = &con->faceList[i];
    if (f->faceGroup == NULL && !f->isDegenerated) {
      *(c++) = f->v1;
      *(c++) = f->v2;
      *(c++) = f->v3;
      *(c++) = SO_END_STRIP_INDEX;
    }
  }
  triSet->coordIndex.finishEditing();

  return triSet;
}



void Material::updateSoMaterial(int index, SoMaterial *m)
{
  m->ambientColor.set1Value(index, ambient);
  m->diffuseColor.set1Value(index, diffuse);
  m->specularColor.set1Value(index, specular);
  m->emissiveColor.set1Value(index, SbColor(0.f,0.f,0.f));
  m->shininess.set1Value(index, shininess);
  m->transparency.set1Value(index, transparency);
}



SoMaterial* Material::getSoMaterial(Context COIN_UNUSED_ARG(*con))
{ return matCache; }



SbBool Material::hasTexture2(tagContext COIN_UNUSED_ARG(*con))
{ return (texture2Cache != NULL); }



SoTexture2* Material::getSoTexture2(tagContext COIN_UNUSED_ARG(*con))
{ return texture2Cache; }



SbBool Material::hasTexture2Transform(tagContext COIN_UNUSED_ARG(*con))
{ return (texture2TransformCache != NULL); }



SoTexture2Transform* Material::getSoTexture2Transform(tagContext COIN_UNUSED_ARG(*con))
{ return texture2TransformCache; }



SoCoordinate3* Context::createSoCoordinate3_i(Context *con) const
{
  assert(con->useIndexedTriSet && "Improper use.");

  SoCoordinate3 *coords = new SoCoordinate3;
  coords->point.setNum(con->numVertices);
  SbVec3f *c = coords->point.startEditing();
  for (int i=0; i<con->numVertices; i++)
    c[i] = con->vertexList[i].point;
  coords->point.finishEditing();
  return coords;
}



SoTextureCoordinate2* Context::createSoTextureCoordinate2_i(tagContext *con) const
{
  assert(con->useIndexedTriSet && "Improper use.");

  SoTextureCoordinate2 *tCoords = new SoTextureCoordinate2;
  tCoords->point.setNum(con->numVertices);
  SbVec2f *c = tCoords->point.startEditing();
  for (int i=0; i<con->numVertices; i++)
    c[i] = con->vertexList[i].texturePoint;
  tCoords->point.finishEditing();
  return tCoords;
}



SoTexture2* Context::genGetEmptyTexture()
{
  if (!genEmptyTexture) {
    genEmptyTexture = new SoTexture2;
    genEmptyTexture->ref();
  }
  return genEmptyTexture;
}



SoTexture2Transform* Context::genGetEmptyTexTransform()
{
  if (!genEmptyTexTransform) {
    genEmptyTexTransform = new SoTexture2Transform;
    genEmptyTexTransform->ref();
  }
  return genEmptyTexTransform;
}



SoShapeHints* Context::genGetOneSidedHints()
{
  if (!genOneSidedHints) {
    genOneSidedHints = new SoShapeHints;
    genOneSidedHints->ref();
    // backface culling on, one-sided lighting
    genOneSidedHints->vertexOrdering = SoShapeHints::COUNTERCLOCKWISE;
    genOneSidedHints->shapeType = SoShapeHints::SOLID;
  }
  return genOneSidedHints;
}



SoShapeHints* Context::genGetTwoSidedHints()
{
  if (!genTwoSidedHints) {
    genTwoSidedHints = new SoShapeHints;
    genTwoSidedHints->ref();
    // backface culling off, two-sided lighting
    genTwoSidedHints->vertexOrdering = SoShapeHints::COUNTERCLOCKWISE;
    genTwoSidedHints->shapeType = SoShapeHints::UNKNOWN_SHAPE_TYPE;
  }
  return genTwoSidedHints;
}



/* Return value of COIN_DEBUG_3DS environment variable. */
static int coin_debug_3ds()
{
  static int d = -1;
  if (d == -1) {
    const char * val = coin_getenv("COIN_DEBUG_3DS");
    d = val ? atoi(val) : 0;
  }
  return d;
}

// *************************************************************************

// This is the only interface exposed to code outside this file.

SbBool
coin_3ds_read_file(SoInput *in, SoSeparator *&root,
                   int appendNormals, float creaseAngle,
                   SbBool loadMaterials, SbBool loadTextures,
                   SbBool loadObjNames, SbBool indexedTriSet,
                   SbBool centerModel, float modelSize)
{
  SoStream s;
  s.setBinary(TRUE);
  s.setEndianOrdering(SoStream::LITTLE_ENDIAN_STREAM);
  s.wrapSoInput(in);

  return read3dsFile(&s, root, appendNormals, creaseAngle, loadMaterials,
                     loadTextures, loadObjNames, indexedTriSet,
                     centerModel, modelSize);
}

// *************************************************************************
