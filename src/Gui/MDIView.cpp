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

#include <boost/signals2.hpp>
#include <boost/core/ignore_unused.hpp>
#include <QAction>
#include <QApplication>
#include <QEvent>
#include <QCloseEvent>
#include <QMdiSubWindow>
#include <QPrintDialog>
#include <QPrintPreviewDialog>
#include <QPrinter>
#include <QPrinterInfo>
#include <QRegularExpression>
#include <QRegularExpressionMatch>

#include <Base/Interpreter.h>
#include <App/Document.h>
#include <App/Application.h>
#include <Gui/PreferencePages/DlgSettingsPDF.h>

#include "MDIView.h"
#include "MDIViewPy.h"
#include "Application.h"
#include "Document.h"
#include "FileDialog.h"
#include "MainWindow.h"
#include "ViewProviderDocumentObject.h"


using namespace Gui;
namespace sp = std::placeholders;

TYPESYSTEM_SOURCE_ABSTRACT(Gui::MDIView, Gui::BaseView)


MDIView::MDIView(Gui::Document* pcDocument, QWidget* parent, Qt::WindowFlags wflags)
    : QMainWindow(parent, wflags)
    , BaseView(pcDocument)
    , pythonObject(nullptr)
    , currentMode(Child)
    , wstate(Qt::WindowNoState)
    , ActiveObjects(pcDocument)
{
    setAttribute(Qt::WA_DeleteOnClose);

    if (pcDocument) {
        // NOLINTBEGIN
        connectDelObject = pcDocument->signalDeletedObject.connect(
            std::bind(&ActiveObjectList::objectDeleted, &ActiveObjects, sp::_1)
        );
        assert(connectDelObject.connected());
        // NOLINTEND
    }
}

MDIView::~MDIView()
{
    // This view might be the focus widget of the main window. In this case we must
    // clear the focus and e.g. set the focus directly to the main window, otherwise
    // the application crashes when accessing this deleted view.
    // This effect only occurs if this widget is not in Child mode, because otherwise
    // the focus stuff is done correctly.
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
    if (connectDelObject.connected()) {
        connectDelObject.disconnect();
    }

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
        // https://forum.freecad.org/viewtopic.php?f=22&t=23070
        parent->close();
    }
    else {
        this->close();
    }

    // detach from document
    if (_pcDocument) {
        onClose();
    }
    _pcDocument = nullptr;
}

MDIView* MDIView::clone()
{
    return nullptr;
}

void MDIView::cloneFrom(const MDIView& from)
{
    setWindowTitle(from.windowTitle());
    setWindowIcon(from.windowIcon());
    resize(from.size());

    // wstate is updated when changing from top-level mode to something else. This hasn't happened
    // yet if the original widget is currently in top-level mode. In this case we want to use the
    // actual windowState of the original widget instead of it's wstate.

    if (from.currentViewMode() == TopLevel) {
        wstate = from.windowState();
    }
    else {
        wstate = from.wstate;
    }
}

PyObject* MDIView::getPyObject()
{
    if (!pythonObject) {
        pythonObject = new MDIViewPy(this);
    }

    Py_INCREF(pythonObject);
    return pythonObject;
}

void MDIView::setOverrideCursor(const QCursor& c)
{
    Q_UNUSED(c);
}

void MDIView::restoreOverrideCursor()
{}

void MDIView::onRelabel(Gui::Document* pDoc)
{
    if (!bIsPassive) {
        // Try to separate document name and view number if there is one
        QString cap = windowTitle();
        // Either with dirty flag ...
        QRegularExpression rx(QLatin1String(R"((\s\:\s\d+\[\*\])$)"));
        QRegularExpressionMatch match;
        // int pos =
        boost::ignore_unused(cap.lastIndexOf(rx, -1, &match));
        if (!match.hasMatch()) {
            // ... or not
            rx.setPattern(QLatin1String(R"((\s\:\s\d+)$)"));
            // pos =
            boost::ignore_unused(cap.lastIndexOf(rx, -1, &match));
        }
        if (match.hasMatch()) {
            cap = QString::fromUtf8(pDoc->getDocument()->Label.getValue());
            cap += match.captured();
            setWindowTitle(cap);
        }
        else {
            cap = QString::fromUtf8(pDoc->getDocument()->Label.getValue());
            cap = QStringLiteral("%1[*]").arg(cap);
            setWindowTitle(cap);
        }
    }
}

