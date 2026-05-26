// SPDX-License-Identifier: LGPL-2.1-or-later

#include <iostream>

#include "FeatureThread.h"
#include "FeatureDressUp.h"

namespace PartDesign
{

PROPERTY_SOURCE(PartDesign::Thread, PartDesign::DressUp)

Thread::Thread()
{
    addThreadType();

    ADD_PROPERTY_TYPE(ThreadFit, (0L), "Hole", App::Prop_None, "Clearance hole fit");
    // ThreadFit.setEnums(ClearanceMetricEnums);

    ADD_PROPERTY_TYPE(ThreadDiameter, (0.0), "Hole", App::Prop_None, "Thread major diameter");
    // ThreadDiameter.setReadOnly(true);

    ADD_PROPERTY_TYPE(ThreadDirection, (0L), "Hole", App::Prop_None, "Thread direction");
    // ThreadDirection.setEnums(ThreadDirectionEnums);
    // ThreadDirection.setReadOnly(true);
}

App::DocumentObjectExecReturn* Thread::execute()
{
    Base::Console().message("THREAD EXECUTED");

    return new App::DocumentObjectExecReturn(
        QT_TRANSLATE_NOOP("Exception", "Thread failed: thread not implemented")
    );
}

void Thread::onChanged(const App::Property* prop)
{
    DressUp::onChanged(prop);
}

void Thread::addThreadType()
{}

}  // namespace PartDesign
