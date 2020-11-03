/****************************************************************************
 *   Copyright (c) 2020 Zheng, Lei (realthunder) <realthunder.dev@gmail.com>*
 *                                                                          *
 *   This file is part of the FreeCAD CAx development system.               *
 *                                                                          *
 *   This library is free software; you can redistribute it and/or          *
 *   modify it under the terms of the GNU Library General Public            *
 *   License as published by the Free Software Foundation; either           *
 *   version 2 of the License, or (at your option) any later version.       *
 *                                                                          *
 *   This library  is distributed in the hope that it will be useful,       *
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of         *
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the          *
 *   GNU Library General Public License for more details.                   *
 *                                                                          *
 *   You should have received a copy of the GNU Library General Public      *
 *   License along with this library; see the file COPYING.LIB. If not,     *
 *   write to the Free Software Foundation, Inc., 59 Temple Place,          *
 *   Suite 330, Boston, MA  02111-1307, USA                                 *
 *                                                                          *
 ****************************************************************************/

#ifndef GUI_PIEMENU_H
#define GUI_PIEMENU_H

#include <memory>
#include <QWidget>
#include <QIcon>
#include <QToolButton>

namespace Gui {


// ------------------------------------------------------------------

/** Implements the pie menu button
 */
class PieButton : public QToolButton
{
    Q_OBJECT
public:
    PieButton(QWidget *parent);
    ~PieButton();

protected:
    bool event(QEvent *);
};

/** Implements the pie menu
 */
class PieMenu : public QWidget
{
    Q_OBJECT

    Q_PROPERTY(int fontSize READ fontSize WRITE setFontSize DESIGNABLE true SCRIPTABLE true)
    Q_PROPERTY(int radius READ radius WRITE setRadius DESIGNABLE true SCRIPTABLE true)
    Q_PROPERTY(qreal offset READ offset WRITE setOffset DESIGNABLE true SCRIPTABLE true)
    Q_PROPERTY(int animateDuration READ animateDuration
            WRITE setAnimateDuration DESIGNABLE true SCRIPTABLE true)

public:
    PieMenu(QMenu *menu, const char *param=0, QWidget *parent=0);
    ~PieMenu();

    static QAction *exec(QMenu *menu, const QPoint &pt,
            const char *param=0, bool forwardKeyPress=false, bool resetOffset=false);

    static bool isEnabled(const char *name);
    static void setEnabled(const char *name, bool enabled=true);

    int radius() const;
    void setRadius(int radius) const;

    int animateDuration() const;
    void setAnimateDuration(int duration);

    int fontSize() const;
    void setFontSize(int size);

    qreal offset() const;
    void setOffset(qreal);

protected:
    void mousePressEvent(QMouseEvent *);
    void mouseReleaseEvent(QMouseEvent *);
    void wheelEvent(QWheelEvent *);
    void mouseMoveEvent(QMouseEvent *);
    void hideEvent(QHideEvent *);
    void leaveEvent(QEvent *);
    void keyPressEvent(QKeyEvent *);
    void paintEvent(QPaintEvent *);
    bool eventFilter(QObject *, QEvent *);

#if QT_VERSION  < 0x050000
Q_SIGNALS:
    void initMenu();
#endif

protected Q_SLOTS:
    void onTriggered(QAction *);
    void onStateChanged();
    void onTimer();

private:
    friend class PieButton;

    class Private;
    std::unique_ptr<Private> pimpl;
};

} // namespace Gui

#endif // GUI_PIEMENU_H
