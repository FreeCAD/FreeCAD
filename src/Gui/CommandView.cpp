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

#include <sstream>
#include <Inventor/events/SoMouseButtonEvent.h>
#include <Inventor/nodes/SoOrthographicCamera.h>
#include <Inventor/nodes/SoPerspectiveCamera.h>
#include <Inventor/SoPickedPoint.h>
#include <QApplication>
#include <QDialog>
#include <QDomDocument>
#include <QDomElement>
#include <QFile>
#include <QFileInfo>
#include <QFont>
#include <QFontMetrics>
#include <QImageReader>
#include <QMessageBox>
#include <QPainter>
#include <QPointer>
#include <QTextStream>

#include <App/ComplexGeoDataPy.h>
#include <App/Document.h>
#include <App/DocumentObject.h>
#include <App/DocumentObjectGroup.h>
#include <App/DocumentObserver.h>
#include <App/GeoFeature.h>
#include <App/GeoFeatureGroupExtension.h>
#include <App/Part.h>
#include <App/Link.h>
#include <Base/Console.h>
#include <Base/Parameter.h>

#include "Command.h"
#include "Action.h"
#include "Application.h"
#include "BitmapFactory.h"
#include "Control.h"
#include "Clipping.h"
#include "DemoMode.h"
#include "Dialogs/DlgSettingsImageImp.h"
#include "Document.h"
#include "FileDialog.h"
#include "ImageView.h"
#include "Inventor/SoAxisCrossKit.h"
#include "Macro.h"
#include "MainWindow.h"
#include "Navigation/NavigationStyle.h"
#include "OverlayParams.h"
#include "OverlayManager.h"
#include "SceneInspector.h"
#include "Selection.h"
#include "Selection/SelectionView.h"
#include "SelectionObject.h"
#include "SoFCOffscreenRenderer.h"
#include "TextureMapping.h"
#include "Tools.h"
#include "Tree.h"
#include "TreeParams.h"
#include "Utilities.h"
#include "View3DInventor.h"
#include "View3DInventorViewer.h"
#include "ViewParams.h"
#include "ViewProviderGeometryObject.h"
#include "WaitCursor.h"

using namespace Gui;
using Gui::Dialog::DlgSettingsImageImp;
namespace sp = std::placeholders;

