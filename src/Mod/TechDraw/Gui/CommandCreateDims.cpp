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
# include <QMessageBox>
# include <iostream>
# include <string>
# include <sstream>
# include <cstdlib>
# include <exception>
# include <boost/regex.hpp>
#endif  //#ifndef _PreComp_

#include <QGraphicsView>

# include <App/DocumentObject.h>
# include <Gui/Action.h>
# include <Gui/Application.h>
# include <Gui/BitmapFactory.h>
# include <Gui/Command.h>
# include <Gui/Control.h>
# include <Gui/Document.h>
# include <Gui/Selection.h>
# include <Gui/MainWindow.h>
# include <Gui/FileDialog.h>
# include <Gui/ViewProvider.h>

# include <Mod/Part/App/PartFeature.h>

# include <Mod/TechDraw/App/DrawViewPart.h>
# include <Mod/TechDraw/App/DrawProjGroupItem.h>
# include <Mod/TechDraw/App/DrawProjGroup.h>
# include <Mod/TechDraw/App/DrawViewDimension.h>
# include <Mod/TechDraw/App/DrawPage.h>
# include <Mod/TechDraw/App/DrawUtil.h>
#include <Mod/TechDraw/Gui/QGVPage.h>


# include "MDIViewPage.h"
# include "ViewProviderPage.h"
#include "TaskLinkDim.h"

using namespace TechDrawGui;
using namespace std;

//===========================================================================
// utility routines
//===========================================================================

//TODO: still need this as separate routine? only used in LinkDimension now
//TODO: code is duplicated in Command and CommandDecorate
TechDraw::DrawPage* _findPageCCD(Gui::Command* cmd)
{
    TechDraw::DrawPage* page = 0;
    //check if a DrawPage is currently displayed
    Gui::MainWindow* w = Gui::getMainWindow();
    Gui::MDIView* mv = w->activeWindow();
    MDIViewPage* mvp = dynamic_cast<MDIViewPage*>(mv);
    if (mvp) {
        QGVPage* qp = mvp->getQGVPage();
        page = qp->getDrawPage();
    } else {
        //DrawPage not displayed, check Selection and/or Document for a DrawPage
        std::vector<App::DocumentObject*> selPages = cmd->getSelection().getObjectsOfType(TechDraw::DrawPage::getClassTypeId());
        if (selPages.empty()) {                                            //no page in selection
            selPages = cmd->getDocument()->getObjectsOfType(TechDraw::DrawPage::getClassTypeId());
            if (selPages.empty()) {                                        //no page in document
                QMessageBox::warning(Gui::getMainWindow(), QObject::tr("No page found"),
                                     QObject::tr("Create a page first."));
                return page;
            } else if (selPages.size() > 1) {                              //multiple pages in document
                QMessageBox::warning(Gui::getMainWindow(), QObject::tr("Too many pages"),
                                     QObject::tr("Can not determine correct page."));
                return page;
            } else {                                                       //use only page in document
                page = dynamic_cast<TechDraw::DrawPage*>(selPages.front());
            }
        } else if (selPages.size() > 1) {                                  //multiple pages in selection
            QMessageBox::warning(Gui::getMainWindow(), QObject::tr("Too many pages"),
                                 QObject::tr("Select exactly 1 page."));
            return page;
        } else {                                                           //use only page in selection
            page = dynamic_cast<TechDraw::DrawPage*>(selPages.front());
        }
    }
    return page;
}

//internal functions
bool _checkSelection(Gui::Command* cmd, unsigned maxObjs = 2);
bool _checkDrawViewPart(Gui::Command* cmd);
bool _checkPartFeature(Gui::Command* cmd);
int _isValidSingleEdge(Gui::Command* cmd);
bool _isValidVertexes(Gui::Command* cmd);
int _isValidEdgeToEdge(Gui::Command* cmd);

enum EdgeType{
        isInvalid,
        isHorizontal,
        isVertical,
        isDiagonal,
        isCircle,
        isCurve,
        isAngle
    };


//===========================================================================
// TechDraw_NewDimension
//===========================================================================

DEF_STD_CMD_A(CmdTechDrawNewDimension);

CmdTechDrawNewDimension::CmdTechDrawNewDimension()
  : Command("TechDraw_NewDimension")
{
    sAppModule      = "TechDraw";
    sGroup          = QT_TR_NOOP("TechDraw");
    sMenuText       = QT_TR_NOOP("Insert a dimension into the drawing");
    sToolTipText    = QT_TR_NOOP("Insert a new dimension");
    sWhatsThis      = "TechDraw_NewDimension";
    sStatusTip      = sToolTipText;
    sPixmap         = "TechDraw_Dimension";
}

