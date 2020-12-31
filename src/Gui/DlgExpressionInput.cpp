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
#include <QApplication>
#include <QPainter>
#include <QDesktopWidget>
#include <QMenu>
#include <QMouseEvent>

#include "DlgExpressionInput.h"
#include "ui_DlgExpressionInput.h"
#include "ExpressionCompleter.h"
#include "Tools.h"
#include "MainWindow.h"
#include "ExprParams.h"
#include <Base/Tools.h>
#include <Base/Console.h>
#include <App/Application.h>
#include <App/ExpressionParser.h>
#include <App/DocumentObject.h>

FC_LOG_LEVEL_INIT("Gui", true, true)

using namespace App;
using namespace Gui::Dialog;

DlgExpressionInput::DlgExpressionInput(const App::ObjectIdentifier & _path,
                                       boost::shared_ptr<const Expression> _expression,
                                       const Base::Unit & _impliedUnit, QWidget *parent)
  : QDialog(parent)
  , ui(new Ui::DlgExpressionInput)
  , expression(_expression ? _expression->copy() : 0)
  , path(_path)
  , discarded(false)
  , impliedUnit(_impliedUnit)
  , minimumWidth(10)
{
    assert(path.getDocumentObject() != 0);

    // Setup UI
    ui->setupUi(this);

    // Connect signal(s)
    connect(ui->expression, SIGNAL(textChanged()), this, SLOT(textChanged()));
    connect(ui->discardBtn, SIGNAL(clicked()), this, SLOT(setDiscarded()));
    
    if (expression) {
        ui->expression->setPlainText(Base::Tools::fromStdString(expression->toString()));
    }
    else {
        QVariant text = parent->property("text");
#if QT_VERSION >= 0x050000
        if (text.canConvert(QMetaType::QString)) {
#else
        if (text.canConvert(QVariant::String)) {
#endif
            ui->expression->setPlainText(text.toString());
        }
    }

    // Set document object on line edit to create auto completer
    DocumentObject * docObj = path.getDocumentObject();
    ui->expression->setDocumentObject(docObj);

    // There are some platforms where setting no system background causes a black
    // rectangle to appear. To avoid this the 'NoSystemBackground' parameter can be
    // set to false. Then a normal non-modal dialog will be shown instead (#0002440).
    this->noBackground = ExprParams::NoSystemBackground();

    ui->expression->setStyleSheet(QLatin1String("margin:0px"));

    if (this->noBackground) {
        setWindowFlags(Qt::Dialog | Qt::FramelessWindowHint);
        setAttribute(Qt::WA_NoSystemBackground, true);
        setAttribute(Qt::WA_TranslucentBackground, true);

        auto hGrp = App::GetApplication().GetParameterGroupByPath(
                "User parameter:BaseApp/Preferences/MainWindow");
        QString mainstyle = QString::fromLatin1(hGrp->GetASCII("StyleSheet").c_str());
        if (!mainstyle.isEmpty()) {
            if(mainstyle.indexOf(QLatin1String("dark"),0,Qt::CaseInsensitive) < 0)
                ui->scrollArea->setStyleSheet(QLatin1String("QScrollArea{background:#f5f5f5;border:1 solid #6e6e6e;}"));
            else
                ui->scrollArea->setStyleSheet(QLatin1String("QScrollArea{background:#6e6e6e;border:1 solid #505050;}"));
        }
    }
    qApp->installEventFilter(this);
    this->setMinimumHeight(120);
    this->textChanged();
    ui->expression->setFocus();

    ui->checkBox->setChecked(ExprParams::AllowReturn()
            || ui->expression->document()->blockCount()>1);
    connect(ui->checkBox, SIGNAL(toggled(bool)), this, SLOT(wantReturnChecked(bool)));

    if (parent) {
        MainWindow *mw = getMainWindow();
        for(auto p=parent; p; p=p->parentWidget()) {
            if (p == mw) {
                QPoint topLeft = parent->mapToGlobal(QPoint(0, 0));
                QPoint bottomRight = parent->mapToGlobal(QPoint(parent->width(), parent->height()));
                int offset = mw->pos().x();
                // Check if the parent widget is closer to the left or the
                // right edge of the main window.
                if (topLeft.x() + bottomRight.x() - offset*2 > mw->width())
                    this->leftAligned = false;

                break;
            }
        }
    }
}

DlgExpressionInput::~DlgExpressionInput()
{
    qApp->removeEventFilter(this);
    delete ui;
}

void DlgExpressionInput::textChanged()
{
    try {
        const QString &text = ui->expression->toPlainText();

        auto textdoc = ui->expression->document();
        int linecount = textdoc->blockCount();
        if (linecount == 0)
            linecount = 1;
        else if (linecount > 5)
            linecount = 5;

        QFontMetrics fm (textdoc->defaultFont());
        QMargins margins = ui->expression->contentsMargins();
        int height = fm.lineSpacing () * linecount
            + (textdoc->documentMargin() + ui->expression->frameWidth ()) * 2
            + margins.top () + margins.bottom ();
        ui->expression->setFixedHeight(height);

        //resize the input field according to text size
        // QFontMetrics fm(ui->expression->font());
        // int width = QtTools::horizontalAdvance(fm, text) + 15;
        // if (width < minimumWidth)
        //     ui->expression->setMinimumWidth(minimumWidth);
        // else
        //     ui->expression->setMinimumWidth(width);
        //
        // if(this->width() < ui->expression->minimumWidth())
        //     setMinimumWidth(ui->expression->minimumWidth());

        //now handle expression
        boost::shared_ptr<Expression> expr(Expression::parse(path.getDocumentObject(), text.toUtf8().constData()));

        if (expr) {
            std::string error = path.getDocumentObject()->ExpressionEngine.validateExpression(path, expr);

            if (error.size() > 0)
                throw Base::RuntimeError(error.c_str());

            std::unique_ptr<Expression> result(expr->eval());

            expression = expr;
            ui->okBtn->setEnabled(true);
            ui->msg->clear();

            ui->msg->setStyleSheet(QString::fromLatin1("*{color: blue}"));

            NumberExpression * n = Base::freecad_dynamic_cast<NumberExpression>(result.get());
            if (n) {
                Base::Quantity value = n->getQuantity();
                QString msg = value.getUserString();

                if (!value.isValid()) {
                    throw Base::ValueError("Not a number");
                }
                else if (!impliedUnit.isEmpty()) {
                    if (!value.getUnit().isEmpty() && value.getUnit() != impliedUnit)
                        throw Base::UnitsMismatchError("Unit mismatch between result and required unit");

                    value.setUnit(impliedUnit);

                }
                else if (!value.getUnit().isEmpty()) {
                    msg += QString::fromUtf8(" (Warning: unit discarded)");
                    ui->msg->setStyleSheet(QString::fromLatin1("*{color: red}"));
                }

                ui->msg->setText(msg);
            }
            else
                ui->msg->setText(Base::Tools::fromStdString(result->toString()));
        }
    }
    catch (Base::Exception & e) {
        ui->msg->setText(QString::fromUtf8(e.what()));
        ui->msg->setStyleSheet(QString::fromLatin1("*{color: red}"));
        ui->okBtn->setDisabled(true);
    }
    this->adjustSize();
}

void DlgExpressionInput::setDiscarded()
{
    discarded = true;
    reject();
}

void DlgExpressionInput::setExpressionInputSize(int width, int height)
{
    (void)height;
    minimumWidth = width;
    adjustPosition();
}

void DlgExpressionInput::mouseReleaseEvent(QMouseEvent* ev)
{
#if 0//defined(Q_OS_WIN)
    if (QWidget::mouseGrabber() == this) {
        QList<QWidget*> childs = this->findChildren<QWidget*>();
        for (QList<QWidget*>::iterator it = childs.begin(); it != childs.end(); ++it) {
            QPoint pos = (*it)->mapFromGlobal(ev->globalPos());
            if ((*it)->rect().contains(pos)) {
                // Create new mouse event with the correct local position
                QMouseEvent me(ev->type(), pos, ev->globalPos(), ev->button(), ev->buttons(), ev->modifiers());
                QObject* obj = *it;
                obj->event(&me);
                if (me.isAccepted()) {
                    break;
                }
            }
        }
    }
#else
    Q_UNUSED(ev);
#endif
}

void DlgExpressionInput::mousePressEvent(QMouseEvent* ev)
{
#if 0//defined(Q_OS_WIN)
    bool handled = false;
    if (QWidget::mouseGrabber() == this) {
        QList<QWidget*> childs = this->findChildren<QWidget*>();
        for (QList<QWidget*>::iterator it = childs.begin(); it != childs.end(); ++it) {
            QPoint pos = (*it)->mapFromGlobal(ev->globalPos());
            if ((*it)->rect().contains(pos)) {
                // Create new mouse event with the correct local position
                QMouseEvent me(ev->type(), pos, ev->globalPos(), ev->button(), ev->buttons(), ev->modifiers());
                QObject* obj = *it;
                obj->event(&me);
                if (me.isAccepted()) {
                    handled = true;
                    break;
                }
            }
        }
    }

    if (handled)
        return;
#else
    Q_UNUSED(ev);
#endif
    // The 'FramelessWindowHint' is also set when the background is transparent.
    if (windowFlags() & Qt::FramelessWindowHint) {
        //we need to reject the dialog when clicked on the background. As the background is transparent
        //this is the expected behaviour for the user
        bool on = ui->expression->completerActive();
        if (!on)
            this->reject();
    }
}

void DlgExpressionInput::show()
{
    QDialog::show();
    this->activateWindow();
    ui->expression->selectAll();
}

void DlgExpressionInput::showEvent(QShowEvent* ev)
{
    QDialog::showEvent(ev);

#if 0//defined(Q_OS_WIN)
    // This way we can fetch click events outside modal dialogs
    QWidget* widget = QApplication::activeModalWidget();
    if (widget) {
        QList<QWidget*> childs = widget->findChildren<QWidget*>();
        if (childs.contains(this)) {
            this->grabMouse();
        }
    }
#endif

    adjustPosition();
}

bool DlgExpressionInput::eventFilter(QObject *obj, QEvent *ev)
{
    // if the user clicks on a widget different to this
    if (ev->type() == QEvent::MouseButtonPress && obj != this) {
        // Since the widget has a transparent background we cannot rely
        // on the size of the widget. Instead, it must be checked if the
        // cursor is on this or an underlying widget or outside.
        if (!underMouse()) {
            // if the expression fields context-menu is open do not close the dialog
            QMenu* menu = qobject_cast<QMenu*>(obj);
            if (menu && menu->parentWidget() == ui->expression) {
                return false;
            }
            bool on = ui->expression->completerActive();
            // Do this only if the completer is not shown
            if (!on) {
                reject();
            }
        }
    }
    else if (ev->type() == QEvent::KeyPress && obj == ui->expression) {
        // Intercept 'Return' key if not allowed
        auto ke = static_cast<QKeyEvent*>(ev);
        bool on = ui->expression->completerActive();
        if (!on && !ExprParams::AllowReturn()
                && ke->key() == Qt::Key_Return
                && ke->modifiers() == Qt::NoModifier) {
            accept();
            return true;
        }
    }

    return false;
}

void DlgExpressionInput::adjustPosition()
{
    if (this->adjustingPosition || !this->isVisible())
        return;
    auto parent = parentWidget();
    if (!parent)
        return;
    Base::StateLocker lock(adjustingPosition);

    QPoint pos = parent->mapToGlobal(QPoint(0, 0));
    if (this->leftAligned) {
        QPoint offset = ui->expression->mapTo(this, 
                QPoint(ui->expression->frameWidth(),ui->expression->frameWidth()));
        pos -= offset;
    } else {
        QPoint offset = ui->expression->mapTo(this, 
                QPoint(-ui->expression->frameWidth(),-ui->expression->frameWidth()));
        pos -= offset;
        if (this->noBackground) {
            QPoint parentPos = QPoint(parent->width(), parent->height());
            QSize sz = ui->expression->frameGeometry().size();
            pos += parentPos - QPoint(sz.width(), sz.height());
        } else {
            QPoint parentPos = parent->mapToGlobal(QPoint(parent->width(), parent->height()));
            if (pos.x() + this->width() > parentPos.x())
                pos.setX(parentPos.x() - this->width());
        }
    }
    this->move(pos);
}

void DlgExpressionInput::wantReturnChecked(bool checked)
{
    ExprParams::setAllowReturn(checked);
}

#include "moc_DlgExpressionInput.cpp"
