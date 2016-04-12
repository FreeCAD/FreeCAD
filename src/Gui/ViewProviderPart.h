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


#include "ViewProviderGeoFeatureGroup.h"
#include "ViewProviderPythonFeature.h"

#include <App/PropertyStandard.h>
#include <App/Part.h>



namespace Gui {

class GuiExport ViewProviderPart : public ViewProviderGeoFeatureGroup
{
    PROPERTY_HEADER(Gui::ViewProviderPart);

public:
    /// constructor.
    ViewProviderPart();
    /// destructor.
    virtual ~ViewProviderPart();

    /// Name of the workbench which created that Part
    App::PropertyString Workbench;

    void attach(App::DocumentObject *pcObject);
    void updateData(const App::Property*);
    void Restore(Base::XMLReader &reader);
    QIcon getIcon(void) const;
    /// returns a list of all possible modes

    virtual bool doubleClicked(void);

//    virtual bool onDelete(const std::vector<std::string> &);

    /// helper to set up the standard content of a Part Object
    static void setUpPart(const App::Part *part);
protected:
    /// get called by the container whenever a property has been changed
    void onChanged(const App::Property* prop);
    void getViewProviders(std::vector<ViewProviderDocumentObject*>&) const;
    void onObjectChanged(const App::DocumentObject&, const App::Property&);

private:
    boost::signals::connection connection;
};

typedef ViewProviderPythonFeatureT<ViewProviderPart> ViewProviderPartPython;

} // namespace Gui

#endif // GUI_VIEWPROVIDER_DOCUMENTOBJECTGROUP_H

