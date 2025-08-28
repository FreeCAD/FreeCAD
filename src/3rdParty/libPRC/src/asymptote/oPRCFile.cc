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

#include "oPRCFile.h"
#include <time.h>
#include <sstream>
#include <iostream>
#include <fstream>
#include <iomanip>
#include <string>
#include <zlib.h>
#include <string.h>

#define WriteUnsignedInteger( value ) out << (uint32_t)(value);
#define WriteInteger( value ) out << (int32_t)(value);
#define WriteDouble( value ) out << (double)(value);
#define WriteString( value ) out << (value);
#define WriteUncompressedUnsignedInteger( value ) writeUncompressedUnsignedInteger(out, (uint32_t)(value));
#define WriteUncompressedBlock( value, count ) out.write((char *)(value),(count));
#define SerializeFileStructureUncompressedUniqueId( value ) (value).serializeFileStructureUncompressedUniqueId(out);
#define SerializeCompressedUniqueId( value ) (value).serializeCompressedUniqueId(out);
#define SerializeContentPRCBase write(out);
#define SerializeRgbColor( value ) (value).serializeRgbColor(out);
#define SerializePicture( value ) (value).serializePicture(out);
#define SerializeTextureDefinition( value ) (value)->serializeTextureDefinition(out);
#define SerializeMarkup( value ) (value)->serializeMarkup(out);
#define SerializeAnnotationEntity( value ) (value)->serializeAnnotationEntity(out);
#define SerializeFontKeysSameFont( value ) (value).serializeFontKeysSameFont(out);
#define SerializeMaterial( value ) (value)->serializeMaterial(out);

#define SerializeUserData UserData(0,0).write(out);
#define SerializeEmptyContentPRCBase ContentPRCBase(PRC_TYPE_ROOT_PRCBase).serializeContentPRCBase(out);
#define SerializeCategory1LineStyle( value ) (value)->serializeCategory1LineStyle(out);
#define SerializeCoordinateSystem( value ) (value)->serializeCoordinateSystem(out);
#define SerializeRepresentationItem( value ) (value)->serializeRepresentationItem(out);
#define SerializePartDefinition( value ) (value)->serializePartDefinition(out);
#define SerializeProductOccurrence( value ) (value)->serializeProductOccurrence(out);
#define SerializeContextAndBodies( value ) (value)->serializeContextAndBodies(out);
#define SerializeGeometrySummary( value ) (value)->serializeGeometrySummary(out);
#define SerializeContextGraphics( value ) (value)->serializeContextGraphics(out);
#define SerializeStartHeader serializeStartHeader(out);
#define SerializeUncompressedFiles  \
 { \
  const uint32_t number_of_uncompressed_files = uncompressed_files.size(); \
  WriteUncompressedUnsignedInteger (number_of_uncompressed_files) \
  for(PRCUncompressedFileList::const_iterator it = uncompressed_files.begin(); it != uncompressed_files.end(); it++) \
  { \
    WriteUncompressedUnsignedInteger ((*it)->file_size) \
    WriteUncompressedBlock ((*it)->data, (*it)->file_size) \
  } \
 }
#define SerializeModelFileData serializeModelFileData(modelFile_out); modelFile_out.compress();
#define SerializeUnit( value ) (value).serializeUnit(out);

using std::string;
using namespace std;

namespace prc {
// Map [0,1] to [0,255]
inline uint8_t byte(double r) 
{
  if(r < 0.0) r=0.0;
  else if(r > 1.0) r=1.0;
  int a=(int)(256.0*r);
  if(a == 256) a=255;
  return a;
}
}

void PRCFileStructure::serializeFileStructureGlobals(PRCbitStream &out)
{
  // even though this is technically not part of this section,
  // it is handled here for convenience
  const uint32_t number_of_schema = 0;
  WriteUnsignedInteger (number_of_schema)

  WriteUnsignedInteger (PRC_TYPE_ASM_FileStructureGlobals)

  PRCSingleAttribute sa((int32_t)PRCVersion);
  PRCAttribute a("__PRC_RESERVED_ATTRIBUTE_PRCInternalVersion");
  a.addKey(sa);
  ContentPRCBase cb(PRC_TYPE_ROOT_PRCBase);
  cb.addAttribute(a);
  cb.serializeContentPRCBase(out);
  WriteUnsignedInteger (number_of_referenced_file_structures)
  // SerializeFileStructureInternalGlobalData
  WriteDouble (tessellation_chord_height_ratio)
  WriteDouble (tessellation_angle_degree)

  // SerializeMarkupSerializationHelper
  WriteString (default_font_family_name)

  const uint32_t number_of_fonts = font_keys_of_font.size();
  WriteUnsignedInteger (number_of_fonts)
  for (uint32_t i=0;i<number_of_fonts;i++)
  {
    SerializeFontKeysSameFont (font_keys_of_font[i])
  }

  const uint32_t number_of_colors = colors.size();
  WriteUnsignedInteger (number_of_colors)
  for (uint32_t i=0;i<number_of_colors;i++)
      SerializeRgbColor (colors[i])

  const uint32_t number_of_pictures = pictures.size();
  WriteUnsignedInteger (number_of_pictures)
  for (uint32_t i=0;i<number_of_pictures;i++)
     SerializePicture (pictures[i])

  const uint32_t number_of_texture_definitions = texture_definitions.size();
  WriteUnsignedInteger (number_of_texture_definitions)
  for (uint32_t i=0;i<number_of_texture_definitions;i++)
     SerializeTextureDefinition (texture_definitions[i])

  const uint32_t number_of_materials = materials.size();
  WriteUnsignedInteger (number_of_materials)
  for (uint32_t i=0;i<number_of_materials;i++)
     SerializeMaterial (materials[i])

  // number of line patterns hard coded for now
  const uint32_t number_of_line_patterns = 1;
  WriteUnsignedInteger (number_of_line_patterns)
  PRCLinePattern().serializeLinePattern(out);

  const uint32_t number_of_styles = styles.size();
  WriteUnsignedInteger (number_of_styles)
  for (uint32_t i=0;i<number_of_styles;i++)
     SerializeCategory1LineStyle (styles[i])

  const uint32_t number_of_fill_patterns = 0;
  WriteUnsignedInteger (number_of_fill_patterns)

  const uint32_t number_of_reference_coordinate_systems = reference_coordinate_systems.size();
  WriteUnsignedInteger (number_of_reference_coordinate_systems)
  for (uint32_t i=0;i<number_of_reference_coordinate_systems;i++)
     SerializeCoordinateSystem (reference_coordinate_systems[i])

  SerializeUserData
}

void PRCFileStructure::serializeFileStructureTree(PRCbitStream &out)
{
  WriteUnsignedInteger (PRC_TYPE_ASM_FileStructureTree)

  SerializeEmptyContentPRCBase

  const uint32_t number_of_part_definitions = part_definitions.size();
  WriteUnsignedInteger (number_of_part_definitions)
  for (uint32_t i=0;i<number_of_part_definitions;i++)
    SerializePartDefinition (part_definitions[i])
	
  const uint32_t number_of_product_occurrences = product_occurrences.size();
  WriteUnsignedInteger (number_of_product_occurrences)
  for (uint32_t i=0;i<number_of_product_occurrences;i++)
  {
    product_occurrences[i]->unit_information.unit_from_CAD_file = true;
    product_occurrences[i]->unit_information.unit = unit;
    SerializeProductOccurrence (product_occurrences[i])
  }

  // SerializeFileStructureInternalData
  WriteUnsignedInteger (PRC_TYPE_ASM_FileStructure)
  SerializeEmptyContentPRCBase
  const uint32_t next_available_index = makePRCID();
  WriteUnsignedInteger (next_available_index)
  const uint32_t index_product_occurence = number_of_product_occurrences;  // Asymptote (oPRCFile) specific - we write the root product last
  WriteUnsignedInteger (index_product_occurence)

  SerializeUserData
}

void PRCFileStructure::serializeFileStructureTessellation(PRCbitStream &out)
{
  WriteUnsignedInteger (PRC_TYPE_ASM_FileStructureTessellation)

  SerializeEmptyContentPRCBase
  const uint32_t number_of_tessellations = tessellations.size();
  WriteUnsignedInteger (number_of_tessellations)
  for (uint32_t i=0;i<number_of_tessellations;i++)
    tessellations[i]->serializeBaseTessData(out);

  SerializeUserData
}

void PRCFileStructure::serializeFileStructureGeometry(PRCbitStream &out)
{
  WriteUnsignedInteger (PRC_TYPE_ASM_FileStructureGeometry)

  SerializeEmptyContentPRCBase
  const uint32_t number_of_contexts = contexts.size();
  WriteUnsignedInteger (number_of_contexts)
  for (uint32_t i=0;i<number_of_contexts;i++)
    SerializeContextAndBodies (contexts[i])

  SerializeUserData
}

void PRCFileStructure::serializeFileStructureExtraGeometry(PRCbitStream &out)
{
  WriteUnsignedInteger (PRC_TYPE_ASM_FileStructureExtraGeometry)

  SerializeEmptyContentPRCBase
  const uint32_t number_of_contexts = contexts.size();
  WriteUnsignedInteger (number_of_contexts)
  for (uint32_t i=0;i<number_of_contexts;i++)
  {
     SerializeGeometrySummary (contexts[i])
     SerializeContextGraphics (contexts[i])
  }

  SerializeUserData
}

void oPRCFile::serializeModelFileData(PRCbitStream &out)
{
  // even though this is technically not part of this section,
  // it is handled here for convenience
  const uint32_t number_of_schema = 0;
  WriteUnsignedInteger (number_of_schema)
  WriteUnsignedInteger (PRC_TYPE_ASM_ModelFile)

  PRCSingleAttribute sa((int32_t)PRCVersion);
  PRCAttribute a("__PRC_RESERVED_ATTRIBUTE_PRCInternalVersion");
  a.addKey(sa);
  ContentPRCBase cb(PRC_TYPE_ROOT_PRCBase,"PRC file");
  cb.addAttribute(a);
  cb.serializeContentPRCBase(out);

  SerializeUnit (unit)

  out << (uint32_t)1; // 1 product occurrence
  //UUID
  SerializeCompressedUniqueId( fileStructures[0]->file_structure_uuid )
  // index+1
  out << (uint32_t)fileStructures[0]->product_occurrences.size();
  // active
  out << true;
  out << (uint32_t)0; // index in model file

  SerializeUserData
}

void makeFileUUID(PRCUniqueId& UUID)
{
  // make a UUID
  static uint32_t count = 0;
  ++count;
  // the minimum requirement on UUIDs is that all must be unique in the file
  UUID.id0 = 0x33595341; // some constant
  UUID.id1 = (uint32_t)time(NULL); // the time
  UUID.id2 = count;
  UUID.id3 = 0xa5a55a5a; // Something random, not seeded by the time, would be nice. But for now, a constant
  // maybe add something else to make it more unique
  // so multiple files can be combined
  // a hash of some data perhaps?
}

void makeAppUUID(PRCUniqueId& UUID)
{
  UUID.id0 = UUID.id1 = UUID.id2 = UUID.id3 = 0;
}

void PRCUncompressedFile::write(ostream &out) const
{
  if(data!=NULL)
  {
    WriteUncompressedUnsignedInteger (file_size)
    out.write((char*)data,file_size);
  }
}

uint32_t PRCUncompressedFile::getSize() const
{
  return sizeof(file_size)+file_size;
}


void PRCStartHeader::serializeStartHeader(ostream &out) const
{
  WriteUncompressedBlock ("PRC",3)
  WriteUncompressedUnsignedInteger (minimal_version_for_read)
  WriteUncompressedUnsignedInteger (authoring_version)
  SerializeFileStructureUncompressedUniqueId( file_structure_uuid );
  SerializeFileStructureUncompressedUniqueId( application_uuid );
}

uint32_t PRCStartHeader::getStartHeaderSize() const
{
  return 3+(2+2*4)*sizeof(uint32_t);
}


void PRCFileStructure::write(ostream &out)
{
  // SerializeFileStructureHeader
  SerializeStartHeader
  SerializeUncompressedFiles
  globals_out.write(out);
  tree_out.write(out);
  tessellations_out.write(out);
  geometry_out.write(out);
  extraGeometry_out.write(out);
}

