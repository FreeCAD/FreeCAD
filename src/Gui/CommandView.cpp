/***************************************************************************
 *   Copyright (c) 2002 Jürgen Riegel <juergen.riegel@web.de>              *
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
# include <Inventor/events/SoMouseButtonEvent.h>
# include <Inventor/nodes/SoOrthographicCamera.h>
# include <Inventor/nodes/SoPerspectiveCamera.h>
# include <QApplication>
# include <QDialog>
# include <QDomDocument>
# include <QDomElement>
# include <QFile>
# include <QFileInfo>
# include <QFont>
# include <QFontMetrics>
# include <QImageReader>
# include <QMessageBox>
# include <QPainter>
# include <QPointer>
# include <QTextStream>
#endif

#include <App/ComplexGeoDataPy.h>
#include <App/Document.h>
#include <App/DocumentObject.h>
#include <App/GeoFeature.h>
#include <App/GeoFeatureGroupExtension.h>
#include <App/MeasureDistance.h>
#include <Base/Console.h>
#include <Base/Parameter.h>

#include "Command.h"
#include "Action.h"
#include "Application.h"
#include "BitmapFactory.h"
#include "Control.h"
#include "Clipping.h"
#include "DemoMode.h"
#include "DlgDisplayPropertiesImp.h"
#include "DlgSettingsImageImp.h"
#include "Document.h"
#include "FileDialog.h"
#include "ImageView.h"
#include "Macro.h"
#include "MainWindow.h"
#include "NavigationStyle.h"
#include "SceneInspector.h"
#include "Selection.h"
#include "SelectionObject.h"
#include "SoAxisCrossKit.h"
#include "SoFCOffscreenRenderer.h"
#include "TextureMapping.h"
#include "Tools.h"
#include "Tree.h"
#include "TreeParams.h"
#include "Utilities.h"
#include "View3DInventor.h"
#include "View3DInventorViewer.h"
#include "ViewParams.h"
#include "ViewProviderMeasureDistance.h"
#include "ViewProviderGeometryObject.h"
#include "WaitCursor.h"

using namespace Gui;
using Gui::Dialog::DlgSettingsImageImp;
namespace sp = std::placeholders;

namespace {
// A helper class to open a transaction when changing properties of view providers.
// It uses the same parameter key as the PropertyView to control the behaviour.
class TransactionView {
    Gui::Document* document;

public:
    static bool getDefault() {
        auto hGrp = App::GetApplication().GetParameterGroupByPath(
                "User parameter:BaseApp/Preferences/PropertyView");
        return hGrp->GetBool("AutoTransactionView", false);
    }
    TransactionView(Gui::Document* doc, const char* name, bool enable = getDefault())
        : document(doc)
    {
        if (document && enable) {
            document->openCommand(name);
        }
        else {
            document = nullptr;
        }
    }

    ~TransactionView() {
        if (document) {
            document->commitCommand();
        }
    }
};
}

//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

DEF_STD_CMD_AC(StdOrthographicCamera)

StdOrthographicCamera::StdOrthographicCamera()
  : Command("Std_OrthographicCamera")
{
    sGroup        = "Standard-View";
    sMenuText     = QT_TR_NOOP("Orthographic view");
    sToolTipText  = QT_TR_NOOP("Switches to orthographic view mode");
    sWhatsThis    = "Std_OrthographicCamera";
    sStatusTip    = QT_TR_NOOP("Switches to orthographic view mode");
    sPixmap       = "view-isometric";
    sAccel        = "V, O";
    eType         = Alter3DView;
}

void StdOrthographicCamera::activated(int iMsg)
{
    if (iMsg == 1) {
        auto view = qobject_cast<View3DInventor*>(getMainWindow()->activeWindow());
        SoCamera* cam = view->getViewer()->getSoRenderManager()->getCamera();
        if (!cam || cam->getTypeId() != SoOrthographicCamera::getClassTypeId())

            doCommand(Command::Gui,"Gui.activeDocument().activeView().setCameraType(\"Orthographic\")");
    }
}

bool StdOrthographicCamera::isActive()
{
    auto view = qobject_cast<View3DInventor*>(getMainWindow()->activeWindow());
    if (view) {
        // update the action group if needed
        bool check = _pcAction->isChecked();
        SoCamera* cam = view->getViewer()->getSoRenderManager()->getCamera();
        bool mode = cam ? cam->getTypeId() == SoOrthographicCamera::getClassTypeId() : false;

        if (mode != check)
            _pcAction->setChecked(mode);
        return true;
    }

    return false;
}

Action * StdOrthographicCamera::createAction()
{
    Action *pcAction = Command::createAction();
    pcAction->setCheckable(true);
    return pcAction;
}

DEF_STD_CMD_AC(StdPerspectiveCamera)

StdPerspectiveCamera::StdPerspectiveCamera()
  : Command("Std_PerspectiveCamera")
{
    sGroup        = "Standard-View";
    sMenuText     = QT_TR_NOOP("Perspective view");
    sToolTipText  = QT_TR_NOOP("Switches to perspective view mode");
    sWhatsThis    = "Std_PerspectiveCamera";
    sStatusTip    = QT_TR_NOOP("Switches to perspective view mode");
    sPixmap       = "view-perspective";
    sAccel        = "V, P";
    eType         = Alter3DView;
}

void StdPerspectiveCamera::activated(int iMsg)
{
    if (iMsg == 1) {
        auto view = qobject_cast<View3DInventor*>(getMainWindow()->activeWindow());
        SoCamera* cam = view->getViewer()->getSoRenderManager()->getCamera();
        if (!cam || cam->getTypeId() != SoPerspectiveCamera::getClassTypeId())

            doCommand(Command::Gui,"Gui.activeDocument().activeView().setCameraType(\"Perspective\")");
    }
}

bool StdPerspectiveCamera::isActive()
{
    auto view = qobject_cast<View3DInventor*>(getMainWindow()->activeWindow());
    if (view) {
        // update the action group if needed
        bool check = _pcAction->isChecked();
        SoCamera* cam = view->getViewer()->getSoRenderManager()->getCamera();
        bool mode = cam ? cam->getTypeId() == SoPerspectiveCamera::getClassTypeId() : false;

        if (mode != check)
            _pcAction->setChecked(mode);

        return true;
    }

    return false;
}

Action * StdPerspectiveCamera::createAction()
{
    Action *pcAction = Command::createAction();
    pcAction->setCheckable(true);
    return pcAction;
}

//===========================================================================

// The two commands below are provided for convenience so that they can be bound
// to a button of a spacemouse

//===========================================================================
// Std_ViewSaveCamera
//===========================================================================

DEF_3DV_CMD(StdCmdViewSaveCamera)

StdCmdViewSaveCamera::StdCmdViewSaveCamera()
  : Command("Std_ViewSaveCamera")
{
    sGroup        = "Standard-View";
    sMenuText     = QT_TR_NOOP("Save current camera");
    sToolTipText  = QT_TR_NOOP("Save current camera settings");
    sStatusTip    = QT_TR_NOOP("Save current camera settings");
    sWhatsThis    = "Std_ViewSaveCamera";
    eType         = Alter3DView;
}

void StdCmdViewSaveCamera::activated(int iMsg)
{
    Q_UNUSED(iMsg);

    auto view = qobject_cast<Gui::View3DInventor*>(Gui::getMainWindow()->activeWindow());
    if (view) {
        view->getViewer()->saveHomePosition();
    }
}

//===========================================================================
// Std_ViewRestoreCamera
//===========================================================================
DEF_3DV_CMD(StdCmdViewRestoreCamera)

StdCmdViewRestoreCamera::StdCmdViewRestoreCamera()
  : Command("Std_ViewRestoreCamera")
{
    sGroup        = "Standard-View";
    sMenuText     = QT_TR_NOOP("Restore saved camera");
    sToolTipText  = QT_TR_NOOP("Restore saved camera settings");
    sStatusTip    = QT_TR_NOOP("Restore saved camera settings");
    sWhatsThis    = "Std_ViewRestoreCamera";
    eType         = Alter3DView;
}

void StdCmdViewRestoreCamera::activated(int iMsg)
{
    Q_UNUSED(iMsg);

    auto view = qobject_cast<Gui::View3DInventor*>(Gui::getMainWindow()->activeWindow());
    if (view) {
        view->getViewer()->resetToHomePosition();
    }
}

//===========================================================================
// Std_FreezeViews
//===========================================================================
class StdCmdFreezeViews : public Gui::Command
{
public:
    StdCmdFreezeViews();
    ~StdCmdFreezeViews() override = default;
    const char* className() const override
    { return "StdCmdFreezeViews"; }

    void setShortcut (const QString &) override;
    QString getShortcut() const override;

protected:
    void activated(int iMsg) override;
    bool isActive() override;
    Action * createAction() override;
    void languageChange() override;

private:
    void onSaveViews();
    void onRestoreViews();

private:
    const int maxViews{50};
    int savedViews{0};
    int offset{0};
    QAction* saveView{nullptr};
    QAction* freezeView{nullptr};
    QAction* clearView{nullptr};
    QAction* separator{nullptr};
};

StdCmdFreezeViews::StdCmdFreezeViews()
  : Command("Std_FreezeViews")
{
    sGroup        = "Standard-View";
    sMenuText     = QT_TR_NOOP("Freeze display");
    sToolTipText  = QT_TR_NOOP("Freezes the current view position");
    sWhatsThis    = "Std_FreezeViews";
    sStatusTip    = QT_TR_NOOP("Freezes the current view position");
    sAccel        = "Shift+F";
    eType         = Alter3DView;
}

Action * StdCmdFreezeViews::createAction()
{
    auto pcAction = new ActionGroup(this, getMainWindow());
    pcAction->setDropDownMenu(true);
    applyCommandData(this->className(), pcAction);

    // add the action items
    saveView = pcAction->addAction(QObject::tr("Save views..."));
    saveView->setWhatsThis(QString::fromLatin1(getWhatsThis()));
    QAction* loadView = pcAction->addAction(QObject::tr("Load views..."));
    loadView->setWhatsThis(QString::fromLatin1(getWhatsThis()));
    pcAction->addAction(QString::fromLatin1(""))->setSeparator(true);
    freezeView = pcAction->addAction(QObject::tr("Freeze view"));
    freezeView->setShortcut(QString::fromLatin1(getAccel()));
    freezeView->setWhatsThis(QString::fromLatin1(getWhatsThis()));
    clearView = pcAction->addAction(QObject::tr("Clear views"));
    clearView->setWhatsThis(QString::fromLatin1(getWhatsThis()));
    separator = pcAction->addAction(QString::fromLatin1(""));
    separator->setSeparator(true);
    offset = pcAction->actions().count();

    // allow up to 50 views
    for (int i=0; i<maxViews; i++)
        pcAction->addAction(QString::fromLatin1(""))->setVisible(false);

    return pcAction;
}

void StdCmdFreezeViews::setShortcut(const QString &shortcut)
{
    if (freezeView)
        freezeView->setShortcut(shortcut);
}

QString StdCmdFreezeViews::getShortcut() const
{
    if (freezeView)
        return freezeView->shortcut().toString();
    return Command::getShortcut();
}

void StdCmdFreezeViews::activated(int iMsg)
{
    auto pcAction = qobject_cast<ActionGroup*>(_pcAction);

    if (iMsg == 0) {
        onSaveViews();
    }
    else if (iMsg == 1) {
        onRestoreViews();
    }
    else if (iMsg == 3) {
        // Create a new view
        const char* ppReturn=nullptr;
        getGuiApplication()->sendMsgToActiveView("GetCamera",&ppReturn);

        QList<QAction*> acts = pcAction->actions();
        int index = 1;
        for (QList<QAction*>::Iterator it = acts.begin()+offset; it != acts.end(); ++it, index++) {
            if (!(*it)->isVisible()) {
                savedViews++;
                QString viewnr = QString(QObject::tr("Restore view &%1")).arg(index);
                (*it)->setText(viewnr);
                (*it)->setToolTip(QString::fromLatin1(ppReturn));
                (*it)->setVisible(true);
                if (index < 10) {
                    (*it)->setShortcut(QKeySequence(QString::fromLatin1("CTRL+%1").arg(index)));
                }
                break;
            }
        }
    }
    else if (iMsg == 4) {
        savedViews = 0;
        QList<QAction*> acts = pcAction->actions();
        for (QList<QAction*>::Iterator it = acts.begin()+offset; it != acts.end(); ++it)
            (*it)->setVisible(false);
    }
    else if (iMsg >= offset) {
        // Activate a view
        QList<QAction*> acts = pcAction->actions();
        QString data = acts[iMsg]->toolTip();
        QString send = QString::fromLatin1("SetCamera %1").arg(data);
        getGuiApplication()->sendMsgToActiveView(send.toLatin1());
    }
}

void StdCmdFreezeViews::onSaveViews()
{
    // Save the views to an XML file
    QString fn = FileDialog::getSaveFileName(getMainWindow(), QObject::tr("Save frozen views"),
                                             QString(), QString::fromLatin1("%1 (*.cam)").arg(QObject::tr("Frozen views")));
    if (fn.isEmpty())
        return;
    QFile file(fn);
    if (file.open(QFile::WriteOnly))
    {
        QTextStream str(&file);
        auto pcAction = qobject_cast<ActionGroup*>(_pcAction);
        QList<QAction*> acts = pcAction->actions();
        str << "<?xml version='1.0' encoding='utf-8'?>\n"
            << "<FrozenViews SchemaVersion=\"1\">\n";
        str << "  <Views Count=\"" << savedViews <<"\">\n";

        for (QList<QAction*>::Iterator it = acts.begin()+offset; it != acts.end(); ++it) {
            if ( !(*it)->isVisible() )
                break;
            QString data = (*it)->toolTip();

            // remove the first line because it's a comment like '#Inventor V2.1 ascii'
            QString viewPos;
            if (!data.isEmpty()) {
                QStringList lines = data.split(QString::fromLatin1("\n"));
                if (lines.size() > 1) {
                    lines.pop_front();
                }
                viewPos = lines.join(QString::fromLatin1(" "));
            }

            str << "    <Camera settings=\"" << viewPos.toLatin1().constData() << "\"/>\n";
        }

        str << "  </Views>\n";
        str << "</FrozenViews>\n";
    }
}

void StdCmdFreezeViews::onRestoreViews()
{
    // Should we clear the already saved views
    if (savedViews > 0) {
        auto ret = QMessageBox::question(getMainWindow(), QObject::tr("Restore views"),
            QObject::tr("Importing the restored views would clear the already stored views.\n"
                        "Do you want to continue?"), QMessageBox::Yes | QMessageBox::No,
                                                     QMessageBox::Yes);
        if (ret != QMessageBox::Yes)
            return;
    }

    // Restore the views from an XML file
    QString fn = FileDialog::getOpenFileName(getMainWindow(), QObject::tr("Restore frozen views"),
                                             QString(), QString::fromLatin1("%1 (*.cam)").arg(QObject::tr("Frozen views")));
    if (fn.isEmpty())
        return;
    QFile file(fn);
    if (!file.open(QFile::ReadOnly)) {
        QMessageBox::critical(getMainWindow(), QObject::tr("Restore views"),
            QObject::tr("Cannot open file '%1'.").arg(fn));
        return;
    }

    QDomDocument xmlDocument;
    QString errorStr;
    int errorLine;
    int errorColumn;

    // evaluate the XML content
    if (!xmlDocument.setContent(&file, true, &errorStr, &errorLine, &errorColumn)) {
        std::cerr << "Parse error in XML content at line " << errorLine
                  << ", column " << errorColumn << ": "
                  << (const char*)errorStr.toLatin1() << std::endl;
        return;
    }

    // get the root element
    QDomElement root = xmlDocument.documentElement();
    if (root.tagName() != QLatin1String("FrozenViews")) {
        std::cerr << "Unexpected XML structure" << std::endl;
        return;
    }

    bool ok;
    int scheme = root.attribute(QString::fromLatin1("SchemaVersion")).toInt(&ok);
    if (!ok)
        return;
    // SchemeVersion "1"
    if (scheme == 1) {
        // read the views, ignore the attribute 'Count'
        QDomElement child = root.firstChildElement(QString::fromLatin1("Views"));
        QDomElement views = child.firstChildElement(QString::fromLatin1("Camera"));
        QStringList cameras;
        while (!views.isNull()) {
            QString setting = views.attribute(QString::fromLatin1("settings"));
            cameras << setting;
            views = views.nextSiblingElement(QString::fromLatin1("Camera"));
        }

        // use this rather than the attribute 'Count' because it could be
        // changed from outside
        int ct = cameras.count();
        auto pcAction = qobject_cast<ActionGroup*>(_pcAction);
        QList<QAction*> acts = pcAction->actions();

        int numRestoredViews = std::min<int>(ct, acts.size()-offset);
        savedViews = numRestoredViews;

        if (numRestoredViews > 0)
            separator->setVisible(true);
        for(int i=0; i<numRestoredViews; i++) {
            QString setting = cameras[i];
            QString viewnr = QString(QObject::tr("Restore view &%1")).arg(i+1);
            acts[i+offset]->setText(viewnr);
            acts[i+offset]->setToolTip(setting);
            acts[i+offset]->setVisible(true);
            if (i < 9) {
                acts[i+offset]->setShortcut(QKeySequence(QString::fromLatin1("CTRL+%1").arg(i+1)));
            }
        }

        // if less views than actions
        for (int index = numRestoredViews+offset; index < acts.count(); index++)
            acts[index]->setVisible(false);
    }
}

bool StdCmdFreezeViews::isActive()
{
    auto view = qobject_cast<View3DInventor*>(getMainWindow()->activeWindow());
    if (view) {
        saveView->setEnabled(savedViews > 0);
        freezeView->setEnabled(savedViews < maxViews);
        clearView->setEnabled(savedViews > 0);
        separator->setVisible(savedViews > 0);
        return true;
    }
    else {
        separator->setVisible(savedViews > 0);
    }

    return false;
}

void StdCmdFreezeViews::languageChange()
{
    Command::languageChange();

    if (!_pcAction)
        return;
    auto pcAction = qobject_cast<ActionGroup*>(_pcAction);
    QList<QAction*> acts = pcAction->actions();
    acts[0]->setText(QObject::tr("Save views..."));
    acts[1]->setText(QObject::tr("Load views..."));
    acts[3]->setText(QObject::tr("Freeze view"));
    acts[4]->setText(QObject::tr("Clear views"));
    int index=1;
    for (QList<QAction*>::Iterator it = acts.begin()+5; it != acts.end(); ++it, index++) {
        if ((*it)->isVisible()) {
            QString viewnr = QString(QObject::tr("Restore view &%1")).arg(index);
            (*it)->setText(viewnr);
        }
    }
}


//===========================================================================
// Std_ToggleClipPlane
//===========================================================================

DEF_STD_CMD_AC(StdCmdToggleClipPlane)

StdCmdToggleClipPlane::StdCmdToggleClipPlane()
  : Command("Std_ToggleClipPlane")
{
    sGroup        = "Standard-View";
    sMenuText     = QT_TR_NOOP("Clipping plane");
    sToolTipText  = QT_TR_NOOP("Toggles clipping plane for active view");
    sWhatsThis    = "Std_ToggleClipPlane";
    sStatusTip    = QT_TR_NOOP("Toggles clipping plane for active view");
    sPixmap       = "Std_ToggleClipPlane";
    eType         = Alter3DView;
}

Action * StdCmdToggleClipPlane::createAction()
{
    Action *pcAction = Command::createAction();
    return pcAction;
}

void StdCmdToggleClipPlane::activated(int iMsg)
{
    Q_UNUSED(iMsg);
    static QPointer<Gui::Dialog::Clipping> clipping = nullptr;
    if (!clipping) {
        auto view = qobject_cast<View3DInventor*>(getMainWindow()->activeWindow());
        if (view) {
            clipping = Gui::Dialog::Clipping::makeDockWidget(view);
        }
    }
}

bool StdCmdToggleClipPlane::isActive()
{
    auto view = qobject_cast<View3DInventor*>(getMainWindow()->activeWindow());
    return view ? true : false;
}

//===========================================================================
// StdCmdDrawStyle
//===========================================================================
class StdCmdDrawStyle : public Gui::Command
{
public:
    StdCmdDrawStyle();
    ~StdCmdDrawStyle() override = default;
    void languageChange() override;
    const char* className() const override {return "StdCmdDrawStyle";}
    void updateIcon(const Gui::MDIView* view);
protected:
    void activated(int iMsg) override;
    bool isActive() override;
    Gui::Action * createAction() override;
};

StdCmdDrawStyle::StdCmdDrawStyle()
  : Command("Std_DrawStyle")
{
    sGroup        = "Standard-View";
    sMenuText     = QT_TR_NOOP("Draw style");
    sToolTipText  = QT_TR_NOOP("Change the draw style of the objects");
    sStatusTip    = QT_TR_NOOP("Change the draw style of the objects");
    sWhatsThis    = "Std_DrawStyle";
    sPixmap       = "DrawStyleAsIs";
    eType         = Alter3DView;

    this->getGuiApplication()->signalActivateView.connect([this](auto view) {
        this->updateIcon(view);
    });
}

Gui::Action * StdCmdDrawStyle::createAction()
{
    auto pcAction = new Gui::ActionGroup(this, Gui::getMainWindow());
    pcAction->setDropDownMenu(true);
    pcAction->setIsMode(true);
    applyCommandData(this->className(), pcAction);

    QAction* a0 = pcAction->addAction(QString());
    a0->setCheckable(true);
    a0->setIcon(BitmapFactory().iconFromTheme("DrawStyleAsIs"));
    a0->setChecked(true);
    a0->setObjectName(QString::fromLatin1("Std_DrawStyleAsIs"));
    a0->setShortcut(QKeySequence(QString::fromUtf8("V,1")));
    a0->setWhatsThis(QString::fromLatin1(getWhatsThis()));
    QAction* a1 = pcAction->addAction(QString());
    a1->setCheckable(true);
    a1->setIcon(BitmapFactory().iconFromTheme("DrawStylePoints"));
    a1->setObjectName(QString::fromLatin1("Std_DrawStylePoints"));
    a1->setShortcut(QKeySequence(QString::fromUtf8("V,2")));
    a1->setWhatsThis(QString::fromLatin1(getWhatsThis()));
    QAction* a2 = pcAction->addAction(QString());
    a2->setCheckable(true);
    a2->setIcon(BitmapFactory().iconFromTheme("DrawStyleWireFrame"));
    a2->setObjectName(QString::fromLatin1("Std_DrawStyleWireframe"));
    a2->setShortcut(QKeySequence(QString::fromUtf8("V,3")));
    a2->setWhatsThis(QString::fromLatin1(getWhatsThis()));
    QAction* a3 = pcAction->addAction(QString());
    a3->setCheckable(true);
    a3->setIcon(BitmapFactory().iconFromTheme("DrawStyleHiddenLine"));
    a3->setObjectName(QString::fromLatin1("Std_DrawStyleHiddenLine"));
    a3->setShortcut(QKeySequence(QString::fromUtf8("V,4")));
    a3->setWhatsThis(QString::fromLatin1(getWhatsThis()));
    QAction* a4 = pcAction->addAction(QString());
    a4->setCheckable(true);
    a4->setIcon(BitmapFactory().iconFromTheme("DrawStyleNoShading"));
    a4->setObjectName(QString::fromLatin1("Std_DrawStyleNoShading"));
    a4->setShortcut(QKeySequence(QString::fromUtf8("V,5")));
    a4->setWhatsThis(QString::fromLatin1(getWhatsThis()));
    QAction* a5 = pcAction->addAction(QString());
    a5->setCheckable(true);
    a5->setIcon(BitmapFactory().iconFromTheme("DrawStyleShaded"));
    a5->setObjectName(QString::fromLatin1("Std_DrawStyleShaded"));
    a5->setShortcut(QKeySequence(QString::fromUtf8("V,6")));
    a5->setWhatsThis(QString::fromLatin1(getWhatsThis()));
    QAction* a6 = pcAction->addAction(QString());
    a6->setCheckable(true);
    a6->setIcon(BitmapFactory().iconFromTheme("DrawStyleFlatLines"));
    a6->setObjectName(QString::fromLatin1("Std_DrawStyleFlatLines"));
    a6->setShortcut(QKeySequence(QString::fromUtf8("V,7")));
    a6->setWhatsThis(QString::fromLatin1(getWhatsThis()));

    pcAction->setIcon(a0->icon());

    _pcAction = pcAction;
    languageChange();
    return pcAction;
}

void StdCmdDrawStyle::languageChange()
{
    Command::languageChange();

    if (!_pcAction)
        return;
    auto pcAction = qobject_cast<Gui::ActionGroup*>(_pcAction);
    QList<QAction*> a = pcAction->actions();

    a[0]->setText(QCoreApplication::translate(
        "Std_DrawStyle", "As is"));
    a[0]->setToolTip(QCoreApplication::translate(
        "Std_DrawStyle", "Normal mode"));

    a[1]->setText(QCoreApplication::translate(
        "Std_DrawStyle", "Points"));
    a[1]->setToolTip(QCoreApplication::translate(
        "Std_DrawStyle", "Points mode"));

    a[2]->setText(QCoreApplication::translate(
        "Std_DrawStyle", "Wireframe"));
    a[2]->setToolTip(QCoreApplication::translate(
        "Std_DrawStyle", "Wireframe mode"));

    a[3]->setText(QCoreApplication::translate(
        "Std_DrawStyle", "Hidden line"));
    a[3]->setToolTip(QCoreApplication::translate(
        "Std_DrawStyle", "Hidden line mode"));

    a[4]->setText(QCoreApplication::translate(
        "Std_DrawStyle", "No shading"));
    a[4]->setToolTip(QCoreApplication::translate(
        "Std_DrawStyle", "No shading mode"));

    a[5]->setText(QCoreApplication::translate(
        "Std_DrawStyle", "Shaded"));
    a[5]->setToolTip(QCoreApplication::translate(
        "Std_DrawStyle", "Shaded mode"));

    a[6]->setText(QCoreApplication::translate(
        "Std_DrawStyle", "Flat lines"));
    a[6]->setToolTip(QCoreApplication::translate(
        "Std_DrawStyle", "Flat lines mode"));
}

void StdCmdDrawStyle::updateIcon(const MDIView *view)
{
    const auto view3d = dynamic_cast<const Gui::View3DInventor *>(view);
    if (!view3d)
        return;
    Gui::View3DInventorViewer *viewer = view3d->getViewer();
    if (!viewer)
        return;
    std::string mode(viewer->getOverrideMode());
    auto actionGroup = dynamic_cast<Gui::ActionGroup *>(_pcAction);
    if (!actionGroup)
        return;

    if (mode == "Point")
    {
        actionGroup->setCheckedAction(1);
        return;
    }
    if (mode == "Wireframe")
    {
        actionGroup->setCheckedAction(2);
        return;
    }
    if (mode == "Hidden Line")
    {
        actionGroup->setCheckedAction(3);
        return;
    }
    if (mode == "No shading")
    {
        actionGroup->setCheckedAction(4);
        return;
    }
    if (mode == "Shaded")
    {
        actionGroup->setCheckedAction(5);
        return;
    }
    if (mode == "Flat Lines")
    {
        actionGroup->setCheckedAction(6);
        return;
    }
    actionGroup->setCheckedAction(0);
}

void StdCmdDrawStyle::activated(int iMsg)
{
    Gui::Document *doc = this->getActiveGuiDocument();
    std::list<MDIView*> views = doc->getMDIViews();
    std::list<MDIView*>::iterator viewIt;
    bool oneChangedSignal(false);
    for (viewIt = views.begin(); viewIt != views.end(); ++viewIt)
    {
        auto view = qobject_cast<View3DInventor*>(*viewIt);
        if (view)
        {
            View3DInventorViewer* viewer;
            viewer = view->getViewer();
            if (viewer)
            {
                switch (iMsg)
                {
                case 1:
                    (oneChangedSignal) ? viewer->updateOverrideMode("Point") : viewer->setOverrideMode("Point");
                    break;
                case 2:
                    (oneChangedSignal) ? viewer->updateOverrideMode("Wireframe") : viewer->setOverrideMode("Wireframe");
                    break;
                case 3:
                    (oneChangedSignal) ? viewer->updateOverrideMode("Hidden Line") : viewer->setOverrideMode("Hidden Line");
                    break;
                case 4:
                    (oneChangedSignal) ? viewer->updateOverrideMode("No Shading") : viewer->setOverrideMode("No Shading");
                    break;
                case 5:
                    (oneChangedSignal) ? viewer->updateOverrideMode("Shaded") : viewer->setOverrideMode("Shaded");
                    break;
                case 6:
                    (oneChangedSignal) ? viewer->updateOverrideMode("Flat Lines") : viewer->setOverrideMode("Flat Lines");
                    break;
                default:
                    (oneChangedSignal) ? viewer->updateOverrideMode("As Is") : viewer->setOverrideMode("As Is");
                    break;
                }
                oneChangedSignal = true;
            }
        }
    }
}

bool StdCmdDrawStyle::isActive()
{
    return Gui::Application::Instance->activeDocument();
}

//===========================================================================
// Std_ToggleVisibility
//===========================================================================
DEF_STD_CMD_A(StdCmdToggleVisibility)

StdCmdToggleVisibility::StdCmdToggleVisibility()
  : Command("Std_ToggleVisibility")
{
    sGroup        = "Standard-View";
    sMenuText     = QT_TR_NOOP("Toggle visibility");
    sToolTipText  = QT_TR_NOOP("Toggles visibility");
    sStatusTip    = QT_TR_NOOP("Toggles visibility");
    sWhatsThis    = "Std_ToggleVisibility";
    sPixmap       = "Std_ToggleVisibility";
    sAccel        = "Space";
    eType         = Alter3DView;
}


void StdCmdToggleVisibility::activated(int iMsg)
{
    Q_UNUSED(iMsg);
    TransactionView transaction(getActiveGuiDocument(), QT_TRANSLATE_NOOP("Command", "Toggle visibility"));
    Selection().setVisible(SelectionSingleton::VisToggle);
}

bool StdCmdToggleVisibility::isActive()
{
    return (Gui::Selection().size() != 0);
}

//===========================================================================
// Std_ToggleSelectability
//===========================================================================
DEF_STD_CMD_A(StdCmdToggleSelectability)

StdCmdToggleSelectability::StdCmdToggleSelectability()
  : Command("Std_ToggleSelectability")
{
    sGroup        = "Standard-View";
    sMenuText     = QT_TR_NOOP("Toggle selectability");
    sToolTipText  = QT_TR_NOOP("Toggles the property of the objects to get selected in the 3D-View");
    sStatusTip    = QT_TR_NOOP("Toggles the property of the objects to get selected in the 3D-View");
    sWhatsThis    = "Std_ToggleSelectability";
    sPixmap       = "view-unselectable";
    eType         = Alter3DView;
}

void StdCmdToggleSelectability::activated(int iMsg)
{
    Q_UNUSED(iMsg);
    // go through all documents
    const std::vector<App::Document*> docs = App::GetApplication().getDocuments();
    for (const auto & doc : docs) {
        Document *pcDoc = Application::Instance->getDocument(doc);
        std::vector<App::DocumentObject*> sel = Selection().getObjectsOfType
            (App::DocumentObject::getClassTypeId(), doc->getName());

        if (sel.empty()) {
            continue;
        }

        TransactionView transaction(pcDoc, QT_TRANSLATE_NOOP("Command", "Toggle selectability"));

        for (const auto & ft : sel) {
            ViewProvider *pr = pcDoc->getViewProviderByName(ft->getNameInDocument());
            if (pr && pr->isDerivedFrom(ViewProviderGeometryObject::getClassTypeId())){
                if (static_cast<ViewProviderGeometryObject*>(pr)->Selectable.getValue())
                    doCommand(Gui,"Gui.getDocument(\"%s\").getObject(\"%s\").Selectable=False"
                                 , doc->getName(), ft->getNameInDocument());
                else
                    doCommand(Gui,"Gui.getDocument(\"%s\").getObject(\"%s\").Selectable=True"
                                 , doc->getName(), ft->getNameInDocument());
            }
        }
    }
}

bool StdCmdToggleSelectability::isActive()
{
    return (Gui::Selection().size() != 0);
}

//===========================================================================
// Std_ShowSelection
//===========================================================================
DEF_STD_CMD_A(StdCmdShowSelection)

StdCmdShowSelection::StdCmdShowSelection()
  : Command("Std_ShowSelection")
{
    sGroup        = "Standard-View";
    sMenuText     = QT_TR_NOOP("Show selection");
    sToolTipText  = QT_TR_NOOP("Show all selected objects");
    sStatusTip    = QT_TR_NOOP("Show all selected objects");
    sWhatsThis    = "Std_ShowSelection";
    sPixmap       = "Std_ShowSelection";
    eType         = Alter3DView;
}

void StdCmdShowSelection::activated(int iMsg)
{
    Q_UNUSED(iMsg);
    Selection().setVisible(SelectionSingleton::VisShow);
}

bool StdCmdShowSelection::isActive()
{
    return (Gui::Selection().size() != 0);
}

//===========================================================================
// Std_HideSelection
//===========================================================================
DEF_STD_CMD_A(StdCmdHideSelection)

StdCmdHideSelection::StdCmdHideSelection()
  : Command("Std_HideSelection")
{
    sGroup        = "Standard-View";
    sMenuText     = QT_TR_NOOP("Hide selection");
    sToolTipText  = QT_TR_NOOP("Hide all selected objects");
    sStatusTip    = QT_TR_NOOP("Hide all selected objects");
    sWhatsThis    = "Std_HideSelection";
    sPixmap       = "Std_HideSelection";
    eType         = Alter3DView;
}

void StdCmdHideSelection::activated(int iMsg)
{
    Q_UNUSED(iMsg);
    Selection().setVisible(SelectionSingleton::VisHide);
}

bool StdCmdHideSelection::isActive()
{
    return (Gui::Selection().size() != 0);
}

//===========================================================================
// Std_SelectVisibleObjects
//===========================================================================
DEF_STD_CMD_A(StdCmdSelectVisibleObjects)

StdCmdSelectVisibleObjects::StdCmdSelectVisibleObjects()
  : Command("Std_SelectVisibleObjects")
{
    sGroup        = "Standard-View";
    sMenuText     = QT_TR_NOOP("Select visible objects");
    sToolTipText  = QT_TR_NOOP("Select visible objects in the active document");
    sStatusTip    = QT_TR_NOOP("Select visible objects in the active document");
    sWhatsThis    = "Std_SelectVisibleObjects";
    sPixmap       = "Std_SelectVisibleObjects";
    eType         = Alter3DView;
}

void StdCmdSelectVisibleObjects::activated(int iMsg)
{
    Q_UNUSED(iMsg);
    // go through active document
    Gui::Document* doc = Application::Instance->activeDocument();
    App::Document* app = doc->getDocument();
    const std::vector<App::DocumentObject*> obj = app->getObjectsOfType
        (App::DocumentObject::getClassTypeId());

    std::vector<App::DocumentObject*> visible;
    visible.reserve(obj.size());
    for (const auto & it : obj) {
        if (doc->isShow(it->getNameInDocument()))
            visible.push_back(it);
    }

    SelectionSingleton& rSel = Selection();
    rSel.setSelection(app->getName(), visible);
}

bool StdCmdSelectVisibleObjects::isActive()
{
    return App::GetApplication().getActiveDocument();
}

//===========================================================================
// Std_ToggleObjects
//===========================================================================
DEF_STD_CMD_A(StdCmdToggleObjects)

StdCmdToggleObjects::StdCmdToggleObjects()
  : Command("Std_ToggleObjects")
{
    sGroup        = "Standard-View";
    sMenuText     = QT_TR_NOOP("Toggle all objects");
    sToolTipText  = QT_TR_NOOP("Toggles visibility of all objects in the active document");
    sStatusTip    = QT_TR_NOOP("Toggles visibility of all objects in the active document");
    sWhatsThis    = "Std_ToggleObjects";
    sPixmap       = "Std_ToggleObjects";
    eType         = Alter3DView;
}

void StdCmdToggleObjects::activated(int iMsg)
{
    Q_UNUSED(iMsg);
    // go through active document
    Gui::Document* doc = Application::Instance->activeDocument();
    App::Document* app = doc->getDocument();
    const std::vector<App::DocumentObject*> obj = app->getObjectsOfType
        (App::DocumentObject::getClassTypeId());

    for (const auto & it : obj) {
        if (doc->isShow(it->getNameInDocument()))
            doCommand(Gui,"Gui.getDocument(\"%s\").getObject(\"%s\").Visibility=False"
                         , app->getName(), it->getNameInDocument());
        else
            doCommand(Gui,"Gui.getDocument(\"%s\").getObject(\"%s\").Visibility=True"
                         , app->getName(), it->getNameInDocument());
    }
}

bool StdCmdToggleObjects::isActive()
{
    return App::GetApplication().getActiveDocument();
}

//===========================================================================
// Std_ShowObjects
//===========================================================================
DEF_STD_CMD_A(StdCmdShowObjects)

StdCmdShowObjects::StdCmdShowObjects()
  : Command("Std_ShowObjects")
{
    sGroup        = "Standard-View";
    sMenuText     = QT_TR_NOOP("Show all objects");
    sToolTipText  = QT_TR_NOOP("Show all objects in the document");
    sStatusTip    = QT_TR_NOOP("Show all objects in the document");
    sWhatsThis    = "Std_ShowObjects";
    sPixmap       = "Std_ShowObjects";
    eType         = Alter3DView;
}

void StdCmdShowObjects::activated(int iMsg)
{
    Q_UNUSED(iMsg);
    // go through active document
    Gui::Document* doc = Application::Instance->activeDocument();
    App::Document* app = doc->getDocument();
    const std::vector<App::DocumentObject*> obj = app->getObjectsOfType
        (App::DocumentObject::getClassTypeId());

    for (const auto & it : obj) {
        doCommand(Gui,"Gui.getDocument(\"%s\").getObject(\"%s\").Visibility=True"
                     , app->getName(), it->getNameInDocument());
    }
}

bool StdCmdShowObjects::isActive()
{
    return App::GetApplication().getActiveDocument();
}

//===========================================================================
// Std_HideObjects
//===========================================================================
DEF_STD_CMD_A(StdCmdHideObjects)

StdCmdHideObjects::StdCmdHideObjects()
  : Command("Std_HideObjects")
{
    sGroup        = "Standard-View";
    sMenuText     = QT_TR_NOOP("Hide all objects");
    sToolTipText  = QT_TR_NOOP("Hide all objects in the document");
    sStatusTip    = QT_TR_NOOP("Hide all objects in the document");
    sWhatsThis    = "Std_HideObjects";
    sPixmap       = "Std_HideObjects";
    eType         = Alter3DView;
}

void StdCmdHideObjects::activated(int iMsg)
{
    Q_UNUSED(iMsg);
    // go through active document
    Gui::Document* doc = Application::Instance->activeDocument();
    App::Document* app = doc->getDocument();
    const std::vector<App::DocumentObject*> obj = app->getObjectsOfType
        (App::DocumentObject::getClassTypeId());

    for (const auto & it : obj) {
        doCommand(Gui,"Gui.getDocument(\"%s\").getObject(\"%s\").Visibility=False"
                     , app->getName(), it->getNameInDocument());
    }
}

bool StdCmdHideObjects::isActive()
{
    return App::GetApplication().getActiveDocument();
}

//===========================================================================
// Std_SetAppearance
//===========================================================================
DEF_STD_CMD_A(StdCmdSetAppearance)

StdCmdSetAppearance::StdCmdSetAppearance()
  : Command("Std_SetAppearance")
{
    sGroup        = "Standard-View";
    sMenuText     = QT_TR_NOOP("Appearance...");
    sToolTipText  = QT_TR_NOOP("Sets the display properties of the selected object");
    sWhatsThis    = "Std_SetAppearance";
    sStatusTip    = QT_TR_NOOP("Sets the display properties of the selected object");
    sPixmap       = "Std_SetAppearance";
    sAccel        = "Ctrl+D";
    eType         = Alter3DView;
}

void StdCmdSetAppearance::activated(int iMsg)
{
    Q_UNUSED(iMsg);
    Gui::Control().showDialog(new Gui::Dialog::TaskDisplayProperties());
}

bool StdCmdSetAppearance::isActive()
{
    return (Gui::Control().activeDialog() == nullptr) &&
           (Gui::Selection().size() != 0);
}

//===========================================================================
// Std_ViewHome
//===========================================================================
DEF_3DV_CMD(StdCmdViewHome)

StdCmdViewHome::StdCmdViewHome()
  : Command("Std_ViewHome")
{
    sGroup        = "Standard-View";
    sMenuText     = QT_TR_NOOP("Home");
    sToolTipText  = QT_TR_NOOP("Set to default home view");
    sWhatsThis    = "Std_ViewHome";
    sStatusTip    = QT_TR_NOOP("Set to default home view");
    sPixmap       = "Std_ViewHome";
    sAccel        = "Home";
    eType         = Alter3DView;
}

void StdCmdViewHome::activated(int iMsg)
{
    Q_UNUSED(iMsg);

    auto hGrp = App::GetApplication().GetParameterGroupByPath("User parameter:BaseApp/Preferences/View");
    std::string default_view = hGrp->GetASCII("NewDocumentCameraOrientation","Top");
    doCommand(Command::Gui,"Gui.activeDocument().activeView().viewDefaultOrientation('%s',0)",default_view.c_str());
    doCommand(Command::Gui,"Gui.SendMsgToActiveView(\"ViewFit\")");
}

//===========================================================================
// Std_ViewBottom
//===========================================================================
DEF_3DV_CMD(StdCmdViewBottom)

StdCmdViewBottom::StdCmdViewBottom()
  : Command("Std_ViewBottom")
{
    sGroup        = "Standard-View";
    sMenuText     = QT_TR_NOOP("Bottom");
    sToolTipText  = QT_TR_NOOP("Set to bottom view");
    sWhatsThis    = "Std_ViewBottom";
    sStatusTip    = QT_TR_NOOP("Set to bottom view");
    sPixmap       = "view-bottom";
    sAccel        = "5";
    eType         = Alter3DView;
}

void StdCmdViewBottom::activated(int iMsg)
{
    Q_UNUSED(iMsg);
    doCommand(Command::Gui,"Gui.activeDocument().activeView().viewBottom()");
}

//===========================================================================
// Std_ViewFront
//===========================================================================
DEF_3DV_CMD(StdCmdViewFront)

StdCmdViewFront::StdCmdViewFront()
  : Command("Std_ViewFront")
{
    sGroup        = "Standard-View";
    sMenuText     = QT_TR_NOOP("Front");
    sToolTipText  = QT_TR_NOOP("Set to front view");
    sWhatsThis    = "Std_ViewFront";
    sStatusTip    = QT_TR_NOOP("Set to front view");
    sPixmap       = "view-front";
    sAccel        = "1";
    eType         = Alter3DView;
}

void StdCmdViewFront::activated(int iMsg)
{
    Q_UNUSED(iMsg);
    doCommand(Command::Gui,"Gui.activeDocument().activeView().viewFront()");
}

//===========================================================================
// Std_ViewLeft
//===========================================================================
DEF_3DV_CMD(StdCmdViewLeft)

StdCmdViewLeft::StdCmdViewLeft()
  : Command("Std_ViewLeft")
{
    sGroup        = "Standard-View";
    sMenuText     = QT_TR_NOOP("Left");
    sToolTipText  = QT_TR_NOOP("Set to left view");
    sWhatsThis    = "Std_ViewLeft";
    sStatusTip    = QT_TR_NOOP("Set to left view");
    sPixmap       = "view-left";
    sAccel        = "6";
    eType         = Alter3DView;
}

void StdCmdViewLeft::activated(int iMsg)
{
    Q_UNUSED(iMsg);
    doCommand(Command::Gui,"Gui.activeDocument().activeView().viewLeft()");
}

//===========================================================================
// Std_ViewRear
//===========================================================================
DEF_3DV_CMD(StdCmdViewRear)

StdCmdViewRear::StdCmdViewRear()
  : Command("Std_ViewRear")
{
    sGroup        = "Standard-View";
    sMenuText     = QT_TR_NOOP("Rear");
    sToolTipText  = QT_TR_NOOP("Set to rear view");
    sWhatsThis    = "Std_ViewRear";
    sStatusTip    = QT_TR_NOOP("Set to rear view");
    sPixmap       = "view-rear";
    sAccel        = "4";
    eType         = Alter3DView;
}

void StdCmdViewRear::activated(int iMsg)
{
    Q_UNUSED(iMsg);
    doCommand(Command::Gui,"Gui.activeDocument().activeView().viewRear()");
}

//===========================================================================
// Std_ViewRight
//===========================================================================
DEF_3DV_CMD(StdCmdViewRight)

StdCmdViewRight::StdCmdViewRight()
  : Command("Std_ViewRight")
{
    sGroup        = "Standard-View";
    sMenuText     = QT_TR_NOOP("Right");
    sToolTipText  = QT_TR_NOOP("Set to right view");
    sWhatsThis    = "Std_ViewRight";
    sStatusTip    = QT_TR_NOOP("Set to right view");
    sPixmap       = "view-right";
    sAccel        = "3";
    eType         = Alter3DView;
}

void StdCmdViewRight::activated(int iMsg)
{
    Q_UNUSED(iMsg);
    doCommand(Command::Gui,"Gui.activeDocument().activeView().viewRight()");
}

//===========================================================================
// Std_ViewTop
//===========================================================================
DEF_3DV_CMD(StdCmdViewTop)

StdCmdViewTop::StdCmdViewTop()
  : Command("Std_ViewTop")
{
    sGroup        = "Standard-View";
    sMenuText     = QT_TR_NOOP("Top");
    sToolTipText  = QT_TR_NOOP("Set to top view");
    sWhatsThis    = "Std_ViewTop";
    sStatusTip    = QT_TR_NOOP("Set to top view");
    sPixmap       = "view-top";
    sAccel        = "2";
    eType         = Alter3DView;
}

void StdCmdViewTop::activated(int iMsg)
{
    Q_UNUSED(iMsg);
    doCommand(Command::Gui,"Gui.activeDocument().activeView().viewTop()");
}


//===========================================================================
// Std_ViewIsometric
//===========================================================================
DEF_3DV_CMD(StdCmdViewIsometric)

StdCmdViewIsometric::StdCmdViewIsometric()
  : Command("Std_ViewIsometric")
{
    sGroup      = "Standard-View";
    sMenuText   = QT_TR_NOOP("Isometric");
    sToolTipText= QT_TR_NOOP("Set to isometric view");
    sWhatsThis  = "Std_ViewIsometric";
    sStatusTip  = QT_TR_NOOP("Set to isometric view");
    sPixmap     = "view-axonometric";
    sAccel      = "0";
    eType         = Alter3DView;
}

void StdCmdViewIsometric::activated(int iMsg)
{
    Q_UNUSED(iMsg);
    doCommand(Command::Gui,"Gui.activeDocument().activeView().viewIsometric()");
}

//===========================================================================
// Std_ViewDimetric
//===========================================================================
DEF_3DV_CMD(StdCmdViewDimetric)

StdCmdViewDimetric::StdCmdViewDimetric()
  : Command("Std_ViewDimetric")
{
    sGroup      = "Standard-View";
    sMenuText   = QT_TR_NOOP("Dimetric");
    sToolTipText= QT_TR_NOOP("Set to dimetric view");
    sWhatsThis  = "Std_ViewDimetric";
    sStatusTip  = QT_TR_NOOP("Set to dimetric view");
    sPixmap       = "Std_ViewDimetric";
    eType         = Alter3DView;
}

void StdCmdViewDimetric::activated(int iMsg)
{
    Q_UNUSED(iMsg);
    doCommand(Command::Gui,"Gui.activeDocument().activeView().viewDimetric()");
}

//===========================================================================
// Std_ViewTrimetric
//===========================================================================
DEF_3DV_CMD(StdCmdViewTrimetric)

StdCmdViewTrimetric::StdCmdViewTrimetric()
  : Command("Std_ViewTrimetric")
{
    sGroup      = "Standard-View";
    sMenuText   = QT_TR_NOOP("Trimetric");
    sToolTipText= QT_TR_NOOP("Set to trimetric view");
    sWhatsThis  = "Std_ViewTrimetric";
    sStatusTip  = QT_TR_NOOP("Set to trimetric view");
    sPixmap       = "Std_ViewTrimetric";
    eType         = Alter3DView;
}

void StdCmdViewTrimetric::activated(int iMsg)
{
    Q_UNUSED(iMsg);
    doCommand(Command::Gui,"Gui.activeDocument().activeView().viewTrimetric()");
}

//===========================================================================
// Std_ViewRotateLeft
//===========================================================================
DEF_3DV_CMD(StdCmdViewRotateLeft)

StdCmdViewRotateLeft::StdCmdViewRotateLeft()
  : Command("Std_ViewRotateLeft")
{
    sGroup        = "Standard-View";
    sMenuText     = QT_TR_NOOP("Rotate Left");
    sToolTipText  = QT_TR_NOOP("Rotate the view by 90\xc2\xb0 counter-clockwise");
    sWhatsThis    = "Std_ViewRotateLeft";
    sStatusTip    = QT_TR_NOOP("Rotate the view by 90\xc2\xb0 counter-clockwise");
    sPixmap       = "view-rotate-left";
    sAccel        = "Shift+Left";
    eType         = Alter3DView;
}

void StdCmdViewRotateLeft::activated(int iMsg)
{
    Q_UNUSED(iMsg);
    doCommand(Command::Gui,"Gui.activeDocument().activeView().viewRotateLeft()");
}


//===========================================================================
// Std_ViewRotateRight
//===========================================================================
DEF_3DV_CMD(StdCmdViewRotateRight)

StdCmdViewRotateRight::StdCmdViewRotateRight()
  : Command("Std_ViewRotateRight")
{
    sGroup        = "Standard-View";
    sMenuText     = QT_TR_NOOP("Rotate Right");
    sToolTipText  = QT_TR_NOOP("Rotate the view by 90\xc2\xb0 clockwise");
    sWhatsThis    = "Std_ViewRotateRight";
    sStatusTip    = QT_TR_NOOP("Rotate the view by 90\xc2\xb0 clockwise");
    sPixmap       = "view-rotate-right";
    sAccel        = "Shift+Right";
    eType         = Alter3DView;
}

void StdCmdViewRotateRight::activated(int iMsg)
{
    Q_UNUSED(iMsg);
    doCommand(Command::Gui,"Gui.activeDocument().activeView().viewRotateRight()");
}


//===========================================================================
// Std_ViewFitAll
//===========================================================================
DEF_STD_CMD_A(StdCmdViewFitAll)

StdCmdViewFitAll::StdCmdViewFitAll()
  : Command("Std_ViewFitAll")
{
    sGroup        = "Standard-View";
    sMenuText     = QT_TR_NOOP("Fit all");
    sToolTipText  = QT_TR_NOOP("Fits the whole content on the screen");
    sWhatsThis    = "Std_ViewFitAll";
    sStatusTip    = QT_TR_NOOP("Fits the whole content on the screen");
    sPixmap       = "zoom-all";
    sAccel        = "V, F";
    eType         = Alter3DView;
}

void StdCmdViewFitAll::activated(int iMsg)
{
    Q_UNUSED(iMsg);
    //doCommand(Command::Gui,"Gui.activeDocument().activeView().fitAll()");
    doCommand(Command::Gui,"Gui.SendMsgToActiveView(\"ViewFit\")");
}

bool StdCmdViewFitAll::isActive()
{
    //return isViewOfType(Gui::View3DInventor::getClassTypeId());
    return getGuiApplication()->sendHasMsgToActiveView("ViewFit");
}

//===========================================================================
// Std_ViewFitSelection
//===========================================================================
DEF_STD_CMD_A(StdCmdViewFitSelection)

StdCmdViewFitSelection::StdCmdViewFitSelection()
  : Command("Std_ViewFitSelection")
{
    sGroup        = "Standard-View";
    sMenuText     = QT_TR_NOOP("Fit selection");
    sToolTipText  = QT_TR_NOOP("Fits the selected content on the screen");
    sWhatsThis    = "Std_ViewFitSelection";
    sStatusTip    = QT_TR_NOOP("Fits the selected content on the screen");
    sAccel        = "V, S";
    sPixmap       = "zoom-selection";
    eType         = Alter3DView;
}

void StdCmdViewFitSelection::activated(int iMsg)
{
    Q_UNUSED(iMsg);
    doCommand(Command::Gui,"Gui.SendMsgToActiveView(\"ViewSelection\")");
}

bool StdCmdViewFitSelection::isActive()
{
  return getGuiApplication()->sendHasMsgToActiveView("ViewSelection");
}

//===========================================================================
// Std_ViewDock
//===========================================================================
DEF_STD_CMD_A(StdViewDock)

StdViewDock::StdViewDock()
  : Command("Std_ViewDock")
{
    sGroup       = "Standard-View";
    sMenuText    = QT_TR_NOOP("Docked");
    sToolTipText = QT_TR_NOOP("Display the active view either in fullscreen, in undocked or docked mode");
    sWhatsThis   = "Std_ViewDock";
    sStatusTip   = QT_TR_NOOP("Display the active view either in fullscreen, in undocked or docked mode");
    sAccel       = "V, D";
    eType        = Alter3DView;
    bCanLog       = false;
}

void StdViewDock::activated(int iMsg)
{
    Q_UNUSED(iMsg);
}

bool StdViewDock::isActive()
{
    MDIView* view = getMainWindow()->activeWindow();
    return (qobject_cast<View3DInventor*>(view) ? true : false);
}

//===========================================================================
// Std_ViewUndock
//===========================================================================
DEF_STD_CMD_A(StdViewUndock)

StdViewUndock::StdViewUndock()
  : Command("Std_ViewUndock")
{
    sGroup       = "Standard-View";
    sMenuText    = QT_TR_NOOP("Undocked");
    sToolTipText = QT_TR_NOOP("Display the active view either in fullscreen, in undocked or docked mode");
    sWhatsThis   = "Std_ViewUndock";
    sStatusTip   = QT_TR_NOOP("Display the active view either in fullscreen, in undocked or docked mode");
    sAccel       = "V, U";
    eType        = Alter3DView;
    bCanLog       = false;
}

void StdViewUndock::activated(int iMsg)
{
    Q_UNUSED(iMsg);
}

bool StdViewUndock::isActive()
{
    MDIView* view = getMainWindow()->activeWindow();
    return (qobject_cast<View3DInventor*>(view) ? true : false);
}

//===========================================================================
// Std_MainFullscreen
//===========================================================================
DEF_STD_CMD(StdMainFullscreen)

StdMainFullscreen::StdMainFullscreen()
  : Command("Std_MainFullscreen")
{
    sGroup       = "Standard-View";
    sMenuText    = QT_TR_NOOP("Fullscreen");
    sToolTipText = QT_TR_NOOP("Display the main window in fullscreen mode");
    sWhatsThis   = "Std_MainFullscreen";
    sStatusTip   = QT_TR_NOOP("Display the main window in fullscreen mode");
    sPixmap      = "view-fullscreen";
    sAccel       = "Alt+F11";
    eType        = Alter3DView;
}

void StdMainFullscreen::activated(int iMsg)
{
    Q_UNUSED(iMsg);
    MDIView* view = getMainWindow()->activeWindow();

    if (view)
        view->setCurrentViewMode(MDIView::Child);

    if (getMainWindow()->isFullScreen())
        getMainWindow()->showNormal();
    else
        getMainWindow()->showFullScreen();
}

//===========================================================================
// Std_ViewFullscreen
//===========================================================================
DEF_STD_CMD_A(StdViewFullscreen)

StdViewFullscreen::StdViewFullscreen()
  : Command("Std_ViewFullscreen")
{
    sGroup       = "Standard-View";
    sMenuText    = QT_TR_NOOP("Fullscreen");
    sToolTipText = QT_TR_NOOP("Display the active view either in fullscreen, in undocked or docked mode");
    sWhatsThis   = "Std_ViewFullscreen";
    sStatusTip   = QT_TR_NOOP("Display the active view either in fullscreen, in undocked or docked mode");
    sPixmap      = "view-fullscreen";
    sAccel       = "F11";
    eType        = Alter3DView;
    bCanLog       = false;
}

void StdViewFullscreen::activated(int iMsg)
{
    Q_UNUSED(iMsg);
}

bool StdViewFullscreen::isActive()
{
    MDIView* view = getMainWindow()->activeWindow();
    return (qobject_cast<View3DInventor*>(view) ? true : false);
}

//===========================================================================
// Std_ViewDockUndockFullscreen
//===========================================================================
DEF_STD_CMD_AC(StdViewDockUndockFullscreen)

StdViewDockUndockFullscreen::StdViewDockUndockFullscreen()
  : Command("Std_ViewDockUndockFullscreen")
{
    sGroup       = "Standard-View";
    sMenuText    = QT_TR_NOOP("Document window");
    sToolTipText = QT_TR_NOOP("Display the active view either in fullscreen, in undocked or docked mode");
    sWhatsThis   = "Std_ViewDockUndockFullscreen";
    sStatusTip   = QT_TR_NOOP("Display the active view either in fullscreen, in undocked or docked mode");
    eType        = Alter3DView;

    CommandManager &rcCmdMgr = Application::Instance->commandManager();
    rcCmdMgr.addCommand(new StdViewDock());
    rcCmdMgr.addCommand(new StdViewUndock());
    rcCmdMgr.addCommand(new StdViewFullscreen());
}

Action * StdViewDockUndockFullscreen::createAction()
{
    auto pcAction = new ActionGroup(this, getMainWindow());
    pcAction->setDropDownMenu(true);
    pcAction->setText(QCoreApplication::translate(
        this->className(), getMenuText()));

    CommandManager &rcCmdMgr = Application::Instance->commandManager();
    Command* cmdD = rcCmdMgr.getCommandByName("Std_ViewDock");
    Command* cmdU = rcCmdMgr.getCommandByName("Std_ViewUndock");
    Command* cmdF = rcCmdMgr.getCommandByName("Std_ViewFullscreen");
    cmdD->addToGroup(pcAction, true);
    cmdU->addToGroup(pcAction, true);
    cmdF->addToGroup(pcAction, true);

    return pcAction;
}

void StdViewDockUndockFullscreen::activated(int iMsg)
{
    // Check if main window is in fullscreen mode.
    if (getMainWindow()->isFullScreen())
        getMainWindow()->showNormal();

    MDIView* view = getMainWindow()->activeWindow();
    if (!view) // no active view
        return;

    // nothing to do when the view is docked and 'Docked' is pressed
    if (iMsg == 0 && view->currentViewMode() == MDIView::Child)
        return;
    // Change the view mode after an mdi view was already visible doesn't
    // work well with Qt5 any more because of some strange OpenGL behaviour.
    // A workaround is to clone the mdi view, set its view mode and delete
    // the original view.
    Gui::Document* doc = Gui::Application::Instance->activeDocument();
    if (doc) {
        Gui::MDIView* clone = doc->cloneView(view);
        if (!clone)
            return;

        const char* ppReturn = nullptr;
        if (view->onMsg("GetCamera", &ppReturn)) {
            std::string sMsg = "SetCamera ";
            sMsg += ppReturn;

            const char** pReturnIgnore=nullptr;
            clone->onMsg(sMsg.c_str(), pReturnIgnore);
        }

        if (iMsg==0) {
            getMainWindow()->addWindow(clone);
        }
        else if (iMsg==1) {
            if (view->currentViewMode() == MDIView::TopLevel)
                getMainWindow()->addWindow(clone);
            else
                clone->setCurrentViewMode(MDIView::TopLevel);
        }
        else if (iMsg==2) {
            if (view->currentViewMode() == MDIView::FullScreen)
                getMainWindow()->addWindow(clone);
            else
                clone->setCurrentViewMode(MDIView::FullScreen);
        }
        // destroy the old view
        view->deleteSelf();
    }
}

bool StdViewDockUndockFullscreen::isActive()
{
    MDIView* view = getMainWindow()->activeWindow();
    if (qobject_cast<View3DInventor*>(view)) {
        // update the action group if needed
        auto pActGrp = qobject_cast<ActionGroup*>(_pcAction);
        if (pActGrp) {
            int index = pActGrp->checkedAction();
            int mode = (int)(view->currentViewMode());
            if (index != mode) {
                // active window has changed with another view mode
                pActGrp->setCheckedAction(mode);
            }
        }

        return true;
    }

    return false;
}


//===========================================================================
// Std_ViewVR
//===========================================================================
DEF_STD_CMD_A(StdCmdViewVR)

StdCmdViewVR::StdCmdViewVR()
  : Command("Std_ViewVR")
{
    sGroup        = "Standard-View";
    sMenuText     = QT_TR_NOOP("FreeCAD-VR");
    sToolTipText  = QT_TR_NOOP("Extend the FreeCAD 3D Window to a Oculus Rift");
    sWhatsThis    = "Std_ViewVR";
    sStatusTip    = QT_TR_NOOP("Extend the FreeCAD 3D Window to a Oculus Rift");
    eType         = Alter3DView;
}

void StdCmdViewVR::activated(int iMsg)
{
   Q_UNUSED(iMsg);
   doCommand(Command::Gui,"Gui.SendMsgToActiveView(\"ViewVR\")");
}

bool StdCmdViewVR::isActive()
{
   return getGuiApplication()->sendHasMsgToActiveView("ViewVR");
}



//===========================================================================
// Std_ViewScreenShot
//===========================================================================
DEF_STD_CMD_A(StdViewScreenShot)

StdViewScreenShot::StdViewScreenShot()
  : Command("Std_ViewScreenShot")
{
    sGroup      = "Standard-View";
    sMenuText   = QT_TR_NOOP("Save image...");
    sToolTipText= QT_TR_NOOP("Creates a screenshot of the active view");
    sWhatsThis  = "Std_ViewScreenShot";
    sStatusTip  = QT_TR_NOOP("Creates a screenshot of the active view");
    sPixmap     = "camera-photo";
    eType       = Alter3DView;
}

void StdViewScreenShot::activated(int iMsg)
{
    Q_UNUSED(iMsg);
    auto view = qobject_cast<View3DInventor*>(getMainWindow()->activeWindow());
    if (view) {
        QStringList formats;
        SbViewportRegion vp(view->getViewer()->getSoRenderManager()->getViewportRegion());
        {
            SoQtOffscreenRenderer rd(vp);
            formats = rd.getWriteImageFiletypeInfo();
        }

        Base::Reference<ParameterGrp> hExt = App::GetApplication().GetUserParameter().GetGroup("BaseApp")
                                   ->GetGroup("Preferences")->GetGroup("General");
        QString ext = QString::fromLatin1(hExt->GetASCII("OffscreenImageFormat").c_str());
        int backtype = hExt->GetInt("OffscreenImageBackground", 0);

        Base::Reference<ParameterGrp> methodGrp = App::GetApplication().GetParameterGroupByPath
            ("User parameter:BaseApp/Preferences/View");
        QByteArray method = methodGrp->GetASCII("SavePicture").c_str();

        QStringList filter;
        QString selFilter;
        for (QStringList::Iterator it = formats.begin(); it != formats.end(); ++it) {
            filter << QString::fromLatin1("%1 %2 (*.%3)").arg((*it).toUpper(),
                QObject::tr("files"), (*it).toLower());
            if (ext == *it)
                selFilter = filter.last();
        }

        FileOptionsDialog fd(getMainWindow(), Qt::WindowFlags());
        fd.setFileMode(QFileDialog::AnyFile);
        fd.setAcceptMode(QFileDialog::AcceptSave);
        fd.setWindowTitle(QObject::tr("Save image"));
        fd.setNameFilters(filter);
        if (!selFilter.isEmpty())
            fd.selectNameFilter(selFilter);

        // create the image options widget
        auto opt = new DlgSettingsImageImp(&fd);
        SbVec2s sz = vp.getWindowSize();
        opt->setImageSize((int)sz[0], (int)sz[1]);
        opt->setBackgroundType(backtype);
        opt->setMethod(method);

        fd.setOptionsWidget(FileOptionsDialog::ExtensionRight, opt);
        fd.setOption(QFileDialog::DontConfirmOverwrite, false);
        opt->onSelectedFilter(fd.selectedNameFilter());
        QObject::connect(&fd, &FileOptionsDialog::filterSelected,
                         opt, &DlgSettingsImageImp::onSelectedFilter);

        if (fd.exec() == QDialog::Accepted) {
            selFilter = fd.selectedNameFilter();
            QString fn = fd.selectedFiles().constFirst();
            // We must convert '\' path separators to '/' before otherwise
            // Python would interpret them as escape sequences.
            fn.replace(QLatin1Char('\\'), QLatin1Char('/'));

            Gui::WaitCursor wc;

            // get the defined values
            int w = opt->imageWidth();
            int h = opt->imageHeight();

            // search for the matching format
            QString format = formats.front(); // take the first as default
            for (QStringList::Iterator it = formats.begin(); it != formats.end(); ++it) {
                if (selFilter.startsWith((*it).toUpper())) {
                    format = *it;
                    break;
                }
            }

            hExt->SetASCII("OffscreenImageFormat", (const char*)format.toLatin1());

            method = opt->method();
            methodGrp->SetASCII("SavePicture", method.constData());

            // which background chosen
            const char* background;
            switch (opt->backgroundType()) {
            case 0:  background = "Current"; break;
            case 1:  background = "White"; break;
            case 2:  background = "Black"; break;
            case 3:  background = "Transparent"; break;
            default: background = "Current"; break;
            }
            hExt->SetInt("OffscreenImageBackground", opt->backgroundType());

            QString comment = opt->comment();
            if (!comment.isEmpty()) {
                // Replace newline escape sequence through '\\n' string to build one big string,
                // otherwise Python would interpret it as an invalid command.
                // Python does the decoding for us.
#if QT_VERSION >= QT_VERSION_CHECK(5,15,0)
                QStringList lines = comment.split(QLatin1String("\n"), Qt::KeepEmptyParts);
#else
                QStringList lines = comment.split(QLatin1String("\n"), QString::KeepEmptyParts);
#endif
                comment = lines.join(QLatin1String("\\n"));
                doCommand(Gui, "Gui.activeDocument().activeView().saveImage('%s',%d,%d,'%s','%s')",
                          fn.toUtf8().constData(), w, h, background, comment.toUtf8().constData());
            }
            else {
                doCommand(Gui, "Gui.activeDocument().activeView().saveImage('%s',%d,%d,'%s')",
                          fn.toUtf8().constData(), w, h, background);
            }

            // When adding a watermark check if the image could be created
            if (opt->addWatermark()) {
                QFileInfo fi(fn);
                QPixmap pixmap;
                if (fi.exists() && pixmap.load(fn)) {
                    QString name = qApp->applicationName();
                    std::map<std::string, std::string>& config = App::Application::Config();
                    QString url = QString::fromLatin1(config["MaintainerUrl"].c_str());
                    url = QUrl(url).host();

                    QPixmap appicon = Gui::BitmapFactory().pixmap(config["AppIcon"].c_str());

                    QPainter painter;
                    painter.begin(&pixmap);

                    painter.drawPixmap(8, h - 15 - appicon.height(), appicon);

                    QFont font = painter.font();
                    font.setPointSize(20);

                    QFontMetrics fm(font);
                    int n = QtTools::horizontalAdvance(fm, name);
                    int h = pixmap.height();

                    painter.setFont(font);
                    painter.drawText(8 + appicon.width(), h - 24, name);

                    font.setPointSize(12);
                    QFontMetrics fm2(font);
                    int u = QtTools::horizontalAdvance(fm2, url);
                    painter.setFont(font);
                    painter.drawText(8 + appicon.width() + n - u, h - 6, url);

                    painter.end();
                    pixmap.save(fn);
                }
            }
        }
    }
}

bool StdViewScreenShot::isActive()
{
    return isViewOfType(Gui::View3DInventor::getClassTypeId());
}

//===========================================================================
// Std_ViewLoadImage
//===========================================================================
DEF_STD_CMD(StdViewLoadImage)

StdViewLoadImage::StdViewLoadImage()
  : Command("Std_ViewLoadImage")
{
    sGroup      = "Standard-View";
    sMenuText   = QT_TR_NOOP("Load image...");
    sToolTipText= QT_TR_NOOP("Loads an image");
    sWhatsThis  = "Std_ViewLoadImage";
    sStatusTip  = QT_TR_NOOP("Loads an image");
    sPixmap     = "image-open";
    eType       = 0;
}

void StdViewLoadImage::activated(int iMsg)
{
    Q_UNUSED(iMsg);

    // add all supported QImage formats
    QStringList mimeTypeFilters;
    QList<QByteArray> supportedMimeTypes = QImageReader::supportedMimeTypes();
    for (const auto& mimeTypeName : supportedMimeTypes) {
        mimeTypeFilters.append(QString::fromLatin1(mimeTypeName));
    }

    // Reading an image
    QFileDialog dialog(Gui::getMainWindow());
    dialog.setWindowTitle(QObject::tr("Choose an image file to open"));
    dialog.setMimeTypeFilters(mimeTypeFilters);
    dialog.selectMimeTypeFilter(QString::fromLatin1("image/png"));
    dialog.setDefaultSuffix(QString::fromLatin1("png"));
    dialog.setAcceptMode(QFileDialog::AcceptOpen);
    dialog.setOption(QFileDialog::DontUseNativeDialog);

    if (dialog.exec()) {
        QString fileName = dialog.selectedFiles().constFirst();
        ImageView* view = new ImageView(Gui::getMainWindow());
        view->loadFile(fileName);
        view->resize(400, 300);
        Gui::getMainWindow()->addWindow(view);
    }
}

//===========================================================================
// Std_ViewCreate
//===========================================================================
DEF_STD_CMD_A(StdCmdViewCreate)

StdCmdViewCreate::StdCmdViewCreate()
  : Command("Std_ViewCreate")
{
    sGroup      = "Standard-View";
    sMenuText   = QT_TR_NOOP("Create new view");
    sToolTipText= QT_TR_NOOP("Creates a new view window for the active document");
    sWhatsThis  = "Std_ViewCreate";
    sStatusTip  = QT_TR_NOOP("Creates a new view window for the active document");
    sPixmap     = "window-new";
    eType         = Alter3DView;
}

void StdCmdViewCreate::activated(int iMsg)
{
    Q_UNUSED(iMsg);
    getActiveGuiDocument()->createView(View3DInventor::getClassTypeId());
    getActiveGuiDocument()->getActiveView()->viewAll();
}

bool StdCmdViewCreate::isActive()
{
    return (getActiveGuiDocument() != nullptr);
}

//===========================================================================
// Std_ToggleNavigation
//===========================================================================
DEF_STD_CMD_A(StdCmdToggleNavigation)

StdCmdToggleNavigation::StdCmdToggleNavigation()
  : Command("Std_ToggleNavigation")
{
    sGroup        = "Standard-View";
    sMenuText     = QT_TR_NOOP("Toggle navigation/Edit mode");
    sToolTipText  = QT_TR_NOOP("Toggle between navigation and edit mode");
    sStatusTip    = QT_TR_NOOP("Toggle between navigation and edit mode");
    sWhatsThis    = "Std_ToggleNavigation";
  //iAccel        = Qt::SHIFT+Qt::Key_Space;
    sAccel        = "Esc";
    sPixmap       = "Std_ToggleNavigation";
    eType         = Alter3DView;
}

void StdCmdToggleNavigation::activated(int iMsg)
{
    Q_UNUSED(iMsg);
    Gui::MDIView* view = Gui::getMainWindow()->activeWindow();
    if (view && view->isDerivedFrom(Gui::View3DInventor::getClassTypeId())) {
        Gui::View3DInventorViewer* viewer = static_cast<Gui::View3DInventor*>(view)->getViewer();
        SbBool toggle = viewer->isRedirectedToSceneGraph();
        viewer->setRedirectToSceneGraph(!toggle);
    }
}

bool StdCmdToggleNavigation::isActive()
{
    //#0001087: Inventor Navigation continues with released Mouse Button
    //This happens because 'Esc' is also used to close the task dialog.
    //Add also new method 'isRedirectToSceneGraphEnabled' to explicitly
    //check if this is allowed.
    if (Gui::Control().activeDialog())
        return false;
    Gui::MDIView* view = Gui::getMainWindow()->activeWindow();
    if (view && view->isDerivedFrom(Gui::View3DInventor::getClassTypeId())) {
        Gui::View3DInventorViewer* viewer = static_cast<Gui::View3DInventor*>(view)->getViewer();
        return viewer->isEditing() && viewer->isRedirectToSceneGraphEnabled();
    }
    return false;
}




//===========================================================================
// Std_ViewExample1
//===========================================================================
DEF_STD_CMD_A(StdCmdAxisCross)

StdCmdAxisCross::StdCmdAxisCross()
  : Command("Std_AxisCross")
{
        sGroup        = "Standard-View";
        sMenuText     = QT_TR_NOOP("Toggle axis cross");
        sToolTipText  = QT_TR_NOOP("Turns on or off the axis cross at the origin");
        sStatusTip    = QT_TR_NOOP("Turns on or off the axis cross at the origin");
        sWhatsThis    = "Std_AxisCross";
        sPixmap       = "Std_AxisCross";
        sAccel        = "A,C";
}

void StdCmdAxisCross::activated(int iMsg)
{
    Q_UNUSED(iMsg);
    auto view = qobject_cast<View3DInventor*>(Gui::getMainWindow()->activeWindow());
    if (view) {
        if (!view->getViewer()->hasAxisCross())
            doCommand(Command::Gui,"Gui.ActiveDocument.ActiveView.setAxisCross(True)");
        else
            doCommand(Command::Gui,"Gui.ActiveDocument.ActiveView.setAxisCross(False)");
    }
}

bool StdCmdAxisCross::isActive()
{
    auto view = qobject_cast<View3DInventor*>(Gui::getMainWindow()->activeWindow());
    if (view && view->getViewer()->hasAxisCross()) {
        if (!_pcAction->isChecked())
            _pcAction->setChecked(true);
    }
    else {
        if (_pcAction->isChecked())
            _pcAction->setChecked(false);
    }
    if (view)
        return true;
    return false;

}

//===========================================================================
// Std_ViewExample1
//===========================================================================
DEF_STD_CMD_A(StdCmdViewExample1)

StdCmdViewExample1::StdCmdViewExample1()
  : Command("Std_ViewExample1")
{
    sGroup        = "Standard-View";
    sMenuText     = QT_TR_NOOP("Inventor example #1");
    sToolTipText  = QT_TR_NOOP("Shows a 3D texture with manipulator");
    sWhatsThis    = "Std_ViewExample1";
    sStatusTip    = QT_TR_NOOP("Shows a 3D texture with manipulator");
    sPixmap       = "Std_Tool1";
    eType         = Alter3DView;
}

void StdCmdViewExample1::activated(int iMsg)
{
    Q_UNUSED(iMsg);
    doCommand(Command::Gui,"Gui.SendMsgToActiveView(\"Example1\")");
}

bool StdCmdViewExample1::isActive()
{
    return getGuiApplication()->sendHasMsgToActiveView("Example1");
}

//===========================================================================
// Std_ViewExample2
//===========================================================================
DEF_STD_CMD_A(StdCmdViewExample2)

StdCmdViewExample2::StdCmdViewExample2()
  : Command("Std_ViewExample2")
{
    sGroup        = "Standard-View";
    sMenuText     = QT_TR_NOOP("Inventor example #2");
    sToolTipText  = QT_TR_NOOP("Shows spheres and drag-lights");
    sWhatsThis    = "Std_ViewExample2";
    sStatusTip    = QT_TR_NOOP("Shows spheres and drag-lights");
    sPixmap       = "Std_Tool2";
    eType         = Alter3DView;
}

void StdCmdViewExample2::activated(int iMsg)
{
    Q_UNUSED(iMsg);
    doCommand(Command::Gui,"Gui.SendMsgToActiveView(\"Example2\")");
}

bool StdCmdViewExample2::isActive()
{
    return getGuiApplication()->sendHasMsgToActiveView("Example2");
}

//===========================================================================
// Std_ViewExample3
//===========================================================================
DEF_STD_CMD_A(StdCmdViewExample3)

StdCmdViewExample3::StdCmdViewExample3()
  : Command("Std_ViewExample3")
{
    sGroup        = "Standard-View";
    sMenuText     = QT_TR_NOOP("Inventor example #3");
    sToolTipText  = QT_TR_NOOP("Shows a animated texture");
    sWhatsThis    = "Std_ViewExample3";
    sStatusTip    = QT_TR_NOOP("Shows a animated texture");
    sPixmap       = "Std_Tool3";
    eType         = Alter3DView;
}

void StdCmdViewExample3::activated(int iMsg)
{
    Q_UNUSED(iMsg);
    doCommand(Command::Gui,"Gui.SendMsgToActiveView(\"Example3\")");
}

bool StdCmdViewExample3::isActive()
{
    return getGuiApplication()->sendHasMsgToActiveView("Example3");
}


//===========================================================================
// Std_ViewIvStereoOff
//===========================================================================
DEF_STD_CMD_A(StdCmdViewIvStereoOff)

StdCmdViewIvStereoOff::StdCmdViewIvStereoOff()
  : Command("Std_ViewIvStereoOff")
{
    sGroup        = "Standard-View";
    sMenuText     = QT_TR_NOOP("Stereo Off");
    sToolTipText  = QT_TR_NOOP("Switch stereo viewing off");
    sWhatsThis    = "Std_ViewIvStereoOff";
    sStatusTip    = QT_TR_NOOP("Switch stereo viewing off");
    sPixmap       = "Std_ViewIvStereoOff";
    eType         = Alter3DView;
}

void StdCmdViewIvStereoOff::activated(int iMsg)
{
    Q_UNUSED(iMsg);
    doCommand(Command::Gui,"Gui.activeDocument().activeView().setStereoType(\"Mono\")");
}

bool StdCmdViewIvStereoOff::isActive()
{
    return getGuiApplication()->sendHasMsgToActiveView("SetStereoOff");
}


//===========================================================================
// Std_ViewIvStereoRedGreen
//===========================================================================
DEF_STD_CMD_A(StdCmdViewIvStereoRedGreen)

StdCmdViewIvStereoRedGreen::StdCmdViewIvStereoRedGreen()
  : Command("Std_ViewIvStereoRedGreen")
{
    sGroup        = "Standard-View";
    sMenuText     = QT_TR_NOOP("Stereo red/cyan");
    sToolTipText  = QT_TR_NOOP("Switch stereo viewing to red/cyan");
    sWhatsThis    = "Std_ViewIvStereoRedGreen";
    sStatusTip    = QT_TR_NOOP("Switch stereo viewing to red/cyan");
    sPixmap       = "Std_ViewIvStereoRedGreen";
    eType         = Alter3DView;
}

void StdCmdViewIvStereoRedGreen::activated(int iMsg)
{
    Q_UNUSED(iMsg);
    doCommand(Command::Gui,"Gui.activeDocument().activeView().setStereoType(\"Anaglyph\")");
}

bool StdCmdViewIvStereoRedGreen::isActive()
{
    return getGuiApplication()->sendHasMsgToActiveView("SetStereoRedGreen");
}

//===========================================================================
// Std_ViewIvStereoQuadBuff
//===========================================================================
DEF_STD_CMD_A(StdCmdViewIvStereoQuadBuff)

StdCmdViewIvStereoQuadBuff::StdCmdViewIvStereoQuadBuff()
  : Command("Std_ViewIvStereoQuadBuff")
{
    sGroup        = "Standard-View";
    sMenuText     = QT_TR_NOOP("Stereo quad buffer");
    sToolTipText  = QT_TR_NOOP("Switch stereo viewing to quad buffer");
    sWhatsThis    = "Std_ViewIvStereoQuadBuff";
    sStatusTip    = QT_TR_NOOP("Switch stereo viewing to quad buffer");
    sPixmap       = "Std_ViewIvStereoQuadBuff";
    eType         = Alter3DView;
}

void StdCmdViewIvStereoQuadBuff::activated(int iMsg)
{
    Q_UNUSED(iMsg);
    doCommand(Command::Gui,"Gui.activeDocument().activeView().setStereoType(\"QuadBuffer\")");
}

bool StdCmdViewIvStereoQuadBuff::isActive()
{
    return getGuiApplication()->sendHasMsgToActiveView("SetStereoQuadBuff");
}

//===========================================================================
// Std_ViewIvStereoInterleavedRows
//===========================================================================
DEF_STD_CMD_A(StdCmdViewIvStereoInterleavedRows)

StdCmdViewIvStereoInterleavedRows::StdCmdViewIvStereoInterleavedRows()
  : Command("Std_ViewIvStereoInterleavedRows")
{
    sGroup        = "Standard-View";
    sMenuText     = QT_TR_NOOP("Stereo Interleaved Rows");
    sToolTipText  = QT_TR_NOOP("Switch stereo viewing to Interleaved Rows");
    sWhatsThis    = "Std_ViewIvStereoInterleavedRows";
    sStatusTip    = QT_TR_NOOP("Switch stereo viewing to Interleaved Rows");
    sPixmap       = "Std_ViewIvStereoInterleavedRows";
    eType         = Alter3DView;
}

void StdCmdViewIvStereoInterleavedRows::activated(int iMsg)
{
    Q_UNUSED(iMsg);
    doCommand(Command::Gui,"Gui.activeDocument().activeView().setStereoType(\"InterleavedRows\")");
}

bool StdCmdViewIvStereoInterleavedRows::isActive()
{
    return getGuiApplication()->sendHasMsgToActiveView("SetStereoInterleavedRows");
}

//===========================================================================
// Std_ViewIvStereoInterleavedColumns
//===========================================================================
DEF_STD_CMD_A(StdCmdViewIvStereoInterleavedColumns)

StdCmdViewIvStereoInterleavedColumns::StdCmdViewIvStereoInterleavedColumns()
  : Command("Std_ViewIvStereoInterleavedColumns")
{
    sGroup        = "Standard-View";
    sMenuText     = QT_TR_NOOP("Stereo Interleaved Columns");
    sToolTipText  = QT_TR_NOOP("Switch stereo viewing to Interleaved Columns");
    sWhatsThis    = "Std_ViewIvStereoInterleavedColumns";
    sStatusTip    = QT_TR_NOOP("Switch stereo viewing to Interleaved Columns");
    sPixmap       = "Std_ViewIvStereoInterleavedColumns";
    eType         = Alter3DView;
}

void StdCmdViewIvStereoInterleavedColumns::activated(int iMsg)
{
    Q_UNUSED(iMsg);
    doCommand(Command::Gui,"Gui.activeDocument().activeView().setStereoType(\"InterleavedColumns\")");
}

bool StdCmdViewIvStereoInterleavedColumns::isActive()
{
    return getGuiApplication()->sendHasMsgToActiveView("SetStereoInterleavedColumns");
}


//===========================================================================
// Std_ViewIvIssueCamPos
//===========================================================================
DEF_STD_CMD_A(StdCmdViewIvIssueCamPos)

StdCmdViewIvIssueCamPos::StdCmdViewIvIssueCamPos()
  : Command("Std_ViewIvIssueCamPos")
{
    sGroup        = "Standard-View";
    sMenuText     = QT_TR_NOOP("Issue camera position");
    sToolTipText  = QT_TR_NOOP("Issue the camera position to the console and to a macro, to easily recall this position");
    sWhatsThis    = "Std_ViewIvIssueCamPos";
    sStatusTip    = QT_TR_NOOP("Issue the camera position to the console and to a macro, to easily recall this position");
    sPixmap       = "Std_ViewIvIssueCamPos";
    eType         = Alter3DView;
}

void StdCmdViewIvIssueCamPos::activated(int iMsg)
{
    Q_UNUSED(iMsg);
    std::string Temp,Temp2;
    std::string::size_type pos;

    const char* ppReturn=nullptr;
    getGuiApplication()->sendMsgToActiveView("GetCamera",&ppReturn);

    // remove the #inventor line...
    Temp2 = ppReturn;
    pos = Temp2.find_first_of("\n");
    Temp2.erase(0,pos);

    // remove all returns
    while((pos=Temp2.find('\n')) != std::string::npos)
        Temp2.replace(pos,1," ");

    // build up the command string
    Temp += "Gui.SendMsgToActiveView(\"SetCamera ";
    Temp += Temp2;
    Temp += "\")";

    Base::Console().Message("%s\n",Temp2.c_str());
    getGuiApplication()->macroManager()->addLine(MacroManager::Gui,Temp.c_str());
}

bool StdCmdViewIvIssueCamPos::isActive()
{
    return getGuiApplication()->sendHasMsgToActiveView("GetCamera");
}


//===========================================================================
// Std_ViewZoomIn
//===========================================================================
DEF_STD_CMD_A(StdViewZoomIn)

StdViewZoomIn::StdViewZoomIn()
  : Command("Std_ViewZoomIn")
{
    sGroup        = "Standard-View";
    sMenuText     = QT_TR_NOOP("Zoom In");
    sToolTipText  = QT_TR_NOOP("Increase the zoom factor by a fixed amount");
    sWhatsThis    = "Std_ViewZoomIn";
    sStatusTip    = QT_TR_NOOP("Increase the zoom factor by a fixed amount");
    sPixmap       = "zoom-in";
    sAccel        = keySequenceToAccel(QKeySequence::ZoomIn);
    eType         = Alter3DView;
}

void StdViewZoomIn::activated(int iMsg)
{
    Q_UNUSED(iMsg);
    getGuiApplication()->sendMsgToFocusView("ZoomIn");
}

bool StdViewZoomIn::isActive()
{
    return getGuiApplication()->sendHasMsgToActiveView("ZoomIn");
}

//===========================================================================
// Std_ViewZoomOut
//===========================================================================
DEF_STD_CMD_A(StdViewZoomOut)

StdViewZoomOut::StdViewZoomOut()
  : Command("Std_ViewZoomOut")
{
    sGroup        = "Standard-View";
    sMenuText     = QT_TR_NOOP("Zoom Out");
    sToolTipText  = QT_TR_NOOP("Decrease the zoom factor by a fixed amount");
    sWhatsThis    = "Std_ViewZoomOut";
    sStatusTip    = QT_TR_NOOP("Decrease the zoom factor by a fixed amount");
    sPixmap       = "zoom-out";
    sAccel        = keySequenceToAccel(QKeySequence::ZoomOut);
    eType         = Alter3DView;
}

void StdViewZoomOut::activated(int iMsg)
{
    Q_UNUSED(iMsg);
    getGuiApplication()->sendMsgToFocusView("ZoomOut");
}

bool StdViewZoomOut::isActive()
{
    return getGuiApplication()->sendHasMsgToActiveView("ZoomOut");
}

namespace {
class SelectionCallbackHandler {

private:
    static std::unique_ptr<SelectionCallbackHandler> currentSelectionHandler;
    QCursor prevSelectionCursor;
    using FnCb = void (*)(void * userdata, SoEventCallback * node);
    FnCb fnCb;
    void* userData;
    bool prevSelectionEn;

public:
    // Creates a selection handler used to implement the common behaviour of BoxZoom, BoxSelection and BoxElementSelection. 
    // Takes the viewer, a selection mode, a cursor, a function pointer to be called on success and a void pointer for user data to be passed to the given function.
    // The selection handler class stores all necessary previous states, registers a event callback and starts the selection in the given mode.    
    // If there is still a selection handler active, this call will generate a message and returns.
    static void Create(View3DInventorViewer* viewer, View3DInventorViewer::SelectionMode selectionMode,
                       const QCursor& cursor, FnCb doFunction= nullptr, void* ud=nullptr)
    {
        if (currentSelectionHandler)
        {
            Base::Console().Message("SelectionCallbackHandler: A selection handler already active.");
            return;
        }

        currentSelectionHandler = std::make_unique<SelectionCallbackHandler>();
        if (viewer)
        {
            currentSelectionHandler->userData = ud;
            currentSelectionHandler->fnCb = doFunction;
            currentSelectionHandler->prevSelectionCursor = viewer->cursor();
            viewer->setEditingCursor(cursor);
            viewer->addEventCallback(SoEvent::getClassTypeId(),
                SelectionCallbackHandler::selectionCallback, currentSelectionHandler.get());
            currentSelectionHandler->prevSelectionEn = viewer->isSelectionEnabled();
            viewer->setSelectionEnabled(false);
            viewer->startSelection(selectionMode);
        }
    }

    void* getUserData() const {
        return userData;
    }

    // Implements the event handler. In the normal case the provided function is called. 
    // Also supports aborting the selection mode by pressing (releasing) the Escape key. 
    static void selectionCallback(void * ud, SoEventCallback * n)
    {
        auto selectionHandler = static_cast<SelectionCallbackHandler*>(ud);
        auto view = static_cast<Gui::View3DInventorViewer*>(n->getUserData());
        const SoEvent* ev = n->getEvent();
        if (ev->isOfType(SoKeyboardEvent::getClassTypeId())) {

            n->setHandled();
            n->getAction()->setHandled();

            const auto ke = static_cast<const SoKeyboardEvent*>(ev);
            const SbBool press = ke->getState() == SoButtonEvent::DOWN ? true : false;
            if (ke->getKey() == SoKeyboardEvent::ESCAPE) {

                if (!press) {
                    view->abortSelection();
                    restoreState(selectionHandler, view);
                }
            }
        }
        else if (ev->isOfType(SoMouseButtonEvent::getClassTypeId())) {
            const auto mbe = static_cast<const SoMouseButtonEvent*>(ev);

            // Mark all incoming mouse button events as handled, especially, to deactivate the selection node
            n->getAction()->setHandled();

            if (mbe->getButton() == SoMouseButtonEvent::BUTTON1 && mbe->getState() == SoButtonEvent::UP)
            {
                if (selectionHandler && selectionHandler->fnCb)
                    selectionHandler->fnCb(selectionHandler->getUserData(), n);
                restoreState(selectionHandler, view);
            }
            // No other mouse events available from Coin3D to implement right mouse up abort
        }
    }

    static void restoreState(SelectionCallbackHandler * selectionHandler, View3DInventorViewer* view)
    {
        if (selectionHandler)
        {
            selectionHandler->fnCb = nullptr;
            view->setEditingCursor(selectionHandler->prevSelectionCursor);
            view->removeEventCallback(SoEvent::getClassTypeId(), SelectionCallbackHandler::selectionCallback, selectionHandler);
            view->setSelectionEnabled(selectionHandler->prevSelectionEn);
        }
        Application::Instance->commandManager().testActive();
        currentSelectionHandler = nullptr;
    }
};
}

std::unique_ptr<SelectionCallbackHandler> SelectionCallbackHandler::currentSelectionHandler = std::unique_ptr<SelectionCallbackHandler>();
//===========================================================================
// Std_ViewBoxZoom
//===========================================================================
/* XPM */
static const char * cursor_box_zoom[] = {
"32 32 3 1",
" 	c None",
".	c #FFFFFF",
"@	c #FF0000",
"      .                         ",
"      .                         ",
"      .                         ",
"      .                         ",
"      .                         ",
"                                ",
".....   .....                   ",
"                                ",
"      .      @@@@@@@            ",
"      .    @@@@@@@@@@@          ",
"      .   @@         @@         ",
"      .  @@. . . . . .@@        ",
"      .  @             @        ",
"        @@ .         . @@       ",
"        @@             @@       ",
"        @@ .         . @@       ",
"        @@             @@       ",
"        @@ .         . @@       ",
"        @@             @@       ",
"        @@ .         . @@       ",
"         @             @        ",
"         @@. . . . . .@@@       ",
"          @@          @@@@      ",
"           @@@@@@@@@@@@  @@     ",
"             @@@@@@@ @@   @@    ",
"                      @@   @@   ",
"                       @@   @@  ",
"                        @@   @@ ",
"                         @@  @@ ",
"                          @@@@  ",
"                           @@   ",
"                                " };

