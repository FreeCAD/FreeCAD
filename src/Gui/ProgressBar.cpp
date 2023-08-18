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
# include <QElapsedTimer>
# include <QKeyEvent>
# include <QMessageBox>
# include <QMetaObject>
# include <QThread>
# include <QTime>
# include <QTimer>
# include <QWindow>
#endif

#include "ProgressBar.h"
#include "MainWindow.h"
#include "ProgressDialog.h"
#include "WaitCursor.h"


using namespace Gui;


namespace Gui {
struct SequencerBarPrivate
{
    ProgressBar* bar;
    WaitCursor* waitCursor;
    QElapsedTimer measureTime;
    QElapsedTimer progressTime;
    QElapsedTimer checkAbortTime;
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
        if (!parent) {
            QWindow* window = qobject_cast<QWindow*>(o);
            if (window)
                parent = QWidget::find(window->winId());
        }
        while (parent) {
            auto* dlg = qobject_cast<QMessageBox*>(parent);
            if (dlg && dlg->isModal())
                return true;
            auto* pd = qobject_cast<QProgressDialog*>(parent);
            if (pd)
                return true;
            parent = parent->parentWidget();
        }

        return false;
    }
};
}

SequencerBar* SequencerBar::_pclSingleton = nullptr;

SequencerBar* SequencerBar::instance()
{
    // not initialized?
    if (!_pclSingleton)
    {
        _pclSingleton = new SequencerBar();
    }

    return _pclSingleton;
}

SequencerBar::SequencerBar()
{
    d = new SequencerBarPrivate;
    d->bar = nullptr;
    d->waitCursor = nullptr;
    d->guiThread = true;
}

SequencerBar::~SequencerBar()
{
    delete d;
}

void SequencerBar::pause()
{
    QThread *currentThread = QThread::currentThread();
    QThread *thr = d->bar->thread(); // this is the main thread
    d->bar->leaveControlEvents(d->guiThread);
    if (thr != currentThread)
        return;

    // allow key handling of dialog and restore cursor
    d->waitCursor->restoreCursor();
    QApplication::setOverrideCursor(Qt::ArrowCursor);
}

void SequencerBar::resume()
{
    QThread *currentThread = QThread::currentThread();
    QThread *thr = d->bar->thread(); // this is the main thread
    if (thr == currentThread) {
        QApplication::restoreOverrideCursor();
        d->waitCursor->setWaitCursor();
    }

    // must be called as last to get control before WaitCursor
    d->bar->enterControlEvents(d->guiThread); // grab again
}

void SequencerBar::startStep()
{
    QThread *currentThread = QThread::currentThread();
    QThread *thr = d->bar->thread(); // this is the main thread
    if (thr != currentThread) {
        d->guiThread = false;
        QMetaObject::invokeMethod(d->bar, "setRangeEx", Qt::QueuedConnection,
            Q_ARG(int, 0), Q_ARG(int, (int)nTotalSteps));
        d->progressTime.start();
        d->checkAbortTime.start();
        d->measureTime.start();
        QMetaObject::invokeMethod(d->bar, "aboutToShow", Qt::QueuedConnection);
        d->bar->enterControlEvents(d->guiThread);
    }
    else {
        d->guiThread = true;
        d->bar->setRangeEx(0, (int)nTotalSteps);
        d->progressTime.start();
        d->checkAbortTime.start();
        d->measureTime.start();
        d->waitCursor = new Gui::WaitCursor;
        d->bar->enterControlEvents(d->guiThread);
        d->bar->aboutToShow();
    }
}

void SequencerBar::checkAbort()
{
    if (d->bar->thread() != QThread::currentThread())
        return;
    if (!wasCanceled()) {
        if(d->checkAbortTime.elapsed() < 500)
            return;
        d->checkAbortTime.restart();
        qApp->processEvents();
        return;
    }
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
    }
}

void SequencerBar::nextStep(bool canAbort)
{
    QThread *currentThread = QThread::currentThread();
    QThread *thr = d->bar->thread(); // this is the main thread
    if (thr != currentThread) {
        if (wasCanceled() && canAbort) {
            abort();
        }
        else {
            setValue((int)nProgress + 1);
        }
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

void SequencerBar::setProgress(size_t step)
{
    QThread* currentThread = QThread::currentThread();
    QThread* thr = d->bar->thread(); // this is the main thread
    if (thr != currentThread) {
        QMetaObject::invokeMethod(d->bar, "show", Qt::QueuedConnection);
    }
    else {
        d->bar->show();
    }

    setValue((int)step);
}

void SequencerBar::setValue(int step)
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
                QMetaObject::invokeMethod(d->bar, "setValueEx", Qt::/*Blocking*/QueuedConnection,
                    Q_ARG(int,d->bar->value()+1));
            }
            else {
                d->bar->setValueEx(d->bar->value()+1);
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
                QMetaObject::invokeMethod(d->bar, "setValueEx", Qt::/*Blocking*/QueuedConnection,
                Q_ARG(int,step));
                if (d->bar->isVisible())
                    showRemainingTime();
            }
            else {
                d->bar->setValueEx(step);
                if (d->bar->isVisible())
                    showRemainingTime();
                d->bar->resetObserveEventFilter();
                qApp->processEvents();
            }
        }
    }
}

void SequencerBar::showRemainingTime()
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
                QMetaObject::invokeMethod(getMainWindow(), "showMessage",
                    Qt::/*Blocking*/QueuedConnection,
                    Q_ARG(QString,status));
            }
            else {
                getMainWindow()->showMessage(status);
            }
        }
    }
}

