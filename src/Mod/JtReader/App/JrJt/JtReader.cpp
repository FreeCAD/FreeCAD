#include "JtReader.h"

#include <iostream>
#include <fstream>
#include <vector>

#include <stdint.h>

#include "TOC_Entry.h"
#include "GUID.h"
#include "UChar.h"
#include "I32.h"
#include "TOC_Entry.h"


using namespace std;


JtReader::JtReader()
{
}


JtReader::~JtReader()
{
}

void JtReader::setFile(const std::string fileName)
{
	this->_fileName = fileName;
}

const std::vector<TOC_Entry>& JtReader::readToc()
{
	ifstream strm;

	strm.open(_fileName, ios::binary);

	if (!strm)
		throw "cannot open file";


	Context cont(strm);

	char headerString[80];
	strm.read(headerString, 80);

	UChar Byte_Order(cont);
	I32 Empty_Field(cont);
	I32 TOC_Offset(cont);

	GUID LSG_Segment_ID(cont);


	cont.Strm.seekg((int32_t)TOC_Offset);
	
	I32 Entry_Count(cont);
	
    TocEntries.resize(Entry_Count);

	for (std::vector<TOC_Entry>::iterator i = TocEntries.begin(); i != TocEntries.end(); ++i)
		i->read(cont);

	return TocEntries;

}
