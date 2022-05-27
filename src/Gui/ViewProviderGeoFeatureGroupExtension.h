/***************************************************************************
 *   Copyright (c) 2011 Jürgen Riegel <FreeCAD@juergen-riegel.net>         *
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

#ifndef GUI_VIEWPROVIDERGEOFEATUREGROUPEXTENSION_H
#define GUI_VIEWPROVIDERGEOFEATUREGROUPEXTENSION_H

#include "ViewProviderGroupExtension.h"


namespace Gui
{

class GuiExport ViewProviderGeoFeatureGroupExtension : public ViewProviderGroupExtension
{
    EXTENSION_PROPERTY_HEADER_WITH_OVERRIDE(Gui::ViewProviderGeoFeatureGroupExtension);

public:
    /// Constructor
    ViewProviderGeoFeatureGroupExtension(void);
    virtual ~ViewProviderGeoFeatureGroupExtension();

    virtual std::vector<App::DocumentObject*> extensionClaimChildren3D(void)const override;
    virtual std::vector< App::DocumentObject* > extensionClaimChildren(void) const override;
    virtual SoSeparator* extensionGetFrontRoot() const override {return pcGroupFront;}
    virtual SoSeparator* extensionGetBackRoot() const override {return pcGroupBack;}
    virtual SoGroup* extensionGetChildRoot(void) const override {return pcGroupChildren;}
    virtual void extensionAttach(App::DocumentObject* pcObject) override;
    virtual void extensionSetDisplayMode(const char* ModeName) override;
    virtual std::vector<std::string> extensionGetDisplayModes(void) const override;
    virtual void extensionFinishRestoring() override;

    /// Show the object in the view: suppresses behavior of DocumentObjectGroup
    virtual void extensionShow(void) override {
        ViewProviderExtension::extensionShow();
    }
    /// Hide the object in the view: suppresses behavior of DocumentObjectGroup
    virtual void extensionHide(void) override {
        ViewProviderExtension::extensionHide();
    }

    virtual void extensionUpdateData(const App::Property*) override;

protected:
    SoSeparator *pcGroupFront;
    SoSeparator *pcGroupBack;
    SoGroup *pcGroupChildren;
};

typedef ViewProviderExtensionPythonT<Gui::ViewProviderGeoFeatureGroupExtension> ViewProviderGeoFeatureGroupExtensionPython;

} //namespace Gui

#endif // GUI_VIEWPROVIDERGEOFEATUREGROUPEXTENSION_H
