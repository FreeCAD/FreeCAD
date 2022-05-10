/***************************************************************************
 *   Copyright (c) 2004 Jürgen Riegel <juergen.riegel@web.de>              *
 *   Copyright (c) 2012 Luke Parry <l.parry@warwick.ac.uk>                 *
 *   Copyright (c) 2019 Franck Jullien <franck.jullien@gmail.com>          *
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

#ifndef DRAWINGGUI_VIEWPROVIDERBALLOON_H
#define DRAWINGGUI_VIEWPROVIDERBALLOON_H

#include <App/PropertyUnits.h>
#include <Mod/TechDraw/App/DrawViewBalloon.h>

#include "ViewProviderDrawingView.h"


namespace TechDrawGui {


class TechDrawGuiExport ViewProviderBalloon : public ViewProviderDrawingView
{
    PROPERTY_HEADER(TechDrawGui::ViewProviderBalloon);

public:
    /// constructor
    ViewProviderBalloon();
    /// destructor
    virtual ~ViewProviderBalloon();

    App::PropertyFont   Font;
    App::PropertyLength Fontsize;
    App::PropertyLength LineWidth;
    App::PropertyBool   LineVisible;
    App::PropertyColor  Color;

    virtual void attach(App::DocumentObject *);
    virtual void setDisplayMode(const char* ModeName);
    virtual bool useNewSelectionModel(void) const {return false;}
    /// returns a list of all possible modes
    virtual std::vector<std::string> getDisplayModes(void) const;
    virtual void updateData(const App::Property*);
    virtual void onChanged(const App::Property* p);
    virtual void setupContextMenu(QMenu*, QObject*, const char*);
    virtual bool setEdit(int ModNum);
    virtual void unsetEdit(int ModNum);
    virtual bool doubleClicked(void);
    virtual bool canDelete(App::DocumentObject* obj) const;

    virtual TechDraw::DrawViewBalloon* getViewObject() const;

protected:
    virtual void handleChangedPropertyType(Base::XMLReader &reader, const char *TypeName, App::Property * prop);
};

} // namespace TechDrawGui


#endif // DRAWINGGUI_VIEWPROVIDERBALLOON_H
