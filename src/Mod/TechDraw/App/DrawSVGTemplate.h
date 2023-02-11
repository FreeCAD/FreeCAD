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

#ifndef TECHDRAW_DrawSVGTemplate_h_
#define TECHDRAW_DrawSVGTemplate_h_

#include <App/FeaturePython.h>
#include <App/PropertyFile.h>
#include <Mod/TechDraw/TechDrawGlobal.h>

#include "DrawTemplate.h"


namespace TechDraw
{

class TechDrawExport DrawSVGTemplate: public TechDraw::DrawTemplate
{
    PROPERTY_HEADER_WITH_OVERRIDE(TechDraw::DrawSVGTemplate);

public:
    DrawSVGTemplate();
    ~DrawSVGTemplate() override;

    App::PropertyFileIncluded PageResult;
    App::PropertyFile Template;

    void onChanged(const App::Property* prop) override;
   /// returns the type name of the ViewProvider
    const char* getViewProviderName(void) const override {
        return "TechDrawGui::ViewProviderTemplate";
    }

    PyObject *getPyObject(void) override;

    double getWidth() const override;
    double getHeight() const override;

    QString processTemplate();

protected:
    void replaceFileIncluded(std::string newTemplateFileName);
    std::map<std::string, std::string> getEditableTextsFromTemplate();

};

using DrawSVGTemplatePython = App::FeaturePythonT<DrawSVGTemplate>;

} //namespace TechDraw

#endif //TECHDRAW_DrawSVGTemplate_h_