#define SerializeFileStructureGlobals serializeFileStructureGlobals(globals_out); globals_out.compress(); sizes[1]=globals_out.getSize();
#define SerializeFileStructureTree serializeFileStructureTree(tree_out); tree_out.compress(); sizes[2]=tree_out.getSize();
#define SerializeFileStructureTessellation serializeFileStructureTessellation(tessellations_out); tessellations_out.compress(); sizes[3]=tessellations_out.getSize();
#define SerializeFileStructureGeometry serializeFileStructureGeometry(geometry_out); geometry_out.compress(); sizes[4]=geometry_out.getSize();
#define SerializeFileStructureExtraGeometry serializeFileStructureExtraGeometry(extraGeometry_out); extraGeometry_out.compress(); sizes[5]=extraGeometry_out.getSize();
#define FlushSerialization resetGraphicsAndName();
void PRCFileStructure::prepare()
{
  uint32_t size = 0;
  size += getStartHeaderSize();
  size += sizeof(uint32_t);
  for(PRCUncompressedFileList::const_iterator it = uncompressed_files.begin(); it != uncompressed_files.end(); it++)
    size += (*it)->getSize();
  sizes[0]=size;

  SerializeFileStructureGlobals
  FlushSerialization

  SerializeFileStructureTree
  FlushSerialization

  SerializeFileStructureTessellation
  FlushSerialization

  SerializeFileStructureGeometry
  FlushSerialization

  SerializeFileStructureExtraGeometry
  FlushSerialization
}

uint32_t PRCFileStructure::getSize()
{
  uint32_t size = 0;
  for(size_t i=0; i<6; i++)
    size += sizes[i];
  return size;
}


void PRCFileStructureInformation::write(ostream &out)
{
  SerializeFileStructureUncompressedUniqueId( UUID );

  WriteUncompressedUnsignedInteger (reserved)
  WriteUncompressedUnsignedInteger (number_of_offsets)
  for(uint32_t i = 0; i < number_of_offsets; ++i)
  {
    WriteUncompressedUnsignedInteger (offsets[i])
  }
}

uint32_t PRCFileStructureInformation::getSize()
{
  return (4+2+number_of_offsets)*sizeof(uint32_t);
}

void PRCHeader::write(ostream &out)
{
  SerializeStartHeader
  WriteUncompressedUnsignedInteger (number_of_file_structures)
  for(uint32_t i = 0; i < number_of_file_structures; ++i)
  {
    fileStructureInformation[i].write(out);
  }
  WriteUncompressedUnsignedInteger (model_file_offset)
  WriteUncompressedUnsignedInteger (file_size)
  SerializeUncompressedFiles
}

uint32_t PRCHeader::getSize()
{
  uint32_t size = getStartHeaderSize() + sizeof(uint32_t);
  for(uint32_t i = 0; i < number_of_file_structures; ++i)
    size += fileStructureInformation[i].getSize();
  size += 3*sizeof(uint32_t);
  for(PRCUncompressedFileList::const_iterator it = uncompressed_files.begin(); it != uncompressed_files.end(); it++)
    size += (*it)->getSize();
  return size;
}

