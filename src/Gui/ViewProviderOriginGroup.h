/***************************************************************************
 *   Copyright (c) Alexander Golubev (Fat-Zer) <fatzer2@gmail.com> 2015    *
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

#ifndef VIEWPROVIDERORIGINGROUP_H_JIXBOPA7
#define VIEWPROVIDERORIGINGROUP_H_JIXBOPA7

#include <boost/signals.hpp>

#include "ViewProviderGeoFeatureGroup.h"

namespace Gui {

class GuiExport ViewProviderOriginGroup: public ViewProviderGeoFeatureGroup
{
    PROPERTY_HEADER(Gui::ViewProviderOriginGroup);
public:
    ViewProviderOriginGroup ();
    virtual ~ViewProviderOriginGroup ();

    virtual std::vector<App::DocumentObject*> claimChildren(void)const;
    virtual std::vector<App::DocumentObject*> claimChildren3D(void)const;

    virtual void attach(App::DocumentObject *pcObject);
    virtual void updateData(const App::Property* prop);

    void updateOriginSize();

protected:
    void slotChangedObjectApp ( const App::DocumentObject& obj );
    void slotChangedObjectGui ( const Gui::ViewProviderDocumentObject& obj );

private:
    std::vector<App::DocumentObject*> constructChildren (
            const std::vector<App::DocumentObject*> &children ) const;

    boost::signals::connection connectChangedObjectApp;
    boost::signals::connection connectChangedObjectGui;
};

} /* Gui  */


#endif /* end of include guard: VIEWPROVIDERORIGINGROUP_H_JIXBOPA7 */
