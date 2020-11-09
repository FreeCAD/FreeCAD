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
# include <QApplication>
# include <QDialog>
# include <QFile>
# include <QFileInfo>
# include <QFont>
# include <QFontMetrics>
# include <QMessageBox>
# include <QPainter>
# include <QPointer>
# include <QTextStream>
# include <boost_bind_bind.hpp>
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
#include "ViewParams.h"
#include "WaitCursor.h"
#include "ViewProviderMeasureDistance.h"
#include "ViewProviderGeometryObject.h"
#include "SceneInspector.h"
#include "DemoMode.h"
#include "TextureMapping.h"
#include "Tools.h"
#include "Utilities.h"
#include "NavigationStyle.h"
#include "OverlayWidgets.h"
#include "SelectionView.h"
#include "MouseSelection.h"

#include <Base/Console.h>
#include <Base/Tools2D.h>
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
#include <App/ComplexGeoDataPy.h>
#include <App/GeoFeatureGroupExtension.h>
#include <App/DocumentObserver.h>

#include <QDomDocument>
#include <QDomElement>

using namespace Gui;
using Gui::Dialog::DlgSettingsImageImp;
namespace bp = boost::placeholders;

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
    saveView->setWhatsThis(QString::fromLatin1(sWhatsThis));
    QAction* loadView = pcAction->addAction(QObject::tr("Load views..."));
    loadView->setWhatsThis(QString::fromLatin1(sWhatsThis));
    pcAction->addAction(QString::fromLatin1(""))->setSeparator(true);
    freezeView = pcAction->addAction(QObject::tr("Freeze view"));
    freezeView->setShortcut(QString::fromLatin1(sAccel));
    freezeView->setWhatsThis(QString::fromLatin1(sWhatsThis));
    clearView = pcAction->addAction(QObject::tr("Clear views"));
    clearView->setWhatsThis(QString::fromLatin1(sWhatsThis));
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
            if (!data.isEmpty()) {
                QStringList lines = data.split(QString::fromLatin1("\n"));
                if (lines.size() > 1) {
                    lines.pop_front();
                }
                viewPos = lines.join(QString::fromLatin1(" "));
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
    static QPointer<Gui::Dialog::Clipping> clipping = nullptr;
    if (!clipping) {
        View3DInventor* view = qobject_cast<View3DInventor*>(getMainWindow()->activeWindow());
        if (view) {
            clipping = Gui::Dialog::Clipping::makeDockWidget(view);
        }
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
    View3DInventor* view = qobject_cast<View3DInventor*>(getMainWindow()->activeWindow());
    return view ? true : false;
#endif
}

//===========================================================================
// StdCmdDrawStyleBase
//===========================================================================

class StdCmdDrawStyleBase : public Command 
{
public:
    StdCmdDrawStyleBase(const char *name, const char *title, 
                        const char *doc, const char *shortcut, const char *pixmap);

    virtual const char* className() const;

protected: 
    bool isActive(void);
    virtual void activated(int iMsg);
};

StdCmdDrawStyleBase::StdCmdDrawStyleBase(const char *name, const char *title, 
                                         const char *doc, const char *shortcut,
                                         const char *pixmap)
    :Command(name)
{
    sGroup        = QT_TR_NOOP("Standard-View");
    sMenuText     = title;
    sToolTipText  = doc;
    sStatusTip    = sToolTipText;
    sWhatsThis    = name;
    sPixmap       = pixmap;
    sAccel        = shortcut;
    eType         = Alter3DView;
}

const char* StdCmdDrawStyleBase::className() const
{
    return "StdCmdDrawStyleBase";
}

bool StdCmdDrawStyleBase::isActive(void)
{
    return Gui::Application::Instance->activeDocument();
}

void StdCmdDrawStyleBase::activated(int iMsg)
{
    (void)iMsg;

    Gui::Document *doc = this->getActiveGuiDocument();
    if (!doc) return;
    auto activeView = doc->getActiveView();
    bool applyAll = !activeView || QApplication::queryKeyboardModifiers() == Qt::ControlModifier;

    doc->foreachView<View3DInventor>( [=](View3DInventor *view) {
        if(applyAll || view == activeView) {
            View3DInventorViewer *viewer = view->getViewer();
            if (!viewer)
                return;
            if (!Base::streq(sMenuText, "Shadow")) {
                viewer->setOverrideMode(sMenuText);
                return;
            }
            if (viewer->getOverrideMode() == "Shadow")
                viewer->toggleShadowLightManip();
            else {
                if (!doc->getDocument()->getPropertyByName("Shadow_ShowGround")) {
                    // If it is the first time shadow is turned on, switch to isometric view
                    viewer->setCameraOrientation(
                            SbRotation(0.424708f, 0.17592f, 0.339851f, 0.820473f));
                }
                viewer->setOverrideMode("Shadow");
            }
        }
    });
}

//===========================================================================
// StdCmdDrawStyle
//===========================================================================

class StdCmdDrawStyle : public GroupCommand
{
public:
    StdCmdDrawStyle();
    virtual const char* className() const {return "StdCmdDrawStyle";}
    bool isActive(void);
    void updateIcon(const MDIView *);
    virtual Action * createAction(void) {
        Action * action = GroupCommand::createAction();
        action->setCheckable(false);
        return action;
    }
};

StdCmdDrawStyle::StdCmdDrawStyle()
  : GroupCommand("Std_DrawStyle")
{
    sGroup        = QT_TR_NOOP("Standard-View");
    sMenuText     = QT_TR_NOOP("Draw style");
    sToolTipText  = QT_TR_NOOP("Change the draw style of the objects");
    sStatusTip    = QT_TR_NOOP("Change the draw style of the objects");
    sWhatsThis    = "Std_DrawStyle";
    eType         = 0;
    bCanLog       = false;

#define DRAW_STYLE_CMD(_name, _title, _doc, _shortcut) \
    addCommand(new StdCmdDrawStyleBase(\
        "Std_DrawStyle" #_name, _title, _doc, _shortcut, "DrawStyle" #_name));

    DRAW_STYLE_CMD(AsIs,"As Is",
            "Draw style, normal display mode", "V,1")
    DRAW_STYLE_CMD(Points, "Points",
            "Draw style, show points only", "V,2")
    DRAW_STYLE_CMD(WireFrame, "Wireframe",
            "Draw style, show wire frame only", "V,3")
    DRAW_STYLE_CMD(HiddenLine, "Hidden Line",
            "Draw style, show hidden line by display object as transparent", "V,4")
    DRAW_STYLE_CMD(NoShading, "No Shading",
            "Draw style, shading forced off", "V,5")
    DRAW_STYLE_CMD(Shaded, "Shaded",
            "Draw style, shading force on", "V,6")
    DRAW_STYLE_CMD(FlatLines, "Flat Lines",
            "Draw style, show both wire frame and face with shading", "V,7")
    DRAW_STYLE_CMD(Tessellation, "Tessellation",
            "Draw style, show tessellation wire frame", "V,8")
    DRAW_STYLE_CMD(Shadow, "Shadow",
            "Draw style, drop shadows for the scene.\n"
            "Click this button while in shadow mode to toggle light manipulator", "V,9");

    this->getGuiApplication()->signalActivateView.connect(boost::bind(&StdCmdDrawStyle::updateIcon, this, bp::_1));
    this->getGuiApplication()->signalViewModeChanged.connect(
        [this](const MDIView *view) {
            if (view == Application::Instance->activeView())
                updateIcon(view);
        });
}

void StdCmdDrawStyle::updateIcon(const MDIView *view)
{
    if (!_pcAction)
        return;
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

    int index = 0;
    if (mode == "Point") 
        index = 1;
    else if (mode == "Wireframe")
        index = 2;
    else if (mode == "Hidden Line")
        index = 3;
    else if (mode == "No shading")
        index = 4;
    else if (mode == "Shaded")
        index = 5;
    else if (mode == "Flat Lines")
        index = 6;
    else if (mode == "Tessellation")
        index = 7;
    else if (mode == "Shadow")
        index = 8;
    _pcAction->setProperty("defaultAction", QVariant(index));
    setup(_pcAction);
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
    Selection().setVisible(SelectionSingleton::VisToggle);
}

bool StdCmdToggleVisibility::isActive(void)
{
    return (Gui::Selection().size() != 0);
}

//===========================================================================
// Std_ToggleGroupVisibility
//===========================================================================
DEF_STD_CMD_A(StdCmdToggleGroupVisibility)

StdCmdToggleGroupVisibility::StdCmdToggleGroupVisibility()
  : Command("Std_ToggleGroupVisibility")
{
    sGroup        = QT_TR_NOOP("Standard-View");
    sMenuText     = QT_TR_NOOP("Toggle group visibility");
    sToolTipText  = QT_TR_NOOP("Toggles visibility of a group and all its nested children");
    sStatusTip    = sToolTipText;
    sWhatsThis    = "Std_ToggleGroupVisibility";
    eType         = Alter3DView;
}


void StdCmdToggleGroupVisibility::activated(int iMsg)
{
    Q_UNUSED(iMsg); 

    auto sels = Gui::Selection().getSelectionT(0,0);
    std::set<App::DocumentObject*> groups;
    for(auto &sel : sels) {
        auto sobj = sel.getSubObject();
        if(!sobj)
            continue;
        if(App::GeoFeatureGroupExtension::isNonGeoGroup(sobj))
            groups.insert(sobj);
    }

    for(auto it=sels.begin();it!=sels.end();) {
        auto &sel = *it;
        auto sobj = sel.getSubObject();
        if(!sobj || groups.count(App::GroupExtension::getGroupOfObject(sobj)))
            it = sels.erase(it);
        else
            ++it;
    }

    if(sels.empty())
        return;

    App::GroupExtension::ToggleNestedVisibility guard;
    Gui::Selection().setVisible(SelectionSingleton::VisToggle,sels);
}

bool StdCmdToggleGroupVisibility::isActive(void)
{
    return (Gui::Selection().size() != 0);
}

//===========================================================================
// Std_ToggleVisibility
//===========================================================================
DEF_STD_CMD_A(StdCmdToggleShowOnTop)

StdCmdToggleShowOnTop::StdCmdToggleShowOnTop()
  : Command("Std_ToggleShowOnTop")
{
    sGroup        = QT_TR_NOOP("Standard-View");
    sMenuText     = QT_TR_NOOP("Toggle show on top");
    sToolTipText  = QT_TR_NOOP("Toggles whether to show the object on top");
    sStatusTip    = sToolTipText;
    sWhatsThis    = "Std_ToggleShowOnTop";
    sAccel        = "Ctrl+Shift+Space";
    eType         = Alter3DView;
}

void StdCmdToggleShowOnTop::activated(int iMsg)
{
    Q_UNUSED(iMsg); 

    auto gdoc = Application::Instance->activeDocument();
    if(!gdoc)
        return;
    auto view = Base::freecad_dynamic_cast<View3DInventor>(gdoc->getActiveView());
    if(!view)
        return;
    auto viewer = view->getViewer();

    std::set<App::SubObjectT> objs;
    for(auto sel : Selection().getSelectionT(gdoc->getDocument()->getName(),0)) {
        sel.setSubName(sel.getSubNameNoElement().c_str());
        if(!objs.insert(sel).second)
            continue;
        bool selected = viewer->isInGroupOnTop(sel.getObjectName().c_str(),sel.getSubName().c_str());
        viewer->checkGroupOnTop(SelectionChanges(selected?SelectionChanges::RmvSelection:SelectionChanges::AddSelection,
                    sel.getDocumentName().c_str(), sel.getObjectName().c_str(), sel.getSubName().c_str()),true);
    }
    if(objs.empty())
        viewer->clearGroupOnTop(true);
}

bool StdCmdToggleShowOnTop::isActive(void)
{
    auto gdoc = Application::Instance->activeDocument();
    return gdoc && Base::freecad_dynamic_cast<View3DInventor>(gdoc->getActiveView());
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
            if (pr && pr->isDerivedFrom(ViewProviderDocumentObject::getClassTypeId())){
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
    Selection().setVisible(SelectionSingleton::VisShow);
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
    Selection().setVisible(SelectionSingleton::VisHide);
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
#if 0
    static QPointer<QDialog> dlg = 0;
    if (!dlg)
        dlg = new Gui::Dialog::DlgDisplayPropertiesImp(true, getMainWindow());
    dlg->setModal(false);
    dlg->setAttribute(Qt::WA_DeleteOnClose);
    dlg->show();
#else
    Gui::Control().showDialog(new Gui::Dialog::TaskDisplayProperties());
#endif
}

bool StdCmdSetAppearance::isActive(void)
{
#if 0
    return Gui::Selection().size() != 0;
#else
    return (Gui::Control().activeDialog() == nullptr) &&
           (Gui::Selection().size() != 0);
#endif
}

//===========================================================================
// Std_ViewHome
//===========================================================================
DEF_3DV_CMD(StdCmdViewHome)

StdCmdViewHome::StdCmdViewHome()
  : Command("Std_ViewHome")
{
    sGroup        = QT_TR_NOOP("Standard-View");
    sMenuText     = QT_TR_NOOP("Home");
    sToolTipText  = QT_TR_NOOP("Set to default home view");
    sWhatsThis    = "Std_ViewHome";
    sStatusTip    = QT_TR_NOOP("Set to default home view");
    //sPixmap       = "view-home";
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
    bCanLog       = false;
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
    bCanLog       = false;
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
    bCanLog       = false;
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
        fd.setWindowTitle(QObject::tr("Save picture"));
        fd.setNameFilters(filter);
        if (!selFilter.isEmpty())
            fd.selectNameFilter(selFilter);

        // create the image options widget
        DlgSettingsImageImp* opt = new DlgSettingsImageImp(&fd);
        SbVec2s sz = vp.getWindowSize();
        opt->setImageSize((int)sz[0], (int)sz[1]);
        opt->setBackgroundType(backtype);
        opt->setMethod(method);

        fd.setOptionsWidget(FileOptionsDialog::ExtensionRight, opt);
        fd.setOption(QFileDialog::DontConfirmOverwrite, false);
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

            method = opt->method();
            methodGrp->SetASCII("SavePicture", method.constData());

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
                // Replace newline escape sequence through '\\n' string to build one big string,
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

                    QFontMetrics fm(font);
                    int n = QtTools::horizontalAdvance(fm, name);
                    int h = pixmap.height();

                    painter.setFont(font);
                    painter.drawText(8+appicon.width(), h-24, name);

                    font.setPointSize(12);
                    int u = QtTools::horizontalAdvance(fm, url);
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
        sAccel        = "A,C";
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
    const SoEvent* ev = cb->getEvent();
    cb->setHandled();
    Gui::View3DInventorViewer* view  = reinterpret_cast<Gui::View3DInventorViewer*>(cb->getUserData());

    bool unselect = false;
    bool backFaceCull = true;
    bool singleSelect = true;

    if(ev) {
        if(ev->isOfType(SoKeyboardEvent::getClassTypeId())) {
            if(static_cast<const SoKeyboardEvent*>(ev)->getKey() == SoKeyboardEvent::ESCAPE) {
                view->stopSelection();
                view->removeEventCallback(SoEvent::getClassTypeId(), selectionCallback, ud);
                view->setEditing(false);
                view->setSelectionEnabled(true);
            }
            return;
        } else if (!ev->isOfType(SoMouseButtonEvent::getClassTypeId()))
            return;

        if(ev->wasShiftDown())  {
            unselect = true;
            singleSelect = false;
        } else if(ev->wasCtrlDown())
            singleSelect = false;
        if(ev->wasAltDown())
            backFaceCull = false;
    }

    bool selectElement = ud?true:false;

    bool center = false;
    auto points = view->getGLPolygon();
    bool doSelect = true;
    if (points.size() <= 1) {
        doSelect = false;
        singleSelect = true;
    } else if (points.size() == 2) {
        center = true;
        if (points[0] == points[1]) {
            doSelect = false;
            singleSelect = true;
        }
        // when selecting from right to left then select by intersection
        // otherwise if the center is inside the rectangle
        else if (points[0][0] > points[1][0])
            center = false;
    } else {
        double sum = 0;
        size_t i=0;
        for(size_t c=points.size()-1; i<c; ++i)
            sum += ((double)points[i+1][0] - points[i][0]) * ((double)points[i+1][1] + points[i][1]);
        sum += ((double)points[0][0] - points[i][0]) * (points[0][1] + points[i][1]);
        // use polygon windings to choose intersection or center inclusion
        center = sum>0;
    }

    if (doSelect) {
        if (singleSelect) {
            App::Document *doc = App::GetApplication().getActiveDocument();
            if (doc)
                Gui::Selection().clearSelection(doc->getName());
        }

        bool currentSelection = (ViewParams::getShowSelectionOnTop() 
                                 && ViewParams::getSelectElementOnTop()
                                 && selectElement);

        auto picked = view->getPickedList(points, center, selectElement, backFaceCull,
                                        currentSelection, unselect, false);
        for (auto &objT : picked) {
            if (unselect)
                Selection().rmvSelection(objT);
            else
                Selection().addSelection(objT);
        }
    }

    if (singleSelect) {
        view->removeEventCallback(SoEvent::getClassTypeId(), selectionCallback, ud);
        view->setEditing(false);
        view->setSelectionEnabled(true);
    } else {
        Command *cmd = (Command*)ud;
        AbstractMouseSelection *sel;
        if (cmd && Base::streq(cmd->getName(), "Std_LassoElementSelection"))
            sel = view->startSelection(View3DInventorViewer::Lasso);
        else
            sel = view->startSelection(View3DInventorViewer::Rubberband);
        if (sel) sel->changeCursorOnKeyPress(2);
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
            viewer->setEditing(true);
            AbstractMouseSelection *sel = viewer->startSelection(View3DInventorViewer::Rubberband);
            if (sel) sel->changeCursorOnKeyPress(1);
            viewer->addEventCallback(SoEvent::getClassTypeId(), selectionCallback);
            viewer->setSelectionEnabled(false);
        }
    }
}

//===========================================================================
// Std_BoxElementSelection
//===========================================================================
DEF_3DV_CMD(StdBoxElementSelection)

StdBoxElementSelection::StdBoxElementSelection()
  : Command("Std_BoxElementSelection")
{
    sGroup        = QT_TR_NOOP("Standard-View");
    sMenuText     = QT_TR_NOOP("Box element selection");
    sToolTipText  = QT_TR_NOOP("Box element selection");
    sWhatsThis    = "Std_BoxElementSelection";
    sStatusTip    = QT_TR_NOOP("Box element selection");
#if QT_VERSION >= 0x040200
    sPixmap       = "edit-element-select-box";
#endif
    sAccel        = "Ctrl+Shift+E";
    eType         = AlterSelection;
}

void StdBoxElementSelection::activated(int iMsg)
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
            viewer->setEditing(true);
            AbstractMouseSelection *sel = viewer->startSelection(View3DInventorViewer::Rubberband);
            if (sel) sel->changeCursorOnKeyPress(1);
            viewer->addEventCallback(SoEvent::getClassTypeId(), selectionCallback, this);
            viewer->setSelectionEnabled(false);
        }
    }
}

