#pragma once

#include "ASMTItem.h"

namespace MbD {
    class ASMTPrincipalMassMarker : public ASMTItem
    {
        //
    public:
        void parseASMT(std::vector<std::string>& lines) override;

		FColDsptr position3D;
		FMatDsptr rotationMatrix;
		double mass, density;
        DiagMatDsptr momentOfInertias;

    };
}

