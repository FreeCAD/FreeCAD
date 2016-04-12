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

class SoGroup;
class SoSeparator;
class SbBox3f;
class SoGetBoundingBoxAction;
namespace PartDesignGui {

/** ViewProvider of the Body feature
 *  This class manage the visual apperance of the features in the
 *  Body feature. That mean while editing all visible features are shown.
 *  If the Body is not active it shows only the result shape (tip).
 * \author jriegel
 */
class PartDesignGuiExport ViewProviderBody : public PartGui::ViewProviderPart
{
    PROPERTY_HEADER(PartDesignGui::ViewProviderBody);

public:
    /// constructor
    ViewProviderBody();
    /// destructor
    virtual ~ViewProviderBody();

    App::PropertyEnumeration DisplayModeBody;
    
    virtual void attach(App::DocumentObject *);

    virtual bool doubleClicked(void);
    virtual std::vector<App::DocumentObject*> claimChildren(void)const;

    // returns the root node where the children gets collected(3D)
    virtual SoGroup* getChildRoot(void) const {return pcBodyChildren;}
    virtual std::vector<App::DocumentObject*> claimChildren3D(void)const;
    virtual void setDisplayMode(const char* ModeName);
    virtual void setOverrideMode(const std::__cxx11::string& mode);

    virtual bool onDelete(const std::vector<std::string> &);

    /// Update the children's highlighting when triggered
    virtual void updateData(const App::Property* prop);
    ///unify children visuals
    virtual void onChanged(const App::Property* prop);

    /// Update the sizes of origin and datums
    void updateOriginDatumSize ();
    
    /**
     * Return the bounding box of visible features
     * @note datums are counted as their base point only
     */
    SbBox3f getBoundBox ();

protected:
    void slotChangedObjectApp ( const App::DocumentObject& obj, const App::Property& prop );
    void slotChangedObjectGui ( const Gui::ViewProviderDocumentObject& obj, const App::Property& prop );

    /// Copy over all visual properties to the child features
    void unifyVisualProperty(const App::Property* prop);
    /// Set Feature viewprovider into visual body mode
    void setVisualBodyMode(bool bodymode);
private:
    /// group used to store children collected by claimChildren3D() in the through (edit) mode.
    SoGroup *pcBodyChildren;

    static const char* BodyModeEnum[];

    boost::signals::connection connectChangedObjectApp;
    boost::signals::connection connectChangedObjectGui;
};



} // namespace PartDesignGui


#endif // PARTGUI_ViewProviderHole_H
