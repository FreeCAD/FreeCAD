/***************************************************************************
 *   Copyright (c) 2016 Victor Titov (DeepSOIC)       <vv.titov@gmail.com> *
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

#ifndef PARTGUI_ViewProviderBodyBase_H
#define PARTGUI_ViewProviderBodyBase_H

#include "ViewProvider.h"
#include <QCoreApplication>

class SoGroup;
class SoSeparator;
class SbBox3f;
class SoGetBoundingBoxAction;
namespace PartGui {

/** ViewProvider of the BodyBase feature
 *  This class manage the visual apperance of the features in the
 *  Body feature. That mean while editing all visible features are shown.
 *  If the Body is not active it shows only the result shape (tip).
 * \author jriegel
 */
class PartGuiExport ViewProviderBodyBase : public ViewProviderPart
{
    Q_DECLARE_TR_FUNCTIONS(PartDesignGui::ViewProviderBody)
    PROPERTY_HEADER(PartGui::ViewProviderBodyBase);

public:
    ViewProviderBodyBase();
    virtual ~ViewProviderBodyBase();

    App::PropertyEnumeration DisplayModeBody;

    virtual void attach(App::DocumentObject *);

    virtual bool doubleClicked(void);
    virtual void setupContextMenu(QMenu* menu, QObject* receiver, const char* member);

    virtual std::vector<App::DocumentObject*> claimChildren(void)const;

    // returns the root node where the children gets collected(3D)
    virtual SoGroup* getChildRoot(void) const {return pcBodyChildren;}

    virtual std::vector<App::DocumentObject*> claimChildren3D(void)const;

    virtual void setDisplayMode(const char* ModeName);
    virtual void setOverrideMode(const std::string& mode);

    virtual bool onDelete(const std::vector<std::string> &);

    /// Update the children's highlighting when triggered
    virtual void updateData(const App::Property* prop);

    virtual void onChanged(const App::Property* prop);

    /// Update the sizes of origin
    virtual void updateOriginDatumSize ();

protected:
    void slotChangedObjectApp ( const App::DocumentObject& obj, const App::Property& prop );
    void slotChangedObjectGui ( const Gui::ViewProviderDocumentObject& obj, const App::Property& prop );

protected:
    /// group used to store children collected by claimChildren3D() in the through (edit) mode.
    SoGroup *pcBodyChildren;

    static const char* BodyModeEnum[];

private:
    boost::signals::connection connectChangedObjectApp;
    boost::signals::connection connectChangedObjectGui;
};

} // namespace PartGui


#endif // PARTGUI_ViewProviderBodyBase_H
