/************
*
*   This file is part of a tool for producing 3D content in the PRC format.
*   Copyright (C) 2008  Orest Shardt <shardtor (at) gmail dot com> and
*                       Michail Vidiassov <master@iaas.msu.ru>
*
*   This program is free software: you can redistribute it and/or modify
*   it under the terms of the GNU Lesser General Public License as published by
*   the Free Software Foundation, either version 3 of the License, or
*   (at your option) any later version.
*
*   This program is distributed in the hope that it will be useful,
*   but WITHOUT ANY WARRANTY; without even the implied warranty of
*   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
*   GNU Lesser General Public License for more details.
*
*   You should have received a copy of the GNU Lesser General Public License
*   along with this program.  If not, see <http://www.gnu.org/licenses/>.
*
*************/

#include "writePRC.h"
#include <climits>
#include <cassert>

// debug print includes
#include <iostream>
#include <iomanip>
#include <fstream>
#include <sstream>

#if !defined(__GNUC__) || defined(__clang__)
#include <vector>
#endif

using namespace std;

#ifndef __GNUC_PREREQ
#define __GNUC_PREREQ(maj, min) (0)
#endif

// Count leading zeros.
uint32_t CLZ(uint32_t a) 
{
#if __GNUC_PREREQ(3,4)
  return __builtin_clz(a);
#else
// find the log base 2 of a 32-bit integer
  static const int MultiplyDeBruijnBitPosition[32] = {
    0, 9, 1, 10, 13, 21, 2, 29, 11, 14, 16, 18, 22, 25, 3, 30,
    8, 12, 20, 28, 15, 17, 24, 7, 19, 27, 23, 6, 26, 5, 4, 31
  };

  a |= a >> 1; // first round down to one less than a power of 2 
  a |= a >> 2;
  a |= a >> 4;
  a |= a >> 8;
  a |= a >> 16;

  return 31-MultiplyDeBruijnBitPosition[(uint32_t)(a * 0x07C4ACDDU) >> 27];
#endif
}

// Portable integer implementation of ceil(log2(x)).
uint32_t Log2(uint32_t x) 
{
  assert(x != 0);
  uint32_t L=31-CLZ(x);
  return ((uint32_t) 1 << L == x) ? L : L+1;
}

#define WriteUnsignedInteger( value ) pbs << (uint32_t)(value);
#define WriteInteger( value ) pbs << (int32_t)(value);
#define WriteCharacter( value ) pbs << (uint8_t)(value);
#define WriteDouble( value ) pbs << (double)(value);
#define WriteBit( value ) pbs << (bool)(value);
#define WriteBoolean( value ) pbs << (bool)(value);
#define WriteString( value ) pbs << (value);
#define SerializeContentPRCBase serializeContentPRCBase(pbs);
#define SerializeGraphics serializeGraphics(pbs);
#define SerializePRCBaseWithGraphics { serializeContentPRCBase(pbs); serializeGraphics(pbs); }
#define SerializeRepresentationItemContent serializeRepresentationItemContent(pbs);
#define SerializeRepresentationItem( value ) (value)->serializeRepresentationItem(pbs);
#define SerializeMarkup( value ) (value).serializeMarkup(pbs);
#define SerializeReferenceUniqueIdentifier( value ) (value).serializeReferenceUniqueIdentifier(pbs);
#define SerializeContentBaseTessData serializeContentBaseTessData(pbs);
#define SerializeTessFace( value ) (value)->serializeTessFace(pbs);
#define SerializeUserData UserData(0,0).write(pbs);
#define SerializeLineAttr( value ) pbs << (uint32_t)((value)+1);
#define SerializeVector3d( value ) (value).serializeVector3d(pbs);
#define SerializeVector2d( value ) (value).serializeVector2d(pbs);
#define SerializeName( value ) writeName(pbs, (value));
#define SerializeInterval( value )  (value).serializeInterval(pbs);
// #define SerializeBoundingBox( value )  (value).serializeBoundingBox(pbs);
#define SerializeDomain( value )  (value).serializeDomain(pbs);
#define SerializeParameterization  serializeParameterization(pbs);
#define SerializeUVParameterization  serializeUVParameterization(pbs);
#define SerializeTransformation  serializeTransformation(pbs);
#define SerializeBaseTopology  serializeBaseTopology(pbs);
#define SerializeBaseGeometry  serializeBaseGeometry(pbs);
#define SerializePtrCurve( value )    {WriteBoolean( false ); if((value)==NULL) pbs << (uint32_t)PRC_TYPE_ROOT; else (value)->serializeCurve(pbs);}
#define SerializePtrSurface( value )  {WriteBoolean( false ); if((value)==NULL) pbs << (uint32_t)PRC_TYPE_ROOT; else (value)->serializeSurface(pbs);}
#define SerializePtrTopology( value ) {WriteBoolean( false ); if((value)==NULL) pbs << (uint32_t)PRC_TYPE_ROOT; else (value)->serializeTopoItem(pbs);}
#define SerializeContentCurve  serializeContentCurve(pbs);
#define SerializeContentWireEdge  serializeContentWireEdge(pbs);
#define SerializeContentBody  serializeContentBody(pbs);
#define SerializeTopoContext  serializeTopoContext(pbs);
#define SerializeContextAndBodies( value )  (value).serializeContextAndBodies(pbs);
#define SerializeBody( value )  (value)->serializeBody(pbs);
#define ResetCurrentGraphics resetGraphics();
#define SerializeContentSurface  serializeContentSurface(pbs);
#define SerializeCompressedUniqueId( value ) (value).serializeCompressedUniqueId(pbs);
#define SerializeUnit( value ) (value).serializeUnit(pbs);
#define SerializeBoundingBox serializeBoundingBox(pbs);
#define SerializeAttributeEntry serializeAttributeEntry(pbs);
#define SerializeContentSingleAttribute( value ) (value).serializeSingleAttribute(pbs);
#define SerializeAttribute( value ) (value).serializeAttribute(pbs);
#define SerializeAttributeData serializeAttributes(pbs);
#define WriteUncompressedUnsignedInteger( value ) writeUncompressedUnsignedInteger(out, (uint32_t)(value));
#define SerializeFileStructureUncompressedUniqueId( value ) (value).serializeFileStructureUncompressedUniqueId(out);

void writeUncompressedUnsignedInteger(ostream &out, uint32_t data)
{
#ifdef WORDS_BIGENDIAN
  out.write(((char*)&data)+3,1);
  out.write(((char*)&data)+2,1);
  out.write(((char*)&data)+1,1);
  out.write(((char*)&data)+0,1);
#else
  out.write(((char*)&data)+0,1);
  out.write(((char*)&data)+1,1);
  out.write(((char*)&data)+2,1);
  out.write(((char*)&data)+3,1);
#endif
}

double PRCVector3d::Length()
{
  return sqrt(x*x+y*y+z*z);
}

bool PRCVector3d::Normalize()
{
 double fLength=Length();
 if(fLength < FLT_EPSILON) return false;
 double factor=1.0/fLength;
 x *= factor;
 y *= factor;
 z *= factor;
 return true;
}

double PRCVector2d::Length()
{
  return sqrt(x*x+y*y);
}

bool PRCVector2d::Normalize()
{
  double fLength=Length();
  if(fLength < FLT_EPSILON) return false;
  double factor=1.0/fLength;
  x *= factor;
  y *= factor;

  return true;
}

void PRCVector2d::serializeVector2d(PRCbitStream &pbs)
{
  WriteDouble (x)
  WriteDouble (y)
}

uint32_t makeCADID()
{
  static uint32_t ID = 1;
  return ID++;
}

uint32_t makePRCID()
{
  static uint32_t ID = 1;
  return ID++;
}

bool type_eligible_for_reference(uint32_t type)
{
  if(
     type == PRC_TYPE_MISC_EntityReference ||
     type == PRC_TYPE_MISC_MarkupLinkedItem ||
     type == PRC_TYPE_RI_BrepModel ||
     type == PRC_TYPE_RI_Curve ||
     type == PRC_TYPE_RI_Direction ||
     type == PRC_TYPE_RI_Plane ||
     type == PRC_TYPE_RI_PointSet ||
     type == PRC_TYPE_RI_PolyBrepModel ||
     type == PRC_TYPE_RI_PolyWire ||
     type == PRC_TYPE_RI_Set ||
     type == PRC_TYPE_RI_CoordinateSystem ||
     type == PRC_TYPE_ASM_ProductOccurence ||
     type == PRC_TYPE_ASM_PartDefinition ||
     type == PRC_TYPE_ASM_Filter ||
     type == PRC_TYPE_MKP_View ||
     type == PRC_TYPE_MKP_Markup ||
     type == PRC_TYPE_MKP_Leader ||
     type == PRC_TYPE_MKP_AnnotationItem ||
     type == PRC_TYPE_MKP_AnnotationSet ||
     type == PRC_TYPE_MKP_AnnotationReference ||
     type == PRC_TYPE_GRAPH_Style ||
     type == PRC_TYPE_GRAPH_Material ||
     type == PRC_TYPE_GRAPH_TextureApplication ||
     type == PRC_TYPE_GRAPH_TextureDefinition ||
     type == PRC_TYPE_GRAPH_LinePattern ||
     type == PRC_TYPE_GRAPH_DottingPattern ||
     type == PRC_TYPE_GRAPH_HatchingPattern ||
     type == PRC_TYPE_GRAPH_SolidPattern ||
     type == PRC_TYPE_GRAPH_VPicturePattern ||
     type == PRC_TYPE_GRAPH_AmbientLight ||
     type == PRC_TYPE_GRAPH_PointLight ||
     type == PRC_TYPE_GRAPH_DirectionalLight ||
     type == PRC_TYPE_GRAPH_SpotLight ||
     type == PRC_TYPE_GRAPH_SceneDisplayParameters ||
     type == PRC_TYPE_GRAPH_Camera
    )
    return true;
  else
    return false;
}

