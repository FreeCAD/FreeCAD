#***************************************************************************
#*   (c) Milos Koutny (milos.koutny@gmail.com) 2012                        *
#*                                                                         *
#*   This file is part of the FreeCAD CAx development system.              *
#*                                                                         *
#*   This program is free software; you can redistribute it and/or modify  *
#*   it under the terms of the GNU Library General Public License (LGPL)   *
#*   as published by the Free Software Foundation; either version 2 of     *
#*   the License, or (at your option) any later version.                   *
#*   for detail see the LICENCE text file.                                 *
#*                                                                         *
#*   FreeCAD is distributed in the hope that it will be useful,            *
#*   but WITHOUT ANY WARRANTY; without even the implied warranty of        *
#*   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the         *
#*   GNU Library General Public License for more details.                  *
#*                                                                         *
#*   You should have received a copy of the GNU Library General Public     *
#*   License along with FreeCAD; if not, write to the Free Software        *
#*   Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  *
#*   USA                                                                   *
#*                                                                         *
#*   Milos Koutny 2010                                                     *
#***************************************************************************/

import FreeCAD, Part, os, FreeCADGui
from FreeCAD import Base
from math import *
import ImportGui

# to distinguish python built-in open function from the one declared here
if open.__module__ in ['__builtin__','io']:
    pythonopen = open

##########################################################
# Script version dated 19-Jan-2012                       #
##########################################################
#Configuration parameters below - use standard slashes / #
##########################################################

## path to table file (simple comma separated values)

model_tab_filename = FreeCAD.getHomePath()+ "Mod/Idf/Idflibs/footprints_models.csv"

## path to directory containing step models

step_path=FreeCAD.getHomePath()+ "Mod/Idf/Idflibs/"

ignore_hole_size=0.5 # size in MM to prevent huge number of drilled holes
EmpDisplayMode=2 # 0='Flat Lines', 1='Shaded', 2='Wireframe', 3='Points'; recommended 2 or 0

IDF_sort=0 # 0-sort per refdes [1 - part number (not preferred)/refdes] 2-sort per footprint/refdes

IDF_diag=0 # 0/1=disabled/enabled output (footprint.lst/missing_models.lst) 
IDF_diag_path="/tmp" # path for output of footprint.lst and missing_models.lst


########################################################################################
#              End config section do not touch code below                              #
########################################################################################

def open(filename):
    """called when freecad opens an Emn file"""
    docname = os.path.splitext(os.path.basename(filename))[0]
    doc = FreeCAD.newDocument(docname)
    message='Started with opening of "'+filename+'" file\n'
    FreeCAD.Console.PrintMessage(message)
    process_emn(doc,filename)

def insert(filename,docname):
    """called when freecad imports an Emn file"""
    FreeCAD.setActiveDocument(docname)
    doc=FreeCAD.getDocument(docname)
    FreeCAD.Console.PrintMessage('Started import of "'+filename+'" file')
    process_emn(doc,filename)
	
