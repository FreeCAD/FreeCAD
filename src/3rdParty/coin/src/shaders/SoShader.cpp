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
  \page coin_shaders_page Shaders in Coin

  Coin 2.5 added support for shaders. The main nodes used are SoShaderProgram,
  SoVertexShader, SoFragmentShader, and SoGeometryShader. A typical scene graph
  with shaders will look something like this:

  \code

  Separator {
    ShaderProgram {
      shaderObject [
        VertexShader {
          sourceProgram "myvertexshader.glsl"
          parameter [
            ShaderParameter1f { name "myvertexparam" value 1.0 }
          ]
        }
        FragmentShader {
          sourceProgram "myfragmentshader.glsl"
          parameter [
            ShaderParameter1f { name "myfragmentparam" value 2.0 }
          ]
        }
      ]
    }
    Cube { }
  }

  \endcode

  This will render the cube with the vertex and fragment shaders
  specified in myvertexshader.glsl and myfragmentshader.glsl. Coin
  also supports ARB shaders and Cg shaders (if the Cg library is
  installed). However, we recommend using GLSL since we will focus
  mostly on support this shader language.

  Coin defines some named parameters that can be added by the
  application programmer, and which will be automatically updated by
  Coin while traversing the scene graph.

  \li coin_texunit[n]_model - Set to 0 when texturing is disabled, and
  to SoTextureImageElement::Model if there's a current texture on the state
  for unit \a n.

  \li coin_light_model - Set to 1 for PHONG, 0 for BASE_COLOR lighting.

  \li coin_two_sided_lighting - Set to 1 for two-sided, 0 for normal

  Example scene graph that renders per fragment OpenGL Phong lighting
  for one light source. The shaders assume the first light source is a
  directional light. This is the case if you open the file in a standard
  examiner viewer.

  The iv-file:
  \code
  Separator {
    ShaderProgram {
      shaderObject [
        VertexShader {
          sourceProgram "perpixel_vertex.glsl"
        }
        FragmentShader {
          sourceProgram "perpixel_fragment.glsl"
        }
      ]
    }
    Complexity { value 1.0 }
    Material { diffuseColor 1 0 0 specularColor 1 1 1 shininess 0.9 }
    Sphere { }

    Translation { translation 3 0 0 }
    Material { diffuseColor 0 1 0 specularColor 1 1 1 shininess 0.9 }
    Cone { }

    Translation { translation 3 0 0 }
    Material { diffuseColor 0.8 0.4 0.1 specularColor 1 1 1 shininess 0.9 }
    Cylinder { }
  }
  \endcode

  The vertex shader (perpixel_vertex.glsl):
  \code
  varying vec3 ecPosition3;
  varying vec3 fragmentNormal;

  void main(void)
  {
    vec4 ecPosition = gl_ModelViewMatrix * gl_Vertex;
    ecPosition3 = ecPosition.xyz / ecPosition.w;
    fragmentNormal = normalize(gl_NormalMatrix * gl_Normal);

    gl_Position = ftransform();
    gl_FrontColor = gl_Color;
  }
  \endcode

  The fragment shader (perpixel_fragment.glsl):
  \code
  varying vec3 ecPosition3;
  varying vec3 fragmentNormal;

  void DirectionalLight(in int i,
                        in vec3 normal,
                        inout vec4 ambient,
                        inout vec4 diffuse,
                        inout vec4 specular)
  {
    float nDotVP; // normal . light direction
    float nDotHV; // normal . light half vector
    float pf;     // power factor

    nDotVP = max(0.0, dot(normal, normalize(vec3(gl_LightSource[i].position))));
    nDotHV = max(0.0, dot(normal, vec3(gl_LightSource[i].halfVector)));

    if (nDotVP == 0.0)
      pf = 0.0;
    else
      pf = pow(nDotHV, gl_FrontMaterial.shininess);

    ambient += gl_LightSource[i].ambient;
    diffuse += gl_LightSource[i].diffuse * nDotVP;
    specular += gl_LightSource[i].specular * pf;
  }

  void main(void)
  {
    vec3 eye = -normalize(ecPosition3);
    vec4 ambient = vec4(0.0);
    vec4 diffuse = vec4(0.0);
    vec4 specular = vec4(0.0);
    vec3 color;

    DirectionalLight(0, normalize(fragmentNormal), ambient, diffuse, specular);

    color =
      gl_FrontLightModelProduct.sceneColor.rgb +
      ambient.rgb * gl_FrontMaterial.ambient.rgb +
      diffuse.rgb * gl_Color.rgb +
      specular.rgb * gl_FrontMaterial.specular.rgb;

    gl_FragColor = vec4(color, gl_Color.a);
  }
  \endcode
