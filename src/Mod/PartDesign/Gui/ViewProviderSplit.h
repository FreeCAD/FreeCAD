/****************************************************************************
 *   Copyright (c) 2019 Zheng, Lei (realthunder) <realthunder.dev@gmail.com>*
 *                                                                          *
 *   This file is part of the FreeCAD CAx development system.               *
 *                                                                          *
 *   This library is free software; you can redistribute it and/or          *
 *   modify it under the terms of the GNU Library General Public            *
 *   License as published by the Free Software Foundation; either           *
 *   version 2 of the License, or (at your option) any later version.       *
 *                                                                          *
 *   This library  is distributed in the hope that it will be useful,       *
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of         *
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the          *
 *   GNU Library General Public License for more details.                   *
 *                                                                          *
 *   You should have received a copy of the GNU Library General Public      *
 *   License along with this library; see the file COPYING.LIB. If not,     *
 *   write to the Free Software Foundation, Inc., 59 Temple Place,          *
 *   Suite 330, Boston, MA  02111-1307, USA                                 *
 *                                                                          *
 ****************************************************************************/

#ifndef PARTGUI_ViewProviderSplit_H
#define PARTGUI_ViewProviderSplit_H

#include "ViewProvider.h"

namespace PartDesignGui {

class PartDesignGuiExport ViewProviderSplit : public ViewProvider
{
    PROPERTY_HEADER_WITH_OVERRIDE(PartDesignGui::ViewProviderSplit);

public:
    ViewProviderSplit();
    virtual ~ViewProviderSplit();

    App::PropertyEnumeration Display;
    
    virtual void attach(App::DocumentObject*) override;
    virtual void onChanged(const App::Property* prop) override;

    virtual SoGroup* getChildRoot(void) const override;

    virtual bool canDragAndDropObject(App::DocumentObject*) const override {return false;}
    virtual bool canDropObject(App::DocumentObject*) const override;
    virtual bool canDropObjects() const override {return true;}
    virtual void dropObject(App::DocumentObject*) override;
    virtual bool canDragObject(App::DocumentObject*) const override;
    virtual bool canDragObjects() const override {return true;}
    virtual void dragObject(App::DocumentObject*) override;
    virtual bool canDelete(App::DocumentObject* obj) const override;
    virtual bool onDelete(const std::vector<std::string> &) override;

    virtual std::vector<App::DocumentObject*> claimChildren(void) const override;
    virtual std::vector<App::DocumentObject*> claimChildren3D(void) const override;

private:
    Gui::CoinPtr<SoGroup> pcGroupChildren;
};

} // namespace PartDesignGui


#endif // PARTGUI_ViewProviderSplit_H
