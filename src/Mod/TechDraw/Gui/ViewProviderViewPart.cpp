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

#include "PreCompiled.h"

#ifndef _PreComp_
#include <QMessageBox>
#include <QTextStream>
# ifdef FC_OS_WIN32
#  include <windows.h>
# endif
#endif

#include <App/Application.h>
#include <App/Document.h>
#include <App/DocumentObject.h>
#include <Base/Parameter.h>
#include <Gui/Control.h>
#include <Gui/MainWindow.h>
#include <Gui/Selection.h>

#include <Mod/TechDraw/App/DrawGeomHatch.h>
#include <Mod/TechDraw/App/DrawHatch.h>
#include <Mod/TechDraw/App/DrawLeaderLine.h>
#include <Mod/TechDraw/App/DrawRichAnno.h>
#include <Mod/TechDraw/App/DrawViewBalloon.h>
#include <Mod/TechDraw/App/DrawViewDetail.h>
#include <Mod/TechDraw/App/DrawViewDimension.h>
#include <Mod/TechDraw/App/DrawViewMulti.h>
#include <Mod/TechDraw/App/LineGroup.h>
#include <Mod/TechDraw/App/Cosmetic.h>
#include <Mod/TechDraw/App/CenterLine.h>

#include "PreferencesGui.h"
#include "QGIView.h"
#include "TaskDetail.h"
#include "ViewProviderViewPart.h"
#include "ViewProviderPage.h"
#include "QGIViewDimension.h"
#include "QGIViewBalloon.h"
#include "QGSPage.h"

using namespace TechDrawGui;
using namespace TechDraw;
using DU = DrawUtil;

PROPERTY_SOURCE(TechDrawGui::ViewProviderViewPart, TechDrawGui::ViewProviderDrawingView)

const char* ViewProviderViewPart::LineStyleEnums[] = { "NoLine",
                                                  "Continuous",
                                                  "Dash",
                                                  "Dot",
                                                  "DashDot",
                                                  "DashDotDot",
                                                  nullptr };

//**************************************************************************
// Construction/Destruction

ViewProviderViewPart::ViewProviderViewPart()
{
    sPixmap = "TechDraw_TreeView";

    static const char *group = "Lines";
    static const char *dgroup = "Decoration";
    static const char *hgroup = "Highlight";
    static const char *sgroup = "Section Line";

    //default line weights

    double weight = TechDraw::LineGroup::getDefaultWidth("Thick");
    ADD_PROPERTY_TYPE(LineWidth, (weight), group, App::Prop_None, "The thickness of visible lines (line groups xx.2");

    weight = TechDraw::LineGroup::getDefaultWidth("Thin");
    ADD_PROPERTY_TYPE(HiddenWidth, (weight), group, App::Prop_None, "The thickness of hidden lines, if enabled (line groups xx.1)");

    weight = TechDraw::LineGroup::getDefaultWidth("Graphic");
    ADD_PROPERTY_TYPE(IsoWidth, (weight), group, App::Prop_None, "The thickness of isoparameter lines, if enabled");

    weight = TechDraw::LineGroup::getDefaultWidth("Extra");
    ADD_PROPERTY_TYPE(ExtraWidth, (weight), group, App::Prop_None, "The thickness of LineGroup Extra lines, if enabled");

    double defScale = Preferences::getPreferenceGroup("Decorations")->GetFloat("CenterMarkScale", 0.50);
    bool   defShowCenters = Preferences::getPreferenceGroup("Decorations")->GetBool("ShowCenterMarks", false);

    //decorations
    ADD_PROPERTY_TYPE(HorizCenterLine ,(false), dgroup, App::Prop_None, "Show a horizontal centerline through view");
    ADD_PROPERTY_TYPE(VertCenterLine ,(false), dgroup, App::Prop_None, "Show a vertical centerline through view");
    ADD_PROPERTY_TYPE(ArcCenterMarks ,(defShowCenters), dgroup, App::Prop_None, "Center marks on/off");
    ADD_PROPERTY_TYPE(CenterScale, (defScale), dgroup, App::Prop_None, "Center mark size adjustment, if enabled");

    //properties that affect Section Line
    ADD_PROPERTY_TYPE(ShowSectionLine ,(true)    ,sgroup, App::Prop_None, "Show/hide section line if applicable");
    SectionLineStyle.setEnums(LineStyleEnums);
    ADD_PROPERTY_TYPE(SectionLineStyle, (PreferencesGui::sectionLineStyle()), sgroup, App::Prop_None,
                        "Set section line style if applicable");
    ADD_PROPERTY_TYPE(SectionLineColor, (prefSectionColor()), sgroup, App::Prop_None,
                        "Set section line color if applicable");
    ADD_PROPERTY_TYPE(SectionLineMarks, (PreferencesGui::sectionLineMarks()), sgroup, App::Prop_None,
                        "Show marks at direction changes for ComplexSection");

    //properties that affect Detail Highlights
    HighlightLineStyle.setEnums(LineStyleEnums);
    ADD_PROPERTY_TYPE(HighlightLineStyle, (prefHighlightStyle()), hgroup, App::Prop_None,
                        "Set highlight line style if applicable");
    ADD_PROPERTY_TYPE(HighlightLineColor, (prefHighlightColor()), hgroup, App::Prop_None,
                        "Set highlight line color if applicable");
    ADD_PROPERTY_TYPE(HighlightAdjust, (0.0), hgroup, App::Prop_None, "Adjusts the rotation of the Detail highlight");

    ADD_PROPERTY_TYPE(ShowAllEdges ,(false)    ,dgroup, App::Prop_None, "Temporarily show invisible lines");
}

