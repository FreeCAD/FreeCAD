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
# include <Inventor/actions/SoGetBoundingBoxAction.h>
# include <Inventor/events/SoMouseButtonEvent.h>
# include <Inventor/nodes/SoOrthographicCamera.h>
# include <Inventor/nodes/SoPerspectiveCamera.h>
# include <QFile>
# include <QFileInfo>
# include <QFont>
# include <QFontMetrics>
# include <QMessageBox>
# include <QPainter>
# include <QTextStream>
# include <boost/bind.hpp>
#endif

#include "Command.h"
#include "Action.h"
#include "Application.h"
#include "BitmapFactory.h"
#include "Control.h"
#include "Clipping.h"
#include "FileDialog.h"
#include "MainWindow.h"
#include "Tree.h"
#include "View.h"
#include "Document.h"
#include "Macro.h"
#include "DlgDisplayPropertiesImp.h"
#include "DlgSettingsImageImp.h"
#include "Selection.h"
#include "SoFCOffscreenRenderer.h"
#include "SoFCBoundingBox.h"
#include "SoFCUnifiedSelection.h"
#include "SoAxisCrossKit.h"
#include "View3DInventor.h"
#include "View3DInventorViewer.h"
#include "WaitCursor.h"
#include "ViewProviderMeasureDistance.h"
#include "ViewProviderGeometryObject.h"
#include "SceneInspector.h"
#include "DemoMode.h"
#include "TextureMapping.h"
#include "Utilities.h"
#include "NavigationStyle.h"

#include <Base/Console.h>
#include <Base/Exception.h>
#include <Base/FileInfo.h>
#include <Base/Reader.h>
#include <Base/Parameter.h>
#include <App/Application.h>
#include <App/Document.h>
#include <App/GeoFeature.h>
#include <App/DocumentObjectGroup.h>
#include <App/MeasureDistance.h>
#include <App/DocumentObject.h>

#include <QDomDocument>
#include <QDomElement>

using namespace Gui;
using Gui::Dialog::DlgSettingsImageImp;

//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

DEF_STD_CMD_AC(StdOrthographicCamera)

StdOrthographicCamera::StdOrthographicCamera()
  : Command("Std_OrthographicCamera")
{
    sGroup        = QT_TR_NOOP("Standard-View");
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
        View3DInventor* view = qobject_cast<View3DInventor*>(getMainWindow()->activeWindow());
        SoCamera* cam = view->getViewer()->getSoRenderManager()->getCamera();
        if (!cam || cam->getTypeId() != SoOrthographicCamera::getClassTypeId())

            doCommand(Command::Gui,"Gui.activeDocument().activeView().setCameraType(\"Orthographic\")");
    }
}

bool StdOrthographicCamera::isActive(void)
{
    View3DInventor* view = qobject_cast<View3DInventor*>(getMainWindow()->activeWindow());
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

Action * StdOrthographicCamera::createAction(void)
{
    Action *pcAction = Command::createAction();
    pcAction->setCheckable(true);
    return pcAction;
}

DEF_STD_CMD_AC(StdPerspectiveCamera)

StdPerspectiveCamera::StdPerspectiveCamera()
  : Command("Std_PerspectiveCamera")
{
    sGroup        = QT_TR_NOOP("Standard-View");
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
        View3DInventor* view = qobject_cast<View3DInventor*>(getMainWindow()->activeWindow());
        SoCamera* cam = view->getViewer()->getSoRenderManager()->getCamera();
        if (!cam || cam->getTypeId() != SoPerspectiveCamera::getClassTypeId())

            doCommand(Command::Gui,"Gui.activeDocument().activeView().setCameraType(\"Perspective\")");
    }
}

bool StdPerspectiveCamera::isActive(void)
{
    View3DInventor* view = qobject_cast<View3DInventor*>(getMainWindow()->activeWindow());
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

Action * StdPerspectiveCamera::createAction(void)
{
    Action *pcAction = Command::createAction();
    pcAction->setCheckable(true);
    return pcAction;
}

//===========================================================================
// Std_FreezeViews
//===========================================================================
class StdCmdFreezeViews : public Gui::Command
{
public:
    StdCmdFreezeViews();
    virtual ~StdCmdFreezeViews(){}
    const char* className() const
    { return "StdCmdFreezeViews"; }

protected:
    virtual void activated(int iMsg);
    virtual bool isActive(void);
    virtual Action * createAction(void);
    virtual void languageChange();

private:
    void onSaveViews();
    void onRestoreViews();

private:
    const int maxViews;
    int savedViews;
    int offset;
    QAction* saveView;
    QAction* freezeView;
    QAction* clearView;
    QAction* separator;
};

StdCmdFreezeViews::StdCmdFreezeViews()
  : Command("Std_FreezeViews")
  , maxViews(50)
  , savedViews(0)
  , offset(0)
  , saveView(0)
  , freezeView(0)
  , clearView(0)
  , separator(0)
{
    sGroup        = QT_TR_NOOP("Standard-View");
    sMenuText     = QT_TR_NOOP("Freeze display");
    sToolTipText  = QT_TR_NOOP("Freezes the current view position");
    sWhatsThis    = "Std_FreezeViews";
    sStatusTip    = QT_TR_NOOP("Freezes the current view position");
    sAccel        = "Shift+F";
    eType         = Alter3DView;
}

Action * StdCmdFreezeViews::createAction(void)
{
    ActionGroup* pcAction = new ActionGroup(this, getMainWindow());
    pcAction->setDropDownMenu(true);
    applyCommandData(this->className(), pcAction);

    // add the action items
    saveView = pcAction->addAction(QObject::tr("Save views..."));
    pcAction->addAction(QObject::tr("Load views..."));
    pcAction->addAction(QString::fromLatin1(""))->setSeparator(true);
    freezeView = pcAction->addAction(QObject::tr("Freeze view"));
    freezeView->setShortcut(QString::fromLatin1(sAccel));
    clearView = pcAction->addAction(QObject::tr("Clear views"));
    separator = pcAction->addAction(QString::fromLatin1(""));
    separator->setSeparator(true);
    offset = pcAction->actions().count();

    // allow up to 50 views
    for (int i=0; i<maxViews; i++)
        pcAction->addAction(QString::fromLatin1(""))->setVisible(false);

    return pcAction;
}

void StdCmdFreezeViews::activated(int iMsg)
{
    ActionGroup* pcAction = qobject_cast<ActionGroup*>(_pcAction);

    if (iMsg == 0) {
        onSaveViews();
    }
    else if (iMsg == 1) {
        onRestoreViews();
    }
    else if (iMsg == 3) {
        // Create a new view
        const char* ppReturn=0;
        getGuiApplication()->sendMsgToActiveView("GetCamera",&ppReturn);

        QList<QAction*> acts = pcAction->actions();
        int index = 0;
        for (QList<QAction*>::ConstIterator it = acts.begin()+offset; it != acts.end(); ++it, index++) {
            if (!(*it)->isVisible()) {
                savedViews++;
                QString viewnr = QString(QObject::tr("Restore view &%1")).arg(index+1);
                (*it)->setText(viewnr);
                (*it)->setToolTip(QString::fromLatin1(ppReturn));
                (*it)->setVisible(true);
                if (index < 9) {
                    int accel = Qt::CTRL+Qt::Key_1;
                    (*it)->setShortcut(accel+index);
                }
                break;
            }
        }
    }
    else if (iMsg == 4) {
        savedViews = 0;
        QList<QAction*> acts = pcAction->actions();
        for (QList<QAction*>::ConstIterator it = acts.begin()+offset; it != acts.end(); ++it)
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
        ActionGroup* pcAction = qobject_cast<ActionGroup*>(_pcAction);
        QList<QAction*> acts = pcAction->actions();
        str << "<?xml version='1.0' encoding='utf-8'?>" << endl
            << "<FrozenViews SchemaVersion=\"1\">" << endl;
        str << "  <Views Count=\"" << savedViews <<"\">" << endl;

        for (QList<QAction*>::ConstIterator it = acts.begin()+offset; it != acts.end(); ++it) {
            if ( !(*it)->isVisible() )
                break;
            QString data = (*it)->toolTip();

            // remove the first line because it's a comment like '#Inventor V2.1 ascii'
            QString viewPos;
            if ( !data.isEmpty() ) {
                QStringList lines = data.split(QString::fromLatin1("\n"));
                if ( lines.size() > 1 ) {
                    lines.pop_front();
                    viewPos = lines.join(QString::fromLatin1(" "));
                }
            }

            str << "    <Camera settings=\"" << viewPos.toLatin1().constData() << "\"/>" << endl;
        }

        str << "  </Views>" << endl;
        str << "</FrozenViews>" << endl;
    }
}

void StdCmdFreezeViews::onRestoreViews()
{
    // Should we clear the already saved views
    if (savedViews > 0) {
        int ret = QMessageBox::question(getMainWindow(), QObject::tr("Restore views"),
            QObject::tr("Importing the restored views would clear the already stored views.\n"
                        "Do you want to continue?"), QMessageBox::Yes|QMessageBox::Default,
                                                     QMessageBox::No|QMessageBox::Escape);
        if (ret!=QMessageBox::Yes)
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
    if (!ok) return;
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
        ActionGroup* pcAction = qobject_cast<ActionGroup*>(_pcAction);
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
            if ( i < 9 ) {
                int accel = Qt::CTRL+Qt::Key_1;
                acts[i+offset]->setShortcut(accel+i);
            }
        }

        // if less views than actions
        for (int index = numRestoredViews+offset; index < acts.count(); index++)
            acts[index]->setVisible(false);
    }
}

