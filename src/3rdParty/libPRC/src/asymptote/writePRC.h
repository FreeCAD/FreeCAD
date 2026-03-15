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

#ifndef __WRITE_PRC_H
#define __WRITE_PRC_H
#include <string>
#include <vector>
#include <deque>
#include <list>
#if defined(__GNUC__) && !defined(__clang__)
#include <ext/slist>
#endif
#include <map>
#include <iostream>
#include "PRCbitStream.h"
#include "PRC.h"
#include <float.h>
#include <math.h>

static const uint32_t m1=(uint32_t)-1;
static const double pi=acos(-1.0);

class PRCVector3d
{
public :
 double x;
 double y;
 double z;
 PRCVector3d() :
 x(0), y(0), z(0) {}
 PRCVector3d(double fx, double fy, double fz) :
 x(fx), y(fy), z(fz) {}
 PRCVector3d(const double c[], double fx=0, double fy=0, double fz=0) :
 x(c?c[0]:fx), y(c?c[1]:fy), z(c?c[2]:fz) {}
 PRCVector3d(const PRCVector3d& sVector3d) :
 x(sVector3d.x), y(sVector3d.y), z(sVector3d.z) {}

 void Set(double fx, double fy, double fz)
 { x = fx; y = fy; z = fz; }
 double Dot(const PRCVector3d & sPt) const
 { return(x*sPt.x)+(y*sPt.y)+(z*sPt.z); }
 double LengthSquared()
 { return(x*x+y*y+z*z); }

 friend PRCVector3d operator + (const PRCVector3d& a, const PRCVector3d& b)
 { return PRCVector3d(a.x+b.x,a.y+b.y,a.z+b.z); }
 friend PRCVector3d operator - (const PRCVector3d& a)
 { return PRCVector3d(-a.x,-a.y,-a.z); }
 friend PRCVector3d operator - (const PRCVector3d& a, const PRCVector3d& b)
 { return PRCVector3d(a.x-b.x,a.y-b.y,a.z-b.z); }
 friend PRCVector3d operator * (const PRCVector3d& a, const double d)
 { return PRCVector3d(a.x*d,a.y*d,a.z*d); }
 friend PRCVector3d operator * (const double d, const PRCVector3d& a)
 { return PRCVector3d(a.x*d,a.y*d,a.z*d); }
 friend PRCVector3d operator / (const PRCVector3d& a, const double d)
 { return PRCVector3d(a.x/d,a.y/d,a.z/d); }
 friend PRCVector3d operator * (const PRCVector3d& a, const PRCVector3d& b)
 { return PRCVector3d((a.y*b.z)-(a.z*b.y), (a.z*b.x)-(a.x*b.z), (a.x*b.y)-(a.y*b.x)); }

 void write(PRCbitStream &out) { out << x << y << z; }
 void serializeVector3d(PRCbitStream &pbs) const { pbs << x << y << z; }
 void serializeVector2d(PRCbitStream &pbs) const { pbs << x << y; }

 double Length();
 bool Normalize();

 bool operator==(const PRCVector3d &v) const
 {
  return x==v.x && y==v.y && z==v.z;
 }
 bool operator!=(const PRCVector3d &v) const
 {
  return !(x==v.x && y==v.y && z==v.z);
 }
 bool operator<(const PRCVector3d &v) const
 {
   if(x!=v.x)
     return (x<v.x);
   if(y!=v.y)
     return (y<v.y);
   return (z<v.z);
 }
 friend std::ostream& operator << (std::ostream& out, const PRCVector3d& v)
 {
   out << "(" << v.x << "," << v.y << "," << v.z << ")";
   return out;
 }
};
/*
class UUID
{
  public:
    UUID(uint32_t u0, uint32_t u1, uint32_t u2, uint32_t u3) :
      id0(u0),id1(u1),id2(u2),id3(u3) {}
    void write(PRCbitStream &out)
    {
      out << id0 << id1 << id2 << id3;
    }
  private:
    uint32_t id0,id1,id2,id3;
}; */

void writeUncompressedUnsignedInteger(std::ostream &out, uint32_t data);

void writeUnit(PRCbitStream &,bool,double);

void writeEmptyMarkups(PRCbitStream&);

class UserData
{
  public:
    UserData(uint32_t s = 0, uint8_t* d = 0) : size(s),data(d) {}
    void write(PRCbitStream&);
  private:
    uint32_t size;
    uint8_t* data;
};

struct PRCAttributeEntry
{
	PRCAttributeEntry() : title_is_integer(false) {}
	PRCAttributeEntry(uint32_t integer) : title_is_integer(true)
  {
    title_integer = integer;
  }
	PRCAttributeEntry(const std::string &text) : title_is_integer(false)
  {
    title_text = text;
  }
  void serializeAttributeEntry(PRCbitStream&) const;
  bool title_is_integer;
  std::string title_text;
  uint32_t title_integer;
};

class PRCSingleAttribute : public PRCAttributeEntry
{
  public:
  PRCSingleAttribute() : type(KEPRCModellerAttributeTypeNull) {}
	PRCSingleAttribute(int32_t integer) : PRCAttributeEntry(), type(KEPRCModellerAttributeTypeInt)
  {
    value.integer = integer;
  }
	PRCSingleAttribute(double real) : PRCAttributeEntry(), type(KEPRCModellerAttributeTypeReal)
  {
    value.real = real;
  }
  PRCSingleAttribute(uint32_t time) : PRCAttributeEntry(), type(KEPRCModellerAttributeTypeTime)
  {
    value.time = time;
  }  
	PRCSingleAttribute(const std::string &text) : PRCAttributeEntry(), type(KEPRCModellerAttributeTypeString)
  {
    value_text = text;}
	PRCSingleAttribute(uint32_t title, int32_t integer) : PRCAttributeEntry(title), type(KEPRCModellerAttributeTypeInt)
  {
    value.integer = integer;
  }
	PRCSingleAttribute(uint32_t title, double real) : PRCAttributeEntry(title), type(KEPRCModellerAttributeTypeReal)
  {
    value.real = real;
  }
  PRCSingleAttribute(uint32_t title, uint32_t time) : PRCAttributeEntry(title), type(KEPRCModellerAttributeTypeTime)
  {
    value.time = time;
  }  
	PRCSingleAttribute(uint32_t title, const std::string &text) : PRCAttributeEntry(title), type(KEPRCModellerAttributeTypeString)
  {
    value_text = text;
  }
	PRCSingleAttribute(const std::string title, int32_t integer) : PRCAttributeEntry(title), type(KEPRCModellerAttributeTypeInt)
  {
    value.integer = integer;
  }
	PRCSingleAttribute(const std::string title, double real) : PRCAttributeEntry(title), type(KEPRCModellerAttributeTypeReal)
  {
    value.real = real;
  }
  PRCSingleAttribute(const std::string title, uint32_t time) : PRCAttributeEntry(title), type(KEPRCModellerAttributeTypeTime)
  {
    value.time = time;
  }  
	PRCSingleAttribute(const std::string title, const std::string &text) : PRCAttributeEntry(title), type(KEPRCModellerAttributeTypeString)
  {
    value_text = text;
  }
  void serializeSingleAttribute(PRCbitStream&) const;
  EPRCModellerAttributeType  type;
  union PRCSingleAttributeData
  {
    int32_t integer;
    double real;
    uint32_t time;
  } value;
  std::string value_text;
};