void CmdTechDrawNewDimension::activated(int iMsg)
{
    bool result = _checkSelection(this,2);
    if (!result)
        return;
    result = _checkDrawViewPart(this);
    if (!result)
        return;

//do we still need to pick DVPs out of selection? or should we complain about junk?
    std::vector<Gui::SelectionObject> selection = getSelection().getSelectionEx();
    TechDraw::DrawViewPart* objFeat = 0;
    std::vector<std::string> SubNames;
    std::vector<Gui::SelectionObject>::iterator itSel = selection.begin();
    for (; itSel != selection.end(); itSel++)  {
        if ((*itSel).getObject()->isDerivedFrom(TechDraw::DrawViewPart::getClassTypeId())) {
            objFeat = dynamic_cast<TechDraw::DrawViewPart*> ((*itSel).getObject());
            SubNames = (*itSel).getSubNames();
        }
    }
    TechDraw::DrawPage* page = objFeat->findParentPage();
    std::string PageName = page->getNameInDocument();

    TechDraw::DrawViewDimension *dim = 0;
    std::string FeatName = getUniqueObjectName("Dimension");
    std::string dimType;
    bool centerLine = false;

    std::vector<App::DocumentObject *> objs;
    std::vector<std::string> subs;

    int edgeType = _isValidSingleEdge(this);

    if (edgeType) {
        if (edgeType < isCircle) {
            dimType = "Distance";
            objs.push_back(objFeat);
            subs.push_back(SubNames[0]);
        } else if (edgeType == isCircle) {
            dimType = "Radius";
            centerLine = true;
        } else {
            dimType = "Radius";
        }
    } else if (_isValidVertexes(this)) {
        dimType = "Distance";
        objs.push_back(objFeat);
        objs.push_back(objFeat);
        subs.push_back(SubNames[0]);
        subs.push_back(SubNames[1]);
    } else if (_isValidEdgeToEdge(this)) {
        int edgeCase = _isValidEdgeToEdge(this);
        objs.push_back(objFeat);
        objs.push_back(objFeat);
        subs.push_back(SubNames[0]);
        subs.push_back(SubNames[1]);
        switch (edgeCase) {
            //TODO: This didn't have the breaks in it before 17 May, but didn't
            // seem to crash either, so should check whether execution can even
            // get here -Ian-
            case isHorizontal:
                dimType = "DistanceX";
                break;
            case isVertical:
                dimType = "DistanceY";
                break;
            case isDiagonal:
                dimType = "Distance";
                break;
            case isAngle:
                dimType = "Angle";
            default:
                break;
        }
    } else {
        QMessageBox::warning(Gui::getMainWindow(), QObject::tr("Incorrect Selection"),
                                                   QObject::tr("Can't make a Dimension from this selection"));
        return;
    }

    openCommand("Create Dimension");
    doCommand(Doc,"App.activeDocument().addObject('TechDraw::DrawViewDimension','%s')",FeatName.c_str());
    doCommand(Doc,"App.activeDocument().%s.Type = '%s'",FeatName.c_str()
                                                       ,dimType.c_str());

    if (centerLine) {
        doCommand(Doc,"App.activeDocument().%s.CentreLines = True", FeatName.c_str());
    } else {
        doCommand(Doc,"App.activeDocument().%s.CentreLines = False", FeatName.c_str());
    }

    std::string contentStr;
    if (dimType == "Radius") {
        contentStr = "r%value%";
    }
    doCommand(Doc,"App.activeDocument().%s.FormatSpec = '%s'",FeatName.c_str()
                                                          ,contentStr.c_str());

    dim = dynamic_cast<TechDraw::DrawViewDimension *>(getDocument()->getObject(FeatName.c_str()));
    dim->References2D.setValues(objs, subs);

    doCommand(Doc,"App.activeDocument().%s.MeasureType = 'Projected'",FeatName.c_str());
    doCommand(Doc,"App.activeDocument().%s.addView(App.activeDocument().%s)",PageName.c_str(),FeatName.c_str());
    commitCommand();

    //Horrible hack to force Tree update
    double x = objFeat->X.getValue();
    objFeat->X.setValue(x);
}

bool CmdTechDrawNewDimension::isActive(void)
{
    // TODO: Also ensure that there's a part selected?
    return hasActiveDocument();
}

//===========================================================================
// TechDraw_NewRadiusDimension
//===========================================================================

DEF_STD_CMD_A(CmdTechDrawNewRadiusDimension);

CmdTechDrawNewRadiusDimension::CmdTechDrawNewRadiusDimension()
  : Command("TechDraw_NewRadiusDimension")
{
    sAppModule      = "TechDraw";
    sGroup          = QT_TR_NOOP("TechDraw");
    sMenuText       = QT_TR_NOOP("Insert a new radius dimension into the drawing");
    sToolTipText    = QT_TR_NOOP("Insert a new radius dimension feature for the selected view");
    sWhatsThis      = "TechDraw_NewRadiusDimension";
    sStatusTip      = sToolTipText;
    sPixmap         = "TechDraw_Dimension_Radius";
}

