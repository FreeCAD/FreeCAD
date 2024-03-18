/***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU Library General Public License as       *
 *   published by the Free Software Foundation; either version 2 of the    *
 *   License, or (at your option) any later version.                       *
 *   for detail see the LICENCE text file.                                 *
 *   Jürgen Riegel 2002                                                    *
 *                                                                         *
 ***************************************************************************/

#include "PreCompiled.h"
#ifndef _PreComp_
#include <sstream>
#include <vector>

#include <QCoreApplication>
#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QMessageBox>
#include <QRegularExpression>
#endif

#include <App/Document.h>
#include <App/PropertyGeo.h>
#include <Base/Tools.h>
#include <Gui/Action.h>
#include <Gui/Application.h>
#include <Gui/BitmapFactory.h>
#include <Gui/Command.h>
#include <Gui/Control.h>
#include <Gui/FileDialog.h>
#include <Gui/MainWindow.h>
#include <Gui/Selection.h>
#include <Mod/Drawing/App/FeaturePage.h>
#include <Mod/Part/App/PartFeature.h>
#include <Mod/Spreadsheet/App/Sheet.h>

#include "TaskDialog.h"
#include "TaskOrthoViews.h"


using namespace DrawingGui;
using namespace std;


//===========================================================================
// CmdDrawingOpen
//===========================================================================

DEF_STD_CMD(CmdDrawingOpen)

CmdDrawingOpen::CmdDrawingOpen()
    : Command("Drawing_Open")
{
    sAppModule = "Drawing";
    sGroup = QT_TR_NOOP("Drawing");
    sMenuText = QT_TR_NOOP("Open SVG...");
    sToolTipText = QT_TR_NOOP("Open a scalable vector graphic");
    sWhatsThis = "Drawing_Open";
    sStatusTip = sToolTipText;
    sPixmap = "actions/document-new";
}

void CmdDrawingOpen::activated(int iMsg)
{
    Q_UNUSED(iMsg);
    // Reading an image
    QString filename = Gui::FileDialog::getOpenFileName(
        Gui::getMainWindow(),
        QObject::tr("Choose an SVG file to open"),
        QString(),
        QString::fromLatin1("%1 (*.svg *.svgz)").arg(QObject::tr("Scalable Vector Graphic")));
    if (!filename.isEmpty()) {
        filename = Base::Tools::escapeEncodeFilename(filename);
        // load the file with the module
        Command::doCommand(Command::Gui, "import Drawing, DrawingGui");
        Command::doCommand(Command::Gui, "DrawingGui.open(\"%s\")", (const char*)filename.toUtf8());
    }
}

//===========================================================================
// Drawing_NewPage
//===========================================================================

DEF_STD_CMD_ACL(CmdDrawingNewPage)

CmdDrawingNewPage::CmdDrawingNewPage()
    : Command("Drawing_NewPage")
{
    sAppModule = "Drawing";
    sGroup = QT_TR_NOOP("Drawing");
    sMenuText = QT_TR_NOOP("Insert new drawing");
    sToolTipText = QT_TR_NOOP("Insert new drawing");
    sWhatsThis = "Drawing_NewPage";
    sStatusTip = sToolTipText;
}

void CmdDrawingNewPage::activated(int iMsg)
{
    Gui::ActionGroup* pcAction = qobject_cast<Gui::ActionGroup*>(_pcAction);
    QAction* a = qAsConst(pcAction)->actions()[iMsg];

    std::string FeatName = getUniqueObjectName(
        QCoreApplication::translate("Drawing_NewPage", "Page").toStdString().c_str());

    QFileInfo tfi(a->property("Template").toString());
    if (tfi.isReadable()) {
        QString filename = Base::Tools::escapeEncodeFilename(tfi.filePath());
        openCommand("Create page");
        doCommand(Doc,
                  "App.activeDocument().addObject('Drawing::FeaturePage','%s')",
                  FeatName.c_str());
        doCommand(Doc,
                  "App.activeDocument().%s.Template = '%s'",
                  FeatName.c_str(),
                  (const char*)filename.toUtf8());
        doCommand(Doc, "App.activeDocument().recompute()");
        doCommand(Doc, "Gui.activeDocument().getObject('%s').show()", FeatName.c_str());
        commitCommand();
    }
    else {
        QMessageBox::critical(Gui::getMainWindow(),
                              QLatin1String("No template"),
                              QLatin1String("No template available for this page size"));
    }
}

