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
  \class SoShadowGroup SoShadowGroup.h FXViz/nodes/SoShadowGroup.h
  \brief The SoShadowGroup node is a group node used for shadow rendering.

  \ingroup coin_fxviz

  Children of this node can receive shadows, and cast shadows on other children.
  Use the SoShadowStyle node to control shadow casters and shadow receivers.

  Please note that all shadow casters will be rendered twice. Once to
  create the shadow map, and once for normal rendering. If you're
  having performance issues, you should consider reducing the number of
  shadow casters.

  The algorithm used to render the shadows is Variance Shadow Maps
  (http://www.punkuser.net/vsm/). As an extra bonus, all geometry
  rendered with shadows can also be rendered with per fragment Phong
  lighting.

  This node will search its subgraph and calculate shadows for all
  SoSpotLight nodes. The node will use one texture unit for each spot
  light, so for this node to work 100%, you need to have
  num-spotlights free texture units while rendering the subgraph.

  Currently, we only support scenes with maximum two texture units
  active while doing shadow rendering (unit 0 and unit 1). This is due
  to the fact that we emulate the OpenGL shading model in a shader
  program, and we're still working on creating a solution that updates
  the shader program during the scene graph traversal. Right now a
  shader program is created when entering the SoShadowGroup node, and
  this is used for the entire subgraph.


  <b>FILE FORMAT/DEFAULTS:</b>
  \code
    SoShadowGroup {
      isActive TRUE
      intensity 0.5
      precision 0.5
      quality 0.5
      shadowCachingEnabled TRUE
      visibilityRadius -1.0
      visibilityFlag LONGEST_BBOX_EDGE_FACTOR

      epsilon 0.00001
      threshold 0.1
      smoothBorder 0.0

    }
  \endcode

  Example scene graph:
  \code
  #Inventor V2.1 ascii

  # to get some lighting when headlight is turned off in the viewer
  DirectionalLight { direction 0 0 -1 intensity 0.2 }

  ShadowGroup {
    quality 1 # to get per pixel lighting

    ShadowStyle { style NO_SHADOWING }

    SpotLight {
      location -8 -8 8.0
      direction 1 1 -1
      cutOffAngle 0.35
      dropOffRate 0.7
    }

    ShadowStyle { style CASTS_SHADOW_AND_SHADOWED }

    Separator {
      Complexity { value 1.0 }
      Material { diffuseColor 1 1 0 specularColor 1 1 1 shininess 0.9 }
      Shuttle { translation0 -3 1 0 translation1 3 -5 0 speed 0.25 on TRUE }
      Translation { translation -5 0 2 }
      Sphere { radius 2.0 }
    }

    Separator {
      Material { diffuseColor 1 0 0 specularColor 1 1 1 shininess 0.9 }
      Shuttle { translation0 0 -5 0 translation1 0 5 0 speed 0.15 on TRUE }
      Translation { translation 0 0 -3 }
      Cube { depth 1.8 }
    }
    Separator {
      Material { diffuseColor 0 1 0 specularColor 1 1 1 shininess 0.9 }
      Shuttle { translation0 -5 0 0 translation1 5 0 0 speed 0.3 on TRUE }
      Translation { translation 0 0 -3 }
      Cube { }
    }

    ShadowStyle { style SHADOWED }
    Coordinate3 { point [ -10 -10 -3, 10 -10 -3, 10 10 -3, -10 10 -3 ] }
    Material { specularColor 1 1 1 shininess 0.9 }

    Complexity { textureQuality 0.1 }
    Texture2 { image 2 2 3 0xffffff 0x225588 0x225588 0xffffff }
    Texture2Transform { scaleFactor 4 4 }
    FaceSet { numVertices 4 }
  }

  \endcode

  \since Coin 2.5
*/


/*!
  \var SoSFBool SoShadowGroup::isActive

  Use this field to turn shadow rendering for the subgraph
  on/off. Default value is TRUE.
*/

/*!
  \var SoSFFloat SoShadowGroup::intensity

  Not used yet. Provided for TGS Inventor compatibility.
*/

/*!
  \var SoSFFloat SoShadowGroup::precision

  Use to calculate the size of the shadow map. A precision of 1.0
  means the maximum shadow buffer size will be used (typically
  2048x2048 on current graphics cards). Default value is 0.5.
*/

/*!
  \var SoSFFloat SoShadowGroup::quality

  Can be used to tune the shader program complexity. A higher value
  will mean that more calculations are done per fragment instead of
  per vertex. Default value is 0.5.

*/

/*!
  \var SoSFBool SoShadowGroup::shadowCachingEnabled

  Not used yet. Provided for TGS Inventor compatibility.
*/

/*!
  \var SoSFFloat SoShadowGroup::visibilityNearRadius

  Can be used to manually set the near clipping plane of the shadow
  maps.  If a negative value is provided, the group will calculate a
  near plane based on the bounding box of the children. Default value
  is -1.0.

  \sa visibilityFlag
*/

/*!
  \var SoSFFloat SoShadowGroup::visibilityRadius

  Can be used to manually set the far clipping plane of the shadow
  maps.  If a negative value is provided, the group will calculate a
  near plane based on the bounding box of the children. Default value
  is -1.0.

  \sa visibilityFlag
*/

/*!
  \var SoSFEnum SoShadowGroup::visibilityFlag

  Determines how visibilityRadius and visibilitNearRadius are used to
  calculate near and far clipping planes for the shadow volume.
*/

/*!
  SoShadowGroup::VisibilityFlag SoShadowGroup::ABSOLUTE_RADIUS

  The absolute values of visibilityNearRadius and visibilityRadius will be used.
*/

/*!
  SoShadowGroup::VisibilityFlag SoShadowGroup::LONGEST_BBOX_EDGE_FACTOR

  The longest bounding box edge will be used to determine near and far clipping planes.

*/

/*!
  SoShadowGroup::VisibilityFlag SoShadowGroup::PROJECTED_BBOX_DEPTH_FACTOR

  The bounding box depth (projected to face the camera) will be used to calculate the clipping planes.

*/

/*!
  \var SoSFInt32 SoShadowGroup::smoothBorder

  We have some problems with this feature so it's not supported at the
  moment.

  Used to add shadow border smoothing. This is currently done as a
  post processing step on the shadow map. The algorithm used is Gauss
  Smoothing, but in the future we'll probably change this, and use a
  summed area sampling method instead. The value should be a
  number between 0 (no smoothing), and 1 (max smoothing).

  If you want to enable smoothing, choosing a low value (~0.1) works
  best in the current implementation.

  Default value is 0.0.
*/

/*!
  \var SoSFFloat SoShadowGroup::epsilon

  Epsilon is used to offset the shadow map depth from the model depth.
  Should be set to as low a number as possible without causing
  flickering in the shadows or on non-shadowed objects. Default value
  is 0.00001.
*/

/*!
  \var SoSFFloat SoShadowGroup::threshold

  Can be used to avoid light bleeding in merged shadows cast from different objects.

  A threshold to completely eliminate all light bleeding can be
  computed from the ratio of overlapping occluder distances from the
  light's perspective. See
  http://forum.beyond3d.com/showthread.php?t=38165 for a discussion
  about this problem.

*/


// use to increase the VSM precision by using all four components
#define DISTRIBUTE_FACTOR 64.0

// use to increase precision by one bit at the cost of some extra processing
#define USE_NEGATIVE 1

// *************************************************************************

#include <FXViz/nodes/SoShadowGroup.h>
#include "coindefs.h"

#include <cmath>

#include <Inventor/nodes/SoSpotLight.h>
#include <Inventor/nodes/SoPointLight.h>
#include <Inventor/nodes/SoDirectionalLight.h>
#include <Inventor/nodes/SoSceneTexture2.h>
#include <Inventor/nodes/SoTransparencyType.h>
#include <Inventor/nodes/SoPerspectiveCamera.h>
#include <Inventor/nodes/SoOrthographicCamera.h>
#include <Inventor/nodes/SoVertexShader.h>
#include <Inventor/nodes/SoFragmentShader.h>
#include <Inventor/nodes/SoShaderProgram.h>
#include <Inventor/nodes/SoShaderParameter.h>
#include <Inventor/nodes/SoCallback.h>
#include <Inventor/nodes/SoClipPlane.h>
#include <Inventor/nodes/SoInfo.h>
#include <Inventor/elements/SoShapeStyleElement.h>
#include <Inventor/elements/SoLightElement.h>
#include <Inventor/elements/SoMultiTextureMatrixElement.h>
#include <Inventor/elements/SoModelMatrixElement.h>
#include <Inventor/elements/SoViewingMatrixElement.h>
#include <Inventor/elements/SoViewVolumeElement.h>
#include <Inventor/elements/SoViewportRegionElement.h>
#include <Inventor/elements/SoGLCacheContextElement.h>
#include <Inventor/elements/SoLazyElement.h>
#include <Inventor/elements/SoTextureQualityElement.h>
#include <Inventor/elements/SoMaterialBindingElement.h>
#include <Inventor/elements/SoNormalElement.h>
#include <Inventor/elements/SoOverrideElement.h>
#include <Inventor/elements/SoTextureOverrideElement.h>
#include <Inventor/elements/SoEnvironmentElement.h>
#include <Inventor/elements/SoClipPlaneElement.h>
#include <Inventor/annex/FXViz/elements/SoShadowStyleElement.h>
#include <Inventor/annex/FXViz/elements/SoGLShadowCullingElement.h>
#include <Inventor/annex/FXViz/nodes/SoShadowStyle.h>
#include <Inventor/annex/FXViz/nodes/SoShadowCulling.h>
#include <Inventor/annex/FXViz/nodes/SoShadowSpotLight.h>
#include <Inventor/annex/FXViz/nodes/SoShadowDirectionalLight.h>
#include <Inventor/nodes/SoCoordinate3.h>
#include <Inventor/nodes/SoTextureUnit.h>
#include <Inventor/nodes/SoShapeHints.h>
#include <Inventor/nodes/SoFaceSet.h>
#include <Inventor/nodes/SoOrthographicCamera.h>
#include <Inventor/SoPath.h>
#include <Inventor/misc/SoTempPath.h>
#include <Inventor/misc/SoGLDriverDatabase.h>
#include <Inventor/actions/SoSearchAction.h>
#include <Inventor/elements/SoShapeStyleElement.h>
#include <Inventor/elements/SoTextureUnitElement.h>
#include <Inventor/elements/SoGLMultiTextureEnabledElement.h>
#include <Inventor/elements/SoCacheElement.h>
#include <Inventor/actions/SoGLRenderAction.h>
#include <Inventor/actions/SoGetMatrixAction.h>
#include <Inventor/actions/SoGetBoundingBoxAction.h>
#include <Inventor/lists/SbList.h>
#include <Inventor/errors/SoDebugError.h>
#include <Inventor/SbMatrix.h>
#include <Inventor/C/glue/gl.h>

#include "nodes/SoSubNodeP.h"
#include "shaders/SoShader.h"
#include "glue/glp.h"
#include "misc/SoShaderGenerator.h"
#include "caches/SoShaderProgramCache.h"
#include "rendering/SoGL.h"

// *************************************************************************

class SoShadowLightCache {
public:
  SoShadowLightCache(SoState * state,
                     const SoPath * path,
                     SoShadowGroup * sg,
                     SoNode * scene,
                     SoNode * bboxscene,
                     const int gausskernelsize,
                     const float gaussstandarddeviation)
  {
    const cc_glglue * glue = cc_glglue_instance(SoGLCacheContextElement::get(state));

    GLint maxsize = 2048;
    GLint maxtexsize = 2048;

    // Testing for maximum proxy texture size doesn't seem to work, so
    // we just have to hardcode the maximum size to 2048 for now.  We
    // still use the proxy texture test in case the maximum size is
    // something smaller than 2048 though.  pederb, 2007-05-03

    // glGetIntegerv(GL_MAX_RENDERBUFFER_SIZE_EXT, &maxsize);
    // glGetIntegerv(GL_MAX_TEXTURE_SIZE, &maxtexsize);
    // if (maxtexsize < maxsize) maxsize = maxtexsize;

    GLenum internalformat = GL_RGBA16F_ARB;
    GLenum format = GL_RGBA;
    GLenum type = GL_FLOAT;

    while (!coin_glglue_is_texture_size_legal(glue, maxsize, maxsize, 0, internalformat, format, type, TRUE) && (maxsize != 0)) {
      maxsize >>= 1;
    }
    if (maxsize == 0) { // Can happen on CentOS 7 in VirtualBox
      glGetIntegerv(GL_MAX_RENDERBUFFER_SIZE, &maxsize);
      glGetIntegerv(GL_MAX_TEXTURE_SIZE, &maxtexsize);
      if (maxtexsize < maxsize) maxsize = maxtexsize;
    }
    const int TEXSIZE = coin_geq_power_of_two((int) (sg->precision.getValue() * SbMin(maxsize, maxtexsize)));

    this->lightid = -1;
    this->vsm_program = NULL;
    this->vsm_farval = NULL;
    this->vsm_nearval = NULL;
    this->gaussmap = NULL;
    this->texunit = -1;
    this->bboxnode = new SoSeparator;
    this->bboxnode->ref();

    this->shadowmapid = new SoShaderParameter1i;
    this->shadowmapid->ref();

    this->fragment_farval = new SoShaderParameter1f;
    this->fragment_farval->ref();

    this->fragment_nearval = new SoShaderParameter1f;
    this->fragment_nearval->ref();

    this->fragment_lightplane = new SoShaderParameter4f;
    this->fragment_lightplane->ref();

    this->maxshadowdistance = new SoShaderParameter1f;
    this->maxshadowdistance->ref();

    this->path = path->copy();
    this->path->ref();
    assert(((SoFullPath*)path)->getTail()->isOfType(SoLight::getClassTypeId()));

    this->light = (SoLight*)((SoFullPath*)path)->getTail();
    this->light->ref();

    this->createVSMProgram();
    this->depthmap = new SoSceneTexture2;
    this->depthmap->ref();
    this->depthmap->transparencyFunction = SoSceneTexture2::NONE;
    this->depthmap->size = SbVec2s(TEXSIZE, TEXSIZE);
    this->depthmap->wrapS = SoSceneTexture2::CLAMP_TO_BORDER;
    this->depthmap->wrapT = SoSceneTexture2::CLAMP_TO_BORDER;

    if (this->vsm_program) {
      this->depthmap->type = SoSceneTexture2::RGBA32F;
      this->depthmap->backgroundColor = SbVec4f(1.0f, 1.0f, 1.0f, 1.0f);
    }
    else {
      this->depthmap->type = SoSceneTexture2::DEPTH;
    }
    SoTransparencyType * tt = new SoTransparencyType;
    tt->value = SoTransparencyType::NONE;

    this->depthmap->sceneTransparencyType = tt;

    if (this->light->isOfType(SoDirectionalLight::getClassTypeId())) {
      this->camera = new SoOrthographicCamera;
    }
    else {
      this->camera = new SoPerspectiveCamera;
    }
    this->camera->ref();
    this->camera->viewportMapping = SoCamera::LEAVE_ALONE;

    SoSeparator * sep = new SoSeparator;
    sep->addChild(this->camera);

    SoCallback * cb = new SoCallback;
    cb->setCallback(shadowmap_glcallback, this);

    sep->addChild(cb);
    if (this->vsm_program) sep->addChild(this->vsm_program);

    if (scene->isOfType(SoShadowGroup::getClassTypeId())) {
      SoShadowGroup * g = (SoShadowGroup*) scene;
      for (int i = 0; i < g->getNumChildren(); i++) {
        sep->addChild(g->getChild(i));
      }
    }
    else sep->addChild(scene);

    if (bboxscene->isOfType(SoShadowGroup::getClassTypeId())) {
      SoShadowGroup * g = (SoShadowGroup*) bboxscene;
      for (int i = 0; i < g->getNumChildren(); i++) {
        this->bboxnode->addChild(g->getChild(i));
      }
    }
    else {
      this->bboxnode->addChild(bboxscene);
    }

    cb = new SoCallback;
    cb->setCallback(shadowmap_post_glcallback, this);
    sep->addChild(cb);

    this->depthmap->scene = sep;
    this->depthmapscene = sep;
    this->depthmapscene->ref();
    this->matrix = SbMatrix::identity();

    if (gausskernelsize > 0) {
      this->gaussmap = new SoSceneTexture2;
      this->gaussmap->ref();
      this->gaussmap->transparencyFunction = SoSceneTexture2::NONE;
      this->gaussmap->size = SbVec2s(TEXSIZE, TEXSIZE);
      this->gaussmap->wrapS = SoSceneTexture2::CLAMP_TO_BORDER;
      this->gaussmap->wrapT = SoSceneTexture2::CLAMP_TO_BORDER;

      this->gaussmap->type = SoSceneTexture2::RGBA32F;
      this->gaussmap->backgroundColor = SbVec4f(1.0f, 1.0f, 1.0f, 1.0f);

      SoShaderProgram * shader = this->createGaussFilter(TEXSIZE, gausskernelsize, gaussstandarddeviation);
      this->gaussmap->scene = this->createGaussSG(shader, this->depthmap);
    }
  }
  ~SoShadowLightCache() {
    if (this->depthmapscene) this->depthmapscene->unref();
    if (this->bboxnode) this->bboxnode->unref();
    if (this->maxshadowdistance) this->maxshadowdistance->unref();
    if (this->vsm_program) this->vsm_program->unref();
    if (this->vsm_farval) this->vsm_farval->unref();
    if (this->vsm_nearval) this->vsm_nearval->unref();
    if (this->fragment_farval) this->fragment_farval->unref();
    if (this->shadowmapid) this->shadowmapid->unref();
    if (this->fragment_nearval) this->fragment_nearval->unref();
    if (this->fragment_lightplane) this->fragment_lightplane->unref();
    if (this->light) this->light->unref();
    if (this->path) this->path->unref();
    if (this->gaussmap) this->gaussmap->unref();
    if (this->depthmap) this->depthmap->unref();
    if (this->camera) this->camera->unref();
  }

  static int
  write_short(FILE * fp, unsigned short val)
  {
    unsigned char tmp[2];
    tmp[0] = (unsigned char)(val >> 8);
    tmp[1] = (unsigned char)(val & 0xff);
    return (int)fwrite(&tmp, 2, 1, fp);
  }

  int dumpBitmap(const char * filename) const {
    GLint vp[4];
    glGetIntegerv(GL_VIEWPORT, vp);
    int width = vp[2];
    int height = vp[3];
    int comp = 1;

    unsigned char * bytes = new unsigned char[width*height*comp];
    glFlush();
    glReadPixels(0,0, width, height, GL_RED, GL_UNSIGNED_BYTE, bytes);

    FILE * fp = fopen(filename, "wb");
    if (!fp) {
      return 0;
    }

    write_short(fp, 0x01da); /* imagic */
    write_short(fp, 0x0001); /* raw (no rle yet) */

    if (comp == 1)
      write_short(fp, 0x0002); /* 2 dimensions (heightmap) */
    else
      write_short(fp, 0x0003); /* 3 dimensions */

    write_short(fp, (unsigned short) width);
    write_short(fp, (unsigned short) height);
    write_short(fp, (unsigned short) comp);

    unsigned char buf[500];
    memset(buf, 0, 500);
    buf[7] = 255; /* set maximum pixel value to 255 */
    strcpy((char *)buf+8, "https://github.com/coin3d/");
    fwrite(buf, 1, 500, fp);

    unsigned char * tmpbuf = new unsigned char[width];

    int x, y, c;
    for (c = 0; c < comp; c++) {
      for (y = 0; y < height; y++) {
        for (x = 0; x < width; x++) {
          tmpbuf[x] = bytes[x * comp + y * comp * width + c];
        }
        fwrite(tmpbuf, 1, width, fp);
      }
    }
    delete [] tmpbuf;
    fclose(fp);
    delete [] bytes;
    return 1;
  }	
  SbBox3f toCameraSpace(const SbXfBox3f & worldbox) const;
  static void shadowmap_glcallback(void * closure, SoAction * action);
  static void shadowmap_post_glcallback(void * closure, SoAction * action);
  void createVSMProgram(void);
  SoShaderProgram * createGaussFilter(const int texsize, const int size, const float stdev);
  SoSeparator * createGaussSG(SoShaderProgram * program, SoSceneTexture2 * tex);

  SbMatrix matrix;
  SoPath * path;
  SoLight * light;
  SoSceneTexture2 * depthmap;
  SoNode * depthmapscene;
  SoSceneTexture2 * gaussmap;
  SoCamera * camera;
  float farval;
  float nearval;
  int texunit;
  int lightid;

  SoSeparator * bboxnode;
  SoShaderProgram * vsm_program;
  SoShaderParameter1i * shadowmapid;
  SoShaderParameter1f * vsm_farval;
  SoShaderParameter1f * vsm_nearval;
  SoShaderParameter1f * fragment_farval;
  SoShaderParameter1f * fragment_nearval;
  SoShaderParameter4f * fragment_lightplane;
  SoShaderGenerator vsm_vertex_generator;
  SoShaderGenerator vsm_fragment_generator;
  SoShaderParameter1f * maxshadowdistance;

  SoColorPacker colorpacker;
  SbColor color;
};

class SoShadowGroupP {
public:
  SoShadowGroupP(SoShadowGroup * master) :
    master(master),
    bboxaction(SbViewportRegion(SbVec2s(100,100))),
    matrixaction(SbViewportRegion(SbVec2s(100,100))),
    shadowlightsvalid(FALSE),
    needscenesearch(TRUE),
    shaderprogram(NULL),
    vertexshader(NULL),
    fragmentshader(NULL),
    vertexshadercache(NULL),
    fragmentshadercache(NULL),
    texunit0(NULL),
    texunit1(NULL),
    lightmodel(NULL),
    twosided(NULL),
    numtexunitsinscene(1),
    hasclipplanes(FALSE),
    subgraphsearchenabled(TRUE)
  {
    this->shaderprogram = new SoShaderProgram;
    this->shaderprogram->ref();
    this->shaderprogram->setEnableCallback(shader_enable_cb, this);
    this->vertexshader = new SoVertexShader;
    this->vertexshader->ref();
    this->fragmentshader = new SoFragmentShader;
    this->fragmentshader->ref();

    this->cameratransform = new SoShaderParameterMatrix;
    this->cameratransform->name = "cameraTransform";
    this->cameratransform->ref();

    this->shaderprogram->shaderObject.set1Value(0, this->vertexshader);
    this->shaderprogram->shaderObject.set1Value(1, this->fragmentshader);
  }
  ~SoShadowGroupP() {
    this->clearLightPaths();
    if (this->lightmodel) this->lightmodel->unref();
    if (this->twosided) this->twosided->unref();
    if (this->texunit0) this->texunit0->unref();
    if (this->texunit1) this->texunit1->unref();
    if (this->vertexshadercache) this->vertexshadercache->unref();
    if (this->fragmentshadercache) this->fragmentshadercache->unref();
    if (this->cameratransform) this->cameratransform->unref();
    if (this->vertexshader) this->vertexshader->unref();
    if (this->fragmentshader) this->fragmentshader->unref();
    if (this->shaderprogram) this->shaderprogram->unref();
    this->deleteShadowLights();
  }

  SoShaderProgram * createVSMProgram(void);

  void clearLightPaths(void) {
    for (int i = 0; i < this->lightpaths.getLength(); i++) {
      this->lightpaths[i]->unref();
    }
    this->lightpaths.truncate(0);
  }
  void copyLightPaths(const SoPathList & pl) {
    for (int i = 0; i < pl.getLength(); i++) {
      SoFullPath * p = (SoFullPath*) pl[i];
      SoNode * tail = p->getTail();
      if (tail->isOfType(SoSpotLight::getClassTypeId()) ||
          tail->isOfType(SoShadowDirectionalLight::getClassTypeId())) {
        SoTempPath * tp = new SoTempPath(p->getLength());
        tp->ref();
        tp->setHead(p->getHead());

        for (int j = 1; j < p->getLength(); j++) {
          tp->append(p->getNode(j));
        }
        this->lightpaths.append(tp);
      }
    }
  }
  void getQuality(SoState * COIN_UNUSED_ARG(state), SbBool & perpixelspot, SbBool & perpixelother) {
    float quality = this->master->quality.getValue();
    perpixelspot = FALSE;
    perpixelother = FALSE;

    if (quality > 0.3) {
      perpixelspot = TRUE;
    }
    if (quality > 0.7) {
      perpixelother = TRUE;
    }
  }
  void deleteShadowLights(void) {
    for (int i = 0; i < this->shadowlights.getLength(); i++) {
      delete this->shadowlights[i];
    }
    this->shadowlights.truncate(0);
  }

  static bool supported(const cc_glglue * glctx, SbString& reason);

  static void shader_enable_cb(void * closure,
                               SoState * state,
                               const SbBool enable);

  void GLRender(SoGLRenderAction * action, const SbBool inpath);
  void setVertexShader(SoState * state);
  void setFragmentShader(SoState * state);
  void updateSpotCamera(SoState * state, SoShadowLightCache * cache, const SbMatrix & transform);
  void updateDirectionalCamera(SoState * state, SoShadowLightCache * cache, const SbMatrix & transform);
  const SbXfBox3f & calcBBox(SoShadowLightCache * cache);

  void renderDepthMap(SoShadowLightCache * cache,
                      SoGLRenderAction * action);
  void updateShadowLights(SoGLRenderAction * action);

  int32_t getFog(SoState * state) {
    return SoEnvironmentElement::getFogType(state);
  }

  SoShadowGroup * master;
  SoSearchAction searchaction;
  SbList <SoTempPath*> lightpaths;
  SoGetBoundingBoxAction bboxaction;
  SoGetMatrixAction matrixaction;

  SbBool shadowlightsvalid;
  SbBool needscenesearch;
  SbList <SoShadowLightCache*> shadowlights;

  SoShaderProgram * shaderprogram;
  SoVertexShader * vertexshader;
  SoFragmentShader * fragmentshader;

  SoShaderGenerator vertexgenerator;
  SoShaderGenerator fragmentgenerator;
  SoShaderParameterMatrix * cameratransform;

  SoShaderProgramCache * vertexshadercache;
  SoShaderProgramCache * fragmentshadercache;
  SoShaderParameter1i * texunit0;
  SoShaderParameter1i * texunit1;
  SoShaderParameter1i * lightmodel;
  SoShaderParameter1i * twosided;

  int numtexunitsinscene;
  SbBool hasclipplanes;
  SbBool subgraphsearchenabled;
};

// *************************************************************************

#define PRIVATE(obj) ((obj)->pimpl)
#define PUBLIC(obj) ((obj)->master)

SO_NODE_SOURCE(SoShadowGroup);

/*!
  Default constructor.
*/
SoShadowGroup::SoShadowGroup(void)
{
  PRIVATE(this) = new SoShadowGroupP(this);

  SO_NODE_INTERNAL_CONSTRUCTOR(SoShadowGroup);

  SO_NODE_ADD_FIELD(isActive, (TRUE));
  SO_NODE_ADD_FIELD(intensity, (0.5f));
  SO_NODE_ADD_FIELD(precision, (0.5f));
  SO_NODE_ADD_FIELD(quality, (0.5f));
  SO_NODE_ADD_FIELD(shadowCachingEnabled, (TRUE));
  SO_NODE_ADD_FIELD(visibilityNearRadius, (-1.0f));
  SO_NODE_ADD_FIELD(visibilityRadius, (-1.0f));
  SO_NODE_ADD_FIELD(epsilon, (0.00001f));
  SO_NODE_ADD_FIELD(threshold, (0.1f));
  SO_NODE_ADD_FIELD(smoothBorder, (0.0f));

  SO_NODE_ADD_FIELD(visibilityFlag, (LONGEST_BBOX_EDGE_FACTOR));

  SO_NODE_DEFINE_ENUM_VALUE(VisibilityFlag, LONGEST_BBOX_EDGE_FACTOR);
  SO_NODE_DEFINE_ENUM_VALUE(VisibilityFlag, ABSOLUTE_RADIUS);
  SO_NODE_DEFINE_ENUM_VALUE(VisibilityFlag, PROJECTED_BBOX_DEPTH_FACTOR);
  SO_NODE_SET_SF_ENUM_TYPE(visibilityFlag, VisibilityFlag);

}

/*!
  Destructor.
*/
SoShadowGroup::~SoShadowGroup()
{
  delete PRIVATE(this);
}

/*!
  \copydetails SoNode::initClass(void)
*/
void
SoShadowGroup::initClass(void)
{
  SO_NODE_INTERNAL_INIT_CLASS(SoShadowGroup, SO_FROM_COIN_2_5);
}

void
SoShadowGroup::init(void)
{
  SoShadowGroup::initClass();
  SoShadowStyleElement::initClass();
  SoGLShadowCullingElement::initClass();
  SoShadowStyle::initClass();
  SoShadowSpotLight::initClass();
  SoShadowDirectionalLight::initClass();
  SoShadowCulling::initClass();
}

// *************************************************************************

/*!
  Reports whether or not the shadow nodes can be used successfully on
  the current system.

  The result will depend on the specific qualities of the graphics
  card and OpenGL driver on the system.

  An important note about this function:

  The API design of this function has a serious shortcoming, as
  features of OpenGL should be tested within an OpenGL context, and
  this function does not provide any means of specifying the
  context. It is implemented in this manner to match the function
  signature in TGS Inventor, for compatibility reasons.

  (A temporary offscreen OpenGL context is set up for the feature
  tests. This should usually be sufficient to decide whether or not
  the graphics driver / card supports the features needed for
  rendering shadows.)

  \since Coin 3.1
*/
SbBool
SoShadowGroup::isSupported(void)
{
  static int supp = -1;
  if (supp != -1) { return supp ? true : false; }

  void * glctx = cc_glglue_context_create_offscreen(256, 256);
  SbBool ok = cc_glglue_context_make_current(glctx);
  if (!ok) {
    SoDebugError::postWarning("SoShadowGroupP::isSupported",
                              "Could not open an OpenGL context.");
    return false;
  }

  const cc_glglue * glue = cc_glglue_instance_from_context_ptr(glctx);

  SbString unused;
  const bool supported = SoShadowGroupP::supported(glue, unused);
  supp = supported ? 1 : 0;

  cc_glglue_context_reinstate_previous(glctx);
  cc_glglue_context_destruct(glctx);

  return supported;
}

// *************************************************************************

void
SoShadowGroup::GLRenderBelowPath(SoGLRenderAction * action)
{
  PRIVATE(this)->GLRender(action, FALSE);
}

void
SoShadowGroup::GLRenderInPath(SoGLRenderAction * action)
{
  PRIVATE(this)->GLRender(action, TRUE);
}

void
SoShadowGroup::notify(SoNotList * nl)
{
  // FIXME: examine notification chain, and detect when an
  // SoSpotLight/SoShadowDirectionalLight is changed. When this
  // happens we can just invalidate the depth map for that spot light,
  // and not the others.

  SoNotRec * rec = nl->getLastRec();
  if (rec->getBase() != this) {
    // was not notified through a field, subgraph was changed

    rec = nl->getFirstRecAtNode();
    if (rec) {
      SoNode * node = (SoNode*) rec->getBase();
      if (node->isOfType(SoGroup::getClassTypeId())) {
        // first rec was from a group node, we need to search the scene graph again
        PRIVATE(this)->shadowlightsvalid = FALSE;

        if (PRIVATE(this)->subgraphsearchenabled) {
          PRIVATE(this)->needscenesearch = TRUE;
        }
      }
      else {
        PRIVATE(this)->shadowlightsvalid = FALSE;
      }
    }
  }

  if (PRIVATE(this)->vertexshadercache) {
    PRIVATE(this)->vertexshadercache->invalidate();
  }
  if (PRIVATE(this)->fragmentshadercache) {
    PRIVATE(this)->fragmentshadercache->invalidate();
  }
  inherited::notify(nl);
}

/*!

  By default, the SoShadowGroup node will search its subgraph for new
  spot lights whenever a group node under it is touched. However, this
  might lead to bad performance in some cases so it is possible to
  disable this feature using this method. If you do disable this
  feature, make sure you enable it again before inserting a new spot
  light, or insert all spot lights in the scene graph before you
  render the scene once, and just set "on" to FALSE if you want to toggle
  spot lights on/off on the fly.

  \since Coin 2.6
 */
void
SoShadowGroup::enableSubgraphSearchOnNotify(const SbBool onoff)
{
  PRIVATE(this)->subgraphsearchenabled = onoff;
}

#undef PRIVATE

// *************************************************************************

void
SoShadowGroupP::updateShadowLights(SoGLRenderAction * action)
{
  int i;
  SoState * state = action->getState();

  if (!this->shadowlightsvalid) {
    int lightidoffset = SoLightElement::getLights(state).getLength();
    float smoothing = PUBLIC(this)->smoothBorder.getValue();
    smoothing = 0.0f; // FIXME: temporary until we have time to fix this feature

    int gaussmatrixsize = 0;
    float gaussstandarddeviation = 0.6f;

    // just hardcode some values for now
    // FIXME: reactivate when properly implemented
    //if (smoothing > 0.9) gaussmatrixsize = 7;
    //else if (smoothing > 0.5) gaussmatrixsize = 5;
    //else if (smoothing > 0.01) gaussmatrixsize = 3;

    const cc_glglue * glue = cc_glglue_instance(SoGLCacheContextElement::get(state));

    if (this->needscenesearch) {
      this->hasclipplanes = SoClipPlaneElement::getInstance(state)->getNum() > 0;
      if (!this->hasclipplanes) {
        this->searchaction.setType(SoClipPlane::getClassTypeId());
        this->searchaction.setInterest(SoSearchAction::FIRST);
        this->searchaction.setSearchingAll(FALSE);
        this->searchaction.apply(PUBLIC(this));
        if (this->searchaction.getPath()) {
          this->hasclipplanes = TRUE;
        }
        this->searchaction.reset();
      }
      // first, search for texture unit nodes
      this->searchaction.setType(SoTextureUnit::getClassTypeId());
      this->searchaction.setInterest(SoSearchAction::ALL);
      this->searchaction.setSearchingAll(FALSE);
      this->searchaction.apply(PUBLIC(this));

      int lastenabled;
      (void) SoMultiTextureEnabledElement::getEnabledUnits(state, lastenabled);
      this->numtexunitsinscene = lastenabled + 1;

      for (i = 0; i < this->searchaction.getPaths().getLength(); i++) {
        SoFullPath * p = (SoFullPath*) this->searchaction.getPaths()[i];
        SoTextureUnit * unit = (SoTextureUnit*) p->getTail();
        if (unit->unit.getValue() >= this->numtexunitsinscene) {
          this->numtexunitsinscene = unit->unit.getValue() + 1;
        }
      }
      if (this->numtexunitsinscene == 0) this->numtexunitsinscene = 1;

      this->searchaction.reset();
      this->searchaction.setType(SoLight::getClassTypeId());
      this->searchaction.setInterest(SoSearchAction::ALL);
      this->searchaction.setSearchingAll(FALSE);
      this->searchaction.apply(PUBLIC(this));
      this->clearLightPaths();
      this->copyLightPaths(this->searchaction.getPaths());
      this->searchaction.reset();
      this->needscenesearch = FALSE;
    }
    int maxunits = cc_glglue_max_texture_units(glue);

    int maxlights = maxunits - this->numtexunitsinscene;
    SbList <SoTempPath*> & pl = this->lightpaths;

    int numlights = 0;
    for (i = 0; i < pl.getLength(); i++) {
      SoLight * light = (SoLight*)((SoFullPath*)(pl[i]))->getTail();
      if (light->on.getValue() && (numlights < maxlights)) numlights++;
    }
    if (numlights != this->shadowlights.getLength()) {
      // just delete and recreate all if the number of spot lights have changed
      this->deleteShadowLights();
      int id = lightidoffset;
      for (i = 0; i < pl.getLength(); i++) {
        SoLight * light = (SoLight*)((SoFullPath*)pl[i])->getTail();
        if (light->on.getValue() && (this->shadowlights.getLength() < maxlights)) {
          SoNode * scene = PUBLIC(this);
          SoNode * bboxscene = PUBLIC(this);
          if (light->isOfType(SoShadowSpotLight::getClassTypeId())) {
            SoShadowSpotLight * ssl = (SoShadowSpotLight*) light;
            if (ssl->shadowMapScene.getValue()) {
              scene = ssl->shadowMapScene.getValue();
            }
          }
          else if (light->isOfType(SoShadowDirectionalLight::getClassTypeId())) {
            SoShadowDirectionalLight * sl = (SoShadowDirectionalLight*) light;
            if (sl->shadowMapScene.getValue()) {
              scene = sl->shadowMapScene.getValue();
            }
          }
          SoShadowLightCache * cache = new SoShadowLightCache(state, pl[i],
                                                              PUBLIC(this),
                                                              scene,
                                                              bboxscene,
                                                              gaussmatrixsize,
                                                              gaussstandarddeviation);
          cache->lightid = id++;
          this->shadowlights.append(cache);
        }
      }
    }
    // validate if spot light paths are still valid
    int i2 = 0;
    int id = lightidoffset;
    for (i = 0; i < pl.getLength(); i++) {
      SoPath * path = pl[i];
      SoLight * light = (SoLight*) ((SoFullPath*)path)->getTail();
      if (light->on.getValue() && (i2 < maxlights)) {
        SoShadowLightCache * cache = this->shadowlights[i2];
        int unit = (maxunits - 1) - i2;
        int lightid = id++;
        if (unit != cache->texunit || lightid != cache->lightid) {
          if (this->vertexshadercache) this->vertexshadercache->invalidate();
          if (this->fragmentshadercache) this->fragmentshadercache->invalidate();
          cache->texunit = unit;
          cache->lightid = lightid;
        }
        if (*(cache->path) != *path) {
          cache->path->unref();
          cache->path = path->copy();
        }
        if (cache->light->isOfType(SoSpotLight::getClassTypeId())) {
          this->matrixaction.apply(path);
          this->updateSpotCamera(state, cache, this->matrixaction.getMatrix());
        }
        i2++;
      }
    }
    this->shadowlightsvalid = TRUE;
  }
  for (i = 0; i < this->shadowlights.getLength(); i++) {
    SoShadowLightCache * cache = this->shadowlights[i];
    if (cache->light->isOfType(SoDirectionalLight::getClassTypeId())) {
      this->matrixaction.apply(cache->path);
      this->updateDirectionalCamera(state, cache, this->matrixaction.getMatrix());
    }
    assert(cache->texunit >= 0);
    assert(cache->lightid >= 0);
    SoTextureUnitElement::set(state, PUBLIC(this), cache->texunit);

    SbMatrix mat = cache->matrix;

    assert(cache->texunit >= 0);

    SoMultiTextureMatrixElement::set(state, PUBLIC(this), cache->texunit, cache->matrix);
    this->renderDepthMap(cache, action);
    SoGLMultiTextureEnabledElement::set(state, PUBLIC(this), cache->texunit,
                                        SoGLMultiTextureEnabledElement::DISABLED);
  }
  SoTextureUnitElement::set(state, PUBLIC(this), 0);
}

const SbXfBox3f &
SoShadowGroupP::calcBBox(SoShadowLightCache * cache)
{
  if (cache->light->isOfType(SoShadowDirectionalLight::getClassTypeId())) {
    SoShadowDirectionalLight * sl = static_cast<SoShadowDirectionalLight*> (cache->light);
    SbVec3f size = sl->bboxSize.getValue();
    if (size[0] >= 0.0f && size[1] >= 0.0f && size[2] >= 0.0f) {
      SbVec3f center = sl->bboxCenter.getValue();
      size *= 0.5f;
      this->bboxaction.getXfBoundingBox() = SbXfBox3f(center-size, center+size);
    }
    else {
      this->bboxaction.apply(cache->bboxnode);
    }
  }
  else {
    this->bboxaction.apply(cache->bboxnode);
  }
  return this->bboxaction.getXfBoundingBox();
}

SbBox3f
SoShadowLightCache::toCameraSpace(const SbXfBox3f & worldbox) const
{
  SoCamera * cam = this->camera;
  SbMatrix mat;
  SbXfBox3f xbox = worldbox;
  mat.setTranslate(- cam->position.getValue());
  xbox.transform(mat);
  mat = cam->orientation.getValue().inverse();
  xbox.transform(mat);
  return xbox.project();
}

void
SoShadowGroupP::updateSpotCamera(SoState * COIN_UNUSED_ARG(state), SoShadowLightCache * cache, const SbMatrix & transform)
{
  SoCamera * cam = cache->camera;
  SoSpotLight * light = static_cast<SoSpotLight*> (cache->light);

  assert(cam->isOfType(SoPerspectiveCamera::getClassTypeId()));
  SbVec3f pos = light->location.getValue();
  transform.multVecMatrix(pos, pos);

  SbVec3f dir = light->direction.getValue();
  transform.multDirMatrix(dir, dir);
  (void) dir.normalize();
  float cutoff = light->cutOffAngle.getValue();
  cam->position.setValue(pos);
  // the maximum heightAngle we can render with a camera is < PI/2,.
  // The max cutoff is therefore PI/4. Some slack is needed, and 0.78
  // is about the maximum angle we can do.
  if (cutoff > 0.78f) cutoff = 0.78f;

  cam->orientation.setValue(SbRotation(SbVec3f(0.0f, 0.0f, -1.0f), dir));
  static_cast<SoPerspectiveCamera*> (cam)->heightAngle.setValue(cutoff * 2.0f);
  SoShadowGroup::VisibilityFlag visflag = (SoShadowGroup::VisibilityFlag) PUBLIC(this)->visibilityFlag.getValue();

  float visnear = PUBLIC(this)->visibilityNearRadius.getValue();
  float visfar = PUBLIC(this)->visibilityRadius.getValue();

  SbBool needbbox =
    (visflag == SoShadowGroup::LONGEST_BBOX_EDGE_FACTOR) ||
    (visflag == SoShadowGroup::PROJECTED_BBOX_DEPTH_FACTOR) ||
    ((visnear < 0.0f) || (visfar < 0.0f));

  if (light->isOfType(SoShadowSpotLight::getClassTypeId())) {
    SoShadowSpotLight * sslight = static_cast<SoShadowSpotLight*> (light);
    const float ssnear = sslight->nearDistance.getValue();
    const float ssfar = sslight->farDistance.getValue();

    if (ssnear > 0.0f && ssfar > ssnear) {
      visnear = ssnear;
      visfar = ssfar;
      needbbox = FALSE;
    }
  }
  if (needbbox) {
    const SbXfBox3f & worldbox = this->calcBBox(cache);
    SbBox3f box = cache->toCameraSpace(worldbox);

    // Bounding box was calculated in camera space, so we need to "flip"
    // the box (because camera is pointing in the (0,0,-1) direction
    // from origo.
    cache->nearval = -box.getMax()[2];
    cache->farval = -box.getMin()[2];

    const int depthbits = 16;
    float r = (float) pow(2.0, (double) depthbits);
    float nearlimit = cache->farval / r;

    if (cache->nearval < nearlimit) {
      cache->nearval = nearlimit;
    }
    const float SLACK = 0.001f;

    cache->nearval = cache->nearval * (1.0f - SLACK);
    cache->farval = cache->farval * (1.0f + SLACK);

    if (visflag == SoShadowGroup::LONGEST_BBOX_EDGE_FACTOR) {
      float sx,sy,sz;
      worldbox.getSize(sx, sy, sz);
      float smax =  SbMax(SbMax(sx, sy), sz);
        if (visnear > 0.0f) visnear = smax * visnear;
        if (visfar > 0.0f) visfar = smax  * visfar;
    }
    else if (visflag == SoShadowGroup::PROJECTED_BBOX_DEPTH_FACTOR) {
      if (visnear > 0.0f) visnear = cache->farval * visnear; // should be calculated from farval, not nearval
      if (visfar > 0.0f) visfar = cache->farval * visfar;
    }
  }

  if (visnear > 0.0f) cache->nearval = visnear;
  if (visfar > 0.0f) cache->farval = visfar;

  if (cache->nearval != cam->nearDistance.getValue()) {
    cam->nearDistance = cache->nearval;
  }
  if (cache->farval != cam->farDistance.getValue()) {
    cam->farDistance = cache->farval;
  }

  float realfarval = cutoff >= 0.0f ? cache->farval / float(cos(cutoff * 2.0f)) : cache->farval;
  cache->fragment_farval->value = realfarval;
  cache->vsm_farval->value = realfarval;

  cache->fragment_nearval->value = cache->nearval;
  cache->vsm_nearval->value = cache->nearval;

  SbViewVolume vv = cam->getViewVolume(1.0f);
  SbMatrix affine, proj;

  vv.getMatrices(affine, proj);
  cache->matrix = affine * proj;
}

void
SoShadowGroupP::updateDirectionalCamera(SoState * state, SoShadowLightCache * cache, const SbMatrix & transform)
{
  SoOrthographicCamera * cam = static_cast<SoOrthographicCamera*>(cache->camera);
  assert(cache->light->isOfType(SoShadowDirectionalLight::getClassTypeId()));
  SoShadowDirectionalLight * light = static_cast<SoShadowDirectionalLight*> (cache->light);

  float maxdist = light->maxShadowDistance.getValue();

  SbVec3f dir = light->direction.getValue();
  dir.normalize();
  transform.multDirMatrix(dir, dir);
  dir.normalize();
  cam->orientation.setValue(SbRotation(SbVec3f(0.0f, 0.0f, -1.0f), dir));

  SbViewVolume vv = SoViewVolumeElement::get(state);
  const SbXfBox3f & worldbox = this->calcBBox(cache);
  SbBool visible = TRUE;
  if (maxdist > 0.0f) {
    float nearv = vv.getNearDist();
    if (maxdist < nearv) visible = FALSE;
    else {
      maxdist -= nearv;
      float depth = vv.getDepth();
      if (maxdist > depth) maxdist = depth;
      vv = vv.zNarrow(1.0f, 1.0f - maxdist/depth);
    }
  }
  SbBox3f isect;
  if (visible) {
    isect = vv.intersectionBox(worldbox);
    if (isect.isEmpty()) visible = FALSE;
  }
  if (!visible) {
    if (cache->depthmap->scene.getValue() == cache->depthmapscene) {
      cache->depthmap->scene = new SoInfo;
    }
    return;
  }
  if (cache->depthmap->scene.getValue() != cache->depthmapscene) {
    cache->depthmap->scene = cache->depthmapscene;
  }
  cam->viewBoundingBox(isect, 1.0f, 1.0f);

  SbBox3f box = cache->toCameraSpace(worldbox);

  // Bounding box was calculated in camera space, so we need to "flip"
  // the box (because camera is pointing in the (0,0,-1) direction
  // from origo. Add a little slack (multiply by 1.01)
  cam->nearDistance = -box.getMax()[2]*1.01f;
  cam->farDistance = -box.getMin()[2]*1.01f;

  SbPlane plane(dir, cam->position.getValue());
  // move to eye space
  plane.transform(SoViewingMatrixElement::get(state));
  SbVec3f N = plane.getNormal();
  float D = plane.getDistanceFromOrigin();

#if 0
  fprintf(stderr,"isect: %g %g %g, %g %g %g\n",
          isect.getMin()[0],
          isect.getMin()[1],
          isect.getMin()[2],
          isect.getMax()[0],
          isect.getMax()[1],
          isect.getMax()[2]);
  fprintf(stderr,"plane: %g %g %g, %g\n", N[0], N[1], N[2], D);
  fprintf(stderr,"nearfar: %g %g\n", cam->nearDistance.getValue(), cam->farDistance.getValue());
  fprintf(stderr,"aspect: %g\n", SoViewportRegionElement::get(state).getViewportAspectRatio());
#endif

  cache->fragment_lightplane->value.setValue(N[0], N[1], N[2], D);

  //SoShadowGroup::VisibilityFlag visflag = (SoShadowGroup::VisibilityFlag) PUBLIC(this)->visibilityFlag.getValue();

  float visnear = cam->nearDistance.getValue();
  float visfar = cam->farDistance.getValue();

  cache->nearval = visnear;
  cache->farval = visfar;

  if (cache->nearval != cam->nearDistance.getValue()) {
    cam->nearDistance = cache->nearval;
  }
  if (cache->farval != cam->farDistance.getValue()) {
    cam->farDistance = cache->farval;
  }

  float realfarval = cache->farval * 1.1f;
  cache->fragment_farval->value = realfarval;
  cache->vsm_farval->value = realfarval;

  cache->fragment_nearval->value = cache->nearval;
  cache->vsm_nearval->value = cache->nearval;

  vv = cam->getViewVolume(1.0f);
  SbMatrix affine, proj;
  vv.getMatrices(affine, proj);
  cache->matrix = affine * proj;
}

void
SoShadowGroupP::renderDepthMap(SoShadowLightCache * cache,
                               SoGLRenderAction * action)
{
  cache->depthmap->GLRender(action);
  if (cache->gaussmap) cache->gaussmap->GLRender(action);
}

namespace {
  void initLightMaterial(SoShaderGenerator & gen, int i) {
    SbString str;
    str.sprintf("ambient = gl_LightSource[%d].ambient;\n"
                "diffuse = gl_LightSource[%d].diffuse;\n"
                "specular = gl_LightSource[%d].specular;\n", i,i,i);
    gen.addMainStatement(str);
  }

  void addDirectionalLight(SoShaderGenerator & gen, int i) {
    initLightMaterial(gen, i);
    SbString str;
    str.sprintf("DirectionalLight(normalize(vec3(gl_LightSource[%d].position)),"
                "vec3(gl_LightSource[%d].halfVector), normal, diffuse, specular);", i,i);
    gen.addMainStatement(str);
  }
  void addSpotLight(SoShaderGenerator & gen, int i, SbBool needdist = FALSE) {
    initLightMaterial(gen, i);
    const char * dist = needdist ? "dist = " : "";
    SbString str;
    str.sprintf("%s SpotLight("
                "vec3(gl_LightSource[%d].position),"
                "vec3(gl_LightSource[%d].constantAttenuation,"
                "     gl_LightSource[%d].linearAttenuation,"
                "     gl_LightSource[%d].quadraticAttenuation),"
                "normalize(gl_LightSource[%d].spotDirection),"
                "gl_LightSource[%d].spotExponent,"
                "gl_LightSource[%d].spotCosCutoff,"
                "eye, ecPosition3, normal, ambient, diffuse, specular);",
                dist, i,i,i,i,i,i,i);
    gen.addMainStatement(str);
  }
  void addDirSpotLight(SoShaderGenerator & gen, int i, SbBool needdist = FALSE) {
    initLightMaterial(gen, i);
    const char * dist = needdist ? "dist = " : "";
    SbString str;
    str.sprintf("%s DirSpotLight("
                " -normalize(vec3(gl_LightSource[%d].spotDirection)),"
                " vec3(gl_LightSource[%d].position),"
                " eye, ecPosition3, normal, diffuse, specular);", dist, i, i);
    gen.addMainStatement(str);
  }

  void addPointLight(SoShaderGenerator & gen, int i) {
    initLightMaterial(gen, i);
    SbString str;
    str.sprintf("PointLight("
                "vec3(gl_LightSource[%d].position),"
                "vec3(gl_LightSource[%d].constantAttenuation,"
                "     gl_LightSource[%d].linearAttenuation,"
                "     gl_LightSource[%d].quadraticAttenuation),"
                " eye, ecPosition3, normal, ambient, diffuse, specular);", i,i,i,i);

    gen.addMainStatement(str);

  }

};

void
SoShadowGroupP::setVertexShader(SoState * state)
{
  int i;
  SoShaderGenerator & gen = this->vertexgenerator;
  gen.reset(FALSE);
  gen.setVersion("#version 120");

  SbBool storedinvalid = SoCacheElement::setInvalid(FALSE);

  state->push();

  SbBool perpixelspot = FALSE;
  SbBool perpixelother = FALSE;

  this->getQuality(state, perpixelspot, perpixelother);

  if (this->vertexshadercache) {
    this->vertexshadercache->unref();
  }
  this->vertexshadercache = new SoShaderProgramCache(state);
  this->vertexshadercache->ref();

  const cc_glglue * glue = cc_glglue_instance(SoGLCacheContextElement::get(state));

  // set active cache to record cache dependencies
  SoCacheElement::set(state, this->vertexshadercache);
  const SoNodeList & lights = SoLightElement::getLights(state);

  int numshadowlights = this->shadowlights.getLength();

  for (i = 0; i < numshadowlights; i++) {
    SbString str;
    str.sprintf("varying vec4 shadowCoord%d;", i);
    gen.addDeclaration(str, FALSE);

    if (!perpixelspot) {
      str.sprintf("varying vec3 spotVertexColor%d;", i);
      gen.addDeclaration(str, FALSE);
    }
  }

  if (numshadowlights) {
    gen.addDeclaration("uniform mat4 cameraTransform;", FALSE);
  }
  gen.addDeclaration("varying vec3 ecPosition3;", FALSE);
  gen.addDeclaration("varying vec3 fragmentNormal;", FALSE);
  gen.addDeclaration("varying vec3 perVertexColor;", FALSE);

  SbBool dirlight = FALSE;
  SbBool pointlight = FALSE;
  SbBool spotlight = FALSE;
  SbString str;

  gen.addMainStatement("vec4 ecPosition = gl_ModelViewMatrix * gl_Vertex;\n"
                       "ecPosition3 = ecPosition.xyz / ecPosition.w;");

  gen.addMainStatement("vec3 normal = normalize(gl_NormalMatrix * gl_Normal);\n"
                       "vec3 eye = -normalize(ecPosition3);\n"
                       "vec4 ambient;\n"
                       "vec4 diffuse;\n"
                       "vec4 specular;\n"
                       "vec4 accambient = vec4(0.0);\n"
                       "vec4 accdiffuse = vec4(0.0);\n"
                       "vec4 accspecular = vec4(0.0);\n"
                       "vec4 color;\n");

  gen.addMainStatement("fragmentNormal = normal;");

  if (!perpixelother) {
    for (i = 0; i < lights.getLength(); i++) {
      SoLight * l = (SoLight*) lights[i];
      if (l->isOfType(SoDirectionalLight::getClassTypeId())) {
        addDirectionalLight(gen, i);
        dirlight = TRUE;
      }
      else if (l->isOfType(SoSpotLight::getClassTypeId())) {
        addSpotLight(gen, i);
        spotlight = TRUE;
      }
      else if (l->isOfType(SoPointLight::getClassTypeId())) {
        addPointLight(gen, i);
        gen.addMainStatement(str);
        pointlight = TRUE;
      }
      else {
        SoDebugError::postWarning("SoShadowGroupP::setVertexShader",
                                  "Unknown light type: %s",
                                  l->getTypeId().getName().getString());
      }
      gen.addMainStatement("accambient += ambient; accdiffuse += diffuse; accspecular += specular;\n");
    }

    if (dirlight) gen.addNamedFunction(SbName("lights/DirectionalLight"), FALSE);
    if (pointlight) gen.addNamedFunction(SbName("lights/PointLight"), FALSE);

    gen.addMainStatement("color = gl_FrontLightModelProduct.sceneColor + "
                         "  accambient * gl_FrontMaterial.ambient + "
                         "  accdiffuse * gl_Color +"
                         "  accspecular * gl_FrontMaterial.specular;\n"
                         );
  }
  else {
    gen.addMainStatement("color = gl_FrontLightModelProduct.sceneColor;\n");
  }

  if (numshadowlights) {
    gen.addMainStatement("vec4 pos = cameraTransform * ecPosition;\n"); // in world space
  }
  for (i = 0; i < numshadowlights; i++) {
    SoShadowLightCache * cache = this->shadowlights[i];
    str.sprintf("shadowCoord%d = gl_TextureMatrix[%d] * pos;\n", i, cache->texunit); // in light space
    gen.addMainStatement(str);

    if (!perpixelspot) {
      spotlight = TRUE;
      addSpotLight(gen, cache->lightid);
      str.sprintf("spotVertexColor%d = \n"
                  "  ambient.rgb * gl_FrontMaterial.ambient.rgb + "
                  "  diffuse.rgb * gl_Color.rgb + "
                  "  specular.rgb * gl_FrontMaterial.specular.rgb;\n", i);
      gen.addMainStatement(str);
    }
  }

  if (spotlight) gen.addNamedFunction(SbName("lights/SpotLight"), FALSE);
  int32_t fogType = this->getFog(state);

  switch (fogType) {
  default:
    assert(0 && "unknown fog type");
  case SoEnvironmentElement::NONE:
    // do nothing
    break;
  case SoEnvironmentElement::HAZE:
  case SoEnvironmentElement::FOG:
  case SoEnvironmentElement::SMOKE:
    gen.addMainStatement("gl_FogFragCoord = abs(ecPosition3.z);\n");
    break;
  }
  gen.addMainStatement("perVertexColor = vec3(clamp(color.r, 0.0, 1.0), clamp(color.g, 0.0, 1.0), clamp(color.b, 0.0, 1.0));"
                       "gl_TexCoord[0] = gl_TextureMatrix[0] * gl_MultiTexCoord0;\n"
                       "gl_TexCoord[1] = gl_TextureMatrix[1] * gl_MultiTexCoord1;\n"
                       "gl_Position = ftransform();\n"
                       "gl_FrontColor = gl_Color;\n");

  if (this->hasclipplanes) {
    if (SoGLDriverDatabase::isSupported(glue, SO_GL_GLSL_CLIP_VERTEX_HW)) {
      gen.addMainStatement("gl_ClipVertex = gl_ModelViewMatrix * gl_Vertex;\n");
    }
  }

  // never update unless the program has actually changed. Creating a
  // new GLSL program is very slow on current drivers.
  if (this->vertexshader->sourceProgram.getValue() != gen.getShaderProgram()) {
    this->vertexshader->sourceProgram = gen.getShaderProgram();
    this->vertexshader->sourceType = SoShaderObject::GLSL_PROGRAM;
    this->vertexshadercache->set(gen.getShaderProgram());

    if (numshadowlights) {
      this->vertexshader->parameter.set1Value(0, this->cameratransform);
    }
    else {
      this->vertexshader->parameter.setNum(0);
    }
#if 0 // for debugging
    fprintf(stderr,"new vertex program: %s\n",
            gen.getShaderProgram().getString());
#endif
  }


  this->vertexshadercache->set(gen.getShaderProgram());

  state->pop();
  SoCacheElement::setInvalid(storedinvalid);

}

void
SoShadowGroupP::setFragmentShader(SoState * state)
{
  int i;

  SoShaderGenerator & gen = this->fragmentgenerator;
  gen.reset(FALSE);
  gen.setVersion("#version 120");

  SbBool perpixelspot = FALSE;
  SbBool perpixelother = FALSE;
  this->getQuality(state, perpixelspot, perpixelother);

  const cc_glglue * glue = cc_glglue_instance(SoGLCacheContextElement::get(state));
  SbBool storedinvalid = SoCacheElement::setInvalid(FALSE);
  state->push();

  if (this->fragmentshadercache) {
    this->fragmentshadercache->unref();
  }
  this->fragmentshadercache = new SoShaderProgramCache(state);
  this->fragmentshadercache->ref();

  // set active cache to record cache dependencies
  SoCacheElement::set(state, this->fragmentshadercache);

  int numshadowlights = this->shadowlights.getLength();
  SbBool dirspot = FALSE;

  // ATi doesn't seem to support gl_FrontFace in hardware. We've only
  // verified that nVidia supports it so far.
  SbBool twosidetest = glue->vendor_is_nvidia && ((perpixelspot && numshadowlights) || perpixelother);


  if (numshadowlights) {
    SbString eps;
    eps.sprintf("const float EPSILON = %f;",
                PUBLIC(this)->epsilon.getValue());
    gen.addDeclaration(eps, FALSE);
    eps.sprintf("const float THRESHOLD = %f;",
                PUBLIC(this)->threshold.getValue());
    gen.addDeclaration(eps, FALSE);
  }
  for (i = 0; i < numshadowlights; i++) {
    SbString str;
    str.sprintf("uniform sampler2D shadowMap%d;", i);
    gen.addDeclaration(str, FALSE);

    str.sprintf("uniform float farval%d;", i);
    gen.addDeclaration(str, FALSE);

    str.sprintf("uniform float nearval%d;", i);
    gen.addDeclaration(str, FALSE);

    str.sprintf("varying vec4 shadowCoord%d;", i);
    gen.addDeclaration(str, FALSE);

    if (!perpixelspot) {
      str.sprintf("varying vec3 spotVertexColor%d;", i);
      gen.addDeclaration(str, FALSE);
    }
    if (this->shadowlights[i]->light->isOfType(SoDirectionalLight::getClassTypeId())) {
      str.sprintf("uniform vec4 lightplane%d;", i);
      gen.addDeclaration(str, FALSE);
    }
  }

  if (numshadowlights) {
#ifdef DISTRIBUTE_FACTOR
    SbString str;
    str.sprintf("const float DISTRIBUTE_FACTOR = %.1f;\n", DISTRIBUTE_FACTOR);
    gen.addDeclaration(str, FALSE);
#endif
  }
  gen.addDeclaration("varying vec3 ecPosition3;", FALSE);
  gen.addDeclaration("varying vec3 fragmentNormal;", FALSE);
  gen.addDeclaration("varying vec3 perVertexColor;", FALSE);

  const SoNodeList & lights = SoLightElement::getLights(state);

  if (numshadowlights) {
    gen.addNamedFunction("vsm/VsmLookup", FALSE);
  }
  gen.addMainStatement("vec3 normal = normalize(fragmentNormal);\n");
  if (twosidetest) {
    gen.addMainStatement("if (coin_two_sided_lighting != 0 && !gl_FrontFacing) normal = -normal;\n");
  }
  gen.addMainStatement("vec3 eye = -normalize(ecPosition3);\n");
  gen.addMainStatement("vec4 ambient = vec4(0.0);\n"
                       "vec4 diffuse = vec4(0.0);\n"
                       "vec4 specular = vec4(0.0);"
                       "vec4 mydiffuse = gl_Color;\n"
                       "vec4 texcolor = (coin_texunit0_model != 0) ? texture2D(textureMap0, gl_TexCoord[0].xy) : vec4(1.0);\n");

  if (this->numtexunitsinscene > 1) {
    gen.addMainStatement("if (coin_texunit1_model != 0) texcolor *= texture2D(textureMap1, gl_TexCoord[1].xy);\n");
  }
  gen.addMainStatement("vec3 color = perVertexColor;\n"
                       "vec3 scolor = vec3(0.0);\n"
                       "float dist;\n"
                       "float shadeFactor;\n"
                       "vec3 coord;\n"
                       "vec4 map;\n"
                       "mydiffuse.a *= texcolor.a;\n");

  if (perpixelspot) {
    SbBool spotlight = FALSE;
    SbBool dirlight = FALSE;
    for (i = 0; i < numshadowlights; i++) {
      SoShadowLightCache * cache = this->shadowlights[i];
      SbBool dirshadow = FALSE;
      SbString str;
      SbBool normalspot = FALSE;
      SbString insidetest = "&& coord.x >= 0.0 && coord.x <= 1.0 && coord.y >= 0.0 && coord.y <= 1.0)";

      SoLight * light = this->shadowlights[i]->light;
      if (light->isOfType(SoSpotLight::getClassTypeId())) {
        SoSpotLight * sl = static_cast<SoSpotLight*> (light);
        if (sl->dropOffRate.getValue() >= 0.0f) {
          insidetest = ")";
          spotlight = TRUE;
          normalspot = TRUE;
        }
        else {
          insidetest = ")";
          dirspot = TRUE;
        }
      }
      else {
        dirshadow = TRUE;
        dirlight = TRUE;
      }
      if (dirshadow) {
        str.sprintf("dist = dot(ecPosition3.xyz, lightplane%d.xyz) - lightplane%d.w;\n", i,i);
        gen.addMainStatement(str);
        addDirectionalLight(gen, cache->lightid);
      }
      else {
        if (normalspot) {
          addSpotLight(gen, cache->lightid, TRUE);
        }
        else {
          addDirSpotLight(gen, cache->lightid, TRUE);
        }
      }
      str.sprintf("coord = 0.5 * (shadowCoord%d.xyz / shadowCoord%d.w + vec3(1.0));\n", i , i);
      gen.addMainStatement(str);
      str.sprintf("map = texture2D(shadowMap%d, coord.xy);\n", i);
      gen.addMainStatement(str);
#ifdef USE_NEGATIVE
      gen.addMainStatement("map = (map + vec4(1.0)) * 0.5;\n");
#endif // USE_NEGATIVE
#ifdef DISTRIBUTE_FACTOR
      gen.addMainStatement("map.xy += map.zw / DISTRIBUTE_FACTOR;\n");
#endif
      str.sprintf("shadeFactor = ((map.x < 0.9999) && (shadowCoord%d.z > -1.0 %s) "
                  "? VsmLookup(map, (dist - nearval%d) / (farval%d - nearval%d), EPSILON, THRESHOLD) : 1.0;\n",
                  i, insidetest.getString(),i,i,i);
      gen.addMainStatement(str);

      if (dirshadow) {
        SoShadowDirectionalLight * sl = static_cast<SoShadowDirectionalLight*> (light);
        if (sl->maxShadowDistance.getValue() > 0.0f) {
          gen.addMainStatement("shadeFactor = 1.0 - shadeFactor;\n");

          // linear falloff
          // str.sprintf("shadeFactor *= max(0.0, min(1.0, 1.0 + ecPosition3.z/maxshadowdistance%d));\n", i);

          // See SoGLEnvironemntElement.cpp (updategl()) to see how the magic exp() constants here are calculated

          // exp(f) falloff
          // str.sprintf("shadeFactor *= min(1.0, exp(5.545*ecPosition3.z/maxshadowdistance%d));\n", i);
          // just use exp(f^2) as a falloff formula for now, consider making this configurable
          str.sprintf("shadeFactor *= min(1.0, exp(2.35*ecPosition3.z*abs(ecPosition3.z)/(maxshadowdistance%d*maxshadowdistance%d)));\n", i,i);
          gen.addMainStatement(str);
          gen.addMainStatement("shadeFactor = 1.0 - shadeFactor;\n");
        }
      }
      gen.addMainStatement("color += shadeFactor * diffuse.rgb * mydiffuse.rgb;");
      gen.addMainStatement("scolor += shadeFactor * gl_FrontMaterial.specular.rgb * specular.rgb;\n");
      gen.addMainStatement("color += ambient.rgb * gl_FrontMaterial.ambient.rgb;\n");
    }

    if (perpixelother) {
      SbBool pointlight = FALSE;
      for (i = 0; i < lights.getLength(); i++) {
        SoLight * l = (SoLight*) lights[i];
        if (l->isOfType(SoDirectionalLight::getClassTypeId())) {
          addDirectionalLight(gen, i);
          dirlight = TRUE;
        }
        else if (l->isOfType(SoSpotLight::getClassTypeId())) {
          addSpotLight(gen, i);
          spotlight = TRUE;
        }
        else if (l->isOfType(SoPointLight::getClassTypeId())) {
          addPointLight(gen, i);
          pointlight = TRUE;
        }
        else {
          SoDebugError::postWarning("SoShadowGroupP::setFragmentShader",
                                    "Unknown light type: %s",
                                    l->getTypeId().getName().getString());
        }
        gen.addMainStatement("color += ambient.rgb * gl_FrontMaterial.ambient.rgb + "
                             "diffuse.rgb * mydiffuse.rgb;\n");
        gen.addMainStatement("scolor += specular.rgb * gl_FrontMaterial.specular.rgb;\n");
      }

      if (dirlight) gen.addNamedFunction(SbName("lights/DirectionalLight"), FALSE);
      if (pointlight) gen.addNamedFunction(SbName("lights/PointLight"), FALSE);
    }
    if (spotlight) gen.addNamedFunction(SbName("lights/SpotLight"), FALSE);
  }

  else {
    for (i = 0; i < numshadowlights; i++) {
      SbString insidetest = "&& coord.x >= 0.0 && coord.x <= 1.0 && coord.y >= 0.0 && coord.y <= 1.0)";

      SoLight * light = this->shadowlights[i]->light;
      if (light->isOfType(SoSpotLight::getClassTypeId())) {
        SoSpotLight * sl = static_cast<SoSpotLight*> (light);
        if (sl->dropOffRate.getValue() >= 0.0f) {
          insidetest = ")";
        }
      }
      SbString str;
      str.sprintf("dist = length(vec3(gl_LightSource[%d].position) - ecPosition3);\n"
                  "coord = 0.5 * (shadowCoord%d.xyz / shadowCoord%d.w + vec3(1.0));\n"
                  "map = texture2D(shadowMap%d, coord.xy);\n"
#ifdef USE_NEGATIVE
                  "map = (map + vec4(1.0)) * 0.5;\n"
#endif // USE_NEGATIVE
#ifdef DISTRIBUTE_FACTOR
                  "map.xy += map.zw / DISTRIBUTE_FACTOR;\n"
#endif
                  "shadeFactor = (shadowCoord%d.z > -1.0%s ? VsmLookup(map, (dist - nearval%d)/(farval%d-nearval%d), EPSILON, THRESHOLD) : 1.0;\n"
                  "color += shadeFactor * spotVertexColor%d;\n",
                  lights.getLength()+i, i , i, i, i,insidetest.getString(), i,i,i,i);
      gen.addMainStatement(str);
    }
  }

  gen.addMainStatement("color = vec3(clamp(color.r, 0.0, 1.0), clamp(color.g, 0.0, 1.0), clamp(color.b, 0.0, 1.0));");
  gen.addMainStatement("if (coin_light_model != 0) { color *= texcolor.rgb; color += scolor; }\n"
                       "else color = mydiffuse.rgb * texcolor.rgb;\n");

  int32_t fogType = this->getFog(state);

  switch (fogType) {
  default:
    assert(0 && "unknown fog type");
  case SoEnvironmentElement::NONE:
    // do nothing
    break;
  case SoEnvironmentElement::HAZE:
    gen.addMainStatement("float fog = (gl_Fog.end - gl_FogFragCoord) * gl_Fog.scale;\n");
    break;
  case SoEnvironmentElement::FOG:
    gen.addMainStatement("float fog = exp(-gl_Fog.density * gl_FogFragCoord);\n");
    break;
  case SoEnvironmentElement::SMOKE:
    gen.addMainStatement("float fogfrag =  gl_FogFragCoord;");
    gen.addMainStatement("float fogdens =  gl_Fog.density;");
    gen.addMainStatement("float fog = exp(-fogdens * fogdens * fogfrag * fogfrag);\n");
    break;
  }
  if (fogType != SoEnvironmentElement::NONE) {
    gen.addMainStatement("color = mix(gl_Fog.color.rgb, color, clamp(fog, 0.0, 1.0));\n");
  }

  gen.addMainStatement("gl_FragColor = vec4(color, mydiffuse.a);");
  gen.addDeclaration("uniform sampler2D textureMap0;\n", FALSE);
  gen.addDeclaration("uniform int coin_texunit0_model;\n", FALSE);
  if (this->numtexunitsinscene > 1) {
    gen.addDeclaration("uniform int coin_texunit1_model;\n", FALSE);
    gen.addDeclaration("uniform sampler2D textureMap1;\n", FALSE);
  }
  gen.addDeclaration("uniform int coin_light_model;\n", FALSE);
  if (twosidetest) {
    gen.addDeclaration("uniform int coin_two_sided_lighting;\n", FALSE);
  }

  if (dirspot) {
    gen.addNamedFunction("lights/DirSpotLight", FALSE);
  }

  this->fragmentshader->parameter.setNum(0);

  for (i = 0; i < numshadowlights; i++) {
    SoShadowLightCache * cache = this->shadowlights[i];

    SoShaderParameter1i * shadowmap = this->shadowlights[i]->shadowmapid;
    SbString str;
    str.sprintf("shadowMap%d", i);
    if (shadowmap->name.getValue() != str) {
      shadowmap->name = str;
    }
    shadowmap->value = cache->texunit;
    this->fragmentshader->parameter.set1Value(this->fragmentshader->parameter.getNum(), shadowmap);
  }

  for (i = 0; i < numshadowlights; i++) {
    SbString str;
    SoShaderParameter1f *farval = this->shadowlights[i]->fragment_farval;
    str.sprintf("farval%d", i);
    if (farval->name.getValue() != str) {
      farval->name = str;
    }
    this->fragmentshader->parameter.set1Value(this->fragmentshader->parameter.getNum(), farval);
  }

  for (i = 0; i < numshadowlights; i++) {
    SbString str;
    SoShaderParameter1f *nearval = this->shadowlights[i]->fragment_nearval;
    str.sprintf("nearval%d", i);
    if (nearval->name.getValue() != str) {
      nearval->name = str;
    }
    this->fragmentshader->parameter.set1Value(this->fragmentshader->parameter.getNum(), nearval);
  }
  SoShaderParameter1i * texmap =
    new SoShaderParameter1i();
  SbString str0;
  str0.sprintf("textureMap0");
  texmap->name = str0;
  texmap->value = 0;

  SoShaderParameter1i * texmap1 = NULL;

  if (!this->texunit0) {
    this->texunit0 = new SoShaderParameter1i;
    this->texunit0->ref();
    this->texunit0->name = "coin_texunit0_model";
    this->texunit0->value = 0;
  }

  if (this->numtexunitsinscene > 1) {
    if (!this->texunit1) {
      this->texunit1 = new SoShaderParameter1i;
      this->texunit1->ref();
      this->texunit1->name = "coin_texunit1_model";
      this->texunit1->value = 0;
    }
    texmap1 = new SoShaderParameter1i();
	SbString str;
	str.sprintf("textureMap1");
    texmap1->name = str;
    texmap1->value = 1;
  }

  if (!this->lightmodel) {
    this->lightmodel = new SoShaderParameter1i;
    this->lightmodel->ref();
    this->lightmodel->name = "coin_light_model";
    this->lightmodel->value = 1;
  }
  this->fragmentshader->parameter.set1Value(this->fragmentshader->parameter.getNum(), texmap);
  if (texmap1) this->fragmentshader->parameter.set1Value(this->fragmentshader->parameter.getNum(), texmap1);
  this->fragmentshader->parameter.set1Value(this->fragmentshader->parameter.getNum(), this->texunit0);
  if (this->numtexunitsinscene > 1) this->fragmentshader->parameter.set1Value(this->fragmentshader->parameter.getNum(), this->texunit1);
  this->fragmentshader->parameter.set1Value(this->fragmentshader->parameter.getNum(), this->lightmodel);

  if (twosidetest) {
    if (!this->twosided) {
      this->twosided = new SoShaderParameter1i;
      this->twosided->ref();
      this->twosided->name = "coin_two_sided_lighting";
      this->twosided->value = 0;
    }
    this->fragmentshader->parameter.set1Value(this->fragmentshader->parameter.getNum(), this->twosided);
  }

  for (i = 0; i < numshadowlights; i++) {
    SoShadowLightCache * cache = this->shadowlights[i];

    if (cache->light->isOfType(SoShadowDirectionalLight::getClassTypeId())) {
      SbString str;
      SoShadowDirectionalLight * sl = static_cast<SoShadowDirectionalLight*> (cache->light);
      if (sl->maxShadowDistance.getValue() > 0.0f) {
        SoShaderParameter1f * maxdist = cache->maxshadowdistance;
        maxdist->value.connectFrom(&sl->maxShadowDistance);
        str.sprintf("maxshadowdistance%d", i);
        if (maxdist->name.getValue() != str) {
          maxdist->name = str;
        }
        SbString uniform;
        uniform.sprintf("uniform float %s;\n", str.getString());
        gen.addDeclaration(uniform, FALSE);
        this->fragmentshader->parameter.set1Value(this->fragmentshader->parameter.getNum(), maxdist);
      }

      SoShaderParameter4f * lightplane = cache->fragment_lightplane;
      str.sprintf("lightplane%d", i);
      if (lightplane->name.getValue() != str) {
        lightplane->name = str;
      }
      this->fragmentshader->parameter.set1Value(this->fragmentshader->parameter.getNum(), lightplane);
    }
  }

  this->shadowlightsvalid = TRUE;
  // never update unless the program has actually changed. Creating a
  // new GLSL program is very slow on current drivers.
  if (this->fragmentshader->sourceProgram.getValue() != gen.getShaderProgram()) {
    // invalidate spotlights, and make sure the cameratransform variable is updated
    this->cameratransform->value.touch();
    this->fragmentshader->sourceProgram = gen.getShaderProgram();
    this->fragmentshader->sourceType = SoShaderObject::GLSL_PROGRAM;

#if 0 // for debugging
    fprintf(stderr,"new fragment program: %s\n",
            gen.getShaderProgram().getString());
#endif // debugging

  }

  this->fragmentshadercache->set(gen.getShaderProgram());
  state->pop();
  SoCacheElement::setInvalid(storedinvalid);
}

void
SoShadowLightCache::createVSMProgram(void)
{
  SoShaderProgram * program = new SoShaderProgram;
  program->ref();

  SoVertexShader * vshader = new SoVertexShader;
  SoFragmentShader * fshader = new SoFragmentShader;

  program->shaderObject.set1Value(0, vshader);
  program->shaderObject.set1Value(1, fshader);

  SoShaderGenerator & vgen = this->vsm_vertex_generator;
  SoShaderGenerator & fgen = this->vsm_fragment_generator;

  vgen.reset(FALSE);
  vgen.setVersion("#version 120");

  SbBool dirlight = this->light->isOfType(SoDirectionalLight::getClassTypeId());

  vgen.addDeclaration("varying vec3 light_vec;", FALSE);
  vgen.addMainStatement("light_vec = (gl_ModelViewMatrix * gl_Vertex).xyz;\n"
                        "gl_Position = ftransform();");

  vshader->sourceProgram = vgen.getShaderProgram();
  vshader->sourceType = SoShaderObject::GLSL_PROGRAM;

  fgen.reset(FALSE);
  fgen.setVersion("#version 120");
#ifdef DISTRIBUTE_FACTOR
  SbString str;
  str.sprintf("const float DISTRIBUTE_FACTOR = %.1f;\n", DISTRIBUTE_FACTOR);
  fgen.addDeclaration(str, FALSE);
#endif
  fgen.addDeclaration("varying vec3 light_vec;", FALSE);
  fgen.addDeclaration("uniform float farval;", FALSE);
  fgen.addDeclaration("uniform float nearval;", FALSE);
  if (!dirlight)  {
    fgen.addMainStatement("float l = (length(light_vec) - nearval) / (farval-nearval);\n");
  }
  else {
    fgen.addMainStatement("float l = (-light_vec.z - nearval) / (farval-nearval);\n");
  }
  fgen.addMainStatement(
#ifdef DISTRIBUTE_FACTOR
                        "vec2 m = vec2(l, l*l);\n"
                        "vec2 f = fract(m * DISTRIBUTE_FACTOR);\n"

#ifdef USE_NEGATIVE
                        "gl_FragColor.rg = (m - (f / DISTRIBUTE_FACTOR)) * 2.0 - vec2(1.0, 1.0);\n"
                        "gl_FragColor.ba = f * 2.0 - vec2(1.0, 1.0);\n"
#else // USE_NEGATIVE
                        "gl_FragColor.rg = m - (f / DISTRIBUTE_FACTOR);\n"
                        "gl_FragColor.ba = f;\n"
#endif // ! USE_NEGATIVE
#else // DISTRIBUTE_FACTOR
#ifdef USE_NEGATIVE
                        "gl_FragColor = vec4(l*2.0 - 1.0, l*l*2.0 - 1.0, 0.0, 0.0);"
#else // USE_NEGATIVE
                        "gl_FragColor = vec4(l, l*l, 0.0, 0.0);"
#endif // !USE_NEGATIVE
#endif // !DISTRIBUTE_FACTOR
                        );
  fshader->sourceProgram = fgen.getShaderProgram();
  fshader->sourceType = SoShaderObject::GLSL_PROGRAM;

  this->vsm_program = program;
  this->vsm_program->ref();

  this->vsm_farval = new SoShaderParameter1f;
  this->vsm_farval->ref();
  this->vsm_farval->name = "farval";

  this->vsm_nearval = new SoShaderParameter1f;
  this->vsm_nearval->ref();
  this->vsm_nearval->name = "nearval";

  fshader->parameter = this->vsm_farval;
  fshader->parameter.set1Value(1, this->vsm_nearval);
}

void
SoShadowGroupP::shader_enable_cb(void * closure,
                                 SoState * state,
                                 const SbBool enable)
{
  SoShadowGroupP * thisp = (SoShadowGroupP*) closure;

  const cc_glglue * glue = cc_glglue_instance(SoGLCacheContextElement::get(state));

  for (int i = 0; i < thisp->shadowlights.getLength(); i++) {
    SoShadowLightCache * cache = thisp->shadowlights[i];
    int unit = cache->texunit;
    if (unit == 0) {
      if (enable) glEnable(GL_TEXTURE_2D);
      else glDisable(GL_TEXTURE_2D);
    }
    else {
      cc_glglue_glActiveTexture(glue, (GLenum) (int(GL_TEXTURE0) + unit));
      if (enable) glEnable(GL_TEXTURE_2D);
      else glDisable(GL_TEXTURE_2D);
      cc_glglue_glActiveTexture(glue, GL_TEXTURE0);

      GLenum glerror = sogl_glerror_debugging() ? glGetError() : GL_NO_ERROR;
      while (glerror) {
          SoDebugError::postWarning("SoShadowGroupP::shader_enable_cb",
              "glError() = %d\n", glerror);
          glerror = glGetError();
      }
    }
  }
}

bool
SoShadowGroupP::supported(const cc_glglue * glue, SbString& reason)
{
  const bool supported =
    cc_glglue_glversion_matches_at_least(glue, 2, 0, 0) &&
    SoGLDriverDatabase::isSupported(glue, SO_GL_FRAMEBUFFER_OBJECT) &&
    SoGLDriverDatabase::isSupported(glue, "GL_ARB_texture_float");

  if (supported) { return true; }

  reason = "Unable to render shadows.";
  if (!SoGLDriverDatabase::isSupported(glue, SO_GL_FRAMEBUFFER_OBJECT)) reason += " Frame buffer objects not supported.";
  if (!cc_glglue_glversion_matches_at_least(glue, 2, 0, 0)) reason += " OpenGL version < 2.0.";
  if (!SoGLDriverDatabase::isSupported(glue, "GL_ARB_texture_float")) reason += " Floating point textures not supported.";

  return false;
}

void
SoShadowGroupP::GLRender(SoGLRenderAction * action, const SbBool inpath)
{
  SoState * state = action->getState();
  const cc_glglue * glue = cc_glglue_instance(SoGLCacheContextElement::get(state));

  // FIXME: should store results in a "context -> supported" map.  -mortene.
  SbString reason;
  const bool supported = SoShadowGroupP::supported(glue, reason);
  if (!supported && PUBLIC(this)->isActive.getValue()) {
    static bool first = true;
    if (first) {
      first = false;
      SoDebugError::postWarning("SoShadowGroupP::GLRender", "%s", reason.getString());
    }
  }

  if (!supported || !PUBLIC(this)->isActive.getValue()) {
    if (inpath) PUBLIC(this)->SoSeparator::GLRenderInPath(action);
    else PUBLIC(this)->SoSeparator::GLRenderBelowPath(action);
    return;
  }

  state->push();

  if (!this->vertexshadercache || !this->vertexshadercache->isValid(state)) {
    // a bit hackish, but saves creating yet another cache
    this->shadowlightsvalid = FALSE;
  }

  SbMatrix camtransform = SoViewingMatrixElement::get(state).inverse();
  if (camtransform != this->cameratransform->value.getValue()) {
    this->cameratransform->value = camtransform;
  }

  SoShadowStyleElement::set(state, PUBLIC(this), SoShadowStyleElement::CASTS_SHADOW_AND_SHADOWED);
  SoShapeStyleElement::setShadowMapRendering(state, TRUE);
  this->updateShadowLights(action);
  SoShapeStyleElement::setShadowMapRendering(state, FALSE);

  if (!this->vertexshadercache || !this->vertexshadercache->isValid(state)) {
    this->setVertexShader(state);
  }

  if (!this->fragmentshadercache || !this->fragmentshadercache->isValid(state)) {
    this->setFragmentShader(state);
  }
  this->shaderprogram->GLRender(action);

  SoShapeStyleElement::setShadowsRendering(state, TRUE);
  if (inpath) PUBLIC(this)->SoSeparator::GLRenderInPath(action);
  else PUBLIC(this)->SoSeparator::GLRenderBelowPath(action);
  SoShapeStyleElement::setShadowsRendering(state, FALSE);
  state->pop();
}

SoShaderProgram *
SoShadowLightCache::createGaussFilter(const int texsize, const int size, const float gaussstandarddeviation)
{
  SoVertexShader * vshader = new SoVertexShader;
  SoFragmentShader * fshader = new SoFragmentShader;
  SoShaderProgram * program = new SoShaderProgram;

  SoShaderParameterArray2f * offset = new SoShaderParameterArray2f;
  offset->name = "offset";
  SoShaderParameterArray1f * kernel = new SoShaderParameterArray1f;
  kernel->name = "kernelvalue";
  SoShaderParameter1i * baseimage = new SoShaderParameter1i;
  baseimage->name = "baseimage";
  baseimage->value = 0;

  int kernelsize = size*size;

  offset->value.setNum(kernelsize);
  kernel->value.setNum(kernelsize);

  SoShaderGenerator fgen;
  SbString str;

  str.sprintf("const int KernelSize = %d;", kernelsize);
  fgen.addDeclaration(str, FALSE);
  fgen.addDeclaration("uniform vec2 offset[KernelSize];", FALSE);
  fgen.addDeclaration("uniform float kernelvalue[KernelSize];", FALSE);
  fgen.addDeclaration("uniform sampler2D baseimage;", FALSE);

  fgen.addMainStatement(
                        "int i;\n"
                        "vec4 sum = vec4(0.0);\n"
                        "for (i = 0; i < KernelSize; i++) {\n"
                        "  vec4 tmp = texture2D(baseimage, gl_TexCoord[0].st + offset[i]);\n"
                        "  sum += tmp * kernelvalue[i];\n"
                        "}\n"
                        "gl_FragColor = sum;\n"
                        );

  const double sigma = (double) gaussstandarddeviation;
  const int center = size / 2;
  const float dt = 1.0f / float(texsize);

  SbVec2f * offsetptr = offset->value.startEditing();
  float * kernelptr = kernel->value.startEditing();

  int c = 0;
  for (int y = 0; y < size; y++) {
    int dy = SbAbs(y - center);
    for (int x = 0; x < size; x++) {
      int dx = SbAbs(x - center);

      kernelptr[c] = (float) ((1.0 /  (2.0 * M_PI * sigma * sigma)) * exp(- double(dx*dx + dy*dy) / (2.0 * sigma * sigma)));
      offsetptr[c] = SbVec2f(float(x-center) * dt, float(y-center)*dt);
      c++;

    }
  }
  offset->value.finishEditing();
  kernel->value.finishEditing();

  program->shaderObject = vshader;
  program->shaderObject.set1Value(1, fshader);

  fshader->sourceProgram = fgen.getShaderProgram();
  fshader->sourceType = SoShaderObject::GLSL_PROGRAM;

  fshader->parameter.set1Value(0, offset);
  fshader->parameter.set1Value(1, kernel);
  fshader->parameter.set1Value(2, baseimage);

  SoShaderGenerator vgen;
  vgen.addMainStatement("gl_TexCoord[0] = gl_Vertex;\n");
  vgen.addMainStatement("gl_Position = ftransform();");

  vshader->sourceProgram = vgen.getShaderProgram();
  vshader->sourceType = SoShaderObject::GLSL_PROGRAM;

  return program;
}

SoSeparator *
SoShadowLightCache::createGaussSG(SoShaderProgram * program, SoSceneTexture2 * tex)
{
  SoSeparator * sep = new SoSeparator;
  SoOrthographicCamera * camera = new SoOrthographicCamera;
  SoShapeHints * sh = new SoShapeHints;

  const float verts[][3] = {
    {0.0f, 0.0f, 0.0f},
    {1.0f, 0.0f, 0.0f},
    {1.0f, 1.0f, 0.0f},
    {0.0f, 1.0f, 0.0f}

  };

  sh->vertexOrdering = SoShapeHints::COUNTERCLOCKWISE;
  sh->faceType = SoShapeHints::CONVEX;
  sh->shapeType = SoShapeHints::SOLID;

  sep->addChild(sh);

  camera->position = SbVec3f(0.5f, 0.5f, 2.0f);
  camera->height = 1.0f;
  camera->aspectRatio = 1.0f;
  camera->viewportMapping = SoCamera::LEAVE_ALONE;

  sep->addChild(camera);
  SoTextureUnit * unit = new SoTextureUnit;
  unit->unit = 0;
  sep->addChild(unit);

  sep->addChild(tex);
  sep->addChild(program);

  SoCoordinate3 * coord = new SoCoordinate3;
  sep->addChild(coord);

  coord->point.setValues(0,4,verts);

  SoFaceSet * fs = new SoFaceSet;
  fs->numVertices = 4;
  sep->addChild(fs);

  return sep;
}

void
SoShadowLightCache::shadowmap_glcallback(void * COIN_UNUSED_ARG(closure), SoAction * action)
{
  if (action->isOfType(SoGLRenderAction::getClassTypeId())) {
    SoState * state = action->getState();
    SoLazyElement::setLightModel(state, SoLazyElement::BASE_COLOR);
    SoTextureQualityElement::set(state, 0.0f);
    SoMaterialBindingElement::set(state, NULL, SoMaterialBindingElement::OVERALL);
    SoNormalElement::set(state, NULL, 0, NULL, FALSE);


    SoOverrideElement::setNormalVectorOverride(state, NULL, TRUE);
    SoOverrideElement::setMaterialBindingOverride(state, NULL, TRUE);
    SoOverrideElement::setLightModelOverride(state, NULL, TRUE);
    SoTextureOverrideElement::setQualityOverride(state, TRUE);
  }
}

void
SoShadowLightCache::shadowmap_post_glcallback(void * COIN_UNUSED_ARG(closure), SoAction * action)
{
  if (action->isOfType(SoGLRenderAction::getClassTypeId())) {
    // for debugging the shadow map
    // reinterpret_cast<SoShadowLightCache*>(closure)->dumpBitmap("/home/pederb/Desktop/shadow.rgb");
    // nothing to do yet
  }
}

#undef PUBLIC
#undef DISTRIBUTE_FACTOR
#undef USE_NEGATIVE

#ifdef COIN_TEST_SUITE

BOOST_AUTO_TEST_CASE(initialized)
{
  SoShadowGroup * node = new SoShadowGroup;
  assert(node);
  node->ref();
  BOOST_CHECK_MESSAGE(node->getTypeId() != SoType::badType(),
                      "missing class initialization");
  node->unref();
}

#endif // COIN_TEST_SUITE