void UserData::write(PRCbitStream &pbs)
{
  pbs << size;
  if(size > 0) {
    uint32_t quot=size/8;
    uint32_t rem=size-8*quot;
    for(uint32_t i = 0; i < quot; ++i)
      pbs << data[i];
    for(uint32_t j = 0; j < rem; ++j) // 0-based, big endian bit counting
      pbs << (bool)((data[quot] & (0x80 >> j))!=0);
  }
}

void PRCAttributeEntry::serializeAttributeEntry(PRCbitStream &pbs) const
{
  WriteBoolean (title_is_integer) 
  if (title_is_integer)
    WriteUnsignedInteger (title_integer)
  else	
    WriteString (title_text)
}

void PRCSingleAttribute::serializeSingleAttribute(PRCbitStream &pbs) const
{
  SerializeAttributeEntry
  WriteUnsignedInteger (type)
  switch (type)
  {
    case KEPRCModellerAttributeTypeInt:
      WriteInteger (value.integer)
      break;
    case KEPRCModellerAttributeTypeReal:
      WriteDouble (value.real)
      break;
    case KEPRCModellerAttributeTypeTime:
      WriteUnsignedInteger (value.time)
      break;
    case KEPRCModellerAttributeTypeString:
      WriteString (value_text)
      break; 
    default:
      break;
  }
}

void PRCAttribute::serializeAttribute(PRCbitStream &pbs) const
{
  WriteUnsignedInteger (PRC_TYPE_MISC_Attribute) 
  
  SerializeAttributeEntry
  const uint32_t size_of_attribute_keys = attribute_keys.size();
  WriteUnsignedInteger (size_of_attribute_keys) 
  for(uint32_t i=0;i<size_of_attribute_keys;i++) 
    SerializeContentSingleAttribute (attribute_keys[i]) 
}

void PRCAttributes::serializeAttributes(PRCbitStream &pbs) const
{
  if (attributes.empty()) { // shortcut for most typical case
    const uint32_t number_of_attributes = 0;
    WriteUnsignedInteger (number_of_attributes) 
    return;
  }
  const uint32_t number_of_attributes = attributes.size();
  WriteUnsignedInteger (number_of_attributes) 
  for(PRCAttributeList::const_iterator it = attributes.begin(); it != attributes.end(); ++it)
  {
    SerializeAttribute(*it)
  }
}

void ContentPRCBase::serializeContentPRCBase(PRCbitStream &pbs) const
{
  SerializeAttributeData

  SerializeName (name)
  if (type_eligible_for_reference(type))
  {
    WriteUnsignedInteger (CAD_identifier)
    WriteUnsignedInteger (CAD_persistent_identifier)
    WriteUnsignedInteger (PRC_unique_identifier)
  }
}


bool IsCompressedType(uint32_t type)
{
  return (type == PRC_TYPE_TOPO_BrepDataCompress || type == PRC_TYPE_TOPO_SingleWireBodyCompress || type == PRC_TYPE_TESS_3D_Compressed);
}

void PRCReferenceUniqueIdentifier::serializeReferenceUniqueIdentifier(PRCbitStream &pbs)
{
  WriteUnsignedInteger (PRC_TYPE_MISC_ReferenceOnPRCBase)
  WriteUnsignedInteger (type)
  const bool reference_in_same_file_structure = true;
  WriteBoolean (reference_in_same_file_structure)
// if (!reference_in_same_file_structure)
//    SerializeCompressedUniqueId (target_file_structure)
  WriteUnsignedInteger (unique_identifier)
}

void PRCRgbColor::serializeRgbColor(PRCbitStream &pbs)
{
  WriteDouble (red)
  WriteDouble (green)
  WriteDouble (blue)
}

void PRCPicture::serializePicture(PRCbitStream &pbs)
{
  WriteUnsignedInteger (PRC_TYPE_GRAPH_Picture)
  SerializeContentPRCBase
  WriteInteger (format) //see Types for picture files
  WriteUnsignedInteger (uncompressed_file_index+1)
  WriteUnsignedInteger (pixel_width)
  WriteUnsignedInteger (pixel_height)
}

void PRCTextureDefinition::serializeTextureDefinition(PRCbitStream &pbs)
{
  uint32_t i=0; // universal index for PRC standart compatibility
  const uint8_t texture_dimension = 2;
  const uint32_t texture_mapping_attributes = texture_mapping_attribute;
  const uint32_t size_texture_mapping_attributes_intensities = 1;
  const double *texture_mapping_attributes_intensities = &texture_mapping_attribute_intensity;
  const uint32_t size_texture_mapping_attributes_components = 1;
  const uint8_t *texture_mapping_attributes_components = &texture_mapping_attribute_components;
  const EPRCTextureMappingType eMappingType = KEPRCTextureMappingType_Stored;
  
  const double red = 1.0;
  const double green = 1.0;
  const double blue = 1.0;
  const double alpha = 1.0;
  const EPRCTextureBlendParameter blend_src_rgb = KEPRCTextureBlendParameter_Unknown;
  const EPRCTextureBlendParameter blend_dst_rgb = KEPRCTextureBlendParameter_Unknown;
  const EPRCTextureBlendParameter blend_src_alpha = KEPRCTextureBlendParameter_Unknown;
  const EPRCTextureBlendParameter blend_dst_alpha = KEPRCTextureBlendParameter_Unknown;
  const EPRCTextureAlphaTest alpha_test = KEPRCTextureAlphaTest_Unknown;
  const double alpha_test_reference = 1.0;
  const EPRCTextureWrappingMode texture_wrapping_mode_R = KEPRCTextureWrappingMode_ClampToBorder;
  const bool texture_transformation = false;

  WriteUnsignedInteger (PRC_TYPE_GRAPH_TextureDefinition)

  SerializeContentPRCBase

  WriteUnsignedInteger (picture_index+1) 
  WriteCharacter (texture_dimension) 

  //   SerializeTextureMappingType
  WriteInteger (eMappingType) // Texture mapping type
  //  if (eMappingType == TEXTURE_MAPPING_OPERATOR)
  //  {
  //     WriteInteger (eMappingOperator) // Texture mapping operator
  //     WriteInteger (transformation)
  //     if (transformation)
  //        SerializeCartesianTransformation3d (transformation) 
  //  }

  WriteUnsignedInteger (texture_mapping_attributes) // Texture mapping attributes 
  WriteUnsignedInteger (size_texture_mapping_attributes_intensities)
  for (i=0;i<size_texture_mapping_attributes_intensities;i++)
     WriteDouble (texture_mapping_attributes_intensities[i])
  WriteUnsignedInteger (size_texture_mapping_attributes_components)
  for (i=0;i<size_texture_mapping_attributes_components;i++)
     WriteCharacter (texture_mapping_attributes_components[i]) 

  WriteInteger (texture_function)
  // reserved for future use; see Texture function 
  if (texture_function == KEPRCTextureFunction_Blend)
  {
     WriteDouble (red) // blend color component in the range [0.0,1.0]
     WriteDouble (green) // blend color component in the range [0.0,1.0]
     WriteDouble (blue) // blend color component in the range [0.0,1.0]
     WriteDouble (alpha) // blend color component in the range [0.0,1.0]
  }

  WriteInteger (blend_src_rgb) // Texture blend parameter 
  // reserved for future use; see Texture blend parameter 
  if (blend_src_rgb != KEPRCTextureBlendParameter_Unknown)
     WriteInteger (blend_dst_rgb) // Texture blend parameter 

  WriteInteger (blend_src_alpha) // Texture blend parameter 
  // reserved for future use; see Texture blend parameter 
  if (blend_src_alpha != KEPRCTextureBlendParameter_Unknown)
     WriteInteger (blend_dst_alpha) // Texture blend parameter 

  WriteCharacter (texture_applying_mode) // Texture applying mode 
  if (texture_applying_mode & PRC_TEXTURE_APPLYING_MODE_ALPHATEST)
  {
     WriteInteger (alpha_test) // Texture alpha test 
     WriteDouble (alpha_test_reference)
  }

  WriteInteger (texture_wrapping_mode_S) // Texture wrapping mode
  if (texture_dimension > 1)
     WriteInteger (texture_wrapping_mode_T) // Texture wrapping mode
  if (texture_dimension > 2 )
     WriteInteger (texture_wrapping_mode_R) // Texture wrapping mode

  WriteBit (texture_transformation)
//  if (texture_transformation)
//     SerializeTextureTransformation (texture_transformation)
}

void PRCMaterialGeneric::serializeMaterialGeneric(PRCbitStream &pbs)
{
  WriteUnsignedInteger (PRC_TYPE_GRAPH_Material)
  SerializeContentPRCBase
  WriteUnsignedInteger (ambient + 1)
  WriteUnsignedInteger (diffuse + 1)
  WriteUnsignedInteger (emissive + 1)
  WriteUnsignedInteger (specular + 1)
  WriteDouble (shininess)
  WriteDouble (ambient_alpha)
  WriteDouble (diffuse_alpha)
  WriteDouble (emissive_alpha)
  WriteDouble (specular_alpha)
}

void PRCTextureApplication::serializeTextureApplication(PRCbitStream &pbs) 
{
  WriteUnsignedInteger (PRC_TYPE_GRAPH_TextureApplication)
  SerializeContentPRCBase

  WriteUnsignedInteger (material_generic_index+1) 
  WriteUnsignedInteger (texture_definition_index+1) 
  WriteUnsignedInteger (next_texture_index+1) 
  WriteUnsignedInteger (UV_coordinates_index+1)
}

void PRCLinePattern::serializeLinePattern(PRCbitStream &pbs)
{
  uint32_t i = 0;
  WriteUnsignedInteger (PRC_TYPE_GRAPH_LinePattern)
  SerializeContentPRCBase
  
  const uint32_t size_lengths = lengths.size();
  WriteUnsignedInteger (size_lengths)
  for (i=0;i<size_lengths;i++)
    WriteDouble (lengths[i])
  WriteDouble (phase)
  WriteBoolean (is_real_length)
}


