/***************************************************************************
 *   Copyright (c) 2004 Werner Mayer <wmayer[at]users.sourceforge.net>     *
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
# include <QApplication>
# include <QGenericReturnArgument>
# include <QKeyEvent>
# include <QMessageBox>
# include <QMetaObject>
# include <QStatusBar>
# include <QThread>
# include <QTime>
# include <QTimer>
#endif

#include "ProgressBar.h"
#include "MainWindow.h"
#include "WaitCursor.h"

using namespace Gui;


namespace Gui {
struct SequencerPrivate
{
    ProgressBar* bar;
    WaitCursor* waitCursor;
    QTime measureTime;
    QTime progressTime;
    QString text;
    bool guiThread;
};

struct ProgressBarPrivate
{
    QTimer* delayShowTimer;
    int minimumDuration;
    int observeEventFilter;

    bool isModalDialog(QObject* o) const
    {
        QWidget* parent = qobject_cast<QWidget*>(o);
        while (parent) {
            QMessageBox* dlg = qobject_cast<QMessageBox*>(parent);
            if (dlg && dlg->isModal())
                return true;
            parent = parent->parentWidget();
        }

        return false;
    }
};
}

Sequencer* Sequencer::_pclSingleton = 0;

Sequencer* Sequencer::instance()
{
    // not initialized?
    if (!_pclSingleton)
    {
        _pclSingleton = new Sequencer();
    }

    return _pclSingleton;
}

Sequencer::Sequencer ()
{
    d = new SequencerPrivate;
    d->bar = 0;
    d->waitCursor = 0;
    d->guiThread = true;
}

Sequencer::~Sequencer ()
{
    delete d;
}

void Sequencer::pause()
{
    QThread *currentThread = QThread::currentThread();
    QThread *thr = d->bar->thread(); // this is the main thread
    if (thr != currentThread)
        return;

    // allow key handling of dialog and restore cursor
    d->bar->leaveControlEvents();
    d->waitCursor->restoreCursor();
    QApplication::setOverrideCursor(Qt::ArrowCursor);
}

void Sequencer::resume()
{
    QThread *currentThread = QThread::currentThread();
    QThread *thr = d->bar->thread(); // this is the main thread
    if (thr != currentThread)
        return;

    QApplication::restoreOverrideCursor();
    d->waitCursor->setWaitCursor();
    // must be called as last to get control before WaitCursor
    d->bar->enterControlEvents(); // grab again
}

void Sequencer::startStep()
{
    QThread *currentThread = QThread::currentThread();
    QThread *thr = d->bar->thread(); // this is the main thread
    if (thr != currentThread) {
        d->guiThread = false;
        d->bar->setRange(0, (int)nTotalSteps);
        d->progressTime.start();
        d->measureTime.start();
        QMetaObject::invokeMethod(d->bar, "aboutToShow", Qt::QueuedConnection);
    }
    else {
        d->guiThread = true;
        d->bar->setRange(0, (int)nTotalSteps);
        d->progressTime.start();
        d->measureTime.start();
        d->waitCursor = new Gui::WaitCursor;
        d->bar->enterControlEvents();
        d->bar->aboutToShow();
    }
}

void Sequencer::nextStep(bool canAbort)
{
    QThread *currentThread = QThread::currentThread();
    QThread *thr = d->bar->thread(); // this is the main thread
    if (thr != currentThread) {
        setValue((int)nProgress+1);
    }
    else {
        if (wasCanceled() && canAbort) {
            // restore cursor
            pause();
            bool ok = d->bar->canAbort();
            // continue and show up wait cursor if needed
            resume();

            // force to abort the operation
            if ( ok ) {
                abort();
            } else {
                rejectCancel();
                setValue((int)nProgress+1);
            }
        }
        else {
            setValue((int)nProgress+1);
        }
    }
}

void Sequencer::setProgress(size_t step)
{
    d->bar->show();
    setValue((int)step);
}

void Sequencer::setValue(int step)
{
    QThread *currentThread = QThread::currentThread();
    QThread *thr = d->bar->thread(); // this is the main thread
    // if number of total steps is unknown then increment only by one
    if (nTotalSteps == 0) {
        int elapsed = d->progressTime.elapsed();
        // allow an update every 100 milliseconds only
        if (elapsed > 100) {
            d->progressTime.restart();
            if (thr != currentThread) {
                QMetaObject::invokeMethod(d->bar, "setValue", Qt::/*Blocking*/QueuedConnection,
                    QGenericReturnArgument(), Q_ARG(int,d->bar->value()+1));
            }
            else {
                d->bar->setValue(d->bar->value()+1);
                qApp->processEvents();
            }
        }
    }
    else {
        int elapsed = d->progressTime.elapsed();
        // allow an update every 100 milliseconds only
        if (elapsed > 100) {
            d->progressTime.restart();
            if (thr != currentThread) {
                QMetaObject::invokeMethod(d->bar, "setValue", Qt::/*Blocking*/QueuedConnection,
                QGenericReturnArgument(), Q_ARG(int,step));
                if (d->bar->isVisible())
                    showRemainingTime();
            }
            else {
                d->bar->setValue(step);
                if (d->bar->isVisible())
                    showRemainingTime();
                d->bar->resetObserveEventFilter();
                qApp->processEvents();
            }
        }
    }
}