bool StdCmdFreezeViews::isActive(void)
{
    View3DInventor* view = qobject_cast<View3DInventor*>(getMainWindow()->activeWindow());
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
    ActionGroup* pcAction = qobject_cast<ActionGroup*>(_pcAction);
    QList<QAction*> acts = pcAction->actions();
    acts[0]->setText(QObject::tr("Save views..."));
    acts[1]->setText(QObject::tr("Load views..."));
    acts[3]->setText(QObject::tr("Freeze view"));
    acts[4]->setText(QObject::tr("Clear views"));
    int index=1;
    for (QList<QAction*>::ConstIterator it = acts.begin()+5; it != acts.end(); ++it, index++) {
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
    sGroup        = QT_TR_NOOP("Standard-View");
    sMenuText     = QT_TR_NOOP("Clipping plane");
    sToolTipText  = QT_TR_NOOP("Toggles clipping plane for active view");
    sWhatsThis    = "Std_ToggleClipPlane";
    sStatusTip    = QT_TR_NOOP("Toggles clipping plane for active view");
    eType         = Alter3DView;
}

Action * StdCmdToggleClipPlane::createAction(void)
{
    Action *pcAction = (Action*)Command::createAction();
#if 0
    pcAction->setCheckable(true);
#endif
    return pcAction;
}

void StdCmdToggleClipPlane::activated(int iMsg)
{
    Q_UNUSED(iMsg);
#if 0
    View3DInventor* view = qobject_cast<View3DInventor*>(getMainWindow()->activeWindow());
    if (view) {
        if (iMsg > 0 && !view->hasClippingPlane())
            view->toggleClippingPlane();
        else if (iMsg == 0 && view->hasClippingPlane())
            view->toggleClippingPlane();
    }
#else
    View3DInventor* view = qobject_cast<View3DInventor*>(getMainWindow()->activeWindow());
    if (view) {
        Gui::Control().showDialog(new Gui::Dialog::TaskClipping(view));
    }
#endif
}

bool StdCmdToggleClipPlane::isActive(void)
{
#if 0
    View3DInventor* view = qobject_cast<View3DInventor*>(getMainWindow()->activeWindow());
    if (view) {
        Action* action = qobject_cast<Action*>(_pcAction);
        if (action->isChecked() != view->hasClippingPlane())
            action->setChecked(view->hasClippingPlane());
        return true;
    }
    else {
        Action* action = qobject_cast<Action*>(_pcAction);
        if (action->isChecked())
            action->setChecked(false);
        return false;
    }
#else
    if (Gui::Control().activeDialog())
        return false;
    return true;
#endif
}

//===========================================================================
// StdCmdDrawStyle
//===========================================================================
class StdCmdDrawStyle : public Gui::Command
{
public:
    StdCmdDrawStyle();
    virtual ~StdCmdDrawStyle(){}
    virtual void languageChange();
    virtual const char* className() const {return "StdCmdDrawStyle";}
    void updateIcon(const Gui::MDIView* view);
protected:
    virtual void activated(int iMsg);
    virtual bool isActive(void);
    virtual Gui::Action * createAction(void);
};

StdCmdDrawStyle::StdCmdDrawStyle()
  : Command("Std_DrawStyle")
{
    sGroup        = QT_TR_NOOP("Standard-View");
    sMenuText     = QT_TR_NOOP("Draw style");
    sToolTipText  = QT_TR_NOOP("Draw style");
    sStatusTip    = QT_TR_NOOP("Draw style");
    sPixmap       = "DrawStyleAsIs";
    eType         = Alter3DView;

    this->getGuiApplication()->signalActivateView.connect(boost::bind(&StdCmdDrawStyle::updateIcon, this, _1));
}

Gui::Action * StdCmdDrawStyle::createAction(void)
{
    Gui::ActionGroup* pcAction = new Gui::ActionGroup(this, Gui::getMainWindow());
    pcAction->setDropDownMenu(true);
    applyCommandData(this->className(), pcAction);

    QAction* a0 = pcAction->addAction(QString());
    a0->setCheckable(true);
    a0->setIcon(BitmapFactory().iconFromTheme("DrawStyleAsIs"));
    a0->setChecked(true);
    a0->setObjectName(QString::fromLatin1("Std_DrawStyleAsIs"));
    a0->setShortcut(QKeySequence(QString::fromUtf8("V,1")));
    QAction* a1 = pcAction->addAction(QString());
    a1->setCheckable(true);
    a1->setIcon(BitmapFactory().iconFromTheme("DrawStyleFlatLines"));
    a1->setObjectName(QString::fromLatin1("Std_DrawStyleFlatLines"));
    a1->setShortcut(QKeySequence(QString::fromUtf8("V,2")));
    QAction* a2 = pcAction->addAction(QString());
    a2->setCheckable(true);
    a2->setIcon(BitmapFactory().iconFromTheme("DrawStyleShaded"));
    a2->setObjectName(QString::fromLatin1("Std_DrawStyleShaded"));
    a2->setShortcut(QKeySequence(QString::fromUtf8("V,3")));
    QAction* a3 = pcAction->addAction(QString());
    a3->setCheckable(true);
    a3->setIcon(BitmapFactory().iconFromTheme("DrawStyleWireFrame"));
    a3->setObjectName(QString::fromLatin1("Std_DrawStyleWireframe"));
    a3->setShortcut(QKeySequence(QString::fromUtf8("V,4")));
    QAction* a4 = pcAction->addAction(QString());
    a4->setCheckable(true);
    a4->setIcon(BitmapFactory().iconFromTheme("DrawStylePoints"));
    a4->setObjectName(QString::fromLatin1("Std_DrawStylePoints"));
    a4->setShortcut(QKeySequence(QString::fromUtf8("V,5")));
    QAction* a5 = pcAction->addAction(QString());
    a5->setCheckable(true);
    a5->setIcon(BitmapFactory().iconFromTheme("DrawStyleWireFrame"));
    a5->setObjectName(QString::fromLatin1("Std_DrawStyleHiddenLine"));
    a5->setShortcut(QKeySequence(QString::fromUtf8("V,6")));
    QAction* a6 = pcAction->addAction(QString());
    a6->setCheckable(true);
    a6->setIcon(BitmapFactory().iconFromTheme("DrawStyleWireFrame"));
    a6->setObjectName(QString::fromLatin1("Std_DrawStyleNoShading"));
    a6->setShortcut(QKeySequence(QString::fromUtf8("V,7")));


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
    Gui::ActionGroup* pcAction = qobject_cast<Gui::ActionGroup*>(_pcAction);
    QList<QAction*> a = pcAction->actions();

    a[0]->setText(QCoreApplication::translate(
        "Std_DrawStyle", "As is"));
    a[0]->setToolTip(QCoreApplication::translate(
        "Std_DrawStyle", "Normal mode"));

    a[1]->setText(QCoreApplication::translate(
        "Std_DrawStyle", "Flat lines"));
    a[1]->setToolTip(QCoreApplication::translate(
        "Std_DrawStyle", "Flat lines mode"));

    a[2]->setText(QCoreApplication::translate(
        "Std_DrawStyle", "Shaded"));
    a[2]->setToolTip(QCoreApplication::translate(
        "Std_DrawStyle", "Shaded mode"));

    a[3]->setText(QCoreApplication::translate(
        "Std_DrawStyle", "Wireframe"));
    a[3]->setToolTip(QCoreApplication::translate(
        "Std_DrawStyle", "Wireframe mode"));

    a[4]->setText(QCoreApplication::translate(
        "Std_DrawStyle", "Points"));
    a[4]->setToolTip(QCoreApplication::translate(
        "Std_DrawStyle", "Points mode"));

    a[5]->setText(QCoreApplication::translate(
        "Std_DrawStyle", "Hidden line"));
    a[5]->setToolTip(QCoreApplication::translate(
        "Std_DrawStyle", "Hidden line mode"));

    a[6]->setText(QCoreApplication::translate(
        "Std_DrawStyle", "No shading"));
    a[6]->setToolTip(QCoreApplication::translate(
        "Std_DrawStyle", "No shading mode"));
}

void StdCmdDrawStyle::updateIcon(const MDIView *view)
{
    const Gui::View3DInventor *view3d = dynamic_cast<const Gui::View3DInventor *>(view);
    if (!view3d)
        return;
    Gui::View3DInventorViewer *viewer = view3d->getViewer();
    if (!viewer)
        return;
    std::string mode(viewer->getOverrideMode());
    Gui::ActionGroup *actionGroup = dynamic_cast<Gui::ActionGroup *>(_pcAction);
    if (!actionGroup)
        return;

    if (mode == "Flat Lines")
    {
        actionGroup->setCheckedAction(1);
        return;
    }
    if (mode == "Shaded")
    {
        actionGroup->setCheckedAction(2);
        return;
    }
    if (mode == "Wireframe")
    {
        actionGroup->setCheckedAction(3);
        return;
    }
    if (mode == "Point")
    {
        actionGroup->setCheckedAction(4);
        return;
    }
    if (mode == "Hidden Line")
    {
        actionGroup->setCheckedAction(5);
        return;
    }
    if (mode == "No shading")
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
        View3DInventor* view = qobject_cast<View3DInventor*>(*viewIt);
        if (view)
        {
            View3DInventorViewer* viewer;
            viewer = view->getViewer();
            if (viewer)
            {
                switch (iMsg)
                {
                case 1:
                    (oneChangedSignal) ? viewer->updateOverrideMode("Flat Lines") : viewer->setOverrideMode("Flat Lines");
                    break;
                case 2:
                    (oneChangedSignal) ? viewer->updateOverrideMode("Shaded") : viewer->setOverrideMode("Shaded");
                    break;
                case 3:
                    (oneChangedSignal) ? viewer->updateOverrideMode("Wireframe") : viewer->setOverrideMode("Wireframe");
                    break;
                case 4:
                    (oneChangedSignal) ? viewer->updateOverrideMode("Point") : viewer->setOverrideMode("Point");
                    break;
                case 5:
                    (oneChangedSignal) ? viewer->updateOverrideMode("Hidden Line") : viewer->setOverrideMode("Hidden Line");
                    break;
                case 6:
                    (oneChangedSignal) ? viewer->updateOverrideMode("No Shading") : viewer->setOverrideMode("No Shading");
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

bool StdCmdDrawStyle::isActive(void)
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
    sGroup        = QT_TR_NOOP("Standard-View");
    sMenuText     = QT_TR_NOOP("Toggle visibility");
    sToolTipText  = QT_TR_NOOP("Toggles visibility");
    sStatusTip    = QT_TR_NOOP("Toggles visibility");
    sWhatsThis    = "Std_ToggleVisibility";
    sAccel        = "Space";
    eType         = Alter3DView;
}

void StdCmdToggleVisibility::activated(int iMsg)
{
    Q_UNUSED(iMsg);
    // go through all documents
    const std::vector<App::Document*> docs = App::GetApplication().getDocuments();
    for (std::vector<App::Document*>::const_iterator it = docs.begin(); it != docs.end(); ++it) {
        Document *pcDoc = Application::Instance->getDocument(*it);
        std::vector<App::DocumentObject*> sel = Selection().getObjectsOfType
            (App::DocumentObject::getClassTypeId(), (*it)->getName());

        // in case a group object and an object of the group is selected then ignore the group object
        std::vector<App::DocumentObject*> ignore;
        for (std::vector<App::DocumentObject*>::iterator ft=sel.begin();ft!=sel.end();++ft) {
            if ((*ft)->getTypeId().isDerivedFrom(App::DocumentObjectGroup::getClassTypeId())) {
                App::DocumentObjectGroup* grp = static_cast<App::DocumentObjectGroup*>(*ft);
                std::vector<App::DocumentObject*> sub = grp->Group.getValues();
                for (std::vector<App::DocumentObject*>::iterator st = sub.begin(); st != sub.end(); ++st) {
                    if (std::find(sel.begin(), sel.end(), *st) != sel.end()) {
                        ignore.push_back(*ft);
                        break;
                    }
                }
            }
        }

        if (!ignore.empty()) {
            std::sort(sel.begin(), sel.end());
            std::sort(ignore.begin(), ignore.end());
            std::vector<App::DocumentObject*> diff;
            std::back_insert_iterator<std::vector<App::DocumentObject*> > biit(diff);
            std::set_difference(sel.begin(), sel.end(), ignore.begin(), ignore.end(), biit);
            sel = diff;
        }

        for (std::vector<App::DocumentObject*>::const_iterator ft=sel.begin();ft!=sel.end();++ft) {
            if (pcDoc && pcDoc->isShow((*ft)->getNameInDocument()))
                doCommand(Gui,"Gui.getDocument(\"%s\").getObject(\"%s\").Visibility=False"
                             , (*it)->getName(), (*ft)->getNameInDocument());
            else
                doCommand(Gui,"Gui.getDocument(\"%s\").getObject(\"%s\").Visibility=True"
                             , (*it)->getName(), (*ft)->getNameInDocument());
        }
    }
}

bool StdCmdToggleVisibility::isActive(void)
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
    sGroup        = QT_TR_NOOP("Standard-View");
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
    for (std::vector<App::Document*>::const_iterator it = docs.begin(); it != docs.end(); ++it) {
        Document *pcDoc = Application::Instance->getDocument(*it);
        std::vector<App::DocumentObject*> sel = Selection().getObjectsOfType
            (App::DocumentObject::getClassTypeId(), (*it)->getName());


        for (std::vector<App::DocumentObject*>::const_iterator ft=sel.begin();ft!=sel.end();++ft) {
            ViewProvider *pr = pcDoc->getViewProviderByName((*ft)->getNameInDocument());
            if (pr && pr->isDerivedFrom(ViewProviderGeometryObject::getClassTypeId())){
                if (static_cast<ViewProviderGeometryObject*>(pr)->Selectable.getValue())
                    doCommand(Gui,"Gui.getDocument(\"%s\").getObject(\"%s\").Selectable=False"
                                 , (*it)->getName(), (*ft)->getNameInDocument());
                else
                    doCommand(Gui,"Gui.getDocument(\"%s\").getObject(\"%s\").Selectable=True"
                                 , (*it)->getName(), (*ft)->getNameInDocument());
            }
        }
    }
}

bool StdCmdToggleSelectability::isActive(void)
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
    sGroup        = QT_TR_NOOP("Standard-View");
    sMenuText     = QT_TR_NOOP("Show selection");
    sToolTipText  = QT_TR_NOOP("Show all selected objects");
    sStatusTip    = QT_TR_NOOP("Show all selected objects");
    sWhatsThis    = "Std_ShowSelection";
    eType         = Alter3DView;
}

void StdCmdShowSelection::activated(int iMsg)
{
    Q_UNUSED(iMsg);
    // go through all documents
    const std::vector<App::Document*> docs = App::GetApplication().getDocuments();
    for (std::vector<App::Document*>::const_iterator it = docs.begin(); it != docs.end(); ++it) {
        const std::vector<App::DocumentObject*> sel = Selection().getObjectsOfType
            (App::DocumentObject::getClassTypeId(), (*it)->getName());
        for(std::vector<App::DocumentObject*>::const_iterator ft=sel.begin();ft!=sel.end();++ft) {
            doCommand(Gui,"Gui.getDocument(\"%s\").getObject(\"%s\").Visibility=True"
                         , (*it)->getName(), (*ft)->getNameInDocument());
        }
    }
}

bool StdCmdShowSelection::isActive(void)
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
    sGroup        = QT_TR_NOOP("Standard-View");
    sMenuText     = QT_TR_NOOP("Hide selection");
    sToolTipText  = QT_TR_NOOP("Hide all selected objects");
    sStatusTip    = QT_TR_NOOP("Hide all selected objects");
    sWhatsThis    = "Std_HideSelection";
    eType         = Alter3DView;
}

void StdCmdHideSelection::activated(int iMsg)
{
    Q_UNUSED(iMsg);
    // go through all documents
    const std::vector<App::Document*> docs = App::GetApplication().getDocuments();
    for (std::vector<App::Document*>::const_iterator it = docs.begin(); it != docs.end(); ++it) {
        const std::vector<App::DocumentObject*> sel = Selection().getObjectsOfType
            (App::DocumentObject::getClassTypeId(), (*it)->getName());
        for(std::vector<App::DocumentObject*>::const_iterator ft=sel.begin();ft!=sel.end();++ft) {
            doCommand(Gui,"Gui.getDocument(\"%s\").getObject(\"%s\").Visibility=False"
                         , (*it)->getName(), (*ft)->getNameInDocument());
        }
    }
}

bool StdCmdHideSelection::isActive(void)
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
    sGroup        = QT_TR_NOOP("Standard-View");
    sMenuText     = QT_TR_NOOP("Select visible objects");
    sToolTipText  = QT_TR_NOOP("Select visible objects in the active document");
    sStatusTip    = QT_TR_NOOP("Select visible objects in the active document");
    sWhatsThis    = "Std_SelectVisibleObjects";
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
    for (std::vector<App::DocumentObject*>::const_iterator it=obj.begin();it!=obj.end();++it) {
        if (doc->isShow((*it)->getNameInDocument()))
            visible.push_back(*it);
    }

    SelectionSingleton& rSel = Selection();
    rSel.setSelection(app->getName(), visible);
}

