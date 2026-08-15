// SPDX-License-Identifier: LGPL-2.1-or-later
// Test-only helper. Nothing in shipped code uses this.

#pragma once

#include <string>
#include <vector>

#include <Gui/StyleParameters/Diagnostics.h>

namespace Gui::StyleParameters
{

/// Captures diagnostics for the lifetime of the object and clears dedup state on both ends.
class DiagnosticsCapture
{
public:
    DiagnosticsCapture()
    {
        Diagnostics::clear();
        subscription_ = Diagnostics::observe([this](const std::string& message) {
            messages_.push_back(message);
        });
    }

    ~DiagnosticsCapture()
    {
        Diagnostics::clear();
    }

    DiagnosticsCapture(const DiagnosticsCapture&) = delete;
    DiagnosticsCapture& operator=(const DiagnosticsCapture&) = delete;

    const std::vector<std::string>& messages() const
    {
        return messages_;
    }

private:
    std::vector<std::string> messages_;
    Diagnostics::Subscription subscription_;
};

}  // namespace Gui::StyleParameters
