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
  , exprFuncDisabler(false)
{
    assert(path.getDocumentObject() != 0);

    // Setup UI
    ui->setupUi(this);

    // Connect signal(s)
    connect(ui->expression, SIGNAL(textChanged()), this, SLOT(textChanged()));
    connect(ui->discardBtn, SIGNAL(clicked()), this, SLOT(setDiscarded()));

    setMouseTracking(true);

    if (expression) {
        ui->expression->setPlainText(Base::Tools::fromStdString(expression->toString()));
    }
    else {
        QVariant text = parent->property("text");
#if QT_VERSION >= 0x050000
        if (text.canConvert(QMetaType::QString))
#else
        if (text.canConvert(QVariant::String))
#endif
        {
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

    auto hGrp = App::GetApplication().GetParameterGroupByPath(
            "User parameter:BaseApp/Preferences/MainWindow");
    QString mainstyle = QString::fromLatin1(hGrp->GetASCII("StyleSheet").c_str());
    bool darkstyle = mainstyle.indexOf(QLatin1String("dark"),0,Qt::CaseInsensitive) >= 0;
    if (darkstyle) {
        this->stylesheet = QLatin1String("*{color:palette(bright-text)}");
        ui->expression->setStyleSheet(QLatin1String("margin:0px; color:black"));
    } else {
        this->stylesheet = QLatin1String("*{color:palette(text)}");
        ui->expression->setStyleSheet(QLatin1String("margin:0px"));
    }
    if (this->noBackground) {
        // Both OSX and Windows will pass through mouse event on complete
        // transparent top level widgets. So we use an almost transparent child
        // proxy widget to force the OS to capture mouse event.
        proxy = new ProxyWidget(this);
        proxy->show();
        proxy->lower();
        proxy->resize(this->rect().size());

        setWindowFlags(Qt::Dialog | Qt::FramelessWindowHint);
        setAttribute(Qt::WA_NoSystemBackground, true);
        setAttribute(Qt::WA_TranslucentBackground, true);
        layout()->setContentsMargins(4,10,4,4);

        uint borderColor;
        if (darkstyle) {
            this->msgStyle = QString::fromLatin1("background:#6e6e6e");
            this->backgroundColor.setRgb(0x6e6e6e);
            borderColor = 0x505050;
        } else {
            this->msgStyle = QString::fromLatin1("background:#f5f5f5");
            borderColor = 0xc3c3c3;
            this->backgroundColor.setRgb(0xf5f5f5);
        }
        QString checkboxStyle = QString::fromLatin1(
                "%1;border:1px solid #%2;border-radius:3px")
            .arg(msgStyle).arg(borderColor,6,16,QLatin1Char('0'));
        ui->checkBoxWantReturn->setStyleSheet(checkboxStyle);
        ui->checkBoxEvalFunc->setStyleSheet(checkboxStyle);
        this->borderColor.setRgb(borderColor);
        msgStyle += QString::fromLatin1(";border:none");
    } else
        this->msgStyle = QString::fromLatin1("background:transparent");

    /// Some qt version seems having trouble apply the stylesheet on creation
    QTimer::singleShot(300, this, SLOT(applyStylesheet()));

    ui->msg->setStyleSheet(this->msgStyle);

    hGrp = App::GetApplication().GetParameterGroupByPath(
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
    adjustingExpressionSize = true;
    timer.start(100);

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

void DlgExpressionInput::resizeEvent(QResizeEvent *ev)
{
    if (proxy)
        proxy->resize(this->rect().size());
    QDialog::resizeEvent(ev);
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
        minSize = minimumSize();
        if (this->noBackground)
            setFixedSize(width(), height());
    }

    try {
        const QString &text = ui->expression->toPlainText();
        if (text.isEmpty()) {
            ui->msg->setPlainText(QString());
            return;
        }
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
            ui->msg->setStyleSheet(QString::fromLatin1("*{%1;%2}").arg(msgStyle,colorLog));

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
                    ui->msg->setStyleSheet(QString::fromLatin1("*{%1;%2}").arg(msgStyle,colorWarning));
                }

                ui->msg->setPlainText(msg);
            }
            else
                ui->msg->setPlainText(Base::Tools::fromStdString(result->toString()));
        }
    }
    catch (App::ExpressionFunctionDisabledException &) {
        ui->msg->setStyleSheet(QString::fromLatin1("*{%1;%2}").arg(msgStyle,colorWarning));
        ui->msg->setPlainText(tr("Function evaluation and attribute writing are disabled while editing. "
                                 "You can enable it by checking 'Evaluate function' here. "
                                 "Be aware that invoking function may cause unexpected change "
                                 "to various objects."));
        ui->okBtn->setDisabled(false);
    }
    catch (Base::Exception & e) {
        ui->msg->setStyleSheet(QString::fromLatin1("*{%1;%2}").arg(msgStyle,colorError));
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
    (void)width;
    adjustPosition();
}

void DlgExpressionInput::applyStylesheet()
{
    this->setStyleSheet(stylesheet);
}

enum HitPos {
    HitNone,
    HitTop = 1,
    HitBottom = 2,
    HitLeft = 4,
    HitRight = 8,
};

int DlgExpressionInput::hitTest(const QPoint &point)
{
    if (!this->noBackground)
        return 0;

    int margin = layout()->contentsRect().left()+1;
    int res = 0;
    if (point.x() < margin) {
        res |= HitLeft;
        if (point.y() < margin) {
            res |= HitTop;
            setCursor(Qt::SizeFDiagCursor);
        } else if (point.y() > height() - margin) {
            res |= HitBottom;
            setCursor(Qt::SizeBDiagCursor);
        } else
            setCursor(Qt::SizeHorCursor);
    } else if (point.x() > width() - margin) {
        res |= HitRight;
        if (point.y() < margin) {
            res |= HitTop;
            setCursor(Qt::SizeBDiagCursor);
        } else if (point.y() > height() - margin) {
            res |= HitBottom;
            setCursor(Qt::SizeFDiagCursor);
        } else
            setCursor(Qt::SizeHorCursor);
    } else if (point.y() < margin) {
        res = HitTop;
        setCursor(Qt::SizeAllCursor);
    } else if (point.y() > height() - margin) {
        res = HitBottom;
        setCursor(Qt::SizeVerCursor);
    } else
        unsetCursor();
    return res;
}

void DlgExpressionInput::mouseReleaseEvent(QMouseEvent* ev)
{
    if (dragging && ev->button() == Qt::LeftButton) {
        unsetCursor();
        dragging = 0;
        minSize = minimumSize();
        setFixedSize(width(), height());
    }
}

void DlgExpressionInput::mousePressEvent(QMouseEvent* ev)
{
    if (ev->buttons() != Qt::LeftButton)
        return;
    if ((dragging = hitTest(ev->pos()))) {
        lastPos = ev->globalPos();
        setMinimumSize(minSize);
        setMaximumSize(QWIDGETSIZE_MAX, QWIDGETSIZE_MAX);
    }
}

void DlgExpressionInput::mouseMoveEvent(QMouseEvent *ev)
{
    if (!dragging) {
        hitTest(ev->pos());
        return;
    }
    QPoint offset = ev->globalPos() - lastPos;
    if (dragging == HitTop) {
        lastPos += offset;
        move(this->pos() + offset);
        return;
    }
    QRect rect = this->geometry();
    QSize maxSize = getMainWindow()->size();
    if (dragging & HitTop) {
        if (rect.height() - offset.y() >= minimumHeight()
            && rect.height() - offset.y() <= maxSize.height())
        {
            lastPos.setY(lastPos.y() + offset.y());
            rect.setTop(rect.top() + offset.y());
        }
    } else if (dragging & HitBottom) {
        if (rect.height() + offset.y() >= minimumHeight()
            && rect.height() + offset.y() <= maxSize.height())
        {
            lastPos.setY(lastPos.y() + offset.y());
            rect.setBottom(rect.bottom() + offset.y());
        }
    }
    if (dragging & HitLeft) {
        if (rect.width() - offset.x() >= minimumWidth()
            && rect.width() - offset.x() <= maxSize.width())
        {
            lastPos.setX(lastPos.x() + offset.x());
            rect.setLeft(rect.left() + offset.x());
        }
    } else if (dragging & HitRight) {
        if (rect.width() + offset.x() >= minimumWidth()
            && rect.width() + offset.x() <= maxSize.width())
        {
            lastPos.setX(lastPos.x() + offset.x());
            rect.setRight(rect.right() + offset.x());
        }
    }
    setGeometry(rect);
}

void DlgExpressionInput::show()
{
    QDialog::show();
    this->activateWindow();
    ui->expression->selectAll();
}

void DlgExpressionInput::onClose()
{
    this->exprFuncDisabler.setActive(false);
    ExprParams::setEditDialogWidth(this->width());
    ExprParams::setEditDialogHeight(this->height());
    ExprParams::setEditDialogTextHeight(this->ui->expression->height());
}

void DlgExpressionInput::closeEvent(QCloseEvent* ev)
{
    QDialog::closeEvent(ev);
    onClose();
}

void DlgExpressionInput::hideEvent(QHideEvent* ev)
{
    QDialog::hideEvent(ev);
    onClose();
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
    if (!obj->isWidgetType())
        return false;

    if (ev->type() == QEvent::MouseMove && obj != this)
        unsetCursor();

    // if the user clicks on a widget different to this
    if (this->noBackground && ev->type() == QEvent::MouseButtonPress) {
        auto widget = static_cast<QWidget*>(obj);
        for (auto w=widget; w; w=w->parentWidget()) {
            if (w == this)
                return false;
        }
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
    auto mw = getMainWindow();
    QPoint topLeft = mw->mapToGlobal(QPoint(0, 0));
    QPoint rightBottom = mw->mapToGlobal(QPoint(mw->width(), mw->height()));
    pos.setX(std::min(std::max(pos.x(), topLeft.x()), rightBottom.x()));
    if (this->leftAligned) {
        QPoint offset = ui->expression->mapTo(this, 
                QPoint(ui->expression->frameWidth(),ui->expression->frameWidth()));
        pos -= offset;
    } else {
        QPoint offset = ui->expression->mapTo(this, 
                QPoint(-ui->expression->frameWidth(),-ui->expression->frameWidth()));
        pos -= offset;
        QPoint parentRightPos = parent->mapToGlobal(QPoint(parent->width(), parent->height()));
        parentRightPos.setX(std::min(parentRightPos.x(), rightBottom.x()));
        if (this->noBackground) {
            parentRightPos = parent->mapFromGlobal(parentRightPos);
            QSize sz = ui->expression->frameGeometry().size();
            pos += parentRightPos - QPoint(sz.width(), sz.height());
        } else {
            if (pos.x() + this->width() > parentRightPos.x())
                pos.setX(parentRightPos.x() - this->width());
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
    if (!ui->checkBoxWantReturn->isChecked()) {
        // leave space for longer line
        ui->expression->setLineWrapMode(QPlainTextEdit::WidgetWidth);
        ui->expression->setMinimumHeight(35);
        height = 35;
    } else {
        ui->expression->setLineWrapMode(QPlainTextEdit::NoWrap);
        const QString &text = ui->expression->toPlainText();
        auto textdoc = ui->expression->document();
        int linecount = textdoc->blockCount();
        if (linecount < 4)
            linecount = 4;
        else if (linecount > 8)
            linecount = 8;

        QFontMetrics fm (textdoc->defaultFont());
        QMargins margins = ui->expression->contentsMargins();
        height = fm.lineSpacing () * linecount
            + (textdoc->documentMargin() + ui->expression->frameWidth ()) * 2
            + margins.top () + margins.bottom ();
    }

    if (height < ExprParams::EditDialogTextHeight())
        height = ExprParams::EditDialogTextHeight();
    if (height < ui->expression->minimumHeight())
        height = ui->expression->minimumHeight();

    bool doResize = false;
    QSize s = this->size();
    auto sizes = ui->splitter->sizes();
    int offset = height - ui->expression->height();
    if (offset != 0) {
        sizes[0] += offset;
        if (offset > 0) {
            s.setHeight(s.height() + offset);
            doResize = true;
        }
    }
    if (s.height() < ExprParams::EditDialogHeight()) {
        s.setHeight(ExprParams::EditDialogHeight());
        doResize = true;
    }
    if (s.width() < ExprParams::EditDialogWidth()) {
        s.setWidth(ExprParams::EditDialogWidth());
        doResize = true;
    }
    if (doResize)
        resize(s);
    if (offset != 0) {
        ui->splitter->setSizes(sizes);
        adjustPosition();
    }
}

void ProxyWidget::paintEvent(QPaintEvent *)
{
    QColor backgroundColor(master->backgroundColor);
    backgroundColor.setAlpha(ExprParams::EditDialogBGAlpha());
    QPainter painter(this);
    QBrush brush(backgroundColor);
    painter.setBrush(brush);
    painter.setPen(master->borderColor);
    painter.drawRoundedRect(0, 0, width()-1, height()-1, 3, 3);

    painter.setPen(Qt::NoPen);
    backgroundColor.setAlpha(255);
    brush.setColor(backgroundColor);
    painter.setBrush(brush);
    painter.drawRoundedRect(0, 0, width()-1, 8, 3, 3);

    brush.setStyle(Qt::Dense4Pattern);
    brush.setColor(QColor(0,0,0));
    painter.setBrush(brush);
    painter.drawRoundedRect(0, 0, width()-1, 8, 3, 3);
}

#include "moc_DlgExpressionInput.cpp"