class PRCAttribute : public PRCAttributeEntry
{
  public:
  PRCAttribute() : PRCAttributeEntry() {}
	PRCAttribute(uint32_t title) : PRCAttributeEntry(title) {}
	PRCAttribute(const std::string title) : PRCAttributeEntry(title) {}
  void serializeAttribute(PRCbitStream &) const;
  PRCSingleAttribute& newKey() { attribute_keys.resize(attribute_keys.size()+1); return attribute_keys.back(); }
  void addKey(const PRCSingleAttribute &key) { attribute_keys.push_back(key); }
  std::deque<PRCSingleAttribute> attribute_keys;
};
#if defined(__GNUC__) && !defined(__clang__)
typedef __gnu_cxx::slist<PRCAttribute> PRCAttributeList;
#else
typedef std::list<PRCAttribute> PRCAttributeList;
#endif

class PRCAttributes
{
  public:
  void serializeAttributes(PRCbitStream&) const;
  PRCAttribute& newAttribute() { attributes.push_front(PRCAttribute()); return attributes.front(); }
  void addAttribute(const PRCAttribute &attribute) { attributes.push_front(attribute); }
  PRCAttributeList attributes;
};

bool type_eligible_for_reference(uint32_t type);
uint32_t makeCADID();
uint32_t makePRCID();

class ContentPRCBase : public PRCAttributes
{
  public:
  ContentPRCBase(uint32_t t, std::string n="") :
    type(t),name(n),CAD_identifier(0), CAD_persistent_identifier(0), PRC_unique_identifier(0)
  {
    if(type_eligible_for_reference(type))
    {
      CAD_identifier = makeCADID();
      PRC_unique_identifier = makePRCID();
    }
  }
  void serializeContentPRCBase(PRCbitStream&) const;
  uint32_t getPRCID() const { return PRC_unique_identifier; }
  uint32_t getType() const { return type; }
  uint32_t type;
  std::string name;
  uint32_t CAD_identifier, CAD_persistent_identifier, PRC_unique_identifier;
};

class PRCReferenceUniqueIdentifier
{
public:
  PRCReferenceUniqueIdentifier() :
    type(0), unique_identifier(m1) {}
  void serializeReferenceUniqueIdentifier(PRCbitStream&);
  uint32_t type;
// bool reference_in_same_file_structure;
// PRCUniqueId target_file_structure;
  uint32_t unique_identifier;
};

extern std::string currentName;
void writeName(PRCbitStream&,const std::string&);
void resetName();

extern uint32_t current_layer_index;
extern uint32_t current_index_of_line_style;
extern uint16_t current_behaviour_bit_field;

void writeGraphics(PRCbitStream&,uint32_t=m1,uint32_t=m1,uint16_t=1,bool=false);
void resetGraphics();

void resetGraphicsAndName();

struct PRCRgbColor
{
  PRCRgbColor(double r=0.0, double g=0.0, double b=0.0) :
    red(r), green(g), blue(b) {}
  double red,green,blue;
  void serializeRgbColor(PRCbitStream&);

  bool operator==(const PRCRgbColor &c) const
  {
    return (red==c.red && green==c.green && blue==c.blue);
  }
  bool operator!=(const PRCRgbColor &c) const
  {
    return !(red==c.red && green==c.green && blue==c.blue);
  }
  bool operator<(const PRCRgbColor &c) const
  {
    if(red!=c.red)
      return (red<c.red);
    if(green!=c.green)
      return (green<c.green);
    return (blue<c.blue);
  }
};

class PRCPicture : public ContentPRCBase
{
public:
  PRCPicture(std::string n="") :
  ContentPRCBase(PRC_TYPE_GRAPH_Picture,n), format(KEPRCPicture_PNG), uncompressed_file_index(m1), pixel_width(0), pixel_height(0) {}
  void serializePicture(PRCbitStream&);
  EPRCPictureDataFormat format;
  uint32_t uncompressed_file_index;
  uint32_t pixel_width;
  uint32_t pixel_height;
};

struct PRCVector2d
{
  PRCVector2d() :
  x(0.0), y(0.0) {}
  PRCVector2d(double X, double Y) :
  x(X), y(Y) {}
  void serializeVector2d(PRCbitStream&);
  double x;
  double y;
  PRCVector2d(const double c[], double fx=0, double fy=0) :
  x(c?c[0]:fx), y(c?c[1]:fy) {}
  PRCVector2d(const PRCVector2d& sVector2d) :
  x(sVector2d.x), y(sVector2d.y) {}
  
  void Set(double fx, double fy)
  { x = fx; y = fy; }
  double Dot(const PRCVector2d & sPt) const
  { return(x*sPt.x)+(y*sPt.y); }
  double LengthSquared()
  { return(x*x+y*y); }
  
  friend PRCVector2d operator + (const PRCVector2d& a, const PRCVector2d& b)
  { return PRCVector2d(a.x+b.x,a.y+b.y); }
  friend PRCVector2d operator - (const PRCVector2d& a)
  { return PRCVector2d(-a.x,-a.y); }
  friend PRCVector2d operator - (const PRCVector2d& a, const PRCVector2d& b)
  { return PRCVector2d(a.x-b.x,a.y-b.y); }
  friend PRCVector2d operator * (const PRCVector2d& a, const double d)
  { return PRCVector2d(a.x*d,a.y*d); }
  friend PRCVector2d operator * (const double d, const PRCVector2d& a)
  { return PRCVector2d(a.x*d,a.y*d); }
  friend PRCVector2d operator / (const PRCVector2d& a, const double d)
  { return PRCVector2d(a.x/d,a.y/d); }
    
  double Length();
  bool Normalize();
  
  bool operator==(const PRCVector2d &v) const
  {
    return x==v.x && y==v.y;
  }
  bool operator!=(const PRCVector2d &v) const
  {
    return !(x==v.x && y==v.y);
  }
  bool operator<(const PRCVector2d &v) const
  {
    if(x!=v.x)
      return (x<v.x);
    return (y<v.y);
  }
  friend std::ostream& operator << (std::ostream& out, const PRCVector2d& v)
  {
    out << "(" << v.x << "," << v.y << ")";
    return out;
  }
};

class PRCTextureDefinition : public ContentPRCBase
{
public:
  PRCTextureDefinition(std::string n="") :
    ContentPRCBase(PRC_TYPE_GRAPH_TextureDefinition,n), picture_index(m1), texture_mapping_attribute(PRC_TEXTURE_MAPPING_DIFFUSE),
    texture_mapping_attribute_intensity(1.0), texture_mapping_attribute_components(PRC_TEXTURE_MAPPING_COMPONENTS_RGBA),
    texture_function(KEPRCTextureFunction_Modulate), texture_applying_mode(PRC_TEXTURE_APPLYING_MODE_NONE),
    texture_wrapping_mode_S(KEPRCTextureWrappingMode_Unknown), texture_wrapping_mode_T(KEPRCTextureWrappingMode_Unknown) // ,
    // texture_transformation(false), texture_flip_S(false), texture_flip_T(false),
    // behaviour(PRC_TRANSFORMATION_Identity), scale(1.0,1.0), uniform_scale(1.0)
    {}
  void serializeTextureDefinition(PRCbitStream&);
  uint32_t picture_index;
  uint32_t texture_mapping_attribute;
  double texture_mapping_attribute_intensity;
  uint8_t texture_mapping_attribute_components;
  EPRCTextureFunction texture_function;
  uint8_t texture_applying_mode;
  EPRCTextureWrappingMode texture_wrapping_mode_S;
  EPRCTextureWrappingMode texture_wrapping_mode_T;
  // bool texture_transformation;
  // bool texture_flip_S;
  // bool texture_flip_T;
  // uint8_t behaviour;
  // PRCVector2d origin;
  // PRCVector2d X;
  // PRCVector2d Y;
  // PRCVector2d scale;
  // double uniform_scale;
  // double X_homegeneous_coord;
  // double Y_homegeneous_coord;
  // double origin_homegeneous_coord;
};
typedef std::deque <PRCTextureDefinition*>  PRCTextureDefinitionList;

