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
  \class SoGLDriverDatabase SoGLDriverDatabase.h Inventor/misc/SoGLDriverDatabase.h
  \brief The SoGLDriverDatabase class is used for looking up broken/slow features in OpenGL drivers.

  Coin will maintain a database of drivers where we have found
  problems with certain features, even if the OpenGL driver claims to
  support it. This is an effort to avoid application or operating
  system crashes when Coin attempts to use a specific feature. Using
  this database we can either disable this feature, or find another
  way to handle it for broken drivers.

*/

#include <Inventor/misc/SoGLDriverDatabase.h>

#include <cstring>

#include <Inventor/C/tidbits.h>
#include <Inventor/SbName.h>
#include <Inventor/SbString.h>
#include <Inventor/C/glue/gl.h>
#include <Inventor/errors/SoDebugError.h>
#include <Inventor/C/tidbits.h>
#include <Inventor/C/XML/document.h>
#include <Inventor/C/XML/element.h>

#include "misc/SbHash.h"
#include "glue/glp.h"
#include "tidbitsp.h"

class SoGLDriverDatabaseP {
  class SoGLDriver {
  public:
    typedef struct {
      int maxmajor, maxminor, maxmicro;
      int minmajor, minminor, minmicro;
    } versionrange;

    SbList <SbName> platform;
    SbList <SbName> vendor;
    SbList <SbName> renderer;
    SbList <versionrange> version;

    SbList <SbName> broken;
    SbList <SbName> slow;
    SbList <SbName> fast;
    SbList <SbName> disabled;

    SbHash<const char *, const char *> features;

    uint32_t contextid;
  };

  class FeatureID {
  public:
    uint32_t contextid;
    SbName feature;

    // needed for SbHash
    operator unsigned long(void) const {
      unsigned long bitmask = (unsigned long) ((uintptr_t) this->feature.getString());
      bitmask ^= contextid;
      return bitmask;
    }
    int operator==(const FeatureID & v) {
      return (this->contextid == v.contextid) && (this->feature == v.feature);
    }
  };

public:
  SoGLDriverDatabaseP();
  ~SoGLDriverDatabaseP();

  void initFunctions();
  static SbBool multidraw_elements_wrapper(const cc_glglue * glue);
  static SbBool glsl_clip_vertex_hw_wrapper(const cc_glglue * glue);

  SbBool isSupported(const cc_glglue * context, const SbName & feature);
  SbBool isBroken(const cc_glglue * context, const SbName & feature);
  SbBool isDisabled(const cc_glglue * context, const SbName & feature);
  SbBool isSlow(const cc_glglue * context, const SbName & feature);
  SbBool isFast(const cc_glglue * context, const SbName & feature);

  SbName getComment(const cc_glglue * context, const SbName & feature);

  SbBool addBuffer(const char * buffer);
  SbBool addFile(const SbName & filename);
  SbBool addFeature(const SbName & feature, const SbName & comment);

  SbBool loadDefaultDatabase();
  SbBool loadFromFile(const SbName & filename);
  SbBool loadFromBuffer(const char * buffer);

  static const int databaseloaderversion;
private:
  void addFeatures(const cc_glglue * context, const cc_xml_element * element, SoGLDriver * driver);

  cc_xml_element * findPlatform(const cc_xml_elt * root, const char * platformstring);
  cc_xml_element * findVendor(const cc_xml_elt * platform, const char * vendorstring);
  cc_xml_element * findDriver(const cc_xml_elt * vendor, const cc_glglue * context);
  SoGLDriver * findGLDriver(const cc_glglue * context);

  SbBool checkDocumentVersion(cc_xml_doc * document);
  SbBool addDocument(const cc_xml_doc * document);

  SbBool mergeFeatures(cc_xml_elt * destination, const cc_xml_elt * source);
  SbBool mergeFeature(cc_xml_elt * destination, const cc_xml_elt * feature);
  SbBool mergeDriver(cc_xml_elt * vendor, const cc_xml_elt * driver);
  SbBool mergeVendor(cc_xml_elt * platform, const cc_xml_elt * vendor);
  SbBool mergePlatform(const cc_xml_elt * platform);
  SbBool mergeRoot(const cc_xml_elt * root);

  cc_xml_elt * getDatabaseRoot();

  cc_xml_doc * database;

  SbList <SoGLDriver*> driverlist;
  SbHash<FeatureID, SbBool> brokencache;
  SbHash<FeatureID, SbBool> slowcache;
  SbHash<FeatureID, SbBool> fastcache;
  SbHash<FeatureID, SbBool> disabledcache;

  typedef SbBool glglue_feature_test_f(const cc_glglue * glue);
  SbHash<const char *, glglue_feature_test_f *> featuremap;
};

// static variables
const int SoGLDriverDatabaseP::databaseloaderversion = 1;


SoGLDriverDatabaseP::SoGLDriverDatabaseP()
{
  this->initFunctions();
  this->database = NULL;
}

