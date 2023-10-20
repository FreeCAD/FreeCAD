/***************************************************************************
 *   Copyright (c) 2023 Ondsel, Inc.                                       *
 *                                                                         *
 *   This file is part of OndselSolver.                                    *
 *                                                                         *
 *   See LICENSE file for details about copyright.                         *
 ***************************************************************************/
 
#pragma once

#include "ASMTSpatialItem.h"
//#include "ASMTRefPoint.h"
//#include "ASMTRefCurve.h"
//#include "ASMTRefSurface.h"
#include "ASMTPrincipalMassMarker.h"
//#include "Units.h"
//#include "ASMTPart.h"
//#include "ASMTJoint.h"
//#include "ASMTMotion.h"

namespace MbD {
    class ASMTRefPoint;
    class ASMTRefCurve;
    class ASMTRefSurface;
    class ASMTPrincipalMassMarker;
    class Units;
    class ASMTPart;
    class ASMTJoint;
    class ASMTMotion;
    class ASMTMarker;

    class EXPORT ASMTSpatialContainer : public ASMTSpatialItem
    {
        //
    public:
        ASMTSpatialContainer();
        void initialize() override;
        void setPrincipalMassMarker(std::shared_ptr<ASMTPrincipalMassMarker> aJ);
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
        ASMTSpatialContainer* partOrAssembly() override;
        void updateFromMbD() override;
        void compareResults(AnalysisType type) override;
        void outputResults(AnalysisType type) override;
        void addRefPoint(std::shared_ptr<ASMTRefPoint> refPoint);
        void addMarker(std::shared_ptr<ASMTMarker> marker);
        std::string generateUniqueMarkerName();
        std::shared_ptr<std::vector<std::shared_ptr<ASMTMarker>>> markerList();
        void setVelocity3D(FColDsptr velocity3D);
        void setOmega3D(FColDsptr omega3D);
        void readVelocity3D(std::vector<std::string>& lines);
        void readOmega3D(std::vector<std::string>& lines);
        void setVelocity3D(double a, double b, double c);
        void setOmega3D(double a, double b, double c);
        void storeOnLevel(std::ofstream& os, int level) override;
        void storeOnLevelVelocity(std::ofstream& os, int level);
        void storeOnLevelOmega(std::ofstream& os, int level);
        void storeOnLevelRefPoints(std::ofstream& os, int level);
        void storeOnLevelRefCurves(std::ofstream& os, int level);
        void storeOnLevelRefSurfaces(std::ofstream& os, int level);
        void storeOnTimeSeries(std::ofstream& os) override;

        FColDsptr velocity3D = std::make_shared<FullColumn<double>>(3);
        FColDsptr omega3D = std::make_shared<FullColumn<double>>(3);
        std::shared_ptr<std::vector<std::shared_ptr<ASMTRefPoint>>> refPoints;
        std::shared_ptr<std::vector<std::shared_ptr<ASMTRefCurve>>> refCurves;
        std::shared_ptr<std::vector<std::shared_ptr<ASMTRefSurface>>> refSurfaces;
        FRowDsptr xs, ys, zs, bryxs, bryys, bryzs;
        FRowDsptr vxs, vys, vzs, omexs, omeys, omezs;
        FRowDsptr axs, ays, azs, alpxs, alpys, alpzs;
        FRowDsptr inxs, inys, inzs, inbryxs, inbryys, inbryzs;
        FRowDsptr invxs, invys, invzs, inomexs, inomeys, inomezs;
        FRowDsptr inaxs, inays, inazs, inalpxs, inalpys, inalpzs;
        std::shared_ptr<ASMTPrincipalMassMarker> principalMassMarker = std::make_shared<ASMTPrincipalMassMarker>();

    };
}