DEF_3DV_CMD(StdViewBoxZoom)

StdViewBoxZoom::StdViewBoxZoom()
  : Command("Std_ViewBoxZoom")
{
    sGroup        = "Standard-View";
    sMenuText     = QT_TR_NOOP("Box zoom");
    sToolTipText  = QT_TR_NOOP("Activate the box zoom tool");
    sWhatsThis    = "Std_ViewBoxZoom";
    sStatusTip    = QT_TR_NOOP("Activate the box zoom tool");
    sPixmap       = "zoom-border";
    sAccel        = "Ctrl+B";
    eType         = Alter3DView;
}

void StdViewBoxZoom::activated(int iMsg)
{
    Q_UNUSED(iMsg);
    auto view = qobject_cast<View3DInventor*>(getMainWindow()->activeWindow());
    if ( view ) {
        View3DInventorViewer* viewer = view->getViewer();
        if (!viewer->isSelecting()) {
            SelectionCallbackHandler::Create(viewer, View3DInventorViewer::BoxZoom, QCursor(QPixmap(cursor_box_zoom), 7, 7));
        }
    }
}

//===========================================================================
// Std_BoxSelection
//===========================================================================
DEF_3DV_CMD(StdBoxSelection)

/* XPM */
static const char * cursor_box_select[] = {
"32 32 4 1",
" 	c None",
".	c #FFFFFF",
"+	c #FF0000",
"@	c #000000",
"      .                         ",
"      .                         ",
"      .                         ",
"      .                         ",
"      .                         ",
"                                ",
".....   .....                   ",
"                                ",
"      .                         ",
"      .                         ",
"      .   +    +++   +++    +++ ",
"      .   +@@                   ",
"      .   +@.@@@                ",
"            @...@@@             ",
"            @......@@           ",
"            @........@@@      + ",
"             @..........@@    + ",
"          +  @............@   + ",
"          +   @........@@@      ",
"          +   @.......@         ",
"              @........@        ",
"               @........@     + ",
"               @...@.....@    + ",
"          +    @..@ @.....@   + ",
"          +     @.@  @.....@    ",
"          +     @.@   @.....@   ",
"                 @     @.....@  ",
"                        @...@   ",
"                         @.@  + ",
"                          @   + ",
"          +++    +++   +++    + ",
"                                " };

