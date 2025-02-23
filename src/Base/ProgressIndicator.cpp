#include "ProgressIndicator.h"

namespace Base
{

ProgressIndicator* ProgressIndicator::defaultInstance = new ProgressIndicator();
ProgressIndicator* ProgressIndicator::instance = defaultInstance;

ProgressIndicator& ProgressIndicator::getInstance()
{
    return *instance;
}

void ProgressIndicator::setInstance(ProgressIndicator* newInstance)
{
    if (newInstance) {
        instance = newInstance;
    }
    else {
        resetInstance();
    }
}

void ProgressIndicator::resetInstance()
{
    instance = defaultInstance;
}

}  // namespace Base
