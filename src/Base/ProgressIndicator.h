#ifndef BASE_PROGRESSINDICATOR_H
#define BASE_PROGRESSINDICATOR_H

#include <memory>

#include "FCGlobal.h"

namespace Base
{

class BaseExport ProgressIndicator
{
public:
    virtual ~ProgressIndicator() = default;

    // Core interface matching Message_ProgressIndicator
    virtual bool UserBreak()
    {
        return false;
    }  // true if user requests break
    virtual void Show(float position, bool isForce)
    {
        (void)position;
        (void)isForce;
    }  // show the progress

    // Singleton management
    static ProgressIndicator& getInstance();
    static void setInstance(ProgressIndicator* newInstance);
    static void resetInstance();

private:
    static ProgressIndicator* instance;
    static ProgressIndicator* defaultInstance;
};

}  // namespace Base
#endif
