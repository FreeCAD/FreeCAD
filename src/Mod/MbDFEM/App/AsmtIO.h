// SPDX-License-Identifier: LGPL-2.1-or-later

#pragma once

#include <string>
#include <vector>

#include <Mod/MbDFEM/MbDFEMGlobal.h>

namespace App
{
class DocumentObject;
}

namespace MbDFEM
{

class MbDAssembly;

MbDFEMExport std::string exportAssemblyAsmt(MbDAssembly* assembly, const std::string& filename);
MbDFEMExport std::vector<App::DocumentObject*> importSolvedAsmt(MbDAssembly* assembly,
                                                                const std::string& filename);

}  // namespace MbDFEM
