/***************************************************************************
 *   Copyright (c) 2009 Werner Mayer <wmayer@users.sourceforge.net>        *
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
# include <QMessageBox>
# include <QPushButton>
# include <QThread>
# include <QTime>
#endif

#include "ProgressDialog.h"
#include "MainWindow.h"


using namespace Gui;

namespace Gui {
struct SequencerDialogPrivate
{
    ProgressDialog* dlg;
    QElapsedTimer measureTime;
    QElapsedTimer progressTime;
    QString text;
    bool guiThread;
    bool canabort;
};
}

SequencerDialog* SequencerDialog::_pclSingleton = nullptr;

SequencerDialog* SequencerDialog::instance()
{
    // not initialized?
    if (!_pclSingleton)
        _pclSingleton = new SequencerDialog();
    return _pclSingleton;
}

SequencerDialog::SequencerDialog ()
{
    d = new SequencerDialogPrivate;
    d->dlg = new ProgressDialog(this,getMainWindow());
    d->dlg->reset(); // stops the timer to force showing the dialog
    d->dlg->hide();
    d->guiThread = true;
    d->canabort = false;
}

SequencerDialog::~SequencerDialog()
{
    delete d;
}

void SequencerDialog::pause()
{
}

void SequencerDialog::resume()
{
}

void SequencerDialog::startStep()
{
    QThread *currentThread = QThread::currentThread();
    QThread *thr = d->dlg->thread(); // this is the main thread
    if (thr != currentThread) {
        d->guiThread = false;
        QMetaObject::invokeMethod(d->dlg, "setRangeEx", Qt::QueuedConnection,
            Q_ARG(int, 0), Q_ARG(int, (int)nTotalSteps));
        d->dlg->setModal(false);
        if (nTotalSteps == 0) {
            d->progressTime.start();
        }

        d->measureTime.start();
        QMetaObject::invokeMethod(d->dlg, "setValueEx", Qt::QueuedConnection,
            Q_ARG(int,0));
        QMetaObject::invokeMethod(d->dlg, "aboutToShow", Qt::QueuedConnection);
    }
    else {
        d->guiThread = true;
        d->dlg->setRangeEx(0, (int)nTotalSteps);
        d->dlg->setModal(true);
        if (nTotalSteps == 0) {
            d->progressTime.start();
        }

        d->measureTime.start();
        d->dlg->setValueEx(0);
        d->dlg->aboutToShow();
    }
}

void SequencerDialog::nextStep(bool canAbort)
{
    d->canabort = canAbort;
    QThread *currentThread = QThread::currentThread();
    QThread *thr = d->dlg->thread(); // this is the main thread
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
            bool ok = d->dlg->canAbort();
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

void SequencerDialog::setProgress(size_t step)
{
    QThread* currentThread = QThread::currentThread();
    QThread* thr = d->dlg->thread(); // this is the main thread
    if (thr != currentThread) {
        QMetaObject::invokeMethod(d->dlg, "show", Qt::QueuedConnection);
    }
    else {
        d->dlg->show();
    }

    setValue((int)step);
}

void SequencerDialog::setValue(int step)
{
    QThread *currentThread = QThread::currentThread();
    QThread *thr = d->dlg->thread(); // this is the main thread
    // if number of total steps is unknown then increment only by one
    if (nTotalSteps == 0) {
        int elapsed = d->progressTime.elapsed();
        // allow an update every 500 milliseconds only
        if (elapsed > 500) {
            d->progressTime.restart();
            if (thr != currentThread) {
                QMetaObject::invokeMethod(d->dlg, "setValueEx", Qt::/*Blocking*/QueuedConnection,
                    Q_ARG(int,d->dlg->value()+1));
            }
            else {
                d->dlg->setValueEx(d->dlg->value()+1);
                qApp->processEvents();
            }
        }
    }
    else {
        if (thr != currentThread) {
            QMetaObject::invokeMethod(d->dlg, "setValueEx", Qt::/*Blocking*/QueuedConnection,
                Q_ARG(int,step));
            if (d->dlg->isVisible())
                showRemainingTime();
        }
        else {
            d->dlg->setValueEx(step);
            if (d->dlg->isVisible())
                showRemainingTime();
            qApp->processEvents();
        }
    }
}

void SequencerDialog::showRemainingTime()
{
    QThread *currentThread = QThread::currentThread();
    QThread *thr = d->dlg->thread(); // this is the main thread

    int elapsed = d->measureTime.elapsed();
    int progress = d->dlg->value();
    int totalSteps = d->dlg->maximum() - d->dlg->minimum();

    QString txt = d->text;
    // More than 5 percent complete or more than 5 secs have elapsed.
    if (progress * 20 > totalSteps || elapsed > 5000) {
        int rest = (int) ( (double) totalSteps/progress * elapsed ) - elapsed;

        // more than 1 secs have elapsed and at least 100 ms are remaining
        if (elapsed > 1000 && rest > 100) {
            QTime time( 0,0, 0);
            time = time.addSecs( rest/1000 );
            QString remain = Gui::ProgressDialog::tr("Remaining: %1").arg(time.toString());
            QString status = QString::fromLatin1("%1\t[%2]").arg(txt, remain);

            if (thr != currentThread) {
                QMetaObject::invokeMethod(d->dlg, "setLabelText",
                    Qt::/*Blocking*/QueuedConnection,
                    Q_ARG(QString,status));
            }
            else {
                d->dlg->setLabelText(status);
            }
        }
    }
}