void CmdTechDrawNewRadiusDimension::activated(int iMsg)
{
    bool result = _checkSelection(this,1);
    if (!result)
        return;
    result = _checkDrawViewPart(this);
    if (!result)
        return;

    std::vector<Gui::SelectionObject> selection = getSelection().getSelectionEx();
    TechDraw::DrawViewPart * objFeat = 0;
    std::vector<std::string> SubNames;

    std::vector<Gui::SelectionObject>::iterator itSel = selection.begin();
    for (; itSel != selection.end(); itSel++)  {
        if ((*itSel).getObject()->isDerivedFrom(TechDraw::DrawViewPart::getClassTypeId())) {
            objFeat = dynamic_cast<TechDraw::DrawViewPart*> ((*itSel).getObject());
            SubNames = (*itSel).getSubNames();
        }
    }
    TechDraw::DrawPage* page = objFeat->findParentPage();
    std::string PageName = page->getNameInDocument();

    TechDraw::DrawViewDimension *dim = 0;
    std::string FeatName = getUniqueObjectName("Dimension");
    bool centerLine = false;

    std::vector<App::DocumentObject *> objs;
    std::vector<std::string> subs;

    int edgeType = _isValidSingleEdge(this);
    if (edgeType == isCircle) {
        centerLine = true;
        objs.push_back(objFeat);
        subs.push_back(SubNames[0]);
    } else {
        std::stringstream edgeMsg;
        edgeMsg << "Can't make a radius Dimension from this selection (edge type: " << edgeType << ")";
        QMessageBox::warning(Gui::getMainWindow(), QObject::tr("Incorrect Selection"),
                                                   QObject::tr(edgeMsg.str().c_str()));
        return;
    }

    openCommand("Create Dimension");
    doCommand(Doc,"App.activeDocument().addObject('TechDraw::DrawViewDimension','%s')",FeatName.c_str());
    doCommand(Doc,"App.activeDocument().%s.Type = '%s'",FeatName.c_str()
                                                       ,"Radius");

    if (centerLine) {
        doCommand(Doc,"App.activeDocument().%s.CentreLines = True", FeatName.c_str());
    } else {
        doCommand(Doc,"App.activeDocument().%s.CentreLines = False", FeatName.c_str());
    }

    doCommand(Doc, "App.activeDocument().%s.FormatSpec = 'r%%value%%'", FeatName.c_str());

    dim = dynamic_cast<TechDraw::DrawViewDimension *>(getDocument()->getObject(FeatName.c_str()));
    dim->References2D.setValues(objs, subs);

    doCommand(Doc,"App.activeDocument().%s.MeasureType = 'Projected'",FeatName.c_str());
    doCommand(Doc,"App.activeDocument().%s.addView(App.activeDocument().%s)",PageName.c_str(),FeatName.c_str());

    commitCommand();

    //Horrible hack to force Tree update
    double x = objFeat->X.getValue();
    objFeat->X.setValue(x);
}

bool CmdTechDrawNewRadiusDimension::isActive(void)
{
    // TODO: Also ensure that there's a part selected?
    return hasActiveDocument();
}

//===========================================================================
// TechDraw_NewDiameterDimension
//===========================================================================

DEF_STD_CMD_A(CmdTechDrawNewDiameterDimension);

CmdTechDrawNewDiameterDimension::CmdTechDrawNewDiameterDimension()
  : Command("TechDraw_NewDiameterDimension")
{
    sAppModule      = "TechDraw";
    sGroup          = QT_TR_NOOP("TechDraw");
    sMenuText       = QT_TR_NOOP("Insert a new diameter dimension into the drawing");
    sToolTipText    = QT_TR_NOOP("Insert a new diameter dimension feature for the selected view");
    sWhatsThis      = "TechDraw_NewDiameterDimension";
    sStatusTip      = sToolTipText;
    sPixmap         = "TechDraw_Dimension_Diameter";
}

void CmdTechDrawNewDiameterDimension::activated(int iMsg)
{
    bool result = _checkSelection(this,1);
    if (!result)
        return;
    result = _checkDrawViewPart(this);
    if (!result)
        return;

    std::vector<Gui::SelectionObject> selection = getSelection().getSelectionEx();
    TechDraw::DrawViewPart * objFeat = 0;
    std::vector<std::string> SubNames;

    std::vector<Gui::SelectionObject>::iterator itSel = selection.begin();
    for (; itSel != selection.end(); itSel++)  {
        if ((*itSel).getObject()->isDerivedFrom(TechDraw::DrawViewPart::getClassTypeId())) {
            objFeat = dynamic_cast<TechDraw::DrawViewPart*> ((*itSel).getObject());
            SubNames = (*itSel).getSubNames();
        }
    }
    TechDraw::DrawPage* page = objFeat->findParentPage();
    std::string PageName = page->getNameInDocument();

    TechDraw::DrawViewDimension *dim = 0;
    std::string FeatName = getUniqueObjectName("Dimension");
    bool centerLine = false;

    std::vector<App::DocumentObject *> objs;
    std::vector<std::string> subs;

    int edgeType = _isValidSingleEdge(this);
    if (edgeType == isCircle) {
        centerLine = true;
        objs.push_back(objFeat);
        subs.push_back(SubNames[0]);
    } else {
        std::stringstream edgeMsg;
        edgeMsg << "Can't make a diameter Dimension from this selection (edge type: " << edgeType << ")";
        QMessageBox::warning(Gui::getMainWindow(), QObject::tr("Incorrect Selection"),
                                                   QObject::tr(edgeMsg.str().c_str()));
        return;
    }

    openCommand("Create Dimension");
    doCommand(Doc,"App.activeDocument().addObject('TechDraw::DrawViewDimension','%s')",FeatName.c_str());
    doCommand(Doc,"App.activeDocument().%s.Type = '%s'",FeatName.c_str()
                                                       ,"Diameter");

    if (centerLine) {
        doCommand(Doc,"App.activeDocument().%s.CentreLines = True", FeatName.c_str());
    } else {
        doCommand(Doc,"App.activeDocument().%s.CentreLines = False", FeatName.c_str());
    }

    doCommand(Doc, "App.activeDocument().%s.FormatSpec = '\u00d8%%value%%'", FeatName.c_str());   // \u00d8 is Capital O with stroke

    dim = dynamic_cast<TechDraw::DrawViewDimension *>(getDocument()->getObject(FeatName.c_str()));
    dim->References2D.setValues(objs, subs);

    doCommand(Doc,"App.activeDocument().%s.MeasureType = 'Projected'",FeatName.c_str());
    doCommand(Doc,"App.activeDocument().%s.addView(App.activeDocument().%s)",PageName.c_str(),FeatName.c_str());

    commitCommand();

    //Horrible hack to force Tree update
    double x = objFeat->X.getValue();
    objFeat->X.setValue(x);
}

