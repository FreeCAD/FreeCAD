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
# include <boost_signals2.hpp>
# include <boost_bind_bind.hpp>
# include <QApplication>
# include <QRegExp>
# include <QEvent>
# include <QCloseEvent>
# include <QMdiSubWindow>
# include <QPrinter>
# include <QPrintDialog>
# include <QPrinterInfo>
# include <iostream>
#endif


#include "MDIView.h"
#include "MDIViewPy.h"
#include "Command.h"
#include "Document.h"
#include "Application.h"
#include "MainWindow.h"
#include "ViewProviderDocumentObject.h"

using namespace Gui;
namespace bp = boost::placeholders;

TYPESYSTEM_SOURCE_ABSTRACT(Gui::MDIView,Gui::BaseView)


MDIView::MDIView(Gui::Document* pcDocument,QWidget* parent, Qt::WindowFlags wflags)
  : QMainWindow(parent, wflags)
  , BaseView(pcDocument)
  , pythonObject(nullptr)
  , currentMode(Child)
  , wstate(Qt::WindowNoState)
  , ActiveObjects(pcDocument)
{
    setAttribute(Qt::WA_DeleteOnClose);

    if (pcDocument)
    {
      connectDelObject = pcDocument->signalDeletedObject.connect
        (boost::bind(&ActiveObjectList::objectDeleted, &ActiveObjects, bp::_1));
      assert(connectDelObject.connected());
    }
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
    if (connectDelObject.connected())
      connectDelObject.disconnect();

    if (pythonObject) {
        Base::PyGILStateLocker lock;
        Py_DECREF(pythonObject);
        pythonObject = nullptr;
    }
}

void MDIView::deleteSelf()
{
    // When using QMdiArea make sure to remove the QMdiSubWindow
    // this view is associated with.
    //
    // #0001023: Crash when quitting after using Windows > Tile
    // Use deleteLater() instead of delete operator.
    QWidget* parent = this->parentWidget();
    if (qobject_cast<QMdiSubWindow*>(parent)) {
        // https://forum.freecadweb.org/viewtopic.php?f=22&t=23070
        parent->close();
    }
    else {
        this->close();
    }

    // detach from document
    if (_pcDocument)
        onClose();
    _pcDocument = 0;
}

PyObject* MDIView::getPyObject()
{
    if (!pythonObject)
        pythonObject = new MDIViewPy(this);

    Py_INCREF(pythonObject);
    return pythonObject;
}

void MDIView::setOverrideCursor(const QCursor& c)
{
    Q_UNUSED(c);
}

void  MDIView::restoreOverrideCursor()
{
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
            cap = QString::fromLatin1("%1[*]").arg(cap);
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
    Q_UNUSED(pMsg);
    Q_UNUSED(ppReturn);
    return false;
}

bool MDIView::onHasMsg(const char* pMsg) const
{
    Q_UNUSED(pMsg);
    return false;
}

bool MDIView::canClose(void)
{
    if (getAppDocument() && getAppDocument()->testStatus(App::Document::TempDoc))
        return true;

    if (!bIsPassive && getGuiDocument() && getGuiDocument()->isLastView()) {
        this->setFocus(); // raises the view to front
        return (getGuiDocument()->canClose(true,true));
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
        // because otherwise the QMdiSubWindow will lose its parent
        // and thus the notification in QMdiSubWindow::closeEvent of
        // other mdi windows to get maximized if this window
        // is maximized will fail.
        // This odd behaviour is caused by the invocation of
        // d->mdiArea->removeSubWindow(parent) which we must let there
        // because otherwise other parts don't work as they should.
        QMainWindow::closeEvent(e);
    }
    else
        e->ignore();
}

void MDIView::windowStateChanged( MDIView* )
{
}

void MDIView::print(QPrinter* printer)
{
    Q_UNUSED(printer);
    std::cerr << "Printing not implemented for " << this->metaObject()->className() << std::endl;
}

void MDIView::print()
{
    std::cerr << "Printing not implemented for " << this->metaObject()->className() << std::endl;
}

void MDIView::printPdf()
{
    std::cerr << "Printing PDF not implemented for " << this->metaObject()->className() << std::endl;
}

void MDIView::printPreview()
{
    std::cerr << "Printing preview not implemented for " << this->metaObject()->className() << std::endl;
}

void MDIView::savePrinterSettings(QPrinter* printer)
{
    auto hGrp = App::GetApplication().GetParameterGroupByPath("User parameter:BaseApp/Preferences/Printer");
    hGrp = hGrp->GetGroup(printer->printerName().toUtf8());

    hGrp->SetInt("DefaultPageSize", printer->pageLayout().pageSize().id());
    hGrp->SetInt("DefaultPageOrientation", static_cast<int>(printer->pageLayout().orientation()));
    hGrp->SetInt("DefaultColorMode", static_cast<int>(printer->colorMode()));
}

void MDIView::restorePrinterSettings(QPrinter* printer)
{
    auto hGrp = App::GetApplication().GetParameterGroupByPath("User parameter:BaseApp/Preferences/Printer");
    hGrp = hGrp->GetGroup(printer->printerName().toUtf8());

    QPrinterInfo info = QPrinterInfo::defaultPrinter();
    int initialDefaultPageSize = info.isNull() ? QPageSize::A4 : info.defaultPageSize().id();
    int defaultPageSize = hGrp->GetInt("DefaultPageSize", initialDefaultPageSize);
    int defaultPageOrientation = hGrp->GetInt("DefaultPageOrientation", QPageLayout::Portrait);
    int defaultColorMode = hGrp->GetInt("DefaultColorMode", QPrinter::ColorMode::Color);

    printer->setPageSize(QPageSize(static_cast<QPageSize::PageSizeId>(defaultPageSize)));
    printer->setPageOrientation(static_cast<QPageLayout::Orientation>(defaultPageOrientation));
    printer->setColorMode(static_cast<QPrinter::ColorMode>(defaultColorMode));
}

QStringList MDIView::undoActions() const
{
    QStringList actions;
    Gui::Document* doc = getGuiDocument();
    if (doc) {
        std::vector<std::string> vecUndos = doc->getUndoVector();
        for (std::vector<std::string>::iterator i = vecUndos.begin(); i != vecUndos.end(); ++i) {
            actions << QCoreApplication::translate("Command", i->c_str());
        }
    }

    return actions;
}

QStringList MDIView::redoActions() const
{
    QStringList actions;
    Gui::Document* doc = getGuiDocument();
    if (doc) {
        std::vector<std::string> vecRedos = doc->getRedoVector();
        for (std::vector<std::string>::iterator i = vecRedos.begin(); i != vecRedos.end(); ++i) {
            actions << QCoreApplication::translate("Command", i->c_str());
        }
    }

    return actions;
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
                    if (qobject_cast<QMdiSubWindow*>(this->parentWidget()))
                        getMainWindow()->removeWindow(this,false);
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
                    if (qobject_cast<QMdiSubWindow*>(this->parentWidget()))
                        getMainWindow()->removeWindow(this,false);
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