SoGLDriverDatabaseP::~SoGLDriverDatabaseP()
{
  if (this->database != NULL) {
#if COIN_DEBUG
    cc_xml_doc_write_to_file(this->database, "database_debug.xml");
#endif
    cc_xml_doc_delete_x(this->database);
  }

  for(int i = 0; i < driverlist.getLength(); i++) {
    delete driverlist[i];
  }
}

SbBool
SoGLDriverDatabaseP::multidraw_elements_wrapper(const cc_glglue * glue)
{
  // FIXME: I'm not able to make glMultiDrawElement work under OS
  // X. It just crashes inside GL when I try to use it. Investigate
  // why this happens. For now we just avoid using
  // glMultiDrawElements() under OS X.  pederb, 2005-02-14
  SbBool ismac = (coin_runtime_os() == COIN_OS_X);
  if (!ismac) return cc_glglue_has_multidraw_vertex_arrays(glue);
  return FALSE;
}

SbBool
SoGLDriverDatabaseP::glsl_clip_vertex_hw_wrapper(const cc_glglue * glue)
{
  // ATi doesn't support gl_ClipVertex in hardware, according to their own own paper
  // "ATI OpenGL Programming and Optimization Guide", which can be found at
  // http://developer.amd.com/media/gpu_assets/ATI_OpenGL_Programming_and_Optimization_Guide.pdf
  if (glue->vendor_is_ati) return FALSE;
  return TRUE;
}

/*
  Initialize tests and reserve some feature names for features that cannot
  be tested directly as a single OpenGL extension test
*/
void
SoGLDriverDatabaseP::initFunctions(void)
{
  this->featuremap[SbName(SO_GL_MULTIDRAW_ELEMENTS).getString()] =
                       (glglue_feature_test_f *) &multidraw_elements_wrapper;
  this->featuremap[SbName(SO_GL_POLYGON_OFFSET).getString()] =
                       (glglue_feature_test_f *) &cc_glglue_has_polygon_offset;
  this->featuremap[SbName(SO_GL_TEXTURE_OBJECT).getString()] =
                       (glglue_feature_test_f *) &cc_glglue_has_texture_objects;
  this->featuremap[SbName(SO_GL_3D_TEXTURES).getString()] =
                       (glglue_feature_test_f *) &cc_glglue_has_3d_textures;
  this->featuremap[SbName(SO_GL_MULTITEXTURE).getString()] =
                       (glglue_feature_test_f *) &cc_glglue_has_multitexture;
  this->featuremap[SbName(SO_GL_TEXSUBIMAGE).getString()] =
                       (glglue_feature_test_f *) &cc_glglue_has_texsubimage;
  this->featuremap[SbName(SO_GL_2D_PROXY_TEXTURES).getString()] =
                       (glglue_feature_test_f *) &cc_glglue_has_2d_proxy_textures;
  this->featuremap[SbName(SO_GL_TEXTURE_EDGE_CLAMP).getString()] =
                       (glglue_feature_test_f *) &cc_glglue_has_texture_edge_clamp;
  this->featuremap[SbName(SO_GL_TEXTURE_COMPRESSION).getString()] =
                       (glglue_feature_test_f *) &cc_glue_has_texture_compression;
  this->featuremap[SbName(SO_GL_COLOR_TABLES).getString()] =
                       (glglue_feature_test_f *) &cc_glglue_has_color_tables;
  this->featuremap[SbName(SO_GL_COLOR_SUBTABLES).getString()] =
                       (glglue_feature_test_f *) &cc_glglue_has_color_subtables;
  this->featuremap[SbName(SO_GL_PALETTED_TEXTURES).getString()] =
                       (glglue_feature_test_f *) &cc_glglue_has_paletted_textures;
  this->featuremap[SbName(SO_GL_BLEND_EQUATION).getString()] =
                       (glglue_feature_test_f *) &cc_glglue_has_blendequation;
  this->featuremap[SbName(SO_GL_VERTEX_ARRAY).getString()] =
                       (glglue_feature_test_f *) &cc_glglue_has_vertex_array;
  this->featuremap[SbName(SO_GL_NV_VERTEX_ARRAY_RANGE).getString()] =
                       (glglue_feature_test_f *) &cc_glglue_has_nv_vertex_array_range;
  this->featuremap[SbName(SO_GL_VERTEX_BUFFER_OBJECT).getString()] =
                       (glglue_feature_test_f *) &cc_glglue_has_vertex_buffer_object;
  this->featuremap[SbName(SO_GL_ARB_FRAGMENT_PROGRAM).getString()] =
                       (glglue_feature_test_f *) &cc_glglue_has_arb_fragment_program;
  this->featuremap[SbName(SO_GL_ARB_VERTEX_PROGRAM).getString()] =
                       (glglue_feature_test_f *) &cc_glglue_has_arb_vertex_program;
  this->featuremap[SbName(SO_GL_ARB_VERTEX_SHADER).getString()] =
                       (glglue_feature_test_f *) &cc_glglue_has_arb_vertex_shader;
  this->featuremap[SbName(SO_GL_ARB_SHADER_OBJECT).getString()] =
                       (glglue_feature_test_f *) &cc_glglue_has_arb_shader_objects;
  this->featuremap[SbName(SO_GL_OCCLUSION_QUERY).getString()] =
                       (glglue_feature_test_f *) &cc_glglue_has_occlusion_query;
  this->featuremap[SbName(SO_GL_FRAMEBUFFER_OBJECT).getString()] =
                       (glglue_feature_test_f *) &cc_glglue_has_framebuffer_objects;
  this->featuremap[SbName(SO_GL_ANISOTROPIC_FILTERING).getString()] =
                       (glglue_feature_test_f *) &cc_glglue_can_do_anisotropic_filtering;
  this->featuremap[SbName(SO_GL_SORTED_LAYERS_BLEND).getString()] =
                       (glglue_feature_test_f *) &cc_glglue_can_do_sortedlayersblend;
  this->featuremap[SbName(SO_GL_BUMPMAPPING).getString()] =
                       (glglue_feature_test_f *) &cc_glglue_can_do_bumpmapping;
  this->featuremap[SbName(SO_GL_VBO_IN_DISPLAYLIST).getString()] =
                       (glglue_feature_test_f *) &coin_glglue_vbo_in_displaylist_supported;
  this->featuremap[SbName(SO_GL_NON_POWER_OF_TWO_TEXTURES).getString()] =
                       (glglue_feature_test_f *) &coin_glglue_non_power_of_two_textures;
  this->featuremap[SbName(SO_GL_GENERATE_MIPMAP).getString()] =
                       (glglue_feature_test_f *) &coin_glglue_has_generate_mipmap;
  this->featuremap[SbName(SO_GL_GLSL_CLIP_VERTEX_HW).getString()] =
                       (glglue_feature_test_f *) &glsl_clip_vertex_hw_wrapper;
}