class PRCMaterial
{
public:
  virtual ~PRCMaterial() {}
  virtual void serializeMaterial(PRCbitStream&) = 0;
};
typedef std::deque <PRCMaterial*>  PRCMaterialList;

class PRCMaterialGeneric : public ContentPRCBase, public PRCMaterial
{
public:
  PRCMaterialGeneric(std::string n="") :
    ContentPRCBase(PRC_TYPE_GRAPH_Material,n),
    ambient(m1), diffuse(m1), emissive(m1), specular(m1), 
    shininess(0.0),
    ambient_alpha(1.0), diffuse_alpha(1.0), emissive_alpha(1.0), specular_alpha(1.0)
    {}
  void serializeMaterialGeneric(PRCbitStream&);
  void serializeMaterial(PRCbitStream &pbs) { serializeMaterialGeneric(pbs); }
  uint32_t picture_index;
  uint32_t ambient;
  uint32_t diffuse;
  uint32_t emissive;
  uint32_t specular;
  double shininess;
  double ambient_alpha;
  double diffuse_alpha;
  double emissive_alpha;
  double specular_alpha;

  bool operator==(const PRCMaterialGeneric &m) const
  {
    return (ambient==m.ambient && diffuse==m.diffuse && emissive==m.emissive && specular==m.specular && shininess==m.shininess &&
            ambient_alpha==m.ambient_alpha && diffuse_alpha==m.diffuse_alpha && emissive_alpha==m.emissive_alpha && specular_alpha==m.specular_alpha);
  }
};

class PRCTextureApplication : public ContentPRCBase, public PRCMaterial
{
public:
  PRCTextureApplication(std::string n="") :
    ContentPRCBase(PRC_TYPE_GRAPH_TextureApplication,n),
    material_generic_index(m1), texture_definition_index(m1),
    next_texture_index(m1), UV_coordinates_index(0)
    {}
  void serializeTextureApplication(PRCbitStream&);
  void serializeMaterial(PRCbitStream &pbs) { serializeTextureApplication(pbs); }
  uint32_t material_generic_index;
  uint32_t texture_definition_index;
  uint32_t next_texture_index;
  uint32_t UV_coordinates_index;
};

class PRCLinePattern : public ContentPRCBase
{
public:
  PRCLinePattern(std::string n="") :
  ContentPRCBase(PRC_TYPE_GRAPH_LinePattern,n),
  phase(0), is_real_length(false) {}
  void serializeLinePattern(PRCbitStream&);
  std::vector<double> lengths;
  double phase;
  bool is_real_length;
};
typedef std::deque <PRCLinePattern*>  PRCLinePatternList;

class PRCStyle : public ContentPRCBase
{
public:
  PRCStyle(std::string n="") :
    ContentPRCBase(PRC_TYPE_GRAPH_Style,n), line_width(0.0), is_vpicture(false), line_pattern_vpicture_index(m1),
    is_material(false), color_material_index(m1), is_transparency_defined(false), transparency(255), additional(0)
    {}
  void serializeCategory1LineStyle(PRCbitStream&);
  double line_width;
  bool is_vpicture;
  uint32_t line_pattern_vpicture_index;
  bool is_material;
  uint32_t color_material_index;
  bool is_transparency_defined;
  uint8_t transparency;
  uint8_t additional;
};
typedef std::deque <PRCStyle*>  PRCStyleList;

class PRCTessFace
{
public:
  PRCTessFace() :
  start_wire(0), used_entities_flag(0),
  start_triangulated(0), number_of_texture_coordinate_indexes(0), 
  is_rgba(false), behaviour(PRC_GRAPHICS_Show)
  {}
  void serializeTessFace(PRCbitStream&);
  std::vector<uint32_t> line_attributes;
  uint32_t start_wire;			// specifing bounding wire seems not to work as of Acrobat/Reader 9.2
  std::vector<uint32_t> sizes_wire;	// specifing bounding wire seems not to work as of Acrobat/Reader 9.2
  uint32_t used_entities_flag;
  uint32_t start_triangulated;
  std::vector<uint32_t> sizes_triangulated;
  uint32_t number_of_texture_coordinate_indexes;
  bool is_rgba;
  std::vector<uint8_t> rgba_vertices;
  uint32_t behaviour;
};
typedef std::deque <PRCTessFace*>  PRCTessFaceList;

class PRCContentBaseTessData
{
public:
  PRCContentBaseTessData() :
  is_calculated(false) {}
  void serializeContentBaseTessData(PRCbitStream&);
  bool is_calculated;
  std::vector<double> coordinates;
};

class PRCTess : public PRCContentBaseTessData
{
public:
  virtual ~PRCTess() {}
  virtual void serializeBaseTessData(PRCbitStream &pbs) = 0;
};
typedef std::deque <PRCTess*>  PRCTessList;

class PRC3DTess : public PRCTess
{
public:
  PRC3DTess() :
  has_faces(false), has_loops(false),
  crease_angle(25.8419)  // arccos(0.9), default found in Acrobat output
  {}
  ~PRC3DTess() { for(PRCTessFaceList::iterator it=face_tessellation.begin(); it!=face_tessellation.end(); ++it) delete *it; }
  void serialize3DTess(PRCbitStream&);
  void serializeBaseTessData(PRCbitStream &pbs) { serialize3DTess(pbs); }
  void addTessFace(PRCTessFace*& pTessFace);

  bool has_faces;
  bool has_loops;
  double crease_angle;
  std::vector<double> normal_coordinate;
  std::vector<uint32_t> wire_index;		// specifing bounding wire seems not to work as of Acrobat/Reader 9.2
  std::vector<uint32_t> triangulated_index;
  PRCTessFaceList face_tessellation;
  std::vector<double> texture_coordinate;
};

class PRC3DWireTess : public PRCTess
{
public:
  PRC3DWireTess() :
  is_rgba(false), is_segment_color(false) {}
  void serialize3DWireTess(PRCbitStream&);
  void serializeBaseTessData(PRCbitStream &pbs) { serialize3DWireTess(pbs); }

  bool is_rgba;
  bool is_segment_color;
  std::vector<uint32_t> wire_indexes;
  std::vector<uint8_t> rgba_vertices;
};

class PRCMarkupTess : public PRCTess
{
public:
  PRCMarkupTess() :
  behaviour(0)
  {}
  void serializeMarkupTess(PRCbitStream&);
  void serializeBaseTessData(PRCbitStream &pbs) { serializeMarkupTess(pbs); }

  std::vector<uint32_t> codes;
  std::vector<std::string> texts;
  std::string label;
  uint8_t behaviour;
};

class PRCGraphics
{
public:
  PRCGraphics() : layer_index(m1), index_of_line_style(m1), behaviour_bit_field(PRC_GRAPHICS_Show) {}
  void serializeGraphics(PRCbitStream&);
  void serializeGraphicsForced(PRCbitStream&);
  bool has_graphics() { return (index_of_line_style!=m1 || layer_index!=m1 || behaviour_bit_field!=PRC_GRAPHICS_Show) ; }
  uint32_t layer_index;
  uint32_t index_of_line_style;
  uint16_t behaviour_bit_field;
};
typedef std::deque <PRCGraphics*>  PRCGraphicsList;

