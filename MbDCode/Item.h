#pragma once
#include <string>

namespace MbD {
	class Item
	{
		//name
	public:
		Item() {}
		Item(const char* str) : name(str) {}
		void setName(std::string& str);
		const std::string& getName() const;
	private:
		std::string name;
	};
}