Gui::Action* CmdDrawingNewPage::createAction(void)
{
    Gui::ActionGroup* pcAction = new Gui::ActionGroup(this, Gui::getMainWindow());
    pcAction->setDropDownMenu(true);
    applyCommandData(this->className(), pcAction);

    QAction* defaultAction = nullptr;
    int defaultId = 0;

    QString lastPaper;
    int lastId = -1;

    std::string path = App::Application::getResourceDir();
    path += "Mod/Drawing/Templates/";
    QDir dir(QString::fromUtf8(path.c_str()), QString::fromLatin1("*.svg"));
    for (unsigned int i = 0; i < dir.count(); i++) {
        QRegularExpression rx(
            QString::fromLatin1("(A|B|C|D|E)(\\d)_(Landscape|Portrait)(_.*\\.|\\.)svg$"));
        auto match = rx.match(dir[i]);
        if (match.hasMatch()) {
            QString paper = match.captured(1);
            int id = match.captured(2).toInt();
            QString orientation = match.captured(3);
            QString info = match.captured(4).mid(1);
            info.chop(1);
            if (!info.isEmpty()) {
                info[0] = info[0].toUpper();
            }

            // group by paper size
            if (!lastPaper.isEmpty()) {
                if (lastPaper != paper) {
                    QAction* sep = pcAction->addAction(QString());
                    sep->setSeparator(true);
                }
                else if (lastId != id) {
                    QAction* sep = pcAction->addAction(QString());
                    sep->setSeparator(true);
                }
            }

            lastPaper = paper;
            lastId = id;

            QFile file(QString::fromLatin1(":/icons/actions/drawing-%1-%2%3.svg")
                           .arg(orientation.toLower(), paper)
                           .arg(id));
            QAction* a = pcAction->addAction(QString());
            if (file.open(QFile::ReadOnly)) {
                QByteArray data = file.readAll();
                a->setIcon(Gui::BitmapFactory().pixmapFromSvg(data, QSize(64, 64)));
            }

            a->setProperty("TemplatePaper", paper);
            a->setProperty("TemplateOrientation", orientation);
            a->setProperty("TemplateId", id);
            a->setProperty("TemplateInfo", info);
            a->setProperty("Template", dir.absoluteFilePath(dir[i]));

            if (id == 3) {
                if (!defaultAction) {
                    // set the first found A3 (A3_Landscape) as default
                    defaultAction = a;
                    defaultId = pcAction->actions().size() - 1;
                }
            }
        }
    }

    _pcAction = pcAction;

    languageChange();
    if (defaultAction) {
        pcAction->setIcon(defaultAction->icon());
        pcAction->setProperty("defaultAction", QVariant(defaultId));
    }
    else if (!pcAction->actions().isEmpty()) {
        pcAction->setIcon(qAsConst(pcAction)->actions()[0]->icon());
        pcAction->setProperty("defaultAction", QVariant(0));
    }

    return pcAction;
}

void CmdDrawingNewPage::languageChange()
{
    Command::languageChange();

    if (!_pcAction) {
        return;
    }
    Gui::ActionGroup* pcAction = qobject_cast<Gui::ActionGroup*>(_pcAction);
    QList<QAction*> a = pcAction->actions();
    for (QList<QAction*>::iterator it = a.begin(); it != a.end(); ++it) {
        if ((*it)->isSeparator()) {
            continue;
        }
        QString paper = (*it)->property("TemplatePaper").toString();
        int id = (*it)->property("TemplateId").toInt();
        QString orientation = (*it)->property("TemplateOrientation").toString();
        if (orientation.compare(QLatin1String("landscape"), Qt::CaseInsensitive) == 0) {
            orientation = QCoreApplication::translate("Drawing_NewPage", "Landscape");
        }
        else if (orientation.compare(QLatin1String("portrait"), Qt::CaseInsensitive) == 0) {
            orientation = QCoreApplication::translate("Drawing_NewPage", "Portrait");
        }
        QString info = (*it)->property("TemplateInfo").toString();

        if (info.isEmpty()) {
            (*it)->setText(QCoreApplication::translate("Drawing_NewPage", "%1%2 %3")
                               .arg(paper, QString::number(id), orientation));
            (*it)->setToolTip(
                QCoreApplication::translate("Drawing_NewPage", "Insert new %1%2 %3 drawing")
                    .arg(paper, QString::number(id), orientation));
        }
        else {
            (*it)->setText(QCoreApplication::translate("Drawing_NewPage", "%1%2 %3 (%4)")
                               .arg(paper, QString::number(id), orientation, info));
            (*it)->setToolTip(
                QCoreApplication::translate("Drawing_NewPage", "Insert new %1%2 %3 (%4) drawing")
                    .arg(paper, QString::number(id), orientation, info));
        }
    }
}