void writeGraphics(PRCbitStream&,const PRCGraphics&,bool=false);

class PRCMarkup: public PRCGraphics, public ContentPRCBase
{
public:
  PRCMarkup(std::string n="") :
    ContentPRCBase(PRC_TYPE_MKP_Markup,n),
    type(KEPRCMarkupType_Unknown), sub_type(KEPRCMarkupSubType_Unknown), index_tessellation(m1) {}
  void serializeMarkup(PRCbitStream&);
  EPRCMarkupType type;
  EPRCMarkupSubType sub_type;
// vector<PRCReferenceUniqueIdentifier> linked_items;
// vector<PRCReferenceUniqueIdentifier> leaders;
  uint32_t index_tessellation;
};
typedef std::deque <PRCMarkup*>  PRCMarkupList;

class PRCAnnotationItem: public PRCGraphics, public ContentPRCBase
{
public:
  PRCAnnotationItem(std::string n="") :
    ContentPRCBase(PRC_TYPE_MKP_AnnotationItem,n) {}
  void serializeAnnotationItem(PRCbitStream&);
  void serializeAnnotationEntity(PRCbitStream &pbs) { serializeAnnotationItem(pbs); }
  PRCReferenceUniqueIdentifier markup;
};
typedef std::deque <PRCAnnotationItem*>  PRCAnnotationItemList;

class PRCRepresentationItemContent: public PRCGraphics, public ContentPRCBase
{
public:
  PRCRepresentationItemContent(uint32_t t, std::string n="") :
    ContentPRCBase(t,n),
    index_local_coordinate_system(m1), index_tessellation(m1) {}
  void serializeRepresentationItemContent(PRCbitStream&);
  uint32_t index_local_coordinate_system;
  uint32_t index_tessellation;
};

class PRCRepresentationItem : public PRCRepresentationItemContent
{
public:
  PRCRepresentationItem(uint32_t t, std::string n="") :
    PRCRepresentationItemContent(t,n) {}
  virtual ~PRCRepresentationItem() {}
  virtual void serializeRepresentationItem(PRCbitStream &pbs) = 0;
};
typedef std::deque <PRCRepresentationItem*>  PRCRepresentationItemList;

class PRCBrepModel : public PRCRepresentationItem
{
public:
  PRCBrepModel(std::string n="") :
    PRCRepresentationItem(PRC_TYPE_RI_BrepModel,n), has_brep_data(true), context_id(m1), body_id(m1), is_closed(false) {}
  void serializeBrepModel(PRCbitStream&);
  void serializeRepresentationItem(PRCbitStream &pbs) { serializeBrepModel(pbs); }
  bool has_brep_data;
  uint32_t context_id;
  uint32_t body_id;
  bool is_closed;
};

class PRCPolyBrepModel : public PRCRepresentationItem
{
public:
  PRCPolyBrepModel(std::string n="") :
    PRCRepresentationItem(PRC_TYPE_RI_PolyBrepModel,n), is_closed(false) {}
  void serializePolyBrepModel(PRCbitStream&);
  void serializeRepresentationItem(PRCbitStream &pbs) { serializePolyBrepModel(pbs); }
  bool is_closed;
};

class PRCPointSet : public PRCRepresentationItem
{
public:
  PRCPointSet(std::string n="") :
    PRCRepresentationItem(PRC_TYPE_RI_PointSet,n) {}
  void serializePointSet(PRCbitStream&);
  void serializeRepresentationItem(PRCbitStream &pbs) { serializePointSet(pbs); }
  std::vector<PRCVector3d> point;
};

class PRCWire : public PRCRepresentationItem
{
public:
  PRCWire(std::string n="") :
    PRCRepresentationItem(PRC_TYPE_RI_Curve,n), has_wire_body(true), context_id(m1), body_id(m1) {}
  void serializeWire(PRCbitStream&);
  void serializeRepresentationItem(PRCbitStream &pbs) { serializeWire(pbs); }
  bool has_wire_body;
  uint32_t context_id;
  uint32_t body_id;
};

class PRCPolyWire : public PRCRepresentationItem
{
public:
  PRCPolyWire(std::string n="") :
    PRCRepresentationItem(PRC_TYPE_RI_PolyWire,n) {}
  void serializePolyWire(PRCbitStream&);
  void serializeRepresentationItem(PRCbitStream &pbs) { serializePolyWire(pbs); }
};

class PRCSet : public PRCRepresentationItem
{
public:
  PRCSet(std::string n="") :
    PRCRepresentationItem(PRC_TYPE_RI_Set,n) {}
  ~PRCSet() { for(PRCRepresentationItemList::iterator it=elements.begin(); it!=elements.end(); ++it) delete *it; }
  void serializeSet(PRCbitStream&);
  void serializeRepresentationItem(PRCbitStream &pbs) { serializeSet(pbs); }
  uint32_t addBrepModel(PRCBrepModel*& pBrepModel);
  uint32_t addPolyBrepModel(PRCPolyBrepModel*& pPolyBrepModel);
  uint32_t addPointSet(PRCPointSet*& pPointSet);
  uint32_t addSet(PRCSet*& pSet);
  uint32_t addWire(PRCWire*& pWire);
  uint32_t addPolyWire(PRCPolyWire*& pPolyWire);
  uint32_t addRepresentationItem(PRCRepresentationItem*& pRepresentationItem);
  PRCRepresentationItemList elements;
};

class PRCTransformation3d
{
public:
  virtual ~PRCTransformation3d() {}
  virtual void serializeTransformation3d(PRCbitStream&) const =0;
};
typedef std::deque <PRCTransformation3d*> PRCTransformation3dList;

class PRCGeneralTransformation3d : public PRCTransformation3d
{
public:
  PRCGeneralTransformation3d()
  {
    setidentity();
  }
  PRCGeneralTransformation3d(const double t[])
  {
    set(t);
  }
  
  void serializeGeneralTransformation3d(PRCbitStream&) const;
  void serializeTransformation3d(PRCbitStream& pbs)  const { serializeGeneralTransformation3d(pbs); }
  double mat[4][4];
  bool operator==(const PRCGeneralTransformation3d &t) const
  {
    for (size_t i=0;i<4;i++)
      for (size_t j=0;j<4;j++)
        if(mat[i][j]!=t.mat[i][j])
         return false;
    return true;
  }
  bool operator<(const PRCGeneralTransformation3d &t) const
  {
    for (size_t i=0;i<4;i++)
      for (size_t j=0;j<4;j++)
        if(mat[i][j]!=t.mat[i][j])
        {
          return (mat[i][j]<t.mat[i][j]);
        }
    return false;
  }
  void set(const double t[])
  {
    if(t!=NULL) 
     for (size_t i=0;i<4;i++)
       for (size_t j=0;j<4;j++)
         mat[i][j]=t[4*i+j];
    else
      setidentity();
  }
  void setidentity()
  {
    mat[0][0]=1; mat[0][1]=0; mat[0][2]=0; mat[0][3]=0;
    mat[1][0]=0; mat[1][1]=1; mat[1][2]=0; mat[1][3]=0;
    mat[2][0]=0; mat[2][1]=0; mat[2][2]=1; mat[2][3]=0;
    mat[3][0]=0; mat[3][1]=0; mat[3][2]=0; mat[3][3]=1;
  }
  bool isidtransform() const {
    return(
    mat[0][0]==1 && mat[0][1]==0 && mat[0][2]==0 && mat[0][3]==0 &&
    mat[1][0]==0 && mat[1][1]==1 && mat[1][2]==0 && mat[1][3]==0 &&
    mat[2][0]==0 && mat[2][1]==0 && mat[2][2]==1 && mat[2][3]==0 &&
    mat[3][0]==0 && mat[3][1]==0 && mat[3][2]==0 && mat[3][3]==1);
  }
  bool isnotidtransform() const {
    return !isidtransform();
  }
  double M(size_t i, size_t j) const {
    // Like Fortran, PRC uses transposed (column-major) format!
    return mat[j][i];
  }
};
typedef std::deque <PRCGeneralTransformation3d> PRCGeneralTransformation3dList;