void Sequencer::showRemainingTime()
{
    QThread *currentThread = QThread::currentThread();
    QThread *thr = d->bar->thread(); // this is the main thread

    int elapsed = d->measureTime.elapsed();
    int progress = d->bar->value();
    int totalSteps = d->bar->maximum() - d->bar->minimum();

    QString txt = d->text;
    // More than 5 percent complete or more than 5 secs have elapsed.
    if (progress * 20 > totalSteps || elapsed > 5000) {
        int rest = (int) ( (double) totalSteps/progress * elapsed ) - elapsed;

        // more than 1 secs have elapsed and at least 100 ms are remaining
        if (elapsed > 1000 && rest > 100) {
            QTime time( 0,0, 0);
            time = time.addSecs( rest/1000 );
            QString remain = Gui::ProgressBar::tr("Remaining: %1").arg(time.toString());
            QString status = QString::fromLatin1("%1\t[%2]").arg(txt, remain);

            if (thr != currentThread) {
                QMetaObject::invokeMethod(getMainWindow()->statusBar(), "showMessage",
                    Qt::/*Blocking*/QueuedConnection,
                    QGenericReturnArgument(),
                    Q_ARG(QString,status));
            }
            else {
                getMainWindow()->showMessage(status);
            }
        }
    }
}

void Sequencer::resetData()
{
    QThread *currentThread = QThread::currentThread();
    QThread *thr = d->bar->thread(); // this is the main thread
    if (thr != currentThread) {
        QMetaObject::invokeMethod(d->bar, "reset", Qt::QueuedConnection);
        QMetaObject::invokeMethod(d->bar, "aboutToHide", Qt::QueuedConnection);
        QMetaObject::invokeMethod(getMainWindow()->statusBar(), "showMessage",
            Qt::/*Blocking*/QueuedConnection,
            QGenericReturnArgument(),
            Q_ARG(QString,QString()));
        QMetaObject::invokeMethod(getMainWindow(), "setPaneText",
            Qt::/*Blocking*/QueuedConnection,
            QGenericReturnArgument(),
            Q_ARG(int,1),
            Q_ARG(QString,QString()));
    }
    else {
        d->bar->reset();
        // Note: Under Qt 4.1.4 this forces to run QWindowsStyle::eventFilter() twice
        // handling the same event thus a warning is printed. Possibly, this is a bug
        // in Qt. The message is QEventDispatcherUNIX::unregisterTimer: invalid argument.
        d->bar->aboutToHide();
        delete d->waitCursor;
        d->waitCursor = 0;
        d->bar->leaveControlEvents();
        getMainWindow()->setPaneText(1, QString());
        getMainWindow()->showMessage(QString());
    }

    SequencerBase::resetData();
}

