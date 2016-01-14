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

# include <Mod/Drawing/App/DrawViewPart.h>
# include <Mod/Drawing/App/DrawProjGroupItem.h>
# include <Mod/Drawing/App/DrawProjGroup.h>
# include <Mod/Drawing/App/DrawViewDimension.h>
# include <Mod/Drawing/App/DrawPage.h>
# include <Mod/Drawing/App/DrawUtil.h>

# include "MDIViewPage.h"
# include "TaskDialog.h"
# include "ViewProviderPage.h"

using namespace TechDrawGui;
using namespace std;

//internal functions
bool _checkSelection(Gui::Command* cmd);
int _isValidSingleEdge(Gui::Command* cmd, bool trueDim=true);
bool _isValidVertexes(Gui::Command* cmd);
int _isValidEdgeToEdge(Gui::Command* cmd, bool trueDim=true);
bool _isTrueAllowed(TechDraw::DrawViewPart* objFeat, const std::vector<std::string> &SubNames);

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
// Drawing_NewDimension
//===========================================================================

DEF_STD_CMD(CmdDrawingNewDimension);

CmdDrawingNewDimension::CmdDrawingNewDimension()
  : Command("Drawing_NewDimension")
{
    sAppModule      = "Drawing";
    sGroup          = QT_TR_NOOP("Drawing");
    sMenuText       = QT_TR_NOOP("Insert a dimension into the drawing");
    sToolTipText    = QT_TR_NOOP("Insert a new dimension");
    sWhatsThis      = "Drawing_NewDimension";
    sStatusTip      = sToolTipText;
    sPixmap         = "Dimension";
}

void CmdDrawingNewDimension::activated(int iMsg)
{
    bool result = _checkSelection(this);
    if (!result)
        return;

    std::vector<Gui::SelectionObject> selection = getSelection().getSelectionEx();
    TechDraw::DrawViewPart * objFeat = dynamic_cast<TechDraw::DrawViewPart *>(selection[0].getObject());
    const std::vector<std::string> &SubNames = selection[0].getSubNames();

    TechDraw::DrawViewDimension *dim = 0;
    std::string FeatName = getUniqueObjectName("Dimension");
    std::string dimType;
    bool centerLine = false;

    std::vector<App::DocumentObject *> objs;
    std::vector<std::string> subs;

    //selected edge(s) must have valid reference to Source edge for True Dimension
    //otherwise Dimension must be Projected
    bool trueDimAllowed = _isTrueAllowed(objFeat,SubNames);
    int edgeType = _isValidSingleEdge(this,trueDimAllowed);

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
    if (dimType == "Angle") {
        contentStr = "%value%\x00b0";
    } else if (dimType == "Radius") {
        contentStr = "r%value%";
    }
    doCommand(Doc,"App.activeDocument().%s.FormatSpec = '%s'",FeatName.c_str()
                                                          ,contentStr.c_str());

    dim = dynamic_cast<TechDraw::DrawViewDimension *>(getDocument()->getObject(FeatName.c_str()));
    dim->References.setValues(objs, subs);

    // make a True dimension if you can, Projected otherwise
    if (trueDimAllowed) {
        doCommand(Doc,"App.activeDocument().%s.ProjectionType = 'True'",FeatName.c_str());
    } else {
        doCommand(Doc,"App.activeDocument().%s.ProjectionType = 'Projected'",FeatName.c_str());
    }

    dim->execute();

    std::vector<App::DocumentObject*> pages = getDocument()->getObjectsOfType(TechDraw::DrawPage::getClassTypeId());
    TechDraw::DrawPage *page = dynamic_cast<TechDraw::DrawPage *>(pages.front());
    page->addView(page->getDocument()->getObject(FeatName.c_str()));

    commitCommand();

    //Horrible hack to force Tree update
    double x = objFeat->X.getValue();
    objFeat->X.setValue(x);
}