bool StdCmdSelectVisibleObjects::isActive(void)
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
    sGroup        = QT_TR_NOOP("Standard-View");
    sMenuText     = QT_TR_NOOP("Toggle all objects");
    sToolTipText  = QT_TR_NOOP("Toggles visibility of all objects in the active document");
    sStatusTip    = QT_TR_NOOP("Toggles visibility of all objects in the active document");
    sWhatsThis    = "Std_ToggleObjects";
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

    for (std::vector<App::DocumentObject*>::const_iterator it=obj.begin();it!=obj.end();++it) {
        if (doc->isShow((*it)->getNameInDocument()))
            doCommand(Gui,"Gui.getDocument(\"%s\").getObject(\"%s\").Visibility=False"
                         , app->getName(), (*it)->getNameInDocument());
        else
            doCommand(Gui,"Gui.getDocument(\"%s\").getObject(\"%s\").Visibility=True"
                         , app->getName(), (*it)->getNameInDocument());
    }
}

bool StdCmdToggleObjects::isActive(void)
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
    sGroup        = QT_TR_NOOP("Standard-View");
    sMenuText     = QT_TR_NOOP("Show all objects");
    sToolTipText  = QT_TR_NOOP("Show all objects in the document");
    sStatusTip    = QT_TR_NOOP("Show all objects in the document");
    sWhatsThis    = "Std_ShowObjects";
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

    for (std::vector<App::DocumentObject*>::const_iterator it=obj.begin();it!=obj.end();++it) {
        doCommand(Gui,"Gui.getDocument(\"%s\").getObject(\"%s\").Visibility=True"
                     , app->getName(), (*it)->getNameInDocument());
    }
}