bool CmdDrawingNewPage::isActive(void)
{
    if (getActiveGuiDocument()) {
        return true;
    }
    else {
        return false;
    }
}

//===========================================================================
// Drawing_NewA3Landscape
//===========================================================================

DEF_STD_CMD_A(CmdDrawingNewA3Landscape)

CmdDrawingNewA3Landscape::CmdDrawingNewA3Landscape()
    : Command("Drawing_NewA3Landscape")
{
    sAppModule = "Drawing";
    sGroup = QT_TR_NOOP("Drawing");
    sMenuText = QT_TR_NOOP("Insert new A3 landscape drawing");
    sToolTipText = QT_TR_NOOP("Insert new A3 landscape drawing");
    sWhatsThis = "Drawing_NewA3Landscape";
    sStatusTip = sToolTipText;
    sPixmap = "actions/drawing-landscape-A3";
}

void CmdDrawingNewA3Landscape::activated(int iMsg)
{
    Q_UNUSED(iMsg);
    std::string FeatName = getUniqueObjectName("Page");

    openCommand("Create page");
    doCommand(Doc, "App.activeDocument().addObject('Drawing::FeaturePage','%s')", FeatName.c_str());
    doCommand(Doc, "App.activeDocument().%s.Template = 'A3_Landscape.svg'", FeatName.c_str());
    doCommand(Doc, "App.activeDocument().recompute()");
    commitCommand();
}

bool CmdDrawingNewA3Landscape::isActive(void)
{
    if (getActiveGuiDocument()) {
        return true;
    }
    else {
        return false;
    }
}


//===========================================================================
// Drawing_NewView
//===========================================================================

DEF_STD_CMD(CmdDrawingNewView)

CmdDrawingNewView::CmdDrawingNewView()
    : Command("Drawing_NewView")
{
    sAppModule = "Drawing";
    sGroup = QT_TR_NOOP("Drawing");
    sMenuText = QT_TR_NOOP("Insert view in drawing");
    sToolTipText = QT_TR_NOOP("Insert a new View of a Part in the active drawing");
    sWhatsThis = "Drawing_NewView";
    sStatusTip = sToolTipText;
    sPixmap = "actions/drawing-view";
}