StdBoxSelection::StdBoxSelection()
  : Command("Std_BoxSelection")
{
    sGroup        = "Standard-View";
    sMenuText     = QT_TR_NOOP("Box selection");
    sToolTipText  = QT_TR_NOOP("Activate the box selection tool");
    sWhatsThis    = "Std_BoxSelection";
    sStatusTip    = QT_TR_NOOP("Activate the box selection tool");
    sPixmap       = "edit-select-box";
    sAccel        = "Shift+B";
    eType         = AlterSelection;
}

using SelectionMode = enum { CENTER, INTERSECT };

static std::vector<std::string> getBoxSelection(
        ViewProviderDocumentObject *vp, SelectionMode mode, bool selectElement,
        const Base::ViewProjMethod &proj, const Base::Polygon2d &polygon,
        const Base::Matrix4D &mat, bool transform=true, int depth=0)
{
    std::vector<std::string> ret;
    auto obj = vp->getObject();
    if(!obj || !obj->getNameInDocument())
        return ret;

    // DO NOT check this view object Visibility, let the caller do this. Because
    // we may be called by upper object hierarchy that manages our visibility.

    auto bbox3 = vp->getBoundingBox(nullptr,transform);
    if(!bbox3.IsValid())
        return ret;

    auto bbox = bbox3.Transformed(mat).ProjectBox(&proj);

    // check if both two boundary points are inside polygon, only
    // valid since we know the given polygon is a box.
    if(polygon.Contains(Base::Vector2d(bbox.MinX,bbox.MinY)) &&
       polygon.Contains(Base::Vector2d(bbox.MaxX,bbox.MaxY)))
    {
        ret.emplace_back("");
        return ret;
    }

    if(!bbox.Intersect(polygon))
        return ret;

    const auto &subs = obj->getSubObjects(App::DocumentObject::GS_SELECT);
    if(subs.empty()) {
        if(!selectElement) {
            if(mode==INTERSECT || polygon.Contains(bbox.GetCenter()))
                ret.emplace_back("");
            return ret;
        }
        Base::PyGILStateLocker lock;
        PyObject *pyobj = nullptr;
        Base::Matrix4D matCopy(mat);
        obj->getSubObject(nullptr,&pyobj,&matCopy,transform,depth);
        if(!pyobj)
            return ret;
        Py::Object pyobject(pyobj,true);
        if(!PyObject_TypeCheck(pyobj,&Data::ComplexGeoDataPy::Type))
            return ret;
        auto data = static_cast<Data::ComplexGeoDataPy*>(pyobj)->getComplexGeoDataPtr();
        for(auto type : data->getElementTypes()) {
            size_t count = data->countSubElements(type);
            if(!count)
                continue;
            for(size_t i=1;i<=count;++i) {
                std::string element(type);
                element += std::to_string(i);
                std::unique_ptr<Data::Segment> segment(data->getSubElementByName(element.c_str()));
                if(!segment)
                    continue;
                std::vector<Base::Vector3d> points;
                std::vector<Data::ComplexGeoData::Line> lines;
                data->getLinesFromSubElement(segment.get(),points,lines);
                if(lines.empty()) {
                    if(points.empty())
                        continue;
                    auto v = proj(points[0]);
                    if(polygon.Contains(Base::Vector2d(v.x,v.y)))
                        ret.push_back(element);
                    continue;
                }
                Base::Polygon2d loop;
                // TODO: can we assume the line returned above are in proper
                // order if the element is a face?
                auto v = proj(points[lines.front().I1]);
                loop.Add(Base::Vector2d(v.x,v.y));
                for(auto &line : lines) {
                    for(auto i=line.I1;i<line.I2;++i) {
                        auto v = proj(points[i+1]);
                        loop.Add(Base::Vector2d(v.x,v.y));
                    }
                }
                if(!polygon.Intersect(loop))
                    continue;
                if(mode==CENTER && !polygon.Contains(loop.CalcBoundBox().GetCenter()))
                    continue;
                ret.push_back(element);
            }
            break;
        }
        return ret;
    }

    size_t count = 0;
    for(auto &sub : subs) {
        App::DocumentObject *parent = nullptr;
        std::string childName;
        Base::Matrix4D smat(mat);
        auto sobj = obj->resolve(sub.c_str(),&parent,&childName,nullptr,nullptr,&smat,transform,depth+1);
        if(!sobj)
            continue;
        int vis;
        if(!parent || (vis=parent->isElementVisible(childName.c_str()))<0)
            vis = sobj->Visibility.getValue()?1:0;

        if(!vis)
            continue;

        auto svp = dynamic_cast<ViewProviderDocumentObject*>(Application::Instance->getViewProvider(sobj));
        if(!svp)
            continue;

        const auto &sels = getBoxSelection(svp,mode,selectElement,proj,polygon,smat,false,depth+1);
        if(sels.size()==1 && sels[0].empty())
            ++count;
        for(auto &sel : sels)
            ret.emplace_back(sub+sel);
    }
    if(count==subs.size()) {
        ret.resize(1);
        ret[0].clear();
    }
    return ret;
}

