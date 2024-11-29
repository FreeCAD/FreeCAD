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

#include "PreCompiled.h"
#ifndef _PreComp_
# include <QMessageBox>
# include <QGroupBox>
# include <QLabel>
# include <QScreen>
#endif // #ifndef _PreComp_

#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QPushButton>
#include <QDialog>

#include <App/Document.h>
#include <Base/Console.h>
#include <Base/Tools.h>
#include <Gui/Application.h>
#include <Gui/BitmapFactory.h>
#include <Gui/Command.h>
#include <Gui/Document.h>
#include <Gui/WaitCursor.h>
#include <Gui/QuantitySpinBox.h>

#include <Mod/TechDraw/App/DrawPage.h>
#include <Mod/TechDraw/App/DrawView.h>
#include <Mod/TechDraw/App/DrawViewPart.h>
#include <Mod/TechDraw/App/DrawProjGroup.h>
#include <Mod/TechDraw/App/DrawProjGroupItem.h>
#include <Mod/TechDraw/App/DrawUtil.h>
#include <Mod/TechDraw/App/Preferences.h>

#include "DrawGuiUtil.h"
#include "TaskProjGroup.h"
#include "ui_TaskProjGroup.h"
#include "MDIViewPage.h"
#include "ViewProviderPage.h"
#include "ViewProviderDrawingView.h"
#include "ViewProviderProjGroupItem.h"


using namespace Gui;
using namespace TechDraw;
using namespace TechDrawGui;

TaskProjGroup::TaskProjGroup(TechDraw::DrawView* featView, bool mode) :
    ui(new Ui_TaskProjGroup),
    view(featView),
    multiView(dynamic_cast<TechDraw::DrawProjGroup*>(view)),
    m_createMode(mode),
    blockCheckboxes(false)
{
    ui->setupUi(this);

    m_page = view->findParentPage();
    Gui::Document* activeGui = Gui::Application::Instance->getDocument(m_page->getDocument());
    Gui::ViewProvider* vp = activeGui->getViewProvider(m_page);
    auto* dvp = static_cast<ViewProviderPage*>(vp);
    m_mdi = dvp->getMDIViewPage();

    connectWidgets();
    initializeUi();
    setUiPrimary();
    updateUi();

    saveGroupState();

    blockUpdate = false;
}