void Sequencer::abort()
{
    //resets
    resetData();
    Base::AbortException exc("Aborting...");
    throw exc;
}

void Sequencer::setText (const char* pszTxt)
{
    QThread *currentThread = QThread::currentThread();
    QThread *thr = d->bar->thread(); // this is the main thread

    // print message to the statusbar
    d->text = pszTxt ? QString::fromUtf8(pszTxt) : QLatin1String("");
    if (thr != currentThread) {
        QMetaObject::invokeMethod(getMainWindow()->statusBar(), "showMessage",
            Qt::/*Blocking*/QueuedConnection,
            QGenericReturnArgument(),
            Q_ARG(QString,d->text));
    }
    else {
        getMainWindow()->showMessage(d->text);
    }
}

bool Sequencer::isBlocking() const
{
    return d->guiThread;
}

QProgressBar* Sequencer::getProgressBar(QWidget* parent)
{
    if (!d->bar)
        d->bar = new ProgressBar(this, parent);
    return d->bar;
}

// -------------------------------------------------------

/* TRANSLATOR Gui::ProgressBar */

ProgressBar::ProgressBar (Sequencer* s, QWidget * parent)
    : QProgressBar(parent), sequencer(s)
{
#ifdef QT_WINEXTRAS_LIB
  m_taskbarButton = nullptr;
  m_taskbarButton = nullptr;
#endif
    d = new Gui::ProgressBarPrivate;
    d->minimumDuration = 2000; // 2 seconds
    d->delayShowTimer = new QTimer(this);
    d->delayShowTimer->setSingleShot(true);
    connect(d->delayShowTimer, SIGNAL(timeout()), this, SLOT(delayedShow()));
    d->observeEventFilter = 0;

    setFixedWidth(120);

    // write percentage to the center
    setAlignment(Qt::AlignHCenter);
    hide();
}

ProgressBar::~ProgressBar ()
{
    disconnect(d->delayShowTimer, SIGNAL(timeout()), this, SLOT(delayedShow()));
    delete d->delayShowTimer;
    delete d;
}

int ProgressBar::minimumDuration() const
{
    return d->minimumDuration;
}

void Gui::ProgressBar::reset()
{
  QProgressBar::reset();
#ifdef QT_WINEXTRAS_LIB
  setupTaskBarProgress();
  m_taskbarProgress->reset();
#endif
}

void Gui::ProgressBar::setRange(int minimum, int maximum)
{
  QProgressBar::setRange(minimum, maximum);
#ifdef QT_WINEXTRAS_LIB
  setupTaskBarProgress();
  m_taskbarProgress->setRange(minimum, maximum);
#endif
}

void Gui::ProgressBar::setMinimum(int minimum)
{
  QProgressBar::setMinimum(minimum);
#ifdef QT_WINEXTRAS_LIB
  setupTaskBarProgress();
  m_taskbarProgress->setMinimum(minimum);
#endif
}

void Gui::ProgressBar::setMaximum(int maximum)
{
  QProgressBar::setMaximum(maximum);
#ifdef QT_WINEXTRAS_LIB
  setupTaskBarProgress();
  m_taskbarProgress->setMaximum(maximum);
#endif
}

void Gui::ProgressBar::setValue(int value)
{
  QProgressBar::setValue(value);
#ifdef QT_WINEXTRAS_LIB
  setupTaskBarProgress();
  m_taskbarProgress->setValue(value);
#endif
}

void ProgressBar::setMinimumDuration (int ms)
{
    if (value() == 0)
    {
        d->delayShowTimer->stop();
        d->delayShowTimer->start(ms);
    }

    d->minimumDuration = ms;
}

