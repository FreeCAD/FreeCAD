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

#include "Item.h"
#include "Part.h"
#include "Joint.h"
#include "SystemSolver.h"
#include "Time.h"

namespace MbD {
	class Part;
	class Joint;
	class SystemSolver;

	class System : public Item
	{
		//ToDo: Needed members admSystem namedItems mbdTime parts jointsMotions forcesTorques sensors variables hasChanged mbdSystemSolver
	public:
		static System& getInstance() {
			//https://medium.com/@caglayandokme/further-enhancing-the-singleton-pattern-in-c-8278b02b1ac7
			static System singleInstance; // Block-scoped static Singleton instance
			return singleInstance;
		};
		static System& getInstance(const char* str) {
			//https://medium.com/@caglayandokme/further-enhancing-the-singleton-pattern-in-c-8278b02b1ac7
			static System singleInstance(str); // Block-scoped static Singleton instance
			return singleInstance;
		};


		std::shared_ptr<std::vector<std::shared_ptr<Part>>> parts;
		std::shared_ptr<std::vector<std::shared_ptr<Joint>>> jointsMotions;
		bool hasChanged = false;
		std::shared_ptr<SystemSolver> systemSolver;
		void addPart(std::shared_ptr<Part> part);
		void runKINEMATICS();
		void initializeLocally() override;
		void initializeGlobally() override;

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