SbBool
SoGLDriverDatabaseP::isSupported(const cc_glglue * context, const SbName & feature)
{
  // check if we're asking about an actual GL extension
  const char * str = feature.getString();
  if ((feature.getLength() > 3) && (str[0] == 'G') && (str[1] == 'L') && (str[2] == '_')) {
    if (!cc_glglue_glext_supported(context, feature)) return FALSE;
  }
  else { // check our lookup table
    SbHash<const char*, glglue_feature_test_f *>::const_iterator iter = this->featuremap.find(feature.getString());
    if (iter!=this->featuremap.const_end()) {
      glglue_feature_test_f * testfunc = iter->obj;
      if (!testfunc(context)) return FALSE;
    }
    else {
      SoDebugError::post("SoGLDriverDatabase::isSupported",
                         "Unknown feature '%s'.", feature.getString());
    }
  }
  return !(this->isBroken(context, feature) || this->isDisabled(context, feature));
}

SbBool
SoGLDriverDatabaseP::isBroken(const cc_glglue * context, const SbName & feature)
{
  FeatureID f;
  f.contextid = context->contextid;
  f.feature = feature;

  SbBool broken = FALSE;
  SbHash<SoGLDriverDatabaseP::FeatureID, int>::const_iterator iter = 
    this->brokencache.find(f);
  if (iter!=this->brokencache.const_end()) {
    broken = iter->obj;
    SoGLDriver * driver = this->findGLDriver(context);
    if (driver) {
      if (driver->broken.find(feature) != -1) broken = TRUE;
    }
    this->brokencache[f] = broken;
  }
  return broken;
}

SbBool
SoGLDriverDatabaseP::isDisabled(const cc_glglue * context, const SbName & feature)
{
  FeatureID f;
  f.contextid = context->contextid;
  f.feature = feature;

  SbBool disabled = FALSE;
  SbHash<SoGLDriverDatabaseP::FeatureID, int>::const_iterator iter =
    this->disabledcache.find(f);
  if (iter!=this->disabledcache.const_end()) {
    disabled = iter->obj;
    SoGLDriver * driver = this->findGLDriver(context);

    if (driver) {
      if (driver->disabled.find(feature) != -1) disabled = TRUE;
    }
    this->disabledcache[f] = disabled;
  }
  return disabled;
}

SbBool
SoGLDriverDatabaseP::isSlow(const cc_glglue * context, const SbName & feature)
{
  if (!this->isSupported(context, feature)) {
    SoDebugError::post("SoGLDriverDatabase::isSlow",
                       "Feature '%s' is not supported for the specified context.",
                       feature.getString());
    return TRUE;
  }
  FeatureID f;
  f.contextid = context->contextid;
  f.feature = feature;

  SbBool slow = FALSE;
  if (!this->slowcache.get(f, slow)) {
    SoGLDriver * driver = this->findGLDriver(context);
    if (driver) {
      if (driver->slow.find(feature) != -1) slow = TRUE;
    }
    this->slowcache[f] = slow;
  }
  return slow;
}

SbBool
SoGLDriverDatabaseP::isFast(const cc_glglue * context, const SbName & feature)
{
  if (!this->isSupported(context, feature)) {
    SoDebugError::post("SoGLDriverDatabase::isFast",
                       "Feature '%s' is not supported for the specified context.",
                       feature.getString());
    return FALSE;
  }

  FeatureID f;
  f.contextid = context->contextid;
  f.feature = feature;

  SbBool fast = FALSE;
  if (!this->fastcache.get(f, fast)) {
    SoGLDriver * driver = this->findGLDriver(context);
    if (driver) {
      if (driver->fast.find(feature) != -1) fast = TRUE;
    }
    this->fastcache[f] = fast;
  }
  return fast;
}