static void doSelect(void* ud, SoEventCallback * cb)
{
    bool selectElement = ud ? true : false;
    auto viewer = static_cast<Gui::View3DInventorViewer*>(cb->getUserData());

    viewer->setSelectionEnabled(true);

    SelectionMode selectionMode = CENTER;

    std::vector<SbVec2f> picked = viewer->getGLPolygon();
    SoCamera* cam = viewer->getSoRenderManager()->getCamera();
    SbViewVolume vv = cam->getViewVolume();
    Gui::ViewVolumeProjection proj(vv);
    Base::Polygon2d polygon;
    if (picked.size() == 2) {
        SbVec2f pt1 = picked[0];
        SbVec2f pt2 = picked[1];
        polygon.Add(Base::Vector2d(pt1[0], pt1[1]));
        polygon.Add(Base::Vector2d(pt1[0], pt2[1]));
        polygon.Add(Base::Vector2d(pt2[0], pt2[1]));
        polygon.Add(Base::Vector2d(pt2[0], pt1[1]));

        // when selecting from right to left then select by intersection
        // otherwise if the center is inside the rectangle
        if (picked[0][0] > picked[1][0])
            selectionMode = INTERSECT;
    }
    else {
        for (const auto & it : picked)
            polygon.Add(Base::Vector2d(it[0],it[1]));
    }

    App::Document* doc = App::GetApplication().getActiveDocument();
    if (doc) {
        cb->setHandled();

        const SoEvent* ev = cb->getEvent();
        if (ev && !ev->wasCtrlDown()) {
            Gui::Selection().clearSelection(doc->getName());
        }

        for(auto obj : doc->getObjects()) {
            if(App::GeoFeatureGroupExtension::getGroupOfObject(obj))
                continue;

            auto vp = dynamic_cast<ViewProviderDocumentObject*>(Application::Instance->getViewProvider(obj));
            if (!vp || !vp->isVisible())
                continue;

            Base::Matrix4D mat;
            for(auto &sub : getBoxSelection(vp,selectionMode,selectElement,proj,polygon,mat))
                Gui::Selection().addSelection(doc->getName(), obj->getNameInDocument(), sub.c_str());
        }
    }
}

