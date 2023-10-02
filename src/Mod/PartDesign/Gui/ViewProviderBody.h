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


#ifndef PARTGUI_ViewProviderBody_H
#define PARTGUI_ViewProviderBody_H

#include <Mod/Part/Gui/ViewProvider.h>
#include <Mod/PartDesign/PartDesignGlobal.h>
#include <Gui/ViewProviderOriginGroupExtension.h>
#include <QCoreApplication>

class SoGroup;
class SoSeparator;
class SbBox3f;
class SoGetBoundingBoxAction;
namespace PartDesignGui {

/** ViewProvider of the Body feature
 *  This class manages the visual appearance of the features in the
 *  Body feature. That means while editing all visible features are shown.
 *  If the Body is not active it shows only the result shape (tip).
 * \author jriegel
 */
class PartDesignGuiExport ViewProviderBody : public PartGui::ViewProviderPart, public Gui::ViewProviderOriginGroupExtension
{
    Q_DECLARE_TR_FUNCTIONS(PartDesignGui::ViewProviderBody)
    PROPERTY_HEADER_WITH_EXTENSIONS(PartDesignGui::ViewProviderBody);

public:
    /// constructor
    ViewProviderBody();
    /// destructor
    ~ViewProviderBody() override;

    App::PropertyEnumeration DisplayModeBody;

    void attach(App::DocumentObject *) override;

    bool doubleClicked() override;
    void setupContextMenu(QMenu* menu, QObject* receiver, const char* member) override;

    std::vector< std::string > getDisplayModes() const override;
    void setDisplayMode(const char* ModeName) override;
    void setOverrideMode(const std::string& mode) override;

    bool onDelete(const std::vector<std::string> &) override;

    /// Update the children's highlighting when triggered
    void updateData(const App::Property* prop) override;
    ///unify children visuals
    void onChanged(const App::Property* prop) override;

    /// Update the sizes of origin and datums
    void updateOriginDatumSize ();

    /**
     * Return the bounding box of visible features
     * @note datums are counted as their base point only
     */
    SbBox3f getBoundBox ();

    /** Check whether objects can be added to the view provider by drag and drop */
    bool canDropObjects() const override;
    /** Check whether the object can be dropped to the view provider by drag and drop */
    bool canDropObject(App::DocumentObject*) const override;
    /** Add an object to the view provider by drag and drop */
    void dropObject(App::DocumentObject*) override;

protected:
    void slotChangedObjectApp ( const App::DocumentObject& obj, const App::Property& prop );
    void slotChangedObjectGui ( const Gui::ViewProviderDocumentObject& obj, const App::Property& prop );

    /// Copy over all visual properties to the child features
    void unifyVisualProperty(const App::Property* prop);
    /// Set Feature viewprovider into visual body mode
    void setVisualBodyMode(bool bodymode);

private:
    void copyColorsfromTip(App::DocumentObject* tip);

private:
    static const char* BodyModeEnum[];

    boost::signals2::connection connectChangedObjectApp;
    boost::signals2::connection connectChangedObjectGui;
};



} // namespace PartDesignGui


#endif // PARTGUI_ViewProviderHole_H
