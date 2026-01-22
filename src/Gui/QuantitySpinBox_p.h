/***************************************************************************
 *   Copyright (c) 2015 Eivind Kvedalen <eivind@kvedalen.name>             *
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
 *   write to the Free Software Foundation, Inc., 51 Franklin Street,      *
 *   Fifth Floor, Boston, MA  02110-1301, USA                              *
 *                                                                         *
 ***************************************************************************/

#pragma once

#include <QLabel>
#include <QMouseEvent>
#include <QLineEdit>

class ExpressionLabel: public QLabel
{
    Q_OBJECT
public:
    ExpressionLabel(QWidget* parent)
        : QLabel(parent)
    {}

    void setExpressionText(const QString& text)
    {
        if (text.isEmpty()) {
            this->setToolTip(genericExpressionEditorTooltip);
        }
        else {
            this->setToolTip(expressionEditorTooltipPrefix + text);
        }
    }

    void show()
    {
        if (auto parentLineEdit = qobject_cast<QLineEdit*>(parent())) {
            // horizontal margin, so text will not be behind the icon
            QMargins margins = parentLineEdit->contentsMargins();
            margins.setRight(2 * margins.right() + sizeHint().width());
            parentLineEdit->setContentsMargins(margins);
        }
        QLabel::show();
    }

protected:
    void mouseReleaseEvent(QMouseEvent* event) override
    {
        if (rect().contains(event->pos())) {
            Q_EMIT clicked();
        }
    }

Q_SIGNALS:
    void clicked();

private:
    const QString genericExpressionEditorTooltip = tr("Enter expression… (=)");
    const QString expressionEditorTooltipPrefix = tr("Expression:") + QStringLiteral(" ");
};
