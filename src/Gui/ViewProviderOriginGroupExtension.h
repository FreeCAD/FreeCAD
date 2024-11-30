/***************************************************************************
 *   Copyright (c) 2015 Alexander Golubev (Fat-Zer) <fatzer2@gmail.com>    *
 *   Copyright (c) 2016 Stefan Tröger <stefantroeger@gmx.net>              *
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

#ifndef GUI_VIEWPROVIDERORIGINGROUPEXTENSION_H
#define GUI_VIEWPROVIDERORIGINGROUPEXTENSION_H

#include "ViewProviderGeoFeatureGroup.h"


namespace Gui
{

class GuiExport ViewProviderOriginGroupExtension : public ViewProviderGeoFeatureGroupExtension
{
    EXTENSION_PROPERTY_HEADER_WITH_OVERRIDE(Gui::ViewProviderOriginGroupExtension);

public:
    /// Constructor
    ViewProviderOriginGroupExtension();
    ~ViewProviderOriginGroupExtension() override;

    std::vector<App::DocumentObject*> extensionClaimChildren()const override;
    std::vector<App::DocumentObject*> extensionClaimChildren3D()const override;

    void extensionAttach(App::DocumentObject *pcObject) override;
    void extensionUpdateData(const App::Property* prop) override;

    void updateOriginSize();

protected:
    void slotChangedObjectApp ( const App::DocumentObject& obj );
    void slotChangedObjectGui ( const Gui::ViewProviderDocumentObject& obj );

private:
    std::vector<App::DocumentObject*> constructChildren (
            const std::vector<App::DocumentObject*> &children ) const;

    boost::signals2::connection connectChangedObjectApp;
    boost::signals2::connection connectChangedObjectGui;
};

using ViewProviderOriginGroupExtensionPython = ViewProviderExtensionPythonT<Gui::ViewProviderOriginGroupExtension>;

} //namespace Gui

#endif // GUI_VIEWPROVIDERORIGINGROUPEXTENSION_H
