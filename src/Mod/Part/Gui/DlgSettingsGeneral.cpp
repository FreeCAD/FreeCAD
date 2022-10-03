/***************************************************************************
 *   Copyright (c) 2007 Werner Mayer <wmayer[at]users.sourceforge.net>     *
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
# include <QButtonGroup>
# include <QRegExp>
# include <QRegExpValidator>
# include <Interface_Static.hxx>
#endif

#include <App/Application.h>
#include <Base/Parameter.h>
#include <Mod/Part/App/ImportStep.h>
#include <Mod/Part/App/Interface.h>

#include "DlgSettingsGeneral.h"
#include "ui_DlgSettingsGeneral.h"
#include "ui_DlgImportExportIges.h"
#include "ui_DlgImportExportStep.h"


using namespace PartGui;

DlgSettingsGeneral::DlgSettingsGeneral(QWidget* parent)
  : PreferencePage(parent), ui(new Ui_DlgSettingsGeneral)
{
    ui->setupUi(this);
}

/**
 *  Destroys the object and frees any allocated resources
 */
DlgSettingsGeneral::~DlgSettingsGeneral()
{
    // no need to delete child widgets, Qt does it all for us
}

void DlgSettingsGeneral::saveSettings()
{
    ui->checkBooleanCheck->onSave();
    ui->checkBooleanRefine->onSave();
    ui->checkSketchBaseRefine->onSave();
    ui->checkObjectNaming->onSave();
}

void DlgSettingsGeneral::loadSettings()
{
    ui->checkBooleanCheck->onRestore();
    ui->checkBooleanRefine->onRestore();
    ui->checkSketchBaseRefine->onRestore();
    ui->checkObjectNaming->onRestore();
}

/**
 * Sets the strings of the subwidgets using the current language.
 */
void DlgSettingsGeneral::changeEvent(QEvent *e)
{
    if (e->type() == QEvent::LanguageChange) {
        ui->retranslateUi(this);
    }
    else {
        QWidget::changeEvent(e);
    }
}

// ----------------------------------------------------------------------------

DlgImportExportIges::DlgImportExportIges(QWidget* parent)
  : PreferencePage(parent), ui(new Ui_DlgImportExportIges)
{
    ui->setupUi(this);
    ui->lineEditProduct->setReadOnly(true);

    bg = new QButtonGroup(this);
    bg->addButton(ui->radioButtonBRepOff, 0);
    bg->addButton(ui->radioButtonBRepOn, 1);

    QRegExp rx;
    rx.setPattern(QString::fromLatin1("[\\x00-\\x7F]+"));
    QRegExpValidator* companyValidator = new QRegExpValidator(ui->lineEditCompany);
    companyValidator->setRegExp(rx);
    ui->lineEditCompany->setValidator(companyValidator);
    QRegExpValidator* authorValidator = new QRegExpValidator(ui->lineEditAuthor);
    authorValidator->setRegExp(rx);
    ui->lineEditAuthor->setValidator(authorValidator);
}

/**
 *  Destroys the object and frees any allocated resources
 */
DlgImportExportIges::~DlgImportExportIges()
{
    // no need to delete child widgets, Qt does it all for us
}

void DlgImportExportIges::saveSettings()
{
    int unit = ui->comboBoxUnits->currentIndex();
    Base::Reference<ParameterGrp> hGrp = App::GetApplication().GetUserParameter()
        .GetGroup("BaseApp")->GetGroup("Preferences")->GetGroup("Mod/Part")->GetGroup("IGES");
    hGrp->SetInt("Unit", unit);
    Part::Interface::writeIgesUnit(static_cast<Part::Interface::Unit>(unit));

    hGrp->SetBool("BrepMode", bg->checkedId() == 1);
    Part::Interface::writeIgesBrepMode(bg->checkedId());

    // Import
    hGrp->SetBool("SkipBlankEntities", ui->checkSkipBlank->isChecked());

    // header info
    hGrp->SetASCII("Company", ui->lineEditCompany->text().toLatin1());
    hGrp->SetASCII("Author", ui->lineEditAuthor->text().toLatin1());

    Part::Interface::writeIgesHeaderAuthor(ui->lineEditAuthor->text().toLatin1());
    Part::Interface::writeIgesHeaderCompany(ui->lineEditCompany->text().toLatin1());
}