void PRCStyle::serializeCategory1LineStyle(PRCbitStream &pbs)
{
  const bool is_additional_1_defined = (additional!=0);
  const uint8_t additional_1 = additional;
  const bool is_additional_2_defined = false;
  const uint8_t additional_2 = 0;
  const bool is_additional_3_defined = false;
  const uint8_t additional_3 = 0;
  WriteUnsignedInteger (PRC_TYPE_GRAPH_Style)
  SerializeContentPRCBase
  WriteDouble (line_width)
  WriteBoolean (is_vpicture)
  WriteUnsignedInteger (line_pattern_vpicture_index + 1)
  WriteBoolean (is_material)
  WriteUnsignedInteger (color_material_index + 1)
  WriteBoolean (is_transparency_defined)
  if (is_transparency_defined)
     WriteCharacter (transparency)
  WriteBoolean (is_additional_1_defined)
  if (is_additional_1_defined)
     WriteCharacter (additional_1)
  WriteBoolean (is_additional_2_defined)
  if (is_additional_2_defined)
     WriteCharacter (additional_2)
  WriteBoolean (is_additional_3_defined)
  if (is_additional_3_defined)
     WriteCharacter (additional_3)
}

std::string currentName;

void writeName(PRCbitStream &pbs,const std::string &name)
{
  pbs << (name == currentName);
  if(name != currentName)
  {
    pbs << name;
    currentName = name;
  }
}

void resetName()
{
  currentName = "";
}

uint32_t current_layer_index = m1;
uint32_t current_index_of_line_style = m1;
uint16_t current_behaviour_bit_field = 1;

void writeGraphics(PRCbitStream &pbs,uint32_t l,uint32_t i,uint16_t b,bool force)
{
  if(force || current_layer_index != l || current_index_of_line_style != i || current_behaviour_bit_field != b)
  {
    pbs << false << (uint32_t)(l+1) << (uint32_t)(i+1)
        << (uint8_t)(b&0xFF) << (uint8_t)((b>>8)&0xFF);
    current_layer_index = l;
    current_index_of_line_style = i;
    current_behaviour_bit_field = b;
  }
  else
    pbs << true;
}

void writeGraphics(PRCbitStream &pbs,const PRCGraphics &graphics,bool force)
{
  if(force || current_layer_index != graphics.layer_index || current_index_of_line_style != graphics.index_of_line_style || current_behaviour_bit_field != graphics.behaviour_bit_field)
  {
    pbs << false
        << (uint32_t)(graphics.layer_index+1)
        << (uint32_t)(graphics.index_of_line_style+1)
        << (uint8_t)(graphics.behaviour_bit_field&0xFF)
        << (uint8_t)((graphics.behaviour_bit_field>>8)&0xFF);
    current_layer_index = graphics.layer_index;
    current_index_of_line_style = graphics.index_of_line_style;
    current_behaviour_bit_field = graphics.behaviour_bit_field;
  }
  else
    pbs << true;
}

void PRCGraphics::serializeGraphics(PRCbitStream &pbs)
{
  if(current_layer_index != this->layer_index || current_index_of_line_style != this->index_of_line_style || current_behaviour_bit_field != this->behaviour_bit_field)
  {
    pbs << false
        << (uint32_t)(this->layer_index+1)
        << (uint32_t)(this->index_of_line_style+1)
        << (uint8_t)(this->behaviour_bit_field&0xFF)
        << (uint8_t)((this->behaviour_bit_field>>8)&0xFF);
    current_layer_index = this->layer_index;
    current_index_of_line_style = this->index_of_line_style;
    current_behaviour_bit_field = this->behaviour_bit_field;
  }
  else
    pbs << true;
}

void PRCGraphics::serializeGraphicsForced(PRCbitStream &pbs)
{
  pbs << false
      << (uint32_t)(this->layer_index+1)
      << (uint32_t)(this->index_of_line_style+1)
      << (uint8_t)(this->behaviour_bit_field&0xFF)
      << (uint8_t)((this->behaviour_bit_field>>8)&0xFF);
  current_layer_index = this->layer_index;
  current_index_of_line_style = this->index_of_line_style;
  current_behaviour_bit_field = this->behaviour_bit_field;
}

void resetGraphics()
{
  current_layer_index = m1;
  current_index_of_line_style = m1;
  current_behaviour_bit_field = 1;
}

void resetGraphicsAndName()
{
  resetGraphics(); resetName();
}

void  PRCMarkup::serializeMarkup(PRCbitStream &pbs)
{
  WriteUnsignedInteger (PRC_TYPE_MKP_Markup)
  SerializeContentPRCBase 
  SerializeGraphics
  WriteUnsignedInteger (type) 
  WriteUnsignedInteger (sub_type) 
  const uint32_t number_of_linked_items = 0;
  WriteUnsignedInteger (number_of_linked_items) 
//  for (i=0;i<number_of_linked_items;i++) 
//     SerializeReferenceUniqueIdentifier (linked_items[i])
  const uint32_t number_of_leaders = 0;
  WriteUnsignedInteger (number_of_leaders) 
//  for (i=0;i<number_of_leaders;i++) 
//     SerializeReferenceUniqueIdentifier (leaders[i])
  WriteUnsignedInteger (index_tessellation + 1) 
  SerializeUserData
}

void  PRCAnnotationItem::serializeAnnotationItem(PRCbitStream &pbs)
{
// group___tf_annotation_item_____serialize2.html
// group___tf_annotation_item_____serialize_content2.html
// group___tf_annotation_entity_____serialize_content2.html
  WriteUnsignedInteger (PRC_TYPE_MKP_AnnotationItem)
  SerializeContentPRCBase 
  SerializeGraphics
  SerializeReferenceUniqueIdentifier (markup)
  SerializeUserData
}

void  PRCRepresentationItemContent::serializeRepresentationItemContent(PRCbitStream &pbs)
{
  SerializeContentPRCBase 
  SerializeGraphics
  WriteUnsignedInteger (index_local_coordinate_system + 1)
  WriteUnsignedInteger (index_tessellation + 1)
}

void  PRCBrepModel::serializeBrepModel(PRCbitStream &pbs)
{
   WriteUnsignedInteger (PRC_TYPE_RI_BrepModel)

   SerializeRepresentationItemContent 
   WriteBit (has_brep_data)
   if (has_brep_data)
   {
      WriteUnsignedInteger (context_id+1)
      WriteUnsignedInteger (body_id+1)
   }
   WriteBoolean (is_closed)
   SerializeUserData
}

void  PRCPolyBrepModel::serializePolyBrepModel(PRCbitStream &pbs)
{
   WriteUnsignedInteger (PRC_TYPE_RI_PolyBrepModel)

   SerializeRepresentationItemContent 
   WriteBoolean (is_closed)
   SerializeUserData
}

void  PRCPointSet::serializePointSet(PRCbitStream &pbs)
{
  WriteUnsignedInteger (PRC_TYPE_RI_PointSet)

  SerializeRepresentationItemContent 

  const uint32_t number_of_points = point.size();
  WriteUnsignedInteger (number_of_points)
  for (uint32_t i=0;i<number_of_points;i++)
  {
     SerializeVector3d (point[i])
  }
  SerializeUserData
}

void  PRCSet::serializeSet(PRCbitStream &pbs)
{
  WriteUnsignedInteger (PRC_TYPE_RI_Set)

  SerializeRepresentationItemContent 

  const uint32_t number_of_elements = elements.size();
  WriteUnsignedInteger (number_of_elements)
  for (uint32_t i=0;i<number_of_elements;i++)
  {
    SerializeRepresentationItem (elements[i])
  }
  SerializeUserData
}

uint32_t PRCSet::addBrepModel(PRCBrepModel*& pBrepModel)
{
  elements.push_back(pBrepModel);
  pBrepModel = NULL;
  return elements.size()-1;
}

uint32_t PRCSet::addPolyBrepModel(PRCPolyBrepModel*& pPolyBrepModel)
{
  elements.push_back(pPolyBrepModel);
  pPolyBrepModel = NULL;
  return elements.size()-1;
}

uint32_t PRCSet::addPointSet(PRCPointSet*& pPointSet)
{
  elements.push_back(pPointSet);
  pPointSet  = NULL;
  return elements.size()-1;
}

uint32_t PRCSet::addSet(PRCSet*& pSet)
{
  elements.push_back(pSet);
  pSet = NULL;
  return elements.size()-1;
}

uint32_t PRCSet::addWire(PRCWire*& pWire)
{
  elements.push_back(pWire);
  pWire = NULL;
  return elements.size()-1;
}

uint32_t PRCSet::addPolyWire(PRCPolyWire*& pPolyWire)
{
  elements.push_back(pPolyWire);
  pPolyWire = NULL;
  return elements.size()-1;
}

uint32_t PRCSet::addRepresentationItem(PRCRepresentationItem*& pRepresentationItem)
{
  elements.push_back(pRepresentationItem);
  pRepresentationItem = NULL;
  return elements.size()-1;
}

void  PRCWire::serializeWire(PRCbitStream &pbs)
{
   WriteUnsignedInteger (PRC_TYPE_RI_Curve)

   SerializeRepresentationItemContent 
   WriteBit (has_wire_body)
   if (has_wire_body)
   {
      WriteUnsignedInteger (context_id+1)
      WriteUnsignedInteger (body_id+1)
   }

   SerializeUserData
}

void  PRCPolyWire::serializePolyWire(PRCbitStream &pbs)
{
  WriteUnsignedInteger (PRC_TYPE_RI_PolyWire)

  SerializeRepresentationItemContent 
  SerializeUserData
}

void  PRCGeneralTransformation3d::serializeGeneralTransformation3d(PRCbitStream &pbs) const
{
  WriteUnsignedInteger (PRC_TYPE_MISC_GeneralTransformation)
  // Like Fortran, PRC uses transposed (column-major) format!
  for (int j=0;j<4;j++)
    for (int i=0;i<4;i++)
     WriteDouble(mat[i][j]); 
}

