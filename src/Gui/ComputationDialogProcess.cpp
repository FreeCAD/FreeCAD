#include "ComputationDialogProcess.h"

#include <atomic>
#include <thread>
#include <signal.h>
#include <unistd.h>
#include <iostream>

#include <QApplication>
#include <QCloseEvent>
#include <QMetaObject>

namespace Gui {

namespace {
    // Helper function to ensure complete reads
    bool readExact(int fd, void* buffer, size_t size) {
        size_t bytesRead = 0;
        char* buf = static_cast<char*>(buffer);
        
        while (bytesRead < size) {
            ssize_t result = read(fd, buf + bytesRead, size - bytesRead);
            
            if (result < 0) {
                // Error occurred
                if (errno == EINTR) {
                    // Interrupted by signal, retry
                    continue;
                }
                return false;
            } else if (result == 0) {
                // EOF
                return false;
            }
            
            bytesRead += result;
        }
        
        return true;
    }
}

ComputationDialogProcess::ComputationDialogProcess(QWidget* parent)
    : QProgressDialog(parent)
    , aborted(false)
{
    setWindowTitle(tr("Computing"));
    setWindowFlags(Qt::Dialog | Qt::CustomizeWindowHint | Qt::WindowTitleHint | Qt::WindowStaysOnTopHint);
    setWindowModality(Qt::ApplicationModal);
    
    setLabelText(tr("This operation may take a while.\nPlease wait or press 'Cancel' to abort."));
    setMinimum(0);
    setMaximum(100);
    
    setMinimumDuration(0); // Show immediately
    setMinimumSize(300, 150);
    adjustSize();
    setFixedSize(size());

    connect(this, &QProgressDialog::canceled, this, &ComputationDialogProcess::abort);
}

void ComputationDialogProcess::Show(float position, bool isForce) {
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

void ComputationDialogProcess::abort() {
    aborted.store(true);
    // Send SIGHUP to parent process
    kill(getppid(), SIGHUP);
}

void ComputationDialogProcess::run() {
    // Start a thread to read from stdin
    std::thread inputThread([this]() {
        float position;
        while (readExact(STDIN_FILENO, &position, sizeof(float))) {
            Show(position, false);
        }
        
        // EOF on stdin, now we can close the dialog
        QMetaObject::invokeMethod(this, &QDialog::accept, Qt::QueuedConnection);
    });

    // Show the dialog and wait for user interaction
    exec();

    // Clean up
    if (inputThread.joinable()) {
        inputThread.join();
    }
}

bool ComputationDialogProcess::UserBreak() {
    return aborted.load();
}

void ComputationDialogProcess::closeEvent(QCloseEvent* event) {
    event->ignore();
}

} // namespace Gui