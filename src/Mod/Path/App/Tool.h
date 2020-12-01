/***************************************************************************
 *   Copyright (c) 2014 Yorik van Havre <yorik@uncreated.net>              *
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

#ifndef PATH_TOOL_H
#define PATH_TOOL_H

#include <vector>
#include <string>
#include <map>
#include <Base/Persistence.h>

namespace Path
{

    /** The representation of a single tool */
    class PathExport Tool : public Base::Persistence
    {
    TYPESYSTEM_HEADER();

    public:
        enum ToolType {
            UNDEFINED,
            DRILL,
            CENTERDRILL,
            COUNTERSINK,
            COUNTERBORE,
            FLYCUTTER,
            REAMER,
            TAP,
            ENDMILL,
            SLOTCUTTER,
            BALLENDMILL,
            CHAMFERMILL,
            CORNERROUND,
            ENGRAVER };

        enum ToolMaterial {
            MATUNDEFINED,
            HIGHSPEEDSTEEL,
            HIGHCARBONTOOLSTEEL,
            CASTALLOY,
            CARBIDE,
            CERAMICS,
            DIAMOND,
            SIALON };

        //constructors
        Tool();
        Tool(const char* name,
             ToolType type=Tool::UNDEFINED,
             ToolMaterial material=Tool::MATUNDEFINED,
             double diameter=10.0,
             double lengthoffset=100,
             double flatradius=0,
             double cornerradius=0,
             double cuttingedgeangle=0,
             double cuttingedgeheight=0);
        ~Tool();

        // from base class
        virtual unsigned int getMemSize (void) const;
        virtual void Save (Base::Writer &/*writer*/) const;
        virtual void Restore(Base::XMLReader &/*reader*/);

        // attributes
        std::string Name;
        ToolType Type;
        ToolMaterial Material;
        double Diameter;
        double LengthOffset;
        double FlatRadius;
        double CornerRadius;
        double CuttingEdgeAngle;
        double CuttingEdgeHeight;

        static const std::vector<std::string> ToolTypes(void);
        static const std::vector<std::string> ToolMaterials(void);
        static const char* TypeName(ToolType typ);
        static ToolType getToolType(std::string type);
        static ToolMaterial getToolMaterial(std::string mat);
        static const char* MaterialName(ToolMaterial mat);
    };
} //namespace Path

#endif // PATH_TOOL_H
