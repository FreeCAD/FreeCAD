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

#ifndef GUI_DIALOG_DLGEXPRESSIONINPUT_H
#define GUI_DIALOG_DLGEXPRESSIONINPUT_H

#include <QDialog>
#include <App/ObjectIdentifier.h>
#include <Base/Unit.h>
#include <memory>

namespace Ui {
class DlgExpressionInput;
}

namespace Base {
class Quantity;
}

namespace App {
class Path;
class Expression;
class DocumentObject;
}

namespace Gui {

namespace Dialog {

class GuiExport NumberRange
{
public:
    void setRange(double minimum, double maximum);
    void clearRange();
    void throwIfOutOfRange(const Base::Quantity&) const;

private:
    double minimum{};
    double maximum{};
    bool defined{false};
};

class GuiExport DlgExpressionInput : public QDialog
{
    Q_OBJECT

public:
    explicit DlgExpressionInput(const App::ObjectIdentifier & _path, std::shared_ptr<const App::Expression> _expression, const Base::Unit &_impliedUnit, QWidget *parent = nullptr);
    ~DlgExpressionInput() override;

    void setRange(double minimum, double maximum);
    void clearRange();
    std::shared_ptr<App::Expression> getExpression() const { return expression; }

    bool discardedFormula() const { return discarded; }

    QPoint expressionPosition() const;
    void   setExpressionInputSize(int width, int height);

public Q_SLOTS:
    void show();

protected:
    void mouseReleaseEvent(QMouseEvent*) override;
    void mousePressEvent(QMouseEvent*) override;

private Q_SLOTS:
    void textChanged(const QString & text);
    void setDiscarded();

private:
    ::Ui::DlgExpressionInput *ui;
    std::shared_ptr<App::Expression> expression;
    App::ObjectIdentifier path;
    bool discarded;
    const Base::Unit impliedUnit;
    NumberRange numberRange;

    int minimumWidth;
};

}
}

#endif // GUI_DIALOG_EXPRESSIONINPUT_H