/*
  Get the comment for \a feature in \a context, returns "undefined" if the
  feature has no entry. Provides extensibility to the database.
*/
SbName
SoGLDriverDatabaseP::getComment(const cc_glglue * context, const SbName & feature)
{
  const char * comment = NULL;

  SoGLDriver * driver = this->findGLDriver(context);

  if (driver) {
    if (!driver->features.get(feature, comment))
      return SbName("undefined");
  }
  return SbName(comment);
}

/*
  Used to find a specific platform element, specified with \a platformstring, inside another element
*/
cc_xml_element *
SoGLDriverDatabaseP::findPlatform(const cc_xml_elt * root, const char * platformstring)
{
  cc_xml_element * platform = NULL;

  unsigned int numplatforms = cc_xml_elt_get_num_children_of_type(root, "platform");

  for(unsigned int i = 0; i < numplatforms; i++) {
    platform = cc_xml_elt_get_child_of_type(root, "platform", i);

    if (platform) {
      cc_xml_element * name = cc_xml_elt_get_child_of_type(platform, "name", 0);

      SbName namestr;

      if (!name) {
#if COIN_DEBUG
        SoDebugError::postWarning("SoGLDriverDatabaseP::findPlatform", "Missing name in platform element!");
#endif
        namestr = "undefined";
      }
      else
        namestr = cc_xml_elt_get_cdata(name);

      if (strcmp(namestr, platformstring) == 0) {
        return platform;
      }
      else {
        unsigned int numaliases = cc_xml_elt_get_num_children_of_type(platform, "alias");

        for(unsigned int j = 0; j < numaliases; j++) {
          if (strcmp(cc_xml_elt_get_cdata(cc_xml_elt_get_child_of_type(platform, "alias", j)), platformstring) == 0) {
            return platform;
          }
        }
      }
    }
  }
  return NULL;
}

/*
  Used to find a specific vendor element, specified with \a vendorstring, inside a platform element.
*/
cc_xml_element *
SoGLDriverDatabaseP::findVendor(const cc_xml_elt * platform, const char * vendorstring)
{
  unsigned int numvendors = cc_xml_elt_get_num_children_of_type(platform, "vendor");

  for(unsigned int i = 0; i < numvendors; i++) {
    cc_xml_element * vendor = cc_xml_elt_get_child_of_type(platform, "vendor", i);

    if (vendor) {
      cc_xml_element * name = cc_xml_elt_get_child_of_type(vendor, "name", 0);

      SbName namestr;

      if (!name) {
#if COIN_DEBUG
        SoDebugError::postWarning("SoGLDriverDatabaseP::findPlatform", "Missing name in vendor element!");
#endif
        namestr = "undefined";
      }
      else
        namestr = cc_xml_elt_get_cdata(name);

      if (strcmp(namestr, vendorstring) == 0) {
        return vendor;
      }
      else {
        unsigned int numaliases = cc_xml_elt_get_num_children_of_type(vendor, "alias");

        for(unsigned int j = 0; j < numaliases; j++) {
          if (strcmp(cc_xml_elt_get_cdata(cc_xml_elt_get_child_of_type(vendor, "alias", j)), vendorstring) == 0) {
            return vendor;
          }
        }
      }
    }
  }
  return NULL;
}

/*
  Used to find a specific driver element inside a vendor element.
*/
cc_xml_element *
SoGLDriverDatabaseP::findDriver(const cc_xml_elt * vendor, const cc_glglue * COIN_UNUSED_ARG(context))
{
  unsigned int numDrivers = cc_xml_elt_get_num_children_of_type(vendor, "driver");

  for(unsigned int k = 0; k < numDrivers; k++) {
    cc_xml_element * driver = cc_xml_elt_get_child_of_type(vendor, "driver", k);
    cc_xml_element * versionrange = cc_xml_elt_get_child_of_type(driver, "versionrange", 0);

    if (!versionrange) {
#if COIN_DEBUG
      SoDebugError::postWarning("SoGLDriverDatabaseP::findDriver", "Missing versioninfo in driver element!");
#endif
    }
    else {
      cc_xml_element * minversionelement = cc_xml_elt_get_child_of_type(versionrange, "minversion", 0);
      cc_xml_element * maxversionelement = cc_xml_elt_get_child_of_type(versionrange, "maxversion", 0);

      SbString minversion;
      SbString maxversion;

      if (minversionelement)
        minversion = cc_xml_elt_get_cdata(minversionelement);
      if (maxversionelement)
        maxversion = cc_xml_elt_get_cdata(maxversionelement);

      unsigned int minversion_major = 0;
      unsigned int minversion_minor = 0;
      unsigned int minversion_micro = 0;
      unsigned int minversion_nano = 0;
      unsigned int maxversion_major = 0;
      unsigned int maxversion_minor = 0;
      unsigned int maxversion_micro = 0;
      unsigned int maxversion_nano = 0;

      SbIntList indices;

      minversion.findAll(".", indices);

      if (indices.getLength() >= 0)
        minversion_major = atoi(minversion.getString());
      if (indices.getLength() > 0)
        minversion_minor = atoi(minversion.getSubString(indices[0] + 1).getString());
      if (indices.getLength() > 1)
        minversion_micro = atoi(minversion.getSubString(indices[1] + 1).getString());
      if (indices.getLength() > 2)
        minversion_nano = atoi(minversion.getSubString(indices[2] + 1).getString());

      maxversion.findAll(".", indices);

      if (indices.getLength() >= 0)
        maxversion_major = atoi(maxversion.getString());
      if (indices.getLength() > 0)
        maxversion_minor = atoi(maxversion.getSubString(indices[0] + 1).getString());
      if (indices.getLength() > 1)
        maxversion_micro = atoi(maxversion.getSubString(indices[1] + 1).getString());
      if (indices.getLength() > 2)
        maxversion_nano = atoi(maxversion.getSubString(indices[2] + 1).getString());

      //FIXME: Match Driver and add features.
      //Must probably use platform specific functions to be of any use.
      //oyshole, 20080314
    }
  }
  return NULL;
}