ViewProviderViewPart::~ViewProviderViewPart()
{

}

void ViewProviderViewPart::onChanged(const App::Property* prop)
{
    if (auto part = getViewPart(); part && part->isDerivedFrom(TechDraw::DrawViewDetail::getClassTypeId()) &&
        prop == &(HighlightAdjust)) {
        auto detail = static_cast<DrawViewDetail*>(getViewPart());
        auto baseDvp = dynamic_cast<DrawViewPart*>(detail->BaseView.getValue());
        if (baseDvp) {
            baseDvp->requestPaint();
        }
        return;
    }

    if (prop == &(LineWidth)   ||
        prop == &(HiddenWidth) ||
        prop == &(IsoWidth) ||
        prop == &(ExtraWidth) ||
        prop == &(HighlightAdjust) ||
        prop == &(ArcCenterMarks) ||
        prop == &(CenterScale) ||
        prop == &(ShowSectionLine)  ||
        prop == &(SectionLineStyle) ||
        prop == &(SectionLineColor) ||
        prop == &(SectionLineMarks) ||
        prop == &(HighlightLineStyle) ||
        prop == &(HighlightLineColor) ||
        prop == &(HorizCenterLine)  ||
        prop == &(VertCenterLine) ) {
        // redraw QGIVP
        QGIView* qgiv = getQView();
        if (qgiv) {
            qgiv->updateView(true);
        }
     }

    ViewProviderDrawingView::onChanged(prop);
}


void ViewProviderViewPart::attach(App::DocumentObject *pcFeat)
{
//    Base::Console().Message("VPVP::attach(%s)\n", pcFeat->getNameInDocument());
    TechDraw::DrawViewMulti* dvm = dynamic_cast<TechDraw::DrawViewMulti*>(pcFeat);
    TechDraw::DrawViewDetail* dvd = dynamic_cast<TechDraw::DrawViewDetail*>(pcFeat);
    if (dvm) {
        sPixmap = "TechDraw_TreeMulti";
    } else if (dvd) {
        sPixmap = "actions/TechDraw_DetailView";
    }

    ViewProviderDrawingView::attach(pcFeat);
}

std::vector<App::DocumentObject*> ViewProviderViewPart::claimChildren() const
{
    // Collect any child Document Objects and put them in the right place in the Feature tree
    // valid children of a ViewPart are:
    //    - Dimensions
    //    - Leaders
    //    - Hatches
    //    - GeomHatches
    //    - Leaders
    std::vector<App::DocumentObject*> temp;
    const std::vector<App::DocumentObject *> &views = getViewPart()->getInList();
    try {
      for(std::vector<App::DocumentObject *>::const_iterator it = views.begin(); it != views.end(); ++it) {
          if((*it)->getTypeId().isDerivedFrom(TechDraw::DrawViewDimension::getClassTypeId())) {
              //TODO: make a list, then prune it.  should be faster?
              bool skip = false;
              std::string dimName = (*it)->getNameInDocument();
              for (auto& t: temp) {                              //only add dim once even if it references 2 geometries
                  std::string tName = t->getNameInDocument();
                  if (dimName == tName) {
                      skip = true;
                      break;
                  }
              }
              if (!skip) {
                  temp.push_back(*it);
              }
          } else if ((*it)->getTypeId().isDerivedFrom(TechDraw::DrawHatch::getClassTypeId())) {
              temp.push_back((*it));
          } else if ((*it)->getTypeId().isDerivedFrom(TechDraw::DrawGeomHatch::getClassTypeId())) {
              temp.push_back((*it));
          } else if ((*it)->getTypeId().isDerivedFrom(TechDraw::DrawViewBalloon::getClassTypeId())) {
              temp.push_back((*it));
          } else if ((*it)->getTypeId().isDerivedFrom(TechDraw::DrawRichAnno::getClassTypeId())) {
              temp.push_back((*it));
          } else if ((*it)->getTypeId().isDerivedFrom(TechDraw::DrawLeaderLine::getClassTypeId())) {
              temp.push_back((*it));
          }
      }
      return temp;
    } catch (...) {
        return std::vector<App::DocumentObject*>();
    }
}