def process_emn(doc,filename):
   """process_emn(document, filename)-> adds emn geometry from emn file"""
   emnfile=pythonopen(filename, "r")
   emn_unit=1.0 #presume milimeter like emn unit
   emn_version=2 #presume emn_version 2
   board_thickness=0 #presume 0 board height
   board_outline=[] #no outline
   drills=[] #no drills
   placement=[] #no placement
   place_item=[] #empty place item
   emnlines=emnfile.readlines()
   emnfile.close()   
   passed_sections=[]
   current_section=""
   section_counter=0
   for emnline in emnlines:
       emnrecords=split_records(emnline)
       if len( emnrecords )==0 : continue
       if len( emnrecords[0] )>4 and emnrecords[0][0:4]==".END":
          passed_sections.append(current_section)
          current_section=""
       elif emnrecords[0][0]==".":
          current_section=emnrecords[0]
          section_counter=0
       section_counter+=1
       if current_section==".HEADER"  and section_counter==2:
          emn_version=int(float(emnrecords[1]))
          FreeCAD.Console.PrintMessage("Emn version: "+emnrecords[1]+"\n")
       if current_section==".HEADER"  and section_counter==3 and emnrecords[1]=="THOU":
          emn_unit=0.0254
          FreeCAD.Console.PrintMessage("UNIT THOU\n" )
       if current_section==".HEADER"  and section_counter==3 and emnrecords[1]=="TNM":
          emn_unit=0.000010
          FreeCAD.Console.PrintMessage("TNM\n" )
       if current_section==".BOARD_OUTLINE"  and section_counter==2:
          board_thickness=emn_unit*float(emnrecords[0])
          FreeCAD.Console.PrintMessage("Found board thickness "+emnrecords[0]+"\n")
       if current_section==".BOARD_OUTLINE"  and section_counter>2:
          board_outline.append([int(emnrecords[0]),float(emnrecords[1])*emn_unit,float(emnrecords[2])*emn_unit,float(emnrecords[3])])
       if current_section==".DRILLED_HOLES"  and section_counter>1 and float(emnrecords[0])*emn_unit>ignore_hole_size:
          drills.append([float(emnrecords[0])*emn_unit,float(emnrecords[1])*emn_unit,float(emnrecords[2])*emn_unit])
       if current_section==".PLACEMENT"  and section_counter>1 and fmod(section_counter,2)==0:
          place_item=[]
          place_item.append(emnrecords[2]) #Reference designator
          place_item.append(emnrecords[1]) #Component part number
          place_item.append(emnrecords[0]) #Package name
       if current_section==".PLACEMENT"  and section_counter>1 and fmod(section_counter,2)==1:
          place_item.append(float(emnrecords[0])*emn_unit) #X
          place_item.append(float(emnrecords[1])*emn_unit) #Y
          place_item.append(float(emnrecords[emn_version])) #Rotation
          place_item.append(emnrecords[emn_version+1]) #Side
          place_item.append(emnrecords[emn_version+2]) #Place Status
          FreeCAD.Console.PrintMessage(str(place_item)+"\n")
          placement.append(place_item)
   FreeCAD.Console.PrintMessage("\n".join(passed_sections)+"\n")
   FreeCAD.Console.PrintMessage("Proceed "+str(Process_board_outline(doc,board_outline,drills,board_thickness))+" outlines\n")
   placement.sort(key=lambda param: (param[IDF_sort],param[0]))
   process_emp(doc,filename,placement,board_thickness)
   place_steps(doc,placement,board_thickness)

def Process_board_outline(doc,board_outline,drills,board_thickness):
    """Process_board_outline(doc,board_outline,drills,board_thickness)-> number proccesed loops

        adds emn geometry from emn file"""
    vertex_index=-1; #presume no vertex
    lines=-1 #presume no lines
    out_shape=[]
    out_face=[]
    for point in board_outline:
       vertex=Base.Vector(point[1],point[2],0) 
       vertex_index+=1
       if vertex_index==0:
          lines=point[0] 
       elif lines==point[0]:
           if point[3]!=0 and point[3]!=360:
              out_shape.append(Part.Arc(prev_vertex,mid_point(prev_vertex,vertex,point[3]),vertex))
              FreeCAD.Console.PrintMessage("mid point "+str(mid_point)+"\n")
           elif point[3]==360:
              per_point=Per_point(prev_vertex,vertex)
              out_shape.append(Part.Arc(per_point,mid_point(per_point,vertex,point[3]/2),vertex))
              out_shape.append(Part.Arc(per_point,mid_point(per_point,vertex,-point[3]/2),vertex))
           else:
              out_shape.append(Part.LineSegment(prev_vertex,vertex))
       else:
          out_shape=Part.Shape(out_shape)
          out_shape=Part.Wire(out_shape.Edges)
          out_face.append(Part.Face(out_shape))
          out_shape=[]
          vertex_index=0 
          lines=point[0] 
       prev_vertex=vertex
    if lines!=-1:
      out_shape=Part.Shape(out_shape)
      out_shape=Part.Wire(out_shape.Edges)
      out_face.append(Part.Face(out_shape))
      outline=out_face[0]
      FreeCAD.Console.PrintMessage("Added outline\n")
      if len(out_face)>1:
        for otl_cut in out_face[1: ]:
          outline=outline.cut(otl_cut)
          FreeCAD.Console.PrintMessage("Cutting shape inside outline\n")
      for drill in drills:
        FreeCAD.Console.PrintMessage("Cutting hole inside outline\n")
        out_shape=Part.makeCircle(drill[0]/2, Base.Vector(drill[1],drill[2],0))
        out_shape=Part.Wire(out_shape.Edges)
        outline=outline.cut(Part.Face(out_shape))
      doc_outline=doc.addObject("Part::Feature","Board_outline")
      doc_outline.Shape=outline 
      #FreeCADGui.Selection.addSelection(doc_outline)
      #FreeCADGui.runCommand("Draft_Upgrade")
      #outline=FreeCAD.ActiveDocument.getObject("Union").Shape
      #FreeCAD.ActiveDocument.removeObject("Union")
      #doc_outline=doc.addObject("Part::Feature","Board_outline")
      doc_outline.Shape=outline.extrude(Base.Vector(0,0,-board_thickness))
      grp=doc.addObject("App::DocumentObjectGroup", "Board_Geoms")
      grp.addObject(doc_outline)
      doc.Board_outline.ViewObject.ShapeColor=(0.0, 0.5, 0.0, 0.0)
    return lines+1

