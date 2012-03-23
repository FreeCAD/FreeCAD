/***************************************************************************
 *   Copyright (c) 2007 Werner Mayer <wmayer[at]users.sourceforge.net>     *
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
 *   write to the Free Software Foundation, Inc., 59 Temple Place,         *
 *   Suite 330, Boston, MA  02111-1307, USA                                *
 *                                                                         *
 ***************************************************************************/


#include "PreCompiled.h"

#ifndef _PreComp_
# include <qapplication.h>
# include <qregexp.h>
# include <QEvent>
# include <QCloseEvent>
# include <QMdiSubWindow>
#endif


#include "MDIView.h"
#include "Command.h"
#include "Document.h"
#include "Application.h"
#include "MainWindow.h"

using namespace Gui;

TYPESYSTEM_SOURCE_ABSTRACT(Gui::MDIView,Gui::BaseView);


MDIView::MDIView(Gui::Document* pcDocument,QWidget* parent, Qt::WFlags wflags)
  : QMainWindow(parent, wflags), BaseView(pcDocument),currentMode(Child), wstate(Qt::WindowNoState)
{
    setAttribute(Qt::WA_DeleteOnClose);
}

MDIView::~MDIView()
{
    //This view might be the focus widget of the main window. In this case we must
    //clear the focus and e.g. set the focus directly to the main window, otherwise
    //the application crashes when accessing this deleted view.
    //This effect only occurs if this widget is not in Child mode, because otherwise
    //the focus stuff is done correctly.
    if (getMainWindow()) {
        QWidget* foc = getMainWindow()->focusWidget();
        if (foc) {
            QWidget* par = foc;
            while (par) {
                if (par == this) {
                    getMainWindow()->setFocus();
                    break;
                }
                par = par->parentWidget();
            }
        }
    }
}

void MDIView::deleteSelf()
{
    // When using QMdiArea make sure to remove the QMdiSubWindow
    // this view is associated with.
#if !defined (NO_USE_QT_MDI_AREA)
    QWidget* parent = this->parentWidget();
    if (qobject_cast<QMdiSubWindow*>(parent))
        delete parent;
    else
#endif
        delete this;
}

void MDIView::onRelabel(Gui::Document *pDoc)
{
    if (!bIsPassive) {
        // Try to separate document name and view number if there is one
        QString cap = windowTitle();
        // Either with dirty flag ...
        QRegExp rx(QLatin1String("(\\s\\:\\s\\d+\\[\\*\\])$"));
        int pos = rx.lastIndexIn(cap);
        if (pos == -1) {
            // ... or not
            rx.setPattern(QLatin1String("(\\s\\:\\s\\d+)$"));
            pos = rx.lastIndexIn(cap);
        }
        if (pos != -1) {
            cap = QString::fromUtf8(pDoc->getDocument()->Label.getValue());
            cap += rx.cap();
            setWindowTitle(cap);
        }
        else {
            cap = QString::fromUtf8(pDoc->getDocument()->Label.getValue());
            cap = QString::fromAscii("%1[*]").arg(cap);
            setWindowTitle(cap);
        }
    }
}

void MDIView::viewAll()
{
}

/// receive a message
bool MDIView::onMsg(const char* pMsg,const char** ppReturn)
{
    return false;
}

bool MDIView::onHasMsg(const char* pMsg) const
{
    return false;
}

bool MDIView::canClose(void)
{
    if (!bIsPassive && getGuiDocument() && getGuiDocument()->isLastView()) {
        this->setFocus(); // raises the view to front
        return (getGuiDocument()->canClose());
    }

    return true;
}

void MDIView::closeEvent(QCloseEvent *e)
{
    if (canClose()) {
        e->accept();
        if (!bIsPassive) {
            // must be detached so that the last view can get asked
            Document* doc = this->getGuiDocument();
            if (doc && !doc->isLastView())
                doc->detachView(this);
        }

        // Note: When using QMdiArea we must not use removeWindow()
        // because otherwise the QMdiSubWindow will loose its parent
        // and thus the notification in QMdiSubWindow::closeEvent of
        // other mdi windows to get maximized if this window
        // is maximized will fail.
        // This odd behaviour is caused by the invocation of 
        // d->mdiArea->removeSubWindow(parent) which we must let there
        // because otherwise other parts don't work as they should.
#if defined (NO_USE_QT_MDI_AREA)
        // avoid flickering
        getMainWindow()->removeWindow(this);
#endif
        QMainWindow::closeEvent(e);
    }
    else
        e->ignore();
}

