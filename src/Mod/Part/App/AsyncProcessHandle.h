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
    AsyncProcessHandle() = default;
    ~AsyncProcessHandle();

    void setup(pid_t childPid, int resultFd);

    // Abort the child process
    void abort();

    // Wait for child process to complete and return results
    // Returns either a TopoShape or throws an exception with the error message
    TopoShape join();

    bool isValid();

private:
    pid_t pid;
    int fd;
    std::atomic<bool> valid;
};

} // namespace Part

#endif // PART_ASYNCPROCESSHANDLE_H
