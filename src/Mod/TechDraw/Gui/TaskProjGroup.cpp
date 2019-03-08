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
#include <Gui/WaitCursor.h>

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

TaskProjGroup::TaskProjGroup(TechDraw::DrawProjGroup* featView, bool mode) :
    ui(new Ui_TaskProjGroup),
    multiView(featView),
    m_createMode(mode)
{
    ui->setupUi(this);

    blockUpdate = true;

    ui->projection->setCurrentIndex(multiView->ProjectionType.getValue());

    setFractionalScale(multiView->getScale());
    ui->cmbScaleType->setCurrentIndex(multiView->ScaleType.getValue());
    
    //Allow or prevent scale changing initially 
    if (multiView->ScaleType.isValue("Custom"))	{
        ui->sbScaleNum->setEnabled(true);
        ui->sbScaleDen->setEnabled(true);
    }
    else {
        ui->sbScaleNum->setEnabled(false);
        ui->sbScaleDen->setEnabled(false);
    }

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

//    //Reset button
//    connect(ui->butReset,   SIGNAL(clicked()), this, SLOT(onResetClicked(void)));

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
    Gui::WaitCursor wc;
    bool changed = false;
    // Obtain name of checkbox
    QString viewName = sender()->objectName();
    int index = viewName.mid(7).toInt();
    const char *viewNameCStr = viewChkIndexToCStr(index);
    if ( toggle && !multiView->hasProjection( viewNameCStr ) ) {
        (void) multiView->addProjection( viewNameCStr );            //maybe this should be send a message instead of blocking?
//        Gui::Command::doCommand(Gui::Command::Doc,                // Gui response is no faster with this. :(
//                                "App.activeDocument().%s.addProjection('%s')",
//                                multiView->getNameInDocument(), viewNameCStr);
        changed = true;
    } else if ( !toggle && multiView->hasProjection( viewNameCStr ) ) {
        multiView->removeProjection( viewNameCStr );
        changed = true;
    }
    if (changed) {
        if (multiView->ScaleType.isValue("Automatic")) {
            double scale = multiView->getScale();
            setFractionalScale(scale);
        }
    }
    wc.restoreCursor();
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
    }
}

//void TaskProjGroup::onResetClicked(void)
//{
//    TechDraw::DrawProjGroupItem* front = multiView->getProjItem("Front");
//    if (front) {
//        setUiPrimary();
//        Gui::Command::updateActive();
//    }
//}

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

    //defaults to prevent scale changing 
    ui->sbScaleNum->setEnabled(false);
    ui->sbScaleDen->setEnabled(false);

    if(index == 0) {
        // Document Scale Type
        Gui::Command::doCommand(Gui::Command::Doc, "App.activeDocument().%s.ScaleType = '%s'", multiView->getNameInDocument()
                                                                                             , "Page");
    } else if(index == 1) {
        // Automatic Scale Type
//        Gui::Command::doCommand(Gui::Command::Doc, "App.activeDocument().%s.ScaleType = '%s'", multiView->getNameInDocument()
//                                                                                             , "Automatic");
        //block recompute
        multiView->ScaleType.setValue("Automatic");
        double autoScale = multiView->calculateAutomaticScale();
        multiView->Scale.setValue(autoScale);
        //unblock recompute

    } else if(index == 2) {
        // Custom Scale Type
        //block recompute
        Gui::Command::doCommand(Gui::Command::Doc, "App.activeDocument().%s.ScaleType = '%s'", multiView->getNameInDocument()
                                                                                             , "Custom");
        ui->sbScaleNum->setEnabled(true);
        ui->sbScaleDen->setEnabled(true);

        int a = ui->sbScaleNum->value();
        int b = ui->sbScaleDen->value();
        double scale = (double) a / (double) b;
        Gui::Command::doCommand(Gui::Command::Doc, "App.activeDocument().%s.Scale = %f", multiView->getNameInDocument()
                                                                                     , scale);
        //unblock recompute
    } else {
        Base::Console().Log("Error - TaskProjGroup::scaleTypeChanged - unknown scale type: %d\n",index);
        return;
    }

    multiView->recomputeFeature();
}

std::pair<int, int> TaskProjGroup::nearestFraction(const double val, const long int maxDenom) const
{
/*
** find rational approximation to given real number
** David Eppstein / UC Irvine / 8 Aug 1993
**
** With corrections from Arno Formella, May 2008
** and additional fiddles by WF 2017
** usage: a.out r d
**   r is real number to approx
**   d is the maximum denominator allowed
**
** based on the theory of continued fractions
** if x = a1 + 1/(a2 + 1/(a3 + 1/(a4 + ...)))
** then best approximation is found by truncating this series
** (with some adjustments in the last term).
**
** Note the fraction can be recovered as the first column of the matrix
**  ( a1 1 ) ( a2 1 ) ( a3 1 ) ...
**  ( 1  0 ) ( 1  0 ) ( 1  0 )
** Instead of keeping the sequence of continued fraction terms,
** we just keep the last partial product of these matrices.
*/
    std::pair<int, int> result;
    long m[2][2];
    long maxden = maxDenom;
    long ai;
    double x = val;
    double startx = x;

    /* initialize matrix */
    m[0][0] = m[1][1] = 1;
    m[0][1] = m[1][0] = 0;

    /* loop finding terms until denom gets too big */
    while (m[1][0] *  ( ai = (long)x ) + m[1][1] <= maxden) {
        long t;
        t = m[0][0] * ai + m[0][1];
        m[0][1] = m[0][0];
        m[0][0] = t;
        t = m[1][0] * ai + m[1][1];
        m[1][1] = m[1][0];
        m[1][0] = t;
        if(x == (double) ai)
            break;     // AF: division by zero
        x = 1/(x - (double) ai);
        if(x > (double) std::numeric_limits<int>::max())
            break;     // AF: representation failure
    }

    /* now remaining x is between 0 and 1/ai */
    /* approx as either 0 or 1/m where m is max that will fit in maxden */
    /* first try zero */
    double error1 = startx - ((double) m[0][0] / (double) m[1][0]);
    int n1 = m[0][0];
    int d1 = m[1][0];

    /* now try other possibility */
    ai = (maxden - m[1][1]) / m[1][0];
    m[0][0] = m[0][0] * ai + m[0][1];
    m[1][0] = m[1][0] * ai + m[1][1];
    double error2 = startx - ((double) m[0][0] / (double) m[1][0]);
    int n2 = m[0][0];
    int d2 = m[1][0];

    if (std::fabs(error1) <= std::fabs(error2)) {
        result.first  = n1;
        result.second = d1;
    } else {
        result.first  = n2;
        result.second = d2;
    }
    return result;
}

void TaskProjGroup::updateTask()
{
    // Update the scale type
    blockUpdate = true;
    ui->cmbScaleType->setCurrentIndex(multiView->ScaleType.getValue());

    // Update the scale value
    setFractionalScale(multiView->getScale());

    blockUpdate = false;
}


void TaskProjGroup::setFractionalScale(double newScale)
{
    blockUpdate = true;

    std::pair<int, int> fraction = nearestFraction(newScale);

    ui->sbScaleNum->setValue(fraction.first);
    ui->sbScaleDen->setValue(fraction.second);
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
    multiView->recomputeFeature();  //just a repaint.  multiView is already marked for recompute by changed to Scale
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

    if (!getCreateMode())  {    //this is an edit session, end the transaction
        Gui::Command::commitCommand();
    }
    //Gui::Command::updateActive();     //no chain of updates here
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