bool ViewProviderViewPart::setEdit(int ModNum)
{
    if (ModNum != ViewProvider::Default ) {
        return ViewProviderDrawingView::setEdit(ModNum);
    }

    if (Gui::Control().activeDialog())  {         //TaskPanel already open!
        return false;
    }
    TechDraw::DrawViewPart* dvp = getViewObject();
    TechDraw::DrawViewDetail* dvd = dynamic_cast<TechDraw::DrawViewDetail*>(dvp);
    if (dvd) {
        if (!dvd->BaseView.getValue()) {
            Base::Console().Error("DrawViewDetail - %s - has no BaseView!\n", dvd->getNameInDocument());
            return false;
        }
        // clear the selection (convenience)
        Gui::Selection().clearSelection();
        Gui::Control().showDialog(new TaskDlgDetail(dvd));
        Gui::Selection().clearSelection();
        Gui::Selection().addSelection(dvd->getDocument()->getName(),
                                        dvd->getNameInDocument());
    }

    return true;
}

bool ViewProviderViewPart::doubleClicked()
{
    setEdit(ViewProvider::Default);
    return true;
}

TechDraw::DrawViewPart* ViewProviderViewPart::getViewObject() const
{
    return dynamic_cast<TechDraw::DrawViewPart*>(pcObject);
}

TechDraw::DrawViewPart* ViewProviderViewPart::getViewPart() const
{
    return getViewObject();
}

void ViewProviderViewPart::handleChangedPropertyType(Base::XMLReader &reader, const char *TypeName, App::Property *prop)
// transforms properties that had been changed
{
    // property LineWidth had the App::PropertyFloat and was changed to App::PropertyLength
    if (prop == &LineWidth && strcmp(TypeName, "App::PropertyFloat") == 0) {
        App::PropertyFloat LineWidthProperty;
        // restore the PropertyFloat to be able to set its value
        LineWidthProperty.Restore(reader);
        LineWidth.setValue(LineWidthProperty.getValue());
    }
    // property HiddenWidth had the App::PropertyFloat and was changed to App::PropertyLength
    else if (prop == &HiddenWidth && strcmp(TypeName, "App::PropertyFloat") == 0) {
        App::PropertyFloat HiddenWidthProperty;
        HiddenWidthProperty.Restore(reader);
        HiddenWidth.setValue(HiddenWidthProperty.getValue());
    }
    // property IsoWidth had the App::PropertyFloat and was changed to App::PropertyLength
    else if (prop == &IsoWidth && strcmp(TypeName, "App::PropertyFloat") == 0) {
        App::PropertyFloat IsoWidthProperty;
        IsoWidthProperty.Restore(reader);
        IsoWidth.setValue(IsoWidthProperty.getValue());
    }
    // property ExtraWidth had the App::PropertyFloat and was changed to App::PropertyLength
    else if (prop == &ExtraWidth && strcmp(TypeName, "App::PropertyFloat") == 0) {
        App::PropertyFloat  ExtraWidthProperty;
        ExtraWidthProperty.Restore(reader);
        ExtraWidth.setValue(ExtraWidthProperty.getValue());
    }
    else {
        ViewProviderDrawingView::handleChangedPropertyType(reader, TypeName, prop);
    }
}

bool ViewProviderViewPart::onDelete(const std::vector<std::string> & subNames)
{
//    Base::Console().Message("VPVP::onDelete() - subs: %d\n", subNames.size());
    // if a cosmetic subelement is in the list of selected subNames then we treat this
    // as a delete of the subelement and not a delete of the DVP
    std::vector<std::string> removables = getSelectedCosmetics(subNames);
    if (!removables.empty()) {
        // we have cosmetics, so remove them and tell Std_Delete not to remove the DVP
        deleteCosmeticElements(removables);
        getViewObject()->recomputeFeature();
        return false;
    }

    // we cannot delete if the view has a section or detail view
    QString bodyMessage;
    QTextStream bodyMessageStream(&bodyMessage);

    // get child views
    auto viewSection = getViewObject()->getSectionRefs();
    auto viewDetail = getViewObject()->getDetailRefs();

    if (!viewSection.empty() || !viewDetail.empty()) {
        bodyMessageStream << qApp->translate("Std_Delete",
            "You cannot delete this view because it has one or more dependent views that would become broken.");
        QMessageBox::warning(Gui::getMainWindow(),
            qApp->translate("Std_Delete", "Object dependencies"), bodyMessage,
            QMessageBox::Ok);
        return false;
    }
    return true;
}

