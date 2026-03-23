// SPDX-License-Identifier: LGPL-2.1-or-later

#pragma once

#include "DocumentObject.h"

#include <string>

namespace App
{

/// Struct that holds information about the last
/// export so that the user does not have to reenter
/// them on each export. The export command either uses
/// the target document's export info
struct ExportInfo {
    std::string location {};
    std::string filename {};
    std::string filter {};
    bool generatedName {false};
    App::DocumentObject* object {nullptr};
};

}
