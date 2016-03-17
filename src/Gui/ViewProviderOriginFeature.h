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
    PROPERTY_HEADER(Gui::ViewProviderOriginFeature);

public:
    /// The display size of the feature
    App::PropertyFloat  Size;

    ViewProviderOriginFeature ();
    virtual ~ViewProviderOriginFeature ();

    /// Get point derived classes will add their specific stuff
    SoSeparator * getOriginFeatureRoot () { return pOriginFeatureRoot; }

    /// Get pointer to the text label associated with the feature
    SoAsciiText * getLabel () { return pLabel; }

    virtual void attach(App::DocumentObject *);
    virtual void updateData(const App::Property *);
    virtual std::vector<std::string> getDisplayModes () const;
    virtual void setDisplayMode (const char* ModeName);

    /// @name Suppress ViewProviderGeometryObject's behaviour
    ///@{
    virtual bool setEdit ( int )
        { return false; }
    virtual void unsetEdit ( int )
        { }
    ///@}

protected:
    virtual void onChanged ( const App::Property* prop );
    virtual bool onDelete ( const std::vector<std::string> & );
protected:
    SoSeparator    * pOriginFeatureRoot;
    SoScale        * pScale;
    SoAsciiText    * pLabel;
};

} /* Gui */

#endif /* end of include guard: VIEWPROVIDEORIGINFEATURE_H_BYJRZNDL */