bool StdCmdShowObjects::isActive(void)
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
    sGroup        = QT_TR_NOOP("Standard-View");
    sMenuText     = QT_TR_NOOP("Hide all objects");
    sToolTipText  = QT_TR_NOOP("Hide all objects in the document");
    sStatusTip    = QT_TR_NOOP("Hide all objects in the document");
    sWhatsThis    = "Std_HideObjects";
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

    for (std::vector<App::DocumentObject*>::const_iterator it=obj.begin();it!=obj.end();++it) {
        doCommand(Gui,"Gui.getDocument(\"%s\").getObject(\"%s\").Visibility=False"
                     , app->getName(), (*it)->getNameInDocument());
    }
}

bool StdCmdHideObjects::isActive(void)
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
    sGroup        = QT_TR_NOOP("Standard-View");
    sMenuText     = QT_TR_NOOP("Appearance...");
    sToolTipText  = QT_TR_NOOP("Sets the display properties of the selected object");
    sWhatsThis    = "Std_SetAppearance";
    sStatusTip    = QT_TR_NOOP("Sets the display properties of the selected object");
    sPixmap       = "Std_Tool1";
    sAccel        = "Ctrl+D";
    eType         = Alter3DView;
}

void StdCmdSetAppearance::activated(int iMsg)
{
    Q_UNUSED(iMsg);
    static QPointer<QDialog> dlg = 0;
    if (!dlg)
        dlg = new Gui::Dialog::DlgDisplayPropertiesImp(getMainWindow());
    dlg->setModal(false);
    dlg->setAttribute(Qt::WA_DeleteOnClose);
    dlg->show();
}

bool StdCmdSetAppearance::isActive(void)
{
    return Gui::Selection().size() != 0;
}

//===========================================================================
// Std_ViewBottom
//===========================================================================
DEF_3DV_CMD(StdCmdViewBottom)

StdCmdViewBottom::StdCmdViewBottom()
  : Command("Std_ViewBottom")
{
    sGroup        = QT_TR_NOOP("Standard-View");
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
    sGroup        = QT_TR_NOOP("Standard-View");
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
    sGroup        = QT_TR_NOOP("Standard-View");
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
    sGroup        = QT_TR_NOOP("Standard-View");
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
    sGroup        = QT_TR_NOOP("Standard-View");
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
    sGroup        = QT_TR_NOOP("Standard-View");
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
    sGroup      = QT_TR_NOOP("Standard-View");
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
    sGroup      = QT_TR_NOOP("Standard-View");
    sMenuText   = QT_TR_NOOP("Dimetric");
    sToolTipText= QT_TR_NOOP("Set to dimetric view");
    sWhatsThis  = "Std_ViewDimetric";
    sStatusTip  = QT_TR_NOOP("Set to dimetric view");
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
    sGroup      = QT_TR_NOOP("Standard-View");
    sMenuText   = QT_TR_NOOP("Trimetric");
    sToolTipText= QT_TR_NOOP("Set to trimetric view");
    sWhatsThis  = "Std_ViewTrimetric";
    sStatusTip  = QT_TR_NOOP("Set to trimetric view");
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
    sGroup        = QT_TR_NOOP("Standard-View");
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
    sGroup        = QT_TR_NOOP("Standard-View");
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
    sGroup        = QT_TR_NOOP("Standard-View");
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

bool StdCmdViewFitAll::isActive(void)
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
    sGroup        = QT_TR_NOOP("Standard-View");
    sMenuText     = QT_TR_NOOP("Fit selection");
    sToolTipText  = QT_TR_NOOP("Fits the selected content on the screen");
    sWhatsThis    = "Std_ViewFitSelection";
    sStatusTip    = QT_TR_NOOP("Fits the selected content on the screen");
    sAccel        = "V, S";
#if QT_VERSION >= 0x040200
    sPixmap       = "zoom-selection";
#endif
    eType         = Alter3DView;
}

void StdCmdViewFitSelection::activated(int iMsg)
{
    Q_UNUSED(iMsg);
    //doCommand(Command::Gui,"Gui.activeDocument().activeView().fitAll()");
    doCommand(Command::Gui,"Gui.SendMsgToActiveView(\"ViewSelection\")");
}

bool StdCmdViewFitSelection::isActive(void)
{
  //return isViewOfType(Gui::View3DInventor::getClassTypeId());
  return getGuiApplication()->sendHasMsgToActiveView("ViewSelection");
}

//===========================================================================
// Std_ViewDock
//===========================================================================
DEF_STD_CMD_A(StdViewDock)

StdViewDock::StdViewDock()
  : Command("Std_ViewDock")
{
    sGroup       = QT_TR_NOOP("Standard-View");
    sMenuText    = QT_TR_NOOP("Docked");
    sToolTipText = QT_TR_NOOP("Display the active view either in fullscreen, in undocked or docked mode");
    sWhatsThis   = "Std_ViewDock";
    sStatusTip   = QT_TR_NOOP("Display the active view either in fullscreen, in undocked or docked mode");
    sAccel       = "V, D";
    eType        = Alter3DView;
}

void StdViewDock::activated(int iMsg)
{
    Q_UNUSED(iMsg);
}

bool StdViewDock::isActive(void)
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
    sGroup       = QT_TR_NOOP("Standard-View");
    sMenuText    = QT_TR_NOOP("Undocked");
    sToolTipText = QT_TR_NOOP("Display the active view either in fullscreen, in undocked or docked mode");
    sWhatsThis   = "Std_ViewUndock";
    sStatusTip   = QT_TR_NOOP("Display the active view either in fullscreen, in undocked or docked mode");
    sAccel       = "V, U";
    eType        = Alter3DView;
}

void StdViewUndock::activated(int iMsg)
{
    Q_UNUSED(iMsg);
}

bool StdViewUndock::isActive(void)
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
    sGroup       = QT_TR_NOOP("Standard-View");
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
    sGroup       = QT_TR_NOOP("Standard-View");
    sMenuText    = QT_TR_NOOP("Fullscreen");
    sToolTipText = QT_TR_NOOP("Display the active view either in fullscreen, in undocked or docked mode");
    sWhatsThis   = "Std_ViewFullscreen";
    sStatusTip   = QT_TR_NOOP("Display the active view either in fullscreen, in undocked or docked mode");
    sPixmap      = "view-fullscreen";
    sAccel       = "F11";
    eType        = Alter3DView;
}

void StdViewFullscreen::activated(int iMsg)
{
    Q_UNUSED(iMsg);
}

bool StdViewFullscreen::isActive(void)
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
    sGroup       = QT_TR_NOOP("Standard-View");
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

