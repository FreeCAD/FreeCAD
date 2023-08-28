/***************************************************************************
 *   Copyright (c) 2022 WandererFan <wandererfan@gmail.com>                *
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

// A widget for editing Vector3d without taking up too much space in the UI.

#include "PreCompiled.h"
#ifndef _PreComp_
#include <QApplication>
#include <QLabel>
#include <QObject>
#include <QPushButton>
#include <QtGui>
#include <QVBoxLayout>
#include <QGridLayout>
#include <QHBoxLayout>
#include <QLabel>
#include <QLineEdit>
#include <QSpacerItem>
#include <QToolButton>
#endif

#include <Mod/TechDraw/TechDrawGlobal.h>

#include <limits>

#include <Base/Console.h>
#include <Base/Tools.h>
#include <Base/UnitsApi.h>

#include <Gui/SpinBox.h>

#include <Mod/TechDraw/App/DrawUtil.h>

#include "VectorEditWidget.h"

using namespace TechDrawGui;
using namespace TechDraw;

VectorEditWidget::VectorEditWidget(QWidget* parent) : QWidget(parent),
    m_minimumWidth(200),
    m_minimumHeight(30),
    m_expandedHeight(155),
    m_blockNotify(false)
{
    m_size = QSize(m_minimumWidth, m_minimumHeight);
    setObjectName(QString::fromUtf8("VectorEdit"));
    buildWidget();

    connect(tbExpand, &QToolButton::toggled, this, &VectorEditWidget::slotExpandButtonToggled);
    connect(dsbX, qOverload<double>(&Gui::DoubleSpinBox::valueChanged), this, &VectorEditWidget::slotXValueChanged);
    connect(dsbY, qOverload<double>(&Gui::DoubleSpinBox::valueChanged), this, &VectorEditWidget::slotYValueChanged);
    connect(dsbZ, qOverload<double>(&Gui::DoubleSpinBox::valueChanged), this, &VectorEditWidget::slotZValueChanged);

    dsbX->installEventFilter(this);
    dsbY->installEventFilter(this);
    dsbZ->installEventFilter(this);
}

//trap Enter press in dsb? so as not to invoke task accept processing
bool VectorEditWidget::eventFilter(QObject *target, QEvent *event)
{
    if (target == dsbX ||
            target == dsbY ||
            target == dsbZ) {
        if (event->type() == QEvent::KeyPress) {
            QKeyEvent *keyEvent = static_cast<QKeyEvent *>(event);
            if (keyEvent->key() == Qt::Key_Return ||
                    keyEvent->key() == Qt::Key_Enter) {
                QDoubleSpinBox* dsb = static_cast<QDoubleSpinBox*>(target);
                dsb->interpretText();
                Q_EMIT dsb->valueChanged(dsb->value());
                return true;
            }
        }
    }
    return QWidget::eventFilter(target, event);
}
void VectorEditWidget::setLabel(std::string newLabel)
{
    QString qNewLabelString = Base::Tools::fromStdString(newLabel);
    lvectorName->setText(qNewLabelString);
}

void VectorEditWidget::setLabel(QString newLabel)
{
    lvectorName->setText(newLabel);
}

void VectorEditWidget::setValue(Base::Vector3d newValue)
{
//    Base::Console().Message("VEW::setValue(%.6f, %.6f, %.6f)\n", newValue.x, newValue.y, newValue.z);
    m_value = newValue;
    dsbX->setValue(m_value.x);
    dsbY->setValue(m_value.y);
    dsbZ->setValue(m_value.z);
    updateDisplay();
}

void VectorEditWidget::setValueNoNotify(Base::Vector3d newValue)
{
//    Base::Console().Message("VEW::setValueNoNotify(%.6f, %.6f, %.6f)\n", newValue.x, newValue.y, newValue.z);
    m_value = newValue;
    m_blockNotify = true;
    dsbX->setValue(m_value.x);
    dsbY->setValue(m_value.y);
    dsbZ->setValue(m_value.z);
    m_blockNotify = false;
    updateDisplay();
}

void VectorEditWidget::slotExpandButtonToggled(bool checked)
{
//    Base::Console().Message("VEW::slotExpand - checked: %d\n", checked);
    if (checked) {
        vectorEditLayout->addLayout(VectorEditItemLayout);
        vectorEditLayout->addItem(verticalSpacer);
        m_size = QSize(m_minimumWidth, m_expandedHeight);

    } else {
        vectorEditLayout->removeItem(VectorEditItemLayout);
        vectorEditLayout->removeItem(verticalSpacer);
        m_size = QSize(m_minimumWidth, m_minimumHeight);
    }
}

//slotXValueChanged can be triggered by the Ui or programmatically.  We only want
//to tell the world about the change if it comes from the Ui.
void VectorEditWidget::slotXValueChanged(double newValue)
{
//    Base::Console().Message("VEW::xValueChanged(%.6f) - m_value.x: %.6f\n", newValue, m_value.x);
    if (!m_blockNotify) {
        //this is a change from the dsb
        m_value.x = newValue;
        updateDisplay();
        Q_EMIT valueChanged(m_value);
    }
}
void VectorEditWidget::slotYValueChanged(double newValue)
{
//    Base::Console().Message("VEW::yValueChanged(%.6f) - m_value.y: %.6f\n", newValue, m_value.y);
    if (!m_blockNotify) {
        //this is a change from the dsb
        m_value.y = newValue;
        updateDisplay();
        Q_EMIT valueChanged(m_value);
    }
}
void VectorEditWidget::slotZValueChanged(double newValue)
{
//    Base::Console().Message("VEW::zValueChanged(%.6f)\n", newValue);
    if (!m_blockNotify) {
        //this is a change from the dsb
        m_value.z = newValue;
        updateDisplay();
        Q_EMIT valueChanged(m_value);
    }
}

void VectorEditWidget::updateDisplay()
{
//    Base::Console().Message("VEW::updateDisplay() - m_value: %s\n", DrawUtil::formatVector(m_value).c_str());
    QString qNewDisplayString = Base::Tools::fromStdString(DrawUtil::formatVector(m_value));
    leVectorDisplay->setText(qNewDisplayString);
}

QSize VectorEditWidget::minimumSizeHint() const
{
    return m_size;
}

void VectorEditWidget::buildWidget()
{
    if (objectName().isEmpty())
        setObjectName(QString::fromUtf8("VectorEdit"));
    QSizePolicy sizePolicy(QSizePolicy::Preferred, QSizePolicy::MinimumExpanding);
    setSizePolicy(sizePolicy);

    vectorEditLayout = new QVBoxLayout(this);
    vectorEditLayout->setObjectName(QString::fromUtf8("vectorEditLayout"));
    vectorEditLayout->setContentsMargins(0, 0, 0, 0);
    VectorEditButtonLayout = new QHBoxLayout();
    VectorEditButtonLayout->setSpacing(0);
    VectorEditButtonLayout->setObjectName(QString::fromUtf8("VectorEditButtonLayout"));

    lvectorName = new QLabel(this);
    lvectorName->setObjectName(QString::fromUtf8("lvectorName"));
    VectorEditButtonLayout->addWidget(lvectorName);

    leVectorDisplay = new QLineEdit(this);
    leVectorDisplay->setObjectName(QString::fromUtf8("leVectorDisplay"));
    VectorEditButtonLayout->addWidget(leVectorDisplay);

    tbExpand = new QToolButton(this);
    tbExpand->setObjectName(QString::fromUtf8("tbExpand"));
    tbExpand->setText(QString::fromUtf8("..."));
    tbExpand->setCheckable(true);
    VectorEditButtonLayout->addWidget(tbExpand);

    VectorEditButtonLayout->setStretch(0, 1);
    VectorEditButtonLayout->setStretch(1, 1);
    vectorEditLayout->addLayout(VectorEditButtonLayout);

    VectorEditItemLayout = new QGridLayout();
    VectorEditItemLayout->setObjectName(QString::fromUtf8("VectorEditItemLayout"));

    lX = new QLabel();
    lX->setObjectName(QString::fromUtf8("lX"));
    lX->setText(QString::fromUtf8("X:"));
    VectorEditItemLayout->addWidget(lX, 0, 0, 1, 1);

    dsbX = new Gui::DoubleSpinBox();
    dsbX->setObjectName(QString::fromUtf8("dsbX"));
    dsbX->setAlignment(Qt::AlignRight|Qt::AlignTrailing|Qt::AlignVCenter);
    dsbX->setKeyboardTracking(false);
    dsbX->setMaximum(std::numeric_limits<double>::max());
    dsbX->setMinimum(std::numeric_limits<double>::lowest());
    dsbX->setDecimals(Base::UnitsApi::getDecimals());
    VectorEditItemLayout->addWidget(dsbX, 0, 1, 1, 1);

    lY = new QLabel();
    lY->setObjectName(QString::fromUtf8("lY"));
    lY->setText(QString::fromUtf8("Y:"));
    VectorEditItemLayout->addWidget(lY, 1, 0, 1, 1);

    dsbY = new Gui::DoubleSpinBox();
    dsbY->setObjectName(QString::fromUtf8("dsbY"));
    dsbY->setAlignment(Qt::AlignRight|Qt::AlignTrailing|Qt::AlignVCenter);
    dsbY->setKeyboardTracking(false);
    dsbY->setMaximum(std::numeric_limits<double>::max());
    dsbY->setMinimum(std::numeric_limits<double>::lowest());
    dsbY->setDecimals(Base::UnitsApi::getDecimals());
    VectorEditItemLayout->addWidget(dsbY, 1, 1, 1, 1);

    lZ = new QLabel();
    lZ->setObjectName(QString::fromUtf8("lZ"));
    lZ->setText(QString::fromUtf8("Z:"));
    VectorEditItemLayout->addWidget(lZ, 2, 0, 1, 1);

    dsbZ = new Gui::DoubleSpinBox();
    dsbZ->setObjectName(QString::fromUtf8("dsbZ"));
    dsbZ->setAlignment(Qt::AlignRight|Qt::AlignTrailing|Qt::AlignVCenter);
    dsbZ->setKeyboardTracking(false);
    dsbZ->setMaximum(std::numeric_limits<double>::max());
    dsbZ->setMinimum(std::numeric_limits<double>::lowest());
    dsbZ->setDecimals(Base::UnitsApi::getDecimals());
    VectorEditItemLayout->addWidget(dsbZ, 2, 1, 1, 1);

    verticalSpacer = new QSpacerItem(20, 40, QSizePolicy::Minimum, QSizePolicy::Expanding);
}

