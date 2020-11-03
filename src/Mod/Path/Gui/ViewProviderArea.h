/****************************************************************************
 *   Copyright (c) 2017 Zheng Lei (realthunder) <realthunder.dev@gmail.com> *
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

#ifndef PATH_ViewProviderArea_H
#define PATH_ViewProviderArea_H

#include <Gui/ViewProviderPythonFeature.h>
#include <Mod/Part/Gui/ViewProviderPlaneParametric.h>

namespace PathGui
{

class PathGuiExport ViewProviderArea : public PartGui::ViewProviderPlaneParametric
{
    PROPERTY_HEADER(PathGui::ViewProviderArea);

public:
    ViewProviderArea();
    virtual ~ViewProviderArea();

    /// grouping handling
    virtual std::vector<App::DocumentObject*> claimChildren(void) const;
    virtual void updateData(const App::Property*);
    virtual bool onDelete(const std::vector<std::string> &);

    /// drag and drop
    virtual bool canDragObjects() const;
    virtual bool canDragObject(App::DocumentObject*) const;
    virtual void dragObject(App::DocumentObject*);
    virtual bool canDropObjects() const;
    virtual bool canDropObject(App::DocumentObject*) const;
    virtual void dropObject(App::DocumentObject*);
};

typedef Gui::ViewProviderPythonFeatureT<ViewProviderArea> ViewProviderAreaPython;


class PathGuiExport ViewProviderAreaView : public PartGui::ViewProviderPlaneParametric
{
    PROPERTY_HEADER(PathGui::ViewProviderAreaView);

public:
    ViewProviderAreaView();
    virtual ~ViewProviderAreaView();
    virtual std::vector<App::DocumentObject*> claimChildren(void) const;
    virtual void updateData(const App::Property*);
    virtual bool onDelete(const std::vector<std::string> &);

    /// drag and drop
    virtual bool canDragObjects() const;
    virtual bool canDragObject(App::DocumentObject*) const;
    virtual void dragObject(App::DocumentObject*);
    virtual bool canDropObjects() const;
    virtual bool canDropObject(App::DocumentObject*) const;
    virtual void dropObject(App::DocumentObject*);
};

typedef Gui::ViewProviderPythonFeatureT<ViewProviderAreaView> ViewProviderAreaViewPython;

} //namespace PathGui


#endif // PATH_ViewProviderArea_H
