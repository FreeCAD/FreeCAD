// SPDX-License-Identifier: LGPL-2.1-or-later

#include "TransientPlacementTargetProvider.h"

#include <algorithm>

using namespace Gui;

TransientPlacementTargetRegistry& TransientPlacementTargetRegistry::instance()
{
    static TransientPlacementTargetRegistry inst;
    return inst;
}

void TransientPlacementTargetRegistry::add(TransientPlacementTargetProvider* provider)
{
    providers_.push_back(provider);
}

void TransientPlacementTargetRegistry::remove(TransientPlacementTargetProvider* provider)
{
    providers_.erase(std::remove(providers_.begin(), providers_.end(), provider), providers_.end());
}

const std::vector<TransientPlacementTargetProvider*>& TransientPlacementTargetRegistry::all() const
{
    return providers_;
}