void SequencerDialog::resetData()
{
    QThread *currentThread = QThread::currentThread();
    QThread *thr = d->dlg->thread(); // this is the main thread
    if (thr != currentThread) {
        QMetaObject::invokeMethod(d->dlg, "resetEx", Qt::QueuedConnection);
        QMetaObject::invokeMethod(d->dlg, "hide", Qt::QueuedConnection);
        QMetaObject::invokeMethod(d->dlg, "setLabelText",
            Qt::/*Blocking*/QueuedConnection,
            Q_ARG(QString,QString()));
    }
    else {
        d->dlg->resetEx();
        // Note: Under Qt 4.1.4 this forces to run QWindowsStyle::eventFilter() twice
        // handling the same event thus a warning is printed. Possibly, this is a bug
        // in Qt. The message is QEventDispatcherUNIX::unregisterTimer: invalid argument.
        d->dlg->hide();
        d->dlg->setLabelText(QString());
    }

    SequencerBase::resetData();
}

bool SequencerDialog::canAbort() const
{
    return d->canabort;
}

void SequencerDialog::abort()
{
    //resets
    resetData();
    Base::AbortException exc("User aborted");
    throw exc;
}

void SequencerDialog::setText (const char* pszTxt)
{
    QThread *currentThread = QThread::currentThread();
    QThread *thr = d->dlg->thread(); // this is the main thread

    // set label text of the dialog
    d->text = pszTxt ? QString::fromUtf8(pszTxt) : QLatin1String("");
    if (thr != currentThread) {
        QMetaObject::invokeMethod(d->dlg, "setLabelText",
            Qt::/*Blocking*/QueuedConnection,
            Q_ARG(QString,d->text));
    }
    else {
        d->dlg->setLabelText(d->text);
    }
}

bool SequencerDialog::isBlocking() const
{
    return d->guiThread;
}

// -------------------------------------------------------

/* TRANSLATOR Gui::ProgressDialog */

ProgressDialog::ProgressDialog (SequencerDialog* s, QWidget * parent)
    : QProgressDialog(parent, Qt::FramelessWindowHint), sequencer(s)
{
#ifdef QT_WINEXTRAS_LIB
    m_taskbarButton = nullptr;
    m_taskbarButton = nullptr;
#endif
    connect(this, &QProgressDialog::canceled, this, &ProgressDialog::onCancel);
}

ProgressDialog::~ProgressDialog() = default;

void ProgressDialog::onCancel()
{
    sequencer->tryToCancel();
}

bool ProgressDialog::canAbort() const
{
    auto ret = QMessageBox::question(getMainWindow(),tr("Aborting"),
    tr("Do you really want to abort the operation?"),  QMessageBox::Yes | QMessageBox::No,
    QMessageBox::No);

    return (ret == QMessageBox::Yes) ? true : false;
}

void ProgressDialog::showEvent(QShowEvent* ev)
{
    QPushButton* btn = findChild<QPushButton*>();
    if (btn)
        btn->setEnabled(sequencer->canAbort());
    QProgressDialog::showEvent(ev);
}

void ProgressDialog::hideEvent(QHideEvent* ev)
{
    QProgressDialog::hideEvent(ev);
}

void ProgressDialog::resetEx()
{
    QProgressDialog::reset();
#ifdef QT_WINEXTRAS_LIB
    setupTaskBarProgress();
    m_taskbarProgress->reset();
#endif
}

void ProgressDialog::setRangeEx(int minimum, int maximum)
{
    QProgressDialog::setRange(minimum, maximum);
#ifdef QT_WINEXTRAS_LIB
    setupTaskBarProgress();
    m_taskbarProgress->setRange(minimum, maximum);
#endif
}

void ProgressDialog::setValueEx(int value)
{
    QProgressDialog::setValue(value);
#ifdef QT_WINEXTRAS_LIB
    setupTaskBarProgress();
    m_taskbarProgress->setValue(value);
#endif
}

void ProgressDialog::aboutToShow()
{
#ifdef QT_WINEXTRAS_LIB
    setupTaskBarProgress();
    m_taskbarProgress->show();
#endif
}

void ProgressDialog::aboutToHide()
{
    hide();
#ifdef QT_WINEXTRAS_LIB
    setupTaskBarProgress();
    m_taskbarProgress->hide();
#endif
}

#ifdef QT_WINEXTRAS_LIB
void ProgressDialog::setupTaskBarProgress()
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

#include "moc_ProgressDialog.cpp"