//===========================================================================
// Drawing_NewRadiusDimension
//===========================================================================

DEF_STD_CMD(CmdDrawingNewRadiusDimension);

CmdDrawingNewRadiusDimension::CmdDrawingNewRadiusDimension()
  : Command("Drawing_NewRadiusDimension")
{
    sAppModule      = "Drawing";
    sGroup          = QT_TR_NOOP("Drawing");
    sMenuText       = QT_TR_NOOP("Insert a new radius dimension into the drawing");
    sToolTipText    = QT_TR_NOOP("Insert a new radius dimension feature for the selected view");
    sWhatsThis      = "Drawing_NewRadiusDimension";
    sStatusTip      = sToolTipText;
    sPixmap         = "Dimension_Radius";
}

void CmdDrawingNewRadiusDimension::activated(int iMsg)
{
    bool result = _checkSelection(this);
    if (!result)
        return;

    std::vector<Gui::SelectionObject> selection = getSelection().getSelectionEx();
    TechDraw::DrawViewPart * objFeat = dynamic_cast<TechDraw::DrawViewPart *>(selection[0].getObject());
    const std::vector<std::string> &SubNames = selection[0].getSubNames();

    TechDraw::DrawViewDimension *dim = 0;
    std::string FeatName = getUniqueObjectName("Dimension");
    bool centerLine = false;

    std::vector<App::DocumentObject *> objs;
    std::vector<std::string> subs;

    //selected edge(s) must have valid reference to Source edge for True Dimension
    //otherwise Dimension must be Projected
    bool trueDimAllowed = _isTrueAllowed(objFeat,SubNames);
    int edgeType = _isValidSingleEdge(this,trueDimAllowed);
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
    dim->References.setValues(objs, subs);

    if (trueDimAllowed) {
        doCommand(Doc,"App.activeDocument().%s.ProjectionType = 'True'",FeatName.c_str());
    } else {
        doCommand(Doc,"App.activeDocument().%s.ProjectionType = 'Projected'",FeatName.c_str());
    }

    dim->execute();

    std::vector<App::DocumentObject*> pages = getDocument()->getObjectsOfType(TechDraw::DrawPage::getClassTypeId());
    TechDraw::DrawPage *page = dynamic_cast<TechDraw::DrawPage *>(pages.front());
    page->addView(page->getDocument()->getObject(FeatName.c_str()));

    commitCommand();

    //Horrible hack to force Tree update
    double x = objFeat->X.getValue();
    objFeat->X.setValue(x);
}

//===========================================================================
// Drawing_NewDiameterDimension
//===========================================================================

DEF_STD_CMD(CmdDrawingNewDiameterDimension);

CmdDrawingNewDiameterDimension::CmdDrawingNewDiameterDimension()
  : Command("Drawing_NewDiameterDimension")
{
    sAppModule      = "Drawing";
    sGroup          = QT_TR_NOOP("Drawing");
    sMenuText       = QT_TR_NOOP("Insert a new diameter dimension into the drawing");
    sToolTipText    = QT_TR_NOOP("Insert a new diameter dimension feature for the selected view");
    sWhatsThis      = "Drawing_NewDiameterDimension";
    sStatusTip      = sToolTipText;
    sPixmap         = "Dimension_Diameter";
}

