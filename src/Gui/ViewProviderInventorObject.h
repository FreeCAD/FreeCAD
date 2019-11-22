/***************************************************************************
 *   Copyright (c) 2007 Werner Mayer <wmayer[at]users.sourceforge.net>     *
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


#ifndef GUI_VIEWPROVIDER_INVENTOROBJECT_H
#define GUI_VIEWPROVIDER_INVENTOROBJECT_H

#include "ViewProviderDocumentObject.h"


namespace Gui
{

class GuiExport ViewProviderInventorObject : public ViewProviderDocumentObject
{
    PROPERTY_HEADER(Gui::ViewProviderInventorObject);

public:
    /// constructor.
    ViewProviderInventorObject();

    /// destructor.
    ~ViewProviderInventorObject();

    void attach(App::DocumentObject *pcObject);
    void setDisplayMode(const char* ModeName);
    std::vector<std::string> getDisplayModes() const;
    void updateData(const App::Property*);
    bool useNewSelectionModel(void) const {return true;}

private:
    void adjustSelectionNodes(SoNode* child, const char* docname, const char* objname);

protected:
    SoSeparator  *pcBuffer;
    SoSeparator  *pcFile;
};

} //namespace Gui


#endif // GUI_VIEWPROVIDER_INVENTOROBJECT_H