void MDIView::windowStateChanged( MDIView* )
{
}

void MDIView::print()
{
    // print command specified but print method not overriden!
    assert(0);
}

void MDIView::printPdf()
{
    // print command specified but print method not overriden!
    assert(0);
}

void MDIView::printPreview()
{
    // print command specified but print method not overriden!
    assert(0);
}

QSize MDIView::minimumSizeHint () const
{
    return QSize(400, 300);
}

void MDIView::changeEvent(QEvent *e)
{
    switch (e->type()) {
        case QEvent::ActivationChange:
            {
                // Forces this top-level window to be the active view of the main window
                if (isActiveWindow()) {
                    if (getMainWindow()->activeWindow() != this)
                        getMainWindow()->setActiveWindow(this);
                }
            }   break;
        case QEvent::WindowTitleChange:
        case QEvent::ModifiedChange:
            {
                // sets the appropriate tab of the tabbar
                getMainWindow()->tabChanged(this);
            }   break;
        default:
            {
                QMainWindow::changeEvent(e);
            }   break;
    }
}

#if defined(Q_WS_X11)
// To fix bug #0000345 move function declaration to here
extern void qt_x11_wait_for_window_manager( QWidget* w ); // defined in qwidget_x11.cpp
#endif

void MDIView::setCurrentViewMode(ViewMode mode)
{
    switch (mode) {
        // go to normal mode
        case Child:
            {
                if (this->currentMode == FullScreen) {
                    showNormal();
                    setWindowFlags(windowFlags() & ~Qt::Window);
                }
                else if (this->currentMode == TopLevel) {
                    this->wstate = windowState();
                    setWindowFlags( windowFlags() & ~Qt::Window );
                }

                if (this->currentMode != Child) {
                    this->currentMode = Child;
                    getMainWindow()->addWindow(this);
                    getMainWindow()->activateWindow();
                    update();
                }
            }   break;
        // go to top-level mode
        case TopLevel:
            {
                if (this->currentMode == Child) {
#if !defined (NO_USE_QT_MDI_AREA)
                    if (qobject_cast<QMdiSubWindow*>(this->parentWidget()))
#endif
                        getMainWindow()->removeWindow(this);
                    setWindowFlags(windowFlags() | Qt::Window);
                    setParent(0, Qt::Window | Qt::WindowTitleHint | Qt::WindowSystemMenuHint | 
                                 Qt::WindowMinMaxButtonsHint);
                    if (this->wstate & Qt::WindowMaximized)
                        showMaximized();
                    else
                        showNormal();

#if defined(Q_WS_X11)
                    //extern void qt_x11_wait_for_window_manager( QWidget* w ); // defined in qwidget_x11.cpp
                    qt_x11_wait_for_window_manager(this);
#endif
                    activateWindow();
                }
                else if (this->currentMode == FullScreen) {
                    if (this->wstate & Qt::WindowMaximized)
                        showMaximized();
                    else
                        showNormal();
                }
            
                this->currentMode = TopLevel;
                update();
            }   break;
        // go to fullscreen mode
        case FullScreen:
            {
                if (this->currentMode == Child) {
#if !defined (NO_USE_QT_MDI_AREA)
                    if (qobject_cast<QMdiSubWindow*>(this->parentWidget()))
#endif
                        getMainWindow()->removeWindow(this);
                    setWindowFlags(windowFlags() | Qt::Window);
                    setParent(0, Qt::Window);
                    showFullScreen();
                }
                else if (this->currentMode == TopLevel) {
                    this->wstate = windowState();
                    showFullScreen();
                }
                
                this->currentMode = FullScreen;
                update();
            }   break;
    }
}

#include "moc_MDIView.cpp"
