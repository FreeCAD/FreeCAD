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

// The CompassWidget has a clickable CompassDialWidget and a fine adjustment
// QDoubleSpinBox.

#include <QApplication>
#include <QIcon>
#include <QLabel>
#include <QObject>
#include <QSignalBlocker>
#include <QToolButton>
#include <QtGui>
#include <QtWidgets/QDoubleSpinBox>
#include <QtWidgets/QVBoxLayout>


#include <Mod/TechDraw/TechDrawGlobal.h>

#include <Gui/QuantitySpinBox.h>

#include <Base/Console.h>
#include <Base/Tools.h>

#include "CompassDialWidget.h"
#include "CompassWidget.h"

using namespace TechDrawGui;

CompassWidget::CompassWidget(QWidget* parent)
    : QWidget(parent), m_minimumWidth(200), m_minimumHeight(200), m_defaultMargin(10), m_angle(0.0)
{
    setObjectName(QStringLiteral("Compass"));
    m_rect = QRect(0, 0, m_minimumWidth, m_minimumHeight);
    buildWidget();
    compassDial->setSize(m_minimumHeight - 2 * m_defaultMargin);

    dsbAngle->installEventFilter(this);
    compassDial->installEventFilter(this);

    connect(compassDial, &CompassDialWidget::angleSelected,
            this, &CompassWidget::slotDialAngleSelected);
    connect(reverseButton, &QToolButton::clicked,
            this, &CompassWidget::reverseDirection);
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
    else if (target == compassDial
             && event->type() == QEvent::Resize) {
        positionReverseButton();
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
    compassLayout->setContentsMargins(0, 0, 0, 0);

    compassDialLayout = new QHBoxLayout();
    compassDialLayout->setObjectName(QStringLiteral("compassDialLayout"));
    compassDialLayout->setContentsMargins(0, 0, 0, 0);

    compassDial = new CompassDialWidget(this);
    compassDial->setObjectName(QStringLiteral("CompassDial"));
    compassDial->setCursor(Qt::PointingHandCursor);
    compassDialLayout->addWidget(compassDial);
    compassLayout->addLayout(compassDialLayout);

    reverseButton = new QToolButton(compassDial);
    reverseButton->setObjectName(
        QStringLiteral("reverseDirectionButton"));
    reverseButton->setAutoRaise(true);
    reverseButton->setIcon(
        QIcon(QStringLiteral(":/icons/button_sort.svg")));
    reverseButton->setIconSize(QSize(18, 18));
    reverseButton->setFixedSize(QSize(26, 26));
    reverseButton->raise();
    positionReverseButton();

    compassControlLayout = new QHBoxLayout();
    compassControlLayout->setObjectName(QStringLiteral("compassControlLayout"));
    compassControlLayout->setContentsMargins(0, 0, 0, 0);
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

    compassControlLayout->setStretch(1, 1);

    compassLayout->addLayout(compassControlLayout);

    retranslateUi();
}

void CompassWidget::retranslateUi()
{
    compassControlLabel->setText(
        QApplication::translate("CompassWidget", "As angle", nullptr));
#ifndef QT_NO_TOOLTIP
    dsbAngle->setToolTip(QApplication::translate(
        "CompassWidget", "The view direction angle relative to +X in the BaseView.", nullptr));
    compassDial->setToolTip(QApplication::translate(
        "CompassWidget",
        "Sets the view direction angle by clicking or dragging, rounded to the nearest 5°",
        nullptr
    ));
    reverseButton->setToolTip(QApplication::translate(
        "CompassWidget", "Reverse the view direction", nullptr));
#endif// QT_NO_TOOLTIP
}

void CompassWidget::positionReverseButton()
{
    if (!compassDial || !reverseButton) {
        return;
    }
    constexpr int cornerMargin = 4;
    reverseButton->move(
        compassDial->width() - reverseButton->width()
            - cornerMargin,
        compassDial->height() - reverseButton->height()
            - cornerMargin);
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

void CompassWidget::slotDialAngleSelected(double angle)
{
    const QSignalBlocker blocker(dsbAngle);
    setDialAngle(angle);
    Q_EMIT angleChanged(angle);
}

void CompassWidget::reverseDirection()
{
    const double reversed =
        std::fmod(m_angle + 540.0, 360.0);
    const QSignalBlocker blocker(dsbAngle);
    setDialAngle(reversed);
    Q_EMIT directionReversed(reversed);
}
