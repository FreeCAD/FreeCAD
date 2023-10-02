/***************************************************************************
 *   Copyright (c) 2015 Stefan Tröger <stefantroeger@gmx.net>              *
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

#ifndef PARTGUI_ViewProviderShapeBinder_H
#define PARTGUI_ViewProviderShapeBinder_H

#include <Gui/ViewProviderPythonFeature.h>
#include <Mod/Part/Gui/ViewProvider.h>
#include <Mod/PartDesign/PartDesignGlobal.h>

namespace PartDesignGui {

class PartDesignGuiExport ViewProviderShapeBinder : public PartGui::ViewProviderPart
{
    PROPERTY_HEADER_WITH_OVERRIDE(PartDesignGui::ViewProviderShapeBinder);

public:
    /// Constructor
    ViewProviderShapeBinder();
    ~ViewProviderShapeBinder() override;

    void setupContextMenu(QMenu*, QObject*, const char*) override;
    void highlightReferences(bool on);

protected:
    bool setEdit(int ModNum) override;
    void unsetEdit(int ModNum) override;

private:
    std::vector<App::Color> originalLineColors;
    std::vector<App::Color> originalFaceColors;

};

class PartDesignGuiExport ViewProviderSubShapeBinder : public PartGui::ViewProviderPart
{
    PROPERTY_HEADER_WITH_OVERRIDE(PartDesignGui::ViewProviderShapeBinder);

public:
    App::PropertyBool UseBinderStyle;

    /// Constructor
    ViewProviderSubShapeBinder();

    bool canDropObjects() const override {return true;}
    bool canDragAndDropObject(App::DocumentObject*) const override {return false;}
    bool canDropObjectEx(App::DocumentObject *obj, App::DocumentObject *owner,
            const char *subname, const std::vector<std::string> &elements) const override;
    std::string dropObjectEx(App::DocumentObject*, App::DocumentObject*, const char *,
            const std::vector<std::string> &) override;
    std::vector<App::DocumentObject*> claimChildren() const override;

    bool doubleClicked() override;
    void setupContextMenu(QMenu* menu, QObject* receiver, const char* member) override;
    bool setEdit(int ModNum) override;
    void attach(App::DocumentObject *obj) override;
    void onChanged(const App::Property *prop) override;

private:
    enum {
        Synchronize = 0,
        SelectObject = 1
    };
    void updatePlacement(bool transaction);
};

using ViewProviderSubShapeBinderPython = Gui::ViewProviderPythonFeatureT<ViewProviderSubShapeBinder>;

} // namespace PartDesignGui


#endif // PARTGUI_ViewProviderShapeBinder_H