bool CmdTechDrawNewDiameterDimension::isActive(void)
{
    // TODO: Also ensure that there's a part selected?
    return hasActiveDocument();
}

//===========================================================================
// TechDraw_NewLengthDimension
//===========================================================================

DEF_STD_CMD_A(CmdTechDrawNewLengthDimension);

CmdTechDrawNewLengthDimension::CmdTechDrawNewLengthDimension()
  : Command("TechDraw_NewLengthDimension")
{
    sAppModule      = "TechDraw";
    sGroup          = QT_TR_NOOP("TechDraw");
    sMenuText       = QT_TR_NOOP("Insert a new length dimension into the drawing");
    sToolTipText    = QT_TR_NOOP("Insert a new length dimension");
    sWhatsThis      = "TechDraw_NewLengthDimension";
    sStatusTip      = sToolTipText;
    sPixmap         = "TechDraw_Dimension_Length";
}

void CmdTechDrawNewLengthDimension::activated(int iMsg)
{
    bool result = _checkSelection(this,2);
    if (!result)
        return;
    result = _checkDrawViewPart(this);
    if (!result)
        return;

    std::vector<Gui::SelectionObject> selection = getSelection().getSelectionEx();
    TechDraw::DrawViewPart * objFeat = 0;
    std::vector<std::string> SubNames;

    std::vector<Gui::SelectionObject>::iterator itSel = selection.begin();
    for (; itSel != selection.end(); itSel++)  {
        if ((*itSel).getObject()->isDerivedFrom(TechDraw::DrawViewPart::getClassTypeId())) {
            objFeat = dynamic_cast<TechDraw::DrawViewPart*> ((*itSel).getObject());
            SubNames = (*itSel).getSubNames();
        }
    }
    TechDraw::DrawPage* page = objFeat->findParentPage();
    std::string PageName = page->getNameInDocument();

    TechDraw::DrawViewDimension *dim = 0;
    std::string FeatName = getUniqueObjectName("Dimension");
    std::string dimType;

    std::vector<App::DocumentObject *> objs;
    std::vector<std::string> subs;

    int edgeType = _isValidSingleEdge(this);
    if ((edgeType == isHorizontal) ||
        (edgeType == isVertical) ||
        (edgeType == isDiagonal)) {
        objs.push_back(objFeat);
        subs.push_back(SubNames[0]);
    } else if (_isValidVertexes(this)) {
        objs.push_back(objFeat);
        objs.push_back(objFeat);
        subs.push_back(SubNames[0]);
        subs.push_back(SubNames[1]);
    } else if ((_isValidEdgeToEdge(this) == isHorizontal) ||
               (_isValidEdgeToEdge(this) == isVertical) ||
               (_isValidEdgeToEdge(this) == isVertical)) {
        objs.push_back(objFeat);
        objs.push_back(objFeat);
        subs.push_back(SubNames[0]);
        subs.push_back(SubNames[1]);
    } else {
        std::stringstream edgeMsg;
        edgeMsg << "Can't make a length Dimension from this selection (edge type: " << edgeType << ")";
        QMessageBox::warning(Gui::getMainWindow(), QObject::tr("Incorrect Selection"),
                                                   QObject::tr(edgeMsg.str().c_str()));
        return;
    }

    openCommand("Create Dimension");
    doCommand(Doc,"App.activeDocument().addObject('TechDraw::DrawViewDimension','%s')", FeatName.c_str());
    doCommand(Doc,"App.activeDocument().%s.Type = '%s'", FeatName.c_str()
                                                       , "Distance");
    dim = dynamic_cast<TechDraw::DrawViewDimension *>(getDocument()->getObject(FeatName.c_str()));
    dim->References2D.setValues(objs, subs);

    doCommand(Doc, "App.activeDocument().%s.FormatSpec = '%%value%%'", FeatName.c_str());

    doCommand(Doc,"App.activeDocument().%s.MeasureType = 'Projected'",FeatName.c_str());
    doCommand(Doc,"App.activeDocument().%s.addView(App.activeDocument().%s)",PageName.c_str(),FeatName.c_str());

    commitCommand();

    //Horrible hack to force Tree update
    double x = objFeat->X.getValue();
    objFeat->X.setValue(x);
}

bool CmdTechDrawNewLengthDimension::isActive(void)
{
    // TODO: Also ensure that there's a part selected?
    return hasActiveDocument();
}

//===========================================================================
// TechDraw_NewDistanceXDimension
//===========================================================================

DEF_STD_CMD_A(CmdTechDrawNewDistanceXDimension);

CmdTechDrawNewDistanceXDimension::CmdTechDrawNewDistanceXDimension()
  : Command("TechDraw_NewDistanceXDimension")
{
    sAppModule      = "TechDraw";
    sGroup          = QT_TR_NOOP("TechDraw");
    sMenuText       = QT_TR_NOOP("Insert a new horizontal dimension into the drawing");
    sToolTipText    = QT_TR_NOOP("Insert a new horizontal-distance dimension");
    sWhatsThis      = "TechDraw_NewDistanceXDimension";
    sStatusTip      = sToolTipText;
    sPixmap         = "TechDraw_Dimension_Horizontal";
}