void oPRCFile::doGroup(PRCgroup& group)
{
    const string& name = group.name;

    PRCProductOccurrence*& product_occurrence        = group.product_occurrence;
    PRCProductOccurrence*& parent_product_occurrence = group.parent_product_occurrence;
    PRCPartDefinition*& part_definition              = group.part_definition;
    PRCPartDefinition*& parent_part_definition       = group.parent_part_definition;

    if(group.options.tess)
    {
      if(!group.lines.empty())
      {
        for(PRCtesslineMap::const_iterator wit=group.lines.begin(); wit!=group.lines.end(); wit++)
        {
          bool same_color = true;
          const PRCtesslineList& lines = wit->second;
          const PRCRgbColor &color = lines.front().color;
          for(PRCtesslineList::const_iterator lit=lines.begin(); lit!=lines.end(); lit++)
            if(color!=lit->color)
            {
              same_color = false;
              break;
            }
          map<PRCVector3d,uint32_t> points;
          PRC3DWireTess *tess = new PRC3DWireTess();
          if(!same_color)
          {
            tess->is_segment_color = true;
            tess->is_rgba = false;
          }
          for(PRCtesslineList::const_iterator lit=lines.begin(); lit!=lines.end(); lit++)
          {
            tess->wire_indexes.push_back(lit->point.size());
            for(uint32_t i=0; i<lit->point.size(); i++)
            {
              map<PRCVector3d,uint32_t>::iterator pPoint = points.find(lit->point[i]);
              if(pPoint!=points.end())
                tess->wire_indexes.push_back(pPoint->second);
              else
              {
                const uint32_t point_index = tess->coordinates.size();
                points.insert(make_pair(lit->point[i],point_index));
                tess->wire_indexes.push_back(point_index);
                tess->coordinates.push_back(lit->point[i].x);
                tess->coordinates.push_back(lit->point[i].y);
                tess->coordinates.push_back(lit->point[i].z);
              }
              if(!same_color && i>0)
              {
                tess->rgba_vertices.push_back(prc::byte(lit->color.red));
                tess->rgba_vertices.push_back(prc::byte(lit->color.green));
                tess->rgba_vertices.push_back(prc::byte(lit->color.blue));
              }
            }
          }
          const uint32_t tess_index = add3DWireTess(tess);
          PRCPolyWire *polyWire = new PRCPolyWire();
          polyWire->index_tessellation = tess_index;
          if(same_color)
            polyWire->index_of_line_style = addColourWidth(RGBAColour(color.red,color.green,color.blue),wit->first);
          else
            polyWire->index_of_line_style = addColourWidth(RGBAColour(1,1,1),wit->first);
          part_definition->addPolyWire(polyWire);
        }
      }
//    make rectangles pairs of triangles in a tesselation
      if(!group.rectangles.empty())
      {
        bool same_color = true;
        const uint32_t &style = group.rectangles.front().style;
        for(PRCtessrectangleList::const_iterator rit=group.rectangles.begin(); rit!=group.rectangles.end(); rit++)
          if(style!=rit->style)
          {
            same_color = false;
            break;
          }
        map<PRCVector3d,uint32_t> points;
        PRC3DTess *tess = new PRC3DTess();
        tess->crease_angle = group.options.crease_angle;
        PRCTessFace *tessFace = new PRCTessFace();
        tessFace->used_entities_flag=PRC_FACETESSDATA_Triangle;
        uint32_t triangles = 0;
        for(PRCtessrectangleList::const_iterator rit=group.rectangles.begin(); rit!=group.rectangles.end(); rit++)
        {
          const bool degenerate = (rit->vertices[0]==rit->vertices[1]);
          uint32_t vertex_indices[4];
          for(size_t i = (degenerate?1:0); i < 4; ++i)
          {
            map<PRCVector3d,uint32_t>::const_iterator pPoint = points.find(rit->vertices[i]);
            if(pPoint!=points.end())
              vertex_indices[i] =  pPoint->second;
            else
            {
              points.insert(make_pair(rit->vertices[i],(vertex_indices[i] = tess->coordinates.size())));
              tess->coordinates.push_back(rit->vertices[i].x);
              tess->coordinates.push_back(rit->vertices[i].y);
              tess->coordinates.push_back(rit->vertices[i].z);
            }
          }
          if(degenerate)
          {
            tess->triangulated_index.push_back(vertex_indices[1]);
            tess->triangulated_index.push_back(vertex_indices[2]);
            tess->triangulated_index.push_back(vertex_indices[3]);
            triangles++;
            if(!same_color)
              tessFace->line_attributes.push_back(rit->style);
          }
          else
          {
            tess->triangulated_index.push_back(vertex_indices[0]);
            tess->triangulated_index.push_back(vertex_indices[2]);
            tess->triangulated_index.push_back(vertex_indices[3]);
            triangles++;
            if(!same_color)
              tessFace->line_attributes.push_back(rit->style);
            tess->triangulated_index.push_back(vertex_indices[3]);
            tess->triangulated_index.push_back(vertex_indices[1]);
            tess->triangulated_index.push_back(vertex_indices[0]);
            triangles++;
            if(!same_color)
              tessFace->line_attributes.push_back(rit->style);
          }
        }
        tessFace->sizes_triangulated.push_back(triangles);
        tess->addTessFace(tessFace);
        const uint32_t tess_index = add3DTess(tess);
        PRCPolyBrepModel *polyBrepModel = new PRCPolyBrepModel();
        polyBrepModel->index_tessellation = tess_index;
        polyBrepModel->is_closed = group.options.closed;
        if(same_color)
          polyBrepModel->index_of_line_style = style;
        part_definition->addPolyBrepModel(polyBrepModel);
      }
    }

    if(!group.quads.empty())
    {
      map<PRCVector3d,uint32_t> points;
      PRC3DTess *tess = new PRC3DTess();
      tess->crease_angle = group.options.crease_angle;
      PRCTessFace *tessFace = new PRCTessFace();
      tessFace->used_entities_flag=PRC_FACETESSDATA_Triangle;
      uint32_t triangles = 0;

      tessFace->is_rgba = false;
      for(PRCtessquadList::const_iterator qit=group.quads.begin(); qit!=group.quads.end(); qit++)
      {
        const RGBAColour* C = qit->colours;
        if(C[0].A != 1.0 || C[1].A != 1.0 || C[2].A != 1.0 || C[3].A != 1.0)
        {
          tessFace->is_rgba = true;
          break;
        }
      }
      bool same_colour = true;
      const RGBAColour& colour = group.quads.front().colours[0];
      for(PRCtessquadList::const_iterator qit=group.quads.begin(); qit!=group.quads.end(); qit++)
      {
        const RGBAColour* C = qit->colours;
        if(colour!=C[0] || colour!=C[1] || colour!=C[2] || colour!=C[3])
        {
          same_colour = false;
          break;
        }
      }
      
      for(PRCtessquadList::const_iterator qit=group.quads.begin(); qit!=group.quads.end(); qit++)
      {
        const RGBAColour* C = qit->colours;
        const bool degenerate = (qit->vertices[0]==qit->vertices[1]);
        uint32_t vertex_indices[4];
        for(size_t i = (degenerate?1:0); i < 4; ++i)
        {
          map<PRCVector3d,uint32_t>::const_iterator pPoint = points.find(qit->vertices[i]);
          if(pPoint!=points.end())
            vertex_indices[i] =  pPoint->second;
          else
          {
            points.insert(make_pair(qit->vertices[i],(vertex_indices[i] = tess->coordinates.size())));
            tess->coordinates.push_back(qit->vertices[i].x);
            tess->coordinates.push_back(qit->vertices[i].y);
            tess->coordinates.push_back(qit->vertices[i].z);
          }
        }
        if(degenerate)
        {
          tess->triangulated_index.push_back(vertex_indices[1]);
          tess->triangulated_index.push_back(vertex_indices[2]);
          tess->triangulated_index.push_back(vertex_indices[3]);
          triangles++;
          if(!same_colour)
          {
            tessFace->rgba_vertices.push_back(prc::byte(C[1].R));
            tessFace->rgba_vertices.push_back(prc::byte(C[1].G));
            tessFace->rgba_vertices.push_back(prc::byte(C[1].B));
            if(tessFace->is_rgba)
              tessFace->rgba_vertices.push_back(prc::byte(C[1].A));
            
            tessFace->rgba_vertices.push_back(prc::byte(C[2].R));
            tessFace->rgba_vertices.push_back(prc::byte(C[2].G));
            tessFace->rgba_vertices.push_back(prc::byte(C[2].B));
            if(tessFace->is_rgba)
              tessFace->rgba_vertices.push_back(prc::byte(C[2].A));
            
            tessFace->rgba_vertices.push_back(prc::byte(C[3].R));
            tessFace->rgba_vertices.push_back(prc::byte(C[3].G));
            tessFace->rgba_vertices.push_back(prc::byte(C[3].B));
            if(tessFace->is_rgba)
              tessFace->rgba_vertices.push_back(prc::byte(C[3].A));
          }
        }
        else
        {
          tess->triangulated_index.push_back(vertex_indices[0]);
          tess->triangulated_index.push_back(vertex_indices[2]);
          tess->triangulated_index.push_back(vertex_indices[3]);
          triangles++;
          if(!same_colour)
          {
            tessFace->rgba_vertices.push_back(prc::byte(C[0].R));
            tessFace->rgba_vertices.push_back(prc::byte(C[0].G));
            tessFace->rgba_vertices.push_back(prc::byte(C[0].B));
            if(tessFace->is_rgba)
              tessFace->rgba_vertices.push_back(prc::byte(C[0].A));
            
            tessFace->rgba_vertices.push_back(prc::byte(C[2].R));
            tessFace->rgba_vertices.push_back(prc::byte(C[2].G));
            tessFace->rgba_vertices.push_back(prc::byte(C[2].B));
            if(tessFace->is_rgba)
              tessFace->rgba_vertices.push_back(prc::byte(C[2].A));
            
            tessFace->rgba_vertices.push_back(prc::byte(C[3].R));
            tessFace->rgba_vertices.push_back(prc::byte(C[3].G));
            tessFace->rgba_vertices.push_back(prc::byte(C[3].B));
            if(tessFace->is_rgba)
              tessFace->rgba_vertices.push_back(prc::byte(C[3].A));
          }
          tess->triangulated_index.push_back(vertex_indices[3]);
          tess->triangulated_index.push_back(vertex_indices[1]);
          tess->triangulated_index.push_back(vertex_indices[0]);
          triangles++;
          if(!same_colour)
          {
            tessFace->rgba_vertices.push_back(prc::byte(C[3].R));
            tessFace->rgba_vertices.push_back(prc::byte(C[3].G));
            tessFace->rgba_vertices.push_back(prc::byte(C[3].B));
            if(tessFace->is_rgba)
              tessFace->rgba_vertices.push_back(prc::byte(C[3].A));
          
            tessFace->rgba_vertices.push_back(prc::byte(C[1].R));
            tessFace->rgba_vertices.push_back(prc::byte(C[1].G));
            tessFace->rgba_vertices.push_back(prc::byte(C[1].B));
            if(tessFace->is_rgba)
              tessFace->rgba_vertices.push_back(prc::byte(C[1].A));
          
            tessFace->rgba_vertices.push_back(prc::byte(C[0].R));
            tessFace->rgba_vertices.push_back(prc::byte(C[0].G));
            tessFace->rgba_vertices.push_back(prc::byte(C[0].B));
            if(tessFace->is_rgba)
              tessFace->rgba_vertices.push_back(prc::byte(C[0].A));
          }
        }
      }
      tessFace->sizes_triangulated.push_back(triangles);
      tess->addTessFace(tessFace);
      const uint32_t tess_index = add3DTess(tess);
      PRCPolyBrepModel *polyBrepModel = new PRCPolyBrepModel();
      polyBrepModel->index_tessellation = tess_index;
      polyBrepModel->is_closed = group.options.closed;
      if(same_colour)
        polyBrepModel->index_of_line_style = addColour(colour);
      part_definition->addPolyBrepModel(polyBrepModel);
    }

    if(!group.points.empty())
    {
      for(PRCpointsetMap::const_iterator pit=group.points.begin(); pit!=group.points.end(); pit++)
      {
        PRCPointSet *pointset = new PRCPointSet();
        pointset->index_of_line_style = pit->first;
        pointset->point = pit->second;
        part_definition->addPointSet(pointset);
      }
    }

    if(!group.pointsets.empty())
    {
      for(std::vector<PRCPointSet*>::iterator pit=group.pointsets.begin(); pit!=group.pointsets.end(); pit++)
      {
        part_definition->addPointSet(*pit);
      }
    }

    if(!group.polymodels.empty())
    {
      for(std::vector<PRCPolyBrepModel*>::iterator pit=group.polymodels.begin(); pit!=group.polymodels.end(); pit++)
      {
        (*pit)->is_closed = group.options.closed;
        part_definition->addPolyBrepModel(*pit);
      }
    }

    if(!group.polywires.empty())
    {
      for(std::vector<PRCPolyWire*>::iterator pit=group.polywires.begin(); pit!=group.polywires.end(); pit++)
      {
        part_definition->addPolyWire(*pit);
      }
    }

    if(!group.wires.empty())
    {
      PRCTopoContext *wireContext = NULL;
      const uint32_t context_index = getTopoContext(wireContext);
      for(PRCwireList::iterator wit=group.wires.begin(); wit!=group.wires.end(); wit++)
      {
        PRCWireEdge *wireEdge = new PRCWireEdge;
        wireEdge->curve_3d = wit->curve;
        PRCSingleWireBody *wireBody = new PRCSingleWireBody;
        wireBody->setWireEdge(wireEdge);
        const uint32_t wire_body_index = wireContext->addSingleWireBody(wireBody);
        PRCWire *wire = new PRCWire();
        wire->index_of_line_style = wit->style;
        wire->context_id = context_index;
        wire->body_id = wire_body_index;
        if(wit->transform)
            wire->index_local_coordinate_system = addTransform(wit->transform);
        part_definition->addWire(wire);
      }
    }

    PRCfaceList &faces = group.faces;
    if(!faces.empty())
    {
      bool same_color = true;
      const uint32_t style = faces.front().style;
      for(PRCfaceList::const_iterator fit=faces.begin(); fit!=faces.end(); fit++)
        if(style!=fit->style)
        {
          same_color = false;
          break;
        }
      PRCTopoContext *context = NULL;
      const uint32_t context_index = getTopoContext(context);
      context->granularity = group.options.granularity;
   // Acrobat 9 also does the following:
   // context->tolerance = group.options.granularity;
   // context->have_smallest_face_thickness = true;
   // context->smallest_thickness = group.options.granularity;
      PRCShell *shell = new PRCShell;

      for(PRCfaceList::iterator fit=faces.begin(); fit!=faces.end(); fit++)
      {
        if(fit->transform || group.options.do_break ||
           (fit->transparent && !group.options.no_break))
        {
          PRCShell *shell = new PRCShell;
          shell->addFace(fit->face);
          PRCConnex *connex = new PRCConnex;
          connex->addShell(shell);
          PRCBrepData *body = new PRCBrepData;
          body->addConnex(connex);
          const uint32_t body_index = context->addBrepData(body);

          PRCBrepModel *brepmodel = new PRCBrepModel();
          brepmodel->index_of_line_style = fit->style;
          brepmodel->context_id = context_index;
          brepmodel->body_id = body_index;
          brepmodel->is_closed = group.options.closed;

          brepmodel->index_local_coordinate_system = addTransform(fit->transform);

          part_definition->addBrepModel(brepmodel);
        }
        else
        {
          if(!same_color)
            fit->face->index_of_line_style = fit->style;
          shell->addFace(fit->face);
        }
      }
      if(shell->face.empty())
      {
        delete shell;
      }
      else
      {
        PRCConnex *connex = new PRCConnex;
        connex->addShell(shell);
        PRCBrepData *body = new PRCBrepData;
        body->addConnex(connex);
        const uint32_t body_index = context->addBrepData(body);
        PRCBrepModel *brepmodel = new PRCBrepModel();
        if(same_color)
          brepmodel->index_of_line_style = style;
        brepmodel->context_id = context_index;
        brepmodel->body_id = body_index;
        brepmodel->is_closed = group.options.closed;
        part_definition->addBrepModel(brepmodel);
      }
    }

    PRCcompfaceList &compfaces = group.compfaces;
    if(!compfaces.empty())
    {
      bool same_color = true;
      const uint32_t style = compfaces.front().style;
      for(PRCcompfaceList::const_iterator fit=compfaces.begin(); fit!=compfaces.end(); fit++)
        if(style!=fit->style)
        {
          same_color = false;
          break;
        }
      PRCTopoContext *context = NULL;
      const uint32_t context_index = getTopoContext(context);
      PRCCompressedBrepData *body = new PRCCompressedBrepData;

      body->serial_tolerance=group.options.compression;
      body->brep_data_compressed_tolerance=0.1*group.options.compression;

      for(PRCcompfaceList::const_iterator fit=compfaces.begin(); fit!=compfaces.end(); fit++)
      {
        if(group.options.do_break ||
           (fit->transparent && !group.options.no_break))
        {
          PRCCompressedBrepData *body = new PRCCompressedBrepData;
          body->face.push_back(fit->face);

          body->serial_tolerance=group.options.compression;
          body->brep_data_compressed_tolerance=2.8346456*
            group.options.compression;
          const uint32_t body_index = context->addCompressedBrepData(body);

          PRCBrepModel *brepmodel = new PRCBrepModel();
          brepmodel->index_of_line_style = fit->style;
          brepmodel->context_id = context_index;
          brepmodel->body_id = body_index;
          brepmodel->is_closed = group.options.closed;

          part_definition->addBrepModel(brepmodel);
        }
        else
        {
          if(!same_color)
            fit->face->index_of_line_style = fit->style;
          body->face.push_back(fit->face);
        }
      }
      if(body->face.empty())
      {
        delete body;
      }
      else
      {
        const uint32_t body_index = context->addCompressedBrepData(body);
        PRCBrepModel *brepmodel = new PRCBrepModel();
        if(same_color)
          brepmodel->index_of_line_style = style;
        brepmodel->context_id = context_index;
        brepmodel->body_id = body_index;
        brepmodel->is_closed = group.options.closed;
        part_definition->addBrepModel(brepmodel);
      }
    }

    // Simplify and reduce to as simple entities as possible
    // products with named representation items can not be reduced to sets, since 
    // outside references are already set
    bool nonamedparts = true;
    for(PRCRepresentationItemList::const_iterator it=part_definition->representation_item.begin(); it!=part_definition->representation_item.end(); it++)
    {
      if (!(*it)->name.empty())
      {
        nonamedparts = false;
        break;
    }
    }
    lastgroupname.clear();
    lastgroupnames.clear();
    // First option - reduce to one element in parent
    if (parent_part_definition && product_occurrence->index_son_occurrence.empty() &&
        part_definition->representation_item.size() == 1 &&
        ( name.empty() || part_definition->representation_item.front()->name.empty() ) &&
        ( !group.transform  || part_definition->representation_item.front()->index_local_coordinate_system==m1) )
    {
      if(part_definition->representation_item.front()->name.empty() )
        part_definition->representation_item.front()->name = name;
      if(part_definition->representation_item.front()->index_local_coordinate_system==m1)
        part_definition->representation_item.front()->index_local_coordinate_system = addTransform(group.transform);
      lastgroupname = calculate_unique_name(part_definition->representation_item.front(), parent_product_occurrence);
      parent_part_definition->addRepresentationItem(part_definition->representation_item.front());
      part_definition->representation_item.clear();
      delete product_occurrence; product_occurrence = NULL;
      delete part_definition; part_definition = NULL;
    }
    // Second option - reduce to a set
    else if (parent_part_definition && product_occurrence->index_son_occurrence.empty() &&
      !part_definition->representation_item.empty() &&
      !group.options.do_break && nonamedparts)
    {
      PRCSet *set = new PRCSet(name);
      set->index_local_coordinate_system = addTransform(group.transform);
      lastgroupname = calculate_unique_name(set, parent_product_occurrence);
      for(PRCRepresentationItemList::iterator it=part_definition->representation_item.begin(); it!=part_definition->representation_item.end(); it++)
      {
        lastgroupnames.push_back(calculate_unique_name(*it, parent_product_occurrence));
        set->addRepresentationItem(*it);
      }
      part_definition->representation_item.clear();
      parent_part_definition->addSet(set);
      delete product_occurrence; product_occurrence = NULL;
      delete part_definition; part_definition = NULL;
    }
    // Third option - create product
    else if ( !product_occurrence->index_son_occurrence.empty() || !part_definition->representation_item.empty())
    {
      // if everything is enclosed in one group - drop the root group
      if (parent_product_occurrence == NULL && group.transform == NULL &&
          part_definition->representation_item.empty() && product_occurrence->index_son_occurrence.size()==1) {
        delete part_definition; part_definition = NULL;
        delete product_occurrence; product_occurrence = NULL;
      }
      else
      {
        lastgroupname = calculate_unique_name(product_occurrence, NULL);
        if (part_definition->representation_item.empty()) {
          delete part_definition; part_definition = NULL;
        }
      else
        {
          for(PRCRepresentationItemList::const_iterator it=part_definition->representation_item.begin(); it!=part_definition->representation_item.end(); it++)
            if ((*it)->name.empty())
              lastgroupnames.push_back(calculate_unique_name(*it, product_occurrence));
        product_occurrence->index_part = addPartDefinition(part_definition);
        }
      if (group.transform) {
        product_occurrence->location = group.transform;
        group.transform = NULL;
      }
      if (parent_product_occurrence) {
        parent_product_occurrence->index_son_occurrence.push_back(addProductOccurrence(product_occurrence));
      }
      else {
        addProductOccurrence(product_occurrence);
      }
    }
    }
    // Last case - absolutely nothing to do
    else
    {
      delete product_occurrence; product_occurrence = NULL;
      delete part_definition; part_definition = NULL;
    }

}

