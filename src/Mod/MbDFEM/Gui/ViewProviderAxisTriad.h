// SPDX-License-Identifier: LGPL-2.1-or-later

#pragma once

#include <Mod/MbDFEM/MbDFEMGlobal.h>

class SoSeparator;
class SoSwitch;

namespace MbDFEMGui
{

MbDFEMGuiExport SoSeparator* createAxisTriad();
MbDFEMGuiExport SoSeparator* createMassMarkerAxisTriad();
MbDFEMGuiExport void updateAxisTriadSwitch(SoSwitch* axisTriadSwitch, bool visible);

}  // namespace MbDFEMGui
