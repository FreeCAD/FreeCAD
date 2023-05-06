#pragma once
#include <string>

#include "Symbolic.h"
namespace MbD {
    class Variable :
        public Symbolic
    {
        //name value 
    public:
	public:
		Variable() {}
		void setName(std::string& str);
		const std::string& getName() const;
	private:
		std::string name;
	};
}