void CmdDrawingNewView::activated(int iMsg)
{
    Q_UNUSED(iMsg);
    std::vector<App::DocumentObject*> shapes =
        getSelection().getObjectsOfType(Part::Feature::getClassTypeId());
    if (shapes.empty()) {
        QMessageBox::warning(Gui::getMainWindow(),
                             QObject::tr("Wrong selection"),
                             QObject::tr("Select a Part object."));
        return;
    }

    std::vector<App::DocumentObject*> pages =
        getSelection().getObjectsOfType(Drawing::FeaturePage::getClassTypeId());
    if (pages.empty()) {
        pages = this->getDocument()->getObjectsOfType(Drawing::FeaturePage::getClassTypeId());
        if (pages.empty()) {
            QMessageBox::warning(Gui::getMainWindow(),
                                 QObject::tr("No page found"),
                                 QObject::tr("Create a page first."));
            return;
        }
    }

    const std::vector<App::DocumentObject*> selectedProjections =
        getSelection().getObjectsOfType(Drawing::FeatureView::getClassTypeId());
    float newX = 10.0;
    float newY = 10.0;
    float newScale = 1.0;
    float newRotation = 0.0;
    Base::Vector3d newDirection(0.0, 0.0, 1.0);
    if (!selectedProjections.empty()) {
        const Drawing::FeatureView* const myView =
            static_cast<Drawing::FeatureView*>(selectedProjections.front());

        newX = myView->X.getValue();
        newY = myView->Y.getValue();
        newScale = myView->Scale.getValue();
        newRotation = myView->Rotation.getValue();

        // The "Direction" property does not belong to Drawing::FeatureView, but to one of the
        // many child classes that are projecting objects into the drawing. Therefore, we get the
        // property by name.
        const App::PropertyVector* const propDirection =
            dynamic_cast<App::PropertyVector*>(myView->getPropertyByName("Direction"));
        if (propDirection) {
            newDirection = propDirection->getValue();
        }
    }

    std::string PageName = pages.front()->getNameInDocument();

    openCommand("Create view");
    for (std::vector<App::DocumentObject*>::iterator it = shapes.begin(); it != shapes.end();
         ++it) {
        std::string FeatName = getUniqueObjectName("View");
        doCommand(Doc,
                  "App.activeDocument().addObject('Drawing::FeatureViewPart','%s')",
                  FeatName.c_str());
        doCommand(Doc,
                  "App.activeDocument().%s.Source = App.activeDocument().%s",
                  FeatName.c_str(),
                  (*it)->getNameInDocument());
        doCommand(Doc,
                  "App.activeDocument().%s.Direction = (%e,%e,%e)",
                  FeatName.c_str(),
                  newDirection.x,
                  newDirection.y,
                  newDirection.z);
        doCommand(Doc, "App.activeDocument().%s.X = %e", FeatName.c_str(), newX);
        doCommand(Doc, "App.activeDocument().%s.Y = %e", FeatName.c_str(), newY);
        doCommand(Doc, "App.activeDocument().%s.Scale = %e", FeatName.c_str(), newScale);
        doCommand(Doc, "App.activeDocument().%s.Rotation = %e", FeatName.c_str(), newRotation);
        doCommand(Doc,
                  "App.activeDocument().%s.addObject(App.activeDocument().%s)",
                  PageName.c_str(),
                  FeatName.c_str());
    }
    updateActive();
    commitCommand();
}

//===========================================================================
// Drawing_OrthoView
//===========================================================================

DEF_STD_CMD_A(CmdDrawingOrthoViews)

CmdDrawingOrthoViews::CmdDrawingOrthoViews()
    : Command("Drawing_OrthoViews")
{
    sAppModule = "Drawing";
    sGroup = QT_TR_NOOP("Drawing");
    sMenuText = QT_TR_NOOP("Insert orthographic views");
    sToolTipText = QT_TR_NOOP("Insert an orthographic projection of a part in the active drawing");
    sWhatsThis = "Drawing_OrthoView";
    sStatusTip = sToolTipText;
    sPixmap = "actions/drawing-orthoviews";
}

void CmdDrawingOrthoViews::activated(int iMsg)
{
    Q_UNUSED(iMsg);
    const std::vector<App::DocumentObject*> shapes =
        getSelection().getObjectsOfType(Part::Feature::getClassTypeId());
    if (shapes.size() != 1) {
        QMessageBox::warning(Gui::getMainWindow(),
                             QObject::tr("Wrong selection"),
                             QObject::tr("Select exactly one Part object."));
        return;
    }

    // Check that a page object exists. TaskDlgOrthoViews will then check for a selected page object
    // and use that, otherwise it will use the first page in the document.
    const std::vector<App::DocumentObject*> pages =
        this->getDocument()->getObjectsOfType(Drawing::FeaturePage::getClassTypeId());
    if (pages.empty()) {
        QMessageBox::warning(Gui::getMainWindow(),
                             QObject::tr("No page found"),
                             QObject::tr("Create a page first."));
        return;
    }

    TaskDlgOrthoViews* dlg = new TaskDlgOrthoViews();
    dlg->setDocumentName(this->getDocument()->getName());
    Gui::Control().showDialog(dlg);
}

