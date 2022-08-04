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

#ifndef DRAWINGGUI_VIEWPROVIDERPAGE_H
#define DRAWINGGUI_VIEWPROVIDERPAGE_H

#include "PreCompiled.h"
#ifndef _PreComp_
#include <QPointer>
#endif

#include <Mod/TechDraw/TechDrawGlobal.h>

#include <boost_signals2.hpp>

#include <App/PropertyUnits.h>
#include <Gui/ViewProviderDocumentObject.h>


namespace TechDraw{
    class DrawPage;
}

namespace TechDrawGui {

class MDIViewPage;
class QGVPage;
class QGSPage;

class TechDrawGuiExport ViewProviderPage : public Gui::ViewProviderDocumentObject
{
    PROPERTY_HEADER_WITH_OVERRIDE(TechDrawGui::ViewProviderPage);

public:
    /// constructor
    ViewProviderPage();
    /// destructor
    ~ViewProviderPage() override;

    App::PropertyBool  ShowFrames;
    App::PropertyBool  ShowGrid;
    App::PropertyDistance GridSpacing;

    void attach(App::DocumentObject *) override;
    void setDisplayMode(const char* ModeName) override;
    bool useNewSelectionModel() const override {return false;}
    /// returns a list of all possible modes
    std::vector<std::string> getDisplayModes() const override;
    /// Hides the view provider
    void hide() override;
    /// Shows the view provider
    void show() override;
    bool isShow() const override;

    /// Claim all the views for the page
    std::vector<App::DocumentObject*> claimChildren() const override;

    /// Is called by the tree if the user double click on the object
    bool doubleClicked() override;
    void setupContextMenu(QMenu*, QObject*, const char*) override;
    bool onDelete(const std::vector<std::string> &) override;
    void onChanged(const App::Property *prop) override;
    void updateData(const App::Property* prop) override;
    void startRestoring() override;
    void finishRestoring() override;
    bool isRestoring() {return !m_docReady;}

    TechDraw::DrawPage* getDrawPage() const;
    void onGuiRepaint(const TechDraw::DrawPage* dp); 
    typedef boost::signals2::scoped_connection Connection;
    Connection connectGuiRepaint;

    void unsetEdit(int ModNum) override;
    MDIViewPage* getMDIViewPage() const;
    bool showMDIViewPage();
    void removeMDIView();

    Gui::MDIView *getMDIView() const override;

    bool getFrameState();
    void setFrameState(bool state);
    void toggleFrameState();
    void setTemplateMarkers(bool state);
    QGVPage *getGraphicsView() { return m_graphicsView; }
    QGSPage* getGraphicsScene() { return m_graphicsScene; }
    void setGraphicsView(QGVPage* gv);
    void setGraphicsScene(QGSPage* gs);
    bool canDelete(App::DocumentObject* obj) const override;
    void  setGrid();

protected:
    bool setEdit(int ModNum) override;

private:
    QPointer<MDIViewPage> m_mdiView;
    bool m_docReady;
    std::string m_pageName;
    QGVPage* m_graphicsView;
    QGSPage* m_graphicsScene;
};

} // namespace TechDrawGui


#endif // DRAWINGGUI_VIEWPROVIDERPAGE_H
