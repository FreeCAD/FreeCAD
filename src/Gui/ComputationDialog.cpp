#include "ComputationDialog.h"

#include <mutex>
#include <condition_variable>
#include <atomic>
#include <thread>
#include <chrono>
#include <QCloseEvent>
#include <QMessageBox>
#include <Base/ProgressIndicator.h>

namespace Gui {

ComputationDialog::ComputationDialog(QWidget* parent)
    : QDialog(parent)
    , aborted(false)
{
    setWindowTitle(tr("Computing"));
    setWindowFlags(Qt::Dialog | Qt::CustomizeWindowHint | Qt::WindowTitleHint | Qt::WindowStaysOnTopHint);
    setWindowModality(Qt::ApplicationModal); // Make sure dialog is modal but doesn't block event processing
    
    auto layout = new QVBoxLayout(this);
    
    // Add informative label
    auto label = new QLabel(tr("This operation may take a while.\nPlease wait or press 'Abort' to cancel."), this);
    label->setAlignment(Qt::AlignCenter);
    layout->addWidget(label);
    
    progressBar = new QProgressBar(this);
    progressBar->setMinimum(0);
    progressBar->setMaximum(0); // This makes it indeterminate
    layout->addWidget(progressBar);

    // Add status label below progress bar
    statusLabel = new QLabel(this);
    statusLabel->setAlignment(Qt::AlignCenter);
    layout->addWidget(statusLabel);
    
    auto abortButton = new QPushButton(tr("Abort"), this);
    layout->addWidget(abortButton);
    connect(abortButton, &QPushButton::clicked, this, &ComputationDialog::onAbort);

    setMinimumSize(300, 150);  // Set reasonable minimum dimensions
    adjustSize();  // Adjust to content
    setFixedSize(size());  // Lock the size
}

void ComputationDialog::Show(float position, bool isForce) {
    (void)isForce;
    int pct = std::clamp(static_cast<int>(position * 100), 0, 100);

    // Update both the status label and progress bar using invokeMethod
    QString status = tr("Progress: %1%").arg(pct);
    QMetaObject::invokeMethod(statusLabel, "setText", Qt::QueuedConnection,
                            Q_ARG(QString, status));

    QMetaObject::invokeMethod(progressBar, "setValue", Qt::QueuedConnection,
                            Q_ARG(int, pct));
    QMetaObject::invokeMethod(progressBar, "setMaximum", Qt::QueuedConnection,
                            Q_ARG(int, 100));

    // Process events to keep UI responsive
    QApplication::processEvents();
}

void ComputationDialog::abort() {
    aborted.store(true);
    reject();
}

void ComputationDialog::run(std::function<void()> func) {
    std::atomic<bool> computationDone(false);
    std::mutex mutex;  // Still needed for the condition variable
    std::condition_variable cv;

    Base::ProgressIndicator::setInstance(this);

    QMessageBox forceAbortBox(
        QMessageBox::Warning,
        QObject::tr("Operation not responding"),
        QObject::tr("The abort operation is taking longer than expected.\nDo you want to forcibly cancel the thread? This will probably crash FreeCAD."),
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
        } catch (const std::exception& e) {
            Base::Console().Error("Computation thread caught exception: %s\n", e.what());
        } catch (...) {
            Base::Console().Error("Computation thread caught unknown exception\n");
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
        std::unique_lock<std::mutex> lock(mutex);  // Fixed: std::lock -> std::mutex
        if (!cv.wait_for(lock, std::chrono::seconds(3),
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
}

bool ComputationDialog::UserBreak() {
    return aborted.load();
}

void ComputationDialog::closeEvent(QCloseEvent* event) {
    event->ignore();
}

void ComputationDialog::onAbort() {
    abort();
}

} // namespace Gui