/***************************************************************************
 *   Copyright (c) 2004 Jürgen Riegel <juergen.riegel@web.de>              *
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

#include <App/DocumentObject.h>
#include <Gui/Application.h>
#include <Gui/Control.h>
#include <Gui/MainWindow.h>
#include <Gui/Selection.h>

#include <Mod/TechDraw/App/LineGroup.h>
#include <Mod/TechDraw/App/DrawLeaderLine.h>
#include <Mod/TechDraw/App/DrawRichAnno.h>
#include <Mod/TechDraw/App/DrawWeldSymbol.h>

#include "PreferencesGui.h"
#include "ZVALUE.h"
#include "QGIView.h"
#include "TaskLeaderLine.h"
#include "ViewProviderLeader.h"

using namespace TechDrawGui;
using namespace TechDraw;

PROPERTY_SOURCE(TechDrawGui::ViewProviderLeader, TechDrawGui::ViewProviderDrawingView)

const char* ViewProviderLeader::LineStyleEnums[] = { "NoLine",
                                                  "Continuous",
                                                  "Dash",
                                                  "Dot",
                                                  "DashDot",
                                                  "DashDotDot",
                                                  nullptr };

//**************************************************************************
// Construction/Destruction

ViewProviderLeader::ViewProviderLeader()
{
    sPixmap = "actions/TechDraw_LeaderLine";

    static const char *group = "Line Format";

    ADD_PROPERTY_TYPE(LineWidth, (getDefLineWeight()), group, (App::PropertyType)(App::Prop_None), "Line width");
    LineStyle.setEnums(LineStyleEnums);
    ADD_PROPERTY_TYPE(LineStyle, (1), group, (App::PropertyType)(App::Prop_None), "Line style");
    ADD_PROPERTY_TYPE(Color, (getDefLineColor()), group, App::Prop_None, "Color of the Markup");

    StackOrder.setValue(ZVALUE::DIMENSION);
}

ViewProviderLeader::~ViewProviderLeader()
{
}

bool ViewProviderLeader::setEdit(int ModNum)
{
//    Base::Console().Message("VPL::setEdit(%d)\n", ModNum);
    if (ModNum != ViewProvider::Default) {
        return ViewProviderDrawingView::setEdit(ModNum);
    }

    if (Gui::Control().activeDialog()) {
        return false; //TaskPanel already open!
    }
    Gui::Selection().clearSelection();
    Gui::Control().showDialog(new TaskDlgLeaderLine(this));
    return true;
}

bool ViewProviderLeader::doubleClicked()
{
//    Base::Console().Message("VPL::doubleClicked()\n");
    setEdit(ViewProvider::Default);
    return true;
}

void ViewProviderLeader::updateData(const App::Property* p)
{
    if (!getFeature()->isRestoring())  {
        if (p == &getFeature()->LeaderParent)  {
            App::DocumentObject* docObj = getFeature()->LeaderParent.getValue();
            TechDraw::DrawView* dv = dynamic_cast<TechDraw::DrawView*>(docObj);
            if (dv) {
                QGIView* qgiv = getQView();
                if (qgiv) {
                    qgiv->onSourceChange(dv);
                }
            }
        }
    }
    ViewProviderDrawingView::updateData(p);
}

void ViewProviderLeader::onChanged(const App::Property* p)
{
    if ((p == &Color) ||
        (p == &LineWidth) ||
        (p == &LineStyle)) {
        QGIView* qgiv = getQView();
        if (qgiv) {
            qgiv->updateView(true);
        }
    }
    ViewProviderDrawingView::onChanged(p);
}

std::vector<App::DocumentObject*> ViewProviderLeader::claimChildren() const
{
    // Collect any child Document Objects and put them in the right place in the Feature tree
    // valid children of a ViewLeader are:
    //    - Rich Annotations
    //    - Weld Symbols
    std::vector<App::DocumentObject*> temp;
    const std::vector<App::DocumentObject *> &views = getFeature()->getInList();
    try {
       for(std::vector<App::DocumentObject *>::const_iterator it = views.begin(); it != views.end(); ++it) {
           if ((*it)->getTypeId().isDerivedFrom(TechDraw::DrawRichAnno::getClassTypeId())) {
                temp.push_back((*it));
           } else if ((*it)->getTypeId().isDerivedFrom(TechDraw::DrawWeldSymbol::getClassTypeId())) {
                temp.push_back((*it));
            }
        }
        return temp;
    }
    catch (...) {
        return std::vector<App::DocumentObject*>();
    }
}

TechDraw::DrawLeaderLine* ViewProviderLeader::getViewObject() const
{
    return dynamic_cast<TechDraw::DrawLeaderLine*>(pcObject);
}

TechDraw::DrawLeaderLine* ViewProviderLeader::getFeature() const
{
    return dynamic_cast<TechDraw::DrawLeaderLine*>(pcObject);
}

double ViewProviderLeader::getDefLineWeight()
{
    return TechDraw::LineGroup::getDefaultWidth("Thin");
}

App::Color ViewProviderLeader::getDefLineColor()
{
    return PreferencesGui::leaderColor();
}

void ViewProviderLeader::handleChangedPropertyType(Base::XMLReader &reader, const char *TypeName, App::Property *prop)
// transforms properties that had been changed
{
    // property LineWidth had the App::PropertyFloat and was changed to App::PropertyLength
    if (prop == &LineWidth && strcmp(TypeName, "App::PropertyFloat") == 0) {
        App::PropertyFloat LineWidthProperty;
        // restore the PropertyFloat to be able to set its value
        LineWidthProperty.Restore(reader);
        LineWidth.setValue(LineWidthProperty.getValue());
    }

    // property LineStyle had the App::PropertyInteger and was changed to App::PropertyIntegerConstraint
    else if (prop == &LineStyle && strcmp(TypeName, "App::PropertyInteger") == 0) {
        App::PropertyInteger LineStyleProperty;
        // restore the PropertyInteger to be able to set its value
        LineStyleProperty.Restore(reader);
        LineStyle.setValue(LineStyleProperty.getValue());
    }

    // property LineStyle had the App::PropertyIntegerConstraint and was changed to App::PropertyEnumeration
    else if (prop == &LineStyle && strcmp(TypeName, "App::PropertyIntegerConstraint") == 0) {
        App::PropertyIntegerConstraint LineStyleProperty;
        // restore the PropertyIntegerConstraint to be able to set its value
        LineStyleProperty.Restore(reader);
        LineStyle.setValue(LineStyleProperty.getValue());
    }

    else {
        ViewProviderDrawingView::handleChangedPropertyType(reader, TypeName, prop);
    }
}

bool ViewProviderLeader::onDelete(const std::vector<std::string> &)
{
    // a leader line cannot be deleted if it has a child weld symbol

    // get childs
    auto childs = claimChildren();

    if (childs.empty()) {
        return true;
    }

    QString bodyMessage;
    QTextStream bodyMessageStream(&bodyMessage);
    bodyMessageStream << qApp->translate("Std_Delete",
        "You cannot delete this leader line because\nit has a weld symbol that would become broken.");
    QMessageBox::warning(Gui::getMainWindow(),
        qApp->translate("Std_Delete", "Object dependencies"), bodyMessage,
        QMessageBox::Ok);
    return false;
}

bool ViewProviderLeader::canDelete(App::DocumentObject *obj) const
{
    // deletions of Leader line objects don't destroy anything
    // thus we can pass this action
    // that the parent view cannot be deleted is handled
    // in its onDelete() function
    Q_UNUSED(obj)
    return true;
}
