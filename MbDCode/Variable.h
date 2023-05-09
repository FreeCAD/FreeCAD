#pragma once
#include <string>

#include "Symbolic.h"
namespace MbD {
    class Variable : public Symbolic
    {
        //name value 
	public:
		Variable();
		Variable(const char* str);
		Variable(double val);
		void initialize();
		void setName(std::string& str);
		const std::string& getName() const;

		std::string name;
		double value;
	};
}

