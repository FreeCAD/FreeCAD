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


#ifndef DRAWINGGUI_VIEWPROVIDERVIEW_H
#define DRAWINGGUI_VIEWPROVIDERVIEW_H

#include <Gui/ViewProviderFeature.h>
#include <Gui/ViewProviderDocumentObjectGroup.h>

#include <Mod/TechDraw/App/DrawView.h>
#include "QGIView.h"

namespace TechDraw {
class DrawView;
}

namespace TechDrawGui {
class QGIView;
class MDIViewPage;

class TechDrawGuiExport ViewProviderDrawingView : public Gui::ViewProviderDocumentObject
{
    PROPERTY_HEADER(TechDrawGui::ViewProviderDrawingView);

public:
    /// constructor
    ViewProviderDrawingView();
    /// destructor
    virtual ~ViewProviderDrawingView();

    App::PropertyBool  KeepLabel;

    virtual void attach(App::DocumentObject *);
    virtual void setDisplayMode(const char* ModeName);
    virtual bool useNewSelectionModel(void) const {return false;}
    /// returns a list of all possible modes
    virtual std::vector<std::string> getDisplayModes(void) const;
    /// Hide the object in the view
    virtual void hide(void);
    /// Show the object in the view
    virtual void show(void);
    virtual bool isShow(void) const;

    virtual void onChanged(const App::Property *prop);
    virtual void updateData(const App::Property*);
    virtual void unsetEdit(int ModNum);

    QGIView* getQView(void);
    MDIViewPage* getMDIViewPage() const;

    /** @name Restoring view provider from document load */
    //@{
    virtual void startRestoring();
    virtual void finishRestoring();
    //@}

    virtual TechDraw::DrawView* getViewObject() const;
    
    void onGuiRepaint(const TechDraw::DrawView* dv); 
    typedef boost::signals::scoped_connection Connection;
    Connection connectGuiRepaint;
    

private:
    bool m_docReady;                                                   //sb MDI + QGraphicsScene ready

};

} // namespace TechDrawGui


#endif // DRAWINGGUI_VIEWPROVIDERVIEW_H
