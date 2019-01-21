/***************************************************************************
 *   Copyright (c) 2014 Luke Parry <l.parry@warwick.ac.uk>                 *
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

#ifndef _TECHDRAW_DrawTemplate_h_
#define _TECHDRAW_DrawTemplate_h_

#include <App/DocumentObject.h>

#include <App/PropertyStandard.h>
#include <App/PropertyUnits.h>
#include <App/FeaturePython.h>

namespace TechDraw
{

class DrawPage;

class TechDrawExport DrawTemplate : public App::DocumentObject
{
    PROPERTY_HEADER(TechDraw::DrawTemplate);

public:
    DrawTemplate(); /// Constructor
    ~DrawTemplate();

    // Page Physical Properties
    App::PropertyLength Width;
    App::PropertyLength Height;
    App::PropertyEnumeration Orientation;
    //App::PropertyString PaperSize;

    App::PropertyMap EditableTexts;

public:

    /// Returns template width in mm
    virtual double getWidth() const;
    /// Returns template height in mm
    virtual double getHeight() const;

    virtual void getBlockDimensions(double &x, double &y, double &width, double &height) const;
    virtual DrawPage* getParentPage() const;

    /** @name methods override Feature */
    //@{
    /// recalculate the Feature
    virtual App::DocumentObjectExecReturn *execute(void);
    //@}


    virtual short mustExecute() const;

    /// returns the type name of the ViewProvider
    virtual const char* getViewProviderName(void) const {
        return "TechDrawGui::ViewProviderTemplate";
    }

    // from base class
    virtual PyObject *getPyObject(void);
    virtual unsigned int getMemSize(void) const;

protected:
    void onChanged(const App::Property* prop);

private:
    static const char* OrientationEnums[];

};

typedef App::FeaturePythonT<DrawTemplate> DrawTemplatePython;

} //namespace TechDraw



#endif //_TECHDRAW_DrawTemplate_h_
