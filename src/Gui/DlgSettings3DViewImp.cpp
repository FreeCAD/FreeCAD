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
# include <QApplication>
# include <QDoubleSpinBox>
# include <QRegExp>
# include <QGridLayout>
# include <QMessageBox>
# include <QTimer>
# include <memory>
#endif

#include "DlgSettings3DViewImp.h"
#include "ui_DlgSettings3DView.h"
#include "Application.h"
#include "Document.h"
#include "MainWindow.h"
#include "NavigationStyle.h"
#include "PrefWidgets.h"
#include "View3DInventor.h"
#include "View3DInventorViewer.h"
#include "ui_MouseButtons.h"
#include <App/Application.h>
#include <Base/Console.h>
#include <Base/Parameter.h>
#include <Base/Tools.h>
#include "ViewParams.h"
#include "Renderer/Renderer.h"

using namespace Gui::Dialog;
using namespace Gui;
using namespace Render;

/* TRANSLATOR Gui::Dialog::DlgSettings3DViewImp */

/**
 *  Constructs a DlgSettings3DViewImp which is a child of 'parent', with the
 *  name 'name' and widget flags set to 'f'
 */
DlgSettings3DViewImp::DlgSettings3DViewImp(QWidget* parent)
    : PreferencePage( parent )
    , ui(new Ui_DlgSettings3DView)
{
    ui->setupUi(this);
    QString tooltip = tr(ViewParams::docRenderCacheMergeCount());
    ui->renderCacheMergeCount->setToolTip(tooltip);
    ui->renderCacheMergeCountLabel->setToolTip(tooltip);
    tooltip = tr(ViewParams::docRenderCacheMergeCountMax());
    ui->renderCacheMergeCountMax->setToolTip(tooltip);
    ui->renderCacheMergeCountMaxLabel->setToolTip(tooltip);
}

/**
 *  Destroys the object and frees any allocated resources
 */
DlgSettings3DViewImp::~DlgSettings3DViewImp()
{
    // no need to delete child widgets, Qt does it all for us
}

void DlgSettings3DViewImp::saveSettings()
{
    // must be done as very first because we create a new instance of NavigatorStyle
    // where we set some attributes afterwards
    ParameterGrp::handle hGrp = App::GetApplication().GetParameterGroupByPath
        ("User parameter:BaseApp/Preferences/View");

    int index = ui->comboAliasing->currentIndex();
    hGrp->SetInt("AntiAliasing", index);

    index = ui->renderCache->currentIndex();
    hGrp->SetInt("RenderCache", index);

    ui->comboRenderer->onSave();

    ui->comboTransparentRender->onSave();

    QVariant const &vBoxMarkerSize = ui->boxMarkerSize->itemData(ui->boxMarkerSize->currentIndex());
    hGrp->SetInt("MarkerSize", vBoxMarkerSize.toInt());

    ui->CheckBox_CornerCoordSystem->onSave();
    ui->CheckBox_ShowAxisCross->onSave();
    ui->CheckBox_WbByTab->onSave();
    ui->CheckBox_ShowFPS->onSave();
    ui->spinPickRadius->onSave();
    ui->CheckBox_use_SW_OpenGL->onSave();
    ui->CheckBox_useVBO->onSave();
    ui->FloatSpinBox_EyeDistance->onSave();
    ui->checkBoxBacklight->onSave();
    ui->backlightColor->onSave();
    ui->sliderIntensity->onSave();
    ui->radioPerspective->onSave();
    ui->radioOrthographic->onSave();
    ui->CheckBox_ApplyToViews->onSave();
    ui->spinPreselectionDelay->onSave();
    ui->renderCacheMergeCount->onSave();
    ui->renderCacheMergeCountMax->onSave();
    ui->checkBoxEnhancedPick->onSave();
}

void DlgSettings3DViewImp::loadSettings()
{
    ui->CheckBox_CornerCoordSystem->onRestore();
    ui->CheckBox_ShowAxisCross->onRestore();
    ui->CheckBox_WbByTab->onRestore();
    ui->CheckBox_ShowFPS->onRestore();
    ui->spinPickRadius->onRestore();
    ui->CheckBox_use_SW_OpenGL->onRestore();
    ui->CheckBox_useVBO->onRestore();
    ui->FloatSpinBox_EyeDistance->onRestore();
    ui->checkBoxBacklight->onRestore();
    ui->backlightColor->onRestore();
    ui->sliderIntensity->onRestore();
    ui->radioPerspective->onRestore();
    ui->radioOrthographic->onRestore();
    ui->CheckBox_ApplyToViews->onRestore();
    ui->spinPreselectionDelay->onRestore();
    ui->renderCacheMergeCount->onRestore();
    ui->renderCacheMergeCountMax->onRestore();
    ui->comboAliasing->onRestore();
    ui->renderCache->onRestore();
    ui->comboTransparentRender->onRestore();
    ui->checkBoxEnhancedPick->onRestore();

    ui->boxMarkerSize->addItem(tr("5px"), QVariant(5));
    ui->boxMarkerSize->addItem(tr("7px"), QVariant(7));
    ui->boxMarkerSize->addItem(tr("9px"), QVariant(9));
    ui->boxMarkerSize->addItem(tr("11px"), QVariant(11));
    ui->boxMarkerSize->addItem(tr("13px"), QVariant(13));
    ui->boxMarkerSize->addItem(tr("15px"), QVariant(15));
    ui->boxMarkerSize->setCurrentIndex(2); // default value 9px
    ui->boxMarkerSize->onRestore();

    ui->comboRenderer->addItem(tr("Default"), QByteArray("Default"));
    for (auto &type : RendererFactory::types())
        ui->comboRenderer->addItem(tr(type.c_str()), QByteArray(type.c_str()));
    ui->comboRenderer->onRestore();
}