std::string oPRCFile::calculate_unique_name(const ContentPRCBase *prc_entity,const ContentPRCBase *prc_occurence)
{
  std::stringstream ss (std::stringstream::in | std::stringstream::out);
  uint8_t *serialization_buffer = NULL;
  PRCbitStream serialization(serialization_buffer,0u);
  const PRCFileStructure *pfile_structure = fileStructures[0];
  const PRCUniqueId& uuid = pfile_structure->file_structure_uuid;
// ConvertUniqueIdentifierToString (prc_entity)
// SerializeCompressedUniqueId (file_structure)
  serialization << uuid.id0 << uuid.id1 << uuid.id2 << uuid.id3;
// WriteUnsignedInteger (type)
  serialization << prc_entity->getType();
// WriteUnsignedInteger (unique_identifier)
  serialization << prc_entity->getPRCID();
  if (prc_occurence)
  {
// serialization_buffer = Flush serialization (serialization) 
  {
    const uint32_t size_serialization = serialization.getSize();
    while(size_serialization == serialization.getSize())
      serialization << false;
  }
// ConvertUniqueIdentifierToString (prc_occurrence_unique_id)
// SerializeCompressedUniqueId (file_structure)
    serialization << uuid.id0 << uuid.id1 << uuid.id2 << uuid.id3;
// WriteUnsignedInteger (type)
    serialization << (uint32_t)PRC_TYPE_ASM_ProductOccurence;
// WriteUnsignedInteger (unique_identifier)
    serialization << prc_occurence->getPRCID();
  }
  ss << (prc_entity->name.empty()?"node":prc_entity->name) << '.';
  const uint32_t size_serialization = serialization.getSize();
  for(size_t j=0; j<size_serialization; j++)
    ss << hex << setfill('0') << setw(2) << (uint32_t)(serialization_buffer[j]);

  return ss.str();
}

bool oPRCFile::finish()
{
  if(groups.size()!=1) {
    fputs("begingroup without matching endgroup",stderr);
    exit(1);
  }
  doGroup(groups.top());

  // write each section's bit data
  fileStructures[0]->prepare();
  SerializeModelFileData

  // create the header

  // fill out enough info so that sizes can be computed correctly
  header.number_of_file_structures = number_of_file_structures;
  header.fileStructureInformation = new PRCFileStructureInformation[number_of_file_structures];
  for(uint32_t i = 0; i < number_of_file_structures; ++i)
  {
    header.fileStructureInformation[i].UUID = fileStructures[i]->file_structure_uuid;
    header.fileStructureInformation[i].reserved = 0;
    header.fileStructureInformation[i].number_of_offsets = 6;
    header.fileStructureInformation[i].offsets = new uint32_t[6];
  }

  header.minimal_version_for_read = PRCVersion;
  header.authoring_version = PRCVersion;
  makeFileUUID(header.file_structure_uuid);
  makeAppUUID(header.application_uuid);

  header.file_size = getSize();
  header.model_file_offset = header.file_size - modelFile_out.getSize();

  uint32_t currentOffset = header.getSize();

  for(uint32_t i = 0; i < number_of_file_structures; ++i)
  {
    for(size_t j=0; j<6; j++)
    {
      header.fileStructureInformation[i].offsets[j] = currentOffset;
      currentOffset += fileStructures[i]->sizes[j];
    }
  }

  // write the data
  header.write(output);

  for(uint32_t i = 0; i < number_of_file_structures; ++i)
  {
    fileStructures[i]->write(output);
  }

  modelFile_out.write(output);
  output.flush();

  for(uint32_t i = 0; i < number_of_file_structures; ++i)
    delete[] header.fileStructureInformation[i].offsets;
  delete[] header.fileStructureInformation;

  return true;
}

uint32_t oPRCFile::getSize()
{
  uint32_t size = header.getSize();

  for(uint32_t i = 0; i < number_of_file_structures; ++i)
  {
    size += fileStructures[i]->getSize();
  }

  size += modelFile_out.getSize();
  return size;
}

uint32_t PRCFileStructure::addPicture(EPRCPictureDataFormat format, uint32_t size, const uint8_t *p, uint32_t width, uint32_t height, string name)
{
  uint8_t *data = NULL;
  uint32_t components=0;
  PRCPicture picture(name);
  if(size==0 || p==NULL)
    { cerr << "image not set" << endl; return m1; }
  PRCUncompressedFile* uncompressed_file = new PRCUncompressedFile;
  if(format==KEPRCPicture_PNG || format==KEPRCPicture_JPG)
  {
    data = new uint8_t[size];
    memcpy(data, p, size);
    uncompressed_files.push_back(uncompressed_file);
    uncompressed_files.back()->file_size = size;
    uncompressed_files.back()->data = data;
    picture.format = format;
    picture.uncompressed_file_index = uncompressed_files.size()-1;
    picture.pixel_width = 0; // width and height are ignored for JPG and PNG pictures - but let us keep things clean
    picture.pixel_height = 0;
    pictures.push_back(picture);
    return pictures.size()-1;
  }

  switch(format)
  {
    case KEPRCPicture_BITMAP_RGB_BYTE:
      components = 3; break;
    case KEPRCPicture_BITMAP_RGBA_BYTE:
      components = 4; break;
    case KEPRCPicture_BITMAP_GREY_BYTE:
      components = 1; break;
    case KEPRCPicture_BITMAP_GREYA_BYTE:
      components = 2; break;
    default:
      { cerr << "unknown picture format" << endl; return m1; }
  }
      if(width==0 || height==0)
        { cerr << "width or height parameter not set" << endl; return m1; }
      if (size < width*height*components)
        { cerr << "image too small" << endl; return m1; }

      {
        uint32_t compressedDataSize = 0;
        const int CHUNK= 1024; // is this reasonable?

        z_stream strm;
        strm.zalloc = Z_NULL;
        strm.zfree = Z_NULL;
        strm.opaque = Z_NULL;
        if(deflateInit(&strm,Z_DEFAULT_COMPRESSION) != Z_OK)
          { cerr << "Compression initialization failed" << endl; return m1; }
        unsigned int sizeAvailable = deflateBound(&strm,size);
        uint8_t *compressedData = (uint8_t*) malloc(sizeAvailable);
        strm.avail_in = size;
        strm.next_in = (unsigned char*)p;
        strm.next_out = (unsigned char*)compressedData;
        strm.avail_out = sizeAvailable;

        int code;
        unsigned int chunks = 0;
        while((code = deflate(&strm,Z_FINISH)) == Z_OK)
        {
          ++chunks;
          // strm.avail_out should be 0 if we got Z_OK
          compressedDataSize = sizeAvailable - strm.avail_out;
          compressedData = (uint8_t*) realloc(compressedData,CHUNK*chunks);
          strm.next_out = (Bytef*)(compressedData + compressedDataSize);
          strm.avail_out += CHUNK;
          sizeAvailable += CHUNK;
        }
        compressedDataSize = sizeAvailable-strm.avail_out;

        if(code != Z_STREAM_END)
        {
          deflateEnd(&strm);
          free(compressedData);
          { cerr << "Compression error" << endl; return m1; }
        }

        deflateEnd(&strm);
        size = compressedDataSize;
        data = new uint8_t[compressedDataSize];
        memcpy(data, compressedData, compressedDataSize);
        free(compressedData);
      }
      uncompressed_files.push_back(uncompressed_file);
      uncompressed_files.back()->file_size = size;
      uncompressed_files.back()->data = data;
      picture.format = format;
      picture.uncompressed_file_index = uncompressed_files.size()-1;
      picture.pixel_width = width;
      picture.pixel_height = height;
      pictures.push_back(picture);
      return pictures.size()-1;
}

uint32_t PRCFileStructure::addTextureDefinition(PRCTextureDefinition*& pTextureDefinition)
{
  texture_definitions.push_back(pTextureDefinition);
  pTextureDefinition = NULL;
  return texture_definitions.size()-1;
}

uint32_t PRCFileStructure::addRgbColor(const PRCRgbColor &color)
{
  colors.push_back(color);
  return 3*(colors.size()-1);
}

uint32_t PRCFileStructure::addRgbColorUnique(const PRCRgbColor &color)
{
  for(uint32_t i = 0; i < colors.size(); ++i)
  {
    if(colors[i] == color)
      return 3*i;
  }
  colors.push_back(color);
  return 3*(colors.size()-1);
}

uint32_t oPRCFile::addColor(const PRCRgbColor &color)
{
  PRCcolorMap::const_iterator pColor = colorMap.find(color);
  if(pColor!=colorMap.end())
    return pColor->second;
//  color_index = addRgbColorUnique(color);
  const uint32_t color_index = fileStructures[0]->addRgbColor(color);
  colorMap.insert(make_pair(color,color_index));
  return color_index;
}

uint32_t oPRCFile::addColour(const RGBAColour &colour)
{
  PRCcolourMap::const_iterator pColour = colourMap.find(colour);
  if(pColour!=colourMap.end())
    return pColour->second;
  const uint32_t color_index = addColor(PRCRgbColor(colour.R, colour.G, colour.B));
  PRCStyle *style = new PRCStyle();
  style->line_width = 1.0;
  style->is_vpicture = false;
  style->line_pattern_vpicture_index = 0;
  style->is_material = false;
  style->color_material_index = color_index;
  style->is_transparency_defined = (colour.A < 1.0);
  style->transparency = (uint8_t)(colour.A * 256);
  style->additional = 0;
  const uint32_t style_index = fileStructures[0]->addStyle(style);
  colourMap.insert(make_pair(colour,style_index));
  return style_index;
}

uint32_t oPRCFile::addColourWidth(const RGBAColour &colour, double width)
{
  RGBAColourWidth colourwidth(colour.R, colour.G, colour.B, colour.A, width);
  PRCcolourwidthMap::const_iterator pColour = colourwidthMap.find(colourwidth);
  if(pColour!=colourwidthMap.end())
    return pColour->second;
  const uint32_t color_index = addColor(PRCRgbColor(colour.R, colour.G, colour.B));
  PRCStyle *style = new PRCStyle();
  style->line_width = width;
  style->is_vpicture = false;
  style->line_pattern_vpicture_index = 0;
  style->is_material = false;
  style->color_material_index = color_index;
  style->is_transparency_defined = (colour.A < 1.0);
  style->transparency = (uint8_t)(colour.A * 256);
  style->additional = 0;
  const uint32_t style_index = fileStructures[0]->addStyle(style);
  colourwidthMap.insert(make_pair(colourwidth,style_index));
  return style_index;
}

