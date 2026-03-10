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

#pragma once

#include <QObject>
#include <QPointer>

#include <fastsignals/signal.h>

#include <App/PropertyUnits.h>
#include <Gui/ViewProviderDocumentObject.h>
#include <Mod/TechDraw/TechDrawGlobal.h>

#include <ViewProviderPageExtension.h>


namespace TechDraw
{
class DrawPage;
class DrawTemplate;
}// namespace TechDraw

namespace TechDrawGui
{

// ?? is changing the base type worthwhile here?  lint thinks so.
enum PageEditMode : std::int16_t {
    ShowDrawing = 10,
    ToggleUpdate = 11 };

class MDIViewPage;
class QGVPage;
class QGSPage;
class QGITemplate;

// NOLINTNEXTLINE
class TechDrawGuiExport ViewProviderPage: public Gui::ViewProviderDocumentObject,
                                          public ViewProviderPageExtension
{
    PROPERTY_HEADER_WITH_OVERRIDE(TechDrawGui::ViewProviderPage);

public:
    /// constructor
    ViewProviderPage();
    /// destructor
    ~ViewProviderPage() override;

// NOLINTBEGIN
    App::PropertyBool ShowFrames;
    App::PropertyBool ShowGrid;
    App::PropertyDistance GridSpacing;
// NOLINTEND

    void attach(App::DocumentObject* pcFeat) override;

    bool canDragObjects() const override;
    bool canDragObject(App::DocumentObject* docObj) const override;
    bool canDropObject(App::DocumentObject* docObj) const override;

    void dropObject(App::DocumentObject* docObj) override;
    void constDropObject(App::DocumentObject* docObj) const;

    bool canDropObjectEx(App::DocumentObject* obj, App::DocumentObject* owner, const char* subname,
                         const std::vector<std::string>& elements) const override;

    bool useNewSelectionModel() const override { return false; }
    /// Hides the view provider
    void hide() override;
    /// Shows the view provider
    void show() override;
    bool isShow() const override;

    /// Claim all the views for the page
    std::vector<App::DocumentObject*> claimChildren() const override;

    /// Is called by the tree if the user double click on the object
    bool doubleClicked() override;
    void setupContextMenu(QMenu* menu, QObject* receiver, const char* member) override;
    bool onDelete(const std::vector<std::string>& parms) override;
    void onChanged(const App::Property* prop) override;
    void updateData(const App::Property* prop) override;

    TechDraw::DrawPage* getDrawPage() const;
    TechDraw::DrawTemplate* getTemplate() const;
    QGITemplate* getQTemplate() const;

    //slots & connections
    void onGuiRepaint(const TechDraw::DrawPage* dp);

// NOLINTBEGIN
    using Connection = fastsignals::scoped_connection;
    Connection connectGuiRepaint;
// NOLINTEND

    void unsetEdit(int ModNum) override;
    MDIViewPage* getMDIViewPage() const;
    bool showMDIViewPage();
    void removeMDIView();
    void switchToMdiViewPage();

    Gui::MDIView* getMDIView() const override;

    bool getFrameState() const;
    void setFrameState(bool state);
    void toggleFrameState();

    void setTemplateMarkers(bool state) const;

    bool canDelete(App::DocumentObject* obj) const override;

    void setGrid();

    QGSPage* getQGSPage() const;
    QGVPage* getQGVPage() const;

    ViewProviderPageExtension* getVPPExtension() const;

    const char* whoAmI() const;

    void fixSceneDependencies();

    void redrawPage() const;

protected:
    bool setEdit(int ModNum) override;
    void createMDIViewPage();

private:
    QPointer<MDIViewPage> m_mdiView;
    std::string m_pageName;
    QPointer<QGVPage> m_graphicsView;
    QGSPage* m_graphicsScene;

    bool m_frameToggle{false};      // replacement for ShowFrame property to avoid marking document changed
};

}// namespace TechDrawGui