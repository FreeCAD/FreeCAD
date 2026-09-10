// SPDX-License-Identifier: LGPL-2.1-or-later

#include "FeatureRib.h"

namespace PartDesign
{

PROPERTY_SOURCE(PartDesign::Rib, PartDesign::ProfileBased)

Rib::Rib()
{
    defineAdditive();
    // Register Rib-specific input properties here as they are implemented.
    // Profile and recompute tracking are already provided by ProfileBased.
}

App::DocumentObjectExecReturn* Rib::execute()
{
    // Implement construction here, then publish AddSubShape and Shape.
    // Until then, do not report a successful feature with no geometry.
    return new App::DocumentObjectExecReturn("Rib construction is not implemented yet");
}

}  // namespace PartDesign
