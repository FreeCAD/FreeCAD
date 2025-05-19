#include "ComputationDialog.h"

#include <mutex>
#include <condition_variable>
#include <atomic>
#include <thread>
#include <chrono>

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

    // Start computation thread
    std::thread computeThread([&]() {
        try {
            func();
        } catch (...) {
            threadException = std::current_exception();
        }

        computationDone.store(true);

        if (isVisible()) {
            QMetaObject::invokeMethod(this, "accept", Qt::QueuedConnection);
        }

        cv.notify_one();
    });

    while (!computationDone.load()) {
        // Wait for a brief moment to see if computation completes quickly
        std::unique_lock<std::mutex> lock(mutex);
        if (!cv.wait_for(lock, std::chrono::seconds(1),
            [&]{ return computationDone.load(); }))  // Atomic load
        {
            // Computation didn't finish quickly, show dialog
            exec();
        }
    }

    computeThread.join();

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