bool CmdDrawingOrthoViews::isActive(void)
{
    if (Gui::Control().activeDialog()) {
        return false;
    }
    return true;
}


//===========================================================================
// Drawing_OpenBrowserView
//===========================================================================

DEF_STD_CMD_A(CmdDrawingOpenBrowserView)

CmdDrawingOpenBrowserView::CmdDrawingOpenBrowserView()
    : Command("Drawing_OpenBrowserView")
{
    // setting the
    sGroup = QT_TR_NOOP("Drawing");
    sMenuText = QT_TR_NOOP("Open &browser view");
    sToolTipText = QT_TR_NOOP("Opens the selected page in a browser view");
    sWhatsThis = "Drawing_OpenBrowserView";
    sStatusTip = QT_TR_NOOP("Opens the selected page in a browser view");
    sPixmap = "actions/drawing-openbrowser";
}

void CmdDrawingOpenBrowserView::activated(int iMsg)
{
    Q_UNUSED(iMsg);
    unsigned int n = getSelection().countObjectsOfType(Drawing::FeaturePage::getClassTypeId());
    if (n != 1) {
        QMessageBox::warning(Gui::getMainWindow(),
                             QObject::tr("Wrong selection"),
                             QObject::tr("Select one Page object."));
        return;
    }
    std::vector<Gui::SelectionSingleton::SelObj> Sel = getSelection().getSelection();
    doCommand(Doc, "PageName = App.activeDocument().%s.PageResult", Sel[0].FeatName);
    doCommand(Doc, "import WebGui");
    doCommand(Doc, "WebGui.openBrowser(PageName)");
}

bool CmdDrawingOpenBrowserView::isActive(void)
{
    return (getActiveGuiDocument() ? true : false);
}

//===========================================================================
// Drawing_Annotation
//===========================================================================

DEF_STD_CMD_A(CmdDrawingAnnotation)

CmdDrawingAnnotation::CmdDrawingAnnotation()
    : Command("Drawing_Annotation")
{
    // setting the
    sGroup = QT_TR_NOOP("Drawing");
    sMenuText = QT_TR_NOOP("&Annotation");
    sToolTipText = QT_TR_NOOP("Inserts an Annotation view in the active drawing");
    sWhatsThis = "Drawing_Annotation";
    sStatusTip = QT_TR_NOOP("Inserts an Annotation view in the active drawing");
    sPixmap = "actions/drawing-annotation";
}

void CmdDrawingAnnotation::activated(int iMsg)
{
    Q_UNUSED(iMsg);
    std::vector<App::DocumentObject*> pages =
        getSelection().getObjectsOfType(Drawing::FeaturePage::getClassTypeId());
    if (pages.empty()) {
        pages = this->getDocument()->getObjectsOfType(Drawing::FeaturePage::getClassTypeId());
        if (pages.empty()) {
            QMessageBox::warning(Gui::getMainWindow(),
                                 QObject::tr("No page found"),
                                 QObject::tr("Create a page first."));
            return;
        }
    }
    std::string PageName = pages.front()->getNameInDocument();
    std::string FeatName = getUniqueObjectName("Annotation");
    openCommand("Create Annotation");
    doCommand(Doc,
              "App.activeDocument().addObject('Drawing::FeatureViewAnnotation','%s')",
              FeatName.c_str());
    doCommand(Doc, "App.activeDocument().%s.X = 10.0", FeatName.c_str());
    doCommand(Doc, "App.activeDocument().%s.Y = 10.0", FeatName.c_str());
    doCommand(Doc, "App.activeDocument().%s.Scale = 7.0", FeatName.c_str());
    doCommand(Doc,
              "App.activeDocument().%s.addObject(App.activeDocument().%s)",
              PageName.c_str(),
              FeatName.c_str());
    updateActive();
    commitCommand();
}

bool CmdDrawingAnnotation::isActive(void)
{
    return (getActiveGuiDocument() ? true : false);
}


//===========================================================================
// Drawing_Clip
//===========================================================================

DEF_STD_CMD_A(CmdDrawingClip)