void MDIView::viewAll()
{}

/// receive a message
bool MDIView::onMsg(const char* pMsg, const char** ppReturn)
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

bool MDIView::canClose()
{
    if (getAppDocument() && getAppDocument()->testStatus(App::Document::TempDoc)) {
        return true;
    }

    if (!bIsPassive && getGuiDocument() && getGuiDocument()->isLastView()) {
        this->setFocus();  // raises the view to front
        return (getGuiDocument()->canClose(true, true));
    }

    return true;
}

void MDIView::closeEvent(QCloseEvent* e)
{
    if (canClose()) {
        e->accept();
        Application::Instance->viewClosed(this);

        if (!bIsPassive) {
            // must be detached so that the last view can get asked
            Document* doc = this->getGuiDocument();
            if (doc && !doc->isLastView()) {
                doc->detachView(this);
            }
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
    else {
        e->ignore();
    }
}

void MDIView::windowStateChanged(QWidget* view)
{
    Q_UNUSED(view)
}

void MDIView::print(QPrinter* printer)
{
    Q_UNUSED(printer);
    std::cerr << "Printing not implemented for " << this->metaObject()->className() << std::endl;
}

void MDIView::print()
{
    QPrinter printer(QPrinter::ScreenResolution);
    printer.setFullPage(true);
    QPrintDialog dlg(&printer, this);
    if (dlg.exec() == QDialog::Accepted) {
        print(&printer);
    }
}

void MDIView::printPdf()
{
    QString filename = FileDialog::getSaveFileName(
        this,
        tr("Export PDF"),
        QString(),
        QStringLiteral("%1 (*.pdf)").arg(tr("PDF file"))
    );
    if (!filename.isEmpty()) {
        QPrinter printer(QPrinter::ScreenResolution);
        // setPdfVersion sets the printed PDF Version to what is chosen in
        // Preferences/Import-Export/PDF more details under:
        // https://www.kdab.com/creating-pdfa-documents-qt/
        printer.setPdfVersion(Gui::Dialog::DlgSettingsPDF::evaluatePDFVersion());
        printer.setOutputFormat(QPrinter::PdfFormat);
        printer.setOutputFileName(filename);
        printer.setCreator(QString::fromStdString(App::Application::getNameWithVersion()));
        print(&printer);
    }
}

void MDIView::printPreview()
{
    QPrinter printer(QPrinter::ScreenResolution);
    QPrintPreviewDialog dlg(&printer, this);
    connect(&dlg, &QPrintPreviewDialog::paintRequested, this, qOverload<QPrinter*>(&MDIView::print));
    dlg.exec();
}

void MDIView::savePrinterSettings(QPrinter* printer)
{
    auto hGrp = App::GetApplication().GetParameterGroupByPath(
        "User parameter:BaseApp/Preferences/Printer"
    );
    QString printerName = printer->printerName();
    if (printerName.isEmpty()) {
        // no printer defined
        return;
    }

    hGrp = hGrp->GetGroup(printerName.toUtf8());

    hGrp->SetInt("DefaultPageSize", printer->pageLayout().pageSize().id());
    hGrp->SetInt("DefaultPageOrientation", static_cast<int>(printer->pageLayout().orientation()));
    hGrp->SetInt("DefaultColorMode", static_cast<int>(printer->colorMode()));
}

void MDIView::restorePrinterSettings(QPrinter* printer)
{
    auto hGrp = App::GetApplication().GetParameterGroupByPath(
        "User parameter:BaseApp/Preferences/Printer"
    );
    QString printerName = printer->printerName();
    if (printerName.isEmpty()) {
        // no printer defined
        return;
    }

    hGrp = hGrp->GetGroup(printerName.toUtf8());

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
        for (const auto& vecUndo : vecUndos) {
            actions << QCoreApplication::translate("Command", vecUndo.c_str());
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
        for (const auto& vecRedo : vecRedos) {
            actions << QCoreApplication::translate("Command", vecRedo.c_str());
        }
    }

    return actions;
}

QSize MDIView::minimumSizeHint() const
{
    return {400, 300};
}


void MDIView::changeEvent(QEvent* e)
{
    switch (e->type()) {
        case QEvent::ActivationChange: {
            // Forces this top-level window to be the active view of the main window
            if (isActiveWindow()) {
                getMainWindow()->setActiveWindow(this);
            }
        } break;
        case QEvent::WindowTitleChange:
        case QEvent::ModifiedChange: {
            // sets the appropriate tab of the tabbar
            getMainWindow()->tabChanged(this);
        } break;
        default: {
            QMainWindow::changeEvent(e);
        } break;
    }
}

bool MDIView::eventFilter(QObject* watched, QEvent* event)
{
    // As long as this widget is a top-level window (either in 'TopLevel' or 'FullScreen' mode) we
    // need to be notified when an action is added to a widget. This action must also be added to
    // this window to allow one to make use of its shortcut (if defined).
    // Note: We don't need to care about removing an action if its parent widget gets destroyed.
    // This does the action itself for us.

    if (watched != this && event->type() == QEvent::ActionAdded) {
        auto actionEvent = static_cast<QActionEvent*>(event);
        QAction* action = actionEvent->action();

        if (!action->isSeparator()) {
            QList<QAction*> acts = actions();
            if (!acts.contains(action)) {
                addAction(action);
            }
        }
    }

    return false;
}

#if defined(Q_WS_X11)
// To fix bug #0000345 move function declaration to here
extern void qt_x11_wait_for_window_manager(QWidget* w);  // defined in qwidget_x11.cpp
#endif

void MDIView::setCurrentViewMode(ViewMode mode)
{
    const ViewMode oldmode = MDIView::currentViewMode();
    if (oldmode == mode) {
        return;
    }

    if (oldmode == Child) {
        // remove window from MDIArea
        if (qobject_cast<QMdiSubWindow*>(parentWidget())) {
            getMainWindow()->removeWindow(this, false);
            setParent(nullptr);
        }
    }
    else if (oldmode == TopLevel) {
        // backup maximize state for top-level mode
        wstate = windowState();
    }

    switch (mode) {
        // go to normal mode
        case Child:
            getMainWindow()->addWindow(this);
            break;

        // go to top-level mode
        case TopLevel:
            if (wstate & Qt::WindowMaximized) {
                // Only calling showMaximized doesn't work when the widget is currently in
                // full-screen mode. We need to exit full-screen mode first or the widget will end
                // up in normal mode. Same if the window is in child mode but maximized.
                setWindowState(windowState() & ~(Qt::WindowMaximized | Qt::WindowFullScreen));
                showMaximized();
            }
            else {
                showNormal();
            }
            break;

        // go to full-screen mode
        case FullScreen:
            showFullScreen();
            break;
    }

    currentMode = mode;

#if defined(Q_WS_X11)
    if (mode == TopLevel && oldmode == Child) {
        // extern void qt_x11_wait_for_window_manager( QWidget* w ); // defined in
        // qwidget_x11.cpp
        qt_x11_wait_for_window_manager(this);
    }
#endif

    activateWindow();

    if (oldmode == Child) {
        // To make a global shortcut working from this window we need to add
        // all existing actions from the mainwindow and its sub-widgets

        QList<QAction*> acts = getMainWindow()->findChildren<QAction*>();
        addActions(acts);

        // To be notfified for new actions
        qApp->installEventFilter(this);
    }
    else if (mode == Child) {
        qApp->removeEventFilter(this);
        QList<QAction*> acts = actions();
        for (QAction* it : acts) {
            removeAction(it);
        }

        // When switching from undocked to docked mode, the widget position is somehow not updated
        // correctly. In this case mapToGlobal(Point()) returns {0, 0} even though the widget is
        // clearly not at the top-left corner of the screen. We fix this by briefly changing the
        // maximum size of the widget.

        const auto oldsize = maximumSize();
        setMaximumSize({1, 1});
        setMaximumSize(oldsize);
    }
}

QString MDIView::buildWindowTitle() const
{
    QString windowTitle;
    if (auto document = getAppDocument()) {
        windowTitle.append(QString::fromStdString(document->Label.getStrValue()));
    }

    return windowTitle;
}

void MDIView::setWindowTitle(const QString& title)
{
    QString newerTitle {title};
    newerTitle.replace(QLatin1Char('&'), QStringLiteral("&&"));
    QMainWindow::setWindowTitle(newerTitle);
}

#include "moc_MDIView.cpp"
