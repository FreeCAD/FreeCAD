// SPDX-License-Identifier: LGPL-2.1-only OR GPL-3.0-only

/****************************************************************************
**
** This file is part of a Qt Solutions component.
**
** Copyright (c) 2009 Nokia Corporation and/or its subsidiary(-ies).
**
** Contact:  Qt Software Information (qt-info@nokia.com)
**
** Commercial Usage
** Licensees holding valid Qt Commercial licenses may use this file in
** accordance with the Qt Solutions Commercial License Agreement provided
** with the Software or, alternatively, in accordance with the terms
** contained in a written agreement between you and Nokia.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 as published by the Free Software
** Foundation and appearing in the file LICENSE.LGPL included in the
** packaging of this file.  Please review the following information to
** ensure the GNU Lesser General Public License version 2.1 requirements
** will be met: http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, Nokia gives you certain
** additional rights. These rights are described in the Nokia Qt LGPL
** Exception version 1.0, included in the file LGPL_EXCEPTION.txt in this
** package.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU
** General Public License version 3.0 as published by the Free Software
** Foundation and appearing in the file LICENSE.GPL included in the
** packaging of this file.  Please review the following information to
** ensure the GNU General Public License version 3.0 requirements will be
** met: http://www.gnu.org/copyleft/gpl.html.
**
** Please note Third Party Software included with Qt Solutions may impose
** additional restrictions and it is the user's responsibility to ensure
** that they have met the licensing requirements of the GPL, LGPL, or Qt
** Solutions Commercial license and the relevant license of the Third
** Party Software they are using.
**
** If you are unsure which license is appropriate for your use, please
** contact the sales department at qt-sales@nokia.com.
**
****************************************************************************/

// clang-format off
#pragma once
#include <QPushButton>
#include <QtCore/QString>
#include <QtGui/QColor>

#include <QLabel>
#include <QtCore/QEvent>
#include <QtGui/QFocusEvent>

#if defined(Q_OS_WIN)
#  if !defined(QT_QTCOLORPICKER_EXPORT) && !defined(QT_QTCOLORPICKER_IMPORT)
#    define QT_QTCOLORPICKER_EXPORT
#  elif defined(QT_QTCOLORPICKER_IMPORT)
#    if defined(QT_QTCOLORPICKER_EXPORT)
#      undef QT_QTCOLORPICKER_EXPORT
#    endif
#    define QT_QTCOLORPICKER_EXPORT __declspec(dllimport)
#  elif defined(QT_QTCOLORPICKER_EXPORT)
#    undef QT_QTCOLORPICKER_EXPORT
#    define QT_QTCOLORPICKER_EXPORT __declspec(dllexport)
#  endif
#else
#  define QT_QTCOLORPICKER_EXPORT
#endif

class ColorPickerPopup;

class QT_QTCOLORPICKER_EXPORT QtColorPicker : public QPushButton
{
    Q_OBJECT

    Q_PROPERTY(bool colorDialog READ colorDialogEnabled WRITE setColorDialogEnabled) // clazy:exclude=qproperty-without-notify

public:
    QtColorPicker(QWidget *parent = nullptr,
                  int columns = -1, bool enableColorDialog = true);

    ~QtColorPicker() override;

    void insertColor(const QColor &color, const QString &text = QString(), int index = -1);

    QColor currentColor() const;

    QColor color(int index) const;

    void setColorDialogEnabled(bool enabled);
    bool colorDialogEnabled() const;

    void setStandardColors();

    static QColor getColor(const QPoint &pos, bool allowCustomColors = true);

public Q_SLOTS:
    void setCurrentColor(const QColor &col);

Q_SIGNALS:
    void colorChanged(const QColor &);
    void colorSet(const QColor &);

protected:
    void paintEvent(QPaintEvent *e) override;

private Q_SLOTS:
    void buttonPressed(bool toggled);
    void popupClosed();

private:
    ColorPickerPopup *popup;
    QColor col;
    bool withColorDialog;
    bool dirty;
    bool firstInserted;
};

// clang-format on