void StdBoxSelection::activated(int iMsg)
{
    Q_UNUSED(iMsg);
    auto view = qobject_cast<View3DInventor*>(getMainWindow()->activeWindow());
    if (view) {
        View3DInventorViewer* viewer = view->getViewer();
        if (!viewer->isSelecting()) {
            // #0002931: Box select misbehaves with touchpad navigation style
            // Notify the navigation style to cleanup internal states
            int mode = viewer->navigationStyle()->getViewingMode();
            if (mode != Gui::NavigationStyle::IDLE) {
                SoKeyboardEvent ev;
                viewer->navigationStyle()->processEvent(&ev);
            }

            SelectionCallbackHandler::Create(viewer, View3DInventorViewer::Rubberband, QCursor(QPixmap(cursor_box_select), 7, 7), doSelect, nullptr);
            viewer->setSelectionEnabled(false);
        }
    }
}

//===========================================================================
// Std_BoxElementSelection
//===========================================================================
/* XPM */
static const char * cursor_box_element_select[] = {
"32 32 6 1",
" 	c None",
".	c #FFFFFF",
"+	c #00FF1B",
"@	c #19A428",
"#	c #FF0000",
"$	c #000000",
"      .                         ",
"      .                         ",
"      .                         ",
"      .                         ",
"      .                         ",
"                                ",
".....   .....                   ",
"       ++++++++++++             ",
"      .+@@@@@@@@@@+             ",
"      .+@@@@@@@@@@+             ",
"      .+@@#@@@@###+  ###    ### ",
"      .+@@#$$@@@@@+             ",
"      .+@@#$.$$$@@+             ",
"       +@@@@$...$$$             ",
"       +@@@@$......$$           ",
"       +@@@@$........$$$      # ",
"       +@@@@@$..........$$    # ",
"       +@@#@@$............$   # ",
"       +++#+++$........$$$      ",
"          #   $.......$         ",
"              $........$        ",
"               $........$     # ",
"               $...$.....$    # ",
"          #    $..$ $.....$   # ",
"          #     $.$  $.....$    ",
"          #     $.$   $.....$   ",
"                 $     $.....$  ",
"                        $...$   ",
"                         $.$  # ",
"                          $   # ",
"          ###    ###   ###    # ",
"                                " };

