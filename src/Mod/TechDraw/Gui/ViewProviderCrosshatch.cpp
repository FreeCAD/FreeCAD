/***************************************************************************
 *   Copyright (c) 2004 Jürgen Riegel <juergen.riegel@web.de>              *
 *   Copyright (c) 2017 Wandererfan <wandererfan@gmail.com>                *
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


#include "PreCompiled.h"

#ifndef _PreComp_
#endif

#include <App/Application.h>
#include <App/Document.h>
#include <App/DocumentObject.h>

#include <Base/Console.h>
#include <Base/Parameter.h>
#include <Base/Exception.h>
#include <Base/Sequencer.h>

//#include <Gui/SoFCSelection.h>
#include <Gui/Selection.h>

#include <Mod/TechDraw/App/DrawCrosshatch.h>
#include <Mod/TechDraw/App/DrawView.h>
#include "ViewProviderCrosshatch.h"

using namespace TechDrawGui;

PROPERTY_SOURCE(TechDrawGui::ViewProviderCrosshatch, Gui::ViewProviderDocumentObject)

//**************************************************************************
// Construction/Destruction

ViewProviderCrosshatch::ViewProviderCrosshatch()
{
    static const char *vgroup = "Format";

    sPixmap = "actions/techdraw-crosshatch";
    App::Color fcColor;
    fcColor.setPackedValue(0x00000000);

    ADD_PROPERTY_TYPE(ColorPattern,(fcColor),vgroup,App::Prop_None,"The color of the pattern");
    ADD_PROPERTY_TYPE(WeightPattern,(0.0),vgroup,App::Prop_None,"Crosshatch pattern line thickness");

}

ViewProviderCrosshatch::~ViewProviderCrosshatch()
{
}

void ViewProviderCrosshatch::attach(App::DocumentObject *pcFeat)
{
    // call parent attach method
    ViewProviderDocumentObject::attach(pcFeat);
}

void ViewProviderCrosshatch::setDisplayMode(const char* ModeName)
{
    ViewProviderDocumentObject::setDisplayMode(ModeName);
}

std::vector<std::string> ViewProviderCrosshatch::getDisplayModes(void) const
{
    // get the modes of the father
    std::vector<std::string> StrList = ViewProviderDocumentObject::getDisplayModes();

    return StrList;
}

//for VP properties
void ViewProviderCrosshatch::onChanged(const App::Property* prop)
{
//    Base::Console().Message("TRACE - VPC::onChanged(%s)\n",prop->getName());

    Gui::ViewProviderDocumentObject::onChanged(prop);
}

//for feature properties
void ViewProviderCrosshatch::updateData(const App::Property* prop)
{
//    Base::Console().Message("TRACE - VPC::updateData(%s)\n",prop->getName());
    if (prop == &WeightPattern   ||
        prop == &ColorPattern     ||
        prop == &(getViewObject()->ScalePattern)) {
        //Base::Console().Message("TRACE - VPC::updateData - should update parent now\n");
        //how does QGIVP find this VP to get properties?
//        Gui::ViewProvider* view = Application::Instance->getDocument(it->pDoc)->getViewProvider(it->pObject);
//        TechDraw::DrawPage* fp = dynamic_cast<TechDraw::DrawPage*>(getDocument()->getObject(PageName.c_str()));
//        Gui::ViewProvider* vp = Gui::Application::Instance->getDocument(getDocument())->getViewProvider(fp);
//        TechDrawGui::ViewProviderPage* dvp = dynamic_cast<TechDrawGui::ViewProviderPage*>(vp);
//        if (dvp) {
//            dvp->show();

        // redraw QGIVP
        //QGIView* qgiv = getQView();    this will be different have to find source's QView
//        if (qgiv) {
//            qgiv->updateView(true);
//        }
     }

    Gui::ViewProviderDocumentObject::updateData(prop);
}

TechDraw::DrawCrosshatch* ViewProviderCrosshatch::getViewObject() const
{
    return dynamic_cast<TechDraw::DrawCrosshatch*>(pcObject);
}
