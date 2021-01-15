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
  , exprFuncDisabler(false)
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

    timer.setSingleShot(true);
    connect(&timer, SIGNAL(timeout()), this, SLOT(onTimer()));

    // Set document object on line edit to create auto completer
    DocumentObject * docObj = path.getDocumentObject();
    ui->expression->setDocumentObject(docObj);

    // There are some platforms where setting no system background causes a black
    // rectangle to appear. To avoid this the 'NoSystemBackground' parameter can be
    // set to false. Then a normal non-modal dialog will be shown instead (#0002440).
    this->noBackground = ExprParams::NoSystemBackground();

    if (this->noBackground) {
        ui->expression->setStyleSheet(QLatin1String("margin:0px"));

        setWindowFlags(Qt::Dialog | Qt::FramelessWindowHint);
        setAttribute(Qt::WA_NoSystemBackground, true);
        setAttribute(Qt::WA_TranslucentBackground, true);
        layout()->setContentsMargins(0,0,0,0);

        auto hGrp = App::GetApplication().GetParameterGroupByPath(
                "User parameter:BaseApp/Preferences/MainWindow");
        QString mainstyle = QString::fromLatin1(hGrp->GetASCII("StyleSheet").c_str());
        uint checkboxColor;
        if (mainstyle.indexOf(QLatin1String("dark"),0,Qt::CaseInsensitive) >= 0) {
            this->background = QString::fromLatin1("background:#6e6e6e");
            checkboxColor = 0x505050;
        } else {
            this->background = QString::fromLatin1("background:#f5f5f5");
            checkboxColor = 0xc3c3c3;
        }
        QString checkboxStyle = QString::fromLatin1(
                "%1;border:1px solid #%2;border-radius:3px")
            .arg(background).arg(checkboxColor,6,16,QLatin1Char('0'));
        ui->checkBoxWantReturn->setStyleSheet(checkboxStyle);
        ui->checkBoxEvalFunc->setStyleSheet(checkboxStyle);
    } else
        this->background = QString::fromLatin1("background:transparent");

    ui->msg->setStyleSheet(this->background);

    auto hGrp = App::GetApplication().GetParameterGroupByPath(
            "User parameter:BaseApp/Preferences/OutputWindow");
    uint color = hGrp->GetUnsigned("colorLogging", 0xffff);
    color >>= 8;
    colorLog = QString::fromLatin1("color:#%1").arg(color, 6, 16, QLatin1Char('0'));

    color = hGrp->GetUnsigned("colorWarning", 0xffaa00ff);
    color >>= 8;
    colorWarning = QString::fromLatin1("color:#%1").arg(color, 6, 16, QLatin1Char('0'));

    color = hGrp->GetUnsigned("colorError", 0xff0000ff);
    color >>= 8;
    colorError = QString::fromLatin1("color:#%1").arg(color, 6, 16, QLatin1Char('0'));

    qApp->installEventFilter(this);
    this->onTimer();
    ui->expression->setFocus();

    ui->splitter->setStretchFactor(0, 0);
    ui->splitter->setStretchFactor(1, 2);

    bool wantreturn = ExprParams::AllowReturn() || ui->expression->document()->blockCount()>1;
    ui->checkBoxWantReturn->setChecked(wantreturn);
    if (wantreturn) {
        adjustingExpressionSize = true;
        timer.start(100);
    } else
        adjustExpressionSize();

    connect(ui->checkBoxWantReturn, SIGNAL(toggled(bool)), this, SLOT(wantReturnChecked(bool)));
    connect(ui->checkBoxEvalFunc, SIGNAL(toggled(bool)), this, SLOT(evalFuncChecked(bool)));

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
    timer.start(300);
}

void DlgExpressionInput::onTimer()
{
    if (adjustingExpressionSize) {
        adjustingExpressionSize = false;
        adjustExpressionSize();
    }

    try {
        const QString &text = ui->expression->toPlainText();
        boost::shared_ptr<Expression> expr(
                Expression::parse(path.getDocumentObject(), text.toUtf8().constData()));

        if (expr) {
            std::string error = path.getDocumentObject()->ExpressionEngine.validateExpression(path, expr);

            if (error.size() > 0)
                throw Base::RuntimeError(error.c_str());

            std::unique_ptr<Expression> result(expr->eval());

            expression = expr;
            ui->okBtn->setEnabled(true);
            ui->msg->setPlainText(QString());
            ui->msg->setStyleSheet(QString::fromLatin1("*{%1;%2}").arg(background,colorLog));

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
                    ui->msg->setStyleSheet(QString::fromLatin1("*{%1;%2}").arg(background,colorWarning));
                }

                ui->msg->setPlainText(msg);
            }
            else
                ui->msg->setPlainText(Base::Tools::fromStdString(result->toString()));
        }
    }
    catch (App::ExpressionFunctionDisabledException &) {
        ui->msg->setStyleSheet(QString::fromLatin1("*{%1;%2}").arg(background,colorWarning));
        ui->msg->setPlainText(tr("Function evaluation and attribute writting are disabled while editing. "
                                 "You can enable it by checking 'Evaluate function' here. "
                                 "Be aware that invoking function may cause unexpected change "
                                 "to various objects."));
        ui->okBtn->setDisabled(false);
    }
    catch (Base::Exception & e) {
        ui->msg->setStyleSheet(QString::fromLatin1("*{%1;%2}").arg(background,colorError));
        ui->msg->setPlainText(QString::fromUtf8(e.what()));
        ui->okBtn->setDisabled(true);
    }
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

void DlgExpressionInput::closeEvent(QCloseEvent* ev)
{
    QDialog::closeEvent(ev);
    this->exprFuncDisabler.setActive(false);
}

void DlgExpressionInput::hideEvent(QHideEvent* ev)
{
    QDialog::hideEvent(ev);
    this->exprFuncDisabler.setActive(false);
}

void DlgExpressionInput::showEvent(QShowEvent* ev)
{
    QDialog::showEvent(ev);
    this->exprFuncDisabler.setActive(!ExprParams::EvalFuncOnEdit());

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

void DlgExpressionInput::evalFuncChecked(bool checked)
{
    ExprParams::setEvalFuncOnEdit(checked);
    this->exprFuncDisabler.setActive(!checked);
    if (checked)
        timer.start(300);
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
    if (checked)
        adjustExpressionSize();
}

void DlgExpressionInput::adjustExpressionSize()
{
    int height;
    if (!ui->checkBoxWantReturn->isChecked())
        height = 30;
    else {
        const QString &text = ui->expression->toPlainText();
        auto textdoc = ui->expression->document();
        int linecount = textdoc->blockCount();
        if (linecount < 3)
            linecount = 3;
        else if (linecount > 6)
            linecount = 6;

        QFontMetrics fm (textdoc->defaultFont());
        QMargins margins = ui->expression->contentsMargins();
        height = fm.lineSpacing () * linecount
            + (textdoc->documentMargin() + ui->expression->frameWidth ()) * 2
            + margins.top () + margins.bottom ();
        if (height < ui->expression->height())
            return;
    }

    int offset = height - ui->expression->height();
    auto sizes = ui->splitter->sizes();
    sizes[0] += offset;
    if (offset > 0) {
        QSize s = this->size();
        s.setHeight(s.height() + offset);
        resize(s);
    }
    ui->splitter->setSizes(sizes);
}

#include "moc_DlgExpressionInput.cpp"