uint32_t oPRCFile::addTransform(PRCGeneralTransformation3d*& transform)
{
  if(!transform)
    return m1;
  PRCtransformMap::const_iterator pTransform = transformMap.find(*transform);
  if(pTransform!=transformMap.end())
    return pTransform->second;
  PRCCoordinateSystem *coordinateSystem = new PRCCoordinateSystem();
  bool transform_replaced = false;
  if(                         transform->M(0,1)==0 && transform->M(0,2)==0 &&
      transform->M(1,0)==0 &&                         transform->M(1,2)==0 &&
      transform->M(2,0)==0 && transform->M(2,1)==0 &&
      transform->M(3,0)==0 && transform->M(3,1)==0 && transform->M(3,2)==0 && transform->M(3,3)==1 )
  {
    transform_replaced = true;
    PRCCartesianTransformation3d *carttransform = new PRCCartesianTransformation3d;
//  if(transform->M(0,3)==0 && transform->M(1,3)==0 && transform->M(1,3)==0 &&
//     transform->M(0,0)==1 && transform->M(1,1)==1 && transform->M(2,2)==1 )
//    carttransform->behaviour = PRC_TRANSFORMATION_Identity;
    if(transform->M(0,3)!=0 || transform->M(1,3)!=0 || transform->M(2,3)!=0)
    {
      carttransform->behaviour |= PRC_TRANSFORMATION_Translate;
      carttransform->origin.Set(transform->M(0,3),transform->M(1,3),transform->M(2,3));
    }
    if(transform->M(0,0)!=transform->M(1,1) || transform->M(0,0)!=transform->M(2,2))
    {
      carttransform->behaviour |= PRC_TRANSFORMATION_NonUniformScale;
      carttransform->scale.Set(transform->M(0,0),transform->M(1,1),transform->M(2,2));
    }
    else
      if(transform->M(0,0)!=1)
      {
        carttransform->behaviour |= PRC_TRANSFORMATION_Scale;
        carttransform->uniform_scale=transform->M(0,0);
      }
    coordinateSystem->axis_set = carttransform;
  }
  else
  coordinateSystem->axis_set = transform;
  const uint32_t coordinate_system_index = fileStructures[0]->addCoordinateSystem(coordinateSystem);
  transformMap.insert(make_pair(*transform,coordinate_system_index));
  if(transform_replaced)
    delete transform;
  transform = NULL;
  return coordinate_system_index;
}

uint32_t oPRCFile::addTransform(const double* t)
{
  if(!t)
    return m1;
  PRCGeneralTransformation3d* transform = new PRCGeneralTransformation3d(t);
  return addTransform(transform);
}

uint32_t oPRCFile::addTransform(const double origin[3], const double x_axis[3], const double y_axis[3], double scale)
{
  PRCCartesianTransformation3d* transform = new PRCCartesianTransformation3d(origin, x_axis, y_axis, scale);
  if(transform->behaviour==PRC_TRANSFORMATION_Identity)
    return m1;
  PRCCoordinateSystem *coordinateSystem = new PRCCoordinateSystem();
  coordinateSystem->axis_set = transform;
  const uint32_t coordinate_system_index = fileStructures[0]->addCoordinateSystem(coordinateSystem);
  return coordinate_system_index;
}

uint32_t oPRCFile::addMaterial(const PRCmaterial& m)
{
  uint32_t material_index = m1;
  const PRCmaterialgeneric materialgeneric(m);
  PRCmaterialgenericMap::const_iterator pMaterialgeneric = materialgenericMap.find(materialgeneric);
  if(pMaterialgeneric!=materialgenericMap.end())
    material_index = pMaterialgeneric->second;
  else
{
  PRCMaterialGeneric *materialGeneric = new PRCMaterialGeneric();
  const PRCRgbColor ambient(m.ambient.R, m.ambient.G, m.ambient.B);
  materialGeneric->ambient = addColor(ambient);
    const PRCRgbColor diffuse(m.diffuse.R, m.diffuse.G, m.diffuse.B);
  materialGeneric->diffuse = addColor(diffuse);
    const PRCRgbColor emissive(m.emissive.R, m.emissive.G, m.emissive.B);

  materialGeneric->emissive = addColor(emissive);
    const PRCRgbColor specular(m.specular.R, m.specular.G, m.specular.B);
  materialGeneric->specular = addColor(specular);

    materialGeneric->shininess = m.shininess;
    materialGeneric->ambient_alpha = m.ambient.A;
    materialGeneric->diffuse_alpha = m.diffuse.A;
    materialGeneric->emissive_alpha = m.emissive.A;
    materialGeneric->specular_alpha = m.specular.A;
    material_index = addMaterialGeneric(materialGeneric);
    materialgenericMap.insert(make_pair(materialgeneric,material_index));
  }
  uint32_t color_material_index = m1;
  if(m.picture_data!=NULL)
  {
    uint32_t picture_index = m1;
    PRCpicture picture(m);
    PRCpictureMap::const_iterator pPicture = pictureMap.find(picture);
    if(pPicture!=pictureMap.end())
      picture_index = pPicture->second;
    else
    {
      picture_index = addPicture(picture);
      uint8_t* data = new uint8_t[picture.size];
      memcpy(data,picture.data,picture.size);
      picture.data = data;
      pictureMap.insert(make_pair(picture,picture_index));
    }

    uint32_t texture_definition_index = m1;
    PRCtexturedefinition texturedefinition(picture_index, m);
    PRCtexturedefinitionMap::const_iterator pTexturedefinition = texturedefinitionMap.find(texturedefinition);
    if(pTexturedefinition!=texturedefinitionMap.end())
      texture_definition_index = pTexturedefinition->second;
    else
    {
      PRCTextureDefinition *TextureDefinition = new PRCTextureDefinition;
      if (m.picture_size==216688 && m.picture_format==KEPRCPicture_JPG)
        TextureDefinition->texture_mapping_attribute=PRC_TEXTURE_MAPPING_OPACITY;
      TextureDefinition->picture_index = picture_index;
      TextureDefinition->texture_function = m.picture_replace ? KEPRCTextureFunction_Replace : KEPRCTextureFunction_Modulate;
      TextureDefinition->texture_wrapping_mode_S = m.picture_repeat ? KEPRCTextureWrappingMode_Repeat : KEPRCTextureWrappingMode_ClampToEdge;
      TextureDefinition->texture_wrapping_mode_T = m.picture_repeat ? KEPRCTextureWrappingMode_Repeat : KEPRCTextureWrappingMode_ClampToEdge;
      TextureDefinition->texture_mapping_attribute_components = (m.picture_format==KEPRCPicture_BITMAP_RGB_BYTE || m.picture_format==KEPRCPicture_JPG) ? PRC_TEXTURE_MAPPING_COMPONENTS_RGB : PRC_TEXTURE_MAPPING_COMPONENTS_RGBA;
      texture_definition_index = addTextureDefinition(TextureDefinition);
      texturedefinitionMap.insert(make_pair(texturedefinition,texture_definition_index));
    }

    uint32_t texture_application_index = m1;
    const PRCtextureapplication textureapplication(material_index, texture_definition_index);
    PRCtextureapplicationMap::const_iterator pTextureapplication = textureapplicationMap.find(textureapplication);
    if(pTextureapplication!=textureapplicationMap.end())
      texture_application_index = pTextureapplication->second;
    else
    {
      PRCTextureApplication *TextureApplication = new PRCTextureApplication;
      TextureApplication->material_generic_index = material_index;
      TextureApplication->texture_definition_index = texture_definition_index;
      texture_application_index = addTextureApplication(TextureApplication);
      textureapplicationMap.insert(make_pair(textureapplication,texture_application_index));
    }

    color_material_index = texture_application_index;
  }
  else
    color_material_index = material_index;

  uint32_t style_index = m1;
  PRCstyle style(0,m.alpha,true,color_material_index);
  PRCstyleMap::const_iterator pStyle = styleMap.find(style);
  if(pStyle!=styleMap.end())
    style_index = pStyle->second;
  else
  {
    PRCStyle *Style = new PRCStyle();
    Style->line_width = 0.0;
    Style->is_vpicture = false;
    Style->line_pattern_vpicture_index = 0;
    Style->is_material = true;
    Style->is_transparency_defined = (m.alpha < 1.0);
    Style->transparency = (uint8_t)(m.alpha * 256);
    Style->additional = 0;
    Style->color_material_index = color_material_index;
    style_index = addStyle(Style);
    styleMap.insert(make_pair(style,style_index));
  }
//  materialMap.insert(make_pair(material,style_index));
   return style_index;
}

bool isid(const double* t)
{
  return(
         t[0]==1 && t[1]==0 && t[2]==0 && t[3]==0 &&
         t[4]==0 && t[5]==1 && t[6]==0 && t[7]==0 &&
         t[8]==0 && t[9]==0 && t[10]==1 && t[11]==0 &&
         t[12]==0 && t[13]==0 && t[14]==0 && t[15]==1 );
}

void oPRCFile::begingroup(const char *name, PRCoptions *options,
                          const double* t)
{
  const PRCgroup &parent_group = groups.top();
  groups.push(PRCgroup());
  PRCgroup &group = groups.top();
  group.name=name;
  if(options) group.options=*options;
  if(t&&!isid(t))
    group.transform = new PRCGeneralTransformation3d(t);
  group.product_occurrence = new PRCProductOccurrence(name);
  group.parent_product_occurrence = parent_group.product_occurrence;
  group.part_definition = new PRCPartDefinition;
  group.parent_part_definition = parent_group.part_definition;
}

void oPRCFile::endgroup()
{
  if(groups.size()<2) {
    fputs("begingroup without matching endgroup",stderr);
    exit(1);
  }
  doGroup(groups.top());
  groups.pop();

// std::cout << lastgroupname << std::endl;
// for(std::vector<std::string>::const_iterator it=lastgroupnames.begin(); it!=lastgroupnames.end(); it++)
//   std::cout << " " << *it << std::endl;

}

PRCgroup& oPRCFile::findGroup()
{
  return groups.top();
}

#define ADDWIRE(curvtype)                                 \
  PRCgroup &group = findGroup();                          \
  group.wires.push_back(PRCwire());                       \
  PRCwire &wire = group.wires.back();                     \
  curvtype *curve = new curvtype;                         \
  wire.curve = curve;                                     \
  wire.style = addColour(c);

#define ADDFACE(surftype)                                 \
  PRCgroup &group = findGroup();                          \
  group.faces.push_back(PRCface());                       \
  PRCface& face = group.faces.back();                     \
  surftype *surface = new surftype;                       \
  face.face = new PRCFace;                                \
  face.face->base_surface = surface;                      \
  face.transparent = m.alpha < 1.0;                       \
  face.style = addMaterial(m);

#define ADDCOMPFACE                                       \
  PRCgroup &group = findGroup();                          \
  group.compfaces.push_back(PRCcompface());               \
  PRCcompface& face = group.compfaces.back();             \
  PRCCompressedFace *compface = new PRCCompressedFace;    \
  face.face = compface;                                   \
  face.transparent = m.alpha < 1.0;                       \
  face.style = addMaterial(m);

void oPRCFile::addPoint(const double P[3], const RGBAColour &c, double w)
{
  PRCgroup &group = findGroup();
  group.points[addColourWidth(c,w)].push_back(PRCVector3d(P[0],P[1],P[2]));
}

void oPRCFile::addPoints(uint32_t n, const double P[][3], const RGBAColour &c, double w)
{
  if(n==0 || P==NULL)
     return;
  PRCgroup &group = findGroup();
  PRCPointSet *pointset = new PRCPointSet();
  group.pointsets.push_back(pointset);
  pointset->index_of_line_style = addColourWidth(c,w);
  pointset->point.reserve(n);
  for(uint32_t i=0; i<n; i++)
    pointset->point.push_back(PRCVector3d(P[i][0],P[i][1],P[i][2]));
}

void oPRCFile::useMesh(uint32_t tess_index, uint32_t style_index, const double origin[3], const double x_axis[3], const double y_axis[3], double scale)
{
  PRCgroup &group = findGroup();
  PRCPolyBrepModel *polyBrepModel = new PRCPolyBrepModel();
  polyBrepModel->index_local_coordinate_system = addTransform(origin, x_axis, y_axis, scale);
  polyBrepModel->index_tessellation = tess_index;
  polyBrepModel->is_closed = group.options.closed;
  polyBrepModel->index_of_line_style = style_index;
  group.polymodels.push_back(polyBrepModel);
}

void oPRCFile::useMesh(uint32_t tess_index, uint32_t style_index, const double* t)
{
  PRCgroup &group = findGroup();
  PRCPolyBrepModel *polyBrepModel = new PRCPolyBrepModel();
  polyBrepModel->index_local_coordinate_system = addTransform(t);
  polyBrepModel->index_tessellation = tess_index;
  polyBrepModel->is_closed = group.options.closed;
  polyBrepModel->index_of_line_style = style_index;
  group.polymodels.push_back(polyBrepModel);
}