class PRCCartesianTransformation3d : public PRCTransformation3d
{
public:
  PRCCartesianTransformation3d() :
    behaviour(PRC_TRANSFORMATION_Identity), origin(0.0,0.0,0.0), X(1.0,0.0,0.0), Y(0.0,1.0,0.0), Z(0.0,0.0,1.0),
    scale(1.0,1.0,1.0), uniform_scale(1.0),
    X_homogeneous_coord(0.0), Y_homogeneous_coord(0.0), Z_homogeneous_coord(0.0), origin_homogeneous_coord(1.0) {}
  PRCCartesianTransformation3d(const double o[3], const double x[3], const double y[3], double sc) :
    behaviour(PRC_TRANSFORMATION_Identity), origin(o,0.0,0.0,0.0), X(x,1.0,0.0,0.0), Y(y,0.0,1.0,0.0), Z(0.0,0.0,1.0),
    scale(1.0,1.0,1.0), uniform_scale(sc),
    X_homogeneous_coord(0.0), Y_homogeneous_coord(0.0), Z_homogeneous_coord(0.0), origin_homogeneous_coord(1.0)
    {
      if(origin!=PRCVector3d(0.0,0.0,0.0))
        behaviour = behaviour | PRC_TRANSFORMATION_Translate;
      if(X!=PRCVector3d(1.0,0.0,0.0) || Y!=PRCVector3d(0.0,1.0,0.0))
        behaviour = behaviour | PRC_TRANSFORMATION_Rotate;
      if(uniform_scale!=1)
        behaviour = behaviour | PRC_TRANSFORMATION_Scale;
    }
  void serializeCartesianTransformation3d(PRCbitStream& pbs) const;
  void serializeTransformation3d(PRCbitStream& pbs) const { serializeCartesianTransformation3d(pbs); }
  uint8_t behaviour;
  PRCVector3d origin;
  PRCVector3d X;
  PRCVector3d Y;
  PRCVector3d Z;
  PRCVector3d scale;
  double uniform_scale;
  double X_homogeneous_coord;
  double Y_homogeneous_coord;
  double Z_homogeneous_coord;
  double origin_homogeneous_coord;
  bool operator==(const PRCCartesianTransformation3d &t) const
  {
    return behaviour==t.behaviour && origin==t.origin && X==t.X && Y==t.Y && Z==t.Z && scale==t.scale && uniform_scale==t.uniform_scale &&
           X_homogeneous_coord==t.X_homogeneous_coord && Y_homogeneous_coord==t.Y_homogeneous_coord &&
           Z_homogeneous_coord==t.Z_homogeneous_coord && origin_homogeneous_coord==t.origin_homogeneous_coord;
  }
};

class PRCTransformation
{
public:
  PRCTransformation() :
    has_transformation(false), geometry_is_2D(false), behaviour(PRC_TRANSFORMATION_Identity),
    origin(0.0,0.0,0.0), x_axis(1.0,0.0,0.0), y_axis(0.0,1.0,0.0), scale(1) {}
  void serializeTransformation(PRCbitStream&);
  bool has_transformation;
  bool geometry_is_2D;
  uint8_t behaviour;
  PRCVector3d origin;
  PRCVector3d x_axis;
  PRCVector3d y_axis;
  double scale;
};

class PRCCoordinateSystem : public PRCRepresentationItem
{
public:
  PRCCoordinateSystem(std::string n="") :
  PRCRepresentationItem(PRC_TYPE_RI_CoordinateSystem,n), axis_set(NULL) {}
  ~PRCCoordinateSystem() { delete axis_set; }
  void serializeCoordinateSystem(PRCbitStream&);
  void serializeRepresentationItem(PRCbitStream &pbs) { serializeCoordinateSystem(pbs); }
  void setAxisSet(PRCGeneralTransformation3d*& transform) { axis_set = transform; transform = NULL; } 
  void setAxisSet(PRCCartesianTransformation3d*& transform) { axis_set = transform; transform = NULL; } 
  PRCTransformation3d *axis_set;
  bool operator==(const PRCCoordinateSystem &t) const
  {
    if(index_local_coordinate_system!=t.index_local_coordinate_system)
      return false;
    PRCGeneralTransformation3d*       axis_set_general = dynamic_cast<PRCGeneralTransformation3d*>(axis_set);
    PRCGeneralTransformation3d*     t_axis_set_general = dynamic_cast<PRCGeneralTransformation3d*>(t.axis_set);
    PRCCartesianTransformation3d*   axis_set_cartesian = dynamic_cast<PRCCartesianTransformation3d*>(axis_set);
    PRCCartesianTransformation3d* t_axis_set_cartesian = dynamic_cast<PRCCartesianTransformation3d*>(t.axis_set);
    if(axis_set_general!=NULL)
      return (t_axis_set_general!=NULL?(*axis_set_general==*t_axis_set_general):false); 
    if(axis_set_cartesian!=NULL)
      return (t_axis_set_cartesian!=NULL?(*axis_set_cartesian==*t_axis_set_cartesian):false); 
    return false;
  }
};
typedef std::deque <PRCCoordinateSystem*>  PRCCoordinateSystemList;

struct PRCFontKey
{
  uint32_t font_size;
  uint8_t  attributes;
};

class PRCFontKeysSameFont
{
public:
  void serializeFontKeysSameFont(PRCbitStream&);
  std::string font_name;
  uint32_t char_set;
  std::vector<PRCFontKey> font_keys;

};

// Topology
class PRCBaseGeometry : public PRCAttributes
{
public:
  PRCBaseGeometry() :
    base_information(false), identifier(0) {}
  PRCBaseGeometry(std::string n, uint32_t id = 0) :
    base_information(true),name(n),identifier(id) {}
  void serializeBaseGeometry(PRCbitStream&);
  bool base_information;
  std::string name;
  uint32_t identifier;
};

class PRCBoundingBox
{
public:
  PRCBoundingBox() : min(0.0,0.0,0.0), max(0.0,0.0,0.0) {}
  PRCBoundingBox(const PRCVector3d &m1, const PRCVector3d& m2) : min(m1),max(m2) {}
  void serializeBoundingBox(PRCbitStream &pbs);
  PRCVector3d min;
  PRCVector3d max;
};

class PRCDomain
{
public:
  void serializeDomain(PRCbitStream &pbs);
  PRCVector2d min;
  PRCVector2d max;
};

class PRCInterval
{
public:
  PRCInterval() : min(0), max(0) {}
  PRCInterval(double m, double M) : min(m), max(M) {}
  void serializeInterval(PRCbitStream &pbs);
  double min;
  double max;
};

class PRCParameterization
{
public:
  PRCParameterization() : parameterization_coeff_a(1), parameterization_coeff_b(0) {}
  PRCParameterization(double min, double max) : interval(min, max), parameterization_coeff_a(1), parameterization_coeff_b(0) {}
  void serializeParameterization(PRCbitStream &pbs);
  PRCInterval interval;
  double parameterization_coeff_a;
  double parameterization_coeff_b;
};