void  PRCCartesianTransformation3d::serializeCartesianTransformation3d(PRCbitStream &pbs) const
{
  WriteUnsignedInteger (PRC_TYPE_MISC_CartesianTransformation)
  WriteCharacter ( behaviour )
  if (behaviour & PRC_TRANSFORMATION_Translate)
     SerializeVector3d ( origin )
  if (behaviour & PRC_TRANSFORMATION_NonOrtho)
  {
     SerializeVector3d ( X )
     SerializeVector3d ( Y )
     SerializeVector3d ( Z )
  }
  else if (behaviour & PRC_TRANSFORMATION_Rotate)
  {
     SerializeVector3d ( X )
     SerializeVector3d ( Y )
  }

  if (behaviour & PRC_TRANSFORMATION_NonUniformScale)
  {
        SerializeVector3d ( scale )
  }
  else if (behaviour & PRC_TRANSFORMATION_Scale)
  {
        WriteDouble ( uniform_scale )
  }

  if (behaviour & PRC_TRANSFORMATION_Homogeneous)
  {
     WriteDouble ( X_homogeneous_coord )
     WriteDouble ( Y_homogeneous_coord )
     WriteDouble ( Z_homogeneous_coord )
     WriteDouble ( origin_homogeneous_coord )
  }
}

void  PRCTransformation::serializeTransformation(PRCbitStream &pbs)
{ 
   WriteBit ( has_transformation )
   if (has_transformation)
   {
      WriteCharacter ( behaviour )
      if ( geometry_is_2D )
      {
         if (behaviour & PRC_TRANSFORMATION_Translate)
            SerializeVector2d ( origin )
         if (behaviour & PRC_TRANSFORMATION_Rotate)
         {
            SerializeVector2d ( x_axis )
            SerializeVector2d ( y_axis )
         }
         if (behaviour & PRC_TRANSFORMATION_Scale)
            WriteDouble ( scale )
      }
      else
      {
         if (behaviour & PRC_TRANSFORMATION_Translate)
            SerializeVector3d ( origin )
         if (behaviour & PRC_TRANSFORMATION_Rotate)
         {
            SerializeVector3d ( x_axis )
            SerializeVector3d ( y_axis )
         }
         if (behaviour & PRC_TRANSFORMATION_Scale)
            WriteDouble ( scale )
      }
   }
}
void  PRCCoordinateSystem::serializeCoordinateSystem(PRCbitStream &pbs)
{
  WriteUnsignedInteger (PRC_TYPE_RI_CoordinateSystem)

  SerializeRepresentationItemContent 
  axis_set->serializeTransformation3d(pbs);
  SerializeUserData
}

void  PRCFontKeysSameFont::serializeFontKeysSameFont(PRCbitStream &pbs)
{
  uint32_t i=0; // universal index for PRC standart compatibility
  WriteString (font_name)
  WriteUnsignedInteger (char_set)
  const uint32_t number_of_font_keys = font_keys.size();
  WriteUnsignedInteger (number_of_font_keys)
  for (i=0;i<number_of_font_keys;i++)
  {
     WriteUnsignedInteger (font_keys[i].font_size + 1)
     WriteCharacter (font_keys[i].attributes)
  }
}

void SerializeArrayRGBA (const std::vector<uint8_t> &rgba_vertices,const bool is_rgba, PRCbitStream &pbs)
{
  uint32_t i = 0;
  uint32_t j = 0;
// number_by_vector can be assigned a value of 3 (RGB) or 4 (RGBA).
// number_of_vectors is equal to number_of_colors / number_by_vector.
  const uint32_t number_by_vector=is_rgba?4:3;
  const std::vector<uint8_t> &vector_color = rgba_vertices;
  const uint32_t number_of_colors=vector_color.size();
  const uint32_t number_of_vectors=number_of_colors / number_by_vector;
  // first one 
  for (i=0;i<number_by_vector;i++)
     WriteCharacter (vector_color[i])
  
  for (i=1;i<number_of_vectors;i++)
  {
     bool b_same = true;
     for (j=0;j<number_by_vector;j++)
     {
        if ((vector_color[i*number_by_vector+j] - vector_color[(i-1)*number_by_vector+j]) != 0)
        {
           b_same = false;
           break;
        }
     }
     WriteBoolean (b_same)
     if (!b_same)
     {
        for (j=0;j<number_by_vector;j++)
           WriteCharacter (vector_color[i*number_by_vector+j])
     }
  }
}

void  PRCTessFace::serializeTessFace(PRCbitStream &pbs)
{
  uint32_t i=0; // universal index for PRC standart compatibility
  WriteUnsignedInteger (PRC_TYPE_TESS_Face)

  const uint32_t size_of_line_attributes=line_attributes.size();
  WriteUnsignedInteger (size_of_line_attributes) 
  for (i=0;i<size_of_line_attributes;i++) 
     SerializeLineAttr (line_attributes[i])

  WriteUnsignedInteger (start_wire) 
  const uint32_t size_of_sizes_wire=sizes_wire.size();
  WriteUnsignedInteger (size_of_sizes_wire) 
  for (i=0;i<size_of_sizes_wire;i++) 
     WriteUnsignedInteger (sizes_wire[i])

  WriteUnsignedInteger (used_entities_flag) 

  WriteUnsignedInteger (start_triangulated) 
  const uint32_t size_of_sizes_triangulated=sizes_triangulated.size();
  WriteUnsignedInteger (size_of_sizes_triangulated) 
  for (i=0;i<size_of_sizes_triangulated;i++) 
     WriteUnsignedInteger (sizes_triangulated[i])

  if(number_of_texture_coordinate_indexes==0 &&
     used_entities_flag &
     (
      PRC_FACETESSDATA_PolyfaceTextured|
      PRC_FACETESSDATA_TriangleTextured|
      PRC_FACETESSDATA_TriangleFanTextured|
      PRC_FACETESSDATA_TriangleStripeTextured|
      PRC_FACETESSDATA_PolyfaceOneNormalTextured|
      PRC_FACETESSDATA_TriangleOneNormalTextured|
      PRC_FACETESSDATA_TriangleFanOneNormalTextured|
      PRC_FACETESSDATA_TriangleStripeOneNormalTextured
     ))
    WriteUnsignedInteger (1)  // workaround for error of not setting number_of_texture_coordinate_indexes
  else 
    WriteUnsignedInteger (number_of_texture_coordinate_indexes)

  const bool has_vertex_colors = !rgba_vertices.empty();
  WriteBoolean (has_vertex_colors)
  if (has_vertex_colors)
  {
     WriteBoolean (is_rgba)
     const bool b_optimised=false;
     WriteBoolean (b_optimised)
     if (!b_optimised)
     {
       SerializeArrayRGBA (rgba_vertices, is_rgba, pbs);
     }
     else
     {
     // not described
     }
  }
  if (size_of_line_attributes) 
     WriteUnsignedInteger (behaviour)
}

void  PRCContentBaseTessData::serializeContentBaseTessData(PRCbitStream &pbs)
{
  uint32_t i=0; // universal index for PRC standart compatibility
  WriteBoolean (is_calculated)
  const uint32_t number_of_coordinates = coordinates.size();
  WriteUnsignedInteger (number_of_coordinates)
  for (i=0;i<number_of_coordinates;i++)
     WriteDouble (coordinates[i])
}

void  PRC3DTess::serialize3DTess(PRCbitStream &pbs)
{
  uint32_t i=0; // universal index for PRC standart compatibility
  WriteUnsignedInteger (PRC_TYPE_TESS_3D)
  SerializeContentBaseTessData 
  WriteBoolean (has_faces)
  WriteBoolean (has_loops)
  const bool must_recalculate_normals=normal_coordinate.empty();
  WriteBoolean (must_recalculate_normals)
  if (must_recalculate_normals)
  {
     const uint8_t normals_recalculation_flags=0;
     // not used; should be zero
     WriteCharacter (normals_recalculation_flags)
     // definition similar to VRML
     WriteDouble (crease_angle)
  }
  
  const uint32_t number_of_normal_coordinates=normal_coordinate.size();
  WriteUnsignedInteger (number_of_normal_coordinates)
  for (i=0;i<number_of_normal_coordinates;i++)
     WriteDouble (normal_coordinate[i])
  
  const uint32_t number_of_wire_indices=wire_index.size();
  WriteUnsignedInteger (number_of_wire_indices)
  for (i=0;i<number_of_wire_indices;i++)
     WriteUnsignedInteger (wire_index[i])
  
  // note : those can be single triangles, triangle fans or stripes
  const uint32_t number_of_triangulated_indices=triangulated_index.size();
  WriteUnsignedInteger (number_of_triangulated_indices)
  for (i=0;i<number_of_triangulated_indices;i++)
     WriteUnsignedInteger (triangulated_index[i])
  
  const uint32_t number_of_face_tessellation=face_tessellation.size();
  WriteUnsignedInteger (number_of_face_tessellation)
  for (i=0;i<number_of_face_tessellation;i++)
     SerializeTessFace (face_tessellation[i])
  
  const uint32_t number_of_texture_coordinates=texture_coordinate.size();
  WriteUnsignedInteger (number_of_texture_coordinates)
  for (i=0;i<number_of_texture_coordinates;i++)
     WriteDouble (texture_coordinate[i])
}

void PRC3DTess::addTessFace(PRCTessFace*& pTessFace)
{
  face_tessellation.push_back(pTessFace);
  pTessFace = NULL;
}

void  PRC3DWireTess::serialize3DWireTess(PRCbitStream &pbs)
{
// group___tf3_d_wire_tess_data_____serialize2.html
// group___tf3_d_wire_tess_data_____serialize_content2.html
  uint32_t i=0; // universal index for PRC standart compatibility
  WriteUnsignedInteger (PRC_TYPE_TESS_3D_Wire)
  SerializeContentBaseTessData 
  const uint32_t number_of_wire_indexes=wire_indexes.size();
  WriteUnsignedInteger (number_of_wire_indexes)
  for (i=0;i<number_of_wire_indexes;i++)
     WriteUnsignedInteger (wire_indexes[i])
  
  const bool has_vertex_colors = !rgba_vertices.empty();
  WriteBoolean (has_vertex_colors)
  if (has_vertex_colors)
  {
     WriteBoolean (is_rgba)
     WriteBoolean (is_segment_color)
     const bool b_optimised=false;
     WriteBoolean (b_optimised)
     if (!b_optimised)
     {
       SerializeArrayRGBA (rgba_vertices, is_rgba, pbs);
     }
     else
     {
     // not described
     }
  }
}

