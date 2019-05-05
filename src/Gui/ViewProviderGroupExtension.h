/***************************************************************************
 *   Copyright (c) 2006 Werner Mayer <wmayer[at]users.sourceforge.net>     *
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


#ifndef GUI_VIEWPROVIDERGROUPEXTENSION_H
#define GUI_VIEWPROVIDERGROUPEXTENSION_H

#include <App/Extension.h>
#include "ViewProviderExtension.h"

namespace Gui
{

class GuiExport ViewProviderGroupExtension : public ViewProviderExtension
{
    EXTENSION_PROPERTY_HEADER_WITH_OVERRIDE(Gui::ViewProviderGroupExtension);

public:
    /// Constructor
    ViewProviderGroupExtension(void);
    virtual ~ViewProviderGroupExtension();

    virtual std::vector<App::DocumentObject*> extensionClaimChildren(void)const override;
    virtual bool extensionCanDragObjects() const override; 
    virtual bool extensionCanDragObject(App::DocumentObject*) const override;
    virtual void extensionDragObject(App::DocumentObject*) override;
    virtual bool extensionCanDropObjects() const override;
    virtual bool extensionCanDropObject(App::DocumentObject*) const override;
    virtual void extensionDropObject(App::DocumentObject*) override;   
    virtual void extensionReplaceObject(App::DocumentObject* oldValue, App::DocumentObject* newValue) override;
 
    virtual void extensionHide(void) override;
    virtual void extensionShow(void) override;

    virtual bool extensionOnDelete(const std::vector<std::string> &) override;

private:
    bool guard;
    std::vector<ViewProvider*> nodes;
};

typedef ViewProviderExtensionPythonT<Gui::ViewProviderGroupExtension> ViewProviderGroupExtensionPython;

} //namespace Gui

#endif // GUI_VIEWPROVIDERGROUPEXTENSION_H