void CmdDrawingNewDiameterDimension::activated(int iMsg)
{
    bool result = _checkSelection(this);
    if (!result)
        return;

    std::vector<Gui::SelectionObject> selection = getSelection().getSelectionEx();
    TechDraw::DrawViewPart * objFeat = dynamic_cast<TechDraw::DrawViewPart *>(selection[0].getObject());
    const std::vector<std::string> &SubNames = selection[0].getSubNames();

    TechDraw::DrawViewDimension *dim = 0;
    std::string FeatName = getUniqueObjectName("Dimension");
    bool centerLine = false;

    std::vector<App::DocumentObject *> objs;
    std::vector<std::string> subs;

    //selected edge(s) must have valid reference to Source edge for True Dimension
    //otherwise Dimension must be Projected
    bool trueDimAllowed = _isTrueAllowed(objFeat,SubNames);

    int edgeType = _isValidSingleEdge(this,trueDimAllowed);
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
    dim->References.setValues(objs, subs);

    // make a True dimension if you can, Projected otherwise
    if (trueDimAllowed) {
        doCommand(Doc,"App.activeDocument().%s.ProjectionType = 'True'",FeatName.c_str());
    } else {
        doCommand(Doc,"App.activeDocument().%s.ProjectionType = 'Projected'",FeatName.c_str());
    }

    dim->execute();

    std::vector<App::DocumentObject*> pages = getDocument()->getObjectsOfType(TechDraw::DrawPage::getClassTypeId());
    TechDraw::DrawPage *page = dynamic_cast<TechDraw::DrawPage *>(pages.front());
    page->addView(page->getDocument()->getObject(FeatName.c_str()));

    commitCommand();

    //Horrible hack to force Tree update
    double x = objFeat->X.getValue();
    objFeat->X.setValue(x);
}


//===========================================================================
// Drawing_NewLengthDimension
//===========================================================================

DEF_STD_CMD(CmdDrawingNewLengthDimension);

CmdDrawingNewLengthDimension::CmdDrawingNewLengthDimension()
  : Command("Drawing_NewLengthDimension")
{
    sAppModule      = "Drawing";
    sGroup          = QT_TR_NOOP("Drawing");
    sMenuText       = QT_TR_NOOP("Insert a new length dimension into the drawing");
    sToolTipText    = QT_TR_NOOP("Insert a new length dimension");
    sWhatsThis      = "Drawing_NewLengthDimension";
    sStatusTip      = sToolTipText;
    sPixmap         = "Dimension_Length";
}

void CmdDrawingNewLengthDimension::activated(int iMsg)
{
    bool result = _checkSelection(this);
    if (!result)
        return;

    std::vector<Gui::SelectionObject> selection = getSelection().getSelectionEx();
    TechDraw::DrawViewPart * objFeat = dynamic_cast<TechDraw::DrawViewPart *>(selection[0].getObject());
    const std::vector<std::string> &SubNames = selection[0].getSubNames();

    TechDraw::DrawViewDimension *dim = 0;
    std::string FeatName = getUniqueObjectName("Dimension");
    std::string dimType;

    std::vector<App::DocumentObject *> objs;
    std::vector<std::string> subs;

    //selected edge(s) must have valid reference to Source edge for True Dimension
    //otherwise Dimension must be Projected
    bool trueDimAllowed = _isTrueAllowed(objFeat,SubNames);
    int edgeType = _isValidSingleEdge(this,trueDimAllowed);
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
    dim->References.setValues(objs, subs);

    doCommand(Doc, "App.activeDocument().%s.FormatSpec = '%%value%%'", FeatName.c_str());

    // make a True dimension if you can, Projected otherwise
    if (trueDimAllowed) {
        doCommand(Doc,"App.activeDocument().%s.ProjectionType = 'True'",FeatName.c_str());
    } else {
        doCommand(Doc,"App.activeDocument().%s.ProjectionType = 'Projected'",FeatName.c_str());
    }

    dim->execute();

    std::vector<App::DocumentObject*> pages = getDocument()->getObjectsOfType(TechDraw::DrawPage::getClassTypeId());
    TechDraw::DrawPage *page = dynamic_cast<TechDraw::DrawPage *>(pages.front());
    page->addView(page->getDocument()->getObject(FeatName.c_str()));

    commitCommand();

    //Horrible hack to force Tree update
    double x = objFeat->X.getValue();
    objFeat->X.setValue(x);
}

//===========================================================================
// Drawing_NewDistanceXDimension
//===========================================================================

