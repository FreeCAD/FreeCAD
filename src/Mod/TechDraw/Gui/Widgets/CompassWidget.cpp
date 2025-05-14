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

// The CompassWidget has several children widgets - a CompassDialWidget, a fine
// adjustment QDoubleSpinBox and QPushButtons that increment the spin box by a
// (usually) larger step than the arrows on the spinbox

#include "PreCompiled.h"
#ifndef _PreComp_
#include <QApplication>
#include <QLabel>
#include <QObject>
#include <QPushButton>
#include <QtGui>
#include <QtWidgets/QDoubleSpinBox>
#include <QtWidgets/QVBoxLayout>
#endif

#include <Mod/TechDraw/TechDrawGlobal.h>

#include <Gui/QuantitySpinBox.h>

#include <Base/Console.h>
#include <Base/Tools.h>

#include "CompassDialWidget.h"
#include "CompassWidget.h"

using namespace TechDrawGui;

CompassWidget::CompassWidget(QWidget* parent)
    : QWidget(parent), m_minimumWidth(200), m_minimumHeight(200), m_defaultMargin(10), m_angle(0.0),
      m_advanceIncrement(10.0)
{
    setObjectName(QStringLiteral("Compass"));
    m_rect = QRect(0, 0, m_minimumWidth, m_minimumHeight);
    buildWidget();
    compassDial->setSize(m_minimumHeight - 2 * m_defaultMargin);

    dsbAngle->installEventFilter(this);

    connect(pbCWAdvance, &QPushButton::pressed, this, &CompassWidget::slotCWAdvance);
    connect(pbCCWAdvance, &QPushButton::pressed, this, &CompassWidget::slotCCWAdvance);
}

//trap Enter press in dsbAngle so as not to invoke task accept processing
bool CompassWidget::eventFilter(QObject* target, QEvent* event)
{
    if (target == dsbAngle) {
        if (event->type() == QEvent::KeyPress) {
            QKeyEvent* keyEvent = static_cast<QKeyEvent*>(event);
            const auto isEnter = keyEvent->key() == Qt::Key_Return || keyEvent->key() == Qt::Key_Enter;
            if (isEnter && dsbAngle->isNormalized()) {
                return true;
            }
        }
    }
    return QWidget::eventFilter(target, event);
}

void CompassWidget::buildWidget()
{
    resize(m_minimumWidth, m_minimumHeight);
    QSizePolicy sizePolicy(QSizePolicy::Preferred, QSizePolicy::Expanding);
    sizePolicy.setHorizontalStretch(0);
    sizePolicy.setVerticalStretch(0);
    sizePolicy.setHeightForWidth(sizePolicy.hasHeightForWidth());
    setSizePolicy(sizePolicy);
    setMinimumSize(QSize(m_minimumWidth, m_minimumHeight));
    compassLayout = new QVBoxLayout(this);
    compassLayout->setObjectName(QStringLiteral("CompassLayout"));

    compassDialLayout = new QHBoxLayout();
    compassDialLayout->setObjectName(QStringLiteral("compassDialLayout"));

    pbCWAdvance = new QPushButton(this);
    pbCWAdvance->setObjectName(QStringLiteral("pbCWAdvance"));
    QIcon icon1;
    icon1.addFile(QStringLiteral(":/icons/arrow-cw.svg"), QSize(), QIcon::Normal, QIcon::On);
    pbCWAdvance->setIcon(icon1);
    compassDialLayout->addWidget(pbCWAdvance);

    compassDial = new CompassDialWidget(this);
    compassDial->setObjectName(QStringLiteral("CompassDial"));
    compassDialLayout->addWidget(compassDial);

    pbCCWAdvance = new QPushButton(this);
    pbCCWAdvance->setObjectName(QStringLiteral("pbCCWAdvance"));
    QIcon icon2;
    icon2.addFile(QStringLiteral(":/icons/arrow-ccw.svg"), QSize(), QIcon::Normal, QIcon::On);
    pbCCWAdvance->setIcon(icon2);
    compassDialLayout->addWidget(pbCCWAdvance);

    compassDialLayout->setStretch(1, 2);
    compassLayout->addLayout(compassDialLayout);

    compassControlLayout = new QHBoxLayout();
    compassControlLayout->setObjectName(QStringLiteral("compassControlLayout"));
    compassControlLabel = new QLabel(this);
    compassControlLabel->setObjectName(QStringLiteral("compassControlLabel"));
    QSizePolicy sizePolicy2(QSizePolicy::Minimum, QSizePolicy::Minimum);
    sizePolicy2.setHorizontalStretch(0);
    sizePolicy2.setVerticalStretch(0);
    sizePolicy2.setHeightForWidth(compassControlLabel->sizePolicy().hasHeightForWidth());
    compassControlLabel->setSizePolicy(sizePolicy2);

    compassControlLayout->addWidget(compassControlLabel);
    dsbAngle = new Gui::QuantitySpinBox(this);
    dsbAngle->setObjectName(QStringLiteral("dsbAngle"));
    dsbAngle->setUnit(Base::Unit::Angle);
    connect(dsbAngle, QOverload<double>::of(&Gui::QuantitySpinBox::valueChanged),
        this, &CompassWidget::slotSpinBoxEnter);

    compassControlLayout->addWidget(dsbAngle);

    compassControlLayout->setStretch(0, 3);
    compassControlLayout->setStretch(1, 2);

    compassLayout->addLayout(compassControlLayout);

    retranslateUi();
}