CmdDrawingClip::CmdDrawingClip()
    : Command("Drawing_Clip")
{
    // setting the
    sGroup = QT_TR_NOOP("Drawing");
    sMenuText = QT_TR_NOOP("&Clip");
    sToolTipText = QT_TR_NOOP("Inserts a clip group in the active drawing");
    sWhatsThis = "Drawing_Annotation";
    sStatusTip = QT_TR_NOOP("Inserts a clip group in the active drawing");
    sPixmap = "actions/drawing-clip";
}

void CmdDrawingClip::activated(int iMsg)
{
    Q_UNUSED(iMsg);
    std::vector<App::DocumentObject*> pages =
        getSelection().getObjectsOfType(Drawing::FeaturePage::getClassTypeId());
    if (pages.empty()) {
        pages = this->getDocument()->getObjectsOfType(Drawing::FeaturePage::getClassTypeId());
        if (pages.empty()) {
            QMessageBox::warning(Gui::getMainWindow(),
                                 QObject::tr("No page found"),
                                 QObject::tr("Create a page first."));
            return;
        }
    }
    std::string PageName = pages.front()->getNameInDocument();
    std::string FeatName = getUniqueObjectName("Clip");
    openCommand("Create Clip");
    doCommand(Doc, "App.activeDocument().addObject('Drawing::FeatureClip','%s')", FeatName.c_str());
    doCommand(Doc,
              "App.activeDocument().%s.addObject(App.activeDocument().%s)",
              PageName.c_str(),
              FeatName.c_str());
    updateActive();
    commitCommand();
}

bool CmdDrawingClip::isActive(void)
{
    return (getActiveGuiDocument() ? true : false);
}


//===========================================================================
// Drawing_Symbol
//===========================================================================

DEF_STD_CMD_A(CmdDrawingSymbol)

CmdDrawingSymbol::CmdDrawingSymbol()
    : Command("Drawing_Symbol")
{
    // setting the
    sGroup = QT_TR_NOOP("Drawing");
    sMenuText = QT_TR_NOOP("&Symbol");
    sToolTipText = QT_TR_NOOP("Inserts a symbol from a svg file in the active drawing");
    sWhatsThis = "Drawing_Symbol";
    sStatusTip = QT_TR_NOOP("Inserts a symbol from a svg file in the active drawing");
    sPixmap = "actions/drawing-symbol";
}

void CmdDrawingSymbol::activated(int iMsg)
{
    Q_UNUSED(iMsg);
    std::vector<App::DocumentObject*> pages =
        getSelection().getObjectsOfType(Drawing::FeaturePage::getClassTypeId());
    if (pages.empty()) {
        pages = this->getDocument()->getObjectsOfType(Drawing::FeaturePage::getClassTypeId());
        if (pages.empty()) {
            QMessageBox::warning(Gui::getMainWindow(),
                                 QObject::tr("No page found"),
                                 QObject::tr("Create a page first."));
            return;
        }
    }
    // Reading an image
    QString filename = Gui::FileDialog::getOpenFileName(
        Gui::getMainWindow(),
        QObject::tr("Choose an SVG file to open"),
        QString(),
        QString::fromLatin1("%1 (*.svg *.svgz)").arg(QObject::tr("Scalable Vector Graphic")));
    if (!filename.isEmpty()) {
        std::string PageName = pages.front()->getNameInDocument();
        std::string FeatName = getUniqueObjectName("Symbol");
        filename = Base::Tools::escapeEncodeFilename(filename);
        openCommand("Create Symbol");
        doCommand(Doc, "import Drawing");
        doCommand(Doc, "f = open(\"%s\",'r')", (const char*)filename.toUtf8());
        doCommand(Doc, "svg = f.read()");
        doCommand(Doc, "f.close()");
        doCommand(Doc,
                  "App.activeDocument().addObject('Drawing::FeatureViewSymbol','%s')",
                  FeatName.c_str());
        doCommand(Doc,
                  "App.activeDocument().%s.Symbol = Drawing.removeSvgTags(svg)",
                  FeatName.c_str());
        doCommand(Doc,
                  "App.activeDocument().%s.addObject(App.activeDocument().%s)",
                  PageName.c_str(),
                  FeatName.c_str());
        updateActive();
        commitCommand();
    }
}

