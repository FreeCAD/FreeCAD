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

#ifndef TECHDRAWGUI_VIEWPROVIDERVIEW_H
#define TECHDRAWGUI_VIEWPROVIDERVIEW_H

#include <Mod/TechDraw/TechDrawGlobal.h>

#include <boost_signals2.hpp> 

#include <Gui/ViewProviderDocumentObject.h>
#include <Mod/TechDraw/App/DrawView.h>


namespace TechDraw {
class DrawView;
}

namespace TechDrawGui {
class QGIView;
class MDIViewPage;

class TechDrawGuiExport ViewProviderDrawingView : public Gui::ViewProviderDocumentObject
{
    PROPERTY_HEADER_WITH_OVERRIDE(TechDrawGui::ViewProviderDrawingView);

public:
    /// constructor
    ViewProviderDrawingView();
    /// destructor
    virtual ~ViewProviderDrawingView();

    App::PropertyBool  KeepLabel;

    virtual void attach(App::DocumentObject *) override;
    virtual void setDisplayMode(const char* ModeName) override;
    virtual bool useNewSelectionModel(void) const override {return false;}
    /// returns a list of all possible modes
    virtual std::vector<std::string> getDisplayModes(void) const override;
    /// Hide the object in the view
    virtual void hide(void) override;
    /// Show the object in the view
    virtual void show(void) override;
    virtual bool isShow(void) const override;

    virtual void onChanged(const App::Property *prop) override;
    virtual void updateData(const App::Property*) override;
    virtual void unsetEdit(int ModNum) override;

    QGIView* getQView(void);
    MDIViewPage* getMDIViewPage() const;
    virtual Gui::MDIView *getMDIView() const override;

    /** @name Restoring view provider from document load */
    //@{
    virtual void startRestoring() override;
    virtual void finishRestoring() override;
    //@}

    virtual TechDraw::DrawView* getViewObject() const;
    
    void onGuiRepaint(const TechDraw::DrawView* dv); 
    typedef boost::signals2::scoped_connection Connection;
    Connection connectGuiRepaint;
    

private:
    bool m_docReady;                                                   //sb MDI + QGraphicsScene ready

};

} // namespace TechDrawGui


#endif // TECHDRAWGUI_VIEWPROVIDERVIEW_H
