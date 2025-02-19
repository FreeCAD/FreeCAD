#include "PreCompiled.h"
#include "TopoShape.h"
#include "AsyncProcessHandle.h"
#include "BooleanOperation.h"

#include <signal.h>
#include <sys/wait.h>
#include <unistd.h>
#include <sstream>
#include <vector>

#include <Base/Exception.h>
#include <Base/Console.h>

#include <Mod/Part/PartGlobal.h>

using namespace Part;

AsyncProcessHandle::AsyncProcessHandle(pid_t childPid, int resultFd)
    : pid(childPid)
    , fd(resultFd)
    , joined(false)
{
}

AsyncProcessHandle::~AsyncProcessHandle()
{
    if (!joined) {
        // Attempt to abort if we haven't joined
        try {
            abort();
        } catch (...) {
            // Ignore exceptions in destructor
        }
    }
    
    if (fd >= 0) {
        close(fd);
    }
}

void AsyncProcessHandle::abort()
{
    if (pid > 0) {
        kill(pid, SIGTERM);
        // Give it a moment to terminate gracefully
        usleep(100000);  // 100ms
        
        // Force kill if still running
        if (kill(pid, 0) == 0) {
            kill(pid, SIGKILL);
        }
    }
}

ssize_t AsyncProcessHandle::readExact(void* buffer, size_t length) {
    ssize_t totalRead = 0;
    while (totalRead < length) {
        ssize_t bytes = read(fd, static_cast<char*>(buffer) + totalRead, length - totalRead);
        if (bytes <= 0) {
            waitpid(pid, nullptr, 0); // Ensure child process is cleaned up
            throw Base::RuntimeError("Failed to read exact number of bytes");
        }
        totalRead += bytes;
    }
    return totalRead;
}

TopoShape AsyncProcessHandle::join()
{
    bool expected = false;
    if (!joined.compare_exchange_strong(expected, true)) {
        throw Base::RuntimeError("Process has already been joined");
    }

    // Wait for child process first to ensure clean exit
    int status;
    if (waitpid(pid, &status, 0) == -1) {
        throw Base::RuntimeError("Error waiting for child process");
    }

    // Check exit status before processing any data
    if (!WIFEXITED(status) || WEXITSTATUS(status) != 0) {
        throw Base::RuntimeError("Child process failed");
    }

    // Read result using BooleanOperation's protocol
    bool isError;
    std::string result = BooleanOperation::readResult(fd, isError);
    
    if (isError) {
        throw Base::RuntimeError(result);
    }

    // Process shape data
    std::istringstream str(result);
    Part::TopoShape shape;
    shape.importBinary(str);
    return shape;
}