DEF_STD_CMD(CmdDrawingNewDistanceXDimension);

CmdDrawingNewDistanceXDimension::CmdDrawingNewDistanceXDimension()
  : Command("Drawing_NewDistanceXDimension")
{
    sAppModule      = "Drawing";
    sGroup          = QT_TR_NOOP("Drawing");
    sMenuText       = QT_TR_NOOP("Insert a new horizontal dimension into the drawing");
    sToolTipText    = QT_TR_NOOP("Insert a new horizontal-distance dimension");
    sWhatsThis      = "Drawing_NewDistanceXDimension";
    sStatusTip      = sToolTipText;
    sPixmap         = "Dimension_Horizontal";
}

void CmdDrawingNewDistanceXDimension::activated(int iMsg)
{
    bool result = _checkSelection(this);
    if (!result)
        return;

    std::vector<Gui::SelectionObject> selection = getSelection().getSelectionEx();
    TechDraw::DrawViewPart * objFeat = dynamic_cast<TechDraw::DrawViewPart *>(selection[0].getObject());
    const std::vector<std::string> &SubNames = selection[0].getSubNames();

    TechDraw::DrawViewDimension *dim = 0;
    std::string FeatName = getUniqueObjectName("Dimension");
    std::string dimType;

    std::vector<App::DocumentObject *> objs;
    std::vector<std::string> subs;

    //selected edge(s) must have valid reference to Source edge for True Dimension
    //otherwise Dimension must be Projected
    bool trueDimAllowed = _isTrueAllowed(objFeat,SubNames);
    int edgeType = _isValidSingleEdge(this,trueDimAllowed);
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
    dim->References.setValues(objs, subs);

    doCommand(Doc, "App.activeDocument().%s.FormatSpec = '%%value%%'", FeatName.c_str());

    // make a True dimension if you can, Projected otherwise
    if (trueDimAllowed) {
        doCommand(Doc,"App.activeDocument().%s.ProjectionType = 'True'",FeatName.c_str());
    } else {
        doCommand(Doc,"App.activeDocument().%s.ProjectionType = 'Projected'",FeatName.c_str());
    }

    dim->execute();

    std::vector<App::DocumentObject*> pages = getDocument()->getObjectsOfType(TechDraw::DrawPage::getClassTypeId());
    TechDraw::DrawPage *page = dynamic_cast<TechDraw::DrawPage *>(pages.front());
    page->addView(page->getDocument()->getObject(FeatName.c_str()));

    commitCommand();

    //Horrible hack to force Tree update
    double x = objFeat->X.getValue();
    objFeat->X.setValue(x);
}


//===========================================================================
// Drawing_NewDistanceYDimension
//===========================================================================

DEF_STD_CMD(CmdDrawingNewDistanceYDimension);

CmdDrawingNewDistanceYDimension::CmdDrawingNewDistanceYDimension()
  : Command("Drawing_NewDistanceYDimension")
{
    sAppModule      = "Drawing";
    sGroup          = QT_TR_NOOP("Drawing");
    sMenuText       = QT_TR_NOOP("Insert a new vertical dimension into the drawing");
    sToolTipText    = QT_TR_NOOP("Insert a new vertical distance dimension");
    sWhatsThis      = "Drawing_NewDistanceYDimension";
    sStatusTip      = sToolTipText;
    sPixmap         = "Dimension_Vertical";
}

