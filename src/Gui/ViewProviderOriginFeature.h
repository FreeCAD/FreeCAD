/***************************************************************************
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

#ifndef VIEWPROVIDEORIGINFEATURE_H_BYJRZNDL
#define VIEWPROVIDEORIGINFEATURE_H_BYJRZNDL

#include "ViewProviderGeometryObject.h"

class SoAsciiText;
class SoScale;

namespace Gui
{

/**
 * View provider associated with an App::OriginFeature.
 */
class GuiExport ViewProviderOriginFeature: public ViewProviderGeometryObject {
    PROPERTY_HEADER_WITH_OVERRIDE(Gui::ViewProviderOriginFeature);

public:
    /// The display size of the feature
    App::PropertyFloat  Size;

    ViewProviderOriginFeature ();
    ~ViewProviderOriginFeature () override;

    /// Get point derived classes will add their specific stuff
    SoSeparator * getOriginFeatureRoot () { return pOriginFeatureRoot; }

    /// Get pointer to the text label associated with the feature
    SoAsciiText * getLabel () { return pLabel; }

    void attach(App::DocumentObject *) override;
    void updateData(const App::Property *) override;
    std::vector<std::string> getDisplayModes () const override;
    void setDisplayMode (const char* ModeName) override;

    /// @name Suppress ViewProviderGeometryObject's behaviour
    ///@{
    bool setEdit ( int ) override
        { return false; }
    void unsetEdit ( int ) override
        { }
    ///@}

protected:
    void onChanged ( const App::Property* prop ) override;
    bool onDelete ( const std::vector<std::string> & ) override;
protected:
    SoSeparator    * pOriginFeatureRoot;
    SoScale        * pScale;
    SoAsciiText    * pLabel;
};

} /* Gui */

#endif /* end of include guard: VIEWPROVIDEORIGINFEATURE_H_BYJRZNDL */
