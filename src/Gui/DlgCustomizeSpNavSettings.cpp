/***************************************************************************
 *   Copyright (c) 2012 Petar Perisin <petar.perisin@gmail.com>            *
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

#include "DlgCustomizeSpNavSettings.h"
#include "ui_DlgCustomizeSpNavSettings.h"
#include "Application.h"
#include "GuiApplicationNativeEventAware.h"


using namespace Gui::Dialog;

DlgCustomizeSpNavSettings::DlgCustomizeSpNavSettings(QWidget *parent) :
    CustomizeActionPage(parent)
  , ui(new Ui_DlgCustomizeSpNavSettings)
  , init(false)
{
    auto app = qobject_cast<GUIApplicationNativeEventAware *>(QApplication::instance());

    if (!app)
        return;
    if (!app->isSpaceballPresent())
    {
        this->setWindowTitle(tr("Spaceball Motion"));
        this->setMessage(tr("No Spaceball Present"));
        return;
    }
    this->init = true;
    ui->setupUi(this);
    setupConnections();
    initialize();
}

DlgCustomizeSpNavSettings::~DlgCustomizeSpNavSettings() = default;

void DlgCustomizeSpNavSettings::setupConnections()
{
    connect(ui->CBDominant, &QCheckBox::clicked,
            this, &DlgCustomizeSpNavSettings::on_CBDominant_clicked);
    connect(ui->CBFlipYZ, &QCheckBox::clicked,
            this, &DlgCustomizeSpNavSettings::on_CBFlipYZ_clicked);
    connect(ui->CBRotations, &QCheckBox::clicked,
            this, &DlgCustomizeSpNavSettings::on_CBRotations_clicked);
    connect(ui->CBTranslations, &QCheckBox::clicked,
            this, &DlgCustomizeSpNavSettings::on_CBTranslations_clicked);
    connect(ui->SliderGlobal, &QSlider::sliderReleased,
            this, &DlgCustomizeSpNavSettings::on_SliderGlobal_sliderReleased);
    connect(ui->CBEnablePanLR, &QCheckBox::clicked,
            this, &DlgCustomizeSpNavSettings::on_CBEnablePanLR_clicked);
    connect(ui->CBReversePanLR, &QCheckBox::clicked,
            this, &DlgCustomizeSpNavSettings::on_CBReversePanLR_clicked);
    connect(ui->SliderPanLR, &QSlider::sliderReleased,
            this, &DlgCustomizeSpNavSettings::on_SliderPanLR_sliderReleased);
    connect(ui->CBEnablePanUD, &QCheckBox::clicked,
            this, &DlgCustomizeSpNavSettings::on_CBEnablePanUD_clicked);
    connect(ui->CBReversePanUD, &QCheckBox::clicked,
            this, &DlgCustomizeSpNavSettings::on_CBReversePanUD_clicked);
    connect(ui->SliderPanUD, &QSlider::sliderReleased,
            this, &DlgCustomizeSpNavSettings::on_SliderPanUD_sliderReleased);
    connect(ui->CBEnableZoom, &QCheckBox::clicked,
            this, &DlgCustomizeSpNavSettings::on_CBEnableZoom_clicked);
    connect(ui->CBReverseZoom, &QCheckBox::clicked,
            this, &DlgCustomizeSpNavSettings::on_CBReverseZoom_clicked);
    connect(ui->SliderZoom, &QSlider::sliderReleased,
            this, &DlgCustomizeSpNavSettings::on_SliderZoom_sliderReleased);
    connect(ui->CBEnableTilt, &QCheckBox::clicked,
            this, &DlgCustomizeSpNavSettings::on_CBEnableTilt_clicked);
    connect(ui->CBReverseTilt, &QCheckBox::clicked,
            this, &DlgCustomizeSpNavSettings::on_CBReverseTilt_clicked);
    connect(ui->SliderTilt, &QSlider::sliderReleased,
            this, &DlgCustomizeSpNavSettings::on_SliderTilt_sliderReleased);
    connect(ui->CBEnableRoll, &QCheckBox::clicked,
            this, &DlgCustomizeSpNavSettings::on_CBEnableRoll_clicked);
    connect(ui->CBReverseRoll, &QCheckBox::clicked,
            this, &DlgCustomizeSpNavSettings::on_CBReverseRoll_clicked);
    connect(ui->SliderRoll, &QSlider::sliderReleased,
            this, &DlgCustomizeSpNavSettings::on_SliderRoll_sliderReleased);
    connect(ui->CBEnableSpin, &QCheckBox::clicked,
            this, &DlgCustomizeSpNavSettings::on_CBEnableSpin_clicked);
    connect(ui->CBReverseSpin, &QCheckBox::clicked,
            this, &DlgCustomizeSpNavSettings::on_CBReverseSpin_clicked);
    connect(ui->SliderSpin, &QSlider::sliderReleased,
            this, &DlgCustomizeSpNavSettings::on_SliderSpin_sliderReleased);
    connect(ui->ButtonDefaultSpNavMotions, &QPushButton::clicked,
            this, &DlgCustomizeSpNavSettings::on_ButtonDefaultSpNavMotions_clicked);
    connect(ui->ButtonCalibrate, &QPushButton::clicked,
            this, &DlgCustomizeSpNavSettings::on_ButtonCalibrate_clicked);
}

void DlgCustomizeSpNavSettings::setMessage(const QString& message)
{
    auto messageLabel = new QLabel(message,this);
    auto layout = new QVBoxLayout();
    auto layout2 = new QHBoxLayout();
    layout2->addStretch();
    layout2->addWidget(messageLabel);
    layout2->addStretch();
    layout->addItem(layout2);
    this->setLayout(layout);
}

void DlgCustomizeSpNavSettings::changeEvent(QEvent *e)
{
    if (e->type() == QEvent::LanguageChange) {
        if (this->init) {
            ui->retranslateUi(this);
        }
        else {
            this->setWindowTitle(tr("Spaceball Motion"));
            QLabel *messageLabel = this->findChild<QLabel*>();
            if (messageLabel) messageLabel->setText(tr("No Spaceball Present"));
        }
    }
    QWidget::changeEvent(e);
}

ParameterGrp::handle DlgCustomizeSpNavSettings::spaceballMotionGroup() const
{
    static ParameterGrp::handle group = App::GetApplication().GetUserParameter().GetGroup("BaseApp")->GetGroup("Spaceball")->GetGroup("Motion");
    return group;
}

void DlgCustomizeSpNavSettings::on_ButtonCalibrate_clicked()
{
    spaceballMotionGroup()->SetBool("Calibrate", true);
}

void DlgCustomizeSpNavSettings::initialize()
{
    ui->CBDominant->setChecked(spaceballMotionGroup()->GetBool("Dominant", false));
    ui->CBFlipYZ->setChecked(spaceballMotionGroup()->GetBool("FlipYZ", false));
    ui->CBRotations->setChecked(spaceballMotionGroup()->GetBool("Rotations", true));
    ui->CBTranslations->setChecked(spaceballMotionGroup()->GetBool("Translations", true));
    ui->SliderGlobal->setValue(spaceballMotionGroup()->GetInt("GlobalSensitivity", 0));

    ui->CBEnablePanLR ->setChecked(spaceballMotionGroup()->GetBool("PanLREnable", true));
    ui->CBReversePanLR->setChecked(spaceballMotionGroup()->GetBool("PanLRReverse", false));
    ui->SliderPanLR   ->setValue(spaceballMotionGroup()->GetInt("PanLRSensitivity", 0));

    ui->CBEnablePanUD ->setChecked(spaceballMotionGroup()->GetBool("PanUDEnable", true));
    ui->CBReversePanUD->setChecked(spaceballMotionGroup()->GetBool("PanUDReverse", false));
    ui->SliderPanUD   ->setValue(spaceballMotionGroup()->GetInt("PanUDSensitivity", 0));

    ui->CBEnableZoom ->setChecked(spaceballMotionGroup()->GetBool("ZoomEnable", true));
    ui->CBReverseZoom->setChecked(spaceballMotionGroup()->GetBool("ZoomReverse", false));
    ui->SliderZoom   ->setValue(spaceballMotionGroup()->GetInt("ZoomSensitivity", 0));

    ui->CBEnableTilt ->setChecked(spaceballMotionGroup()->GetBool("TiltEnable", true));
    ui->CBReverseTilt->setChecked(spaceballMotionGroup()->GetBool("TiltReverse", false));
    ui->SliderTilt   ->setValue(spaceballMotionGroup()->GetInt("TiltSensitivity", 0));

    ui->CBEnableRoll ->setChecked(spaceballMotionGroup()->GetBool("RollEnable", true));
    ui->CBReverseRoll->setChecked(spaceballMotionGroup()->GetBool("RollReverse", false));
    ui->SliderRoll   ->setValue(spaceballMotionGroup()->GetInt("RollSensitivity", 0));

    ui->CBEnableSpin ->setChecked(spaceballMotionGroup()->GetBool("SpinEnable", true));
    ui->CBReverseSpin->setChecked(spaceballMotionGroup()->GetBool("SpinReverse", false));
    ui->SliderSpin   ->setValue(spaceballMotionGroup()->GetInt("SpinSensitivity", 0));

    ui->CBEnableTilt ->setEnabled(ui->CBRotations->isChecked());
    ui->CBReverseTilt->setEnabled(ui->CBRotations->isChecked() && ui->CBEnableTilt->isChecked());
    ui->SliderTilt   ->setEnabled(ui->CBRotations->isChecked() && ui->CBEnableTilt->isChecked());
    ui->CBEnableRoll ->setEnabled(ui->CBRotations->isChecked());
    ui->CBReverseRoll->setEnabled(ui->CBRotations->isChecked() && ui->CBEnableRoll->isChecked());
    ui->SliderRoll   ->setEnabled(ui->CBRotations->isChecked() && ui->CBEnableRoll->isChecked());
    ui->CBEnableSpin ->setEnabled(ui->CBRotations->isChecked());
    ui->CBReverseSpin->setEnabled(ui->CBRotations->isChecked() && ui->CBEnableSpin->isChecked());
    ui->SliderSpin   ->setEnabled(ui->CBRotations->isChecked() && ui->CBEnableSpin->isChecked());

    ui->CBEnablePanLR ->setEnabled(ui->CBTranslations->isChecked());
    ui->CBReversePanLR->setEnabled(ui->CBTranslations->isChecked() && ui->CBEnablePanLR->isChecked());
    ui->SliderPanLR   ->setEnabled(ui->CBTranslations->isChecked() && ui->CBEnablePanLR->isChecked());
    ui->CBEnablePanUD ->setEnabled(ui->CBTranslations->isChecked());
    ui->CBReversePanUD->setEnabled(ui->CBTranslations->isChecked() && ui->CBEnablePanUD->isChecked());
    ui->SliderPanUD   ->setEnabled(ui->CBTranslations->isChecked() && ui->CBEnablePanUD->isChecked());
    ui->CBEnableZoom  ->setEnabled(ui->CBTranslations->isChecked());
    ui->CBReverseZoom ->setEnabled(ui->CBTranslations->isChecked() && ui->CBEnableZoom->isChecked());
    ui->SliderZoom    ->setEnabled(ui->CBTranslations->isChecked() && ui->CBEnableZoom->isChecked());
}

void DlgCustomizeSpNavSettings::on_ButtonDefaultSpNavMotions_clicked()
{
    spaceballMotionGroup()->Clear();
    initialize();
}

void DlgCustomizeSpNavSettings::on_CBDominant_clicked()
{
    spaceballMotionGroup()->SetBool("Dominant", ui->CBDominant->isChecked());
}

void DlgCustomizeSpNavSettings::on_CBFlipYZ_clicked()
{
    spaceballMotionGroup()->SetBool("FlipYZ", ui->CBFlipYZ->isChecked());
}

void DlgCustomizeSpNavSettings::on_CBRotations_clicked()
{
    spaceballMotionGroup()->SetBool("Rotations", ui->CBRotations->isChecked());

    ui->CBEnableTilt ->setEnabled(ui->CBRotations->isChecked());
    ui->CBReverseTilt->setEnabled(ui->CBRotations->isChecked() && ui->CBEnableTilt->isChecked());
    ui->SliderTilt   ->setEnabled(ui->CBRotations->isChecked() && ui->CBEnableTilt->isChecked());
    ui->CBEnableRoll ->setEnabled(ui->CBRotations->isChecked());
    ui->CBReverseRoll->setEnabled(ui->CBRotations->isChecked() && ui->CBEnableRoll->isChecked());
    ui->SliderRoll   ->setEnabled(ui->CBRotations->isChecked() && ui->CBEnableRoll->isChecked());
    ui->CBEnableSpin ->setEnabled(ui->CBRotations->isChecked());
    ui->CBReverseSpin->setEnabled(ui->CBRotations->isChecked() && ui->CBEnableSpin->isChecked());
    ui->SliderSpin   ->setEnabled(ui->CBRotations->isChecked() && ui->CBEnableSpin->isChecked());
}

void DlgCustomizeSpNavSettings::on_CBTranslations_clicked()
{
    spaceballMotionGroup()->SetBool("Translations", ui->CBTranslations->isChecked());

    ui->CBEnablePanLR ->setEnabled(ui->CBTranslations->isChecked());
    ui->CBReversePanLR->setEnabled(ui->CBTranslations->isChecked() && ui->CBEnablePanLR->isChecked());
    ui->SliderPanLR   ->setEnabled(ui->CBTranslations->isChecked() && ui->CBEnablePanLR->isChecked());
    ui->CBEnablePanUD ->setEnabled(ui->CBTranslations->isChecked());
    ui->CBReversePanUD->setEnabled(ui->CBTranslations->isChecked() && ui->CBEnablePanUD->isChecked());
    ui->SliderPanUD   ->setEnabled(ui->CBTranslations->isChecked() && ui->CBEnablePanUD->isChecked());
    ui->CBEnableZoom  ->setEnabled(ui->CBTranslations->isChecked());
    ui->CBReverseZoom ->setEnabled(ui->CBTranslations->isChecked() && ui->CBEnableZoom->isChecked());
    ui->SliderZoom    ->setEnabled(ui->CBTranslations->isChecked() && ui->CBEnableZoom->isChecked());
}

void DlgCustomizeSpNavSettings::on_SliderGlobal_sliderReleased()
{
    spaceballMotionGroup()->SetInt("GlobalSensitivity", ui->SliderGlobal->value());
}

void DlgCustomizeSpNavSettings::on_CBEnablePanLR_clicked()
{
    spaceballMotionGroup()->SetBool("PanLREnable", ui->CBEnablePanLR->isChecked());

    ui->CBReversePanLR->setEnabled(ui->CBEnablePanLR->isChecked());
    ui->SliderPanLR   ->setEnabled(ui->CBEnablePanLR->isChecked());
}

void DlgCustomizeSpNavSettings::on_CBReversePanLR_clicked()
{
    spaceballMotionGroup()->SetBool("PanLRReverse", ui->CBReversePanLR->isChecked());
}

void DlgCustomizeSpNavSettings::on_SliderPanLR_sliderReleased()
{
    spaceballMotionGroup()->SetInt("PanLRSensitivity", ui->SliderPanLR->value());
}

void DlgCustomizeSpNavSettings::on_CBEnablePanUD_clicked()
{
    spaceballMotionGroup()->SetBool("PanUDEnable", ui->CBEnablePanUD->isChecked());

    ui->CBReversePanUD->setEnabled(ui->CBEnablePanUD->isChecked());
    ui->SliderPanUD   ->setEnabled(ui->CBEnablePanUD->isChecked());
}

void DlgCustomizeSpNavSettings::on_CBReversePanUD_clicked()
{
    spaceballMotionGroup()->SetBool("PanUDReverse", ui->CBReversePanUD->isChecked());
}

void DlgCustomizeSpNavSettings::on_SliderPanUD_sliderReleased()
{
    spaceballMotionGroup()->SetInt("PanUDSensitivity", ui->SliderPanUD->value());
}

void DlgCustomizeSpNavSettings::on_CBEnableZoom_clicked()
{
    spaceballMotionGroup()->SetBool("ZoomEnable", ui->CBEnableZoom->isChecked());

    ui->CBReverseZoom ->setEnabled(ui->CBEnableZoom->isChecked());
    ui->SliderZoom    ->setEnabled(ui->CBEnableZoom->isChecked());
}

void DlgCustomizeSpNavSettings::on_CBReverseZoom_clicked()
{
    spaceballMotionGroup()->SetBool("ZoomReverse", ui->CBReverseZoom->isChecked());
}

void DlgCustomizeSpNavSettings::on_SliderZoom_sliderReleased()
{
    spaceballMotionGroup()->SetInt("ZoomSensitivity", ui->SliderZoom->value());
}

void DlgCustomizeSpNavSettings::on_CBEnableTilt_clicked()
{
    spaceballMotionGroup()->SetBool("TiltEnable", ui->CBEnableTilt->isChecked());

    ui->CBReverseTilt->setEnabled(ui->CBEnableTilt->isChecked());
    ui->SliderTilt   ->setEnabled(ui->CBEnableTilt->isChecked());
}

void DlgCustomizeSpNavSettings::on_CBReverseTilt_clicked()
{
    spaceballMotionGroup()->SetBool("TiltReverse", ui->CBReverseTilt->isChecked());
}

void DlgCustomizeSpNavSettings::on_SliderTilt_sliderReleased()
{
    spaceballMotionGroup()->SetInt("TiltSensitivity", ui->SliderTilt->value());
}

void DlgCustomizeSpNavSettings::on_CBEnableRoll_clicked()
{
    spaceballMotionGroup()->SetBool("RollEnable", ui->CBEnableRoll->isChecked());

    ui->CBReverseRoll->setEnabled(ui->CBEnableRoll->isChecked());
    ui->SliderRoll   ->setEnabled(ui->CBEnableRoll->isChecked());
}

void DlgCustomizeSpNavSettings::on_CBReverseRoll_clicked()
{
    spaceballMotionGroup()->SetBool("RollReverse", ui->CBReverseRoll->isChecked());
}

void DlgCustomizeSpNavSettings::on_SliderRoll_sliderReleased()
{
    spaceballMotionGroup()->SetInt("RollSensitivity", ui->SliderRoll->value());
}

void DlgCustomizeSpNavSettings::on_CBEnableSpin_clicked()
{
    spaceballMotionGroup()->SetBool("SpinEnable", ui->CBEnableSpin->isChecked());

    ui->CBReverseSpin->setEnabled(ui->CBEnableSpin->isChecked());
    ui->SliderSpin   ->setEnabled(ui->CBEnableSpin->isChecked());
}

void DlgCustomizeSpNavSettings::on_CBReverseSpin_clicked()
{
    spaceballMotionGroup()->SetBool("SpinReverse", ui->CBReverseSpin->isChecked());
}

void DlgCustomizeSpNavSettings::on_SliderSpin_sliderReleased()
{
    spaceballMotionGroup()->SetInt("SpinSensitivity", ui->SliderSpin->value());
}

void DlgCustomizeSpNavSettings::onAddMacroAction(const QByteArray &macroName)
{
    //don't need to do anything here.
    Q_UNUSED(macroName);
}

void DlgCustomizeSpNavSettings::onRemoveMacroAction(const QByteArray &macroName)
{
    //don't need to do anything here.
    Q_UNUSED(macroName);
}

void DlgCustomizeSpNavSettings::onModifyMacroAction(const QByteArray &macroName)
{
    //don't need to do anything here.
    Q_UNUSED(macroName);
}

#include "moc_DlgCustomizeSpNavSettings.cpp"