class PRCUVParameterization
{
public:
  PRCUVParameterization() : swap_uv(false),
    parameterization_on_u_coeff_a(1), parameterization_on_v_coeff_a(1),
    parameterization_on_u_coeff_b(0), parameterization_on_v_coeff_b(0) {}
  void serializeUVParameterization(PRCbitStream &pbs);
  bool swap_uv;
  PRCDomain uv_domain;
  double parameterization_on_u_coeff_a;
  double parameterization_on_v_coeff_a;
  double parameterization_on_u_coeff_b;
  double parameterization_on_v_coeff_b;
};

class PRCControlPoint
{
public:
  PRCControlPoint() :
   x(0), y(0), z(0), w(1) {}
  PRCControlPoint(double X, double Y, double Z=0, double W=1) :
   x(X), y(Y), z(Z), w(W) {}
  PRCControlPoint(const PRCVector3d &v) :
   x(v.x), y(v.y), z(v.z), w(1) {}
  void Set(double fx, double fy, double fz, double fw=1)
   { x = fx; y = fy; z = fz; w = fw; }
  double x;
  double y;
  double z;
  double w;
};

class PRCContentSurface: public PRCBaseGeometry
{
public:
  PRCContentSurface() :
    PRCBaseGeometry(), extend_info(KEPRCExtendTypeNone) {}
  PRCContentSurface(std::string n) :
    PRCBaseGeometry(n,makeCADID()),extend_info(KEPRCExtendTypeNone) {} 
  void serializeContentSurface(PRCbitStream&);
  EPRCExtendType extend_info;
};

class PRCSurface : public PRCContentSurface
{
public:
  PRCSurface() :
    PRCContentSurface() {}
  PRCSurface(std::string n) :
    PRCContentSurface(n) {}
  virtual ~PRCSurface() {}
  virtual void  serializeSurface(PRCbitStream &pbs) = 0;
};

class PRCNURBSSurface : public PRCSurface
{
public:
  PRCNURBSSurface() :
    PRCSurface(), knot_type(KEPRCKnotTypeUnspecified), surface_form(KEPRCBSplineSurfaceFormUnspecified) {}
  PRCNURBSSurface(std::string n) :
    PRCSurface(n), knot_type(KEPRCKnotTypeUnspecified), surface_form(KEPRCBSplineSurfaceFormUnspecified) {}
  void  serializeNURBSSurface(PRCbitStream &pbs);
  void  serializeSurface(PRCbitStream &pbs) { serializeNURBSSurface(pbs); }
  bool is_rational;
  uint32_t degree_in_u;
  uint32_t degree_in_v;
  std::vector<PRCControlPoint> control_point;
  std::vector<double> knot_u;
  std::vector<double> knot_v;
  const EPRCKnotType knot_type;
  const EPRCBSplineSurfaceForm surface_form;
};

class PRCContentCurve: public PRCBaseGeometry
{
public:
  PRCContentCurve() :
    PRCBaseGeometry(), extend_info(KEPRCExtendTypeNone), is_3d(true) {}
  PRCContentCurve(std::string n) :
    PRCBaseGeometry(n,makeCADID()),extend_info(KEPRCExtendTypeNone), is_3d(true) {} 
  void serializeContentCurve(PRCbitStream&);
  EPRCExtendType extend_info;
  bool is_3d;
};

class PRCCurve : public PRCContentCurve
{
public:
  PRCCurve() :
    PRCContentCurve() {}
  PRCCurve(std::string n) :
    PRCContentCurve(n) {}
  virtual ~PRCCurve() {}
  virtual void  serializeCurve(PRCbitStream &pbs) = 0;
};
typedef std::deque <PRCCurve*>  PRCCurveList;

class PRCNURBSCurve : public PRCCurve
{
public:
  PRCNURBSCurve() :
    PRCCurve(), knot_type(KEPRCKnotTypeUnspecified), curve_form(KEPRCBSplineCurveFormUnspecified) {}
  PRCNURBSCurve(std::string n) :
    PRCCurve(n), knot_type(KEPRCKnotTypeUnspecified), curve_form(KEPRCBSplineCurveFormUnspecified) {}
  void  serializeNURBSCurve(PRCbitStream &pbs);
  void  serializeCurve(PRCbitStream &pbs) { serializeNURBSCurve(pbs); }
  bool is_rational;
  uint32_t degree;
  std::vector<PRCControlPoint> control_point;
  std::vector<double> knot;
  const EPRCKnotType knot_type;
  const EPRCBSplineCurveForm curve_form;
};

class PRCPolyLine : public PRCCurve, public PRCTransformation, public PRCParameterization
{
public:
  PRCPolyLine() :
    PRCCurve() {}
  PRCPolyLine(std::string n) :
    PRCCurve(n) {}
  void  serializePolyLine(PRCbitStream &pbs);
  void  serializeCurve(PRCbitStream &pbs) { serializePolyLine(pbs); }
  std::vector<PRCVector3d> point;
};

class PRCCircle : public PRCCurve, public PRCTransformation, public PRCParameterization
{
public:
  PRCCircle() :
    PRCCurve(), PRCParameterization(0,2*pi) {}
  PRCCircle(std::string n) :
    PRCCurve(n), PRCParameterization(0,2*pi) {}
  void  serializeCircle(PRCbitStream &pbs);
  void  serializeCurve(PRCbitStream &pbs) { serializeCircle(pbs); }
  double radius;
};

class PRCComposite : public PRCCurve, public PRCTransformation, public PRCParameterization
{
public:
  PRCComposite() :
    PRCCurve() {}
  PRCComposite(std::string n) :
    PRCCurve(n) {}
  void  serializeComposite(PRCbitStream &pbs);
  void  serializeCurve(PRCbitStream &pbs) { serializeComposite(pbs); }
  PRCCurveList base_curve;
  std::vector<bool> base_sense;
  bool is_closed;
};

class PRCBlend01 : public PRCSurface, public PRCTransformation, public PRCUVParameterization
{
public:
  PRCBlend01() :
    PRCSurface(), center_curve(NULL), origin_curve(NULL), tangent_curve(NULL) {}
  PRCBlend01(std::string n) :
    PRCSurface(n), center_curve(NULL), origin_curve(NULL), tangent_curve(NULL) {}
  ~PRCBlend01() { delete center_curve; delete origin_curve; delete tangent_curve; }
  void  serializeBlend01(PRCbitStream &pbs);
  void  serializeSurface(PRCbitStream &pbs) { serializeBlend01(pbs); }
// void  setCenterCurve (PRCCurve*& curve) { center_curve  = curve; curve = NULL; }
// void  setOriginCurve (PRCCurve*& curve) { origin_curve  = curve; curve = NULL; }
// void  setTangentCurve(PRCCurve*& curve) { tangent_curve = curve; curve = NULL; }
  PRCCurve* center_curve;
  PRCCurve* origin_curve;
  PRCCurve* tangent_curve;
};

class PRCRuled : public PRCSurface, public PRCTransformation, public PRCUVParameterization
{
public:
  PRCRuled() :
    PRCSurface(), first_curve(NULL), second_curve(NULL) {}
  PRCRuled(std::string n) :
    PRCSurface(n) {}
  ~PRCRuled() { delete first_curve; delete second_curve; }
  void  serializeRuled(PRCbitStream &pbs);
  void  serializeSurface(PRCbitStream &pbs) { serializeRuled(pbs); }
// void  setFirstCurve(PRCCurve*&  curve) { first_curve  = curve; curve = NULL; }
// void  setSecondCurve(PRCCurve*& curve) { second_curve = curve; curve = NULL; }
  PRCCurve* first_curve;
  PRCCurve* second_curve;
};