void DlgImportExportIges::loadSettings()
{
    Base::Reference<ParameterGrp> hGrp = App::GetApplication().GetUserParameter()
        .GetGroup("BaseApp")->GetGroup("Preferences")->GetGroup("Mod/Part")->GetGroup("IGES");
    int unit = hGrp->GetInt("Unit", 0);
    ui->comboBoxUnits->setCurrentIndex(unit);

    int value = Part::Interface::writeIgesBrepMode();
    bool brep = hGrp->GetBool("BrepMode", value > 0);
    if (brep)
        ui->radioButtonBRepOn->setChecked(true);
    else
        ui->radioButtonBRepOff->setChecked(true);

    // Import
    ui->checkSkipBlank->setChecked(hGrp->GetBool("SkipBlankEntities", true));

    // header info
    ui->lineEditCompany->setText(QString::fromStdString(hGrp->GetASCII("Company",
        Part::Interface::writeIgesHeaderCompany())));
    ui->lineEditAuthor->setText(QString::fromStdString(hGrp->GetASCII("Author",
        Part::Interface::writeIgesHeaderAuthor())));
    ui->lineEditProduct->setText(QString::fromLatin1(
        Part::Interface::writeIgesHeaderProduct()));
}

/**
 * Sets the strings of the subwidgets using the current language.
 */
void DlgImportExportIges::changeEvent(QEvent *e)
{
    if (e->type() == QEvent::LanguageChange) {
        ui->retranslateUi(this);
    }
    else {
        QWidget::changeEvent(e);
    }
}

// ----------------------------------------------------------------------------

DlgImportExportStep::DlgImportExportStep(QWidget* parent)
  : PreferencePage(parent), ui(new Ui_DlgImportExportStep)
{
    ui->setupUi(this);

    ui->comboBoxSchema->setItemData(0, QByteArray("AP203"));
    ui->comboBoxSchema->setItemData(1, QByteArray("AP214CD"));
    ui->comboBoxSchema->setItemData(2, QByteArray("AP214DIS"));
    ui->comboBoxSchema->setItemData(3, QByteArray("AP214IS"));
    ui->comboBoxSchema->setItemData(4, QByteArray("AP242DIS"));

    ui->lineEditProduct->setReadOnly(true);
    //ui->radioButtonAP203->setToolTip(tr("Configuration controlled 3D designs of mechanical parts and assemblies"));
    //ui->radioButtonAP214->setToolTip(tr("Core data for automotive mechanical design processes"));

    // https://tracker.dev.opencascade.org/view.php?id=25654
    ui->checkBoxPcurves->setToolTip(tr("This parameter indicates whether parametric curves (curves in parametric space of surface)\n"
                                       "should be written into the STEP file. This parameter can be set to off in order to minimize\n"
                                       "the size of the resulting STEP file."));

    QRegExp rx;
    rx.setPattern(QString::fromLatin1("[\\x00-\\x7F]+"));
    QRegExpValidator* companyValidator = new QRegExpValidator(ui->lineEditCompany);
    companyValidator->setRegExp(rx);
    ui->lineEditCompany->setValidator(companyValidator);
    QRegExpValidator* authorValidator = new QRegExpValidator(ui->lineEditAuthor);
    authorValidator->setRegExp(rx);
    ui->lineEditAuthor->setValidator(authorValidator);

    Part::ImportExportSettings settings;
    ui->checkBoxMergeCompound->setChecked(settings.getReadShapeCompoundMode());
    ui->checkBoxExportHiddenObj->setChecked(settings.getExportHiddenObject());
    ui->checkBoxImportHiddenObj->setChecked(settings.getImportHiddenObject());
    ui->checkBoxExportLegacy->setChecked(settings.getExportLegacy());
    ui->checkBoxKeepPlacement->setChecked(settings.getExportKeepPlacement());
    ui->checkBoxUseLinkGroup->setChecked(settings.getUseLinkGroup());
    ui->checkBoxUseBaseName->setChecked(settings.getUseBaseName());
    ui->checkBoxReduceObjects->setChecked(settings.getReduceObjects());
    ui->checkBoxExpandCompound->setChecked(settings.getExpandCompound());
    ui->checkBoxShowProgress->setChecked(settings.getShowProgress());
}

