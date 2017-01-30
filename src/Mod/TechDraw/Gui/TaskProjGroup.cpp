/***************************************************************************
 *   Copyright (c) 2014 Joe Dowsett <dowsettjoe[at]yahoo[dot]co[dot]uk>    *
 *   Copyright (c) 2014  Luke Parry <l.parry@warwick.ac.uk>                *
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
#include <cmath>
#endif // #ifndef _PreComp_

#include <Base/Console.h>

#include <Gui/Application.h>
#include <Gui/BitmapFactory.h>
#include <Gui/Command.h>
#include <Gui/Control.h>
#include <Gui/Document.h>
#include <Gui/View3DInventor.h>
#include <Gui/View3DInventorViewer.h>

#include <Inventor/SbVec3f.h>

#include <Mod/Part/App/PartFeature.h>

#include <Mod/TechDraw/App/DrawPage.h>
#include <Mod/TechDraw/App/DrawUtil.h>
#include <Mod/TechDraw/App/DrawView.h>
#include <Mod/TechDraw/App/DrawViewPart.h>

#include <Mod/TechDraw/App/DrawProjGroupItem.h>
#include <Mod/TechDraw/App/DrawProjGroup.h>

#include "ViewProviderProjGroup.h"
#include "ViewProviderProjGroupItem.h"
#include "ViewProviderPage.h"
#include "TaskProjGroup.h"
#include <Mod/TechDraw/Gui/ui_TaskProjGroup.h>

using namespace Gui;
using namespace TechDraw;
using namespace TechDrawGui;

//TODO: Look into this, seems we might be able to delete it now?  IR
#if 0 // needed for Qt's lupdate utility
    qApp->translate("QObject", "Make axonometric...");
    qApp->translate("QObject", "Edit axonometric settings...");
    qApp->translate("QObject", "Make orthographic");
#endif


TaskProjGroup::TaskProjGroup(TechDraw::DrawProjGroup* featView, bool mode) :
    ui(new Ui_TaskProjGroup),
    multiView(featView),
    m_createMode(mode)
{
    ui->setupUi(this);

    blockUpdate = true;

    ui->projection->setCurrentIndex(multiView->ProjectionType.getValue());

    setFractionalScale(multiView->Scale.getValue());
    ui->cmbScaleType->setCurrentIndex(multiView->ScaleType.getValue());

    // Initially toggle view checkboxes if needed
    setupViewCheckboxes(true);

    blockUpdate = false;

    // Rotation buttons
    // Note we don't do the custom one here, as it's handled by [a different function that's held up in customs]
    connect(ui->butTopRotate,   SIGNAL(clicked()), this, SLOT(rotateButtonClicked(void)));
    connect(ui->butCWRotate,    SIGNAL(clicked()), this, SLOT(rotateButtonClicked(void)));
    connect(ui->butRightRotate, SIGNAL(clicked()), this, SLOT(rotateButtonClicked(void)));
    connect(ui->butDownRotate,  SIGNAL(clicked()), this, SLOT(rotateButtonClicked(void)));
    connect(ui->butLeftRotate,  SIGNAL(clicked()), this, SLOT(rotateButtonClicked(void)));
    connect(ui->butCCWRotate,   SIGNAL(clicked()), this, SLOT(rotateButtonClicked(void)));

    //3D button
    connect(ui->but3D,   SIGNAL(clicked()), this, SLOT(on3DClicked(void)));
    //Reset button
    connect(ui->butReset,   SIGNAL(clicked()), this, SLOT(onResetClicked(void)));

    // Slot for Scale Type
    connect(ui->cmbScaleType, SIGNAL(currentIndexChanged(int)), this, SLOT(scaleTypeChanged(int)));
    connect(ui->sbScaleNum,   SIGNAL(valueChanged(int)), this, SLOT(scaleManuallyChanged(int)));
    connect(ui->sbScaleDen,   SIGNAL(valueChanged(int)), this, SLOT(scaleManuallyChanged(int)));

    // Slot for Projection Type (layout)
    connect(ui->projection, SIGNAL(currentIndexChanged(int)), this, SLOT(projectionTypeChanged(int)));

    m_page = multiView->findParentPage();
    Gui::Document* activeGui = Gui::Application::Instance->getDocument(m_page->getDocument());
    Gui::ViewProvider* vp = activeGui->getViewProvider(m_page);
    ViewProviderPage* dvp = static_cast<ViewProviderPage*>(vp);
    m_mdi = dvp->getMDIViewPage();

    setUiPrimary();
}

TaskProjGroup::~TaskProjGroup()
{
    delete ui;
}

void TaskProjGroup::viewToggled(bool toggle)
{
    bool changed = false;
    // Obtain name of checkbox
    QString viewName = sender()->objectName();
    int index = viewName.mid(7).toInt();
    const char *viewNameCStr = viewChkIndexToCStr(index);
    App::DocumentObject* newObj;
    TechDraw::DrawView* newView;
    if ( toggle && !multiView->hasProjection( viewNameCStr ) ) {
        newObj = multiView->addProjection( viewNameCStr );
        newView = static_cast<TechDraw::DrawView*>(newObj);
        m_mdi->redraw1View(newView);
        changed = true;
    } else if ( !toggle && multiView->hasProjection( viewNameCStr ) ) {
        multiView->removeProjection( viewNameCStr );
        changed = true;
    }
    if (changed) {
        multiView->recomputeFeature();
        if (multiView->ScaleType.isValue("Automatic")) {
            double scale = multiView->Scale.getValue();
            setFractionalScale(scale);
        }
    }

}

void TaskProjGroup::rotateButtonClicked(void)
{
    if ( multiView && ui ) {
        const QObject *clicked = sender();


        if ( clicked == ui->butTopRotate ) {          //change Front View Dir by 90
            multiView->rotateUp();
        } else if ( clicked == ui->butDownRotate) {
            multiView->rotateDown();
        } else if ( clicked == ui->butRightRotate) {
            multiView->rotateRight();
        } else if ( clicked == ui->butLeftRotate) {
            multiView->rotateLeft();
        } else if ( clicked == ui->butCWRotate ) {              //doesn't change Anchor view dir. changes projType of secondaries, not dir
            multiView->spinCW();
        } else if ( clicked == ui->butCCWRotate) {
            multiView->spinCCW();
        }
        setUiPrimary();
        Gui::Command::updateActive();
    }
}

void TaskProjGroup::on3DClicked(void)
{
    std::pair<Base::Vector3d,Base::Vector3d> dir3D = get3DViewDir();
    Base::Vector3d dir = dir3D.first;
    dir = DrawUtil::closestBasis(dir);
    Base::Vector3d up = dir3D.second;
    up = DrawUtil::closestBasis(up);
    TechDraw::DrawProjGroupItem* front = multiView->getProjItem("Front");
    if (front) {                              //why "if front"???
        multiView->setTable(dir,up);
        setUiPrimary();
        Gui::Command::updateActive();
    }
}

void TaskProjGroup::onResetClicked(void)
{
    TechDraw::DrawProjGroupItem* front = multiView->getProjItem("Front");
    if (front) {
        multiView->resetTable();
        setUiPrimary();
        Gui::Command::updateActive();
    }
}

void TaskProjGroup::projectionTypeChanged(int index)
{
    if(blockUpdate)
        return;

    if(index == 0) {
        //layout per Page (Document)
        Gui::Command::doCommand(Gui::Command::Doc,
                                "App.activeDocument().%s.ProjectionType = '%s'",
                                multiView->getNameInDocument(), "Default");
    } else if(index == 1) {
        // First Angle layout
        Gui::Command::doCommand(Gui::Command::Doc,
                                "App.activeDocument().%s.ProjectionType = '%s'",
                                multiView->getNameInDocument(), "First Angle");
    } else if(index == 2) {
        // Third Angle layout
        Gui::Command::doCommand(Gui::Command::Doc,
                                "App.activeDocument().%s.ProjectionType = '%s'",
                                multiView->getNameInDocument(), "Third Angle");
    } else {
        Base::Console().Log("Error - TaskProjGroup::projectionTypeChanged - unknown projection layout: %d\n",
                            index);
        return;
    }

    // Update checkboxes so checked state matches the drawing
    setupViewCheckboxes();

}

void TaskProjGroup::scaleTypeChanged(int index)
{
    if(blockUpdate)
        return;

    if(index == 0) {
        // Document Scale Type
        Gui::Command::doCommand(Gui::Command::Doc, "App.activeDocument().%s.ScaleType = '%s'", multiView->getNameInDocument()
                                                                                             , "Page");
    } else if(index == 1) {
        // Automatic Scale Type
        Gui::Command::doCommand(Gui::Command::Doc, "App.activeDocument().%s.ScaleType = '%s'", multiView->getNameInDocument()
                                                                                             , "Automatic");
    } else if(index == 2) {
        // Custom Scale Type
        Gui::Command::doCommand(Gui::Command::Doc, "App.activeDocument().%s.ScaleType = '%s'", multiView->getNameInDocument()
                                                                                             , "Custom");
        int a = ui->sbScaleNum->value();
        int b = ui->sbScaleDen->value();
        double scale = (double) a / (double) b;
        Gui::Command::doCommand(Gui::Command::Doc, "App.activeDocument().%s.Scale = %f", multiView->getNameInDocument()
                                                                                     , scale);
    } else {
        Base::Console().Log("Error - TaskProjGroup::scaleTypeChanged - unknown scale type: %d\n",index);
        return;
    }

    multiView->recomputeFeature();
    Gui::Command::updateActive();
}

// ** David Eppstein / UC Irvine / 8 Aug 1993
// Reworked 2015 IR to add the power of logarithms!
void TaskProjGroup::nearestFraction(double val, int &n, int &d) const
{
    int exponent = std::floor(std::log10(val));
    if (exponent > 1 || exponent < -1) {
        val *= std::pow(10, -exponent);
    }

    n = 1;  // numerator
    d = 1;  // denominator
    double fraction = n / d;
    //double m = fabs(fraction - val);

    while (fabs(fraction - val) > 0.001) {
        if (fraction < val) {
            ++n;
        } else {
            ++d;
            n = (int) round(val * d);
        }
        fraction = n / (double) d;
    }

    if (exponent > 1) {
            n *= std::pow(10, exponent);
    } else if (exponent < -1) {
            d *= std::pow(10, -exponent);
    }
}

void TaskProjGroup::updateTask()
{
    // Update the scale type
    blockUpdate = true;
    ui->cmbScaleType->setCurrentIndex(multiView->ScaleType.getValue());

    // Update the scale value
    setFractionalScale(multiView->Scale.getValue());

    blockUpdate = false;
}


void TaskProjGroup::setFractionalScale(double newScale)
{
    blockUpdate = true;
    int num, den;

    nearestFraction(newScale, num, den);

    ui->sbScaleNum->setValue(num);
    ui->sbScaleDen->setValue(den);
    blockUpdate = false;
}

void TaskProjGroup::scaleManuallyChanged(int i)
{
    Q_UNUSED(i);
    if(blockUpdate)
        return;
    if (!multiView->ScaleType.isValue("Custom")) {                               //ignore if not custom!
        return;
    }

    int a = ui->sbScaleNum->value();
    int b = ui->sbScaleDen->value();

    double scale = (double) a / (double) b;
    Gui::Command::doCommand(Gui::Command::Doc, "App.activeDocument().%s.Scale = %f", multiView->getNameInDocument()
                                                                                     , scale);
    multiView->recomputeFeature();
    Gui::Command::updateActive();
}

void TaskProjGroup::changeEvent(QEvent *e)
{
    if (e->type() == QEvent::LanguageChange) {
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
    assert (multiView != NULL);

    bool thirdAngle = multiView->usedProjectionType().isValue("Third Angle");
    switch(index) {
        case 0: return (thirdAngle ? "FrontTopLeft" : "FrontBottomRight");
        case 1: return (thirdAngle ? "Top" : "Bottom");
        case 2: return (thirdAngle ? "FrontTopRight" : "FrontBottomLeft");
        case 3: return (thirdAngle ? "Left" : "Right");
        case 4: return (thirdAngle ? "Front" : "Front");
        case 5: return (thirdAngle ? "Right" : "Left");
        case 6: return (thirdAngle ? "Rear" : "Rear");
        case 7: return (thirdAngle ? "FrontBottomLeft" : "FrontTopRight");
        case 8: return (thirdAngle ? "Bottom" : "Top");
        case 9: return (thirdAngle ? "FrontBottomRight" : "FrontTopLeft");
        default: return NULL;
    }
}
void TaskProjGroup::setupViewCheckboxes(bool addConnections)
{
    if ( multiView == NULL ) {
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
        if (addConnections) {
            connect(box, SIGNAL(toggled(bool)), this, SLOT(viewToggled(bool)));
        }

        const char *viewStr = viewChkIndexToCStr(i);
        if ( viewStr != NULL && multiView->hasProjection(viewStr) ) {
            box->setCheckState(Qt::Checked);
        } else {
            box->setCheckState(Qt::Unchecked);
        }
    }
}

void TaskProjGroup::setUiPrimary()
{
    Base::Vector3d frontDir = multiView->getAnchorDirection();
    ui->lePrimary->setText(formatVector(frontDir));
}


//should return a configuration?  frontdir,upDir mapped in DPG
std::pair<Base::Vector3d,Base::Vector3d> TaskProjGroup::get3DViewDir()
{
    std::pair<Base::Vector3d,Base::Vector3d> result;
    Base::Vector3d viewDir(0.0,-1.0,0.0);                                       //default to front
    Base::Vector3d viewUp(0.0,0.0,1.0);                                         //default to top
    std::list<MDIView*> mdis = Gui::Application::Instance->activeDocument()->getMDIViews();
    Gui::View3DInventor *view;
    Gui::View3DInventorViewer *viewer = nullptr;
    for (auto& m: mdis) {                                                       //find the 3D viewer
        view = dynamic_cast<Gui::View3DInventor*>(m);
        if (view) {
            viewer = view->getViewer();
            break;
        }
    }
    if (!viewer) {
        Base::Console().Log("LOG - TaskProjGroup could not find a 3D viewer\n");
        return std::make_pair( viewDir, viewUp);
    }

    SbVec3f dvec  = viewer->getViewDirection();
    SbVec3f upvec = viewer->getUpDirection();

    viewDir = Base::Vector3d(dvec[0], dvec[1], dvec[2]);
    viewUp  = Base::Vector3d(upvec[0],upvec[1],upvec[2]);
    viewDir *= -1.0;              //Inventor dir is opposite TD dir, Inventor up is same as TD up
    viewDir = DrawUtil::closestBasis(viewDir);
    viewUp  = DrawUtil::closestBasis(viewUp);
    result = std::make_pair(viewDir,viewUp);
    return result;
}


QString TaskProjGroup::formatVector(Base::Vector3d v)
{
    QString data = QString::fromLatin1("[%1 %2 %3]")
        .arg(QLocale::system().toString(v.x, 'f', 2))
        .arg(QLocale::system().toString(v.y, 'f', 2))
        .arg(QLocale::system().toString(v.z, 'f', 2));
    return data;
}

bool TaskProjGroup::accept()
{
    Gui::Document* doc = Gui::Application::Instance->getDocument(multiView->getDocument());
    if (!doc) return false;

    Gui::Command::commitCommand();
    Gui::Command::updateActive();
    Gui::Command::doCommand(Gui::Command::Gui,"Gui.ActiveDocument.resetEdit()");

    return true;
}

bool TaskProjGroup::reject()
{
    Gui::Document* doc = Gui::Application::Instance->getDocument(multiView->getDocument());
    if (!doc) return false;

    if (getCreateMode()) {
        std::string multiViewName = multiView->getNameInDocument();
        std::string PageName = multiView->findParentPage()->getNameInDocument();

        Gui::Command::doCommand(Gui::Command::Gui,"App.activeDocument().%s.purgeProjections()",
                                multiViewName.c_str());
        Gui::Command::doCommand(Gui::Command::Gui,"App.activeDocument().%s.removeView(App.activeDocument().%s)",
                                PageName.c_str(),multiViewName.c_str());
        Gui::Command::doCommand(Gui::Command::Gui,"App.activeDocument().removeObject('%s')",multiViewName.c_str());
        Gui::Command::doCommand(Gui::Command::Gui,"Gui.ActiveDocument.resetEdit()");
        //make sure any dangling objects are cleaned up 
        Gui::Command::doCommand(Gui::Command::Gui,"App.activeDocument().recompute()");
    } else {
        if (Gui::Command::hasPendingCommand()) {
            std::vector<std::string> undos = Gui::Application::Instance->activeDocument()->getUndoVector();
            Gui::Application::Instance->activeDocument()->undo(1);
            multiView->rebuildViewList();
        } else {
            Base::Console().Log("TaskProjGroup: Edit mode - NO command is active\n");
        }

        Gui::Command::updateActive();
        Gui::Command::doCommand(Gui::Command::Gui,"Gui.ActiveDocument.resetEdit()");
    }
    return false;
}

/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//TODO: Do we really need to hang on to the TaskDlgProjGroup in this class? IR
TaskDlgProjGroup::TaskDlgProjGroup(TechDraw::DrawProjGroup* featView, bool mode)
    : TaskDialog()
    , viewProvider(nullptr)
    , multiView(featView)
{
    //viewProvider = dynamic_cast<const ViewProviderProjGroup *>(featView);
    widget  = new TaskProjGroup(featView,mode);
    taskbox = new Gui::TaskView::TaskBox(Gui::BitmapFactory().pixmap("actions/techdraw-projgroup"),
                                         widget->windowTitle(), true, 0);
    taskbox->groupLayout()->addWidget(widget);
    Content.push_back(taskbox);
}

TaskDlgProjGroup::~TaskDlgProjGroup()
{
}

void TaskDlgProjGroup::update()
{
    widget->updateTask();
}

void TaskDlgProjGroup::setCreateMode(bool b)
{
    widget->setCreateMode(b);
}

//==== calls from the TaskView ===============================================================
void TaskDlgProjGroup::open()
{
    if (!widget->getCreateMode())  {    //this is an edit session, start a transaction
        Gui::Command::openCommand("Edit Projection Group");
    }
}

void TaskDlgProjGroup::clicked(int)
{
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


#include <Mod/TechDraw/Gui/moc_TaskProjGroup.cpp>