/*
  Find and/or create an SoGLDriver for the current context. SoGLDriver objects
  are cached per context.
*/
SoGLDriverDatabaseP::SoGLDriver *
SoGLDriverDatabaseP::findGLDriver(const cc_glglue * context)
{
  //FIXME: Expand platform search with versioning etc.
  //20080318, oyshole

  SbName platformstring("undefined");

  switch(coin_runtime_os()) {
    case COIN_OS_X:
      platformstring = "Apple";
      break;
    case COIN_MSWINDOWS:
      platformstring = "Win32";
      break;
    case COIN_UNIX:
      platformstring = "Unix";
      break;
    default:
      break;
  };

  SbName vendorstring(context->vendorstr);
  SbName renderer(context->rendererstr);
  SbName versionstring(context->versionstr);

  SoGLDriver * driver = NULL;

  // Check if a driver object has been created for this context. If so
  // use this driver.
  for(int i = 0; i < driverlist.getLength(); i++) {
    if (driverlist[i]->contextid == context->contextid) {
      driver = driverlist[i];
      break;
    }
  }

  if (!driver) {
    driver = new SoGLDriver();
    driver->contextid = context->contextid;

    driverlist.append(driver);

    if (this->database) {
      cc_xml_element * root = cc_xml_doc_get_root(this->database);

      if (root) {
        addFeatures(context, root, driver);
        const cc_xml_element * platform = findPlatform(root, platformstring);

        if (platform) {
          addFeatures(context, platform, driver);
          const cc_xml_element * vendor = findVendor(platform, vendorstring);

          if (vendor) {
            addFeatures(context, vendor, driver);
            /*const cc_xml_element * driver = */findDriver(vendor, context);

            // FIXME: Implement driver matching
            // oyshole, 20080314
          }
        }
      }
    }
  }
  return driver;
}

/*
  Load the database from the XML file specified in \a filename
*/
SbBool
SoGLDriverDatabaseP::loadFromFile(const SbName & filename)
{
  if (this->database != NULL)
    cc_xml_doc_delete_x(this->database);

  this->database = cc_xml_doc_new();

  SbBool result = cc_xml_doc_read_file_x(this->database, filename);

  if (!result || !checkDocumentVersion(this->database)) {
    cc_xml_doc_delete_x(this->database);
    this->database = NULL;
  }
  return result;
}

/*
  Load the database from the buffer specified in \a buffer
*/
SbBool
SoGLDriverDatabaseP::loadFromBuffer(const char * buffer)
{
  if (this->database != NULL)
    cc_xml_doc_delete_x(this->database);

  this->database = cc_xml_doc_new();

  SbBool result = cc_xml_doc_read_buffer_x(this->database, buffer, strlen(buffer));

  if (!result || !checkDocumentVersion(this->database)) {
    cc_xml_doc_delete_x(this->database);
    this->database = NULL;
  }
  return result;
}

/*
  Merge the feature elements from \a source into \a destination
*/
SbBool
SoGLDriverDatabaseP::mergeFeatures(cc_xml_elt * destination, const cc_xml_elt * source)
{
  SbBool result = TRUE;
  unsigned int numfeatures = cc_xml_elt_get_num_children_of_type(source, "feature");

  for(unsigned int i = 0; i < numfeatures; i++) {
    cc_xml_element * feature = cc_xml_elt_get_child_of_type(source, "feature", i);

    if (!mergeFeature(destination, feature))
      result = FALSE;
  }
  return result;
}

