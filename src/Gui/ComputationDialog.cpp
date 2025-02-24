#include "ComputationDialog.h"

#include <mutex>
#include <condition_variable>
#include <atomic>
#include <thread>
#include <chrono>
#include <Python.h>
#include <FCConfig.h>

#ifdef FC_OS_WIN32
#include <windows.h>
#endif

#include <QApplication>
#include <QCloseEvent>
#include <QMessageBox>
#include <QThread>
#include <Base/ProgressIndicator.h>

namespace Gui {

ComputationDialog::ComputationDialog(QWidget* parent)
    : QProgressDialog(parent)
    , aborted(false)
{
    setWindowTitle(tr("Computing"));
    setWindowFlags(Qt::Dialog | Qt::CustomizeWindowHint | Qt::WindowTitleHint | Qt::WindowStaysOnTopHint);
    setWindowModality(Qt::ApplicationModal);
    
    setLabelText(tr("This operation may take a while.\nPlease wait or press 'Cancel' to abort."));
    setMinimum(0);
    setMaximum(0); // Makes it indeterminate
    
    setMinimumDuration(0); // Show immediately
    setMinimumSize(300, 150);
    adjustSize();
    setFixedSize(size());

    connect(this, &QProgressDialog::canceled, this, &ComputationDialog::abort);
}

void ComputationDialog::Show(float position, bool isForce) {
    (void)isForce;

    // Ensure UI updates happen on the main thread
    QMetaObject::invokeMethod(this, [this, position]() {
        if (position < 0) {
            // set as "indeterminate"
            setMaximum(0);
            setValue(0);
        } else {
            int pct = std::clamp(static_cast<int>(position * 100), 0, 100);
            setMaximum(100);
            setValue(pct);
        }
    }, Qt::QueuedConnection);
}

void ComputationDialog::abort() {
    aborted.store(true);
    reject();
}

void forceTerminate(std::thread& thread) {
#if defined(FC_OS_WIN32)
    TerminateThread(thread.native_handle(), 1);
    CloseHandle(thread.native_handle());
#else
    pthread_cancel(thread.native_handle());
#endif
    thread.detach();
}

void ComputationDialog::run(std::function<void()> func) {
    std::atomic<bool> computationDone(false);
    std::mutex mutex;
    std::condition_variable cv;
    std::exception_ptr threadException;

    // If we already hold the GIL, just run directly to avoid deadlock
    // TODO: Can we find a way to transfer GIL ownership to the thread?
    // I found that when I tried to transfer GIL ownership to the thread
    // by releasing it in the main thread and acquiring it in the computation
    // thread, it caused a segfault which I could reproduce by opening up
    // a new spreadsheet, typing a number into the A1 cell, and then trying
    // to assign an alias to the cell. Don't know why.
    if (PyGILState_Check()) {
        func();
        return;
    }

    Base::ProgressIndicator::setInstance(this);

    QMessageBox forceAbortBox(
        QMessageBox::Warning,
        QObject::tr("Operation not responding"),
        QObject::tr("Aborting the operation is taking longer than expected.\nDo you want to forcibly cancel the thread? This will probably crash FreeCAD."),
        QMessageBox::Yes | QMessageBox::No,
        Gui::MainWindow::getInstance());
    forceAbortBox.setDefaultButton(QMessageBox::No);

    // Start computation thread
    std::thread computeThread([&]() {
#if !defined(FC_OS_WIN32)
        pthread_setcancelstate(PTHREAD_CANCEL_ENABLE, nullptr);
        pthread_setcanceltype(PTHREAD_CANCEL_ASYNCHRONOUS, nullptr);
#endif

        try {
            func();
        } catch (...) {
            threadException = std::current_exception();
        }

        computationDone.store(true);

        if (isVisible()) {
            QMetaObject::invokeMethod(this, "accept", Qt::QueuedConnection);
        }
        if (forceAbortBox.isVisible()) {
            QMetaObject::invokeMethod(&forceAbortBox, "reject", Qt::QueuedConnection);
        }

        cv.notify_one();
    });

    {
        // Wait for a brief moment to see if computation completes quickly
        std::unique_lock<std::mutex> lock(mutex);
        if (!cv.wait_for(lock, std::chrono::seconds(1),
            [&]{ return computationDone.load(); }))  // Atomic load
        {
            // Computation didn't finish quickly, show dialog
            exec();
        }
    }

    while (!computationDone.load()) {
        const int waitForThread = 3;  // seconds
        std::unique_lock<std::mutex> lock(mutex);
        const bool waited = cv.wait_for(lock, std::chrono::seconds(waitForThread),
            [&]{ return computationDone.load(); });
        if (waited || forceAbortBox.exec() != QMessageBox::Yes || computationDone.load()) {
            continue;
        }
        Base::Console().Error("Force aborting computation thread\n");

        // TODO: save the backup document now!

        forceTerminate(computeThread);
        break;
    }

    if (computeThread.joinable()) {
        computeThread.join();
    }

    Base::ProgressIndicator::resetInstance();

    // Re-throw any exception that occurred in the thread
    if (threadException) {
        std::rethrow_exception(threadException);
    }
}

bool ComputationDialog::UserBreak() {
    return aborted.load();
}

void ComputationDialog::closeEvent(QCloseEvent* event) {
    event->ignore();
}

} // namespace Gui