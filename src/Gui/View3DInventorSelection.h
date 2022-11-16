/****************************************************************************
 *   Copyright (c) 2022 Zheng Lei (realthunder) <realthunder.dev@gmail.com> *
 *                                                                          *
 *   This file is part of the FreeCAD CAx development system.               *
 *                                                                          *
 *   This library is free software; you can redistribute it and/or          *
 *   modify it under the terms of the GNU Library General Public            *
 *   License as published by the Free Software Foundation; either           *
 *   version 2 of the License, or (at your option) any later version.       *
 *                                                                          *
 *   This library  is distributed in the hope that it will be useful,       *
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of         *
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the          *
 *   GNU Library General Public License for more details.                   *
 *                                                                          *
 *   You should have received a copy of the GNU Library General Public      *
 *   License along with this library; see the file COPYING.LIB. If not,     *
 *   write to the Free Software Foundation, Inc., 59 Temple Place,          *
 *   Suite 330, Boston, MA  02111-1307, USA                                 *
 *                                                                          *
 ****************************************************************************/

#ifndef GUI_VIEW3DINVENTORSELECTION_H
#define GUI_VIEW3DINVENTORSELECTION_H

#include <map>
#include <string>
#include <Gui/Selection.h>

class SoGroup;
class SoNode;
class SoSeparator;

namespace Gui {

class Document;
class SoFCUnifiedSelection;

class GuiExport View3DInventorSelection
{
public:
    View3DInventorSelection(SoFCUnifiedSelection* root);
    ~View3DInventorSelection();

    void setDocument(Gui::Document *pcDocument) {
        guiDocument = pcDocument;
    }
    Gui::Document* getDocument() const {
        return guiDocument;
    }

    void checkGroupOnTop(const SelectionChanges &Reason);
    void clearGroupOnTop();

private:
    SoGroup * pcGroupOnTop;
    SoGroup * pcGroupOnTopSel;
    SoGroup * pcGroupOnTopPreSel;
    SoFCUnifiedSelection* selectionRoot;
    std::map<std::string, SoNode*> objectsOnTop;
    std::map<std::string, SoNode*> objectsOnTopPreSel;
    Gui::Document* guiDocument = nullptr;
};

} //namespace Gui

#endif // GUI_VIEW3DINVENTORSELECTION_H
