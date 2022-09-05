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
class ViewProviderPage;

class TechDrawGuiExport ViewProviderDrawingView : public Gui::ViewProviderDocumentObject
{
    PROPERTY_HEADER_WITH_OVERRIDE(TechDrawGui::ViewProviderDrawingView);

public:
    /// constructor
    ViewProviderDrawingView();
    /// destructor
    ~ViewProviderDrawingView() override;

    App::PropertyBool  KeepLabel;

    void attach(App::DocumentObject *) override;
    bool useNewSelectionModel() const override {return false;}
    /// Hide the object in the view
    void hide() override;
    /// Show the object in the view
    void show() override;
    bool isShow() const override;

    void onChanged(const App::Property *prop) override;
    void updateData(const App::Property*) override;

    QGIView* getQView();
    MDIViewPage* getMDIViewPage() const;
    Gui::MDIView *getMDIView() const override;
    ViewProviderPage* getViewProviderPage() const;

    /** @name Restoring view provider from document load */
    //@{
    void startRestoring() override;
    void finishRestoring() override;
    //@}

    virtual TechDraw::DrawView* getViewObject() const;
    void showProgressMessage(const std::string featureName, const std::string text) const;

    void onGuiRepaint(const TechDraw::DrawView* dv);
    void onProgressMessage(const TechDraw::DrawView* dv,
                         const std::string featureName,
                         const std::string text);
    using Connection = boost::signals2::scoped_connection;
    Connection connectGuiRepaint;
    Connection connectProgressMessage;

private:
    void multiParentPaint(std::vector<TechDraw::DrawPage*>& pages);
    void singleParentPaint(const TechDraw::DrawView* dv);


};

} // namespace TechDrawGui


#endif // TECHDRAWGUI_VIEWPROVIDERVIEW_H
