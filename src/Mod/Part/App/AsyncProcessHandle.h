#ifndef PART_ASYNCPROCESSHANDLE_H
#define PART_ASYNCPROCESSHANDLE_H

#include <sys/types.h>
#include <Base/Exception.h>
#include <Mod/Part/PartGlobal.h>
#include <atomic>

namespace Part {

// Forward declaration
class TopoShape;

class PartExport AsyncProcessHandle {
public:
    AsyncProcessHandle(pid_t childPid, int resultFd);
    ~AsyncProcessHandle();

    // Abort the child process
    void abort();

    // Wait for child process to complete and return results
    // Returns either a TopoShape or throws an exception with the error message
    TopoShape join();

private:
    pid_t pid;
    int fd;
    std::atomic<bool> joined;  // Atomic flag for thread-safe access
    ssize_t readExact(void* buffer, size_t length);
};

} // namespace Part

#endif // PART_ASYNCPROCESSHANDLE_H
