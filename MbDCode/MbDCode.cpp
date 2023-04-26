#include <iostream>
#include "Item.h"
#include "System.h"

using namespace MbD;

int main()
{
	std::cout << "Hello World!\n";
	auto& TheSystem = System::getInstance();
	std::string str = "TheSystem";
	TheSystem.setName(str);
	std::cout << TheSystem.getName();
	//auto fixedPart = std::make_shared<Part>();
	//str = "FixedPart";
	//fixedPart->setName(str);
}