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

#pragma once

#include <QWidget>
#include <QSize>
#include <QString>

QT_BEGIN_NAMESPACE
class QVBoxLayout;
class QHBoxLayout;
class QGridLayout;
class QLabel;
class QLineEdit;
class QToolButton;
class QSpacerItem;
QT_END_NAMESPACE

namespace Gui {
class DoubleSpinBox;
}

#include <Base/Vector3D.h>

namespace TechDrawGui {

class VectorEditWidget : public QWidget
{
    Q_OBJECT

public:
    VectorEditWidget(QWidget* parent = 0);
    ~VectorEditWidget() override = default;

    QSize minimumSizeHint() const override;
    bool eventFilter(QObject *target, QEvent *event) override;

    void setLabel(std::string newLabel);
    void setLabel(QString newLabel);
    Base::Vector3d value() const { return m_value; }

Q_SIGNALS:
    void valueChanged(Base::Vector3d newValue);

public Q_SLOTS:
    void setValue(Base::Vector3d newValue);
    void setValueNoNotify(Base::Vector3d newValue);

protected:
    void buildWidget();

protected Q_SLOTS:
    void slotExpandButtonToggled(bool checked);
    void slotXValueChanged(double newValue);
    void slotYValueChanged(double newValue);
    void slotZValueChanged(double newValue);

    void updateDisplay();

private:
    int m_minimumWidth;
    int m_minimumHeight;
    int m_expandedHeight;
    bool m_blockNotify;

    QSize m_size;

    Base::Vector3d m_value;

    QVBoxLayout *vectorEditLayout;
    QHBoxLayout *VectorEditButtonLayout;
    QLabel *lvectorName;
    QLineEdit *leVectorDisplay;
    QToolButton *tbExpand;
    QGridLayout *VectorEditItemLayout;
    Gui::DoubleSpinBox *dsbX;
    Gui::DoubleSpinBox *dsbY;
    Gui::DoubleSpinBox *dsbZ;
    QLabel *lX;
    QLabel *lY;
    QLabel *lZ;
    QSpacerItem *verticalSpacer;
};

} //namespace TechDrawGui