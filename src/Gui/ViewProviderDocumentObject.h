/***************************************************************************
 *   Copyright (c) 2004 Jürgen Riegel <juergen.riegel@web.de>              *
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


#ifndef GUI_VIEWPROVIDER_DOCUMENTOBJECT_H
#define GUI_VIEWPROVIDER_DOCUMENTOBJECT_H

#include <Inventor/SoType.h>

#include "ViewProvider.h"
#include <App/DocumentObject.h>

class SoMaterial;
class SoDrawStyle;
class SoType;

namespace App
{
  class DocumentObject;
  class Material;
}


namespace Gui {

class MDIView;

class GuiExport ViewProviderDocumentObject : public ViewProvider
{
    PROPERTY_HEADER(Gui::ViewProviderDocumentObject);

public:
    /// constructor.
    ViewProviderDocumentObject();

    /// destructor.
    virtual ~ViewProviderDocumentObject();

    // Display properties
    App::PropertyEnumeration DisplayMode;
    App::PropertyBool Visibility;

    virtual void attach(App::DocumentObject *pcObject);
    /// Get the default display mode
    virtual const char* getDefaultDisplayMode() const;
    /// Return a list of all possible modes
    virtual std::vector<std::string> getDisplayModes(void) const;
    /// Set the active mode, i.e. the first item of the 'Display' property.
    void setActiveMode();
    /// Hide the object in the view
    virtual void hide(void);
    /// Show the object in the view
    virtual void show(void);

    /// Get a list of TaskBoxes associated with this object
    virtual void getTaskViewContent(std::vector<Gui::TaskView::TaskContent*>&) const;

    /// Run a redraw
    void updateView();
    /// Gets called if some of the property hade bin changed
    virtual void updateData(const App::Property*){}
    /// Get the object of this ViewProvider object
    App::DocumentObject *getObject(void) const {return pcObject;}
    /// Get the python wrapper for that ViewProvider
    PyObject* getPyObject();

    /** @name Restoring view provider from document load */
    //@{
    virtual void startRestoring();
    virtual void finishRestoring();
    //@}

protected:
    /// Get the active mdi view of a view provider
    Gui::MDIView* getActiveView() const;
    /// Gets called by the container whenever a property has been changed
    virtual void onChanged(const App::Property* prop);
    /** Searches in all view providers that are attached to an object that
     * is part of the same document as the object this view provider is
     * attached to for an front root of \a type.
     * Before calling this function this view provider has to be attached
     * to an object. The method returns after the first front root node 
     * matches. If no front root node matches, 0 is returned.
     */
    SoNode* findFrontRootOfType(const SoType& type) const;

protected:
    App::DocumentObject *pcObject;

private:
    std::vector<const char*> aDisplayEnumsArray;
    std::vector<std::string> aDisplayModesArray;
};


} // namespace Gui

#endif // GUI_VIEWPROVIDER_DOCUMENTOBJECT_H

