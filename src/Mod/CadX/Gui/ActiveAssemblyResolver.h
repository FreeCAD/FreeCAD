// SPDX-License-Identifier: LGPL-2.1-or-later

#pragma once

#include <string>

namespace CadX
{

struct ActiveAssemblyContext
{
    std::string documentUid;
    std::string documentName;
    std::string assemblyObjectName;
    std::string assemblyLabel;
    std::string activeViewId;
    bool editModeProof = false;
    bool activeViewProof = false;
};

struct ActiveAssemblyResolution
{
    bool ok = false;
    std::string errorCode;
    std::string diagnostic;
    ActiveAssemblyContext context;
};

class ActiveAssemblyResolver
{
public:
    ActiveAssemblyResolution resolve() const;
};

}  // namespace CadX