namespace
{
// A helper class to open a transaction when changing properties of view providers.
// It uses the same parameter key as the PropertyView to control the behaviour.
class TransactionView
{
    Gui::Document* document;

public:
    static bool getDefault()
    {
        auto hGrp = App::GetApplication().GetParameterGroupByPath(
            "User parameter:BaseApp/Preferences/PropertyView"
        );
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

    ~TransactionView()
    {
        if (document) {
            document->commitCommand();
        }
    }
};
}  // namespace

//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

DEF_STD_CMD_AC(StdOrthographicCamera)

StdOrthographicCamera::StdOrthographicCamera()
    : Command("Std_OrthographicCamera")
{
    sGroup = "Standard-View";
    sMenuText = QT_TR_NOOP("Orthographic View");
    sToolTipText = QT_TR_NOOP("Switches to orthographic view mode");
    sWhatsThis = "Std_OrthographicCamera";
    sStatusTip = sToolTipText;
    sPixmap = "view-isometric";
    sAccel = "V, O";
    eType = Alter3DView;
}

void StdOrthographicCamera::activated(int iMsg)
{
    if (iMsg == 1) {
        auto view = qobject_cast<View3DInventor*>(getMainWindow()->activeWindow());
        SoCamera* cam = view->getViewer()->getSoRenderManager()->getCamera();
        if (!cam || cam->getTypeId() != SoOrthographicCamera::getClassTypeId()) {

            doCommand(Command::Gui, "Gui.activeDocument().activeView().setCameraType(\"Orthographic\")");
        }
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

        if (mode != check) {
            _pcAction->setChecked(mode);
        }
        return true;
    }

    return false;
}

Action* StdOrthographicCamera::createAction()
{
    Action* pcAction = Command::createAction();
    pcAction->setCheckable(true);
    return pcAction;
}

DEF_STD_CMD_AC(StdPerspectiveCamera)

StdPerspectiveCamera::StdPerspectiveCamera()
    : Command("Std_PerspectiveCamera")
{
    sGroup = "Standard-View";
    sMenuText = QT_TR_NOOP("Perspective View");
    sToolTipText = QT_TR_NOOP("Switches to perspective view mode");
    sWhatsThis = "Std_PerspectiveCamera";
    sStatusTip = sToolTipText;
    sPixmap = "view-perspective";
    sAccel = "V, P";
    eType = Alter3DView;
}

void StdPerspectiveCamera::activated(int iMsg)
{
    if (iMsg == 1) {
        auto view = qobject_cast<View3DInventor*>(getMainWindow()->activeWindow());
        SoCamera* cam = view->getViewer()->getSoRenderManager()->getCamera();
        if (!cam || cam->getTypeId() != SoPerspectiveCamera::getClassTypeId()) {

            doCommand(Command::Gui, "Gui.activeDocument().activeView().setCameraType(\"Perspective\")");
        }
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

        if (mode != check) {
            _pcAction->setChecked(mode);
        }

        return true;
    }

    return false;
}

Action* StdPerspectiveCamera::createAction()
{
    Action* pcAction = Command::createAction();
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
    sGroup = "Standard-View";
    sMenuText = QT_TR_NOOP("Save Current Camera");
    sToolTipText = QT_TR_NOOP("Saves the current camera settings");
    sStatusTip = sToolTipText;
    sWhatsThis = "Std_ViewSaveCamera";
    eType = Alter3DView;
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
    sGroup = "Standard-View";
    sMenuText = QT_TR_NOOP("Restore Saved Camera");
    sToolTipText = QT_TR_NOOP("Restores the saved camera settings");
    sStatusTip = sToolTipText;
    sWhatsThis = "Std_ViewRestoreCamera";
    eType = Alter3DView;
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
class StdCmdFreezeViews: public Gui::Command
{
public:
    StdCmdFreezeViews();
    ~StdCmdFreezeViews() override = default;
    const char* className() const override
    {
        return "StdCmdFreezeViews";
    }

    void setShortcut(const QString&) override;
    QString getShortcut() const override;

protected:
    void activated(int iMsg) override;
    bool isActive() override;
    Action* createAction() override;
    void languageChange() override;

private:
    void onSaveViews();
    void onRestoreViews();

private:
    const int maxViews {50};
    int savedViews {0};
    int offset {0};
    QAction* saveView {nullptr};
    QAction* freezeView {nullptr};
    QAction* clearView {nullptr};
    QAction* separator {nullptr};
};

StdCmdFreezeViews::StdCmdFreezeViews()
    : Command("Std_FreezeViews")
{
    sGroup = "Standard-View";
    sMenuText = QT_TR_NOOP("F&reeze Display");
    sToolTipText = QT_TR_NOOP("Freezes the current view position");
    sWhatsThis = "Std_FreezeViews";
    sStatusTip = sToolTipText;
    sAccel = "Shift+F";
    eType = Alter3DView;
}

Action* StdCmdFreezeViews::createAction()
{
    auto pcAction = new ActionGroup(this, getMainWindow());
    pcAction->setDropDownMenu(true);
    applyCommandData(this->className(), pcAction);

    // add the action items
    saveView = pcAction->addAction(QObject::tr("&Save Views…"));
    saveView->setWhatsThis(QString::fromLatin1(getWhatsThis()));
    QAction* loadView = pcAction->addAction(QObject::tr("&Load Views…"));
    loadView->setWhatsThis(QString::fromLatin1(getWhatsThis()));
    pcAction->addAction(QStringLiteral(""))->setSeparator(true);
    freezeView = pcAction->addAction(QObject::tr("F&reeze View"));
    freezeView->setShortcut(QString::fromLatin1(getAccel()));
    freezeView->setWhatsThis(QString::fromLatin1(getWhatsThis()));
    clearView = pcAction->addAction(QObject::tr("&Clear Views"));
    clearView->setWhatsThis(QString::fromLatin1(getWhatsThis()));
    separator = pcAction->addAction(QStringLiteral(""));
    separator->setSeparator(true);
    offset = pcAction->actions().count();

    // allow up to 50 views
    for (int i = 0; i < maxViews; i++) {
        pcAction->addAction(QStringLiteral(""))->setVisible(false);
    }

    return pcAction;
}

void StdCmdFreezeViews::setShortcut(const QString& shortcut)
{
    if (freezeView) {
        freezeView->setShortcut(shortcut);
    }
}

QString StdCmdFreezeViews::getShortcut() const
{
    if (freezeView) {
        return freezeView->shortcut().toString();
    }
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
        const char* ppReturn = nullptr;
        getGuiApplication()->sendMsgToActiveView("GetCamera", &ppReturn);

        QList<QAction*> acts = pcAction->actions();
        int index = 1;
        for (QList<QAction*>::Iterator it = acts.begin() + offset; it != acts.end(); ++it, index++) {
            if (!(*it)->isVisible()) {
                savedViews++;
                QString viewnr = QString(QObject::tr("Restore View &%1")).arg(index);
                (*it)->setText(viewnr);
                (*it)->setToolTip(QString::fromLatin1(ppReturn));
                (*it)->setVisible(true);
                if (index < 10) {
                    (*it)->setShortcut(QKeySequence(QStringLiteral("CTRL+%1").arg(index)));
                }
                break;
            }
        }
    }
    else if (iMsg == 4) {
        savedViews = 0;
        QList<QAction*> acts = pcAction->actions();
        for (QList<QAction*>::Iterator it = acts.begin() + offset; it != acts.end(); ++it) {
            (*it)->setVisible(false);
        }
    }
    else if (iMsg >= offset) {
        // Activate a view
        QList<QAction*> acts = pcAction->actions();
        QString data = acts[iMsg]->toolTip();
        QString send = QStringLiteral("SetCamera %1").arg(data);
        getGuiApplication()->sendMsgToActiveView(send.toLatin1());
    }
}

void StdCmdFreezeViews::onSaveViews()
{
    // Save the views to an XML file
    QString fn = FileDialog::getSaveFileName(
        getMainWindow(),
        QObject::tr("Save Frozen Views"),
        QString(),
        QStringLiteral("%1 (*.cam)").arg(QObject::tr("Frozen views"))
    );
    if (fn.isEmpty()) {
        return;
    }
    QFile file(fn);
    if (file.open(QFile::WriteOnly)) {
        QTextStream str(&file);
        auto pcAction = qobject_cast<ActionGroup*>(_pcAction);
        QList<QAction*> acts = pcAction->actions();
        str << "<?xml version='1.0' encoding='utf-8'?>\n"
            << "<FrozenViews SchemaVersion=\"1\">\n";
        str << "  <Views Count=\"" << savedViews << "\">\n";

        for (QList<QAction*>::Iterator it = acts.begin() + offset; it != acts.end(); ++it) {
            if (!(*it)->isVisible()) {
                break;
            }
            QString data = (*it)->toolTip();

            // remove the first line because it's a comment like '#Inventor V2.1 ascii'
            QString viewPos;
            if (!data.isEmpty()) {
                QStringList lines = data.split(QStringLiteral("\n"));
                if (lines.size() > 1) {
                    lines.pop_front();
                }
                viewPos = lines.join(QStringLiteral(" "));
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
        auto ret = QMessageBox::question(
            getMainWindow(),
            QObject::tr("Restore Views"),
            QObject::tr(
                "Importing the restored views would clear the already stored views.\n"
                "Continue?"
            ),
            QMessageBox::Yes | QMessageBox::No,
            QMessageBox::Yes
        );
        if (ret != QMessageBox::Yes) {
            return;
        }
    }

    // Restore the views from an XML file
    QString fn = FileDialog::getOpenFileName(
        getMainWindow(),
        QObject::tr("Restore Frozen Views"),
        QString(),
        QStringLiteral("%1 (*.cam)").arg(QObject::tr("Frozen views"))
    );
    if (fn.isEmpty()) {
        return;
    }
    QFile file(fn);
    if (!file.open(QFile::ReadOnly)) {
        QMessageBox::critical(
            getMainWindow(),
            QObject::tr("Restore Views"),
            QObject::tr("Cannot open file '%1'.").arg(fn)
        );
        return;
    }

    QDomDocument xmlDocument;

#if QT_VERSION >= QT_VERSION_CHECK(6, 5, 0)
    if (const auto result
        = xmlDocument.setContent(&file, QDomDocument::ParseOption::UseNamespaceProcessing);
        !result) {
        std::cerr << "Parse error in XML content at line " << result.errorLine << ", column "
                  << result.errorColumn << ": " << qPrintable(result.errorMessage) << std::endl;
        return;
    }
#else
    QString errorStr;
    int errorLine;
    int errorColumn;

    // evaluate the XML content
    if (!xmlDocument.setContent(&file, true, &errorStr, &errorLine, &errorColumn)) {
        std::cerr << "Parse error in XML content at line " << errorLine << ", column "
                  << errorColumn << ": " << (const char*)errorStr.toLatin1() << std::endl;
        return;
    }
#endif

    // get the root element
    QDomElement root = xmlDocument.documentElement();
    if (root.tagName() != QLatin1String("FrozenViews")) {
        std::cerr << "Unexpected XML structure" << std::endl;
        return;
    }

    bool ok;
    int scheme = root.attribute(QStringLiteral("SchemaVersion")).toInt(&ok);
    if (!ok) {
        return;
    }
    // SchemeVersion "1"
    if (scheme == 1) {
        // read the views, ignore the attribute 'Count'
        QDomElement child = root.firstChildElement(QStringLiteral("Views"));
        QDomElement views = child.firstChildElement(QStringLiteral("Camera"));
        QStringList cameras;
        while (!views.isNull()) {
            QString setting = views.attribute(QStringLiteral("settings"));
            cameras << setting;
            views = views.nextSiblingElement(QStringLiteral("Camera"));
        }

        // use this rather than the attribute 'Count' because it could be
        // changed from outside
        int ct = cameras.count();
        auto pcAction = qobject_cast<ActionGroup*>(_pcAction);
        QList<QAction*> acts = pcAction->actions();

        int numRestoredViews = std::min<int>(ct, acts.size() - offset);
        savedViews = numRestoredViews;

        if (numRestoredViews > 0) {
            separator->setVisible(true);
        }
        for (int i = 0; i < numRestoredViews; i++) {
            QString setting = cameras[i];
            QString viewnr = QString(QObject::tr("Restore View &%1")).arg(i + 1);
            acts[i + offset]->setText(viewnr);
            acts[i + offset]->setToolTip(setting);
            acts[i + offset]->setVisible(true);
            if (i < 9) {
                acts[i + offset]->setShortcut(QKeySequence(QStringLiteral("CTRL+%1").arg(i + 1)));
            }
        }

        // if less views than actions
        for (int index = numRestoredViews + offset; index < acts.count(); index++) {
            acts[index]->setVisible(false);
        }
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

    if (!_pcAction) {
        return;
    }
    auto pcAction = qobject_cast<ActionGroup*>(_pcAction);
    QList<QAction*> acts = pcAction->actions();
    acts[0]->setText(QObject::tr("&Save Views…"));
    acts[1]->setText(QObject::tr("&Load Views…"));
    acts[3]->setText(QObject::tr("F&reeze View"));
    acts[4]->setText(QObject::tr("&Clear Views"));
    int index = 1;
    for (QList<QAction*>::Iterator it = acts.begin() + 5; it != acts.end(); ++it, index++) {
        if ((*it)->isVisible()) {
            QString viewnr = QString(QObject::tr("Restore View &%1")).arg(index);
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
    sGroup = "Standard-View";
    sMenuText = QT_TR_NOOP("Clippin&g View");
    sToolTipText = QT_TR_NOOP("Toggles clipping of the active view");
    sWhatsThis = "Std_ToggleClipPlane";
    sStatusTip = sToolTipText;
    sPixmap = "Std_ToggleClipPlane";
    eType = Alter3DView;
}

Action* StdCmdToggleClipPlane::createAction()
{
    Action* pcAction = Command::createAction();
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
class StdCmdDrawStyle: public Gui::Command
{
public:
    StdCmdDrawStyle();
    ~StdCmdDrawStyle() override = default;
    void languageChange() override;
    const char* className() const override
    {
        return "StdCmdDrawStyle";
    }
    void updateIcon(const Gui::MDIView* view);

protected:
    void activated(int iMsg) override;
    bool isActive() override;
    Gui::Action* createAction() override;
};

StdCmdDrawStyle::StdCmdDrawStyle()
    : Command("Std_DrawStyle")
{
    sGroup = "Standard-View";
    sMenuText = QT_TR_NOOP("&Draw Style");
    sToolTipText = QT_TR_NOOP("Changes the draw style of the objects");
    sStatusTip = sToolTipText;
    sWhatsThis = "Std_DrawStyle";
    sPixmap = "DrawStyleAsIs";
    eType = Alter3DView;

    this->getGuiApplication()->signalActivateView.connect([this](auto view) {
        this->updateIcon(view);
    });
}

Gui::Action* StdCmdDrawStyle::createAction()
{
    auto pcAction = new Gui::ActionGroup(this, Gui::getMainWindow());
    pcAction->setDropDownMenu(true);
    pcAction->setIsMode(true);
    applyCommandData(this->className(), pcAction);

    QAction* a0 = pcAction->addAction(QString());
    a0->setCheckable(true);
    a0->setIcon(BitmapFactory().iconFromTheme("DrawStyleAsIs"));
    a0->setChecked(true);
    a0->setObjectName(QStringLiteral("Std_DrawStyleAsIs"));
    a0->setShortcut(QKeySequence(QStringLiteral("V,1")));
    a0->setWhatsThis(QString::fromLatin1(getWhatsThis()));
    QAction* a1 = pcAction->addAction(QString());
    a1->setCheckable(true);
    a1->setIcon(BitmapFactory().iconFromTheme("DrawStylePoints"));
    a1->setObjectName(QStringLiteral("Std_DrawStylePoints"));
    a1->setShortcut(QKeySequence(QStringLiteral("V,2")));
    a1->setWhatsThis(QString::fromLatin1(getWhatsThis()));
    QAction* a2 = pcAction->addAction(QString());
    a2->setCheckable(true);
    a2->setIcon(BitmapFactory().iconFromTheme("DrawStyleWireFrame"));
    a2->setObjectName(QStringLiteral("Std_DrawStyleWireframe"));
    a2->setShortcut(QKeySequence(QStringLiteral("V,3")));
    a2->setWhatsThis(QString::fromLatin1(getWhatsThis()));
    QAction* a3 = pcAction->addAction(QString());
    a3->setCheckable(true);
    a3->setIcon(BitmapFactory().iconFromTheme("DrawStyleHiddenLine"));
    a3->setObjectName(QStringLiteral("Std_DrawStyleHiddenLine"));
    a3->setShortcut(QKeySequence(QStringLiteral("V,4")));
    a3->setWhatsThis(QString::fromLatin1(getWhatsThis()));
    QAction* a4 = pcAction->addAction(QString());
    a4->setCheckable(true);
    a4->setIcon(BitmapFactory().iconFromTheme("DrawStyleNoShading"));
    a4->setObjectName(QStringLiteral("Std_DrawStyleNoShading"));
    a4->setShortcut(QKeySequence(QStringLiteral("V,5")));
    a4->setWhatsThis(QString::fromLatin1(getWhatsThis()));
    QAction* a5 = pcAction->addAction(QString());
    a5->setCheckable(true);
    a5->setIcon(BitmapFactory().iconFromTheme("DrawStyleShaded"));
    a5->setObjectName(QStringLiteral("Std_DrawStyleShaded"));
    a5->setShortcut(QKeySequence(QStringLiteral("V,6")));
    a5->setWhatsThis(QString::fromLatin1(getWhatsThis()));
    QAction* a6 = pcAction->addAction(QString());
    a6->setCheckable(true);
    a6->setIcon(BitmapFactory().iconFromTheme("DrawStyleFlatLines"));
    a6->setObjectName(QStringLiteral("Std_DrawStyleFlatLines"));
    a6->setShortcut(QKeySequence(QStringLiteral("V,7")));
    a6->setWhatsThis(QString::fromLatin1(getWhatsThis()));

    pcAction->setIcon(a0->icon());

    _pcAction = pcAction;
    languageChange();
    return pcAction;
}

void StdCmdDrawStyle::languageChange()
{
    Command::languageChange();

    if (!_pcAction) {
        return;
    }
    auto pcAction = qobject_cast<Gui::ActionGroup*>(_pcAction);
    QList<QAction*> a = pcAction->actions();

    a[0]->setText(QCoreApplication::translate("Std_DrawStyle", "&1 As Is"));
    a[0]->setToolTip(QCoreApplication::translate("Std_DrawStyle", "Normal mode"));

    a[1]->setText(QCoreApplication::translate("Std_DrawStyle", "&2 Points"));
    a[1]->setToolTip(QCoreApplication::translate("Std_DrawStyle", "Points mode"));

    a[2]->setText(QCoreApplication::translate("Std_DrawStyle", "&3 Wireframe"));
    a[2]->setToolTip(QCoreApplication::translate("Std_DrawStyle", "Wireframe mode"));

    a[3]->setText(QCoreApplication::translate("Std_DrawStyle", "&4 Hidden Line"));
    a[3]->setToolTip(QCoreApplication::translate("Std_DrawStyle", "Hidden line mode"));

    a[4]->setText(QCoreApplication::translate("Std_DrawStyle", "&5 No Shading"));
    a[4]->setToolTip(QCoreApplication::translate("Std_DrawStyle", "No shading mode"));

    a[5]->setText(QCoreApplication::translate("Std_DrawStyle", "&6 Shaded"));
    a[5]->setToolTip(QCoreApplication::translate("Std_DrawStyle", "Shaded mode"));

    a[6]->setText(QCoreApplication::translate("Std_DrawStyle", "&7 Flat Lines"));
    a[6]->setToolTip(QCoreApplication::translate("Std_DrawStyle", "Flat lines mode"));
}

void StdCmdDrawStyle::updateIcon(const MDIView* view)
{
    const auto view3d = dynamic_cast<const Gui::View3DInventor*>(view);
    if (!view3d) {
        return;
    }
    Gui::View3DInventorViewer* viewer = view3d->getViewer();
    if (!viewer) {
        return;
    }
    std::string mode(viewer->getOverrideMode());
    auto actionGroup = dynamic_cast<Gui::ActionGroup*>(_pcAction);
    if (!actionGroup) {
        return;
    }

    if (mode == "Point") {
        actionGroup->setCheckedAction(1);
        return;
    }
    if (mode == "Wireframe") {
        actionGroup->setCheckedAction(2);
        return;
    }
    if (mode == "Hidden Line") {
        actionGroup->setCheckedAction(3);
        return;
    }
    if (mode == "No shading") {
        actionGroup->setCheckedAction(4);
        return;
    }
    if (mode == "Shaded") {
        actionGroup->setCheckedAction(5);
        return;
    }
    if (mode == "Flat Lines") {
        actionGroup->setCheckedAction(6);
        return;
    }
    actionGroup->setCheckedAction(0);
}

void StdCmdDrawStyle::activated(int iMsg)
{
    Gui::Document* doc = this->getActiveGuiDocument();
    std::list<MDIView*> views = doc->getMDIViews();
    std::list<MDIView*>::iterator viewIt;
    bool oneChangedSignal(false);
    for (viewIt = views.begin(); viewIt != views.end(); ++viewIt) {
        auto view = qobject_cast<View3DInventor*>(*viewIt);
        if (view) {
            View3DInventorViewer* viewer;
            viewer = view->getViewer();
            if (viewer) {
                switch (iMsg) {
                    case 1:
                        (oneChangedSignal) ? viewer->updateOverrideMode("Point")
                                           : viewer->setOverrideMode("Point");
                        break;
                    case 2:
                        (oneChangedSignal) ? viewer->updateOverrideMode("Wireframe")
                                           : viewer->setOverrideMode("Wireframe");
                        break;
                    case 3:
                        (oneChangedSignal) ? viewer->updateOverrideMode("Hidden Line")
                                           : viewer->setOverrideMode("Hidden Line");
                        break;
                    case 4:
                        (oneChangedSignal) ? viewer->updateOverrideMode("No Shading")
                                           : viewer->setOverrideMode("No Shading");
                        break;
                    case 5:
                        (oneChangedSignal) ? viewer->updateOverrideMode("Shaded")
                                           : viewer->setOverrideMode("Shaded");
                        break;
                    case 6:
                        (oneChangedSignal) ? viewer->updateOverrideMode("Flat Lines")
                                           : viewer->setOverrideMode("Flat Lines");
                        break;
                    default:
                        (oneChangedSignal) ? viewer->updateOverrideMode("As Is")
                                           : viewer->setOverrideMode("As Is");
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
    sGroup = "Standard-View";
    sMenuText = QT_TR_NOOP("Toggle &Visibility");
    sToolTipText = QT_TR_NOOP("Toggles the visibility of the selection");
    sStatusTip = sToolTipText;
    sWhatsThis = "Std_ToggleVisibility";
    sPixmap = "Std_ToggleVisibility";
    sAccel = "Space";
    eType = Alter3DView;
}


void StdCmdToggleVisibility::activated(int iMsg)
{
    Q_UNUSED(iMsg);
    TransactionView transaction(
        getActiveGuiDocument(),
        QT_TRANSLATE_NOOP("Command", "Toggle Visibility")
    );
    Selection().setVisible(SelectionSingleton::VisToggle);
}

bool StdCmdToggleVisibility::isActive()
{
    return (Gui::Selection().size() != 0);
}

//===========================================================================
// Std_ToggleTransparency
//===========================================================================
DEF_STD_CMD_A(StdCmdToggleTransparency)

StdCmdToggleTransparency::StdCmdToggleTransparency()
    : Command("Std_ToggleTransparency")
{
    sGroup = "Standard-View";
    sMenuText = QT_TR_NOOP("Toggle Transparenc&y");
    static std::string toolTip = std::string("<p>")
        + QT_TR_NOOP("Toggles the transparency of the selected objects. Transparency "
                     "can be fine-tuned in the appearance task dialog")
        + "</p>";
    sToolTipText = toolTip.c_str();
    sStatusTip = sToolTipText;
    sWhatsThis = "Std_ToggleTransparency";
    sPixmap = "Std_ToggleTransparency";
    sAccel = "V,T";
    eType = Alter3DView;
}

void StdCmdToggleTransparency::activated(int iMsg)
{
    Q_UNUSED(iMsg);
    getActiveGuiDocument()->openCommand(QT_TRANSLATE_NOOP("Command", "Toggle Transparency"));

    std::vector<Gui::SelectionSingleton::SelObj> sels = Gui::Selection().getCompleteSelection();

    std::vector<Gui::ViewProvider*> viewsToToggle = {};

    for (Gui::SelectionSingleton::SelObj& sel : sels) {
        App::DocumentObject* obj = sel.pObject;
        if (!obj) {
            continue;
        }

        bool isGroup = dynamic_cast<App::Part*>(obj) || dynamic_cast<App::LinkGroup*>(obj)
            || dynamic_cast<App::DocumentObjectGroup*>(obj);

        auto addObjects = [](App::DocumentObject* obj, std::vector<Gui::ViewProvider*>& views) {
            App::Document* doc = obj->getDocument();
            Gui::ViewProvider* view = Application::Instance->getDocument(doc)->getViewProvider(obj);
            App::Property* prop = view->getPropertyByName("Transparency");
            if (prop && prop->isDerivedFrom<App::PropertyInteger>()) {
                // To prevent toggling the tip of a PD body (see #11353), we check if the parent has
                // a Tip prop.
                const std::vector<App::DocumentObject*> parents = obj->getInList();
                if (!parents.empty()) {
                    App::Document* parentDoc = parents[0]->getDocument();
                    Gui::ViewProvider* parentView
                        = Application::Instance->getDocument(parentDoc)->getViewProvider(parents[0]);
                    App::Property* parentProp = parents[0]->getPropertyByName("Tip");
                    if (parentProp) {
                        // Make sure it has a transparency prop too
                        parentProp = parentView->getPropertyByName("Transparency");
                        if (parentProp && parentProp->isDerivedFrom<App::PropertyInteger>()) {
                            view = parentView;
                        }
                    }
                }

                if (std::ranges::find(views, view) == views.end()) {
                    views.push_back(view);
                }
            }
        };

        if (isGroup) {
            for (App::DocumentObject* subobj : obj->getOutListRecursive()) {
                addObjects(subobj, viewsToToggle);
            }
        }
        else {
            addObjects(obj, viewsToToggle);
        }
    }

    bool oneTransparent = false;
    for (auto* view : viewsToToggle) {
        App::Property* prop = view->getPropertyByName("Transparency");
        if (prop && prop->isDerivedFrom<App::PropertyInteger>()) {
            auto* transparencyProp = dynamic_cast<App::PropertyInteger*>(prop);
            int transparency = transparencyProp->getValue();
            if (transparency != 0) {
                oneTransparent = true;
            }
        }
    }

    auto hGrp = App::GetApplication().GetParameterGroupByPath(
        "User parameter:BaseApp/Preferences/View"
    );
    int userTransparency = hGrp->GetInt("ToggleTransparency", 70);

    int transparency = oneTransparent ? 0 : userTransparency;

    for (auto* view : viewsToToggle) {
        App::Property* prop = view->getPropertyByName("Transparency");
        if (prop && prop->isDerivedFrom<App::PropertyInteger>()) {
            auto* transparencyProp = dynamic_cast<App::PropertyInteger*>(prop);
            transparencyProp->setValue(transparency);
        }
    }

    getActiveGuiDocument()->commitCommand();
}

bool StdCmdToggleTransparency::isActive()
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
    sGroup = "Standard-View";
    sMenuText = QT_TR_NOOP("Toggle Se&lectability");
    sToolTipText = QT_TR_NOOP("Toggles the property of the objects to get selected in the 3D view");
    sStatusTip = sToolTipText;
    sWhatsThis = "Std_ToggleSelectability";
    sPixmap = "view-unselectable";
    eType = Alter3DView;
}

void StdCmdToggleSelectability::activated(int iMsg)
{
    Q_UNUSED(iMsg);
    // go through all documents
    const std::vector<App::Document*> docs = App::GetApplication().getDocuments();
    for (const auto& doc : docs) {
        Document* pcDoc = Application::Instance->getDocument(doc);
        std::vector<App::DocumentObject*> sel
            = Selection().getObjectsOfType(App::DocumentObject::getClassTypeId(), doc->getName());

        if (sel.empty()) {
            continue;
        }

        TransactionView transaction(pcDoc, QT_TRANSLATE_NOOP("Command", "Toggle Selectability"));

        for (const auto& ft : sel) {
            ViewProvider* pr = pcDoc->getViewProviderByName(ft->getNameInDocument());
            if (pr && pr->isDerivedFrom<ViewProviderGeometryObject>()) {
                if (static_cast<ViewProviderGeometryObject*>(pr)->Selectable.getValue()) {
                    doCommand(
                        Gui,
                        "Gui.getDocument(\"%s\").getObject(\"%s\").Selectable=False",
                        doc->getName(),
                        ft->getNameInDocument()
                    );
                }
                else {
                    doCommand(
                        Gui,
                        "Gui.getDocument(\"%s\").getObject(\"%s\").Selectable=True",
                        doc->getName(),
                        ft->getNameInDocument()
                    );
                }
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
    sGroup = "Standard-View";
    sMenuText = QT_TR_NOOP("Sho&w Selection");
    sToolTipText = QT_TR_NOOP("Shows all selected objects");
    sStatusTip = sToolTipText;
    sWhatsThis = "Std_ShowSelection";
    sPixmap = "Std_ShowSelection";
    eType = Alter3DView;
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
    sGroup = "Standard-View";
    sMenuText = QT_TR_NOOP("&Hide Selection");
    sToolTipText = QT_TR_NOOP("Hides all selected objects");
    sStatusTip = sToolTipText;
    sWhatsThis = "Std_HideSelection";
    sPixmap = "Std_HideSelection";
    eType = Alter3DView;
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
    sGroup = "Standard-View";
    sMenuText = QT_TR_NOOP("&Select Visible Objects");
    sToolTipText = QT_TR_NOOP("Selects all visible objects in the active document");
    sStatusTip = sToolTipText;
    sWhatsThis = "Std_SelectVisibleObjects";
    sPixmap = "Std_SelectVisibleObjects";
    eType = Alter3DView;
}

void StdCmdSelectVisibleObjects::activated(int iMsg)
{
    Q_UNUSED(iMsg);
    // go through active document
    Gui::Document* doc = Application::Instance->activeDocument();
    App::Document* app = doc->getDocument();
    const std::vector<App::DocumentObject*> obj = app->getObjectsOfType(
        App::DocumentObject::getClassTypeId()
    );

    std::vector<App::DocumentObject*> visible;
    visible.reserve(obj.size());
    for (const auto& it : obj) {
        if (doc->isShow(it->getNameInDocument())) {
            visible.push_back(it);
        }
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
    sGroup = "Standard-View";
    sMenuText = QT_TR_NOOP("To&ggle All Objects");
    sToolTipText = QT_TR_NOOP("Toggles the visibility of all objects in the active document");
    sStatusTip = sToolTipText;
    sWhatsThis = "Std_ToggleObjects";
    sPixmap = "Std_ToggleObjects";
    eType = Alter3DView;
}

void StdCmdToggleObjects::activated(int iMsg)
{
    Q_UNUSED(iMsg);
    // go through active document
    Gui::Document* doc = Application::Instance->activeDocument();
    App::Document* app = doc->getDocument();
    const std::vector<App::DocumentObject*> obj = app->getObjectsOfType(
        App::DocumentObject::getClassTypeId()
    );

    for (const auto& it : obj) {
        if (doc->isShow(it->getNameInDocument())) {
            doCommand(
                Gui,
                "Gui.getDocument(\"%s\").getObject(\"%s\").Visibility=False",
                app->getName(),
                it->getNameInDocument()
            );
        }
        else {
            doCommand(
                Gui,
                "Gui.getDocument(\"%s\").getObject(\"%s\").Visibility=True",
                app->getName(),
                it->getNameInDocument()
            );
        }
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
    sGroup = "Standard-View";
    sMenuText = QT_TR_NOOP("Show &All Objects");
    sToolTipText = QT_TR_NOOP("Shows all objects in the document");
    sStatusTip = sToolTipText;
    sWhatsThis = "Std_ShowObjects";
    sPixmap = "Std_ShowObjects";
    eType = Alter3DView;
}

void StdCmdShowObjects::activated(int iMsg)
{
    Q_UNUSED(iMsg);
    // go through active document
    Gui::Document* doc = Application::Instance->activeDocument();
    App::Document* app = doc->getDocument();
    const std::vector<App::DocumentObject*> obj = app->getObjectsOfType(
        App::DocumentObject::getClassTypeId()
    );

    for (const auto& it : obj) {
        doCommand(
            Gui,
            "Gui.getDocument(\"%s\").getObject(\"%s\").Visibility=True",
            app->getName(),
            it->getNameInDocument()
        );
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
    sGroup = "Standard-View";
    sMenuText = QT_TR_NOOP("Hide All &Objects");
    sToolTipText = QT_TR_NOOP("Hides all objects in the document");
    sStatusTip = sToolTipText;
    sWhatsThis = "Std_HideObjects";
    sPixmap = "Std_HideObjects";
    eType = Alter3DView;
}

void StdCmdHideObjects::activated(int iMsg)
{
    Q_UNUSED(iMsg);
    // go through active document
    Gui::Document* doc = Application::Instance->activeDocument();
    App::Document* app = doc->getDocument();
    const std::vector<App::DocumentObject*> obj = app->getObjectsOfType(
        App::DocumentObject::getClassTypeId()
    );

    for (const auto& it : obj) {
        doCommand(
            Gui,
            "Gui.getDocument(\"%s\").getObject(\"%s\").Visibility=False",
            app->getName(),
            it->getNameInDocument()
        );
    }
}

bool StdCmdHideObjects::isActive()
{
    return App::GetApplication().getActiveDocument();
}

//===========================================================================
// Std_ViewHome
//===========================================================================
DEF_3DV_CMD(StdCmdViewHome)

StdCmdViewHome::StdCmdViewHome()
    : Command("Std_ViewHome")
{
    sGroup = "Standard-View";
    sMenuText = QT_TR_NOOP("&Home");
    sToolTipText = QT_TR_NOOP("Sets the camera to the default home view");
    sWhatsThis = "Std_ViewHome";
    sStatusTip = sToolTipText;
    sPixmap = "Std_ViewHome";
    sAccel = "Home";
    eType = Alter3DView;
}

void StdCmdViewHome::activated(int iMsg)
{
    Q_UNUSED(iMsg);

    auto hGrp = App::GetApplication().GetParameterGroupByPath(
        "User parameter:BaseApp/Preferences/View"
    );
    std::string default_view = hGrp->GetASCII("NewDocumentCameraOrientation", "Top");
    doCommand(
        Command::Gui,
        "Gui.activeDocument().activeView().viewDefaultOrientation('%s',0)",
        default_view.c_str()
    );
    doCommand(Command::Gui, "Gui.SendMsgToActiveView(\"ViewFit\")");
}

//===========================================================================
// Std_ViewBottom
//===========================================================================
DEF_3DV_CMD(StdCmdViewBottom)

StdCmdViewBottom::StdCmdViewBottom()
    : Command("Std_ViewBottom")
{
    sGroup = "Standard-View";
    sMenuText = QT_TR_NOOP("&5 Bottom");
    sToolTipText = QT_TR_NOOP("Sets the camera to the bottom view");
    sWhatsThis = "Std_ViewBottom";
    sStatusTip = sToolTipText;
    sPixmap = "view-bottom";
    sAccel = "5";
    eType = Alter3DView;
}

void StdCmdViewBottom::activated(int iMsg)
{
    Q_UNUSED(iMsg);
    doCommand(Command::Gui, "Gui.activeDocument().activeView().viewBottom()");
}

//===========================================================================
// Std_ViewFront
//===========================================================================
DEF_3DV_CMD(StdCmdViewFront)

StdCmdViewFront::StdCmdViewFront()
    : Command("Std_ViewFront")
{
    sGroup = "Standard-View";
    sMenuText = QT_TR_NOOP("&1 Front");
    sToolTipText = QT_TR_NOOP("Sets the camera to the front view");
    sWhatsThis = "Std_ViewFront";
    sStatusTip = sToolTipText;
    sPixmap = "view-front";
    sAccel = "1";
    eType = Alter3DView;
}

void StdCmdViewFront::activated(int iMsg)
{
    Q_UNUSED(iMsg);
    doCommand(Command::Gui, "Gui.activeDocument().activeView().viewFront()");
}

//===========================================================================
// Std_ViewLeft
//===========================================================================
DEF_3DV_CMD(StdCmdViewLeft)

StdCmdViewLeft::StdCmdViewLeft()
    : Command("Std_ViewLeft")
{
    sGroup = "Standard-View";
    sMenuText = QT_TR_NOOP("&6 Left");
    sToolTipText = QT_TR_NOOP("Sets the camera to the left view");
    sWhatsThis = "Std_ViewLeft";
    sStatusTip = sToolTipText;
    sPixmap = "view-left";
    sAccel = "6";
    eType = Alter3DView;
}

void StdCmdViewLeft::activated(int iMsg)
{
    Q_UNUSED(iMsg);
    doCommand(Command::Gui, "Gui.activeDocument().activeView().viewLeft()");
}

//===========================================================================
// Std_ViewRear
//===========================================================================
DEF_3DV_CMD(StdCmdViewRear)

StdCmdViewRear::StdCmdViewRear()
    : Command("Std_ViewRear")
{
    sGroup = "Standard-View";
    sMenuText = QT_TR_NOOP("&4 Rear");
    sToolTipText = QT_TR_NOOP("Sets the camera to the rear view");
    sWhatsThis = "Std_ViewRear";
    sStatusTip = sToolTipText;
    sPixmap = "view-rear";
    sAccel = "4";
    eType = Alter3DView;
}

void StdCmdViewRear::activated(int iMsg)
{
    Q_UNUSED(iMsg);
    doCommand(Command::Gui, "Gui.activeDocument().activeView().viewRear()");
}

//===========================================================================
// Std_ViewRight
//===========================================================================
DEF_3DV_CMD(StdCmdViewRight)

StdCmdViewRight::StdCmdViewRight()
    : Command("Std_ViewRight")
{
    sGroup = "Standard-View";
    sMenuText = QT_TR_NOOP("&3 Right");
    sToolTipText = QT_TR_NOOP("Sets the camera to the right view");
    sWhatsThis = "Std_ViewRight";
    sStatusTip = sToolTipText;
    sPixmap = "view-right";
    sAccel = "3";
    eType = Alter3DView;
}

void StdCmdViewRight::activated(int iMsg)
{
    Q_UNUSED(iMsg);
    doCommand(Command::Gui, "Gui.activeDocument().activeView().viewRight()");
}

//===========================================================================
// Std_ViewTop
//===========================================================================
DEF_3DV_CMD(StdCmdViewTop)

StdCmdViewTop::StdCmdViewTop()
    : Command("Std_ViewTop")
{
    sGroup = "Standard-View";
    sMenuText = QT_TR_NOOP("&2 Top");
    sToolTipText = QT_TR_NOOP("Sets the camera to the top view");
    sWhatsThis = "Std_ViewTop";
    sStatusTip = sToolTipText;
    sPixmap = "view-top";
    sAccel = "2";
    eType = Alter3DView;
}

void StdCmdViewTop::activated(int iMsg)
{
    Q_UNUSED(iMsg);
    doCommand(Command::Gui, "Gui.activeDocument().activeView().viewTop()");
}


//===========================================================================
// Std_ViewIsometric
//===========================================================================
DEF_3DV_CMD(StdCmdViewIsometric)

StdCmdViewIsometric::StdCmdViewIsometric()
    : Command("Std_ViewIsometric")
{
    sGroup = "Standard-View";
    sMenuText = QT_TR_NOOP("&Isometric");
    sToolTipText = QT_TR_NOOP("Sets the camera to the isometric view");
    sWhatsThis = "Std_ViewIsometric";
    sStatusTip = sToolTipText;
    sPixmap = "view-axonometric";
    sAccel = "0";
    eType = Alter3DView;
}

void StdCmdViewIsometric::activated(int iMsg)
{
    Q_UNUSED(iMsg);
    doCommand(Command::Gui, "Gui.activeDocument().activeView().viewIsometric()");
}

//===========================================================================
// Std_ViewDimetric
//===========================================================================
DEF_3DV_CMD(StdCmdViewDimetric)

StdCmdViewDimetric::StdCmdViewDimetric()
    : Command("Std_ViewDimetric")
{
    sGroup = "Standard-View";
    sMenuText = QT_TR_NOOP("&Dimetric");
    sToolTipText = QT_TR_NOOP("Sets the camera to the dimetric view");
    sWhatsThis = "Std_ViewDimetric";
    sStatusTip = sToolTipText;
    sPixmap = "Std_ViewDimetric";
    eType = Alter3DView;
}

void StdCmdViewDimetric::activated(int iMsg)
{
    Q_UNUSED(iMsg);
    doCommand(Command::Gui, "Gui.activeDocument().activeView().viewDimetric()");
}

//===========================================================================
// Std_ViewTrimetric
//===========================================================================
DEF_3DV_CMD(StdCmdViewTrimetric)

StdCmdViewTrimetric::StdCmdViewTrimetric()
    : Command("Std_ViewTrimetric")
{
    sGroup = "Standard-View";
    sMenuText = QT_TR_NOOP("&Trimetric");
    sToolTipText = QT_TR_NOOP("Sets the camera to the trimetric view");
    sWhatsThis = "Std_ViewTrimetric";
    sStatusTip = sToolTipText;
    sPixmap = "Std_ViewTrimetric";
    eType = Alter3DView;
}

void StdCmdViewTrimetric::activated(int iMsg)
{
    Q_UNUSED(iMsg);
    doCommand(Command::Gui, "Gui.activeDocument().activeView().viewTrimetric()");
}

//===========================================================================
// Std_ViewRotateLeft
//===========================================================================
DEF_3DV_CMD(StdCmdViewRotateLeft)

StdCmdViewRotateLeft::StdCmdViewRotateLeft()
    : Command("Std_ViewRotateLeft")
{
    sGroup = "Standard-View";
    sMenuText = QT_TR_NOOP("Rotate &Left");
    sToolTipText = QT_TR_NOOP("Rotates the view by 90\xc2\xb0 counter-clockwise");
    sWhatsThis = "Std_ViewRotateLeft";
    sStatusTip = sToolTipText;
    sPixmap = "view-rotate-left";
    sAccel = "Shift+Left";
    eType = Alter3DView;
}

void StdCmdViewRotateLeft::activated(int iMsg)
{
    Q_UNUSED(iMsg);
    doCommand(Command::Gui, "Gui.activeDocument().activeView().viewRotateLeft()");
}


//===========================================================================
// Std_ViewRotateRight
//===========================================================================
DEF_3DV_CMD(StdCmdViewRotateRight)

StdCmdViewRotateRight::StdCmdViewRotateRight()
    : Command("Std_ViewRotateRight")
{
    sGroup = "Standard-View";
    sMenuText = QT_TR_NOOP("Rotates &Right");
    sToolTipText = QT_TR_NOOP("Rotates the view by 90\xc2\xb0 clockwise");
    sWhatsThis = "Std_ViewRotateRight";
    sStatusTip = sToolTipText;
    sPixmap = "view-rotate-right";
    sAccel = "Shift+Right";
    eType = Alter3DView;
}

void StdCmdViewRotateRight::activated(int iMsg)
{
    Q_UNUSED(iMsg);
    doCommand(Command::Gui, "Gui.activeDocument().activeView().viewRotateRight()");
}


//===========================================================================
// Std_ViewFitAll
//===========================================================================
DEF_STD_CMD_A(StdCmdViewFitAll)

StdCmdViewFitAll::StdCmdViewFitAll()
    : Command("Std_ViewFitAll")
{
    sGroup = "Standard-View";
    sMenuText = QT_TR_NOOP("&Fit All");
    sToolTipText = QT_TR_NOOP("Fits all content into the 3D view");
    sWhatsThis = "Std_ViewFitAll";
    sStatusTip = sToolTipText;
    sPixmap = "zoom-all";
    sAccel = "V, F";
    eType = Alter3DView;
}

void StdCmdViewFitAll::activated(int iMsg)
{
    Q_UNUSED(iMsg);
    // doCommand(Command::Gui,"Gui.activeDocument().activeView().fitAll()");
    doCommand(Command::Gui, "Gui.SendMsgToActiveView(\"ViewFit\")");
}

bool StdCmdViewFitAll::isActive()
{
    // return isViewOfType(Gui::View3DInventor::getClassTypeId());
    return getGuiApplication()->sendHasMsgToActiveView("ViewFit");
}

//===========================================================================
// Std_ViewFitSelection
//===========================================================================
DEF_STD_CMD_A(StdCmdViewFitSelection)

StdCmdViewFitSelection::StdCmdViewFitSelection()
    : Command("Std_ViewFitSelection")
{
    sGroup = "Standard-View";
    sMenuText = QT_TR_NOOP("Fit &Selection");
    sToolTipText = QT_TR_NOOP("Fits the selected content into the 3D view");
    sWhatsThis = "Std_ViewFitSelection";
    sStatusTip = sToolTipText;
    sAccel = "V, S";
    sPixmap = "zoom-selection";
    eType = Alter3DView;
}

void StdCmdViewFitSelection::activated(int iMsg)
{
    Q_UNUSED(iMsg);
    doCommand(Command::Gui, "Gui.SendMsgToActiveView(\"ViewSelection\")");
}

bool StdCmdViewFitSelection::isActive()
{
    return getGuiApplication()->sendHasMsgToActiveView("ViewSelection");
}

//===========================================================================
// Std_ViewCommandGroup
//===========================================================================
class StdCmdViewGroup: public Gui::GroupCommand
{
public:
    StdCmdViewGroup()
        : GroupCommand("Std_ViewGroup")
    {
        sGroup = "Standard-View";
        sMenuText = QT_TR_NOOP("Standard &Views");
        sToolTipText = QT_TR_NOOP("Changes to a standard view");
        sStatusTip = sToolTipText;
        sWhatsThis = "Std_ViewGroup";
        sPixmap = "view-isometric";
        eType = Alter3DView;

        setCheckable(false);
        setRememberLast(true);

        addCommand("Std_ViewIsometric");
        addCommand("Std_ViewFront");
        addCommand("Std_ViewTop");
        addCommand("Std_ViewRight");
        addCommand("Std_ViewRear");
        addCommand("Std_ViewBottom");
        addCommand("Std_ViewLeft");
    }

    const char* className() const override
    {
        return "StdCmdViewGroup";
    }

    bool isActive() override
    {
        return hasActiveDocument();
    }
};

//===========================================================================
// Std_ViewDock
//===========================================================================
DEF_STD_CMD_A(StdViewDock)

StdViewDock::StdViewDock()
    : Command("Std_ViewDock")
{
    sGroup = "Standard-View";
    sMenuText = QT_TR_NOOP("&Docked");
    sToolTipText = QT_TR_NOOP(
        "Displays the active view either in fullscreen, undocked, or docked mode"
    );
    sWhatsThis = "Std_ViewDock";
    sStatusTip = sToolTipText;
    sAccel = "V, D";
    eType = Alter3DView;
    bCanLog = false;
}

void StdViewDock::activated(int iMsg)
{
    Q_UNUSED(iMsg);
}

bool StdViewDock::isActive()
{
    MDIView* view = getMainWindow()->activeWindow();
    return view != nullptr;
}

//===========================================================================
// Std_ViewUndock
//===========================================================================
DEF_STD_CMD_A(StdViewUndock)

StdViewUndock::StdViewUndock()
    : Command("Std_ViewUndock")
{
    sGroup = "Standard-View";
    sMenuText = QT_TR_NOOP("&Undocked");
    sToolTipText = QT_TR_NOOP(
        "Displays the active view either in fullscreen, undocked, or docked mode"
    );
    sWhatsThis = "Std_ViewUndock";
    sStatusTip = sToolTipText;
    sAccel = "V, U";
    eType = Alter3DView;
    bCanLog = false;
}

void StdViewUndock::activated(int iMsg)
{
    Q_UNUSED(iMsg);
}

bool StdViewUndock::isActive()
{
    MDIView* view = getMainWindow()->activeWindow();
    return view != nullptr;
}

//===========================================================================
// Std_MainFullscreen
//===========================================================================
DEF_STD_CMD(StdMainFullscreen)

StdMainFullscreen::StdMainFullscreen()
    : Command("Std_MainFullscreen")
{
    sGroup = "Standard-View";
    sMenuText = QT_TR_NOOP("Fullscreen");
    sToolTipText = QT_TR_NOOP("Displays the main window in fullscreen mode");
    sWhatsThis = "Std_MainFullscreen";
    sStatusTip = sToolTipText;
    sPixmap = "view-fullscreen";
    sAccel = "Alt+F11";
    eType = Alter3DView;
}

void StdMainFullscreen::activated(int iMsg)
{
    Q_UNUSED(iMsg);
    MDIView* view = getMainWindow()->activeWindow();

    if (view) {
        view->setCurrentViewMode(MDIView::Child);
    }

    if (getMainWindow()->isFullScreen()) {
        getMainWindow()->showNormal();
    }
    else {
        getMainWindow()->showFullScreen();
    }
}

//===========================================================================
// Std_ViewFullscreen
//===========================================================================
DEF_STD_CMD_A(StdViewFullscreen)

StdViewFullscreen::StdViewFullscreen()
    : Command("Std_ViewFullscreen")
{
    sGroup = "Standard-View";
    sMenuText = QT_TR_NOOP("&Fullscreen");
    sToolTipText = QT_TR_NOOP(
        "Displays the active view either in fullscreen, undocked, or docked mode"
    );
    sWhatsThis = "Std_ViewFullscreen";
    sStatusTip = sToolTipText;
    sPixmap = "view-fullscreen";
    sAccel = "F11";
    eType = Alter3DView;
    bCanLog = false;
}

void StdViewFullscreen::activated(int iMsg)
{
    Q_UNUSED(iMsg);
}

bool StdViewFullscreen::isActive()
{
    MDIView* view = getMainWindow()->activeWindow();
    return view != nullptr;
}

//===========================================================================
// Std_ViewDockUndockFullscreen
//===========================================================================
DEF_STD_CMD_AC(StdViewDockUndockFullscreen)

StdViewDockUndockFullscreen::StdViewDockUndockFullscreen()
    : Command("Std_ViewDockUndockFullscreen")
{
    sGroup = "Standard-View";
    sMenuText = QT_TR_NOOP("D&ocument Window");
    sToolTipText = QT_TR_NOOP(
        "Displays the active view either in fullscreen, undocked, or docked mode"
    );
    sWhatsThis = "Std_ViewDockUndockFullscreen";
    sStatusTip = sToolTipText;
    eType = Alter3DView;

    CommandManager& rcCmdMgr = Application::Instance->commandManager();
    rcCmdMgr.addCommand(new StdViewDock());
    rcCmdMgr.addCommand(new StdViewUndock());
    rcCmdMgr.addCommand(new StdViewFullscreen());
}

Action* StdViewDockUndockFullscreen::createAction()
{
    auto pcAction = new ActionGroup(this, getMainWindow());
    pcAction->setDropDownMenu(true);
    pcAction->setText(QCoreApplication::translate(this->className(), getMenuText()));

    CommandManager& rcCmdMgr = Application::Instance->commandManager();
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
    if (getMainWindow()->isFullScreen()) {
        getMainWindow()->showNormal();
    }

    MDIView* view = getMainWindow()->activeWindow();
    if (!view) {  // no active view
        return;
    }

    const auto oldmode = view->currentViewMode();
    auto mode = (MDIView::ViewMode)iMsg;

    // Pressing the same button again toggles the view back to docked.
    if (mode == oldmode) {
        mode = MDIView::Child;
    }

    if (mode == oldmode) {
        return;
    }

    // Change the view mode after an mdi view was already visible doesn't
    // work well with Qt5 any more because of some strange OpenGL behaviour.
    // A workaround is to clone the mdi view, set its view mode and delete
    // the original view.

    bool needsClone = mode == MDIView::Child || oldmode == MDIView::Child;
    Gui::MDIView* clone = needsClone ? view->clone() : nullptr;

    if (clone) {
        if (mode == MDIView::Child) {
            getMainWindow()->addWindow(clone);
        }
        else {
            clone->setCurrentViewMode(mode);
        }

        // destroy the old view
        view->deleteSelf();
    }
    else {
        // no clone needed, simply change the view mode
        view->setCurrentViewMode(mode);
    }
}

bool StdViewDockUndockFullscreen::isActive()
{
    MDIView* view = getMainWindow()->activeWindow();
    if (!view) {
        return false;
    }

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


//===========================================================================
// Std_ViewVR
//===========================================================================
DEF_STD_CMD_A(StdCmdViewVR)

StdCmdViewVR::StdCmdViewVR()
    : Command("Std_ViewVR")
{
    sGroup = "Standard-View";
    sMenuText = QT_TR_NOOP("FreeCAD VR");
    sToolTipText = QT_TR_NOOP("Extends the FreeCAD 3D Window to a VR device");
    sWhatsThis = "Std_ViewVR";
    sStatusTip = sToolTipText;
    eType = Alter3DView;
}

void StdCmdViewVR::activated(int iMsg)
{
    Q_UNUSED(iMsg);
    doCommand(Command::Gui, "Gui.SendMsgToActiveView(\"ViewVR\")");
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
    sGroup = "Standard-View";
    sMenuText = QT_TR_NOOP("Save &Image…");
    sToolTipText = QT_TR_NOOP("Creates a screenshot of the active view");
    sWhatsThis = "Std_ViewScreenShot";
    sStatusTip = sToolTipText;
    sPixmap = "Std_ViewScreenShot";
    eType = Alter3DView;
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

        Base::Reference<ParameterGrp> hExt = App::GetApplication()
                                                 .GetUserParameter()
                                                 .GetGroup("BaseApp")
                                                 ->GetGroup("Preferences")
                                                 ->GetGroup("General");
        QString ext = QString::fromLatin1(hExt->GetASCII("OffscreenImageFormat").c_str());
        int backtype = hExt->GetInt("OffscreenImageBackground", 0);

        Base::Reference<ParameterGrp> methodGrp = App::GetApplication().GetParameterGroupByPath(
            "User parameter:BaseApp/Preferences/View"
        );
        QByteArray method = methodGrp->GetASCII("SavePicture").c_str();

        QStringList filter;
        QString selFilter;
        for (QStringList::Iterator it = formats.begin(); it != formats.end(); ++it) {
            filter << QStringLiteral("%1 %2 (*.%3)")
                          .arg((*it).toUpper(), QObject::tr("files"), (*it).toLower());
            if (ext == *it) {
                selFilter = filter.last();
            }
        }

        FileOptionsDialog fd(getMainWindow(), Qt::WindowFlags());
        fd.setFileMode(QFileDialog::AnyFile);
        fd.setAcceptMode(QFileDialog::AcceptSave);
        fd.setWindowTitle(QObject::tr("Save Image"));
        fd.setNameFilters(filter);
        if (!selFilter.isEmpty()) {
            fd.selectNameFilter(selFilter);
        }

        // create the image options widget
        auto opt = new DlgSettingsImageImp(&fd);
        SbVec2s sz = vp.getWindowSize();
        opt->setImageSize((int)sz[0], (int)sz[1]);
        opt->setBackgroundType(backtype);
        opt->setMethod(method);

        fd.setOptionsWidget(FileOptionsDialog::ExtensionRight, opt);
        fd.setOption(QFileDialog::DontConfirmOverwrite, false);
        opt->onSelectedFilter(fd.selectedNameFilter());
        QObject::connect(
            &fd,
            &FileOptionsDialog::filterSelected,
            opt,
            &DlgSettingsImageImp::onSelectedFilter
        );

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
            QString format = formats.front();  // take the first as default
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
                case 0:
                    background = "Current";
                    break;
                case 1:
                    background = "White";
                    break;
                case 2:
                    background = "Black";
                    break;
                case 3:
                    background = "Transparent";
                    break;
                default:
                    background = "Current";
                    break;
            }
            hExt->SetInt("OffscreenImageBackground", opt->backgroundType());

            QString comment = opt->comment();
            if (!comment.isEmpty()) {
                // Replace newline escape sequence through '\\n' string to build one big string,
                // otherwise Python would interpret it as an invalid command.
                // Python does the decoding for us.
                QStringList lines = comment.split(QLatin1String("\n"), Qt::KeepEmptyParts);

                comment = lines.join(QLatin1String("\\n"));
                doCommand(
                    Gui,
                    "Gui.activeDocument().activeView().saveImage('%s',%d,%d,'%s','%s')",
                    fn.toUtf8().constData(),
                    w,
                    h,
                    background,
                    comment.toUtf8().constData()
                );
            }
            else {
                doCommand(
                    Gui,
                    "Gui.activeDocument().activeView().saveImage('%s',%d,%d,'%s')",
                    fn.toUtf8().constData(),
                    w,
                    h,
                    background
                );
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
    sGroup = "Standard-View";
    sMenuText = QT_TR_NOOP("&Load Image…");
    sToolTipText = QT_TR_NOOP("Loads an image");
    sWhatsThis = "Std_ViewLoadImage";
    sStatusTip = sToolTipText;
    sPixmap = "image-open";
    eType = 0;
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
    dialog.setWindowTitle(QObject::tr("Choose an Image File to Open"));
    dialog.setMimeTypeFilters(mimeTypeFilters);
    dialog.selectMimeTypeFilter(QStringLiteral("image/png"));
    dialog.setDefaultSuffix(QStringLiteral("png"));
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
    sGroup = "Standard-View";
    sMenuText = QT_TR_NOOP("New 3D View");
    sToolTipText = QT_TR_NOOP("Opens a new 3D view window for the active document");
    sWhatsThis = "Std_ViewCreate";
    sStatusTip = sToolTipText;
    sPixmap = "window-new";
    eType = Alter3DView;
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
    sGroup = "Standard-View";
    sMenuText = QT_TR_NOOP("Toggle Navigation/&Edit Mode");
    sToolTipText = QT_TR_NOOP("Toggles between navigation and edit mode");
    sStatusTip = sToolTipText;
    sWhatsThis = "Std_ToggleNavigation";
    // iAccel        = Qt::SHIFT+Qt::Key_Space;
    sAccel = "Esc";
    sPixmap = "Std_ToggleNavigation";
    eType = Alter3DView;
}

void StdCmdToggleNavigation::activated(int iMsg)
{
    Q_UNUSED(iMsg);
    Gui::MDIView* view = Gui::getMainWindow()->activeWindow();
    if (view && view->isDerivedFrom<Gui::View3DInventor>()) {
        Gui::View3DInventorViewer* viewer = static_cast<Gui::View3DInventor*>(view)->getViewer();
        SbBool toggle = viewer->isRedirectedToSceneGraph();
        viewer->setRedirectToSceneGraph(!toggle);
    }
}

bool StdCmdToggleNavigation::isActive()
{
    // #0001087: Inventor Navigation continues with released Mouse Button
    // This happens because 'Esc' is also used to close the task dialog.
    // Add also new method 'isRedirectToSceneGraphEnabled' to explicitly
    // check if this is allowed.
    if (Gui::Control().activeDialog()) {
        return false;
    }
    Gui::MDIView* view = Gui::getMainWindow()->activeWindow();
    if (view && view->isDerivedFrom<Gui::View3DInventor>()) {
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
    sGroup = "Standard-View";
    sMenuText = QT_TR_NOOP("Toggle A&xis Cross");
    sToolTipText = QT_TR_NOOP("Toggles the axis cross at the origin");
    sStatusTip = sToolTipText;
    sWhatsThis = "Std_AxisCross";
    sPixmap = "Std_AxisCross";
    sAccel = "A,C";
}

void StdCmdAxisCross::activated(int iMsg)
{
    Q_UNUSED(iMsg);
    auto view = qobject_cast<View3DInventor*>(Gui::getMainWindow()->activeWindow());
    if (view) {
        if (!view->getViewer()->hasAxisCross()) {
            doCommand(Command::Gui, "Gui.ActiveDocument.ActiveView.setAxisCross(True)");
        }
        else {
            doCommand(Command::Gui, "Gui.ActiveDocument.ActiveView.setAxisCross(False)");
        }
    }
}

bool StdCmdAxisCross::isActive()
{
    auto view = qobject_cast<View3DInventor*>(Gui::getMainWindow()->activeWindow());
    if (view && view->getViewer()->hasAxisCross()) {
        if (!_pcAction->isChecked()) {
            _pcAction->setChecked(true);
        }
    }
    else {
        if (_pcAction->isChecked()) {
            _pcAction->setChecked(false);
        }
    }
    if (view) {
        return true;
    }
    return false;
}

//===========================================================================
// Std_ViewExample1
//===========================================================================
DEF_STD_CMD_A(StdCmdViewExample1)

StdCmdViewExample1::StdCmdViewExample1()
    : Command("Std_ViewExample1")
{
    sGroup = "Standard-View";
    sMenuText = QT_TR_NOOP("Inventor Example #1");
    sToolTipText = QT_TR_NOOP("Shows a 3D texture with manipulator");
    sWhatsThis = "Std_ViewExample1";
    sStatusTip = sToolTipText;
    sPixmap = "Std_Tool1";
    eType = Alter3DView;
}

void StdCmdViewExample1::activated(int iMsg)
{
    Q_UNUSED(iMsg);
    doCommand(Command::Gui, "Gui.SendMsgToActiveView(\"Example1\")");
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
    sGroup = "Standard-View";
    sMenuText = QT_TR_NOOP("Inventor Example #2");
    sToolTipText = QT_TR_NOOP("Shows spheres and drag-lights");
    sWhatsThis = "Std_ViewExample2";
    sStatusTip = sToolTipText;
    sPixmap = "Std_Tool2";
    eType = Alter3DView;
}

void StdCmdViewExample2::activated(int iMsg)
{
    Q_UNUSED(iMsg);
    doCommand(Command::Gui, "Gui.SendMsgToActiveView(\"Example2\")");
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
    sGroup = "Standard-View";
    sMenuText = QT_TR_NOOP("Inventor Example #3");
    sToolTipText = QT_TR_NOOP("Shows an animated texture");
    sWhatsThis = "Std_ViewExample3";
    sStatusTip = sToolTipText;
    sPixmap = "Std_Tool3";
    eType = Alter3DView;
}

void StdCmdViewExample3::activated(int iMsg)
{
    Q_UNUSED(iMsg);
    doCommand(Command::Gui, "Gui.SendMsgToActiveView(\"Example3\")");
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
    sGroup = "Standard-View";
    sMenuText = QT_TR_NOOP("Stereo &Off");
    sToolTipText = QT_TR_NOOP("Switches stereo viewing off");
    sWhatsThis = "Std_ViewIvStereoOff";
    sStatusTip = sToolTipText;
    sPixmap = "Std_ViewIvStereoOff";
    eType = Alter3DView;
}

void StdCmdViewIvStereoOff::activated(int iMsg)
{
    Q_UNUSED(iMsg);
    doCommand(Command::Gui, "Gui.activeDocument().activeView().setStereoType(\"Mono\")");
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
    sGroup = "Standard-View";
    sMenuText = QT_TR_NOOP("Stereo Re&d/Cyan");
    sToolTipText = QT_TR_NOOP("Switches stereo viewing to red/cyan");
    sWhatsThis = "Std_ViewIvStereoRedGreen";
    sStatusTip = sToolTipText;
    sPixmap = "Std_ViewIvStereoRedGreen";
    eType = Alter3DView;
}

void StdCmdViewIvStereoRedGreen::activated(int iMsg)
{
    Q_UNUSED(iMsg);
    doCommand(Command::Gui, "Gui.activeDocument().activeView().setStereoType(\"Anaglyph\")");
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
    sGroup = "Standard-View";
    sMenuText = QT_TR_NOOP("Stereo &Quad Buffer");
    sToolTipText = QT_TR_NOOP("Switches stereo viewing to quad buffer");
    sWhatsThis = "Std_ViewIvStereoQuadBuff";
    sStatusTip = sToolTipText;
    sPixmap = "Std_ViewIvStereoQuadBuff";
    eType = Alter3DView;
}

void StdCmdViewIvStereoQuadBuff::activated(int iMsg)
{
    Q_UNUSED(iMsg);
    doCommand(Command::Gui, "Gui.activeDocument().activeView().setStereoType(\"QuadBuffer\")");
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
    sGroup = "Standard-View";
    sMenuText = QT_TR_NOOP("Stereo Interleaved &Rows");
    sToolTipText = QT_TR_NOOP("Switches stereo viewing to interleaved rows");
    sWhatsThis = "Std_ViewIvStereoInterleavedRows";
    sStatusTip = sToolTipText;
    sPixmap = "Std_ViewIvStereoInterleavedRows";
    eType = Alter3DView;
}

void StdCmdViewIvStereoInterleavedRows::activated(int iMsg)
{
    Q_UNUSED(iMsg);
    doCommand(Command::Gui, "Gui.activeDocument().activeView().setStereoType(\"InterleavedRows\")");
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
    sGroup = "Standard-View";
    sMenuText = QT_TR_NOOP("Stereo Interleaved &Columns");
    sToolTipText = QT_TR_NOOP("Switches stereo viewing to interleaved columns");
    sWhatsThis = "Std_ViewIvStereoInterleavedColumns";
    sStatusTip = sToolTipText;
    sPixmap = "Std_ViewIvStereoInterleavedColumns";
    eType = Alter3DView;
}

void StdCmdViewIvStereoInterleavedColumns::activated(int iMsg)
{
    Q_UNUSED(iMsg);
    doCommand(Command::Gui, "Gui.activeDocument().activeView().setStereoType(\"InterleavedColumns\")");
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
    sGroup = "Standard-View";
    sMenuText = QT_TR_NOOP("Issue Camera &Position");
    sToolTipText = QT_TR_NOOP(
        "Issues the camera position to the console and to a macro, to easily recall this position"
    );
    sWhatsThis = "Std_ViewIvIssueCamPos";
    sStatusTip = sToolTipText;
    sPixmap = "Std_ViewIvIssueCamPos";
    eType = Alter3DView;
}

void StdCmdViewIvIssueCamPos::activated(int iMsg)
{
    Q_UNUSED(iMsg);
    std::string Temp, Temp2;
    std::string::size_type pos;

    const char* ppReturn = nullptr;
    getGuiApplication()->sendMsgToActiveView("GetCamera", &ppReturn);

    // remove the #inventor line...
    Temp2 = ppReturn;
    pos = Temp2.find_first_of("\n");
    Temp2.erase(0, pos);

    // remove all returns
    while ((pos = Temp2.find('\n')) != std::string::npos) {
        Temp2.replace(pos, 1, " ");
    }

    // build up the command string
    Temp += "Gui.SendMsgToActiveView(\"SetCamera ";
    Temp += Temp2;
    Temp += "\")";

    Base::Console().message("%s\n", Temp2.c_str());
    getGuiApplication()->macroManager()->addLine(MacroManager::Gui, Temp.c_str());
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
    sGroup = "Standard-View";
    sMenuText = QT_TR_NOOP("Zoom &In");
    sToolTipText = QT_TR_NOOP("Increases the zoom factor by a fixed amount");
    sWhatsThis = "Std_ViewZoomIn";
    sStatusTip = sToolTipText;
    sPixmap = "zoom-in";
    sAccel = keySequenceToAccel(QKeySequence::ZoomIn);
    eType = Alter3DView;
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
    sGroup = "Standard-View";
    sMenuText = QT_TR_NOOP("Zoom &Out");
    sToolTipText = QT_TR_NOOP("Decreases the zoom factor by a fixed amount");
    sWhatsThis = "Std_ViewZoomOut";
    sStatusTip = sToolTipText;
    sPixmap = "zoom-out";
    sAccel = keySequenceToAccel(QKeySequence::ZoomOut);
    eType = Alter3DView;
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

namespace
{
class SelectionCallbackHandler
{

private:
    static std::unique_ptr<SelectionCallbackHandler> currentSelectionHandler;
    QCursor prevSelectionCursor;
    using FnCb = void (*)(void* userdata, SoEventCallback* node);
    FnCb fnCb;
    void* userData;
    bool prevSelectionEn;

public:
    // Creates a selection handler used to implement the common behaviour of BoxZoom, BoxSelection
    // and BoxElementSelection. Takes the viewer, a selection mode, a cursor, a function pointer to
    // be called on success and a void pointer for user data to be passed to the given function. The
    // selection handler class stores all necessary previous states, registers a event callback and
    // starts the selection in the given mode. If there is still a selection handler active, this
    // call will generate a message and returns.
    static void Create(
        View3DInventorViewer* viewer,
        View3DInventorViewer::SelectionMode selectionMode,
        const QCursor& cursor,
        FnCb doFunction = nullptr,
        void* ud = nullptr
    )
    {
        if (currentSelectionHandler) {
            Base::Console().message("SelectionCallbackHandler: A selection handler already active.");
            return;
        }

        currentSelectionHandler = std::make_unique<SelectionCallbackHandler>();
        if (viewer) {
            currentSelectionHandler->userData = ud;
            currentSelectionHandler->fnCb = doFunction;
            currentSelectionHandler->prevSelectionCursor = viewer->cursor();
            viewer->setEditingCursor(cursor);
            viewer->addEventCallback(
                SoEvent::getClassTypeId(),
                SelectionCallbackHandler::selectionCallback,
                currentSelectionHandler.get()
            );
            currentSelectionHandler->prevSelectionEn = viewer->isSelectionEnabled();
            viewer->setSelectionEnabled(false);
            viewer->startSelection(selectionMode);
        }
    }

    void* getUserData() const
    {
        return userData;
    }

    // Implements the event handler. In the normal case the provided function is called.
    // Also supports aborting the selection mode by pressing (releasing) the Escape key.
    static void selectionCallback(void* ud, SoEventCallback* n)
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

            // Mark all incoming mouse button events as handled, especially, to deactivate the
            // selection node
            n->getAction()->setHandled();

            if (mbe->getButton() == SoMouseButtonEvent::BUTTON1
                && mbe->getState() == SoButtonEvent::UP) {
                if (selectionHandler && selectionHandler->fnCb) {
                    selectionHandler->fnCb(selectionHandler->getUserData(), n);
                }
                restoreState(selectionHandler, view);
            }
            // No other mouse events available from Coin3D to implement right mouse up abort
        }
    }

    static void restoreState(SelectionCallbackHandler* selectionHandler, View3DInventorViewer* view)
    {
        if (selectionHandler) {
            selectionHandler->fnCb = nullptr;
            view->setEditingCursor(selectionHandler->prevSelectionCursor);
            view->removeEventCallback(
                SoEvent::getClassTypeId(),
                SelectionCallbackHandler::selectionCallback,
                selectionHandler
            );
            view->setSelectionEnabled(selectionHandler->prevSelectionEn);
        }
        Application::Instance->commandManager().testActive();
        currentSelectionHandler = nullptr;
    }

    static QCursor makeCursor(
        [[maybe_unused]] QWidget* widget,
        const QSize& size,
        const char* svgFile,
        int hotX,
        int hotY
    )
    {
        qreal hotXF = hotX;
        qreal hotYF = hotY;
#if !defined(Q_OS_WIN32) && !defined(Q_OS_MACOS)
        if (qApp->platformName() == QLatin1String("xcb")) {
            qreal pRatio = widget->devicePixelRatioF();
            hotXF *= pRatio;
            hotYF *= pRatio;
        }
#endif
        QPixmap px(Gui::BitmapFactory().pixmapFromSvg(svgFile, size));
        return QCursor(px, static_cast<int>(hotXF), static_cast<int>(hotYF));
    }
};
}  // namespace

std::unique_ptr<SelectionCallbackHandler> SelectionCallbackHandler::currentSelectionHandler
    = std::unique_ptr<SelectionCallbackHandler>();
//===========================================================================
// Std_ViewBoxZoom
//===========================================================================

DEF_3DV_CMD(StdViewBoxZoom)

StdViewBoxZoom::StdViewBoxZoom()
    : Command("Std_ViewBoxZoom")
{
    sGroup = "Standard-View";
    sMenuText = QT_TR_NOOP("&Box Zoom");
    sToolTipText = QT_TR_NOOP("Activates the box zoom tool");
    sWhatsThis = "Std_ViewBoxZoom";
    sStatusTip = sToolTipText;
    sPixmap = "zoom-border";
    sAccel = "Ctrl+B";
    eType = Alter3DView;
}

void StdViewBoxZoom::activated(int iMsg)
{
    Q_UNUSED(iMsg);
    auto view = qobject_cast<View3DInventor*>(getMainWindow()->activeWindow());
    if (view) {
        View3DInventorViewer* viewer = view->getViewer();
        if (!viewer->isSelecting()) {
            // NOLINTBEGIN
            QCursor cursor
                = SelectionCallbackHandler::makeCursor(viewer, QSize(32, 32), "zoom-border-cross", 6, 6);
            SelectionCallbackHandler::Create(viewer, View3DInventorViewer::BoxZoom, cursor);
            // NOLINTEND
        }
    }
}

//===========================================================================
// Std_BoxSelection
//===========================================================================
DEF_3DV_CMD(StdBoxSelection)

StdBoxSelection::StdBoxSelection()
    : Command("Std_BoxSelection")
{
    sGroup = "Standard-View";
    sMenuText = QT_TR_NOOP("&Box Selection");
    sToolTipText = QT_TR_NOOP("Activates the box selection tool");
    sWhatsThis = "Std_BoxSelection";
    sStatusTip = sToolTipText;
    sPixmap = "edit-select-box";
    sAccel = "Shift+B";
    eType = AlterSelection;
}

using SelectionMode = enum
{
    CENTER,
    INTERSECT
};

static std::vector<std::string> getBoxSelection(
    ViewProviderDocumentObject* vp,
    SelectionMode mode,
    bool selectElement,
    const Base::ViewProjMethod& proj,
    const Base::Polygon2d& polygon,
    const Base::Matrix4D& mat,
    bool transform = true,
    int depth = 0
)
{
    std::vector<std::string> ret;
    auto obj = vp->getObject();
    if (!obj || !obj->isAttachedToDocument()) {
        return ret;
    }

    // DO NOT check this view object Visibility, let the caller do this. Because
    // we may be called by upper object hierarchy that manages our visibility.

    auto bbox3 = vp->getBoundingBox(nullptr, transform);
    if (!bbox3.IsValid()) {
        return ret;
    }

    auto bbox = bbox3.Transformed(mat).ProjectBox(&proj);

    // check if both two boundary points are inside polygon, only
    // valid since we know the given polygon is a box.
    if (polygon.Contains(Base::Vector2d(bbox.MinX, bbox.MinY))
        && polygon.Contains(Base::Vector2d(bbox.MaxX, bbox.MaxY))) {
        ret.emplace_back("");
        return ret;
    }

    if (!bbox.Intersect(polygon)) {
        return ret;
    }

    const auto& subs = obj->getSubObjects(App::DocumentObject::GS_SELECT);
    if (subs.empty()) {
        if (!selectElement) {
            if (mode == INTERSECT || polygon.Contains(bbox.GetCenter())) {
                ret.emplace_back("");
            }
            return ret;
        }
        Base::PyGILStateLocker lock;
        PyObject* pyobj = nullptr;
        Base::Matrix4D matCopy(mat);
        obj->getSubObject(nullptr, &pyobj, &matCopy, transform, depth);
        if (!pyobj) {
            return ret;
        }
        Py::Object pyobject(pyobj, true);
        if (!PyObject_TypeCheck(pyobj, &Data::ComplexGeoDataPy::Type)) {
            return ret;
        }
        auto data = static_cast<Data::ComplexGeoDataPy*>(pyobj)->getComplexGeoDataPtr();
        for (auto type : data->getElementTypes()) {
            size_t count = data->countSubElements(type);
            if (!count) {
                continue;
            }
            for (size_t i = 1; i <= count; ++i) {
                std::string element(type);
                element += std::to_string(i);
                std::unique_ptr<Data::Segment> segment(data->getSubElementByName(element.c_str()));
                if (!segment) {
                    continue;
                }
                std::vector<Base::Vector3d> points;
                std::vector<Data::ComplexGeoData::Line> lines;
                data->getLinesFromSubElement(segment.get(), points, lines);
                if (lines.empty()) {
                    if (points.empty()) {
                        continue;
                    }
                    auto v = proj(points[0]);
                    if (polygon.Contains(Base::Vector2d(v.x, v.y))) {
                        ret.push_back(element);
                    }
                    continue;
                }
                Base::Polygon2d loop;
                // TODO: can we assume the line returned above are in proper
                // order if the element is a face?
                auto v = proj(points[lines.front().I1]);
                loop.Add(Base::Vector2d(v.x, v.y));
                for (auto& line : lines) {
                    for (auto i = line.I1; i < line.I2; ++i) {
                        auto v = proj(points[i + 1]);
                        loop.Add(Base::Vector2d(v.x, v.y));
                    }
                }
                if (!polygon.Intersect(loop)) {
                    continue;
                }
                if (mode == CENTER && !polygon.Contains(loop.CalcBoundBox().GetCenter())) {
                    continue;
                }
                ret.push_back(element);
            }
            break;
        }
        return ret;
    }

    size_t count = 0;
    for (auto& sub : subs) {
        App::DocumentObject* parent = nullptr;
        std::string childName;
        Base::Matrix4D smat(mat);
        auto sobj
            = obj->resolve(sub.c_str(), &parent, &childName, nullptr, nullptr, &smat, transform, depth + 1);
        if (!sobj) {
            continue;
        }
        int vis;
        if (!parent || (vis = parent->isElementVisible(childName.c_str())) < 0) {
            vis = sobj->Visibility.getValue() ? 1 : 0;
        }

        if (!vis) {
            continue;
        }

        auto svp = freecad_cast<ViewProviderDocumentObject*>(
            Application::Instance->getViewProvider(sobj)
        );
        if (!svp) {
            continue;
        }

        const auto& sels
            = getBoxSelection(svp, mode, selectElement, proj, polygon, smat, false, depth + 1);
        if (sels.size() == 1 && sels[0].empty()) {
            ++count;
        }
        for (auto& sel : sels) {
            ret.emplace_back(sub + sel);
        }
    }
    if (count == subs.size()) {
        ret.resize(1);
        ret[0].clear();
    }
    return ret;
}

static void doSelect(void* ud, SoEventCallback* cb)
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
        if (picked[0][0] > picked[1][0]) {
            selectionMode = INTERSECT;
        }
    }
    else {
        for (const auto& it : picked) {
            polygon.Add(Base::Vector2d(it[0], it[1]));
        }
    }

    App::Document* doc = App::GetApplication().getActiveDocument();
    if (doc) {
        cb->setHandled();

        const SoEvent* ev = cb->getEvent();
        if (ev && !ev->wasCtrlDown()) {
            Gui::Selection().clearSelection(doc->getName());
        }

        const std::vector<App::DocumentObject*> objects = doc->getObjects();
        for (auto obj : objects) {
            if (App::GeoFeatureGroupExtension::getGroupOfObject(obj)) {
                continue;
            }

            auto vp = freecad_cast<ViewProviderDocumentObject*>(
                Application::Instance->getViewProvider(obj)
            );
            if (!vp || !vp->isVisible()) {
                continue;
            }

            Base::Matrix4D mat;
            for (auto& sub : getBoxSelection(vp, selectionMode, selectElement, proj, polygon, mat)) {
                Gui::Selection().addSelection(doc->getName(), obj->getNameInDocument(), sub.c_str());
            }
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

            // NOLINTBEGIN
            QCursor cursor = SelectionCallbackHandler::makeCursor(
                viewer,
                QSize(32, 32),
                "edit-select-box-cross",
                6,
                6
            );
            SelectionCallbackHandler::Create(
                viewer,
                View3DInventorViewer::Rubberband,
                cursor,
                doSelect,
                nullptr
            );
            viewer->setSelectionEnabled(false);
            // NOLINTEND
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
    sGroup = "Standard-View";
    sMenuText = QT_TR_NOOP("Bo&x Element Selection");
    sToolTipText = QT_TR_NOOP("Activates box element selection");
    sWhatsThis = "Std_BoxElementSelection";
    sStatusTip = sToolTipText;
    sPixmap = "edit-element-select-box";
    sAccel = "Shift+E";
    eType = AlterSelection;
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

            // NOLINTBEGIN
            QCursor cursor = SelectionCallbackHandler::makeCursor(
                viewer,
                QSize(32, 32),
                "edit-element-select-box-cross",
                6,
                6
            );
            SelectionCallbackHandler::Create(
                viewer,
                View3DInventorViewer::Rubberband,
                cursor,
                doSelect,
                this
            );
            viewer->setSelectionEnabled(false);
            // NOLINTEND
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
    sGroup = "TreeView";
    sMenuText = QT_TR_NOOP("&Go to Selection");
    sToolTipText = QT_TR_NOOP("Scrolls to the first selected item");
    sWhatsThis = "Std_TreeSelection";
    sStatusTip = sToolTipText;
    eType = Alter3DView;
    sPixmap = "tree-goto-sel";
    sAccel = "T,G";
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
    sGroup = "View";
    sMenuText = QT_TR_NOOP("Collapse Selected Items");
    sToolTipText = QT_TR_NOOP("Collapses the currently selected tree items");
    sWhatsThis = "Std_TreeCollapse";
    sStatusTip = sToolTipText;
    eType = Alter3DView;
}

void StdCmdTreeCollapse::activated(int iMsg)
{
    Q_UNUSED(iMsg);
    QList<TreeWidget*> tree = Gui::getMainWindow()->findChildren<TreeWidget*>();
    for (QList<TreeWidget*>::iterator it = tree.begin(); it != tree.end(); ++it) {
        (*it)->expandSelectedItems(TreeItemMode::CollapseItem);
    }
}

//===========================================================================
// Std_TreeExpand
//===========================================================================

DEF_STD_CMD(StdCmdTreeExpand)

StdCmdTreeExpand::StdCmdTreeExpand()
    : Command("Std_TreeExpand")
{
    sGroup = "View";
    sMenuText = QT_TR_NOOP("Expand Selected Items");
    sToolTipText = QT_TR_NOOP("Expands the currently selected tree items");
    sWhatsThis = "Std_TreeExpand";
    sStatusTip = sToolTipText;
    eType = Alter3DView;
}

void StdCmdTreeExpand::activated(int iMsg)
{
    Q_UNUSED(iMsg);
    QList<TreeWidget*> tree = Gui::getMainWindow()->findChildren<TreeWidget*>();
    for (QList<TreeWidget*>::iterator it = tree.begin(); it != tree.end(); ++it) {
        (*it)->expandSelectedItems(TreeItemMode::ExpandItem);
    }
}

//===========================================================================
// Std_TreeSelectAllInstance
//===========================================================================

DEF_STD_CMD_A(StdCmdTreeSelectAllInstances)

StdCmdTreeSelectAllInstances::StdCmdTreeSelectAllInstances()
    : Command("Std_TreeSelectAllInstances")
{
    sGroup = "View";
    sMenuText = QT_TR_NOOP("Select All Instances");
    sToolTipText = QT_TR_NOOP("Selects all instances of the currently selected object");
    sWhatsThis = "Std_TreeSelectAllInstances";
    sStatusTip = sToolTipText;
    sPixmap = "sel-instance";
    eType = AlterSelection;
}

bool StdCmdTreeSelectAllInstances::isActive()
{
    const auto& sels = Selection().getSelectionEx(
        "*",
        App::DocumentObject::getClassTypeId(),
        ResolveMode::OldStyleElement,
        true
    );
    if (sels.empty()) {
        return false;
    }
    auto obj = sels[0].getObject();
    if (!obj || !obj->isAttachedToDocument()) {
        return false;
    }
    return freecad_cast<ViewProviderDocumentObject*>(Application::Instance->getViewProvider(obj))
        != nullptr;
}

void StdCmdTreeSelectAllInstances::activated(int iMsg)
{
    Q_UNUSED(iMsg);
    const auto& sels = Selection().getSelectionEx(
        "*",
        App::DocumentObject::getClassTypeId(),
        ResolveMode::OldStyleElement,
        true
    );
    if (sels.empty()) {
        return;
    }
    auto obj = sels[0].getObject();
    if (!obj || !obj->isAttachedToDocument()) {
        return;
    }
    auto vpd = freecad_cast<ViewProviderDocumentObject*>(Application::Instance->getViewProvider(obj));
    if (!vpd) {
        return;
    }
    Selection().selStackPush();
    Selection().clearCompleteSelection();
    const auto trees = getMainWindow()->findChildren<TreeWidget*>();
    for (auto tree : trees) {
        tree->selectAllInstances(*vpd);
    }
    Selection().selStackPush();
}


//===========================================================================
// Std_SceneInspector
//===========================================================================

DEF_3DV_CMD(StdCmdSceneInspector)

StdCmdSceneInspector::StdCmdSceneInspector()
    : Command("Std_SceneInspector")
{
    // setting the
    sGroup = "Tools";
    sMenuText = QT_TR_NOOP("Scene I&nspector");
    sToolTipText = QT_TR_NOOP("Opens the scene inspector");
    sWhatsThis = "Std_SceneInspector";
    sStatusTip = sToolTipText;
    eType = Alter3DView;
    sPixmap = "Std_SceneInspector";
}

void StdCmdSceneInspector::activated(int iMsg)
{
    Q_UNUSED(iMsg);
    Gui::Document* doc = Application::Instance->activeDocument();
    if (doc) {
        static QPointer<Gui::Dialog::DlgInspector> dlg = nullptr;
        if (!dlg) {
            dlg = new Gui::Dialog::DlgInspector(getMainWindow());
        }
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
    sGroup = "Tools";
    sMenuText = QT_TR_NOOP("Text&ure Mapping");
    sToolTipText = QT_TR_NOOP("Maps textures to shapes");
    sWhatsThis = "Std_TextureMapping";
    sStatusTip = sToolTipText;
    sPixmap = "Std_TextureMapping";
    eType = Alter3DView;
}

void StdCmdTextureMapping::activated(int iMsg)
{
    Q_UNUSED(iMsg);
    Gui::Control().showDialog(new Gui::Dialog::TaskTextureMapping);
}

bool StdCmdTextureMapping::isActive()
{
    Gui::MDIView* view = getMainWindow()->activeWindow();
    return view && view->isDerivedFrom<Gui::View3DInventor>() && (!(Gui::Control().activeDialog()));
}

DEF_STD_CMD(StdCmdDemoMode)

StdCmdDemoMode::StdCmdDemoMode()
    : Command("Std_DemoMode")
{
    sGroup = "Standard-View";
    sMenuText = QT_TR_NOOP("View &Turntable");
    sToolTipText = QT_TR_NOOP("Opens a turntable view");
    sWhatsThis = "Std_DemoMode";
    sStatusTip = sToolTipText;
    eType = Alter3DView;
    sPixmap = "Std_DemoMode";
}

void StdCmdDemoMode::activated(int iMsg)
{
    Q_UNUSED(iMsg);
    static QPointer<QDialog> dlg = nullptr;
    if (!dlg) {
        dlg = new Gui::Dialog::DemoMode(getMainWindow());
    }
    dlg->setAttribute(Qt::WA_DeleteOnClose);
    dlg->show();
}


//===========================================================================
// Std_SelBack
//===========================================================================

DEF_STD_CMD_A(StdCmdSelBack)

StdCmdSelBack::StdCmdSelBack()
    : Command("Std_SelBack")
{
    sGroup = "View";
    sMenuText = QT_TR_NOOP("Selection &Back");
    static std::string toolTip = std::string("<p>")
        + QT_TR_NOOP("Restores the previous tree view selection. "
                     "Only works if tree RecordSelection mode is switched on.")
        + "</p>";
    sToolTipText = toolTip.c_str();
    sWhatsThis = "Std_SelBack";
    sStatusTip = sToolTipText;
    sPixmap = "sel-back";
    sAccel = "S, B";
    eType = AlterSelection;
}

void StdCmdSelBack::activated(int iMsg)
{
    Q_UNUSED(iMsg);
    Selection().selStackGoBack();
}

bool StdCmdSelBack::isActive()
{
    return Selection().selStackBackSize() > 1;
}

//===========================================================================
// Std_SelForward
//===========================================================================

DEF_STD_CMD_A(StdCmdSelForward)

StdCmdSelForward::StdCmdSelForward()
    : Command("Std_SelForward")
{
    sGroup = "View";
    sMenuText = QT_TR_NOOP("Selection &Forward");
    static std::string toolTip = std::string("<p>")
        + QT_TR_NOOP("Restores the next tree view selection. "
                     "Only works if tree RecordSelection mode is switched on.")
        + "</p>";
    sToolTipText = toolTip.c_str();
    sWhatsThis = "Std_SelForward";
    sStatusTip = sToolTipText;
    sPixmap = "sel-forward";
    sAccel = "S, F";
    eType = AlterSelection;
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
#define TREEVIEW_DOC_CMD_DEF(_name, _v) \
    DEF_STD_CMD_AC(StdTree##_name) \
    void StdTree##_name::activated(int) \
    { \
        TreeParams::setDocumentMode(_v); \
        if (_pcAction) \
            _pcAction->setBlockedChecked(true); \
    } \
    Action* StdTree##_name::createAction(void) \
    { \
        Action* pcAction = Command::createAction(); \
        pcAction->setCheckable(true); \
        pcAction->setIcon(QIcon()); \
        _pcAction = pcAction; \
        isActive(); \
        return pcAction; \
    } \
    bool StdTree##_name::isActive() \
    { \
        bool checked = TreeParams::getDocumentMode() == _v; \
        if (_pcAction && _pcAction->isChecked() != checked) \
            _pcAction->setBlockedChecked(checked); \
        return true; \
    }

TREEVIEW_DOC_CMD_DEF(SingleDocument, 0)

StdTreeSingleDocument::StdTreeSingleDocument()
    : Command("Std_TreeSingleDocument")
{
    sGroup = "TreeView";
    sMenuText = QT_TR_NOOP("&Single Document");
    sToolTipText = QT_TR_NOOP("Displays only the active document in the tree view");
    sWhatsThis = "Std_TreeSingleDocument";
    sStatusTip = sToolTipText;
    sPixmap = "tree-doc-single";
    eType = 0;
}

//===========================================================================
// Std_TreeMultiDocument
//===========================================================================
TREEVIEW_DOC_CMD_DEF(MultiDocument, 1)

StdTreeMultiDocument::StdTreeMultiDocument()
    : Command("Std_TreeMultiDocument")
{
    sGroup = "TreeView";
    sMenuText = QT_TR_NOOP("&Multi Document");
    sToolTipText = QT_TR_NOOP("Displays all documents in the tree view");
    sWhatsThis = "Std_TreeMultiDocument";
    sStatusTip = sToolTipText;
    sPixmap = "tree-doc-multi";
    eType = 0;
}

//===========================================================================
// Std_TreeCollapseDocument
//===========================================================================
TREEVIEW_DOC_CMD_DEF(CollapseDocument, 2)

StdTreeCollapseDocument::StdTreeCollapseDocument()
    : Command("Std_TreeCollapseDocument")
{
    sGroup = "TreeView";
    sMenuText = QT_TR_NOOP("Collapse/E&xpand");
    sToolTipText = QT_TR_NOOP("Expands the active document and collapses all others");
    sWhatsThis = "Std_TreeCollapseDocument";
    sStatusTip = sToolTipText;
    sPixmap = "tree-doc-collapse";
    eType = 0;
}

//===========================================================================
// Std_TreeSyncView
//===========================================================================
#define TREEVIEW_CMD_DEF(_name) \
    DEF_STD_CMD_AC(StdTree##_name) \
    void StdTree##_name::activated(int) \
    { \
        auto checked = !TreeParams::get##_name(); \
        TreeParams::set##_name(checked); \
        if (_pcAction) \
            _pcAction->setBlockedChecked(checked); \
    } \
    Action* StdTree##_name::createAction() \
    { \
        Action* pcAction = Command::createAction(); \
        pcAction->setCheckable(true); \
        pcAction->setIcon(QIcon()); \
        _pcAction = pcAction; \
        isActive(); \
        return pcAction; \
    } \
    bool StdTree##_name::isActive() \
    { \
        bool checked = TreeParams::get##_name(); \
        if (_pcAction && _pcAction->isChecked() != checked) \
            _pcAction->setBlockedChecked(checked); \
        return true; \
    }

TREEVIEW_CMD_DEF(SyncView)

StdTreeSyncView::StdTreeSyncView()
    : Command("Std_TreeSyncView")
{
    sGroup = "TreeView";
    sMenuText = QT_TR_NOOP("&1 Sync View");
    sToolTipText = QT_TR_NOOP(
        "Switches to the 3D view containing the selected item from the tree view"
    );
    sStatusTip = sToolTipText;
    sWhatsThis = "Std_TreeSyncView";
    sPixmap = "tree-sync-view";
    sAccel = "T,1";
    eType = 0;
}

//===========================================================================
// Std_TreeSyncSelection
//===========================================================================
TREEVIEW_CMD_DEF(SyncSelection)

StdTreeSyncSelection::StdTreeSyncSelection()
    : Command("Std_TreeSyncSelection")
{
    sGroup = "TreeView";
    sMenuText = QT_TR_NOOP("&2 Sync Selection");
    sToolTipText = QT_TR_NOOP(
        "Expands the tree item when the corresponding object is selected in the 3D view"
    );
    sStatusTip = sToolTipText;
    sWhatsThis = "Std_TreeSyncSelection";
    sPixmap = "tree-sync-sel";
    sAccel = "T,2";
    eType = 0;
}

//===========================================================================
// Std_TreeSyncPlacement
//===========================================================================
TREEVIEW_CMD_DEF(SyncPlacement)

StdTreeSyncPlacement::StdTreeSyncPlacement()
    : Command("Std_TreeSyncPlacement")
{
    sGroup = "TreeView";
    sMenuText = QT_TR_NOOP("&3 Sync Placement");
    sToolTipText
        = QT_TR_NOOP("Adjusts the placement on drag-and-drop of objects across coordinate systems (e.g. in part containers)");
    sStatusTip = sToolTipText;
    sWhatsThis = "Std_TreeSyncPlacement";
    sPixmap = "tree-sync-pla";
    sAccel = "T,3";
    eType = 0;
}

//===========================================================================
// Std_TreePreSelection
//===========================================================================
TREEVIEW_CMD_DEF(PreSelection)

StdTreePreSelection::StdTreePreSelection()
    : Command("Std_TreePreSelection")
{
    sGroup = "TreeView";
    sMenuText = QT_TR_NOOP("&4 Preselection");
    sToolTipText = QT_TR_NOOP(
        "Preselects the object in 3D view when hovering the cursor over the tree item"
    );
    sStatusTip = sToolTipText;
    sWhatsThis = "Std_TreePreSelection";
    sPixmap = "tree-pre-sel";
    sAccel = "T,4";
    eType = 0;
}

//===========================================================================
// Std_TreeRecordSelection
//===========================================================================
TREEVIEW_CMD_DEF(RecordSelection)

StdTreeRecordSelection::StdTreeRecordSelection()
    : Command("Std_TreeRecordSelection")
{
    sGroup = "TreeView";
    sMenuText = QT_TR_NOOP("&5 Record Selection");
    sToolTipText
        = QT_TR_NOOP("Records the selection in the tree view in order to go back/forward using the navigation buttons");
    sStatusTip = sToolTipText;
    sWhatsThis = "Std_TreeRecordSelection";
    sPixmap = "tree-rec-sel";
    sAccel = "T,5";
    eType = 0;
}

//===========================================================================
// Std_TreeDrag
//===========================================================================
DEF_STD_CMD(StdTreeDrag)

StdTreeDrag::StdTreeDrag()
    : Command("Std_TreeDrag")
{
    sGroup = "TreeView";
    sMenuText = QT_TR_NOOP("Initiate &Dragging");
    sToolTipText = QT_TR_NOOP("Initiates dragging of the currently selected tree items");
    sStatusTip = sToolTipText;
    sWhatsThis = "Std_TreeDrag";
    sPixmap = "tree-item-drag";
    sAccel = "T,D";
    eType = 0;
}

void StdTreeDrag::activated(int)
{
    if (Gui::Selection().hasSelection()) {
        const auto trees = getMainWindow()->findChildren<TreeWidget*>();
        for (auto tree : trees) {
            if (tree->isVisible()) {
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
class StdCmdTreeViewActions: public GroupCommand
{
public:
    StdCmdTreeViewActions()
        : GroupCommand("Std_TreeViewActions")
    {
        sGroup = "TreeView";
        sMenuText = QT_TR_NOOP("Tree View Actions");
        sToolTipText = QT_TR_NOOP("Tree view behavior options and actions");
        sWhatsThis = "Std_TreeViewActions";
        sStatusTip = sToolTipText;
        eType = 0;
        bCanLog = false;

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

        addCommand(new StdTreeDrag(), !cmds.empty());
        addCommand(new StdTreeSelection(), !cmds.empty());

        addCommand();

        addCommand(new StdCmdSelBack());
        addCommand(new StdCmdSelForward());
    }
    const char* className() const override
    {
        return "StdCmdTreeViewActions";
    }
};


//======================================================================
// Std_SelBoundingBox
//===========================================================================
DEF_STD_CMD_AC(StdCmdSelBoundingBox)

StdCmdSelBoundingBox::StdCmdSelBoundingBox()
    : Command("Std_SelBoundingBox")
{
    sGroup = "View";
    sMenuText = QT_TR_NOOP("&Bounding Box");
    sToolTipText = QT_TR_NOOP("Shows selection bounding box");
    sWhatsThis = "Std_SelBoundingBox";
    sStatusTip = sToolTipText;
    sPixmap = "sel-bbox";
    eType = Alter3DView;
}

void StdCmdSelBoundingBox::activated(int iMsg)
{
    bool checked = !!iMsg;
    if (checked != ViewParams::instance()->getShowSelectionBoundingBox()) {
        ViewParams::instance()->setShowSelectionBoundingBox(checked);
        if (_pcAction) {
            _pcAction->setBlockedChecked(checked);
        }
    }
}

bool StdCmdSelBoundingBox::isActive()
{
    if (_pcAction) {
        bool checked = _pcAction->isChecked();
        if (checked != ViewParams::instance()->getShowSelectionBoundingBox()) {
            _pcAction->setBlockedChecked(!checked);
        }
    }
    return true;
}

Action* StdCmdSelBoundingBox::createAction()
{
    Action* pcAction = Command::createAction();
    pcAction->setCheckable(true);
    return pcAction;
}

//===========================================================================
// Std_DockOverlayAll
//===========================================================================

DEF_STD_CMD(StdCmdDockOverlayAll)

StdCmdDockOverlayAll::StdCmdDockOverlayAll()
    : Command("Std_DockOverlayAll")
{
    sGroup = "View";
    sMenuText = QT_TR_NOOP("Toggle Overl&ay for All Panels");
    sToolTipText = QT_TR_NOOP("Toggled overlay mode for all docked panels");
    sWhatsThis = "Std_DockOverlayAll";
    sStatusTip = sToolTipText;
    eType = 0;
}

void StdCmdDockOverlayAll::activated(int iMsg)
{
    Q_UNUSED(iMsg);
    OverlayManager::instance()->setOverlayMode(OverlayManager::OverlayMode::ToggleAll);
}

//===========================================================================
// Std_DockOverlayTransparentAll
//===========================================================================

DEF_STD_CMD(StdCmdDockOverlayTransparentAll)

StdCmdDockOverlayTransparentAll::StdCmdDockOverlayTransparentAll()
    : Command("Std_DockOverlayTransparentAll")
{
    sGroup = "View";
    sMenuText = QT_TR_NOOP("Toggle Tra&nsparent Panels");
    sToolTipText = QT_TR_NOOP(
        "Toggles transparent mode for all docked overlay panels.\n"
        "This makes the docked panels stay transparent at all times."
    );
    sWhatsThis = "Std_DockOverlayTransparentAll";
    sStatusTip = sToolTipText;
    eType = 0;
}

void StdCmdDockOverlayTransparentAll::activated(int iMsg)
{
    Q_UNUSED(iMsg);
    OverlayManager::instance()->setOverlayMode(OverlayManager::OverlayMode::ToggleTransparentAll);
}

//===========================================================================
// Std_DockOverlayToggle
//===========================================================================

DEF_STD_CMD(StdCmdDockOverlayToggle)

StdCmdDockOverlayToggle::StdCmdDockOverlayToggle()
    : Command("Std_DockOverlayToggle")
{
    sGroup = "View";
    sMenuText = QT_TR_NOOP("Toggle &Overlay");
    sToolTipText = QT_TR_NOOP("Toggles overlay mode for the docked window under the cursor");
    sWhatsThis = "Std_DockOverlayToggle";
    sStatusTip = sToolTipText;
    eType = 0;
}

void StdCmdDockOverlayToggle::activated(int iMsg)
{
    Q_UNUSED(iMsg);
    OverlayManager::instance()->setOverlayMode(OverlayManager::OverlayMode::ToggleActive);
}

//===========================================================================
// Std_DockOverlayToggleTransparent
//===========================================================================

DEF_STD_CMD(StdCmdDockOverlayToggleTransparent)

StdCmdDockOverlayToggleTransparent::StdCmdDockOverlayToggleTransparent()
    : Command("Std_DockOverlayToggleTransparent")
{
    sGroup = "Standard-View";
    sMenuText = QT_TR_NOOP("Toggle Tran&sparent Mode");
    sToolTipText = QT_TR_NOOP(
        "Toggles transparent mode for the docked panel under cursor.\n"
        "This makes the docked panel stay transparent at all times."
    );
    sWhatsThis = "Std_DockOverlayToggleTransparent";
    sStatusTip = sToolTipText;
    eType = 0;
}

void StdCmdDockOverlayToggleTransparent::activated(int iMsg)
{
    Q_UNUSED(iMsg);
    OverlayManager::instance()->setOverlayMode(OverlayManager::OverlayMode::ToggleTransparent);
}

//===========================================================================
// Std_DockOverlayToggleLeft
//===========================================================================

DEF_STD_CMD(StdCmdDockOverlayToggleLeft)

StdCmdDockOverlayToggleLeft::StdCmdDockOverlayToggleLeft()
    : Command("Std_DockOverlayToggleLeft")
{
    sGroup = "Standard-View";
    sMenuText = QT_TR_NOOP("Toggle &Left");
    sToolTipText = QT_TR_NOOP("Toggles the visibility of the left overlay panel");
    sWhatsThis = "Std_DockOverlayToggleLeft";
    sStatusTip = sToolTipText;
    sAccel = "Ctrl+Left";
    sPixmap = "Std_DockOverlayToggleLeft";
    eType = 0;
}

void StdCmdDockOverlayToggleLeft::activated(int iMsg)
{
    Q_UNUSED(iMsg);
    OverlayManager::instance()->setOverlayMode(OverlayManager::OverlayMode::ToggleLeft);
}

//===========================================================================
// Std_DockOverlayToggleRight
//===========================================================================

DEF_STD_CMD(StdCmdDockOverlayToggleRight)

StdCmdDockOverlayToggleRight::StdCmdDockOverlayToggleRight()
    : Command("Std_DockOverlayToggleRight")
{
    sGroup = "Standard-View";
    sMenuText = QT_TR_NOOP("Toggle &Right");
    sToolTipText = QT_TR_NOOP("Toggles the visibility of the right overlay panel");
    sWhatsThis = "Std_DockOverlayToggleRight";
    sStatusTip = sToolTipText;
    sAccel = "Ctrl+Right";
    sPixmap = "Std_DockOverlayToggleRight";
    eType = 0;
}

void StdCmdDockOverlayToggleRight::activated(int iMsg)
{
    Q_UNUSED(iMsg);
    OverlayManager::instance()->setOverlayMode(OverlayManager::OverlayMode::ToggleRight);
}

//===========================================================================
// Std_DockOverlayToggleTop
//===========================================================================

DEF_STD_CMD(StdCmdDockOverlayToggleTop)

StdCmdDockOverlayToggleTop::StdCmdDockOverlayToggleTop()
    : Command("Std_DockOverlayToggleTop")
{
    sGroup = "Standard-View";
    sMenuText = QT_TR_NOOP("Toggle &Top");
    sToolTipText = QT_TR_NOOP("Toggles the visibility of the top overlay panel");
    sWhatsThis = "Std_DockOverlayToggleTop";
    sStatusTip = sToolTipText;
    sAccel = "Ctrl+Up";
    sPixmap = "Std_DockOverlayToggleTop";
    eType = 0;
}

void StdCmdDockOverlayToggleTop::activated(int iMsg)
{
    Q_UNUSED(iMsg);
    OverlayManager::instance()->setOverlayMode(OverlayManager::OverlayMode::ToggleTop);
}

//===========================================================================
// Std_DockOverlayToggleBottom
//===========================================================================

DEF_STD_CMD(StdCmdDockOverlayToggleBottom)

StdCmdDockOverlayToggleBottom::StdCmdDockOverlayToggleBottom()
    : Command("Std_DockOverlayToggleBottom")
{
    sGroup = "Standard-View";
    sMenuText = QT_TR_NOOP("Toggle &Bottom");
    sToolTipText = QT_TR_NOOP("Toggles the visibility of the bottom overlay panel");
    sWhatsThis = "Std_DockOverlayToggleBottom";
    sStatusTip = sToolTipText;
    sAccel = "Ctrl+Down";
    sPixmap = "Std_DockOverlayToggleBottom";
    eType = 0;
}

void StdCmdDockOverlayToggleBottom::activated(int iMsg)
{
    Q_UNUSED(iMsg);
    OverlayManager::instance()->setOverlayMode(OverlayManager::OverlayMode::ToggleBottom);
}

//===========================================================================
// Std_DockOverlayMouseTransparent
//===========================================================================

DEF_STD_CMD_AC(StdCmdDockOverlayMouseTransparent)

StdCmdDockOverlayMouseTransparent::StdCmdDockOverlayMouseTransparent()
    : Command("Std_DockOverlayMouseTransparent")
{
    sGroup = "View";
    sMenuText = QT_TR_NOOP("Bypass &Mouse Events in Overlay Panels");
    sToolTipText = QT_TR_NOOP("Bypasses all mouse events in docked overlay panels");
    sWhatsThis = "Std_DockOverlayMouseTransparent";
    sStatusTip = sToolTipText;
    sAccel = "T, T";
    eType = NoTransaction;
}

void StdCmdDockOverlayMouseTransparent::activated(int iMsg)
{
    (void)iMsg;
    bool checked = !OverlayManager::instance()->isMouseTransparent();
    OverlayManager::instance()->setMouseTransparent(checked);
    if (_pcAction) {
        _pcAction->setBlockedChecked(checked);
    }
}

Action* StdCmdDockOverlayMouseTransparent::createAction()
{
    Action* pcAction = Command::createAction();
    pcAction->setCheckable(true);
    pcAction->setIcon(QIcon());
    _pcAction = pcAction;
    isActive();
    return pcAction;
}

bool StdCmdDockOverlayMouseTransparent::isActive()
{
    bool checked = OverlayManager::instance()->isMouseTransparent();
    if (_pcAction && _pcAction->isChecked() != checked) {
        _pcAction->setBlockedChecked(checked);
    }
    return true;
}

// ============================================================================

class StdCmdDockOverlay: public GroupCommand
{
public:
    StdCmdDockOverlay()
        : GroupCommand("Std_DockOverlay")
    {
        sGroup = "View";
        sMenuText = QT_TR_NOOP("Overlay Docked Panel");
        sToolTipText = QT_TR_NOOP("Sets the docked panel in overlay mode");
        sWhatsThis = "Std_DockOverlay";
        sStatusTip = sToolTipText;
        eType = 0;
        bCanLog = false;

        addCommand(new StdCmdDockOverlayAll());
        addCommand(new StdCmdDockOverlayTransparentAll());
        addCommand();
        addCommand(new StdCmdDockOverlayToggle());
        addCommand(new StdCmdDockOverlayToggleTransparent());
        addCommand();
        addCommand(new StdCmdDockOverlayMouseTransparent());
        addCommand();
        addCommand(new StdCmdDockOverlayToggleLeft());
        addCommand(new StdCmdDockOverlayToggleRight());
        addCommand(new StdCmdDockOverlayToggleTop());
        addCommand(new StdCmdDockOverlayToggleBottom());
    };
    virtual const char* className() const
    {
        return "StdCmdDockOverlay";
    }
};

//===========================================================================
// Std_StoreWorkingView
//===========================================================================
DEF_STD_CMD_A(StdStoreWorkingView)

StdStoreWorkingView::StdStoreWorkingView()
    : Command("Std_StoreWorkingView")
{
    sGroup = "Standard-View";
    sMenuText = QT_TR_NOOP("St&ore Working View");
    sToolTipText = QT_TR_NOOP("Stores a temporary working view for the current document");
    sStatusTip = sToolTipText;
    sWhatsThis = "Std_StoreWorkingView";
    sAccel = "Shift+End";
    eType = NoTransaction;
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
    sGroup = "Standard-View";
    sMenuText = QT_TR_NOOP("R&ecall Working View");
    sToolTipText = QT_TR_NOOP("Recalls a previously stored temporary working view");
    sStatusTip = sToolTipText;
    sWhatsThis = "Std_RecallWorkingView";
    sAccel = "End";
    eType = NoTransaction;
}

void StdRecallWorkingView::activated(int iMsg)
{
    Q_UNUSED(iMsg);
    if (auto view = dynamic_cast<Gui::View3DInventor*>(Gui::getMainWindow()->activeWindow())) {
        if (view->getViewer()->hasHomePosition()) {
            view->getViewer()->resetToHomePosition();
        }
    }
}

bool StdRecallWorkingView::isActive()
{
    auto view = dynamic_cast<Gui::View3DInventor*>(Gui::getMainWindow()->activeWindow());
    return view && view->getViewer()->hasHomePosition();
}

//===========================================================================
// Std_AlignToSelection
//===========================================================================
DEF_STD_CMD_A(StdCmdAlignToSelection)

StdCmdAlignToSelection::StdCmdAlignToSelection()
    : Command("Std_AlignToSelection")
{
    sGroup = "View";
    sMenuText = QT_TR_NOOP("&Align to Selection");
    sToolTipText = QT_TR_NOOP("Aligns the camera view to the selected elements in the 3D view");
    sWhatsThis = "Std_AlignToSelection";
    sPixmap = "align-to-selection";
    eType = Alter3DView;
}

void StdCmdAlignToSelection::activated(int iMsg)
{
    Q_UNUSED(iMsg);
    doCommand(Command::Gui, "Gui.SendMsgToActiveView(\"AlignToSelection\")");
}

bool StdCmdAlignToSelection::isActive()
{
    return getGuiApplication()->sendHasMsgToActiveView("AlignToSelection");
}

//===========================================================================
// Std_ClarifySelection
//===========================================================================

DEF_STD_CMD_A(StdCmdClarifySelection)

StdCmdClarifySelection::StdCmdClarifySelection()
    : Command("Std_ClarifySelection")
{
    sGroup = "View";
    sMenuText = QT_TR_NOOP("Clarify Selection");
    sToolTipText = QT_TR_NOOP(
        "Displays a context menu at the mouse cursor to select overlapping "
        "or obstructed geometry in the 3D view.\n"
    );
    sWhatsThis = "Std_ClarifySelection";
    sStatusTip = sToolTipText;
    sAccel = "G, G";
    sPixmap = "tree-pre-sel";
    eType = NoTransaction | AlterSelection;
}

void StdCmdClarifySelection::activated(int iMsg)
{
    Q_UNUSED(iMsg);

    // Get the active view
    auto view3d = freecad_cast<View3DInventor*>(Application::Instance->activeView());
    if (!view3d) {
        return;
    }

    auto viewer = view3d->getViewer();
    if (!viewer) {
        return;
    }

    QWidget* widget = viewer->getGLWidget();
    if (!widget) {
        return;
    }

    // check if we have a stored right-click position (context menu) or should use current cursor
    // position (keyboard shortcut)
    SbVec2s point;
    auto& storedPosition = viewer->navigationStyle()->getRightClickPosition();
    if (storedPosition.has_value()) {
        point = storedPosition.value();
    }
    else {
        QPoint pos = QCursor::pos();
        QPoint local = widget->mapFromGlobal(pos);

        qreal devicePixelRatio = widget->devicePixelRatioF();
        point = SbVec2s(
            static_cast<short>(local.x() * devicePixelRatio),
            static_cast<short>((widget->height() - local.y() - 1) * devicePixelRatio)
        );
    }

    // Use ray picking to get all objects under cursor
    SoRayPickAction pickAction(viewer->getSoRenderManager()->getViewportRegion());
    pickAction.setPoint(point);

    constexpr double defaultMultiplier = 5.0F;
    double clarifyRadiusMultiplier
        = App::GetApplication()
              .GetParameterGroupByPath("User parameter:BaseApp/Preferences/View")
              ->GetFloat("ClarifySelectionRadiusMultiplier", defaultMultiplier);

    pickAction.setRadius(viewer->getPickRadius() * clarifyRadiusMultiplier);
    pickAction.setPickAll(static_cast<SbBool>(true));  // Get all objects under cursor
    pickAction.apply(viewer->getSoRenderManager()->getSceneGraph());

    const SoPickedPointList& pplist = pickAction.getPickedPointList();
    if (pplist.getLength() == 0) {
        return;
    }

    // Convert picked points to PickData list
    std::vector<PickData> selections;

    for (int i = 0; i < pplist.getLength(); ++i) {
        SoPickedPoint* pp = pplist[i];
        if (!pp || !pp->getPath()) {
            continue;
        }

        ViewProvider* vp = viewer->getViewProviderByPath(pp->getPath());
        if (!vp) {
            continue;
        }

        // Cast to ViewProviderDocumentObject to get the object
        auto vpDoc = freecad_cast<Gui::ViewProviderDocumentObject*>(vp);
        if (!vpDoc) {
            continue;
        }

        App::DocumentObject* obj = vpDoc->getObject();
        if (!obj) {
            continue;
        }

        // Get element information - handle sub-objects like Assembly parts
        std::string elementName = vp->getElement(pp->getDetail());
        std::string subName;

        // Try to get more detailed sub-object information
        bool hasSubObject = false;
        if (vp->getElementPicked(pp, subName)) {
            hasSubObject = true;
        }

        // Create PickData with selection information
        PickData pickData {
            .obj = obj,
            .element = elementName,
            .docName = obj->getDocument()->getName(),
            .objName = obj->getNameInDocument(),
            .subName = hasSubObject ? subName : elementName
        };

        selections.push_back(pickData);
    }

    if (selections.empty()) {
        return;
    }

    QPoint globalPos;
    if (storedPosition.has_value()) {
        qreal devicePixelRatio = widget->devicePixelRatioF();
        int logicalHeight = static_cast<int>(widget->height());
        QPoint localPos(
            static_cast<int>(point[0] / devicePixelRatio),
            logicalHeight - static_cast<int>(point[1] / devicePixelRatio) - 1
        );
        globalPos = widget->mapToGlobal(localPos);
    }
    else {
        globalPos = QCursor::pos();
    }

    // Use SelectionMenu to display and handle the pick menu
    SelectionMenu contextMenu(widget);
    contextMenu.doPick(selections, globalPos);
}

bool StdCmdClarifySelection::isActive()
{
    return qobject_cast<View3DInventor*>(getMainWindow()->activeWindow()) != nullptr;
}

//===========================================================================
// Instantiation
//===========================================================================


namespace Gui
{

void CreateViewStdCommands()
{
    // NOLINTBEGIN
    CommandManager& rcCmdMgr = Application::Instance->commandManager();

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
    rcCmdMgr.addCommand(new StdCmdViewGroup());
    rcCmdMgr.addCommand(new StdCmdAlignToSelection());
    rcCmdMgr.addCommand(new StdCmdClarifySelection());

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
    rcCmdMgr.addCommand(new StdCmdToggleVisibility());
    rcCmdMgr.addCommand(new StdCmdToggleTransparency());
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
    rcCmdMgr.addCommand(new StdCmdSceneInspector());
    rcCmdMgr.addCommand(new StdCmdTextureMapping());
    rcCmdMgr.addCommand(new StdCmdDemoMode());
    rcCmdMgr.addCommand(new StdCmdToggleNavigation());
    rcCmdMgr.addCommand(new StdCmdAxisCross());
    rcCmdMgr.addCommand(new StdCmdSelBoundingBox());
    rcCmdMgr.addCommand(new StdCmdTreeViewActions());
    rcCmdMgr.addCommand(new StdCmdDockOverlay());

    auto hGrp = App::GetApplication().GetParameterGroupByPath(
        "User parameter:BaseApp/Preferences/View"
    );
    if (hGrp->GetASCII("GestureRollFwdCommand").empty()) {
        hGrp->SetASCII("GestureRollFwdCommand", "Std_SelForward");
    }
    if (hGrp->GetASCII("GestureRollBackCommand").empty()) {
        hGrp->SetASCII("GestureRollBackCommand", "Std_SelBack");
    }
    // NOLINTEND
}

}  // namespace Gui