DEF_3DV_CMD(StdBoxElementSelection)

StdBoxElementSelection::StdBoxElementSelection()
  : Command("Std_BoxElementSelection")
{
    sGroup        = "Standard-View";
    sMenuText     = QT_TR_NOOP("Box element selection");
    sToolTipText  = QT_TR_NOOP("Box element selection");
    sWhatsThis    = "Std_BoxElementSelection";
    sStatusTip    = QT_TR_NOOP("Box element selection");
    sPixmap       = "edit-element-select-box";
    sAccel        = "Shift+E";
    eType         = AlterSelection;
}

void StdBoxElementSelection::activated(int iMsg)
{
    Q_UNUSED(iMsg);
    auto view = qobject_cast<View3DInventor*>(getMainWindow()->activeWindow());
    if (view) {
        View3DInventorViewer* viewer = view->getViewer();
        if (!viewer->isSelecting()) {
            // #0002931: Box select misbehaves with touchpad navigation style
            // Notify the navigation style to cleanup internal states
            int mode = viewer->navigationStyle()->getViewingMode();
            if (mode != Gui::NavigationStyle::IDLE) {
                SoKeyboardEvent ev;
                viewer->navigationStyle()->processEvent(&ev);
            }

            SelectionCallbackHandler::Create(viewer, View3DInventorViewer::Rubberband, QCursor(QPixmap(cursor_box_element_select), 7, 7), doSelect, this);
            viewer->setSelectionEnabled(false);
        }
    }
}