void  PRCMarkupTess::serializeMarkupTess(PRCbitStream &pbs)
{
// group___tf_markup_tess_data_____serialize2.html
// group___tf_markup_tess_data_____serialize_content2.html
  uint32_t i=0; // universal index for PRC standart compatibility
  WriteUnsignedInteger (PRC_TYPE_TESS_Markup)
  SerializeContentBaseTessData 

  const uint32_t number_of_codes=codes.size();
  WriteUnsignedInteger (number_of_codes)
  for (i=0;i<number_of_codes;i++)
     WriteUnsignedInteger (codes[i])
  const uint32_t number_of_texts=texts.size();
  WriteUnsignedInteger (number_of_texts)
  for (i=0;i<number_of_texts;i++)
     WriteString (texts[i])
  WriteString (label) // label of tessellation
  WriteCharacter (behaviour)
}

void writeUnit(PRCbitStream &out,bool fromCAD,double unit)
{
  out << fromCAD << unit;
}

void writeEmptyMarkups(PRCbitStream &out)
{
  out << (uint32_t)0 // # of linked items
      << (uint32_t)0 // # of leaders
      << (uint32_t)0 // # of markups
      << (uint32_t)0; // # of annotation entities
}

void PRCBaseTopology::serializeBaseTopology(PRCbitStream &pbs)
{
   WriteBoolean (base_information)
   if (base_information)
   {
      SerializeAttributeData
      SerializeName (name)
      WriteUnsignedInteger (identifier)
   }
}

void PRCBaseGeometry::serializeBaseGeometry(PRCbitStream &pbs)
{
   WriteBoolean (base_information)
   if (base_information)
   {
      SerializeAttributeData
      SerializeName (name)
      WriteUnsignedInteger (identifier)
   }
}

void PRCContentBody::serializeContentBody(PRCbitStream &pbs)
{
   SerializeBaseTopology
   WriteCharacter ( behavior )
}

void PRCBoundingBox::serializeBoundingBox(PRCbitStream &pbs)
{
   SerializeVector3d ( min )
   SerializeVector3d ( max )
}

void PRCDomain::serializeDomain(PRCbitStream &pbs)
{
   SerializeVector2d ( min )
   SerializeVector2d ( max )
}

void PRCInterval::serializeInterval(PRCbitStream &pbs)
{
   WriteDouble ( min )
   WriteDouble ( max )
}

void PRCParameterization::serializeParameterization(PRCbitStream &pbs)
{
   SerializeInterval ( interval )
   WriteDouble ( parameterization_coeff_a )
   WriteDouble ( parameterization_coeff_b )
}

void PRCUVParameterization::serializeUVParameterization(PRCbitStream &pbs)
{
   WriteBoolean ( swap_uv )
   SerializeDomain ( uv_domain )
   WriteDouble ( parameterization_on_u_coeff_a )
   WriteDouble ( parameterization_on_v_coeff_a )
   WriteDouble ( parameterization_on_u_coeff_b )
   WriteDouble ( parameterization_on_v_coeff_b )
}

void PRCContentSurface::serializeContentSurface(PRCbitStream &pbs)
{
   SerializeBaseGeometry
   WriteUnsignedInteger ( extend_info )
}

void  PRCNURBSSurface::serializeNURBSSurface(PRCbitStream &pbs)
{ 
   uint32_t i=0;
//  uint32_t i=0, j=0;
   WriteUnsignedInteger (PRC_TYPE_SURF_NURBS) 

   SerializeContentSurface 
   WriteBoolean ( is_rational )
   WriteUnsignedInteger ( degree_in_u )
   WriteUnsignedInteger ( degree_in_v )
   const uint32_t highest_index_of_knots_in_u = knot_u.size()-1;
   const uint32_t highest_index_of_knots_in_v = knot_v.size()-1;
   const uint32_t highest_index_of_control_point_in_u = highest_index_of_knots_in_u - degree_in_u - 1;
   const uint32_t highest_index_of_control_point_in_v = highest_index_of_knots_in_v - degree_in_v - 1;
   WriteUnsignedInteger ( highest_index_of_control_point_in_u )
   WriteUnsignedInteger ( highest_index_of_control_point_in_v )
   WriteUnsignedInteger ( highest_index_of_knots_in_u )
   WriteUnsignedInteger ( highest_index_of_knots_in_v )
   for (i=0; i < (highest_index_of_control_point_in_u+1)*(highest_index_of_control_point_in_v+1); i++)
   {
      WriteDouble ( control_point[i].x )
      WriteDouble ( control_point[i].y )
      WriteDouble ( control_point[i].z )
      if (is_rational)
         WriteDouble ( control_point[i].w )
   }
//  for (i=0; i<=highest_index_of_control_point_in_u; i++)
//  {
//     for (j=0; j<=highest_index_of_control_point_in_v; j++)
//     {
//        WriteDouble ( control_point[i*(highest_index_of_control_point_in_u+1)+j].x )
//        WriteDouble ( control_point[i*(highest_index_of_control_point_in_u+1)+j].y )
//        WriteDouble ( control_point[i*(highest_index_of_control_point_in_u+1)+j].z )
//        if (is_rational)
//           WriteDouble ( control_point[i*(highest_index_of_control_point_in_u+1)+j].w )
//     }
//  }
   for (i=0; i<=highest_index_of_knots_in_u; i++)
      WriteDouble ( knot_u[i] )
   for (i=0; i<=highest_index_of_knots_in_v; i++)
      WriteDouble ( knot_v[i] )
   WriteUnsignedInteger ( knot_type )
   WriteUnsignedInteger ( surface_form )
}

void writeUnsignedIntegerWithVariableBitNumber(PRCbitStream &pbs, uint32_t value, uint32_t bit_number)
{
   uint32_t i;
   for(i=0; i<bit_number; i++)
   {
      if( value >= 1u<<(bit_number - 1 - i) )
      {
         WriteBoolean (true)
      
         value -= 1u<<(bit_number - 1 - i);
      }
      else
      {
         WriteBoolean (false)
      }
   }
}
#define WriteUnsignedIntegerWithVariableBitNumber( value, bit_number )  writeUnsignedIntegerWithVariableBitNumber( pbs, (value), (bit_number) );

void writeIntegerWithVariableBitNumber(PRCbitStream &pbs, int32_t iValue, uint32_t uBitNumber)
{ 
  WriteBoolean(iValue<0);
  WriteUnsignedIntegerWithVariableBitNumber(abs(iValue), uBitNumber - 1);
}
#define WriteIntegerWithVariableBitNumber( value, bit_number )  writeIntegerWithVariableBitNumber( pbs, (value), (bit_number) );

void writeDoubleWithVariableBitNumber(PRCbitStream &pbs, double dValue,double dTolerance, unsigned uBitNumber)
{
// calling functions must ensure no overflow
  int32_t iTempValue = (int32_t) ( dValue / dTolerance );
  WriteIntegerWithVariableBitNumber(iTempValue, uBitNumber);
}
#define WriteDoubleWithVariableBitNumber( value, bit_number )  writeDoubleWithVariableBitNumber( pbs, (value), (bit_number) );

uint32_t  GetNumberOfBitsUsedToStoreUnsignedInteger(uint32_t uValue)
{
  uint32_t uNbBit=2;
  uint32_t uTemp = 2;
  while(uValue >= uTemp)
  {
    uTemp*=2;
    uNbBit++;
  }
  return uNbBit-1;
}

void  writeNumberOfBitsThenUnsignedInteger(PRCbitStream &pbs, uint32_t unsigned_integer)
{
   uint32_t number_of_bits = GetNumberOfBitsUsedToStoreUnsignedInteger( unsigned_integer );
   WriteUnsignedIntegerWithVariableBitNumber ( number_of_bits, 5 )
   WriteUnsignedIntegerWithVariableBitNumber ( unsigned_integer, number_of_bits )
}
#define WriteNumberOfBitsThenUnsignedInteger( value ) writeNumberOfBitsThenUnsignedInteger( pbs, value );

uint32_t  GetNumberOfBitsUsedToStoreInteger(int32_t iValue)
{
   return GetNumberOfBitsUsedToStoreUnsignedInteger(abs(iValue))+1;
}

int32_t intdiv(double dValue, double dTolerance)
{
  double ratio=fabs(dValue)/dTolerance;
  assert(ratio <= INT_MAX);
  int32_t iTempValue=(int32_t) ratio;
  if(ratio - iTempValue >= 0.5) iTempValue++;
  if(dValue < 0) 
    return -iTempValue;
  else
    return iTempValue;
}

// round dValue to nearest multiple of dTolerance
double roundto(double dValue, double dTolerance)
{
    return intdiv(dValue, dTolerance) * dTolerance;
}

PRCVector3d roundto(PRCVector3d vec, double dTolerance)
{
    PRCVector3d res;
    res.x = roundto(vec.x,dTolerance);
    res.y = roundto(vec.y,dTolerance);
    res.z = roundto(vec.z,dTolerance);
    return    res;
}

uint32_t  GetNumberOfBitsUsedToStoreDouble(double dValue, double dTolerance )
{
   return GetNumberOfBitsUsedToStoreInteger(intdiv(dValue,dTolerance));
}

struct itriple
{
  int32_t x;
  int32_t y;
  int32_t z;
};

uint32_t  GetNumberOfBitsUsedToStoreTripleInteger(const itriple &iTriple)
{
   const uint32_t x_bits = GetNumberOfBitsUsedToStoreInteger(iTriple.x);
   const uint32_t y_bits = GetNumberOfBitsUsedToStoreInteger(iTriple.y);
   const uint32_t z_bits = GetNumberOfBitsUsedToStoreInteger(iTriple.z);
   uint32_t bits = x_bits;
   if(y_bits > bits)
     bits = y_bits;
   if(z_bits > bits)
     bits = z_bits;
   return bits;
}

itriple iroundto(PRCVector3d vec, double dTolerance)
{
    itriple res;
    res.x = intdiv(vec.x, dTolerance);
    res.y = intdiv(vec.y, dTolerance);
    res.z = intdiv(vec.z, dTolerance);
    return    res;
}

