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
# include <cmath>
# include <QMessageBox>
#endif // #ifndef _PreComp_

#include <Base/Console.h>
#include <Gui/Application.h>
#include <Gui/BitmapFactory.h>
#include <Gui/Command.h>
#include <Gui/Document.h>
#include <Gui/WaitCursor.h>

#include <Mod/TechDraw/App/DrawPage.h>
#include <Mod/TechDraw/App/DrawProjGroupItem.h>
#include <Mod/TechDraw/App/DrawProjGroup.h>

#include "TaskProjGroup.h"
#include "ui_TaskProjGroup.h"
#include "MDIViewPage.h"
#include "ViewProviderPage.h"
#include "ViewProviderProjGroup.h"


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

    ui->cbAutoDistribute->setChecked(multiView->AutoDistribute.getValue());
    // disable if no AutoDistribute
    ui->sbXSpacing->setEnabled(multiView->AutoDistribute.getValue());
    ui->sbYSpacing->setEnabled(multiView->AutoDistribute.getValue());
    ui->sbXSpacing->setValue(multiView->spacingX.getValue());
    ui->sbYSpacing->setValue(multiView->spacingY.getValue());

    // Initially toggle view checkboxes if needed
    setupViewCheckboxes(true);

    blockUpdate = false;

    // Rotation buttons
    // Note we don't do the custom one here, as it's handled by [a different function that's held up in customs]
    connect(ui->butTopRotate,   &QPushButton::clicked, this, &TaskProjGroup::rotateButtonClicked);
    connect(ui->butCWRotate,    &QPushButton::clicked, this, &TaskProjGroup::rotateButtonClicked);
    connect(ui->butRightRotate, &QPushButton::clicked, this, &TaskProjGroup::rotateButtonClicked);
    connect(ui->butDownRotate,  &QPushButton::clicked, this, &TaskProjGroup::rotateButtonClicked);
    connect(ui->butLeftRotate,  &QPushButton::clicked, this, &TaskProjGroup::rotateButtonClicked);
    connect(ui->butCCWRotate,   &QPushButton::clicked, this, &TaskProjGroup::rotateButtonClicked);

//    //Reset button
//    connect(ui->butReset,   SIGNAL(clicked()), this, SLOT(onResetClicked()));

    // Slot for Scale Type
    connect(ui->cmbScaleType, qOverload<int>(&QComboBox::currentIndexChanged), this, &TaskProjGroup::scaleTypeChanged);
    connect(ui->sbScaleNum,   qOverload<int>(&QSpinBox::valueChanged), this, &TaskProjGroup::scaleManuallyChanged);
    connect(ui->sbScaleDen,   qOverload<int>(&QSpinBox::valueChanged), this, &TaskProjGroup::scaleManuallyChanged);

    // Slot for Projection Type (layout)
#if QT_VERSION < QT_VERSION_CHECK(5,15,0)
    connect(ui->projection, qOverload<const QString&>(&QComboBox::currentIndexChanged), this, &TaskProjGroup::projectionTypeChanged);
#else
    connect(ui->projection, qOverload<int>(&QComboBox::currentIndexChanged), this, [=](int index) {
        projectionTypeChanged(ui->projection->itemText(index));
    });
#endif

    // Spacing
    connect(ui->cbAutoDistribute, &QPushButton::clicked, this, &TaskProjGroup::AutoDistributeClicked);
    connect(ui->sbXSpacing, qOverload<double>(&QuantitySpinBox::valueChanged), this, &TaskProjGroup::spacingChanged);
    connect(ui->sbYSpacing, qOverload<double>(&QuantitySpinBox::valueChanged), this, &TaskProjGroup::spacingChanged);
    ui->sbXSpacing->setUnit(Base::Unit::Length);
    ui->sbYSpacing->setUnit(Base::Unit::Length);

    m_page = multiView->findParentPage();
    Gui::Document* activeGui = Gui::Application::Instance->getDocument(m_page->getDocument());
    Gui::ViewProvider* vp = activeGui->getViewProvider(m_page);
    ViewProviderPage* dvp = static_cast<ViewProviderPage*>(vp);
    m_mdi = dvp->getMDIViewPage();

    setUiPrimary();
    saveGroupState();
}

