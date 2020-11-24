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


#ifndef PATH_TOOLTABLE_H
#define PATH_TOOLTABLE_H

#include <vector>
#include <string>
#include <map>
#include <Base/Persistence.h>
#include "Tool.h"

namespace Path
{    /** The representation of a table of tools */
    class PathExport Tooltable : public Base::Persistence
    {
    TYPESYSTEM_HEADER();

    public:
        //constructors
        Tooltable();
        ~Tooltable();

        // from base class
        virtual unsigned int getMemSize (void) const;
        virtual void Save (Base::Writer &/*writer*/) const;
        virtual void Restore(Base::XMLReader &/*reader*/);

        // new functions
        void addTool(const Tool &tool); // adds a tool at the end
        void setTool(const Tool &tool, int); // inserts a tool
        void deleteTool(int); // deletes a tool

        // auto
        unsigned int getSize(void) const {return Tools.size();}
        const Tool &getTool(int pos) {return *Tools[pos];}
        const std::map<int,Tool*> &getTools(void) const {return Tools;}
        bool hasTool(int pos) const {return (Tools.count(pos) != 0);}

        // attributes
        std::map<int,Tool*> Tools;
        int Version;
        std::string Name;
    };

} //namespace Path

#endif // PATH_TOOLTABLE_H
