// SPDX-License-Identifier: LGPL-2.1-or-later

#pragma once

#include <Mod/Assembly/AssemblyGlobal.h>

namespace App
{
class Document;
}

namespace AssemblyGui
{

class AssemblyGuiExport LightweightWorkspaceProxyViewOptimizer
{
public:
    static void init();
    static void refreshDocument(const App::Document& doc);
    static void refreshAllOpenDocuments();
    static void optimizeActiveView();
};

}  // namespace AssemblyGui