//===========================================================================
// Std_TreeSelection
//===========================================================================

DEF_STD_CMD(StdTreeSelection)

StdTreeSelection::StdTreeSelection()
  : Command("Std_TreeSelection")
{
    sGroup        = "TreeView";
    sMenuText     = QT_TR_NOOP("Go to selection");
    sToolTipText  = QT_TR_NOOP("Scroll to first selected item");
    sWhatsThis    = "Std_TreeSelection";
    sStatusTip    = QT_TR_NOOP("Scroll to first selected item");
    eType         = Alter3DView;
    sPixmap       = "tree-goto-sel";
    sAccel        = "T,G";
}

void StdTreeSelection::activated(int iMsg)
{
    Q_UNUSED(iMsg);
    TreeWidget::scrollItemToTop();
}

//===========================================================================
// Std_TreeCollapse
//===========================================================================

DEF_STD_CMD(StdCmdTreeCollapse)

StdCmdTreeCollapse::StdCmdTreeCollapse()
  : Command("Std_TreeCollapse")
{
    sGroup        = "View";
    sMenuText     = QT_TR_NOOP("Collapse selected item");
    sToolTipText  = QT_TR_NOOP("Collapse currently selected tree items");
    sWhatsThis    = "Std_TreeCollapse";
    sStatusTip    = QT_TR_NOOP("Collapse currently selected tree items");
    eType         = Alter3DView;
}

void StdCmdTreeCollapse::activated(int iMsg)
{
    Q_UNUSED(iMsg);
    QList<TreeWidget*> tree = Gui::getMainWindow()->findChildren<TreeWidget*>();
    for (QList<TreeWidget*>::iterator it = tree.begin(); it != tree.end(); ++it)
        (*it)->expandSelectedItems(TreeItemMode::CollapseItem);
}

//===========================================================================
// Std_TreeExpand
//===========================================================================

DEF_STD_CMD(StdCmdTreeExpand)

StdCmdTreeExpand::StdCmdTreeExpand()
  : Command("Std_TreeExpand")
{
    sGroup        = "View";
    sMenuText     = QT_TR_NOOP("Expand selected item");
    sToolTipText  = QT_TR_NOOP("Expand currently selected tree items");
    sWhatsThis    = "Std_TreeExpand";
    sStatusTip    = QT_TR_NOOP("Expand currently selected tree items");
    eType         = Alter3DView;
}

void StdCmdTreeExpand::activated(int iMsg)
{
    Q_UNUSED(iMsg);
    QList<TreeWidget*> tree = Gui::getMainWindow()->findChildren<TreeWidget*>();
    for (QList<TreeWidget*>::iterator it = tree.begin(); it != tree.end(); ++it)
        (*it)->expandSelectedItems(TreeItemMode::ExpandItem);
}

//===========================================================================
// Std_TreeSelectAllInstance
//===========================================================================

DEF_STD_CMD_A(StdCmdTreeSelectAllInstances)

StdCmdTreeSelectAllInstances::StdCmdTreeSelectAllInstances()
  : Command("Std_TreeSelectAllInstances")
{
    sGroup        = "View";
    sMenuText     = QT_TR_NOOP("Select all instances");
    sToolTipText  = QT_TR_NOOP("Select all instances of the current selected object");
    sWhatsThis    = "Std_TreeSelectAllInstances";
    sStatusTip    = QT_TR_NOOP("Select all instances of the current selected object");
    sPixmap       = "sel-instance";
    eType         = AlterSelection;
}

bool StdCmdTreeSelectAllInstances::isActive()
{
    const auto &sels = Selection().getSelectionEx("*",App::DocumentObject::getClassTypeId(), ResolveMode::OldStyleElement, true);
    if(sels.empty())
        return false;
    auto obj = sels[0].getObject();
    if(!obj || !obj->getNameInDocument())
        return false;
    return dynamic_cast<ViewProviderDocumentObject*>(
            Application::Instance->getViewProvider(obj)) != nullptr;
}

void StdCmdTreeSelectAllInstances::activated(int iMsg)
{
    Q_UNUSED(iMsg);
    const auto &sels = Selection().getSelectionEx("*",App::DocumentObject::getClassTypeId(), ResolveMode::OldStyleElement, true);
    if(sels.empty())
        return;
    auto obj = sels[0].getObject();
    if(!obj || !obj->getNameInDocument())
        return;
    auto vpd = dynamic_cast<ViewProviderDocumentObject*>(
            Application::Instance->getViewProvider(obj));
    if(!vpd)
        return;
    Selection().selStackPush();
    Selection().clearCompleteSelection();
    const auto trees = getMainWindow()->findChildren<TreeWidget*>();
    for(auto tree : trees)
        tree->selectAllInstances(*vpd);
    Selection().selStackPush();
}

//===========================================================================
// Std_MeasureDistance
//===========================================================================

DEF_STD_CMD_A(StdCmdMeasureDistance)

StdCmdMeasureDistance::StdCmdMeasureDistance()
  : Command("Std_MeasureDistance")
{
    sGroup        = "View";
    sMenuText     = QT_TR_NOOP("Measure distance");
    sToolTipText  = QT_TR_NOOP("Activate the distance measurement tool");
    sWhatsThis    = "Std_MeasureDistance";
    sStatusTip    = QT_TR_NOOP("Activate the distance measurement tool");
    sPixmap       = "view-measurement";
    eType         = Alter3DView;
}

// Yay for cheezy drawings!
/* XPM */
static const char * cursor_ruler[] = {
"32 32 3 1",
" 	c None",
".	c #FFFFFF",
"+	c #FF0000",
"      .                         ",
"      .                         ",
"      .                         ",
"      .                         ",
"      .                         ",
"                                ",
".....   .....                   ",
"                                ",
"      .                         ",
"      .                         ",
"      .        ++               ",
"      .       +  +              ",
"      .      +   ++             ",
"            +   +  +            ",
"           +   +    +           ",
"          +   +     ++          ",
"          +        +  +         ",
"           +           +        ",
"            +         + +       ",
"             +       +   +      ",
"              +           +     ",
"               +         + +    ",
"                +       +   +   ",
"                 +           +  ",
"                  +         + + ",
"                   +       +  ++",
"                    +     +   + ",
"                     +       +  ",
"                      +     +   ",
"                       +   +    ",
"                        + +     ",
"                         +      "};
void StdCmdMeasureDistance::activated(int iMsg)
{
    Q_UNUSED(iMsg);
    Gui::Document* doc = Gui::Application::Instance->activeDocument();
    auto view = static_cast<Gui::View3DInventor*>(doc->getActiveView());
    if (view) {
        Gui::View3DInventorViewer* viewer = view->getViewer();
        viewer->setEditing(true);
        viewer->setEditingCursor(QCursor(QPixmap(cursor_ruler), 7, 7));

        // Derives from QObject and we have a parent object, so we don't
        // require a delete.
        auto marker = new PointMarker(viewer);
        viewer->addEventCallback(SoEvent::getClassTypeId(),
            ViewProviderMeasureDistance::measureDistanceCallback, marker);
     }
}

bool StdCmdMeasureDistance::isActive()
{
    App::Document* doc = App::GetApplication().getActiveDocument();
    if (!doc || doc->countObjectsOfType(App::GeoFeature::getClassTypeId()) == 0)
        return false;

    Gui::MDIView* view = Gui::getMainWindow()->activeWindow();
    if (view && view->isDerivedFrom(Gui::View3DInventor::getClassTypeId())) {
        Gui::View3DInventorViewer* viewer = static_cast<Gui::View3DInventor*>(view)->getViewer();
        return !viewer->isEditing();
    }

    return false;
}

//===========================================================================
// Std_SceneInspector
//===========================================================================

DEF_3DV_CMD(StdCmdSceneInspector)

StdCmdSceneInspector::StdCmdSceneInspector()
  : Command("Std_SceneInspector")
{
    // setting the
    sGroup        = "Tools";
    sMenuText     = QT_TR_NOOP("Scene inspector...");
    sToolTipText  = QT_TR_NOOP("Scene inspector");
    sWhatsThis    = "Std_SceneInspector";
    sStatusTip    = QT_TR_NOOP("Scene inspector");
    eType         = Alter3DView;
    sPixmap       = "Std_SceneInspector";
}

void StdCmdSceneInspector::activated(int iMsg)
{
    Q_UNUSED(iMsg);
    Gui::Document* doc = Application::Instance->activeDocument();
    if (doc) {
        static QPointer<Gui::Dialog::DlgInspector> dlg = nullptr;
        if (!dlg)
            dlg = new Gui::Dialog::DlgInspector(getMainWindow());
        dlg->setDocument(doc);
        dlg->setAttribute(Qt::WA_DeleteOnClose);
        dlg->show();
    }
}

//===========================================================================
// Std_TextureMapping
//===========================================================================

DEF_STD_CMD_A(StdCmdTextureMapping)

StdCmdTextureMapping::StdCmdTextureMapping()
  : Command("Std_TextureMapping")
{
    // setting the
    sGroup        = "Tools";
    sMenuText     = QT_TR_NOOP("Texture mapping...");
    sToolTipText  = QT_TR_NOOP("Texture mapping");
    sWhatsThis    = "Std_TextureMapping";
    sStatusTip    = QT_TR_NOOP("Texture mapping");
    sPixmap       = "Std_TextureMapping";
    eType         = Alter3DView;
}

void StdCmdTextureMapping::activated(int iMsg)
{
    Q_UNUSED(iMsg);
    Gui::Control().showDialog(new Gui::Dialog::TaskTextureMapping);
}

bool StdCmdTextureMapping::isActive()
{
    Gui::MDIView* view = getMainWindow()->activeWindow();
    return view && view->isDerivedFrom(Gui::View3DInventor::getClassTypeId())
                && (!(Gui::Control().activeDialog()));
}

DEF_STD_CMD(StdCmdDemoMode)

StdCmdDemoMode::StdCmdDemoMode()
  : Command("Std_DemoMode")
{
    sGroup        = "Standard-View";
    sMenuText     = QT_TR_NOOP("View turntable...");
    sToolTipText  = QT_TR_NOOP("View turntable");
    sWhatsThis    = "Std_DemoMode";
    sStatusTip    = QT_TR_NOOP("View turntable");
    eType         = Alter3DView;
    sPixmap       = "Std_DemoMode";
}

void StdCmdDemoMode::activated(int iMsg)
{
    Q_UNUSED(iMsg);
    static QPointer<QDialog> dlg = nullptr;
    if (!dlg)
        dlg = new Gui::Dialog::DemoMode(getMainWindow());
    dlg->setAttribute(Qt::WA_DeleteOnClose);
    dlg->show();
}

//===========================================================================
// Part_Measure_Clear_All
//===========================================================================

DEF_STD_CMD(CmdViewMeasureClearAll)

CmdViewMeasureClearAll::CmdViewMeasureClearAll()
  : Command("View_Measure_Clear_All")
{
    sGroup        = "Measure";
    sMenuText     = QT_TR_NOOP("Clear measurement");
    sToolTipText  = QT_TR_NOOP("Clear all visible measurements");
    sWhatsThis    = "View_Measure_Clear_All";
    sStatusTip    = sToolTipText;
    sPixmap       = "Part_Measure_Clear_All";
}

void CmdViewMeasureClearAll::activated(int iMsg)
{
    Q_UNUSED(iMsg);
    auto view = dynamic_cast<Gui::View3DInventor*>(Gui::Application::Instance->
        activeDocument()->getActiveView());
    if (!view)
        return;
    Gui::View3DInventorViewer *viewer = view->getViewer();
    if (!viewer)
        return;
    viewer->eraseAllDimensions();
}

//===========================================================================
// Part_Measure_Toggle_All
//===========================================================================

DEF_STD_CMD(CmdViewMeasureToggleAll)

CmdViewMeasureToggleAll::CmdViewMeasureToggleAll()
  : Command("View_Measure_Toggle_All")
{
    sGroup        = "Measure";
    sMenuText     = QT_TR_NOOP("Toggle measurement");
    sToolTipText  = QT_TR_NOOP("Turn on or off the display of all measurements");
    sWhatsThis    = "View_Measure_Toggle_All";
    sStatusTip    = sToolTipText;
    sPixmap       = "Part_Measure_Toggle_All";
}

void CmdViewMeasureToggleAll::activated(int iMsg)
{
    Q_UNUSED(iMsg);
    ParameterGrp::handle group = App::GetApplication().GetUserParameter().
    GetGroup("BaseApp")->GetGroup("Preferences")->GetGroup("View");
    bool visibility = group->GetBool("DimensionsVisible", true);
    if (visibility)
        group->SetBool("DimensionsVisible", false);
    else
      group->SetBool("DimensionsVisible", true);
}

//===========================================================================
// Std_SelBack
//===========================================================================

DEF_STD_CMD_A(StdCmdSelBack)

StdCmdSelBack::StdCmdSelBack()
  :Command("Std_SelBack")
{
  sGroup        = "View";
  sMenuText     = QT_TR_NOOP("Selection Back");
  static std::string toolTip = std::string("<p>")
      + QT_TR_NOOP("Restore the previous Tree view selection. "
      "Only works if Tree RecordSelection mode is switched on.")
      + "</p>";
  sToolTipText = toolTip.c_str();
  sWhatsThis    = "Std_SelBack";
  sStatusTip    = sToolTipText;
  sPixmap       = "sel-back";
  sAccel        = "S, B";
  eType         = AlterSelection;
}

void StdCmdSelBack::activated(int iMsg)
{
    Q_UNUSED(iMsg);
    Selection().selStackGoBack();
}

bool StdCmdSelBack::isActive()
{
  return Selection().selStackBackSize()>1;
}

//===========================================================================
// Std_SelForward
//===========================================================================

DEF_STD_CMD_A(StdCmdSelForward)

StdCmdSelForward::StdCmdSelForward()
  :Command("Std_SelForward")
{
  sGroup        = "View";
  sMenuText     = QT_TR_NOOP("Selection Forward");
  static std::string toolTip = std::string("<p>")
      + QT_TR_NOOP("Restore the next Tree view selection. "
      "Only works if Tree RecordSelection mode is switched on.")
      + "</p>";
  sToolTipText = toolTip.c_str();
  sWhatsThis    = "Std_SelForward";
  sStatusTip    = sToolTipText;
  sPixmap       = "sel-forward";
  sAccel        = "S, F";
  eType         = AlterSelection;
}

void StdCmdSelForward::activated(int iMsg)
{
    Q_UNUSED(iMsg);
    Selection().selStackGoForward();
}

bool StdCmdSelForward::isActive()
{
  return !!Selection().selStackForwardSize();
}

//=======================================================================
// Std_TreeSingleDocument
//===========================================================================
#define TREEVIEW_DOC_CMD_DEF(_name,_v) \
DEF_STD_CMD_AC(StdTree##_name) \
void StdTree##_name::activated(int){ \
    TreeParams::setDocumentMode(_v);\
    if(_pcAction) _pcAction->setChecked(true,true);\
}\
Action * StdTree##_name::createAction(void) {\
    Action *pcAction = Command::createAction();\
    pcAction->setCheckable(true);\
    pcAction->setIcon(QIcon());\
    _pcAction = pcAction;\
    isActive();\
    return pcAction;\
}\
bool StdTree##_name::isActive() {\
    bool checked = TreeParams::getDocumentMode()==_v;\
    if(_pcAction && _pcAction->isChecked()!=checked)\
        _pcAction->setChecked(checked,true);\
    return true;\
}

