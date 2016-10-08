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
#include "ViewProviderGroupExtension.h"

namespace Gui {

class GuiExport ViewProviderDocumentObjectGroup : public ViewProviderDocumentObject, 
                                                  public ViewProviderGroupExtension
{
    PROPERTY_HEADER_WITH_EXTENSIONS(Gui::ViewProviderDocumentObjectGroup);

public:
    /// constructor.
    ViewProviderDocumentObjectGroup();
    /// destructor.
    virtual ~ViewProviderDocumentObjectGroup();

    QIcon getIcon(void) const;
    /// returns a list of all possible modes
    std::vector<std::string> getDisplayModes(void) const;
    bool isShow(void) const;

    /// get called if the user hover over a object in the tree
    virtual bool allowDrop(const std::vector<const App::DocumentObject*> &objList,
                           Qt::KeyboardModifiers keys,
                           Qt::MouseButtons mouseBts,
                           const QPoint &pos);
    /// get called if the user drops some objects
    virtual void drop(const std::vector<const App::DocumentObject*> &objList,
                      Qt::KeyboardModifiers keys,
                      Qt::MouseButtons mouseBts,
                      const QPoint &pos);

protected:
    void getViewProviders(std::vector<ViewProviderDocumentObject*>&) const;

private:
    std::vector<ViewProvider*> nodes;
};

typedef ViewProviderPythonFeatureT<ViewProviderDocumentObjectGroup> ViewProviderDocumentObjectGroupPython;

} // namespace Gui

#endif // GUI_VIEWPROVIDER_DOCUMENTOBJECTGROUP_H