void oPRCFile::useLines(uint32_t tess_index, uint32_t style_index, const double origin[3], const double x_axis[3], const double y_axis[3], double scale)
{
  PRCgroup &group = findGroup();
  PRCPolyWire *polyWire = new PRCPolyWire();
  polyWire->index_local_coordinate_system = addTransform(origin, x_axis, y_axis, scale);
  polyWire->index_tessellation = tess_index;
  polyWire->index_of_line_style = style_index;
  group.polywires.push_back(polyWire);
}

void oPRCFile::useLines(uint32_t tess_index, uint32_t style_index, const double* t)
{
  PRCgroup &group = findGroup();
  PRCPolyWire *polyWire = new PRCPolyWire();
  polyWire->index_local_coordinate_system = addTransform(t);
  polyWire->index_tessellation = tess_index;
  polyWire->index_of_line_style = style_index;
  group.polywires.push_back(polyWire);
}

void oPRCFile::addTriangles(uint32_t nP, const double P[][3], uint32_t nI, const uint32_t PI[][3], const PRCmaterial &m,
 uint32_t nN, const double N[][3],   const uint32_t NI[][3],
 uint32_t nT, const double T[][2],   const uint32_t TI[][3],
 uint32_t nC, const RGBAColour C[],  const uint32_t CI[][3],
 uint32_t nM, const PRCmaterial M[], const uint32_t MI[], double ca)
{
  if(nP==0 || P==NULL || nI==0 || PI==NULL)
     return;
  const uint32_t tess_index = createTriangleMesh(nP, P, nI, PI, m, nN, N, NI, nT, T, TI, nC, C, CI, nM, M, MI, ca);
  useMesh(tess_index,m1);
}

uint32_t oPRCFile::createTriangleMesh(uint32_t nP, const double P[][3], uint32_t nI, const uint32_t PI[][3], const uint32_t style_index,
 uint32_t nN, const double N[][3],  const uint32_t NI[][3],
 uint32_t nT, const double T[][2],  const uint32_t TI[][3],
 uint32_t nC, const RGBAColour C[], const uint32_t CI[][3],
 uint32_t nS, const uint32_t S[],   const uint32_t SI[], double ca)
{
  if(nP==0 || P==NULL || nI==0 || PI==NULL)
     return m1;

  const bool triangle_color = (nS != 0 && S != NULL && SI != NULL);
  const bool vertex_color   = (nC != 0 && C != NULL && CI != NULL);
  const bool has_normals    = (nN != 0 && N != NULL && NI != NULL);
  const bool textured       = (nT != 0 && T != NULL && TI != NULL);

  PRC3DTess *tess = new PRC3DTess();
  PRCTessFace *tessFace = new PRCTessFace();
  tessFace->used_entities_flag = textured ? PRC_FACETESSDATA_TriangleTextured : PRC_FACETESSDATA_Triangle;
  tessFace->number_of_texture_coordinate_indexes = textured ? 1 : 0;
  tess->coordinates.reserve(3*nP);
  for(uint32_t i=0; i<nP; i++)
  {
    tess->coordinates.push_back(P[i][0]);
    tess->coordinates.push_back(P[i][1]);
    tess->coordinates.push_back(P[i][2]);
  }
  if(has_normals)
  {
    tess->normal_coordinate.reserve(3*nN);
  for(uint32_t i=0; i<nN; i++)
  {
    tess->normal_coordinate.push_back(N[i][0]);
    tess->normal_coordinate.push_back(N[i][1]);
    tess->normal_coordinate.push_back(N[i][2]);
  }
  }
  else
    tess->crease_angle = ca;
  if(textured)
  {
		tess->texture_coordinate.reserve(2*nT);
		for(uint32_t i=0; i<nT; i++)
		{
			tess->texture_coordinate.push_back(T[i][0]);
			tess->texture_coordinate.push_back(T[i][1]);
		}
  }
  tess->triangulated_index.reserve(3*nI+(has_normals?3:0)*nI+(textured?3:0)*nI);
  for(uint32_t i=0; i<nI; i++)
  {
    if(has_normals)
    tess->triangulated_index.push_back(3*NI[i][0]);
    if(textured)
    tess->triangulated_index.push_back(2*TI[i][0]);
    tess->triangulated_index.push_back(3*PI[i][0]);
    if(has_normals)
    tess->triangulated_index.push_back(3*NI[i][1]);
    if(textured)
    tess->triangulated_index.push_back(2*TI[i][1]);
    tess->triangulated_index.push_back(3*PI[i][1]);
    if(has_normals)
    tess->triangulated_index.push_back(3*NI[i][2]);
    if(textured)
    tess->triangulated_index.push_back(2*TI[i][2]);
    tess->triangulated_index.push_back(3*PI[i][2]);
  }
  tessFace->sizes_triangulated.push_back(nI);
  if(triangle_color)
  {
    tessFace->line_attributes.reserve(nI);
    for(uint32_t i=0; i<nI; i++)
       tessFace->line_attributes.push_back(SI[i]);
  }
  else if (style_index != m1 )
  {
      tessFace->line_attributes.push_back(style_index);
  }
  if(vertex_color)
  {
    tessFace->is_rgba=false;
    for(uint32_t i=0; i<nI; i++)
      if(1.0 != C[CI[i][0]].A || 1.0 != C[CI[i][1]].A || 1.0 != C[CI[i][2]].A)
      {
         tessFace->is_rgba=true;
         break;
      }

    tessFace->rgba_vertices.reserve((tessFace->is_rgba?4:3)*3*nI);
    for(uint32_t i=0; i<nI; i++)
    {
       tessFace->rgba_vertices.push_back(prc::byte(C[CI[i][0]].R));
       tessFace->rgba_vertices.push_back(prc::byte(C[CI[i][0]].G));
       tessFace->rgba_vertices.push_back(prc::byte(C[CI[i][0]].B));
       if(tessFace->is_rgba)
       tessFace->rgba_vertices.push_back(prc::byte(C[CI[i][0]].A));
       tessFace->rgba_vertices.push_back(prc::byte(C[CI[i][1]].R));
       tessFace->rgba_vertices.push_back(prc::byte(C[CI[i][1]].G));
       tessFace->rgba_vertices.push_back(prc::byte(C[CI[i][1]].B));
       if(tessFace->is_rgba)
       tessFace->rgba_vertices.push_back(prc::byte(C[CI[i][1]].A));
       tessFace->rgba_vertices.push_back(prc::byte(C[CI[i][2]].R));
       tessFace->rgba_vertices.push_back(prc::byte(C[CI[i][2]].G));
       tessFace->rgba_vertices.push_back(prc::byte(C[CI[i][2]].B));
       if(tessFace->is_rgba)
       tessFace->rgba_vertices.push_back(prc::byte(C[CI[i][2]].A));
    }
  }
  tess->addTessFace(tessFace);
  const uint32_t tess_index = add3DTess(tess);
  return tess_index;
}

void oPRCFile::addQuads(uint32_t nP, const double P[][3], uint32_t nI, const uint32_t PI[][4], const PRCmaterial &m,
 uint32_t nN, const double N[][3],   const uint32_t NI[][4],
 uint32_t nT, const double T[][2],   const uint32_t TI[][4],
 uint32_t nC, const RGBAColour C[],  const uint32_t CI[][4],
 uint32_t nM, const PRCmaterial M[], const uint32_t MI[], double ca)
{
  if(nP==0 || P==NULL || nI==0 || PI==NULL)
     return;
  const uint32_t tess_index = createQuadMesh(nP, P, nI, PI, m, nN, N, NI, nT, T, TI, nC, C, CI, nM, M, MI, ca);
  useMesh(tess_index,m1);
}

uint32_t oPRCFile::createQuadMesh(uint32_t nP, const double P[][3], uint32_t nI, const uint32_t PI[][4], uint32_t style_index,
 uint32_t nN, const double N[][3],   const uint32_t NI[][4],
 uint32_t nT, const double T[][2],   const uint32_t TI[][4],
 uint32_t nC, const RGBAColour C[],  const uint32_t CI[][4],
 uint32_t nS, const uint32_t S[],    const uint32_t SI[], double ca)
{
  if(nP==0 || P==NULL || nI==0 || PI==NULL)
     return m1;

  const bool triangle_color = (nS != 0 && S != NULL && SI != NULL);
  const bool vertex_color   = (nC != 0 && C != NULL && CI != NULL);
  const bool has_normals    = (nN != 0 && N != NULL && NI != NULL);
  const bool textured       = (nT != 0 && T != NULL && TI != NULL);

  PRC3DTess *tess = new PRC3DTess();
  PRCTessFace *tessFace = new PRCTessFace();
  tessFace->used_entities_flag = textured ? PRC_FACETESSDATA_TriangleTextured : PRC_FACETESSDATA_Triangle;
  tessFace->number_of_texture_coordinate_indexes = textured ? 1 : 0;
  tess->coordinates.reserve(3*nP);
  for(uint32_t i=0; i<nP; i++)
  {
    tess->coordinates.push_back(P[i][0]);
    tess->coordinates.push_back(P[i][1]);
    tess->coordinates.push_back(P[i][2]);
  }
  if(has_normals)
  {
    tess->normal_coordinate.reserve(3*nN);
  for(uint32_t i=0; i<nN; i++)
  {
    tess->normal_coordinate.push_back(N[i][0]);
    tess->normal_coordinate.push_back(N[i][1]);
    tess->normal_coordinate.push_back(N[i][2]);
  }
  }
  else
    tess->crease_angle = ca;
  if(textured)
  {
    tess->texture_coordinate.reserve(2*nT);
  for(uint32_t i=0; i<nT; i++)
  {
    tess->texture_coordinate.push_back(T[i][0]);
    tess->texture_coordinate.push_back(T[i][1]);
  }
  }
  tess->triangulated_index.reserve(2*(3*nI+(has_normals?3:0)*nI+(textured?3:0)*nI));
  for(uint32_t i=0; i<nI; i++)
  {
    // first triangle
    if(has_normals)
    tess->triangulated_index.push_back(3*NI[i][0]);
    if(textured)
    tess->triangulated_index.push_back(2*TI[i][0]);
    tess->triangulated_index.push_back(3*PI[i][0]);
    if(has_normals)
    tess->triangulated_index.push_back(3*NI[i][1]);
    if(textured)
    tess->triangulated_index.push_back(2*TI[i][1]);
    tess->triangulated_index.push_back(3*PI[i][1]);
    if(has_normals)
    tess->triangulated_index.push_back(3*NI[i][3]);
    if(textured)
    tess->triangulated_index.push_back(2*TI[i][3]);
    tess->triangulated_index.push_back(3*PI[i][3]);
    // second triangle
    if(has_normals)
    tess->triangulated_index.push_back(3*NI[i][1]);
    if(textured)
    tess->triangulated_index.push_back(2*TI[i][1]);
    tess->triangulated_index.push_back(3*PI[i][1]);
    if(has_normals)
    tess->triangulated_index.push_back(3*NI[i][2]);
    if(textured)
    tess->triangulated_index.push_back(2*TI[i][2]);
    tess->triangulated_index.push_back(3*PI[i][2]);
    if(has_normals)
    tess->triangulated_index.push_back(3*NI[i][3]);
    if(textured)
    tess->triangulated_index.push_back(2*TI[i][3]);
    tess->triangulated_index.push_back(3*PI[i][3]);
  }
  tessFace->sizes_triangulated.push_back(2*nI);
  if(triangle_color)
  {
    tessFace->line_attributes.reserve(2*nI);
    for(uint32_t i=0; i<nI; i++)
    {
       tessFace->line_attributes.push_back(SI[i]);
       tessFace->line_attributes.push_back(SI[i]);
    }
  }
  else
  {
      tessFace->line_attributes.push_back(style_index);
  }
  if(vertex_color)
  {
    tessFace->is_rgba=false;
    for(uint32_t i=0; i<nI; i++)
      if(1.0 != C[CI[i][0]].A || 1.0 != C[CI[i][1]].A || 1.0 != C[CI[i][2]].A)
      {
         tessFace->is_rgba=true;
         break;
      }

    tessFace->rgba_vertices.reserve(2*(tessFace->is_rgba?4:3)*3*nI);
    for(uint32_t i=0; i<nI; i++)
    {
       // first triangle
       tessFace->rgba_vertices.push_back(prc::byte(C[CI[i][0]].R));
       tessFace->rgba_vertices.push_back(prc::byte(C[CI[i][0]].G));
       tessFace->rgba_vertices.push_back(prc::byte(C[CI[i][0]].B));
       if(tessFace->is_rgba)
       tessFace->rgba_vertices.push_back(prc::byte(C[CI[i][0]].A));
       tessFace->rgba_vertices.push_back(prc::byte(C[CI[i][1]].R));
       tessFace->rgba_vertices.push_back(prc::byte(C[CI[i][1]].G));
       tessFace->rgba_vertices.push_back(prc::byte(C[CI[i][1]].B));
       if(tessFace->is_rgba)
       tessFace->rgba_vertices.push_back(prc::byte(C[CI[i][1]].A));
       tessFace->rgba_vertices.push_back(prc::byte(C[CI[i][3]].R));
       tessFace->rgba_vertices.push_back(prc::byte(C[CI[i][3]].G));
       tessFace->rgba_vertices.push_back(prc::byte(C[CI[i][3]].B));
       if(tessFace->is_rgba)
       tessFace->rgba_vertices.push_back(prc::byte(C[CI[i][3]].A));
       // second triangle
       tessFace->rgba_vertices.push_back(prc::byte(C[CI[i][1]].R));
       tessFace->rgba_vertices.push_back(prc::byte(C[CI[i][1]].G));
       tessFace->rgba_vertices.push_back(prc::byte(C[CI[i][1]].B));
       if(tessFace->is_rgba)
       tessFace->rgba_vertices.push_back(prc::byte(C[CI[i][1]].A));
       tessFace->rgba_vertices.push_back(prc::byte(C[CI[i][2]].R));
       tessFace->rgba_vertices.push_back(prc::byte(C[CI[i][2]].G));
       tessFace->rgba_vertices.push_back(prc::byte(C[CI[i][2]].B));
       if(tessFace->is_rgba)
       tessFace->rgba_vertices.push_back(prc::byte(C[CI[i][2]].A));
       tessFace->rgba_vertices.push_back(prc::byte(C[CI[i][3]].R));
       tessFace->rgba_vertices.push_back(prc::byte(C[CI[i][3]].G));
       tessFace->rgba_vertices.push_back(prc::byte(C[CI[i][3]].B));
       if(tessFace->is_rgba)
       tessFace->rgba_vertices.push_back(prc::byte(C[CI[i][3]].A));
    }
  }
  tess->addTessFace(tessFace);
  const uint32_t tess_index = add3DTess(tess);
  return tess_index;
}

