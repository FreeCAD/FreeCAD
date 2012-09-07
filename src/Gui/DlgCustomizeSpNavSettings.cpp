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
#include "GuiApplicationNativeEventAware.h"
#include "Application.h"


using namespace Gui::Dialog;

DlgCustomizeSpNavSettings::DlgCustomizeSpNavSettings(QWidget *parent) :
    CustomizeActionPage(parent)
{
    GUIApplicationNativeEventAware *app = qobject_cast<GUIApplicationNativeEventAware *>(QApplication::instance());

    if (!app)
        return;
    if (!app->isSpaceballPresent())
    {
        this->setWindowTitle(tr("Spaceball Motion"));
        this->setMessage(tr("No Spaceball Present"));
        return;
    }
    this->setupUi(this);
    initialize();
}

DlgCustomizeSpNavSettings::~DlgCustomizeSpNavSettings()
{
}

void DlgCustomizeSpNavSettings::setMessage(const QString& message)
{
    QLabel *messageLabel = new QLabel(message,this);
    QVBoxLayout *layout = new QVBoxLayout();
    QHBoxLayout *layout2 = new QHBoxLayout();
    layout2->addStretch();
    layout2->addWidget(messageLabel);
    layout2->addStretch();
    layout->addItem(layout2);
    this->setLayout(layout);
}