void SequencerBar::resetData()
{
    QThread *currentThread = QThread::currentThread();
    QThread *thr = d->bar->thread(); // this is the main thread
    if (thr != currentThread) {
        QMetaObject::invokeMethod(d->bar, "resetEx", Qt::QueuedConnection);
        QMetaObject::invokeMethod(d->bar, "aboutToHide", Qt::QueuedConnection);
        QMetaObject::invokeMethod(getMainWindow(), "showMessage",
            Qt::/*Blocking*/QueuedConnection,
            Q_ARG(QString,QString()));
        QMetaObject::invokeMethod(getMainWindow(), "setPaneText",
            Qt::/*Blocking*/QueuedConnection,
            Q_ARG(int,1),
            Q_ARG(QString,QString()));
        d->bar->leaveControlEvents(d->guiThread);
    }
    else {
        d->bar->resetEx();
        // Note: Under Qt 4.1.4 this forces to run QWindowsStyle::eventFilter() twice
        // handling the same event thus a warning is printed. Possibly, this is a bug
        // in Qt. The message is QEventDispatcherUNIX::unregisterTimer: invalid argument.
        d->bar->aboutToHide();
        delete d->waitCursor;
        d->waitCursor = nullptr;
        d->bar->leaveControlEvents(d->guiThread);
        getMainWindow()->setPaneText(1, QString());
        getMainWindow()->showMessage(QString());
    }

    SequencerBase::resetData();
}

void SequencerBar::abort()
{
    //resets
    resetData();
    Base::AbortException exc("User aborted");
    throw exc;
}

void SequencerBar::setText (const char* pszTxt)
{
    QThread *currentThread = QThread::currentThread();
    QThread *thr = d->bar->thread(); // this is the main thread

    // print message to the statusbar
    d->text = pszTxt ? QString::fromUtf8(pszTxt) : QLatin1String("");
    if (thr != currentThread) {
        QMetaObject::invokeMethod(getMainWindow(), "showMessage",
            Qt::/*Blocking*/QueuedConnection,
            Q_ARG(QString,d->text));
    }
    else {
        getMainWindow()->showMessage(d->text);
    }
}

bool SequencerBar::isBlocking() const
{
    return d->guiThread;
}

QProgressBar* SequencerBar::getProgressBar(QWidget* parent)
{
    if (!d->bar)
        d->bar = new ProgressBar(this, parent);
    return d->bar;
}

// -------------------------------------------------------

/* TRANSLATOR Gui::ProgressBar */

ProgressBar::ProgressBar (SequencerBar* s, QWidget * parent)
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
    connect(d->delayShowTimer, &QTimer::timeout, this, &ProgressBar::delayedShow);
    d->observeEventFilter = 0;

    setFixedWidth(120);

    // write percentage to the center
    setAlignment(Qt::AlignHCenter);
    hide();
}

ProgressBar::~ProgressBar ()
{
    disconnect(d->delayShowTimer, &QTimer::timeout, this, &ProgressBar::delayedShow);
    delete d->delayShowTimer;
    delete d;
}

int ProgressBar::minimumDuration() const
{
    return d->minimumDuration;
}

void ProgressBar::resetEx()
{
  QProgressBar::reset();
#ifdef QT_WINEXTRAS_LIB
  setupTaskBarProgress();
  m_taskbarProgress->reset();
#endif
}

void ProgressBar::setRangeEx(int minimum, int maximum)
{
  QProgressBar::setRange(minimum, maximum);
#ifdef QT_WINEXTRAS_LIB
  setupTaskBarProgress();
  m_taskbarProgress->setRange(minimum, maximum);
#endif
}

void ProgressBar::setValueEx(int value)
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
    auto ret = QMessageBox::question(getMainWindow(),tr("Aborting"),
    tr("Do you really want to abort the operation?"),  QMessageBox::Yes | QMessageBox::No,
    QMessageBox::No);

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

void ProgressBar::enterControlEvents(bool grab)
{
    qApp->installEventFilter(this);

    // Make sure that we get the key events, otherwise the Inventor viewer usurps the key events
    // This also disables accelerators.
#if defined(Q_OS_LINUX)
    Q_UNUSED(grab)
#else
    if (grab)
        grabKeyboard();
#endif
}

void ProgressBar::leaveControlEvents(bool release)
{
    qApp->removeEventFilter(this);

#if defined(Q_OS_LINUX)
    Q_UNUSED(release)
#else
    // release the keyboard again
    if (release)
        releaseKeyboard();
#endif
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
    if (sequencer->isRunning() && e) {
        QThread* currentThread = QThread::currentThread();
        QThread* thr = this->thread(); // this is the main thread
        if (thr != currentThread) {
            if (e->type() == QEvent::KeyPress) {
                auto ke = static_cast<QKeyEvent*>(e);
                if (ke->key() == Qt::Key_Escape) {
                    // cancel the operation
                    sequencer->tryToCancel();
                    return true;
                }
            }
            return QProgressBar::eventFilter(o, e);
        }

        // main thread
        switch ( e->type() )
        {
        // check for ESC
        case QEvent::KeyPress:
            {
                auto ke = static_cast<QKeyEvent*>(e);
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
        case QEvent::MouseButtonRelease:
        case QEvent::MouseMove:
        case QEvent::NativeGesture:
        case QEvent::ContextMenu:
            {
                if (!d->isModalDialog(o))
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
                if (!d->isModalDialog(o)) {
                    QApplication::beep();
                    return true;
                }
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
