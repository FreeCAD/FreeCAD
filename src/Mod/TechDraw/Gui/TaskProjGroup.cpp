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

#include <Mod/Part/App/PartFeature.h>

#include <Mod/TechDraw/App/DrawPage.h>
#include <Mod/TechDraw/App/DrawViewPart.h>

#include <Mod/TechDraw/App/DrawProjGroupItem.h>
#include <Mod/TechDraw/App/DrawProjGroup.h>

#include "TaskProjGroup.h"
#include "ui_TaskProjGroup.h"

using namespace Gui;
using namespace TechDrawGui;

//TODO: Look into this, seems we might be able to delete it now?  IR
#if 0 // needed for Qt's lupdate utility
    qApp->translate("QObject", "Make axonometric...");
    qApp->translate("QObject", "Edit axonometric settings...");
    qApp->translate("QObject", "Make orthographic");
#endif


TaskProjGroup::TaskProjGroup(TechDraw::DrawProjGroup* featView) : ui(new Ui_TaskProjGroup),
                                                                                           multiView(featView)
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

    // Slot for Scale Type
    connect(ui->cmbScaleType, SIGNAL(currentIndexChanged(int)), this, SLOT(scaleTypeChanged(int)));
    connect(ui->scaleNum,     SIGNAL(textEdited(const QString &)), this, SLOT(scaleManuallyChanged(const QString &)));
    connect(ui->scaleDenom,   SIGNAL(textEdited(const QString &)), this, SLOT(scaleManuallyChanged(const QString &)));

    // Slot for Projection Type (layout)
    connect(ui->projection, SIGNAL(currentIndexChanged(int)), this, SLOT(projectionTypeChanged(int)));
}

TaskProjGroup::~TaskProjGroup()
{
    delete ui;
}

void TaskProjGroup::viewToggled(bool toggle)
{
    // Obtain name of checkbox
    QString viewName = sender()->objectName();
    int index = viewName.mid(7).toInt();
    const char *viewNameCStr = viewChkIndexToCStr(index);

    //Gui::Command::openCommand("Toggle orthographic view");    //TODO: Is this for undo?

    if ( toggle && !multiView->hasProjection( viewNameCStr ) ) {
        multiView->addProjection( viewNameCStr );
    } else if ( !toggle && multiView->hasProjection( viewNameCStr ) ) {
        multiView->removeProjection( viewNameCStr );
    }

    /// Called to notify the GUI that the scale has changed
    Gui::Command::commitCommand();
    Gui::Command::updateActive();
}

void TaskProjGroup::rotateButtonClicked(void)
{
    if ( multiView && ui ) {
        const QObject *clicked = sender();

        // Any translation/scale/etc applied here will be ignored, as
        // DrawProjGroup::setFrontViewOrientation() only
        // uses it to set Direction and XAxisDirection.
        Base::Matrix4D m = multiView->viewOrientationMatrix.getValue();

        // TODO: Construct these directly
        Base::Matrix4D t;

        //TODO: Consider changing the vectors around depending on whether we're in First or Third angle mode - might be more intuitive? IR
        if ( clicked == ui->butTopRotate ) {
            t.rotX(M_PI / -2);
        } else if ( clicked == ui->butCWRotate ) {
            t.rotY(M_PI / -2);
        } else if ( clicked == ui->butRightRotate) {
            t.rotZ(M_PI / 2);
        } else if ( clicked == ui->butDownRotate) {
            t.rotX(M_PI / 2);
        } else if ( clicked == ui->butLeftRotate) {
            t.rotZ(M_PI / -2);
        } else if ( clicked == ui->butCCWRotate) {
            t.rotY(M_PI / 2);
        }
        m *= t;

        multiView->setFrontViewOrientation(m);
        Gui::Command::updateActive();
    }
}