void CompassWidget::retranslateUi()
{
    compassControlLabel->setText(
        QApplication::translate("CompassWidget", "View Direction as Angle", nullptr));
#ifndef QT_NO_TOOLTIP
    dsbAngle->setToolTip(QApplication::translate(
        "CompassWidget", "The view direction angle relative to +X in the BaseView.", nullptr));
    pbCWAdvance->setToolTip(QApplication::translate(
        "CompassWidget", "Advance the view direction in clockwise direction.", nullptr));
    pbCCWAdvance->setToolTip(QApplication::translate(
        "CompassWidget", "Advance the view direction in anti-clockwise direction.", nullptr));
#endif// QT_NO_TOOLTIP
}

QSize CompassWidget::sizeHint() const { return m_rect.size(); }

QSize CompassWidget::minimumSizeHint() const
{
    return QRect(0, 0, m_minimumWidth, m_minimumHeight).size();
}

void CompassWidget::paintEvent(QPaintEvent* event)
{
    Q_UNUSED(event);
    QWidget::paintEvent(event);
}

// set the compass dial and spinbox to a new angle
void CompassWidget::setDialAngle(double newAngle)
{
    //    Base::Console().message("CW::setDialAngle(%.3f)\n", newAngle);
    m_angle = newAngle;
    if (compassDial) {
        compassDial->setAngle(m_angle);
    }
    if (dsbAngle) {
        dsbAngle->setValue(m_angle);
    }
}

//slot for updates from spinbox on Enter/Return press.
void CompassWidget::slotSpinBoxEnter(double newAngle)
{
    //    Base::Console().message("CW::slotSpinBoxEnter(%.3f)\n", newAngle);
    if (dsbAngle) {
        m_angle = newAngle;
        Q_EMIT angleChanged(m_angle);
        if (compassDial) {
            compassDial->setAngle(m_angle);
        }
    }
}

void CompassWidget::slotCWAdvance()
{
    double angle = m_angle - m_advanceIncrement;
    if (angle < -360.0) {
        angle = angle + 360.0;
    }
    setDialAngle(angle);
    Q_EMIT angleChanged(angle);
}

void CompassWidget::slotCCWAdvance()
{
    double angle = m_angle + m_advanceIncrement;
    if (angle > dsbAngle->maximum()) {
        angle = angle - dsbAngle->maximum();
    }
    if (angle < dsbAngle->minimum()) {
        angle = angle + dsbAngle->minimum();
    }
    setDialAngle(angle);
    Q_EMIT angleChanged(angle);
}

void CompassWidget::setAdvanceIncrement(double newIncrement) { m_advanceIncrement = newIncrement; }