void CmdTechDrawNewDistanceXDimension::activated(int iMsg)
{
    bool result = _checkSelection(this,2);
    if (!result)
        return;
    result = _checkDrawViewPart(this);
    if (!result)
        return;

    std::vector<Gui::SelectionObject> selection = getSelection().getSelectionEx();
    TechDraw::DrawViewPart * objFeat = 0;
    std::vector<std::string> SubNames;

    std::vector<Gui::SelectionObject>::iterator itSel = selection.begin();
    for (; itSel != selection.end(); itSel++)  {
        if ((*itSel).getObject()->isDerivedFrom(TechDraw::DrawViewPart::getClassTypeId())) {
            objFeat = dynamic_cast<TechDraw::DrawViewPart*> ((*itSel).getObject());
            SubNames = (*itSel).getSubNames();
        }
    }
    TechDraw::DrawPage* page = objFeat->findParentPage();
    std::string PageName = page->getNameInDocument();

    TechDraw::DrawViewDimension *dim = 0;
    std::string FeatName = getUniqueObjectName("Dimension");
    std::string dimType;

    std::vector<App::DocumentObject *> objs;
    std::vector<std::string> subs;

    int edgeType = _isValidSingleEdge(this);
    if ((edgeType == isHorizontal) ||
        (edgeType == isDiagonal)) {
        objs.push_back(objFeat);
        subs.push_back(SubNames[0]);
    } else if (_isValidVertexes(this)) {
        objs.push_back(objFeat);
        objs.push_back(objFeat);
        subs.push_back(SubNames[0]);
        subs.push_back(SubNames[1]);
    } else if (_isValidEdgeToEdge(this) == isHorizontal) {
        objs.push_back(objFeat);
        objs.push_back(objFeat);
        subs.push_back(SubNames[0]);
        subs.push_back(SubNames[1]);
    } else {
        std::stringstream edgeMsg;
        edgeMsg << "Can't make a horizontal Dimension from this selection (edge type: " << edgeType << ")";
        QMessageBox::warning(Gui::getMainWindow(), QObject::tr("Incorrect Selection"),
                                                   QObject::tr(edgeMsg.str().c_str()));
        return;
    }

    openCommand("Create Dimension");
    doCommand(Doc,"App.activeDocument().addObject('TechDraw::DrawViewDimension','%s')",FeatName.c_str());
    doCommand(Doc,"App.activeDocument().%s.Type = '%s'",FeatName.c_str()
                                                       ,"DistanceX");

    dim = dynamic_cast<TechDraw::DrawViewDimension *>(getDocument()->getObject(FeatName.c_str()));
    dim->References2D.setValues(objs, subs);

    doCommand(Doc, "App.activeDocument().%s.FormatSpec = '%%value%%'", FeatName.c_str());

    doCommand(Doc,"App.activeDocument().%s.MeasureType = 'Projected'",FeatName.c_str());
    doCommand(Doc,"App.activeDocument().%s.addView(App.activeDocument().%s)",PageName.c_str(),FeatName.c_str());

    commitCommand();

    //Horrible hack to force Tree update
    double x = objFeat->X.getValue();
    objFeat->X.setValue(x);
}

bool CmdTechDrawNewDistanceXDimension::isActive(void)
{
    // TODO: Also ensure that there's a part selected?
    return hasActiveDocument();
}

//===========================================================================
// TechDraw_NewDistanceYDimension
//===========================================================================

DEF_STD_CMD_A(CmdTechDrawNewDistanceYDimension);

CmdTechDrawNewDistanceYDimension::CmdTechDrawNewDistanceYDimension()
  : Command("TechDraw_NewDistanceYDimension")
{
    sAppModule      = "TechDraw";
    sGroup          = QT_TR_NOOP("TechDraw");
    sMenuText       = QT_TR_NOOP("Insert a new vertical dimension into the drawing");
    sToolTipText    = QT_TR_NOOP("Insert a new vertical distance dimension");
    sWhatsThis      = "TechDraw_NewDistanceYDimension";
    sStatusTip      = sToolTipText;
    sPixmap         = "TechDraw_Dimension_Vertical";
}

