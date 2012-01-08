/***************************************************************************
 *   Copyright (c) J�rgen Riegel          (juergen.riegel@web.de) 2002     *
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
# include <sstream>
# include <QString>
# include <QDir>
# include <QFileInfo>
# include <QLineEdit>
# include <QPointer>
# include <Standard_math.hxx>
# include <TopoDS_Shape.hxx>
# include <Inventor/events/SoMouseButtonEvent.h>
#endif

#include <Base/Console.h>
#include <Base/Exception.h>
#include <App/Document.h>
#include <Gui/Application.h>
#include <Gui/BitmapFactory.h>
#include <Gui/Command.h>
#include <Gui/Control.h>
#include <Gui/Document.h>
#include <Gui/FileDialog.h>
#include <Gui/MainWindow.h>
#include <Gui/Selection.h>
#include <Gui/View3DInventor.h>
#include <Gui/View3DInventorViewer.h>

#include "../App/PartFeature.h"
#include "DlgPartImportStepImp.h"
#include "DlgBooleanOperation.h"
#include "DlgExtrusion.h"
#include "DlgRevolution.h"
#include "DlgFilletEdges.h"
#include "DlgPrimitives.h"
#include "CrossSections.h"
#include "Mirroring.h"
#include "ViewProvider.h"
#include "TaskShapeBuilder.h"
#include "TaskLoft.h"


//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

//===========================================================================
// Part_PickCurveNet
//===========================================================================
DEF_STD_CMD(CmdPartPickCurveNet);

CmdPartPickCurveNet::CmdPartPickCurveNet()
  :Command("Part_PickCurveNet")
{
    sAppModule    = "Part";
    sGroup        = QT_TR_NOOP("Part");
    sMenuText     = QT_TR_NOOP("Pick curve network");
    sToolTipText  = QT_TR_NOOP("Pick a curve network");
    sWhatsThis    = sToolTipText;
    sStatusTip    = sToolTipText;
    sPixmap       = "Test1";
}

void CmdPartPickCurveNet::activated(int iMsg)
{

}

//===========================================================================
// Part_NewDoc
//===========================================================================
DEF_STD_CMD(CmdPartNewDoc);

CmdPartNewDoc::CmdPartNewDoc()
  :Command("Part_NewDoc")
{
    sAppModule    = "Part";
    sGroup        = "Part";
    sMenuText     = "New document";
    sToolTipText  = "Create an empty part document";
    sWhatsThis    = sToolTipText;
    sStatusTip    = sToolTipText;
    sPixmap       = "New";
}

void CmdPartNewDoc::activated(int iMsg)
{
    doCommand(Doc,"d = App.New()");
    updateActive();
}

//===========================================================================
// Part_Box2
//===========================================================================
DEF_STD_CMD_A(CmdPartBox2);

CmdPartBox2::CmdPartBox2()
  :Command("Part_Box2")
{
    sAppModule    = "Part";
    sGroup        = QT_TR_NOOP("Part");
    sMenuText     = QT_TR_NOOP("Box fix 1");
    sToolTipText  = QT_TR_NOOP("Create a box solid without dialog");
    sWhatsThis    = "Part_Box2";
    sStatusTip    = sToolTipText;
    sPixmap       = "Part_Box";
}


void CmdPartBox2::activated(int iMsg)
{
    openCommand("Part Box Create");
    doCommand(Doc,"from FreeCAD import Base");
    doCommand(Doc,"import Part");
    doCommand(Doc,"__fb__ = App.ActiveDocument.addObject(\"Part::Box\",\"PartBox\")");
    doCommand(Doc,"__fb__.Location = Base.Vector(0.0,0.0,0.0)");
    doCommand(Doc,"__fb__.Length = 100.0");
    doCommand(Doc,"__fb__.Width = 100.0");
    doCommand(Doc,"__fb__.Height = 100.0");
    doCommand(Doc,"del __fb__");
    commitCommand();
    updateActive();
}

bool CmdPartBox2::isActive(void)
{
    if (getActiveGuiDocument())
        return true;
    else
        return false;
}

//===========================================================================
// Part_Box3
//===========================================================================
DEF_STD_CMD_A(CmdPartBox3);

CmdPartBox3::CmdPartBox3()
  :Command("Part_Box3")
{
    sAppModule    = "Part";
    sGroup        = QT_TR_NOOP("Part");
    sMenuText     = QT_TR_NOOP("Box fix 2");
    sToolTipText  = QT_TR_NOOP("Create a box solid without dialog");
    sWhatsThis    = "Part_Box3";
    sStatusTip    = sToolTipText;
    sPixmap       = "Part_Box";
}

void CmdPartBox3::activated(int iMsg)
{
    openCommand("Part Box Create");
    doCommand(Doc,"from FreeCAD import Base");
    doCommand(Doc,"import Part");
    doCommand(Doc,"__fb__ = App.ActiveDocument.addObject(\"Part::Box\",\"PartBox\")");
    doCommand(Doc,"__fb__.Location = Base.Vector(50.0,50.0,50.0)");
    doCommand(Doc,"__fb__.Length = 100.0");
    doCommand(Doc,"__fb__.Width = 100.0");
    doCommand(Doc,"__fb__.Height = 100.0");
    doCommand(Doc,"del __fb__");
    commitCommand();
    updateActive();
}

bool CmdPartBox3::isActive(void)
{
    if (getActiveGuiDocument())
        return true;
    else
        return false;
}

//===========================================================================
// Part_Primitives
//===========================================================================
DEF_STD_CMD_A(CmdPartPrimitives);

CmdPartPrimitives::CmdPartPrimitives()
  :Command("Part_Primitives")
{
    sAppModule    = "Part";
    sGroup        = QT_TR_NOOP("Part");
    sMenuText     = QT_TR_NOOP("Create primitives...");
    sToolTipText  = QT_TR_NOOP("Creation of parametrized geometric primitives");
    sWhatsThis    = "Part_Primitives";
    sStatusTip    = sToolTipText;
    sPixmap       = "Part_CreatePrimitives";
}

void CmdPartPrimitives::activated(int iMsg)
{
    PartGui::TaskPrimitives* dlg = new PartGui::TaskPrimitives();
    Gui::Control().showDialog(dlg);
}

bool CmdPartPrimitives::isActive(void)
{
    return (hasActiveDocument() && !Gui::Control().activeDialog());
}

//===========================================================================
// Part_Cut
//===========================================================================
DEF_STD_CMD_A(CmdPartCut);

CmdPartCut::CmdPartCut()
  :Command("Part_Cut")
{
    sAppModule    = "Part";
    sGroup        = QT_TR_NOOP("Part");
    sMenuText     = QT_TR_NOOP("Cut");
    sToolTipText  = QT_TR_NOOP("Make a cut of two shapes");
    sWhatsThis    = "Part_Cut";
    sStatusTip    = sToolTipText;
    sPixmap       = "Part_Cut";
}

void CmdPartCut::activated(int iMsg)
{
    unsigned int n = getSelection().countObjectsOfType(Part::Feature::getClassTypeId());
    if (n != 2) {
        QMessageBox::warning(Gui::getMainWindow(), QObject::tr("Wrong selection"),
            QObject::tr("Select two shapes please."));
        return;
    }

    std::vector<Gui::SelectionSingleton::SelObj> Sel = getSelection().getSelection();

    std::string FeatName = getUniqueObjectName("Cut");
    std::string BaseName  = Sel[0].FeatName;
    std::string ToolName  = Sel[1].FeatName;

    openCommand("Part Cut");
    doCommand(Doc,"App.activeDocument().addObject(\"Part::Cut\",\"%s\")",FeatName.c_str());
    doCommand(Doc,"App.activeDocument().%s.Base = App.activeDocument().%s",FeatName.c_str(),BaseName.c_str());
    doCommand(Doc,"App.activeDocument().%s.Tool = App.activeDocument().%s",FeatName.c_str(),ToolName.c_str());
    doCommand(Gui,"Gui.activeDocument().hide('%s')",BaseName.c_str());
    doCommand(Gui,"Gui.activeDocument().hide('%s')",ToolName.c_str());
    copyVisual(FeatName.c_str(), "ShapeColor", BaseName.c_str());
    copyVisual(FeatName.c_str(), "DisplayMode", BaseName.c_str());
    updateActive();
    commitCommand();
}

bool CmdPartCut::isActive(void)
{
    return getSelection().countObjectsOfType(Part::Feature::getClassTypeId())==2;
}

//===========================================================================
// Part_Common
//===========================================================================
DEF_STD_CMD_A(CmdPartCommon);

CmdPartCommon::CmdPartCommon()
  :Command("Part_Common")
{
    sAppModule    = "Part";
    sGroup        = QT_TR_NOOP("Part");
    sMenuText     = QT_TR_NOOP("Intersection");
    sToolTipText  = QT_TR_NOOP("Make an intersection of two shapes");
    sWhatsThis    = "Part_Common";
    sStatusTip    = sToolTipText;
    sPixmap       = "Part_Common";
}

void CmdPartCommon::activated(int iMsg)
{
    unsigned int n = getSelection().countObjectsOfType(Part::Feature::getClassTypeId());
    if (n < 2) {
        QMessageBox::warning(Gui::getMainWindow(), QObject::tr("Wrong selection"),
            QObject::tr("Select two shapes or more, please."));
        return;
    }

    std::string FeatName = getUniqueObjectName("Common");

    std::vector<Gui::SelectionSingleton::SelObj> Sel = getSelection().getSelection();
    std::stringstream str;
    std::vector<std::string> tempSelNames;
    str << "App.activeDocument()." << FeatName << ".Shapes = [";
    for (std::vector<Gui::SelectionSingleton::SelObj>::iterator it = Sel.begin(); it != Sel.end(); ++it){
        str << "App.activeDocument()." << it->FeatName << ",";
        tempSelNames.push_back(it->FeatName);
    }
    str << "]";

    openCommand("Common");
    doCommand(Doc,"App.activeDocument().addObject(\"Part::MultiCommon\",\"%s\")",FeatName.c_str());
    runCommand(Doc,str.str().c_str());
    for (std::vector<std::string>::iterator it = tempSelNames.begin(); it != tempSelNames.end(); ++it)
        doCommand(Gui,"Gui.activeDocument().%s.Visibility=False",it->c_str());
    copyVisual(FeatName.c_str(), "ShapeColor", tempSelNames.front().c_str());
    copyVisual(FeatName.c_str(), "DisplayMode", tempSelNames.front().c_str());
    updateActive();
    commitCommand();
}

bool CmdPartCommon::isActive(void)
{
    return getSelection().countObjectsOfType(Part::Feature::getClassTypeId())>=2;
}

//===========================================================================
// Part_Fuse
//===========================================================================
DEF_STD_CMD_A(CmdPartFuse);

CmdPartFuse::CmdPartFuse()
  :Command("Part_Fuse")
{
    sAppModule    = "Part";
    sGroup        = QT_TR_NOOP("Part");
    sMenuText     = QT_TR_NOOP("Union");
    sToolTipText  = QT_TR_NOOP("Make a union of several shapes");
    sWhatsThis    = "Part_Fuse";
    sStatusTip    = sToolTipText;
    sPixmap       = "Part_Fuse";
}

void CmdPartFuse::activated(int iMsg)
{
    unsigned int n = getSelection().countObjectsOfType(Part::Feature::getClassTypeId());
    if (n < 2) {
        QMessageBox::warning(Gui::getMainWindow(), QObject::tr("Wrong selection"),
            QObject::tr("Select two shapes or more, please."));
        return;
    }

    std::string FeatName = getUniqueObjectName("Fusion");

    std::vector<Gui::SelectionSingleton::SelObj> Sel = getSelection().getSelection();
    std::stringstream str;
    std::vector<std::string> tempSelNames;
    str << "App.activeDocument()." << FeatName << ".Shapes = [";
    for (std::vector<Gui::SelectionSingleton::SelObj>::iterator it = Sel.begin(); it != Sel.end(); ++it){
        str << "App.activeDocument()." << it->FeatName << ",";
        tempSelNames.push_back(it->FeatName);
    }
    str << "]";

    openCommand("Fusion");
    doCommand(Doc,"App.activeDocument().addObject(\"Part::MultiFuse\",\"%s\")",FeatName.c_str());
    runCommand(Doc,str.str().c_str());
    for (std::vector<std::string>::iterator it = tempSelNames.begin(); it != tempSelNames.end(); ++it)
        doCommand(Gui,"Gui.activeDocument().%s.Visibility=False",it->c_str());
    copyVisual(FeatName.c_str(), "ShapeColor", tempSelNames.front().c_str());
    copyVisual(FeatName.c_str(), "DisplayMode", tempSelNames.front().c_str());
    updateActive();
    commitCommand();
}

bool CmdPartFuse::isActive(void)
{
    return getSelection().countObjectsOfType(Part::Feature::getClassTypeId())>=2;
}

//===========================================================================
// Part_Section
//===========================================================================
DEF_STD_CMD_A(CmdPartSection);

CmdPartSection::CmdPartSection()
  :Command("Part_Section")
{
    sAppModule    = "Part";
    sGroup        = QT_TR_NOOP("Part");
    sMenuText     = QT_TR_NOOP("Section");
    sToolTipText  = QT_TR_NOOP("Make a section of two shapes");
    sWhatsThis    = "Part_Section";
    sStatusTip    = sToolTipText;
    sPixmap       = "Part_Section";
}


void CmdPartSection::activated(int iMsg)
{
    unsigned int n = getSelection().countObjectsOfType(Part::Feature::getClassTypeId());
    if (n != 2) {
        QMessageBox::warning(Gui::getMainWindow(), QObject::tr("Wrong selection"),
            QObject::tr("Select two shapes please."));
        return;
    }

    std::vector<Gui::SelectionSingleton::SelObj> Sel = getSelection().getSelection();
    std::string FeatName = getUniqueObjectName("Section");
    std::string BaseName  = Sel[0].FeatName;
    std::string ToolName  = Sel[1].FeatName;

    openCommand("Section");
    doCommand(Doc,"App.activeDocument().addObject(\"Part::Section\",\"%s\")",FeatName.c_str());
    doCommand(Doc,"App.activeDocument().%s.Base = App.activeDocument().%s",FeatName.c_str(),BaseName.c_str());
    doCommand(Doc,"App.activeDocument().%s.Tool = App.activeDocument().%s",FeatName.c_str(),ToolName.c_str());
    doCommand(Gui,"Gui.activeDocument().hide('%s')",BaseName.c_str());
    doCommand(Gui,"Gui.activeDocument().hide('%s')",ToolName.c_str());
    doCommand(Gui,"Gui.activeDocument().%s.LineColor = Gui.activeDocument().%s.ShapeColor", FeatName.c_str(),BaseName.c_str());
    updateActive();
    commitCommand();
}

bool CmdPartSection::isActive(void)
{
    return getSelection().countObjectsOfType(Part::Feature::getClassTypeId())==2;
}

//===========================================================================
// CmdPartImport
//===========================================================================
DEF_STD_CMD_A(CmdPartImport);

CmdPartImport::CmdPartImport()
  :Command("Part_Import")
{
    sAppModule    = "Part";
    sGroup        = QT_TR_NOOP("Part");
    sMenuText     = QT_TR_NOOP("Import CAD...");
    sToolTipText  = QT_TR_NOOP("Imports a CAD file");
    sWhatsThis    = "Part_Import";
    sStatusTip    = sToolTipText;
    sPixmap       = "Part_Import";
}


void CmdPartImport::activated(int iMsg)
{
    QStringList filter;
    filter << QObject::tr("All CAD Files (*.stp *.step *.igs *.iges *.brp *.brep)");
    filter << QObject::tr("STEP (*.stp *.step)");
    filter << QObject::tr("IGES (*.igs *.iges)");
    filter << QObject::tr("BREP (*.brp *.brep)");
    filter << QObject::tr("All Files (*.*)");

    QString fn = Gui::FileDialog::getOpenFileName(Gui::getMainWindow(), QString(), QString(), filter.join(QLatin1String(";;")));
    if (!fn.isEmpty()) {
        App::Document* pDoc = getDocument();
        if (!pDoc) return; // no document
        openCommand("Import Part");
        QString ext = QFileInfo(fn).suffix().toLower();
        if (ext == QLatin1String("step") || 
            ext == QLatin1String("stp")  ||
            ext == QLatin1String("iges") ||
            ext == QLatin1String("igs")) {
            doCommand(Doc, "import ImportGui");
            doCommand(Doc, "ImportGui.insert(\"%s\",\"%s\")", (const char*)fn.toUtf8(), pDoc->getName());
        }
        else {
            doCommand(Doc, "import Part");
            doCommand(Doc, "Part.insert(\"%s\",\"%s\")", (const char*)fn.toUtf8(), pDoc->getName());
        }
        commitCommand();
    }
}

bool CmdPartImport::isActive(void)
{
    if (getActiveGuiDocument())
        return true;
    else
        return false;
}

//===========================================================================
// CmdPartExport
//===========================================================================
DEF_STD_CMD_A(CmdPartExport);

CmdPartExport::CmdPartExport()
  : Command("Part_Export")
{
    sAppModule    = "Part";
    sGroup        = QT_TR_NOOP("Part");
    sMenuText     = QT_TR_NOOP("Export CAD...");
    sToolTipText  = QT_TR_NOOP("Exports to a CAD file");
    sWhatsThis    = "Part_Export";
    sStatusTip    = sToolTipText;
  //sPixmap       = "Part_Export";
}

void CmdPartExport::activated(int iMsg)
{
    QStringList filter;
    filter << QObject::tr("All CAD Files (*.stp *.step *.igs *.iges *.brp *.brep)");
    filter << QObject::tr("STEP (*.stp *.step)");
    filter << QObject::tr("IGES (*.igs *.iges)");
    filter << QObject::tr("BREP (*.brp *.brep)");
    filter << QObject::tr("All Files (*.*)");

    QString fn = Gui::FileDialog::getSaveFileName(Gui::getMainWindow(), QString(), QString(), filter.join(QLatin1String(";;")));
    if (!fn.isEmpty()) {
        App::Document* pDoc = getDocument();
        if (!pDoc) return; // no document
        openCommand("Import Part");
        QString ext = QFileInfo(fn).suffix().toLower();
        if (ext == QLatin1String("step") || 
            ext == QLatin1String("stp")  ||
            ext == QLatin1String("iges") ||
            ext == QLatin1String("igs")) {
            Gui::Application::Instance->exportTo((const char*)fn.toUtf8(),pDoc->getName(),"ImportGui");
        }
        else {
            Gui::Application::Instance->exportTo((const char*)fn.toUtf8(),pDoc->getName(),"Part");
        }
        commitCommand();
    }
}

bool CmdPartExport::isActive(void)
{
    return Gui::Selection().countObjectsOfType(Part::Feature::getClassTypeId()) > 0;
}

//===========================================================================
// PartImportCurveNet
//===========================================================================
DEF_STD_CMD_A(CmdPartImportCurveNet);

CmdPartImportCurveNet::CmdPartImportCurveNet()
  :Command("Part_ImportCurveNet")
{
    sAppModule  = "Part";
    sGroup      = QT_TR_NOOP("Part");
    sMenuText   = QT_TR_NOOP("Import curve network...");
    sToolTipText= QT_TR_NOOP("Import a curve network");
    sWhatsThis  = "Part_ImportCurveNet";
    sStatusTip  = sToolTipText;
    sPixmap     = "Part_Box";
}

void CmdPartImportCurveNet::activated(int iMsg)
{
    QStringList filter;
    filter << QObject::tr("All CAD Files (*.stp *.step *.igs *.iges *.brp *.brep)");
    filter << QObject::tr("STEP (*.stp *.step)");
    filter << QObject::tr("IGES (*.igs *.iges)");
    filter << QObject::tr("BREP (*.brp *.brep)");
    filter << QObject::tr("All Files (*.*)");

    QString fn = Gui::FileDialog::getOpenFileName(Gui::getMainWindow(), QString(), QString(), filter.join(QLatin1String(";;")));
    if (!fn.isEmpty()) {
        QFileInfo fi; fi.setFile(fn);
        openCommand("Part Import Curve Net");
        doCommand(Doc,"f = App.activeDocument().addObject(\"Part::CurveNet\",\"%s\")", (const char*)fi.baseName().toAscii());
        doCommand(Doc,"f.FileName = \"%s\"",(const char*)fn.toAscii());
        commitCommand();
        updateActive();
    }
}

bool CmdPartImportCurveNet::isActive(void)
{
    if (getActiveGuiDocument())
        return true;
    else
        return false;
}

//===========================================================================
// Part_MakeSolid
//===========================================================================
DEF_STD_CMD_A(CmdPartMakeSolid);

CmdPartMakeSolid::CmdPartMakeSolid()
  :Command("Part_MakeSolid")
{
    sAppModule    = "Part";
    sGroup        = QT_TR_NOOP("Part");
    sMenuText     = QT_TR_NOOP("Convert to solid");
    sToolTipText  = QT_TR_NOOP("Create solid from a shell or compound");
    sWhatsThis    = "Part_MakeSolid";
    sStatusTip    = sToolTipText;
}

void CmdPartMakeSolid::activated(int iMsg)
{
    std::vector<App::DocumentObject*> objs = Gui::Selection().getObjectsOfType
        (Part::Feature::getClassTypeId());
    doCommand(Doc, "import Part");
    for (std::vector<App::DocumentObject*>::iterator it = objs.begin(); it != objs.end(); ++it) {
        const TopoDS_Shape& shape = static_cast<Part::Feature*>(*it)->Shape.getValue();
        if (!shape.IsNull()) {
            TopAbs_ShapeEnum type = shape.ShapeType();
            QString str;
            if (type == TopAbs_SOLID) {
                Base::Console().Message("%s is ignored because it is already a solid.\n",
                    (*it)->Label.getValue());
            }
            else if (type == TopAbs_COMPOUND || type == TopAbs_COMPSOLID) {
                str = QString::fromAscii(
                    "__s__=App.ActiveDocument.%1.Shape.Faces\n"
                    "__s__=Part.Solid(Part.Shell(__s__))\n"
                    "__o__=App.ActiveDocument.addObject(\"Part::Feature\",\"%1_solid\")\n"
                    "__o__.Label=\"%2 (Solid)\"\n"
                    "__o__.Shape=__s__\n"
                    "del __s__, __o__"
                    )
                    .arg(QLatin1String((*it)->getNameInDocument()))
                    .arg(QLatin1String((*it)->Label.getValue()));
            }
            else if (type == TopAbs_SHELL) {
                str = QString::fromAscii(
                    "__s__=App.ActiveDocument.%1.Shape\n"
                    "__s__=Part.Solid(__s__)\n"
                    "__o__=App.ActiveDocument.addObject(\"Part::Feature\",\"%1_solid\")\n"
                    "__o__.Label=\"%2 (Solid)\"\n"
                    "__o__.Shape=__s__\n"
                    "del __s__, __o__"
                    )
                    .arg(QLatin1String((*it)->getNameInDocument()))
                    .arg(QLatin1String((*it)->Label.getValue()));
            }
            else {
                Base::Console().Message("%s is ignored because it is neither a shell nor a compound.\n",
                    (*it)->Label.getValue());
            }

            try {
                if (!str.isEmpty())
                    doCommand(Doc, (const char*)str.toAscii());
            }
            catch (const Base::Exception& e) {
                Base::Console().Error("Cannot convert %s because %s.\n",
                    (*it)->Label.getValue(), e.what());
            }
        }
    }
}

bool CmdPartMakeSolid::isActive(void)
{
    return Gui::Selection().countObjectsOfType
        (Part::Feature::getClassTypeId()) > 0;
}

//===========================================================================
// Part_ReverseShape
//===========================================================================
DEF_STD_CMD_A(CmdPartReverseShape);

CmdPartReverseShape::CmdPartReverseShape()
  :Command("Part_ReverseShape")
{
    sAppModule    = "Part";
    sGroup        = QT_TR_NOOP("Part");
    sMenuText     = QT_TR_NOOP("Reverse shapes");
    sToolTipText  = QT_TR_NOOP("Reverse orientation of shapes");
    sWhatsThis    = "Part_ReverseShape";
    sStatusTip    = sToolTipText;
}

void CmdPartReverseShape::activated(int iMsg)
{
    std::vector<App::DocumentObject*> objs = Gui::Selection().getObjectsOfType
        (Part::Feature::getClassTypeId());
    doCommand(Doc, "import Part");
    for (std::vector<App::DocumentObject*>::iterator it = objs.begin(); it != objs.end(); ++it) {
        const TopoDS_Shape& shape = static_cast<Part::Feature*>(*it)->Shape.getValue();
        if (!shape.IsNull()) {
            QString str = QString::fromAscii(
                "__s__=App.ActiveDocument.%1.Shape.copy()\n"
                "__s__.reverse()\n"
                "__o__=App.ActiveDocument.addObject(\"Part::Feature\",\"%1_rev\")\n"
                "__o__.Label=\"%2 (Rev)\"\n"
                "__o__.Shape=__s__\n"
                "del __s__, __o__"
                )
                .arg(QLatin1String((*it)->getNameInDocument()))
                .arg(QLatin1String((*it)->Label.getValue()));

            try {
                if (!str.isEmpty())
                    doCommand(Doc, (const char*)str.toAscii());
            }
            catch (const Base::Exception& e) {
                Base::Console().Error("Cannot convert %s because %s.\n",
                    (*it)->Label.getValue(), e.what());
            }
        }
    }
}

bool CmdPartReverseShape::isActive(void)
{
    return Gui::Selection().countObjectsOfType
        (Part::Feature::getClassTypeId()) > 0;
}

//===========================================================================
// Part_Boolean
//===========================================================================
DEF_STD_CMD_A(CmdPartBoolean);

CmdPartBoolean::CmdPartBoolean()
  :Command("Part_Boolean")
{
    sAppModule    = "Part";
    sGroup        = QT_TR_NOOP("Part");
    sMenuText     = QT_TR_NOOP("Boolean...");
    sToolTipText  = QT_TR_NOOP("Run a boolean operation with two shapes selected");
    sWhatsThis    = "Part_Boolean";
    sStatusTip    = sToolTipText;
    sPixmap       = "Part_Booleans";
}

void CmdPartBoolean::activated(int iMsg)
{
    Gui::TaskView::TaskDialog* dlg = Gui::Control().activeDialog();
    if (!dlg)
        dlg = new PartGui::TaskBooleanOperation();
    Gui::Control().showDialog(dlg);
}

bool CmdPartBoolean::isActive(void)
{
    return (hasActiveDocument() && !Gui::Control().activeDialog());
}

//===========================================================================
// Part_Extrude
//===========================================================================
DEF_STD_CMD_A(CmdPartExtrude);

CmdPartExtrude::CmdPartExtrude()
  :Command("Part_Extrude")
{
    sAppModule    = "Part";
    sGroup        = QT_TR_NOOP("Part");
    sMenuText     = QT_TR_NOOP("Extrude...");
    sToolTipText  = QT_TR_NOOP("Extrude a selected sketch");
    sWhatsThis    = sToolTipText;
    sStatusTip    = sToolTipText;
    sPixmap       = "Part_Extrude";
}

void CmdPartExtrude::activated(int iMsg)
{
    Gui::Control().showDialog(new PartGui::TaskExtrusion());
}

bool CmdPartExtrude::isActive(void)
{
    return (hasActiveDocument() && !Gui::Control().activeDialog());
}

//===========================================================================
// Part_Revolve
//===========================================================================
DEF_STD_CMD_A(CmdPartRevolve);

CmdPartRevolve::CmdPartRevolve()
  :Command("Part_Revolve")
{
    sAppModule    = "Part";
    sGroup        = QT_TR_NOOP("Part");
    sMenuText     = QT_TR_NOOP("Revolve...");
    sToolTipText  = QT_TR_NOOP("Revolve a selected shape");
    sWhatsThis    = sToolTipText;
    sStatusTip    = sToolTipText;
    sPixmap       = "Part_Revolve";
}

void CmdPartRevolve::activated(int iMsg)
{
    Gui::Control().showDialog(new PartGui::TaskRevolution());
}

bool CmdPartRevolve::isActive(void)
{
    return (hasActiveDocument() && !Gui::Control().activeDialog());
}

//===========================================================================
// Part_Fillet
//===========================================================================
DEF_STD_CMD_A(CmdPartFillet);

CmdPartFillet::CmdPartFillet()
  :Command("Part_Fillet")
{
    sAppModule    = "Part";
    sGroup        = QT_TR_NOOP("Part");
    sMenuText     = QT_TR_NOOP("Fillet...");
    sToolTipText  = QT_TR_NOOP("Fillet the selected edges of a shape");
    sWhatsThis    = sToolTipText;
    sStatusTip    = sToolTipText;
    sPixmap       = "Part_Fillet";
}

void CmdPartFillet::activated(int iMsg)
{
    Gui::Control().showDialog(new PartGui::TaskFilletEdges(0));
}

bool CmdPartFillet::isActive(void)
{
    return (hasActiveDocument() && !Gui::Control().activeDialog());
}

//===========================================================================
// Part_Mirror
//===========================================================================
DEF_STD_CMD_A(CmdPartMirror);

CmdPartMirror::CmdPartMirror()
  :Command("Part_Mirror")
{
    sAppModule    = "Part";
    sGroup        = QT_TR_NOOP("Part");
    sMenuText     = QT_TR_NOOP("Mirroring...");
    sToolTipText  = QT_TR_NOOP("Mirroring a selected shape");
    sWhatsThis    = sToolTipText;
    sStatusTip    = sToolTipText;
    sPixmap       = "Part_MirrorPNG";
}

void CmdPartMirror::activated(int iMsg)
{
    Gui::Control().showDialog(new PartGui::TaskMirroring());
}

bool CmdPartMirror::isActive(void)
{
    return (hasActiveDocument() && !Gui::Control().activeDialog());
}

//===========================================================================
// Part_CrossSections
//===========================================================================
DEF_STD_CMD_A(CmdPartCrossSections);

CmdPartCrossSections::CmdPartCrossSections()
  :Command("Part_CrossSections")
{
    sAppModule    = "Part";
    sGroup        = QT_TR_NOOP("Part");
    sMenuText     = QT_TR_NOOP("Cross-sections...");
    sToolTipText  = QT_TR_NOOP("Cross-sections");
    sWhatsThis    = "Part_CrossSections";
    sStatusTip    = sToolTipText;
//  sPixmap       = "Part_Booleans";
}

void CmdPartCrossSections::activated(int iMsg)
{
    Gui::TaskView::TaskDialog* dlg = Gui::Control().activeDialog();
    if (!dlg) {
        std::vector<App::DocumentObject*> obj = Gui::Selection().getObjectsOfType
            (Part::Feature::getClassTypeId());
        Base::BoundBox3d bbox;
        for (std::vector<App::DocumentObject*>::iterator it = obj.begin(); it != obj.end(); ++it) {
            bbox.Add(static_cast<Part::Feature*>(*it)->Shape.getBoundingBox());
        }
        dlg = new PartGui::TaskCrossSections(bbox);
    }
    Gui::Control().showDialog(dlg);
}

bool CmdPartCrossSections::isActive(void)
{
    return (Gui::Selection().countObjectsOfType(Part::Feature::getClassTypeId()) > 0 &&
            !Gui::Control().activeDialog());
}

//--------------------------------------------------------------------------------------

DEF_STD_CMD_A(CmdPartBuilder);

CmdPartBuilder::CmdPartBuilder()
  :Command("Part_Builder")
{
    sAppModule    = "Part";
    sGroup        = QT_TR_NOOP("Part");
    sMenuText     = QT_TR_NOOP("Shape builder...");
    sToolTipText  = QT_TR_NOOP("Advanced utility to create shapes");
    sWhatsThis    = sToolTipText;
    sStatusTip    = sToolTipText;
    sPixmap       = "Part_Shapebuilder";
}

void CmdPartBuilder::activated(int iMsg)
{
    Gui::Control().showDialog(new PartGui::TaskShapeBuilder());
}

bool CmdPartBuilder::isActive(void)
{
    return (hasActiveDocument() && !Gui::Control().activeDialog());
}

//--------------------------------------------------------------------------------------

DEF_STD_CMD_A(CmdPartLoft);

CmdPartLoft::CmdPartLoft()
  : Command("Part_Loft")
{
    sAppModule    = "Part";
    sGroup        = QT_TR_NOOP("Part");
    sMenuText     = QT_TR_NOOP("Loft...");
    sToolTipText  = QT_TR_NOOP("Advanced utility to lofts");
    sWhatsThis    = sToolTipText;
    sStatusTip    = sToolTipText;
}

void CmdPartLoft::activated(int iMsg)
{
    Gui::Control().showDialog(new PartGui::TaskLoft());
}

bool CmdPartLoft::isActive(void)
{
    return (hasActiveDocument() && !Gui::Control().activeDialog());
}

//--------------------------------------------------------------------------------------

DEF_STD_CMD_A(CmdShapeInfo);

CmdShapeInfo::CmdShapeInfo()
  :Command("Part_ShapeInfo")
{
    sAppModule    = "Part";
    sGroup        = "Part";
    sMenuText     = "Shape info...";
    sToolTipText  = "Info about shape";
    sWhatsThis    = sToolTipText;
    sStatusTip    = sToolTipText;
    sPixmap       = "Part_ShapeInfo";
}

void CmdShapeInfo::activated(int iMsg)
{
    static const char * const part_pipette[]={
        "32 32 17 1",
        "# c #000000",
        "j c #080808",
        "b c #101010",
        "f c #1c1c1c",
        "g c #4c4c4c",
        "c c #777777",
        "a c #848484",
        "i c #9c9c9c",
        "l c #b9b9b9",
        "e c #cacaca",
        "n c #d6d6d6",
        "k c #dedede",
        "d c #e7e7e7",
        "m c #efefef",
        "h c #f7f7f7",
        "w c #ffffff",
        ". c None",
        "................................",
        ".....................#####......",
        "...................#######......",
        "...................#########....",
        "..................##########....",
        "..................##########....",
        "..................##########....",
        ".................###########....",
        "...............#############....",
        ".............###############....",
        ".............#############......",
        ".............#############......",
        "...............ab######.........",
        "..............cdef#####.........",
        ".............ghdacf####.........",
        "............#ehiacj####.........",
        "............awiaaf####..........",
        "...........iheacf##.............",
        "..........#kdaag##..............",
        ".........gedaacb#...............",
        ".........lwiac##................",
        ".......#amlaaf##................",
        ".......cheaag#..................",
        "......#ndaag##..................",
        ".....#imaacb#...................",
        ".....iwlacf#....................",
        "....#nlaag##....................",
        "....feaagj#.....................",
        "....caag##......................",
        "....ffbj##......................",
        "................................",
        "................................"};

    Gui::Document* doc = Gui::Application::Instance->activeDocument();
    Gui::View3DInventor* view = static_cast<Gui::View3DInventor*>(doc->getActiveView());
    //if (view) {
    //    Gui::View3DInventorViewer* viewer = view->getViewer();
    //    viewer->setEditing(true);
    //    viewer->getWidget()->setCursor(QCursor(QPixmap(part_pipette),4,29));
    //    viewer->addEventCallback(SoMouseButtonEvent::getClassTypeId(), PartGui::ViewProviderPart::shapeInfoCallback);
    // }
}

bool CmdShapeInfo::isActive(void)
{
    App::Document* doc = App::GetApplication().getActiveDocument();
    if (!doc || doc->countObjectsOfType(Part::Feature::getClassTypeId()) == 0)
        return false;

    Gui::MDIView* view = Gui::getMainWindow()->activeWindow();
    if (view && view->isDerivedFrom(Gui::View3DInventor::getClassTypeId())) {
        Gui::View3DInventorViewer* viewer = static_cast<Gui::View3DInventor*>(view)->getViewer();
        return !viewer->isEditing();
    }

    return false;
}

DEF_STD_CMD_A(CmdPartRuledSurface);

CmdPartRuledSurface::CmdPartRuledSurface()
  : Command("Part_RuledSurface")
{
    sAppModule      = "Part";
    sGroup          = QT_TR_NOOP("Part");
    sMenuText       = QT_TR_NOOP("Create ruled surface");
    sToolTipText    = QT_TR_NOOP("Create a ruled surface from two curves");
    sWhatsThis      = sToolTipText;
    sStatusTip      = sToolTipText;
    sPixmap         = "Part_RuledSurface";
}

void CmdPartRuledSurface::activated(int iMsg)
{
    bool ok = false;
    TopoDS_Shape curve1, curve2;
    std::string link1, link2, obj1, obj2;
    Gui::SelectionFilter edgeFilter  ("SELECT Part::Feature SUBELEMENT Edge COUNT 1..2");
    Gui::SelectionFilter wireFilter  ("SELECT Part::Feature SUBELEMENT Wire COUNT 1..2");
    Gui::SelectionFilter partFilter  ("SELECT Part::Feature COUNT 2");
    bool matchEdge = edgeFilter.match();
    bool matchWire = wireFilter.match();
    if (matchEdge || matchWire) {
        // get the selected object
        const std::vector<Gui::SelectionObject>& result = matchEdge
            ? edgeFilter.Result[0] : wireFilter.Result[0];
        // two edges from one object
        if (result.size() == 1) {
            const Part::Feature* part = static_cast<const Part::Feature*>(result[0].getObject());
            const std::vector<std::string>& edges = result[0].getSubNames();
            if (edges.size() != 2) {
                ok = false;
            }
            else {
                ok = true;
                // get the selected sub-shapes
                const Part::TopoShape& shape = part->Shape.getValue();
                curve1 = shape.getSubShape(edges[0].c_str());
                curve2 = shape.getSubShape(edges[1].c_str());
                obj1 = result[0].getObject()->getNameInDocument();
                link1 = edges[0];
                obj2 = result[0].getObject()->getNameInDocument();
                link2 = edges[1];
            }
        }
        // two objects and one edge per object
        else if (result.size() == 2) {
            const Part::Feature* part1 = static_cast<const Part::Feature*>(result[0].getObject());
            const std::vector<std::string>& edges1 = result[0].getSubNames();
            const Part::Feature* part2 = static_cast<const Part::Feature*>(result[1].getObject());
            const std::vector<std::string>& edges2 = result[1].getSubNames();
            if (edges1.size() != 1 || edges2.size() != 1) {
                ok = false;
            }
            else {
                ok = true;
                const Part::TopoShape& shape1 = part1->Shape.getValue();
                curve1 = shape1.getSubShape(edges1[0].c_str());
                const Part::TopoShape& shape2 = part2->Shape.getValue();
                curve2 = shape2.getSubShape(edges2[0].c_str());
                obj1 = result[0].getObject()->getNameInDocument();
                link1 = edges1[0];
                obj2 = result[1].getObject()->getNameInDocument();
                link2 = edges2[0];
            }
        }
    }
    else if (partFilter.match()) {
        const std::vector<Gui::SelectionObject>& result = partFilter.Result[0];
        const Part::Feature* part1 = static_cast<const Part::Feature*>(result[0].getObject());
        const Part::Feature* part2 = static_cast<const Part::Feature*>(result[1].getObject());
        const Part::TopoShape& shape1 = part1->Shape.getValue();
        curve1 = shape1._Shape;
        const Part::TopoShape& shape2 = part2->Shape.getValue();
        curve2 = shape2._Shape;
        obj1 = part1->getNameInDocument();
        obj2 = part2->getNameInDocument();

        if (!curve1.IsNull() && !curve2.IsNull()) {
            if (curve1.ShapeType() == TopAbs_EDGE &&
                curve2.ShapeType() == TopAbs_EDGE)
                ok = true;
            if (curve1.ShapeType() == TopAbs_WIRE &&
                curve2.ShapeType() == TopAbs_WIRE)
                ok = true;
        }
    }

    if (!ok) {
        QMessageBox::warning(Gui::getMainWindow(), QObject::tr("Wrong selection"),
            QObject::tr("You have to select either two edges or two wires."));
        return;
    }

    openCommand("Create ruled surface");
    doCommand(Doc, "FreeCAD.ActiveDocument.addObject('Part::RuledSurface','Filled shape')");
    doCommand(Doc, "FreeCAD.ActiveDocument.ActiveObject.Curve1=(FreeCAD.ActiveDocument.%s,['%s'])"
                 ,obj1.c_str(), link1.c_str());
    doCommand(Doc, "FreeCAD.ActiveDocument.ActiveObject.Curve2=(FreeCAD.ActiveDocument.%s,['%s'])"
                 ,obj2.c_str(), link2.c_str());
    commitCommand();
    updateActive();
}

bool CmdPartRuledSurface::isActive(void)
{
    return getActiveGuiDocument();
}


void CreatePartCommands(void)
{
    Gui::CommandManager &rcCmdMgr = Gui::Application::Instance->commandManager();

    rcCmdMgr.addCommand(new CmdPartMakeSolid());
    rcCmdMgr.addCommand(new CmdPartReverseShape());
    rcCmdMgr.addCommand(new CmdPartBoolean());
    rcCmdMgr.addCommand(new CmdPartExtrude());
    rcCmdMgr.addCommand(new CmdPartMirror());
    rcCmdMgr.addCommand(new CmdPartRevolve());
    rcCmdMgr.addCommand(new CmdPartCrossSections());
    rcCmdMgr.addCommand(new CmdPartFillet());
    rcCmdMgr.addCommand(new CmdPartCommon());
    rcCmdMgr.addCommand(new CmdPartCut());
    rcCmdMgr.addCommand(new CmdPartFuse());
    rcCmdMgr.addCommand(new CmdPartSection());
    //rcCmdMgr.addCommand(new CmdPartBox2());
    //rcCmdMgr.addCommand(new CmdPartBox3());
    rcCmdMgr.addCommand(new CmdPartPrimitives());

    rcCmdMgr.addCommand(new CmdPartImport());
    rcCmdMgr.addCommand(new CmdPartExport());
    rcCmdMgr.addCommand(new CmdPartImportCurveNet());
    rcCmdMgr.addCommand(new CmdPartPickCurveNet());
    rcCmdMgr.addCommand(new CmdShapeInfo());
    rcCmdMgr.addCommand(new CmdPartRuledSurface());
    rcCmdMgr.addCommand(new CmdPartBuilder());
    rcCmdMgr.addCommand(new CmdPartLoft());
} 