*/

#include "shaders/SoShader.h"

#include <cassert>
#include <cstdio>
#include <cstdlib>

#include <Inventor/nodes/SoShaderProgram.h>
#include <Inventor/nodes/SoShaderObject.h>
#include <Inventor/nodes/SoFragmentShader.h>
#include <Inventor/nodes/SoVertexShader.h>
#include <Inventor/nodes/SoGeometryShader.h>
#include <Inventor/nodes/SoShaderParameter.h>
#include <Inventor/elements/SoGLShaderProgramElement.h>
#include <Inventor/C/tidbits.h>
#include <Inventor/errors/SoDebugError.h>

#include "glue/cg.h"
#include "misc/SbHash.h"
#include "tidbitsp.h"

// *************************************************************************

#include <data/shaders/lights/SpotLight.h>
#include <data/shaders/lights/PointLight.h>
#include <data/shaders/lights/DirectionalLight.h>
#include <data/shaders/lights/DirSpotLight.h>
#include <data/shaders/vsm/VsmLookup.h>

// *************************************************************************

static const char * SO_SHADER_DIR = NULL;
static SbHash<const char *, char *> * shader_dict = NULL;
static SbHash<const char *, char *> * shader_builtin_dict = NULL;

static void
soshader_cleanup(void)
{
  for(
      SbHash<const char *, char *>::const_iterator iter =
       shader_dict->const_begin();
      iter!=shader_dict->const_end();
      ++iter
      ) {
    delete[] iter->obj;
  }
  delete shader_dict;

  // no need to apply on objects since strings are compiled into the
  // library and should not be deleted
  delete shader_builtin_dict;
}

void
SoShader::init(void)
{
  // Trigger loading and init of Cg library glue.
  //
  // FIXME: this function should rather be used from the relevant
  // class(es), so it is loaded only on demand. 20050125 mortene.
  (void)cc_cgglue_available();

  // --- initialization of elements (must be done first) ---------------
  if (SoGLShaderProgramElement::getClassTypeId() == SoType::badType())
    SoGLShaderProgramElement::initClass();

  // --- initialization of shader nodes --------------------------------
  if (SoShaderProgram::getClassTypeId() == SoType::badType())
    SoShaderProgram::initClass();
  if (SoShaderObject::getClassTypeId() == SoType::badType())
    SoShaderObject::initClass();
  if (SoFragmentShader::getClassTypeId() == SoType::badType())
    SoFragmentShader::initClass();
  if (SoVertexShader::getClassTypeId() == SoType::badType())
    SoVertexShader::initClass();
  if (SoGeometryShader::getClassTypeId() == SoType::badType())
    SoGeometryShader::initClass();

  // --- initialization of parameter nodes -----------------------------
  if (SoShaderParameter::getClassTypeId() == SoType::badType())
    SoShaderParameter::initClass();
  if (SoUniformShaderParameter::getClassTypeId() == SoType::badType())
    SoUniformShaderParameter::initClass();

  // float vector parameter nodes
  if (SoShaderParameter1f::getClassTypeId() == SoType::badType())
    SoShaderParameter1f::initClass();
  if (SoShaderParameter2f::getClassTypeId() == SoType::badType())
    SoShaderParameter2f::initClass();
  if (SoShaderParameter3f::getClassTypeId() == SoType::badType())
    SoShaderParameter3f::initClass();
  if (SoShaderParameter4f::getClassTypeId() == SoType::badType())
    SoShaderParameter4f::initClass();

  // float vector array parameter nodes
  if (SoShaderParameterArray1f::getClassTypeId() == SoType::badType())
    SoShaderParameterArray1f::initClass();
  if (SoShaderParameterArray2f::getClassTypeId() == SoType::badType())
    SoShaderParameterArray2f::initClass();
  if (SoShaderParameterArray3f::getClassTypeId() == SoType::badType())
    SoShaderParameterArray3f::initClass();
  if (SoShaderParameterArray4f::getClassTypeId() == SoType::badType())
    SoShaderParameterArray4f::initClass();

  // matrix parameter nodes
  if (SoShaderStateMatrixParameter::getClassTypeId() == SoType::badType())
    SoShaderStateMatrixParameter::initClass();
  if (SoShaderParameterMatrix::getClassTypeId() == SoType::badType())
    SoShaderParameterMatrix::initClass();
  if (SoShaderParameterMatrixArray::getClassTypeId() == SoType::badType())
    SoShaderParameterMatrixArray::initClass();

  // int32 support
  if (SoShaderParameter1i::getClassTypeId() == SoType::badType())
    SoShaderParameter1i::initClass();

  // FIXME: Do we need int32 support (like in TGS)? 20040924 martin
#if 1
  if (SoShaderParameter2i::getClassTypeId() == SoType::badType())
    SoShaderParameter2i::initClass();
  if (SoShaderParameter3i::getClassTypeId() == SoType::badType())
    SoShaderParameter3i::initClass();
  if (SoShaderParameter4i::getClassTypeId() == SoType::badType())
    SoShaderParameter4i::initClass();
  if (SoShaderParameterArray1i::getClassTypeId() == SoType::badType())
    SoShaderParameterArray1i::initClass();
  if (SoShaderParameterArray2i::getClassTypeId() == SoType::badType())
    SoShaderParameterArray2i::initClass();
  if (SoShaderParameterArray3i::getClassTypeId() == SoType::badType())
    SoShaderParameterArray3i::initClass();
  if (SoShaderParameterArray4i::getClassTypeId() == SoType::badType())
    SoShaderParameterArray4i::initClass();
#endif

  SO_SHADER_DIR = coin_getenv("SO_SHADER_DIR");
  shader_dict = new SbHash<const char *, char *>;
  shader_builtin_dict = new SbHash<const char *, char *>;
  setupBuiltinShaders();

  coin_atexit((coin_atexit_f*) soshader_cleanup, CC_ATEXIT_NORMAL);
}