Action * StdViewDockUndockFullscreen::createAction(void)
{
    ActionGroup* pcAction = new ActionGroup(this, getMainWindow());
    pcAction->setDropDownMenu(true);
    pcAction->setText(QCoreApplication::translate(
        this->className(), sMenuText));

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
    if (!view) return; // no active view

#if defined(HAVE_QT5_OPENGL)
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

        const char* ppReturn = 0;
        if (view->onMsg("GetCamera", &ppReturn)) {
            std::string sMsg = "SetCamera ";
            sMsg += ppReturn;

            const char** pReturnIgnore=0;
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
#else
    if (iMsg==0) {
        view->setCurrentViewMode(MDIView::Child);
    }
    else if (iMsg==1) {
        if (view->currentViewMode() == MDIView::TopLevel)
            view->setCurrentViewMode(MDIView::Child);
        else
            view->setCurrentViewMode(MDIView::TopLevel);
    }
    else if (iMsg==2) {
        if (view->currentViewMode() == MDIView::FullScreen)
            view->setCurrentViewMode(MDIView::Child);
        else
            view->setCurrentViewMode(MDIView::FullScreen);
    }
#endif
}

bool StdViewDockUndockFullscreen::isActive(void)
{
    MDIView* view = getMainWindow()->activeWindow();
    if (qobject_cast<View3DInventor*>(view)) {
        // update the action group if needed
        ActionGroup* pActGrp = qobject_cast<ActionGroup*>(_pcAction);
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
    sGroup        = QT_TR_NOOP("Standard-View");
    sMenuText     = QT_TR_NOOP("FreeCAD-VR");
    sToolTipText  = QT_TR_NOOP("Extend the FreeCAD 3D Window to a Oculus Rift");
    sWhatsThis    = "Std_ViewVR";
    sStatusTip    = QT_TR_NOOP("Extend the FreeCAD 3D Window to a Oculus Rift");
    eType         = Alter3DView;
}

void StdCmdViewVR::activated(int iMsg)
{
    Q_UNUSED(iMsg);
  //doCommand(Command::Gui,"Gui.activeDocument().activeView().fitAll()");
   doCommand(Command::Gui,"Gui.SendMsgToActiveView(\"ViewVR\")");
}

bool StdCmdViewVR::isActive(void)
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
    sGroup      = QT_TR_NOOP("Standard-View");
    sMenuText   = QT_TR_NOOP("Save picture...");
    sToolTipText= QT_TR_NOOP("Creates a screenshot of the active view");
    sWhatsThis  = "Std_ViewScreenShot";
    sStatusTip  = QT_TR_NOOP("Creates a screenshot of the active view");
    sPixmap     = "camera-photo";
    eType         = Alter3DView;
}

void StdViewScreenShot::activated(int iMsg)
{
    Q_UNUSED(iMsg);
    View3DInventor* view = qobject_cast<View3DInventor*>(getMainWindow()->activeWindow());
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
        int backtype = hExt->GetInt("OffscreenImageBackground",0);

        QStringList filter;
        QString selFilter;
        for (QStringList::Iterator it = formats.begin(); it != formats.end(); ++it) {
            filter << QString::fromLatin1("%1 %2 (*.%3)").arg((*it).toUpper(),
                QObject::tr("files"), (*it).toLower());
            if (ext == *it)
                selFilter = filter.last();
        }

        FileOptionsDialog fd(getMainWindow(), 0);
        fd.setFileMode(QFileDialog::AnyFile);
        fd.setAcceptMode(QFileDialog::AcceptSave);
        fd.setWindowTitle(QObject::tr("Save picture"));
        fd.setNameFilters(filter);
        if (!selFilter.isEmpty())
            fd.selectNameFilter(selFilter);

        // create the image options widget
        DlgSettingsImageImp* opt = new DlgSettingsImageImp(&fd);
        SbVec2s sz = vp.getWindowSize();
        opt->setImageSize((int)sz[0], (int)sz[1]);
        opt->setBackgroundType(backtype);

        fd.setOptionsWidget(FileOptionsDialog::ExtensionRight, opt);
        fd.setConfirmOverwrite(true);
        opt->onSelectedFilter(fd.selectedNameFilter());
        QObject::connect(&fd, SIGNAL(filterSelected(const QString&)),
                         opt, SLOT(onSelectedFilter(const QString&)));

        if (fd.exec() == QDialog::Accepted) {
            selFilter = fd.selectedNameFilter();
            QString fn = fd.selectedFiles().front();
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

            // which background chosen
            const char* background;
            switch(opt->backgroundType()){
                case 0:  background="Current"; break;
                case 1:  background="White"; break;
                case 2:  background="Black"; break;
                case 3:  background="Transparent"; break;
                default: background="Current"; break;
            }
            hExt->SetInt("OffscreenImageBackground",opt->backgroundType());

            QString comment = opt->comment();
            if (!comment.isEmpty()) {
                // Replace newline escape sequence trough '\\n' string to build one big string,
                // otherwise Python would interpret it as an invalid command.
                // Python does the decoding for us.
                QStringList lines = comment.split(QLatin1String("\n"), QString::KeepEmptyParts );
                    comment = lines.join(QLatin1String("\\n"));
                doCommand(Gui,"Gui.activeDocument().activeView().saveImage('%s',%d,%d,'%s','%s')",
                            fn.toUtf8().constData(),w,h,background,comment.toUtf8().constData());
            }
            else {
                doCommand(Gui,"Gui.activeDocument().activeView().saveImage('%s',%d,%d,'%s')",
                            fn.toUtf8().constData(),w,h,background);
            }

            // When adding a watermark check if the image could be created
            if (opt->addWatermark()) {
                QFileInfo fi(fn);
                QPixmap pixmap;
                if (fi.exists() && pixmap.load(fn)) {
                    QString name = qApp->applicationName();
                    std::map<std::string, std::string>& config = App::Application::Config();
                    QString url  = QString::fromLatin1(config["MaintainerUrl"].c_str());
                    url = QUrl(url).host();

                    QPixmap appicon = Gui::BitmapFactory().pixmap(config["AppIcon"].c_str());

                    QPainter painter;
                    painter.begin(&pixmap);

                    painter.drawPixmap(8, h-15-appicon.height(), appicon);

                    QFont font = painter.font();
                    font.setPointSize(20);

                    int n = QFontMetrics(font).width(name);
                    int h = pixmap.height();

                    painter.setFont(font);
                    painter.drawText(8+appicon.width(), h-24, name);

                    font.setPointSize(12);
                    int u = QFontMetrics(font).width(url);
                    painter.setFont(font);
                    painter.drawText(8+appicon.width()+n-u, h-9, url);

                    painter.end();
                    pixmap.save(fn);
                }
            }
        }
    }
}

bool StdViewScreenShot::isActive(void)
{
    return isViewOfType(Gui::View3DInventor::getClassTypeId());
}


//===========================================================================
// Std_ViewCreate
//===========================================================================
DEF_STD_CMD_A(StdCmdViewCreate)

StdCmdViewCreate::StdCmdViewCreate()
  : Command("Std_ViewCreate")
{
    sGroup      = QT_TR_NOOP("Standard-View");
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

    ParameterGrp::handle hViewGrp = App::GetApplication().GetParameterGroupByPath("User parameter:BaseApp/Preferences/View");
    if (hViewGrp->GetBool("ShowAxisCross"))
        doCommand(Command::Gui,"Gui.ActiveDocument.ActiveView.setAxisCross(True)");
}

bool StdCmdViewCreate::isActive(void)
{
    return (getActiveGuiDocument()!=NULL);
}

//===========================================================================
// Std_ToggleNavigation
//===========================================================================
DEF_STD_CMD_A(StdCmdToggleNavigation)