void oPRCFile::addQuad(const double P[][3], const RGBAColour C[])
{
  PRCgroup &group = findGroup();

  group.quads.push_back(PRCtessquad());
  PRCtessquad &quad = group.quads.back();
  for(size_t i = 0; i < 4; i++)
  {
    quad.vertices[i].x = P[i][0];
    quad.vertices[i].y = P[i][1];
    quad.vertices[i].z = P[i][2];
    quad.colours[i] = C[i];
  }
}
/*
void oPRCFile::addTriangle(const double P[][3], const double T[][2], uint32_t style_index)
{
  PRCgroup &group = findGroup();

  group.triangles.push_back(PRCtesstriangle());
  PRCtesstriangle &triangle = group.triangles.back();
  for(size_t i = 0; i < 3; i++)
  {
    triangle.vertices[i].x = P[i][0];
    triangle.vertices[i].y = P[i][1];
    triangle.vertices[i].z = P[i][2];
    triangle.texcoords[i].x = T[i][0];
    triangle.texcoords[i].y = T[i][1];
  }
  triangle.style = style_index;
}
*/

void oPRCFile::addLines(uint32_t nP, const double P[][3], uint32_t nI, const uint32_t PI[],
 const RGBAColour& c, double w,
 bool segment_color, uint32_t nC, const RGBAColour C[], uint32_t nCI, const uint32_t CI[])
{
  if(nP==0 || P==NULL || nI==0 || PI==NULL)
    return;
  const uint32_t tess_index = createLines(nP, P, nI, PI, segment_color, nC, C, nCI, CI);
  useLines(tess_index, c, w);
}

uint32_t oPRCFile::createLines(uint32_t nP, const double P[][3], uint32_t nI, const uint32_t PI[],
 bool segment_color, uint32_t nC, const RGBAColour C[], uint32_t nCI, const uint32_t CI[])
{
  if(nP==0 || P==NULL || nI==0 || PI==NULL)
    return m1;

  const bool vertex_color  = (nC != 0 && C != NULL && CI != NULL);

  PRC3DWireTess *tess = new PRC3DWireTess();
  tess->coordinates.reserve(3*nP);
  for(uint32_t i=0; i<nP; i++)
  {
    tess->coordinates.push_back(P[i][0]);
    tess->coordinates.push_back(P[i][1]);
    tess->coordinates.push_back(P[i][2]);
  }
  tess->wire_indexes.reserve(nI);
  for(uint32_t i=0; i<nI;)
  {
    tess->wire_indexes.push_back(PI[i]);
    const uint32_t ni = i+PI[i]+1;
    for(i++; i<ni; i++)
      tess->wire_indexes.push_back(3*PI[i]);
  }
  if(vertex_color)
  {
    tess->is_segment_color = segment_color;
    tess->is_rgba=false;
    for(uint32_t i=0; i<nCI; i++)
      if(1.0 != C[CI[i]].A)
      {
         tess->is_rgba=true;
         break;
      }
    tess->rgba_vertices.reserve((tess->is_rgba?4:3)*nCI);
    for(uint32_t i=0; i<nCI; i++)
    {
       tess->rgba_vertices.push_back(prc::byte(C[CI[i]].R));
       tess->rgba_vertices.push_back(prc::byte(C[CI[i]].G));
       tess->rgba_vertices.push_back(prc::byte(C[CI[i]].B));
       if(tess->is_rgba)
       tess->rgba_vertices.push_back(prc::byte(C[CI[i]].A));
    }
  }
  const uint32_t tess_index = add3DWireTess(tess);
  return tess_index;
}

void oPRCFile::addLine(uint32_t n, const double P[][3], const RGBAColour &c, double w)
{
  PRCgroup &group = findGroup();
  if(group.options.tess)
  {
    group.lines[w].push_back(PRCtessline());
    PRCtessline& line = group.lines[w].back();
    line.color.red   = c.R;
    line.color.green = c.G;
    line.color.blue  = c.B;
    for(uint32_t i=0; i<n; i++)
      line.point.push_back(PRCVector3d(P[i][0],P[i][1],P[i][2]));
  }
  else
  {
    ADDWIRE(PRCPolyLine)
    curve->point.resize(n);
    for(uint32_t i=0; i<n; i++)
     curve->point[i].Set(P[i][0],P[i][1],P[i][2]);
    curve->interval.min = 0;
    curve->interval.max = curve->point.size()-1;
  }
}

void oPRCFile::addBezierCurve(uint32_t n, const double cP[][3],
                              const RGBAColour &c)
{
  ADDWIRE(PRCNURBSCurve)
  curve->is_rational = false;
  curve->degree = 3;
  const size_t NUMBER_OF_POINTS = n;
  curve->control_point.resize(NUMBER_OF_POINTS);
  for(size_t i = 0; i < NUMBER_OF_POINTS; ++i)
    curve->control_point[i].Set(cP[i][0],cP[i][1],cP[i][2]);
  curve->knot.resize(3+NUMBER_OF_POINTS+1);
  curve->knot[0] = 1;
  for(size_t i = 1; i < 3+NUMBER_OF_POINTS; ++i)
    curve->knot[i] = (i+2)/3; // integer division is intentional
  curve->knot[3+NUMBER_OF_POINTS] = (3+NUMBER_OF_POINTS+1)/3;
}

void oPRCFile::addCurve(uint32_t d, uint32_t n, const double cP[][3], const double *k, const RGBAColour &c, const double w[])
{
  ADDWIRE(PRCNURBSCurve)
  curve->is_rational = (w!=NULL);
  curve->degree = d;
  curve->control_point.resize(n);
  for(uint32_t i = 0; i < n; i++)
    if(w)
      curve->control_point[i].Set(cP[i][0]*w[i],cP[i][1]*w[i],cP[i][2]*w[i],w[i]);
    else
      curve->control_point[i].Set(cP[i][0],cP[i][1],cP[i][2]);
  curve->knot.resize(d+n+1);
  for(uint32_t i = 0; i < d+n+1; i++)
    curve->knot[i] = k[i];
}

void oPRCFile::addRectangle(const double P[][3], const PRCmaterial &m)
{
  PRCgroup &group = findGroup();
  if(group.options.tess)
  {
    group.rectangles.push_back(PRCtessrectangle());
    PRCtessrectangle &rectangle = group.rectangles.back();
    rectangle.style = addMaterial(m);
    for(size_t i = 0; i < 4; i++)
    {
       rectangle.vertices[i].x = P[i][0];
       rectangle.vertices[i].y = P[i][1];
       rectangle.vertices[i].z = P[i][2];
    }
  }
  else if(group.options.compression == 0.0)
  {
    ADDFACE(PRCNURBSSurface)

    surface->is_rational = false;
    surface->degree_in_u = 1;
    surface->degree_in_v = 1;
    surface->control_point.resize(4);
    for(size_t i = 0; i < 4; ++i)
    {
        surface->control_point[i].x = P[i][0];
        surface->control_point[i].y = P[i][1];
        surface->control_point[i].z = P[i][2];
    }
    surface->knot_u.resize(4);
    surface->knot_v.resize(4);
    surface->knot_v[0] = surface->knot_u[0] = 1;
    surface->knot_v[1] = surface->knot_u[1] = 3;
    surface->knot_v[2] = surface->knot_u[2] = 4;
    surface->knot_v[3] = surface->knot_u[3] = 4;
  }
  else
  {
    ADDCOMPFACE

    compface->degree = 1;
    compface->control_point.resize(4);
    for(size_t i = 0; i < 4; ++i)
    {
        compface->control_point[i].x = P[i][0];
        compface->control_point[i].y = P[i][1];
        compface->control_point[i].z = P[i][2];
    }
  }
}

void oPRCFile::addPatch(const double cP[][3], const PRCmaterial &m)
{
  PRCgroup &group = findGroup();
  if(group.options.compression == 0.0)
  {
    ADDFACE(PRCNURBSSurface)

    surface->is_rational = false;
    surface->degree_in_u = 3;
    surface->degree_in_v = 3;
    surface->control_point.resize(16);
    for(size_t i = 0; i < 16; ++i)
    {
        surface->control_point[i].x = cP[i][0];
        surface->control_point[i].y = cP[i][1];
        surface->control_point[i].z = cP[i][2];
    }
    surface->knot_u.resize(8);
    surface->knot_v.resize(8);
    surface->knot_v[0] = surface->knot_u[0] = 1;
    surface->knot_v[1] = surface->knot_u[1] = 1;
    surface->knot_v[2] = surface->knot_u[2] = 1;
    surface->knot_v[3] = surface->knot_u[3] = 1;
    surface->knot_v[4] = surface->knot_u[4] = 2;
    surface->knot_v[5] = surface->knot_u[5] = 2;
    surface->knot_v[6] = surface->knot_u[6] = 2;
    surface->knot_v[7] = surface->knot_u[7] = 2;
  }
  else
  {
    ADDCOMPFACE

    compface->degree = 3;
    compface->control_point.resize(16);
    for(size_t i = 0; i < 16; ++i)
    {
        compface->control_point[i].x = cP[i][0];
        compface->control_point[i].y = cP[i][1];
        compface->control_point[i].z = cP[i][2];
    }
  }
}

