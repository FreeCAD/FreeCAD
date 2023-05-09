#pragma once
#include <string>

namespace MbD {
	class Item
	{
		//name
	public:
		Item();
		Item(const char* str);
		virtual void initialize();
		virtual void initializeLocally();
		virtual void initializeGlobally();
		void setName(std::string& str);
		const std::string& getName() const;

	private:
		std::string name;
	};
}