def mid_point(prev_vertex,vertex,angle):
    """mid_point(prev_vertex,vertex,angle)-> mid_vertex
       
       returns mid point on arc of angle between prev_vertex and vertex"""
    angle=radians(angle/2)
    basic_angle=atan2(vertex.y-prev_vertex.y,vertex.x-prev_vertex.x)-pi/2
    shift=(1-cos(angle))*hypot(vertex.y-prev_vertex.y,vertex.x-prev_vertex.x)/2/sin(angle)
    midpoint=Base.Vector((vertex.x+prev_vertex.x)/2+shift*cos(basic_angle),(vertex.y+prev_vertex.y)/2+shift*sin(basic_angle),0)
    return midpoint

def split_records(line_record):
    """split_records(line_record)-> list of strings(records)
       
       standard separator list separator is space, records containing encapsulated by " """
    split_result=[]
    quote_pos=line_record.find('"')
    while quote_pos!=-1:
       if quote_pos>0:
          split_result.extend(line_record[ :quote_pos].split())
          line_record=line_record[quote_pos: ]
          quote_pos=line_record.find('"',1)
       else: 
          quote_pos=line_record.find('"',1)
       if quote_pos!=-1:
          split_result.append(line_record[ :quote_pos+1])
          line_record=line_record[quote_pos+1: ]
       else:
          split_result.append(line_record) 
          line_record=""
       quote_pos=line_record.find('"')
    split_result.extend(line_record.split())
    return split_result
	