void CmdTechDrawNewDistanceYDimension::activated(int iMsg)
{
    bool result = _checkSelection(this,2);
    if (!result)
        return;
    result = _checkDrawViewPart(this);
    if (!result)
        return;

    std::vector<Gui::SelectionObject> selection = getSelection().getSelectionEx();
    TechDraw::DrawViewPart * objFeat = 0;
    std::vector<std::string> SubNames;

    std::vector<Gui::SelectionObject>::iterator itSel = selection.begin();
    for (; itSel != selection.end(); itSel++)  {
        if ((*itSel).getObject()->isDerivedFrom(TechDraw::DrawViewPart::getClassTypeId())) {
            objFeat = dynamic_cast<TechDraw::DrawViewPart*> ((*itSel).getObject());
            SubNames = (*itSel).getSubNames();
        }
    }
    TechDraw::DrawPage* page = objFeat->findParentPage();
    std::string PageName = page->getNameInDocument();

    TechDraw::DrawViewDimension *dim = 0;
    std::string FeatName = getUniqueObjectName("Dimension");
    std::string dimType;

    std::vector<App::DocumentObject *> objs;
    std::vector<std::string> subs;

    int edgeType = _isValidSingleEdge(this);
    if ((edgeType == isVertical) ||
        (edgeType == isDiagonal)) {
        objs.push_back(objFeat);
        subs.push_back(SubNames[0]);
    } else if (_isValidVertexes(this)) {
        objs.push_back(objFeat);
        objs.push_back(objFeat);
        subs.push_back(SubNames[0]);
        subs.push_back(SubNames[1]);
    } else if (_isValidEdgeToEdge(this) == isVertical) {
        objs.push_back(objFeat);
        objs.push_back(objFeat);
        subs.push_back(SubNames[0]);
        subs.push_back(SubNames[1]);
    } else {
        std::stringstream edgeMsg;
        edgeMsg << "Can't make a vertical Dimension from this selection (edge type: " << edgeType << ")";
        QMessageBox::warning(Gui::getMainWindow(), QObject::tr("Incorrect Selection"),
                                                   QObject::tr(edgeMsg.str().c_str()));
        return;
    }

    openCommand("Create Dimension");
    doCommand(Doc,"App.activeDocument().addObject('TechDraw::DrawViewDimension','%s')",FeatName.c_str());
    doCommand(Doc,"App.activeDocument().%s.Type = '%s'",FeatName.c_str()
                                                       ,"DistanceY");
    dim = dynamic_cast<TechDraw::DrawViewDimension *>(getDocument()->getObject(FeatName.c_str()));
    dim->References2D.setValues(objs, subs);

    doCommand(Doc, "App.activeDocument().%s.FormatSpec = '%%value%%'", FeatName.c_str());

    doCommand(Doc,"App.activeDocument().%s.MeasureType = 'Projected'",FeatName.c_str());
    doCommand(Doc,"App.activeDocument().%s.addView(App.activeDocument().%s)",PageName.c_str(),FeatName.c_str());

    commitCommand();

    //Horrible hack to force Tree update
    double x = objFeat->X.getValue();
    objFeat->X.setValue(x);
}

bool CmdTechDrawNewDistanceYDimension::isActive(void)
{
    // TODO: Also ensure that there's a part selected?
    return hasActiveDocument();
}

//===========================================================================
// TechDraw_NewAngleDimension
//===========================================================================

DEF_STD_CMD_A(CmdTechDrawNewAngleDimension);

CmdTechDrawNewAngleDimension::CmdTechDrawNewAngleDimension()
  : Command("TechDraw_NewAngleDimension")
{
    sAppModule      = "TechDraw";
    sGroup          = QT_TR_NOOP("TechDraw");
    sMenuText       = QT_TR_NOOP("Insert a new angle dimension into the drawing");
    sToolTipText    = QT_TR_NOOP("Insert a new angle dimension");
    sWhatsThis      = "TechDraw_NewAngleDimension";
    sStatusTip      = sToolTipText;
    sPixmap         = "TechDraw_Dimension_Angle";
}

void CmdTechDrawNewAngleDimension::activated(int iMsg)
{
    bool result = _checkSelection(this,2);
    if (!result)
        return;
    result = _checkDrawViewPart(this);
    if (!result)
        return;

    std::vector<Gui::SelectionObject> selection = getSelection().getSelectionEx();
    TechDraw::DrawViewPart * objFeat = 0;
    std::vector<std::string> SubNames;

    std::vector<Gui::SelectionObject>::iterator itSel = selection.begin();
    for (; itSel != selection.end(); itSel++)  {
        if ((*itSel).getObject()->isDerivedFrom(TechDraw::DrawViewPart::getClassTypeId())) {
            objFeat = dynamic_cast<TechDraw::DrawViewPart*> ((*itSel).getObject());
            SubNames = (*itSel).getSubNames();
        }
    }
    TechDraw::DrawPage* page = objFeat->findParentPage();
    std::string PageName = page->getNameInDocument();

    TechDraw::DrawViewDimension *dim = 0;
    std::string FeatName = getUniqueObjectName("Dimension");

    std::vector<App::DocumentObject *> objs;
    std::vector<std::string> subs;

    int edgeType = _isValidEdgeToEdge(this);
    if (edgeType == isAngle) {
        objs.push_back(objFeat);
        objs.push_back(objFeat);
        subs.push_back(SubNames[0]);
        subs.push_back(SubNames[1]);
    } else {
        QMessageBox::warning(Gui::getMainWindow(), QObject::tr("Incorrect Selection"),
                                                   QObject::tr("Can't make an angle Dimension from this selection"));
        return;
    }

    openCommand("Create Dimension");
    doCommand(Doc,"App.activeDocument().addObject('TechDraw::DrawViewDimension','%s')",FeatName.c_str());
    doCommand(Doc,"App.activeDocument().%s.Type = '%s'",FeatName.c_str()
                                                       ,"Angle");

    dim = dynamic_cast<TechDraw::DrawViewDimension *>(getDocument()->getObject(FeatName.c_str()));
    dim->References2D.setValues(objs, subs);

    doCommand(Doc,"App.activeDocument().%s.MeasureType = 'Projected'",FeatName.c_str());
    doCommand(Doc,"App.activeDocument().%s.addView(App.activeDocument().%s)",PageName.c_str(),FeatName.c_str());

    commitCommand();

    //Horrible hack to force Tree update
    double x = objFeat->X.getValue();
    objFeat->X.setValue(x);
}

bool CmdTechDrawNewAngleDimension::isActive(void)
{
    // TODO: Also ensure that there's a part selected?
    return hasActiveDocument();
}

//! link 3D geometry to Dimension(s) on a Page
//TODO: should we present all potential Dimensions from all Pages?
//===========================================================================
// TechDraw_LinkDimension
//===========================================================================