void  PRCCompressedFace::serializeCompressedFace(PRCbitStream &pbs, double brep_data_compressed_tolerance)
{
   serializeCompressedAnaNurbs( pbs, brep_data_compressed_tolerance );
}
#define SerializeCompressedFace( value ) (value)->serializeCompressedFace( pbs, brep_data_compressed_tolerance );

void  PRCCompressedFace::serializeContentCompressedFace(PRCbitStream &pbs)
{
   WriteBoolean ( orientation_surface_with_shell )
   const bool surface_is_trimmed = false;
   WriteBoolean ( surface_is_trimmed )
}

void  PRCCompressedFace::serializeCompressedAnaNurbs(PRCbitStream &pbs, double brep_data_compressed_tolerance)
{
   // WriteCompressedEntityType ( PRC_HCG_AnaNurbs )
   const bool is_a_curve = false;
   WriteBoolean ( is_a_curve ) 
   WriteUnsignedIntegerWithVariableBitNumber (13 , 4)
   serializeContentCompressedFace( pbs );
   serializeCompressedNurbs( pbs, brep_data_compressed_tolerance );
}

void  PRCCompressedFace::serializeCompressedNurbs(PRCbitStream &pbs, double brep_data_compressed_tolerance)
{
   const double nurbs_tolerance = 0.2*brep_data_compressed_tolerance;
   const uint32_t degree_in_u = degree;
   const uint32_t degree_in_v = degree;
   
   WriteUnsignedIntegerWithVariableBitNumber ( degree_in_u, 5)
   WriteUnsignedIntegerWithVariableBitNumber ( degree_in_v, 5)

   const uint32_t number_of_knots_in_u = 4; // 0011 or 00001111 knot vector - just 2 spans
   WriteUnsignedIntegerWithVariableBitNumber (number_of_knots_in_u - 2, 16)
   uint32_t number_bit = degree_in_u ? Log2( degree_in_u + 2 ) : 2;
   WriteBoolean (false) // Multiplicity_is_already_stored - no
   WriteUnsignedIntegerWithVariableBitNumber( degree_in_u+1,number_bit)
   WriteBoolean (true) // Multiplicity_is_already_stored - yes
   const uint32_t number_of_knots_in_v = 4; // 0011 or 00001111 knot vector - just 2 spans
   WriteUnsignedIntegerWithVariableBitNumber (number_of_knots_in_v - 2, 16)
   number_bit = degree_in_v ? Log2( degree_in_v + 2 ) : 2;
   WriteBoolean (false) // Multiplicity_is_already_stored - no
   WriteUnsignedIntegerWithVariableBitNumber( degree_in_v+1,number_bit)
   WriteBoolean (true) // Multiplicity_is_already_stored - yes
  
   const bool is_closed_u = false; 
   WriteBoolean ( is_closed_u )
   const bool is_closed_v = false; 
   WriteBoolean ( is_closed_v )

   const uint32_t number_of_control_point_in_u = degree_in_u + 1;
   const uint32_t number_of_control_point_in_v = degree_in_v + 1;

#if defined(__GNUC__) && !defined(__clang__)
   PRCVector3d P[number_of_control_point_in_u][number_of_control_point_in_v];
#else
   vector<vector<PRCVector3d> > P(number_of_control_point_in_u, vector<PRCVector3d>(number_of_control_point_in_v));
#endif
   for(uint32_t i=0;i<number_of_control_point_in_u;i++)
   for(uint32_t j=0;j<number_of_control_point_in_v;j++)
      P[i][j] = control_point[i*number_of_control_point_in_v+j];
#ifdef __GNUC__
   itriple compressed_control_point[number_of_control_point_in_u][number_of_control_point_in_v];
   uint32_t control_point_type[number_of_control_point_in_u][number_of_control_point_in_v];
#else
   vector<vector<itriple> > compressed_control_point(number_of_control_point_in_u, vector<itriple>(number_of_control_point_in_v));
   vector<vector<uint32_t> > control_point_type(number_of_control_point_in_u, vector<uint32_t>(number_of_control_point_in_v));
#endif

   uint32_t number_of_bits_for_isomin = 1;
   uint32_t number_of_bits_for_rest = 1;
   
   for(uint32_t j = 1; j < number_of_control_point_in_v; j++)
   {
      compressed_control_point[0][j] = iroundto(P[0][j]-P[0][j-1], nurbs_tolerance );
      P[0][j] = P[0][j-1] + roundto(P[0][j]-P[0][j-1], nurbs_tolerance);
      uint32_t bit_size = GetNumberOfBitsUsedToStoreTripleInteger(compressed_control_point[0][j]);
      if (bit_size > number_of_bits_for_isomin)
        number_of_bits_for_isomin = bit_size;
   }

   for(uint32_t i = 1; i < number_of_control_point_in_u; i++)
   {
      compressed_control_point[i][0] = iroundto(P[i][0]-P[i-1][0], nurbs_tolerance );
      P[i][0] = P[i-1][0] + roundto(P[i][0]-P[i-1][0], nurbs_tolerance);
      uint32_t bit_size = GetNumberOfBitsUsedToStoreTripleInteger(compressed_control_point[i][0]);
      if (bit_size > number_of_bits_for_isomin)
        number_of_bits_for_isomin = bit_size;
   }

   for(uint32_t i=1;i<number_of_control_point_in_u;i++)
   for(uint32_t j=1;j<number_of_control_point_in_v;j++)
   {
     compressed_control_point[i][j].x = 0;
     compressed_control_point[i][j].y = 0;
     compressed_control_point[i][j].z = 0;
     
     PRCVector3d V = P[i-1][j] - P[i-1][j-1];
     PRCVector3d U = P[i][j-1] - P[i-1][j-1];
     PRCVector3d Pc = P[i][j] - (P[i-1][j-1] + U + V);

     if(Pc.Length() < nurbs_tolerance)
     {
       control_point_type[i][j] = 0;
       P[i][j] = P[i-1][j-1] + U + V;
     }
     else 
     {
       PRCVector3d N = U*V;
       PRCVector3d Ue = U;
       PRCVector3d Ne = N;
       if( V.Length() < FLT_EPSILON || !Ue.Normalize() || !Ne.Normalize())
       {
          control_point_type[i][j] = 3;
       // Pc = roundto(Pc, nurbs_tolerance); // not sure if this rounding really happens, need to experiment, docs imply but do not state
          compressed_control_point[i][j] = iroundto(Pc, nurbs_tolerance);
          P[i][j] = P[i-1][j-1] + U + V + roundto(Pc, nurbs_tolerance); // see above
       }
       else
       {
         PRCVector3d NUe = Ne*Ue;
         double x = Pc.Dot(Ue);
         double y = Pc.Dot(NUe);
         double z = Pc.Dot(Ne);

         if(x*x+y*y<nurbs_tolerance*nurbs_tolerance)
         {
           control_point_type[i][j] = 1;
           compressed_control_point[i][j] = iroundto(PRCVector3d(0.0,0.0,z), nurbs_tolerance);
           P[i][j] = P[i-1][j-1] + U + V + roundto(z, nurbs_tolerance)*Ne; // see above
         }
         else
         {
           if(fabs(z)<nurbs_tolerance/2)
           {
             control_point_type[i][j] = 2;
             compressed_control_point[i][j] = iroundto(PRCVector3d(x,y,0), nurbs_tolerance);
             P[i][j] = P[i-1][j-1] + U + V + roundto(x, nurbs_tolerance)*Ue + roundto(y, nurbs_tolerance)*NUe; // see above
           }
           else
           {
             control_point_type[i][j] = 3;
             compressed_control_point[i][j] = iroundto(Pc, nurbs_tolerance);
             P[i][j] = P[i-1][j-1] + U + V + roundto(Pc, nurbs_tolerance); // see above
           }
         }
       }
     }
     uint32_t bit_size = GetNumberOfBitsUsedToStoreTripleInteger(compressed_control_point[i][j]);
     if (bit_size > number_of_bits_for_rest)
       number_of_bits_for_rest = bit_size;
   }

   if( number_of_bits_for_rest == 2 ) number_of_bits_for_rest--; // really I think it must be unconditional, but so it seems to be done in Adobe Acrobat (9.3)
   WriteUnsignedIntegerWithVariableBitNumber ( number_of_bits_for_isomin, 20 )
   WriteUnsignedIntegerWithVariableBitNumber ( number_of_bits_for_rest,   20 )
   WriteDouble ( P[0][0].x )
   WriteDouble ( P[0][0].y )
   WriteDouble ( P[0][0].z )
   
   for(uint32_t j = 1; j < number_of_control_point_in_v; j++)
   {
      WriteIntegerWithVariableBitNumber(compressed_control_point[0][j].x, number_of_bits_for_isomin+1)
      WriteIntegerWithVariableBitNumber(compressed_control_point[0][j].y, number_of_bits_for_isomin+1)
      WriteIntegerWithVariableBitNumber(compressed_control_point[0][j].z, number_of_bits_for_isomin+1)
   }
   
   for(uint32_t i = 1; i < number_of_control_point_in_u; i++)
   {
      WriteIntegerWithVariableBitNumber(compressed_control_point[i][0].x, number_of_bits_for_isomin+1)
      WriteIntegerWithVariableBitNumber(compressed_control_point[i][0].y, number_of_bits_for_isomin+1)
      WriteIntegerWithVariableBitNumber(compressed_control_point[i][0].z, number_of_bits_for_isomin+1)
   }
   
   for(uint32_t i = 1; i < number_of_control_point_in_u; i++)
   {
      for(uint32_t j = 1; j < number_of_control_point_in_v; j++)
      {
         WriteUnsignedIntegerWithVariableBitNumber ( control_point_type[i][j], 2 )
         
         if(control_point_type[i][j] == 1)
         {
            WriteIntegerWithVariableBitNumber ( compressed_control_point[i][j].z, number_of_bits_for_rest+1 )
         }
         else if(control_point_type[i][j] == 2)
         {
            WriteIntegerWithVariableBitNumber ( compressed_control_point[i][j].x, number_of_bits_for_rest+1 )
            WriteIntegerWithVariableBitNumber ( compressed_control_point[i][j].y, number_of_bits_for_rest+1 )
         }
         else if(control_point_type[i][j] == 3)
         {
            WriteIntegerWithVariableBitNumber ( compressed_control_point[i][j].x, number_of_bits_for_rest+1 )
            WriteIntegerWithVariableBitNumber ( compressed_control_point[i][j].y, number_of_bits_for_rest+1 )
            WriteIntegerWithVariableBitNumber ( compressed_control_point[i][j].z, number_of_bits_for_rest+1 )
         }
      }
   }
   
   const uint32_t type_param_u = 0;
   WriteBoolean( type_param_u == 0 )
   const uint32_t type_param_v = 0;
   WriteBoolean( type_param_v == 0 )
   const bool is_rational = false;
   WriteBoolean( is_rational )
}

