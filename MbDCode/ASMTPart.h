#pragma once

#include "ASMTItem.h"
#include "ASMTRefPoint.h"
#include "ASMTRefCurve.h"
#include "ASMTRefSurface.h"
#include "ASMTPrincipalMassMarker.h"

namespace MbD {
    class ASMTPart : public ASMTItem
    {
        //
    public:
        void parseASMT(std::vector<std::string>& lines) override;

        std::string name;
        FColDsptr position3D, velocity3D, omega3D;
        FMatDsptr rotationMatrix;
        std::shared_ptr<ASMTPrincipalMassMarker> principalMassMarker;
        //std::shared_ptr<std::vector<std::shared_ptr<ASMTFeature>>> featureOrder;
        std::shared_ptr<std::vector<std::shared_ptr<ASMTRefPoint>>> refPoints;
        std::shared_ptr<std::vector<std::shared_ptr<ASMTRefCurve>>> refCurves;
        std::shared_ptr<std::vector<std::shared_ptr<ASMTRefSurface>>> refSurfaces;

    };
}

