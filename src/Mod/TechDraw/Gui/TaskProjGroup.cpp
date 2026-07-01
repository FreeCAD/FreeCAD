/***************************************************************************
 *   Copyright (c) 2014 Joe Dowsett <dowsettjoe[at]yahoo[dot]co[dot]uk>    *
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

# include <QMessageBox>
# include <QGroupBox>
# include <QLabel>
# include <QScreen>

#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QPushButton>
#include <QDialog>

#include <App/Document.h>
#include <App/Link.h>
#include <Base/Console.h>
#include <Base/Tools.h>
#include <Base/BaseClass.h>
#include <Gui/Application.h>
#include <Gui/BitmapFactory.h>
#include <Gui/Command.h>
#include <Gui/FileDialog.h>
#include <Gui/Document.h>
#include <Gui/MainWindow.h>
#include <Gui/WaitCursor.h>
#include <Gui/QuantitySpinBox.h>
#include <Gui/Selection/Selection.h>
#include <Mod/Spreadsheet/App/Sheet.h>

#include <Mod/TechDraw/App/DrawPage.h>
#include <Mod/TechDraw/App/DrawView.h>
#include <Mod/TechDraw/App/DrawViewPart.h>
#include <Mod/TechDraw/App/DrawProjGroup.h>
#include <Mod/TechDraw/App/DrawProjGroupItem.h>
#include <Mod/TechDraw/App/DrawUtil.h>
#include <Mod/TechDraw/App/Preferences.h>
#include <Mod/TechDraw/App/DrawViewSpreadsheet.h>

#include "DrawGuiUtil.h"
#include "TaskProjGroup.h"
#include "ui_TaskProjGroup.h"
#include "MDIViewPage.h"
#include "ViewProviderPage.h"
#include "ViewProviderDrawingView.h"
#include "ViewProviderProjGroupItem.h"
#include "ViewProviderProjGroup.h"


using namespace Gui;
using namespace TechDraw;
using namespace TechDrawGui;

TaskProjGroup::TaskProjGroup(bool mode) :
    ui(new Ui_TaskProjGroup),
    view(nullptr),
    multiView(nullptr),
    m_page(nullptr),
    m_mdi(nullptr),
    m_createMode(mode),
    blockCheckboxes(false)
{
    ui->setupUi(this);

    connectWidgets();
    initializeUi();
    setUiPrimary();
    updateUi();

    saveGroupState();

    blockUpdate = false;

    if (m_createMode) {
        attachSelection();
        Gui::SelectionChanges msg;
        msg.Type = Gui::SelectionChanges::SetSelection;
        onSelectionChanged(msg);
    }
}

void TaskProjGroup::connectWidgets()
{
    connect(ui->openFileButton, &QPushButton::clicked, this, &TaskProjGroup::openFileButtonClicked);
    // Rotation buttons
    connect(ui->butTopRotate,   &QPushButton::clicked, this, &TaskProjGroup::rotateButtonClicked);
    connect(ui->butCWRotate,    &QPushButton::clicked, this, &TaskProjGroup::rotateButtonClicked);
    connect(ui->butRightRotate, &QPushButton::clicked, this, &TaskProjGroup::rotateButtonClicked);
    connect(ui->butDownRotate,  &QPushButton::clicked, this, &TaskProjGroup::rotateButtonClicked);
    connect(ui->butLeftRotate,  &QPushButton::clicked, this, &TaskProjGroup::rotateButtonClicked);
    connect(ui->butCCWRotate,   &QPushButton::clicked, this, &TaskProjGroup::rotateButtonClicked);
    connect(ui->butFront,       &QPushButton::clicked, this, &TaskProjGroup::rotateButtonClicked);
    connect(ui->butCam,         &QPushButton::clicked, this, &TaskProjGroup::rotateButtonClicked);

    connect(ui->lePrimary,   &QPushButton::clicked, this, &TaskProjGroup::customDirectionClicked);

    // Slot for Scale Type
    connect(ui->cmbScaleType, qOverload<int>(&QComboBox::currentIndexChanged), this, &TaskProjGroup::scaleTypeChanged);
    connect(ui->sbScaleNum,   qOverload<int>(&QSpinBox::valueChanged), this, &TaskProjGroup::scaleManuallyChanged);
    connect(ui->sbScaleDen,   qOverload<int>(&QSpinBox::valueChanged), this, &TaskProjGroup::scaleManuallyChanged);

    // Slot for Projection Type (layout)
    connect(ui->projection, qOverload<int>(&QComboBox::currentIndexChanged), this, [this]() {
        projectionTypeChanged(ui->projection->currentIndex());
    });

    // Spacing
    connect(ui->cbAutoDistribute, &QPushButton::clicked, this, &TaskProjGroup::AutoDistributeClicked);
    connect(ui->sbXSpacing, qOverload<double>(&QuantitySpinBox::valueChanged), this, &TaskProjGroup::spacingChanged);
    connect(ui->sbYSpacing, qOverload<double>(&QuantitySpinBox::valueChanged), this, &TaskProjGroup::spacingChanged);

    //Spreadsheet
    connect(ui->leCellStart, &QLineEdit::editingFinished, this, &TaskProjGroup::spreadsheetRangeChanged);
    connect(ui->leCellEnd, &QLineEdit::editingFinished, this, &TaskProjGroup::spreadsheetRangeChanged);
}

void TaskProjGroup::initializeUi()
{
    if (!view) {
        return;
    }

    if (multiView) {
        // we have a projection group as input
        ui->projection->setCurrentIndex(multiView->ProjectionType.getValue());
        ui->cbAutoDistribute->setChecked(multiView->AutoDistribute.getValue());
        // disable if no AutoDistribute
        ui->sbXSpacing->setEnabled(multiView->AutoDistribute.getValue());
        ui->sbYSpacing->setEnabled(multiView->AutoDistribute.getValue());
        ui->sbXSpacing->setValue(multiView->spacingX.getValue());
        ui->sbYSpacing->setValue(multiView->spacingY.getValue());
    } else {
        ui->projection->setCurrentIndex(Preferences::projectionAngle());
        ui->cbAutoDistribute->setChecked(Preferences::groupAutoDistribute());
        ui->sbXSpacing->setValue(Preferences::groupSpaceX());
        ui->sbYSpacing->setValue(Preferences::groupSpaceY());
    }

    setFractionalScale(view->getScale());
    ui->cmbScaleType->setCurrentIndex(view->ScaleType.getValue());

    //Allow or prevent scale changing initially
    if (view->ScaleType.isValue("Custom"))	{
        ui->sbScaleNum->setEnabled(true);
        ui->sbScaleDen->setEnabled(true);
    }
    else {
        ui->sbScaleNum->setEnabled(false);
        ui->sbScaleDen->setEnabled(false);
    }

    // Initially toggle view checkboxes if needed
    setupViewCheckboxes(true);

    ui->sbXSpacing->setUnit(Base::Unit::Length);
    ui->sbYSpacing->setUnit(Base::Unit::Length);

    if (Preferences::useCameraDirection()) {
        ui->butCam->setChecked(true);
    } else {
        ui->butFront->setChecked(true);
    }

}

static std::pair<App::DocumentObject*, std::string> faceFromSelection()
{
    auto selection = Gui::Selection().getSelectionEx(
        nullptr, App::DocumentObject::getClassTypeId(), Gui::ResolveMode::NoResolve);

    if (selection.empty()) {
        return { nullptr, "" };
    }

    for (auto& sel : selection) {
        for (auto& sub : sel.getSubNames()) {
            if (TechDraw::DrawUtil::getGeomTypeFromName(sub) == "Face") {
                return { sel.getObject(), sub };
            }
        }
    }

    return { nullptr, "" };
}

static std::pair<Base::Vector3d, Base::Vector3d> viewDirection()
{
    if (!Preferences::useCameraDirection()) {
        return { Base::Vector3d(0, -1, 0), Base::Vector3d(1, 0, 0) };
    }

    auto faceInfo = faceFromSelection();
    if (faceInfo.first) {
        return DrawGuiUtil::getProjDirFromFace(faceInfo.first, faceInfo.second);
    }

    return DrawGuiUtil::get3DDirAndRot();
}

//! checks for directions that are almost +/- x,y,z.
static Base::Vector3d checkDirectionVsBasis(Base::Vector3d dir)
{
    Base::Vector3d closest = DrawUtil::closestBasisOriented(dir);
    if (dir.IsEqual(closest, Precision::Confusion())) {
        return closest;
    }

    double angleDeg = Base::toDegrees(dir.GetAngle(closest));
    constexpr double MaxAngleDeg{1.0};  // absolutely a WAG.
    if (std::fabs(angleDeg) < MaxAngleDeg) {
        // close to a basis, but not quite equal
        auto msgText = QObject::tr("Selected Direction is within %1 degrees of a standard direction. "
                    "Replace selected Direction with %2?")
                    .arg(QString::number(angleDeg))
                    .arg(QString::fromStdString(DrawUtil::formatVector(closest)));
        QMessageBox::StandardButton rc = QMessageBox::question(
            Gui::getMainWindow(), QObject::tr("Direction is close to standard"),
            msgText,
            QMessageBox::StandardButtons(QMessageBox::Yes | QMessageBox::No));
        if (rc == QMessageBox::Yes) {
            return closest;
        }
    }

    // not close to a basis vector.
    return dir;

}

void TaskProjGroup::removeView()
{
    if (!view || dynamic_cast<TechDraw::DrawViewPart*>(view)) {
        return;
    }
    std::string pageName = m_page->getNameInDocument();
    std::string viewName = view->getNameInDocument();
    view = nullptr;
    Gui::Command::doCommand(Gui::Command::Doc, "App.activeDocument().%s.removeView(App.activeDocument().%s)",
        pageName.c_str(), viewName.c_str());
    Gui::Command::doCommand(Gui::Command::Doc, "App.activeDocument().removeObject('%s')",
        viewName.c_str());
}

void TaskProjGroup::clearViews()
{
    if (view) {
        std::string pageName = m_page->getNameInDocument();
        std::string viewName = view->getNameInDocument();
        view = nullptr;
        multiView = nullptr;
        int tid = Gui::Command::openActiveDocumentCommand(QT_TRANSLATE_NOOP("Command", "Remove view"));
        Gui::Command::doCommand(Gui::Command::Doc,
            "App.activeDocument().%s.removeView(App.activeDocument().%s)",
            pageName.c_str(), viewName.c_str());
        Gui::Command::doCommand(Gui::Command::Doc,
            "App.activeDocument().removeObject('%s')",
            viewName.c_str());
        Gui::Command::commitCommand(tid);
    }
    updateUi();
}

void TaskProjGroup::addSheetView(App::DocumentObject* obj) {
    removeView();
    std::string SpreadName = obj->getNameInDocument();
    std::string PageName = m_page->getNameInDocument();

    int tid = Gui::Command::openActiveDocumentCommand(QT_TRANSLATE_NOOP("Command", "Create spreadsheet view"));
    std::string FeatName = App::GetApplication().getActiveDocument()->getUniqueObjectName("Sheet");
    
    Gui::Command::doCommand(Gui::Command::Doc, "App.activeDocument().addObject('TechDraw::DrawViewSpreadsheet', '%s')",
        FeatName.c_str());
    Gui::Command::doCommand(Gui::Command::Doc, "App.activeDocument().%s.translateLabel('DrawViewSpreadsheet', 'Sheet', '%s')",
        FeatName.c_str(), FeatName.c_str());
    Gui::Command::doCommand(Gui::Command::Doc, "App.activeDocument().%s.Source = App.activeDocument().%s", FeatName.c_str(),
        SpreadName.c_str());
    Gui::Command::doCommand(Gui::Command::Doc, "App.activeDocument().%s.addView(App.activeDocument().%s)", PageName.c_str(),
        FeatName.c_str());
    Gui::Command::doCommand(Gui::Command::Doc, "if App.activeDocument().%s.Scale: App.activeDocument().%s.Scale = App.activeDocument().%s.Scale",
        PageName.c_str(), FeatName.c_str(), PageName.c_str());

    Gui::Command::updateActive();
    Gui::Command::commitCommand(tid);
    view = dynamic_cast<TechDraw::DrawView*>(
        App::GetApplication().getActiveDocument()->getObject(FeatName.c_str()));
    if (view) {
        m_viewName = view->getNameInDocument();
    }
    initializeUi();
    updateUi();
}

void TaskProjGroup::addBIMView(App::DocumentObject* obj) {
    removeView();
    std::string PageName = m_page->getNameInDocument();
    std::string FeatName = App::GetApplication().getActiveDocument()->getUniqueObjectName("BIM view");
    std::string SourceName = obj->getNameInDocument();

    int tid = Gui::Command::openActiveDocumentCommand(QT_TRANSLATE_NOOP("Command", "Create BIM view"));
    
    Gui::Command::doCommand(Gui::Command::Doc, "App.activeDocument().addObject('TechDraw::DrawViewArch', '%s')",
        FeatName.c_str());
    Gui::Command::doCommand(Gui::Command::Doc, "App.activeDocument().%s.translateLabel('DrawViewArch', 'BIM view', '%s')",
        FeatName.c_str(), FeatName.c_str());
    Gui::Command::doCommand(Gui::Command::Doc, "App.activeDocument().%s.Source = App.activeDocument().%s", FeatName.c_str(),
        SourceName.c_str());
    Gui::Command::doCommand(Gui::Command::Doc, "App.activeDocument().%s.addView(App.activeDocument().%s)", PageName.c_str(),
        FeatName.c_str());
    Gui::Command::doCommand(Gui::Command::Doc, "if App.activeDocument().%s.Scale: App.activeDocument().%s.Scale = App.activeDocument().%s.Scale",
        PageName.c_str(), FeatName.c_str(), PageName.c_str());

    Gui::Command::updateActive();
    Gui::Command::commitCommand(tid);
    view = dynamic_cast<TechDraw::DrawView*>(
        App::GetApplication().getActiveDocument()->getObject(FeatName.c_str()));
    if (view) {
        m_viewName = view->getNameInDocument();
    }
    initializeUi();
    updateUi();
}

void TaskProjGroup::addDraftView(App::DocumentObject* obj) {
    removeView();
    
    std::string PageName = m_page->getNameInDocument();
    std::string FeatName = App::GetApplication().getActiveDocument()->getUniqueObjectName("DraftView");
    std::string SourceName = obj->getNameInDocument();

    std::pair<Base::Vector3d, Base::Vector3d> dirs = DrawGuiUtil::get3DDirAndRot();
    Base::Vector3d checkedDir = checkDirectionVsBasis(dirs.first);

    int tid = Gui::Command::openActiveDocumentCommand(QT_TRANSLATE_NOOP("Command", "Create DraftView"));
    Gui::Command::doCommand(Gui::Command::Doc, "App.activeDocument().addObject('TechDraw::DrawViewDraft', '%s')",
              FeatName.c_str());
    Gui::Command::doCommand(Gui::Command::Doc, "App.activeDocument().%s.translateLabel('DrawViewDraft', 'DraftView', '%s')",
          FeatName.c_str(), FeatName.c_str());
    Gui::Command::doCommand(Gui::Command::Doc, "App.activeDocument().%s.Source = App.activeDocument().%s", FeatName.c_str(),
              SourceName.c_str());
    Gui::Command::doCommand(Gui::Command::Doc, "App.activeDocument().%s.addView(App.activeDocument().%s)", PageName.c_str(),
              FeatName.c_str());
    Gui::Command::doCommand(Gui::Command::Doc, "if App.activeDocument().%s.Scale: App.activeDocument().%s.Scale = App.activeDocument().%s.Scale",
              PageName.c_str(), FeatName.c_str(), PageName.c_str());
    Gui::Command::doCommand(Gui::Command::Doc, "App.activeDocument().%s.Direction = FreeCAD.Vector(%.12f, %.12f, %.12f)",
              FeatName.c_str(), checkedDir.x, checkedDir.y, checkedDir.z);
    Gui::Command::updateActive();
    Gui::Command::commitCommand(tid);
    view = dynamic_cast<TechDraw::DrawView*>(
        App::GetApplication().getActiveDocument()->getObject(FeatName.c_str()));
    if (view) {
        m_viewName = view->getNameInDocument();
    }
    initializeUi();
    updateUi();
}

void TaskProjGroup::onSelectionChanged(const Gui::SelectionChanges& msg)
{
    if (m_processingSelection) {
        return;
    }
    Base::StateLocker lock(m_processingSelection);

    if (!m_createMode) {
        return;
    }

    if (msg.Type != Gui::SelectionChanges::AddSelection
        && msg.Type != Gui::SelectionChanges::RmvSelection
        && msg.Type != Gui::SelectionChanges::SetSelection
        && msg.Type != Gui::SelectionChanges::ClrSelection) {

        return;
    }

    if (!m_page) {
        auto* mvp = qobject_cast<MDIViewPage*>(Gui::getMainWindow()->activeWindow());
        if (!mvp) {
            return;
        }
        m_page = mvp->getViewProviderPage()->getDrawPage();
        if (!m_page) {
            return;
        }

        m_deletedObjectConnection = m_page->getDocument()->signalDeletedObject.connect([this](const App::DocumentObject& obj) {
            onDeletedObject(const_cast<App::DocumentObject*>(&obj));
        });
    }

    std::vector<App::DocumentObject*> shapes, xShapes;
    App::DocumentObject* partObj = nullptr;
    std::string faceName;

    std::vector<SelectionObject> selection = Gui::Selection().getSelectionEx();

    if (!selection.empty()) {
        bool objectFound = false;
        for (auto& sel : selection) {
            auto obj = sel.getObject();
            if (!obj) {
                continue;
            }
            if (!obj->isDerivedFrom<TechDraw::DrawPage>() && !obj->isDerivedFrom<TechDraw::DrawView>()) {
                objectFound = true;
                break;
            }
        }
        if (!objectFound) {
            return;
        }
    }

    if (m_defaultViewSet && selection.empty()) {
        return;
    }

    Gui::WaitCursor wc;
    clearViews();

    for (auto& sel : selection) {
        bool is_linked = false;
        auto obj = sel.getObject();
        if (!obj) {
            continue;
        }

        if (obj->isDerivedFrom<TechDraw::DrawPage>() || obj->isDerivedFrom<TechDraw::DrawView>()) {
            continue;
        }
        if (obj->isDerivedFrom<Spreadsheet::Sheet>()) {
            addSheetView(obj);
            continue;
        }
        else if (DrawGuiUtil::isArchSection(obj)) {
            addBIMView(obj);
            continue;
        }
        else if (DrawGuiUtil::isDraftObject(obj)) {
            addDraftView(obj);
            continue;
        }

        if (obj->isDerivedFrom<App::LinkElement>()
            || obj->isDerivedFrom<App::LinkGroup>()
            || obj->isDerivedFrom<App::Link>()) {
            is_linked = true;
        }
        // If parent of the obj is a link to another document, we possibly need to treat non-link obj as linked, too
        // 1st, is obj in another document?
        if (obj->getDocument() != m_page->getDocument()) {
            std::set<App::DocumentObject*> parents = obj->getInListEx(true);
            for (auto& parent : parents) {
                // Only consider parents in the current document, i.e. possible links in this View's document
                if (parent->getDocument() != m_page->getDocument()) {
                    continue;
                }
                // 2nd, do we really have a link to obj?
                if (parent->isDerivedFrom<App::LinkElement>()
                    || parent->isDerivedFrom<App::LinkGroup>()
                    || parent->isDerivedFrom<App::Link>()) {
                    // We have a link chain from this document to obj, and obj is in another document -> it is an XLink target
                    is_linked = true;
                }
            }
        }

        if (is_linked) {
            xShapes.push_back(obj);
            continue;
        }
        //not a Link and not null.  assume to be drawable.  Undrawables will be
        // skipped later.
        shapes.push_back(obj);
        if (partObj) {
            continue;
        }
        //don't know if this works for an XLink
        for (auto& sub : sel.getSubNames()) {
            if (TechDraw::DrawUtil::getGeomTypeFromName(sub) == "Face") {
                faceName = sub;
                //
                partObj = obj;
                break;
            }
        }
    }

    bool foundView = false;
    if (shapes.empty() && xShapes.empty() && !m_defaultViewSet) {

        if (view) {
            foundView = true;
        }
        App::Document* doc = App::GetApplication().getActiveDocument();
        if (doc) {
            if (!foundView) {
                auto bodies = doc->getObjectsOfType(Base::Type::fromName("PartDesign::Body"));
                if (!bodies.empty() && bodies.size() == 1) {
                    shapes.push_back(bodies.front());
                    foundView = true;
                }
            }
            if (!foundView) {
                auto allObjs = doc->getObjects();
                int draftObjects = 0;
                App::DocumentObject* draftObject = nullptr;
                for (auto* obj : allObjs) {
                    if (DrawGuiUtil::isDraftObject(obj)) {
                        draftObjects++;
                        draftObject = obj;
                    }
                }
                if (draftObjects == 1) {
                    addDraftView(draftObject);
                    foundView = true;
                }
            }

            if (!foundView) {
                auto allObjs = doc->getObjects();
                int spreadsheetObjects = 0;
                App::DocumentObject* spreadsheetObject = nullptr;
                for (auto* obj : allObjs) {
                    if (obj->isDerivedFrom<Spreadsheet::Sheet>()) {
                        spreadsheetObjects++;
                        spreadsheetObject = obj;
                    }
                }
                if (spreadsheetObjects == 1) {
                    addSheetView(spreadsheetObject);
                    foundView = true;
                }
            }
        }
    }

    m_defaultViewSet = true;

    if (shapes.empty() && xShapes.empty()) {
        return;
    }

    int tid = Gui::Command::openActiveDocumentCommand(QT_TRANSLATE_NOOP("Command", "Create view"));
    std::string pageName = m_page->getNameInDocument();

    if (view) {
        Gui::Command::doCommand(Gui::Command::Doc, "App.activeDocument().%s.removeView(App.activeDocument().%s)",
            pageName.c_str(), view->getNameInDocument());
        Gui::Command::doCommand(Gui::Command::Doc, "App.activeDocument().removeObject('%s')",
            view->getNameInDocument());
        view = nullptr;
        multiView = nullptr;
    }

    std::string FeatName = App::GetApplication().getActiveDocument()->getUniqueObjectName("View");
    Gui::Command::doCommand(Gui::Command::Doc, "App.activeDocument().addObject('TechDraw::DrawProjGroupItem', '%s')",
        FeatName.c_str());
    Gui::Command::doCommand(Gui::Command::Doc, "App.activeDocument().%s.translateLabel('DrawProjGroupItem', 'View', '%s')",
        FeatName.c_str(), FeatName.c_str());
    Gui::Command::doCommand(Gui::Command::Doc, "App.activeDocument().%s.addView(App.activeDocument().%s)", pageName.c_str(),
        FeatName.c_str());

    App::DocumentObject* docObj = m_page->getDocument()->getObject(FeatName.c_str());
    auto* dvp = dynamic_cast<TechDraw::DrawViewPart*>(docObj);
    if (!dvp) {
        throw Base::TypeError("CmdTechDrawView DVP not found\n");
    }
    dvp->Source.setValues(shapes);
    dvp->XSource.setValues(xShapes);

    m_page->getDocument()->setStatus(App::Document::Status::SkipRecompute, true);
    std::pair<Base::Vector3d, Base::Vector3d> dirs = viewDirection();
    Base::Vector3d checkedDir = checkDirectionVsBasis(dirs.first);
    Gui::Command::doCommand(Gui::Command::Doc,
              "App.activeDocument().%s.Direction = FreeCAD.Vector(%.12f, %.12f, %.12f)",
              FeatName.c_str(), checkedDir.x, checkedDir.y, checkedDir.z);
    Gui::Command::doCommand(Gui::Command::Doc,
              "App.activeDocument().%s.RotationVector = FreeCAD.Vector(%.12f, %.12f, %.12f)",
              FeatName.c_str(), dirs.second.x, dirs.second.y, dirs.second.z);
    Gui::Command::doCommand(Gui::Command::Doc,
              "App.activeDocument().%s.XDirection = FreeCAD.Vector(%.12f, %.12f, %.12f)",
              FeatName.c_str(), dirs.second.x, dirs.second.y, dirs.second.z);

    m_page->getDocument()->setStatus(App::Document::Status::SkipRecompute, false);
    Gui::Command::doCommand(Gui::Command::Doc, "App.activeDocument().%s.recompute()", FeatName.c_str());
    Gui::Command::commitCommand(tid);


    view = dvp;
    multiView = dynamic_cast<TechDraw::DrawProjGroup*>(view);
    m_viewName = view->getNameInDocument();
    Gui::Document* activeGui = Gui::Application::Instance->getDocument(m_page->getDocument());
    Gui::ViewProvider* vp = activeGui->getViewProvider(m_page);
    auto* vpPage = static_cast<ViewProviderPage*>(vp);
    m_mdi = vpPage->getMDIViewPage();

    scaleTypeChanged(ui->cmbScaleType->currentIndex());

    initializeUi();
    updateUi();
}

void TaskProjGroup::onDeletedObject(App::DocumentObject* obj)
{
    if (obj == view) {
        view = nullptr;
        multiView = nullptr;
        updateUi();
    }
}


//! enable/disable the appropriate widgets
void TaskProjGroup::updateUi()
{
    if (multiView) {
        setWindowTitle(QObject::tr("Projection Group"));
        ui->projection->show();
        ui->cbAutoDistribute->show();
        ui->sbXSpacing->show();
        ui->sbYSpacing->show();
        ui->label_7->show();
        ui->label_10->show();
        ui->label_11->show();
        ui->secondaryProjGroupbox->show();
    }
    else {
        setWindowTitle(QObject::tr("New View"));
        ui->projection->hide();
        ui->cbAutoDistribute->hide();
        ui->sbXSpacing->hide();
        ui->sbYSpacing->hide();
        ui->label_7->hide();
        ui->label_10->hide();
        ui->label_11->hide();

        auto* dpgi = dynamic_cast<TechDraw::DrawProjGroupItem*>(view);
        bool isNonPartView = view && !dynamic_cast<TechDraw::DrawViewPart*>(view);
        ui->secondaryProjGroupbox->show();
        ui->label->show();
        ui->cmbScaleType->show();
        ui->sbScaleNum->show();
        ui->label_4->show();
        ui->sbScaleDen->show();
        ui->directionGroupbox->show();

        if (!dpgi) {
            ui->secondaryProjGroupbox->hide();
        }

        if (isNonPartView) {
            ui->secondaryProjGroupbox->hide();
            ui->directionGroupbox->hide();
        }

        if (m_createMode) {
            ui->label1->show();
            ui->openFileButton->show();
        }
        else {
            ui->label1->hide();
            ui->openFileButton->hide();
        }

        if (dynamic_cast<TechDraw::DrawViewSpreadsheet*>(view)) {
            ui->spreadsheetGroupBox->show();
        }
        else {
            ui->spreadsheetGroupBox->hide();
        }
    }
}

void TaskProjGroup::saveGroupState()
{
    if (!view) {
        return;
    }

    m_saveScaleType = view->ScaleType.getValueAsString();
    m_saveScale = view->Scale.getValue();

    if (multiView) {
        m_saveSource = multiView->Source.getValues();
        m_saveProjType = multiView->ProjectionType.getValue();
        m_saveAutoDistribute = multiView->AutoDistribute.getValue();
        m_saveSpacingX = multiView->spacingX.getValue();
        m_saveSpacingY = multiView->spacingY.getValue();
        DrawProjGroupItem* anchor = multiView->getAnchor();
        m_saveDirection = anchor->Direction.getValue();

        for( const auto it : multiView->Views.getValues() ) {
            auto view( dynamic_cast<DrawProjGroupItem *>(it) );
            if (view) {
                m_saveViewNames.emplace_back(view->Type.getValueAsString());
            }
        }
    }
}

//never used?
void TaskProjGroup::restoreGroupState()
{
    if (!view) {
        return;
    }

    view->ScaleType.setValue(m_saveScaleType.c_str());
    view->Scale.setValue(m_saveScale);

    if (multiView) {
        multiView->ProjectionType.setValue(m_saveProjType);
        multiView->AutoDistribute.setValue(m_saveAutoDistribute);
        multiView->spacingX.setValue(m_saveSpacingX);
        multiView->spacingY.setValue(m_saveSpacingY);
        multiView->purgeProjections();
        for(auto & sv : m_saveViewNames) {
            if (sv != "Front") {
                multiView->addProjection(sv.c_str());
            }
        }
    }
}

void TaskProjGroup::viewToggled(bool toggle)
{
    if (!view) {
        return;
    }

    Gui::WaitCursor wc;
    bool changed = false;
    // Obtain name of checkbox
    int index = sender()->objectName().mid(7).toInt();
    const char *viewNameCStr = viewChkIndexToCStr(index);

    if (!blockCheckboxes) {
        if (multiView) {
            // Check if only front is left. If so switch to normal view.
            if (multiView->Views.getValues().size() == 2 && !toggle) {
                turnProjGroupToView();
                wc.restoreCursor();
                return;
            }
        }
        else {
            // If toggle then we remove the view object and create a proj group instead.
            turnViewToProjGroup();
            changed = true;
        }
    }

    if (toggle && !multiView->hasProjection(viewNameCStr)) {
        Gui::Command::doCommand(Gui::Command::Doc,
                                "App.activeDocument().%s.addProjection('%s')",
                                multiView->getNameInDocument(), viewNameCStr);
        changed = true;
    }
    else if (!toggle && multiView->hasProjection(viewNameCStr)) {
        if (multiView->canDelete(viewNameCStr)) {
            multiView->removeProjection( viewNameCStr );
            changed = true;
        }
    }

    if (changed) {
        // necessary to prevent position problems
        Gui::Document* activeGui = Gui::Application::Instance->getDocument(m_page->getDocument());
        auto* vppg = static_cast<ViewProviderProjGroup*>(activeGui->getViewProvider(multiView));
        vppg->regroupSubViews();
        if (view->ScaleType.isValue("Automatic")) {
            double scale = view->getScale();
            setFractionalScale(scale);
        }
        view->recomputeFeature();
    }
    wc.restoreCursor();
}


void TaskProjGroup::turnViewToProjGroup()
{
    App::Document* doc = view->getDocument();

    std::string multiViewName = doc->getUniqueObjectName("ProjGroup");
    Gui::Command::doCommand(Gui::Command::Gui, "App.activeDocument().addObject('TechDraw::DrawProjGroup', '%s')", multiViewName.c_str());
    Gui::Command::doCommand(Gui::Command::Gui, "App.activeDocument().%s.addView(App.activeDocument().%s)", view->findParentPage()->getNameInDocument(), multiViewName.c_str());

    auto* viewPart = static_cast<TechDraw::DrawViewPart*>(view);

    multiView = static_cast<TechDraw::DrawProjGroup*>(doc->getObject(multiViewName.c_str()));
    multiView->Source.setValues(viewPart->Source.getValues());
    multiView->XSource.setValues(viewPart->XSource.getValues());
    multiView->X.setValue(viewPart->X.getValue());
    multiView->Y.setValue(viewPart->Y.getValue());
    multiView->Scale.setValue(viewPart->Scale.getValue());
    multiView->ScaleType.setValue(viewPart->ScaleType.getValue());
    multiView->ProjectionType.setValue(Preferences::projectionAngle());

    multiView->addView(viewPart);
    multiView->Anchor.setValue(viewPart);
    multiView->Anchor.purgeTouched();
    multiView->AutoDistribute.setValue(ui->cbAutoDistribute->isChecked());

    viewPart->X.setValue(0.0);
    viewPart->Y.setValue(0.0);
    viewPart->ScaleType.setValue("Custom");
    viewPart->ScaleType.setStatus(App::Property::Hidden, true);
    viewPart->Scale.setStatus(App::Property::Hidden, true);
    viewPart->Label.setValue("Front");
    viewPart->LockPosition.setValue(true);
    viewPart->LockPosition.setStatus(App::Property::ReadOnly, true); //Front should stay locked.
    viewPart->LockPosition.purgeTouched();

    m_page->requestPaint();
    view = multiView;
    m_page->removeView(viewPart);   // prevent multiple entries in tree

    updateUi();
}

void TaskProjGroup::turnProjGroupToView()
{
    TechDraw::DrawViewPart* viewPart = multiView->getAnchor();
    viewPart->Scale.setValue(multiView->Scale.getValue());
    viewPart->ScaleType.setValue(multiView->ScaleType.getValue());
    viewPart->Scale.setStatus(App::Property::Hidden, false);
    viewPart->ScaleType.setStatus(App::Property::Hidden, false);
    viewPart->Label.setValue("View");
    viewPart->LockPosition.setValue(false);
    viewPart->LockPosition.setStatus(App::Property::ReadOnly, false);
    viewPart->X.setValue(multiView->X.getValue());
    viewPart->Y.setValue(multiView->Y.getValue());
    m_page->addView(viewPart);

    // remove viewPart from views before deleting the group.
    multiView->removeView(viewPart);

    Gui::Command::doCommand(Gui::Command::Gui, "App.activeDocument().removeObject('%s')", multiView->getNameInDocument());

    viewPart->recomputeFeature();
    Gui::Document* activeGui = Gui::Application::Instance->getDocument(m_page->getDocument());
    auto* vpView = static_cast<ViewProviderProjGroupItem*>(activeGui->getViewProvider(viewPart));
    if (vpView) {
        vpView->updateIcon();
        vpView->fixSceneDependencies();
    }

    view = viewPart;
    multiView = nullptr;

    updateUi();
}

void TaskProjGroup::customDirectionClicked()
{
    if (!view) {
        return;
    }

    auto* dirEditDlg = new DirectionEditDialog();

    if (multiView) {
        dirEditDlg->setDirection(multiView->getAnchor()->Direction.getValue());
        dirEditDlg->setAngle(0.0);
    }
    else {
        auto* viewPart = static_cast<TechDraw::DrawViewPart*>(view);
        dirEditDlg->setDirection(viewPart->Direction.getValue());
        dirEditDlg->setAngle(0.0);
    }

    if (dirEditDlg->exec() == QDialog::Accepted) {
        if (multiView) {
            multiView->getAnchor()->Direction.setValue(dirEditDlg->getDirection());
            multiView->spin(Base::toRadians(dirEditDlg->getAngle()));
        }
        else {
            auto* viewPart = static_cast<TechDraw::DrawViewPart*>(view);
            viewPart->Direction.setValue(dirEditDlg->getDirection());
            viewPart->spin(Base::toRadians(dirEditDlg->getAngle()));
        }

        setUiPrimary();
    }


    delete dirEditDlg;
}

void TaskProjGroup::openFileButtonClicked() {

    const Gui::FileDialog::FilterList filterList {
        {QObject::tr("SVG or Image files"), {"*.svg", "*.svgz", "*.jpg", "*.jpeg", "*.png", "*.bmp"}},
        Gui::FileDialog::Filter::AllFiles(),
    };
    QString filename = Gui::FileDialog::getOpenFileName(Gui::getMainWindow(),
        QObject::tr("Select a SVG or Image file to open"),
        Preferences::defaultSymbolDir(),
        filterList);

    if (!filename.isEmpty()) {

        clearViews();
        std::string pageName = m_page->getNameInDocument();
        
        if (filename.endsWith(QStringLiteral(".svg"), Qt::CaseInsensitive)
            || filename.endsWith(QStringLiteral(".svgz"), Qt::CaseInsensitive)) {
            
                std::string FeatName = App::GetApplication().getActiveDocument()->getUniqueObjectName("Symbol");
            auto filespec = DrawUtil::cleanFilespecBackslash(
                Base::Tools::escapeEncodeFilename(filename.toStdString()));
            
            int tid = Gui::Command::openActiveDocumentCommand(QT_TRANSLATE_NOOP("Command", "Create Symbol"));
            Gui::Command::doCommand(Gui::Command::Doc, "import codecs");
            Gui::Command::doCommand(Gui::Command::Doc,
                      "f = codecs.open(\"%s\", 'r', encoding=\"utf-8\")",
                      filespec.c_str());
            Gui::Command::doCommand(Gui::Command::Doc, "svg = f.read()");
            Gui::Command::doCommand(Gui::Command::Doc, "f.close()");
            Gui::Command::doCommand(Gui::Command::Doc,
                      "App.activeDocument().addObject('TechDraw::DrawViewSymbol', '%s')",
                      FeatName.c_str());
            Gui::Command::doCommand(
                Gui::Command::Doc,
                "App.activeDocument().%s.translateLabel('DrawViewSymbol', 'Symbol', '%s')",
                FeatName.c_str(),
                FeatName.c_str());
            Gui::Command::doCommand(Gui::Command::Doc, "App.activeDocument().%s.Symbol = svg", FeatName.c_str());
            Gui::Command::doCommand(Gui::Command::Doc,
                      "App.activeDocument().%s.addView(App.activeDocument().%s)",
                      pageName.c_str(),
                      FeatName.c_str());
            Gui::Command::commitCommand(tid);
            view = dynamic_cast<TechDraw::DrawView*>(
                App::GetApplication().getActiveDocument()->getObject(FeatName.c_str()));
        }
        else {
            std::string FeatName = App::GetApplication().getActiveDocument()->getUniqueObjectName("Image");
            auto filespec = DrawUtil::cleanFilespecBackslash(
                Base::Tools::escapeEncodeFilename(filename.toStdString()));
            int tid = Gui::Command::openActiveDocumentCommand(QT_TRANSLATE_NOOP("Command", "Create image"));
            Gui::Command::doCommand(Gui::Command::Doc, "App.activeDocument().addObject('TechDraw::DrawViewImage', '%s')", FeatName.c_str());
            Gui::Command::doCommand(Gui::Command::Doc, "App.activeDocument().%s.translateLabel('DrawViewImage', 'Image', '%s')",
                FeatName.c_str(), FeatName.c_str());
            Gui::Command::doCommand(Gui::Command::Doc, "App.activeDocument().%s.ImageFile = '%s'", FeatName.c_str(), filespec.c_str());
            Gui::Command::doCommand(Gui::Command::Doc, "App.activeDocument().%s.addView(App.activeDocument().%s)", pageName.c_str(), FeatName.c_str());
            Gui::Command::updateActive();
            Gui::Command::commitCommand(tid);
            view = dynamic_cast<TechDraw::DrawView*>(
                App::GetApplication().getActiveDocument()->getObject(FeatName.c_str()));
        }

        Gui::Command::updateActive();

        multiView = nullptr;

        if (view) {
            m_viewName = view->getNameInDocument();
        }
        initializeUi();
        updateUi();
    }
}

void TaskProjGroup::spreadsheetRangeChanged() {
    auto* sheetFeat = dynamic_cast<TechDraw::DrawViewSpreadsheet*>(view);
    if (sheetFeat) {
        sheetFeat->CellStart.setValue(ui->leCellStart->text().toStdString().c_str());
        sheetFeat->CellEnd.setValue(ui->leCellEnd->text().toStdString().c_str());
        sheetFeat->execute();
    }
}

void TaskProjGroup::rotateButtonClicked()
{
    if ( view && ui ) {
        const QObject *clicked = sender();

        auto handleCameraButton = [&]() {
            std::string faceName;
            App::DocumentObject* obj = nullptr;
            auto selection = Gui::Command::getSelection().getSelectionEx();
            for (auto& sel : selection) {
                for (auto& sub : sel.getSubNames()) {
                    if (TechDraw::DrawUtil::getGeomTypeFromName(sub) == "Face") {
                        obj = sel.getObject();
                        faceName = sub;
                        break;
                    }
                }
                if (!faceName.empty()) {
                    break;
                }
            }

            std::pair<Base::Vector3d, Base::Vector3d> dirs = !faceName.empty() ?
                DrawGuiUtil::getProjDirFromFace(obj, faceName)
                : DrawGuiUtil::get3DDirAndRot();
            return dirs;
        };

        if (multiView) {
            //change Front View Dir by 90
            if (clicked == ui->butTopRotate) multiView->rotate(RotationMotion::Up);
            else if (clicked == ui->butDownRotate) multiView->rotate(RotationMotion::Down);
            else if (clicked == ui->butRightRotate) multiView->rotate(RotationMotion::Right);
            else if (clicked == ui->butLeftRotate) multiView->rotate(RotationMotion::Left);
            else if (clicked == ui->butCWRotate) multiView->spin(SpinDirection::CW);
            else if (clicked == ui->butCCWRotate) multiView->spin(SpinDirection::CCW);
            else if (clicked == ui->butFront) {
                multiView->getAnchor()->Direction.setValue(Base::Vector3d(0.0, -1.0, 0.0));
                multiView->getAnchor()->RotationVector.setValue(Base::Vector3d(1.0, 0.0, 0.0));
                multiView->getAnchor()->XDirection.setValue(Base::Vector3d(1.0, 0.0, 0.0));
                multiView->updateSecondaryDirs();
            }
            else if (clicked == ui->butCam) {
                std::pair<Base::Vector3d, Base::Vector3d> dirs = handleCameraButton();
                multiView->getAnchor()->Direction.setValue(dirs.first);
                multiView->getAnchor()->RotationVector.setValue(dirs.second);
                multiView->getAnchor()->XDirection.setValue(dirs.second);
                multiView->updateSecondaryDirs();
            }
        }
        else {
            auto* viewPart = static_cast<TechDraw::DrawViewPart*>(view);
            if (clicked == ui->butTopRotate) viewPart->rotate(RotationMotion::Up);
            else if (clicked == ui->butDownRotate) viewPart->rotate(RotationMotion::Down);
            else if (clicked == ui->butRightRotate) viewPart->rotate(RotationMotion::Right);
            else if (clicked == ui->butLeftRotate) viewPart->rotate(RotationMotion::Left);
            else if (clicked == ui->butCWRotate) viewPart->spin(SpinDirection::CW);
            else if (clicked == ui->butCCWRotate) viewPart->spin(SpinDirection::CCW);
            else if (clicked == ui->butFront) {
                viewPart->Direction.setValue(Base::Vector3d(0.0,-1.0,0.0));
                viewPart->XDirection.setValue(Base::Vector3d(1.0, 0.0, 0.0));
                viewPart->recomputeFeature();
            }
            else if (clicked == ui->butCam) {
                std::pair<Base::Vector3d, Base::Vector3d> dirs = handleCameraButton();

                viewPart->Direction.setValue(dirs.first);
                viewPart->XDirection.setValue(dirs.second);
                viewPart->recomputeFeature();
            }
        }

        setUiPrimary();
    }
}

void TaskProjGroup::projectionTypeChanged(int index)
{
    if(blockUpdate || !multiView) {
        return;
    }

    multiView->ProjectionType.setValue((long)index);

    // Update checkboxes so checked state matches the drawing
    blockCheckboxes = true;
    setupViewCheckboxes();
    blockCheckboxes = false;

    // set the tooltips of the checkboxes
    ui->chkView0->setToolTip(getToolTipForBox(0));
    ui->chkView1->setToolTip(getToolTipForBox(1));
    ui->chkView2->setToolTip(getToolTipForBox(2));
    ui->chkView3->setToolTip(getToolTipForBox(3));
    ui->chkView4->setToolTip(getToolTipForBox(4));
    ui->chkView5->setToolTip(getToolTipForBox(5));
    ui->chkView6->setToolTip(getToolTipForBox(6));
    ui->chkView7->setToolTip(getToolTipForBox(7));
    ui->chkView8->setToolTip(getToolTipForBox(8));
    ui->chkView9->setToolTip(getToolTipForBox(9));

    // R/L and T/B view need to be repositioned and just recomputing a single view will not do this
    multiView->recomputeChildren();
}

void TaskProjGroup::scaleTypeChanged(int index)
{
    if (blockUpdate) {
        return;
    }

    if (!view) {
        return;
    }

    ui->sbScaleNum->setEnabled(false);
    ui->sbScaleDen->setEnabled(false);

    if (index == 0) {
        // Document Scale Type
        view->ScaleType.setValue("Page");
    }
    else if (index == 1) {
        // Automatic Scale Type
        //block recompute
        view->ScaleType.setValue("Automatic");
        double autoScale = view->autoScale();
        view->Scale.setValue(autoScale);
        //unblock recompute

    }
    else if (index == 2) {
        // Custom Scale Type
        //block recompute
        view->ScaleType.setValue("Custom");
        ui->sbScaleNum->setEnabled(true);
        ui->sbScaleDen->setEnabled(true);

        int numerator = ui->sbScaleNum->value();
        int denominator = ui->sbScaleDen->value();
        view->Scale.setValue((double)numerator / (double)denominator);
    }
}

void TaskProjGroup::AutoDistributeClicked(bool clicked)
{
    if (blockUpdate || !multiView) {
        return;
    }
    multiView->AutoDistribute.setValue(clicked);
    multiView->recomputeFeature();
}

void TaskProjGroup::spacingChanged()
{
    if (blockUpdate || !multiView) {
        return;
    }

    multiView->spacingX.setValue(ui->sbXSpacing->value().getValue());
    multiView->spacingY.setValue(ui->sbYSpacing->value().getValue());

    multiView->autoPositionChildren();
}


void TaskProjGroup::updateTask()
{
    if (!view) {
        return;
    }
    // Update the scale type
    blockUpdate = true;
    ui->cmbScaleType->setCurrentIndex(view->ScaleType.getValue());

    // Update the scale value
    setFractionalScale(view->getScale());

    blockUpdate = false;
}


void TaskProjGroup::setFractionalScale(double newScale)
{
    blockUpdate = true;

    std::pair<int, int> fraction = DrawUtil::nearestFraction(newScale);

    ui->sbScaleNum->setValue(fraction.first);
    ui->sbScaleDen->setValue(fraction.second);
    blockUpdate = false;
}

void TaskProjGroup::scaleManuallyChanged(int unused)
{
    Q_UNUSED(unused);
    if(blockUpdate) {
        return;
    }
    if (!view || !view->ScaleType.isValue("Custom")) {
        return;
    }

    int numerator = ui->sbScaleNum->value();
    int denominator = ui->sbScaleDen->value();

    double scale = (double) numerator / (double) denominator;
    Gui::Command::doCommand(Gui::Command::Doc, "App.activeDocument().%s.Scale = %f", view->getNameInDocument()
                                                                                     , scale);
    view->recomputeFeature();
}

void TaskProjGroup::changeEvent(QEvent *event)
{
    if (event->type() == QEvent::LanguageChange) {
        ui->retranslateUi(this);
    }
}

const char * TaskProjGroup::viewChkIndexToCStr(int index)
{
    //   Third angle:  FTL  T  FTRight
    //                  L   F   Right   Rear
    //                 FBL  B  FBRight
    //
    //   First angle:  FBRight  B  FBL
    //                  Right   F   L  Rear
    //                 FTRight  T  FTL

    bool thirdAngle = useThirdAngle();
    switch(index) {
        case 0: return (thirdAngle ? "FrontTopLeft" : "FrontBottomRight");
        case 1: return (thirdAngle ? "Top" : "Bottom");
        case 2: return (thirdAngle ? "FrontTopRight" : "FrontBottomLeft");
        case 3: return (thirdAngle ? "Left" : "Right");
        case 4: return "Front";
        case 5: return (thirdAngle ? "Right" : "Left");
        case 6: return "Rear";
        case 7: return (thirdAngle ? "FrontBottomLeft" : "FrontTopRight");
        case 8: return (thirdAngle ? "Bottom" : "Top");
        case 9: return (thirdAngle ? "FrontBottomRight" : "FrontTopLeft");
        default: return nullptr;
    }
}

QString TaskProjGroup::getToolTipForBox(int boxNumber)
{
    bool thirdAngle = useThirdAngle();
    switch(boxNumber) {
        case 0: {return (thirdAngle ? tr("FrontTopLeft") : tr("FrontBottomRight")); break;}
        case 1: {return (thirdAngle ? tr("Top") : tr("Bottom")); break;}
        case 2: {return (thirdAngle ? tr("FrontTopRight") : tr("FrontBottomLeft")); break;}
        case 3: {return (thirdAngle ? tr("Left" ): tr("Right")); break;}
        case 4: {return tr("Front"); break; }
        case 5: {return (thirdAngle ? tr("Right") : tr("Left")); break;}
        case 6: {return tr("Rear"); break; }
        case 7: {return (thirdAngle ? tr("FrontBottomLeft") : tr("FrontTopRight")); break;}
        case 8: {return (thirdAngle ? tr("Bottom") : tr("Top")); break;}
        case 9: {return (thirdAngle ? tr("FrontBottomRight") : tr("FrontTopLeft")); break;}
        default: {return {}; break;}
    }
}

bool TaskProjGroup::useThirdAngle()
{
    if (!view) {
        // something is wrong if this happens
        throw Base::RuntimeError("TaskProjGroup - no view!");
    }

    auto page = view->findParentPage();
    if (!page) {
        return false;
    }

   if (!multiView) {
        return Preferences::projectionAngle();
    }

    if (multiView->ProjectionType.getValue() == (long)DrawProjGroup::ViewProjectionConvention::ThirdAngle) {
        return true;
    }

    if (multiView->ProjectionType.getValue() == (long)DrawProjGroup::ViewProjectionConvention::Page &&
        page->ProjectionType.getValue() == (long)DrawPage::PageProjectionConvention::ThirdAngle) {
        return true;
    }
    return false;
}

void TaskProjGroup::setupViewCheckboxes(bool addConnections)
{
    if (!view) {
        return;
    }

    // There must be a better way to construct this list...
    QCheckBox * viewCheckboxes[] = { ui->chkView0,
                                     ui->chkView1,
                                     ui->chkView2,
                                     ui->chkView3,
                                     ui->chkView4,
                                     ui->chkView5,
                                     ui->chkView6,
                                     ui->chkView7,
                                     ui->chkView8,
                                     ui->chkView9 };

    for (int i = 0; i < 10; ++i) {
        QCheckBox *box = viewCheckboxes[i];
        box->setToolTip(getToolTipForBox(i));
        const char *viewStr = viewChkIndexToCStr(i);

        if (!multiView) {
            box->setCheckState(strcmp(viewStr, "Front") == 0 ? Qt::Checked : Qt::Unchecked);
        }

        if (addConnections) {
            connect(box, &QCheckBox::toggled, this, &TaskProjGroup::viewToggled, Qt::UniqueConnection);
        }

        if (multiView) {
            if (viewStr && multiView->hasProjection(viewStr)) {
                box->setCheckState(Qt::Checked);
                if (!multiView->canDelete(viewStr)) {
                    box->setEnabled(false);
                }
            }
            else {
                box->setCheckState(Qt::Unchecked);
            }
        }
    }
}


void TaskProjGroup::setView(TechDraw::DrawView* newView)
{
    view = newView;
    multiView = dynamic_cast<TechDraw::DrawProjGroup*>(newView);
    if (newView) {
        m_page = newView->findParentPage();
        m_viewName = newView->getNameInDocument();
        if (m_page) {
            Gui::Document* activeGui = Gui::Application::Instance->getDocument(m_page->getDocument());
            auto* vpPage = static_cast<ViewProviderPage*>(activeGui->getViewProvider(m_page));
            m_mdi = vpPage->getMDIViewPage();
        }

        if (auto* sheetView = dynamic_cast<TechDraw::DrawViewSpreadsheet*>(newView)) {
            ui->leCellStart->setText(sheetView->CellStart.getValue());
            ui->leCellEnd->setText(sheetView->CellEnd.getValue());
        }
    }
    initializeUi();
    updateUi();
}


//! sets the main direction of the view
// Note: does not set any of the other values that one would expect to be initialized
void TaskProjGroup::setUiPrimary()
{
    Base::Vector3d frontDir;
    if (multiView) {
        frontDir = multiView->getAnchorDirection();
    }
    else {
        auto* viewPart = static_cast<TechDraw::DrawViewPart*>(view);
        if (viewPart) {
            frontDir = viewPart->Direction.getValue();
        }
    }
    ui->lePrimary->setText(formatVector(frontDir));
}

QString TaskProjGroup::formatVector(Base::Vector3d vec)
{
    QString data = QStringLiteral("[%1 %2 %3]")
        .arg(QLocale().toString(vec.x, 'f', 2),
             QLocale().toString(vec.y, 'f', 2),
             QLocale().toString(vec.z, 'f', 2));
    return data;
}

void TaskProjGroup::saveButtons(QPushButton* btnOK,
                             QPushButton* btnCancel)
{
    m_btnOK = btnOK;
    m_btnCancel = btnCancel;
}


bool TaskProjGroup::apply()
{
    if (!view) {
        return true;
    }
    if (multiView) {
        multiView->recomputeChildren();
    }
    view->recomputeFeature();

    return true;
}

bool TaskProjGroup::accept()
{
    detachSelection();
    if (!m_page || !view) {
        return true;
    }
    Gui::Document* doc = Gui::Application::Instance->getDocument(m_page->getDocument());
    if (!doc) {
        return false;
    }
    auto viewCheck = m_page->getDocument()->getObject(m_viewName.c_str());
    if (!viewCheck) {
        // view has been deleted while this dialog is open
        return false;
    }

    Gui::Command::doCommand(Gui::Command::Gui, "Gui.ActiveDocument.resetEdit()");

    view = nullptr;
    multiView = nullptr;
    m_page = nullptr;
    m_mdi = nullptr;
    m_viewName.clear();

    m_defaultViewSet = false;

    Gui::Selection().clearSelection();

    return true;
}

bool TaskProjGroup::reject()
{
    detachSelection();
    if (!m_page) {
        return true;
    }
    if (!view) {
        if (getCreateMode()) {
            clearViews();
        }
        Gui::Command::runCommand(Gui::Command::Gui, "Gui.ActiveDocument.resetEdit()");
        m_page = nullptr;
        m_mdi = nullptr;
        m_viewName.clear();
        return true;
    }

    Gui::Document* doc = Gui::Application::Instance->getDocument(m_page->getDocument());
    if (!doc) {
        return false;
    }

    auto viewCheck = m_page->getDocument()->getObject(m_viewName.c_str());
    if (!viewCheck) {
        // view has been deleted while this dialog is open
        return false;
    }

    if (getCreateMode()) {
        //remove the object completely from the document
        const char* viewName = view->getNameInDocument();
        const char* PageName = view->findParentPage()->getNameInDocument();

        if (multiView) {
            Gui::Command::doCommand(Gui::Command::Gui, "App.activeDocument().%s.purgeProjections()",
                viewName);
        }
        Gui::Command::doCommand(Gui::Command::Gui, "App.activeDocument().%s.removeView(App.activeDocument().%s)",
            PageName, viewName);
        Gui::Command::doCommand(Gui::Command::Gui, "App.activeDocument().removeObject('%s')", viewName);
        Gui::Command::doCommand(Gui::Command::Gui, "Gui.ActiveDocument.resetEdit()");

        view = nullptr;
        multiView = nullptr;
        clearViews();
    }
    else {
        //set the DPG and its views back to entry state.
        if (doc->hasPendingCommand()) {
            doc->abortCommand();
        }
        // Restore views to initial spacing
        if (multiView) {
            multiView->autoPositionChildren();
        }
    }
    Gui::Command::runCommand(Gui::Command::Gui, "Gui.ActiveDocument.resetEdit()");

    Gui::Selection().clearSelection();
    return false;
}

/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//TODO: Do we really need to hang on to the TaskDlgProjGroup in this class? IR
TaskDlgProjGroup::TaskDlgProjGroup(bool mode)
    : viewProvider(nullptr)
    , view(nullptr)
{
    widget  = new TaskProjGroup(mode);
    taskbox = new Gui::TaskView::TaskBox(Gui::BitmapFactory().pixmap("actions/TechDraw_ProjectionGroup"),
                                         widget->windowTitle(), true, nullptr);
    taskbox->groupLayout()->addWidget(widget);
    Content.push_back(taskbox);
}

void TaskDlgProjGroup::update()
{
    widget->updateTask();
}


void TaskDlgProjGroup::setView(TechDraw::DrawView* v)
{
    view = v;
    widget->setView(v);
}

void TaskDlgProjGroup::setCreateMode(bool mode)
{
    widget->setCreateMode(mode);
}

void TaskDlgProjGroup::modifyStandardButtons(QDialogButtonBox* box)
{
    QPushButton* btnOK = box->button(QDialogButtonBox::Ok);
    QPushButton* btnCancel = box->button(QDialogButtonBox::Cancel);
    widget->saveButtons(btnOK, btnCancel);
}

//==== calls from the TaskView ===============================================================
void TaskDlgProjGroup::open()
{
    if (!widget->getCreateMode())  {    //this is an edit session, start a transaction
        if (dynamic_cast<TechDraw::DrawProjGroup*>(view)) {
            App::GetApplication().setActiveTransaction(App::TransactionName{.name="Edit Projection Group", .temporary=true});
        }
        else {
            App::GetApplication().setActiveTransaction(App::TransactionName{.name="Edit Part View", .temporary=true});
        }
    }
}

void TaskDlgProjGroup::clicked(int i)
{
//    Q_UNUSED(i);
    if (i == QMessageBox::Apply) {
        widget->apply();
    }
}

bool TaskDlgProjGroup::accept()
{
    widget->accept();
    return true;
}

bool TaskDlgProjGroup::reject()
{
    widget->reject();
    return true;
}


DirectionEditDialog::DirectionEditDialog(QWidget* parent) : QDialog(parent) {
    setWindowFlags(Qt::Popup); // Make the dialog non-intrusive
    createUI();
}

void DirectionEditDialog::setDirection(const Base::Vector3d& pos) {
    xSpinBox->setValue(pos.x);
    ySpinBox->setValue(pos.y);
    zSpinBox->setValue(pos.z);
}

Base::Vector3d DirectionEditDialog::getDirection() const {
    return Base::Vector3d(xSpinBox->value().getValue(), ySpinBox->value().getValue(), zSpinBox->value().getValue());
}

void DirectionEditDialog::setAngle(double val) {
    angleSpinBox->setValue(val);
}

double DirectionEditDialog::getAngle() const {
    return angleSpinBox->value().getValue();
}

void DirectionEditDialog::showEvent(QShowEvent* event) {
    QDialog::showEvent(event);

    // Calculate the position to ensure the dialog appears within the screen boundaries
    QPoint cursorPos = QCursor::pos();
    QSize screenSize = QApplication::primaryScreen()->size(); // Get the size of the primary screen
    int x = cursorPos.x();
    int y = cursorPos.y();
    int dialogWidth = this->width();
    int dialogHeight = this->height();

    // Check if the dialog goes beyond the right edge of the screen
    if (x + dialogWidth > screenSize.width()) {
        x = screenSize.width() - dialogWidth;
    }

    // Check if the dialog goes beyond the bottom edge of the screen
    if (y + dialogHeight > screenSize.height()) {
        y = screenSize.height() - dialogHeight;
    }

    // Move the dialog to the calculated position
    this->move(x, y);
}

void DirectionEditDialog::createUI() {
    auto* directionGroup = new QGroupBox(tr("Direction"));
    auto* directionLayout = new QVBoxLayout; // Use QVBoxLayout for vertical alignment

    // Create layout and widgets for X
    auto* xLayout = new QHBoxLayout;
    auto* xLabel = new QLabel(QStringLiteral("X: "));
    xSpinBox = new Gui::QuantitySpinBox;
    xSpinBox->setUnit(Base::Unit::Length);
    xLayout->addWidget(xLabel);
    xLayout->addWidget(xSpinBox);

    // Create layout and widgets for Y
    auto* yLayout = new QHBoxLayout;
    auto* yLabel = new QLabel(QStringLiteral("Y: "));
    ySpinBox = new Gui::QuantitySpinBox;
    ySpinBox->setUnit(Base::Unit::Length);
    yLayout->addWidget(yLabel);
    yLayout->addWidget(ySpinBox);

    // Create layout and widgets for Z
    auto* zLayout = new QHBoxLayout;
    auto* zLabel = new QLabel(QStringLiteral("Z: "));
    zSpinBox = new Gui::QuantitySpinBox;
    zSpinBox->setUnit(Base::Unit::Length);
    zLayout->addWidget(zLabel);
    zLayout->addWidget(zSpinBox);

    // Add the layouts to the direction group
    directionLayout->addLayout(xLayout);
    directionLayout->addLayout(yLayout);
    directionLayout->addLayout(zLayout);
    directionGroup->setLayout(directionLayout);

    angleSpinBox = new Gui::QuantitySpinBox;
    angleSpinBox->setUnit(Base::Unit::Angle);

    auto* buttonsLayout = new QHBoxLayout;
    auto* okButton = new QPushButton(tr("OK"));
    auto* cancelButton = new QPushButton(tr("Cancel"));
    buttonsLayout->addWidget(okButton);
    buttonsLayout->addWidget(cancelButton);

    auto* mainLayout = new QVBoxLayout;
    mainLayout->addWidget(directionGroup);
    mainLayout->addWidget(new QLabel(tr("Rotate by")));
    mainLayout->addWidget(angleSpinBox);
    mainLayout->addLayout(buttonsLayout);
    setLayout(mainLayout);

    connect(okButton, &QPushButton::clicked, this, &QDialog::accept);
    connect(cancelButton, &QPushButton::clicked, this, &QDialog::reject);
}

#include <Mod/TechDraw/Gui/moc_TaskProjGroup.cpp>
