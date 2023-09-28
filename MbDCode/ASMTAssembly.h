/***************************************************************************
 *   Copyright (c) 2023 Ondsel, Inc.                                       *
 *                                                                         *
 *   This file is part of OndselSolver.                                    *
 *                                                                         *
 *   See LICENSE file for details about copyright.                         *
 ***************************************************************************/

#pragma once
#include <fstream>	

#include "ASMTSpatialContainer.h"
#include "FullColumn.h"
#include "FullMatrix.h"

namespace MbD {
	class ASMTRefPoint;
	class ASMTRefCurve;
	class ASMTRefSurface;
	class ASMTPart;
	class ASMTKinematicIJ;
	class ASMTConstraintSet;
	class ASMTForceTorque;
	class ASMTConstantGravity;
	class ASMTSimulationParameters;
	class ASMTAnimationParameters;
	class ASMTJoint;
	class ASMTMotion;
	class Units;
	class ASMTTime;
	class SystemSolver;
	class ASMTItemIJ;

	class ASMTAssembly : public ASMTSpatialContainer
	{
		//
	public:
		static void runSinglePendulum();
		static void runFile(const char* chars);
		void initialize() override;
		ASMTAssembly* root() override;
		void setNotes(std::string& str);
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
		void outputResults(AnalysisType type) override;
		void addPart(std::shared_ptr<ASMTPart> part);
		void addJoint(std::shared_ptr<ASMTJoint> joint);
		void addMotion(std::shared_ptr<ASMTMotion> motion);
		void setConstantGravity(std::shared_ptr<ASMTConstantGravity> constantGravity);
		void setSimulationParameters(std::shared_ptr<ASMTSimulationParameters> simulationParameters);

		std::string notes;
		std::shared_ptr<std::vector<std::shared_ptr<ASMTPart>>> parts = std::make_shared<std::vector<std::shared_ptr<ASMTPart>>>();
		std::shared_ptr<std::vector<std::shared_ptr<ASMTKinematicIJ>>> kinematicIJs = std::make_shared<std::vector<std::shared_ptr<ASMTKinematicIJ>>>();
		std::shared_ptr<std::vector<std::shared_ptr<ASMTConstraintSet>>> constraintSets = std::make_shared<std::vector<std::shared_ptr<ASMTConstraintSet>>>();
		std::shared_ptr<std::vector<std::shared_ptr<ASMTJoint>>> joints = std::make_shared<std::vector<std::shared_ptr<ASMTJoint>>>();
		std::shared_ptr<std::vector<std::shared_ptr<ASMTMotion>>> motions = std::make_shared<std::vector<std::shared_ptr<ASMTMotion>>>();
		std::shared_ptr<std::vector<std::shared_ptr<ASMTForceTorque>>> forcesTorques = std::make_shared<std::vector<std::shared_ptr<ASMTForceTorque>>>();
		std::shared_ptr<ASMTConstantGravity> constantGravity;
		std::shared_ptr<ASMTSimulationParameters> simulationParameters;
		std::shared_ptr<ASMTAnimationParameters> animationParameters;
		std::shared_ptr<std::vector<double>> times;
		std::shared_ptr<ASMTTime> asmtTime = std::make_shared<ASMTTime>();
		std::shared_ptr<Units> mbdUnits;

	};
}

