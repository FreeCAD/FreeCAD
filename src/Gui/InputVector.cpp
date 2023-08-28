/***************************************************************************
 *   Copyright (c) 2008 Werner Mayer <wmayer[at]users.sourceforge.net>     *
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
# include <QComboBox>
#endif

#include <Base/UnitsApi.h>

#include "InputVector.h"
#include "ui_InputVector.h"
#include "QuantitySpinBox.h"


using namespace Gui;

LocationWidget::LocationWidget (QWidget * parent)
  : QWidget(parent)
{
    box = new QGridLayout();

    xValue = new QuantitySpinBox(this);
    xValue->setMinimum(-2.14748e+09);
    xValue->setMaximum(2.14748e+09);
    xLabel = new QLabel(this);
    box->addWidget(xLabel, 0, 0, 1, 1);
    box->addWidget(xValue, 0, 1, 1, 1);

    yValue = new QuantitySpinBox(this);
    yValue->setMinimum(-2.14748e+09);
    yValue->setMaximum(2.14748e+09);
    yLabel = new QLabel(this);
    box->addWidget(yLabel, 1, 0, 1, 1);
    box->addWidget(yValue, 1, 1, 1, 1);

    zValue = new QuantitySpinBox(this);
    zValue->setMinimum(-2.14748e+09);
    zValue->setMaximum(2.14748e+09);
    zLabel = new QLabel(this);
    box->addWidget(zLabel, 2, 0, 1, 1);
    box->addWidget(zValue, 2, 1, 1, 1);

    dLabel = new QLabel(this);
    dValue = new QComboBox(this);
    dValue->setCurrentIndex(-1);
    box->addWidget(dLabel, 3, 0, 1, 1);
    box->addWidget(dValue, 3, 1, 1, 1);

    xValue->setUnit(Base::Unit::Length);
    yValue->setUnit(Base::Unit::Length);
    zValue->setUnit(Base::Unit::Length);

    auto gridLayout = new QGridLayout(this);
    gridLayout->addLayout(box, 0, 0, 1, 2);

    connect(dValue, qOverload<int>(&QComboBox::activated),
            this, &LocationWidget::onDirectionActivated);
    retranslateUi();
}

LocationWidget::~LocationWidget() = default;

QSize LocationWidget::sizeHint() const
{
    return {150,100};
}

void LocationWidget::changeEvent(QEvent* e)
{
    if (e->type() == QEvent::LanguageChange) {
        this->retranslateUi();
    }
    QWidget::changeEvent(e);
}

void LocationWidget::retranslateUi()
{
    xLabel->setText(QApplication::translate("Gui::LocationWidget", "X:"));
    yLabel->setText(QApplication::translate("Gui::LocationWidget", "Y:"));
    zLabel->setText(QApplication::translate("Gui::LocationWidget", "Z:"));
    dLabel->setText(QApplication::translate("Gui::LocationWidget", "Direction:"));

    if (dValue->count() == 0) {
        dValue->insertItems(0, QStringList()
         << QApplication::translate("Gui::LocationDialog", "X")
         << QApplication::translate("Gui::LocationDialog", "Y")
         << QApplication::translate("Gui::LocationDialog", "Z")
         << QApplication::translate("Gui::LocationDialog", "User defined...")
        );

        dValue->setCurrentIndex(2);

        // Vector3d declared to use with QVariant see Gui/propertyeditor/PropertyItem.h
        dValue->setItemData(0, QVariant::fromValue<Base::Vector3d>(Base::Vector3d(1,0,0)));
        dValue->setItemData(1, QVariant::fromValue<Base::Vector3d>(Base::Vector3d(0,1,0)));
        dValue->setItemData(2, QVariant::fromValue<Base::Vector3d>(Base::Vector3d(0,0,1)));
    }
    else {
        dValue->setItemText(0, QApplication::translate("Gui::LocationDialog", "X"));
        dValue->setItemText(1, QApplication::translate("Gui::LocationDialog", "Y"));
        dValue->setItemText(2, QApplication::translate("Gui::LocationDialog", "Z"));
        dValue->setItemText(dValue->count()-1,
            QApplication::translate("Gui::LocationDialog", "User defined..."));
    }
}

Base::Vector3d LocationWidget::getPosition() const
{
    return Base::Vector3d(this->xValue->value().getValue(),
                          this->yValue->value().getValue(),
                          this->zValue->value().getValue());
}

void LocationWidget::setPosition(const Base::Vector3d& v)
{
    this->xValue->setValue(v.x);
    this->yValue->setValue(v.y);
    this->zValue->setValue(v.z);
}

void LocationWidget::setDirection(const Base::Vector3d& dir)
{
    if (dir.Length() < Base::Vector3d::epsilon()) {
        return;
    }

    // check if the user-defined direction is already there
    for (int i=0; i<dValue->count()-1; i++) {
        QVariant data = dValue->itemData (i);
        if (data.canConvert<Base::Vector3d>()) {
            const auto val = data.value<Base::Vector3d>();
            if (val == dir) {
                dValue->setCurrentIndex(i);
                return;
            }
        }
    }

    // add a new item before the very last item
    QString display = QString::fromLatin1("(%1,%2,%3)")
        .arg(dir.x)
        .arg(dir.y)
        .arg(dir.z);
    dValue->insertItem(dValue->count()-1, display,
        QVariant::fromValue<Base::Vector3d>(dir));
    dValue->setCurrentIndex(dValue->count()-2);
}

Base::Vector3d LocationWidget::getDirection() const
{
    QVariant data = dValue->itemData (this->dValue->currentIndex());
    if (data.canConvert<Base::Vector3d>()) {
        return data.value<Base::Vector3d>();
    }
    else {
        return Base::Vector3d(0,0,1);
    }
}

Base::Vector3d LocationWidget::getUserDirection(bool* ok) const
{
    Gui::Dialog::Ui_InputVector iv;
    QDialog dlg(const_cast<LocationWidget*>(this));
    iv.setupUi(&dlg);
    iv.vectorX->setDecimals(Base::UnitsApi::getDecimals());
    iv.vectorY->setDecimals(Base::UnitsApi::getDecimals());
    iv.vectorZ->setDecimals(Base::UnitsApi::getDecimals());
    Base::Vector3d dir;
    if (dlg.exec()) {
        dir.x = iv.vectorX->value();
        dir.y = iv.vectorY->value();
        dir.z = iv.vectorZ->value();
        if (ok) *ok = true;
    }
    else {
        if (ok) *ok = false;
    }

    return dir;
}

void LocationWidget::onDirectionActivated(int index)
{
    // last item is selected to define direction by user
    if (index+1 == dValue->count()) {
        bool ok;
        Base::Vector3d dir = this->getUserDirection(&ok);
        if (ok) {
            if (dir.Length() < Base::Vector3d::epsilon()) {
                QMessageBox::critical(this, LocationDialog::tr("Wrong direction"),
                    LocationDialog::tr("Direction must not be the null vector"));
                return;
            }

            setDirection(dir);
        }
    }
}

// ----------------------------------------------------------------------------

LocationDialog::LocationDialog(QWidget* parent, Qt::WindowFlags fl)
  : QDialog(parent, fl)
{
}

LocationDialog::~LocationDialog() = default;

Base::Vector3d LocationDialog::getUserDirection(bool* ok) const
{
    Gui::Dialog::Ui_InputVector iv;
    QDialog dlg(const_cast<LocationDialog*>(this));
    iv.setupUi(&dlg);
    iv.vectorX->setDecimals(Base::UnitsApi::getDecimals());
    iv.vectorY->setDecimals(Base::UnitsApi::getDecimals());
    iv.vectorZ->setDecimals(Base::UnitsApi::getDecimals());
    Base::Vector3d dir;
    if (dlg.exec()) {
        dir.x = iv.vectorX->value();
        dir.y = iv.vectorY->value();
        dir.z = iv.vectorZ->value();
        if (ok) *ok = true;
    }
    else {
        if (ok) *ok = false;
    }

    return dir;
}

void LocationDialog::onDirectionActivated(int index)
{
    directionActivated(index);
}

// -----------------------------------------------------------

LocationDialogUiImp::~LocationDialogUiImp() = default;

Base::Vector3d LocationDialogUiImp::getDirection() const
{
    return ui->getDirection();
}

Base::Vector3d LocationDialogUiImp::getPosition() const
{
    return ui->getPosition();
}

void LocationDialogUiImp::changeEvent(QEvent *e)
{
    if (e->type() == QEvent::LanguageChange) {
        ui->retranslate(this);
    }
    else {
        QDialog::changeEvent(e);
    }
}

void LocationDialogUiImp::directionActivated(int index)
{
    ui->directionActivated(this,index);
}

#include "moc_InputVector.cpp"