bool CmdDrawingSymbol::isActive(void)
{
    return (getActiveGuiDocument() ? true : false);
}


//===========================================================================
// Drawing_ExportPage
//===========================================================================

DEF_STD_CMD_A(CmdDrawingExportPage)

CmdDrawingExportPage::CmdDrawingExportPage()
    : Command("Drawing_ExportPage")
{
    // setting the
    sGroup = QT_TR_NOOP("File");
    sMenuText = QT_TR_NOOP("&Export page...");
    sToolTipText = QT_TR_NOOP("Export a page to an SVG file");
    sWhatsThis = "Drawing_ExportPage";
    sStatusTip = QT_TR_NOOP("Export a page to an SVG file");
    sPixmap = "document-save";
}

void CmdDrawingExportPage::activated(int iMsg)
{
    Q_UNUSED(iMsg);
    unsigned int n = getSelection().countObjectsOfType(Drawing::FeaturePage::getClassTypeId());
    if (n != 1) {
        QMessageBox::warning(Gui::getMainWindow(),
                             QObject::tr("Wrong selection"),
                             QObject::tr("Select one Page object."));
        return;
    }

    QStringList filter;
    filter << QString::fromLatin1("%1 (*.svg)").arg(QObject::tr("Scalable Vector Graphic"));
    filter << QString::fromLatin1("%1 (*.*)").arg(QObject::tr("All Files"));

    QString fn = Gui::FileDialog::getSaveFileName(Gui::getMainWindow(),
                                                  QObject::tr("Export page"),
                                                  QString(),
                                                  filter.join(QLatin1String(";;")));
    if (!fn.isEmpty()) {
        std::vector<Gui::SelectionSingleton::SelObj> Sel = getSelection().getSelection();
        openCommand("Drawing export page");

        doCommand(Doc, "PageFile = open(App.activeDocument().%s.PageResult,'r')", Sel[0].FeatName);
        std::string fname = (const char*)fn.toUtf8();
        fname = Base::Tools::escapeEncodeFilename(fname);
        doCommand(Doc, "OutFile = open(\"%s\",'w')", fname.c_str());
        doCommand(Doc, "OutFile.write(PageFile.read())");
        doCommand(Doc, "del OutFile,PageFile");

        commitCommand();
    }
}

bool CmdDrawingExportPage::isActive(void)
{
    return (getActiveGuiDocument() ? true : false);
}

//===========================================================================
// Drawing_ProjectShape
//===========================================================================

DEF_STD_CMD_A(CmdDrawingProjectShape)

CmdDrawingProjectShape::CmdDrawingProjectShape()
    : Command("Drawing_ProjectShape")
{
    // setting the
    sGroup = QT_TR_NOOP("Drawing");
    sMenuText = QT_TR_NOOP("Project shape...");
    sToolTipText = QT_TR_NOOP("Project shape onto a user-defined plane");
    sStatusTip = QT_TR_NOOP("Project shape onto a user-defined plane");
    sWhatsThis = "Drawing_ProjectShape";
}

void CmdDrawingProjectShape::activated(int iMsg)
{
    Q_UNUSED(iMsg);
    Gui::TaskView::TaskDialog* dlg = Gui::Control().activeDialog();
    if (!dlg) {
        dlg = new DrawingGui::TaskProjection();
        dlg->setButtonPosition(Gui::TaskView::TaskDialog::South);
    }
    Gui::Control().showDialog(dlg);
}

bool CmdDrawingProjectShape::isActive(void)
{
    int ct = Gui::Selection().countObjectsOfType(Part::Feature::getClassTypeId());
    return (ct > 0 && !Gui::Control().activeDialog());
}


//===========================================================================
// Drawing_Draft_View
//===========================================================================

DEF_STD_CMD_A(CmdDrawingDraftView)

CmdDrawingDraftView::CmdDrawingDraftView()
    : Command("Drawing_DraftView")
{
    // setting the
    sGroup = QT_TR_NOOP("Drawing");
    sMenuText = QT_TR_NOOP("&Draft View");
    sToolTipText =
        QT_TR_NOOP("Inserts a Draft view of the selected object(s) in the active drawing");
    sWhatsThis = "Drawing_DraftView";
    sStatusTip = QT_TR_NOOP("Inserts a Draft view of the selected object(s) in the active drawing");
    sPixmap = "actions/drawing-draft-view";
}