StdCmdToggleNavigation::StdCmdToggleNavigation()
  : Command("Std_ToggleNavigation")
{
    sGroup        = QT_TR_NOOP("Standard-View");
    sMenuText     = QT_TR_NOOP("Toggle navigation/Edit mode");
    sToolTipText  = QT_TR_NOOP("Toggle between navigation and edit mode");
    sStatusTip    = QT_TR_NOOP("Toggle between navigation and edit mode");
    sWhatsThis    = "Std_ToggleNavigation";
  //iAccel        = Qt::SHIFT+Qt::Key_Space;
    sAccel        = "Esc";
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

bool StdCmdToggleNavigation::isActive(void)
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



#if 0 // old Axis command
// Command to show/hide axis cross
class StdCmdAxisCross : public Gui::Command
{
private:
    SoShapeScale* axisCross;
    SoGroup* axisGroup;
public:
    StdCmdAxisCross() : Command("Std_AxisCross"), axisCross(0), axisGroup(0)
    {
        sGroup        = QT_TR_NOOP("Standard-View");
        sMenuText     = QT_TR_NOOP("Toggle axis cross");
        sToolTipText  = QT_TR_NOOP("Toggle axis cross");
        sStatusTip    = QT_TR_NOOP("Toggle axis cross");
        sWhatsThis    = "Std_AxisCross";
    }
    ~StdCmdAxisCross()
    {
        if (axisGroup)
            axisGroup->unref();
        if (axisCross)
            axisCross->unref();
    }
    const char* className() const
    { return "StdCmdAxisCross"; }

    Action * createAction(void)
    {
        axisCross = new Gui::SoShapeScale;
        axisCross->ref();
        Gui::SoAxisCrossKit* axisKit = new Gui::SoAxisCrossKit();
        axisKit->set("xAxis.appearance.drawStyle", "lineWidth 2");
        axisKit->set("yAxis.appearance.drawStyle", "lineWidth 2");
        axisKit->set("zAxis.appearance.drawStyle", "lineWidth 2");
        axisCross->setPart("shape", axisKit);
        axisGroup = new SoSkipBoundingGroup;
        axisGroup->ref();
        axisGroup->addChild(axisCross);

        Action *pcAction = Gui::Command::createAction();
        pcAction->setCheckable(true);
        return pcAction;
    }

protected:
    void activated(int iMsg)
    {
        float scale = 1.0f;

        Gui::View3DInventor* view = qobject_cast<Gui::View3DInventor*>
            (getMainWindow()->activeWindow());
        if (view) {
            SoNode* scene = view->getViewer()->getSceneGraph();
            SoSeparator* sep = static_cast<SoSeparator*>(scene);
            bool hasaxis = (sep->findChild(axisGroup) != -1);
            if (iMsg > 0 && !hasaxis) {
                axisCross->scaleFactor = scale;
                sep->addChild(axisGroup);
            }
            else if (iMsg == 0 && hasaxis) {
                sep->removeChild(axisGroup);
            }
        }
    }

    bool isActive(void)
    {
        Gui::View3DInventor* view = qobject_cast<View3DInventor*>(Gui::getMainWindow()->activeWindow());
        if (view) {
            Gui::View3DInventorViewer* viewer = view->getViewer();
            if (!viewer)
                return false; // no active viewer
            SoGroup* group = dynamic_cast<SoGroup*>(viewer->getSceneGraph());
            if (!group)
                return false; // empty scene graph
            bool hasaxis = group->findChild(axisGroup) != -1;
            if (_pcAction->isChecked() != hasaxis)
                _pcAction->setChecked(hasaxis);
            return true;
        }
        else {
            if (_pcAction->isChecked())
                _pcAction->setChecked(false);
            return false;
        }
    }
};
#else
//===========================================================================
// Std_ViewExample1
//===========================================================================
DEF_STD_CMD_A(StdCmdAxisCross)

StdCmdAxisCross::StdCmdAxisCross()
  : Command("Std_AxisCross")
{
        sGroup        = QT_TR_NOOP("Standard-View");
        sMenuText     = QT_TR_NOOP("Toggle axis cross");
        sToolTipText  = QT_TR_NOOP("Toggle axis cross");
        sStatusTip    = QT_TR_NOOP("Toggle axis cross");
        sWhatsThis    = "Std_AxisCross";
}

void StdCmdAxisCross::activated(int iMsg)
{
    Q_UNUSED(iMsg);
    Gui::View3DInventor* view = qobject_cast<View3DInventor*>(Gui::getMainWindow()->activeWindow());
    if (view) {
        if (view->getViewer()->hasAxisCross() == false)
            doCommand(Command::Gui,"Gui.ActiveDocument.ActiveView.setAxisCross(True)");
        else
            doCommand(Command::Gui,"Gui.ActiveDocument.ActiveView.setAxisCross(False)");

        ParameterGrp::handle hViewGrp = App::GetApplication().GetParameterGroupByPath("User parameter:BaseApp/Preferences/View");
        hViewGrp->SetBool("ShowAxisCross", view->getViewer()->hasAxisCross());
    }
}

bool StdCmdAxisCross::isActive(void)
{
    Gui::View3DInventor* view = qobject_cast<View3DInventor*>(Gui::getMainWindow()->activeWindow());
    if (view && view->getViewer()->hasAxisCross()) {
        if (!_pcAction->isChecked())
            _pcAction->setChecked(true);
    }
    else {
        if (_pcAction->isChecked())
            _pcAction->setChecked(false);
    }
    if (view ) return true;
    return false;

}

#endif

//===========================================================================
// Std_ViewExample1
//===========================================================================
DEF_STD_CMD_A(StdCmdViewExample1)

StdCmdViewExample1::StdCmdViewExample1()
  : Command("Std_ViewExample1")
{
    sGroup        = QT_TR_NOOP("Standard-View");
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

bool StdCmdViewExample1::isActive(void)
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
    sGroup        = QT_TR_NOOP("Standard-View");
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

bool StdCmdViewExample2::isActive(void)
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
    sGroup        = QT_TR_NOOP("Standard-View");
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

bool StdCmdViewExample3::isActive(void)
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
    sGroup        = QT_TR_NOOP("Standard-View");
    sMenuText     = QT_TR_NOOP("Stereo Off");
    sToolTipText  = QT_TR_NOOP("Switch stereo viewing off");
    sWhatsThis    = "Std_ViewIvStereoOff";
    sStatusTip    = QT_TR_NOOP("Switch stereo viewing off");
    sPixmap       = "Std_Tool6";
    eType         = Alter3DView;
}

void StdCmdViewIvStereoOff::activated(int iMsg)
{
    Q_UNUSED(iMsg);
    doCommand(Command::Gui,"Gui.activeDocument().activeView().setStereoType(\"None\")");
}

bool StdCmdViewIvStereoOff::isActive(void)
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
    sGroup        = QT_TR_NOOP("Standard-View");
    sMenuText     = QT_TR_NOOP("Stereo red/cyan");
    sToolTipText  = QT_TR_NOOP("Switch stereo viewing to red/cyan");
    sWhatsThis    = "Std_ViewIvStereoRedGreen";
    sStatusTip    = QT_TR_NOOP("Switch stereo viewing to red/cyan");
    sPixmap       = "Std_Tool7";
    eType         = Alter3DView;
}

void StdCmdViewIvStereoRedGreen::activated(int iMsg)
{
    Q_UNUSED(iMsg);
    doCommand(Command::Gui,"Gui.activeDocument().activeView().setStereoType(\"Anaglyph\")");
}

bool StdCmdViewIvStereoRedGreen::isActive(void)
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
    sGroup        = QT_TR_NOOP("Standard-View");
    sMenuText     = QT_TR_NOOP("Stereo quad buffer");
    sToolTipText  = QT_TR_NOOP("Switch stereo viewing to quad buffer");
    sWhatsThis    = "Std_ViewIvStereoQuadBuff";
    sStatusTip    = QT_TR_NOOP("Switch stereo viewing to quad buffer");
    sPixmap       = "Std_Tool7";
    eType         = Alter3DView;
}

void StdCmdViewIvStereoQuadBuff::activated(int iMsg)
{
    Q_UNUSED(iMsg);
    doCommand(Command::Gui,"Gui.activeDocument().activeView().setStereoType(\"QuadBuffer\")");
}

bool StdCmdViewIvStereoQuadBuff::isActive(void)
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
    sGroup        = QT_TR_NOOP("Standard-View");
    sMenuText     = QT_TR_NOOP("Stereo Interleaved Rows");
    sToolTipText  = QT_TR_NOOP("Switch stereo viewing to Interleaved Rows");
    sWhatsThis    = "Std_ViewIvStereoInterleavedRows";
    sStatusTip    = QT_TR_NOOP("Switch stereo viewing to Interleaved Rows");
    sPixmap       = "Std_Tool7";
    eType         = Alter3DView;
}

void StdCmdViewIvStereoInterleavedRows::activated(int iMsg)
{
    Q_UNUSED(iMsg);
    doCommand(Command::Gui,"Gui.activeDocument().activeView().setStereoType(\"InterleavedRows\")");
}

bool StdCmdViewIvStereoInterleavedRows::isActive(void)
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
    sGroup        = QT_TR_NOOP("Standard-View");
    sMenuText     = QT_TR_NOOP("Stereo Interleaved Columns");
    sToolTipText  = QT_TR_NOOP("Switch stereo viewing to Interleaved Columns");
    sWhatsThis    = "Std_ViewIvStereoInterleavedColumns";
    sStatusTip    = QT_TR_NOOP("Switch stereo viewing to Interleaved Columns");
    sPixmap       = "Std_Tool7";
    eType         = Alter3DView;
}

void StdCmdViewIvStereoInterleavedColumns::activated(int iMsg)
{
    Q_UNUSED(iMsg);
    doCommand(Command::Gui,"Gui.activeDocument().activeView().setStereoType(\"InterleavedColumns\")");
}

bool StdCmdViewIvStereoInterleavedColumns::isActive(void)
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
    sGroup        = QT_TR_NOOP("Standard-View");
    sMenuText     = QT_TR_NOOP("Issue camera position");
    sToolTipText  = QT_TR_NOOP("Issue the camera position to the console and to a macro, to easily recall this position");
    sWhatsThis    = "Std_ViewIvIssueCamPos";
    sStatusTip    = QT_TR_NOOP("Issue the camera position to the console and to a macro, to easily recall this position");
    sPixmap       = "Std_Tool8";
    eType         = Alter3DView;
}

void StdCmdViewIvIssueCamPos::activated(int iMsg)
{
    Q_UNUSED(iMsg);
    std::string Temp,Temp2;
    std::string::size_type pos;

    const char* ppReturn=0;
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

bool StdCmdViewIvIssueCamPos::isActive(void)
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
    sGroup        = QT_TR_NOOP("Standard-View");
    sMenuText     = QT_TR_NOOP("Zoom In");
    sToolTipText  = QT_TR_NOOP("Zoom In");
    sWhatsThis    = "Std_ViewZoomIn";
    sStatusTip    = QT_TR_NOOP("Zoom In");
#if QT_VERSION >= 0x040200
    sPixmap       = "zoom-in";
#endif
    sAccel        = keySequenceToAccel(QKeySequence::ZoomIn);
    eType         = Alter3DView;
}

