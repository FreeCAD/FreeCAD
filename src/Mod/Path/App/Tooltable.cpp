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


#include "PreCompiled.h"

#ifndef _PreComp_

#endif
#include <Base/Writer.h>
#include <Base/Reader.h>
#include <Base/Exception.h>
#include "Tooltable.h"

using namespace Base;
using namespace Path;

TYPESYSTEM_SOURCE(Path::Tooltable , Base::Persistence)

Tooltable::Tooltable()
{
    Version = 0;
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
