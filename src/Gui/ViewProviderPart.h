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


#include "ViewProviderOriginGroup.h"
#include "ViewProviderDragger.h"
#include "ViewProviderPythonFeature.h"


namespace Gui {

class GuiExport ViewProviderPart : public ViewProviderGeometryObject,
                                   public ViewProviderOriginGroupExtension
{
    PROPERTY_HEADER_WITH_EXTENSIONS(Gui::ViewProviderPart);
    typedef ViewProviderGeometryObject inherited;

public:
    App::PropertyColorList OverrideColorList;
    App::PropertyBool OverrideMaterial;

    /// constructor.
    ViewProviderPart();
    /// destructor.
    virtual ~ViewProviderPart();

    virtual bool doubleClicked(void) override;
    virtual void setupContextMenu(QMenu* menu, QObject* receiver, const char* member) override;

    virtual std::map<std::string, App::Color> getElementColors(const char *subname=0) const override;
    virtual void setElementColors(const std::map<std::string, App::Color> &colors) override;

    virtual void finishRestoring() override;

protected:
    App::PropertyLinkSub *getColoredElementsProperty() const;
    void applyColors();
    virtual void buildChildren3D();

protected:
    /// get called by the container whenever a property has been changed
    virtual void onChanged(const App::Property* prop) override;
    virtual void updateData(const App::Property*) override;

    virtual bool setEdit(int ModNum) override;
    virtual void setEditViewer(View3DInventorViewer*, int ModNum) override;
};

typedef ViewProviderPythonFeatureT<ViewProviderPart> ViewProviderPartPython;

} // namespace Gui

#endif // GUI_VIEWPROVIDER_ViewProviderPart_H