TREEVIEW_DOC_CMD_DEF(SingleDocument,0)

StdTreeSingleDocument::StdTreeSingleDocument()
  : Command("Std_TreeSingleDocument")
{
    sGroup       = "TreeView";
    sMenuText    = QT_TR_NOOP("Single document");
    sToolTipText = QT_TR_NOOP("Only display the active document in the tree view");
    sWhatsThis   = "Std_TreeSingleDocument";
    sStatusTip   = QT_TR_NOOP("Only display the active document in the tree view");
    sPixmap      = "tree-doc-single";
    eType        = 0;
}

//===========================================================================
// Std_TreeMultiDocument
//===========================================================================
TREEVIEW_DOC_CMD_DEF(MultiDocument,1)

StdTreeMultiDocument::StdTreeMultiDocument()
  : Command("Std_TreeMultiDocument")
{
    sGroup       = "TreeView";
    sMenuText    = QT_TR_NOOP("Multi document");
    sToolTipText = QT_TR_NOOP("Display all documents in the tree view");
    sWhatsThis   = "Std_TreeMultiDocument";
    sStatusTip   = QT_TR_NOOP("Display all documents in the tree view");
    sPixmap      = "tree-doc-multi";
    eType        = 0;
}

//===========================================================================
// Std_TreeCollapseDocument
//===========================================================================
TREEVIEW_DOC_CMD_DEF(CollapseDocument,2)

StdTreeCollapseDocument::StdTreeCollapseDocument()
  : Command("Std_TreeCollapseDocument")
{
    sGroup       = "TreeView";
    sMenuText    = QT_TR_NOOP("Collapse/Expand");
    sToolTipText = QT_TR_NOOP("Expand active document and collapse all others");
    sWhatsThis   = "Std_TreeCollapseDocument";
    sStatusTip   = QT_TR_NOOP("Expand active document and collapse all others");
    sPixmap      = "tree-doc-collapse";
    eType        = 0;
}

//===========================================================================
// Std_TreeSyncView
//===========================================================================
#define TREEVIEW_CMD_DEF(_name) \
DEF_STD_CMD_AC(StdTree##_name) \
void StdTree##_name::activated(int){ \
    auto checked = !TreeParams::get##_name();\
    TreeParams::set##_name(checked);\
    if(_pcAction) _pcAction->setChecked(checked,true);\
}\
Action * StdTree##_name::createAction(void) {\
    Action *pcAction = Command::createAction();\
    pcAction->setCheckable(true);\
    pcAction->setIcon(QIcon());\
    _pcAction = pcAction;\
    isActive();\
    return pcAction;\
}\
bool StdTree##_name::isActive() {\
    bool checked = TreeParams::get##_name();\
    if(_pcAction && _pcAction->isChecked()!=checked)\
        _pcAction->setChecked(checked,true);\
    return true;\
}

TREEVIEW_CMD_DEF(SyncView)

StdTreeSyncView::StdTreeSyncView()
  : Command("Std_TreeSyncView")
{
    sGroup       = "TreeView";
    sMenuText    = QT_TR_NOOP("Sync view");
    sToolTipText = QT_TR_NOOP("Auto switch to the 3D view containing the selected item");
    sStatusTip   = sToolTipText;
    sWhatsThis   = "Std_TreeSyncView";
    sPixmap      = "tree-sync-view";
    sAccel       = "T,1";
    eType        = 0;
}

//===========================================================================
// Std_TreeSyncSelection
//===========================================================================
TREEVIEW_CMD_DEF(SyncSelection)

StdTreeSyncSelection::StdTreeSyncSelection()
  : Command("Std_TreeSyncSelection")
{
    sGroup       = "TreeView";
    sMenuText    = QT_TR_NOOP("Sync selection");
    sToolTipText = QT_TR_NOOP("Auto expand tree item when the corresponding object is selected in 3D view");
    sStatusTip   = sToolTipText;
    sWhatsThis   = "Std_TreeSyncSelection";
    sPixmap      = "tree-sync-sel";
    sAccel       = "T,2";
    eType        = 0;
}

//===========================================================================
// Std_TreeSyncPlacement
//===========================================================================
TREEVIEW_CMD_DEF(SyncPlacement)

StdTreeSyncPlacement::StdTreeSyncPlacement()
  : Command("Std_TreeSyncPlacement")
{
    sGroup       = "TreeView";
    sMenuText    = QT_TR_NOOP("Sync placement");
    sToolTipText = QT_TR_NOOP("Auto adjust placement on drag and drop objects across coordinate systems");
    sStatusTip   = sToolTipText;
    sWhatsThis   = "Std_TreeSyncPlacement";
    sPixmap      = "tree-sync-pla";
    sAccel       = "T,3";
    eType        = 0;
}

//===========================================================================
// Std_TreePreSelection
//===========================================================================
TREEVIEW_CMD_DEF(PreSelection)

StdTreePreSelection::StdTreePreSelection()
  : Command("Std_TreePreSelection")
{
    sGroup       = "TreeView";
    sMenuText    = QT_TR_NOOP("Pre-selection");
    sToolTipText = QT_TR_NOOP("Preselect the object in 3D view when mouse over the tree item");
    sStatusTip   = sToolTipText;
    sWhatsThis   = "Std_TreePreSelection";
    sPixmap      = "tree-pre-sel";
    sAccel       = "T,4";
    eType        = 0;
}

//===========================================================================
// Std_TreeRecordSelection
//===========================================================================
TREEVIEW_CMD_DEF(RecordSelection)

StdTreeRecordSelection::StdTreeRecordSelection()
  : Command("Std_TreeRecordSelection")
{
    sGroup       = "TreeView";
    sMenuText    = QT_TR_NOOP("Record selection");
    sToolTipText = QT_TR_NOOP("Record selection in tree view in order to go back/forward using navigation button");
    sStatusTip   = sToolTipText;
    sWhatsThis   = "Std_TreeRecordSelection";
    sPixmap      = "tree-rec-sel";
    sAccel       = "T,5";
    eType        = 0;
}

//===========================================================================
// Std_TreeDrag
//===========================================================================
DEF_STD_CMD(StdTreeDrag)

StdTreeDrag::StdTreeDrag()
  : Command("Std_TreeDrag")
{
    sGroup       = "TreeView";
    sMenuText    = QT_TR_NOOP("Initiate dragging");
    sToolTipText = QT_TR_NOOP("Initiate dragging of current selected tree items");
    sStatusTip   = sToolTipText;
    sWhatsThis   = "Std_TreeDrag";
    sPixmap      = "tree-item-drag";
    sAccel       = "T,D";
    eType        = 0;
}

void StdTreeDrag::activated(int)
{
    if(Gui::Selection().hasSelection()) {
        const auto trees = getMainWindow()->findChildren<TreeWidget*>();
        for(auto tree : trees) {
            if(tree->isVisible()) {
                tree->startDragging();
                break;
            }
        }
    }
}

//======================================================================
// Std_TreeViewActions
//===========================================================================
//
class StdCmdTreeViewActions : public GroupCommand
{
public:
    StdCmdTreeViewActions()
        :GroupCommand("Std_TreeViewActions")
    {
        sGroup        = "TreeView";
        sMenuText     = QT_TR_NOOP("TreeView actions");
        sToolTipText  = QT_TR_NOOP("TreeView behavior options and actions");
        sWhatsThis    = "Std_TreeViewActions";
        sStatusTip    = QT_TR_NOOP("TreeView behavior options and actions");
        eType         = 0;
        bCanLog       = false;

        addCommand(new StdTreeSyncView());
        addCommand(new StdTreeSyncSelection());
        addCommand(new StdTreeSyncPlacement());
        addCommand(new StdTreePreSelection());
        addCommand(new StdTreeRecordSelection());

        addCommand();

        addCommand(new StdTreeSingleDocument());
        addCommand(new StdTreeMultiDocument());
        addCommand(new StdTreeCollapseDocument());

        addCommand();

        addCommand(new StdTreeDrag(),!cmds.empty());
        addCommand(new StdTreeSelection(),!cmds.empty());

        addCommand();

        addCommand(new StdCmdSelBack());
        addCommand(new StdCmdSelForward());
    }
    const char* className() const override {return "StdCmdTreeViewActions";}
};


//======================================================================
// Std_SelBoundingBox
//===========================================================================
DEF_STD_CMD_AC(StdCmdSelBoundingBox)

StdCmdSelBoundingBox::StdCmdSelBoundingBox()
  :Command("Std_SelBoundingBox")
{
  sGroup        = "View";
  sMenuText     = QT_TR_NOOP("&Bounding box");
  sToolTipText  = QT_TR_NOOP("Show selection bounding box");
  sWhatsThis    = "Std_SelBoundingBox";
  sStatusTip    = QT_TR_NOOP("Show selection bounding box");
  sPixmap       = "sel-bbox";
  eType         = Alter3DView;
}

void StdCmdSelBoundingBox::activated(int iMsg)
{
    bool checked = !!iMsg;
    if(checked != ViewParams::instance()->getShowSelectionBoundingBox()) {
        ViewParams::instance()->setShowSelectionBoundingBox(checked);
        if(_pcAction)
            _pcAction->setChecked(checked,true);
    }
}

bool StdCmdSelBoundingBox::isActive()
{
    if(_pcAction) {
        bool checked = _pcAction->isChecked();
        if(checked != ViewParams::instance()->getShowSelectionBoundingBox())
            _pcAction->setChecked(!checked,true);
    }
    return true;
}

Action * StdCmdSelBoundingBox::createAction()
{
    Action *pcAction = Command::createAction();
    pcAction->setCheckable(true);
    return pcAction;
}

//===========================================================================
// Std_StoreWorkingView
//===========================================================================
DEF_STD_CMD_A(StdStoreWorkingView)

StdStoreWorkingView::StdStoreWorkingView()
  : Command("Std_StoreWorkingView")
{
    sGroup        = "Standard-View";
    sMenuText     = QT_TR_NOOP("Store working view");
    sToolTipText  = QT_TR_NOOP("Store a document-specific temporary working view");
    sStatusTip    = QT_TR_NOOP("Store a document-specific temporary working view");
    sWhatsThis    = "Std_StoreWorkingView";
    sAccel        = "Shift+End";
    eType         = NoTransaction;
}

void StdStoreWorkingView::activated(int iMsg)
{
    Q_UNUSED(iMsg);
    if (auto view = dynamic_cast<Gui::View3DInventor*>(Gui::getMainWindow()->activeWindow())) {
        view->getViewer()->saveHomePosition();
    }
}

bool StdStoreWorkingView::isActive()
{
    return dynamic_cast<Gui::View3DInventor*>(Gui::getMainWindow()->activeWindow());
}

//===========================================================================
// Std_RecallWorkingView
//===========================================================================
DEF_STD_CMD_A(StdRecallWorkingView)

StdRecallWorkingView::StdRecallWorkingView()
  : Command("Std_RecallWorkingView")
{
    sGroup        = "Standard-View";
    sMenuText     = QT_TR_NOOP("Recall working view");
    sToolTipText  = QT_TR_NOOP("Recall previously stored temporary working view");
    sStatusTip    = QT_TR_NOOP("Recall previously stored temporary working view");
    sWhatsThis    = "Std_RecallWorkingView";
    sAccel        = "End";
    eType         = NoTransaction;
}

void StdRecallWorkingView::activated(int iMsg)
{
    Q_UNUSED(iMsg);
    if (auto view = dynamic_cast<Gui::View3DInventor*>(Gui::getMainWindow()->activeWindow())) {
        if (view->getViewer()->hasHomePosition())
            view->getViewer()->resetToHomePosition();
    }
}

bool StdRecallWorkingView::isActive()
{
    auto view = dynamic_cast<Gui::View3DInventor*>(Gui::getMainWindow()->activeWindow());
    return view && view->getViewer()->hasHomePosition();
}

//===========================================================================
// Instantiation
//===========================================================================


namespace Gui {

void CreateViewStdCommands()
{
    // NOLINTBEGIN
    CommandManager &rcCmdMgr = Application::Instance->commandManager();

    // views
    rcCmdMgr.addCommand(new StdCmdViewBottom());
    rcCmdMgr.addCommand(new StdCmdViewHome());
    rcCmdMgr.addCommand(new StdCmdViewFront());
    rcCmdMgr.addCommand(new StdCmdViewLeft());
    rcCmdMgr.addCommand(new StdCmdViewRear());
    rcCmdMgr.addCommand(new StdCmdViewRight());
    rcCmdMgr.addCommand(new StdCmdViewTop());
    rcCmdMgr.addCommand(new StdCmdViewIsometric());
    rcCmdMgr.addCommand(new StdCmdViewDimetric());
    rcCmdMgr.addCommand(new StdCmdViewTrimetric());
    rcCmdMgr.addCommand(new StdCmdViewFitAll());
    rcCmdMgr.addCommand(new StdCmdViewVR());
    rcCmdMgr.addCommand(new StdCmdViewFitSelection());
    rcCmdMgr.addCommand(new StdCmdViewRotateLeft());
    rcCmdMgr.addCommand(new StdCmdViewRotateRight());
    rcCmdMgr.addCommand(new StdStoreWorkingView());
    rcCmdMgr.addCommand(new StdRecallWorkingView());

    rcCmdMgr.addCommand(new StdCmdViewExample1());
    rcCmdMgr.addCommand(new StdCmdViewExample2());
    rcCmdMgr.addCommand(new StdCmdViewExample3());

    rcCmdMgr.addCommand(new StdCmdViewIvStereoQuadBuff());
    rcCmdMgr.addCommand(new StdCmdViewIvStereoRedGreen());
    rcCmdMgr.addCommand(new StdCmdViewIvStereoInterleavedColumns());
    rcCmdMgr.addCommand(new StdCmdViewIvStereoInterleavedRows());
    rcCmdMgr.addCommand(new StdCmdViewIvStereoOff());

    rcCmdMgr.addCommand(new StdCmdViewIvIssueCamPos());

    rcCmdMgr.addCommand(new StdCmdViewCreate());
    rcCmdMgr.addCommand(new StdViewScreenShot());
    rcCmdMgr.addCommand(new StdViewLoadImage());
    rcCmdMgr.addCommand(new StdMainFullscreen());
    rcCmdMgr.addCommand(new StdViewDockUndockFullscreen());
    rcCmdMgr.addCommand(new StdCmdSetAppearance());
    rcCmdMgr.addCommand(new StdCmdToggleVisibility());
    rcCmdMgr.addCommand(new StdCmdToggleSelectability());
    rcCmdMgr.addCommand(new StdCmdShowSelection());
    rcCmdMgr.addCommand(new StdCmdHideSelection());
    rcCmdMgr.addCommand(new StdCmdSelectVisibleObjects());
    rcCmdMgr.addCommand(new StdCmdToggleObjects());
    rcCmdMgr.addCommand(new StdCmdShowObjects());
    rcCmdMgr.addCommand(new StdCmdHideObjects());
    rcCmdMgr.addCommand(new StdOrthographicCamera());
    rcCmdMgr.addCommand(new StdPerspectiveCamera());
    rcCmdMgr.addCommand(new StdCmdToggleClipPlane());
    rcCmdMgr.addCommand(new StdCmdDrawStyle());
    rcCmdMgr.addCommand(new StdCmdViewSaveCamera());
    rcCmdMgr.addCommand(new StdCmdViewRestoreCamera());
    rcCmdMgr.addCommand(new StdCmdFreezeViews());
    rcCmdMgr.addCommand(new StdViewZoomIn());
    rcCmdMgr.addCommand(new StdViewZoomOut());
    rcCmdMgr.addCommand(new StdViewBoxZoom());
    rcCmdMgr.addCommand(new StdBoxSelection());
    rcCmdMgr.addCommand(new StdBoxElementSelection());
    rcCmdMgr.addCommand(new StdCmdTreeExpand());
    rcCmdMgr.addCommand(new StdCmdTreeCollapse());
    rcCmdMgr.addCommand(new StdCmdTreeSelectAllInstances());
    rcCmdMgr.addCommand(new StdCmdMeasureDistance());
    rcCmdMgr.addCommand(new StdCmdSceneInspector());
    rcCmdMgr.addCommand(new StdCmdTextureMapping());
    rcCmdMgr.addCommand(new StdCmdDemoMode());
    rcCmdMgr.addCommand(new StdCmdToggleNavigation());
    rcCmdMgr.addCommand(new StdCmdAxisCross());
    rcCmdMgr.addCommand(new CmdViewMeasureClearAll());
    rcCmdMgr.addCommand(new CmdViewMeasureToggleAll());
    rcCmdMgr.addCommand(new StdCmdSelBoundingBox());
    rcCmdMgr.addCommand(new StdCmdTreeViewActions());

    auto hGrp = App::GetApplication().GetParameterGroupByPath("User parameter:BaseApp/Preferences/View");
    if(hGrp->GetASCII("GestureRollFwdCommand").empty())
        hGrp->SetASCII("GestureRollFwdCommand","Std_SelForward");
    if(hGrp->GetASCII("GestureRollBackCommand").empty())
        hGrp->SetASCII("GestureRollBackCommand","Std_SelBack");
    // NOLINTEND
}

} // namespace Gui
