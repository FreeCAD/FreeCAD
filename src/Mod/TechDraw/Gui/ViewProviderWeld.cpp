/***************************************************************************
 *   Copyright (c) 2019 Wanderer Fan <wandererfan@gmail.com>               *
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
# include <QMessageBox>
# include <QTextStream>
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

#include "TaskWeldingSymbol.h"

#include "ViewProviderWeld.h"

using namespace TechDrawGui;

PROPERTY_SOURCE(TechDrawGui::ViewProviderWeld, TechDrawGui::ViewProviderDrawingView)

//**************************************************************************
// Construction/Destruction

ViewProviderWeld::ViewProviderWeld()
{
    sPixmap = "actions/techdraw-weldsymbol";
    static const char *group = "Text";

    ADD_PROPERTY_TYPE(Font, (prefFontName().c_str()),group,App::Prop_None, "The name of the font to use");
    ADD_PROPERTY_TYPE(FontSize, (prefFontSize()), group,
                                (App::PropertyType)(App::Prop_None),"Tail text size");
    ADD_PROPERTY_TYPE(TileFontSize, (prefFontSize() * prefTileTextAdjust()), group,
                                (App::PropertyType)(App::Prop_None),"Text size on individual symbol tiles");
}

ViewProviderWeld::~ViewProviderWeld()
{
}

void ViewProviderWeld::attach(App::DocumentObject *pcFeat)
{
    // call parent attach method
    ViewProviderDrawingView::attach(pcFeat);
}

void ViewProviderWeld::setDisplayMode(const char* ModeName)
{
    ViewProviderDrawingView::setDisplayMode(ModeName);
}

std::vector<std::string> ViewProviderWeld::getDisplayModes(void) const
{
    // get the modes of the father
    std::vector<std::string> StrList = ViewProviderDrawingView::getDisplayModes();

    return StrList;
}

void ViewProviderWeld::updateData(const App::Property* prop)
{
    ViewProviderDrawingView::updateData(prop);
}

void ViewProviderWeld::onChanged(const App::Property* p)
{
    QGIView* qgiv = getQView();
    if (qgiv) {
        qgiv->updateView(true);
    }

    ViewProviderDrawingView::onChanged(p);
}

std::vector<App::DocumentObject*> ViewProviderWeld::claimChildren(void) const
{
    // Collect any child Document Objects and put them in the right place in the Feature tree
    // valid children of a DrawWeldSymbol are:
    //    - DrawTiles
    std::vector<App::DocumentObject*> temp;
    const std::vector<App::DocumentObject *> &tiles = getFeature()->getInList();
    try {
        for(std::vector<App::DocumentObject *>::const_iterator it = tiles.begin(); it != tiles.end(); ++it) {
            if ((*it)->getTypeId().isDerivedFrom(TechDraw::DrawTile::getClassTypeId())) {
                temp.push_back((*it));
            }
        }
      return temp;
    } catch (...) {
        std::vector<App::DocumentObject*> tmp;
        return tmp;
    }
}

bool ViewProviderWeld::setEdit(int ModNum)
{
//    Base::Console().Message("VPW::setEdit(%d)\n",ModNum);
    if (ModNum == ViewProvider::Default ) {
        if (Gui::Control().activeDialog())  {         //TaskPanel already open!
            return false;
        }
        // clear the selection (convenience)
        Gui::Selection().clearSelection();
        Gui::Control().showDialog(new TaskDlgWeldingSymbol(getFeature()));
        return true;
    } else {
        return ViewProviderDrawingView::setEdit(ModNum);
    }
    return true;
}

void ViewProviderWeld::unsetEdit(int ModNum)
{
    Q_UNUSED(ModNum);
    if (ModNum == ViewProvider::Default) {
        Gui::Control().closeDialog();
    }
    else {
        ViewProviderDrawingView::unsetEdit(ModNum);
    }
}

bool ViewProviderWeld::doubleClicked(void)
{
//    Base::Console().Message("VPW::doubleClicked()\n");
    setEdit(ViewProvider::Default);
    return true;
}

std::string ViewProviderWeld::prefFontName(void)
{
    Base::Reference<ParameterGrp> hGrp = App::GetApplication().GetUserParameter()
                                         .GetGroup("BaseApp")->GetGroup("Preferences")->
                                          GetGroup("Mod/TechDraw/Labels");
    std::string fontName = hGrp->GetASCII("LabelFont", "osifont");
    return fontName;
}

double ViewProviderWeld::prefFontSize(void)
{
    Base::Reference<ParameterGrp> hGrp = App::GetApplication().GetUserParameter()
                                         .GetGroup("BaseApp")->GetGroup("Preferences")->
                                 GetGroup("Mod/TechDraw/Dimensions");
    double fontSize = hGrp->GetFloat("FontSize", QGIView::DefaultFontSizeInMM);
    return fontSize;
}

double ViewProviderWeld::prefTileTextAdjust(void)
{
    Base::Reference<ParameterGrp> hGrp = App::GetApplication().GetUserParameter()
                                         .GetGroup("BaseApp")->GetGroup("Preferences")->
                                 GetGroup("Mod/TechDraw/Dimensions");
    double adjust   = hGrp->GetFloat("TileTextAdjust", 0.75);
    return adjust;
}

bool ViewProviderWeld::onDelete(const std::vector<std::string> &)
{
    // a weld cannot be deleted if it has a tile

    // get childs
    auto childs = claimChildren();

    if (!childs.empty()) {
        QString bodyMessage;
        QTextStream bodyMessageStream(&bodyMessage);
        bodyMessageStream << qApp->translate("Std_Delete",
            "You cannot delete this weld symbol because\n it has a tile weld that would become broken.");
        QMessageBox::warning(Gui::getMainWindow(),
            qApp->translate("Std_Delete", "Object dependencies"), bodyMessage,
            QMessageBox::Ok);
        return false;
    }
    else {
        return true;
    }
}

bool ViewProviderWeld::canDelete(App::DocumentObject *obj) const
{
    // deletions of Weld objects don't destroy anything
    // thus we can pass this action
    // that the parent LeaderLine cannot be deleted is handled
    // in its onDelete() function
    Q_UNUSED(obj)
    return true;
}

TechDraw::DrawWeldSymbol* ViewProviderWeld::getViewObject() const
{
    return dynamic_cast<TechDraw::DrawWeldSymbol*>(pcObject);
}

TechDraw::DrawWeldSymbol* ViewProviderWeld::getFeature() const
{
    return getViewObject();
}
