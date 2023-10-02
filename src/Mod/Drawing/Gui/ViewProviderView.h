/***************************************************************************
 *   Copyright (c) 2004 Jürgen Riegel <juergen.riegel@web.de>              *
 *                                                                         *
 *   This file is Drawing of the FreeCAD CAx development system.           *
 *                                                                         *
 *   This library is free software; you can redistribute it and/or         *
 *   modify it under the terms of the GNU Library General Public           *
 *   License as published by the Free Software Foundation; either          *
 *   version 2 of the License, or (at your option) any later version.      *
 *                                                                         *
 *   This library  is distributed in the hope that it will be useful,      *
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of        *
 *   MERCHANTABILITY or FITNESS FOR A DrawingICULAR PURPOSE.  See the      *
 *   GNU Library General Public License for more details.                  *
 *                                                                         *
 *   You should have received a copy of the GNU Library General Public     *
 *   License along with this library; see the file COPYING.LIB. If not,    *
 *   write to the Free Software Foundation, Inc., 59 Temple Place,         *
 *   Suite 330, Boston, MA  02111-1307, USA                                *
 *                                                                         *
 ***************************************************************************/

#ifndef DRAWINGGUI_VIEWPROVIDERVIEW_H
#define DRAWINGGUI_VIEWPROVIDERVIEW_H

#include <Gui/ViewProviderDocumentObjectGroup.h>


namespace DrawingGui
{


class DrawingGuiExport ViewProviderDrawingView: public Gui::ViewProviderDocumentObject
{
    PROPERTY_HEADER(DrawingGui::ViewProviderDrawingView);

public:
    /// constructor
    ViewProviderDrawingView();
    /// destructor
    virtual ~ViewProviderDrawingView();


    virtual void attach(App::DocumentObject*);
    virtual void setDisplayMode(const char* ModeName);
    virtual bool useNewSelectionModel(void) const
    {
        return false;
    }
    /// returns a list of all possible modes
    virtual std::vector<std::string> getDisplayModes(void) const;
    virtual void updateData(const App::Property*);
    /// Hide the object in the view
    virtual void hide(void);
    /// Show the object in the view
    virtual void show(void);
    virtual bool isShow(void) const;

    /** @name Restoring view provider from document load */
    //@{
    virtual void startRestoring();
    virtual void finishRestoring();
    //@}
};

using ViewProviderDrawingViewPython = Gui::ViewProviderPythonFeatureT<ViewProviderDrawingView>;

class DrawingGuiExport ViewProviderDrawingClip: public Gui::ViewProviderDocumentObjectGroup
{
    PROPERTY_HEADER(DrawingGui::ViewProviderDrawingClip);

public:
    /// constructor
    ViewProviderDrawingClip();
    /// destructor
    virtual ~ViewProviderDrawingClip();


    virtual void attach(App::DocumentObject*);
    virtual void setDisplayMode(const char* ModeName);
    virtual bool useNewSelectionModel(void) const
    {
        return false;
    }
    /// returns a list of all possible modes
    virtual std::vector<std::string> getDisplayModes(void) const;
    virtual void updateData(const App::Property*);
    /// Hide the object in the view
    virtual void hide(void);
    /// Show the object in the view
    virtual void show(void);
    virtual bool isShow(void) const;

    /** @name Restoring view provider from document load */
    //@{
    virtual void startRestoring();
    virtual void finishRestoring();
    //@}
};

}  // namespace DrawingGui


#endif  // DRAWINGGUI_VIEWPROVIDERVIEW_H
