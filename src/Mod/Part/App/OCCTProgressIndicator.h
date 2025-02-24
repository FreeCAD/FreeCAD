#ifndef PART_OCCTPROGRESSINDICATOR_H
#define PART_OCCTPROGRESSINDICATOR_H

#include <Base/ProgressIndicator.h>
#include <Base/Console.h>
#include <Message_ProgressIndicator.hxx>
#include <chrono>
#if defined(FC_OS_LINUX)
#include <execinfo.h>
#endif

#include <Mod/Part/PartGlobal.h>

namespace Part {

class PartExport OCCTProgressIndicator : public Message_ProgressIndicator
{
public:
    OCCTProgressIndicator() 
        : Message_ProgressIndicator()
    {
        if (Base::ProgressIndicator::getInstance().isDefaultInstance()) {
            startedAt = std::chrono::steady_clock::now();
#if defined(FC_OS_LINUX)
            void *array[50];
            size_t size = backtrace(array, 40);
            char **strings = backtrace_symbols(array, size);
            std::stringstream str;
            for (size_t i = 0; i < size; i++) {
                str << strings[i] << std::endl;
            }
            startBacktrace = str.str();
            free(strings);
#else
            startBacktrace = "Backtrace not available on this platform";
#endif
        }
    }

    ~OCCTProgressIndicator()
    {
        if (!startBacktrace.empty()) {  // Only if we captured a backtrace
            auto now = std::chrono::steady_clock::now();
            auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(now - startedAt).count();
            if (duration > 1000) {  // 1 second
                Base::Console().Message("OCCT Progress operation took %d ms\n", duration);
                Base::Console().Message("Started at:\n%s\n", startBacktrace.c_str());
            }
        }
    }

    Standard_Boolean UserBreak() override { 
        return Base::ProgressIndicator::getInstance().UserBreak(); 
    }

    void Show(const Message_ProgressScope& scope, const Standard_Boolean isForce) override {
        float pos = -1; // negative means indeterminate
        if (!scope.IsInfinite()) {
            pos = GetPosition();
        }
        Base::ProgressIndicator::getInstance().Show(pos, isForce);
    }

private:
    std::chrono::steady_clock::time_point startedAt;
    std::string startBacktrace;
};

} // namespace Part

#endif // PART_OCCTPROGRESSINDICATOR_H
