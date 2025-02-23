#include "ComputationDialog.h"

#include <mutex>
#include <condition_variable>
#include <atomic>
#include <thread>
#include <chrono>

#ifdef _WIN32
#include <windows.h>
#endif

#include <QApplication>
#include <QCloseEvent>
#include <QMessageBox>
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
        
        QApplication::processEvents();
    }, Qt::QueuedConnection);
}

void ComputationDialog::abort() {
    aborted.store(true);
    reject();
}

void ComputationDialog::run(std::function<void()> func) {
    std::atomic<bool> computationDone(false);
    std::mutex mutex;
    std::condition_variable cv;
    std::exception_ptr threadException;

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
        #if defined(__linux__) || defined(__FreeBSD__)
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
            QMetaObject::invokeMethod(&forceAbortBox, "accept", Qt::QueuedConnection);
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
        // Wait for up to 3 seconds for the thread to finish
        std::unique_lock<std::mutex> lock(mutex);
        if (!cv.wait_for(lock, std::chrono::seconds(3),
            [&]{ return computationDone.load(); }))
        {
            if (forceAbortBox.exec() == QMessageBox::Yes) {
                // check computationDone again just in case the thread completed while the dialog was open
                if (!computationDone.load()) {
                    Base::Console().Error("Force aborting computation thread\n");

                    // TODO: save the backup document now!

                    #if defined(__linux__) || defined(__FreeBSD__)
                    pthread_cancel(computeThread.native_handle());
                    #elif defined(_WIN32)
                    TerminateThread(computeThread.native_handle(), 1);
                    CloseHandle(computeThread.native_handle());
                    #elif defined(__APPLE__)
                    pthread_kill(computeThread.native_handle(), SIGTERM);
                    #endif
                    computeThread.detach();
                    break; // Break out of the while loop
                }
            }
        }
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