def process_emp(doc,filename,placement,board_thickness):
   """process_emp(doc,filename,placement,board_thickness) -> place components from emn file to board"""
   filename=filename.partition(".emn")[0]+".emp"
   empfile=pythonopen(filename, "r")
   emp_unit=1.0 #presume millimeter like emn unit
   emp_version=2 #presume emn_version 2
   comp_height=0 #presume 0 part height
   comp_outline=[] #no part outline
   comp_GeometryName="" # no geometry name
   comp_PartNumber="" # no Part Number
   comp_height=0 # no Comp Height
   emplines=empfile.readlines()
   empfile.close()   
   passed_sections=[]
   current_section=""
   section_counter=0
   comps=[]
   for empline in emplines:
     emprecords=split_records(empline)
     if len( emprecords )==0 : continue
     if len( emprecords[0] )>4 and emprecords[0][0:4]==".END":
        passed_sections.append(current_section)
        current_section=""
        if comp_PartNumber!="":
          if comp_height==0:
            comp_height=0.1	
          comps.append((comp_PartNumber,[Process_comp_outline(doc,comp_outline,comp_height),comp_GeometryName]))
          comp_PartNumber=""
          comp_outline=[]
     elif emprecords[0][0]==".":
        current_section=emprecords[0]
        section_counter=0
     section_counter+=1
     if current_section==".HEADER"  and section_counter==2:
        emp_version=int(float(emprecords[1]))
        FreeCAD.Console.PrintMessage("Emp version: "+emprecords[1]+"\n")
     if (current_section==".ELECTRICAL" or current_section==".MECHANICAL") and section_counter==2 and emprecords[2]=="THOU":
        emp_unit=0.0254
     if (current_section==".ELECTRICAL" or current_section==".MECHANICAL") and section_counter==2 and emprecords[2]=="MM":
        emp_unit=1
     if (current_section==".ELECTRICAL" or current_section==".MECHANICAL") and section_counter==2:
        comp_outline=[] #no part outline
        comp_GeometryName=emprecords[0] # geometry name
        comp_PartNumber=emprecords[1] # Part Number
        comp_height=emp_unit*float(emprecords[3]) # Comp Height
     if (current_section==".ELECTRICAL" or current_section==".MECHANICAL") and section_counter>2:
        comp_outline.append([float(emprecords[1])*emp_unit,float(emprecords[2])*emp_unit,float(emprecords[3])]) #add point of outline
   FreeCAD.Console.PrintMessage("\n".join(passed_sections)+"\n")
   #Write file with list of footprint
   if IDF_diag==1:
     empfile=pythonopen(IDF_diag_path+"/footprint.lst", "w")
     for compx in comps:
       empfile.writelines(str(compx[1][1])+"\n")
     empfile.close()
   #End section of list footprint  
   comps=dict(comps)
   grp=doc.addObject("App::DocumentObjectGroup", "EMP Models")
   for place_item in placement:
     if place_item[1] in comps:
       doc_comp=doc.addObject("Part::Feature",place_item[0])
       FreeCAD.Console.PrintMessage("Adding EMP model "+str(place_item[0])+"\n")
       doc_comp.Shape=comps[place_item[1]][0]
       doc_comp.ViewObject.DisplayMode=EmpDisplayMode
       z_pos=0
       rotateY=0
       if place_item[6]=='BOTTOM':
          rotateY=pi
          z_pos=-board_thickness
       placmnt=Base.Placement(Base.Vector(place_item[3],place_item[4],z_pos),toQuaternion(rotateY,place_item[5]*pi/180,0))
       doc_comp.Placement=placmnt
       grp.addObject(doc_comp)
   return 1

def Process_comp_outline(doc,comp_outline,comp_height):
    """Process_comp_outline(doc,comp_outline,comp_height)->part shape
       Create solid component shape base on its outline"""
    vertex_index=-1; #presume no vertex
    out_shape=[]
    if comp_outline==[]:  #force 0.2mm circle shape for components without place outline definition
       comp_outline.append([0.0,0.0,0.0])
       comp_outline.append([0.1,0.0,360.0])
    for point in comp_outline:
       vertex=Base.Vector(point[0],point[1],0) 
       vertex_index+=1
       if vertex_index>0:
         if point[2]!=0 and point[2]!=360:
            out_shape.append(Part.Arc(prev_vertex,mid_point(prev_vertex,vertex,point[2]),vertex))
            FreeCAD.Console.PrintMessage("mid point "+str(mid_point)+"\n")
         elif point[2]==360:
            per_point=Per_point(prev_vertex,vertex)
            out_shape.append(Part.Arc(per_point,mid_point(per_point,vertex,point[2]/2),vertex))
            out_shape.append(Part.Arc(per_point,mid_point(per_point,vertex,-point[2]/2),vertex))
         else:
            out_shape.append(Part.LineSegment(prev_vertex,vertex))
       prev_vertex=vertex
    out_shape=Part.Shape(out_shape)
    out_shape=Part.Wire(out_shape.Edges)
    out_shape=Part.Face(out_shape)
    out_shape=out_shape.extrude(Base.Vector(0,0,comp_height))
    #Part.show(out_shape)
    return out_shape

