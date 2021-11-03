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

#include <QTimer>
#include <QDialog>
#include <Base/Unit.h>
#include <App/ObjectIdentifier.h>
#include <App/ExpressionParser.h>
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

class ProxyWidget;

class GuiExport DlgExpressionInput : public QDialog
{
    Q_OBJECT

     /*
     Sample style for use in .qss file
     Colors in sample are not meant to be readable
     All properties are optional
     Ignoring Output Window settings might annoy some users, use with caution
     
     Gui--Dialog--DlgExpressionInput {
        qproperty-ignoreOutputWindowColors: true;
        qproperty-textColor: black;
        qproperty-textBackgroundColor: white;
        qproperty-logColor: white;
        qproperty-logBackgroundColor: blue;
        qproperty-warningColor: black;
        qproperty-warningBackgroundColor: yellow;
        qproperty-errorColor: red;
        qproperty-errorBackgroundColor: white;
    }
    */

    Q_PROPERTY(bool ignoreOutputWindowColors READ ignoreOutputWindowColors WRITE setIgnoreOutputWindowColors)

    Q_PROPERTY(QColor textColor READ textColor WRITE setTextColor)
    Q_PROPERTY(QColor textBackgroundColor READ textBackgroundColor WRITE setTextBackgroundColor)

    Q_PROPERTY(QColor logColor READ logColor WRITE setLogColor)
    Q_PROPERTY(QColor logBackgroundColor READ logBackgroundColor WRITE setLogBackgroundColor)

    Q_PROPERTY(QColor warningColor READ warningColor WRITE setWarningColor)
    Q_PROPERTY(QColor warningBackgroundColor READ warningBackgroundColor WRITE setWarningBackgroundColor)
    
    Q_PROPERTY(QColor errorColor READ errorColor WRITE setErrorColor)
    Q_PROPERTY(QColor errorBackgroundColor READ errorBackgroundColor WRITE setErrorBackgroundColor)

public:
    explicit DlgExpressionInput(const App::ObjectIdentifier & _path, std::shared_ptr<const App::Expression> _expression, const Base::Unit &_impliedUnit, QWidget *parent = 0);
    ~DlgExpressionInput();

    std::shared_ptr<App::Expression> getExpression() const { return expression; }

    bool discardedFormula() const { return discarded; }

    void setExpressionInputSize(int width, int height);

    bool eventFilter(QObject *obj, QEvent *event);

    void setIgnoreOutputWindowColors(bool flag);
    void setTextColor(QColor color);
    void setTextBackgroundColor(QColor color);
    void setLogColor(QColor color);
    void setLogBackgroundColor(QColor color);
    void setWarningColor(QColor color);
    void setWarningBackgroundColor(QColor color);
    void setErrorColor(QColor color);
    void setErrorBackgroundColor(QColor color);

    bool ignoreOutputWindowColors() const;
    QColor textColor() const;
    QColor textBackgroundColor() const;
    QColor logColor() const;
    QColor logBackgroundColor() const;
    QColor warningColor() const;
    QColor warningBackgroundColor() const;
    QColor errorColor() const;
    QColor errorBackgroundColor() const;

public Q_SLOTS:
    void show();

protected:
    void showEvent(QShowEvent*);
    void hideEvent(QHideEvent*);
    void closeEvent(QCloseEvent*);
    void mouseReleaseEvent(QMouseEvent*);
    void mousePressEvent(QMouseEvent*);
    void mouseMoveEvent(QMouseEvent*);
    void resizeEvent(QResizeEvent *);

    void adjustPosition();
    void adjustExpressionSize();
    void onClose();
    int hitTest(const QPoint &point);

private Q_SLOTS:
    void textChanged();
    void setDiscarded();
    void wantReturnChecked(bool);
    void evalFuncChecked(bool);
    void onTimer();

private:
    QTimer timer;
    ::Ui::DlgExpressionInput *ui;
    std::shared_ptr<App::Expression> expression;
    App::ObjectIdentifier path;
    bool discarded;
    const Base::Unit impliedUnit;

    QColor borderColor;
    QColor backgroundColor;

    QString textColorStyle;
    QColor _textColor;
    QColor _textBackgroundColor;

    QString logColorStyle;
    QColor _logColor;
    QColor _logBackgroundColor;

    QString warningColorStyle;
    QColor _warningColor;
    QColor _warningBackgroundColor;

    QString errorColorStyle;
    QColor _errorColor;
    QColor _errorBackgroundColor;

    bool _ignoreOutputWindowColors = false;

    bool hasTextColor = false;
    bool hasLogColor = false;
    bool hasWarningColor = false;
    bool hasErrorColor = false;
    bool hasTextBackgroundColor = false;
    bool hasLogBackgroundColor = false;
    bool hasWarningBackgroundColor = false;
    bool hasErrorBackgroundColor = false;

    bool adjustingPosition = false;
    bool noBackground;
    bool leftAligned = true;
    bool adjustingExpressionSize = false;
    int dragging = 0;
    QPoint lastPos;
    QSize minSize;
    ProxyWidget *proxy = nullptr;
    friend class ProxyWidget;

    App::ExpressionFunctionCallDisabler exprFuncDisabler;

    void setupColors();
    QString colorPriority(QRgb userColor, QRgb fcDefaultColor, 
                          bool hasStyleSheet, bool isDarkStyle, 
                          bool hasSsColor, bool hasSsBackgroundColor,
                          QRgb darkOverrideColor, QRgb darkOverrideBackgroundColor,
                          QRgb lightOverrideColor, QRgb lightOverrideBackgroundColor,
                          const QColor &styleSheetColor, const QColor &styleSheetBackgroundColor);
};

class ProxyWidget : public QWidget
{
public:
    ProxyWidget(DlgExpressionInput *parent)
        :QWidget(parent), master(parent)
    {
        setAttribute(Qt::WA_NoSystemBackground, true);
        setAttribute(Qt::WA_TranslucentBackground, true);
        setMouseTracking(true);
    }

    void paintEvent(QPaintEvent *);

    void mousePressEvent(QMouseEvent* ev)
    {
        master->mousePressEvent(ev);
    }

    void mouseReleaseEvent(QMouseEvent *ev)
    {
        master->mouseReleaseEvent(ev);
    }

    void mouseMoveEvent(QMouseEvent *ev)
    {
        master->mouseMoveEvent(ev);
    }

private:
    DlgExpressionInput *master;
};

}
}

#endif // GUI_DIALOG_EXPRESSIONINPUT_H
