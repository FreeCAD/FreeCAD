#include "PreCompiled.h"

#include "Parameter.h"
#include "ParameterRef.h"

using namespace GCS;

Parameter::Parameter(double value, double scale, bool fixed, int tag){
    this->savedValue = value;
    this->scale = scale;
    this->fixed = fixed;
    this->tag = tag;
}

Parameter::Parameter(const std::string& label, double value, double scale, bool fixed, int tag)
{
    this->savedValue = value;
    this->scale = scale;
    this->fixed = fixed;
    this->tag = tag;
    this->label = label;
}

void Parameter::pasteFrom(const Parameter& from)
{
    //copy everything but indexes
    int ownIndex = _ownIndex;
    int masterIndex = _masterIndex;
    *this = from;
    _ownIndex = ownIndex;
    _masterIndex = masterIndex;
}

void Parameter::pasteFrom(const ParameterRef from)
{
    pasteFrom(from.param());
    savedValue = from.savedValue();//make sure to obey redirects
}