/**
 * Sets the strings of the subwidgets using the current language.
 */
void DlgSettings3DViewImp::changeEvent(QEvent *e)
{
    if (e->type() == QEvent::LanguageChange) {
        ui->retranslateUi(this);
    }
    else {
        QWidget::changeEvent(e);
    }
}

namespace {
bool applyCameraType(bool delayTrigger, ParameterGrp *hGrp)
{
    if (!delayTrigger)
        return true;

    if (hGrp->GetBool("ApplyCameraTypeToAll", false)) {
        const char *cameraType = hGrp->GetBool("Perspective", false) ?
            "PerspectiveCamera" : "OrthographicCamera";
        for (auto doc : App::GetApplication().getDocuments()) {
            auto gdoc = Application::Instance->getDocument(doc);
            if (!gdoc) continue;
            gdoc->foreachView<MDIView>([cameraType](MDIView *view) {
                view->onMsg(cameraType, nullptr);
            });
        }
    }
    return false;
}

bool applyAntiAlias(bool delayTrigger, ParameterGrp *)
{
    if (!delayTrigger)
        return true;

    auto activeView = Base::freecad_dynamic_cast<View3DInventor>(
            getMainWindow()->activeWindow());
    std::vector<View3DInventor*> views;
    std::map<View3DInventor*, View3DInventor*> bindings;
    for (auto doc : App::GetApplication().getDocuments()) {
        auto gdoc = Application::Instance->getDocument(doc);
        if (!gdoc) continue;
        gdoc->foreachView<View3DInventor>([&views, &bindings](View3DInventor *view) {
            views.push_back(view);
            if (auto target = view->boundView())
                bindings[view] = target;
        });
    }
    std::map<View3DInventor*, View3DInventor*> viewMap;
    for (auto view : views) {
        auto clone = static_cast<View3DInventor*>(
                view->getGuiDocument()->cloneView(view));
        if (!clone)
            continue;
        const char* ppReturn = 0;
        if (view->onMsg("GetCamera", &ppReturn)) {
            std::string sMsg = "SetCamera ";
            sMsg += ppReturn;
            const char** pReturnIgnore=0;
            clone->onMsg(sMsg.c_str(), pReturnIgnore);
        }
        if (view->currentViewMode() == MDIView::Child)
            getMainWindow()->addWindow(clone);
        else
            clone->setCurrentViewMode(view->currentViewMode());
        viewMap[view] = clone;
        view->deleteSelf();
    }

    for (auto &v : bindings) {
        if (auto clone = viewMap[v.first]) {
            if (auto target = viewMap[v.second])
                clone->bindCamera(target->getCamera(), false);
            else
                clone->bindCamera(v.second->getCamera(), false);
        }
    }

    auto it = viewMap.find(activeView);
    if (it != viewMap.end())
        getMainWindow()->setActiveWindow(it->second);
    return false;
}

struct ParamKey {
    ParameterGrp::handle hGrp;
    const char *key;
    mutable bool pending = false;

    ParamKey(const char *key, const char *path = nullptr)
        :hGrp(App::GetApplication().GetUserParameter().GetGroup(
            path ? path : "BaseApp/Preferences/View"))
        ,key(key)
    {}

    ParamKey(ParameterGrp *h, const char *key)
        :hGrp(h), key(key)
    {}

    bool operator < (const ParamKey &other) const {
        if (hGrp < other.hGrp)
            return true;
        if (hGrp > other.hGrp)
            return false;
        return strcmp(key, other.key) < 0;
    }
};

struct ParamHandlers {
    std::map<ParamKey, bool (*)(bool, ParameterGrp*)> handlers;
    QTimer timer;

    void attach() {
        handlers[ParamKey("Orthographic")] = applyCameraType;
        handlers[ParamKey("Perspective")] = applyCameraType;
        handlers[ParamKey("ApplyCameraTypeToAll")] = applyCameraType;

        handlers[ParamKey("AntiAliasing")] = applyAntiAlias;

        App::GetApplication().GetUserParameter().signalParamChanged.connect(
            [this](ParameterGrp *Param, ParameterGrp::ParamType, const char *Name, const char *) {
                if (!Param || !Name)
                    return;
                auto it =  handlers.find(ParamKey(Param, Name));
                if (it != handlers.end() && it->second(false, Param)) {
                    it->first.pending = true;
                    timer.start(100);
                }
            });
        timer.setSingleShot(true);
        QObject::connect(&timer, &QTimer::timeout, [this]() {
            for (auto &v : handlers) {
                if (v.first.pending) {
                    v.first.pending = false;
                    v.second(true, v.first.hGrp);
                }
            }
        });
    }
};

ParamHandlers _ParamHandlers;
} // anonymous namespace


void DlgSettings3DViewImp::attachObserver()
{
    _ParamHandlers.attach();
    ViewParams::init();
}

#include "moc_DlgSettings3DViewImp.cpp"