def place_steps(doc,placement,board_thickness):
    """ place_steps(doc,placement,board_thickness)->place step models on board 

        list of models and path to step files is set at start of this script
                 model_tab_filename= "" &   step_path="" """
    model_file=pythonopen(model_tab_filename, "r")
    model_lines=model_file.readlines()
    model_file.close()   
    model_dict=[]
    if IDF_diag==1:
        model_file=pythonopen(IDF_diag_path+"/missing_models.lst", "w")
    keys=[]
    #prev_step="*?.*?" #hope nobody will insert this step filename
    step_dict=[]
    for model_line in model_lines:
        model_records=split_records(model_line)  
        if len(model_records)>1 and model_records[0] and not model_records[0] in keys:
           keys.append(model_records[0])  
           model_dict.append((str(model_records[0]).replace('"',''),str(model_records[1]).replace('"','')))
    model_dict=dict(model_dict)
    validkeys=filter(lambda x:x in  [place_item[2] for place_item in placement], model_dict.keys())
    FreeCAD.Console.PrintMessage("Step models to be loaded for footprints: "+str(validkeys)+"\n")
    grp=doc.addObject("App::DocumentObjectGroup", "Step Lib")
    for validkey in validkeys:
         ImportGui.insert(step_path+model_dict[validkey],FreeCAD.ActiveDocument.Name)
         #partName=FreeCAD.ActiveDocument.ActiveObject.Name
         impPart=FreeCAD.ActiveDocument.ActiveObject
         #impPart.Shape=FreeCAD.ActiveDocument.ActiveObject.Shape
         #impPart.ViewObject.DiffuseColor=FreeCAD.ActiveDocument.ActiveObject.ViewObject.DiffuseColor
         impPart.ViewObject.Visibility=0
         impPart.Label=validkey
         grp.addObject(impPart)
         step_dict.append((validkey,impPart))
         FreeCAD.Console.PrintMessage("Reading step file "+str(model_dict[validkey])+" for footprint "+str(validkey)+"\n")
    step_dict=dict(step_dict)
    grp=doc.addObject("App::DocumentObjectGroup", "Step Models")
    for place_item in placement:
      if place_item[2] in step_dict:
        step_model=doc.addObject("Part::Feature",place_item[0]+"_s")
        FreeCAD.Console.PrintMessage("Adding STEP model "+str(place_item[0])+"\n")
        #if prev_step!=place_item[2]:
        #   model0=Part.read(step_path+"/"+model_dict[place_item[2]])
        #   prev_step=place_item[2]
        step_model.Shape=step_dict[place_item[2]].Shape
        step_model.ViewObject.DiffuseColor=step_dict[place_item[2]].ViewObject.DiffuseColor
        z_pos=0
        rotateY=0
        if place_item[6]=='BOTTOM':
           rotateY=pi
           z_pos=-board_thickness
        placmnt=Base.Placement(Base.Vector(place_item[3],place_item[4],z_pos),toQuaternion(rotateY,place_item[5]*pi/180,0))
        step_model.Placement=placmnt
        grp.addObject(step_model)
      else: 
        if IDF_diag==1:
            model_file.writelines(str(place_item[0])+" "+str(place_item[2])+"\n")
            model_file.close() 
    
def toQuaternion(heading, attitude,bank): # rotation heading=around Y, attitude =around Z,  bank attitude =around X
    """toQuaternion(heading, attitude,bank)->FreeCAD.Base.Rotation(Quternion)"""
    c1 = cos(heading/2)
    s1 = sin(heading/2)
    c2 = cos(attitude/2)
    s2 = sin(attitude/2)
    c3 = cos(bank/2)
    s3 = sin(bank/2)
    c1c2 = c1*c2
    s1s2 = s1*s2
    w = c1c2*c3 - s1s2*s3
    x = c1c2*s3 + s1s2*c3
    y = s1*c2*c3 + c1*s2*s3
    z = c1*s2*c3 - s1*c2*s3
    return  FreeCAD.Base.Rotation(x,y,z,w)  

def Per_point(prev_vertex,vertex):
    """Per_point(center,vertex)->per point

       returns opposite perimeter point of circle"""
    #basic_angle=atan2(prev_vertex.y-vertex.y,prev_vertex.x-vertex.x)
    #shift=hypot(prev_vertex.y-vertex.y,prev_vertex.x-vertex.x)
    #perpoint=Base.Vector(prev_vertex.x+shift*cos(basic_angle),prev_vertex.y+shift*sin(basic_angle),0)
    perpoint=Base.Vector(2*prev_vertex.x-vertex.x,2*prev_vertex.y-vertex.y,0)
    return perpoint

  