bool ViewProviderViewPart::canDelete(App::DocumentObject *obj) const
{
//    Base::Console().Message("VPVP::canDelete()\n");
    // deletions of part objects (detail view, View etc.) are valid
    // that it cannot be deleted if it has a child view is handled in the onDelete() function
    Q_UNUSED(obj)
    return true;
}

//! extract the names of cosmetic subelements from the list of all selected elements
std::vector<std::string> ViewProviderViewPart::getSelectedCosmetics(std::vector<std::string> subNames)
{
//    Base::Console().Message("VPVP::getSelectedCosmetics(%d removables)\n", subNames.size());

    std::vector<std::string> result;
    // pick out any cosmetic vertices or edges in the selection
    for (auto& sub : subNames) {
        if (DU::getGeomTypeFromName(sub) == "Vertex") {
            if (DU::isCosmeticVertex(getViewObject(), sub)) {
                result.emplace_back(sub);
            }
        } else if (DU::getGeomTypeFromName(sub) == "Edge") {
            if (DU::isCosmeticEdge(getViewObject(), sub)  ||
                DU::isCenterLine(getViewObject(), sub)) {
                result.emplace_back(sub);
            }
        }
    }
    return result;
}

//! delete cosmetic elements for a list of subelement names
void ViewProviderViewPart::deleteCosmeticElements(std::vector<std::string> removables)
{
//    Base::Console().Message("VPVP::deleteCosmeticElements(%d removables)\n", removables.size());
    for (auto& name : removables) {
        if (DU::getGeomTypeFromName(name) == "Vertex") {
            CosmeticVertex* vert = getViewObject()->getCosmeticVertexBySelection(name);
            getViewObject()->removeCosmeticVertex(vert->getTagAsString());
            continue;
        }
        if (DU::getGeomTypeFromName(name) == "Edge") {
            CosmeticEdge* edge = getViewObject()->getCosmeticEdgeBySelection(name);
            if (edge) {
                // if not edge, something has gone very wrong!
                getViewObject()->removeCosmeticEdge(edge->getTagAsString());
                continue;
            }
            CenterLine* line = getViewObject()->getCenterLineBySelection(name);
            if (line) {
                getViewObject()->removeCenterLine(line->getTagAsString());
                continue;
            }
        }
    }
}

App::Color ViewProviderViewPart::prefSectionColor()
{
    return PreferencesGui::sectionLineColor();
}

App::Color ViewProviderViewPart::prefHighlightColor()
{
    App::Color fcColor;
    fcColor.setPackedValue(Preferences::getPreferenceGroup("Decorations")->GetUnsigned("HighlightColor", 0x00000000));
    return fcColor;
}

int ViewProviderViewPart::prefHighlightStyle()
{
    return Preferences::getPreferenceGroup("Decorations")->GetInt("HighlightStyle", 2);
}

// it can happen that Dimensions/Balloons/etc can lose their parent item if the
// the parent is deleted, then undo is invoked.  The linkages on the App side are
// handled by the undo mechanism, but the QGraphicsScene parentage is not reset.
// TODO: does this need to be implemented for Leaderlines and ???? others?
void ViewProviderViewPart::fixSceneDependencies()
{
//    Base::Console().Message("VPVP::fixSceneDependencies()\n");
    auto scene = getViewProviderPage()->getQGSPage();
    auto partQView = getQView();

    auto dimensions =  getViewPart()->getDimensions();
    for (auto& dim : dimensions) {
        auto dimQView = dynamic_cast<QGIViewDimension *>(scene->findQViewForDocObj(dim));
        if (dimQView && dimQView->parentItem() != partQView) {
            // need to add the dim QView to this QGIViewPart
            scene->addDimToParent(dimQView, partQView);
        }
    }

    auto balloons = getViewPart()->getBalloons();
    for (auto& bal : balloons) {
        auto balQView = dynamic_cast<QGIViewBalloon*>(scene->findQViewForDocObj(bal));
        if (balQView && balQView->parentItem() != partQView) {
            // need to add the balloon QView to this QGIViewPart
            scene->addBalloonToParent(balQView, partQView);
        }
    }
}
