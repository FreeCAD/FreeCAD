#pragma once

#include <string>
#include <vector>
#include "TOC_Entry.h"

class LodHandler;



class JtReader
{
public:
	JtReader();
	~JtReader();

	void setFile(const std::string fileName);

	const std::vector<TOC_Entry>& readToc();

	void readSegment(int tocIndex);

	void readLodSegment(const TOC_Entry&, LodHandler&);

	static const GUID TriStripSetShapeLODElement_ID;


protected:
	std::string _fileName;
	vector<TOC_Entry> TocEntries;

};