void CmdDrawingNewDistanceYDimension::activated(int iMsg)
{
    bool result = _checkSelection(this);
    if (!result)
        return;

    std::vector<Gui::SelectionObject> selection = getSelection().getSelectionEx();
    TechDraw::DrawViewPart * objFeat = dynamic_cast<TechDraw::DrawViewPart *>(selection[0].getObject());
    const std::vector<std::string> &SubNames = selection[0].getSubNames();

    TechDraw::DrawViewDimension *dim = 0;
    std::string FeatName = getUniqueObjectName("Dimension");
    std::string dimType;

    std::vector<App::DocumentObject *> objs;
    std::vector<std::string> subs;

    //selected edge(s) must have valid reference to Source edge for True Dimension
    //otherwise Dimension must be Projected
    bool trueDimAllowed = _isTrueAllowed(objFeat,SubNames);
    int edgeType = _isValidSingleEdge(this,trueDimAllowed);
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
    dim->References.setValues(objs, subs);

    doCommand(Doc, "App.activeDocument().%s.FormatSpec = '%%value%%'", FeatName.c_str());

    // make a True dimension if you can, Projected otherwise
    if (trueDimAllowed) {
        doCommand(Doc,"App.activeDocument().%s.ProjectionType = 'True'",FeatName.c_str());
    } else {
        doCommand(Doc,"App.activeDocument().%s.ProjectionType = 'Projected'",FeatName.c_str());
    }

    dim->execute();

    std::vector<App::DocumentObject*> pages = getDocument()->getObjectsOfType(TechDraw::DrawPage::getClassTypeId());
    TechDraw::DrawPage *page = dynamic_cast<TechDraw::DrawPage *>(pages.front());
    page->addView(page->getDocument()->getObject(FeatName.c_str()));

    commitCommand();

    //Horrible hack to force Tree update
    double x = objFeat->X.getValue();
    objFeat->X.setValue(x);
}


//===========================================================================
// Drawing_NewAngleDimension
//===========================================================================

DEF_STD_CMD(CmdDrawingNewAngleDimension);

CmdDrawingNewAngleDimension::CmdDrawingNewAngleDimension()
  : Command("Drawing_NewAngleDimension")
{
    sAppModule      = "Drawing";
    sGroup          = QT_TR_NOOP("Drawing");
    sMenuText       = QT_TR_NOOP("Insert a new angle dimension into the drawing");
    sToolTipText    = QT_TR_NOOP("Insert a new angle dimension");
    sWhatsThis      = "Drawing_NewAngleDimension";
    sStatusTip      = sToolTipText;
    sPixmap         = "Dimension_Angle";
}

void CmdDrawingNewAngleDimension::activated(int iMsg)
{
    bool result = _checkSelection(this);
    if (!result)
        return;

    std::vector<Gui::SelectionObject> selection = getSelection().getSelectionEx();
    TechDraw::DrawViewPart * objFeat = dynamic_cast<TechDraw::DrawViewPart *>(selection[0].getObject());
    const std::vector<std::string> &SubNames = selection[0].getSubNames();

    TechDraw::DrawViewDimension *dim = 0;
    std::string FeatName = getUniqueObjectName("Dimension");

    std::vector<App::DocumentObject *> objs;
    std::vector<std::string> subs;

    //selected edge(s) must have valid reference to Source edge for True Dimension
    //otherwise Dimension must be Projected
    bool trueDimAllowed = _isTrueAllowed(objFeat,SubNames);
    int edgeType = _isValidEdgeToEdge(this,trueDimAllowed);
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

    doCommand(Doc,"App.activeDocument().%s.FormatSpec = '%s'",FeatName.c_str()
                                                          ,"%value%\u00b0");        // \u00b0 is degree sign

    dim = dynamic_cast<TechDraw::DrawViewDimension *>(getDocument()->getObject(FeatName.c_str()));
    dim->References.setValues(objs, subs);

    // make a True dimension if you can, Projected otherwise
    if (trueDimAllowed) {
        doCommand(Doc,"App.activeDocument().%s.ProjectionType = 'True'",FeatName.c_str());
    } else {
        doCommand(Doc,"App.activeDocument().%s.ProjectionType = 'Projected'",FeatName.c_str());
    }

    dim->execute();

    std::vector<App::DocumentObject*> pages = getDocument()->getObjectsOfType(TechDraw::DrawPage::getClassTypeId());
    TechDraw::DrawPage *page = dynamic_cast<TechDraw::DrawPage *>(pages.front());
    page->addView(page->getDocument()->getObject(FeatName.c_str()));

    commitCommand();

    //Horrible hack to force Tree update
    double x = objFeat->X.getValue();
    objFeat->X.setValue(x);
}

