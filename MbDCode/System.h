/*****************************************************************//**
 * \file   System.h
 * \brief  Multibody system of parts, joints, forces.
 * 
 * \author askoh
 * \date   May 2023
 *********************************************************************/

#pragma once
#include <memory>
#include <vector>
#include <functional>

#include "Item.h"
//#include "Time.h"

namespace MbD {
	class Part;
	class Joint;
	class SystemSolver;
	class Time;

	class System : public Item
	{
		//ToDo: Needed members admSystem namedItems mbdTime parts jointsMotions forcesTorques sensors variables hasChanged mbdSystemSolver
	public:
		static System& getInstance() {
			//https://medium.com/@caglayandokme/further-enhancing-the-singleton-pattern-in-c-8278b02b1ac7
			static System singleInstance; // Block-scoped static Singleton instance
			return singleInstance;
		};

		void initialize() override;
		void initializeLocally() override;
		void initializeGlobally() override;
		void runKINEMATICS();
		void outputInput();
		void outputInitialConditions();
		void outputTimeSeries();
		std::shared_ptr<std::vector<std::string>> discontinuitiesAtIC();
		void partsJointsMotionsDo(const std::function <void(std::shared_ptr<Item>)>& f);
		void logString(std::string& str);
		double mbdTimeValue();


		std::shared_ptr<std::vector<std::shared_ptr<Part>>> parts;
		std::shared_ptr<std::vector<std::shared_ptr<Joint>>> jointsMotions;
		bool hasChanged = false;
		std::shared_ptr<SystemSolver> systemSolver;
		void addPart(std::shared_ptr<Part> part);

		std::shared_ptr<Time> time;
	private:
		System();
		System(const char* str);
		//System() = default; // Private constructor
		System(const System&) = delete;
		System& operator=(const System&) = delete; // Deleted copy assignment
		~System() = default; // Private destructor
	};
}