/**
 *  Destroys the object and frees any allocated resources
 */
DlgImportExportStep::~DlgImportExportStep()
{
    // no need to delete child widgets, Qt does it all for us
}

void DlgImportExportStep::saveSettings()
{
    // General
    Part::ImportExportSettings settings;
    settings.setWriteSurfaceCurveMode(ui->checkBoxPcurves->isChecked());

    // STEP
    int unit = ui->comboBoxUnits->currentIndex();
    settings.setUnit(static_cast<Part::ImportExportSettings::Unit>(unit));

    // scheme
    // possible values: AP203, AP214CD (1996), AP214DIS (1998), AP214IS (2002), AP242DIS
    QByteArray schema = ui->comboBoxSchema->itemData(ui->comboBoxSchema->currentIndex()).toByteArray();
    settings.setScheme(schema);

    // header info
    settings.setCompany(ui->lineEditCompany->text().toLatin1());
    settings.setAuthor(ui->lineEditAuthor->text().toLatin1());

    // (h)STEP of Import module
    ui->checkBoxMergeCompound->onSave();
    ui->checkBoxExportHiddenObj->onSave();
    ui->checkBoxExportLegacy->onSave();
    ui->checkBoxKeepPlacement->onSave();
    ui->checkBoxImportHiddenObj->onSave();
    ui->checkBoxUseLinkGroup->onSave();
    ui->checkBoxUseBaseName->onSave();
    ui->checkBoxReduceObjects->onSave();
    ui->checkBoxExpandCompound->onSave();
    ui->checkBoxShowProgress->onSave();
    ui->comboBoxImportMode->onSave();
}

void DlgImportExportStep::loadSettings()
{
    // General
    Part::ImportExportSettings settings;
    ui->checkBoxPcurves->setChecked(settings.getWriteSurfaceCurveMode());

    // STEP
    ui->comboBoxUnits->setCurrentIndex(static_cast<int>(settings.getUnit()));

    // scheme
    QByteArray ap(settings.getScheme().c_str());
    int index = ui->comboBoxSchema->findData(QVariant(ap));
    if (index >= 0)
        ui->comboBoxSchema->setCurrentIndex(index);

    // header info
    ui->lineEditCompany->setText(QString::fromStdString(settings.getCompany()));
    ui->lineEditAuthor->setText(QString::fromStdString(settings.getAuthor()));
    ui->lineEditProduct->setText(QString::fromStdString(settings.getProductName()));

    // (h)STEP of Import module
    ui->checkBoxMergeCompound->onRestore();
    ui->checkBoxExportHiddenObj->onRestore();
    ui->checkBoxExportLegacy->onRestore();
    ui->checkBoxKeepPlacement->onRestore();
    ui->checkBoxImportHiddenObj->onRestore();
    ui->checkBoxUseLinkGroup->onRestore();
    ui->checkBoxUseBaseName->onRestore();
    ui->checkBoxReduceObjects->onRestore();
    ui->checkBoxExpandCompound->onRestore();
    ui->checkBoxShowProgress->onRestore();
    ui->comboBoxImportMode->onRestore();
}

/**
 * Sets the strings of the subwidgets using the current language.
 */
void DlgImportExportStep::changeEvent(QEvent *e)
{
    if (e->type() == QEvent::LanguageChange) {
        ui->retranslateUi(this);
    }
    else {
        QWidget::changeEvent(e);
    }
}

#include "moc_DlgSettingsGeneral.cpp"