void TaskProjGroup::connectWidgets()
{
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
#if QT_VERSION < QT_VERSION_CHECK(5,15,0)
    connect(ui->projection, qOverload<const QString&>(&QComboBox::currentIndexChanged), this, &TaskProjGroup::projectionTypeChanged);
#else
    connect(ui->projection, qOverload<int>(&QComboBox::currentIndexChanged), this, [this](int index) {
        projectionTypeChanged(ui->projection->itemText(index));
    });
#endif

    // Spacing
    connect(ui->cbAutoDistribute, &QPushButton::clicked, this, &TaskProjGroup::AutoDistributeClicked);
    connect(ui->sbXSpacing, qOverload<double>(&QuantitySpinBox::valueChanged), this, &TaskProjGroup::spacingChanged);
    connect(ui->sbYSpacing, qOverload<double>(&QuantitySpinBox::valueChanged), this, &TaskProjGroup::spacingChanged);
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
    }
    else {
        setWindowTitle(QObject::tr("Part View"));
        ui->projection->hide();
        ui->cbAutoDistribute->hide();
        ui->sbXSpacing->hide();
        ui->sbYSpacing->hide();
        ui->label_7->hide();
        ui->label_10->hide();
        ui->label_11->hide();

        // if the view is not a proj group item, then we disable secondary projs.
        auto* dpgi = dynamic_cast<TechDraw::DrawProjGroupItem*>(view);
        if (!dpgi) {
            ui->secondaryProjGroupbox->hide();
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
        m_saveProjType = multiView->ProjectionType.getValueAsString();
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
        multiView->ProjectionType.setValue(m_saveProjType.c_str());
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
    m_page->removeView(viewPart);

    multiView = static_cast<TechDraw::DrawProjGroup*>(doc->getObject(multiViewName.c_str()));
    multiView->Source.setValues(viewPart->Source.getValues());
    multiView->XSource.setValues(viewPart->XSource.getValues());
    multiView->X.setValue(viewPart->X.getValue());
    multiView->Y.setValue(viewPart->Y.getValue());
    multiView->Scale.setValue(viewPart->Scale.getValue());
    multiView->ScaleType.setValue(viewPart->ScaleType.getValue());
    multiView->ProjectionType.setValue(Preferences::projectionAngle());
    viewPart->X.setValue(0.0);
    viewPart->Y.setValue(0.0);
    viewPart->ScaleType.setValue("Custom");
    viewPart->ScaleType.setStatus(App::Property::Hidden, true);
    viewPart->Scale.setStatus(App::Property::Hidden, true);
    viewPart->Label.setValue("Front");

    multiView->addView(viewPart);
    multiView->Anchor.setValue(view);
    multiView->Anchor.purgeTouched();

    viewPart->LockPosition.setValue(true);
    viewPart->LockPosition.setStatus(App::Property::ReadOnly, true); //Front should stay locked.
    viewPart->LockPosition.purgeTouched();

    m_page->requestPaint();
    view = multiView;

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

    Gui::Document* activeGui = Gui::Application::Instance->getDocument(m_page->getDocument());
    auto* vp = static_cast<ViewProviderProjGroupItem*>(activeGui->getViewProvider(viewPart));
    if (vp) {
        vp->updateIcon();
    }
    viewPart->recomputeFeature();

    view = viewPart;
    multiView = nullptr;

    updateUi();
}

void TaskProjGroup::customDirectionClicked()
{
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
            if (clicked == ui->butTopRotate) multiView->rotate("Up");
            else if (clicked == ui->butDownRotate) multiView->rotate("Down");
            else if (clicked == ui->butRightRotate) multiView->rotate("Right");
            else if (clicked == ui->butLeftRotate) multiView->rotate("Left");
            else if (clicked == ui->butCWRotate) multiView->spin("CW");
            else if (clicked == ui->butCCWRotate) multiView->spin("CCW");
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
            if (clicked == ui->butTopRotate) viewPart->rotate("Up");
            else if (clicked == ui->butDownRotate) viewPart->rotate("Down");
            else if (clicked == ui->butRightRotate) viewPart->rotate("Right");
            else if (clicked == ui->butLeftRotate) viewPart->rotate("Left");
            else if (clicked == ui->butCWRotate) viewPart->spin("CW");
            else if (clicked == ui->butCCWRotate) viewPart->spin("CCW");
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

//void TaskProjGroup::projectionTypeChanged(int index)
void TaskProjGroup::projectionTypeChanged(QString qText)
{
    if(blockUpdate || !multiView) {
        return;
    }

    if (qText == QString::fromUtf8("Page")) {
        multiView->ProjectionType.setValue("Default");
    }
    else {
        std::string text = qText.toStdString();
        multiView->ProjectionType.setValue(text.c_str());
    }

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

    //defaults to prevent scale changing
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
        double scale = (double) numerator / (double) denominator;
        view->Scale.setValue(scale);
        //unblock recompute
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
    multiView->recomputeFeature();
}

void TaskProjGroup::updateTask()
{
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
    if (!view->ScaleType.isValue("Custom")) {  //ignore if not custom!
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
    //   Third Angle:  FTL  T  FTRight
    //                  L   F   Right   Rear
    //                 FBL  B  FBRight
    //
    //   First Angle:  FBRight  B  FBL
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

    bool thirdAngle = (bool) Preferences::projectionAngle();
    if (!multiView) {
        return thirdAngle;
    }

    if (multiView->usedProjectionType().isValue("Third Angle")) {
        thirdAngle = true;
    } else if (multiView->usedProjectionType().isValue("Default") &&
        page->ProjectionType.isValue("Third Angle")) {
        thirdAngle = true;
    }
    return thirdAngle;
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
            connect(box, &QCheckBox::toggled, this, &TaskProjGroup::viewToggled);
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
    QString data = QString::fromLatin1("[%1 %2 %3]")
        .arg(QLocale().toString(vec.x, 'f', 2),
             QLocale().toString(vec.y, 'f', 2),
             QLocale().toString(vec.z, 'f', 2));
    return data;
}

void TaskProjGroup::saveButtons(QPushButton* btnOK,
                             QPushButton* btnCancel,
                             QPushButton* btnApply)
{
    m_btnOK = btnOK;
    m_btnCancel = btnCancel;
    m_btnApply = btnApply;
}


bool TaskProjGroup::apply()
{
    if (multiView) {
        multiView->recomputeChildren();
    }
    view->recomputeFeature();

    return true;
}

bool TaskProjGroup::accept()
{
    Gui::Document* doc = Gui::Application::Instance->getDocument(view->getDocument());
    if (!doc) {
        return false;
    }

    if (multiView) {
        multiView->recomputeChildren();
    }
    view->recomputeFeature();

    Gui::Command::doCommand(Gui::Command::Gui, "Gui.ActiveDocument.resetEdit()");

    return true;
}

bool TaskProjGroup::reject()
{
    Gui::Document* doc = Gui::Application::Instance->getDocument(view->getDocument());
    if (!doc) {
        return false;
    }

    if (getCreateMode()) {
        //remove the object completely from the document
        const char* viewName = view->getNameInDocument();
        const char* PageName = view->findParentPage()->getNameInDocument();

        if (multiView) {
            Gui::Command::doCommand(Gui::Command::Gui, "App.activeDocument().%s.purgeProjections()",
                viewName);
            Gui::Command::doCommand(Gui::Command::Gui, "App.activeDocument().%s.removeView(App.activeDocument().%s)",
                PageName, viewName);
        }
        Gui::Command::doCommand(Gui::Command::Gui, "App.activeDocument().removeObject('%s')", viewName);
        Gui::Command::doCommand(Gui::Command::Gui, "Gui.ActiveDocument.resetEdit()");
    }
    else {
        //set the DPG and its views back to entry state.
        if (Gui::Command::hasPendingCommand()) {
            Gui::Command::abortCommand();
        }
    }
    Gui::Command::runCommand(Gui::Command::Gui, "Gui.ActiveDocument.resetEdit()");
    return false;
}

/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//TODO: Do we really need to hang on to the TaskDlgProjGroup in this class? IR
TaskDlgProjGroup::TaskDlgProjGroup(TechDraw::DrawView* featView, bool mode)
    : viewProvider(nullptr)
    , view(featView)
{
    //viewProvider = dynamic_cast<const ViewProviderDrawingView *>(featView);
    widget  = new TaskProjGroup(featView, mode);
    taskbox = new Gui::TaskView::TaskBox(Gui::BitmapFactory().pixmap("actions/TechDraw_ProjectionGroup"),
                                         widget->windowTitle(), true, nullptr);
    taskbox->groupLayout()->addWidget(widget);
    Content.push_back(taskbox);
}

void TaskDlgProjGroup::update()
{
    widget->updateTask();
}

void TaskDlgProjGroup::setCreateMode(bool mode)
{
    widget->setCreateMode(mode);
}

void TaskDlgProjGroup::modifyStandardButtons(QDialogButtonBox* box)
{
    QPushButton* btnOK = box->button(QDialogButtonBox::Ok);
    QPushButton* btnCancel = box->button(QDialogButtonBox::Cancel);
    QPushButton* btnApply = box->button(QDialogButtonBox::Apply);
    widget->saveButtons(btnOK, btnCancel, btnApply);
}

//==== calls from the TaskView ===============================================================
void TaskDlgProjGroup::open()
{
    if (!widget->getCreateMode())  {    //this is an edit session, start a transaction
        if (dynamic_cast<TechDraw::DrawProjGroup*>(view)) {
            App::GetApplication().setActiveTransaction("Edit Projection Group", true);
        }
        else {
            App::GetApplication().setActiveTransaction("Edit Part View", true);
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
