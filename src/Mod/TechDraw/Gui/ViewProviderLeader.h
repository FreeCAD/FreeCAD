/***************************************************************************
 *   Copyright (c) 2004 Jürgen Riegel <juergen.riegel@web.de>              *
 *   Copyright (c) 2019 Wanderer Fan <wandererfan@gmail.com>               *
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


#ifndef DRAWINGGUI_VIEWPROVIDERLEADER_H
#define DRAWINGGUI_VIEWPROVIDERLEADER_H

#include <App/PropertyUnits.h>

#include <Mod/TechDraw/App/DrawLeaderLine.h>

#include "ViewProviderDrawingView.h"

namespace TechDraw {
class DrawTextLeader;
class DrawLeaderLine;
}

namespace TechDrawGui {

class TechDrawGuiExport ViewProviderLeader : public ViewProviderDrawingView
{
    PROPERTY_HEADER(TechDrawGui::ViewProviderLeader);

public:
    /// constructor
    ViewProviderLeader();
    /// destructor
    virtual ~ViewProviderLeader();

    App::PropertyFloat    LineWidth;
    App::PropertyInteger  LineStyle;
    App::PropertyColor    Color;

    virtual void attach(App::DocumentObject *);
/*    virtual void setDisplayMode(const char* ModeName);*/
    virtual bool useNewSelectionModel(void) const {return false;}
/*    virtual std::vector<std::string> getDisplayModes(void) const;*/
    virtual void updateData(const App::Property*);
    virtual void onChanged(const App::Property* p);
    virtual bool setEdit(int ModNum);
    virtual void unsetEdit(int ModNum);
    virtual bool doubleClicked(void);

    virtual TechDraw::DrawLeaderLine* getViewObject() const;
    TechDraw::DrawLeaderLine* getFeature()  const;

protected:
    double getDefLineWeight(void);
    App::Color getDefLineColor(void);
};

//******************************************************************************
class TechDrawGuiExport ViewProviderTextLeader : public ViewProviderLeader
{
    PROPERTY_HEADER(TechDrawGui::ViewProviderTextLeader);

public:
    ViewProviderTextLeader();
    virtual ~ViewProviderTextLeader();

    App::PropertyFont     Font;
    App::PropertyLength   Fontsize;
    App::PropertyFloat    MaxWidth;
    App::PropertyBool     ShowFrame;

    virtual void attach(App::DocumentObject *);
    virtual void updateData(const App::Property*);
    virtual void onChanged(const App::Property* p);
    virtual bool setEdit(int ModNum);
    virtual void unsetEdit(int ModNum);
    virtual bool doubleClicked(void);

/*    virtual TechDraw::DrawTextLeader* getViewObject()  const;*/
    TechDraw::DrawTextLeader* getFeature()  const;

protected:
    std::string getDefFont(void) const;
    double getDefFontSize(void) const;

};


} // namespace TechDrawGui


#endif // DRAWINGGUI_VIEWPROVIDERLEADER_H