void StdViewZoomIn::activated(int iMsg)
{
    Q_UNUSED(iMsg);
    View3DInventor* view = qobject_cast<View3DInventor*>(getMainWindow()->activeWindow());
    if ( view ) {
        View3DInventorViewer* viewer = view->getViewer();
        viewer->navigationStyle()->zoomIn();
    }
}

bool StdViewZoomIn::isActive(void)
{
    return (qobject_cast<View3DInventor*>(getMainWindow()->activeWindow()));
}

//===========================================================================
// Std_ViewZoomOut
//===========================================================================
DEF_STD_CMD_A(StdViewZoomOut)

StdViewZoomOut::StdViewZoomOut()
  : Command("Std_ViewZoomOut")
{
    sGroup        = QT_TR_NOOP("Standard-View");
    sMenuText     = QT_TR_NOOP("Zoom Out");
    sToolTipText  = QT_TR_NOOP("Zoom Out");
    sWhatsThis    = "Std_ViewZoomOut";
    sStatusTip    = QT_TR_NOOP("Zoom Out");
#if QT_VERSION >= 0x040200
    sPixmap       = "zoom-out";
#endif
    sAccel        = keySequenceToAccel(QKeySequence::ZoomOut);
    eType         = Alter3DView;
}

void StdViewZoomOut::activated(int iMsg)
{
    Q_UNUSED(iMsg);
    View3DInventor* view = qobject_cast<View3DInventor*>(getMainWindow()->activeWindow());
    if (view) {
        View3DInventorViewer* viewer = view->getViewer();
        viewer->navigationStyle()->zoomOut();
    }
}

bool StdViewZoomOut::isActive(void)
{
    return (qobject_cast<View3DInventor*>(getMainWindow()->activeWindow()));
}

//===========================================================================
// Std_ViewBoxZoom
//===========================================================================
DEF_3DV_CMD(StdViewBoxZoom)

StdViewBoxZoom::StdViewBoxZoom()
  : Command("Std_ViewBoxZoom")
{
    sGroup        = QT_TR_NOOP("Standard-View");
    sMenuText     = QT_TR_NOOP("Box zoom");
    sToolTipText  = QT_TR_NOOP("Box zoom");
    sWhatsThis    = "Std_ViewBoxZoom";
    sStatusTip    = QT_TR_NOOP("Box zoom");
#if QT_VERSION >= 0x040200
    sPixmap       = "zoom-border";
#endif
    sAccel        = "Ctrl+B";
    eType         = Alter3DView;
}

void StdViewBoxZoom::activated(int iMsg)
{
    Q_UNUSED(iMsg);
    View3DInventor* view = qobject_cast<View3DInventor*>(getMainWindow()->activeWindow());
    if ( view ) {
        View3DInventorViewer* viewer = view->getViewer();
        if (!viewer->isSelecting())
            viewer->startSelection(View3DInventorViewer::BoxZoom);
    }
}

//===========================================================================
// Std_BoxSelection
//===========================================================================
DEF_3DV_CMD(StdBoxSelection)

StdBoxSelection::StdBoxSelection()
  : Command("Std_BoxSelection")
{
    sGroup        = QT_TR_NOOP("Standard-View");
    sMenuText     = QT_TR_NOOP("Box selection");
    sToolTipText  = QT_TR_NOOP("Box selection");
    sWhatsThis    = "Std_BoxSelection";
    sStatusTip    = QT_TR_NOOP("Box selection");
#if QT_VERSION >= 0x040200
    sPixmap       = "edit-select-box";
#endif
    sAccel        = "Shift+B";
    eType         = AlterSelection;
}

static void selectionCallback(void * ud, SoEventCallback * cb)
{
    Gui::View3DInventorViewer* view  = reinterpret_cast<Gui::View3DInventorViewer*>(cb->getUserData());
    view->removeEventCallback(SoMouseButtonEvent::getClassTypeId(), selectionCallback, ud);
    SoNode* root = view->getSceneGraph();
    static_cast<Gui::SoFCUnifiedSelection*>(root)->selectionRole.setValue(true);

    typedef enum { CENTER, INTERSECT } SelectionMode;
    SelectionMode selectionMode = CENTER;

    std::vector<SbVec2f> picked = view->getGLPolygon();
    SoCamera* cam = view->getSoRenderManager()->getCamera();
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
        for (std::vector<SbVec2f>::const_iterator it = picked.begin(); it != picked.end(); ++it)
            polygon.Add(Base::Vector2d((*it)[0],(*it)[1]));
    }

    App::Document* doc = App::GetApplication().getActiveDocument();
    if (doc) {
        cb->setHandled();

        const SoEvent* ev = cb->getEvent();
        if (ev && !ev->wasCtrlDown()) {
            Gui::Selection().clearSelection(doc->getName());
        }

        std::vector<App::DocumentObject*> geom = doc->getObjectsOfType<App::DocumentObject>();
        for (std::vector<App::DocumentObject*>::iterator it = geom.begin(); it != geom.end(); ++it) {
            Gui::ViewProvider* vp = Application::Instance->getViewProvider(*it);
            if (!vp->isVisible())
                continue;
            // 0002706: For box selection use SoGetBoundingBoxAction for
            // the view provider of an object
            SoGetBoundingBoxAction bboxAction(view->getViewportRegion());
            bboxAction.apply(vp->getRoot());
            SbBox3f bbox = bboxAction.getBoundingBox();
            Base::BoundBox3d bbox3;
            bbox3.Add(Base::convertTo<Base::Vector3d>(bbox.getMax()));
            bbox3.Add(Base::convertTo<Base::Vector3d>(bbox.getMin()));

            if (selectionMode == CENTER) {
                Base::Vector3d pt2d;
                pt2d = proj(bbox3.GetCenter());
                if (polygon.Contains(Base::Vector2d(pt2d.x, pt2d.y))) {
                    Gui::Selection().addSelection(doc->getName(), (*it)->getNameInDocument());
                }
            }
            else {
                Base::BoundBox2d bbox2 = bbox3.ProjectBox(&proj);
                if (bbox2.Intersect(polygon)) {
                    Gui::Selection().addSelection(doc->getName(), (*it)->getNameInDocument());
                }
            }
        }
    }
}

void StdBoxSelection::activated(int iMsg)
{
    Q_UNUSED(iMsg);
    View3DInventor* view = qobject_cast<View3DInventor*>(getMainWindow()->activeWindow());
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
            viewer->startSelection(View3DInventorViewer::Rubberband);
            viewer->addEventCallback(SoMouseButtonEvent::getClassTypeId(), selectionCallback);
            SoNode* root = viewer->getSceneGraph();
            static_cast<Gui::SoFCUnifiedSelection*>(root)->selectionRole.setValue(false);
        }
    }
}

//===========================================================================
// Std_TreeSelection
//===========================================================================

DEF_STD_CMD(StdCmdTreeSelection)

StdCmdTreeSelection::StdCmdTreeSelection()
  : Command("Std_TreeSelection")
{
    sGroup        = QT_TR_NOOP("View");
    sMenuText     = QT_TR_NOOP("Go to selection");
    sToolTipText  = QT_TR_NOOP("Scroll to first selected item");
    sWhatsThis    = "Std_TreeSelection";
    sStatusTip    = QT_TR_NOOP("Scroll to first selected item");
    eType         = Alter3DView;
}

void StdCmdTreeSelection::activated(int iMsg)
{
    Q_UNUSED(iMsg);
    QList<TreeWidget*> tree = Gui::getMainWindow()->findChildren<TreeWidget*>();
    for (QList<TreeWidget*>::iterator it = tree.begin(); it != tree.end(); ++it) {
        Gui::Document* doc = Gui::Application::Instance->activeDocument();
        (*it)->scrollItemToTop(doc);
    }
}

//===========================================================================
// Std_MeasureDistance
//===========================================================================

DEF_STD_CMD_A(StdCmdMeasureDistance)

StdCmdMeasureDistance::StdCmdMeasureDistance()
  : Command("Std_MeasureDistance")
{
    sGroup        = QT_TR_NOOP("View");
    sMenuText     = QT_TR_NOOP("Measure distance");
    sToolTipText  = QT_TR_NOOP("Measure distance");
    sWhatsThis    = "Std_MeasureDistance";
    sStatusTip    = QT_TR_NOOP("Measure distance");
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
    Gui::View3DInventor* view = static_cast<Gui::View3DInventor*>(doc->getActiveView());
    if (view) {
        Gui::View3DInventorViewer* viewer = view->getViewer();
        viewer->setEditing(true);
        viewer->setEditingCursor(QCursor(QPixmap(cursor_ruler), 7, 7));

        // Derives from QObject and we have a parent object, so we don't
        // require a delete.
        PointMarker* marker = new PointMarker(viewer);
        viewer->addEventCallback(SoEvent::getClassTypeId(),
            ViewProviderMeasureDistance::measureDistanceCallback, marker);
     }
}