void CreateDrawingCommandsDims(void)
{
    Gui::CommandManager &rcCmdMgr = Gui::Application::Instance->commandManager();

    rcCmdMgr.addCommand(new CmdDrawingNewDimension());
    rcCmdMgr.addCommand(new CmdDrawingNewRadiusDimension());
    rcCmdMgr.addCommand(new CmdDrawingNewDiameterDimension());
    rcCmdMgr.addCommand(new CmdDrawingNewLengthDimension());
    rcCmdMgr.addCommand(new CmdDrawingNewDistanceXDimension());
    rcCmdMgr.addCommand(new CmdDrawingNewDistanceYDimension());
    rcCmdMgr.addCommand(new CmdDrawingNewAngleDimension());
}

//===========================================================================
// Selection Validation Helpers
//===========================================================================

//! common checks of Selection for Dimension commands
bool _checkSelection(Gui::Command* cmd) {
    std::vector<Gui::SelectionObject> selection = cmd->getSelection().getSelectionEx();
    if (selection.size() == 0) {
        QMessageBox::warning(Gui::getMainWindow(), QObject::tr("Incorrect selection"),
                             QObject::tr("Select an object first"));
        return false;
    }

    TechDraw::DrawViewPart * objFeat = dynamic_cast<TechDraw::DrawViewPart *>(selection[0].getObject());
    if(!objFeat) {
        QMessageBox::warning(Gui::getMainWindow(), QObject::tr("Incorrect selection"),
                             QObject::tr("No Feature in selection"));
        return false;
    }

    const std::vector<std::string> &SubNames = selection[0].getSubNames();
    if (SubNames.size() != 1 && SubNames.size() != 2){
        QMessageBox::warning(Gui::getMainWindow(), QObject::tr("Incorrect selection"),
            QObject::tr("Wrong number of objects selected"));
        return false;
    }

    std::vector<App::DocumentObject*> pages = cmd->getDocument()->getObjectsOfType(TechDraw::DrawPage::getClassTypeId());
    if (pages.empty()){
        QMessageBox::warning(Gui::getMainWindow(), QObject::tr("Incorrect selection"),
            QObject::tr("Create a page to insert."));
        return false;
    }
    return true;
}