void DlgCustomizeSpNavSettings::changeEvent(QEvent *e)
{
    if (e->type() == QEvent::LanguageChange) {
        this->retranslateUi(this);
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
    CBDominant->setChecked(spaceballMotionGroup()->GetBool("Dominant", false));
    CBFlipYZ->setChecked(spaceballMotionGroup()->GetBool("FlipYZ", false));
    CBRotations->setChecked(spaceballMotionGroup()->GetBool("Rotations", true));
    CBTranslations->setChecked(spaceballMotionGroup()->GetBool("Translations", true));
    SliderGlobal->setValue(spaceballMotionGroup()->GetInt("GlobalSensitivity", 0));

    CBEnablePanLR ->setChecked(spaceballMotionGroup()->GetBool("PanLREnable", true));
    CBReversePanLR->setChecked(spaceballMotionGroup()->GetBool("PanLRReverse", false));
    SliderPanLR   ->setValue(spaceballMotionGroup()->GetInt("PanLRSensitivity", 0));

    CBEnablePanUD ->setChecked(spaceballMotionGroup()->GetBool("PanUDEnable", true));
    CBReversePanUD->setChecked(spaceballMotionGroup()->GetBool("PanUDReverse", false));
    SliderPanUD   ->setValue(spaceballMotionGroup()->GetInt("PanUDSensitivity", 0));

    CBEnableZoom ->setChecked(spaceballMotionGroup()->GetBool("ZoomEnable", true));
    CBReverseZoom->setChecked(spaceballMotionGroup()->GetBool("ZoomReverse", false));
    SliderZoom   ->setValue(spaceballMotionGroup()->GetInt("ZoomSensitivity", 0));

    CBEnableTilt ->setChecked(spaceballMotionGroup()->GetBool("TiltEnable", true));
    CBReverseTilt->setChecked(spaceballMotionGroup()->GetBool("TiltReverse", false));
    SliderTilt   ->setValue(spaceballMotionGroup()->GetInt("TiltSensitivity", 0));

    CBEnableRoll ->setChecked(spaceballMotionGroup()->GetBool("RollEnable", true));
    CBReverseRoll->setChecked(spaceballMotionGroup()->GetBool("RollReverse", false));
    SliderRoll   ->setValue(spaceballMotionGroup()->GetInt("RollSensitivity", 0));

    CBEnableSpin ->setChecked(spaceballMotionGroup()->GetBool("SpinEnable", true));
    CBReverseSpin->setChecked(spaceballMotionGroup()->GetBool("SpinReverse", false));
    SliderSpin   ->setValue(spaceballMotionGroup()->GetInt("SpinSensitivity", 0)); 

    CBEnableTilt ->setEnabled(CBRotations->isChecked());
    CBReverseTilt->setEnabled(CBRotations->isChecked() && CBEnableTilt->isChecked()); 
    SliderTilt   ->setEnabled(CBRotations->isChecked() && CBEnableTilt->isChecked());
    CBEnableRoll ->setEnabled(CBRotations->isChecked());
    CBReverseRoll->setEnabled(CBRotations->isChecked() && CBEnableRoll->isChecked());
    SliderRoll   ->setEnabled(CBRotations->isChecked() && CBEnableRoll->isChecked());
    CBEnableSpin ->setEnabled(CBRotations->isChecked());
    CBReverseSpin->setEnabled(CBRotations->isChecked() && CBEnableSpin->isChecked());
    SliderSpin   ->setEnabled(CBRotations->isChecked() && CBEnableSpin->isChecked());

    CBEnablePanLR ->setEnabled(CBTranslations->isChecked());
    CBReversePanLR->setEnabled(CBTranslations->isChecked() && CBEnablePanLR->isChecked()); 
    SliderPanLR   ->setEnabled(CBTranslations->isChecked() && CBEnablePanLR->isChecked());
    CBEnablePanUD ->setEnabled(CBTranslations->isChecked());
    CBReversePanUD->setEnabled(CBTranslations->isChecked() && CBEnablePanUD->isChecked());
    SliderPanUD   ->setEnabled(CBTranslations->isChecked() && CBEnablePanUD->isChecked());
    CBEnableZoom  ->setEnabled(CBTranslations->isChecked());
    CBReverseZoom ->setEnabled(CBTranslations->isChecked() && CBEnableZoom->isChecked());
    SliderZoom    ->setEnabled(CBTranslations->isChecked() && CBEnableZoom->isChecked());
}

void DlgCustomizeSpNavSettings::on_ButtonDefaultSpNavMotions_clicked()
{
    spaceballMotionGroup()->Clear(); 
    initialize();
}

void DlgCustomizeSpNavSettings::on_CBDominant_clicked()
{
    spaceballMotionGroup()->SetBool("Dominant", CBDominant->isChecked());
}

void DlgCustomizeSpNavSettings::on_CBFlipYZ_clicked()
{
    spaceballMotionGroup()->SetBool("FlipYZ", CBFlipYZ->isChecked());
}

void DlgCustomizeSpNavSettings::on_CBRotations_clicked()
{
    spaceballMotionGroup()->SetBool("Rotations", CBRotations->isChecked());

    CBEnableTilt ->setEnabled(CBRotations->isChecked());
    CBReverseTilt->setEnabled(CBRotations->isChecked() && CBEnableTilt->isChecked()); 
    SliderTilt   ->setEnabled(CBRotations->isChecked() && CBEnableTilt->isChecked());
    CBEnableRoll ->setEnabled(CBRotations->isChecked());
    CBReverseRoll->setEnabled(CBRotations->isChecked() && CBEnableRoll->isChecked());
    SliderRoll   ->setEnabled(CBRotations->isChecked() && CBEnableRoll->isChecked());
    CBEnableSpin ->setEnabled(CBRotations->isChecked());
    CBReverseSpin->setEnabled(CBRotations->isChecked() && CBEnableSpin->isChecked());
    SliderSpin   ->setEnabled(CBRotations->isChecked() && CBEnableSpin->isChecked());

}

void DlgCustomizeSpNavSettings::on_CBTranslations_clicked()
{
    spaceballMotionGroup()->SetBool("Translations", CBTranslations->isChecked());

    CBEnablePanLR ->setEnabled(CBTranslations->isChecked());
    CBReversePanLR->setEnabled(CBTranslations->isChecked() && CBEnablePanLR->isChecked()); 
    SliderPanLR   ->setEnabled(CBTranslations->isChecked() && CBEnablePanLR->isChecked());
    CBEnablePanUD ->setEnabled(CBTranslations->isChecked());
    CBReversePanUD->setEnabled(CBTranslations->isChecked() && CBEnablePanUD->isChecked());
    SliderPanUD   ->setEnabled(CBTranslations->isChecked() && CBEnablePanUD->isChecked());
    CBEnableZoom  ->setEnabled(CBTranslations->isChecked());
    CBReverseZoom ->setEnabled(CBTranslations->isChecked() && CBEnableZoom->isChecked());
    SliderZoom    ->setEnabled(CBTranslations->isChecked() && CBEnableZoom->isChecked());
}

void DlgCustomizeSpNavSettings::on_SliderGlobal_sliderReleased()
{
    spaceballMotionGroup()->SetInt("GlobalSensitivity", SliderGlobal->value());
}

void DlgCustomizeSpNavSettings::on_CBEnablePanLR_clicked()           
{    
    spaceballMotionGroup()->SetBool("PanLREnable", CBEnablePanLR->isChecked());

    CBReversePanLR->setEnabled(CBEnablePanLR->isChecked()); 
    SliderPanLR   ->setEnabled(CBEnablePanLR->isChecked());
}

void DlgCustomizeSpNavSettings::on_CBReversePanLR_clicked()           
{
    spaceballMotionGroup()->SetBool("PanLRReverse", CBReversePanLR->isChecked());
}

void DlgCustomizeSpNavSettings::on_SliderPanLR_sliderReleased() 
{
    spaceballMotionGroup()->SetInt("PanLRSensitivity", SliderPanLR->value());
}

void DlgCustomizeSpNavSettings::on_CBEnablePanUD_clicked()           
{
    spaceballMotionGroup()->SetBool("PanUDEnable", CBEnablePanUD->isChecked());

    CBReversePanUD->setEnabled(CBEnablePanUD->isChecked());
    SliderPanUD   ->setEnabled(CBEnablePanUD->isChecked());
}

void DlgCustomizeSpNavSettings::on_CBReversePanUD_clicked()          
{
    spaceballMotionGroup()->SetBool("PanUDReverse", CBReversePanUD->isChecked());
}

void DlgCustomizeSpNavSettings::on_SliderPanUD_sliderReleased()
{
    spaceballMotionGroup()->SetInt("PanUDSensitivity", SliderPanUD->value());
}

void DlgCustomizeSpNavSettings::on_CBEnableZoom_clicked()        
{    
    spaceballMotionGroup()->SetBool("ZoomEnable", CBEnableZoom->isChecked());

    CBReverseZoom ->setEnabled(CBEnableZoom->isChecked());
    SliderZoom    ->setEnabled(CBEnableZoom->isChecked());
}

void DlgCustomizeSpNavSettings::on_CBReverseZoom_clicked()          
{
    spaceballMotionGroup()->SetBool("ZoomReverse", CBReverseZoom->isChecked());
}

void DlgCustomizeSpNavSettings::on_SliderZoom_sliderReleased()
{
    spaceballMotionGroup()->SetInt("ZoomSensitivity", SliderZoom->value());
}

void DlgCustomizeSpNavSettings::on_CBEnableTilt_clicked()      
{
    spaceballMotionGroup()->SetBool("TiltEnable", CBEnableTilt->isChecked());

    CBReverseTilt->setEnabled(CBEnableTilt->isChecked()); 
    SliderTilt   ->setEnabled(CBEnableTilt->isChecked());
}

void DlgCustomizeSpNavSettings::on_CBReverseTilt_clicked()        
{
    spaceballMotionGroup()->SetBool("TiltReverse", CBReverseTilt->isChecked());
}

void DlgCustomizeSpNavSettings::on_SliderTilt_sliderReleased() 
{
    spaceballMotionGroup()->SetInt("TiltSensitivity", SliderTilt->value());
}

void DlgCustomizeSpNavSettings::on_CBEnableRoll_clicked()  
{
    spaceballMotionGroup()->SetBool("RollEnable", CBEnableRoll->isChecked());

    CBReverseRoll->setEnabled(CBEnableRoll->isChecked());
    SliderRoll   ->setEnabled(CBEnableRoll->isChecked());
}

void DlgCustomizeSpNavSettings::on_CBReverseRoll_clicked()   
{
    spaceballMotionGroup()->SetBool("RollReverse", CBReverseRoll->isChecked());
}

void DlgCustomizeSpNavSettings::on_SliderRoll_sliderReleased()
{
    spaceballMotionGroup()->SetInt("RollSensitivity", SliderRoll->value());
}

void DlgCustomizeSpNavSettings::on_CBEnableSpin_clicked()
{
    spaceballMotionGroup()->SetBool("SpinEnable", CBEnableSpin->isChecked());

    CBReverseSpin->setEnabled(CBEnableSpin->isChecked());
    SliderSpin   ->setEnabled(CBEnableSpin->isChecked());
}

void DlgCustomizeSpNavSettings::on_CBReverseSpin_clicked()
{
    spaceballMotionGroup()->SetBool("SpinReverse", CBReverseSpin->isChecked());
}

void DlgCustomizeSpNavSettings::on_SliderSpin_sliderReleased()
{
    spaceballMotionGroup()->SetInt("SpinSensitivity", SliderSpin->value());
}

void DlgCustomizeSpNavSettings::onAddMacroAction(const QByteArray &macroName)
{
    //don't need to do anything here.
}

void DlgCustomizeSpNavSettings::onRemoveMacroAction(const QByteArray &macroName)
{
    //don't need to do anything here.
}

void DlgCustomizeSpNavSettings::onModifyMacroAction(const QByteArray &macroName)
{
    //don't need to do anything here.
}

#include "moc_DlgCustomizeSpNavSettings.cpp"
