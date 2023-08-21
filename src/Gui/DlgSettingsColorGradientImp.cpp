/***************************************************************************
 *   Copyright (c) 2005 Werner Mayer <wmayer[at]users.sourceforge.net>     *
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
# include <limits>
# include <QButtonGroup>
# include <QDoubleValidator>
# include <QFontMetrics>
# include <QLocale>
# include <QMessageBox>
#endif

#include "DlgSettingsColorGradientImp.h"
#include "ui_DlgSettingsColorGradient.h"
#include "SpinBox.h"
#include "Tools.h"


using namespace Gui::Dialog;

/* TRANSLATOR Gui::Dialog::DlgSettingsColorGradientImp */

/**
 *  Constructs a DlgSettingsColorGradientImp as a child of 'parent', with the
 *  name 'name' and widget flags set to 'f'.
 */
DlgSettingsColorGradientImp::DlgSettingsColorGradientImp(const App::ColorGradient& cg,
                                                         QWidget* parent, Qt::WindowFlags fl)
  : QDialog( parent, fl )
  , validator(nullptr)
  , ui(new Ui_DlgSettingsColorGradient)
{
    ui->setupUi(this);
    ui->spinBoxLabel->setRange(5, 30);
    ui->spinBoxDecimals->setMaximum(std::numeric_limits<float>::digits10);

    // remove the automatic help button in dialog title since we don't use it
    setWindowFlag(Qt::WindowContextHelpButtonHint, false);

    // the elementary charge is 1.6e-19, since such values might be the result of
    // simulations, use this as boundary for a scientific validator
    validator = new QDoubleValidator(-2e19, 2e19, ui->spinBoxDecimals->maximum(), this);
    validator->setNotation(QDoubleValidator::ScientificNotation);
    ui->floatLineEditMax->setValidator(validator);
    ui->floatLineEditMin->setValidator(validator);

    // assure that the LineEdit is as wide to contain numbers with 4 digits and 6 decimals
    QFontMetrics fm(ui->floatLineEditMax->font());
    ui->floatLineEditMax->setMinimumWidth(QtTools::horizontalAdvance(fm, QString::fromLatin1("-8000.000000")));

    setColorModelNames(cg.getColorModelNames());
    setProfile(cg.getProfile());
    setupConnections();
}

/**
 *  Destroys the object and frees any allocated resources
 */
DlgSettingsColorGradientImp::~DlgSettingsColorGradientImp() = default;

void DlgSettingsColorGradientImp::setupConnections()
{
    auto group = new QButtonGroup(this);
    group->setExclusive(true);
    group->addButton(ui->radioButtonFlow);
    group->addButton(ui->radioButtonZero);
#if QT_VERSION >= QT_VERSION_CHECK(5,15,0)
    connect(group, &QButtonGroup::idClicked,
            this, &DlgSettingsColorGradientImp::colorModelChanged);
#else
    connect(group, qOverload<int>(&QButtonGroup::buttonClicked),
            this, &DlgSettingsColorGradientImp::colorModelChanged);
#endif
    connect(ui->comboBoxModel, qOverload<int>(&QComboBox::currentIndexChanged),
            this, &DlgSettingsColorGradientImp::colorModelChanged);

    connect(ui->checkBoxGrayed, &QCheckBox::toggled,
            this, &DlgSettingsColorGradientImp::colorModelChanged);

    connect(ui->checkBoxInvisible, &QCheckBox::toggled,
            this, &DlgSettingsColorGradientImp::colorModelChanged);

    connect(ui->floatLineEditMax, &QLineEdit::editingFinished,
            this, &DlgSettingsColorGradientImp::colorModelChanged);

    connect(ui->floatLineEditMin, &QLineEdit::editingFinished,
            this, &DlgSettingsColorGradientImp::colorModelChanged);

    connect(ui->spinBoxLabel, qOverload<int>(&QSpinBox::valueChanged),
            this, &DlgSettingsColorGradientImp::colorModelChanged);

    connect(ui->spinBoxDecimals, qOverload<int>(&QSpinBox::valueChanged),
            this, &DlgSettingsColorGradientImp::colorModelChanged);
}

App::ColorGradientProfile DlgSettingsColorGradientImp::getProfile() const
{
    App::ColorGradientProfile profile;
    profile.tColorModel = colorModel();
    profile.tStyle = colorStyle();
    profile.visibility.setFlag(App::Visibility::Grayed, isOutGrayed());
    profile.visibility.setFlag(App::Visibility::Invisible, isOutInvisible());
    profile.ctColors = numberOfLabels();
    getRange(profile.fMin, profile.fMax);
    return profile;
}

