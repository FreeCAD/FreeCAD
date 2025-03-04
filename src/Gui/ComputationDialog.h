#ifndef GUI_COMPUTATION_DIALOG_H
#define GUI_COMPUTATION_DIALOG_H

#include <atomic>
#include <functional>
#include <signal.h>
#include "Base/ProgressIndicator.h"

#include "FCGlobal.h"

namespace Gui {

class GuiExport ComputationDialog : public Base::ProgressIndicator {
public:
    void Show(float position, bool isForce) override;
    bool UserBreak() override;
    void run(std::function<void()> func);

protected:
    void launchChildProcess();
    void stopChildProcess();

private:
    std::atomic<bool> aborted;
    int fd = -1;
    int childPid = -1;
    struct sigaction oldSa;
    static ComputationDialog* currentDialog;
};

} // namespace Gui

#endif // GUI_COMPUTATION_DIALOG_H
