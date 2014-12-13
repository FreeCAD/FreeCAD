#pragma once

#include <string>
#include <vector>
#include "TOC_Entry.h"


class JtReader
{
public:
	JtReader();
	~JtReader();

	void setFile(const std::string fileName);

	const std::vector<TOC_Entry>& readToc();

	void readSegment(int tocIndex);

	


protected:
	std::string _fileName;
	vector<TOC_Entry> TocEntries;

};