void DlgSettingsColorGradientImp::setProfile(const App::ColorGradientProfile& pro)
{
    setColorModel(pro.tColorModel);
    setColorStyle(pro.tStyle);
    setOutGrayed(pro.visibility.testFlag(App::Visibility::Grayed));
    setOutInvisible(pro.visibility.testFlag(App::Visibility::Invisible));
    setNumberOfLabels(pro.ctColors);
    setRange(pro.fMin, pro.fMax);
}

void DlgSettingsColorGradientImp::setColorModel(std::size_t index)
{
    ui->comboBoxModel->setCurrentIndex(index);
}

std::size_t DlgSettingsColorGradientImp::colorModel() const
{
    return static_cast<std::size_t>(ui->comboBoxModel->currentIndex());
}

void DlgSettingsColorGradientImp::setColorModelNames(const std::vector<std::string>& names)
{
    ui->comboBoxModel->clear();
    for (const auto& it : names) {
        ui->comboBoxModel->addItem(QString::fromStdString(it));
    }
}

void DlgSettingsColorGradientImp::setColorStyle( App::ColorBarStyle tStyle )
{
    switch ( tStyle )
    {
    case App::ColorBarStyle::FLOW:
        ui->radioButtonFlow->setChecked(true);
        break;
    case App::ColorBarStyle::ZERO_BASED:
        ui->radioButtonZero->setChecked(true);
        break;
    }
}

App::ColorBarStyle DlgSettingsColorGradientImp::colorStyle() const
{
    return ui->radioButtonZero->isChecked() ? App::ColorBarStyle::ZERO_BASED
                                            : App::ColorBarStyle::FLOW;
}

void DlgSettingsColorGradientImp::setOutGrayed( bool grayed )
{
    ui->checkBoxGrayed->setChecked( grayed );
}

bool DlgSettingsColorGradientImp::isOutGrayed() const
{
    return ui->checkBoxGrayed->isChecked();
}

void DlgSettingsColorGradientImp::setOutInvisible( bool invisible )
{
    ui->checkBoxInvisible->setChecked( invisible );
}

bool DlgSettingsColorGradientImp::isOutInvisible() const
{
    return ui->checkBoxInvisible->isChecked();
}

void DlgSettingsColorGradientImp::setRange(float fMin, float fMax)
{
    auto toString = [=](float value, int decimals) {
        int pos = 0;
        while (decimals > 0) {
            QString str = QLocale().toString(value, 'g', decimals--);
            if (validator->validate(str, pos) == QValidator::Acceptable) {
                return str;
            }
        }

        return QLocale().toString(value, 'g', 1);
    };

    ui->floatLineEditMax->blockSignals(true);
    ui->floatLineEditMax->setText(toString(fMax, numberOfDecimals()));
    ui->floatLineEditMax->blockSignals(false);

    ui->floatLineEditMin->blockSignals(true);
    ui->floatLineEditMin->setText(toString(fMin, numberOfDecimals()));
    ui->floatLineEditMin->blockSignals(false);
}

void DlgSettingsColorGradientImp::getRange(float& fMin, float& fMax) const
{
    fMax = QLocale().toFloat(ui->floatLineEditMax->text());
    fMin = QLocale().toFloat(ui->floatLineEditMin->text());
}

void DlgSettingsColorGradientImp::setNumberOfLabels(int val)
{
    ui->spinBoxLabel->setValue( val );
}

int DlgSettingsColorGradientImp::numberOfLabels() const
{
    return ui->spinBoxLabel->value();
}

void DlgSettingsColorGradientImp::setNumberOfDecimals(int val, float fMin, float fMax)
{
    ui->spinBoxDecimals->setValue(val);
    setRange(fMin, fMax);
}

int DlgSettingsColorGradientImp::numberOfDecimals() const
{
    return ui->spinBoxDecimals->value();
}

void DlgSettingsColorGradientImp::accept()
{
    double fMax = QLocale().toDouble(ui->floatLineEditMax->text());
    double fMin = QLocale().toDouble(ui->floatLineEditMin->text());

    if (fMax <= fMin) {
        QMessageBox::warning(this, tr("Wrong parameter"),
            tr("The maximum value must be higher than the minimum value."));
    }
    else {
        QDialog::accept();
    }
}

#include "moc_DlgSettingsColorGradientImp.cpp"

