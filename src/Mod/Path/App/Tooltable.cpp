/***************************************************************************
 *   Copyright (c) Yorik van Havre (yorik@uncreated.net) 2014              *
 *                                                                         *
 *   This file is part of the FreeCAD CAx development system.              *
 *                                                                         *
 *   This library is free software; you can redistribute it and/or         *
 *   modify it under the terms of the GNU Library General Public           *
 *   License as published by the Free Software Foundation; either          *
 *   version 2 of the License, or (at your option) any later version.      *
 *                                                                         *
 *   This library  is distributed in the hope that it will be useful,      *
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of        *
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the         *
 *   GNU Library General Public License for more details.                  *
 *                                                                         *
 *   You should have received a copy of the GNU Library General Public     *
 *   License along with this library; see the file COPYING.LIB. If not,    *
 *   write to the Free Software Foundation, Inc., 59 Temple Place,         *
 *   Suite 330, Boston, MA  02111-1307, USA                                *
 *                                                                         *
 ***************************************************************************/


#include "PreCompiled.h"

#ifndef _PreComp_

#endif
#include <Base/Writer.h>
#include <Base/Reader.h>
#include <Base/Exception.h>
#include "Tooltable.h"

using namespace Base;
using namespace Path;


// TOOL


TYPESYSTEM_SOURCE(Path::Tool , Base::Persistence);

// Constructors & destructors

Tool::Tool(const char* name,
           ToolType type,
           ToolMaterial /*material*/,
           double diameter,
           double lengthoffset,
           double flatradius,
           double cornerradius,
           double cuttingedgeangle,
           double cuttingedgeheight)
:Name(name),Type(type),Material(MATUNDEFINED),Diameter(diameter),LengthOffset(lengthoffset),
FlatRadius(flatradius),CornerRadius(cornerradius),CuttingEdgeAngle(cuttingedgeangle),
CuttingEdgeHeight(cuttingedgeheight)
{
}

Tool::Tool()
{
    Type = UNDEFINED;
    Material = MATUNDEFINED;
    Diameter = 0;
    LengthOffset = 0;
    FlatRadius = 0;
    CornerRadius = 0;
    CuttingEdgeAngle = 180;
    CuttingEdgeHeight = 0;
}

Tool::~Tool()
{
}

// Reimplemented from base class

unsigned int Tool::getMemSize (void) const
{
    return 0;
}

void Tool::Save (Writer &writer) const
{
    writer.Stream() << writer.ind() << "<Tool "
                    << "name=\"" << encodeAttribute(Name) << "\" "
                    << "diameter=\"" << Diameter << "\" "
                    << "length=\"" << LengthOffset << "\" "
                    << "flat=\"" <<  FlatRadius << "\" "
                    << "corner=\"" << CornerRadius << "\" "
                    << "angle=\"" << CuttingEdgeAngle << "\" "
                    << "height=\"" << CuttingEdgeHeight << "\" "
                    << "type=\"" << TypeName(Type) << "\" "
                    << "mat=\"" << MaterialName(Material) << "\" "
                    << "/>" << std::endl;
}

void Tool::Restore(XMLReader &reader)
{
    reader.readElement("Tool");
    Name = reader.getAttribute("name");
    Diameter          = reader.hasAttribute("diameter") ? (double) reader.getAttributeAsFloat("diameter") : 0.0;
    LengthOffset      = reader.hasAttribute("length")   ? (double) reader.getAttributeAsFloat("length")   : 0.0;
    FlatRadius        = reader.hasAttribute("flat")     ? (double) reader.getAttributeAsFloat("flat")     : 0.0;
    CornerRadius      = reader.hasAttribute("corner")   ? (double) reader.getAttributeAsFloat("corner")   : 0.0;
    CuttingEdgeAngle  = reader.hasAttribute("angle")    ? (double) reader.getAttributeAsFloat("angle")    : 180.0;
    CuttingEdgeHeight = reader.hasAttribute("height")   ? (double) reader.getAttributeAsFloat("height")   : 0.0;
    std::string type  = reader.hasAttribute("type")     ? reader.getAttribute("type") : "";
    std::string mat   = reader.hasAttribute("mat")      ? reader.getAttribute("mat")  : "";

    Type = getToolType(type);
    Material = getToolMaterial(mat);


}

const std::vector<std::string> Tool::ToolTypes(void)
{
    std::vector<std::string> toolTypes(13);
    toolTypes[0] ="EndMill";
    toolTypes[1] ="Drill";
    toolTypes[2] ="CenterDrill";
    toolTypes[3] ="CounterSink";
    toolTypes[4] ="CounterBore";
    toolTypes[5] ="FlyCutter";
    toolTypes[6] ="Reamer";
    toolTypes[7] ="Tap";
    toolTypes[8] ="SlotCutter";
    toolTypes[9] ="BallEndMill";
    toolTypes[10] ="ChamferMill";
    toolTypes[11] ="CornerRound";
    toolTypes[12] ="Engraver";
    return toolTypes;

}

const std::vector<std::string> Tool::ToolMaterials(void)
{
    std::vector<std::string> toolMat(7);
    toolMat[0] ="Carbide";
    toolMat[1] ="HighSpeedSteel";
    toolMat[2] ="HighCarbonToolSteel";
    toolMat[3] ="CastAlloy";
    toolMat[4] ="Ceramics";
    toolMat[5] ="Diamond";
    toolMat[6] ="Sialon";
    return toolMat;

}