DEF_STD_CMD_A(CmdTechDrawLinkDimension);

CmdTechDrawLinkDimension::CmdTechDrawLinkDimension()
  : Command("TechDraw_LinkDimension")
{
    sAppModule      = "TechDraw";
    sGroup          = QT_TR_NOOP("TechDraw");
    sMenuText       = QT_TR_NOOP("Link a dimension to 3D geometry");
    sToolTipText    = QT_TR_NOOP("Link a dimension to 3D geometry");
    sWhatsThis      = "TechDraw_LinkDimension";
    sStatusTip      = sToolTipText;
    sPixmap         = "TechDraw_Dimension_Link";
}

void CmdTechDrawLinkDimension::activated(int iMsg)
{
    TechDraw::DrawPage* page = _findPageCCD(this);
    if (!page) {
        return;
    }
    std::string PageName = page->getNameInDocument();

    bool result = _checkSelection(this,2);
    if (!result)
        return;

    std::vector<Gui::SelectionObject> selection = getSelection().getSelectionEx();
    Part::Feature* obj3D = 0;
    std::vector<std::string> subs;

    std::vector<Gui::SelectionObject>::iterator itSel = selection.begin();
    for (; itSel != selection.end(); itSel++)  {
        if ((*itSel).getObject()->isDerivedFrom(Part::Feature::getClassTypeId())) {
            obj3D = dynamic_cast<Part::Feature*> ((*itSel).getObject());
            subs = (*itSel).getSubNames();
        }
    }

    if (!obj3D) {
        QMessageBox::warning(Gui::getMainWindow(), QObject::tr("Incorrect Selection"),
                                                   QObject::tr("Can't link a dimension to this selection"));
        return;
    }

    // dialog to select the Dimension to link
    Gui::Control().showDialog(new TaskDlgLinkDim(obj3D,subs,page));

    page->getDocument()->recompute();                                  //still need to recompute in Gui. why?
}

bool CmdTechDrawLinkDimension::isActive(void)
{
    return hasActiveDocument();
}

void CreateTechDrawCommandsDims(void)
{
    Gui::CommandManager &rcCmdMgr = Gui::Application::Instance->commandManager();

    rcCmdMgr.addCommand(new CmdTechDrawNewDimension());
    rcCmdMgr.addCommand(new CmdTechDrawNewRadiusDimension());
    rcCmdMgr.addCommand(new CmdTechDrawNewDiameterDimension());
    rcCmdMgr.addCommand(new CmdTechDrawNewLengthDimension());
    rcCmdMgr.addCommand(new CmdTechDrawNewDistanceXDimension());
    rcCmdMgr.addCommand(new CmdTechDrawNewDistanceYDimension());
    rcCmdMgr.addCommand(new CmdTechDrawNewAngleDimension());
    rcCmdMgr.addCommand(new CmdTechDrawLinkDimension());
}

//===========================================================================
// Selection Validation Helpers
//===========================================================================

//! common checks of Selection for Dimension commands
//non-empty selection, no more than maxObjs selected and at least 1 DrawingPage exists
bool _checkSelection(Gui::Command* cmd, unsigned maxObjs) {
    std::vector<Gui::SelectionObject> selection = cmd->getSelection().getSelectionEx();
    if (selection.size() == 0) {
        QMessageBox::warning(Gui::getMainWindow(), QObject::tr("Incorrect selection"),
                             QObject::tr("Select an object first"));
        return false;
    }

    const std::vector<std::string> SubNames = selection[0].getSubNames();
    if (SubNames.size() > maxObjs){
        QMessageBox::warning(Gui::getMainWindow(), QObject::tr("Incorrect selection"),
            QObject::tr("Too many objects selected"));
        return false;
    }

    std::vector<App::DocumentObject*> pages = cmd->getDocument()->getObjectsOfType(TechDraw::DrawPage::getClassTypeId());
    if (pages.empty()){
        QMessageBox::warning(Gui::getMainWindow(), QObject::tr("Incorrect selection"),
            QObject::tr("Create a page first."));
        return false;
    }
    return true;
}

bool _checkDrawViewPart(Gui::Command* cmd) {
    std::vector<Gui::SelectionObject> selection = cmd->getSelection().getSelectionEx();
    TechDraw::DrawViewPart * objFeat = dynamic_cast<TechDraw::DrawViewPart *>(selection[0].getObject());
    if(!objFeat) {
        QMessageBox::warning(Gui::getMainWindow(), QObject::tr("Incorrect selection"),
                             QObject::tr("No DrawViewPart in selection."));
        return false;
    }
    return true;
}

bool _checkPartFeature(Gui::Command* cmd) {
    bool result = false;
    std::vector<Gui::SelectionObject> selection = cmd->getSelection().getSelectionEx();
    std::vector<Gui::SelectionObject>::iterator itSel = selection.begin();
    for (; itSel != selection.end(); itSel++) {
        if (itSel->isDerivedFrom(Part::Feature::getClassTypeId())) {
            result = true;
        }
    }
    if(!result) {
        QMessageBox::warning(Gui::getMainWindow(), QObject::tr("Incorrect selection"),
                             QObject::tr("No DrawViewPart in selection."));
    }
    return result;
}

