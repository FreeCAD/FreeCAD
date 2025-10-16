#include <cstddef>
#include <execinfo.h>

int main() {
    void *callstack[128];
    size_t nMaxFrames = sizeof(callstack) / sizeof(callstack[0]);
    size_t nFrames = backtrace(callstack, nMaxFrames);
    char **symbols = backtrace_symbols(callstack, nFrames);
    return 0;
}