void PRCCompressedBrepData::serializeCompressedShell(PRCbitStream &pbs)
{
   uint32_t i;
   const uint32_t number_of_face = face.size();
   WriteBoolean ( number_of_face == 1 )

   if( number_of_face != 1 )
      WriteNumberOfBitsThenUnsignedInteger (number_of_face)
   
   for( i=0; i < number_of_face; i++)
         SerializeCompressedFace ( face[i] )
   
   const bool is_an_iso_face = false;
   for( i=0; i < number_of_face; i++)
      WriteBoolean ( is_an_iso_face )
}

void PRCCompressedBrepData::serializeCompressedBrepData(PRCbitStream &pbs)
{
   WriteUnsignedInteger ( PRC_TYPE_TOPO_BrepDataCompress )
   SerializeContentBody
   
   WriteDouble ( brep_data_compressed_tolerance )
   const uint32_t number_of_bits_to_store_reference = 1;
   WriteNumberOfBitsThenUnsignedInteger ( number_of_bits_to_store_reference )
   const uint32_t number_vertex_iso = 0;
   WriteUnsignedIntegerWithVariableBitNumber ( number_vertex_iso, number_of_bits_to_store_reference )
   const uint32_t number_edge_iso = 0;
   WriteUnsignedIntegerWithVariableBitNumber ( number_edge_iso, number_of_bits_to_store_reference )
   
   const uint32_t number_of_shell = 1;
   const uint32_t number_of_connex = 1;
   WriteBoolean ( number_of_shell == 1 && number_of_connex == 1 )
   serializeCompressedShell( pbs );

   uint32_t i;
   const uint32_t number_of_faces = face.size();
   for(i=0; i< number_of_faces; i++)
      face[i]->serializeBaseTopology( pbs );
}

void  PRCBlend01::serializeBlend01(PRCbitStream &pbs)
{ 
   WriteUnsignedInteger (PRC_TYPE_SURF_Blend01) 

   SerializeContentSurface
   SerializeTransformation
   SerializeUVParameterization
   SerializePtrCurve ( center_curve ) 
   SerializePtrCurve ( origin_curve ) 
   SerializePtrCurve ( tangent_curve ) 
}

void  PRCRuled::serializeRuled(PRCbitStream &pbs)
{ 
   WriteUnsignedInteger (PRC_TYPE_SURF_Ruled) 

   SerializeContentSurface
   SerializeTransformation
   SerializeUVParameterization
   SerializePtrCurve ( first_curve ) 
   SerializePtrCurve ( second_curve ) 
}

void  PRCSphere::serializeSphere(PRCbitStream &pbs)
{ 
   WriteUnsignedInteger (PRC_TYPE_SURF_Sphere) 

   SerializeContentSurface
   SerializeTransformation
   SerializeUVParameterization
   WriteDouble ( radius )
}

void  PRCCone::serializeCone(PRCbitStream &pbs)
{ 
   WriteUnsignedInteger (PRC_TYPE_SURF_Cone) 

   SerializeContentSurface
   SerializeTransformation
   SerializeUVParameterization
   WriteDouble ( bottom_radius )
   WriteDouble ( semi_angle )
}

void  PRCCylinder::serializeCylinder(PRCbitStream &pbs)
{ 
   WriteUnsignedInteger (PRC_TYPE_SURF_Cylinder) 

   SerializeContentSurface
   SerializeTransformation
   SerializeUVParameterization
   WriteDouble ( radius )
}

void  PRCTorus::serializeTorus(PRCbitStream &pbs)
{ 
   WriteUnsignedInteger (PRC_TYPE_SURF_Torus) 

   SerializeContentSurface
   SerializeTransformation
   SerializeUVParameterization
   WriteDouble ( major_radius )
   WriteDouble ( minor_radius )
}

void PRCFace::serializeFace(PRCbitStream &pbs)
{ 
   uint32_t i = 0;
   WriteUnsignedInteger (PRC_TYPE_TOPO_Face) 

   SerializeBaseTopology
   SerializePtrSurface ( base_surface )
   WriteBit ( have_surface_trim_domain )
   if ( have_surface_trim_domain )
      SerializeDomain ( surface_trim_domain ) 
   WriteBit ( have_tolerance )
   if ( have_tolerance )
      WriteDouble ( tolerance ) 
   WriteUnsignedInteger ( number_of_loop )
   WriteInteger ( outer_loop_index )
   for (i=0;i<number_of_loop;i++)
   {
//    SerializePtrTopology ( loop[i] )
   }
}

void PRCShell::serializeShell(PRCbitStream &pbs)
{ 
   uint32_t i = 0;
   WriteUnsignedInteger (PRC_TYPE_TOPO_Shell) 

   SerializeBaseTopology
   WriteBoolean ( shell_is_closed )
   uint32_t number_of_face = face.size();
   WriteUnsignedInteger ( number_of_face ) 
   for (i=0;i<number_of_face;i++)
   {
      SerializePtrTopology ( face[i] )
      WriteCharacter ( orientation_surface_with_shell[i] )
   }
}

void PRCShell::addFace(PRCFace*& pFace, uint8_t orientation)
{
  face.push_back(pFace);
  pFace = NULL;
  orientation_surface_with_shell.push_back(orientation);
}

void PRCConnex::serializeConnex(PRCbitStream &pbs)
{ 
   uint32_t i = 0;
   WriteUnsignedInteger (PRC_TYPE_TOPO_Connex) 

   SerializeBaseTopology
   uint32_t number_of_shell = shell.size();
   WriteUnsignedInteger ( number_of_shell ) 
   for (i=0;i<number_of_shell;i++)
   {
      SerializePtrTopology ( shell[i] )
   }
}

void PRCConnex::addShell(PRCShell*& pShell)
{
  shell.push_back(pShell);
  pShell = NULL;
}

#define have_bbox( behavior ) (behavior!=0)
void PRCBrepData::serializeBrepData(PRCbitStream &pbs)
{
   uint32_t i = 0;
   WriteUnsignedInteger ( PRC_TYPE_TOPO_BrepData) 

   SerializeContentBody 
   uint32_t number_of_connex = connex.size();
   WriteUnsignedInteger ( number_of_connex ) 
   for ( i=0; i<number_of_connex; i++)
   {
      SerializePtrTopology ( connex[i] )
   }
   if ( have_bbox(behavior) )
      SerializeBoundingBox
}
#undef have_bbox

void PRCBrepData::addConnex(PRCConnex*& pConnex)
{
  connex.push_back(pConnex);
  pConnex = NULL;
}

void PRCContentWireEdge::serializeContentWireEdge(PRCbitStream &pbs)
{
   SerializeBaseTopology
   SerializePtrCurve ( curve_3d )
   WriteBit ( has_curve_trim_interval )
   if ( has_curve_trim_interval )
      SerializeInterval ( curve_trim_interval )
}

void PRCWireEdge::serializeWireEdge(PRCbitStream &pbs)
{
   WriteUnsignedInteger (PRC_TYPE_TOPO_WireEdge) 
   SerializeContentWireEdge 
}

void PRCContentCurve::serializeContentCurve(PRCbitStream &pbs)
{
   SerializeBaseGeometry
   WriteUnsignedInteger ( extend_info )
   WriteBoolean ( is_3d )
}

void  PRCNURBSCurve::serializeNURBSCurve(PRCbitStream &pbs)
{ 
   uint32_t i=0;
   WriteUnsignedInteger (PRC_TYPE_CRV_NURBS) 

   SerializeContentCurve 
   WriteBoolean ( is_rational )
   WriteUnsignedInteger ( degree )
   uint32_t highest_index_of_control_point = control_point.size()-1;
   uint32_t highest_index_of_knots = knot.size()-1;
   WriteUnsignedInteger ( highest_index_of_control_point )
   WriteUnsignedInteger ( highest_index_of_knots )
   for (i=0; i<=highest_index_of_control_point; i++)
   {
      WriteDouble ( control_point[i].x )
      WriteDouble ( control_point[i].y )
      if (is_3d)
         WriteDouble ( control_point[i].z )
      if (is_rational)
         WriteDouble ( control_point[i].w )
   }
   for (i=0; i<=highest_index_of_knots; i++)
      WriteDouble ( knot[i] )
   WriteUnsignedInteger ( knot_type )
   WriteUnsignedInteger ( curve_form )
}

void  PRCPolyLine::serializePolyLine(PRCbitStream &pbs)
{ 
   uint32_t i=0;
   WriteUnsignedInteger (PRC_TYPE_CRV_PolyLine) 

   SerializeContentCurve 
   SerializeTransformation
   SerializeParameterization
   uint32_t number_of_point = point.size();
   WriteUnsignedInteger ( number_of_point ) 
   for (i=0; i<number_of_point; i++) 
   {
      if (is_3d)
         SerializeVector3d ( point[i] )
      else
         SerializeVector2d ( point[i] )
   }
}

void  PRCCircle::serializeCircle(PRCbitStream &pbs)
{ 
   WriteUnsignedInteger (PRC_TYPE_CRV_Circle) 

   SerializeContentCurve 
   SerializeTransformation
   SerializeParameterization
   WriteDouble ( radius )
}