void CmdDrawingDraftView::activated(int iMsg)
{
    Q_UNUSED(iMsg);
    addModule(Gui, "Draft");
    doCommand(Gui, "Gui.runCommand(\"Draft_Drawing\")");
}

bool CmdDrawingDraftView::isActive(void)
{
    return (getActiveGuiDocument() ? true : false);
}


//===========================================================================
// Drawing_Spreadheet_View
//===========================================================================

DEF_STD_CMD_A(CmdDrawingSpreadsheetView)

CmdDrawingSpreadsheetView::CmdDrawingSpreadsheetView()
    : Command("Drawing_SpreadsheetView")
{
    // setting the
    sGroup = QT_TR_NOOP("Drawing");
    sMenuText = QT_TR_NOOP("&Spreadsheet View");
    sToolTipText = QT_TR_NOOP("Inserts a view of a selected spreadsheet in the active drawing");
    sWhatsThis = "Drawing_SpreadsheetView";
    sStatusTip = QT_TR_NOOP("Inserts a view of a selected spreadsheet in the active drawing");
    sPixmap = "actions/drawing-spreadsheet";
}

void CmdDrawingSpreadsheetView::activated(int iMsg)
{
    Q_UNUSED(iMsg);
    const std::vector<App::DocumentObject*> spreads =
        getSelection().getObjectsOfType(Spreadsheet::Sheet::getClassTypeId());
    if (spreads.size() != 1) {
        QMessageBox::warning(Gui::getMainWindow(),
                             QObject::tr("Wrong selection"),
                             QObject::tr("Select exactly one Spreadsheet object."));
        return;
    }
    const std::vector<App::DocumentObject*> pages =
        this->getDocument()->getObjectsOfType(Drawing::FeaturePage::getClassTypeId());
    if (pages.empty()) {
        QMessageBox::warning(Gui::getMainWindow(),
                             QObject::tr("No page found"),
                             QObject::tr("Create a page first."));
        return;
    }
    std::string SpreadName = spreads.front()->getNameInDocument();
    std::string PageName = pages.front()->getNameInDocument();
    openCommand("Create spreadsheet view");
    std::string FeatName = getUniqueObjectName("View");
    doCommand(Doc,
              "App.activeDocument().addObject('Drawing::FeatureViewSpreadsheet','%s')",
              FeatName.c_str());
    doCommand(Doc,
              "App.activeDocument().%s.Source = App.activeDocument().%s",
              FeatName.c_str(),
              SpreadName.c_str());
    doCommand(Doc,
              "App.activeDocument().%s.addObject(App.activeDocument().%s)",
              PageName.c_str(),
              FeatName.c_str());
    updateActive();
    commitCommand();
}

bool CmdDrawingSpreadsheetView::isActive(void)
{
    return (getActiveGuiDocument() ? true : false);
}


void CreateDrawingCommands(void)
{
    Gui::CommandManager& rcCmdMgr = Gui::Application::Instance->commandManager();

    rcCmdMgr.addCommand(new CmdDrawingOpen());
    rcCmdMgr.addCommand(new CmdDrawingNewPage());
    rcCmdMgr.addCommand(new CmdDrawingNewA3Landscape());
    rcCmdMgr.addCommand(new CmdDrawingNewView());
    rcCmdMgr.addCommand(new CmdDrawingOrthoViews());
    rcCmdMgr.addCommand(new CmdDrawingOpenBrowserView());
    rcCmdMgr.addCommand(new CmdDrawingAnnotation());
    rcCmdMgr.addCommand(new CmdDrawingClip());
    rcCmdMgr.addCommand(new CmdDrawingSymbol());
    rcCmdMgr.addCommand(new CmdDrawingExportPage());
    rcCmdMgr.addCommand(new CmdDrawingProjectShape());
    rcCmdMgr.addCommand(new CmdDrawingDraftView());
    rcCmdMgr.addCommand(new CmdDrawingSpreadsheetView());
}