//===========================================================================
// Std_LassoElementSelection
//===========================================================================
DEF_3DV_CMD(StdLassoElementSelection)

StdLassoElementSelection::StdLassoElementSelection()
  : Command("Std_LassoElementSelection")
{
    sGroup        = QT_TR_NOOP("Standard-View");
    sMenuText     = QT_TR_NOOP("Lasso element selection");
    sToolTipText  = QT_TR_NOOP("Lasso element selection");
    sWhatsThis    = "Std_LassoElementSelection";
    sStatusTip    = QT_TR_NOOP("Lasso element selection");
#if QT_VERSION >= 0x040200
    sPixmap       = "edit-element-select-lasso";
#endif
    sAccel        = "Shift+S";
    eType         = AlterSelection;
}

void StdLassoElementSelection::activated(int iMsg)
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
            viewer->setEditing(true);
            AbstractMouseSelection *sel = viewer->startSelection(View3DInventorViewer::Lasso);
            if (sel) sel->changeCursorOnKeyPress(1);
            viewer->addEventCallback(SoEvent::getClassTypeId(), selectionCallback, this);
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
    sGroup        = QT_TR_NOOP("TreeView");
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
    sGroup        = QT_TR_NOOP("View");
    sMenuText     = QT_TR_NOOP("Collapse selected item");
    sToolTipText  = QT_TR_NOOP("Collapse currently selected tree items");
    sWhatsThis    = "Std_TreeCollapse";
    sStatusTip    = QT_TR_NOOP("Collapse currently selected tree items");
    eType         = Alter3DView;
}

