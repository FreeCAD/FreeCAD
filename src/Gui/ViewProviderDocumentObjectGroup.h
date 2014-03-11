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


#ifndef GUI_VIEWPROVIDER_DOCUMENTOBJECTGROUP_H
#define GUI_VIEWPROVIDER_DOCUMENTOBJECTGROUP_H


#include "ViewProviderDocumentObject.h"
#include "ViewProviderPythonFeature.h"

namespace Gui {

class GuiExport ViewProviderDocumentObjectGroup : public ViewProviderDocumentObject
{
    PROPERTY_HEADER(Gui::ViewProviderDocumentObjectGroup);

public:
    /// constructor.
    ViewProviderDocumentObjectGroup();
    /// destructor.
    virtual ~ViewProviderDocumentObjectGroup();

    virtual std::vector<App::DocumentObject*> claimChildren(void)const;
    virtual bool canDragObjects() const;
    virtual void dragObject(App::DocumentObject*);
    virtual bool canDropObjects() const;
    virtual void dropObject(App::DocumentObject*);

    void attach(App::DocumentObject *pcObject);
    void updateData(const App::Property*);
    void Restore(Base::XMLReader &reader);
    QIcon getIcon(void) const;
    /// returns a list of all possible modes
    std::vector<std::string> getDisplayModes(void) const;
    void hide(void);
    void show(void);
    bool isShow(void) const;

    virtual bool onDelete(const std::vector<std::string> &);

protected:
    /// get called by the container whenever a property has been changed
    void onChanged(const App::Property* prop);
    void getViewProviders(std::vector<ViewProviderDocumentObject*>&) const;

private:
    bool visible; // helper variable
    std::vector<ViewProvider*> nodes;
};

typedef ViewProviderPythonFeatureT<ViewProviderDocumentObjectGroup> ViewProviderDocumentObjectGroupPython;

} // namespace Gui

#endif // GUI_VIEWPROVIDER_DOCUMENTOBJECTGROUP_H

