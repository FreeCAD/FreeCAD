// SPDX-License-Identifier: LGPL-2.1-or-later

#include <iostream>

#include "FeatureThread.h"

namespace PartDesign
{

Thread::Thread()
{
    addThreadType();
}

void Thread::onChanged(const App::Property* prop)
{
    ProfileBased::onChanged(prop);
}

void Thread::addThreadType()
{}

}  // namespace PartDesign
