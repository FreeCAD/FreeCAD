#pragma once

#include <string>


class JtReader
{
public:
	JtReader();
	~JtReader();

	void setFile(const std::string fileName);

	void readToc();


protected:
	std::string _fileName;

};