void StdCmdTreeCollapse::activated(int iMsg)
{
    Q_UNUSED(iMsg); 
    TreeWidget::expandSelectedItems(TreeItemMode::CollapseItem);
}

//===========================================================================
// Std_TreeExpand
//===========================================================================

DEF_STD_CMD(StdCmdTreeExpand)

StdCmdTreeExpand::StdCmdTreeExpand()
  : Command("Std_TreeExpand")
{
    sGroup        = QT_TR_NOOP("View");
    sMenuText     = QT_TR_NOOP("Expand selected item");
    sToolTipText  = QT_TR_NOOP("Expand currently selected tree items");
    sWhatsThis    = "Std_TreeExpand";
    sStatusTip    = QT_TR_NOOP("Expand currently selected tree items");
    eType         = Alter3DView;
}

void StdCmdTreeExpand::activated(int iMsg)
{
    Q_UNUSED(iMsg); 
    TreeWidget::expandSelectedItems(TreeItemMode::ExpandItem);
}

//===========================================================================
// Std_TreeSelectAllInstance
//===========================================================================

DEF_STD_CMD_A(StdCmdTreeSelectAllInstances)

StdCmdTreeSelectAllInstances::StdCmdTreeSelectAllInstances()
  : Command("Std_TreeSelectAllInstances")
{
    sGroup        = QT_TR_NOOP("View");
    sMenuText     = QT_TR_NOOP("Select all instances");
    sToolTipText  = QT_TR_NOOP("Select all instances of the current selected object");
    sWhatsThis    = "Std_TreeSelectAllInstances";
    sStatusTip    = QT_TR_NOOP("Select all instances of the current selected object");
    sPixmap       = "sel-instance";
    eType         = AlterSelection;
}

