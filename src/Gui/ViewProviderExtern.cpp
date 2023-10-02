/***************************************************************************
 *   Copyright (c) 2004 Jürgen Riegel <juergen.riegel@web.de>              *
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
# include <cstring>
# include <Inventor/nodes/SoSeparator.h>
# include <Inventor/nodes/SoSwitch.h>
#endif

#include <Base/Exception.h>
#include <Base/Stream.h>

#include "ViewProviderExtern.h"
#include "SoFCSelection.h"


using std::vector;
using std::string;


using namespace Gui;

PROPERTY_SOURCE(Gui::ViewProviderExtern, Gui::ViewProvider)


ViewProviderExtern::ViewProviderExtern() = default;

ViewProviderExtern::~ViewProviderExtern() = default;

void ViewProviderExtern::setModeByString(const char* name, const char* ivFragment)
{
    SoInput in;
    in.setBuffer((void*)ivFragment,std::strlen(ivFragment));
    setModeBySoInput(name,in);
}

void ViewProviderExtern::setModeByFile(const char* name, const char* ivFileName)
{
    SoInput in;
    Base::ifstream file(ivFileName, std::ios::in | std::ios::binary);
    if (file){
        std::streamoff size = 0;
        std::streambuf* buf = file.rdbuf();
        if (buf) {
            std::streamoff curr;
            curr = buf->pubseekoff(0, std::ios::cur, std::ios::in);
            size = buf->pubseekoff(0, std::ios::end, std::ios::in);
            buf->pubseekoff(curr, std::ios::beg, std::ios::in);
        }

        // read in the file
        std::vector<unsigned char> content;
        content.reserve(size);
        unsigned char c;
        while (file.get((char&)c)) {
            content.push_back(c);
        }

        file.close();
        in.setBuffer(&(content[0]),content.size());
        setModeBySoInput(name,in);
    }
}

void ViewProviderExtern::setModeBySoInput(const char* name, SoInput &ivFileInput)
{
    SoSeparator * root = SoDB::readAll(&ivFileInput);
    if (root) {
        std::vector<std::string>::iterator pos = std::find<std::vector<std::string>
           ::iterator,string>(modes.begin(),modes.end(),string(name));
        if (pos == modes.end()) {
            // new mode
            modes.emplace_back(name);
            addDisplayMaskMode(root, name);
            setDisplayMaskMode(name);
        }
        else {
            // existing mode
            // not implemented yet
            assert(0);
            root->unref();
        }
    }
    else {
        throw Base::RuntimeError("No valid Inventor input");
    }

    return;
}

void ViewProviderExtern::adjustDocumentName(const char* docname)
{
    for (int i=0; i<this->pcModeSwitch->getNumChildren(); i++) {
        SoNode* child = this->pcModeSwitch->getChild(i);
        adjustRecursiveDocumentName(child, docname);
    }
}

void ViewProviderExtern::adjustRecursiveDocumentName(SoNode* child, const char* docname)
{
    if (child->getTypeId().isDerivedFrom(SoFCSelection::getClassTypeId())) {
        static_cast<SoFCSelection*>(child)->documentName = docname;
    }
    else if (child->getTypeId().isDerivedFrom( SoGroup::getClassTypeId())) {
        SoGroup* group = static_cast<SoGroup*>(child);
        for (int i=0; i<group->getNumChildren(); i++) {
            SoNode* subchild = group->getChild(i);
            adjustRecursiveDocumentName(subchild, docname);
        }
    }
}

const char* ViewProviderExtern::getDefaultDisplayMode() const
{
    // returns the first item of the available modes
    return (modes.empty() ? "" : modes.front().c_str());
}

std::vector<std::string> ViewProviderExtern::getDisplayModes() const
{
    return modes;
}
