#include "JtReader.h"

#include <assert.h>
#include <fstream>
#include <iostream>
#include <vector>

#include "Element_Header.h"
#include "GUID.h"
#include "I32.h"
#include "LodHandler.h"
#include "Segment_Header.h"
#include "TOC_Entry.h"
#include "UChar.h"


using namespace std;


const GUID JtReader::TriStripSetShapeLODElement_ID(0x10dd10ab,
                                                   0x2ac8,
                                                   0x11d1,
                                                   0x9b,
                                                   0x6b,
                                                   0x0,
                                                   0x80,
                                                   0xc7,
                                                   0xbb,
                                                   0x59,
                                                   0x97);

JtReader::JtReader()
{}


JtReader::~JtReader()
{}

void JtReader::setFile(const std::string fileName)
{
    this->_fileName = fileName;
}

const std::vector<TOC_Entry>& JtReader::readToc()
{
    ifstream strm;

    strm.open(_fileName, ios::binary);

    if (!strm) {
        throw "cannot open file";
    }


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

    for (std::vector<TOC_Entry>::iterator i = TocEntries.begin(); i != TocEntries.end(); ++i) {
        i->read(cont);
    }

    return TocEntries;
}

void JtReader::readLodSegment(const TOC_Entry& toc, LodHandler& /*handler*/)
{
    std::ifstream strm;
    strm.open(_fileName, ios::binary);

    if (!strm) {
        throw "cannot open file";
    }

    strm.seekg((int32_t)toc.Segment_Offset);

    Context cont(strm);


    // check if called with the right Toc
    assert(toc.getSegmentType() == 7);

    // read the segment header
    Segment_Header header(cont);

    // read the non zip Element header
    Element_Header eHeader(cont, false);

    if (eHeader.Object_Type_ID != TriStripSetShapeLODElement_ID) {
        return;
    }
}