/*
  Merge the feature element \a feature into \a destination
*/
SbBool
SoGLDriverDatabaseP::mergeFeature(cc_xml_elt * destination, const cc_xml_elt * feature)
{
  cc_xml_element * name = cc_xml_elt_get_child_of_type(feature, "name", 0);
  SbName featurename = "undefined";

  if (name)
    featurename = cc_xml_elt_get_cdata(name);

  unsigned int numfeatures = cc_xml_elt_get_num_children_of_type(destination, "feature");

  // Search for entries on same feature string
  for(unsigned int i = 0; i < numfeatures; i++) {
    cc_xml_element * existingfeature = cc_xml_elt_get_child_of_type(destination, "feature", i);
    cc_xml_element * existingname = cc_xml_elt_get_child_of_type(existingfeature, "name", 0);

    SbName existingfeaturename = "undefined";

    if (existingname)
      existingfeaturename = cc_xml_elt_get_cdata(existingname);

    if (existingfeaturename == featurename) {
      cc_xml_element * comment = cc_xml_elt_get_child_of_type(feature, "comment", 0);
      SbName commentstr = cc_xml_elt_get_cdata(comment);

      cc_xml_element * existingcomment = cc_xml_elt_get_child_of_type(existingfeature, "comment", 0);

      cc_xml_elt_set_cdata_x(existingcomment, commentstr);

      return TRUE;
    }
  }
  cc_xml_elt_add_child_x(destination, cc_xml_elt_clone(feature));
  return TRUE;
}

/*
  Merge the driver element \a driver into the vendor element \a vendor
*/
SbBool
SoGLDriverDatabaseP::mergeDriver(cc_xml_elt * COIN_UNUSED_ARG(vendor), const cc_xml_elt * COIN_UNUSED_ARG(driver))
{
  return TRUE;
}

SbBool
SoGLDriverDatabaseP::mergeVendor(cc_xml_elt * platform, const cc_xml_elt * vendor)
{
  SbBool result = TRUE;
  cc_xml_element * name = cc_xml_elt_get_child_of_type(vendor, "name", 0);
  SbName namestr = cc_xml_elt_get_cdata(name);

  cc_xml_elt * existingvendor = NULL;

  existingvendor = findVendor(platform, namestr);

  if (existingvendor == NULL) {
    // Try Aliases
    unsigned int numaliases = cc_xml_elt_get_num_children_of_type(vendor, "alias");

    for(unsigned int i = 0; i < numaliases; i++) {
      SbName aliasstr = cc_xml_elt_get_cdata(cc_xml_elt_get_child_of_type(vendor, "alias", i));

      existingvendor = findVendor(platform, aliasstr);
    }
  }

  if (existingvendor == NULL) {
    cc_xml_elt_add_child_x(platform, cc_xml_elt_clone(vendor));
    result = TRUE;
  }
  else {
    if (!mergeFeatures(existingvendor, vendor))
      result = FALSE;

    unsigned int numdrivers = cc_xml_elt_get_num_children_of_type(vendor, "driver");

    for(unsigned int i = 0; i < numdrivers; i++) {
      cc_xml_element * driver = cc_xml_elt_get_child_of_type(vendor, "driver", i);

      if (!mergeDriver(existingvendor, driver))
        result = FALSE;
    }
  }
  return result;
}

/*
  Merge the platform element \a platform into the database
*/
SbBool
SoGLDriverDatabaseP::mergePlatform(const cc_xml_elt * platform)
{
  SbBool result = TRUE;
  cc_xml_element * name = cc_xml_elt_get_child_of_type(platform, "name", 0);
  SbName namestr = cc_xml_elt_get_cdata(name);

  cc_xml_elt * root = getDatabaseRoot();

  cc_xml_elt * existingplatform = NULL;

  existingplatform = findPlatform(root, namestr);

  if (existingplatform == NULL) {
    // Try Aliases
    unsigned int numaliases = cc_xml_elt_get_num_children_of_type(platform, "alias");

    for(unsigned int i = 0; i < numaliases; i++) {
      SbName aliasstr = cc_xml_elt_get_cdata(cc_xml_elt_get_child_of_type(platform, "alias", i));

      existingplatform = findPlatform(root, aliasstr);
    }
  }

  if (existingplatform == NULL) {
    cc_xml_elt_add_child_x(root, cc_xml_elt_clone(platform));
    result = TRUE;
  }
  else {
    if (!mergeFeatures(existingplatform, platform))
      result = FALSE;

    unsigned int numvendors = cc_xml_elt_get_num_children_of_type(platform, "vendor");

    for(unsigned int i = 0; i < numvendors; i++) {
      cc_xml_element * vendor = cc_xml_elt_get_child_of_type(platform, "vendor", i);

      if (!mergeVendor(existingplatform, vendor))
        result = FALSE;
    }
  }
  return result;
}

/*
  Merge the database in \a root into the current database
*/
SbBool
SoGLDriverDatabaseP::mergeRoot(const cc_xml_elt * root)
{
  SbBool result = TRUE;

  if (!mergeFeatures(getDatabaseRoot(), root))
    result = FALSE;

  unsigned int numplatforms = cc_xml_elt_get_num_children_of_type(root, "platform");

  for(unsigned int i = 0; i < numplatforms; i++) {
    cc_xml_elt * platform = cc_xml_elt_get_child_of_type(root, "platform", i);

    if (!mergePlatform(platform))
      result = FALSE;
  }
  return result;
}

