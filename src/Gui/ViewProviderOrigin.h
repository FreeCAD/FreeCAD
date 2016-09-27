/***************************************************************************
 *   Copyright (c) Stefan Tröger          (stefantroeger@gmx.net) 2015     *
 *   Copyright (c) 2015 Alexander Golubev (Fat-Zer) <fatzer2@gmail.com>    *
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


#ifndef GUI_VIEWPROVIDER_ViewProviderOrigin_H
#define GUI_VIEWPROVIDER_ViewProviderOrigin_H

#include <App/PropertyGeo.h>

#include "ViewProviderDocumentObject.h"

namespace Gui {

class Document;

class GuiExport ViewProviderOrigin : public ViewProviderDocumentObject
{
    PROPERTY_HEADER(Gui::ViewProviderOrigin);

public:
    /// Size of the origin as setted by the part.
    App::PropertyVector Size;

    /// constructor.
    ViewProviderOrigin();
    /// destructor.
    virtual ~ViewProviderOrigin();

    /// @name Override methodes
    ///@{
    virtual std::vector<App::DocumentObject*> claimChildren(void) const;
    virtual std::vector<App::DocumentObject*> claimChildren3D(void) const;

    virtual SoGroup* getChildRoot(void) const {return pcGroupChildren;};

    virtual void attach(App::DocumentObject* pcObject);
    virtual std::vector<std::string> getDisplayModes(void) const;
    virtual void setDisplayMode(const char* ModeName);
    ///@}

    /** @name Temporary visability mode
     * Control the visability of origin and associated objects when needed
     */
    ///@{
    /// Set temporary visability of some of origin's objects e.g. while rotating or mirroring
    void setTemporaryVisibility (bool axis, bool planes);
    /// Returns true if the origin in temporary visability mode
    bool isTemporaryVisibility ();
    /// Reset the visability
    void resetTemporaryVisibility ();
    ///@}

    /// Returns default size. Use this if it is not possible to determin apropriate size by other means
    static double defaultSize() {return 10.;}
protected:
    virtual void onChanged(const App::Property* prop);
    virtual bool onDelete(const std::vector<std::string> &);

private:
    SoGroup *pcGroupChildren;

    std::map<Gui::ViewProvider*, bool> tempVisMap;
};

} // namespace Gui

#endif // GUI_VIEWPROVIDER_DOCUMENTOBJECTGROUP_H