void  PRCComposite::serializeComposite(PRCbitStream &pbs)
{ 
   uint32_t i=0;
   WriteUnsignedInteger (PRC_TYPE_CRV_Composite) 

   SerializeContentCurve 
   SerializeTransformation
   SerializeParameterization
   uint32_t number_of_curves = base_curve.size();
   WriteUnsignedInteger ( number_of_curves ) 
   for (i=0; i<number_of_curves; i++) 
   {
      SerializePtrCurve ( base_curve[i] )
      WriteBoolean ( base_sense[i] )
   }
   WriteBoolean ( is_closed )
}

void PRCTopoContext::serializeTopoContext(PRCbitStream &pbs)
{ 
   WriteUnsignedInteger (PRC_TYPE_TOPO_Context) 

   SerializeContentPRCBase
   WriteCharacter ( behaviour )
   WriteDouble ( granularity )
   WriteDouble ( tolerance )
   WriteBoolean ( have_smallest_face_thickness )
   if ( have_smallest_face_thickness )
      WriteDouble ( smallest_thickness )
   WriteBoolean ( have_scale )
   if ( have_scale )
      WriteDouble ( scale )
}

void PRCTopoContext::serializeContextAndBodies(PRCbitStream &pbs)
{ 
   uint32_t i=0;
   SerializeTopoContext 
   uint32_t number_of_bodies = body.size();
   WriteUnsignedInteger (number_of_bodies) 
   for (i=0;i<number_of_bodies;i++) 
      SerializeBody (body[i]) 
}

void PRCTopoContext::serializeGeometrySummary(PRCbitStream &pbs)
{ 
   uint32_t i=0;
   uint32_t number_of_bodies = body.size();
   WriteUnsignedInteger (number_of_bodies) 
   for (i=0;i<number_of_bodies;i++) 
   {
      WriteUnsignedInteger ( body[i]->serialType() ) 
      if ( IsCompressedType(body[i]->serialType()) )
      {
         WriteDouble ( body[i]->serialTolerance() ) 
      }
   }
}

void PRCTopoContext::serializeContextGraphics(PRCbitStream &pbs)
{ 
   uint32_t i=0, j=0, k=0, l=0;
   ResetCurrentGraphics
   uint32_t number_of_body = body.size();
   PRCGraphicsList element;
   bool has_graphics = false;
   for (i=0;i<number_of_body;i++)
   {
        if ( body[i]->topo_item_type == PRC_TYPE_TOPO_BrepData && dynamic_cast<PRCBrepData*>(body[i]))
        {
                PRCBrepData *body_i = dynamic_cast<PRCBrepData*>(body[i]);
                for (j=0;j<body_i->connex.size();j++)
                {
                        for(k=0;k<body_i->connex[j]->shell.size();k++)
                        {
                                for( l=0;l<body_i->connex[j]->shell[k]->face.size();l++)
                                {
                                        element.push_back( body_i->connex[j]->shell[k]->face[l] );
                                        has_graphics = has_graphics || body_i->connex[j]->shell[k]->face[l]->has_graphics();
                                }
                        }
                }
        }
        else if ( body[i]->topo_item_type == PRC_TYPE_TOPO_BrepDataCompress && dynamic_cast<PRCCompressedBrepData*>(body[i]))
        {
                PRCCompressedBrepData *body_i = dynamic_cast<PRCCompressedBrepData*>(body[i]);
             	for( l=0;l<body_i->face.size();l++)
             	{
             		element.push_back( body_i->face[l] );
             		has_graphics = has_graphics || body_i->face[l]->has_graphics();
             	}
        }
   }
   uint32_t number_of_treat_type = 0;
   if (has_graphics && !element.empty())
     number_of_treat_type = 1;
   WriteUnsignedInteger (number_of_treat_type) 
   for (i=0;i<number_of_treat_type;i++) 
   {
      const uint32_t element_type = PRC_TYPE_TOPO_Face;
      WriteUnsignedInteger (element_type) 
      const uint32_t number_of_element = element.size();
      WriteUnsignedInteger (number_of_element) 
      for (j=0;j<number_of_element;j++) 
      {
         WriteBoolean ( element[j]->has_graphics() ) 
         if (element[j]->has_graphics()) 
         {
            element[j]->serializeGraphics(pbs);
         }
      }
   }
}

uint32_t PRCTopoContext::addSingleWireBody(PRCSingleWireBody*& pSingleWireBody)
{
  body.push_back(pSingleWireBody);
  pSingleWireBody = NULL;
  return body.size()-1;
}

uint32_t PRCTopoContext::addBrepData(PRCBrepData*& pBrepData)
{
  body.push_back(pBrepData);
  pBrepData = NULL;
  return body.size()-1;
}

uint32_t PRCTopoContext::addCompressedBrepData(PRCCompressedBrepData*& pCompressedBrepData)
{
  body.push_back(pCompressedBrepData);
  pCompressedBrepData = NULL;
  return body.size()-1;
}

void PRCSingleWireBody::serializeSingleWireBody(PRCbitStream &pbs)
{
  WriteUnsignedInteger ( PRC_TYPE_TOPO_SingleWireBody) 

  SerializeContentBody 
  SerializePtrTopology ( wire_edge )
}

void PRCUniqueId::serializeCompressedUniqueId(PRCbitStream &pbs) const
{
   WriteUnsignedInteger (id0) 
   WriteUnsignedInteger (id1) 
   WriteUnsignedInteger (id2) 
   WriteUnsignedInteger (id3) 	
}

void PRCUniqueId::serializeFileStructureUncompressedUniqueId(std::ostream& out) const
{
   WriteUncompressedUnsignedInteger (id0) 
   WriteUncompressedUnsignedInteger (id1) 
   WriteUncompressedUnsignedInteger (id2) 
   WriteUncompressedUnsignedInteger (id3) 
}

void PRCUnit::serializeUnit(PRCbitStream &pbs)
{
   WriteBoolean (unit_from_CAD_file)
   WriteDouble (unit)
}

void PRCProductOccurrence::serializeProductOccurrence(PRCbitStream &pbs)
{
   WriteUnsignedInteger ( PRC_TYPE_ASM_ProductOccurence ) 

   SerializePRCBaseWithGraphics 

// SerializeReferencesOfProductOccurrence 
   WriteUnsignedInteger (index_part+1)
   WriteUnsignedInteger (index_prototype+1)
   if (index_prototype != m1)
   {
      WriteBoolean (prototype_in_same_file_structure)
      if (!prototype_in_same_file_structure)
         SerializeCompressedUniqueId (prototype_file_structure)
   }
   WriteUnsignedInteger(index_external_data+1)
   if (index_external_data != m1)
   {
      WriteBoolean (external_data_in_same_file_structure)
      if (!external_data_in_same_file_structure)
         SerializeCompressedUniqueId (external_data_file_structure)
   }
   const uint32_t number_of_son_product_occurrences = index_son_occurrence.size();
   WriteUnsignedInteger (number_of_son_product_occurrences)
   for (uint32_t i=0;i<number_of_son_product_occurrences;i++)
      WriteUnsignedInteger (index_son_occurrence[i])

   WriteCharacter (product_behaviour)

// SerializeProductInformation (product_information)
   SerializeUnit (unit_information)
   WriteCharacter (product_information_flags)
   WriteInteger (product_load_status)

   const bool has_location = location != NULL;
   WriteBit (has_location)
   if (has_location)
     location->serializeTransformation3d (pbs);
   
   WriteUnsignedInteger (0) // number_of_references
   
// SerializeMarkups (markups)
   WriteUnsignedInteger (0) // number_of_linked_items 
   WriteUnsignedInteger (0) // number_of_leaders 
   WriteUnsignedInteger (0) // number_of_markups 
   WriteUnsignedInteger (0) // number_of_annotation_entities 

   
   WriteUnsignedInteger (0) // number_of_views
   WriteBit (false) // has_entity_filter
   WriteUnsignedInteger (0) // number_of_display_filters
   WriteUnsignedInteger (0) // number_of_scene_display_parameters

   SerializeUserData
}

uint32_t PRCPartDefinition::addBrepModel(PRCBrepModel*& pBrepModel)
{
  representation_item.push_back(pBrepModel);
  pBrepModel = NULL;
  return representation_item.size()-1;
}

uint32_t PRCPartDefinition::addPolyBrepModel(PRCPolyBrepModel*& pPolyBrepModel)
{
  representation_item.push_back(pPolyBrepModel);
  pPolyBrepModel = NULL;
  return representation_item.size()-1;
}

uint32_t PRCPartDefinition::addPointSet(PRCPointSet*& pPointSet)
{
  representation_item.push_back(pPointSet);
  pPointSet = NULL;
  return representation_item.size()-1;
}

uint32_t PRCPartDefinition::addSet(PRCSet*& pSet)
{
  representation_item.push_back(pSet);
  pSet = NULL;
  return representation_item.size()-1;
}

uint32_t PRCPartDefinition::addWire(PRCWire*& pWire)
{
  representation_item.push_back(pWire);
  pWire = NULL;
  return representation_item.size()-1;
}

uint32_t PRCPartDefinition::addPolyWire(PRCPolyWire*& pPolyWire)
{
  representation_item.push_back(pPolyWire);
  pPolyWire = NULL;
  return representation_item.size()-1;
}

uint32_t PRCPartDefinition::addRepresentationItem(PRCRepresentationItem*& pRepresentationItem)
{
  representation_item.push_back(pRepresentationItem);
  pRepresentationItem = NULL;
  return representation_item.size()-1;
}

void PRCPartDefinition::serializePartDefinition(PRCbitStream &pbs)
{
  WriteUnsignedInteger ( PRC_TYPE_ASM_PartDefinition ) 
	
  SerializePRCBaseWithGraphics
  SerializeBoundingBox

  uint32_t number_of_representation_items = representation_item.size();
  WriteUnsignedInteger (number_of_representation_items)
  for (uint32_t i=0;i<number_of_representation_items;i++)
    SerializeRepresentationItem (representation_item[i])
	
  // SerializeMarkups (markups)
  WriteUnsignedInteger (0) // number_of_linked_items 
  WriteUnsignedInteger (0) // number_of_leaders 
  WriteUnsignedInteger (0) // number_of_markups 
  WriteUnsignedInteger (0) // number_of_annotation_entities 

  WriteUnsignedInteger (0) // number_of_views
  SerializeUserData
}	