/*
  Get a pointer to the root element of the current database. If the database
  is not created, it will be initialized with an empty root element.
*/
cc_xml_elt *
SoGLDriverDatabaseP::getDatabaseRoot()
{
  if (this->database == NULL)
    this->database = cc_xml_doc_new();

  cc_xml_elt * root = cc_xml_doc_get_root(this->database);

  if (!root) {
    root = cc_xml_elt_new();
    cc_xml_elt_set_type_x(root, "featuredatabase");
    cc_xml_doc_set_root_x(this->database, root);
  }
  return root;
}

/*
  Check if the version of the XML document is current. This returns TRUE
  if the database is assumed safe for loading, FALSE if not.
*/
SbBool
SoGLDriverDatabaseP::checkDocumentVersion(cc_xml_doc * document)
{
  if (!document)
    return FALSE;

  cc_xml_element * root = cc_xml_doc_get_root(document);

  if (!root)
    return FALSE;

  cc_xml_element * version = cc_xml_elt_get_child_of_type(root, "version", 0);

  if (!version) {
    // FIXME: Should issue warning, but SoDebugError is not initialized yet.
    // SoDebugError::postWarning("SoDriverDatabaseP::checkDocumentVersion",
    // "Version element not found, this might indicate old or corrupted data "
    // "which could lead to errors!");
    // 20080325, oyshole
    return TRUE;
  }

  const char * versionstring = cc_xml_elt_get_cdata(version);
  int versionnumber = atoi(versionstring);

  // FIXME: Implement handling of versions properly instead of just warning
  // 20080325, oyshole

  if (versionnumber != databaseloaderversion) { // Current version is 1
    // FIXME: Should issue warning, but SoDebugError is not initialized yet.
    // SoDebugError::postWarning("SoDriverDatabaseP::checkDocumentVersion",
    // "The version number of the XML data being imported does not correspond"
    // "to the current loader version. This could lead to errors!");
    // 20080325, oyshole
  }
  return TRUE;
}

/*
  Add a XML document \a document to the current database.
*/
SbBool
SoGLDriverDatabaseP::addDocument(const cc_xml_doc * document)
{
  cc_xml_element * root = cc_xml_doc_get_root(document);

  if (!root) {
    return FALSE;
  }

  SbBool result = TRUE;
  SbName roottype = cc_xml_elt_get_type(root);

  if (roottype == "featuredatabase") {
    // Merge on root node
    result = mergeRoot(root);
  }
  else if (roottype == "feature") {
    // Merge global feature
    cc_xml_elt * db = getDatabaseRoot();
    mergeFeature(db, root);
  }
  else if (roottype == "platform") {
    // Merge on platform
    result = mergePlatform(root);
  }
  else {
    // No insertable element found
    result = FALSE;
  }
  return result;
}

/*
  Add XML data from \a buffer to the database.
*/
SbBool
SoGLDriverDatabaseP::addBuffer(const char * buffer)
{
  cc_xml_doc * doc = cc_xml_doc_new();
  SbBool result = cc_xml_doc_read_buffer_x(doc, buffer, strlen(buffer));

  if (!result || !checkDocumentVersion(doc)) {
    cc_xml_doc_delete_x(doc);
    return FALSE;
  }

  result = addDocument(doc);
  cc_xml_doc_delete_x(doc);
  return result;
}

/*
  Add XML data from the file specified in \a filename to the database.
*/
SbBool
SoGLDriverDatabaseP::addFile(const SbName & filename)
{
  cc_xml_doc * doc = cc_xml_doc_new();
  SbBool result = cc_xml_doc_read_file_x(doc, filename);

  if (!result || !checkDocumentVersion(doc)) {
    cc_xml_doc_delete_x(doc);
    return FALSE;
  }

  result = addDocument(doc);
  cc_xml_doc_delete_x(doc);
  return result;
}

/*
  Add a feature to the database.
*/
SbBool
SoGLDriverDatabaseP::addFeature(const SbName & feature, const SbName & comment)
{
  cc_xml_elt * root = getDatabaseRoot();

  cc_xml_elt * featureelement = cc_xml_elt_new_from_data("feature", NULL);
  cc_xml_elt * nameelement = cc_xml_elt_new_from_data("name", NULL);
  cc_xml_elt * commentelement = cc_xml_elt_new_from_data("comment", NULL);

  cc_xml_elt_set_cdata_x(nameelement, feature);
  cc_xml_elt_set_cdata_x(commentelement, comment);

  cc_xml_elt_add_child_x(featureelement, nameelement);
  cc_xml_elt_add_child_x(featureelement, commentelement);

  return mergeFeature(root, featureelement);
}

/*
  Loads default database.
*/
SbBool
SoGLDriverDatabaseP::loadDefaultDatabase()
{
  // FIXME: Implement default loading of database. This could be from
  // a header file preprocessed and included with Coin, or from a
  // directory (possibly defined with an env. variable).
  // 20080325, oyshole
  return TRUE;
}

