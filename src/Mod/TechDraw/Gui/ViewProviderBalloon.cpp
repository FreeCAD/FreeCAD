/***************************************************************************
 *   Copyright (c) 2004 Jürgen Riegel <juergen.riegel@web.de>              *
 *   Copyright (c) 2012 Luke Parry <l.parry@warwick.ac.uk>                 *
 *   Copyright (c) 2019 Franck Jullien <franck.jullien@gmail.com>          *
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

/// Here the FreeCAD includes sorted by Base,App,Gui......
#include <Base/Console.h>
#include <Base/Parameter.h>
#include <Base/Exception.h>
#include <Base/Sequencer.h>
#include <App/Application.h>
#include <App/Document.h>
#include <App/DocumentObject.h>

#include <Gui/Application.h>
#include <Gui/BitmapFactory.h>
#include <Gui/Control.h>
#include <Gui/Command.h>
#include <Gui/Document.h>
#include <Gui/MainWindow.h>
#include <Gui/Selection.h>
#include <Gui/ViewProviderDocumentObject.h>

#include <Mod/TechDraw/App/LineGroup.h>

#include "TaskBalloon.h"
#include "ViewProviderBalloon.h"

using namespace TechDrawGui;

PROPERTY_SOURCE(TechDrawGui::ViewProviderBalloon, TechDrawGui::ViewProviderDrawingView)

//**************************************************************************
// Construction/Destruction

ViewProviderBalloon::ViewProviderBalloon()
{
    sPixmap = "TechDraw_Balloon";

    static const char *group = "Balloon Format";

    Base::Reference<ParameterGrp> hGrp = App::GetApplication().GetUserParameter()
                                         .GetGroup("BaseApp")->GetGroup("Preferences")->GetGroup("Mod/TechDraw/Labels");
    std::string fontName = hGrp->GetASCII("LabelFont", "osifont");

    hGrp = App::GetApplication().GetUserParameter()
                                         .GetGroup("BaseApp")->GetGroup("Preferences")->GetGroup("Mod/TechDraw/Dimensions");
    double fontSize = hGrp->GetFloat("FontSize", QGIView::DefaultFontSizeInMM);
    ADD_PROPERTY_TYPE(Font,(fontName.c_str()),group,App::Prop_None, "The name of the font to use");
    ADD_PROPERTY_TYPE(Fontsize,(fontSize),group,(App::PropertyType)(App::Prop_None),"Dimension text size in units");
    
    hGrp = App::GetApplication().GetUserParameter().GetGroup("BaseApp")->GetGroup("Preferences")->GetGroup("Mod/TechDraw/Decorations");
    std::string lgName = hGrp->GetASCII("LineGroup","FC 0.70mm");
    auto lg = TechDraw::LineGroup::lineGroupFactory(lgName);
    double weight = lg->getWeight("Thin");
    delete lg;                                   //Coverity CID 174670
    ADD_PROPERTY_TYPE(LineWidth,(weight),group,(App::PropertyType)(App::Prop_None),"Balloon line width");

    hGrp = App::GetApplication().GetUserParameter()
                                        .GetGroup("BaseApp")->GetGroup("Preferences")->GetGroup("Mod/TechDraw/Dimensions");
    App::Color fcColor;
    fcColor.setPackedValue(hGrp->GetUnsigned("Color", 0x00000000));
    ADD_PROPERTY_TYPE(Color,(fcColor),group,App::Prop_None,"The color of the Dimension");
}

ViewProviderBalloon::~ViewProviderBalloon()
{
}

void ViewProviderBalloon::attach(App::DocumentObject *pcFeat)
{
    // call parent attach method
    ViewProviderDrawingView::attach(pcFeat);
}

void ViewProviderBalloon::setDisplayMode(const char* ModeName)
{
    ViewProviderDrawingView::setDisplayMode(ModeName);
}

std::vector<std::string> ViewProviderBalloon::getDisplayModes(void) const
{
    // get the modes of the father
    std::vector<std::string> StrList = ViewProviderDrawingView::getDisplayModes();

    return StrList;
}

bool ViewProviderBalloon::setEdit(int ModNum)
{
    if (ModNum == ViewProvider::Default ) {
        if (Gui::Control().activeDialog())  {
            return false;
        }
        // clear the selection (convenience)
        Gui::Selection().clearSelection();
        auto qgivBalloon(dynamic_cast<QGIViewBalloon*>(getQView()));
        if (qgivBalloon) {
            Gui::Control().showDialog(new TaskDlgBalloon(qgivBalloon));
        }
        return true;
    } else {
        return ViewProviderDrawingView::setEdit(ModNum);
    }
    return true;
}

void ViewProviderBalloon::unsetEdit(int ModNum)
{
    if (ModNum == ViewProvider::Default) {
        Gui::Control().closeDialog();
    }
    else {
        ViewProviderDrawingView::unsetEdit(ModNum);
    }
}

bool ViewProviderBalloon::doubleClicked(void)
{
    setEdit(ViewProvider::Default);
    return true;
}

void ViewProviderBalloon::updateData(const App::Property* p)
{
    ViewProviderDrawingView::updateData(p);
}

void ViewProviderBalloon::onChanged(const App::Property* p)
{
    if ((p == &Font)  ||
        (p == &Fontsize) ||
        (p == &LineWidth)) {
        QGIView* qgiv = getQView();
        if (qgiv) {
            qgiv->updateView(true);
        }
    }
    Gui::ViewProviderDocumentObject::onChanged(p);
}

TechDraw::DrawViewBalloon* ViewProviderBalloon::getViewObject() const
{
    return dynamic_cast<TechDraw::DrawViewBalloon*>(pcObject);
}

void ViewProviderBalloon::handleChangedPropertyType(Base::XMLReader &reader, const char *TypeName, App::Property *prop)
// transforms properties that had been changed
{
    // property LineWidth had the App::PropertyFloat and was changed to App::PropertyLength
    if (prop == &LineWidth && strcmp(TypeName, "App::PropertyFloat") == 0) {
        App::PropertyFloat LineWidthProperty;
        // restore the PropertyFloat to be able to set its value
        LineWidthProperty.Restore(reader);
        LineWidth.setValue(LineWidthProperty.getValue());
    }
}
