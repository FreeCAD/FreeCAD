#pragma once
#include <memory>
#include <vector>

#include "Item.h"
#include "Part.h"
#include "Joint.h"
#include "SystemSolver.h"

namespace MbD {
	class Part;
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
		std::vector<std::shared_ptr<Part>> parts;
		std::vector<std::shared_ptr<Joint>> jointsMotions;
		bool hasChanged = false;
		std::shared_ptr<SystemSolver> systemSolver;
		void addPart(std::shared_ptr<Part> part);
	private:
		System();
		//System() = default; // Private constructor
		System(const System&) = delete;
		System& operator=(const System&) = delete; // Deleted copy assignment
		~System() = default; // Private destructor
	};
}
