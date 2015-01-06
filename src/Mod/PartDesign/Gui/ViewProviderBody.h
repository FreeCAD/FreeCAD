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


namespace PartDesignGui {

/** ViewProvider of the Body feature
 *  This class manage the visual apperance of the features in the 
 *  Body feature. That mean while editing only the tip feature is 
 *  visible. If the Body is not active it shows only the result shape (tip). 
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
    
    virtual void attach(App::DocumentObject *);
    virtual void setDisplayMode(const char* ModeName);
    /// returns a list of all possible modes
    virtual std::vector<std::string> getDisplayModes(void) const;

    virtual bool doubleClicked(void);
    std::vector<App::DocumentObject*> claimChildren(void)const;

    // returns the root node where the children gets collected(3D)
    virtual SoGroup* getChildRoot(void) const {return pcBodyChildren;}
    std::vector<App::DocumentObject*> claimChildren3D(void)const;

    /// Update the children's highlighting when triggered
    void updateData(const App::Property* prop);    

private:
    /// group used to store children collected by claimChildren3D()
    SoGroup *pcBodyChildren;
    /// group used to show the tip element in "edit" mode
    SoGroup *pcBodyTip;

    /// Update the children's highlighting
    //void updateTree();


};



} // namespace PartDesignGui


#endif // PARTGUI_ViewProviderHole_H
