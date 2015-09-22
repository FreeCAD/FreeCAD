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

#include "PreCompiled.h"
#include <QPainter>
#include <QDesktopWidget>

#include "DlgExpressionInput.h"
#include "ui_DlgExpressionInput.h"
#include "ExpressionCompleter.h"
#include <Base/Tools.h>
#include <App/Expression.h>
#include <App/DocumentObject.h>

using namespace App;
using namespace Gui::Dialog;

const int DlgExpressionInput::h = 15;
const int DlgExpressionInput::r = 30;
const int DlgExpressionInput::d = 7;

DlgExpressionInput::DlgExpressionInput(const App::ObjectIdentifier & _path, boost::shared_ptr<const Expression> _expression, const Base::Unit & _impliedUnit, QWidget *parent) :
    QDialog(parent),
    ui(new Ui::DlgExpressionInput),
    expression(_expression ? _expression->copy() : 0),
    path(_path),
    discarded(false),
    impliedUnit(_impliedUnit),
    l(30)
{
    assert(path.getDocumentObject() != 0);

    // Setup UI
    ui->setupUi(this);

    if (expression) {
        ui->expression->setText(Base::Tools::fromStdString(expression->toString()));
        textChanged(Base::Tools::fromStdString(expression->toString()));
    }

    // Connect signal(s)
    connect(ui->expression, SIGNAL(textChanged(QString)), this, SLOT(textChanged(QString)));
    connect(ui->discardBtn, SIGNAL(clicked()), this, SLOT(setDiscarded()));

    // Set document object on line edit to create auto completer
    DocumentObject * docObj = path.getDocumentObject();
    ui->expression->setDocumentObject(docObj);

    setWindowFlags(Qt::Widget | Qt::FramelessWindowHint);
    setAttribute(Qt::WA_NoSystemBackground, true);
    setAttribute(Qt::WA_TranslucentBackground, true);
    setParent(0);

    ui->expression->setFocus();

    QDesktopWidget widget;
    setMinimumWidth(widget.availableGeometry(widget.primaryScreen()).width()/2);

#ifndef FC_DEBUG
    ui->parsedExpr->setVisible(false);
#endif

}

DlgExpressionInput::~DlgExpressionInput()
{
    delete ui;
}

QPoint DlgExpressionInput::tip() const
{
    return QPoint(l - d, 0);
}

void DlgExpressionInput::setGeometry(int x, int y, int w, int h)
{
    QDesktopWidget widget;
    int screenWidth = widget.availableGeometry(widget.primaryScreen()).width();

    if (x + w > screenWidth) {
        l = l + (x + w - screenWidth);
        x = screenWidth - w - 10;
    }

    QWidget::setGeometry(x, y, w, h);
}

void DlgExpressionInput::paintEvent(QPaintEvent * event) {
    QPainter painter(this);
    QPainterPath path;

    path.moveTo(0, h + r / 2);
    path.arcTo(QRect(0, h, r, r), 180, -90);
    path.lineTo(l, h);
    path.lineTo(l - d, 0);
    path.lineTo(l + d, h);
    path.lineTo(width() - r - 1, h);
    path.arcTo(QRect(width() - r - 1, h, r, r), 90, -90);
    path.lineTo(width() - 1, height() - r);
    path.arcTo(QRect(width() - r - 1, height() - r - 1, r, r), 0, -90);
    path.lineTo(r, height() - 1);
    path.arcTo(QRect(0, height() - r - 1, r, r), -90, -90);
    path.lineTo(0, h + r/2);

    QPen pen(Qt::black);
    QBrush brush(QColor(250, 250, 180));
    pen.setWidthF(2.0);
    painter.setBrush(brush);
    painter.setPen(pen);

    painter.fillPath(path, brush);
    painter.drawPath(path);
}

void DlgExpressionInput::textChanged(const QString &text)
{
    try {
        boost::shared_ptr<Expression> expr(ExpressionParser::parse(path.getDocumentObject(), text.toUtf8().constData()));

        if (expr) {
            std::string error = path.getDocumentObject()->ExpressionEngine.validateExpression(path, expr);

            if (error.size() > 0)
                throw Base::Exception(error.c_str());

#ifdef FC_DEBUG
            ui->parsedExpr->setText(Base::Tools::fromStdString(expr->toString()));
#endif

            std::auto_ptr<Expression> result(expr->eval());

            expression = expr;
            ui->okBtn->setEnabled(true);
            ui->errorMsg->setText(QString::fromUtf8(""));

            NumberExpression * n = Base::freecad_dynamic_cast<NumberExpression>(result.get());
            if (n) {
                Base::Quantity value = n->getQuantity();

                if (!value.getUnit().isEmpty() && value.getUnit() != impliedUnit)
                    throw Base::Exception("Unit mismatch between result and required unit");

                value.setUnit(impliedUnit);

                ui->result->setText(value.getUserString());
            }
            else
                ui->result->setText(Base::Tools::fromStdString(result->toString()));
        }
    }
    catch (Base::Exception & e) {
        ui->errorMsg->setText(QString::fromUtf8(e.what()));
        QPalette p(ui->errorMsg->palette());
        p.setColor(QPalette::WindowText, Qt::red);
        ui->errorMsg->setPalette(p);
        ui->okBtn->setDisabled(true);
        ui->result->setText(QString::fromAscii("--"));
    }
}

void DlgExpressionInput::setDiscarded()
{
    discarded = true;
    reject();
}

#include "moc_DlgExpressionInput.cpp"