bool StdCmdTreeSelectAllInstances::isActive(void)
{
    const auto &sels = Selection().getSelectionEx("*",App::DocumentObject::getClassTypeId(),true,true);
    if(sels.empty())
        return false;
    auto obj = sels[0].getObject();
    if(!obj || !obj->getNameInDocument())
        return false;
    return dynamic_cast<ViewProviderDocumentObject*>(
            Application::Instance->getViewProvider(obj))!=0;
}

void StdCmdTreeSelectAllInstances::activated(int iMsg)
{
    Q_UNUSED(iMsg); 
    const auto &sels = Selection().getSelectionEx("*",App::DocumentObject::getClassTypeId(),true,true);
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
    TreeWidget::selectAllInstances(*vpd);
    Selection().selStackPush();
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
    sAccel        = "T, I";
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
// Std_SelUp
//===========================================================================

DEF_STD_CMD_AC(StdCmdSelUp)

StdCmdSelUp::StdCmdSelUp()
  :Command("Std_SelUp")
{
  sGroup        = QT_TR_NOOP("View");
  sMenuText     = QT_TR_NOOP("&Up hierarchy");
  sToolTipText  = QT_TR_NOOP("Go up object hierarchy of the current selection");
  sWhatsThis    = "Std_SelUp";
  sStatusTip    = sToolTipText;
  sPixmap       = "sel-up";
  sAccel        = "U, U";
  eType         = NoTransaction | AlterSelection | NoHistory;
}

bool StdCmdSelUp::isActive(void)
{
    return App::GetApplication().getActiveDocument();
}

void StdCmdSelUp::activated(int iMsg)
{
    Q_UNUSED(iMsg); 
    if (_pcAction)
        static_cast<SelUpAction*>(_pcAction)->popup(QCursor::pos());
}

Action * StdCmdSelUp::createAction(void)
{
    Action *pcAction;
    pcAction = new SelUpAction(this, getMainWindow());
    pcAction->setIcon(BitmapFactory().iconFromTheme(sPixmap));
    pcAction->setShortcut(QString::fromLatin1(sAccel));
    applyCommandData(this->className(), pcAction);
    return pcAction;
}

//===========================================================================
// Std_SelBack
//===========================================================================

DEF_STD_CMD_A(StdCmdSelBack)

StdCmdSelBack::StdCmdSelBack()
  :Command("Std_SelBack")
{
  sGroup        = QT_TR_NOOP("View");
  sMenuText     = QT_TR_NOOP("&Back");
  sToolTipText  = QT_TR_NOOP("Go back to previous selection");
  sWhatsThis    = "Std_SelBack";
  sStatusTip    = QT_TR_NOOP("Go back to previous selection");
  sPixmap       = "sel-back";
  sAccel        = "S, B";
  eType         = AlterSelection;
}

void StdCmdSelBack::activated(int iMsg)
{
    Q_UNUSED(iMsg); 
    Selection().selStackGoBack();
    TreeWidget::scrollItemToTop();
}

bool StdCmdSelBack::isActive(void)
{
    return Selection().selStackBackSize()>0;
}

//===========================================================================
// Std_SelForward
//===========================================================================

DEF_STD_CMD_A(StdCmdSelForward)

StdCmdSelForward::StdCmdSelForward()
  :Command("Std_SelForward")
{
  sGroup        = QT_TR_NOOP("View");
  sMenuText     = QT_TR_NOOP("&Forward");
  sToolTipText  = QT_TR_NOOP("Repeat the backed selection");
  sWhatsThis    = "Std_SelForward";
  sStatusTip    = QT_TR_NOOP("Repeat the backed selection");
  sPixmap       = "sel-forward";
  sAccel        = "S, F";
  eType         = AlterSelection;
}

void StdCmdSelForward::activated(int iMsg)
{
    Q_UNUSED(iMsg); 
    Selection().selStackGoForward();
    TreeWidget::scrollItemToTop();
}

bool StdCmdSelForward::isActive(void)
{
  return !!Selection().selStackForwardSize();
}

//===========================================================================
// Std_SelGeometry
//===========================================================================

DEF_STD_CMD_A(StdCmdPickGeometry)

StdCmdPickGeometry::StdCmdPickGeometry()
  :Command("Std_PickGeometry")
{
  sGroup        = QT_TR_NOOP("View");
  sMenuText     = QT_TR_NOOP("Pick geometry");
  sToolTipText  = QT_TR_NOOP("Pick hidden geometires under the mouse cursor in 3D view.\n"
                             "This command is supposed to be actived by keyboard shortcut.");
  sWhatsThis    = "Std_PickGeometry";
  sStatusTip    = sToolTipText;
  // sPixmap       = "sel-geo";
  sAccel        = "G, G";
  eType         = NoTransaction | AlterSelection | NoDefaultAction | NoHistory;
}

void StdCmdPickGeometry::activated(int iMsg)
{
    Q_UNUSED(iMsg); 

    QPoint pos = QCursor::pos();
    QWidget *widget = qApp->widgetAt(pos);
    if (widget)
        widget = widget->parentWidget();
    auto viewer = qobject_cast<View3DInventorViewer*>(widget);
    if (!viewer)
        return;

    auto sels = viewer->getPickedList(false);
    if (sels.empty())
        return;

    SelectionMenu menu;
    menu.doPick(sels);
}

bool StdCmdPickGeometry::isActive(void)
{
    auto view = Application::Instance->activeView();
    return view && view->isDerivedFrom(View3DInventor::getClassTypeId());
}

//=======================================================================
// Std_TreeSingleDocument
//===========================================================================
#define TREEVIEW_DOC_CMD_DEF(_name,_v) \
DEF_STD_CMD_AC(StdTree##_name) \
void StdTree##_name::activated(int){ \
    TreeParams::Instance()->setDocumentMode(_v);\
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
    bool checked = TreeParams::Instance()->DocumentMode()==_v;\
    if(_pcAction && _pcAction->isChecked()!=checked)\
        _pcAction->setChecked(checked,true);\
    return true;\
}
        
TREEVIEW_DOC_CMD_DEF(SingleDocument,0)

StdTreeSingleDocument::StdTreeSingleDocument()
  : Command("Std_TreeSingleDocument")
{
    sGroup       = QT_TR_NOOP("TreeView");
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
    sGroup       = QT_TR_NOOP("TreeView");
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
    sGroup       = QT_TR_NOOP("TreeView");
    sMenuText    = QT_TR_NOOP("Collapse/Expand");
    sToolTipText = QT_TR_NOOP("Expand active document and collapse all others");
    sWhatsThis   = "Std_TreeCollapseDocument";
    sStatusTip   = QT_TR_NOOP("Expand active document and collapse all others");
    sPixmap      = "tree-doc-collapse";
    eType        = 0;
}

//===========================================================================
// StdCmdCheckableOption
//===========================================================================

class StdCmdCheckableOption : public Gui::Command
{
public:
    StdCmdCheckableOption(const char *name)
        :Command(name)
    {}

protected: 
    virtual void activated(int iMsg) {
        auto checked = !!iMsg;
        setOption(checked);
        if(_pcAction) _pcAction->setChecked(checked,true);
    }

    virtual bool isActive(void) {
        bool checked = getOption();
        if(_pcAction && _pcAction->isChecked()!=checked)
            _pcAction->setChecked(checked,true);
        return true;
    }

    virtual Gui::Action * createAction(void) {
        Action *pcAction = Command::createAction();
        pcAction->setCheckable(true);
        pcAction->setIcon(QIcon());
        _pcAction = pcAction;
        isActive();
        return pcAction;
    }

    virtual bool getOption() const = 0;
    virtual void setOption(bool checked) = 0;
};

#define TREEVIEW_CMD_DEF(_name) \
class StdTree##_name : public StdCmdCheckableOption \
{\
public:\
    StdTree##_name();\
    virtual const char* className() const\
    { return "StdTree" #_name; }\
protected: \
    virtual void setOption(bool checked) {\
        TreeParams::Instance()->set##_name(checked);\
    }\
    virtual bool getOption(void) const {\
        return TreeParams::Instance()->_name();\
    }\
};\
StdTree##_name::StdTree##_name():StdCmdCheckableOption("Std_Tree" #_name)

//===========================================================================
// Std_TreeSyncView
//===========================================================================

TREEVIEW_CMD_DEF(SyncView)
{
    sGroup       = QT_TR_NOOP("TreeView");
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
{
    sGroup       = QT_TR_NOOP("TreeView");
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
{
    sGroup       = QT_TR_NOOP("TreeView");
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
{
    sGroup       = QT_TR_NOOP("TreeView");
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
{
    sGroup       = QT_TR_NOOP("TreeView");
    sMenuText    = QT_TR_NOOP("Record selection");
    sToolTipText = QT_TR_NOOP("Record selection in tree view in order to go back/forward using navigation button");
    sStatusTip   = sToolTipText;
    sWhatsThis   = "Std_TreeRecordSelection";
    sPixmap      = "tree-rec-sel";
    sAccel       = "T,5";
    eType        = 0;
}

//===========================================================================
// Std_TreeResizableColumn
//===========================================================================
TREEVIEW_CMD_DEF(ResizableColumn)
{
    sGroup       = QT_TR_NOOP("TreeView");
    sMenuText    = QT_TR_NOOP("Resizable column");
    sToolTipText = QT_TR_NOOP("Make treeview column resizable");
    sStatusTip   = sToolTipText;
    sWhatsThis   = "Std_TreeResizableColumn";
    eType        = NoDefaultAction;
}

//===========================================================================
// Std_TreeDrag
//===========================================================================
DEF_STD_CMD(StdTreeDrag)

StdTreeDrag::StdTreeDrag()
  : Command("Std_TreeDrag")
{
    sGroup       = QT_TR_NOOP("TreeView");
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
        for(auto tree : getMainWindow()->findChildren<TreeWidget*>()) {
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
        sGroup        = QT_TR_NOOP("View");
        sMenuText     = QT_TR_NOOP("TreeView actions");
        sToolTipText  = QT_TR_NOOP("TreeView behavior options and actions");
        sWhatsThis    = "Std_TreeViewActions";
        sStatusTip    = QT_TR_NOOP("TreeView behavior options and actions");
        eType         = 0;
        bCanLog       = false;

        addCommand(new StdTreeSyncView());
        addCommand(new StdTreeSyncSelection());
        addCommand(new StdTreeSyncPlacement());
        addCommand(Application::Instance->commandManager().getCommandByName("Std_TreePreSelection"));
        addCommand(new StdTreeRecordSelection());

        addCommand();

        addCommand(new StdTreeResizableColumn());

        addCommand();

        addCommand(new StdTreeSingleDocument());
        addCommand(new StdTreeMultiDocument());
        addCommand(new StdTreeCollapseDocument());

        addCommand();

        addCommand(new StdTreeDrag(),cmds.size());
        addCommand(new StdTreeSelection(),cmds.size());
    };
    virtual const char* className() const {return "StdCmdTreeViewActions";}
};


#define VIEW_CMD_DEF(_name,_option) \
class StdCmd##_name : public StdCmdCheckableOption \
{\
public:\
    StdCmd##_name();\
    virtual const char* className() const\
    { return "StdCmd" #_name; }\
protected: \
    virtual void setOption(bool checked) {\
        ViewParams::instance()->set##_option(checked);\
    }\
    virtual bool getOption(void) const {\
        return ViewParams::instance()->get##_option();\
    }\
};\
StdCmd##_name::StdCmd##_name():StdCmdCheckableOption("Std_" #_name)

//======================================================================
// Std_SelBoundingBox
//======================================================================
VIEW_CMD_DEF(SelBoundingBox, ShowSelectionBoundingBox)
{
  sGroup        = QT_TR_NOOP("View");
  sMenuText     = QT_TR_NOOP("&Bounding box");
  sToolTipText  = ViewParams::docShowSelectionBoundingBox();
  sWhatsThis    = "Std_SelBoundingBox";
  sStatusTip    = sToolTipText;
  sPixmap       = "sel-bbox";
  eType         = Alter3DView;
}

//======================================================================
// Std_TightBoundingBox
//======================================================================
VIEW_CMD_DEF(TightBoundingBox, UseTightBoundingBox)
{
  sGroup        = QT_TR_NOOP("View");
  sMenuText     = QT_TR_NOOP("Tighten bounding box");
  sToolTipText  = ViewParams::docUseTightBoundingBox();
  sWhatsThis    = "Std_TightBoundingBox";
  sStatusTip    = sToolTipText;
  eType         = NoDefaultAction;
}

//======================================================================
// Std_ProjectBoundingBox
//======================================================================
VIEW_CMD_DEF(ProjectBoundingBox, RenderProjectedBBox)
{
  sGroup        = QT_TR_NOOP("View");
  sMenuText     = QT_TR_NOOP("Project bounding box");
  sToolTipText  = ViewParams::docRenderProjectedBBox();
  sWhatsThis    = "Std_ProjectBoundingBox";
  sStatusTip    = sToolTipText;
  eType         = NoDefaultAction;
}

//======================================================================
// Std_SelectionFaceWire
//======================================================================
VIEW_CMD_DEF(SelectionFaceWire, SelectionFaceWire)
{
  sGroup        = QT_TR_NOOP("View");
  sMenuText     = QT_TR_NOOP("Show selected face wires");
  sToolTipText  = ViewParams::docSelectionFaceWire();
  sWhatsThis    = "Std_SelectionFaceWire";
  sStatusTip    = sToolTipText;
  eType         = NoDefaultAction;
}

//======================================================================
// Std_SelOnTop
//======================================================================
VIEW_CMD_DEF(SelOnTop, ShowSelectionOnTop)
{
  sGroup        = QT_TR_NOOP("View");
  sMenuText     = QT_TR_NOOP("&Selection on top");
  sToolTipText  = ViewParams::docShowSelectionOnTop();
  sWhatsThis    = "Std_SelOnTop";
  sStatusTip    = sToolTipText;
  sPixmap       = "sel-on-top";
  sAccel        = "V, T";
  eType         = Alter3DView;
}

//======================================================================
// Std_SelHierarchyAscend
//======================================================================
VIEW_CMD_DEF(SelHierarchyAscend, HierarchyAscend)
{
  sGroup        = QT_TR_NOOP("View");
  sMenuText     = QT_TR_NOOP("Hierarchy selection");
  sToolTipText  = ViewParams::docHierarchyAscend();
  sWhatsThis    = "Std_SelHierarhcyAscend";
  sStatusTip    = sToolTipText;
  eType         = NoDefaultAction;
}

//======================================================================
// Std_PartialHighlightOnFullSelect
//======================================================================
VIEW_CMD_DEF(PartialHighlightOnFullSelect, PartialHighlightOnFullSelect)
{
  sGroup        = QT_TR_NOOP("View");
  sMenuText     = QT_TR_NOOP("&Partial highlight");
  sToolTipText  = ViewParams::docPartialHighlightOnFullSelect();
  sWhatsThis    = "Std_PartialHighlightOnFullSelect";
  sStatusTip    = sToolTipText;
  eType         = NoDefaultAction;
}

//======================================================================
// Std_EditingAutoTransparent
//======================================================================
VIEW_CMD_DEF(EditingAutoTransparent, EditingAutoTransparent)
{
  sGroup        = QT_TR_NOOP("View");
  sMenuText     = QT_TR_NOOP("Auto transparent on edit");
  sToolTipText  = ViewParams::docEditingAutoTransparent();
  sWhatsThis    = "Std_EditingAutoTransparent";
  sStatusTip    = sToolTipText;
  eType         = NoDefaultAction;
}

//======================================================================
// Std_PreselEdgeOnly
//======================================================================
VIEW_CMD_DEF(PreselEdgeOnly, ShowHighlightEdgeOnly)
{
  sGroup        = QT_TR_NOOP("View");
  sMenuText     = QT_TR_NOOP("&Highlight edge only");
  sToolTipText  = ViewParams::docShowHighlightEdgeOnly();
  sWhatsThis    = "Std_PreselEdgeOnly";
  sStatusTip    = sToolTipText;
  sPixmap       = "presel-edge-only";
  eType         = Alter3DView;
}

//======================================================================
// Std_MapChildrenPlacement
//======================================================================
VIEW_CMD_DEF(MapChildrenPlacement, MapChildrenPlacement)
{
  sGroup        = QT_TR_NOOP("View");
  sMenuText     = QT_TR_NOOP("Map children (Experimental!)");
  sToolTipText  = ViewParams::docMapChildrenPlacement();
  sWhatsThis    = "Std_MapChildrenPlacement";
  sStatusTip    = sToolTipText;
  eType         = NoDefaultAction;
}


//////////////////////////////////////////////////////////

class StdCmdSelOptions : public GroupCommand
{
public:
    StdCmdSelOptions()
        :GroupCommand("Std_SelOptions")
    {
        sGroup        = QT_TR_NOOP("View");
        sMenuText     = QT_TR_NOOP("Selection options");
        sToolTipText  = QT_TR_NOOP("Selection behavior options");
        sWhatsThis    = "Std_SelOptions";
        sStatusTip    = sToolTipText;
        eType         = 0;
        bCanLog       = false;

        addCommand(new StdCmdSelBoundingBox());
        addCommand(new StdCmdTightBoundingBox());
        addCommand(new StdCmdProjectBoundingBox());
        addCommand();
        addCommand(new StdCmdSelOnTop());
        addCommand(new StdCmdPartialHighlightOnFullSelect());
        addCommand(new StdCmdEditingAutoTransparent());
        addCommand(new StdCmdSelectionFaceWire());
        addCommand(new StdCmdPreselEdgeOnly());
        addCommand(Application::Instance->commandManager().getCommandByName("Std_TreePreSelection"));
        addCommand(new StdCmdSelHierarchyAscend());
        addCommand();
        addCommand(new StdCmdMapChildrenPlacement());
    };
    virtual const char* className() const {return "StdCmdSelOptions";}
};

#ifdef FC_HAS_DOCK_OVERLAY

//===========================================================================
// Std_DockOverlayAll
//===========================================================================

DEF_STD_CMD(StdCmdDockOverlayAll)

StdCmdDockOverlayAll::StdCmdDockOverlayAll()
  :Command("Std_DockOverlayAll")
{
  sGroup        = QT_TR_NOOP("View");
  sMenuText     = QT_TR_NOOP("Toggle overlay for all");
  sToolTipText  = QT_TR_NOOP("Toggle overlay mode for all docked windows");
  sWhatsThis    = "Std_DockOverlayAll";
  sStatusTip    = sToolTipText;
  sAccel        = "F4";
  eType         = 0;
}

void StdCmdDockOverlayAll::activated(int iMsg)
{
    Q_UNUSED(iMsg); 
    OverlayManager::instance()->setOverlayMode(OverlayManager::ToggleAll);
}

//===========================================================================
// Std_DockOverlayTransparentAll
//===========================================================================

DEF_STD_CMD(StdCmdDockOverlayTransparentAll)

StdCmdDockOverlayTransparentAll::StdCmdDockOverlayTransparentAll()
  :Command("Std_DockOverlayTransparentAll")
{
  sGroup        = QT_TR_NOOP("View");
  sMenuText     = QT_TR_NOOP("Toggle transparent for all");
  sToolTipText  = QT_TR_NOOP("Toggle transparent for all overlay docked window.\n"
                             "This makes the docked widget stay transparent at al times.");
  sWhatsThis    = "Std_DockOverlayTransparentAll";
  sStatusTip    = sToolTipText;
  sAccel        = "SHIFT+F4";
  eType         = 0;
}

void StdCmdDockOverlayTransparentAll::activated(int iMsg)
{
    Q_UNUSED(iMsg); 
    OverlayManager::instance()->setOverlayMode(OverlayManager::ToggleTransparentAll);
}

//===========================================================================
// Std_DockOverlayToggle
//===========================================================================

DEF_STD_CMD(StdCmdDockOverlayToggle)

StdCmdDockOverlayToggle::StdCmdDockOverlayToggle()
  :Command("Std_DockOverlayToggle")
{
  sGroup        = QT_TR_NOOP("View");
  sMenuText     = QT_TR_NOOP("Toggle overlay");
  sToolTipText  = QT_TR_NOOP("Toggle overlay mode of the docked window under cursor");
  sWhatsThis    = "Std_DockOverlayToggle";
  sStatusTip    = sToolTipText;
  sAccel        = "F3";
  eType         = 0;
}

void StdCmdDockOverlayToggle::activated(int iMsg)
{
    Q_UNUSED(iMsg); 
    OverlayManager::instance()->setOverlayMode(OverlayManager::ToggleActive);
}

//===========================================================================
// Std_DockOverlayToggleTransparent
//===========================================================================

DEF_STD_CMD(StdCmdDockOverlayToggleTransparent)

StdCmdDockOverlayToggleTransparent::StdCmdDockOverlayToggleTransparent()
  :Command("Std_DockOverlayToggleTransparent")
{
    sGroup        = QT_TR_NOOP("Standard-View");
    sMenuText     = QT_TR_NOOP("Toggle transparent");
    sToolTipText  = QT_TR_NOOP("Toggle transparent mode for the docked widget under cursor.\n"
                               "This makes the docked widget stay transparent at al times.");
    sWhatsThis    = "Std_DockOverlayToggleTransparent";
    sStatusTip    = sToolTipText;
    sAccel        = "SHIFT+F3";
    eType         = 0;
}

void StdCmdDockOverlayToggleTransparent::activated(int iMsg)
{
    Q_UNUSED(iMsg); 
    OverlayManager::instance()->setOverlayMode(OverlayManager::ToggleTransparent);
}

//===========================================================================
// Std_DockOverlayIncrease
//===========================================================================

DEF_STD_CMD(StdCmdDockOverlayIncrease)

StdCmdDockOverlayIncrease::StdCmdDockOverlayIncrease()
  :Command("Std_DockOverlayIncrease")
{
  sGroup        = QT_TR_NOOP("View");
  sMenuText     = QT_TR_NOOP("Increase overlay size");
  sToolTipText  = QT_TR_NOOP("Increase the overlayed widget size under cursor");
  sWhatsThis    = "Std_DockOverlayIncrease";
  sStatusTip    = sToolTipText;
  sAccel        = "ALT+F3";
  eType         = 0;
}

void StdCmdDockOverlayIncrease::activated(int iMsg)
{
    Q_UNUSED(iMsg); 
    OverlayManager::instance()->changeOverlaySize(5);
}

//===========================================================================
// Std_DockOverlayDecrease
//===========================================================================

DEF_STD_CMD(StdCmdDockOverlayDecrease)

StdCmdDockOverlayDecrease::StdCmdDockOverlayDecrease()
  :Command("Std_DockOverlayDecrease")
{
  sGroup        = QT_TR_NOOP("View");
  sMenuText     = QT_TR_NOOP("Decrease overlay size");
  sToolTipText  = QT_TR_NOOP("Decrease the overlayed widget size under cursor");
  sWhatsThis    = "Std_DockOverlayDecrease";
  sStatusTip    = sToolTipText;
  sAccel        = "ALT+F2";
  eType         = 0;
}

void StdCmdDockOverlayDecrease::activated(int iMsg)
{
    Q_UNUSED(iMsg); 
    OverlayManager::instance()->changeOverlaySize(-5);
}

//===========================================================================
// Std_DockOverlayAutoView
//===========================================================================

VIEW_CMD_DEF(DockOverlayAutoView, DockOverlayAutoView)
{
    sGroup        = QT_TR_NOOP("Standard-View");
    sMenuText     = QT_TR_NOOP("Auto hide non 3D view");
    sToolTipText  = QT_TR_NOOP("Activate auto hide for non 3D view");
    sWhatsThis    = "Std_DockOverlayAutoView";
    sStatusTip    = sToolTipText;
    eType         = 0;
}

//===========================================================================
// Std_DockOverlayExtraState
//===========================================================================

VIEW_CMD_DEF(DockOverlayExtraState, DockOverlayExtraState)
{
    sGroup        = QT_TR_NOOP("Standard-View");
    sMenuText     = QT_TR_NOOP("More hiding in overlay");
    sToolTipText  = QT_TR_NOOP("Hide more widgets when in overlay mode");
    sWhatsThis    = "Std_DockOverlayExtraState";
    sStatusTip    = sToolTipText;
    eType         = 0;
}

//===========================================================================
// Std_DockOverlayActivateOnHover
//===========================================================================

VIEW_CMD_DEF(DockOverlayActivateOnHover, DockOverlayActivateOnHover)
{
    sGroup        = QT_TR_NOOP("Standard-View");
    sMenuText     = QT_TR_NOOP("Activate on hover");
    sToolTipText  = QT_TR_NOOP("Activate auto hidden overlay on mouse hover.\n"
                               "If disabled, then activate on mouse click");
    sWhatsThis    = "Std_DockOverlayActivateOnHover";
    sStatusTip    = sToolTipText;
    eType         = 0;
}

//===========================================================================
// Std_DockOverlayAutoMouseThrough
//===========================================================================

VIEW_CMD_DEF(DockOverlayAutoMouseThrough, DockOverlayAutoMouseThrough)
{
    sGroup        = QT_TR_NOOP("Standard-View");
    sMenuText     = QT_TR_NOOP("Auto mouse event pass through");
    sToolTipText  = QT_TR_NOOP("Auto pass through mouse event on completely transparent background");
    sWhatsThis    = "Std_DockOverlayAutoMouseThrough";
    sStatusTip    = sToolTipText;
    eType         = 0;
}

//===========================================================================
// Std_DockOverlayCheckNaviCube
//===========================================================================

VIEW_CMD_DEF(DockOverlayCheckNaviCube, DockOverlayCheckNaviCube)
{
    sGroup        = QT_TR_NOOP("Standard-View");
    sMenuText     = QT_TR_NOOP("Make space for NaviCube");
    sToolTipText  = QT_TR_NOOP("Adjust overlay size to make space for Navigation Cube\n"
                               "Note that it only respects the cube position setting in\n"
                               "the preference dialog, and will not work for custom\n"
                               "position obtained through dragging the cube.");
    sWhatsThis    = "Std_DockOverlayCheckNaviCube";
    sStatusTip    = sToolTipText;
    eType         = 0;
}

//===========================================================================
// Std_DockOverlayMouseTransparent
//===========================================================================

DEF_STD_CMD_AC(StdCmdDockOverlayMouseTransparent)

StdCmdDockOverlayMouseTransparent::StdCmdDockOverlayMouseTransparent()
  :Command("Std_DockOverlayMouseTransparent")
{
  sGroup        = QT_TR_NOOP("View");
  sMenuText     = QT_TR_NOOP("Bypass mouse event in dock overlay");
  sToolTipText  = QT_TR_NOOP("Bypass all mouse event in dock overlay");
  sWhatsThis    = "Std_DockOverlayMouseTransparent";
  sStatusTip    = sToolTipText;
  sAccel        = "T, T";
  eType         = NoTransaction;
}

void StdCmdDockOverlayMouseTransparent::activated(int iMsg)
{
    auto checked = !!iMsg;
    OverlayManager::instance()->setMouseTransparent(checked);
    if(_pcAction)
        _pcAction->setChecked(checked,true);
}

Action * StdCmdDockOverlayMouseTransparent::createAction(void) {
    Action *pcAction = Command::createAction();
    pcAction->setCheckable(true);
    pcAction->setIcon(QIcon());
    _pcAction = pcAction;
    isActive();
    return pcAction;
}

bool StdCmdDockOverlayMouseTransparent::isActive() {
    bool checked = OverlayManager::instance()->isMouseTransparent();
    if(_pcAction && _pcAction->isChecked()!=checked)
        _pcAction->setChecked(checked,true);
    return true;
}

// ============================================================================

class StdCmdDockOverlay : public GroupCommand
{
public:
    StdCmdDockOverlay()
        :GroupCommand("Std_DockOverlay")
    {
        sGroup        = QT_TR_NOOP("View");
        sMenuText     = QT_TR_NOOP("Dock window overlay");
        sToolTipText  = QT_TR_NOOP("Setting docked window overlay mode");
        sWhatsThis    = "Std_DockOverlay";
        sStatusTip    = sToolTipText;
        eType         = 0;
        bCanLog       = false;

        addCommand(new StdCmdDockOverlayAll());
        addCommand(new StdCmdDockOverlayTransparentAll());
        addCommand();
        addCommand(new StdCmdDockOverlayToggle());
        addCommand(new StdCmdDockOverlayToggleTransparent());
        addCommand();
        addCommand(new StdCmdDockOverlayIncrease());
        addCommand(new StdCmdDockOverlayDecrease());
        addCommand();
        addCommand(new StdCmdDockOverlayAutoView());
        addCommand(new StdCmdDockOverlayExtraState());
        addCommand(new StdCmdDockOverlayActivateOnHover());
        addCommand(new StdCmdDockOverlayAutoMouseThrough());
        addCommand(new StdCmdDockOverlayCheckNaviCube());
        addCommand();
        addCommand(new StdCmdDockOverlayMouseTransparent());
    };
    virtual const char* className() const {return "StdCmdDockOverlay";}
};
#endif // FC_HAS_DOCK_OVERLAY

//===========================================================================
// Std_BindViewCamera
//===========================================================================

DEF_STD_CMD_AC(StdCmdBindViewCamera)

StdCmdBindViewCamera::StdCmdBindViewCamera()
  : Command("Std_BindViewCamera")
{
    sGroup        = QT_TR_NOOP("View");
    sMenuText     = QT_TR_NOOP("Bind view camera");
    sToolTipText  = QT_TR_NOOP("Bind the camera position of the active view to another view");
    sWhatsThis    = "Std_BindViewCamera";
    sStatusTip    = sToolTipText;
    eType         = 0;
}

void StdCmdBindViewCamera::activated(int iMsg)
{
    // Handled by the related QAction objects
    Q_UNUSED(iMsg); 
}

bool StdCmdBindViewCamera::isActive(void)
{
    if(Base::freecad_dynamic_cast<View3DInventor>(Application::Instance->activeView()))
        return true;
    return false;
}

Action * StdCmdBindViewCamera::createAction(void)
{
    Action *pcAction;
    pcAction = new ViewCameraBindingAction(this, getMainWindow());
    applyCommandData(this->className(), pcAction);
    return pcAction;
}

//===========================================================================
// Std_CloseLinkedView
//===========================================================================

DEF_STD_CMD_A(StdCmdCloseLinkedView)

StdCmdCloseLinkedView::StdCmdCloseLinkedView()
  :Command("Std_CloseLinkedView")
{
  sGroup        = QT_TR_NOOP("View");
  sMenuText     = QT_TR_NOOP("Close all linked view");
  sToolTipText  = QT_TR_NOOP("Close all views of the linked documents.\n"
                             "The linked documents stayed open.");
  sWhatsThis    = "Std_CloseLinkedView";
  sStatusTip    = sToolTipText;
  eType         = Alter3DView;
}

void StdCmdCloseLinkedView::activated(int iMsg)
{
    Q_UNUSED(iMsg); 
    App::Document *activeDoc = App::GetApplication().getActiveDocument();
    if (!activeDoc)
        return;
    for(auto doc : activeDoc->getDependentDocuments()) {
        if (doc == activeDoc) continue;
        Gui::Document *gdoc = Application::Instance->getDocument(doc);
        if (!gdoc) continue;
        for(auto view : gdoc->getMDIViews())
            getMainWindow()->removeWindow(view);
    }
}

bool StdCmdCloseLinkedView::isActive(void)
{
  return App::GetApplication().getActiveDocument() != nullptr;
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
    rcCmdMgr.addCommand(new StdCmdToggleGroupVisibility());
    rcCmdMgr.addCommand(new StdCmdToggleShowOnTop());
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
    rcCmdMgr.addCommand(new StdBoxElementSelection());
    rcCmdMgr.addCommand(new StdLassoElementSelection());
    rcCmdMgr.addCommand(new StdTreePreSelection());
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
    rcCmdMgr.addCommand(new StdCmdSelOptions());
    rcCmdMgr.addCommand(new StdCmdSelBack());
    rcCmdMgr.addCommand(new StdCmdSelForward());
    rcCmdMgr.addCommand(new StdCmdSelUp());
    rcCmdMgr.addCommand(new StdCmdTreeViewActions());
    rcCmdMgr.addCommand(new StdCmdBindViewCamera());
    rcCmdMgr.addCommand(new StdCmdPickGeometry());
    rcCmdMgr.addCommand(new StdCmdCloseLinkedView());

#ifdef FC_HAS_DOCK_OVERLAY
    rcCmdMgr.addCommand(new StdCmdDockOverlay());
#endif

    auto hGrp = App::GetApplication().GetParameterGroupByPath("User parameter:BaseApp/Preferences/View");
    if(hGrp->GetASCII("GestureRollFwdCommand").empty())
        hGrp->SetASCII("GestureRollFwdCommand","Std_SelForward");
    if(hGrp->GetASCII("GestureRollBackCommand").empty())
        hGrp->SetASCII("GestureRollBackCommand","Std_SelBack");
}

} // namespace Gui