void TaskProjGroup::projectionTypeChanged(int index)
{
    if(blockUpdate)
        return;

    Gui::Command::openCommand("Update projection type");
    if(index == 0) {
        //layout per Page (Document)
        Gui::Command::doCommand(Gui::Command::Doc,
                                "App.activeDocument().%s.ProjectionType = '%s'",
                                multiView->getNameInDocument(), "Document");
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
        Gui::Command::abortCommand();
        Base::Console().Log("Error - TaskProjGroup::projectionTypeChanged - unknown projection layout: %d\n",
                            index);
        return;
    }

    // Update checkboxes so checked state matches the drawing
    setupViewCheckboxes();

    Gui::Command::commitCommand();
    Gui::Command::updateActive();
}

void TaskProjGroup::scaleTypeChanged(int index)
{
    if(blockUpdate)
        return;

    Gui::Command::openCommand("Update projection scale type");
    if(index == 0) {
        //Automatic Scale Type
        Gui::Command::doCommand(Gui::Command::Doc, "App.activeDocument().%s.ScaleType = '%s'", multiView->getNameInDocument()
                                                                                             , "Document");
    } else if(index == 1) {
        // Document Scale Type
        Gui::Command::doCommand(Gui::Command::Doc, "App.activeDocument().%s.ScaleType = '%s'", multiView->getNameInDocument()
                                                                                             , "Automatic");
    } else if(index == 2) {
        // Custom Scale Type
        Gui::Command::doCommand(Gui::Command::Doc, "App.activeDocument().%s.ScaleType = '%s'", multiView->getNameInDocument()
                                                                                             , "Custom");
    } else {
        Gui::Command::abortCommand();
        Base::Console().Log("Error - TaskProjGroup::scaleTypeChanged - unknown scale type: %d\n",index);
        return;
    }
    Gui::Command::commitCommand();
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

    ui->scaleNum->setText(QString::number(num));
    ui->scaleDenom->setText(QString::number(den));
    blockUpdate = false;
}

void TaskProjGroup::scaleManuallyChanged(const QString & text)
{
    //TODO: See what this is about - shouldn't be simplifying the scale ratio while it's being edited... IR
    if(blockUpdate)
        return;

    bool ok1, ok2;

    int a = ui->scaleNum->text().toInt(&ok1);
    int b = ui->scaleDenom->text().toInt(&ok2);

    double scale = (double) a / (double) b;
    if (ok1 && ok2) {
        // If we were not in Custom, switch to Custom in two steps
        bool switchToCustom = (strcmp(multiView->ScaleType.getValueAsString(), "Custom") != 0);
        if(switchToCustom) {
            // First, send out command to put us into custom scale
            scaleTypeChanged(ui->cmbScaleType->findText(QString::fromLatin1("Custom")));
            switchToCustom = true;
        }

        Gui::Command::openCommand("Update custom scale");
        Gui::Command::doCommand(Gui::Command::Doc, "App.activeDocument().%s.Scale = %f", multiView->getNameInDocument()
                                                                                         , scale);
        Gui::Command::commitCommand();
        Gui::Command::updateActive();

        if(switchToCustom) {
            // Second, update the GUI
            ui->cmbScaleType->setCurrentIndex(ui->cmbScaleType->findText(QString::fromLatin1("Custom")));
        }
    }
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

/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//TODO: Do we really need to hang on to the TaskDlgProjGroup in this class? IR
TaskDlgProjGroup::TaskDlgProjGroup(TechDraw::DrawProjGroup* featView) : TaskDialog(),
                                                                                                 multiView(featView)
{
    viewProvider = dynamic_cast<const ViewProviderProjGroup *>(featView);
    widget  = new TaskProjGroup(featView);
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

//==== calls from the TaskView ===============================================================
void TaskDlgProjGroup::open()
{
}

void TaskDlgProjGroup::clicked(int)
{
}

bool TaskDlgProjGroup::accept()
{
    return true;//!widget->user_input();
}

bool TaskDlgProjGroup::reject()
{
    return true;
}


#include "moc_TaskProjGroup.cpp"