/*
  Add the features under \a element to the SoGLDriver specified in \a driver
*/
void
SoGLDriverDatabaseP::addFeatures(const cc_glglue * COIN_UNUSED_ARG(context), const cc_xml_element * element, SoGLDriver * driver)
{
  unsigned int numfeatures = cc_xml_elt_get_num_children_of_type(element, "feature");

  for(unsigned int i = 0; i < numfeatures; i++) {
    cc_xml_element * feature = cc_xml_elt_get_child_of_type(element, "feature", i);
    cc_xml_element * name = cc_xml_elt_get_child_of_type(feature, "name", 0);
    cc_xml_element * comment = cc_xml_elt_get_child_of_type(feature, "comment", 0);

    SbName featurename = "undefined";
    SbName commentstr = "undefined";

    if (name && comment) {
      featurename = cc_xml_elt_get_cdata(name);
      commentstr = cc_xml_elt_get_cdata(comment);

      driver->features[featurename] = commentstr;
    }

  #if COIN_DEBUG
    SoDebugError::postWarning("SoGLDriverDatabaseP::addFeature", "Feature %s is %s!", featurename.getString(), commentstr.getString());
  #endif

    if (strcmp("disabled", commentstr) == 0) {
      driver->disabled.append(featurename);
    }
    else if (strcmp("broken", commentstr) == 0) {
      driver->broken.append(featurename);
    }
    else if (strcmp("slow", commentstr) == 0) {
      driver->slow.append(featurename);
    }
    else if (strcmp("fast", commentstr) == 0) {
      driver->fast.append(featurename);
    }
    else if (strcmp("enabled", commentstr) == 0) {
      // Do nothing, let extension/feature tests decide
    }
    else if (strcmp("supported", commentstr) == 0) {
      // Do nothing, let extension/feature tests decide
    }
    else {
#if COIN_DEBUG
    SoDebugError::postWarning("SoGLDriverDatabaseP::addFeature",
      "Feature %s has unknown or no comment.",
      featurename.getString());
#endif
    }
  }
}

/*!

  Convenience function which checks whether \a feature is supported
  for \a context.  If \a feature is an OpenGL extension, it checks if
  it is actually supported by the driver, and then calls
  SoGLDriverDatabase::isBroken() to check if the feature is broken for
  \a context.

 */
SbBool
SoGLDriverDatabase::isSupported(const cc_glglue * context, const SbName & feature)
{
  return pimpl()->isSupported(context, feature);
}

/*!
  Checks the driver database to see if \a feature is tagged as broken.
*/
SbBool
SoGLDriverDatabase::isBroken(const cc_glglue * context, const SbName & feature)
{
  return pimpl()->isBroken(context, feature);
}

/*!
  Checks the driver database to see if \a feature is tagged as being slow.
*/
SbBool
SoGLDriverDatabase::isSlow(const cc_glglue * context, const SbName & feature)
{
  return pimpl()->isSlow(context, feature);
}

/*!
  Checks the driver database to see if \a feature is tagged as fast.
*/
SbBool
SoGLDriverDatabase::isFast(const cc_glglue * context, const SbName & feature)
{
  return pimpl()->isFast(context, feature);
}

/*!
  Get the comment for \a feature in \a context, returns "undefined" if the
  feature has no entry. Provides extensibility to the database.
*/
SbName
SoGLDriverDatabase::getComment(const cc_glglue * context, const SbName & feature)
{
  return pimpl()->getComment(context, feature);
}

/*!
  Load the driver database from \a buffer
*/
void
SoGLDriverDatabase::loadFromBuffer(const char * buffer)
{
  pimpl()->loadFromBuffer(buffer);
}

/*!
  Load the driver database from the file specified in \a filename
*/
void
SoGLDriverDatabase::loadFromFile(const SbName & filename)
{
  pimpl()->loadFromFile(filename);
}

/*!
  Add the XML data in \a buffer to the driver database
*/
void
SoGLDriverDatabase::addBuffer(const char * buffer)
{
  pimpl()->addBuffer(buffer);
}

/*!
  Add the XML data in \a filename to the driver database
*/
void
SoGLDriverDatabase::addFile(const SbName & filename)
{
  pimpl()->addFile(filename);
}

/*!
  Add a feature to the driver database
*/
void
SoGLDriverDatabase::addFeature(const SbName & feature, const SbName & comment)
{
  pimpl()->addFeature(feature, comment);
}

static SoGLDriverDatabaseP * pimpl_instance = NULL;

static void sogldriverdatabase_atexit(void)
{
  delete pimpl_instance;
  pimpl_instance = NULL;
}

SoGLDriverDatabaseP *
SoGLDriverDatabase::pimpl(void)
{
  if (pimpl_instance == NULL) {
    pimpl_instance = new SoGLDriverDatabaseP;
    cc_coin_atexit((coin_atexit_f*) sogldriverdatabase_atexit);
  }
  return pimpl_instance;
}

/**************************************************************************/

#include <iostream>
using namespace std;

void find_features(const cc_xml_elt * element);

void
SoGLDriverDatabase::init(void)
{
  // make sure the private static class is created to avoid race conditions
  (void) pimpl();

  pimpl()->loadDefaultDatabase();
}

/**************************************************************************/