void oPRCFile::addSurface(uint32_t dU, uint32_t dV, uint32_t nU, uint32_t nV,
                          const double cP[][3], const double *kU,
                          const double *kV, const PRCmaterial &m,
                          const double w[])
{
  ADDFACE(PRCNURBSSurface)

  surface->is_rational = (w!=NULL);
  surface->degree_in_u = dU;
  surface->degree_in_v = dV;
  surface->control_point.resize(nU*nV);
  for(size_t i = 0; i < nU*nV; i++)
    if(w)
      surface->control_point[i]=PRCControlPoint(cP[i][0]*w[i],cP[i][1]*w[i],cP[i][2]*w[i],w[i]);
    else
      surface->control_point[i]=PRCControlPoint(cP[i][0],cP[i][1],cP[i][2]);
  surface->knot_u.insert(surface->knot_u.end(), kU, kU+(dU+nU+1));
  surface->knot_v.insert(surface->knot_v.end(), kV, kV+(dV+nV+1));
}

#define SETTRANSF \
  if(t&&!isid(t))                                                             \
    face.transform = new PRCGeneralTransformation3d(t);                       \
  if(origin) surface->origin.Set(origin[0],origin[1],origin[2]);              \
  if(x_axis) surface->x_axis.Set(x_axis[0],x_axis[1],x_axis[2]);              \
  if(y_axis) surface->y_axis.Set(y_axis[0],y_axis[1],y_axis[2]);              \
  surface->scale = scale;                                                     \
  surface->geometry_is_2D = false;                                            \
  if(surface->origin!=PRCVector3d(0.0,0.0,0.0))                                     \
    surface->behaviour = surface->behaviour | PRC_TRANSFORMATION_Translate;   \
  if(surface->x_axis!=PRCVector3d(1.0,0.0,0.0)||surface->y_axis!=PRCVector3d(0.0,1.0,0.0)) \
    surface->behaviour = surface->behaviour | PRC_TRANSFORMATION_Rotate;      \
  if(surface->scale!=1)                                                       \
    surface->behaviour = surface->behaviour | PRC_TRANSFORMATION_Scale;       \
  surface->has_transformation = (surface->behaviour != PRC_TRANSFORMATION_Identity);

#define PRCFACETRANSFORM const double origin[3], const double x_axis[3], const double y_axis[3], double scale, const double* t

void oPRCFile::addTube(uint32_t n, const double cP[][3], const double oP[][3], bool straight, const PRCmaterial &m, PRCFACETRANSFORM)
{
  ADDFACE(PRCBlend01)
  SETTRANSF
  if(straight)
  {
    PRCPolyLine *center_curve = new PRCPolyLine;
    center_curve->point.resize(n);
    for(uint32_t i=0; i<n; i++)
      center_curve->point[i].Set(cP[i][0],cP[i][1],cP[i][2]);
    center_curve->interval.min = 0;
    center_curve->interval.max = center_curve->point.size()-1;
    surface->center_curve = center_curve;

    PRCPolyLine *origin_curve = new PRCPolyLine;
    origin_curve->point.resize(n);
    for(uint32_t i=0; i<n; i++)
      origin_curve->point[i].Set(oP[i][0],oP[i][1],oP[i][2]);
    origin_curve->interval.min = 0;
    origin_curve->interval.max = origin_curve->point.size()-1;
    surface->origin_curve = origin_curve;

    surface->uv_domain.min.x = 0;
    surface->uv_domain.max.x = 2*pi;
    surface->uv_domain.min.y = 0;
    surface->uv_domain.max.y = n-1;
  }
  else
  {
    PRCNURBSCurve *center_curve = new PRCNURBSCurve;
    center_curve->is_rational = false;
    center_curve->degree = 3;
    const uint32_t CENTER_NUMBER_OF_POINTS = n;
    center_curve->control_point.resize(CENTER_NUMBER_OF_POINTS);
    for(uint32_t i = 0; i < CENTER_NUMBER_OF_POINTS; ++i)
      center_curve->control_point[i].Set(cP[i][0],cP[i][1],cP[i][2]);
    center_curve->knot.resize(3+CENTER_NUMBER_OF_POINTS+1);
    center_curve->knot[0] = 1;
    for(uint32_t i = 1; i < 3+CENTER_NUMBER_OF_POINTS; ++i)
      center_curve->knot[i] = (i+2)/3; // integer division is intentional
    center_curve->knot[3+CENTER_NUMBER_OF_POINTS] = (3+CENTER_NUMBER_OF_POINTS+1)/3;
    surface->center_curve = center_curve;

    PRCNURBSCurve *origin_curve = new PRCNURBSCurve;
    origin_curve->is_rational = false;
    origin_curve->degree = 3;
    const uint32_t ORIGIN_NUMBER_OF_POINTS = n;
    origin_curve->control_point.resize(ORIGIN_NUMBER_OF_POINTS);
    for(uint32_t i = 0; i < ORIGIN_NUMBER_OF_POINTS; ++i)
      origin_curve->control_point[i].Set(oP[i][0],oP[i][1],oP[i][2]);
    origin_curve->knot.resize(3+ORIGIN_NUMBER_OF_POINTS+1);
    origin_curve->knot[0] = 1;
    for(size_t i = 1; i < 3+ORIGIN_NUMBER_OF_POINTS; ++i)
      origin_curve->knot[i] = (i+2)/3; // integer division is intentional
    origin_curve->knot[3+ORIGIN_NUMBER_OF_POINTS] = (3+ORIGIN_NUMBER_OF_POINTS+1)/3;
    surface->origin_curve = origin_curve;

    surface->uv_domain.min.x = 0;
    surface->uv_domain.max.x = 2*pi;
    surface->uv_domain.min.y = 1; // first knot
    surface->uv_domain.max.y = (3+CENTER_NUMBER_OF_POINTS+1)/3; // last knot
  }
}

void oPRCFile::addHemisphere(double radius, const PRCmaterial &m, PRCFACETRANSFORM)
{
  ADDFACE(PRCSphere)
  SETTRANSF
  surface->uv_domain.min.x = 0;
  surface->uv_domain.max.x = 2*pi;
  surface->uv_domain.min.y = 0;
  surface->uv_domain.max.y = 0.5*pi;
  surface->radius = radius;
}

void oPRCFile::addSphere(double radius, const PRCmaterial &m, PRCFACETRANSFORM)
{
  ADDFACE(PRCSphere)
  SETTRANSF
  surface->uv_domain.min.x = 0;
  surface->uv_domain.max.x = 2*pi;
  surface->uv_domain.min.y =-0.5*pi;
  surface->uv_domain.max.y = 0.5*pi;
  surface->radius = radius;
}

void oPRCFile::addDisk(double radius, const PRCmaterial &m, PRCFACETRANSFORM)
{
  ADDFACE(PRCRuled)
  SETTRANSF
  PRCCircle *first_curve = new PRCCircle;
  first_curve->radius = radius;
  surface->first_curve = first_curve;
  PRCCircle *second_curve = new PRCCircle;
  second_curve->radius = 0;
  surface->second_curve = second_curve;

  surface->uv_domain.min.x = 0;
  surface->uv_domain.max.x = 1;
  surface->uv_domain.min.y = 0;
  surface->uv_domain.max.y = 2*pi;
  surface->parameterization_on_v_coeff_a = -1;
  surface->parameterization_on_v_coeff_b = 2*pi;
}

void oPRCFile::addCylinder(double radius, double height, const PRCmaterial &m, PRCFACETRANSFORM)
{
  ADDFACE(PRCCylinder)
  SETTRANSF
  surface->uv_domain.min.x = 0;
  surface->uv_domain.max.x = 2*pi;
  surface->uv_domain.min.y = (height>0)?0:height;
  surface->uv_domain.max.y = (height>0)?height:0;
  surface->radius = radius;
}

void oPRCFile::addCone(double radius, double height, const PRCmaterial &m, PRCFACETRANSFORM)
{
  ADDFACE(PRCCone)
  SETTRANSF
  surface->uv_domain.min.x = 0;
  surface->uv_domain.max.x = 2*pi;
  surface->uv_domain.min.y = (height>0)?0:height;
  surface->uv_domain.max.y = (height>0)?height:0;
  surface->bottom_radius = radius;
  surface->semi_angle = -atan(radius/height);;
}

void oPRCFile::addTorus(double major_radius, double minor_radius, double angle1, double angle2, const PRCmaterial &m, PRCFACETRANSFORM)
{
  ADDFACE(PRCTorus)
  SETTRANSF
  surface->uv_domain.min.x = (angle1/180)*pi;
  surface->uv_domain.max.x = (angle2/180)*pi;
  surface->uv_domain.min.y = 0;
  surface->uv_domain.max.y = 2*pi;
  surface->major_radius = major_radius;
  surface->minor_radius = minor_radius;
}

#undef PRCFACETRANSFORM
#undef ADDFACE
#undef ADDWIRE
#undef SETTRANSF

uint32_t PRCFileStructure::addMaterialGeneric(PRCMaterialGeneric*& pMaterialGeneric)
{
  materials.push_back(pMaterialGeneric);
  pMaterialGeneric = NULL;
  return materials.size()-1;
}

uint32_t PRCFileStructure::addTextureApplication(PRCTextureApplication*& pTextureApplication)
{
  materials.push_back(pTextureApplication);
  pTextureApplication = NULL;
  return materials.size()-1;
}

uint32_t PRCFileStructure::addStyle(PRCStyle*& pStyle)
{
  styles.push_back(pStyle);
  pStyle = NULL;
  return styles.size()-1;
}

uint32_t PRCFileStructure::addPartDefinition(PRCPartDefinition*& pPartDefinition)
{
  part_definitions.push_back(pPartDefinition);
  pPartDefinition = NULL;
  return part_definitions.size()-1;
}

uint32_t PRCFileStructure::addProductOccurrence(PRCProductOccurrence*& pProductOccurrence)
{
  product_occurrences.push_back(pProductOccurrence);
  pProductOccurrence = NULL;
  return product_occurrences.size()-1;
}

uint32_t PRCFileStructure::addTopoContext(PRCTopoContext*& pTopoContext)
{
  contexts.push_back(pTopoContext);
  pTopoContext = NULL;
  return contexts.size()-1;
}

uint32_t PRCFileStructure::getTopoContext(PRCTopoContext*& pTopoContext)
{
  pTopoContext = new PRCTopoContext;
  contexts.push_back(pTopoContext);
  return contexts.size()-1;
}

uint32_t PRCFileStructure::add3DTess(PRC3DTess*& p3DTess)
{
  tessellations.push_back(p3DTess);
  p3DTess = NULL;
  return tessellations.size()-1;
}

uint32_t PRCFileStructure::add3DWireTess(PRC3DWireTess*& p3DWireTess)
{
  tessellations.push_back(p3DWireTess);
  p3DWireTess = NULL;
  return tessellations.size()-1;
}
/*
uint32_t PRCFileStructure::addMarkupTess(PRCMarkupTess*& pMarkupTess)
{
  tessellations.push_back(pMarkupTess);
  pMarkupTess = NULL;
  return tessellations.size()-1;
}

uint32_t PRCFileStructure::addMarkup(PRCMarkup*& pMarkup)
{
  markups.push_back(pMarkup);
  pMarkup = NULL;
  return markups.size()-1;
}

uint32_t PRCFileStructure::addAnnotationItem(PRCAnnotationItem*& pAnnotationItem)
{
  annotation_entities.push_back(pAnnotationItem);
  pAnnotationItem = NULL;
  return annotation_entities.size()-1;
}
*/
uint32_t PRCFileStructure::addCoordinateSystem(PRCCoordinateSystem*& pCoordinateSystem)
{
  reference_coordinate_systems.push_back(pCoordinateSystem);
  pCoordinateSystem = NULL;
  return reference_coordinate_systems.size()-1;
}

uint32_t PRCFileStructure::addCoordinateSystemUnique(PRCCoordinateSystem*& pCoordinateSystem)
{
  for(uint32_t i = 0; i < reference_coordinate_systems.size(); ++i)
  {
    if(*(reference_coordinate_systems[i])==*pCoordinateSystem) {
      pCoordinateSystem = NULL;
      return i;
    }
  }
  reference_coordinate_systems.push_back(pCoordinateSystem);
  pCoordinateSystem = NULL;
  return reference_coordinate_systems.size()-1;
}