Tool::ToolType Tool::getToolType(std::string type)
{
    Tool::ToolType Type;
    if(type=="EndMill")
        Type = Tool::ENDMILL;
    else if(type=="Drill")
        Type = Tool::DRILL;
    else if(type=="CenterDrill")
        Type = Tool::CENTERDRILL;
    else if(type=="CounterSink")
        Type = Tool::COUNTERSINK;
    else if(type=="CounterBore")
        Type = Tool::COUNTERBORE;
    else if(type=="FlyCutter")
        Type = Tool::FLYCUTTER;
    else if(type=="Reamer")
        Type = Tool::REAMER;
    else if(type=="Tap")
        Type = Tool::TAP;
    else if(type=="SlotCutter")
        Type = Tool::SLOTCUTTER;
    else if(type=="BallEndMill")
        Type = Tool::BALLENDMILL;
    else if(type=="ChamferMill")
        Type = Tool::CHAMFERMILL;
    else if(type=="CornerRound")
        Type = Tool::CORNERROUND;
    else if(type=="Engraver")
        Type = Tool::ENGRAVER;
    else
        Type = Tool::UNDEFINED;

    return Type;
}

Tool::ToolMaterial Tool::getToolMaterial(std::string mat)
{
    Tool::ToolMaterial Material;
    if(mat=="Carbide")
        Material = Tool::CARBIDE;
    else if(mat=="HighSpeedSteel")
        Material = Tool::HIGHSPEEDSTEEL;
    else if(mat=="HighCarbonToolSteel")
        Material = Tool::HIGHCARBONTOOLSTEEL;
    else if(mat=="CastAlloy")
        Material = Tool::CASTALLOY;
    else if(mat=="Ceramics")
        Material = Tool::CERAMICS;
    else if(mat=="Diamond")
        Material = Tool::DIAMOND;
    else if(mat=="Sialon")
        Material = Tool::SIALON;
    else
        Material = Tool::MATUNDEFINED;

    return Material;
}

const char* Tool::TypeName(Tool::ToolType typ) {
    switch (typ) {
      case Tool::DRILL:
        return "Drill";
      case Tool::CENTERDRILL:
        return "CenterDrill";
      case Tool::COUNTERSINK:
        return "CounterSink";
      case Tool::COUNTERBORE:
        return "CounterBore";
      case Tool::FLYCUTTER:
        return "FlyCutter";
      case Tool::REAMER:
        return "Reamer";
      case Tool::TAP:
        return "Tap";
      case Tool::ENDMILL:
        return "EndMill";
      case Tool::SLOTCUTTER:
        return "SlotCutter";
      case Tool::BALLENDMILL:
        return "BallEndMill";
      case Tool::CHAMFERMILL:
        return "ChamferMill";
      case Tool::CORNERROUND:
        return "CornerRound";
      case Tool::ENGRAVER:
        return "Engraver";
      case Tool::UNDEFINED:
        return "Undefined";
    }
    return "Undefined";
}

const char* Tool::MaterialName(Tool::ToolMaterial mat)
{
  switch (mat) {
    case Tool::HIGHSPEEDSTEEL:
        return "HighSpeedSteel";
    case Tool::CARBIDE:
        return "Carbide";
    case Tool::HIGHCARBONTOOLSTEEL:
        return "HighCarbonToolSteel";
    case Tool::CASTALLOY:
        return "CastAlloy";
    case Tool::CERAMICS:
        return "Ceramics";
    case Tool::DIAMOND:
        return "Diamond";
    case Tool::SIALON:
        return "Sialon";
    case Tool::MATUNDEFINED:
        return "Undefined";
  }
  return "Undefined";
}

// TOOLTABLE



TYPESYSTEM_SOURCE(Path::Tooltable , Base::Persistence);

Tooltable::Tooltable()
{
}

Tooltable::~Tooltable()
{
}

void Tooltable::addTool(const Tool &tool)
{
    Tool *tmp = new Tool(tool);
    if (!Tools.empty()) {
        int max = 0;
        for(std::map<int,Tool*>::const_iterator i = Tools.begin(); i != Tools.end(); ++i) {
            int k = i->first;
            if (k > max)
                max = k;
        }
        Tools[max+1]= tmp;
    } else
        Tools[1] = tmp;
}

void Tooltable::setTool(const Tool &tool, int pos)
{
    if (pos == -1) {
        addTool(tool);
    } else {
        Tool *tmp = new Tool(tool);
        Tools[pos] = tmp;
    }
}

void Tooltable::deleteTool(int pos)
{
    if (Tools.find(pos) != Tools.end()) {
        Tools.erase(pos);
    } else {
        throw Base::IndexError("Index not found");
    }
}

unsigned int Tooltable::getMemSize (void) const
{
    return 0;
}

void Tooltable::Save (Writer &writer) const
{
    writer.Stream() << writer.ind() << "<Tooltable count=\"" <<  getSize() <<"\">" << std::endl;
    writer.incInd();
    for(std::map<int,Tool*>::const_iterator i = Tools.begin(); i != Tools.end(); ++i) {
        int k = i->first;
        Tool *v = i->second;
        writer.Stream() << writer.ind() << "<Toolslot number=\"" << k << "\">" << std::endl;
        writer.incInd();
        v->Save(writer);
        writer.decInd();
        writer.Stream() << writer.ind() << "</Toolslot>" << std::endl;
    }
    writer.decInd();
    writer.Stream() << writer.ind() << "</Tooltable>" << std::endl ;

}

void Tooltable::Restore (XMLReader &reader)
{
    Tools.clear();
    reader.readElement("Tooltable");
    int count = reader.getAttributeAsInteger("count");
    for (int i = 0; i < count; i++) {
        reader.readElement("Toolslot");
        int id = reader.getAttributeAsInteger("number");
        Tool *tmp = new Tool();
        tmp->Restore(reader);
        Tools[id] = tmp;
    }
}