//! verify that Selection contains a valid Geometry for a single Edge Dimension
int _isValidSingleEdge(Gui::Command* cmd) {
    int edgeType = isInvalid;
    std::vector<Gui::SelectionObject> selection = cmd->getSelection().getSelectionEx();
    TechDraw::DrawViewPart * objFeat = dynamic_cast<TechDraw::DrawViewPart *>(selection[0].getObject());
    const std::vector<std::string> SubNames = selection[0].getSubNames();
    if (SubNames.size() == 1) {                                                 //only 1 subshape selected
        if (DrawUtil::getGeomTypeFromName(SubNames[0]) == "Edge") {                                //the Name starts with "Edge"
            int GeoId = DrawUtil::getIndexFromName(SubNames[0]);
            TechDrawGeometry::BaseGeom* geom = objFeat->getProjEdgeByIndex(GeoId);
            if (!geom) {
                Base::Console().Error("Logic Error: no geometry for GeoId: %d\n",GeoId);
                return isInvalid;
            }

            if(geom->geomType == TechDrawGeometry::GENERIC) {
                TechDrawGeometry::Generic* gen1 = static_cast<TechDrawGeometry::Generic *>(geom);
                if(gen1->points.size() > 2) {                                   //the edge is a polyline
                    return isInvalid;
                }
                Base::Vector2D line = gen1->points.at(1) - gen1->points.at(0);
                if(fabs(line.fY) < FLT_EPSILON ) {
                    edgeType = isHorizontal;
                } else if(fabs(line.fX) < FLT_EPSILON) {
                    edgeType = isVertical;
                } else {
                    edgeType = isDiagonal;
                }
            } else if (geom->geomType == TechDrawGeometry::CIRCLE ||
                       geom->geomType == TechDrawGeometry::ELLIPSE ||
                       geom->geomType == TechDrawGeometry::ARCOFCIRCLE ||
                       geom->geomType == TechDrawGeometry::ARCOFELLIPSE ) {
                edgeType = isCircle;
            } else if (geom->geomType == TechDrawGeometry::BSPLINE) {
                edgeType = isCurve;
            } else {
                edgeType = isInvalid;
            }
        }
    }
    return edgeType;
}

//! verify that Selection contains valid geometries for a Vertex to Vertex Dimension
bool _isValidVertexes(Gui::Command* cmd) {
    std::vector<Gui::SelectionObject> selection = cmd->getSelection().getSelectionEx();
    const std::vector<std::string> SubNames = selection[0].getSubNames();
    if(SubNames.size() == 2) {                                         //there are 2
        if (DrawUtil::getGeomTypeFromName(SubNames[0]) == "Vertex" &&                    //they both start with "Vertex"
            DrawUtil::getGeomTypeFromName(SubNames[1]) == "Vertex") {
            return true;
        }
    }
    return false;
}

//! verify that the Selection contains valid geometries for an Edge to Edge Dimension
int _isValidEdgeToEdge(Gui::Command* cmd) {
//TODO: can the edges be in 2 different features??
    int edgeType = isInvalid;
    std::vector<Gui::SelectionObject> selection = cmd->getSelection().getSelectionEx();
    TechDraw::DrawViewPart* objFeat0 = dynamic_cast<TechDraw::DrawViewPart *>(selection[0].getObject());
    //TechDraw::DrawViewPart* objFeat1 = dynamic_cast<TechDraw::DrawViewPart *>(selection[1].getObject());
    const std::vector<std::string> SubNames = selection[0].getSubNames();
    if(SubNames.size() == 2) {                                         //there are 2
        if (DrawUtil::getGeomTypeFromName(SubNames[0]) == "Edge" &&                      //they both start with "Edge"
            DrawUtil::getGeomTypeFromName(SubNames[1]) == "Edge") {
            int GeoId0 = DrawUtil::getIndexFromName(SubNames[0]);
            int GeoId1 = DrawUtil::getIndexFromName(SubNames[1]);
            TechDrawGeometry::BaseGeom* geom0 = objFeat0->getProjEdgeByIndex(GeoId0);
            TechDrawGeometry::BaseGeom* geom1 = objFeat0->getProjEdgeByIndex(GeoId1);
            if ((!geom0) || (!geom1)) {
                Base::Console().Error("Logic Error: no geometry for GeoId: %d or GeoId: %d\n",GeoId0,GeoId1);
                return isInvalid;
            }

            if(geom0->geomType == TechDrawGeometry::GENERIC &&
               geom1->geomType == TechDrawGeometry::GENERIC) {
                TechDrawGeometry::Generic *gen0 = static_cast<TechDrawGeometry::Generic *>(geom0);
                TechDrawGeometry::Generic *gen1 = static_cast<TechDrawGeometry::Generic *>(geom1);
                if(gen0->points.size() > 2 ||
                   gen1->points.size() > 2) {                          //the edge is a polyline
                    return isInvalid;
                }
                Base::Vector2D line0 = gen0->points.at(1) - gen0->points.at(0);
                Base::Vector2D line1 = gen1->points.at(1) - gen1->points.at(0);
                double xprod = fabs(line0.fX * line1.fY - line0.fY * line1.fX);
                if(xprod > FLT_EPSILON) {                              //edges are not parallel
                    return isAngle;
                }
                if(fabs(line0.fX) < FLT_EPSILON && fabs(line1.fX) < FLT_EPSILON) {
                    edgeType = isHorizontal;
                } else if(fabs(line0.fY) < FLT_EPSILON && fabs(line1.fY) < FLT_EPSILON) {
                    edgeType = isVertical;
                } else {
                    edgeType = isDiagonal;
                }
            } else {
                return isInvalid;
            }
        }
    }
    return edgeType;
}
