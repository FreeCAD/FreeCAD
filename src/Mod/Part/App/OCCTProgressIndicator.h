#ifndef PART_OCCTPROGRESSINDICATOR_H
#define PART_OCCTPROGRESSINDICATOR_H

#include <Base/ProgressIndicator.h>
#include <Message_ProgressIndicator.hxx>

#include <Mod/Part/PartGlobal.h>

namespace Part {

class PartExport OCCTProgressIndicator : public Message_ProgressIndicator
{
public:
    OCCTProgressIndicator() 
        : Message_ProgressIndicator()
    {}

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
};

} // namespace Part

#endif // PART_OCCTPROGRESSINDICATOR_H
