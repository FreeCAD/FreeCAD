#include "ComputationDialog.h"

#include <mutex>
#include <condition_variable>
#include <atomic>
#include <thread>
#include <chrono>
#include <Python.h>
#include <FCConfig.h>
#include <sys/wait.h>

#include <QApplication>
#include <QCloseEvent>
#include <QMessageBox>
#include <QThread>
#include <Base/ProgressIndicator.h>
#include <App/Application.h>

namespace Gui {

// Add these helper functions at the beginning of the namespace
namespace {
    // Helper function to ensure complete writes
    bool writeExact(int fd, const void* buffer, size_t size) {
        size_t bytesWritten = 0;
        const char* buf = static_cast<const char*>(buffer);
        
        while (bytesWritten < size) {
            ssize_t result = write(fd, buf + bytesWritten, size - bytesWritten);
            
            if (result < 0) {
                // Error occurred
                if (errno == EINTR) {
                    // Interrupted by signal, retry
                    continue;
                }
                return false;
            }
            
            bytesWritten += result;
        }
        
        return true;
    }
}

// Static member declaration
ComputationDialog* ComputationDialog::currentDialog = nullptr;

void ComputationDialog::Show(float position, bool isForce) {
    (void)isForce;

    // write current position to the pipe as binary data, handle partial writes
    if (fd > 0) {
        writeExact(fd, &position, sizeof(float));
    }
}

bool ComputationDialog::UserBreak() {
    return aborted.load();
}

void ComputationDialog::run(std::function<void()> func) {
    std::atomic<bool> computationDone(false);
    std::mutex mutex;
    std::condition_variable cv;
    std::exception_ptr threadException;

    Base::ProgressIndicator::setInstance(this);

    // we launch a child process that will run the computation dialog so that all
    // of the FreeCAD code can run on the main thread, this works around issues
    // with code that needs to use the Python GIL or interact with Qt
    launchChildProcess();

    // run the actual computation
    try {
        func();
    } catch (...) {
        threadException = std::current_exception();
    }

    // Clean up child process
    stopChildProcess();

    Base::ProgressIndicator::resetInstance();

    // Re-throw any exception that occurred in the thread
    if (threadException) {
        std::rethrow_exception(threadException);
    }
}

void ComputationDialog::launchChildProcess() {
    int pipefd[2];

    // Store "this" pointer in the static class member
    currentDialog = this;

    // Ignore SIGPIPE to prevent termination when writing to a closed pipe
    signal(SIGPIPE, SIG_IGN);

    // Install SIGHUP handler - the child process will send a SIGHUP to the parent when
    // the "Cancel" button is clicked
    memset(&oldSa, 0, sizeof(oldSa));
    struct sigaction sa;
    memset(&sa, 0, sizeof(sa));
    sa.sa_handler = [](int) { 
        // Use the stored pointer instead of static_cast from ProgressIndicator
        if (currentDialog) {
            currentDialog->aborted.store(true);
        }
    };
    sigaction(SIGHUP, &sa, &oldSa);
    
    // Create pipe for communication
    if (pipe(pipefd) == -1) {
        throw std::runtime_error("Failed to create pipe");
    }
    
    // Fork process
    childPid = fork();
    
    if (childPid == -1) {
        // Fork failed
        close(pipefd[0]);
        close(pipefd[1]);
        throw std::runtime_error("Failed to fork process");
    }
    
    if (childPid == 0) {
        // Child process
        close(pipefd[1]); // Close write end of pipe
        
        // Redirect pipe to stdin
        if (dup2(pipefd[0], STDIN_FILENO) == -1) {
            fprintf(stderr, "Failed to redirect pipe to stdin\n");
            _exit(2);
        }
        close(pipefd[0]);
        
        // Launch FreeCAD binary
        std::string freecadExe = App::GetApplication().getHomePath() + "/bin/" + App::GetApplication().getExecutableName();
        execl(freecadExe.c_str(), App::GetApplication().getExecutableName().c_str(), "--computation-dialog", nullptr);
        
        // If execl returns, it failed
        fprintf(stderr, "Failed to launch FreeCAD binary %s\n", freecadExe.c_str());
        _exit(2);
    } else {
        // Parent process
        close(pipefd[0]); // Close read end of pipe
        fd = pipefd[1];   // Store write end for later use
        
    }
}

void ComputationDialog::stopChildProcess() {
    // Restore original signal handler
    sigaction(SIGHUP, &oldSa, nullptr);

    // Reset the static pointer
    currentDialog = nullptr;

    // Send SIGTERM to child process before waiting
    if (childPid > 0) {
        kill(childPid, SIGTERM);
    }

    // Clean up child process
    if (childPid > 0) {
        int status;
        waitpid(childPid, &status, 0);
        childPid = -1;
    }

    // Close pipe if open
    if (fd > 0) {
        close(fd);
        fd = -1;
    }
}

} // namespace Gui