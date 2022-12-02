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

#ifndef GUI_VIEWPROVIDER_ViewProviderPart_H
#define GUI_VIEWPROVIDER_ViewProviderPart_H

#include "ViewProviderDragger.h"
#include "ViewProviderOriginGroup.h"
#include "ViewProviderPythonFeature.h"


namespace Gui {

class GuiExport ViewProviderPart : public ViewProviderDragger,
                                   public ViewProviderOriginGroupExtension
{
    PROPERTY_HEADER_WITH_EXTENSIONS(Gui::ViewProviderPart);

public:
    /// constructor.
    ViewProviderPart();
    /// destructor.
    ~ViewProviderPart() override;

    bool doubleClicked() override;
    void setupContextMenu(QMenu* menu, QObject* receiver, const char* member) override;

    /// deliver the icon shown in the tree view
    /// override from ViewProvider.h
    QIcon getIcon() const override;

protected:
    /// get called by the container whenever a property has been changed
    void onChanged(const App::Property* prop) override;
    /// a second icon for the Assembly type
    const char* aPixmap;

};

using ViewProviderPartPython = ViewProviderPythonFeatureT<ViewProviderPart>;

} // namespace Gui

#endif // GUI_VIEWPROVIDER_ViewProviderPart_H

