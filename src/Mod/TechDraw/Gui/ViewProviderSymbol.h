/***************************************************************************
 *   Copyright (c) 2004 Jürgen Riegel <juergen.riegel@web.de>              *
 *   Copyright (c) 2012 Luke Parry <l.parry@warwick.ac.uk>                 *
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


#ifndef DRAWINGGUI_VIEWPROVIDERSYMBOL_H
#define DRAWINGGUI_VIEWPROVIDERSYMBOL_H

#include <Mod/TechDraw/App/DrawView.h>
#include <Mod/TechDraw/App/DrawViewSymbol.h>
#include "ViewProviderDrawingView.h"
#include "ViewProviderDrawingView.h"

namespace TechDrawGui {


class TechDrawGuiExport ViewProviderSymbol : public ViewProviderDrawingView
{
    PROPERTY_HEADER(TechDrawGui::ViewProviderSymbol);

public:
    /// constructor
    ViewProviderSymbol();
    /// destructor
    virtual ~ViewProviderSymbol();


    virtual void attach(App::DocumentObject *);
    virtual void setDisplayMode(const char* ModeName);
    virtual bool useNewSelectionModel(void) const {return false;}
    /// returns a list of all possible modes
    virtual std::vector<std::string> getDisplayModes(void) const;
    virtual void updateData(const App::Property*);

    virtual TechDraw::DrawViewSymbol* getViewObject() const;
};

class TechDrawGuiExport ViewProviderDraft : public ViewProviderSymbol
{
    PROPERTY_HEADER(TechDrawGui::ViewProviderDraft);

public:
    /// constructor
    ViewProviderDraft();
    /// destructor
    virtual ~ViewProviderDraft();
};

class TechDrawGuiExport ViewProviderArch : public ViewProviderSymbol
{
    PROPERTY_HEADER(TechDrawGui::ViewProviderArch);

public:
    /// constructor
    ViewProviderArch();
    /// destructor
    virtual ~ViewProviderArch();
};

} // namespace TechDrawGui


#endif // DRAWINGGUI_VIEWPROVIDERSYMBOL_H
