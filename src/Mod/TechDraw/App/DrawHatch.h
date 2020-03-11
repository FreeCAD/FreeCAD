/***************************************************************************
 *   Copyright (c) 2015 WandererFan <wandererfan@gmail.com>                *
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

#ifndef _TechDraw_DrawHatch_h_
#define _TechDraw_DrawHatch_h_

# include <App/DocumentObject.h>
# include <App/FeaturePython.h>
# include <App/PropertyLinks.h>
#include <App/PropertyFile.h>

namespace TechDraw
{
class DrawViewPart;

class TechDrawExport DrawHatch : public App::DocumentObject
{
    PROPERTY_HEADER_WITH_OVERRIDE(TechDraw::DrawHatch);

public:
    DrawHatch();
    virtual ~DrawHatch();

    App::PropertyVector      DirProjection;                            //Source is only valid for original projection?
    App::PropertyLinkSub     Source;                                   //the dvp & face this hatch belongs to
    App::PropertyFile        HatchPattern;
    App::PropertyFileIncluded SvgIncluded;

    virtual App::DocumentObjectExecReturn *execute(void) override;

    virtual const char* getViewProviderName(void) const override {
        return "TechDrawGui::ViewProviderHatch";
    }
    virtual void unsetupObject(void) override;

    //return PyObject as DrawHatchPy
    virtual PyObject *getPyObject(void) override;

    DrawViewPart* getSourceView(void) const;
    bool affectsFace(int i);
    bool removeSub(std::string toRemove);
    bool removeSub(int i);
    bool empty(void);
    static bool faceIsHatched(int i,std::vector<TechDraw::DrawHatch*> hatchObjs);


protected:
    void onChanged(const App::Property* prop) override;
    virtual void onDocumentRestored() override;
    virtual void setupObject() override;
    void setupSvgIncluded(void);
    void replaceSvgIncluded(std::string newSvgFile);
    void copyFile(std::string inSpec, std::string outSpec);

private:

};

typedef App::FeaturePythonT<DrawHatch> DrawHatchPython;

} //namespace TechDraw
#endif
