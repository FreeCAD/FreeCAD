// SPDX-License-Identifier: LGPL-2.1-or-later

#pragma once

#include "CommandLine.h"

#include <map>
#include <string>

namespace App
{

/** Apply parsed startup options to the application configuration map.
 *
 * This adapter intentionally preserves the established startup ordering: options which affect
 * the map are applied first, dump/get requests observe that map, and --set-config is applied only
 * afterward. Options with runtime effects, such as --python-path, are handled by Application.
 */
AppExport StartupResult applyStartupOptionsToConfig(
    const StartupOptions& options,
    std::map<std::string, std::string>& config
);

}  // namespace App
