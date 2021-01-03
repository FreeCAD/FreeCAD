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

#include <QShortcut>
#include <QLabel>
#include <QMouseEvent>
#include <QPixmapCache>
#include "Widgets.h"
#include "BitmapFactory.h"
#include "ExprParams.h"

class ExpressionLabel : public QLabel
{
    Q_OBJECT
public:
    enum State {
        None,
        Binding,
        Bound,
    };

    ExpressionLabel(QWidget * parent) : QLabel(parent)
    {
        hide();
        setCursor(Qt::ArrowCursor);
        /* Icon for f(x) */
        QFontMetrics fm(parent->font());
        iconHeight = fm.height() + 2;
        setStyleSheet(QString::fromLatin1(
                    "QLabel { border: none;"
                            " padding: 0px;"
                            " width: %1px;"
                            " height: %1px }").arg(iconHeight));
        setState(None);
        parent->installEventFilter(this);
        if (parent->parentWidget()) {
            const std::string & trigger = Gui::ExprParams::EditorTrigger();
            if (trigger.size() == 1) {
                shortcutKey = QString::fromLatin1(trigger.c_str());
                parent->parentWidget()->installEventFilter(this);
            } else {
                QKeySequence seq(QLatin1String(trigger.c_str()));
                if (!seq.isEmpty()) {
                    shortcut = new QShortcut(seq, parent->parentWidget());
                    connect(shortcut, SIGNAL(activated()), this, SIGNAL(clicked()));
                }
            }
        }
    }

    void setState(State state) {
        _state = state;
        QIcon icon;
        QString stylesheet;
        if (_state == Bound) {
            static QIcon icn;
            if (icn.isNull())
                icn = Gui::BitmapFactory().pixmapFromSvg("bound-expression.svg", QSize(64, 64));
            icon = icn;
            stylesheet = QString::fromLatin1("QLineEdit { padding-right: %1px } ").arg(getOffset()+1);
            show();
        } else {
            static QIcon icn;
            if (icn.isNull())
                icn = Gui::BitmapFactory().pixmapFromSvg("bound-expression-unset.svg", QSize(64, 64));
            icon = icn;
            if (_state == Binding && Gui::ExprParams::AutoHideEditorIcon())
                show();
            else
                hide();
        }
        auto parent = qobject_cast<QLineEdit*>(parentWidget());
        if (parent)
            parent->setStyleSheet(stylesheet);
        // Let QIcon.pixmap() to handle cache and HDPI
        setPixmap(icon.pixmap(iconHeight));
    }

protected:
    void mouseReleaseEvent(QMouseEvent * event) {
        if (rect().contains(event->pos()))
                Q_EMIT clicked();
    }

    bool event(QEvent *ev) {
        if (ev->type() == QEvent::ToolTip) {
            ev->accept();
            QHelpEvent* he = static_cast<QHelpEvent*>(ev);
            QString text = toolTip();
            if (text.isEmpty())
                text = tr("Press = to bring up the expression editor.");
            Gui::ToolTip::showText(he->globalPos(), text, this);
            return true;
        }
        return QLabel::event(ev);
    }

    int getOffset() {
        if (!parentWidget())
            return 0;
        return iconHeight + parentWidget()->style()->pixelMetric(QStyle::PM_SpinBoxFrameWidth);
    }

    bool eventFilter(QObject *o, QEvent *ev) {
        auto parent = parentWidget();
        if (o && o == parent) {
            switch(ev->type()) {
            case QEvent::Enter:
                if (_state != None)
                    show();
                break;
            case QEvent::Leave:
                if (_state != Bound
                        || (_state == Binding && Gui::ExprParams::AutoHideEditorIcon()))
                    hide();
                break;
            case QEvent::Resize:
                move(parent->rect().right() - getOffset(), 0);
                break;
            default:
                break;
            }
        } else if (_state != None
                && parent
                && parent->parentWidget()
                && o == parent->parentWidget()) {
            if (ev->type() == QEvent::KeyPress) {
                auto ke = static_cast<QKeyEvent*>(ev);
                if (ke->modifiers() == Qt::NoModifier
                        && ke->text().compare(shortcutKey, Qt::CaseInsensitive)==0) {
                    Q_EMIT(clicked());
                    return true;
                }
            }
        }
        return false;
    }

Q_SIGNALS:
    void clicked();

private:
    State _state;
    int iconHeight;
    QShortcut *shortcut = nullptr;
    QString shortcutKey;
};

#endif // QUANTITYSPINBOX_P_H
