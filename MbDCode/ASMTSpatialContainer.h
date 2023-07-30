#pragma once

#include "ASMTSpatialItem.h"
#include "ASMTRefPoint.h"
#include "ASMTRefCurve.h"
#include "ASMTRefSurface.h"
#include "ASMTPrincipalMassMarker.h"
#include "Units.h"

namespace MbD {
    class ASMTSpatialContainer : public ASMTSpatialItem
    {
        //
    public:
        void initialize() override;
        void readRefPoints(std::vector<std::string>& lines);
        void readRefPoint(std::vector<std::string>& lines);
        void readRefCurves(std::vector<std::string>& lines);
        void readRefCurve(std::vector<std::string>& lines);
        void readRefSurfaces(std::vector<std::string>& lines);
        void readRefSurface(std::vector<std::string>& lines);
        void readXs(std::vector<std::string>& lines);
        void readYs(std::vector<std::string>& lines);
        void readZs(std::vector<std::string>& lines);
        void readBryantxs(std::vector<std::string>& lines);
        void readBryantys(std::vector<std::string>& lines);
        void readBryantzs(std::vector<std::string>& lines);
        void readVXs(std::vector<std::string>& lines);
        void readVYs(std::vector<std::string>& lines);
        void readVZs(std::vector<std::string>& lines);
        void readOmegaXs(std::vector<std::string>& lines);
        void readOmegaYs(std::vector<std::string>& lines);
        void readOmegaZs(std::vector<std::string>& lines);
        void readAXs(std::vector<std::string>& lines);
        void readAYs(std::vector<std::string>& lines);
        void readAZs(std::vector<std::string>& lines);
        void readAlphaXs(std::vector<std::string>& lines);
        void readAlphaYs(std::vector<std::string>& lines);
        void readAlphaZs(std::vector<std::string>& lines);
        void createMbD(std::shared_ptr<System> mbdSys, std::shared_ptr<Units> mbdUnits) override;
        FColDsptr rOcmO();
        std::shared_ptr<EulerParameters<double>> qEp();
        virtual FColDsptr vOcmO();
        virtual FColDsptr omeOpO();
        std::shared_ptr<ASMTSpatialContainer> part() override;
        void updateFromMbD() override;
        void compareResults(AnalysisType type) override;

        std::shared_ptr<std::vector<std::shared_ptr<ASMTRefPoint>>> refPoints;
        std::shared_ptr<std::vector<std::shared_ptr<ASMTRefCurve>>> refCurves;
        std::shared_ptr<std::vector<std::shared_ptr<ASMTRefSurface>>> refSurfaces;
        FRowDsptr xs, ys, zs, bryxs, bryys, bryzs;
        FRowDsptr vxs, vys, vzs, omexs, omeys, omezs;
        FRowDsptr axs, ays, azs, alpxs, alpys, alpzs;
        FRowDsptr inxs, inys, inzs, inbryxs, inbryys, inbryzs;
        FRowDsptr invxs, invys, invzs, inomexs, inomeys, inomezs;
        FRowDsptr inaxs, inays, inazs, inalpxs, inalpys, inalpzs;
        std::shared_ptr<ASMTPrincipalMassMarker> principalMassMarker;

    };
}

