/***************************************************************************
 *   Copyright (c) 2011 Juergen Riegel <FreeCAD@juergen-riegel.net>        *
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


#ifndef PARTGUI_ViewProvider_H
#define PARTGUI_ViewProvider_H

#include <Mod/Part/Gui/ViewProvider.h>
#include "ViewProviderBody.h"
#include <Gui/ViewProviderPythonFeature.h>

#include <Mod/Part/Gui/ViewProviderAttachExtension.h>

namespace PartDesignGui {

class TaskDlgFeatureParameters;

/**
 * A common base class for all part design features view providers
 */
class PartDesignGuiExport ViewProvider : public PartGui::ViewProviderPart, PartGui::ViewProviderAttachExtension
{
    typedef PartGui::ViewProviderPart inherited;
    PROPERTY_HEADER_WITH_OVERRIDE(PartDesignGui::ViewProvider);

public:
    /// constructor
    ViewProvider();
    /// destructor
    virtual ~ViewProvider();

    virtual bool doubleClicked(void) override;
    void updateData(const App::Property*) override;
    void onChanged(const App::Property* prop) override;

    void setTipIcon(bool onoff);

    //body mode means that the object is part of a body and that the body is used to set the
    //visual properties, not the features. Hence setting body mode to true will hide most
    //viewprovider properties.
    void setBodyMode(bool bodymode);

    //makes this viewprovider visible in the scene graph without changing any properties,
    //not the visibility one and also not the display mode. This can be used to show the
    //shape of this viewprovider from other viewproviders without doing anything to the
    //document and properties.
    void makeTemporaryVisible(bool);

    //Returns the ViewProvider of the body the feature belongs to, or NULL, if not in a body
    ViewProviderBody* getBodyViewProvider();

    virtual PyObject* getPyObject(void) override;

protected:
    virtual void setupContextMenu(QMenu* menu, QObject* receiver, const char* member) override;
    virtual bool setEdit(int ModNum) override;
    virtual void unsetEdit(int ModNum) override;

    virtual bool onDelete(const std::vector<std::string> &) override;

    virtual QIcon mergeOverlayIcons (const QIcon & orig) const override;

    /**
     * Returns a newly create dialog for the part to be placed in the task view
     * Must be reimplemented in subclasses.
     */
    virtual TaskDlgFeatureParameters *getEditDialog();

    std::string oldWb;
    App::DocumentObject* oldTip;
    bool isSetTipIcon;
};

typedef Gui::ViewProviderPythonFeatureT<ViewProvider> ViewProviderPython;

} // namespace PartDesignGui


#endif // PARTGUI_ViewProviderHole_H
