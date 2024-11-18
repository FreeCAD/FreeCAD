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

#ifndef QUANTITYSPINBOX_P_H
#define QUANTITYSPINBOX_P_H

#include <QLabel>
#include <QMouseEvent>

class ExpressionLabel : public QLabel
{
    Q_OBJECT
public:
    ExpressionLabel(QWidget * parent) : QLabel(parent) { }

    void setExpressionText(const QString& text) {
        if (text.isEmpty())
            this->setToolTip(genericExpressionEditorTooltip);
        else
            this->setToolTip(expressionEditorTooltipPrefix + text);
    }

protected:
    void mouseReleaseEvent(QMouseEvent * event) override {
        if (rect().contains(event->pos()))
                Q_EMIT clicked();
    }

Q_SIGNALS:
    void clicked();

private:

    const QString genericExpressionEditorTooltip = tr("Enter an expression... (=)");
    const QString expressionEditorTooltipPrefix = tr("Expression:") + QLatin1String(" ");
};

#endif // QUANTITYSPINBOX_P_H