class PRCSphere : public PRCSurface, public PRCTransformation, public PRCUVParameterization
{
public:
  PRCSphere() :
    PRCSurface() {}
  PRCSphere(std::string n) :
    PRCSurface(n) {}
  void  serializeSphere(PRCbitStream &pbs);
  void  serializeSurface(PRCbitStream &pbs) { serializeSphere(pbs); }
  double radius;
};

class PRCCone : public PRCSurface, public PRCTransformation, public PRCUVParameterization
{
public:
  PRCCone() :
    PRCSurface() {}
  PRCCone(std::string n) :
    PRCSurface(n) {}
  void  serializeCone(PRCbitStream &pbs);
  void  serializeSurface(PRCbitStream &pbs) { serializeCone(pbs); }
  double bottom_radius;
  double semi_angle;
};

class PRCCylinder : public PRCSurface, public PRCTransformation, public PRCUVParameterization
{
public:
  PRCCylinder() :
    PRCSurface() {}
  PRCCylinder(std::string n) :
    PRCSurface(n) {}
  void  serializeCylinder(PRCbitStream &pbs);
  void  serializeSurface(PRCbitStream &pbs) { serializeCylinder(pbs); }
  double radius;
};

class PRCTorus : public PRCSurface, public PRCTransformation, public PRCUVParameterization
{
public:
  PRCTorus() :
    PRCSurface() {}
  PRCTorus(std::string n) :
    PRCSurface(n) {}
  void  serializeTorus(PRCbitStream &pbs);
  void  serializeSurface(PRCbitStream &pbs) { serializeTorus(pbs); }
  double major_radius;
  double minor_radius;
};

class PRCBaseTopology : public PRCAttributes
{
public:
  PRCBaseTopology() :
    base_information(false),identifier(0) {}
  PRCBaseTopology(std::string n, uint32_t id = 0) :
    base_information(true),name(n),identifier(id) {}
  void serializeBaseTopology(PRCbitStream&);
  bool base_information;
  std::string name;
  uint32_t identifier;
};

class PRCTopoItem
{
public:
  virtual ~PRCTopoItem() {}
  virtual void serializeTopoItem(PRCbitStream&)=0;
};

class PRCContentBody: public PRCBaseTopology
{
public:
  PRCContentBody() :
    PRCBaseTopology(), behavior(0) {}
  PRCContentBody(std::string n) :
    PRCBaseTopology(n,makeCADID()), behavior(0) {}
  void serializeContentBody(PRCbitStream&);
  uint8_t behavior;
};

class PRCBody : public PRCContentBody, public PRCTopoItem
{
public:
  PRCBody() :
    PRCContentBody(), topo_item_type(PRC_TYPE_ROOT) {}
  PRCBody(uint32_t tit) :
    PRCContentBody(), topo_item_type(tit) {}
  PRCBody(uint32_t tit, std::string n) :
    PRCContentBody(n), topo_item_type(tit) {}
  virtual ~PRCBody() {}
  virtual void serializeBody(PRCbitStream &pbs) = 0;
  void serializeTopoItem(PRCbitStream &pbs) { serializeBody(pbs); }
  uint32_t serialType() { return topo_item_type; }
  virtual double serialTolerance() { return 0; }
  const uint32_t topo_item_type;
};
typedef std::deque <PRCBody*>  PRCBodyList;

class PRCContentWireEdge : public PRCBaseTopology
{
public:
  PRCContentWireEdge() :
    PRCBaseTopology(), curve_3d(NULL), has_curve_trim_interval(false) {}
  PRCContentWireEdge(std::string n) :
    PRCBaseTopology(n,makeCADID()), curve_3d(NULL), has_curve_trim_interval(false) {} 
  ~PRCContentWireEdge() { delete curve_3d; }
  void serializeContentWireEdge(PRCbitStream &pbs);
// void setCurve(PRCCurve*& curve) { curve_3d = curve; curve = NULL; }
  PRCCurve* curve_3d;
  bool has_curve_trim_interval;
  PRCInterval curve_trim_interval;
};

class PRCWireEdge : public PRCContentWireEdge, public PRCTopoItem
{
public:
  void serializeWireEdge(PRCbitStream &pbs);
  void serializeTopoItem(PRCbitStream &pbs) { serializeWireEdge(pbs); }
};

class PRCSingleWireBody : public PRCBody
{
public:
  PRCSingleWireBody() :
    PRCBody(PRC_TYPE_TOPO_SingleWireBody), wire_edge(NULL) {}
  PRCSingleWireBody(std::string n) :
    PRCBody(PRC_TYPE_TOPO_SingleWireBody, n), wire_edge(NULL) {}
  ~PRCSingleWireBody() { delete wire_edge; }
  void serializeSingleWireBody(PRCbitStream &pbs);
  void serializeBody(PRCbitStream &pbs) { serializeSingleWireBody(pbs); }
  void setWireEdge(PRCWireEdge*& wireEdge) { wire_edge = wireEdge; wireEdge = NULL; }  
  PRCWireEdge* wire_edge;
};

class PRCFace : public PRCBaseTopology, public PRCTopoItem, public PRCGraphics
{
public:
  PRCFace() :
    PRCBaseTopology(), base_surface(NULL), have_surface_trim_domain(false), have_tolerance(false), tolerance(0), number_of_loop(0), outer_loop_index(-1) {}
  PRCFace(std::string n) :
    PRCBaseTopology(n,makeCADID()), base_surface(NULL), have_surface_trim_domain(false), have_tolerance(false), tolerance(0), number_of_loop(0), outer_loop_index(-1) {} 
  ~PRCFace() { delete base_surface; }
  void serializeFace(PRCbitStream &pbs);
  void serializeTopoItem(PRCbitStream &pbs) { serializeFace(pbs); }
  void setBaseSurface(PRCSurface*& surface) { base_surface = surface; surface = NULL; }
  PRCSurface *base_surface;
  const bool have_surface_trim_domain;
  PRCDomain surface_trim_domain;
  const bool have_tolerance;
  const double tolerance;
  const uint32_t number_of_loop;
  const int32_t outer_loop_index;
// PRCLoopList loop;
};
typedef std::deque <PRCFace*>  PRCFaceList;

class PRCShell : public PRCBaseTopology, public PRCTopoItem
{
public:
  PRCShell() :
    PRCBaseTopology(), shell_is_closed(false) {}
  PRCShell(std::string n) :
    PRCBaseTopology(n,makeCADID()), shell_is_closed(false) {}
  ~PRCShell() { for(PRCFaceList::iterator it=face.begin(); it!=face.end(); ++it) delete *it; }
  void serializeShell(PRCbitStream &pbs);
  void serializeTopoItem(PRCbitStream &pbs) { serializeShell(pbs); }
  void addFace(PRCFace*& pFace, uint8_t orientation=2);
  bool shell_is_closed;
  PRCFaceList face;
  std::vector<uint8_t> orientation_surface_with_shell;
};
typedef std::deque <PRCShell*>  PRCShellList;

