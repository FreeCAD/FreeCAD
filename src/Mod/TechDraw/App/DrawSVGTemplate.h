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

#ifndef _TECHDRAW_DrawSVGTemplate_h_
#define _TECHDRAW_DrawSVGTemplate_h_

#include <App/PropertyStandard.h>
#include <App/PropertyFile.h>
#include <App/FeaturePython.h>
#include <QRectF>
#include "DrawTemplate.h"

namespace TechDraw
{

class TechDrawExport DrawSVGTemplate: public TechDraw::DrawTemplate
{
    PROPERTY_HEADER(TechDraw::DrawSVGTemplate);

public:
    DrawSVGTemplate();
    ~DrawSVGTemplate();

    App::PropertyFileIncluded PageResult;
    App::PropertyFile Template;

    /** @name methods override Feature */
    //@{
    /// recalculate the Feature
    virtual App::DocumentObjectExecReturn *execute(void);
    //@}

    short mustExecute() const;

    /// returns the type name of the ViewProvider
    virtual const char* getViewProviderName(void) const {
        return "TechDrawGui::ViewProviderTemplate";
    }

    virtual PyObject *getPyObject(void);
    virtual unsigned int getMemSize(void) const;

    double getWidth() const;
    double getHeight() const;
    void getBlockDimensions(double &x, double &y, double &width, double &height) const;

protected:
    void onChanged(const App::Property* prop);

    /// Returns map with <editable name, default text>
    /*!
     * Also populates editableSvgIds
     */
    std::map<std::string, std::string> getEditableTextsFromTemplate();

    QRectF blockDimensions;
};

typedef App::FeaturePythonT<DrawSVGTemplate> DrawSVGTemplatePython;

} //namespace TechDraw

#endif //_TECHDRAW_DrawSVGTemplate_h_
