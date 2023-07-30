#pragma once
#include <fstream>	

#include "ASMTSpatialContainer.h"
#include "ASMTRefPoint.h"
#include "ASMTRefCurve.h"
#include "ASMTRefSurface.h"
#include "ASMTPart.h"
#include "ASMTKinematicIJ.h"
#include "ASMTConstraintSet.h"
#include "ASMTForceTorque.h"
#include "ASMTConstantGravity.h"
#include "ASMTSimulationParameters.h"
#include "ASMTAnimationParameters.h"
#include "FullColumn.h"
#include "FullMatrix.h"
#include "ASMTJoint.h"
#include "ASMTMotion.h"
#include "Units.h"
#include "ASMTTime.h"
#include "SystemSolver.h"

namespace MbD {
    class ASMTAssembly : public ASMTSpatialContainer
    {
        //
    public:
        static void runFile(const char* chars);
        ASMTAssembly* root() override;
        void parseASMT(std::vector<std::string>& lines) override;
        void readNotes(std::vector<std::string>& lines);
        void readParts(std::vector<std::string>& lines);
        void readPart(std::vector<std::string>& lines);
        void readKinematicIJs(std::vector<std::string>& lines);
        void readKinematicIJ(std::vector<std::string>& lines);
        void readConstraintSets(std::vector<std::string>& lines);
        void readJoints(std::vector<std::string>& lines);
        void readMotions(std::vector<std::string>& lines);
        void readGeneralConstraintSets(std::vector<std::string>& lines);
        void readForcesTorques(std::vector<std::string>& lines);
        void readConstantGravity(std::vector<std::string>& lines);
        void readSimulationParameters(std::vector<std::string>& lines);
        void readAnimationParameters(std::vector<std::string>& lines);
        void readTimeSeries(std::vector<std::string>& lines);
        void readTimes(std::vector<std::string>& lines);
        void readAssemblySeries(std::vector<std::string>& lines);
        void readPartSeriesMany(std::vector<std::string>& lines);
        void readPartSeries(std::vector<std::string>& lines);
        void readJointSeriesMany(std::vector<std::string>& lines);
        void readJointSeries(std::vector<std::string>& lines);
        void readMotionSeriesMany(std::vector<std::string>& lines);
        void readMotionSeries(std::vector<std::string>& lines);

        void outputFor(AnalysisType type);
        void logString(std::string& str);
        void logString(double value);
        void preMbDrun(std::shared_ptr<System> mbdSys);
        void postMbDrun();
        void calcCharacteristicDimensions();
        double calcCharacteristicTime();
        double calcCharacteristicMass();
        double calcCharacteristicLength();
        std::shared_ptr<std::vector<std::shared_ptr<ASMTItemIJ>>> connectorList();
        std::shared_ptr<std::map<std::string, std::shared_ptr<ASMTMarker>>>markerMap();
        void deleteMbD();
        void createMbD(std::shared_ptr<System> mbdSys, std::shared_ptr<Units> mbdUnits) override;
        void runKINEMATIC();
        void initprincipalMassMarker();
        std::shared_ptr<ASMTSpatialContainer> spatialContainerAt(std::shared_ptr<ASMTAssembly> self, std::string& longname);
        std::shared_ptr<ASMTMarker> markerAt(std::string& longname);
        std::shared_ptr<ASMTJoint> jointAt(std::string& longname);
        std::shared_ptr<ASMTMotion> motionAt(std::string& longname);
        std::shared_ptr<ASMTForceTorque> forceTorqueAt(std::string& longname);
        FColDsptr vOcmO() override;
        FColDsptr omeOpO() override;
        std::shared_ptr<ASMTTime> geoTime();
        void updateFromMbD() override;
        void compareResults(AnalysisType type) override;

        std::string notes;
        std::shared_ptr<std::vector<std::shared_ptr<ASMTPart>>> parts;
        std::shared_ptr<std::vector<std::shared_ptr<ASMTKinematicIJ>>> kinematicIJs;
        std::shared_ptr<std::vector<std::shared_ptr<ASMTConstraintSet>>> constraintSets;
        std::shared_ptr<std::vector<std::shared_ptr<ASMTJoint>>> joints;
        std::shared_ptr<std::vector<std::shared_ptr<ASMTMotion>>> motions;
        std::shared_ptr<std::vector<std::shared_ptr<ASMTForceTorque>>> forcesTorques;
        std::shared_ptr<ASMTConstantGravity> constantGravity;
        std::shared_ptr<ASMTSimulationParameters> simulationParameters;
        std::shared_ptr<ASMTAnimationParameters> animationParameters;
        std::shared_ptr<std::vector<double>> times;
        std::shared_ptr<ASMTTime> asmtTime = std::make_shared<ASMTTime>();
        std::shared_ptr<Units> mbdUnits;

    };
}