class PRCConnex : public PRCBaseTopology, public PRCTopoItem
{
public:
  PRCConnex() :
    PRCBaseTopology() {}
  PRCConnex(std::string n) :
    PRCBaseTopology(n,makeCADID()) {} 
  ~PRCConnex() { for(PRCShellList::iterator it=shell.begin(); it!=shell.end(); ++it) delete *it; }
  void serializeConnex(PRCbitStream &pbs);
  void serializeTopoItem(PRCbitStream &pbs) { serializeConnex(pbs); }
  void addShell(PRCShell*& pShell);
  PRCShellList shell;
};
typedef std::deque <PRCConnex*>  PRCConnexList;

class PRCBrepData : public PRCBody, public PRCBoundingBox
{
public:
  PRCBrepData() :
    PRCBody(PRC_TYPE_TOPO_BrepData) {}
  PRCBrepData(std::string n) :
    PRCBody(PRC_TYPE_TOPO_BrepData, n) {}
  ~PRCBrepData() { for(PRCConnexList::iterator it=connex.begin(); it!=connex.end(); ++it) delete *it; }
  void serializeBrepData(PRCbitStream &pbs);
  void serializeBody(PRCbitStream &pbs) { serializeBrepData(pbs); }
  void addConnex(PRCConnex*& pConnex);
  PRCConnexList connex;
};

// For now - treat just the case of Bezier surfaces cubic 4x4 or linear 2x2
class PRCCompressedFace : public PRCBaseTopology, public PRCGraphics
{
public:
  PRCCompressedFace() :
    PRCBaseTopology(), orientation_surface_with_shell(true), degree(0) {}
  PRCCompressedFace(std::string n) :
    PRCBaseTopology(n,makeCADID()), orientation_surface_with_shell(true), degree(0) {} 
  void serializeCompressedFace(PRCbitStream &pbs, double brep_data_compressed_tolerance);
  void serializeContentCompressedFace(PRCbitStream &pbs);
  void serializeCompressedAnaNurbs(PRCbitStream &pbs, double brep_data_compressed_tolerance);
  void serializeCompressedNurbs(PRCbitStream &pbs, double brep_data_compressed_tolerance);
  bool orientation_surface_with_shell;
  uint32_t degree;
  std::vector<PRCVector3d> control_point;
};
typedef std::deque <PRCCompressedFace*>  PRCCompressedFaceList;

// For now - treat just the case of one connex/one shell
class PRCCompressedBrepData : public PRCBody
{
public:
  PRCCompressedBrepData() :
    PRCBody(PRC_TYPE_TOPO_BrepDataCompress), serial_tolerance(0), brep_data_compressed_tolerance(0) {}
  PRCCompressedBrepData(std::string n) :
    PRCBody(PRC_TYPE_TOPO_BrepDataCompress, n), serial_tolerance(0), brep_data_compressed_tolerance(0) {}
  ~PRCCompressedBrepData() { for(PRCCompressedFaceList::iterator it=face.begin(); it!=face.end(); ++it) delete *it; }
  void serializeCompressedBrepData(PRCbitStream &pbs);
  void serializeBody(PRCbitStream &pbs) { serializeCompressedBrepData(pbs); }
  void serializeCompressedShell(PRCbitStream &pbs);
  double serialTolerance() { return serial_tolerance; }
  double serial_tolerance;
  double brep_data_compressed_tolerance;
  PRCCompressedFaceList face;
};

class PRCTopoContext : public ContentPRCBase
{
public:
  PRCTopoContext(std::string n="") :
  ContentPRCBase(PRC_TYPE_TOPO_Context,n), behaviour(0), granularity(1), tolerance(0),
   have_smallest_face_thickness(false), smallest_thickness(0), have_scale(false), scale(1) {}
  ~PRCTopoContext() { for(PRCBodyList::iterator it=body.begin(); it!=body.end(); ++it) delete *it; }
  void serializeTopoContext(PRCbitStream&);
  void serializeContextAndBodies(PRCbitStream&);
  void serializeGeometrySummary(PRCbitStream&);
  void serializeContextGraphics(PRCbitStream&);
  uint32_t addSingleWireBody(PRCSingleWireBody*& body);
  uint32_t addBrepData(PRCBrepData*& body);
  uint32_t addCompressedBrepData(PRCCompressedBrepData*& body);
  uint8_t  behaviour;
  double granularity;
  double tolerance;
  bool have_smallest_face_thickness;
  double smallest_thickness;
  bool have_scale;
  double scale;
  PRCBodyList body;
};
typedef std::deque <PRCTopoContext*>  PRCTopoContextList;

class PRCUniqueId
{
public:
  PRCUniqueId() : id0(0), id1(0), id2(0), id3(0)  {}
  void serializeCompressedUniqueId(PRCbitStream&) const;
  void serializeFileStructureUncompressedUniqueId(std::ostream& out) const;
  uint32_t id0;
  uint32_t id1;
  uint32_t id2;
  uint32_t id3;
};

class PRCUnit
{
public:
  PRCUnit() : unit_from_CAD_file(false), unit(1) {}
  PRCUnit(double u, bool ufcf=true) : unit_from_CAD_file(ufcf), unit(u) {}
  void serializeUnit(PRCbitStream&);
  bool unit_from_CAD_file;
  double unit;
};

class PRCProductOccurrence: public PRCGraphics, public ContentPRCBase
{
public:
  PRCProductOccurrence(std::string n="") :
    ContentPRCBase(PRC_TYPE_ASM_ProductOccurence,n),
    index_part(m1),
    index_prototype(m1), prototype_in_same_file_structure(true),
    index_external_data(m1), external_data_in_same_file_structure(true),
    product_behaviour(0), product_information_flags(0), product_load_status(KEPRCProductLoadStatus_Loaded),
    location(NULL) {}
  ~PRCProductOccurrence() { delete location; }
  void setLocation(PRCGeneralTransformation3d*& transform) { location = transform; transform = NULL; }
  void serializeProductOccurrence(PRCbitStream&);
  uint32_t index_part;
  uint32_t index_prototype;
  bool prototype_in_same_file_structure;
  PRCUniqueId prototype_file_structure;
  uint32_t index_external_data;
  bool external_data_in_same_file_structure;
  PRCUniqueId external_data_file_structure;
  std::vector<uint32_t> index_son_occurrence;
  uint8_t product_behaviour;
  PRCUnit unit_information;
  uint8_t product_information_flags;
  EPRCProductLoadStatus product_load_status;
  PRCGeneralTransformation3d *location;
};
typedef std::deque <PRCProductOccurrence*>  PRCProductOccurrenceList;

class PRCPartDefinition: public PRCGraphics, public ContentPRCBase, public PRCBoundingBox
{
public:
	PRCPartDefinition(std::string n="") :
    ContentPRCBase(PRC_TYPE_ASM_PartDefinition,n) {}
  ~PRCPartDefinition() { for(PRCRepresentationItemList::iterator it=representation_item.begin(); it!=representation_item.end(); ++it) delete *it; }
	void serializePartDefinition(PRCbitStream&);
	uint32_t addBrepModel(PRCBrepModel*& pBrepModel);
	uint32_t addPolyBrepModel(PRCPolyBrepModel*& pPolyBrepModel);
	uint32_t addPointSet(PRCPointSet*& pPointSet);
	uint32_t addSet(PRCSet*& pSet);
	uint32_t addWire(PRCWire*& pWire);
	uint32_t addPolyWire(PRCPolyWire*& pPolyWire);
	uint32_t addRepresentationItem(PRCRepresentationItem*& pRepresentationItem);
	PRCRepresentationItemList representation_item;
};
typedef std::deque <PRCPartDefinition*>  PRCPartDefinitionList;

#endif //__WRITE_PRC_H