void ProgressBar::aboutToShow()
{
    // delay showing the bar
    d->delayShowTimer->start(d->minimumDuration);
#ifdef QT_WINEXTRAS_LIB
    setupTaskBarProgress();
    m_taskbarProgress->show();
#endif
}

void ProgressBar::delayedShow()
{
    if (!isVisible() && !sequencer->wasCanceled() && sequencer->isRunning()) {
        show();
    }
}

void ProgressBar::aboutToHide()
{
    hide();
#ifdef QT_WINEXTRAS_LIB
    setupTaskBarProgress();
    m_taskbarProgress->hide();
#endif
}

bool ProgressBar::canAbort() const
{
    int ret = QMessageBox::question(getMainWindow(),tr("Aborting"),
    tr("Do you really want to abort the operation?"),  QMessageBox::Yes,
    QMessageBox::No|QMessageBox::Default);

    return (ret == QMessageBox::Yes) ? true : false;
}

void ProgressBar::showEvent(QShowEvent* e)
{
    QProgressBar::showEvent(e);
    d->delayShowTimer->stop();
}

void ProgressBar::hideEvent(QHideEvent* e)
{
    QProgressBar::hideEvent(e);
    d->delayShowTimer->stop();
}

void ProgressBar::resetObserveEventFilter()
{
    d->observeEventFilter = 0;
}

void ProgressBar::enterControlEvents()
{
    qApp->installEventFilter(this);

    // Make sure that we get the key events, otherwise the Inventor viewer usurps the key events
    // This also disables accelerators.
    grabKeyboard();
}

void ProgressBar::leaveControlEvents()
{
    qApp->removeEventFilter(this);

    // release the keyboard again
    releaseKeyboard();
}

#ifdef QT_WINEXTRAS_LIB
void ProgressBar::setupTaskBarProgress()
{
  if (!m_taskbarButton || !m_taskbarProgress)
  {
    m_taskbarButton = new QWinTaskbarButton(this);
    m_taskbarButton->setWindow(MainWindow::getInstance()->windowHandle());
    //m_myButton->setOverlayIcon(QIcon(""));

    m_taskbarProgress = m_taskbarButton->progress();
  }
}
#endif

bool ProgressBar::eventFilter(QObject* o, QEvent* e)
{
    if (sequencer->isRunning() && e != 0) {
        switch ( e->type() )
        {
        // check for ESC
        case QEvent::KeyPress:
            {
                QKeyEvent* ke = (QKeyEvent*)e;
                if (ke->key() == Qt::Key_Escape) {
                    // eventFilter() was called from the application 50 times without performing a new step (app could hang)
                    if (d->observeEventFilter > 50) {
                        // tries to unlock the application if it hangs (probably due to incorrect usage of Base::Sequencer)
                        if (ke->modifiers() & (Qt::ControlModifier | Qt::AltModifier)) {
                            sequencer->resetData();
                            return true;
                        }
                    }

                    // cancel the operation
                    sequencer->tryToCancel();
                }

                return true;
            }   break;

        // ignore all these events
        case QEvent::KeyRelease:
        case QEvent::Enter:
        case QEvent::Leave:
        case QEvent::MouseButtonDblClick:
        case QEvent::ContextMenu:
            {
                return true;
            }   break;

        // special case if the main window's close button was pressed
        case QEvent::Close:
            {
                // avoid to exit while app is working
                // note: all other widget types are allowed to be closed anyway
                if (o == getMainWindow()) {
                    e->ignore();
                    return true;
                }
            }   break;

        // do a system beep and ignore the event
        case QEvent::MouseButtonPress:
            {
                if (d->isModalDialog(o))
                    return false;
                QApplication::beep();
                return true;
            }   break;

        default:
            {
            }   break;
        }

        d->observeEventFilter++;
    }

    return QProgressBar::eventFilter(o, e);
}


#include "moc_ProgressBar.cpp"