bool StdCmdMeasureDistance::isActive(void)
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
    sGroup        = QT_TR_NOOP("Tools");
    sMenuText     = QT_TR_NOOP("Scene inspector...");
    sToolTipText  = QT_TR_NOOP("Scene inspector");
    sWhatsThis    = "Std_SceneInspector";
    sStatusTip    = QT_TR_NOOP("Scene inspector");
    eType         = Alter3DView;
}

void StdCmdSceneInspector::activated(int iMsg)
{
    Q_UNUSED(iMsg);
    Gui::Document* doc = Application::Instance->activeDocument();
    if (doc) {
        static QPointer<Gui::Dialog::DlgInspector> dlg = 0;
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
    sGroup        = QT_TR_NOOP("Tools");
    sMenuText     = QT_TR_NOOP("Texture mapping...");
    sToolTipText  = QT_TR_NOOP("Texture mapping");
    sWhatsThis    = "Std_TextureMapping";
    sStatusTip    = QT_TR_NOOP("Texture mapping");
    eType         = Alter3DView;
}

void StdCmdTextureMapping::activated(int iMsg)
{
    Q_UNUSED(iMsg);
    Gui::Control().showDialog(new Gui::Dialog::TaskTextureMapping);
}

bool StdCmdTextureMapping::isActive(void)
{
    Gui::MDIView* view = getMainWindow()->activeWindow();
    return view && view->isDerivedFrom(Gui::View3DInventor::getClassTypeId())
                && (Gui::Control().activeDialog()==0);
}

DEF_STD_CMD(StdCmdDemoMode)

StdCmdDemoMode::StdCmdDemoMode()
  : Command("Std_DemoMode")
{
    sGroup        = QT_TR_NOOP("Standard-View");
    sMenuText     = QT_TR_NOOP("View turntable...");
    sToolTipText  = QT_TR_NOOP("View turntable");
    sWhatsThis    = "Std_DemoMode";
    sStatusTip    = QT_TR_NOOP("View turntable");
    eType         = Alter3DView;
}

void StdCmdDemoMode::activated(int iMsg)
{
    Q_UNUSED(iMsg);
    static QPointer<QDialog> dlg = 0;
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
    sGroup        = QT_TR_NOOP("Measure");
    sMenuText     = QT_TR_NOOP("Clear measurement");
    sToolTipText  = QT_TR_NOOP("Clear measurement");
    sWhatsThis    = "View_Measure_Clear_All";
    sStatusTip    = sToolTipText;
    sPixmap       = "Part_Measure_Clear_All";
}

void CmdViewMeasureClearAll::activated(int iMsg)
{
    Q_UNUSED(iMsg);
    Gui::View3DInventor *view = dynamic_cast<Gui::View3DInventor*>(Gui::Application::Instance->
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
    sGroup        = QT_TR_NOOP("Measure");
    sMenuText     = QT_TR_NOOP("Toggle measurement");
    sToolTipText  = QT_TR_NOOP("Toggle measurement");
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
// Std_TreeSingleDocument
//===========================================================================
DEF_STD_CMD(StdTreeSingleDocument)

StdTreeSingleDocument::StdTreeSingleDocument()
  : Command("Std_TreeSingleDocument")
{
    sGroup       = QT_TR_NOOP("View");
    sMenuText    = QT_TR_NOOP("Single Document");
    sToolTipText = QT_TR_NOOP("Only display the active document in the tree view");
    sWhatsThis   = "Std_TreeSingleDocument";
    sStatusTip   = QT_TR_NOOP("Only display the active document in the tree view");
    eType        = 0;
}

void StdTreeSingleDocument::activated(int iMsg)
{
    Q_UNUSED(iMsg);
}

//===========================================================================
// Std_TreeMultiDocument
//===========================================================================
DEF_STD_CMD(StdTreeMultiDocument)

StdTreeMultiDocument::StdTreeMultiDocument()
  : Command("Std_TreeMultiDocument")
{
    sGroup       = QT_TR_NOOP("View");
    sMenuText    = QT_TR_NOOP("Multi Document");
    sToolTipText = QT_TR_NOOP("Display all documents in the tree view");
    sWhatsThis   = "Std_TreeMultiDocument";
    sStatusTip   = QT_TR_NOOP("Display all documents in the tree view");
    eType        = 0;
}

void StdTreeMultiDocument::activated(int iMsg)
{
    Q_UNUSED(iMsg);
}


//===========================================================================
// Std_TreeCollapseDocument
//===========================================================================
DEF_STD_CMD(StdTreeCollapseDocument)

StdTreeCollapseDocument::StdTreeCollapseDocument()
  : Command("Std_TreeCollapseDocument")
{
    sGroup       = QT_TR_NOOP("View");
    sMenuText    = QT_TR_NOOP("Collapse/Expand");
    sToolTipText = QT_TR_NOOP("Expand active document and collapse all others");
    sWhatsThis   = "Std_TreeCollapseDocument";
    sStatusTip   = QT_TR_NOOP("Expand active document and collapse all others");
    eType        = 0;
}

void StdTreeCollapseDocument::activated(int iMsg)
{
    Q_UNUSED(iMsg);
}


//===========================================================================
// Std_TreeViewDocument
//===========================================================================

DEF_STD_CMD_AC(StdTreeViewDocument);

StdTreeViewDocument::StdTreeViewDocument()
  : Command("Std_TreeViewDocument")
{
    sGroup        = QT_TR_NOOP("View");
    sMenuText     = QT_TR_NOOP("Document Tree");
    sToolTipText  = QT_TR_NOOP("Set visibility of inactive documents in tree view");
    sWhatsThis    = "Std_TreeViewDocument";
    sStatusTip    = QT_TR_NOOP("Set visibility of inactive documents in tree view");
    eType         = 0;

    CommandManager &rcCmdMgr = Application::Instance->commandManager();
    rcCmdMgr.addCommand(new StdTreeSingleDocument());
    rcCmdMgr.addCommand(new StdTreeMultiDocument());
    rcCmdMgr.addCommand(new StdTreeCollapseDocument());
}

Action * StdTreeViewDocument::createAction(void)
{
    ActionGroup* pcAction = new ActionGroup(this, getMainWindow());
    pcAction->setDropDownMenu(true);
    pcAction->setText(QCoreApplication::translate(this->className(), sMenuText));

    CommandManager &grp = Application::Instance->commandManager();
    Command* cmd0 = grp.getCommandByName("Std_TreeSingleDocument");
    Command* cmd1 = grp.getCommandByName("Std_TreeMultiDocument");
    Command* cmd2 = grp.getCommandByName("Std_TreeCollapseDocument");
    cmd0->addToGroup(pcAction, true);
    cmd1->addToGroup(pcAction, true);
    cmd2->addToGroup(pcAction, true);

    return pcAction;
}

void StdTreeViewDocument::activated(int iMsg)
{
    ParameterGrp::handle group = App::GetApplication().GetUserParameter().
    GetGroup("BaseApp")->GetGroup("Preferences")->GetGroup("View");
    group->SetInt("TreeViewDocument", iMsg);
    App::GetApplication().setActiveDocument(App::GetApplication().getActiveDocument());
}


bool StdTreeViewDocument::isActive(void)
{
    ActionGroup* grp = qobject_cast<ActionGroup*>(_pcAction);
    if (grp) {
        ParameterGrp::handle group = App::GetApplication().GetUserParameter().
        GetGroup("BaseApp")->GetGroup("Preferences")->GetGroup("View");
        int mode = group->GetInt("TreeViewDocument", 0);
        int index = grp->checkedAction();
        if (index != mode) {
            grp->setCheckedAction(mode);
        }
    }

    return true;
}

//===========================================================================
// Instantiation
//===========================================================================


namespace Gui {

void CreateViewStdCommands(void)
{
    CommandManager &rcCmdMgr = Application::Instance->commandManager();

    // views
    rcCmdMgr.addCommand(new StdCmdViewBottom());
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
    rcCmdMgr.addCommand(new StdCmdFreezeViews());
    rcCmdMgr.addCommand(new StdViewZoomIn());
    rcCmdMgr.addCommand(new StdViewZoomOut());
    rcCmdMgr.addCommand(new StdViewBoxZoom());
    rcCmdMgr.addCommand(new StdBoxSelection());
    rcCmdMgr.addCommand(new StdCmdTreeSelection());
    rcCmdMgr.addCommand(new StdCmdMeasureDistance());
    rcCmdMgr.addCommand(new StdCmdSceneInspector());
    rcCmdMgr.addCommand(new StdCmdTextureMapping());
    rcCmdMgr.addCommand(new StdCmdDemoMode());
    rcCmdMgr.addCommand(new StdCmdToggleNavigation());
    rcCmdMgr.addCommand(new StdCmdAxisCross());
    rcCmdMgr.addCommand(new CmdViewMeasureClearAll());
    rcCmdMgr.addCommand(new CmdViewMeasureToggleAll());
    rcCmdMgr.addCommand(new StdTreeViewDocument());
}

} // namespace Gui