//! verify that Selection contains a valid Geometry for a single Edge Dimension (True or Projected)
int _isValidSingleEdge(Gui::Command* cmd, bool trueDim) {
    int edgeType = isInvalid;
    std::vector<Gui::SelectionObject> selection = cmd->getSelection().getSelectionEx();
    TechDraw::DrawViewPart * objFeat = dynamic_cast<TechDraw::DrawViewPart *>(selection[0].getObject());
    const std::vector<std::string> &SubNames = selection[0].getSubNames();
    if (SubNames.size() == 1) {                                                 //only 1 subshape selected
        if (DrawUtil::getGeomTypeFromName(SubNames[0]) == "Edge") {                                //the Name starts with "Edge"
            int GeoId = DrawUtil::getIndexFromName(SubNames[0]);
            DrawingGeometry::BaseGeom* geom = NULL;
            if (trueDim) {
                int ref = objFeat->getEdgeRefByIndex(GeoId);
                geom = objFeat->getCompleteEdge(ref);                  //project edge onto its shape to get 2D geom
            } else {
                geom = objFeat->getProjEdgeByIndex(GeoId);
            }
            if (!geom) {
                Base::Console().Error("Logic Error: no geometry for GeoId: %d\n",GeoId);
                return isInvalid;
            }

            if(geom->geomType == DrawingGeometry::GENERIC) {
                DrawingGeometry::Generic* gen1 = static_cast<DrawingGeometry::Generic *>(geom);
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
            } else if (geom->geomType == DrawingGeometry::CIRCLE ||
                       geom->geomType == DrawingGeometry::ELLIPSE ||
                       geom->geomType == DrawingGeometry::ARCOFCIRCLE ||
                       geom->geomType == DrawingGeometry::ARCOFELLIPSE ) {
                edgeType = isCircle;
            } else if (geom->geomType == DrawingGeometry::BSPLINE) {
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
    const std::vector<std::string> &SubNames = selection[0].getSubNames();
    if(SubNames.size() == 2) {                                         //there are 2
        if (DrawUtil::getGeomTypeFromName(SubNames[0]) == "Vertex" &&                    //they both start with "Vertex"
            DrawUtil::getGeomTypeFromName(SubNames[1]) == "Vertex") {
            return true;
        }
    }
    return false;
}

//! verify that the Selection contains valid geometries for an Edge to Edge Dimension
int _isValidEdgeToEdge(Gui::Command* cmd, bool trueDim) {
//TODO: can the edges be in 2 different features??
    int edgeType = isInvalid;
    std::vector<Gui::SelectionObject> selection = cmd->getSelection().getSelectionEx();
    TechDraw::DrawViewPart* objFeat0 = dynamic_cast<TechDraw::DrawViewPart *>(selection[0].getObject());
    //TechDraw::DrawViewPart* objFeat1 = dynamic_cast<TechDraw::DrawViewPart *>(selection[1].getObject());
    const std::vector<std::string> &SubNames = selection[0].getSubNames();
    if(SubNames.size() == 2) {                                         //there are 2
        if (DrawUtil::getGeomTypeFromName(SubNames[0]) == "Edge" &&                      //they both start with "Edge"
            DrawUtil::getGeomTypeFromName(SubNames[1]) == "Edge") {
            int GeoId0 = DrawUtil::getIndexFromName(SubNames[0]);
            int GeoId1 = DrawUtil::getIndexFromName(SubNames[1]);
            DrawingGeometry::BaseGeom* geom0 = NULL;
            DrawingGeometry::BaseGeom* geom1 = NULL;
            if (trueDim) {
                int ref0 = objFeat0->getEdgeRefByIndex(GeoId0);
                int ref1 = objFeat0->getEdgeRefByIndex(GeoId1);
                geom0 = objFeat0->getCompleteEdge(ref0);
                geom1 = objFeat0->getCompleteEdge(ref1);
            } else {
                geom0 = objFeat0->getProjEdgeByIndex(GeoId0);
                geom1 = objFeat0->getProjEdgeByIndex(GeoId1);
            }
            if ((!geom0) || (!geom1)) {
                Base::Console().Error("Logic Error: no geometry for GeoId: %d or GeoId: %d\n",GeoId0,GeoId1);
                return isInvalid;
            }

            if(geom0->geomType == DrawingGeometry::GENERIC &&
               geom1->geomType == DrawingGeometry::GENERIC) {
                DrawingGeometry::Generic *gen0 = static_cast<DrawingGeometry::Generic *>(geom0);
                DrawingGeometry::Generic *gen1 = static_cast<DrawingGeometry::Generic *>(geom1);
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

//! verify that each SubName has a corresponding Edge geometry in objFeat->Source
bool _isTrueAllowed(TechDraw::DrawViewPart* objFeat, const std::vector<std::string> &SubNames)
{
    std::vector<std::string>::const_iterator it = SubNames.begin();
    bool trueDimAllowed = true;
    for (; it != SubNames.end(); it++) {
        int idx = DrawUtil::getIndexFromName((*it));
        int ref = objFeat->getEdgeRefByIndex(idx);
        if (ref < 0) {
            trueDimAllowed = false;
        }
    }
    return trueDimAllowed;
}