void TaskProjGroup::saveGroupState()
{
//    Base::Console().Message("TPG::saveGroupState()\n");
    if (!multiView)
        return;

    m_saveSource   = multiView->Source.getValues();
    m_saveProjType = multiView->ProjectionType.getValueAsString();
    m_saveScaleType = multiView->ScaleType.getValueAsString();
    m_saveScale = multiView->Scale.getValue();
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

//never used?
void TaskProjGroup::restoreGroupState()
{
    Base::Console().Message("TPG::restoreGroupState()\n");
    if (!multiView)
        return;

    multiView->ProjectionType.setValue(m_saveProjType.c_str());
    multiView->ScaleType.setValue(m_saveScaleType.c_str());
    multiView->Scale.setValue(m_saveScale);
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

void TaskProjGroup::viewToggled(bool toggle)
{
    Gui::WaitCursor wc;
    bool changed = false;
    // Obtain name of checkbox
    QString viewName = sender()->objectName();
    int index = viewName.mid(7).toInt();
    const char *viewNameCStr = viewChkIndexToCStr(index);
    if ( toggle && !multiView->hasProjection( viewNameCStr ) ) {
        Gui::Command::doCommand(Gui::Command::Doc,
                                "App.activeDocument().%s.addProjection('%s')",
                                multiView->getNameInDocument(), viewNameCStr);
        changed = true;
    } else if ( !toggle && multiView->hasProjection( viewNameCStr ) ) {
        if (multiView->canDelete(viewNameCStr)) {
            multiView->removeProjection( viewNameCStr );
            changed = true;
        }
    }
    if (changed) {
        if (multiView->ScaleType.isValue("Automatic")) {
            double scale = multiView->getScale();
            setFractionalScale(scale);
        }
    }
    wc.restoreCursor();
}

void TaskProjGroup::rotateButtonClicked()
{
    if ( multiView && ui ) {
        const QObject *clicked = sender();

        //change Front View Dir by 90
        if ( clicked == ui->butTopRotate ) multiView->rotate("Up");
        else if (clicked == ui->butDownRotate) multiView->rotate("Down");
        else if (clicked == ui->butRightRotate) multiView->rotate("Right");
        else if (clicked == ui->butLeftRotate) multiView->rotate("Left");
        else if (clicked == ui->butCWRotate ) multiView->spin("CW");
        else if (clicked == ui->butCCWRotate) multiView->spin("CCW");

        setUiPrimary();
    }
}

//void TaskProjGroup::projectionTypeChanged(int index)
void TaskProjGroup::projectionTypeChanged(QString qText)
{
    if(blockUpdate) {
        return;
    }

    if (qText == QString::fromUtf8("Page")) {
        multiView->ProjectionType.setValue("Default");
    } else {
        std::string text = qText.toStdString();
        multiView->ProjectionType.setValue(text.c_str());
    }

    // Update checkboxes so checked state matches the drawing
    setupViewCheckboxes();
    multiView->recomputeFeature();
}

void TaskProjGroup::scaleTypeChanged(int index)
{
    if (blockUpdate)
        return;

    //defaults to prevent scale changing
    ui->sbScaleNum->setEnabled(false);
    ui->sbScaleDen->setEnabled(false);

    if (index == 0) {
        // Document Scale Type
        multiView->ScaleType.setValue("Page");
    } else if (index == 1) {
        // Automatic Scale Type
        //block recompute
        multiView->ScaleType.setValue("Automatic");
        double autoScale = multiView->autoScale();
        multiView->Scale.setValue(autoScale);
        //unblock recompute

    } else if (index == 2) {
        // Custom Scale Type
        //block recompute
        multiView->ScaleType.setValue("Custom");
        ui->sbScaleNum->setEnabled(true);
        ui->sbScaleDen->setEnabled(true);

        int a = ui->sbScaleNum->value();
        int b = ui->sbScaleDen->value();
        double scale = (double) a / (double) b;
        multiView->Scale.setValue(scale);
        //unblock recompute
    }
}

void TaskProjGroup::AutoDistributeClicked(bool clicked)
{
    if (blockUpdate) {
        return;
    }
    multiView->AutoDistribute.setValue(clicked);
    multiView->recomputeFeature();
}

void TaskProjGroup::spacingChanged()
{
    if (blockUpdate) {
        return;
    }
    multiView->spacingX.setValue(ui->sbXSpacing->value().getValue());
    multiView->spacingY.setValue(ui->sbYSpacing->value().getValue());
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

void TaskProjGroup::scaleManuallyChanged(int unused)
{
    Q_UNUSED(unused);
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
    assert (multiView);

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
        default: return nullptr;
    }
}
void TaskProjGroup::setupViewCheckboxes(bool addConnections)
{
    if (!multiView)
        return;

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
            connect(box, &QCheckBox::toggled, this, &TaskProjGroup::viewToggled);
        }

        const char *viewStr = viewChkIndexToCStr(i);
        if (viewStr && multiView->hasProjection(viewStr)) {
            box->setCheckState(Qt::Checked);
            if (!multiView->canDelete(viewStr)) {
                box->setEnabled(false);
            }
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
//    Base::Console().Message("TPG::apply()\n");
    multiView->recomputeChildren();
    multiView->recomputeFeature();

    return true;
}

bool TaskProjGroup::accept()
{
//    Base::Console().Message("TPG::accept()\n");
    Gui::Document* doc = Gui::Application::Instance->getDocument(multiView->getDocument());
    if (!doc)
        return false;

    multiView->recomputeChildren();
    multiView->recomputeFeature();

    Gui::Command::doCommand(Gui::Command::Gui, "Gui.ActiveDocument.resetEdit()");

    return true;
}

bool TaskProjGroup::reject()
{
    Gui::Document* doc = Gui::Application::Instance->getDocument(multiView->getDocument());
    if (!doc)
        return false;

    if (getCreateMode()) {
        //remove the object completely from the document
        std::string multiViewName = multiView->getNameInDocument();
        std::string PageName = multiView->findParentPage()->getNameInDocument();

        Gui::Command::doCommand(Gui::Command::Gui, "App.activeDocument().%s.purgeProjections()",
                                multiViewName.c_str());
        Gui::Command::doCommand(Gui::Command::Gui, "App.activeDocument().%s.removeView(App.activeDocument().%s)",
                                PageName.c_str(), multiViewName.c_str());
        Gui::Command::doCommand(Gui::Command::Gui, "App.activeDocument().removeObject('%s')", multiViewName.c_str());
        Gui::Command::doCommand(Gui::Command::Gui, "Gui.ActiveDocument.resetEdit()");
    } else {
        //set the DPG and it's views back to entry state.
        if (Gui::Command::hasPendingCommand()) {
            Gui::Command::abortCommand();
//            std::vector<std::string> undos = Gui::Application::Instance->activeDocument()->getUndoVector();
//            Gui::Application::Instance->activeDocument()->undo(1);
//            multiView->rebuildViewList();
//            apply();
        }
    }
    Gui::Command::runCommand(Gui::Command::Gui, "Gui.ActiveDocument.resetEdit()");
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
    widget  = new TaskProjGroup(featView, mode);
    taskbox = new Gui::TaskView::TaskBox(Gui::BitmapFactory().pixmap("actions/TechDraw_ProjectionGroup"),
                                         widget->windowTitle(), true, nullptr);
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
        App::GetApplication().setActiveTransaction("Edit Projection Group", true);
    }
}

void TaskDlgProjGroup::clicked(int i)
{
//    Q_UNUSED(i);
//    Base::Console().Message("TDPG::clicked(%X)\n", i);
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


#include <Mod/TechDraw/Gui/moc_TaskProjGroup.cpp>
