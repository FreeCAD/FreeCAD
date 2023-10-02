/***************************************************************************
 *   Copyright (c) 2015 Stefan Tröger <stefantroeger@gmx.net>              *
 *   Copyright (c) 2015 Alexander Golubev (Fat-Zer) <fatzer2@gmail.com>    *
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

#ifndef GUI_VIEWPROVIDER_ViewProviderOrigin_H
#define GUI_VIEWPROVIDER_ViewProviderOrigin_H

#include <App/PropertyGeo.h>

#include "ViewProviderDocumentObject.h"


namespace Gui {

class Document;

class GuiExport ViewProviderOrigin : public ViewProviderDocumentObject
{
    PROPERTY_HEADER_WITH_OVERRIDE(Gui::ViewProviderOrigin);

public:
    /// Size of the origin as set by the part.
    App::PropertyVector Size;

    /// constructor.
    ViewProviderOrigin();
    /// destructor.
    ~ViewProviderOrigin() override;

    /// @name Override methods
    ///@{
    std::vector<App::DocumentObject*> claimChildren() const override;
    std::vector<App::DocumentObject*> claimChildren3D() const override;

    SoGroup* getChildRoot() const override {return pcGroupChildren;}

    void attach(App::DocumentObject* pcObject) override;
    std::vector<std::string> getDisplayModes() const override;
    void setDisplayMode(const char* ModeName) override;
    ///@}

    /** @name Temporary visibility mode
     * Control the visibility of origin and associated objects when needed
     */
    ///@{
    /// Set temporary visibility of some of origin's objects e.g. while rotating or mirroring
    void setTemporaryVisibility (bool axis, bool planes);
    /// Returns true if the origin in temporary visibility mode
    bool isTemporaryVisibility ();
    /// Reset the visibility
    void resetTemporaryVisibility ();
    ///@}

    bool canDragObjects() const override {
        return false;
    }

    /// Returns default size. Use this if it is not possible to determine appropriate size by other means
    static double defaultSize();

    // the factor by which the axes are longer than the planes
    static constexpr float axesScaling = 1.5f;

    // default color for origini: light-blue (50, 150, 250, 255 stored as 0xRRGGBBAA)
    static const uint32_t defaultColor = 0x3296faff;
protected:
    void onChanged(const App::Property* prop) override;
    bool onDelete(const std::vector<std::string> &) override;

private:
    SoGroup *pcGroupChildren;

    std::map<Gui::ViewProvider*, bool> tempVisMap;
};

} // namespace Gui

#endif // GUI_VIEWPROVIDER_ViewProviderOrigin_H