const char *
SoShader::getNamedScript(const SbName & name, const Type type)
{
  char * shader = NULL;

  if (SO_SHADER_DIR) {
    SbString filename(SO_SHADER_DIR);
    filename += "/";
    filename += name.getString();

    switch (type) {
    case ARB_SHADER:
      filename += ".arb";
      break;
    case CG_SHADER:
      filename += ".cg";
      break;
    case GLSL_SHADER:
      filename += ".glsl";
      break;
    default:
      assert(0 && "unknown shader type");
      break;
    }

    SbName shadername(filename.getString());

    if (!shader_dict->get(shadername.getString(), shader)) {
      FILE * fp = fopen(filename.getString(), "rb");
      if (fp) {
        (void) fseek(fp, 0, SEEK_END);
        size_t size = (size_t) ftell(fp);
        (void) fseek(fp, 0, SEEK_SET);

        shader = new char[size+1];
        shader[size] = 0;
        shader_dict->put(shadername, shader);

        if (!(fread(shader, size, 1, fp) == 1)) {
          SoDebugError::postWarning("SoShader::getNamedScript",
                                    "Unable to read shader: %s",
                                    filename.getString());
        }
        fclose(fp);
      }
      else {
        shader_dict->put(shadername, NULL);
        SoDebugError::postWarning("SoShader::getNamedScript",
                                  "Unable to find shader: %s",
                                  filename.getString());
      }
    }
  }
  if (!shader) {
    // try builtin shaders
    if (!shader_builtin_dict->get(name.getString(), shader)) {
      SoDebugError::postWarning("SoShader::getNamedScript",
                                "Unable to find builtin shader: %s",
                                name.getString());
    }
  }

  return shader;
}

void
SoShader::setupBuiltinShaders(void)
{
  shader_builtin_dict->put(SbName("lights/PointLight").getString(), (char*) POINTLIGHT_shadersource);
  shader_builtin_dict->put(SbName("lights/SpotLight").getString(), (char*) SPOTLIGHT_shadersource);
  shader_builtin_dict->put(SbName("lights/DirectionalLight").getString(), (char*) DIRECTIONALLIGHT_shadersource);
  shader_builtin_dict->put(SbName("lights/DirSpotLight").getString(), (char*) DIRSPOTLIGHT_shadersource);
  shader_builtin_dict->put(SbName("vsm/VsmLookup").getString(), (char*) VSMLOOKUP_shadersource);
}
