#pragma once
#include <string>
namespace MbD {
	class Item
	{
	public:
		Item() {}
		void setName(std::string& str);
		const std::string& getName() const;
	private:
		std::string name;
	};
}

