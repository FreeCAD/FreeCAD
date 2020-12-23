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

#include "ProgressDialog.h"
#include "MainWindow.h"


using namespace Gui;


namespace Gui {
struct SequencerDialogPrivate
{
    ProgressDialog* dlg;
    QTime measureTime;
    QTime progressTime;
    QTime checkAbortTime;
    QString text;
    bool guiThread;
};
}


SequencerDialog* SequencerDialog::_pclSingleton = 0;

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
    d->guiThread = true;
}

SequencerDialog::~SequencerDialog()
{
    delete d;
}

void SequencerDialog::pause()
{
    QThread *currentThread = QThread::currentThread();
    QThread *thr = d->dlg->thread(); // this is the main thread
    if (thr == currentThread)
        // allow key handling of dialog
        d->dlg->leaveControlEvents();
}

void SequencerDialog::resume()
{
    QThread *currentThread = QThread::currentThread();
    QThread *thr = d->dlg->thread(); // this is the main thread
    if (thr == currentThread)
        d->dlg->enterControlEvents(); // grab again
}

void SequencerDialog::startStep()
{
    QThread *currentThread = QThread::currentThread();
    QThread *thr = d->dlg->thread(); // this is the main thread
    if (thr != currentThread) {
        d->guiThread = false;
        d->dlg->setRange(0, (int)nTotalSteps);
        d->dlg->setModal(false);
        if (nTotalSteps == 0) {
            d->progressTime.start();
            d->checkAbortTime.start();
        }

        d->measureTime.start();
        QMetaObject::invokeMethod(d->dlg, "setValue", Qt::QueuedConnection,
            QGenericReturnArgument(), Q_ARG(int,0));
    }
    else {
        d->guiThread = true;
        d->dlg->setRange(0, (int)nTotalSteps);
        d->dlg->setModal(true);
        if (nTotalSteps == 0) {
            d->progressTime.start();
            d->checkAbortTime.start();
        }

        d->measureTime.start();
        d->dlg->setValue(0);
        d->dlg->enterControlEvents();
    }
}

void SequencerDialog::checkAbort() {
    if(d->dlg->thread() != QThread::currentThread())
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
    bool ok = d->dlg->canAbort();
    // continue and show up wait cursor if needed
    resume();

    // force to abort the operation
    if ( ok ) {
        abort();
    } else {
        rejectCancel();
    }
}

void SequencerDialog::nextStep(bool canAbort)
{
    QThread *currentThread = QThread::currentThread();
    QThread *thr = d->dlg->thread(); // this is the main thread
    if (thr != currentThread) {
        setProgress((int)nProgress+1);
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
                setProgress((int)nProgress+1);
            }
        }
        else {
            setProgress((int)nProgress+1);
        }
    }
}

void SequencerDialog::setProgress(int step)
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
                QMetaObject::invokeMethod(d->dlg, "setValue", Qt::/*Blocking*/QueuedConnection,
                    QGenericReturnArgument(), Q_ARG(int,d->dlg->value()+1));
            }
            else {
                d->dlg->setValue(d->dlg->value()+1);
                qApp->processEvents();
            }
        }
    }
    else {
        if (thr != currentThread) {
            QMetaObject::invokeMethod(d->dlg, "setValue", Qt::/*Blocking*/QueuedConnection,
                QGenericReturnArgument(), Q_ARG(int,step));
            if (d->dlg->isVisible())
                showRemainingTime();
        }
        else {
            d->dlg->setValue(step);
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
            QString status = QString::fromLatin1("%1\t[%2]").arg(txt).arg(remain);

            if (thr != currentThread) {
                QMetaObject::invokeMethod(d->dlg, "setLabelText",
                    Qt::/*Blocking*/QueuedConnection,
                    QGenericReturnArgument(),
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
        QMetaObject::invokeMethod(d->dlg, "reset", Qt::QueuedConnection);
        QMetaObject::invokeMethod(d->dlg, "hide", Qt::QueuedConnection);
        QMetaObject::invokeMethod(d->dlg, "setLabelText",
            Qt::/*Blocking*/QueuedConnection,
            QGenericReturnArgument(),
            Q_ARG(QString,QString()));
    }
    else {
        d->dlg->reset();
        // Note: Under Qt 4.1.4 this forces to run QWindowsStyle::eventFilter() twice 
        // handling the same event thus a warning is printed. Possibly, this is a bug
        // in Qt. The message is QEventDispatcherUNIX::unregisterTimer: invalid argument.
        d->dlg->hide();
        d->dlg->setLabelText(QString());
        d->dlg->leaveControlEvents();
    }

    SequencerBase::resetData();
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
            QGenericReturnArgument(),
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
    connect(this, SIGNAL(canceled()), this, SLOT(onCancel()));
}

ProgressDialog::~ProgressDialog ()
{
}

void ProgressDialog::onCancel()
{
    sequencer->tryToCancel();
}

bool ProgressDialog::canAbort() const
{
    int ret = QMessageBox::question(getMainWindow(),tr("Aborting"),
    tr("Do you really want to abort the operation?"),  QMessageBox::Yes, 
    QMessageBox::No|QMessageBox::Default);

    return (ret == QMessageBox::Yes) ? true : false;
}

void ProgressDialog::enterControlEvents()
{
    qApp->installEventFilter(this);

    // Make sure that we get the key events, otherwise the Inventor viewer usurps the key events
    // This also disables accelerators.
    grabKeyboard();
}

void ProgressDialog::leaveControlEvents()
{
    qApp->removeEventFilter(this);

    // release the keyboard again
    releaseKeyboard();
}

bool ProgressDialog::eventFilter(QObject* o, QEvent* e)
{
    if (sequencer->isRunning() && e != 0) {
        switch ( e->type() )
        {
        // check for ESC
        case QEvent::KeyPress:
            {
                QKeyEvent* ke = (QKeyEvent*)e;
                if (ke->key() == Qt::Key_Escape) {
                    // cancel the operation
                    this->cancel();
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
                QApplication::beep();
                return true;
            }   break;

        default:
            {
            }   break;
        }
    }

    return QProgressDialog::eventFilter(o, e);
}

#include "moc_ProgressDialog.cpp"
